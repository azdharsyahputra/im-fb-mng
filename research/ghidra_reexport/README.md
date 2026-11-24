# Ghidra re-export recipe (unblocks the un-dumped function bodies)

The committed `docs/ghidra_dumps/*.c` were exported from the Ghidra project
`/Users/csadeveloper/tools/cm2008_full` (program `CM2008.exe`), which is the UNPACKED analysis.
The retail `CM2008.exe` on the owner's ISO is **SecuROM-packed** — opening it fresh in Ghidra/REA
yields junk (verified: import timeout + "unable to read bytes" at overlay addresses). So the only
static-readable source for any not-yet-dumped function is this existing project.

`Decomp.java` is a GhidraScript (Ghidra 12 dropped Jython, so Java, not Python) that decompiles any
list of addresses to a file. `reexport.sh OUT.txt 0xADDR ...` runs it headless with `-noanalysis`
(no re-import, fast). Proven: FUN_0077ee80 decompiles cleanly to `GCMatchModule::ProcessData`.

This is static analysis of an existing database — no game execution, no DRM bypass. The ONLY
match-engine piece this cannot reach is the SecuROM-protected picker cluster (0x5b37xx-0x5b52xx),
which stays runtime-only (see RUNTIME_CAPTURE_HOWTO.md).
