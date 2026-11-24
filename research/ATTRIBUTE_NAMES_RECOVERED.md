# ATTRIBUTE_NAMES_RECOVERED.md — the CM2008 player-attribute name vocabulary (A1)

**Status: names VERIFIED from the crack image + formula.xml. Exact `.dat` column-index
binding NOT yet done (needs the PlayerProperties loader — see "Remaining").**

This is the static recovery of *what the player attributes are called*, the blocker for naming the
77 `PlayerProperties` / 65 `GoalkeeperProperties` columns and for real per-facet TeamRating (P2/P5).
No value was invented; every name below is a literal string read from the game's own files.

## How it was recovered (static, no execution, no fabrication)

1. **Harvester `FUN_005d6dd0`** (unpacked `cm2008.exe`, image base 0x400000) is an unrolled sequence.
   Per attribute it does:
   ```
   call  <leaf 0x0081xxxx>     ; resolve + evaluate one named formula -> st0 (float)
   fstp  dword [esp]           ; pass the float
   mov   ecx, esi             ; this = the runtime attribute object
   call  0x005d6d50           ; convert float -> byte (al)
   mov   byte [esi + DISP], al ; store into the object's attribute slot
   ```
   Extracted with capstone: **88 (leaf -> dest-offset) pairs**, dest offsets 0x02..0x59.

2. **Each leaf is a named-formula resolver.** It pushes two `.rdata` string-constant pointers and
   calls **`0x00662cf0` = `GCFormulaModule::Find`** (confirmed by the lab). The two strings are the
   formula *group* and *name*, e.g. leaf `0x0081be30` pushes `"PlayerConstant"` + `"KeeperBravery"`.
   Extracting those constants for all 88 leaves yields the **derived-attribute catalog** below
   (85 `PlayerConstant` + 3 `PlayerDegradeable`).

3. **`formula.xml`** (decrypted, `DataXML/formula.xml`) defines each of those formulas. Their bodies
   read **raw `GCPlayer` attributes** via nested `<variable type="GCPlayer"><variable name="X"
   type="int"/>`. Collecting every distinct leaf-int name under a `GCPlayer` scope yields the
   **raw-attribute vocabulary** — the names of the raw columns — **70 distinct** (below).

The three layers: raw `GCPlayer` ints (the `.dat` columns) → `PlayerConstant`/`PlayerDegradeable`
formulas (derived, what the match engine consumes) → runtime attribute object slots `esi+DISP`.

## Layer 1 — Raw GCPlayer attributes (the `.dat` column vocabulary), 70 distinct — VERIFIED

```
Acceleration Adaptability Aggression Agility Ambition Anticipation AttackingMidfielder
Balance Bravery Central CentralDefender Concentration Condition Confidence Consistency
CoordinatingDefense Creativity Crosses Crossing CurrentAbility DeadBalls DecisionMaking
DefensiveMidfielder Determination Dribbling Eccentricity Finishing FirstTouch FitnessStat
Flair Forward FullBack Handling Heading InjuryResistance Jumping Kicking Leadership LeftFoot
LeftSide LongShots Marking Midfielder Morale Movement OneOnOnes Pace Passing PenaltyTaking
Positioning Pressure Professionalism RightFoot RightSide Roaming ShotStopping Sportsmanship
Stamina Strength Striker Sweeper Tackling TeamWork ThrowIns Throwing Versatility Vision
WingBack Winger WorkRate
```

Notes:
- Visible skills: Finishing, Tackling, Passing, Crossing, Marking, Heading, Dribbling, FirstTouch,
  LongShots, Positioning, Anticipation, Vision, Creativity, Flair, Determination, Bravery, WorkRate,
  Agility, Balance, Jumping, Pace, Acceleration, Strength, Stamina, etc.
- GK skills: Handling, Kicking, OneOnOnes, ShotStopping, Throwing, Crosses, CoordinatingDefense,
  Eccentricity (these are the ones used under keeper scopes).
- Footedness: LeftFoot, RightFoot (CM uses a 1..20 scale — matches the 14 `<=20`-ranged columns in
  the fingerprint, which also cover positional familiarities).
- Positional familiarity: Striker, Forward, AttackingMidfielder, Midfielder, DefensiveMidfielder,
  CentralDefender, Sweeper, FullBack, WingBack, Winger, Central, LeftSide, RightSide.
- Hidden/meta: CurrentAbility, Morale, Condition, Consistency, Pressure, Professionalism, Ambition,
  Determination, Sportsmanship, TeamWork, Leadership, Adaptability, Versatility, InjuryResistance,
  FitnessStat, Concentration, DecisionMaking, Confidence, Movement, DeadBalls, PenaltyTaking, Roaming.

Raw count 70 vs. on-disk columns 77 (outfield) / 65 (GK): the 70 is the set *referenced by
formulas*; a few on-disk columns are unreferenced (e.g. ids/reputation) and some are GK-only, so the
per-table column set is a subset/superset of this vocabulary. Binding each name to its column index
is the remaining step.

## Layer 2 — Derived PlayerConstant / PlayerDegradeable catalog — VERIFIED

`dest` = byte offset in the runtime attribute object (`esi+DISP`); the outfield and keeper branches
reuse the same slots, so a slot number appears once per branch. `leaf` = resolver address.

### PlayerDegradeable (3) — age-degradable
| dest | leaf | name |
|---|---|---|
| 0x57 | 0x0081ff70 | Pace |
| 0x58 | 0x00820090 | Acceleration |
| 0x59 | 0x008201b0 | Deceleration |

### PlayerConstant — keeper branch (14)
| dest | leaf | name |
|---|---|---|
| 0x02 | 0x0081be30 | KeeperBravery |
| 0x0d | 0x0081bf50 | KeeperRunningOut |
| 0x0e | 0x0081c070 | SmotheringAbility |
| 0x11 | 0x0081c190 | KeeperReach |
| 0x12 | 0x0081c2b0 | KeeperAgility |
| 0x17 | 0x0081c3d0 | KeeperRecovery |
| 0x18 | 0x0081c4f0 | KeeperGrip |
| 0x19 | 0x0081c610 | KeeperHandStrength |
| 0x1a | 0x0081c730 | KeeperCrosses |
| 0x1d | 0x0081c850 | KeeperKicking |
| 0x1e | 0x0081c970 | KeeperThrowing |
| 0x20 | 0x0081ca90 | KeeperPunching |
| 0x25 | 0x0081cbb0 | KeeperCoordinateDefence |
| 0x26 | 0x0081cdf0 | KeeperEccentricity |

### PlayerConstant — outfield branch (71)
ActingAbility, Adaptability, Aggression, Alertness, Ambition, AttackingMidAbility, BallControl,
BlockTackleStrength, BodyBalance, BodyStrength, CentralAbility, CentralDefenderAbility,
ClearanceAgilityLeft, ClearanceAgilityRight, ClearancePower, Consistency, CrossAccuracy,
CrossAgilityLeft, CrossAgilityRight, CrossRangeLeft, CrossRangeRight, CurlAbilityLeft,
CurlAbilityRight, DefensiveMidAbility, DivingHeader, DribbleAgility, Fitness, ForwardAbility,
FullBackAbility, Handball, HardTackleFrequency, HeadAccuracy, HeadRange, Leadership, LeftSideAbility,
Marking, MidfieldAbility, OverAggression, PassCaution, PassCreativity, PassingAccuracy, PhysicalPlay,
PlayerAbility, Professionalism, RangeTargetStepRight, Resilience, RightSideAbility, Roaming,
RunFrequency, ShirtPulling, ShootingBrain, ShootingFinesse, ShotAccuracy, ShotComposureLeft,
ShotComposureRight, Spring, StrikerAbility, SweeperAbility, TackleAvoidance, TackleCaution,
TackleChaseRange, TackleFrequency, TacklingAbility, ThrowAccuracy, ThrowPower, TrickDelivery,
TrickReading, Trickyness, Versatility, WingBackAbility, WingerAbility.

(Full `dest`/`leaf` per name in the session scratch dump; each is one `PlayerConstant`-group formula
in `formula.xml`, so its exact raw inputs are already readable there.)

## Cross-validation hook

`super-manager-engine/cmd/attrfp` fingerprints every raw column (sentinel%, min/max/mean/std,
distinct, deciles, `Scale20`). The 14 `<=20` columns line up with LeftFoot/RightFoot + positional
familiarities; the bell-shaped 0..100 columns line up with the graded skills above. When a
column-index binding is proposed it can be checked against these distributions immediately.

## Remaining to a full column→name map

1. **PlayerProperties loader** — the function that reads `PlayerProperties.dat` rows into the
   `GCPlayer` attribute fields gives `.dat column index -> field`. Join with the name→field registry
   (how `formula.xml` variable names resolve to `GCPlayer` offsets) for the exact per-index name.
   Best done with Ghidra xrefs on the unpacked `cm2008_full` project (capstone has no xref index).
2. Alternatively, `attrmap.Solve` (value-matching) seeded with this 70-name candidate set + a tiny
   real capture (a handful of named players' attributes read off the owned game UI) resolves indices
   uniquely without the loader.

Everything above is read from the owner's own files; no game data is committed and no names are
reproduced outside this research note.

## Related — 64 match-stat metric names (recovered by the lab session, for cross-reference)

A different layer from the player attributes above: these are the per-match **outcome metrics**
(resolver section-0, n=4,121,340 samples), not `.dat` player columns. Recovered by the lab session
(`cmd/native resolver stats`) and recorded here so the name vocabulary is not lost. They are stored
**alphabetically** in section-0, which (like the attribute UI lists above) means this order is NOT the
match stat-block struct order — the struct field order is a separate, still-open join (lab P3,
`FUN_005a6330`).

```
Assists ChallengesAvoided ChallengesMade Clearances CompletedPasses Condition Confidence Crosses
CrossesCompleted CrossesFumbled CrossesPunches CrossesTaken CynicalFouls DirtyFouls FailedToControl
Fouls FoulsCommited FoulsReceived FreeKickDirect FreeKickIndirect GoalsConceded GoalsScored
GroundCovered Headers HeadersKey HeadersWon Injured InjuryLevel Interceptions KeptPossession
KeyActions LostPossession MajorInjuries MinorFouls MinorInjuries MissControlled Morale NumberRedCards
NumberYellowCards Offsides OwnGoalsScored PassesCompleted PassesFailed PassesKey PassesReceived
PenGoalsScored Performance PoorCrosses RunsMade RunsMadeKey ScaledPerformance ShotsFumbled ShotsHeld
ShotsOffTarget ShotsOnGoal ShotsOnTarget ShotsParried ShotsTipped TacklesKey TacklesLost TacklesWon
Touches Tricks Warnings
```
