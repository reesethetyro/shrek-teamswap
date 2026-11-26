using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.Drawing;

internal static class InstallerTests
{
    private static int checks;
    private static string root;
    private static readonly JavaScriptSerializer Json = new JavaScriptSerializer();
    private static void Check(bool value, string message)
    {
        if (!value) throw new Exception(message);
        checks++;
    }
    private static string Dir(string name)
    {
        string p = Path.Combine(root, name); Directory.CreateDirectory(p); return p;
    }
    private static void Write(string path, string text)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)); File.WriteAllText(path, text);
    }
    private static string Game(string name, bool data)
    {
        string folder = Dir(name); File.WriteAllBytes(Path.Combine(folder, "Shrek.exe"), new byte[] { 0 });
        if (data) Directory.CreateDirectory(Path.Combine(folder, "gamedata"));
        return folder;
    }
    private static string Item(string directory, string name, string folder, bool incomplete)
    {
        string path = Path.Combine(directory, name + ".item");
        Write(path, Json.Serialize(new Dictionary<string, object> {
            { "InstallLocation", folder }, { "DisplayName", "DreamWorks Shrek 2 Remastered" },
            { "LaunchExecutable", "Shrek.exe" }, { "bIsIncompleteInstall", incomplete }
        })); return path;
    }
    private static List<Installer.GameInstallation> Find(string steam, string epic)
    {
        return Installer.FindGames(steam == null ? new string[0] : new[] { steam },
                                   epic == null ? new string[0] : new[] { epic });
    }
    private static void CopyTree(string source, string destination)
    {
        Directory.CreateDirectory(destination);
        foreach (string file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
        {
            string relative = file.Substring(source.Length).TrimStart('\\', '/');
            string target = Path.Combine(destination, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(target)); File.Copy(file, target);
        }
    }
    private static Dictionary<string, string> Snapshot(string folder)
    {
        var hashes = new Dictionary<string, string>();
        foreach (string file in Directory.GetFiles(folder, "*", SearchOption.AllDirectories))
            hashes.Add(file.Substring(folder.Length), Installer.Hash(File.ReadAllBytes(file)));
        return hashes;
    }
    private static void Same(Dictionary<string, string> expected, string folder)
    {
        var actual = Snapshot(folder); Check(actual.Count == expected.Count, "File count changed unexpectedly");
        foreach (var entry in expected)
            Check(actual.ContainsKey(entry.Key) && actual[entry.Key] == entry.Value, "Unexpected file change: " + entry.Key);
    }

    [STAThread]
    private static int Main(string[] args)
    {
        try
        {
            root = Path.GetFullPath(args[0]);
            string fixture = Path.GetFullPath(args[1]); string patch = Path.GetFullPath(args[2]);
            if (Directory.Exists(root)) throw new Exception("Use a fresh test output directory.");
            Directory.CreateDirectory(root);
            Check(Find(null, null).Count == 0, "Empty discovery");
            Check(Find(Path.Combine(root, "absent-steam"), Path.Combine(root, "absent-epic")).Count == 0, "Absent launchers");
            string steam = Dir("Steam Root");
            string steamGame = Game(Path.Combine("Steam Root", "steamapps", "common", "Shrek Game"), true);
            Write(Path.Combine(steam, "steamapps", "appmanifest_2523740.acf"), "\"AppState\" { \"installdir\" \"Shrek Game\" }");
            var found = Find(steam, null);
            Check(found.Count == 1 && found[0].Folder == steamGame && found[0].Store == "Steam", "Steam root discovery");
            string library = Dir("Extra Steam Library");
            string libraryGame = Game(Path.Combine("Extra Steam Library", "steamapps", "common", "Other Shrek"), true);
            Write(Path.Combine(library, "steamapps", "appmanifest_2523740.acf"), "\"installdir\" \"Other Shrek\"");
            Write(Path.Combine(steam, "steamapps", "libraryfolders.vdf"), "\"libraryfolders\" { \"0\" { \"path\" " + Json.Serialize(steam) + " } \"1\" { \"path\" " + Json.Serialize(library) + " } }");
            found = Find(steam, null);
            Check(found.Count == 2 && found[1].Folder == libraryGame, "Extra Steam library and deduplication");

            string epic = Dir("Epic Manifests");
            string epicGame = Game("Epic custom library's Shrek caf\u00e9", true);
            string good = Item(epic, "20-good", epicGame, false);
            Write(good, File.ReadAllText(good).Replace("\u00e9", "\\u00e9"));
            Write(Path.Combine(epic, "00-corrupt.item"), "{broken");
            Write(Path.Combine(epic, "01-array.item"), "[]");
            Write(Path.Combine(epic, "02-null.item"), "{\"InstallLocation\":null}");
            Write(Path.Combine(epic, "03-bool.item"), "{\"InstallLocation\":true}");
            Write(Path.Combine(epic, "04-relative.item"), "{\"InstallLocation\":\"relative\"}");
            Write(Path.Combine(epic, "05-invalid-path.item"), Json.Serialize(new { InstallLocation = "C:\\bad:path" }));
            Item(epic, "06-missing", Path.Combine(root, "no-game"), false);
            Item(epic, "07-incomplete", Game("Incomplete download", true), true);
            Item(epic, "08-no-data", Game("Wrong folder", false), false);
            Item(epic, "21-duplicate", epicGame.ToUpperInvariant() + Path.DirectorySeparatorChar, false);
            found = Find(null, epic);
            Check(found.Count == 1 && found[0].Folder == epicGame && found[0].Store == "Epic Games", "Epic JSON, custom path, Unicode, stale/corrupt manifests");
            using (var locked = new FileStream(Path.Combine(epic, "00-corrupt.item"), FileMode.Open, FileAccess.ReadWrite, FileShare.None))
                Check(Find(null, epic).Count == 1, "Locked Epic record does not block a valid record");
            found = Find(steam, epic);
            Check(found.Count == 3 && found[2].Folder == epicGame, "Both stores enumerated");
            Check(Find("C:\\bad:path", epic).Count == 1, "Invalid Steam root does not block Epic");
            Item(epic, "22-cross-store-duplicate", steamGame, false);
            Check(Find(steam, epic).Count == 3, "Cross-store folder deduplication");

            Application.EnableVisualStyles();
            using (var window = new InstallerWindow(found))
            {
                var combo = (ComboBox)typeof(InstallerWindow).GetField("location", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(window);
                var status = (Label)typeof(InstallerWindow).GetField("status", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(window);
                Check(combo.Items.Count == 3 && combo.Text == steamGame && status.Text.StartsWith("Steam installation"), "Startup folder/store label");
                combo.SelectedIndex = 2;
                Check(combo.Text == epicGame && status.Text.StartsWith("Epic Games installation"), "Epic selectable without browsing");
                combo.Text = Path.Combine(root, "manual");
                Check(status.Text.StartsWith("Folder selected"), "Manual folder is not mislabeled as Steam");
                window.PerformLayout();
                foreach (Control control in window.Controls)
                    Check(window.ClientRectangle.Contains(control.Bounds), "Control clipped outside installer: " + control.Text);
                foreach (Control control in window.Controls)
                    if (control is Label && !control.AutoSize)
                    {
                        Size preferred = TextRenderer.MeasureText(control.Text, control.Font, control.Size, TextFormatFlags.WordBreak);
                        Check(preferred.Height <= control.Height, "Installer text does not fit: " + control.Text);
                    }
            }
            using (var empty = new InstallerWindow(new Installer.GameInstallation[0]))
            {
                var combo = (ComboBox)typeof(InstallerWindow).GetField("location", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(empty);
                Check(combo.Items.Count == 0 && combo.Text == "", "No installation fallback remains browsable");
            }

            // Real patch round-trip in a COPY of the known clean fixture.
            // Naming it Epic tests store-independent file checks, not an actual Epic build.
            string epicClone = Path.Combine(root, "Epic compatible-files fixture");
            CopyTree(fixture, epicClone);
            Write(Path.Combine(epicClone, "gamedata", "save-test.txt"), "save data must remain unchanged");
            var vanilla = Snapshot(epicClone);
            Check(Installer.Change(epicClone, patch, true).StartsWith("Team Swap installed"), "Install matching files in Epic-style folder");
            Check(Installer.Change(epicClone, patch, true).Contains("already installed"), "Install idempotence");
            Check(Installer.Change(epicClone, patch, false).StartsWith("Team Swap uninstalled"), "Uninstall matching files");
            Same(vanilla, epicClone);
            Check(!Directory.Exists(Path.Combine(epicClone, ".teamswap-transaction")), "Transaction removed after round-trip");
            Check(Installer.Change(epicClone, patch, false).Contains("already uninstalled"), "Uninstall idempotence");
            byte[] different = File.ReadAllBytes(Path.Combine(epicClone, "Shrek.exe")); different[different.Length - 1] ^= 1;
            File.WriteAllBytes(Path.Combine(epicClone, "Shrek.exe"), different);
            var unsupported = Snapshot(epicClone); bool rejected = false;
            try { Installer.Change(epicClone, patch, true); }
            catch (InvalidDataException error)
            {
                rejected = error.Message.Contains("Unsupported or modified game file: Shrek.exe") && error.Message.Contains("different Epic build");
            }
            Check(rejected, "Different build rejected with explicit Epic compatibility explanation");
            Same(unsupported, epicClone);
            Check(!Directory.Exists(Path.Combine(epicClone, ".teamswap-transaction")), "No transaction or writes for unsupported build");

            string report = "PASS " + checks + " checks: launcher fixtures, selectable Steam/Epic folders, UI bounds, real patch round-trip, unchanged saves, unsupported-file rejection. Actual Steam or Epic file round-trip; live gameplay remains separate.";
            Write(Path.Combine(root, "results.txt"), report); Console.WriteLine(report); return 0;
        }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}
