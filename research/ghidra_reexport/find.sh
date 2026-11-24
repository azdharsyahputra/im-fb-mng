#!/bin/sh
# Run FindGeom.java over the unpacked cm2008_full DB (read-only static scan). Usage: find.sh OUT.txt
GHIDRA=${GHIDRA:-/Users/csadeveloper/tools/ghidra_12.1.4_PUBLIC}
PROJ=${PROJ:-/Users/csadeveloper/tools/cm2008_full}
OUT="$1"
exec "$GHIDRA/support/analyzeHeadless" "$PROJ" CM2008 -process CM2008.exe -noanalysis \
  -scriptPath "$(dirname "$0")" -postScript FindGeom.java "$OUT"
