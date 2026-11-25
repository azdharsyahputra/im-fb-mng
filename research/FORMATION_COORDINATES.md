# Formation pitch coordinates — VERIFIED source (FieldPosition.dat + PositionTeamTactics.dat)

Resolves the long-standing open item "runtime NameID→FieldPosition-cell assignment UNKNOWN". The real
per-formation, per-player on-pitch home cells ARE in two shipped DATa files (NOT code-resident). Decoded
from bytes and re-verified independently.

## File 1 — DatabaseFiles/FieldPosition.dat (8584 B) — VERIFIED
`DATa`, count 268, **32-byte records**:
```
+0  u32 id (1..268)      +4  u32 namehash (role key)
+8  i32 Column_base      +12 i32 Row_base
+16 i32 Column_attack    +20 i32 Row_attack
+24 i32 Column_defend    +28 i32 Row_defend      (-1 = unused)
```
Integer GRID CELLS (not floats). **Column = depth 0..15** (0 = own goal line → opponent goal),
**Row = width 1..13** (1/13 = touchlines, 7 = centre). Three (Column,Row) pairs = base / in-attack /
in-defence positions. Samples: `id=1 (0,7)(0,7)(0,7)` = GK (never moves); `id=3 (5,13)(10,13)(5,13)` =
right-touchline player pushing up (Col 5→10) in attack; `id=102 (9,11)(14,10)(9,11)`.

## File 2 — DatabaseFiles/PositionTeamTactics.dat (3308 B) — VERIFIED
`DATa`, count 275 = **25 formations × 11 slots**, **12-byte records** (three i32):
```
col0 PositionTacticsID  (14 distinct: {1..11,19,20,21} — role/duty profile)
col1 FormationID        (1..25, each appearing exactly 11×)
col2 FieldPositionID    (FK → FieldPosition.dat; value 1 appears 25× = the shared GK)
```
So **FormationID (col1) → its 11 FieldPositionIDs (col2) → FieldPosition.dat cells.** Complete
formation→geometry link, VERIFIED end-to-end.

### Verification (independent, from bytes)
- FieldPosition: count 268, stride 32.0; id1=(0,7)/(0,7)/(0,7), id3=(5,13)/(10,13)/(5,13); Column base
  range 0..14, Row base range 1..13.
- PositionTeamTactics: count 275, stride 12.0; col1 = 1..25 each ×11; col2 value 1 appears 25×.
- FormationID 8 base cells: GK (0,7); back four (4,1)(4,5)(4,9)(4,13); mids (7,5)(7,9); wide (9,1)(9,13);
  strikers (13,5)(13,9) → a clean **4-4-2**. The 25 IDs decode to coherent full-pitch 11-man shapes.

## .tac confirmed to hold NO grid
`cmp -l 442.tac 433.tac` → exactly 4 differing bytes at 0x525–0x528 (the NameID dword); all other 2033
bytes identical. The 25 .tac are one GCTactics template keyed by a NameID; the geometry is in the two .dat.

## Status
- RESOLVED: the NameID→FieldPosition-cell assignment (it is the FieldPositionID FK chain above).
- Ported: super-manager-engine/formation/grid.go (LoadGrid + SlotsByID/AttackSlotsByID/DefendSlotsByID;
  normalize X=Column/15, Y=(Row-1)/12). The standard-thirds layout is now only a fallback.
- Still soft (does NOT block coordinates): FormationID(1..25) → formation-NAME binding is shape-matched
  (exact confirmation needs the game name-hash fn over TeamTactic.dat namehash); grid→metres scale is a
  normalization choice. Likely runtime reader: GCFieldPositionModule / the loader near VA 0x70f600–0x70fc00.
