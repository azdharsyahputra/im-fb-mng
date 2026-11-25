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

## Third archetype pass — 34/77 bound

Added deep passers (Xabi Alonso), playmakers (Riquelme, Seedorf), engines (Nedved, Kuyt, Gerrard),
pure defenders (Nesta, Zambrotta), target men (Koller), a poacher (Villa). +7 HIGH bindings:

- col 18 Anticipation (Inzaghi/van Nistelrooy + defenders; Riquelme/Robben low)
- col 45 Positioning (Maldini/Cannavaro/Terry/Gerrard; pure forwards low)
- col 46 Passing (Scholes/Giggs/Gerrard/Totti; Crouch/Walcott low)
- col 54 Jumping (tall: Koller/van Nistelrooy/Vidić/Ferdinand; Walcott/Robben low) — **corrects** the
  earlier tentative col54=CentralDefender
- col 55 CentralDefender familiarity (Cannavaro/Nesta/Maldini/Terry; attackers ~20)
- col 60 DeadBalls (Scholes/Gerrard/Pirlo/Rooney set-piece takers; Inzaghi 24)
- col 61 PenaltyTaking (Totti/Villa/Riquelme/Alonso; sparse ~1%)

Bound total: 34/77 — 14 sliders + col 7 (VERIFIED) + 19 archetype-INFERRED
(playerdb.PlayerPropertyInferredColumn). Remaining ~43 are the tightly-correlated mental cluster
(Composure/DecisionMaking/Concentration/Creativity/Flair/WorkRate/Determination), a few physicals
(Stamina/Balance-variants), FirstTouch/Technique, and the rest of the positional-familiarity block —
each needs a sharper single-trait archetype or the loader disasm for ground truth.

## Loader disasm — machinery found, but NO static disk-order table (negative result)

Chased the ground-truth column order in the crack. Found the full attribute machinery:
- The 77-attribute schema is built at ~0xa7d1c0: for each attribute it allocates a 0xC-byte
  descriptor {vtable, nameString, accessorFnPtr} and stores it into a declaration-order global
  array at 0xc20c20[] (idx0=Heading … idx76=…).
- Each attribute's **accessor** (0x602d50=Heading, 0x605410=Tackling, …) reads its field from the
  runtime object. Disassembling all 77: the runtime object stores attributes in **declaration order**,
  one dword each (attr N at dword N×4, value in the high bits, `shr …,0x1c`). Bit-packed runtime,
  byte-aligned on disk.

The blocker: **the on-disk `.dat` column order is a permutation of declaration order with no static
table.** Evidence:
- Every accessor-pointer array in the image (0xc1fc48, 0xc1fc94, 0xc20288, and a second schema at
  0xa7eac0→0xc20b78) is in **declaration order** — none is disk order.
- The runtime bit-position does not map to the disk column (Heading bit28→col33, Marking bit188→col2,
  Tackling bit60→col22, Pace bit484→col34 — scrambled).
- The `.dat` header is 8 bytes (magic+count), no per-file column manifest.
- A second schema (0xa7eac0) binds the SAME accessor addresses to DIFFERENT names, so accessors are
  generic field-position readers, not attribute identities.

Conclusion: recovering the disk permutation statically would require tracing the live deserialisation
loop (read 77 u16 → setter calls) rather than reading a table — it is not a clean one-shot extract.
The archetype value-matching remains the practical route and is the source of the 34/77 bindings.
Full runtime accessor layout saved alongside (acc_layout_full).

## Fourth pass — coarse/fine split corrected; positional block bound (42/77)

Distinct-count classification revealed the on-disk block structure:
- cols 0..17 = COARSE (~11–19 distinct levels) = **positional familiarities + footedness**
- cols 18..62 = FINE (52–94 distinct) = graded 0..100 **skills**
- cols 63..76 = the <=20 instruction sliders (VERIFIED)

This corrects two earlier mis-reads that conflated a coarse positional with a fine skill:
- **col 2 = CentralDefender** (positional; CB group 100, others ~1) — was wrongly "Marking".
- **col 9 = WingBack** (positional; FB 87 + Wing 80) — was wrongly "Crossing".
- The real fine **Marking** skill is **col 55** (Nesta/Maldini/Cannavaro/Terry top, attackers ~20) —
  previously mislabelled CentralDefender.

Positional familiarities bound by position-group averages (CB/FB/Wing/Strik/DM/AM/CM archetypes):
col 0 AttackingMidfielder · col 2 CentralDefender · col 4 DefensiveMidfielder · col 5 Forward ·
col 9 WingBack · col 10 Midfielder · col 14 Striker · col 15 Sweeper · col 16 FullBack · col 17 Winger.

Bound total: 42/77 (15 VERIFIED: sliders + col7 ThrowIns; 27 INFERRED in
playerdb.PlayerPropertyInferredColumn). Remaining coarse cols (c1/c3/c7/c8/c11/c12) include foot
ratings (RightFoot is near-universally high) + a few positions (GoalKeeper/Roaming/LeftSide/
RightSide/Central); remaining fine cols are the correlated mental cluster + Crossing/FirstTouch/
Stamina/Concentration.

## Fifth pass — foot + mental cluster (51/77 bound)

Added foot (col8 LeftFoot, col11 RightFoot via left/right-footed archetypes) and the resolvable
mental/physical fine skills: col24 Stamina, col26 WorkRate, col35 Determination, col37 Movement,
col40 Bravery, col43 Aggression, col48 DecisionMaking (each by engine/workhorse/brave/smart
archetype top-rankers). col25's best fit (Composure) is not in the 77-name set, so it was left
unbound rather than forced.

**Bound: 51/77** — 15 VERIFIED (14 sliders + col7 ThrowIns) + 36 INFERRED
(playerdb.PlayerPropertyInferredColumn).

Remaining ~26 columns are the genuinely-correlated cluster that archetypes cannot uniquely separate:
- Technical/creative overlap: Creativity, Vision, Flair, FirstTouch, Technique, Crossing (fine) — all
  high for the same technical players.
- Personality overlap: Consistency, Professionalism, Pressure, Loyalty, Sportsmanship, Ambition,
  Adaptability, Versatility, Confidence, Leadership, Concentration, TeamWork — all high for the same
  pro/veteran players.
- Plus Fitness/NaturalFitness, InjuryResistance/RecoveryRate, and the rarer positional/foot slots
  (GoalKeeper familiarity, Roaming, LeftSide, RightSide, Central, PreferredFoot).
These would need either a single-trait capture per attribute or the live-deserialisation trace; the
main gameplay-relevant attributes (skills, physical, positions, foot, set-pieces) are all bound.

## Sixth pass — creative cluster; col44 Flair bound, rest resists (52/77)

Added Ronaldinho (flair), Xavi/Fàbregas (vision/creativity), Puyol, Lahm, Lennon. One clean new
binding: **col44 = Flair** (Ronaldinho 100 maxes it, Messi 95, centre-backs 15-19). The remaining
creative/technical cols could NOT be uniquely split — e.g. col53 (Pirlo/Robben/Ronaldinho/Riquelme)
and col62 (Ronaldinho/Totti/Riquelme + van Nistelrooy) mix pure creators with poachers, so
Creativity/Vision/FirstTouch/Technique overlap; col58 (Nesta/Maldini/Cannavaro/Puyol) is clearly a
defensive-focus attribute (Concentration candidate) but not uniquely nameable. MED candidates noted,
not committed.

**Reliable ceiling via archetypes: 52/77.** The ~25 unbound are the correlated creative +
personality attributes (Creativity, Vision, FirstTouch, Technique, Crossing, Concentration,
Consistency, Professionalism, Pressure, Loyalty, Sportsmanship, Ambition, Adaptability, Versatility,
Confidence, Leadership, TeamWork, Fitness, InjuryResistance/Recovery) and rare slots (GoalKeeper
familiarity, Roaming, LeftSide, RightSide, Central, PreferredFoot). Splitting these reliably needs a
single-trait capture per attribute or the live-deserialisation trace — not archetype value-matching.

## Seventh pass — TWO real captures: validate + correct (high confidence)

The owner supplied two genuine CM2008 profile screenshots with full 1..20 attribute values:
Bojan Krkic (Barcelona, id 143410) and "Jorda" (= Javier Martina, PSV, id 120728). Scale confirmed
**display*5 ≈ raw (±~5)**. Using both as ground truth:

- **Validated** ~20 archetype bindings to within a few points (Finishing, Dribbling, Heading, Marking,
  Pace, Acceleration, Agility, Balance, Stamina, Passing, Positioning, Anticipation, DecisionMaking,
  Flair, Bravery, Aggression, Jumping, LongShots, WorkRate). The archetype method is sound.
- **Corrected** six that the captures refuted (archetype value-match had them wrong):
  old c22 Tackling, c35 Determination, c37 Movement, c38 Strength, c60 DeadBalls, c61 PenaltyTaking.
- **Reassigned** where BOTH captures and archetypes agree: c19 Consistency, c29 Determination,
  c51 Crossing, c57 Movement, c58 Tackling, c62 PenaltyTaking.

Key lesson: two captures alone match some columns by coincidence (a raw pair 75/60 fits several
attributes), so a binding is only committed when the captures AND the 38-player archetype signal
agree. Columns where they conflict (Creativity/Vision/FirstTouch/TeamWork/Leadership overlap) stay
unbound — each needs another capture with a distinctive value to triangulate. More single-attribute
captures would keep closing the gap; the method now has a validated scale and two anchors.

## Eighth pass — correlation signatures on the full 76,051-record corpus (no captures needed)

A different, capture-free lever: instead of per-player archetypes, compute per-column Pearson
correlations across ALL 76,051 rows and read each unbound column by its correlation *signature*
against already-bound anchor columns. This pinned three of the long-standing unbound rare positional
slots and forced a correction on the col60/61 set-piece pair. (Script: scratch `corr.py`/`corr2.py`;
all from the owner's file, nothing committed.)

Authoritative-name recheck: the 62-name GCPropertyPlayer **debug dumper FUN_00609ff0 @0x609ff0**
("---GCPropertyPlayer Output Start---") was decompiled as a second witness to the schema-registration
names — it prints each attribute as a 7-bit field (`&0x7f`, 4 per 32-bit dword) in runtime-struct
order, re-confirming the name SET (and that LeftFoot/RightFoot are 0..100 ABILITIES; PreferredFoot is
a separate schema field, field 62, not a dumped runtime field).

### Pinned (UPGRADED L→H)
- **col 1 = Central** (positional familiarity). corr with central positions all positive — Striker
  +0.32, CentralDefender +0.29, DefMid +0.18, AttMid +0.16 — and negative with wide — FullBack -0.41,
  Winger -0.15. Unique "plays centrally" signature among the unbound slots {Central, LeftSide,
  RightSide, Roaming, GoalKeeper}.
- **col 6 = LeftSide** and **col 12 = RightSide**, separated decisively by footedness:
  corr(col6, LeftFoot col8) **+0.25** vs corr(col6, RightFoot) -0.04; corr(col12, RightFoot col11)
  **+0.33** vs corr(col12, LeftFoot) -0.09. Left-side familiarity tracks left-footedness and vice
  versa — the clean discriminator the earlier passes lacked.

### Corrected (set-piece pair; resolves the 7th-pass "col60 DeadBalls refuted" dangling thread)
- This file is **outfield-only**: PlayerProperties ids (76,051) ∩ GoalkeeperProperties ids (8,984) =
  **0 overlap**. So a column cannot be a keeper rating, and even the GoalKeeper *familiarity* (schema
  field 48, "can an outfielder deputise") is NOT the 98.8%-sentinel col61.
- **col 61 = DeadBalls** (was "GoalKeeper"). col61 is ~1% populated and pairs **+0.746 with
  PenaltyTaking (col62)**, +0.65 Crossing, +0.63 LongShots, +0.57 col60, -0.37 CentralDefender — the
  textbook dead-ball/free-kick specialist profile, co-populated with the penalty takers. DeadBalls and
  PenaltyTaking are the two schema set-piece fields and here they are the two rare (~900-player) cols
  61/62.
- **col 60 = UNRESOLVED (X)** — the 7th pass already refuted col60=DeadBalls via captures; with
  DeadBalls now placed at col61, col60's identity is open (it is a normal 0..100 attacking-technique
  bell: corr Flair +0.46, Finishing +0.45, LongShots +0.44, Crossing +0.37). Left unbound, not guessed.
- **GoalKeeper familiarity (schema 48): now UNBOUND**, honestly, rather than mis-slotted at col61.

### Checked but NOT upgraded (stay LOW — honest)
- col 25 TeamWork: corr(Determination col29) +0.63 only places it in the attitude cluster; cannot be
  split from the other hidden mentals (the 7th pass' best fit was "Composure", not in the name set).
- col 49 Versatility: corr with #positions-played +0.12 — right direction, too weak to pin.
- col 3 Roaming, col 13 PreferredFoot (weak-foot rating: low mean 6, ~0 foot correlation — consistent
  with a real but undistinctive schema field), col 50/59 injury Resistance/RecoveryRate, and the rest
  of the hidden-personality cluster: no distinctive on-pitch correlate exists in attribute values, so
  they need a single-trait capture per attr or the live-deserialisation trace.

Net effect on `super-manager-engine/playerdb/fullmap.go`: +3 H (Central/LeftSide/RightSide),
col61 L→DeadBalls(M), col60 M→X(unresolved), GoalKeeper dropped (unbound). Name vocabulary now cited
to BOTH the schema-registration fn and the dumper. Confidence tier "X" (proven-wrong, not re-pinned)
added.

## Ninth pass — live/static deserialisation trace: EXHAUSTIVE NEGATIVE RESULT

Goal: recover the full disk-column permutation by tracing the PlayerProperties deserialiser (read 77
u16 -> setter calls), which would make all 77 columns VERIFIED at once. Result: the permutation is
**not statically recoverable**, proven on every remaining front.

### The schema (.inc) ships NOWHERE
The disk order is defined by `data/pc/data/structure/PlayerProperties.inc`. Confirmed absent from all
three possible sources:
- **exe**: the `.inc` path string @0xb88958 is referenced only by FUN_00665610, which passes it to the
  assert/error reporter FUN_0042dc90 together with `"GCRawTableReadEntry"` + line `0x16` — i.e. it is a
  **source-location label for asserts, not a runtime file open**. The schema itself is compiled in.
- **extracted game**: no `*.inc`, no `Structure/` dir anywhere under `extracted_game/` (and none in
  DataXML/DataMisc/_Support_Build_Generated_Support_Files).
- **installer**: `unshield l Setup/data1.cab` lists **2511 files, zero `.inc`** (only
  `DatabaseFiles\PlayerProperties.dat`). The InstallShield cabs do not carry the schema sources.

### The exe has declaration order, not disk order (re-confirmed)
- Schema-registration (~0xa7d1c0) builds 0xC-byte descriptors in **declaration order** (Heading=0 …).
  Disk order is a different permutation — e.g. Heading is declaration-field 0 but archetype-verified at
  **disk col 33**; disk col 0 is AttackingMidfielder. So disk != declaration order (settled).
- Every accessor-pointer array in the image (0xc1fc48/0xc1fc94/0xc20288/0xc20b78) is in declaration
  order; none is disk order (prior 8th-"Loader disasm" pass). The `.dat` header is 8 bytes
  (magic+count) with no per-column manifest. The generic `GCRawTable` reader maps columns by the
  (runtime-built-from-.inc) descriptor list, so with the `.inc` compiled-in-but-not-emitted-as-a-table
  there is no static array to read the permutation from.

### Runtime trace not available
The target is a Windows PE; this is a clean-room **static** reconstruction on macOS — the binary is not
executed (and REA `open_binary` runs on the committed decompilation, not a live process). A Frida/gdb
deserialiser trace would resolve it on Windows but is out of scope here.

### External-fact lever also blocked
Correlating hidden mentals (Loyalty/Ambition/Professionalism rise or fall with age) against player age
needs a confirmed DOB/age field. Player.dat field names are UNKNOWN (DATABASE_SCHEMA_MAP: only id@0 and
an f64@16 are named) and Person DOB is not pinned, so the join would be speculative — not pursued.

### Honest leftovers (need a per-attribute capture OR a Windows runtime trace)
- **col60** identity (DeadBalls moved to col61; col60 is a normal attacking-technique bell, name open).
- **GoalKeeper familiarity** (schema field 48): UNBOUND. In the coarse positional block (cols 0..17)
  only col3 and col13 remain LOW, and **col13's low mean (6) fits a "can deputise in goal" 0..100
  rating as well as it fits PreferredFoot (weak-foot)** — genuinely ambiguous, left unchanged.
- The hidden-mental intra-cluster order (TeamWork/Loyalty/Ambition/Pressure/Professionalism/
  Sportsmanship/Adaptability/Confidence/Versatility): no distinctive on-pitch or external correlate.

**Bottom line:** the attribute-value + correlation method has reached its ceiling. The only ways to
close the remaining ~15 columns are (a) a few single-attribute captures off the game UI (the owner's
route — fastest), or (b) a live-process deserialiser trace on Windows. No static artifact in the exe or
shipped data contains the disk permutation.

## Tenth pass — real capture (Aaron Ramsey) via DOB join; detailed/non-detailed split proven

Owner supplied a CM2008 profile + information capture: **Aaron Ramsey, Cardiff, age 16, DOB 26-12-1990**,
Midfielder (Centre), Preferred Foot Right, with full 1..20 visible attributes.

### Record located by DOB, not by value-match
Value-matching his visible skills failed (his values are mostly 10-13 → raw ~50-65, i.e. the population
mean — thousands of records fit). Instead joined on the hard DOB: the byte pattern `1a 0c c6 07`
(day=26, month=12, u16 year=1990) occurs **exactly once** in Person.dat (102,990 persons, 34-byte
records, DOB at record offset 16) → **person id 154675**. That id's PlayerProperties record has a
central-midfielder, right-footed positional profile — confirmed Ramsey.

### KEY FINDING — non-detailed players store NO skill values (runtime-generated)
Ramsey's PlayerProperties record has **cols 18..76 entirely SENTINEL (0xffff)**; only the intrinsic
position/foot cols are populated:

| col | raw | reading | check |
|---|---|---|---|
| 0 | 79 | AttackingMidfielder familiarity | box-to-box mid ✓ |
| 1 | 100 | Central | centre player ✓ |
| 2 | SENT | CentralDefender | a midfielder lacks it ✓ |
| 4 | 79 | DefensiveMidfielder | box-to-box mid ✓ |
| 8 | 60 | LeftFoot ability | weaker foot |
| 10 | 100 | Midfielder | natural CM ✓ |
| 11 | 100 | RightFoot ability | **preferred foot Right = col11>col8 ✓** |

So the UI skills (Finishing 13, Passing 16, …) for a lower-league/young player are **generated at
runtime, not stored** — this IS the ~83% sentinel skill block. Consequence for the engine: non-detailed
players need attribute GENERATION (from CA/PA + position), not a disk read. And the **capture-solve
method only works on DETAILED (top-club) players** whose skill block is populated (e.g. the earlier
Bojan/Martina captures). A lower-league capture cannot pin skill columns.

### What the capture DID confirm / correct
- **Positional + foot block holds against a real central midfielder**: col0 AttMid / col1 Central /
  col4 DefMid / col10 Midfielder set; col2 CentralDefender + the wide/side slots (6,12) + Striker/
  Winger/etc. sentinel — exactly a centre-mid. col11 RightFoot (100) > col8 LeftFoot (60) reproduces
  the UI "Preferred Foot: Right".
- **col13 is NOT PreferredFoot** (now Unknown13/X): it is SENTINEL for Ramsey although his preferred
  foot is defined — an intrinsic property would be stored like col8/col11. Preferred foot is derived
  (argmax of the two foot abilities); the schema's "PreferredFoot" field maps to an unidentified column
  or is computed. This agrees with the 8th-pass ~0 foot-correlation for col13.

### Net
fullmap.go: col13 PreferredFoot(L) → Unknown13(X). Positional/foot bindings (col0/1/2/4/8/10/11)
reinforced by a real capture. Skill/mental columns still need a **top-club player capture** (fastest) —
a lower-league player's skills are not on disk.

## Eleventh pass — 3-detailed-player capture solve corrects 7 swapped columns

The owner sent a batch of CM2008 profile captures (sibling session). Usable DETAILED outfield players:
**Fernando Torres (Liverpool, id 48489)** and **Luka Modric (Dinamo Zagreb, id 134772)**, both located by
DOB join in Person.dat (Torres 20-03-1984, Modric 09-09-1985; disambiguated among same-DOB persons by
"detailed + position"). Combined with the known **Bojan (id 143410)**, that is 3 full (fine=43) vectors.
(Also in the batch: Bojan/Zarate re-shots already used; 2 GOALKEEPERS — Ventura/Grohe — for the separate
GoalkeeperProperties map; and one non-CM2008 screenshot, discarded.)

Method: per disk column, the (Torres, Modric, Bojan) raw triple vs each visible attribute's display*5
triple. ~Most existing bindings reproduced all 3 within a few points (Finishing 23, Dribbling 52,
Marking 55, Tackling 58, Bravery 40, Determination 29, Acceleration 27, Pace 34, Anticipation 18,
Flair 44, Agility 28, DeadBalls 61, PenaltyTaking 62 — so the map is ~85% right). But a **cluster of 7
columns was HARD-REFUTED and re-pinned** (old binding impossible for at least one player; new binding
reproduced all 3, Modric/Bojan usually exact):

| col | was | triple (T,M,B) | now | why old is impossible |
|---|---|---|---|---|
| 22 | Strength | (37,45,60) | **Positioning** | Torres Strength 20→100, col22=37 |
| 33 | Heading | (73,45,75) | **Jumping** | Modric Jumping 9→45 = col33; Heading 11→55 ≠45 |
| 36 | Aggression | (92,70,60) | **Strength** | Torres Aggression 13→65, col36=92 |
| 38 | Ambition | (63,65,75) | **Aggression** | exact Aggression (65,65,75); Ambition hidden/guess |
| 45 | Positioning | (80,83,60) | **Leadership** | Torres Positioning 9→45, col45=80 |
| 50 | InjuryResistance | (40,77,75) | **Versatility** | Torres Versatility 8→40 = col50 exactly |
| 54 | Jumping | (82,54,75) | **Heading** | pairs with col33: Heading/Jumping were swapped |

Heading↔Jumping (33/54) were simply transposed; the other five form a Strength/Positioning/Leadership/
Aggression reshuffle. Displaced: **col32** (was Leadership) and **col49** (was Versatility) now hold an
unidentified attribute each → marked Unknown/X; the two homeless hidden names (Ambition, InjuryResistance)
are unlocated. col61 DeadBalls confirmed (M→H).

Ported to fullmap.go (all 7 → confidence H; col32/col49 → X). Bojan wire test still passes (reads by
name). Limitation: all 3 captures are attack-minded, so the technical cluster (Passing/Movement/Vision/
Creativity/FirstTouch) and the hidden-personality cluster are not fully separable — a defender or a
physically-contrasting capture would tighten them. KEEPER captures (Ventura/Grohe) remain available to
map GoalkeeperProperties.dat (separate file / gknames.go) in a future pass.

## Twelfth pass — GoalkeeperProperties.dat structure (keeper captures non-detailed)

Owner's batch included two keeper captures (Ventura/Porto, Grohe/Gremio). Checked them against
**GoalkeeperProperties.dat** (8984 recs x 134B = u32 id + **65 u16 cols**). Both are NON-DETAILED
(record nset = 1..2 of 65 — GK skills runtime-generated, like Ramsey), so they cannot bind GK columns.
Confirmed the other session's note.

Measured on-disk structure (useful groundwork):
- **cols 0..17** coarse positional familiarities + feet (ndist<=9). **col7 = 100 for EVERY keeper** =
  the GoalKeeper position familiarity (=20), VERIFIED. col11 = RightFoot ability (mean ~91).
- **cols 18..62** the 45 FINE graded attributes (GK-technical + shared mental/physical).
- **cols 63..64** the 2 keeper instruction sliders (only <=15 cols) = {StayOnLineOrRushOut,
  PunchCrossesOrCatch}, VERIFIED as the pair.

**Shared-layout evidence (strong):** the fine block appears to reuse PlayerProperties' disk-column order
for shared attributes — the outfield-skill columns keepers are poor at are low-mean at the SAME indices:
col20 (FirstTouch) 28, col23 (Finishing) 42, col51 (Crossing) 38, col52 (Dribbling) 33, col53
(Creativity) 39. So ONE detailed keeper capture would transfer the ~37 shared columns from the
(capture-corrected) PlayerProperties map and leave only the 8 GK-technical columns (ShotStopping/
Handling/CoordinatingDefense/Crosses/Kicking/Throwing/OneOnOnes/Eccentricity) to pin. 1586/8984 keepers
are detailed, so a top-club keeper (Casillas/Buffon/Van der Sar/Cech) would complete it. Groundwork
recorded in playerdb/gknames.go.

## Thirteenth pass — Torres excluded (post-season capture); disk-order manifest hunt = runtime-built

**Torres capture INVALID for the .dat:** his screenshot is dated **24-05-08** (end of season 1), so his
attributes had developed away from the shipped **01-07-07** `PlayerProperties.dat`. Excluded. The 7
column corrections from the 11th pass were **re-confirmed on Modric + Bojan alone** (both captured
01-07-07 = the .dat state), near-EXACT (Δ0 on most): col22=Positioning, col33=Jumping, col36=Strength,
col38=Aggression, col45=Leadership, col50=Versatility, col54=Heading all HOLD; the old bindings are
hard-refuted on these valid players too (e.g. col45-as-Positioning needs Modric 9→45 but reads 83). So
the corrections stand on valid data; Torres was redundant.

**Disk-order manifest — searched everywhere, it is RUNTIME-BUILT (not a static table):**
The engine must map each disk column to a field to work (correct). Where that map lives:
- **exe strings:** the 77 attribute-name strings exist in **two blocks sorted ALPHABETICALLY** (e.g.
  0xb79f48: PenaltyTaking, DeadBalls, WorkRate, Vision, Versatility… = reverse-alpha) — these are a
  name→id LOOKUP table, NOT disk order. String address order != disk order.
- **descriptor arrays** (0xc20b78 second schema, 0xc20c20 first schema): **zero-filled in the static
  image** — they are populated at STARTUP by the schema-registration code. So the live disk→field table
  exists only once the game is running.
- the `PlayerProperties.inc` path in the exe is only an **assert label** (passed to the error reporter),
  not a runtime file open; and the `.inc` ships nowhere (exe/game/installer) — so the game does NOT read
  it at runtime; the order is baked into the GCRawTable init.
Conclusion: the map is real but **materialises only at runtime** (GCRawTable descriptor list built at
startup). Reading it statically needs a full trace of that init / the deserializer's disk-offset→bit-
position writes (deep, not a one-shot extract — matches the 8th-pass loader-disasm negative result). A
**live debugger on Windows** would dump it directly; in clean-room static, **valid start-dated captures**
remain the practical bridge (already pinning ~62/77 columns).

## Fourteenth pass — AUTHORITATIVE 77/77 map recovered from the loader (supersedes all capture work)

The disk→name map IS statically recoverable after all — from the loader's ASSEMBLY (Ghidra's pseudocode
of it is misleading). **FUN_0072cb40 @0x72cb40** reads the 158-byte record (u32 id + 77 u16) via
GCRawTableReadEntry (FUN_00665610 → flat 0x9e-byte read) and, in its body 0x72cdf9–0x72d79d, unpacks each
column positionally: **disk col i = word [ESP+0x88+2*i]**, bit-packed into runtime dword k shift s with
`dst ^= ((col<<s) ^ dst) & (0x7f<<s)`; (dword,shift)→name via the GCPropertyPlayer dumper @0x609ff0. The
recovered map **validates 35/35 against Bojan** (id 143410, raw ≈ display×5). This is the game's own
binding — authoritative, no runtime, no captures needed. Ported verbatim to playerdb/fullmap.go (all V).

FULL MAP (disk col → name): 0 AttackingMidfielder, 1 Central, 2 CentralDefender, **3 = youth-regen CONTROL
field (not an attribute)**, 4 DefensiveMidfielder, 5 Forward, 6 FullBack, **7 GoalKeeper**, 8 LeftFoot,
9 LeftSide, 10 Midfielder, 11 RightFoot, 12 RightSide, 13 Roaming, 14 Striker, 15 Sweeper, 16 WingBack,
17 Winger, 18 Anticipation, 19 Concentration, 20 Creativity, 21 DecisionMaking, 22 Positioning,
23 Movement, 24 TeamWork, 25 Vision, 26 WorkRate, 27 Acceleration, 28 Agility, 29 Balance, 30 Fitness,
31 InjuryRecoveryRate, 32 InjuryResistance, 33 Jumping, 34 Pace, 35 Stamina, 36 Strength, 37 Adaptability,
38 Aggression, 39 Ambition, 40 Bravery, 41 Confidence, 42 Consistency, 43 Determination, 44 Flair,
45 Leadership, 46 Loyalty, 47 Pressure, 48 Professionalism, 49 Sportsmanship, 50 Versatility, 51 Crossing,
52 Dribbling, 53 FirstTouch, 54 Heading, 55 Marking, 56 Passing, 57 Finishing, 58 Tackling, 59 ThrowIns,
60 LongShots, 61 DeadBalls, 62 PenaltyTaking, 63..76 the 14 instruction sliders (4-bit nibbles in dwords
21..23). There is NO PreferredFoot column (preferred foot = argmax(LeftFoot col8, RightFoot col11)).

Corrections vs the prior capture map (loader is authoritative; captures swapped value-indistinguishable
pairs): **col7 = GoalKeeper, not ThrowIns** (ThrowIns=col59; GK familiarity ≈1 in an outfield file =
same "rare" shape that fooled the signature guess); **col3 = control field, not Roaming** (Roaming=col13);
col23=Movement (Finishing=col57); col29=Balance (Determination=col43); col6=FullBack (LeftSide=col9); and
the whole technical/mental cluster (cols 19-25, 31-32, 37, 39, 42, 46-49, 53, 56, 60) which captures could
not separate. The capture pass was right only on distinctive-spread attrs (Positioning, Jumping, Strength,
Aggression, Leadership, Versatility, Heading, DeadBalls). **All 77 are now VERIFIED by the loader + Bojan.**
