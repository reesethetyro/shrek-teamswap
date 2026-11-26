# Source map

| Area | Source | Notes |
| --- | --- | --- |
| Installer UI, discovery, patch validation, install/uninstall and transaction recovery | `src/installer/Installer.cs` | Full current installer source; no downloader. |
| Quick Swap, keyboard/mouse/controller input, settings persistence and current safeguards | `src/quick/quick.cpp` | Shrek carrying check, jumping/attack restrictions, Fairy hover exception. |
| Aim-ring cleanup, per-character potion exclusions, Hide HUD rename and zero-counter rendering | `src/quick/quick.cpp` and platform `entry.s` files | Native aim teardown and six damage-multiplier call adapters. |
| Fairy shield, duration/countdown, lever visuals, floor exits, description, settings layout and portrait handling | `src/fairy/PLATFORM/` | Platform-specific C++/assembly sources; native assets are referenced, not distributed here. |
| Crash Coliseum apples/cookies and spawn behavior | `src/arena_ammo/` | Uses native pickup roots from locally installed game data. |
| Offline Crash Coliseum personal-ammo eligibility | `src/clock_gate/` | The later ammo addition broadens the gate to the three supported owners; see review notes below. |
| Original live-swap commit, roster drafts, health/ammo history, scripted-sequence restrictions, menu and portraits | `src/core_legacy/` | Historical public implementation, before later native-address ports. |
| Earlier expanded-roster implementation | `src/roster_foundation/` | Historical prerequisite source; external local inputs documented in BUILDING.md. |
| Address/PE compatibility conversion history | `src/compatibility_history/` | Reference only; paths normalized, historical inputs excluded. |
| Localization and portrait menu asset changes | `src/assets/text-edits.json` | Guarded source text recipes, 12 assets per supported build. |
| Aqua shield health ring | `tools/palette.py` | Derives pixels locally; hash-checked source and generated palette. |
| Native PE injection, COFF relocation, hook and unwind-table emission | `src/build_reference/` | Actual release-stage builder logic; external historical inputs remain documented. |
| Feature hooks and native RVA mappings | `review/hooks-*.json`, `review/compile-config.json` | Addresses/short hook instructions; no game executable. |

The clock gate source reflects the final arena-ammo change: the earlier clock-only owner comparison is bypassed with two NOPs, permitting the three native personal-ammo owner lookups in offline Crash Coliseum. `tools/compile_gates.py` verifies the final gate text for all four builds.
