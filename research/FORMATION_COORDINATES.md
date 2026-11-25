# Formation pitch coordinates — VERIFIED source (FieldPosition.dat + PositionTeamTactics.dat)

> **UPDATE 2 — formation NAMES now VERIFIED (supersedes the earlier "squad-tactic presets" note).** Each
> shipped .tac file carries a 32-bit **NameID at offset 0x525**, and those 25 values match the `namehash`
> column of **TeamTactic.dat EXACTLY, in id order** (24/25 bit-identical; id 24 = "541" by elimination). So
> TeamTactic.dat IS the formation NAME index, and **TeamTactic.id → formation name is VERIFIED** (no hashing
> of strings needed — the stored NameID == the stored namehash; the earlier zlib.crc32 attempt failed only
> because it was hashing name strings, which is not how these columns relate). Ported: formation/names.go
> FormationNames (id→name). The subagent's "TeamTactic.dat = squad-tactic presets" was WRONG.
>
> STILL PARTIAL — the coordinate key→name join: PositionTeamTactics.dat's group key is **TeamTacticsID**
> (1..25, per key schema §2.1: FieldPositionID, TeamTacticsID, PositionTacticsID), a DIFFERENT ordering with
> no fixed offset to TeamTactic.id. Each key resolves to a real 11-man SHAPE (VERIFIED coords), and the
> DISTINCTIVE shapes bind to a name by fine line-structure (541, 4141, 433, 4321, 424, 4231, 4411, 4132,
> 3421, 253 — VERIFIED-unique); the **442_* and 532_* VARIANTS share a base shape and cannot be told apart
> by cell geometry** (the variant .tac store no grid), so those keys resolve only to the base family. The
> earlier "FormationID 8 = 442 name-aligned" phrasing stays RETRACTED (it is a 4-4-2 by shape). Coordinates
> by numeric key are VERIFIED and usable (formation/grid.go SlotsByID); names via formation/names.go.

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

## Variant separation (442_* / 532_*) — structural distinctives VERIFIED
Dumping the full (Column,Row) base cells per coordinate key separates the variants by their defining
sub-position (not just coarse shape):
- **442_Diamond** = key 7: midfield is a diamond — DM (7,7) + AM (11,7), both central (vs a flat 4).
- **532_Sweeper** = key 4: a lone deepest central defender (4,7) sitting behind the back line.
- **532_HolePlayer** = key 5: an advanced central AM "in the hole" (11,7) ahead of a flat back 5.
These three are VERIFIED by the cell pattern. The **_Attacking / _Defensive** variants are then assigned
within each family by team depth (Attacking = line pushed up / strikers at col14; Defensive = withdrawn /
strikers at col13): key 10=442_Attacking, 8=442_Defensive, 9=442(base); 17=532_Attacking, 3=532_Defensive
— INFERRED-by-depth (medium confidence). Remaining ambiguous keys (1/25 both 5-4-1 → 541/451; 22/23 → 334/
325; 2) are left UNBOUND rather than guessed — they need the Row sub-pattern or another source. Ported in
formation/names.go ShapeKeyToName with per-entry confidence.

## Cross-reference exhausted — the TeamTacticsID→name join is CODE-resident
Checked every candidate DATA source for a key→name join; NONE exists:
- .tac files: only the 4-byte NameID @0x525 differs across all 25 (confirmed by full pairwise diff) — one
  template, no geometry/key field.
- TeamTactics.dat (25×51), PositionTactics.dat (16×73): scalar presets, no formation hash in any record.
- TeamTacticsFormation.dat / TeamTacticFormation.dat (48×8): UI menu grouping (member-id, group-id 1..18),
  no key→name.
- Only TeamTactic.dat carries the formation namehashes (= the id→name we already have).
So the TeamTacticsID(geometry)→name join is not in shipped data; it is code-resident (the loader near the
GCPositionTeamTacticsModule / formation selector).

Structural matching (re-verified from cells) FULLY resolves the 442 and 532 families:
- 442: key7=Diamond (DM+AM), 16=4132, 8/9/10 = Defensive/base/Attacking by depth.
- 532: key2=Sweeper (lone CB at col3 behind the back four — corrects the earlier key4=Sweeper), 5=HolePlayer
  (AM at col11), 3/4 = Defensive/Attacking by depth.
Bound ≈19/25 (formation/names.go ShapeKeyToName). The irreducible remainder (keys 1,25,17 at the
5-defender/4+DM boundary → 541/451/41311; 19,21 → 3412/343; 23 → 325) differs by a single player's line
assignment, below the clustering threshold — those need the loader code trace, not more data.
