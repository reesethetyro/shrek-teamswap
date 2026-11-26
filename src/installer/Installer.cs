using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using System.Web.Script.Serialization;
using Microsoft.Win32;

internal static class Installer
{
    private const string PatchHash = "d72ac3379e85ac3526698203fb0d4aa50e472f1cc514fddce282ee81c994dbe3";

    [STAThread]
    private static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new InstallerWindow());
    }

    internal static string Hash(byte[] bytes)
    {
        using (var sha = SHA256.Create())
            return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
    }

    private sealed class PatchSet
    {
        internal string Name;
        internal List<PatchEntry> Entries;
    }

    private static List<PatchSet> ReadPatch(string file)
    {
        byte[] data = File.ReadAllBytes(file);
        if (Hash(data) != PatchHash)
            throw new InvalidDataException("TeamSwap.patch does not match this installer. Extract both files again.");
        using (var stream = new GZipStream(new MemoryStream(data), CompressionMode.Decompress))
        using (var reader = new BinaryReader(stream))
        {
            if (Encoding.ASCII.GetString(reader.ReadBytes(4)) != "TSW2")
                throw new InvalidDataException("Unrecognized patch data.");
            int count = reader.ReadInt32();
            var sets = new List<PatchSet>(count);
            for (int i = 0; i < count; i++)
            {
                string name = Encoding.ASCII.GetString(reader.ReadBytes(reader.ReadInt32()));
                int files = reader.ReadInt32();
                var entries = new List<PatchEntry>(files);
                for (int j = 0; j < files; j++) entries.Add(new PatchEntry(reader));
                sets.Add(new PatchSet { Name = name, Entries = entries });
            }
            return sets;
        }
    }

    private static List<PatchEntry> SelectPatch(string game, List<PatchSet> sets)
    {
        byte[] exe = File.ReadAllBytes(Path.Combine(game, "Shrek.exe"));
        foreach (var set in sets)
            foreach (var entry in set.Entries)
                if (entry.Path == "Shrek.exe" && (Matches(exe, entry.VanillaHash) || Matches(exe, entry.ModHash)))
                    return set.Entries;
        throw new InvalidDataException("Unsupported or modified game file: Shrek.exe" +
            Environment.NewLine + "Supported builds: Steam 26746348 / 26880364 (CL593174) and Epic CL588802 / CL593174. A different Epic build needs a matching patch." +
            Environment.NewLine + "No files from this request were changed.");
    }

    internal static string Change(string game, string patchFile, bool install)
    {
        game = Path.GetFullPath(game.Trim().Trim('"'));
        if (!File.Exists(Path.Combine(game, "Shrek.exe")) || !Directory.Exists(Path.Combine(game, "gamedata")))
            throw new DirectoryNotFoundException("Choose the game folder that contains Shrek.exe and gamedata.");
        if (!File.Exists(patchFile))
            throw new FileNotFoundException("TeamSwap.patch is missing. Extract the entire download into one folder.");
        var sets = ReadPatch(patchFile);
        using (var mutex = new Mutex(false, "Shrek2RemasteredTeamSwapInstaller"))
        {
            bool locked;
            try { locked = mutex.WaitOne(0); }
            catch (AbandonedMutexException) { locked = true; }
            if (!locked)
                throw new InvalidOperationException("Another Team Swap installer is already working.");
            try
            {
                RequireGameClosed(game);
                return Apply(game, SelectPatch(game, sets), install);
            }
            finally { mutex.ReleaseMutex(); }
        }
    }

    private static string Apply(string game, List<PatchEntry> entries, bool install)
    {
        string transaction = Path.Combine(game, ".teamswap-transaction");
        if (Directory.Exists(transaction))
            Recover(game, transaction, entries);
        var changes = new Dictionary<int, byte[]>();
        for (int index = 0; index < entries.Count; index++)
        {
            PatchEntry entry = entries[index];
            string file = Path.Combine(game, entry.Path);
            if (!File.Exists(file))
                throw new FileNotFoundException("Missing game file: " + entry.Path);
            byte[] existing = File.ReadAllBytes(file);
            byte[] expected = install ? entry.VanillaHash : entry.ModHash;
            byte[] desired = install ? entry.ModHash : entry.VanillaHash;
            if (Matches(existing, desired))
                continue;
            if (!Matches(existing, expected))
                throw new InvalidDataException("Unsupported or modified game file: " + entry.Path +
                    Environment.NewLine + "This patch requires matching Steam 26746348 / 26880364 or Epic CL588802 / CL593174 files." +
                    Environment.NewLine + "The game may have updated or another mod may have changed this file." +
                    Environment.NewLine + "No files from this request were changed.");
            var replacement = (install ? entry.Install : entry.Uninstall).Apply(existing);
            if (!Matches(replacement, desired))
                throw new InvalidDataException("Patch verification failed for " + entry.Path);
            changes.Add(index, replacement);
        }
        if (changes.Count == 0)
            return install ? "Team Swap is already installed." : "Team Swap is already uninstalled.";

        Directory.CreateDirectory(transaction);
        try
        {
            foreach (var file in changes)
                WriteDurably(Staged(transaction, file.Key), file.Value);
            RequireGameClosed(game);
            // Check again after staging, before replacing any destination.
            foreach (var file in changes)
            {
                PatchEntry entry = entries[file.Key];
                byte[] expected = install ? entry.VanillaHash : entry.ModHash;
                if (!Matches(File.ReadAllBytes(Path.Combine(game, entry.Path)), expected))
                    throw new IOException("The game files changed during staging. Pause Steam or Epic updates and try again.");
            }
            foreach (var file in changes)
            {
                string target = Path.Combine(game, entries[file.Key].Path);
                File.Replace(Staged(transaction, file.Key), target, Backup(transaction, file.Key));
            }
            for (int i = 0; i < entries.Count; i++)
            {
                PatchEntry entry = entries[i];
                if (!Matches(File.ReadAllBytes(Path.Combine(game, entry.Path)),
                             install ? entry.ModHash : entry.VanillaHash))
                    throw new IOException("Final verification failed for " + entry.Path);
            }
            WriteDurably(Path.Combine(transaction, "complete"), new byte[] { 1 });
        }
        catch
        {
            Recover(game, transaction, entries);
            throw;
        }
        Recover(game, transaction, entries);
        return install ? "Team Swap installed"
                       : "Team Swap uninstalled. The original game files were restored.";
    }

    private static void Recover(string game, string transaction, List<PatchEntry> entries)
    {
        if ((File.GetAttributes(transaction) & FileAttributes.ReparsePoint) != 0)
            throw new IOException("The Team Swap transaction folder must be a normal folder.");
        bool complete = File.Exists(Path.Combine(transaction, "complete"));
        if (!complete)
        {
            for (int i = 0; i < entries.Count; i++)
            {
                string backup = Backup(transaction, i);
                if (!File.Exists(backup))
                    continue;
                PatchEntry entry = entries[i];
                string target = Path.Combine(game, entry.Path);
                byte[] old = File.ReadAllBytes(backup);
                byte[] current = File.ReadAllBytes(target);
                if ((!Matches(old, entry.VanillaHash) && !Matches(old, entry.ModHash)) ||
                    (!Matches(current, entry.VanillaHash) && !Matches(current, entry.ModHash)))
                    throw new IOException("An unfinished Team Swap operation needs attention. Retained backup for " + entry.Path);
                File.Replace(backup, target, null);
            }
        }
        for (int i = 0; i < entries.Count; i++)
        {
            File.Delete(Staged(transaction, i));
            File.Delete(Backup(transaction, i));
        }
        File.Delete(Path.Combine(transaction, "complete"));
        Directory.Delete(transaction);
    }

    private static string Staged(string directory, int index)
    {
        return Path.Combine(directory, index + ".new");
    }

    private static string Backup(string directory, int index)
    {
        return Path.Combine(directory, index + ".old");
    }

    private static void WriteDurably(string file, byte[] bytes)
    {
        using (var stream = new FileStream(file, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        {
            stream.Write(bytes, 0, bytes.Length);
            stream.Flush(true);
        }
    }

    private static bool Matches(byte[] bytes, byte[] hash)
    {
        return Hash(bytes) == BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
    }

    private static void RequireGameClosed(string game)
    {
        string executable = Path.Combine(game, "Shrek.exe");
        foreach (Process process in Process.GetProcessesByName("Shrek"))
        {
            using (process)
            {
                string path;
                try { path = process.MainModule.FileName; }
                catch (InvalidOperationException) { continue; }
                catch (System.ComponentModel.Win32Exception) { path = executable; }
                if (string.Equals(path, executable, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("Close Shrek 2 Remastered before installing or uninstalling Team Swap.");
            }
        }
    }

    internal static List<GameInstallation> FindGames()
    {
        string steam = null;
        try
        {
            steam = Registry.GetValue(@"HKEY_CURRENT_USER\Software\Valve\Steam", "SteamPath", "") as string;
        }
        catch (System.Security.SecurityException) { }
        catch (UnauthorizedAccessException) { }
        if (string.IsNullOrWhiteSpace(steam))
            steam = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Steam");
        // Epic documents its local .item manifest directory here:
        // https://dev.epicgames.com/documentation/en-us/unreal-engine/academic-installation-of-unreal-engine
        string epic = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData),
                                   @"Epic\EpicGamesLauncher\Data\Manifests");
        return FindGames(new[] { steam }, new[] { epic });
    }

    // Injectable launcher roots also allow discovery tests without changing
    // registry entries, launcher installations, or real game files.
    internal static List<GameInstallation> FindGames(IEnumerable<string> steamRoots, IEnumerable<string> epicManifestFolders)
    {
        var result = new List<GameInstallation>();
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (string steam in steamRoots)
        {
            var libraries = new List<string>();
            if (!string.IsNullOrWhiteSpace(steam)) libraries.Add(steam);
            try
            {
                string listing = Path.Combine(steam, "steamapps", "libraryfolders.vdf");
                if (File.Exists(listing))
                    foreach (Match match in Regex.Matches(File.ReadAllText(listing), "\"path\"\\s+\"([^\"]+)\""))
                        libraries.Add(match.Groups[1].Value.Replace(@"\\", @"\"));
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
            catch (ArgumentException) { }
            catch (System.Security.SecurityException) { }
            foreach (string library in libraries)
            {
                try
                {
                    string steamApps = Path.Combine(library, "steamapps");
                    string manifest = Path.Combine(steamApps, "appmanifest_2523740.acf");
                    if (!File.Exists(manifest)) continue;
                    Match match = Regex.Match(File.ReadAllText(manifest), "\"installdir\"\\s+\"([^\"]+)\"");
                    if (match.Success)
                        AddInstallation(result, seen, Path.Combine(steamApps, "common", match.Groups[1].Value), "Steam");
                }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
                catch (ArgumentException) { }
                catch (System.Security.SecurityException) { }
            }
        }
        foreach (string directory in epicManifestFolders)
        {
            string[] manifests;
            try
            {
                if (string.IsNullOrWhiteSpace(directory) || !Directory.Exists(directory)) continue;
                manifests = Directory.GetFiles(directory, "*.item");
                Array.Sort(manifests, StringComparer.OrdinalIgnoreCase);
            }
            catch (IOException) { continue; }
            catch (UnauthorizedAccessException) { continue; }
            catch (ArgumentException) { continue; }
            catch (System.Security.SecurityException) { continue; }
            foreach (string manifest in manifests)
            {
                try
                {
                    var data = new JavaScriptSerializer().DeserializeObject(File.ReadAllText(manifest)) as Dictionary<string, object>;
                    if (data == null) continue;
                    object incomplete;
                    if (data.TryGetValue("bIsIncompleteInstall", out incomplete) && incomplete is bool && (bool)incomplete) continue;
                    object location;
                    if (data.TryGetValue("InstallLocation", out location) && location is string)
                        AddInstallation(result, seen, (string)location, "Epic Games");
                }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
                catch (ArgumentException) { }
                catch (InvalidOperationException) { }
                catch (System.Security.SecurityException) { }
            }
        }
        return result;
    }

    private static void AddInstallation(List<GameInstallation> result, HashSet<string> seen, string folder, string store)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(folder) || !Path.IsPathRooted(folder)) return;
            folder = Path.GetFullPath(folder);
            if (!string.Equals(folder, Path.GetPathRoot(folder), StringComparison.OrdinalIgnoreCase))
                folder = folder.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            if (File.Exists(Path.Combine(folder, "Shrek.exe")) && Directory.Exists(Path.Combine(folder, "gamedata")) && seen.Add(folder))
                result.Add(new GameInstallation(folder, store));
        }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
        catch (ArgumentException) { }
        catch (NotSupportedException) { }
        catch (System.Security.SecurityException) { }
    }

    internal sealed class GameInstallation
    {
        internal readonly string Folder;
        internal readonly string Store;
        internal GameInstallation(string folder, string store) { Folder = folder; Store = store; }
        public override string ToString() { return Folder; }
    }

    private sealed class PatchEntry
    {
        internal readonly string Path;
        internal readonly byte[] VanillaHash;
        internal readonly byte[] ModHash;
        internal readonly Delta Install;
        internal readonly Delta Uninstall;

        internal PatchEntry(BinaryReader reader)
        {
            Path = Encoding.ASCII.GetString(reader.ReadBytes(reader.ReadInt32()));
            VanillaHash = reader.ReadBytes(32);
            ModHash = reader.ReadBytes(32);
            Install = new Delta(reader);
            Uninstall = new Delta(reader);
        }
    }

    private sealed class Delta
    {
        private readonly int size;
        private readonly List<Op> operations;

        internal Delta(BinaryReader reader)
        {
            size = reader.ReadInt32();
            int count = reader.ReadInt32();
            operations = new List<Op>(count);
            for (int i = 0; i < count; i++)
            {
                int source = reader.ReadInt32();
                int length = reader.ReadInt32();
                operations.Add(new Op(source, length, source == -1 ? reader.ReadBytes(length) : null));
            }
        }

        internal byte[] Apply(byte[] input)
        {
            byte[] output = new byte[size];
            int offset = 0;
            foreach (Op op in operations)
            {
                if (op.Literal == null)
                    Buffer.BlockCopy(input, op.Source, output, offset, op.Length);
                else
                    Buffer.BlockCopy(op.Literal, 0, output, offset, op.Length);
                offset += op.Length;
            }
            if (offset != size)
                throw new InvalidDataException("Incomplete patch output.");
            return output;
        }

        private sealed class Op
        {
            internal readonly int Source;
            internal readonly int Length;
            internal readonly byte[] Literal;

            internal Op(int source, int length, byte[] literal)
            {
                Source = source;
                Length = length;
                Literal = literal;
            }
        }
    }
}

internal sealed class InstallerWindow : Form
{
    private readonly ComboBox location;
    private readonly Label status;

    internal InstallerWindow() : this(Installer.FindGames()) { }

    internal InstallerWindow(IEnumerable<Installer.GameInstallation> installations)
    {
        Text = "Team Swap Installer";
        ClientSize = new Size(516, 236);
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        AutoScaleMode = AutoScaleMode.Font;
        Font = SystemFonts.MessageBoxFont;

        var title = new Label { Text = "Shrek 2 Remastered Team Swap", AutoSize = true,
            Font = new Font(Font.FontFamily, 14f, FontStyle.Bold), Location = new Point(16, 16) };
        var instruction = new Label { Text = "Close the game first. Steam / Epic folders are detected automatically.\nSteam: 26880364 / 26746348. Epic: CL593174 / CL588802.",
            Size = new Size(480, 36), Location = new Point(18, 49) };
        Controls.Add(title);
        Controls.Add(instruction);
        Controls.Add(new Label { Text = "Game folder", AutoSize = true, Location = new Point(18, 92) });
        location = new ComboBox { DropDownStyle = ComboBoxStyle.DropDown, Location = new Point(18, 112), Width = 390 };
        foreach (var game in installations) location.Items.Add(game);
        Controls.Add(location);
        Controls.Add(MakeButton("Browse...", 416, 110, 82, Browse));
        Controls.Add(MakeButton("Install", 18, 152, 105, delegate { Apply(true); }));
        Controls.Add(MakeButton("Uninstall", 132, 152, 105, delegate { Apply(false); }));
        status = new Label { Size = new Size(480, 34), Location = new Point(18, 190) };
        Controls.Add(status);
        location.TextChanged += delegate { UpdateLocationStatus(); };
        if (location.Items.Count > 0) location.SelectedIndex = 0;
        UpdateLocationStatus();
    }

    private void UpdateLocationStatus()
    {
        var game = location.SelectedItem as Installer.GameInstallation;
        status.Text = string.IsNullOrWhiteSpace(location.Text) ? "Choose the folder containing Shrek.exe and gamedata."
            : game != null && string.Equals(game.Folder, location.Text, StringComparison.OrdinalIgnoreCase)
              ? game.Store + " installation found. Game files will be verified on Install."
              : "Folder selected. Game files will be verified on Install.";
    }

    private static Button MakeButton(string name, int x, int y, int width, EventHandler action)
    {
        var button = new Button { Text = name, Location = new Point(x, y), Size = new Size(width, 29) };
        button.Click += action;
        return button;
    }

    private void Browse(object sender, EventArgs args)
    {
        using (var picker = new FolderBrowserDialog())
        {
            picker.Description = "Select the Shrek 2 Remastered game folder.";
            picker.SelectedPath = location.Text;
            picker.ShowNewFolderButton = false;
            if (picker.ShowDialog(this) == DialogResult.OK)
                location.Text = picker.SelectedPath;
        }
    }

    private void Apply(bool install)
    {
        try
        {
            UseWaitCursor = true;
            string patch = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "TeamSwap.patch");
            status.Text = Installer.Change(location.Text, patch, install);
            MessageBox.Show(this, status.Text, Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (UnauthorizedAccessException)
        {
            MessageBox.Show(this, "Windows denied access to the game folder. Run the installer as administrator if your Steam or Epic library requires it.",
                Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
        catch (Exception error)
        {
            MessageBox.Show(this, error.Message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
        finally { UseWaitCursor = false; }
    }
}






