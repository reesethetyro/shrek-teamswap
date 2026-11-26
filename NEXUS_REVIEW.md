# Nexus Mods staff review

## Release identity

See `review/release-manifest.json` for the exact distributed ZIP, installer, payload and all clean/modded file hashes. This review snapshot was prepared from the current desktop release, not from the separate experimental character work.

## Suggested review order

1. `src/installer/Installer.cs`: complete executable installer source.
2. `src/quick/quick.cpp` and its platform entry shims: newest gameplay changes.
3. Platform Fairy C++/assembly and arena-pickup source.
4. Historical core/roster/compatibility sources, with the limitations in BUILDING.md.
5. Hash manifests and source-package validation results.

## Installer behavior

- Discovers local Steam installations through Steam's registry/library manifests, and Epic installations through local Epic manifests; also permits manual folder selection.
- Reads the adjacent TeamSwap.patch, verifies its embedded expected SHA-256, and requires supported hashes for every target file before applying changes.
- Checks that the matching Shrek process is closed. It patches files on disk; it is not an external live-process injector.
- Replaces Shrek.exe, localization/menu text, and the menu portrait-reference YAML files listed in the release manifest. Uses a `.teamswap-transaction` directory with staged files and backups, then verifies outputs and supports rollback/recovery.
- Install and uninstall paths use the same reviewed patch tables. Save files are not listed as patch targets.
- Source inspection found no HTTP download client, external binary launch, startup persistence, credential collection or telemetry implementation in the installer. This is an implementation description, not an antivirus certification or Nexus approval.

The mod's injected runtime code calls the game's native engine functions. It references native game assets and existing online-state APIs but does not contain a separate download or update service.

## Submission email draft

To: support@nexusmods.com
Subject: Team Swap — source code submitted for quarantine review

Hello Nexus Mods staff,

Following your request, I have prepared the Team Swap source code for review:

Repository: [PASTE YOUR GITHUB REPOSITORY URL]
Reviewed commit or tag: [PASTE THE COMMIT HASH OR TAG]
Nexus mod page: [PASTE YOUR NEXUS MOD PAGE URL]

The repository contains the current installer source, gameplay C++ and assembly, source-to-release hash checks, asset-edit recipes and build documentation. It excludes game executables/assets and the unfinished character-expansion project. BUILDING.md explicitly documents the historical core build dependencies and distinguishes runtime-layer compilation from a complete clean-to-release build.

The release ZIP SHA-256 is recorded in review/release-manifest.json. There are no alternate binary download links in this source package.

Please let me know if you require additional build inputs or documentation. I will await your response before proceeding with distribution as instructed.

Thank you,
[YOUR NEXUS USERNAME]

## Before sending

Upload the folder contents to GitHub, replace the bracketed email fields, and identify an immutable commit/tag. Send the repository link rather than an alternate installer download link. This package has not been uploaded or emailed automatically, and approval is pending staff review.
