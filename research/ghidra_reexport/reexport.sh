#!/bin/sh
# Re-export clean decompilation of CM2008 functions from the UNPACKED Ghidra project cm2008_full.
# The retail CM2008.exe on the ISO is SecuROM-packed (static analysis = junk); cm2008_full is the
# existing unpacked analysis database the committed docs/ghidra_dumps were made from. This reads
# that static DB only — it does NOT run the game or touch DRM.
#
# Usage: reexport.sh OUT.txt 0xADDR [0xADDR ...]
GHIDRA=${GHIDRA:-/Users/csadeveloper/tools/ghidra_12.1.4_PUBLIC}
PROJ=${PROJ:-/Users/csadeveloper/tools/cm2008_full}
OUT="$1"; shift
exec "$GHIDRA/support/analyzeHeadless" "$PROJ" CM2008 -process CM2008.exe -noanalysis \
  -scriptPath "$(dirname "$0")" -postScript Decomp.java "$OUT" "$@"
