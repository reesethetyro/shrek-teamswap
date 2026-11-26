# Native patch-builder reference

These scripts preserve the PE section allocation, COFF relocation, hook emission and unwind-table construction used by the release stages. Personal/toolchain paths were normalized to `inputs/`. They rely on historical baseline images, symbol maps and previous-stage files that are not bundled, so they are not standalone build commands. The current C++/assembly is under the other src folders; the tested inspection compile commands are in the root BUILDING.md.
