# PLAYERPROPERTIES_COLUMN_BINDING.md — binding the 77 names to the on-disk .dat columns

Companion to `PLAYERPROPERTIES_SCHEMA_NAMES.md` (the 77 schema field names in declaration order,
recovered from the schema-registration fn ~0xa7d1e1). This note records the **value-match** of those
names against the real `DatabaseFiles/PlayerProperties.dat` (76,051 rows × 77 u16 cols, +u32 id),
using the per-column fingerprint (`super-manager-engine/cmd/attrfp`).

## Result: schema declaration order is NOT the on-disk column order for cols 0..60

Measured per-column distributions (non-sentinel %, max, distinct, mean, `<=20` share) do not fit the
schema name at the same index for the first 61 columns. Clearest refutation: schema index 48 =
`GoalKeeper` (a positional-familiarity field, which must be low/keeper-sparse for the mostly-outfield
population), but **disk column 48 is a normal 0..100 bell (mean ≈ 59, 93 distinct, not sparse)**.
The whole 18..60 block is 43 near-identical 0..100 bells, so no index in it matches a specific name
by shape.

## Confirmed by signature: disk cols 63..76 == schema 63..76 (the 14 instruction sliders)

These are the only 14 pure-`<=20` columns in the table (max = 15, 100% of values `<=20`), and the
schema's last 14 names are exactly the 14 tactical "X-Or-Y" instruction sliders. The match is unique
and VERIFIED:

| disk col | name |
|---|---|
| 63 | DropDeepOrPushUp |
| 64 | LookForOverlapsOrSitBack |
| 65 | CarryBallOutOfDefenceOrGetRidOfBall |
| 66 | StayOnFeetOrLookToDive |
| 67 | HoldPositionOrBombAround |
| 68 | DistributeToWingsOrPlayThroughMiddle |
| 69 | SwitchFlanksStretchPlayOrLookForSimplePass |
| 70 | HoldUpTheBallOrGoForGoal |
| 71 | RoundTheKeeperOrShoot |
| 72 | DriftOutWideOrStayCentral |
| 73 | CutInsideOrHugTouchline |
| 74 | RunWithBallOrLookToReleaseOthers |
| 75 | PassBackwardsOrPassForwards |
| 76 | LongRangeShootingOrWorkOpenings |

(Disk cols 61..62 are only ~1% populated 0..100 bells — positionally they line up with schema
`PenaltyTaking`/`PreferredFoot`, but low population makes this only plausible, not confirmed.)

## Structure of the on-disk columns (measured)

- **cols 0..17** — higher population (26–51%), **low distinct (~9–26 levels)**, extreme means
  (e.g. col1 mean 90, col3 mean 75 / 9 distinct, col7 ≈ all 1s, col13 mean 6). These are quantized
  (coarse-stepped) fields, not the fine 0..100 skills. Schema names for 0..17 are the technical +
  mental skills (Heading, Tackling, Dribbling, Crossing, Finishing, …), so either the disk order
  differs or these are stored on a coarser scale than the 18..60 block.
- **cols 18..60** — ~17% population, **high distinct (~80–98)**, clean 0..100 bells centred ~55–60.
  These are the fine-grained graded attributes ("detailed" players only). 43 columns.
- **cols 63..76** — the 14 instruction sliders (`<=20`), confirmed above.

## What remains, and how to finish it

The 77 **names** are VERIFIED. Disk columns **63..76** are bound. The **0..62** block needs
value-matching with anchors, because 43 near-identical bells cannot be separated by distribution
alone. Two ways:

1. **Small real capture** — read ~8–10 named players' attribute values off the owned game UI
   (read-only), then `attrmap.Solve` resolves each disk column uniquely by matching those known
   values; the fingerprint then validates the whole map.
2. **Loader disasm** — the function that reads `PlayerProperties.dat` into the runtime object
   assigns disk-column → field in order; pairing it with the schema's field→name closes it without a
   capture. (The schema-registration fn is known; the serialiser/loader order is the missing link.)

All measured from the owner's own file; no game data committed, no names invented, no column bound
on a guess.
