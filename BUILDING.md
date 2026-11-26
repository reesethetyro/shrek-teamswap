# Building and verification

## Requirements

- Windows x64 for the installer and native Windows-target checks.
- Python 3.
- LLVM clang with the `x86_64-pc-windows-msvc` target. The verification run used the clang bundled with Unity 6000.0.43f1; another compiler version may produce different text hashes.
- .NET Framework C# compiler (`csc.exe`) for the WinForms installer.
- For the source health-bar decode only: `python -m pip install -r requirements-review.txt`.

Dependencies and compiler binaries are not vendored. Commands run from the repository root. The tools write beneath ignored `build/`; they do not install into a game directory.

## Compile the installer

```powershell
powershell -NoProfile -File tools/build_installer.ps1
```

Use `-Csc PATH_TO_CSC` if needed. The result is `build/Team Swap Installer.exe`. The installer intentionally requires the exact separately reviewed TeamSwap.patch identified by its embedded SHA-256; that payload is not part of this source repository. Compiling the installer does not install the mod. PE metadata/compiler differences can change the rebuilt installer file hash.

## Compile current runtime source layers

```powershell
python tools/compile_sources.py --clang PATH_TO_CLANG --game-root PATH_TO_YOUR_GAME
```

The game directory is read-only. This command reads only the original `gamedata/common/t_hud_playerhealthbar_bar_ui.dds` texture, validates its hash, decodes its KTX2/BC7 contents and regenerates the aqua palette. It then compiles Fairy/settings, Quick Swap/fixes and arena-pickup source for Steam 26746348, Epic CL588802, Steam 26880364/CL593174 and Epic CL593174.

`build/compile-results.json` records the twelve text hashes. Matching expected hashes verifies those code layers, not a full game executable. Fairy text is compared with its pre-Quick-Swap layer; the subsequent hook rebindings are recorded in `review/hooks-*.json`.

## Compile the final arena owner gate

```powershell
python tools/compile_gates.py --clang PATH_TO_CLANG
```

All four assembled gates are checked against the final release's gate hashes.

## Compile the historical menu/swap core

```powershell
python tools/compile_core.py --clang PATH_TO_CLANG
```

This uses the original public nine-character core and its captured default configuration. Optional historical test switches remain disabled. The object is an inspection build for the earlier core source revision, not a drop-in replacement for the current compatibility ports. The retained legacy executable differs from this source object at 325 bytes after later edits; this is recorded explicitly in review/legacy-core-config.json.

## Regenerate modified text assets

```powershell
python tools/apply_asset_edits.py --platform Steam26880364 --game-root PATH_TO_CLEAN_GAME
```

Requires exact clean localization and menu YAML inputs. Already-modded inputs are rejected by hash. Output goes to `build/assets/PLATFORM`, never the game directory. The changes and output hashes match the release manifest.

## Full-build limitation and historical inputs

The oldest roster foundation was assembled through an earlier game-mod chain. Its source references older clean executables, intermediate roster/weight-help images, relocation plans and disassembly/symbol records. The compatibility-port scripts also consume older and newer clean/baseline images and matching address data. These proprietary/local inputs are deliberately absent.

`src/roster_foundation/` and `src/compatibility_history/` preserve available implementation/provenance, but are **reference sources**, not advertised as standalone current-release builders. Personal historical paths in the latter have been normalized to `inputs/`. No full clean-to-final rebuild is claimed. If staff require a complete bootstrapped release build, that legacy chain must be reconstructed and validated separately before making that claim.

## Tests included with this review

`tests/InstallerTests.cs` contains the installer regression harness; it requires a matching reviewed payload and disposable clean game fixtures. Previous reports are explicitly historical under `review/previous-checks/`. Fresh source-package checks are in `review/source-package-validation.json` after validation. Compilation, hash comparison and installer round-trips are distinct from live gameplay testing.
