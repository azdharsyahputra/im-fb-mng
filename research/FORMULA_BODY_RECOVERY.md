# CM2008 Formula Body Recovery — Consumers & Inputs (Workstream B)

Clean-room, **static only**. No Ghidra run, no execution, no decryption of
`formula.xml.blowfish`, no invented coefficients or bodies. Evidence =
`scratchpad/formula.log` (829-func formula closure), `scratchpad/wave3.log`
(match-sim band), `scratchpad/getters_full.txt` (810 name/group/default/getter),
and **raw bytes** of `/Volumes/CM2008/Crack JPN/cm2008.exe` (file_offset = VA − 0x400000;
verified: `.text` VA 0x401000 == raw 0x1000, image base 0x400000).

**Scope of this doc:** map each (match-impacting) formula to (a) its **CONSUMER** —
the match-engine code that reads the getter's float — and (b) its **INPUT** — the
subject/context the accessor binds. The formula **trees/bodies** (inputs, weights,
position/duty modifiers, condition/effect bodies) live in the encrypted payload and are
**NOT** recovered here; see `FORMULA_SUBSYSTEM.md` / `BLOWFISH_DATA_RECOVERY.md`.

Classification per claim: **VERIFIED** (bytes/decompile prove it), **INFERRED**
(structure implies it), **UNKNOWN** (encrypted/anti-disassembled). Nothing invented.

---

## 0. TL;DR — the consumer architecture (the central finding)

The 810 formula getters are **not** read scattered across the match loop. They are
harvested by **exactly two centralized functions**, each of which calls every getter in
its domain **once** and writes the resulting float into a flat struct. Downstream
per-frame match code then reads the struct, never the getters. **VERIFIED** by an
exhaustive `E8 rel32` call scan of `.text` (806/810 getters resolve to a direct caller;
scan: `scratchpad/scan_callers.py`, `scratchpad/consumers.json`).

| Harvester | Getters it consumes | Input (subject) | Output | Match impact |
|---|---|---|---|---|
| **FUN_005d6dd0** | all **101 `PlayerConstant`** (passing/shooting/tackling/movement **player attributes**) | **a Player** (param at `[esp+0x14]`→`edi`, null-checked, reads `[edi+0x14]…`) | per-player scaled-attribute array (`this`=`ecx`=`esi`) | translates a player's ability ratings into the match engine's 0–100 attribute space |
| **FUN_009f4bd0** | **360 global constants** across 6 groups: `PlayerData`(187), `MatchResolver`(51), `PlayerState`(41), `SquadData`(37), `ResolverConstants`(26), `OfficialState`(18) | **none** (all getters are `float10 FUN_xxx(void)`) | one **ResolverSettings struct** at `[esi]` (≈0x5e0 bytes); each constant stored at a fixed `fstp dword [esi+disp32]` offset | holds all match physics + behavioural + positional + officiating tuning |

- **FUN_005d6dd0** signature (bytes `83ec08 56 57 8b7c2414 85ff 8bf1 0f84… 8b4714`):
  `__thiscall`, `this`=`ecx`→`esi` (output), 2nd arg `edi` = **Player*** (tested, then
  `mov eax,[edi+0x14]`). Each `PlayerConstant` getter takes that player handle
  (`float10 FUN_00818210(undefined4 param_1)`, e.g. `Composure`). **VERIFIED.**
- **FUN_009f4bd0** signature (bytes `81ec08040000 53 8b9c24141400 55 … 8be9 … 56`):
  `__thiscall`, `this`=`ecx`→`ebp`, store base `esi` = the settings struct; constants are
  `void` getters (no subject). **VERIFIED.**
- **FUN_009f4bd0 is called at match init**: `scratchpad/gap.log` shows it invoked (twice)
  inside the match-setup function (near `FUN_009f9400`, right after `__controlfp(0x20000,
  0x30000)` sets the match FP mode, alongside `FUN_009f7730/009ea050/009f15b0`). **VERIFIED.**
- **Downstream per-frame consumers** (the code that actually moves players, resolves
  passes/shots/tackles) read `[esi+off]` from this struct. Those reads are **INFERRED**
  (the struct offsets are proven; the specific reader functions are not individually
  traced here) and correspond to the reconstruction's `match/sim/{movement,defending,
  detailed_engine,resolver}.c`.

**Consequence for body recovery:** we recover, per formula, (1) its getter VA, (2) its
default, (3) its harvester consumer, (4) its input subject, and — uniquely — (5) its
**byte-exact slot offset in the match struct** (`fstp` displacement). The *formula tree*
that computes the value when the payload is present remains UNKNOWN; the default is the
fail-open value used when the formula is absent from the registry.

---

## 1. MOVEMENT / ACCELERATION  *(priority 1)*

**Group `MatchResolver` (51 formulas).** Consumer: **FUN_009f4bd0** (single caller, 1 call
each — VERIFIED). Input: **none** (global `void` getters). Stored to the ResolverSettings
struct at `[esi+0x09c .. esi+0x1b8]`. Defaults VERIFIED from bound `DAT_` floats.

| Formula | default | struct slot | Formula | default | struct slot |
|---|---|---|---|---|---|
| MinBaseAcceleration | 2 | esi+0xfc | MaxBaseAcceleration | 3 | esi+0x100 |
| MinBaseDeceleration | −4 | esi+0x104 | MaxBaseDeceleration | −5 | esi+0x108 |
| MinWalkingSpeed | 2 | esi+0x10c | MaxWalkingSpeed | 3 | esi+0x110 |
| MinJoggingSpeed | 3 | esi+0x114 | MaxJoggingSpeed | 5 | esi+0x118 |
| MinRunningSpeed | 6 | esi+0x11c | MaxRunningSpeed | 7 | esi+0x120 |
| MinSprintingSpeed | 8 | esi+0x124 | MaxSprintingSpeed | 9 | esi+0x128 |
| StandingAccelerationMultiplier | (compute) | esi+0x12c | StandingDecelerationMultiplier | (compute) | esi+0x140 |
| WalkingAccelerationMultiplier | (compute) | esi+0x130 | WalkingDecelerationMultiplier | (compute) | esi+0x144 |
| JoggingAccelerationMultiplier | 1.5 | esi+0x134 | JoggingDecelerationMultiplier | (compute) | esi+0x148 |
| SprintingAccelerationMultiplier | 5 | esi+0x138 | SprintingDecelerationMultiplier | (compute) | esi+0x14c |
| RunningAccelerationMultiplier | 2.5 | esi+0x13c | RunningDecelerationMultiplier | (compute) | esi+0x150 |
| BackingAccelerationMultiplier | 0.9 | esi+0x154 | BackingDecelerationMultiplier | 0.2 | esi+0x158 |
| MinBackingSpeed | 5 | esi+0x15c | MaxBackingSpeed | 6 | esi+0x160 |
| SidlingLeftAccelerationMultiplier | 1.5 | esi+0x164 | SidlingLeftDecelerationMultiplier | 2 | esi+0x168 |
| MinSidlingLeftSpeed | 5 | esi+0x16c | MaxSidlingLeftSpeed | 6 | esi+0x170 |
| SidlingRightAccelerationMultiplier | 1.5 | esi+0x174 | SidlingRightDecelerationMultiplier | 2 | esi+0x178 |
| MinSidlingRightSpeed | 5 | esi+0x17c | MaxSidlingRightSpeed | 6 | esi+0x180 |
| DivingLeftAccelerationMultiplier | 6 | esi+0x184 | DivingLeftDecelerationMultiplier | 8 | esi+0x188 |
| MinDivingLeftSpeed | 8 | esi+0x18c | MaxDivingLeftSpeed | 9 | esi+0x190 |
| DivingRightAccelerationMultiplier | 6 | esi+0x194 | DivingRightDecelerationMultiplier | 8 | esi+0x198 |
| MinDivingRightSpeed | 8 | esi+0x19c | MaxDivingRightSpeed | 9 | esi+0x1a0 |
| SlidingAccelerationMultiplier | 6 | esi+0x1a4 | SlidingDecelerationMultiplier | 8 | esi+0x1a8 |
| MinSlidingSpeed | 8 | esi+0x1ac | MaxSlidingSpeed | 9 | esi+0x1b0 |
| MinSpeedToTurningCircleRadius | 0.15 | esi+0xf4 | MaxSpeedToTurningCircleRadius | 0.3 | esi+0xf8 |
| RunningFatigueModifier | 0.002 | esi+0x1b4 | SprintingFatigueModifier | 0.02 | esi+0x1b8 |
| UpdateDegradableStatsPeriod | 60 | esi+0x9c | | | |

- Pattern (VERIFIED): a **gait model** with 8 gaits (Standing/Walking/Jogging/Running/
  Sprinting/Backing/Sidling L+R/Diving L+R/Sliding), each with Min/Max speed + accel/decel
  multiplier; plus base accel/decel bounds and a speed→turning-radius pair and two fatigue
  modifiers. The struct interleaves accel/decel in paired columns (e.g. accel at
  +0x12c..+0x13c, matching decel at +0x140..+0x150).
- `(compute)` = **no default** (`DAT_None`): the value is *compute-only* — meaningful only
  when the encrypted formula body exists; there is no fallback constant. These are the
  getters most dependent on the payload. The accel/decel **multipliers** for the base
  gaits (Standing/Walking/Jogging-decel/Running-decel/Sprinting-decel) are compute-only.
- **Player-side movement attributes** feeding the gait choice (`Mobility`, `Spring`,
  `BodyBalance`, `Height`, `Fitness`, `Acceleration`, `Deceleration`) are in
  `PlayerConstant` / are harvested by **FUN_005d6dd0** per player (§6), default 100.

**Recoverable:** getter→consumer→input→default→struct-offset = **VERIFIED**.
**Not recoverable:** how Min/Max + multiplier combine with the player attribute to yield
instantaneous accel (the formula tree), and the gait-selection logic in the payload = **UNKNOWN**.

---

## 2. PASSING / RECEIVING  *(priority 2)*

Two layers, both **VERIFIED** for consumer/input/default/offset:

**2a. Global passing tuning — group `PlayerData`, consumer FUN_009f4bd0, input none:**

| Formula | default | slot | Formula | default | slot |
|---|---|---|---|---|---|
| PassDifficulty | 22.5 | esi+0x1ec | HighPassDifficulty | 33.75 | esi+0x1f0 |
| ClosePassDistance | 8 | esi+0x204 | ShortPassChoiceDamper | 0.25 | esi+0x224 |
| MaxPassSafety | 6 | esi+0x234 | MinPassSafety | 4 | esi+0x238 |
| MaxHighPassSafety | 10 | esi+0x23c | MaxSpacePassSafety | 10 | esi+0x240 |
| MinHighSpacePassDistance | 625 | esi+0x248 | MinValidPassSpeed | 10 | esi+0x274 |
| MinimumPassRangeSquared | 81 | esp+0x24 (inline) | MinSpacePassDistance | 100 | (inline-use) |
| CompletedPassesPerformance | 0.1 | (perf, §6) | KeyPassesPerformance | 0.5 | (perf, §6) |

Compute-only (no default) passing formulas: `EasyPass`, `StraightPass`, `SidePass`,
`HighGoodControl`, `HighPassAngleRange`, `SpacePassAngleRange`, `MinSpacePassAngle`,
`FeedScale` — their value exists only in the payload.

**2b. Per-player passing attributes — group `PlayerConstant`, consumer FUN_005d6dd0,
input Player, default 100:** `PassingAccuracy` (FUN_0081e110), `PassCreativity`
(FUN_0081d930), `PassCaution` (FUN_00819650), `PassingRangeLeft`/`Right`
(FUN_00818570/690), `PassingAgilityLeft`/`Right` (FUN_008187b0/8d0), `BallControl`
(FUN_0081d5d0), `Composure` (FUN_00818210), `DeadballTakingSkill` (FUN_00818f90).
Ball-flight physics they key into live in `ResolverConstants` (§3).

**Recoverable:** full consumer/input/default/offset map = VERIFIED.
**Not recoverable:** the pass-success tree combining attribute × distance × safety ×
`PassDifficulty` = UNKNOWN (payload).

---

## 3. SHOOTING / FINISHING  *(priority 3)*

**3a. Ball & shot physics — group `ResolverConstants` (26), consumer FUN_009f4bd0, input
none, ALL compute-only (no default, `DAT_None`).** Stored `[esi+0x028..0x098]`:

| Formula | slot | Formula | slot |
|---|---|---|---|
| BaseBallSpeedGentle | esi+0x28 | BaseBallSpeedMedium | esi+0x2c |
| BaseBallSpeedMediumFirm | esi+0x30 | BaseBallSpeedFirm | esi+0x34 |
| BaseBallSpeedFirmer | esi+0x38 | BaseBallSpeedStrong | esi+0x3c |
| BaseBallSpeedMaximum | esi+0x40 | BaseKickReactionTime{Gentle..Maximum} | esi+0x48..0x60 |
| BaseBallKickHeight{Flat,Head,Directional,Overhead,High,Maximum} | esi+0x68..0x7c | BaseThrowReactionTime | esi+0x84 |
| BallElasticity | esi+0x88 | AccurateShotRangeVariance | esi+0x8c |
| AccurateShotAngleVariance | esi+0x90 | InaccurateShotRangeVariance | esi+0x94 |
| InaccurateShotAngleVariance | esi+0x98 | | |

This is the shared **ball-physics table** (7 speed tiers, 7 reaction-time tiers, 6 kick
heights, elasticity, and the accurate/inaccurate shot range+angle variance pairs that set
shot spread). Because every entry is compute-only, shot/ball physics is the family **most
dependent on the encrypted payload** — no usable fallback exists.

**3b. Shot-decision tuning — group `PlayerData`, consumer FUN_009f4bd0:**
`MustShootRange`=15 (esi+0x1f4), `MinShot`=20 (esi+0x1fc), `RunShotDifficulty`/
`SprintShotDifficulty` (compute-only), plus volley/header finish spans
(`HeaderFinishSpan`=2 esi+0x34c, `LongRangeVolley`=15 esi+0x37c, `HalfVolleyHeight`=0.6
esi+0x380). Goal-mouth scoring (`MidHeightScore`, `MidRangeScore`, `MidSoloScore`,
`MidSpeedScore`, `FastGoalSpeed`, `WideGoalTop/Bottom`) is in `OfficialState`.

**3c. Per-player shooting attributes — group `PlayerConstant`, consumer FUN_005d6dd0,
input Player, default 100:** `ShotAccuracy` (FUN_0081ec50), `ShootingBrain`
(FUN_0081da50), `ShootingFinesse` (FUN_0081db70), `ShotRangeLeft/Right`
(FUN_008189f0/8b10), `ShotComposureLeft/Right` (FUN_008190b0/91d0), `ShotAgilityLeft/
Right` (FUN_00818c30/d50), `CurlAbilityLeft/Right`, `PenaltyTakingSkill`, `DivingHeader`,
`HeadAccuracy`, `HeadRange`.

**Recoverable:** VERIFIED consumer/input/offset; defaults VERIFIED for PlayerData/
PlayerConstant, **absent** for all ResolverConstants. **Not recoverable:** the shot-outcome
tree (power/accuracy/variance → on-target/goal) = UNKNOWN.

---

## 4. TACKLING / DEFENSIVE  *(priority 4)*

**4a. Tackle/challenge mechanics — group `PlayerData`+`PlayerState`+`SquadData`, consumer
FUN_009f4bd0, input none:**

| Formula | grp | default | slot |
|---|---|---|---|
| MinBlockTackleResult | PlayerData | 10 | esi+0x280 |
| MaxBlockTackleResult | PlayerData | 35 | esi+0x284 |
| HookTackleBallArea | PlayerData | 6 | esi+0x2dc |
| LungeDistance | PlayerData | 4 | esi+0x368 |
| GoallineClearance | PlayerData | 4 | esi+0x36c |
| StabDistance | PlayerData | 6 | esi+0x378 |
| SlideTackleBallArea | PlayerData | 6 | esi+0x388 |
| SlideFoulResult | PlayerData | 0.6 | esi+0x38c |
| SmotherFoulResult | PlayerData | 0.6 | esi+0x330 |
| MinDefendDistance / MaxDefendDistance | PlayerState | 5 / 8 | esi+0x420 / 0x424 |
| SmotherDistance | PlayerState | 8 | esi+0x43c |
| SlideCautionDistance | PlayerState | 4 | esi+0x44c |
| MinTackleChaseRange / MaxTackleChaseRange | PlayerState | 6 / 12 | esi+0x454 / 0x458 |
| TackleEasy / TackleHard | SquadData | (compute) | esi+0x4d8 / 0x4dc |

Compute-only defensive formulas (payload-only): `BlockTackleRandom`, `BlockDeflection`,
`BlockEarliestWon/Lost`, `SlideBallDeflection/Effect`, `SlideTimeSpan`,
`SmotherBallDeflection/Effect`, `DirtyFoulResult`, `TrippingTime`, `ElbowingTime`.

**4b. Officiating / fouls — group `OfficialState`, consumer FUN_009f4bd0:**
`CynicalFoulRating`, `DirtyFoulRating`, `MinorFoulRating`, `NudgeFoulRating`,
`ProffessionalFoulAngle`, `MaximumWallPlayers` (all compute-only); `CardDistance`=2.

**4c. Per-player tackling attributes — group `PlayerConstant`, consumer FUN_005d6dd0,
input Player, default 100:** `TacklingAbility` (FUN_0081a190), `TackleFrequency`
(FUN_00819f50), `TackleCaution` (FUN_0081a2b0), `TackleAvoidance` (FUN_0081a4f0),
`TackleChaseRange` (FUN_008192f0), `BlockTackleStrength` (FUN_00819410),
`HardTackleFrequency` (FUN_0081a070), `Marking` (FUN_0081e230), `Aggression`,
`OverAggression`, `ShirtPulling`, `SmotheringAbility`.

---

## 5. POSITIONING / AWARENESS  *(priority 5)*

**Group `PlayerData`+`PlayerState`+`SquadData`, consumer FUN_009f4bd0, input none.** Key
positional constants and offsets (VERIFIED):

| Formula | default | slot |
|---|---|---|
| OffsideLineScale | 0.6 | esi+0x1c8 |
| OffsideLineScaleMin / Max | 0.3 / 0.9 | esi+0x1cc / 0x1d0 |
| FreeKickOffsideLineScale | 0.45 | esi+0x1d4 |
| CommitToRun | 0.1 | esi+0x1d8 |
| BehindBallScale | 0.5 | esi+0x1dc |
| PoorAngleBias | 0.5 | esi+0x218 |
| OwnHalfRun / DeepOwnHalfRun | −30 / −30 | esi+0x264 / 0x268 |
| CrossAreaWidth | 15 | esi+0x308 |
| DeepAreaLength | 40 | esi+0x30c |
| TouchlineAreaLength | 12 | esi+0x318 |
| AntiWidthBias | 4 | esi+0x394 |
| MaxRoamDistance | 80 | esi+0x3cc |
| ActiveRangeScale | 2 | (PlayerData) |
| CloseInMarking / TightMarking / LooseMarking | 30 / 3 / 8 | esi+0x3bc / 0x3b0 / 0x3b4 |
| MarkingDistanceScale | 0.02 | esi+0x3b8 |

Per-player awareness attributes — `PlayerConstant`, consumer FUN_005d6dd0, input Player,
default 100: `GameAwareness` (FUN_0081ccd0), `Alertness` (FUN_00819530), `Roaming`
(FUN_0081dc90), `RunFrequency` (FUN_0081ddb0), `Mobility`, plus the **positional-role
bias** family (`*CenterBias`, `*LeftBias`, `*RightBias`, `StrikerBias`, `SweeperBias`,
`GoalKeeperBias`, etc. — these set where a player drifts by duty).

---

## 6. PLAYER PERFORMANCE / MATCH RATING  *(priority 6)*

**Group `PlayerData` "…Performance" formulas — consumer FUN_009f4bd0, input none,
defaults VERIFIED (these ARE the rating weights).** Stored `[esi+0x53c..0x5d4]`. These are
the per-event contributions to a player's match rating:

| Formula | weight (default) | | Formula | weight (default) |
|---|---|---|---|---|
| KeyPassesPerformance | +0.5 | | CompletedPassesPerformance | +0.1 |
| AssistsPerformance | +0.6 | | BadPassesPerformance | −0.2 |
| OnTargetShotsPerformance | +0.3 | | BadShotsPerformance | −0.1 |
| TacklesWonPerformance | +0.1 | | BadTacklesPerformance | −0.05 |
| KeyTacklesPerformance | +0.3 | | TacklesAvoidedPerformance | +0.1 |
| InterceptionsPerformance | +0.2 | | ClearancePerformance | +0.1 |
| HeadersPerformance | +0.1 | | KeyHeadersPerformance | +0.3 |
| RunsPerformance | +0.1 | | KeptPossessionPerformance | +0.1 |
| LostPossessionPerformance | −0.1 | | FailedControlPerformance | −0.2 |
| OffsidePerformance | −0.02 | | CynicalPerformance | −0.4 |
| DirtyPerformance | −0.6 | | OwnGoalsPerformance | −0.7 |
| YellowCardPerformance | −1 | | RedCardPerformance | −2 |
| PenGoalsPerformance | +0.65 | | PenaltyMissedPerformance | −2 |
| GoalConcededKeeperPerformance | −0.5 | | PenaltySavedPerformance | +5 |
| AttackerGoalPerformance | +4 | | MidfielderGoalPerformance | +5 |
| KeeperDefenderGoalPerformance | +6 | | PerformanceBalancing | 1.1 |

(Keeper handling perf: `ShotsHeld/Parried/Tipped/FumbledPerformance`,
`CrossesTaken/Punched/FumbledPerformance` at esi+0x59x..0x5bx.) This is the match-rating
accumulator; `PerformanceBalancing`=1.1 is the global scaler. See
`PLAYER_PERFORMANCE_FORMULAS.md` for the sampler-side view. **Defaults VERIFIED**; the
exact accumulation tree (per-event × count × position modifier) is UNKNOWN (payload).

---

## 7. OTHER (non-match)  *(priority 7)*

`ProspectiveManagers`(77), `PlayerHappiness`(37), `BoardRequest`(37), `TrainingFacilities`
(30), `TeamSelection`(26), `WageLimits`(19), `ManagerSacking`(17), `PersonHappiness`(17),
`Reputation`(10), `Budget`(9), `TransferModule`(9), `ClubInfo`(4), etc. Each is a
`float10 FUN_xxx(subject)` getter with its own single caller (not in the match band
0x009xxxxx). Not expanded here; defaults are in `FORMULA_COEFFICIENT_REGISTRY.md`. Their
consumers live in the career/AI code, not the match sim.

---

## 8. What IS recoverable vs. what is in the encrypted payload

**Recoverable statically (this doc):**
- Getter VA + name + group + default for all 810. **VERIFIED** (`getters_full.txt`, byte-bound `DAT_`).
- Consumer function for each getter (806/810 via E8 scan). **VERIFIED**.
- The two-harvester architecture + match-init call-site. **VERIFIED**.
- Input subject per getter: Player handle (PlayerConstant via FUN_005d6dd0) vs. none/global
  (all others via FUN_009f4bd0). **VERIFIED** from signatures.
- **Byte-exact struct offset** for every FUN_009f4bd0 constant (300 `fstp [esi+disp]`).
  **VERIFIED** — a complete `ResolverSettings` layout (Appendix A).
- Which getters are compute-only (no default) vs. have a fallback default. **VERIFIED**.

**In the encrypted payload / anti-disassembled (NOT recovered — not invented):**
- Every formula **tree/body**: the inputs it reads, weights, aggregation, position/duty/
  tactical modifiers. **UNKNOWN** (`formula.xml.blowfish`).
- The downstream per-frame reader functions' exact arithmetic (how `[esi+off]` combines
  with player attributes to move/resolve). Struct offsets proven; reader bodies **INFERRED/UNKNOWN**.
- `condition`/`effect` bodies and effect-application targets. **UNKNOWN** (see
  `FORMULA_CONDITION_EFFECT_ENGINE.md`; anti-disassembled).
- For all `ResolverConstants` (ball/shot physics) and many movement multipliers:
  **no default exists**, so those values are fully payload-dependent.

---

## 9. Per-family coverage matrix

Stages: **Identified** (formulas+getters named) · **Data-flow mapped** (consumer+input+
offset) · **Algorithm recovered** (the body/tree) · **Implemented** (clean-room code) ·
**Validated** (tested vs. ground truth).

| Family | Identified | Data-flow mapped | Algorithm recovered | Implemented | Validated |
|---|---|---|---|---|---|
| 1 Movement/accel | ✅ 51 | ✅ FUN_009f4bd0, offsets 0x9c–0x1b8 | ⛔ tree in payload (5 decel mults compute-only) | ◐ evaluator skeleton (`formula_eval.c`) | ⛔ |
| 2 Passing/receiving | ✅ | ✅ FUN_009f4bd0 (PlayerData) + FUN_005d6dd0 (attrs) | ⛔ payload | ◐ | ⛔ |
| 3 Shooting/finishing | ✅ | ✅ offsets 0x28–0x98 + 0x1f4.. | ⛔ payload; **all ball physics compute-only** | ◐ | ⛔ |
| 4 Tackling/defensive | ✅ | ✅ PlayerData/State/Squad/Official + attrs | ⛔ payload | ◐ | ⛔ |
| 5 Positioning/awareness | ✅ | ✅ offsets + bias attrs | ⛔ payload | ◐ | ⛔ |
| 6 Player performance | ✅ | ✅ offsets 0x53c–0x5d4; **weights VERIFIED** | ◐ weights known, accumulation tree ⛔ | ◐ | ⛔ |
| 7 Other (non-match) | ✅ | ◑ caller known, not analysed | ⛔ | ⛔ | ⛔ |

**Biggest gaps:**
1. **All formula trees/bodies** remain in `formula.xml.blowfish` — the single blocking
   artifact. Every "Algorithm recovered" cell is ⛔ for this reason.
2. **Ball/shot physics (ResolverConstants) and 5 movement decel multipliers are
   compute-only** — no default fallback, so these families cannot even be approximated
   without the payload.
3. **Downstream per-frame readers not individually traced.** We proved the struct offsets
   (where values land) but not every match function that reads each `[esi+off]`; linking
   offset→reader→in-game-effect is the next static step (match band 0x009xxxxx in
   `wave3.log`; note: those functions do **not** call the getters directly — they read the
   harvested struct).
4. **condition/effect runtime** (rules) still UNKNOWN (anti-disassembled).

---

## Appendix A — Full FUN_009f4bd0 ResolverSettings struct offset map

300 `fstp dword [esi+disp32]` stores, VERIFIED from `.text` bytes
(`scratchpad/scan_callers.py`, `scratchpad/disasm2.py`,
`scratchpad/harvester_9f4bd0_offsets.txt`). Format: `Formula  Group  base+offset`.
`None` = read but used inline (not cached to the struct). Scripts + raw tables are in the
session scratchpad (ephemeral); regenerate from the binary with the two scripts above.

```
BaseBallSpeedGentle              ResolverConstants  esi+0x28
BaseBallSpeedMedium              ResolverConstants  esi+0x2c
BaseBallSpeedMediumFirm          ResolverConstants  esi+0x30
BaseBallSpeedFirm                ResolverConstants  esi+0x34
BaseBallSpeedFirmer              ResolverConstants  esi+0x38
BaseBallSpeedStrong              ResolverConstants  esi+0x3c
BaseBallSpeedMaximum             ResolverConstants  esi+0x40
BaseBallKickHeightFlat           ResolverConstants  esi+0x68
BaseBallKickHeightHead           ResolverConstants  esi+0x6c
BaseBallKickHeightDirectional    ResolverConstants  esi+0x70
BaseBallKickHeightOverhead       ResolverConstants  esi+0x74
BaseBallKickHeightHigh           ResolverConstants  esi+0x78
BaseBallKickHeightMaximum        ResolverConstants  esi+0x7c
BaseKickReactionTimeGentle       ResolverConstants  esi+0x48
BaseKickReactionTimeMedium       ResolverConstants  esi+0x4c
BaseKickReactionTimeMediumFirm   ResolverConstants  esi+0x50
BaseKickReactionTimeFirm         ResolverConstants  esi+0x54
BaseKickReactionTimeFirmer       ResolverConstants  esi+0x58
BaseKickReactionTimeStrong       ResolverConstants  esi+0x5c
BaseKickReactionTimeMaximum      ResolverConstants  esi+0x60
BaseThrowReactionTime            ResolverConstants  esi+0x84
BallElasticity                   ResolverConstants  esi+0x88
AccurateShotRangeVariance        ResolverConstants  esi+0x8c
AccurateShotAngleVariance        ResolverConstants  esi+0x90
InaccurateShotRangeVariance      ResolverConstants  esi+0x94
InaccurateShotAngleVariance      ResolverConstants  esi+0x98
UpdateDegradableStatsPeriod      MatchResolver      esi+0x9c
MaximumWallPlayers               OfficialState      None
NudgeFoulRating                  OfficialState      None
MinorFoulRating                  OfficialState      None
CynicalFoulRating                OfficialState      None
DirtyFoulRating                  OfficialState      None
ProffessionalFoulAngle           OfficialState      None
CardDistance                     OfficialState      esi+0xb8
LowGoalHeight                    OfficialState      esi+0xbc
HighGoalHeight                   OfficialState      esi+0xc0
SlowGoalSpeed                    OfficialState      esi+0xc4
FastGoalSpeed                    OfficialState      esi+0xc8
WideGoalTop                      OfficialState      esi+0xcc
WideGoalBottom                   OfficialState      esi+0xd0
MidRangeScore                    OfficialState      esi+0xd4
MidSpeedScore                    OfficialState      esi+0xd8
MidPassScore                     OfficialState      esi+0xdc
MidSoloScore                     OfficialState      esi+0xe0
MidHeightScore                   OfficialState      esi+0xe4
RunningFatigueModifier           MatchResolver      esi+0x1b4
SprintingFatigueModifier         MatchResolver      esi+0x1b8
MinBaseAcceleration              MatchResolver      esi+0xfc
MaxBaseAcceleration              MatchResolver      esi+0x100
MinBaseDeceleration              MatchResolver      esi+0x104
MaxBaseDeceleration              MatchResolver      esi+0x108
MinWalkingSpeed                  MatchResolver      esi+0x10c
MaxWalkingSpeed                  MatchResolver      esi+0x110
MinJoggingSpeed                  MatchResolver      esi+0x114
MaxJoggingSpeed                  MatchResolver      esi+0x118
MinRunningSpeed                  MatchResolver      esi+0x11c
MaxRunningSpeed                  MatchResolver      esi+0x120
MinSprintingSpeed                MatchResolver      esi+0x124
MaxSprintingSpeed                MatchResolver      esi+0x128
StandingAccelerationMultiplier   MatchResolver      esi+0x12c
StandingDecelerationMultiplier   MatchResolver      esi+0x140
WalkingAccelerationMultiplier    MatchResolver      esi+0x130
WalkingDecelerationMultiplier    MatchResolver      esi+0x144
JoggingAccelerationMultiplier    MatchResolver      esi+0x134
JoggingDecelerationMultiplier    MatchResolver      esi+0x148
RunningAccelerationMultiplier    MatchResolver      esi+0x13c
RunningDecelerationMultiplier    MatchResolver      esi+0x150
SprintingAccelerationMultiplier  MatchResolver      esi+0x138
SprintingDecelerationMultiplier  MatchResolver      esi+0x14c
MinSpeedToTurningCircleRadius    MatchResolver      esi+0xf4
MaxSpeedToTurningCircleRadius    MatchResolver      esi+0xf8
BackingAccelerationMultiplier    MatchResolver      esi+0x154
BackingDecelerationMultiplier    MatchResolver      esi+0x158
MinBackingSpeed                  MatchResolver      esi+0x15c
MaxBackingSpeed                  MatchResolver      esi+0x160
SlidingAccelerationMultiplier    MatchResolver      esi+0x1a4
SlidingDecelerationMultiplier    MatchResolver      esi+0x1a8
MinSlidingSpeed                  MatchResolver      esi+0x1ac
MaxSlidingSpeed                  MatchResolver      esi+0x1b0
SidlingLeftAccelerationMultiplier MatchResolver      esi+0x164
SidlingLeftDecelerationMultiplier MatchResolver      esi+0x168
MinSidlingLeftSpeed              MatchResolver      esi+0x16c
MaxSidlingLeftSpeed              MatchResolver      esi+0x170
SidlingRightAccelerationMultiplier MatchResolver      esi+0x174
SidlingRightDecelerationMultiplier MatchResolver      esi+0x178
MinSidlingRightSpeed             MatchResolver      esi+0x17c
MaxSidlingRightSpeed             MatchResolver      esi+0x180
DivingLeftAccelerationMultiplier MatchResolver      esi+0x184
DivingLeftDecelerationMultiplier MatchResolver      esi+0x188
MinDivingLeftSpeed               MatchResolver      esi+0x18c
MaxDivingLeftSpeed               MatchResolver      esi+0x190
DivingRightAccelerationMultiplier MatchResolver      esi+0x194
DivingRightDecelerationMultiplier MatchResolver      esi+0x198
MinDivingRightSpeed              MatchResolver      esi+0x19c
MaxDivingRightSpeed              MatchResolver      esi+0x1a0
SetupPower                       PlayerData         esi+0x1bc
SkipDelay                        PlayerData         esi+0x1c0
CollisionInjuryChance            PlayerData         esi+0x1c4
PhysicalFatigue                  PlayerData         esi+0xe8
MentalFatigue                    PlayerData         esi+0xec
SkillFatigue                     PlayerData         esi+0xf0
OffsideLineScale                 PlayerData         esi+0x1c8
OffsideLineScaleMin              PlayerData         esi+0x1cc
OffsideLineScaleMax              PlayerData         esi+0x1d0
FreeKickOffsideLineScale         PlayerData         esi+0x1d4
BehindBallScale                  PlayerData         esi+0x1dc
CommitToRun                      PlayerData         esi+0x1d8
KickFxScale                      PlayerData         esi+0x1e0
HeadFxScale                      PlayerData         esi+0x1e4
MinimumPassRangeSquared          PlayerData         esp+0x24
PassDifficulty                   PlayerData         esi+0x1ec
HighPassDifficulty               PlayerData         esi+0x1f0
MustShootRange                   PlayerData         esi+0x1f4
CutInDistance                    PlayerData         esi+0x1f8
MinShot                          PlayerData         esi+0x1fc
EaseOfDribble                    PlayerData         esi+0x200
ClosePassDistance                PlayerData         esi+0x204
EasyPass                         PlayerData         None
StraightPass                     PlayerData         None
SidePass                         PlayerData         None
FeintRangeStart                  PlayerData         esi+0x208
FeintRange                       PlayerData         esi+0x20c
MinimumDribbleCaution            PlayerData         esi+0x210
StandardDribbleCaution           PlayerData         esi+0x214
PoorAngleBias                    PlayerData         esi+0x218
FeedScale                        PlayerData         None
FeedCloseRange                   PlayerData         esi+0x220
ShortPassChoiceDamper            PlayerData         esi+0x224
MaxPassSafety                    PlayerData         esi+0x234
MinPassSafety                    PlayerData         esi+0x238
MaxHighPassSafety                PlayerData         esi+0x23c
MaxSpacePassSafety               PlayerData         esi+0x240
MinSpacePassDistance             PlayerData         esp+0x24
MinHighSpacePassDistance         PlayerData         esi+0x248
HighPassAngleRange               PlayerData         None
SpacePassAngleRange              PlayerData         None
MinSpacePassAngle                PlayerData         None
TooCloseToCurl                   PlayerData         esi+0x258
DiveDistance                     PlayerData         esi+0x25c
SprintDistance                   PlayerData         esi+0x260
OwnHalfRun                       PlayerData         esi+0x264
DeepOwnHalfRun                   PlayerData         esi+0x268
RunShotDifficulty                PlayerData         None
SprintShotDifficulty             PlayerData         None
MinValidPassSpeed                PlayerData         esi+0x274
LowGoodControl                   PlayerData         esi+0x278
HighGoodControl                  PlayerData         esi+0x27c
MinBlockTackleResult             PlayerData         esi+0x280
MaxBlockTackleResult             PlayerData         esi+0x284
MinDeflection                    PlayerData         esi+0x288
BestFeint                        PlayerData         esi+0x28c
ShirtPullingRange                PlayerData         esi+0x290
ShirtPullingBallRange            PlayerData         esi+0x294
PullShirtTime                    PlayerData         esi+0x298
PullShirtFrequency               PlayerData         esi+0x29c
ElbowRange                       PlayerData         esi+0x2a0
ElbowBallRange                   PlayerData         esi+0x2a4
ElbowingTime                     PlayerData         esi+0x2a8
BlockTackleRandom                PlayerData         None
BlockDeflection                  PlayerData         None
ElbowEffect                      PlayerData         None
ShirtPullingEffect               PlayerData         None
BlockEarliestLost                PlayerData         None
BlockEarliestWon                 PlayerData         None
TripRange                        PlayerData         esi+0x2c4
TripBallRange                    PlayerData         esi+0x2c8
TrippingTime                     PlayerData         esi+0x2cc
TrickDistance                    PlayerData         esi+0x2d0
DirtyFoulResult                  PlayerData         esi+0x2d4
HookFoulResult                   PlayerData         esi+0x2d8
HookTackleBallArea               PlayerData         esi+0x2dc
TripEffect                       PlayerData         None
HookAheadTarget                  PlayerData         None
HookBallDeflection               PlayerData         None
HookBallEffect                   PlayerData         None
DirtyBallDeflection              PlayerData         None
DirtyBallEffect                  PlayerData         None
SlideAheadTarget                 PlayerData         None
SpinAngle1                       PlayerData         None
LollyPopAngle1                   PlayerData         None
DummyAngle1                      PlayerData         None
CrossAreaWidth                   PlayerData         esi+0x308
DeepAreaLength                   PlayerData         esi+0x30c
LowCrossHeight                   PlayerData         esi+0x310
HighCrossHeight                  PlayerData         esi+0x314
TouchlineAreaLength              PlayerData         esi+0x318
CrossAbilityScale                PlayerData         esi+0x31c
MaxDistanceFromManCrossTarget    PlayerData         None
MaxDistanceFromAreaCrossTarget   PlayerData         None
MinDistanceToCrossTarget         PlayerData         None
HeadAgility                      PlayerData         esi+0x32c
SmotherFoulResult                PlayerData         esi+0x330
BallSpeedEffect                  PlayerData         esi+0x334
SmotherBallDeflection            PlayerData         None
SmotherBallEffect                PlayerData         None
HeaderTimeSpan                   PlayerData         esi+0x348
HeaderFinishSpan                 PlayerData         esi+0x34c
AttackHeaderDistance             PlayerData         esi+0x350
LongRangeDivingHeader            PlayerData         esi+0x354
LongRangeHeader                  PlayerData         esi+0x358
PressureJumpDistance             PlayerData         esi+0x35c
MinimumHeadRange                 PlayerData         esi+0x360
ExtraHeadRange                   PlayerData         esi+0x364
LungeDistance                    PlayerData         esi+0x368
GoallineClearance                PlayerData         esi+0x36c
UnderPressure                    PlayerData         esi+0x370
LayOffRange                      PlayerData         esi+0x374
StabDistance                     PlayerData         esi+0x378
LongRangeVolley                  PlayerData         esi+0x37c
HalfVolleyHeight                 PlayerData         esi+0x380
LongRangeBicycle                 PlayerData         esi+0x384
SlideTackleBallArea              PlayerData         esi+0x388
SlideFoulResult                  PlayerData         esi+0x38c
SlideTimeSpan                    PlayerData         esi+0x390
AntiWidthBias                    PlayerData         esi+0x394
HeaderTooClose                   PlayerData         esp+0x24
ExtraSaveReach                   PlayerData         esi+0x39c
CatchDistance                    PlayerData         esi+0x3a0
CloseDive                        PlayerData         esi+0x3a4
CloseKick                        PlayerData         esi+0x3a8
MaxReactionTime                  PlayerData         esi+0x3ac
TightMarking                     PlayerData         esi+0x3b0
LooseMarking                     PlayerData         esi+0x3b4
MarkingDistanceScale             PlayerData         esi+0x3b8
CloseInMarking                   PlayerData         esi+0x3bc
UninterestedRangeScale           PlayerData         esi+0x3c0
ActiveRangeScale                 PlayerData         esi+0x3c4
HuntRangeScale                   PlayerData         esi+0x3c8
MaxRoamDistance                  PlayerData         esi+0x3cc
TightAngleHeader                 PlayerData         None
TightAngleDivingHeader           PlayerData         None
TightAngleVolley                 PlayerData         None
TightAngleBicycle                PlayerData         None
SlideBallDeflection              PlayerData         None
SlideBallEffect                  PlayerData         None
ThrowInClose                     SquadData          esi+0x3e8
AttackingThrow                   SquadData          esi+0x4a4
ThrowRunUp                       PlayerState        esi+0x3ec
FreeKickRunUp                    PlayerState        esi+0x3f0
PenaltyRunUp                     PlayerState        esi+0x3f4
SetPieceFreeKickRunUp            PlayerState        esi+0x3f8
CornerRunUp                      PlayerState        esi+0x3fc
GoalKickRunUpScale               PlayerState        esi+0x400
HandsKickRunUp                   PlayerState        esi+0x404
CelebrationDistance              PlayerState        esi+0x408
KeeperCollision                  PlayerState        esi+0x40c
FallingForwards                  PlayerState        esi+0x410
FallingBackwards                 PlayerState        esi+0x414
MinComposure                     PlayerState        esi+0x418
MaxExtraComposure                PlayerState        esi+0x41c
MinDefendDistance                PlayerState        esi+0x420
MaxDefendDistance                PlayerState        esi+0x424
PenaltyAreaCaution               PlayerState        esi+0x428
PhysicalCautionDistance          PlayerState        esi+0x42c
DefaultRisingFreeKick            PlayerState        esi+0x430
MinimumKeeperUpdate              PlayerState        esi+0x434
KeeperWidthBias                  PlayerState        esi+0x438
SmotherDistance                  PlayerState        esi+0x43c
RunOutDistance                   PlayerState        esi+0x440
BowlFuture                       PlayerState        esi+0x444
RollFuture                       PlayerState        esi+0x448
SlideCautionDistance             PlayerState        esi+0x44c
LongRangeFreeKick                PlayerState        esi+0x450
MinTackleChaseRange              PlayerState        esi+0x454
MaxTackleChaseRange              PlayerState        esi+0x458
ValidPassApproachSpeed           PlayerState        esi+0x45c
ShortCornerDistance              PlayerState        None
TightAngleFreeKick               PlayerState        None
LollyPopAngle2                   PlayerState        None
SpinAngle2                       PlayerState        None
DummyAngle2                      PlayerState        None
TightCollisionAngle              PlayerState        None
RunAdjusterDegrees               PlayerState        None
TripAngle                        PlayerState        None
ShirtPullAngle                   PlayerState        None
ElbowAngle                       PlayerState        None
HookAngle                        PlayerState        None
SlideAngle                       PlayerState        None
Cluster                          SquadData          None
CloseFreeKickAngle               SquadData          None
OverFreeKickAngle                SquadData          None
BowlDistance                     SquadData          esi+0x490
RollDistance                     SquadData          esi+0x494
MinThrowDistance                 SquadData          esi+0x498
MinSetPieceDistance              SquadData          esi+0x49c
InitialDesperation               SquadData          esi+0x4a0
AttackingThrow                   SquadData          esi+0x4a4
MaxThrowInDistance               SquadData          esi+0x4a8
MinThrowInDistance               SquadData          esi+0x4ac
AttackingFreeKick                SquadData          esi+0x4b0
CloseToFreeKickTarget            SquadData          esi+0x4b4
MaxKickWidth                     SquadData          esi+0x4b8
ActiveRadius                     SquadData          esi+0x4bc
LosingDesperation                SquadData          esi+0x4c0
WinningMargin                    SquadData          esi+0x4c4
NoPressureTimeMinutes            SquadData          esi+0x4c8
NoPressureExtraTimeMinutes       SquadData          esi+0x4cc
MaxPressure                      SquadData          esi+0x4d0
PressureScale                    SquadData          esi+0x4d4
TackleEasy                       SquadData          esi+0x4d8
TackleHard                       SquadData          esi+0x4dc
GoForwardZScale                  SquadData          esi+0x4e0
GoForwardXScale                  SquadData          esi+0x4e4
FreeKickStayForwardZScale        SquadData          esi+0x4e8
CloseFreeKickDistance            SquadData          esi+0x4ec
OverFreeKickDistance             SquadData          esi+0x4f0
DisruptWallDistance              SquadData          esi+0x4f4
FreeKickBackZScale               SquadData          esi+0x4f8
FreeKickBackXScale               SquadData          esi+0x4fc
FrustrationScale                 SquadData          esi+0x500
AddedImpetus                     SquadData          esi+0x504
LosingImpetus                    SquadData          esi+0x508
RollTooClose                     SquadData          esi+0x50c
RedCardPerformance               PlayerData         esi+0x51c
YellowCardPerformance            PlayerData         esi+0x520
TouchesPerformance               PlayerData         esi+0x524
CompletedPassesPerformance       PlayerData         esi+0x528
BadPassesPerformance             PlayerData         esi+0x52c
KeyPassesPerformance             PlayerData         esi+0x530
OnTargetShotsPerformance         PlayerData         esi+0x534
BadShotsPerformance              PlayerData         esi+0x538
KeeperDefenderGoalPerformance    PlayerData         esi+0x53c
MidfielderGoalPerformance        PlayerData         esi+0x540
AttackerGoalPerformance          PlayerData         esi+0x544
PenGoalsPerformance              PlayerData         esi+0x548
OwnGoalsPerformance              PlayerData         esi+0x54c
BadTacklesPerformance            PlayerData         esi+0x550
TacklesWonPerformance            PlayerData         esi+0x554
KeyTacklesPerformance            PlayerData         esi+0x558
DirtyPerformance                 PlayerData         esi+0x55c
CynicalPerformance               PlayerData         esi+0x560
TacklesAvoidedPerformance        PlayerData         esi+0x564
GoodCrossesPerformance           PlayerData         esi+0x568
BadCrossesPerformance            PlayerData         esi+0x56c
GroundCoveredPerformance         PlayerData         esi+0x570
AssistsPerformance               PlayerData         esi+0x574
KeptPossessionPerformance        PlayerData         esi+0x578
FailedControlPerformance         PlayerData         esi+0x57c
InterceptionsPerformance         PlayerData         esi+0x580
RunsPerformance                  PlayerData         esi+0x584
ClearancePerformance             PlayerData         esi+0x588
HeadersPerformance               PlayerData         esi+0x58c
KeyHeadersPerformance            PlayerData         esi+0x590
OffsidePerformance               PlayerData         esi+0x594
TricksPerformance                PlayerData         esi+0x598
LostPossessionPerformance        PlayerData         esi+0x59c
CrossesTakenPerformance          PlayerData         esi+0x5a0
CrossesPunchedPerformance        PlayerData         esi+0x5a4
CrossesFumbledPerformance        PlayerData         esi+0x5a8
ShotsHeldPerformance             PlayerData         esi+0x5ac
ShotsParriedPerformance          PlayerData         esi+0x5b0
ShotsTippedPerformance           PlayerData         esi+0x5b4
ShotsFumbledPerformance          PlayerData         esi+0x5b8
GoalConcededKeeperPerformance    PlayerData         esi+0x5bc
GoalConcededDefenderPerformance  PlayerData         esi+0x5c0
CleanSheetKeeperDefenderPerformance PlayerData         esi+0x5c4
CleanSheetMidfielderPerformance  PlayerData         esi+0x5c8
PenaltySavedPerformance          PlayerData         esi+0x5cc
PenaltyMissedPerformance         PlayerData         esi+0x5d0
PerformanceBalancing             PlayerData         esi+0x5d4
```
