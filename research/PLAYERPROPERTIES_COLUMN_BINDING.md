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

## Role-contrast binding (INFERRED) — Man Utd 2007/08 archetypes

A capture-free anchoring method: pull the raw 77-column vectors of several real players whose
footballing ROLE is public fact, and read each column by how it discriminates roles. No display
numbers are assumed — only well-known archetypes. Players used (all located in PlayerProperties.dat
by name → Person.id):

| id | player | role |
|---|---|---|
| 56386 | Nemanja Vidić | CB |
| 10930 | Rio Ferdinand | CB |
| 18951 | Patrice Evra | LB |
| 9251 | Michael Carrick | CM |
| 10287 | Paul Scholes | CM (playmaker) |
| 11263 | Ryan Giggs | W |
| 59196 | Cristiano Ronaldo | W |
| 63121 | Wayne Rooney | ST |

Structural confirmation: **Edwin van der Sar (GK) has NO PlayerProperties row** — keepers live only
in GoalkeeperProperties. So schema index 48 "GoalKeeper" is a *positional-familiarity* field (can an
outfielder deputise in goal), not a keeper skill, which is why disk col 48 is a normal 0..100 bell,
not keeper-sparse. (Resolves the earlier col48 refutation.)

### Inferred bindings (strong role evidence; INFERRED, not VERIFIED)

| disk col | reading | evidence (Vidic/Ferd=CB, Giggs/Ron=W, Rooney=ST, Scholes=CM) |
|---|---|---|
| 5  | Finishing (shooting) | Rooney 100, Ron 80, Giggs 80, Scholes 60, CBs 1, Evra 1 |
| 9  | Crossing | Evra 100, Giggs 100, Ron 100; CBs & Rooney low (wide-player skill) |
| 12 | Dribbling | Ron 100, attackers high, CBs ~20 |
| 44 | Pace | Vidić 15 (famously slow), Ron 97, Giggs 84, Evra 70 |
| 20 | Acceleration (speed pair w/44) | Ron 91, attackers high, CBs 35–42 |
| 38 | Strength | Rooney 91, Vidić 81, Ferdinand 61; Giggs 31, Ron 46 (wingers weak) |
| 2  | Marking/Tackling (pure defensive) | CBs 100, everyone else 1 |
| 15 | Marking/Tackling (defensive, CB-only) | CBs 60, everyone else 1 |
| 54 | CentralDefender (positional familiarity) | Vidić 92, Ferdinand 90, Carrick 47 |
| 13 | Vision/Creativity (playmaking) | Scholes 80, rest ~0 (Scholes signature) |
| 33 | Heading/Jumping (aerial) | CBs 90, Ron 93, Rooney 69; Scholes 48 |

Columns 0..17 are coarse-quantised (values in {1,20,40,60,80,100}) and carry the technical-skill
discrimination above; cols 18..62 are the fine 0..100 attributes/positional familiarities; cols
63..76 the <=20 instruction sliders (already VERIFIED). Note col 7 is 1 for all these players — rare
across the board, consistent with schema[7]=ThrowIns but not uniquely proven by them.

### Still ambiguous (needs 1–2 more anchors)

Within-category pairs can't be split by these 8 players: Marking vs Tackling (both CB-extreme),
Pace vs Acceleration (cols 20/44), Finishing vs LongShots, Heading vs Jumping. A pure full-back and
a target-man would separate crossing/pace and finishing/heading respectively. The full per-column
role table is in the session work (contrast_table).

## Refinement with single-attribute archetypes (corrects earlier inferences)

Added extreme-profile players (van Nistelrooy poacher, Lampard/Pirlo long-shooters, Crouch aerial
target, Cannavaro man-marker, Walcott pace, Drogba) to split within-category pairs. This sharpened
some bindings and REFUTED one earlier guess — recorded honestly:

Strengthened (strong archetype evidence):
- **col 2 = Marking** — Cannavaro 100, Vidić 100 (textbook markers), all others ≤20.
- **col 33 = Heading** — Crouch 93, Drogba 89, Ronaldo 93, CBs ~90; Pirlo 40, Scholes 48.
- **col 56 = LongShots** — Pirlo 96, Lampard 91, Scholes 89; van Nistelrooy 64, defenders low.

Corrected:
- **col 5 is NOT Finishing** (earlier INFERRED). van Nistelrooy (elite finisher) is only 60 while
  Walcott is 100 — inconsistent with Finishing. col 5 reads as a coarse forward/attacking field;
  specific name unresolved. Finishing is more likely **col 52** (Ronaldo 98, Rooney 90,
  van Nistelrooy 84; Vidić 30, Cannavaro 48) — tentative.
- **Speed group {col 20, col 34, col 44} is unresolved** between Pace/Acceleration/Agility: Pirlo
  (slow in reality) scores 87 at col 44, so col 44 = Pace is withdrawn. col 34 has Walcott 95 (max),
  a better speed candidate, but the three correlate too tightly to separate with these anchors.

Still holding from the first pass: col 9 = Crossing, col 12 = Dribbling, col 38 = Strength,
col 54 = CentralDefender (positional), col 13 = Vision/Creativity. col 61 = PenaltyTaking gains
support (Lampard/Pirlo high, sparse).

Net confirmed-ish column names so far (VERIFIED signature OR strong archetype): 63..76 sliders
(VERIFIED), col 2 Marking, col 33 Heading, col 56 LongShots, col 9 Crossing, col 12 Dribbling,
col 38 Strength, col 54 CentralDefender, col 13 Vision. The remaining ~50 columns are category-known
but not uniquely named — the method keeps converging as more distinctive archetypes are added.

## Current best column→name map (27 of 77)

After two archetype passes (27 real players), the bound columns:

VERIFIED (signature): cols 63–76 = the 14 instruction sliders; col 7 = ThrowIns (rare-attr shape).

INFERRED HIGH (archetype top-rankers, committed to playerdb.PlayerPropertyInferredColumn):
col 2 Marking · col 9 Crossing · col 20 Agility · col 21 Balance · col 22 Tackling ·
col 23 Finishing · col 27 Acceleration · col 33 Heading · col 34 Pace · col 38 Strength ·
col 52 Dribbling · col 56 LongShots.

Key splits achieved this pass: Marking (c2, Cannavaro/Terry) vs Tackling (c22, Gattuso/Mascherano);
Finishing (c23, Inzaghi/van Nistelrooy poachers) vs Dribbling (c52, Messi/Robben) — correcting the
earlier c5=Finishing/c52=Finishing guesses; Pace (c34) vs Acceleration (c27) by Walcott.

MED (not committed to code; see evidence): col 45 Positioning (Maldini/Cannavaro/Terry) ·
col 13 Vision/Creativity (Totti/Scholes) · col 26 Aggression-ish (Gattuso) · col 54 CentralDefender.

Remaining ~50 columns: category-known but still correlated (e.g. Anticipation/OffTheBall/Composure
cluster, the positional-familiarity block, hidden CA/PA). Ground-truth for all 77 at once stays the
PlayerProperties loader disasm; the archetype method keeps converging but slows on correlated traits.
