# CM2008 Formula Consumer Map — All 810 Formulas → Consumers + Inputs

Clean-room, **static / bytes-only** (no Ghidra, no execution, no payload decryption). This is the consolidated per-name table joining three VERIFIED byte-level sources with a name-based match-family classification. **No coefficient or formula body is invented** — every default is a byte-bound `DAT_` float read from `cm2008.exe`; `compute-only` means the getter has `DAT_None` (no fallback constant) and is meaningful only when the encrypted `formula.xml.blowfish` tree is present.

## Provenance & classification

| Column | Source | Status |
|---|---|---|
| name / group / getter FUN / default | `scratchpad/getters_full.txt` (810 rows) | **VERIFIED** (defaults spot-checked vs binary bytes: `DAT_00ba1714`=100.0, `DAT_00b5b114`=0.9, `DAT_00b5b5ec`=0.3, `DAT_00b5b5d4`=0.7, `DAT_00b3f894`=1000.0) |
| consumer FUN | `scratchpad/consumers.json` (E8 rel32 call scan, 810/810 resolved) | **VERIFIED** |
| ResolverSettings offset | `scratchpad/harvester_9f4bd0_offsets.txt` (300 `fstp [esi+disp]` stores) | **VERIFIED** |
| match family | name-keyword classifier over match groups | **INFERRED** (names + `FORMULA_BODY_RECOVERY.md` family map) |
| formula body / tree / weights | — | **UNKNOWN** (encrypted payload; not recovered, not invented) |

### The two centralizers (from `FORMULA_BODY_RECOVERY.md`, VERIFIED)

- **`FUN_005d6dd0`** (`__thiscall`, 2nd arg = **Player\***): harvests **105 per-player attribute getters** (101 `PlayerConstant` + degradable/fluid locomotion attrs) into a per-player scaled-attribute array. Input = a Player; default for essentially all = **100**.
- **`FUN_009f4bd0`** (`__thiscall`, called at match init after `__controlfp`): harvests **359 global `void` getters** into one ~0x5e0-byte **ResolverSettings** struct via `fstp dword [esi+disp32]`. Input = none (global tuning). Offset column = that `disp32`; `None` = read but used inline, not cached.
- **direct**: 346 getters (the non-match career/AI/finance families) each have their own single career-code caller (e.g. `FUN_007a02f0`, `FUN_007479e0`); not harvested into either match struct.

## Per-family counts & usable-now vs needs-payload split

| # | Family | Formulas | Usable now (has default) | Blocked (compute-only) | Via 009f4bd0 | Via 005d6dd0 | Direct |
|---|---|--:|--:|--:|--:|--:|--:|
| 1 | Movement/Acceleration | 103 | 87 | 16 | 80 | 23 | 0 |
| 2 | Passing/Receiving | 44 | 36 | 8 | 35 | 9 | 0 |
| 3 | Shooting/Finishing | 90 | 54 | 36 | 75 | 15 | 0 |
| 4 | Tackling/Defensive (incl. goalkeeping) | 113 | 75 | 38 | 85 | 28 | 0 |
| 5 | Positioning/Awareness | 67 | 60 | 7 | 37 | 30 | 0 |
| 6 | Player Performance/Match Rating | 47 | 45 | 2 | 47 | 0 | 0 |
| 7 | Other (career/AI/finance/non-match) | 346 | 157 | 189 | 0 | 0 | 346 |
| | **TOTAL** | **810** | **514** | **296** | 359 | 105 | 346 |

- **Usable now = 514 / 810** have a byte-bound default and so can seed the engine immediately (the fail-open value the shipped engine uses when a formula is absent from the registry). **Blocked = 296 / 810** are `compute-only` (`DAT_None`): no default exists, so their value is fully payload-dependent.
- For the **6 match families: 464 formulas** (357 usable now / 107 blocked). The match-critical blocked pockets are **all 26 `ResolverConstants` ball/shot physics**, the **5 base-gait accel/decel multipliers**, and the slide/hook/trip/elbow/smother **deflection/effect/time** defensive sub-formulas plus the `OfficialState` foul-rating set.

## 1. Movement/Acceleration — 103 formulas (87 usable / 16 compute-only)

| name | group | getter FUN | consumer | ResolverSettings off | default | input |
|---|---|---|---|---|---|---|
| Acceleration | PlayerDegradeable | FUN_00820090 | FUN_005d6dd0 | — | 100 | Player |
| BackingAccelerationMultiplier | MatchResolver | FUN_00830be0 | FUN_009f4bd0 | esi+0x154 | 0.9 | none (global) |
| BackingDecelerationMultiplier | MatchResolver | FUN_00830cd0 | FUN_009f4bd0 | esi+0x158 | 0.2 | none (global) |
| BestFeint | PlayerData | FUN_00837990 | FUN_009f4bd0 | esi+0x28c | 3 | none (global) |
| BodyBalance | PlayerConstant | FUN_00819e30 | FUN_005d6dd0 | — | 100 | Player |
| BodyStrength | PlayerConstant | FUN_00819d10 | FUN_005d6dd0 | — | 100 | Player |
| ClearanceAgilityLeft | PlayerConstant | FUN_0081bbf0 | FUN_005d6dd0 | — | 100 | Player |
| ClearanceAgilityRight | PlayerConstant | FUN_0081bd10 | FUN_005d6dd0 | — | 100 | Player |
| CollisionInjuryChance | PlayerData | FUN_008347e0 | FUN_009f4bd0 | esi+0x1c4 | 0.4 | none (global) |
| CrossAgilityLeft | PlayerConstant | FUN_0081b890 | FUN_005d6dd0 | — | 100 | Player |
| CrossAgilityRight | PlayerConstant | FUN_0081b9b0 | FUN_005d6dd0 | — | 100 | Player |
| CutInDistance | PlayerData | FUN_008356e0 | FUN_009f4bd0 | esi+0x1f8 | 28 | none (global) |
| Deceleration | PlayerDegradeable | FUN_008201b0 | FUN_005d6dd0 | — | 100 | Player |
| DiveDistance | PlayerData | FUN_00836e50 | FUN_009f4bd0 | esi+0x25c | compute-only | none (global) |
| DivingLeftAccelerationMultiplier | MatchResolver | FUN_00831720 | FUN_009f4bd0 | esi+0x184 | 6 | none (global) |
| DivingLeftDecelerationMultiplier | MatchResolver | FUN_00831810 | FUN_009f4bd0 | esi+0x188 | 8 | none (global) |
| DivingRightAccelerationMultiplier | MatchResolver | FUN_00831ae0 | FUN_009f4bd0 | esi+0x194 | 6 | none (global) |
| DivingRightDecelerationMultiplier | MatchResolver | FUN_00831bd0 | FUN_009f4bd0 | esi+0x198 | 8 | none (global) |
| DribbleAgility | PlayerConstant | FUN_00819770 | FUN_005d6dd0 | — | 100 | Player |
| DribbleCaution | PlayerConstant | FUN_00818450 | FUN_005d6dd0 | — | 100 | Player |
| DummyAngle1 | PlayerData | FUN_008395b0 | FUN_009f4bd0 | None | compute-only | none (global) |
| DummyAngle2 | PlayerState | FUN_008446e0 | FUN_009f4bd0 | None | compute-only | none (global) |
| EaseOfDribble | PlayerData | FUN_008358c0 | FUN_009f4bd0 | esi+0x200 | 45 | none (global) |
| FeintRange | PlayerData | FUN_00835e60 | FUN_009f4bd0 | esi+0x20c | compute-only | none (global) |
| FeintRangeStart | PlayerData | FUN_00835d70 | FUN_009f4bd0 | esi+0x208 | 5 | none (global) |
| Fitness | PlayerConstant | FUN_0081aa90 | FUN_005d6dd0 | — | 100 | Player |
| HalfVolleyHeight | PlayerData | FUN_0083dcf0 | FUN_009f4bd0 | esi+0x380 | 0.6 | none (global) |
| HeadAgility | PlayerData | FUN_00839f10 | FUN_009f4bd0 | esi+0x32c | 0.25 | none (global) |
| Height | PlayerConstant | FUN_0081cf10 | FUN_005d6dd0 | — | 100 | Player |
| HighCrossHeight | PlayerData | FUN_00839970 | FUN_009f4bd0 | esi+0x314 | 1.75 | none (global) |
| HighGoalHeight | OfficialState | FUN_00841e90 | FUN_009f4bd0 | esi+0xc0 | 1.5 | none (global) |
| JoggingAccelerationMultiplier | MatchResolver | FUN_00830640 | FUN_009f4bd0 | esi+0x134 | 1.5 | none (global) |
| JoggingDecelerationMultiplier | MatchResolver | FUN_00830730 | FUN_009f4bd0 | esi+0x148 | compute-only | none (global) |
| LollyPopAngle1 | PlayerData | FUN_008394c0 | FUN_009f4bd0 | None | compute-only | none (global) |
| LollyPopAngle2 | PlayerState | FUN_00844500 | FUN_009f4bd0 | None | compute-only | none (global) |
| LowCrossHeight | PlayerData | FUN_00839880 | FUN_009f4bd0 | esi+0x310 | 0.5 | none (global) |
| LowGoalHeight | OfficialState | FUN_00841da0 | FUN_009f4bd0 | esi+0xbc | 0.5 | none (global) |
| MaxBackingSpeed | MatchResolver | FUN_00830eb0 | FUN_009f4bd0 | esi+0x160 | 6 | none (global) |
| MaxBaseAcceleration | MatchResolver | FUN_0082f830 | FUN_009f4bd0 | esi+0x100 | 3 | none (global) |
| MaxBaseDeceleration | MatchResolver | FUN_0082fa10 | FUN_009f4bd0 | esi+0x108 | -5 | none (global) |
| MaxDivingLeftSpeed | MatchResolver | FUN_008319f0 | FUN_009f4bd0 | esi+0x190 | 9 | none (global) |
| MaxDivingRightSpeed | MatchResolver | FUN_00831db0 | FUN_009f4bd0 | esi+0x1a0 | 9 | none (global) |
| MaxJoggingSpeed | MatchResolver | FUN_0082fdd0 | FUN_009f4bd0 | esi+0x118 | 5 | none (global) |
| MaxRunningSpeed | MatchResolver | FUN_0082ffb0 | FUN_009f4bd0 | esi+0x120 | 7 | none (global) |
| MaxSidlingLeftSpeed | MatchResolver | FUN_00831270 | FUN_009f4bd0 | esi+0x170 | 6 | none (global) |
| MaxSidlingRightSpeed | MatchResolver | FUN_00831630 | FUN_009f4bd0 | esi+0x180 | 6 | none (global) |
| MaxSlidingSpeed | MatchResolver | FUN_00832170 | FUN_009f4bd0 | esi+0x1b0 | 9 | none (global) |
| MaxSpeedToTurningCircleRadius | MatchResolver | FUN_0082f650 | FUN_009f4bd0 | esi+0xf8 | 0.3 | none (global) |
| MaxSprintingSpeed | MatchResolver | FUN_00830190 | FUN_009f4bd0 | esi+0x128 | 9 | none (global) |
| MaxWalkingSpeed | MatchResolver | FUN_0082fbf0 | FUN_009f4bd0 | esi+0x110 | 3 | none (global) |
| MentalFatigue | PlayerData | FUN_008349c0 | FUN_009f4bd0 | esi+0xec | 0.4 | none (global) |
| MidHeightScore | OfficialState | FUN_00842700 | FUN_009f4bd0 | esi+0xe4 | 4 | none (global) |
| MinBackingSpeed | MatchResolver | FUN_00830dc0 | FUN_009f4bd0 | esi+0x15c | 5 | none (global) |
| MinBaseAcceleration | MatchResolver | FUN_0082f740 | FUN_009f4bd0 | esi+0xfc | 2 | none (global) |
| MinBaseDeceleration | MatchResolver | FUN_0082f920 | FUN_009f4bd0 | esi+0x104 | -4 | none (global) |
| MinDivingLeftSpeed | MatchResolver | FUN_00831900 | FUN_009f4bd0 | esi+0x18c | 8 | none (global) |
| MinDivingRightSpeed | MatchResolver | FUN_00831cc0 | FUN_009f4bd0 | esi+0x19c | 8 | none (global) |
| MinJoggingSpeed | MatchResolver | FUN_0082fce0 | FUN_009f4bd0 | esi+0x114 | 3 | none (global) |
| MinRunningSpeed | MatchResolver | FUN_0082fec0 | FUN_009f4bd0 | esi+0x11c | 6 | none (global) |
| MinSidlingLeftSpeed | MatchResolver | FUN_00831180 | FUN_009f4bd0 | esi+0x16c | 5 | none (global) |
| MinSidlingRightSpeed | MatchResolver | FUN_00831540 | FUN_009f4bd0 | esi+0x17c | 5 | none (global) |
| MinSlidingSpeed | MatchResolver | FUN_00832080 | FUN_009f4bd0 | esi+0x1ac | 8 | none (global) |
| MinSpeedToTurningCircleRadius | MatchResolver | FUN_0082f560 | FUN_009f4bd0 | esi+0xf4 | 0.15 | none (global) |
| MinSprintingSpeed | MatchResolver | FUN_008300a0 | FUN_009f4bd0 | esi+0x124 | 8 | none (global) |
| MinWalkingSpeed | MatchResolver | FUN_0082fb00 | FUN_009f4bd0 | esi+0x10c | 2 | none (global) |
| MinimumDribbleCaution | PlayerData | FUN_00835f50 | FUN_009f4bd0 | esi+0x210 | 8 | none (global) |
| Mobility | PlayerConstant | FUN_00818330 | FUN_005d6dd0 | — | 100 | Player |
| PassingAgilityLeft | PlayerConstant | FUN_008187b0 | FUN_005d6dd0 | — | 100 | Player |
| PassingAgilityRight | PlayerConstant | FUN_008188d0 | FUN_005d6dd0 | — | 100 | Player |
| PhysicalFatigue | PlayerData | FUN_008348d0 | FUN_009f4bd0 | esi+0xe8 | 0.4 | none (global) |
| PlayerDegradeable | (meta) | FUN_0081ff70 | FUN_005d6dd0 | — | 100 | Player |
| Reaction | PlayerFluid | FUN_0081fe50 | FUN_005d6dd0 | — | 100 | Player |
| RunningAccelerationMultiplier | MatchResolver | FUN_00830820 | FUN_009f4bd0 | esi+0x13c | 2.5 | none (global) |
| RunningDecelerationMultiplier | MatchResolver | FUN_00830910 | FUN_009f4bd0 | esi+0x150 | compute-only | none (global) |
| RunningFatigueModifier | MatchResolver | FUN_0082f380 | FUN_009f4bd0 | esi+0x1b4 | 0.002 | none (global) |
| ShotAgilityLeft | PlayerConstant | FUN_00818c30 | FUN_005d6dd0 | — | 100 | Player |
| ShotAgilityRight | PlayerConstant | FUN_00818d50 | FUN_005d6dd0 | — | 100 | Player |
| SidlingLeftAccelerationMultiplier | MatchResolver | FUN_00830fa0 | FUN_009f4bd0 | esi+0x164 | 1.5 | none (global) |
| SidlingLeftDecelerationMultiplier | MatchResolver | FUN_00831090 | FUN_009f4bd0 | esi+0x168 | 2 | none (global) |
| SidlingRightAccelerationMultiplier | MatchResolver | FUN_00831360 | FUN_009f4bd0 | esi+0x174 | 1.5 | none (global) |
| SidlingRightDecelerationMultiplier | MatchResolver | FUN_00831450 | FUN_009f4bd0 | esi+0x178 | 2 | none (global) |
| SkillFatigue | PlayerData | FUN_00834ab0 | FUN_009f4bd0 | esi+0xf0 | 0.2 | none (global) |
| SkipDelay | PlayerData | FUN_008346f0 | FUN_009f4bd0 | esi+0x1c0 | 2 | none (global) |
| SlidingAccelerationMultiplier | MatchResolver | FUN_00831ea0 | FUN_009f4bd0 | esi+0x1a4 | 6 | none (global) |
| SlidingDecelerationMultiplier | MatchResolver | FUN_00831f90 | FUN_009f4bd0 | esi+0x1a8 | 8 | none (global) |
| SpinAngle1 | PlayerData | FUN_008393d0 | FUN_009f4bd0 | None | compute-only | none (global) |
| SpinAngle2 | PlayerState | FUN_008445f0 | FUN_009f4bd0 | None | compute-only | none (global) |
| Spring | PlayerConstant | FUN_0081d030 | FUN_005d6dd0 | — | 100 | Player |
| SprintDistance | PlayerData | FUN_00836f40 | FUN_009f4bd0 | esi+0x260 | 20 | none (global) |
| SprintingAccelerationMultiplier | MatchResolver | FUN_00830a00 | FUN_009f4bd0 | esi+0x138 | 5 | none (global) |
| SprintingDecelerationMultiplier | MatchResolver | FUN_00830af0 | FUN_009f4bd0 | esi+0x14c | compute-only | none (global) |
| SprintingFatigueModifier | MatchResolver | FUN_0082f470 | FUN_009f4bd0 | esi+0x1b8 | 0.02 | none (global) |
| StandardDribbleCaution | PlayerData | FUN_00836040 | FUN_009f4bd0 | esi+0x214 | 12 | none (global) |
| StandingAccelerationMultiplier | MatchResolver | FUN_00830280 | FUN_009f4bd0 | esi+0x12c | compute-only | none (global) |
| StandingDecelerationMultiplier | MatchResolver | FUN_00830370 | FUN_009f4bd0 | esi+0x140 | compute-only | none (global) |
| TightCollisionAngle | PlayerState | FUN_008447d0 | FUN_009f4bd0 | None | compute-only | none (global) |
| TrickDelivery | PlayerConstant | FUN_0081a850 | FUN_005d6dd0 | — | 100 | Player |
| TrickDistance | PlayerData | FUN_00838980 | FUN_009f4bd0 | esi+0x2d0 | 15 | none (global) |
| TrickReading | PlayerConstant | FUN_0081a730 | FUN_005d6dd0 | — | 100 | Player |
| Trickyness | PlayerConstant | FUN_0081a610 | FUN_005d6dd0 | — | 100 | Player |
| UpdateDegradableStatsPeriod | MatchResolver | FUN_0082f290 | FUN_009f4bd0 | esi+0x9c | 60 | none (global) |
| WalkingAccelerationMultiplier | MatchResolver | FUN_00830460 | FUN_009f4bd0 | esi+0x130 | compute-only | none (global) |
| WalkingDecelerationMultiplier | MatchResolver | FUN_00830550 | FUN_009f4bd0 | esi+0x144 | compute-only | none (global) |

## 2. Passing/Receiving — 44 formulas (36 usable / 8 compute-only)

| name | group | getter FUN | consumer | ResolverSettings off | default | input |
|---|---|---|---|---|---|---|
| AttackingThrow | SquadData | FUN_0083fa00 | FUN_009f4bd0 | esi+0x4a4 | 60 | none (global) |
| ClosePassDistance | PlayerData | FUN_008359b0 | FUN_009f4bd0 | esi+0x204 | 8 | none (global) |
| CrossAbilityScale | PlayerData | FUN_00839b50 | FUN_009f4bd0 | esi+0x31c | 200 | none (global) |
| CrossAccuracy | PlayerConstant | FUN_0081eb30 | FUN_005d6dd0 | — | 100 | Player |
| CrossAreaWidth | PlayerData | FUN_008396a0 | FUN_009f4bd0 | esi+0x308 | 15 | none (global) |
| CrossRangeRight | PlayerConstant | FUN_0081b770 | FUN_005d6dd0 | — | 100 | Player |
| EasyPass | PlayerData | FUN_00835aa0 | FUN_009f4bd0 | None | compute-only | none (global) |
| FeedCloseRange | PlayerData | FUN_00836310 | FUN_009f4bd0 | esi+0x220 | 40 | none (global) |
| FeedScale | PlayerData | FUN_00836220 | FUN_009f4bd0 | None | compute-only | none (global) |
| HighGoodControl | PlayerData | FUN_008375d0 | FUN_009f4bd0 | esi+0x27c | compute-only | none (global) |
| HighPassAngleRange | PlayerData | FUN_00836a90 | FUN_009f4bd0 | None | compute-only | none (global) |
| HighPassDifficulty | PlayerData | FUN_00835500 | FUN_009f4bd0 | esi+0x1f0 | 33.75 | none (global) |
| LayOffRange | PlayerData | FUN_0083da20 | FUN_009f4bd0 | esi+0x374 | 10 | none (global) |
| LowGoodControl | PlayerData | FUN_008374e0 | FUN_009f4bd0 | esi+0x278 | 0.3 | none (global) |
| MaxDistanceFromAreaCrossTarget | PlayerData | FUN_00839d30 | FUN_009f4bd0 | None | 12 | none (global) |
| MaxDistanceFromManCrossTarget | PlayerData | FUN_00839c40 | FUN_009f4bd0 | None | 8 | none (global) |
| MaxHighPassSafety | PlayerData | FUN_008366d0 | FUN_009f4bd0 | esi+0x23c | 10 | none (global) |
| MaxPassSafety | PlayerData | FUN_008364f0 | FUN_009f4bd0 | esi+0x234 | 6 | none (global) |
| MaxSpacePassSafety | PlayerData | FUN_008367c0 | FUN_009f4bd0 | esi+0x240 | 10 | none (global) |
| MaxThrowInDistance | SquadData | FUN_0083faf0 | FUN_009f4bd0 | esi+0x4a8 | 20 | none (global) |
| MinDistanceToCrossTarget | PlayerData | FUN_00839e20 | FUN_009f4bd0 | None | 20 | none (global) |
| MinHighSpacePassDistance | PlayerData | FUN_008369a0 | FUN_009f4bd0 | esi+0x248 | 625 | none (global) |
| MinPassSafety | PlayerData | FUN_008365e0 | FUN_009f4bd0 | esi+0x238 | 4 | none (global) |
| MinSpacePassAngle | PlayerData | FUN_00836c70 | FUN_009f4bd0 | None | compute-only | none (global) |
| MinSpacePassDistance | PlayerData | FUN_008368b0 | FUN_009f4bd0 | esp+0x24 | 100 | none (global) |
| MinThrowDistance | SquadData | FUN_0083f730 | FUN_009f4bd0 | esi+0x498 | 5 | none (global) |
| MinThrowInDistance | SquadData | FUN_0083fbe0 | FUN_009f4bd0 | esi+0x4ac | 20 | none (global) |
| MinValidPassSpeed | PlayerData | FUN_008373f0 | FUN_009f4bd0 | esi+0x274 | 10 | none (global) |
| MinimumPassRangeSquared | PlayerData | FUN_00835320 | FUN_009f4bd0 | esp+0x24 | 81 | none (global) |
| PassCaution | PlayerConstant | FUN_00819650 | FUN_005d6dd0 | — | 100 | Player |
| PassCreativity | PlayerConstant | FUN_0081d930 | FUN_005d6dd0 | — | 100 | Player |
| PassDifficulty | PlayerData | FUN_00835410 | FUN_009f4bd0 | esi+0x1ec | 22.5 | none (global) |
| PassingAccuracy | PlayerConstant | FUN_0081e110 | FUN_005d6dd0 | — | 100 | Player |
| PassingRangeLeft | PlayerConstant | FUN_00818570 | FUN_005d6dd0 | — | 100 | Player |
| PassingRangeRight | PlayerConstant | FUN_00818690 | FUN_005d6dd0 | — | 100 | Player |
| ShortPassChoiceDamper | PlayerData | FUN_00836400 | FUN_009f4bd0 | esi+0x224 | 0.25 | none (global) |
| SidePass | PlayerData | FUN_00835c80 | FUN_009f4bd0 | None | compute-only | none (global) |
| SpacePassAngleRange | PlayerData | FUN_00836b80 | FUN_009f4bd0 | None | compute-only | none (global) |
| StraightPass | PlayerData | FUN_00835b90 | FUN_009f4bd0 | None | compute-only | none (global) |
| ThrowAccuracy | PlayerConstant | FUN_0081dff0 | FUN_005d6dd0 | — | 100 | Player |
| ThrowInClose | SquadData | FUN_008409f0 | FUN_009f4bd0 | esi+0x3e8 | 10 | none (global) |
| ThrowPower | PlayerConstant | FUN_0081ded0 | FUN_005d6dd0 | — | 100 | Player |
| ThrowRunUp | PlayerState | FUN_008427f0 | FUN_009f4bd0 | esi+0x3ec | 6 | none (global) |
| ValidPassApproachSpeed | PlayerState | FUN_00844230 | FUN_009f4bd0 | esi+0x45c | 12 | none (global) |

## 3. Shooting/Finishing — 90 formulas (54 usable / 36 compute-only)

| name | group | getter FUN | consumer | ResolverSettings off | default | input |
|---|---|---|---|---|---|---|
| AccurateShotAngleVariance | ResolverConstants | FUN_00823480 | FUN_009f4bd0 | esi+0x90 | compute-only | none (global) |
| AccurateShotRangeVariance | ResolverConstants | FUN_00823390 | FUN_009f4bd0 | esi+0x8c | compute-only | none (global) |
| AttackHeaderDistance | PlayerData | FUN_0083d1b0 | FUN_009f4bd0 | esi+0x350 | 20 | none (global) |
| AttackingFreeKick | SquadData | FUN_0083fcd0 | FUN_009f4bd0 | esi+0x4b0 | 60 | none (global) |
| BallControl | PlayerConstant | FUN_0081d5d0 | FUN_005d6dd0 | — | 100 | Player |
| BallElasticity | ResolverConstants | FUN_008232a0 | FUN_009f4bd0 | esi+0x88 | compute-only | none (global) |
| BallSpeedEffect | PlayerData | FUN_0083a0f0 | FUN_009f4bd0 | esi+0x334 | 0.004 | none (global) |
| BaseBallKickHeightDirectional | ResolverConstants | FUN_00822760 | FUN_009f4bd0 | esi+0x70 | compute-only | none (global) |
| BaseBallKickHeightFlat | ResolverConstants | FUN_00822580 | FUN_009f4bd0 | esi+0x68 | compute-only | none (global) |
| BaseBallKickHeightHead | ResolverConstants | FUN_00822670 | FUN_009f4bd0 | esi+0x6c | compute-only | none (global) |
| BaseBallKickHeightHigh | ResolverConstants | FUN_00822940 | FUN_009f4bd0 | esi+0x78 | compute-only | none (global) |
| BaseBallKickHeightMaximum | ResolverConstants | FUN_00822a30 | FUN_009f4bd0 | esi+0x7c | compute-only | none (global) |
| BaseBallKickHeightOverhead | ResolverConstants | FUN_00822850 | FUN_009f4bd0 | esi+0x74 | compute-only | none (global) |
| BaseBallSpeedFirm | ResolverConstants | FUN_008221c0 | FUN_009f4bd0 | esi+0x34 | compute-only | none (global) |
| BaseBallSpeedFirmer | ResolverConstants | FUN_008222b0 | FUN_009f4bd0 | esi+0x38 | compute-only | none (global) |
| BaseBallSpeedGentle | ResolverConstants | FUN_00821ef0 | FUN_009f4bd0 | esi+0x28 | compute-only | none (global) |
| BaseBallSpeedMaximum | ResolverConstants | FUN_00822490 | FUN_009f4bd0 | esi+0x40 | compute-only | none (global) |
| BaseBallSpeedMedium | ResolverConstants | FUN_00821fe0 | FUN_009f4bd0 | esi+0x2c | compute-only | none (global) |
| BaseBallSpeedMediumFirm | ResolverConstants | FUN_008220d0 | FUN_009f4bd0 | esi+0x30 | compute-only | none (global) |
| BaseBallSpeedStrong | ResolverConstants | FUN_008223a0 | FUN_009f4bd0 | esi+0x3c | compute-only | none (global) |
| BaseKickReactionTimeFirm | ResolverConstants | FUN_00822df0 | FUN_009f4bd0 | esi+0x54 | compute-only | none (global) |
| BaseKickReactionTimeFirmer | ResolverConstants | FUN_00822ee0 | FUN_009f4bd0 | esi+0x58 | compute-only | none (global) |
| BaseKickReactionTimeGentle | ResolverConstants | FUN_00822b20 | FUN_009f4bd0 | esi+0x48 | compute-only | none (global) |
| BaseKickReactionTimeMaximum | ResolverConstants | FUN_008230c0 | FUN_009f4bd0 | esi+0x60 | compute-only | none (global) |
| BaseKickReactionTimeMedium | ResolverConstants | FUN_00822c10 | FUN_009f4bd0 | esi+0x4c | compute-only | none (global) |
| BaseKickReactionTimeMediumFirm | ResolverConstants | FUN_00822d00 | FUN_009f4bd0 | esi+0x50 | compute-only | none (global) |
| BaseKickReactionTimeStrong | ResolverConstants | FUN_00822fd0 | FUN_009f4bd0 | esi+0x5c | compute-only | none (global) |
| BaseThrowReactionTime | ResolverConstants | FUN_008231b0 | FUN_009f4bd0 | esi+0x84 | compute-only | none (global) |
| BehindBallScale | PlayerData | FUN_00835050 | FUN_009f4bd0 | esi+0x1dc | 0.5 | none (global) |
| CloseFreeKickAngle | SquadData | FUN_00841530 | FUN_009f4bd0 | None | compute-only | none (global) |
| CloseFreeKickDistance | SquadData | FUN_00840db0 | FUN_009f4bd0 | esi+0x4ec | 6 | none (global) |
| CloseToFreeKickTarget | SquadData | FUN_0083fdc0 | FUN_009f4bd0 | esi+0x4b4 | 20 | none (global) |
| CurlAbilityLeft | PlayerConstant | FUN_0081d6f0 | FUN_005d6dd0 | — | 100 | Player |
| CurlAbilityRight | PlayerConstant | FUN_0081d810 | FUN_005d6dd0 | — | 100 | Player |
| DeadballTakingSkill | PlayerConstant | FUN_00818f90 | FUN_005d6dd0 | — | 100 | Player |
| DefaultRisingFreeKick | PlayerState | FUN_008437e0 | FUN_009f4bd0 | esi+0x430 | 2 | none (global) |
| DivingHeader | PlayerConstant | FUN_0081d390 | FUN_005d6dd0 | — | 100 | Player |
| ExtraHeadRange | PlayerData | FUN_0083d660 | FUN_009f4bd0 | esi+0x364 | 8 | none (global) |
| FastGoalSpeed | OfficialState | FUN_00842070 | FUN_009f4bd0 | esi+0xc8 | 28 | none (global) |
| FreeKickBackXScale | SquadData | FUN_00840bd0 | FUN_009f4bd0 | esi+0x4fc | 0.66 | none (global) |
| FreeKickBackZScale | SquadData | FUN_00840ae0 | FUN_009f4bd0 | esi+0x4f8 | 0.33 | none (global) |
| FreeKickOffsideLineScale | PlayerData | FUN_00834ba0 | FUN_009f4bd0 | esi+0x1d4 | 0.45 | none (global) |
| FreeKickRunUp | PlayerState | FUN_008428e0 | FUN_009f4bd0 | esi+0x3f0 | 4 | none (global) |
| FreeKickStayForwardZScale | SquadData | FUN_00840cc0 | FUN_009f4bd0 | esi+0x4e8 | 0.28 | none (global) |
| GoalKickRunUpScale | PlayerState | FUN_00842ca0 | FUN_009f4bd0 | esi+0x400 | 15 | none (global) |
| HeadAccuracy | PlayerConstant | FUN_0081d270 | FUN_005d6dd0 | — | 100 | Player |
| HeadFxScale | PlayerData | FUN_00835230 | FUN_009f4bd0 | esi+0x1e4 | 700 | none (global) |
| HeadRange | PlayerConstant | FUN_0081d150 | FUN_005d6dd0 | — | 100 | Player |
| HeaderFinishSpan | PlayerData | FUN_0083d0c0 | FUN_009f4bd0 | esi+0x34c | 2 | none (global) |
| HeaderTimeSpan | PlayerData | FUN_0083cfd0 | FUN_009f4bd0 | esi+0x348 | compute-only | none (global) |
| HeaderTooClose | PlayerData | FUN_0083e290 | FUN_009f4bd0 | esp+0x24 | 16 | none (global) |
| InaccurateShotAngleVariance | ResolverConstants | FUN_00823660 | FUN_009f4bd0 | esi+0x98 | compute-only | none (global) |
| InaccurateShotRangeVariance | ResolverConstants | FUN_00823570 | FUN_009f4bd0 | esi+0x94 | compute-only | none (global) |
| KickFxScale | PlayerData | FUN_00835140 | FUN_009f4bd0 | esi+0x1e0 | 700 | none (global) |
| LongRangeBicycle | PlayerData | FUN_0083dde0 | FUN_009f4bd0 | esi+0x384 | 15 | none (global) |
| LongRangeDivingHeader | PlayerData | FUN_0083d2a0 | FUN_009f4bd0 | esi+0x354 | 15 | none (global) |
| LongRangeFreeKick | PlayerState | FUN_00843f60 | FUN_009f4bd0 | esi+0x450 | 30 | none (global) |
| LongRangeHeader | PlayerData | FUN_0083d390 | FUN_009f4bd0 | esi+0x358 | 15 | none (global) |
| LongRangeVolley | PlayerData | FUN_0083dc00 | FUN_009f4bd0 | esi+0x37c | 15 | none (global) |
| MaxKickWidth | SquadData | FUN_0083feb0 | FUN_009f4bd0 | esi+0x4b8 | 50 | none (global) |
| MidPassScore | OfficialState | FUN_00842520 | FUN_009f4bd0 | esi+0xdc | 4 | none (global) |
| MidRangeScore | OfficialState | FUN_00842340 | FUN_009f4bd0 | esi+0xd4 | 8 | none (global) |
| MidSoloScore | OfficialState | FUN_00842610 | FUN_009f4bd0 | esi+0xe0 | 10 | none (global) |
| MidSpeedScore | OfficialState | FUN_00842430 | FUN_009f4bd0 | esi+0xd8 | 20 | none (global) |
| MinShot | PlayerData | FUN_008357d0 | FUN_009f4bd0 | esi+0x1fc | 20 | none (global) |
| MinimumHeadRange | PlayerData | FUN_0083d570 | FUN_009f4bd0 | esi+0x360 | 8 | none (global) |
| MustShootRange | PlayerData | FUN_008355f0 | FUN_009f4bd0 | esi+0x1f4 | 15 | none (global) |
| OverFreeKickAngle | SquadData | FUN_00841620 | FUN_009f4bd0 | None | compute-only | none (global) |
| OverFreeKickDistance | SquadData | FUN_00840ea0 | FUN_009f4bd0 | esi+0x4f0 | 10 | none (global) |
| PenaltyRunUp | PlayerState | FUN_008429d0 | FUN_009f4bd0 | esi+0x3f4 | 6 | none (global) |
| PenaltyTakingSkill | PlayerConstant | FUN_00818e70 | FUN_005d6dd0 | — | 100 | Player |
| RunShotDifficulty | PlayerData | FUN_00837210 | FUN_009f4bd0 | None | compute-only | none (global) |
| SetPieceFreeKickRunUp | PlayerState | FUN_00842ac0 | FUN_009f4bd0 | esi+0x3f8 | 8 | none (global) |
| ShootingBrain | PlayerConstant | FUN_0081da50 | FUN_005d6dd0 | — | 100 | Player |
| ShootingFinesse | PlayerConstant | FUN_0081db70 | FUN_005d6dd0 | — | 100 | Player |
| ShotAccuracy | PlayerConstant | FUN_0081ec50 | FUN_005d6dd0 | — | 100 | Player |
| ShotComposureLeft | PlayerConstant | FUN_008190b0 | FUN_005d6dd0 | — | 100 | Player |
| ShotComposureRight | PlayerConstant | FUN_008191d0 | FUN_005d6dd0 | — | 100 | Player |
| ShotRangeLeft | PlayerConstant | FUN_008189f0 | FUN_005d6dd0 | — | 100 | Player |
| ShotRangeRight | PlayerConstant | FUN_00818b10 | FUN_005d6dd0 | — | 100 | Player |
| SlowGoalSpeed | OfficialState | FUN_00841f80 | FUN_009f4bd0 | esi+0xc4 | 8 | none (global) |
| SprintShotDifficulty | PlayerData | FUN_00837300 | FUN_009f4bd0 | None | compute-only | none (global) |
| TightAngleBicycle | PlayerData | FUN_0083f280 | FUN_009f4bd0 | None | compute-only | none (global) |
| TightAngleDivingHeader | PlayerData | FUN_0083f0a0 | FUN_009f4bd0 | None | compute-only | none (global) |
| TightAngleFreeKick | PlayerState | FUN_00844410 | FUN_009f4bd0 | None | compute-only | none (global) |
| TightAngleHeader | PlayerData | FUN_0083efb0 | FUN_009f4bd0 | None | compute-only | none (global) |
| TightAngleVolley | PlayerData | FUN_0083f190 | FUN_009f4bd0 | None | compute-only | none (global) |
| TooCloseToCurl | PlayerData | FUN_00836d60 | FUN_009f4bd0 | esi+0x258 | 15 | none (global) |
| WideGoalBottom | OfficialState | FUN_00842250 | FUN_009f4bd0 | esi+0xd0 | 3 | none (global) |
| WideGoalTop | OfficialState | FUN_00842160 | FUN_009f4bd0 | esi+0xcc | -3 | none (global) |

## 4. Tackling/Defensive (incl. goalkeeping) — 113 formulas (75 usable / 38 compute-only)

| name | group | getter FUN | consumer | ResolverSettings off | default | input |
|---|---|---|---|---|---|---|
| Aggression | PlayerConstant | FUN_0081e590 | FUN_005d6dd0 | — | 100 | Player |
| BlockDeflection | PlayerData | FUN_00838200 | FUN_009f4bd0 | None | compute-only | none (global) |
| BlockEarliestLost | PlayerData | FUN_008384d0 | FUN_009f4bd0 | None | compute-only | none (global) |
| BlockEarliestWon | PlayerData | FUN_008385c0 | FUN_009f4bd0 | None | compute-only | none (global) |
| BlockTackleRandom | PlayerData | FUN_00838110 | FUN_009f4bd0 | None | compute-only | none (global) |
| BlockTackleStrength | PlayerConstant | FUN_00819410 | FUN_005d6dd0 | — | 100 | Player |
| BowlDistance | SquadData | FUN_0083f550 | FUN_009f4bd0 | esi+0x490 | 65 | none (global) |
| BowlFuture | PlayerState | FUN_00843c90 | FUN_009f4bd0 | esi+0x444 | compute-only | none (global) |
| CardDistance | OfficialState | FUN_00841cb0 | FUN_009f4bd0 | esi+0xb8 | 2 | none (global) |
| CatchDistance | PlayerData | FUN_0083e470 | FUN_009f4bd0 | esi+0x3a0 | compute-only | none (global) |
| CelebrationDistance | PlayerState | FUN_00842e80 | FUN_009f4bd0 | esi+0x408 | 3 | none (global) |
| CentralDefenderAbility | PlayerConstant | FUN_0081f0d0 | FUN_005d6dd0 | — | 100 | Player |
| ClearancePower | PlayerConstant | FUN_0081bad0 | FUN_005d6dd0 | — | 100 | Player |
| CloseDive | PlayerData | FUN_0083e560 | FUN_009f4bd0 | esi+0x3a4 | 2 | none (global) |
| CloseInMarking | PlayerData | FUN_0083eb00 | FUN_009f4bd0 | esi+0x3bc | 30 | none (global) |
| CloseKick | PlayerData | FUN_0083e650 | FUN_009f4bd0 | esi+0x3a8 | compute-only | none (global) |
| CynicalFoulRating | OfficialState | FUN_008419e0 | FUN_009f4bd0 | None | compute-only | none (global) |
| DirtyBallDeflection | PlayerData | FUN_00839100 | FUN_009f4bd0 | None | compute-only | none (global) |
| DirtyBallEffect | PlayerData | FUN_008391f0 | FUN_009f4bd0 | None | compute-only | none (global) |
| DirtyFoulRating | OfficialState | FUN_00841ad0 | FUN_009f4bd0 | None | compute-only | none (global) |
| DirtyFoulResult | PlayerData | FUN_00838a70 | FUN_009f4bd0 | esi+0x2d4 | compute-only | none (global) |
| DisruptWallDistance | SquadData | FUN_00840f90 | FUN_009f4bd0 | esi+0x4f4 | 9.5 | none (global) |
| ElbowAngle | PlayerState | FUN_00844b90 | FUN_009f4bd0 | None | compute-only | none (global) |
| ElbowBallRange | PlayerData | FUN_00837f30 | FUN_009f4bd0 | esi+0x2a4 | 6 | none (global) |
| ElbowEffect | PlayerData | FUN_008382f0 | FUN_009f4bd0 | None | compute-only | none (global) |
| ElbowRange | PlayerData | FUN_00837e40 | FUN_009f4bd0 | esi+0x2a0 | 4 | none (global) |
| ElbowingTime | PlayerData | FUN_00838020 | FUN_009f4bd0 | esi+0x2a8 | compute-only | none (global) |
| ExtraSaveReach | PlayerData | FUN_0083e380 | FUN_009f4bd0 | esi+0x39c | 2 | none (global) |
| FallingBackwards | PlayerState | FUN_00843150 | FUN_009f4bd0 | esi+0x414 | -2 | none (global) |
| FallingForwards | PlayerState | FUN_00843060 | FUN_009f4bd0 | esi+0x410 | 3 | none (global) |
| GoallineClearance | PlayerData | FUN_0083d840 | FUN_009f4bd0 | esi+0x36c | 4 | none (global) |
| Handball | PlayerConstant | FUN_0081d4b0 | FUN_005d6dd0 | — | 100 | Player |
| HandsKickRunUp | PlayerState | FUN_00842d90 | FUN_009f4bd0 | esi+0x404 | 4 | none (global) |
| HardTackleFrequency | PlayerConstant | FUN_0081a070 | FUN_005d6dd0 | — | 100 | Player |
| HookAheadTarget | PlayerData | FUN_00838e30 | FUN_009f4bd0 | None | compute-only | none (global) |
| HookAngle | PlayerState | FUN_00844c80 | FUN_009f4bd0 | None | compute-only | none (global) |
| HookBallDeflection | PlayerData | FUN_00838f20 | FUN_009f4bd0 | None | compute-only | none (global) |
| HookBallEffect | PlayerData | FUN_00839010 | FUN_009f4bd0 | None | compute-only | none (global) |
| HookFoulResult | PlayerData | FUN_00838b60 | FUN_009f4bd0 | esi+0x2d8 | 0.3 | none (global) |
| HookTackleBallArea | PlayerData | FUN_00838c50 | FUN_009f4bd0 | esi+0x2dc | 6 | none (global) |
| KeeperAgility | PlayerConstant | FUN_0081c2b0 | FUN_005d6dd0 | — | 100 | Player |
| KeeperBravery | PlayerConstant | FUN_0081be30 | FUN_005d6dd0 | — | 100 | Player |
| KeeperCollision | PlayerState | FUN_00842f70 | FUN_009f4bd0 | esi+0x40c | 2 | none (global) |
| KeeperCoordinateDefence | PlayerConstant | FUN_0081cbb0 | FUN_005d6dd0 | — | 100 | Player |
| KeeperCrosses | PlayerConstant | FUN_0081c730 | FUN_005d6dd0 | — | 100 | Player |
| KeeperEccentricity | PlayerConstant | FUN_0081cdf0 | FUN_005d6dd0 | — | 100 | Player |
| KeeperGrip | PlayerConstant | FUN_0081c4f0 | FUN_005d6dd0 | — | 100 | Player |
| KeeperHandStrength | PlayerConstant | FUN_0081c610 | FUN_005d6dd0 | — | 100 | Player |
| KeeperKicking | PlayerConstant | FUN_0081c850 | FUN_005d6dd0 | — | 100 | Player |
| KeeperPunching | PlayerConstant | FUN_0081ca90 | FUN_005d6dd0 | — | 100 | Player |
| KeeperReach | PlayerConstant | FUN_0081c190 | FUN_005d6dd0 | — | 100 | Player |
| KeeperRecovery | PlayerConstant | FUN_0081c3d0 | FUN_005d6dd0 | — | 100 | Player |
| KeeperRunningOut | PlayerConstant | FUN_0081bf50 | FUN_005d6dd0 | — | 100 | Player |
| KeeperThrowing | PlayerConstant | FUN_0081c970 | FUN_005d6dd0 | — | 100 | Player |
| KeeperWidthBias | PlayerState | FUN_008439c0 | FUN_009f4bd0 | esi+0x438 | 2 | none (global) |
| LooseMarking | PlayerData | FUN_0083e920 | FUN_009f4bd0 | esi+0x3b4 | 8 | none (global) |
| LungeDistance | PlayerData | FUN_0083d750 | FUN_009f4bd0 | esi+0x368 | 4 | none (global) |
| Marking | PlayerConstant | FUN_0081e230 | FUN_005d6dd0 | — | 100 | Player |
| MarkingDistanceScale | PlayerData | FUN_0083ea10 | FUN_009f4bd0 | esi+0x3b8 | 0.02 | none (global) |
| MaxBlockTackleResult | PlayerData | FUN_008377b0 | FUN_009f4bd0 | esi+0x284 | 35 | none (global) |
| MaxDefendDistance | PlayerState | FUN_00843510 | FUN_009f4bd0 | esi+0x424 | 8 | none (global) |
| MaxReactionTime | PlayerData | FUN_0083e740 | FUN_009f4bd0 | esi+0x3ac | compute-only | none (global) |
| MaxTackleChaseRange | PlayerState | FUN_00844140 | FUN_009f4bd0 | esi+0x458 | 12 | none (global) |
| MaximumWallPlayers | OfficialState | FUN_00841710 | FUN_009f4bd0 | None | compute-only | none (global) |
| MinBlockTackleResult | PlayerData | FUN_008376c0 | FUN_009f4bd0 | esi+0x280 | 10 | none (global) |
| MinDefendDistance | PlayerState | FUN_00843420 | FUN_009f4bd0 | esi+0x420 | 5 | none (global) |
| MinDeflection | PlayerData | FUN_008378a0 | FUN_009f4bd0 | esi+0x288 | 0.5 | none (global) |
| MinTackleChaseRange | PlayerState | FUN_00844050 | FUN_009f4bd0 | esi+0x454 | 6 | none (global) |
| MinimumKeeperUpdate | PlayerState | FUN_008438d0 | FUN_009f4bd0 | esi+0x434 | 0.1 | none (global) |
| MinorFoulRating | OfficialState | FUN_008418f0 | FUN_009f4bd0 | None | compute-only | none (global) |
| NudgeFoulRating | OfficialState | FUN_00841800 | FUN_009f4bd0 | None | compute-only | none (global) |
| OverAggression | PlayerConstant | FUN_0081a3d0 | FUN_005d6dd0 | — | 100 | Player |
| PenaltyAreaCaution | PlayerState | FUN_00843600 | FUN_009f4bd0 | esi+0x428 | 0.05 | none (global) |
| PhysicalCautionDistance | PlayerState | FUN_008436f0 | FUN_009f4bd0 | esi+0x42c | compute-only | none (global) |
| ProffessionalFoulAngle | OfficialState | FUN_00841bc0 | FUN_009f4bd0 | None | compute-only | none (global) |
| PullShirtFrequency | PlayerData | FUN_00837d50 | FUN_009f4bd0 | esi+0x29c | 0.05 | none (global) |
| PullShirtTime | PlayerData | FUN_00837c60 | FUN_009f4bd0 | esi+0x298 | compute-only | none (global) |
| RollDistance | SquadData | FUN_0083f640 | FUN_009f4bd0 | esi+0x494 | 20 | none (global) |
| RollFuture | PlayerState | FUN_00843d80 | FUN_009f4bd0 | esi+0x448 | 0.5 | none (global) |
| RollTooClose | SquadData | FUN_00841350 | FUN_009f4bd0 | esi+0x50c | 20 | none (global) |
| RunOutDistance | PlayerState | FUN_00843ba0 | FUN_009f4bd0 | esi+0x440 | 50 | none (global) |
| ShirtPullAngle | PlayerState | FUN_00844aa0 | FUN_009f4bd0 | None | compute-only | none (global) |
| ShirtPulling | PlayerConstant | FUN_00819ad0 | FUN_005d6dd0 | — | 100 | Player |
| ShirtPullingBallRange | PlayerData | FUN_00837b70 | FUN_009f4bd0 | esi+0x294 | 6 | none (global) |
| ShirtPullingEffect | PlayerData | FUN_008383e0 | FUN_009f4bd0 | None | compute-only | none (global) |
| ShirtPullingRange | PlayerData | FUN_00837a80 | FUN_009f4bd0 | esi+0x290 | 4 | none (global) |
| SlideAheadTarget | PlayerData | FUN_008392e0 | FUN_009f4bd0 | None | compute-only | none (global) |
| SlideAngle | PlayerState | FUN_00844d70 | FUN_009f4bd0 | None | compute-only | none (global) |
| SlideBallDeflection | PlayerData | FUN_0083f370 | FUN_009f4bd0 | None | compute-only | none (global) |
| SlideBallEffect | PlayerData | FUN_0083f460 | FUN_009f4bd0 | None | compute-only | none (global) |
| SlideCautionDistance | PlayerState | FUN_00843e70 | FUN_009f4bd0 | esi+0x44c | 4 | none (global) |
| SlideFoulResult | PlayerData | FUN_0083dfc0 | FUN_009f4bd0 | esi+0x38c | 0.6 | none (global) |
| SlideTackleBallArea | PlayerData | FUN_0083ded0 | FUN_009f4bd0 | esi+0x388 | 6 | none (global) |
| SlideTimeSpan | PlayerData | FUN_0083e0b0 | FUN_009f4bd0 | esi+0x390 | compute-only | none (global) |
| SmotherBallDeflection | PlayerData | FUN_0083a1e0 | FUN_009f4bd0 | None | compute-only | none (global) |
| SmotherBallEffect | PlayerData | FUN_0083a2d0 | FUN_009f4bd0 | None | compute-only | none (global) |
| SmotherDistance | PlayerState | FUN_00843ab0 | FUN_009f4bd0 | esi+0x43c | 8 | none (global) |
| SmotherFoulResult | PlayerData | FUN_0083a000 | FUN_009f4bd0 | esi+0x330 | 0.6 | none (global) |
| SmotheringAbility | PlayerConstant | FUN_0081c070 | FUN_005d6dd0 | — | 100 | Player |
| StabDistance | PlayerData | FUN_0083db10 | FUN_009f4bd0 | esi+0x378 | 6 | none (global) |
| TackleAvoidance | PlayerConstant | FUN_0081a4f0 | FUN_005d6dd0 | — | 100 | Player |
| TackleCaution | PlayerConstant | FUN_0081a2b0 | FUN_005d6dd0 | — | 100 | Player |
| TackleChaseRange | PlayerConstant | FUN_008192f0 | FUN_005d6dd0 | — | 100 | Player |
| TackleEasy | SquadData | FUN_00840630 | FUN_009f4bd0 | esi+0x4d8 | 0.5 | none (global) |
| TackleFrequency | PlayerConstant | FUN_00819f50 | FUN_005d6dd0 | — | 100 | Player |
| TackleHard | SquadData | FUN_00840720 | FUN_009f4bd0 | esi+0x4dc | 1.5 | none (global) |
| TacklingAbility | PlayerConstant | FUN_0081a190 | FUN_005d6dd0 | — | 100 | Player |
| TightMarking | PlayerData | FUN_0083e830 | FUN_009f4bd0 | esi+0x3b0 | 3 | none (global) |
| TripAngle | PlayerState | FUN_008449b0 | FUN_009f4bd0 | None | compute-only | none (global) |
| TripBallRange | PlayerData | FUN_008387a0 | FUN_009f4bd0 | esi+0x2c8 | 6 | none (global) |
| TripEffect | PlayerData | FUN_00838d40 | FUN_009f4bd0 | None | compute-only | none (global) |
| TripRange | PlayerData | FUN_008386b0 | FUN_009f4bd0 | esi+0x2c4 | 4 | none (global) |
| TrippingTime | PlayerData | FUN_00838890 | FUN_009f4bd0 | esi+0x2cc | compute-only | none (global) |

## 5. Positioning/Awareness — 67 formulas (60 usable / 7 compute-only)

| name | group | getter FUN | consumer | ResolverSettings off | default | input |
|---|---|---|---|---|---|---|
| ActingAbility | PlayerConstant | FUN_00819890 | FUN_005d6dd0 | — | 100 | Player |
| ActiveRadius | SquadData | FUN_0083ffa0 | FUN_009f4bd0 | esi+0x4bc | 8 | none (global) |
| ActiveRangeScale | PlayerData | FUN_0083ece0 | FUN_009f4bd0 | esi+0x3c4 | 2 | none (global) |
| Adaptability | PlayerConstant | FUN_0081ea10 | FUN_005d6dd0 | — | 100 | Player |
| AddedImpetus | SquadData | FUN_00841170 | FUN_009f4bd0 | esi+0x504 | 0.02 | none (global) |
| Alertness | PlayerConstant | FUN_00819530 | FUN_005d6dd0 | — | 100 | Player |
| Ambition | PlayerConstant | FUN_0081e7d0 | FUN_005d6dd0 | — | 100 | Player |
| AntiWidthBias | PlayerData | FUN_0083e1a0 | FUN_009f4bd0 | esi+0x394 | 4 | none (global) |
| AttackingMidAbility | PlayerConstant | FUN_0081f670 | FUN_005d6dd0 | — | 100 | Player |
| CentralAbility | PlayerConstant | FUN_0081faf0 | FUN_005d6dd0 | — | 100 | Player |
| Cluster | SquadData | FUN_00841440 | FUN_009f4bd0 | None | compute-only | none (global) |
| CommitToRun | PlayerData | FUN_00834f60 | FUN_009f4bd0 | esi+0x1d8 | 0.1 | none (global) |
| Composure | PlayerConstant | FUN_00818210 | FUN_005d6dd0 | — | 100 | Player |
| Consistency | PlayerConstant | FUN_0081e350 | FUN_005d6dd0 | — | 100 | Player |
| CornerRunUp | PlayerState | FUN_00842bb0 | FUN_009f4bd0 | esi+0x3fc | 15 | none (global) |
| DeepAreaLength | PlayerData | FUN_00839790 | FUN_009f4bd0 | esi+0x30c | 40 | none (global) |
| DeepOwnHalfRun | PlayerData | FUN_00837120 | FUN_009f4bd0 | esi+0x268 | -30 | none (global) |
| DefensiveMidAbility | PlayerConstant | FUN_0081f310 | FUN_005d6dd0 | — | 100 | Player |
| DegreesControlStepLeft | PlayerConstant | FUN_0081abb0 | FUN_005d6dd0 | — | compute-only | Player |
| ForwardAbility | PlayerConstant | FUN_0081f9d0 | FUN_005d6dd0 | — | 100 | Player |
| FrustrationScale | SquadData | FUN_00841080 | FUN_009f4bd0 | esi+0x500 | 0.1 | none (global) |
| FullBackAbility | PlayerConstant | FUN_0081efb0 | FUN_005d6dd0 | — | 100 | Player |
| GameAwareness | PlayerConstant | FUN_0081ccd0 | FUN_005d6dd0 | — | 100 | Player |
| GoForwardXScale | SquadData | FUN_00840900 | FUN_009f4bd0 | esi+0x4e4 | 0.28 | none (global) |
| GoForwardZScale | SquadData | FUN_00840810 | FUN_009f4bd0 | esi+0x4e0 | 0.25 | none (global) |
| HuntRangeScale | PlayerData | FUN_0083edd0 | FUN_009f4bd0 | esi+0x3c8 | compute-only | none (global) |
| InitialDesperation | SquadData | FUN_0083f910 | FUN_009f4bd0 | esi+0x4a0 | 0.2 | none (global) |
| Leadership | PlayerConstant | FUN_0081e470 | FUN_005d6dd0 | — | 100 | Player |
| LeftFoot | PlayerConstant | FUN_0081ed70 | FUN_005d6dd0 | — | 100 | Player |
| LeftSideAbility | PlayerConstant | FUN_0081fc10 | FUN_005d6dd0 | — | 100 | Player |
| LosingDesperation | SquadData | FUN_00840090 | FUN_009f4bd0 | esi+0x4c0 | 0.75 | none (global) |
| LosingImpetus | SquadData | FUN_00841260 | FUN_009f4bd0 | esi+0x508 | 0.025 | none (global) |
| MaxExtraComposure | PlayerState | FUN_00843330 | FUN_009f4bd0 | esi+0x41c | 10 | none (global) |
| MaxPressure | SquadData | FUN_00840450 | FUN_009f4bd0 | esi+0x4d0 | 25 | none (global) |
| MaxRoamDistance | PlayerData | FUN_0083eec0 | FUN_009f4bd0 | esi+0x3cc | 80 | none (global) |
| MidfieldAbility | PlayerConstant | FUN_0081f790 | FUN_005d6dd0 | — | 100 | Player |
| MinComposure | PlayerState | FUN_00843240 | FUN_009f4bd0 | esi+0x418 | 10 | none (global) |
| MinSetPieceDistance | SquadData | FUN_0083f820 | FUN_009f4bd0 | esi+0x49c | compute-only | none (global) |
| NoPressureExtraTimeMinutes | SquadData | FUN_00840360 | FUN_009f4bd0 | esi+0x4cc | 75 | none (global) |
| NoPressureTimeMinutes | SquadData | FUN_00840270 | FUN_009f4bd0 | esi+0x4c8 | 70 | none (global) |
| OffsideLineScale | PlayerData | FUN_00834c90 | FUN_009f4bd0 | esi+0x1c8 | 0.6 | none (global) |
| OffsideLineScaleMax | PlayerData | FUN_00834e70 | FUN_009f4bd0 | esi+0x1d0 | 0.9 | none (global) |
| OffsideLineScaleMin | PlayerData | FUN_00834d80 | FUN_009f4bd0 | esi+0x1cc | 0.3 | none (global) |
| OwnHalfRun | PlayerData | FUN_00837030 | FUN_009f4bd0 | esi+0x264 | -30 | none (global) |
| PhysicalPlay | PlayerConstant | FUN_008199b0 | FUN_005d6dd0 | — | 100 | Player |
| PlayerAbility | PlayerConstant | FUN_00819bf0 | FUN_005d6dd0 | — | 100 | Player |
| PoorAngleBias | PlayerData | FUN_00836130 | FUN_009f4bd0 | esi+0x218 | 0.5 | none (global) |
| PressureJumpDistance | PlayerData | FUN_0083d480 | FUN_009f4bd0 | esi+0x35c | 4 | none (global) |
| PressureScale | SquadData | FUN_00840540 | FUN_009f4bd0 | esi+0x4d4 | 0.003 | none (global) |
| Professionalism | PlayerConstant | FUN_0081e6b0 | FUN_005d6dd0 | — | 100 | Player |
| Resilience | PlayerConstant | FUN_0081a970 | FUN_005d6dd0 | — | 100 | Player |
| RightFoot | PlayerConstant | FUN_0081ee90 | FUN_005d6dd0 | — | 100 | Player |
| RightSideAbility | PlayerConstant | FUN_0081fd30 | FUN_005d6dd0 | — | 100 | Player |
| Roaming | PlayerConstant | FUN_0081dc90 | FUN_005d6dd0 | — | 100 | Player |
| RunAdjusterDegrees | PlayerState | FUN_008448c0 | FUN_009f4bd0 | None | compute-only | none (global) |
| RunFrequency | PlayerConstant | FUN_0081ddb0 | FUN_005d6dd0 | — | 100 | Player |
| SetupPower | PlayerData | FUN_00834600 | FUN_009f4bd0 | esi+0x1bc | 64 | none (global) |
| ShortCornerDistance | PlayerState | FUN_00844320 | FUN_009f4bd0 | None | compute-only | none (global) |
| StrikerAbility | PlayerConstant | FUN_0081f8b0 | FUN_005d6dd0 | — | 100 | Player |
| SweeperAbility | PlayerConstant | FUN_0081f1f0 | FUN_005d6dd0 | — | 100 | Player |
| TouchlineAreaLength | PlayerData | FUN_00839a60 | FUN_009f4bd0 | esi+0x318 | 12 | none (global) |
| UnderPressure | PlayerData | FUN_0083d930 | FUN_009f4bd0 | esi+0x370 | 6 | none (global) |
| UninterestedRangeScale | PlayerData | FUN_0083ebf0 | FUN_009f4bd0 | esi+0x3c0 | 3 | none (global) |
| Versatility | PlayerConstant | FUN_0081e8f0 | FUN_005d6dd0 | — | 100 | Player |
| WingBackAbility | PlayerConstant | FUN_0081f430 | FUN_005d6dd0 | — | 100 | Player |
| WingerAbility | PlayerConstant | FUN_0081f550 | FUN_005d6dd0 | — | 100 | Player |
| WinningMargin | SquadData | FUN_00840180 | FUN_009f4bd0 | esi+0x4c4 | compute-only | none (global) |

## 6. Player Performance/Match Rating — 47 formulas (45 usable / 2 compute-only)

| name | group | getter FUN | consumer | ResolverSettings off | default | input |
|---|---|---|---|---|---|---|
| AssistsPerformance | PlayerData | FUN_0083b860 | FUN_009f4bd0 | esi+0x574 | 0.6 | none (global) |
| AttackerGoalPerformance | PlayerData | FUN_0083ad20 | FUN_009f4bd0 | esi+0x544 | 4 | none (global) |
| BadCrossesPerformance | PlayerData | FUN_0083b680 | FUN_009f4bd0 | esi+0x56c | 0.05 | none (global) |
| BadPassesPerformance | PlayerData | FUN_0083a780 | FUN_009f4bd0 | esi+0x52c | -0.2 | none (global) |
| BadShotsPerformance | PlayerData | FUN_0083aa50 | FUN_009f4bd0 | esi+0x538 | -0.1 | none (global) |
| BadTacklesPerformance | PlayerData | FUN_0083aff0 | FUN_009f4bd0 | esi+0x550 | -0.05 | none (global) |
| CleanSheetKeeperDefenderPerformance | PlayerData | FUN_0083cb20 | FUN_009f4bd0 | esi+0x5c4 | 4 | none (global) |
| CleanSheetMidfielderPerformance | PlayerData | FUN_0083cc10 | FUN_009f4bd0 | esi+0x5c8 | compute-only | none (global) |
| ClearancePerformance | PlayerData | FUN_0083bd10 | FUN_009f4bd0 | esi+0x588 | 0.1 | none (global) |
| CompletedPassesPerformance | PlayerData | FUN_0083a690 | FUN_009f4bd0 | esi+0x528 | 0.1 | none (global) |
| CrossesFumbledPerformance | PlayerData | FUN_0083c490 | FUN_009f4bd0 | esi+0x5a8 | -0.4 | none (global) |
| CrossesPunchedPerformance | PlayerData | FUN_0083c3a0 | FUN_009f4bd0 | esi+0x5a4 | 0.2 | none (global) |
| CrossesTakenPerformance | PlayerData | FUN_0083c2b0 | FUN_009f4bd0 | esi+0x5a0 | 0.3 | none (global) |
| CynicalPerformance | PlayerData | FUN_0083b3b0 | FUN_009f4bd0 | esi+0x560 | -0.4 | none (global) |
| DirtyPerformance | PlayerData | FUN_0083b2c0 | FUN_009f4bd0 | esi+0x55c | -0.6 | none (global) |
| FailedControlPerformance | PlayerData | FUN_0083ba40 | FUN_009f4bd0 | esi+0x57c | -0.2 | none (global) |
| GoalConcededDefenderPerformance | PlayerData | FUN_0083ca30 | FUN_009f4bd0 | esi+0x5c0 | -0.5 | none (global) |
| GoalConcededKeeperPerformance | PlayerData | FUN_0083c940 | FUN_009f4bd0 | esi+0x5bc | -0.5 | none (global) |
| GoodCrossesPerformance | PlayerData | FUN_0083b590 | FUN_009f4bd0 | esi+0x568 | 0.2 | none (global) |
| GroundCoveredPerformance | PlayerData | FUN_0083b770 | FUN_009f4bd0 | esi+0x570 | compute-only | none (global) |
| HeadersPerformance | PlayerData | FUN_0083be00 | FUN_009f4bd0 | esi+0x58c | 0.1 | none (global) |
| InterceptionsPerformance | PlayerData | FUN_0083bb30 | FUN_009f4bd0 | esi+0x580 | 0.2 | none (global) |
| KeeperDefenderGoalPerformance | PlayerData | FUN_0083ab40 | FUN_009f4bd0 | esi+0x53c | 6 | none (global) |
| KeptPossessionPerformance | PlayerData | FUN_0083b950 | FUN_009f4bd0 | esi+0x578 | 0.1 | none (global) |
| KeyHeadersPerformance | PlayerData | FUN_0083bef0 | FUN_009f4bd0 | esi+0x590 | 0.3 | none (global) |
| KeyPassesPerformance | PlayerData | FUN_0083a870 | FUN_009f4bd0 | esi+0x530 | 0.5 | none (global) |
| KeyTacklesPerformance | PlayerData | FUN_0083b1d0 | FUN_009f4bd0 | esi+0x558 | 0.3 | none (global) |
| LostPossessionPerformance | PlayerData | FUN_0083c1c0 | FUN_009f4bd0 | esi+0x59c | -0.1 | none (global) |
| MidfielderGoalPerformance | PlayerData | FUN_0083ac30 | FUN_009f4bd0 | esi+0x540 | 5 | none (global) |
| OffsidePerformance | PlayerData | FUN_0083bfe0 | FUN_009f4bd0 | esi+0x594 | -0.02 | none (global) |
| OnTargetShotsPerformance | PlayerData | FUN_0083a960 | FUN_009f4bd0 | esi+0x534 | 0.3 | none (global) |
| OwnGoalsPerformance | PlayerData | FUN_0083af00 | FUN_009f4bd0 | esi+0x54c | -0.7 | none (global) |
| PenGoalsPerformance | PlayerData | FUN_0083ae10 | FUN_009f4bd0 | esi+0x548 | 0.65 | none (global) |
| PenaltyMissedPerformance | PlayerData | FUN_0083cdf0 | FUN_009f4bd0 | esi+0x5d0 | -2 | none (global) |
| PenaltySavedPerformance | PlayerData | FUN_0083cd00 | FUN_009f4bd0 | esi+0x5cc | 5 | none (global) |
| PerformanceBalancing | PlayerData | FUN_0083cee0 | FUN_009f4bd0 | esi+0x5d4 | 1.1 | none (global) |
| RedCardPerformance | PlayerData | FUN_0083a3c0 | FUN_009f4bd0 | esi+0x51c | -2 | none (global) |
| RunsPerformance | PlayerData | FUN_0083bc20 | FUN_009f4bd0 | esi+0x584 | 0.1 | none (global) |
| ShotsFumbledPerformance | PlayerData | FUN_0083c850 | FUN_009f4bd0 | esi+0x5b8 | -0.4 | none (global) |
| ShotsHeldPerformance | PlayerData | FUN_0083c580 | FUN_009f4bd0 | esi+0x5ac | 0.3 | none (global) |
| ShotsParriedPerformance | PlayerData | FUN_0083c670 | FUN_009f4bd0 | esi+0x5b0 | 0.2 | none (global) |
| ShotsTippedPerformance | PlayerData | FUN_0083c760 | FUN_009f4bd0 | esi+0x5b4 | 0.2 | none (global) |
| TacklesAvoidedPerformance | PlayerData | FUN_0083b4a0 | FUN_009f4bd0 | esi+0x564 | 0.1 | none (global) |
| TacklesWonPerformance | PlayerData | FUN_0083b0e0 | FUN_009f4bd0 | esi+0x554 | 0.1 | none (global) |
| TouchesPerformance | PlayerData | FUN_0083a5a0 | FUN_009f4bd0 | esi+0x524 | 0.01 | none (global) |
| TricksPerformance | PlayerData | FUN_0083c0d0 | FUN_009f4bd0 | esi+0x598 | 0.03 | none (global) |
| YellowCardPerformance | PlayerData | FUN_0083a4b0 | FUN_009f4bd0 | esi+0x520 | -1 | none (global) |

## 7. Other (career/AI/finance/non-match) — 346 formulas (157 usable / 189 compute-only) — summarized

Not match-engine formulas: career simulation, AI, finance, morale, training, transfers, board/manager logic. Each has a single **direct** career-code caller (none go through the two match harvesters, except 8 PlayerData/State match rows that are listed under their real match family above). Enumerated by group:

| Group | Count | Usable | Compute-only | Primary consumer FUN |
|---|--:|--:|--:|---|
| ProspectiveManagers | 77 | 38 | 39 | FUN_007a02f0 |
| PlayerHappiness | 37 | 12 | 25 | FUN_0078a1e0 |
| BoardRequest | 37 | 8 | 29 | FUN_007479e0 |
| TrainingFacilities | 30 | 0 | 30 | FUN_0074e560 |
| TeamSelection | 26 | 26 | 0 | FUN_005db6b0 |
| WageLimits | 19 | 19 | 0 | FUN_0058a4a0 |
| PersonHappiness | 17 | 5 | 12 | FUN_006e30c0 |
| ManagerSacking | 17 | 4 | 13 | FUN_007a9570 |
| Reputation | 10 | 0 | 10 | FUN_00792b60 |
| TransferModule | 9 | 7 | 2 | FUN_007c6770 |
| Budget | 9 | 7 | 2 | FUN_00589620 |
| MatchReport | 6 | 6 | 0 | FUN_0070b150 |
| CoachReport | 5 | 0 | 5 | FUN_00732410 |
| PhysioInjuries | 5 | 3 | 2 | FUN_0078b7a0 |
| PhysioReport | 5 | 0 | 5 | FUN_00732410 |
| PlayerReputationPostMatchProgression | 5 | 0 | 5 | FUN_00792b60 |
| ClubInfo | 4 | 4 | 0 | FUN_00581f60 |
| ManagerDiscipline | 4 | 4 | 0 | FUN_00732410 |
| TransferListNotify | 3 | 0 | 3 | FUN_007a9b50 |
| CompetitionIncome | 3 | 3 | 0 | FUN_00751f50 |
| FinancialCosts | 3 | 2 | 1 | FUN_0074fd50 |
| PostMatchMutation | 3 | 3 | 0 | FUN_00721ca0 |
| NewPlayerSalary | 3 | 0 | 3 | FUN_00714440 |
| RollForwardData | 2 | 0 | 2 | FUN_007194e0 |
| TrainingModule | 2 | 2 | 0 | FUN_00798440 |
| PersonAgeing | 2 | 2 | 0 | FUN_00789d50 |
| BoardAI | 1 | 1 | 0 | FUN_00564150 |
| PlayerProfessionalism | 1 | 0 | 1 | FUN_0078a1e0 |
| MatchModule | 1 | 1 | 0 | FUN_0077cb80 |

<details><summary>Full name list for group 7 (name — group — getter FUN — consumer — default)</summary>

| name | group | getter FUN | consumer | default |
|---|---|---|---|---|
| AssistantWage | WageLimits | FUN_00824170 | FUN_00581e50 (direct) | 1000 |
| AttackingMidfielderCenterBias | TeamSelection | FUN_00821470 | FUN_005db6b0 (direct) | 100 |
| AttackingMidfielderLeftBias | TeamSelection | FUN_00821350 | FUN_005db6b0 (direct) | 100 |
| AttackingMidfielderRightBias | TeamSelection | FUN_00821590 | FUN_005db6b0 (direct) | 100 |
| AwayFromHomeNation | PlayerHappiness | FUN_0082dca0 | FUN_0078a1e0 (direct) | compute-only |
| AwayFromHomeNationNoLanguage | PlayerHappiness | FUN_0082dd90 | FUN_0078a1e0 (direct) | compute-only |
| AwayFromHomeNationNoManagerLanguage | PlayerHappiness | FUN_0082de80 | FUN_0078a1e0 (direct) | compute-only |
| BigClubNotifyQualityThreshold | TransferListNotify | FUN_008597e0 | FUN_007a9b50 (direct) | compute-only |
| BoardInvestAmount | BoardAI | FUN_00833130 | FUN_00564150 (direct) | 10000 |
| BoardRequestExpandStadiumBoardAmbitionThreshold | BoardRequest | FUN_0084c210 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExpandStadiumBoardOpinionThreshold | BoardRequest | FUN_0084c030 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExpandStadiumEEOYBThreshold | BoardRequest | FUN_0084c300 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExpandStadiumHigherDivisionThreshold | BoardRequest | FUN_0084c5d0 | FUN_007479e0 (direct) | 0.9 |
| BoardRequestExpandStadiumHoneymoonPeriodMonthsThreshold | BoardRequest | FUN_0084c120 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExpandStadiumSameDivisionThreshold | BoardRequest | FUN_0084c4e0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExpandStadiumSeasonGateThreshold | BoardRequest | FUN_0084c3f0 | FUN_007479e0 (direct) | 0.9 |
| BoardRequestExtraRebuildTimeBoardOpinionThresholdHigh | BoardRequest | FUN_0084cd50 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraRebuildTimeBoardOpinionThresholdLow | BoardRequest | FUN_0084ce40 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraRebuildTimeHoneymoonPeriodMonthsThreshold | BoardRequest | FUN_0084cf30 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraTransferFundsBoardAmbitionThreshold | BoardRequest | FUN_0084b8b0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraTransferFundsBoardOpinionThreshold | BoardRequest | FUN_0084b400 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraTransferFundsBudgetLeagueAverageThreshold | BoardRequest | FUN_0084b6d0 | FUN_007479e0 (direct) | 0.3 |
| BoardRequestExtraTransferFundsBudgetUsedThreshold | BoardRequest | FUN_0084b5e0 | FUN_007479e0 (direct) | 0.7 |
| BoardRequestExtraTransferFundsEEOYBThreshold | BoardRequest | FUN_0084b4f0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraTransferFundsHoneymoonPeriodMonthsThreshold | BoardRequest | FUN_0084b9a0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraTransferFundsNumberPlayersThreshold | BoardRequest | FUN_0084b7c0 | FUN_007479e0 (direct) | 0.3 |
| BoardRequestExtraWageFundsAtWeeklySalaryCapThreshold | BoardRequest | FUN_0084bf40 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraWageFundsBoardAmbitionThreshold | BoardRequest | FUN_0084bd60 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraWageFundsBoardOpinionThreshold | BoardRequest | FUN_0084ba90 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraWageFundsEEOYBThreshold | BoardRequest | FUN_0084bb80 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraWageFundsHoneymoonPeriodMonthsThreshold | BoardRequest | FUN_0084be50 | FUN_007479e0 (direct) | compute-only |
| BoardRequestExtraWageFundsNumberPlayersThreshold | BoardRequest | FUN_0084bc70 | FUN_007479e0 (direct) | 0.3 |
| BoardRequestImproveFacilitiesBoardAmbitionThreshold | BoardRequest | FUN_0084c8a0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestImproveFacilitiesBoardOpinionThreshold | BoardRequest | FUN_0084c6c0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestImproveFacilitiesDivisionAverageThreshold | BoardRequest | FUN_0084ca80 | FUN_007479e0 (direct) | 0.3 |
| BoardRequestImproveFacilitiesEEOYBThreshold | BoardRequest | FUN_0084c990 | FUN_007479e0 (direct) | compute-only |
| BoardRequestImproveFacilitiesHappyEnoughLevel | BoardRequest | FUN_0084cc60 | FUN_007479e0 (direct) | compute-only |
| BoardRequestImproveFacilitiesHoneymoonPeriodMonthsThreshold | BoardRequest | FUN_0084c7b0 | FUN_007479e0 (direct) | compute-only |
| BoardRequestImproveFacilitiesUnhappinessThreshold | BoardRequest | FUN_0084cb70 | FUN_007479e0 (direct) | 0.3 |
| BoardUltimatumConditionBoardOpinionOnManagerThreshold | BoardRequest | FUN_0084d110 | FUN_007479e0 (direct) | compute-only |
| BoardUltimatumConditionEstimatedEndOfYearBalanceThreshold | BoardRequest | FUN_0084d200 | FUN_007479e0 (direct) | compute-only |
| BoardUltimatumConditionHoneymoonPeriodThreshold | BoardRequest | FUN_0084d020 | FUN_007479e0 (direct) | compute-only |
| BoardUltimatumFailureBoardOpinionReasonBoardOpinionChange | BoardRequest | FUN_0084d4d0 | FUN_007479e0 (direct) | compute-only |
| BoardUltimatumFailureEEOYBReasonBoardOpinionChange | BoardRequest | FUN_0084d5c0 | FUN_007479e0 (direct) | compute-only |
| BoardUltimatumFailureHoneymoonPeriodReasonBoardOpinionChange | BoardRequest | FUN_0084d3e0 | FUN_007479e0 (direct) | compute-only |
| BoardUltimatumSuccessBoardOpinionChange | BoardRequest | FUN_0084d2f0 | FUN_007479e0 (direct) | compute-only |
| CentralDefenderBias | TeamSelection | FUN_00820b70 | FUN_005db6b0 (direct) | 100 |
| ClubFinanceIsBankrupt | ClubInfo | FUN_00833010 | FUN_00581f60 (direct) | 1000 |
| ClubFinanceIsOkay | ClubInfo | FUN_00832ef0 | FUN_00581f60 (direct) | 1000 |
| ClubFinanceIsRich | ClubInfo | FUN_00832cb0 | FUN_00581f60 (direct) | 1000 |
| ClubFinanceIsSecure | ClubInfo | FUN_00832dd0 | FUN_00581f60 (direct) | 1000 |
| ClubFormHappy | PlayerHappiness | FUN_0082dac0 | FUN_0078a1e0 (direct) | compute-only |
| ClubFormUnhappy | PlayerHappiness | FUN_0082dbb0 | FUN_0078a1e0 (direct) | compute-only |
| ClubReputationAbilityFractionReserves | TeamSelection | FUN_00821d10 | FUN_00721ca0 (direct) | 100 |
| ClubReputationAbilityFractionSeniors | TeamSelection | FUN_00821c20 | FUN_00721ca0 (direct) | 100 |
| ClubReputationAbilityFractionYouth | TeamSelection | FUN_00821e00 | FUN_00721ca0 (direct) | 100 |
| CoachThresholdAdmires | CoachReport | FUN_0082c440 | FUN_00732410 (direct) | compute-only |
| CoachThresholdAverage | CoachReport | FUN_0082c620 | <none> (direct) | compute-only |
| CoachThresholdGood | CoachReport | FUN_0082c530 | FUN_00732410 (direct) | compute-only |
| CoachThresholdPoor | CoachReport | FUN_0082c710 | FUN_00732410 (direct) | compute-only |
| CoachThresholdSuperlative | CoachReport | FUN_0082c350 | FUN_00732410 (direct) | compute-only |
| CoachWage | WageLimits | FUN_00824290 | FUN_00581e50 (direct) | 1000 |
| ConfidenceRecoveryRate | PlayerHappiness | FUN_0082eba0 | FUN_0078a1e0 (direct) | compute-only |
| ContinentalTVRevenue | CompetitionIncome | FUN_00825190 | FUN_006ac9f0 (direct) | 100 |
| CupEarlyRoundTVRevenue | CompetitionIncome | FUN_008252b0 | FUN_00751f50 (direct) | 100 |
| CupLateRoundTVRevenue | CompetitionIncome | FUN_008253d0 | FUN_00751f50 (direct) | 100 |
| DailyConditionRecovery | PhysioInjuries | FUN_0082cad0 | FUN_0078b7a0 (direct) | 10 |
| DailyInjuryConditionReduction | PhysioInjuries | FUN_0082cbc0 | FUN_0078b7a0 (direct) | 10 |
| DefaultMatchFitnessThreshold | TeamSelection | FUN_00821b30 | FUN_00721ca0 (direct) | 100 |
| DefenderCountSellThreshold | TransferModule | FUN_00845220 | FUN_007c6770 (direct) | 6 |
| DefensiveMidfielderCenterBias | TeamSelection | FUN_00820db0 | FUN_005db6b0 (direct) | 100 |
| DefensiveMidfielderLeftBias | TeamSelection | FUN_00820c90 | FUN_005db6b0 (direct) | 100 |
| DefensiveMidfielderRightBias | TeamSelection | FUN_00820ed0 | FUN_005db6b0 (direct) | 100 |
| DesiredSellPriceToBaseValueTrialThreshold | TransferModule | FUN_008455e0 | FUN_007c6770 (direct) | compute-only |
| Dividends | FinancialCosts | FUN_00824cb0 | FUN_0074fd50 (direct) | 100 |
| EstimatedEndOfYearBalance | Budget | FUN_00825cd0 | FUN_00565810+FUN_00582130+FUN_0058ab60 (direct) | compute-only |
| EventHomesickDurationInterpFromMax | PersonHappiness | FUN_0084ac80 | FUN_006e30c0 (direct) | 540 |
| EventHomesickDurationInterpFromMin | PersonHappiness | FUN_0084ab90 | FUN_006e30c0 (direct) | compute-only |
| EventHomesickMaxRange | PersonHappiness | FUN_0084a410 | FUN_006e30c0 (direct) | compute-only |
| EventHomesickMinRange | PersonHappiness | FUN_0084a500 | FUN_006e30c0 (direct) | compute-only |
| EventHomesickStartTimeOffset | PersonHappiness | FUN_0084a5f0 | FUN_006e30c0 (direct) | compute-only |
| EventHomesickThresholdInterpFromMaxAge | PersonHappiness | FUN_0084a7d0 | FUN_006e30c0 (direct) | 40 |
| EventHomesickThresholdInterpFromMinAge | PersonHappiness | FUN_0084a6e0 | FUN_006e30c0 (direct) | 16 |
| EventHomesickThresholdInterpToMaxAdaptability | PersonHappiness | FUN_0084a9b0 | FUN_006e30c0 (direct) | 70 |
| EventHomesickThresholdInterpToMinAdaptability | PersonHappiness | FUN_0084a8c0 | FUN_006e30c0 (direct) | compute-only |
| EventHomesickThresholdNumberOfSuccessiveDaysInArea | PersonHappiness | FUN_0084aaa0 | FUN_006e30c0 (direct) | compute-only |
| EventPraiseDuration | PersonHappiness | FUN_00848070 | FUN_006e95c0 (direct) | compute-only |
| EventPraiseMaxOffsetOfAveragePerformance | PersonHappiness | FUN_00848160 | FUN_006e95c0 (direct) | compute-only |
| EventPraiseMaxRange | PersonHappiness | FUN_00847da0 | FUN_006e95c0 (direct) | compute-only |
| EventPraiseMinOffsetOfAveragePerformance | PersonHappiness | FUN_00848250 | FUN_006e95c0 (direct) | compute-only |
| EventPraiseMinRange | PersonHappiness | FUN_00847e90 | FUN_006e95c0 (direct) | compute-only |
| EventPraiseRepeatEffectFactor | PersonHappiness | FUN_00848340 | FUN_006e95c0 (direct) | 0.5 |
| EventPraiseStartTimeOffset | PersonHappiness | FUN_00847f80 | FUN_006e95c0 (direct) | compute-only |
| FirstTeam | WageLimits | FUN_00823990 | FUN_0058a4a0 (direct) | 1000 |
| FirstTeamPostRequestSuccess | WageLimits | FUN_00824710 | FUN_0058a4a0 (direct) | 1000 |
| FitnessThresholdNoConcerns | PhysioReport | FUN_0082bf10 | FUN_00732410 (direct) | compute-only |
| FitnessThresholdProne | PhysioReport | FUN_0082c240 | FUN_00732410 (direct) | compute-only |
| FitnessThresholdRatherProne | PhysioReport | FUN_0082c020 | FUN_00732410 (direct) | compute-only |
| FitnessThresholdSomewhatProne | PhysioReport | FUN_0082c130 | FUN_00732410 (direct) | compute-only |
| ForwardCenterBias | TeamSelection | FUN_008218f0 | FUN_005db6b0 (direct) | 100 |
| ForwardCountSellThreshold | TransferModule | FUN_00845400 | FUN_007c6770 (direct) | 4 |
| FringePlayer | WageLimits | FUN_00823bd0 | FUN_0058a4a0 (direct) | 1000 |
| FullBackLeftBias | TeamSelection | FUN_00820930 | FUN_005db6b0 (direct) | 100 |
| FullBackRightBias | TeamSelection | FUN_00820a50 | FUN_005db6b0 (direct) | 100 |
| GeneralInjuryChance | PhysioInjuries | FUN_0082c800 | FUN_0078b7a0 (direct) | compute-only |
| GoalKeeperBias | TeamSelection | FUN_008204b0 | FUN_005db6b0 (direct) | 100 |
| GoodTrainingCondiitons | PlayerHappiness | FUN_0082df70 | FUN_0078a1e0 (direct) | compute-only |
| HappinessAmbitionThreshold | PlayerHappiness | FUN_0082cf80 | FUN_0078a1e0 (direct) | 75 |
| HappinessMediaStatementThreshold | PlayerHappiness | FUN_0082e9c0 | FUN_0078a1e0 (direct) | compute-only |
| HappinessStateThreshold | PlayerHappiness | FUN_0082cda0 | FUN_0078a1e0 (direct) | 5 |
| HappinessStateThresholdMultiplier | PlayerHappiness | FUN_0082ce90 | FUN_0078a1e0 (direct) | 1.5 |
| HappinessStatementThreshold | PlayerHappiness | FUN_0082e8d0 | FUN_0078a1e0 (direct) | compute-only |
| HappinessTrainingThreshold | PlayerHappiness | FUN_0082d070 | FUN_0078a1e0 (direct) | 3 |
| HappinessUpdateFrequency | PlayerHappiness | FUN_0082ccb0 | FUN_0078a1e0 (direct) | 5 |
| HotProspect | WageLimits | FUN_00823cf0 | FUN_0058a4a0 (direct) | 1000 |
| HotProspectPostRequestSuccess | WageLimits | FUN_00824950 | FUN_0058a4a0 (direct) | 1000 |
| IndispensablePlayer | WageLimits | FUN_00823750 | FUN_00582740+FUN_0058a4a0+FUN_0058a6d0 (direct) | 1000 |
| InjuryFrequency | PhysioInjuries | FUN_0082c9e0 | FUN_0078b7a0 (direct) | 5 |
| InjuryRecurenceThreshold | PhysioReport | FUN_0082be00 | FUN_00732410 (direct) | compute-only |
| KeeperCountSellThreshold | TransferModule | FUN_00845130 | FUN_007c6770 (direct) | 2 |
| KeyPlayer | WageLimits | FUN_00823870 | FUN_0058a4a0 (direct) | 1000 |
| KeyPlayerPostRequestSuccess | WageLimits | FUN_008245f0 | FUN_0058a4a0 (direct) | 1000 |
| LoanMonthlyInterestPercent | FinancialCosts | FUN_00824fb0 | FUN_007502b0 (direct) | 0.458 |
| LoanThresholdBalance | Budget | FUN_00826200 | FUN_00565810 (direct) | compute-only |
| MajorConcernThreshold | PlayerHappiness | FUN_0082d160 | FUN_0078a1e0 (direct) | -10 |
| ManagerDisciplineBaseMoodiness | ManagerDiscipline | FUN_00845d60 | FUN_00732410 (direct) | 0.5 |
| ManagerDisciplineMistakeProbability | ManagerDiscipline | FUN_00845c70 | FUN_00732410 (direct) | 0.1 |
| ManagerDisciplineThreshold | ManagerDiscipline | FUN_00845f40 | FUN_00732410 (direct) | 90 |
| ManagerDisciplineViolenceThreshold | ManagerDiscipline | FUN_00845e50 | FUN_00732410 (direct) | 60 |
| ManagerSackingBaseWeightDomesticBoardOpinionOfManager | ManagerSacking | FUN_008531e0 | FUN_007a9570 (direct) | 10 |
| ManagerSackingBaseWeightDomesticLoyalty | ManagerSacking | FUN_008532d0 | FUN_007a9570 (direct) | 0.9 |
| ManagerSackingBaseWeightNationalBoardOpinionOfManager | ManagerSacking | FUN_008533c0 | FUN_007a9570 (direct) | 10 |
| ManagerSackingBaseWeightNationalLoyalty | ManagerSacking | FUN_008534b0 | FUN_007a9570 (direct) | 0.9 |
| ManagerSackingSackabilityNationalMainPhaseThresholdHigh | ManagerSacking | FUN_00853f30 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalMainPhaseThresholdLow | ManagerSacking | FUN_00854040 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalMainToQualifyingPhaseThresholdHigh | ManagerSacking | FUN_00854150 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalMainToQualifyingPhaseThresholdLow | ManagerSacking | FUN_00854260 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalQualifyingPhaseThresholdHigh | ManagerSacking | FUN_00853af0 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalQualifyingPhaseThresholdLow | ManagerSacking | FUN_00853c00 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalQualifyingToMainPhaseThresholdHigh | ManagerSacking | FUN_00853d10 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityNationalQualifyingToMainPhaseThresholdLow | ManagerSacking | FUN_00853e20 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingSackabilityThresholdDomestic | ManagerSacking | FUN_008539e0 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingWarningDomesticFormGamesObservedThreshold | ManagerSacking | FUN_008538d0 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingWarningDomesticFormScoreThreshold | ManagerSacking | FUN_008537c0 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingWarningDomesticThresholdHigh | ManagerSacking | FUN_008535a0 | FUN_007a9570 (direct) | compute-only |
| ManagerSackingWarningDomesticThresholdLow | ManagerSacking | FUN_008536b0 | FUN_007a9570 (direct) | compute-only |
| ManagerWage | WageLimits | FUN_00824050 | FUN_00581e50 (direct) | 1000 |
| MediaReportPerformanceThreshold | MatchReport | FUN_008458b0 | FUN_0070b150 (direct) | 85 |
| MediaReportReputationThreshold | MatchReport | FUN_008457c0 | FUN_0070b150 (direct) | 30 |
| MediaReportScoutedPlayerGoodPerformanceThreshold | MatchReport | FUN_00845a90 | FUN_0070b150 (direct) | 80 |
| MediaReportScoutedPlayerPoorPerformanceThreshold | MatchReport | FUN_00845b80 | FUN_0070b150 (direct) | 30 |
| MediaReportSignificantPerformanceThreshold | MatchReport | FUN_008459a0 | FUN_0070b150 (direct) | 90 |
| MediaReportSignificantScoreThreshold | MatchReport | FUN_008456d0 | FUN_0070b150 (direct) | 3 |
| MidfielderCenterBias | TeamSelection | FUN_00821110 | FUN_005db6b0 (direct) | 100 |
| MidfielderCountSellThreshold | TransferModule | FUN_00845310 | FUN_007c6770 (direct) | 8 |
| MidfielderLeftBias | TeamSelection | FUN_00820ff0 | FUN_005db6b0 (direct) | 100 |
| MidfielderRightBias | TeamSelection | FUN_00821230 | FUN_005db6b0 (direct) | 100 |
| MinimumNegotiatePercentage | TransferModule | FUN_00844f50 | FUN_007c6770 (direct) | 90 |
| MinimumTimePlayedForReputationUpdate | PlayerReputationPostMatchProgression | FUN_00858480 | FUN_00792b60 (direct) | compute-only |
| MinimumTransferListedNegotiatePercentage | TransferModule | FUN_00845040 | FUN_007c6770 (direct) | 75 |
| MonthsTillHomesick | PlayerHappiness | FUN_0082d340 | FUN_0078a1e0 (direct) | 3 |
| MonthsTillNaturalised | PlayerHappiness | FUN_0082d430 | FUN_0078a1e0 (direct) | 36 |
| MoraleRecoveryRate | PlayerHappiness | FUN_0082eab0 | FUN_0078a1e0 (direct) | compute-only |
| MutateConditionScale | PostMatchMutation | FUN_0084b220 | FUN_00721ca0 (direct) | 0.5 |
| MutateConfidenceScale | PostMatchMutation | FUN_0084b310 | FUN_00721ca0 (direct) | 0.4 |
| MutateMoraleScale | PostMatchMutation | FUN_0084b130 | FUN_00721ca0 (direct) | 0.1 |
| NeededPlayerSellPriceMultipler | TransferModule | FUN_008454f0 | FUN_007c6770 (direct) | compute-only |
| NeedsRestThreshold | TeamSelection | FUN_008203c0 | FUN_005f6370+FUN_00721ca0 (direct) | 100 |
| NewPlayerSalaryHighCap | NewPlayerSalary | FUN_00859c00 | FUN_00714440 (direct) | compute-only |
| NewPlayerSalaryLowCap | NewPlayerSalary | FUN_00859b10 | FUN_00714440 (direct) | compute-only |
| NewPlayerSalaryWageBudgetPercent | NewPlayerSalary | FUN_00859cf0 | FUN_00714440 (direct) | compute-only |
| NotNeeded | WageLimits | FUN_00823f30 | FUN_0058a4a0 (direct) | 1000 |
| NotNeededPostRequestSuccess | WageLimits | FUN_00824b90 | FUN_0058a4a0 (direct) | 1000 |
| NotSelectedLastFixture | PlayerHappiness | FUN_0082d9d0 | FUN_0078a1e0 (direct) | compute-only |
| PercentageHomeReputationIncrease | PlayerReputationPostMatchProgression | FUN_008582a0 | FUN_00792b60 (direct) | compute-only |
| PercentageWorldReputationIncrease | PlayerReputationPostMatchProgression | FUN_00858390 | FUN_00792b60 (direct) | compute-only |
| PhysiosWage | WageLimits | FUN_008243b0 | FUN_00581e50 (direct) | 1000 |
| PlayedLastFixture | PlayerHappiness | FUN_0082d7f0 | FUN_0078a1e0 (direct) | compute-only |
| PlayedLastFixtureSubbed | PlayerHappiness | FUN_0082d8e0 | FUN_0078a1e0 (direct) | compute-only |
| PlayerCriticism | PlayerHappiness | FUN_0082e240 | FUN_0078a1e0 (direct) | compute-only |
| PlayerFine | PlayerHappiness | FUN_0082e6f0 | FUN_0078a1e0 (direct) | compute-only |
| PlayerNotifyAbilityThreshold | TransferListNotify | FUN_008595c0 | FUN_007a9b50 (direct) | compute-only |
| PlayerNotifyPotentialThreshold | TransferListNotify | FUN_008596d0 | FUN_007a9b50 (direct) | compute-only |
| PlayerPraise | PlayerHappiness | FUN_0082e150 | FUN_0078a1e0 (direct) | compute-only |
| PlayerProfessionalismModifierFine | PlayerProfessionalism | FUN_00846030 | FUN_0078a1e0 (direct) | compute-only |
| PlayerTransferListed | PlayerHappiness | FUN_0082e510 | FUN_0078a1e0 (direct) | compute-only |
| PlayerUndeservedCriticism | PlayerHappiness | FUN_0082e420 | FUN_0078a1e0 (direct) | compute-only |
| PlayerUndeservedFine | PlayerHappiness | FUN_0082e7e0 | FUN_0078a1e0 (direct) | compute-only |
| PlayerUndeservedPraise | PlayerHappiness | FUN_0082e330 | FUN_0078a1e0 (direct) | compute-only |
| PlayerUnemployed | PlayerHappiness | FUN_0082d520 | FUN_0078a1e0 (direct) | compute-only |
| PoorTrainingCondiitons | PlayerHappiness | FUN_0082e060 | FUN_0078a1e0 (direct) | compute-only |
| ProspectiveManagersAffordableSalaryRemainingWageBudgetFactor | ProspectiveManagers | FUN_0084ea80 | FUN_007a02f0 (direct) | 0.5 |
| ProspectiveManagersBaseWeightDomesticComponentAmbition | ProspectiveManagers | FUN_008503f0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightDomesticComponentClubAreaPersonNationContrast | ProspectiveManagers | FUN_008508a0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightDomesticComponentCurrentAbility | ProspectiveManagers | FUN_0084fd60 | FUN_007a02f0 (direct) | 55 |
| ProspectiveManagersBaseWeightDomesticComponentDetermination | ProspectiveManagers | FUN_00850300 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightDomesticComponentHomeReputation | ProspectiveManagers | FUN_00850030 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightDomesticComponentLanguageCompetency | ProspectiveManagers | FUN_008505d0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightDomesticComponentLoyalty | ProspectiveManagers | FUN_0084ff40 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightDomesticComponentPotentialAbility | ProspectiveManagers | FUN_0084fe50 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightDomesticComponentPreviousClub | ProspectiveManagers | FUN_008504e0 | FUN_007a02f0 (direct) | 20 |
| ProspectiveManagersBaseWeightDomesticComponentProfessionalism | ProspectiveManagers | FUN_00850210 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightDomesticComponentRivalry | ProspectiveManagers | FUN_008506c0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightDomesticComponentUnemployment | ProspectiveManagers | FUN_008507b0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightDomesticComponentWorldReputation | ProspectiveManagers | FUN_00850120 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightNationalComponentAmbition | ProspectiveManagers | FUN_00851020 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightNationalComponentClubAreaPersonNationContrast | ProspectiveManagers | FUN_008514d0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightNationalComponentCurrentAbility | ProspectiveManagers | FUN_00850990 | FUN_007a02f0 (direct) | 55 |
| ProspectiveManagersBaseWeightNationalComponentDetermination | ProspectiveManagers | FUN_00850f30 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightNationalComponentHomeReputation | ProspectiveManagers | FUN_00850c60 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightNationalComponentLanguageCompetency | ProspectiveManagers | FUN_00851200 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightNationalComponentLoyalty | ProspectiveManagers | FUN_00850b70 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightNationalComponentPotentialAbility | ProspectiveManagers | FUN_00850a80 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersBaseWeightNationalComponentPreviousClub | ProspectiveManagers | FUN_00851110 | FUN_007a02f0 (direct) | 20 |
| ProspectiveManagersBaseWeightNationalComponentProfessionalism | ProspectiveManagers | FUN_00850e40 | <none> (direct) | compute-only |
| ProspectiveManagersBaseWeightNationalComponentRivalry | ProspectiveManagers | FUN_008512f0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightNationalComponentUnemployment | ProspectiveManagers | FUN_008513e0 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersBaseWeightNationalComponentWorldReputation | ProspectiveManagers | FUN_00850d50 | FUN_007a02f0 (direct) | 10 |
| ProspectiveManagersComponentValueDisfavourManagerAtRivalClubDoesntExist | ProspectiveManagers | FUN_00851c50 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueDisfavourManagerAtRivalClubExists | ProspectiveManagers | FUN_00851b60 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpSrcMax | ProspectiveManagers | FUN_008523d0 | FUN_007a02f0 (direct) | 80 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpSrcMin | ProspectiveManagers | FUN_008522e0 | FUN_007a02f0 (direct) | 20 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpTrgMax | ProspectiveManagers | FUN_008525b0 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpTrgMin | ProspectiveManagers | FUN_008524c0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpSrcMax | ProspectiveManagers | FUN_00852010 | FUN_007a02f0 (direct) | 80 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpSrcMin | ProspectiveManagers | FUN_00851f20 | FUN_007a02f0 (direct) | 20 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpTrgMax | ProspectiveManagers | FUN_008521f0 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpTrgMin | ProspectiveManagers | FUN_00852100 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueDomesticLanguageCompetencyFluent | ProspectiveManagers | FUN_008515c0 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueDomesticLanguageCompetencyNone | ProspectiveManagers | FUN_008517a0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueDomesticLanguageCompetencySlight | ProspectiveManagers | FUN_008516b0 | FUN_007a02f0 (direct) | 65 |
| ProspectiveManagersComponentValueFavourUnemployedManagerWhenEmployed | ProspectiveManagers | FUN_00851e30 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueFavourUnemployedManagerWhenUnemployed | ProspectiveManagers | FUN_00851d40 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpSrcMax | ProspectiveManagers | FUN_00852b50 | FUN_007a02f0 (direct) | 80 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpSrcMin | ProspectiveManagers | FUN_00852a60 | FUN_007a02f0 (direct) | 20 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpTrgMax | ProspectiveManagers | FUN_00852d30 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpTrgMin | ProspectiveManagers | FUN_00852c40 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpSrcMax | ProspectiveManagers | FUN_00852790 | FUN_007a02f0 (direct) | 80 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpSrcMin | ProspectiveManagers | FUN_008526a0 | FUN_007a02f0 (direct) | 20 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpTrgMax | ProspectiveManagers | FUN_00852970 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpTrgMin | ProspectiveManagers | FUN_00852880 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueNationalLanguageCompetencyFluent | ProspectiveManagers | FUN_00851890 | FUN_007a02f0 (direct) | 100 |
| ProspectiveManagersComponentValueNationalLanguageCompetencyNone | ProspectiveManagers | FUN_00851a70 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentValueNationalLanguageCompetencySlight | ProspectiveManagers | FUN_00851980 | FUN_007a02f0 (direct) | 65 |
| ProspectiveManagersComponentWeightDomesticHomeReputationFactorWhenForeignManager | ProspectiveManagers | FUN_00852f10 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentWeightDomesticWorldReputationFactorWhenHomeManager | ProspectiveManagers | FUN_00852e20 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentWeightNationalHomeReputationFactorWhenForeignManager | ProspectiveManagers | FUN_008530f0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersComponentWeightNationalWorldReputationFactorWhenHomeManager | ProspectiveManagers | FUN_00853000 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersContinentalContrastBelievabilityEconomicRatingThreshold | ProspectiveManagers | FUN_0084fb40 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersContinentalContrastBelievabilityReputationThreshold | ProspectiveManagers | FUN_0084fc50 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersFixtureImminentPrestigiousnessThreshold | ProspectiveManagers | FUN_0084ec80 | FUN_007a02f0 (direct) | 95 |
| ProspectiveManagersManagerSearchingDesperatenessFactorMultiplier | ProspectiveManagers | FUN_0084fa30 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersManagerUnapproachableAsProspectiveManagerPeriodInMonths | ProspectiveManagers | FUN_0084eb70 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersNumMonthsRestrictingPreviousManagerReturning | ProspectiveManagers | FUN_0084ed70 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersObscureManagerlessClubFrequencyMaxDays | ProspectiveManagers | FUN_0084e790 | FUN_007a02f0 (direct) | 150 |
| ProspectiveManagersObscureManagerlessClubFrequencyMinDays | ProspectiveManagers | FUN_0084e880 | FUN_007a02f0 (direct) | 7 |
| ProspectiveManagersObscureManagerlessClubReputationObservationThreshold | ProspectiveManagers | FUN_0084e970 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationBandOffsetDomestic | ProspectiveManagers | FUN_0084ee80 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationBandOffsetNational | ProspectiveManagers | FUN_0084ef90 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationBandOffsetNationalBoostSameArea | ProspectiveManagers | FUN_0084f0a0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationBaseOffsetDomestic | ProspectiveManagers | FUN_0084f1b0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationBaseOffsetNational | ProspectiveManagers | FUN_0084f2c0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationSafetyBaseOffsetDomesticMax | ProspectiveManagers | FUN_0084f4e0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationSafetyBaseOffsetDomesticMin | ProspectiveManagers | FUN_0084f3d0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationSafetyBaseOffsetNationalMax | ProspectiveManagers | FUN_0084f700 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersReputationSafetyBaseOffsetNationalMin | ProspectiveManagers | FUN_0084f5f0 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersUnemployedAdvantageDomestic | ProspectiveManagers | FUN_0084f810 | FUN_007a02f0 (direct) | compute-only |
| ProspectiveManagersUnemployedAdvantageNational | ProspectiveManagers | FUN_0084f920 | FUN_007a02f0 (direct) | compute-only |
| ReputationManagerAnnualAward | Reputation | FUN_00846e40 | FUN_00792b60 (direct) | compute-only |
| ReputationManagerMonthlyAward | Reputation | FUN_00846b70 | FUN_00792b60 (direct) | compute-only |
| ReputationPlayerAnnualAward | Reputation | FUN_00846f30 | FUN_00792b60 (direct) | compute-only |
| ReputationPlayerDecreaseClubTransfer | Reputation | FUN_008468a0 | FUN_00792b60 (direct) | compute-only |
| ReputationPlayerIncreaseClubTransfer | Reputation | FUN_008467b0 | FUN_00792b60 (direct) | compute-only |
| ReputationPlayerManOfMatch | Reputation | FUN_00846a80 | FUN_00792b60 (direct) | compute-only |
| ReputationPlayerMonthlyAward | Reputation | FUN_00846c60 | FUN_00792b60 (direct) | compute-only |
| ReputationWorldRatio | Reputation | FUN_00846990 | FUN_00792b60 (direct) | compute-only |
| ReputationYouthPlayerAnnualAward | Reputation | FUN_00847020 | FUN_00792b60 (direct) | compute-only |
| ReputationYouthPlayerMonthlyAward | Reputation | FUN_00846d50 | FUN_00792b60 (direct) | compute-only |
| RollForwardPercentageChance | RollForwardData | FUN_00856460 | FUN_007194e0 (direct) | compute-only |
| RollForwardWeeks | RollForwardData | FUN_00856350 | FUN_007194e0 (direct) | compute-only |
| Rotation | WageLimits | FUN_00823ab0 | FUN_0058a4a0 (direct) | 1000 |
| RotationPostRequestSuccess | WageLimits | FUN_00824830 | FUN_0058a4a0 (direct) | 1000 |
| ScoutWage | WageLimits | FUN_008244d0 | FUN_00581e50 (direct) | 1000 |
| SimulatedResolverGoalFrequency | MatchModule | FUN_00834420 | FUN_0077cb80 (direct) | 0.5 |
| SlightConcernThreshold | PlayerHappiness | FUN_0082d250 | FUN_0078a1e0 (direct) | -5 |
| StarPlayerAbilityRatio | PlayerHappiness | FUN_0082ec90 | FUN_0078a1e0 (direct) | 1.1 |
| StarPlayerReputationRatio | TransferModule | FUN_00844e60 | FUN_007c6770 (direct) | 1.1 |
| StarPlayerSold | PlayerHappiness | FUN_0082e600 | FUN_0078a1e0 (direct) | compute-only |
| StrikerBias | TeamSelection | FUN_00821a10 | FUN_005db6b0 (direct) | 100 |
| SubPlayedLastFixture | PlayerHappiness | FUN_0082d700 | FUN_0078a1e0 (direct) | compute-only |
| SubWatchedLastFixture | PlayerHappiness | FUN_0082d610 | FUN_0078a1e0 (direct) | compute-only |
| SweeperBias | TeamSelection | FUN_008205d0 | FUN_005db6b0 (direct) | 100 |
| TargetRangeLowerOffset | PlayerReputationPostMatchProgression | FUN_008586a0 | FUN_00792b60 (direct) | compute-only |
| TargetRangeUpperOffset | PlayerReputationPostMatchProgression | FUN_00858590 | FUN_00792b60 (direct) | compute-only |
| TaxPercent | FinancialCosts | FUN_00824dd0 | FUN_00750070 (direct) | compute-only |
| TiredThreshold | TeamSelection | FUN_008202d0 | FUN_00721ca0 (direct) | 100 |
| TrainingFacilitiesUpdateCostLevel0 | TrainingFacilities | FUN_00854370 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel1 | TrainingFacilities | FUN_00854480 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel2 | TrainingFacilities | FUN_00854590 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel3 | TrainingFacilities | FUN_008546a0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel4 | TrainingFacilities | FUN_008547b0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel5 | TrainingFacilities | FUN_008548c0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel6 | TrainingFacilities | FUN_008549d0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel7 | TrainingFacilities | FUN_00854ae0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel8 | TrainingFacilities | FUN_00854bf0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateCostLevel9 | TrainingFacilities | FUN_00854d00 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel0Max | TrainingFacilities | FUN_00854f20 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel0Min | TrainingFacilities | FUN_00854e10 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel1Max | TrainingFacilities | FUN_00855140 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel1Min | TrainingFacilities | FUN_00855030 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel2Max | TrainingFacilities | FUN_00855360 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel2Min | TrainingFacilities | FUN_00855250 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel3Max | TrainingFacilities | FUN_00855580 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel3Min | TrainingFacilities | FUN_00855470 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel4Max | TrainingFacilities | FUN_008557a0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel4Min | TrainingFacilities | FUN_00855690 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel5Max | TrainingFacilities | FUN_008559c0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel5Min | TrainingFacilities | FUN_008558b0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel6Max | TrainingFacilities | FUN_00855be0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel6Min | TrainingFacilities | FUN_00855ad0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel7Max | TrainingFacilities | FUN_00855e00 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel7Min | TrainingFacilities | FUN_00855cf0 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel8Max | TrainingFacilities | FUN_00856020 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel8Min | TrainingFacilities | FUN_00855f10 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel9Max | TrainingFacilities | FUN_00856240 | FUN_0074e560 (direct) | compute-only |
| TrainingFacilitiesUpdateRangeLevel9Min | TrainingFacilities | FUN_00856130 | FUN_0074e560 (direct) | compute-only |
| TrainingInjuryChance | PhysioInjuries | FUN_0082c8f0 | FUN_0078b7a0 (direct) | compute-only |
| TrainingTruancyFrequency | TrainingModule | FUN_00834240 | FUN_00798440 (direct) | 5 |
| TrainingTruancyProfessionalismThreshold | TrainingModule | FUN_00834330 | FUN_00798440 (direct) | 75 |
| TransferBudgetBalancePart | Budget | FUN_00825a90 | <none> (direct) | 100 |
| TransferBudgetIncomePart | Budget | FUN_00825bb0 | FUN_0058ab60 (direct) | 100 |
| TransferBudgetStartGame | Budget | FUN_00825970 | FUN_0058ab60 (direct) | 100 |
| WageBudgetMax | Budget | FUN_00825850 | FUN_00589620 (direct) | 100 |
| WageBudgetMin | Budget | FUN_00825730 | <none> (direct) | 100 |
| WageBudgetNegativeBalance | Budget | FUN_00825610 | FUN_00589620 (direct) | 100 |
| WageBudgetPositiveBalance | Budget | FUN_008254f0 | FUN_00589620 (direct) | 100 |
| WeeksUntilFluent | PlayerHappiness | FUN_0082ed80 | FUN_0078a580 (direct) | 50 |
| WeeksUntilSpoken | PlayerHappiness | FUN_0082eea0 | FUN_0078a580 (direct) | 20 |
| WeeksUntilStaffFluent | PersonAgeing | FUN_008474d0 | FUN_00789d50 (direct) | 50 |
| WeeksUntilStaffSpoken | PersonAgeing | FUN_008475f0 | FUN_00789d50 (direct) | 20 |
| WingBackLeftBias | TeamSelection | FUN_008206f0 | FUN_005db6b0 (direct) | 100 |
| WingBackRightBias | TeamSelection | FUN_00820810 | FUN_005db6b0 (direct) | 100 |
| WingerLeftBias | TeamSelection | FUN_008216b0 | FUN_005db6b0 (direct) | 100 |
| WingerRightBias | TeamSelection | FUN_008217d0 | FUN_005db6b0 (direct) | 100 |
| Youngster | WageLimits | FUN_00823e10 | FUN_0058a4a0 (direct) | 1000 |
| YoungsterPostRequestSuccess | WageLimits | FUN_00824a70 | FUN_0058a4a0 (direct) | 1000 |

</details>

## Classification confidence

- **VERIFIED** (bytes prove it): every row's getter VA/FUN, default value, consumer FUN, and (for 009f4bd0 rows) struct offset.
- **INFERRED** (name + architecture): the `match family` assignment. Goalkeeping (`Keeper*`, catch/save/punch/parry/tip) is folded into **Tackling/Defensive**; dribble/feint/trick/collision/fatigue into **Movement**; crossing/throwing into **Passing**; goal-mouth scoring (`OfficialState Mid*Score`) and ball/kick physics into **Shooting**; role-ability & mental `PlayerConstant` residuals into **Positioning**.
- **UNKNOWN**: all formula trees/bodies, per-frame reader arithmetic, condition/effect bodies — in `formula.xml.blowfish`.
