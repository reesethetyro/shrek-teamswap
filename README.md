# Team Swap — source review snapshot

Source for the Windows Shrek 2 Remastered Team Swap mod, prepared for Nexus Mods staff review on **2026-11-26**.

This snapshot corresponds to the release ZIP identified in [`review/release-manifest.json`](review/release-manifest.json). It includes the current Quick Swap carrying safeguard, Fairy hover exception, hidden zero counters, potion handling, Fairy changes, Swap Settings and Crash Coliseum additions. The roster is the released nine-character roster. Unfinished character-expansion work is excluded.

## Start here

- [`NEXUS_REVIEW.md`](NEXUS_REVIEW.md): review entry points, installer behavior and submission draft.
- [`BUILDING.md`](BUILDING.md): tested compile commands, inputs and build limitations.
- [`SOURCE_MAP.md`](SOURCE_MAP.md): where each feature lives.
- [`src/installer/Installer.cs`](src/installer/Installer.cs): complete installer source.
- [`src/quick/quick.cpp`](src/quick/quick.cpp): current Quick Swap and latest fixes.
- [`review/release-manifest.json`](review/release-manifest.json): release, payload and per-file hashes.

## Contents and boundaries

The repository contains C++, assembly, C#, Python/PowerShell build helpers, text-asset editing recipes, address configurations and review reports. It does **not** contain game executables, the distributed installer executable, TeamSwap.patch, game textures/models/audio, save files, PDBs, credentials or personal installation paths.

The health-bar texture is derived locally from a supported game copy during the compile check; the original or modified pixels are not committed. Text-asset recipes contain only the changed text and references, not complete game files.

## Build status

The installer compiles independently. The twelve current runtime targets (three layers across four game builds) can be compiled into inspection objects, and their linked text was checked against the corresponding release layers. The historical public menu/swap core also compiles separately for inspection.

**This is not a tested one-command clean-game-to-release build.** The old expanded-roster foundation and compatibility-port chain still require historical local inputs. Those sources and the missing-input boundary are documented rather than hidden behind a generated executable or an opaque payload. See BUILDING.md before interpreting the compile checks as a full release rebuild.

No repository upload or email submission is performed by these files. Do not add alternate binary download links while review is pending.
