# PlayerProperties schema field names (P5/A1 anchor) — recovered static from the crack

The PlayerProperties schema-registration function (crack VA **~0xa7d1e1** onward) registers each field
in declaration order: `push <fieldNameStr>; call 0x405530; mov [0xc739xx], eax`. Harvesting the pushed
string pointers yields **exactly 77 field names**, matching the 77 u16 columns of PlayerProperties.dat
(record = u32 id + 77 u16). The string "PlayerProperties" (0xb7a21c) is pushed in the same builder,
confirming this is that table's schema. NAMES are VERIFIED (the game's own schema strings).

## The 77 field names, in schema declaration order
| # | name | # | name | # | name |
|---|---|---|---|---|---|
|0|Heading|26|InjuryResistance|52|Midfielder|
|1|Tackling|27|Agility|53|AttackingMidfielder|
|2|Dribbling|28|InjuryRecoveryRate|54|Striker|
|3|Crossing|29|Determination|55|WingBack|
|4|Finishing|30|Professionalism|56|Roaming|
|5|Marking|31|Pressure|57|FullBack|
|6|FirstTouch|32|Loyalty|58|Winger|
|7|ThrowIns|33|Sportsmanship|59|Forward|
|8|Passing|34|Consistency|60|DeadBalls|
|9|Anticipation|35|Flair|61|PenaltyTaking|
|10|Creativity|36|Leadership|62|PreferredFoot|
|11|DecisionMaking|37|Aggression|63|DropDeepOrPushUp|
|12|Movement|38|Bravery|64|LookForOverlapsOrSitBack|
|13|Positioning|39|Confidence|65|CarryBallOutOfDefenceOrGetRidOfBall|
|14|TeamWork|40|Versatility|66|StayOnFeetOrLookToDive|
|15|WorkRate|41|Adaptability|67|HoldPositionOrBombAround|
|16|Concentration|42|Ambition|68|DistributeToWingsOrPlayThroughMiddle|
|17|Vision|43|LeftFoot|69|SwitchFlanksStretchPlayOrLookForSimplePass|
|18|Fitness|44|RightFoot|70|HoldUpTheBallOrGoForGoal|
|19|Acceleration|45|RightSide|71|RoundTheKeeperOrShoot|
|20|Balance|46|LeftSide|72|DriftOutWideOrStayCentral|
|21|Jumping|47|Central|73|CutInsideOrHugTouchline|
|22|Pace|48|GoalKeeper|74|RunWithBallOrLookToReleaseOthers|
|23|Stamina|49|Sweeper|75|PassBackwardsOrPassForwards|
|24|Strength|50|CentralDefender|76|LongRangeShootingOrWorkOpenings|
|25|LongShots|51|DefensiveMidfielder| | |

## Status
- The 77 field NAMES (and their schema declaration order) — **VERIFIED** (schema registration fn).
- "schema order == on-disk .dat column order" — **NOT CONFIRMED**: a naive index map fails to validate
  against real PlayerProperties.dat (e.g. taking col48 as GoalKeeper gives 99.8% high values, not the
  keeper-sparse shape expected; per-column non-sentinel counts vary widely). So the 77 are the exact
  candidate NAME SET and a likely order; the precise column↔name binding needs value-matching
  (attrmap.Solve / the 76k-player fingerprints — owned by the A1/coordinator work).

This closes the "what are the names" half of A1 statically; the column-binding half is value-matching.

## Column-shape cross-check vs real PlayerProperties.dat (76,051 records) — PARTIAL binding
Profiling every on-disk u16 column's shape confirms the schema order for the TAIL and one sparse attr,
but leaves the main block for value-matching:
- **cols 63–76 CONFIRMED** = the 14 "…OrX…" tendency fields (schema 63–76): all max=15, distinct≈15,
  ~99.5% sentinel. The count (14) and 0–15 range match exactly.
- **cols 61–62 CONFIRMED-ish** = PenaltyTaking / PreferredFoot (schema 61–62): ~98.8% sentinel (set for
  few players).
- **col 7 CONFIRMED** = ThrowIns (schema 7): 99.9% of values ≤2, max 79 — a rare specialist attribute.
- **cols 18–60**: uniformly ~82% sentinel (⇒ ≈18% ≈ 13.7k real players carry attributes; the rest are
  empty person slots) and near-continuous 0–100 (distinct 80–98). A consistent attribute block, but the
  naive map fails a spot-check (col48-as-GoalKeeper → 99.8% of non-sentinel ≥15, not keeper-sparse), so
  the 0–60 on-disk order may be permuted vs schema and/or positions use a different scale.
- **cols 0–17**: ~48–73% sentinel, distinct≈24 (quantized) — structurally distinct from 18–60.

**Verdict:** names VERIFIED; on-disk order CONFIRMED for cols 7 and 61–76; the 0–60 block binding needs
value-matching against real captures / the 76k-player fingerprints (A1/coordinator tooling).
