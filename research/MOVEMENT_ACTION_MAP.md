# CM2008 Match Engine — Priority 3: Player Actions & Movement (Behavioral Recovery)

Clean-room behavioral recovery from Ghidra decompilation dumps. No binary executed; no
Ghidra session opened. All decompiled text treated as DATA.

Sources read:
- `docs/ghidra_dumps/match_ai/match_movement_ai.c` (1.8 MB, 800 fns) + `..._index.csv`
- `docs/ghidra_dumps/raw/match_engine.c` + `match_engine_index.csv`
- `docs/ghidra_dumps/match_player/decompiled.c` + `index.txt` (better-typed copies of the same fns)

Evidence-confidence tags used throughout: **VERIFIED** (read in body), **INFERRED**
(deduced from pattern/context), **APPROXIMATE** (math read but semantics uncertain),
**UNKNOWN**.

---

## 0. HEADLINE FINDING — the anchor and the "movement" file are mis-named by the seeder

The extraction heuristic tagged functions by **nearby string literals** ("Positioning",
"AttackingMidfielder", "ClosingDown", "MarkingDistanceScale", …). Those strings are
**field names in reflection/serialization tables and config keys**, not evidence of
football logic. Verifying bodies (not names) shows that essentially every function in this
corpus belongs to one of five NON-simulation layers:

1. **Reflection / schema registration** — builds field-descriptor tables via string→atom
   interning (`FUN_00405530`, `FUN_004020d0`).
2. **Serialization / export** — dumps player/match records (`_fprintf`, `FUN_00413550`).
3. **Presentation** — match camera + 2D overlays (offside line, radar).
4. **Config accessors** — read tuning coefficients from a "PlayerData" config DB.
5. **Engine orchestration** — collects results of matches simulated on worker threads.

**The continuous movement PHYSICS (velocity integration, pursuit, pass/shot trajectory)
is NOT present as recoverable formula code in these dumps.** It runs inside the threaded
match sim whose *results* are collected by `GCMatchModule::ProcessData`
(`FUN_0077ee80`), and its tuning constants live in the external **PlayerData** config
table, not in these function bodies. This is the single most important thing to carry
forward: do not reconstruct movement math from this corpus — it is the wrong layer.

### The anchor FUN_00a1fdf0 is a player-attribute serializer, NOT movement — VERIFIED
`match_movement_ai.c:4002`, seed `[positioning, movement, attacking]`, 1 caller, 81 callees.
Body iterates a player collection and for each player registers ~60 attribute field names
then writes their values:
- `4124`: `if ((*(short *)(DAT_00c78384 + 0x1e) == 0) && (*(int *)(DAT_00c78384 + 0x24) != 0))`
  — picks a player-array base pointer (`DAT_00c78384+0x24`).
- `4660`: `puStack_70 = puVar9 + 0x1e;` with end `local_58 = base + count*0x3c` — **player
  record stride = 0x3c (60) bytes** (`0x1e` ushorts). **VERIFIED.**
- `4172`–`4346`: literal key registration via `FUN_00413550()` for
  `"Morale","Condition","TransferValue","BaseValue","Acceleration",…,"Positioning",
  "Passing",…,"Striker","Winger"`. The tag words come from these strings. **VERIFIED.**
- `00a1fdf0` is also listed in `match_player/index.txt` as
  `injuryresistance/injuryrecoveryrate` — same serializer, different string match. **VERIFIED.**

So `FUN_00a1fdf0` = "serialize every player's attributes". Not movement. (Its sibling
`FUN_00a20c20`, 14.8 KB, the largest fn, is the same thing to a FILE: 223× `_fprintf` +
`_fopen`, seed `attacking|marking|positioning|movement` — a debug text export. **VERIFIED.**)

---

## 1. MOVEMENT & POSITIONING

**What exists in-corpus:** match-VIEW positioning (camera + entity display), not player sim.

- **Camera orientation — `FUN_008b56d0`** (`match_movement_ai.c:38057`): deg→rad conversion
  into a view/camera object. **VERIFIED math:**
  - `38068`: `in_ECX[0x73] = (int)((float)auStack_c * 0.017453292);`  (× π/180)
  - `38069`: `in_ECX[0x74] = (int)((float)auStack_8 * 0.017453292);`
  - `38070`: `in_ECX[0x75] = (int)((float)(in_ECX + 0x66) * 0.5555556);` (× 1/1.8)
  - `0.017453292` = radians-per-degree. `0.5555556` ≈ 1/1.8 (deg/unit). Reads a value via
    vtable `(**(code **)(*in_ECX + 0x158))(...)`. → **camera pitch/yaw/zoom, PRESENTATION.**
- **fpatan helper — `FUN_00699dd0`** (`match_movement_ai.c:52303`, depth 2): `in_ST1 =
  fpatan(in_ST2,in_ST1);` — an atan2 wrapper. **SUPPORTING math helper.** VERIFIED.
- **Match-view per-entity update — `FUN_009ba5e0`** (`match_movement_ai.c:16522`, the ONLY
  fn seeded purely `movement`; 26 callees, 0 callers). This is the nearest thing to a
  per-tick update, but it updates a **displayed** entity:
  - `16526`: reads handle at `DAT_00c94cf8+0xac`; sentinels `0x80000` and `0xfffff` →
    treated as null (`iVar5 = 0`), else `index*0x20 + *(DAT_00c784a8+0x2c)` → resolve
    record (**stride 0x20 = 32 B**). **VERIFIED.**
  - `16537`: ushort handle at `+0xc0`; sentinels `0x8000`/`0xffff`; else
    `index*0x2c + *(DAT_00c779ac+0x24)` (**stride 0x2c = 44 B**). **VERIFIED.**
  - Caches resolved pointers into the view object at `+0x128,+0x12c,+0x130,+0x134,+0x138`;
    then `iVar5 = FUN_009b5cf0()` returns a state in {0,1,2,4} and dispatches to
    `FUN_009b5430/009b5b50/009b9490/009b8dc0/...`. → per-state **view** refresh. **INFERRED.**
  - The `0x8000/0xffff` and `0x80000/0xfffff` values are the engine's **null-handle
    sentinels** for player/ball references — a genuinely reusable fact. **VERIFIED.**

**Player movement physics (position x/y, velocity, speed clamp, pursuit target):
UNKNOWN / not in corpus.** No velocity-integration or target-seek arithmetic was found in
any traced body; the 193 "multiply" patterns in the file are serialization/index math, and
the only non-trivial float constants are the camera deg→rad and the pitch→screen scalers
below.

---

## 2. BALL POSSESSION & TRANSFER

- The possession CONCEPT is a **match-rule predicate atom**, not a flag computed here:
  `GCMatchPredicatePlayerWithBall`, `GCMatchPredicateTeamPossessionPortion` are interned in
  `FUN_008a7770` (see §7). **VERIFIED (string/atom registration).**
- `match_player/index.txt` and the `possession`-seeded index rows all resolve to
  predicate-registration or stat fields (`KeptPossession`, `LostPossession`). **VERIFIED.**
- **No read/write of a "has-ball" boolean on a player struct was found.** Possession state
  transfer lives in the threaded sim / FSM (§7), not in-corpus. **UNKNOWN (implementation).**

---

## 3. PASSING & RECEIVING

- "Pass" exists only as rule atoms / stat fields: `GCMatchPredicatePass`,
  `GCMatchAxiomBackPass`, stats `"Passes","CompletedPasses","PassesKey","PassesReceived",
  "PoorCrosses","Crosses"` (registered as serializable `uint32` stat columns). **VERIFIED.**
- "Passing" also appears as a **tactic field** in the TeamTactics schema (§6) and a player
  attribute (`PassingValue`). **VERIFIED (schema), decision math UNKNOWN.**
- **Pass selection/trajectory math: UNKNOWN / not in corpus.**

---

## 4. SHOOTING

- Only as axioms/stats: `GCMatchAxiomShotAtTarget`, `GCMatchAxiomShotHeader`,
  `GCMatchAxiomShotFreekick`; game states `eGameStateGoalSpotted/GoalActivated`,
  `eGameStatePenalty*`; stats `"ShotsOnTarget","ShotsHeld","ShotsParried","ShotsTipped",
  "ShotsFumbled"`. **VERIFIED (vocabulary only).**
- **Shot power/placement/xG-style math: UNKNOWN / not in corpus.**

---

## 5. TACKLING

- `GCMatchPredicateMadeTackle`; stats `"TacklesWon","TacklesKey","ChallengesMade",
  "ChallengesAvoided"`; tactic field `"Tackling"`; attribute `"Tackling"`/`TacklesWon`.
- `FUN_0097f380` (seed `tackle`, 16 callees, `match_movement_ai.c:15232`) — body is
  **19× `FUN_00405530` atom-interning** = tackle-field **schema registration**, not a
  tackle resolver. **VERIFIED.** The `tackle`-tagged rows are registration/stat code.
- **Tackle success/foul probability math: UNKNOWN / not in corpus.**

---

## 6. DEFENSIVE POSITIONING (marking / closing-down / offside)

- **`FUN_00a76870`** (`match_player/decompiled.c:591`, seed `closingdown`) builds the
  **TeamTactics reflection schema** — field descriptors `{atom, typecode, name, flags}`:
  `"TeamTactics","Passing","Tackling","ClosingDown","Mentality","CornersLeft/Right",
  "FreeKicksLeft/Right","UserTactics","ThrowInLeft/Right","OffsideTrap","OffsideTrapDepth"/
  "MenBehindBall","CounterAttack","IsDeleted"`. Each entry: atom from `FUN_00405530("uint32")`,
  typecode 2, name ptr, flag 7, size 4, default 0xffffffff. **VERIFIED — this enumerates the
  tactic knobs but is pure schema.**
- **`FUN_0083ea10` "MarkingDistanceScale"** (`match_player/decompiled.c:5177`) — a **config
  accessor**: interns keys `"PlayerData"` + `"MarkingDistanceScale"`, looks up via
  `FUN_00662cf0` (cached in `DAT_00c90278`); **if absent returns default `_DAT_00b5e8c4`**
  (a float in `.data`). **VERIFIED.** → *Marking distance is a data-driven coefficient read
  from the PlayerData config DB; its numeric value is NOT in this code (it is `_DAT_00b5e8c4`
  / the config file). Do not invent it.*
- **Offside line (PRESENTATION) — `FUN_009aa250`** (`match_movement_ai.c:16003`, seed
  `offside`): positions the on-screen offside-line UI ("OffsideLineMenu"). **VERIFIED
  pitch→screen scaling:**
  - `16016`: `local_8 = ROUND((float)in_ECX[0x34] * 0.008888889 * 56.25);`
  - `16018`: `local_8 = ROUND((float)in_ECX[0x34] * 0.008888889 * 16.199999);`
  - `0.008888889` ≈ 1/112.5 (= 0.8/90); combined with 56.25 and 16.2 these are
    **pitch-coordinate → screen-pixel scalers**, applied to field `in_ECX[0x34]`. Sets
    sprite X via vtable `+0x5c`. → overlay rendering, **PRESENTATION.** (Same scaler reused
    in `FUN_009a89b0`, `match_movement_ai.c:41707`, `*(in_ECX+0xd0)*0.008888889*param_1`.)
- `ClosingDown`/`TeamClosingDown`/`MarkingDistanceScale` **decision logic (who marks whom,
  closing-down trigger distance): UNKNOWN** — only the knobs (schema) and one scale accessor
  are in-corpus.

---

## 7. PLAYER DECISION-MAKING & MATCH-EVENT GENERATION

The match is modelled as a **rule system over a game-state FSM** — this is the clearest
recovered structure.

### 7a. GameState FSM (match-event vocabulary) — VERIFIED
`match_engine.c` defines an `eGameState*` enum; `FUN_0077ddc0`
(`match_engine.c:6794`) is the **enum→string mapper** (`switch(state)`), giving the ordering:
`case 0 = eGameStateInPlay`, `1 = HandsKickActivated`, `2 = HandsKickSpotted`, … States cover
`InPlay, KickOff(Waiting/Spotted), Goal(Spotted/Activated), Offside(Spotted), Foul(Spotted),
FreeKick/CloseFK(Spotted/Activated), CornerKick(Spotted/Activated), GoalKick, ThrowIn(Spotted/
Activated), Penalty(Spotted/Activated/Shootout), Backpass(Spotted/Activated), HalfOver/Activated,
GameOver, ApresGoal, Substitution`. The recurring **`*Spotted` → `*Activated` two-phase**
pattern = event *detected*, then set-piece *positioning engaged*. **VERIFIED (vocabulary &
two-phase model); transition predicates UNKNOWN.**

### 7b. Predicate/Axiom registry hub — `FUN_008a7770` — VERIFIED
(`match_engine.c:148` / better-typed `match_player/decompiled.c:148` as
`__fastcall FUN_008a7770(int param_1)`). One-time registration: guarded by bitmask
`_DAT_00c92c24` (bits 1,2,4,8,…), each predicate/axiom **name interned via `FUN_00405530`**
and inserted into a registry dictionary at `param_1+0x3c` (lookups via `FUN_004f90f0`
compared against end-iterator `iVar1+4`; insert via `FUN_00892570`; dispatch via vtable
`+0x48`). Registers the full predicate set (`PlayerWithBall, Foul, GoalKick, Corners,
Clearances, FreeKicks, Header, Cross, ThrowIn, MadeTackle, Pass, TeamPossessionPortion`) and
axiom set (`ShotFreekick, ShotHeader, Runs, BackPass, ShotAtTarget, Offside, Goal,
KeeperTouch, YellowCard, RedCard`). **VERIFIED — this is the match-rule vocabulary setup; the
per-tick EVALUATION of these predicates (where stats increment and events fire) is UNKNOWN.**

### 7c. Engine orchestration — `GCMatchModule::ProcessData` `FUN_0077ee80` — VERIFIED
(`match_engine.c:596`, seed `gcmatchmodule`). Increments `_DAT_00c84c94`, reads `rdtsc`,
walks a red-black tree of match items at `in_ECX+0x58..0x68` (calling `FUN_0077d800` per
node), then drains a list of completed fixtures at `+0x70..0x78`, logging
`"End of fixture %d that ran on a thread."` (`FUN_00411c10`, level 0x1a). Wraps work in
vtable transaction calls `(**(in_ECX+0x30))+4 / +0xc`. → **matches are simulated on worker
threads; ProcessData collects & commits their results.** VERIFIED. *This confirms the
physics sim is out-of-corpus.*

**Decision branching thresholds (pass vs shot vs dribble vs tackle): UNKNOWN.** No
predicate-evaluation body with comparative thresholds was located; only registration,
schema, and the FSM vocabulary were recovered.

---

## Field read/write table (VERIFIED offsets)

| Struct / base | Offset | Access | Meaning (confidence) |
|---|---|---|---|
| Player collection `DAT_00c78384` | +0x24 | read | player-array base ptr (VERIFIED) |
| " | +0x1c | read | player count (VERIFIED) |
| " | +0x1e | read | mode/flag gate (INFERRED) |
| Player record | stride 0x3c=60B | iter | per-player record size (VERIFIED) |
| View/camera obj `in_ECX` (FUN_008b56d0) | [0x73],[0x74] | write | pitch/yaw = deg×0.017453292 (VERIFIED) |
| " | [0x75] | write | zoom/roll = x×0.5555556 (VERIFIED) |
| " | +0x66, vtbl+0x158 | read | source angle (INFERRED) |
| View entity `in_ECX` (FUN_009ba5e0) | +0x128,+0x12c | write | resolved player/ball record ptrs (INFERRED) |
| " | +0x130,+0x134,+0x138 | write | derived display state (INFERRED) |
| Entity handle src `DAT_00c94cf8` | +0xac (int) | read | player handle; null=0x80000/0xfffff (VERIFIED) |
| " | +0xc0 (ushort) | read | ball/obj handle; null=0x8000/0xffff (VERIFIED) |
| Record array bases | `DAT_00c784a8+0x2c` (×0x20), `DAT_00c779ac+0x24` (×0x2c) | read | handle→ptr resolve (VERIFIED) |
| Offside UI `in_ECX` (FUN_009aa250) | [0x34] | read | pitch coord → ×0.008888889×{56.25,16.2} px (VERIFIED) |
| " | [0x42],[0x30],[0x58..0x5a],[0x59] | read | UI child objects, vtbl+0x5c setX (INFERRED) |
| Rule-registry obj (FUN_008a7770) | +0x3c | read/write | predicate/axiom dictionary (VERIFIED) |
| Global init bitmask | `_DAT_00c92c24` | read/write | static-init guard bits (VERIFIED) |
| Config cache | `DAT_00c90278` / default `_DAT_00b5e8c4` | read/write | MarkingDistanceScale value+fallback (VERIFIED) |
| GCMatchModule `in_ECX` (FUN_0077ee80) | +0x58..0x68, +0x70..0x78 | read/write | match-item tree + completed-fixture list (VERIFIED) |

---

## CORE / SUPPORTING / PRESENTATION classification (functions actually traced)

| Addr | File:line | Verified role | Class |
|---|---|---|---|
| FUN_0077ee80 | match_engine.c:596 | GCMatchModule::ProcessData — collects threaded match results | **CORE (orchestration)** |
| FUN_008a7770 | match_engine.c:148 | Registers GCMatchPredicate/Axiom atoms into registry | **CORE (rule vocab setup)** |
| FUN_0077ddc0 | match_engine.c:6794 | eGameState enum→string (logging) | SUPPORTING (reflection) |
| FUN_009ba5e0 | match_movement_ai.c:16522 | Match-view entity update (handle resolve + state dispatch) | **CORE (view)** / borderline |
| FUN_0083ea10 | match_player:5177 | MarkingDistanceScale config accessor (+default) | SUPPORTING (config) |
| FUN_008b56d0 | match_movement_ai.c:38057 | Camera deg→rad orientation | PRESENTATION |
| FUN_00699dd0 | match_movement_ai.c:52303 | fpatan/atan2 wrapper | SUPPORTING (math) |
| FUN_009a89b0 | match_movement_ai.c:41699 | angle/pitch scale helper (×0.008888889) | SUPPORTING (math) |
| FUN_009aa250 | match_movement_ai.c:16003 | Offside-line overlay positioning (OffsideLineMenu) | PRESENTATION |
| FUN_00a1fdf0 | match_movement_ai.c:4002 | Per-player attribute serializer (anchor) | SUPPORTING (serialization) |
| FUN_00a20c20 | match_movement_ai.c:17208 | Match/player text export to FILE (223× fprintf) | SUPPORTING (serialization) |
| FUN_00a82320 | match_movement_ai.c:6147 | Player-attribute reflection schema (112× atom, uint32) | SUPPORTING (schema) |
| FUN_00a76870 | match_player:591 | TeamTactics reflection schema builder | SUPPORTING (schema) |
| FUN_0097f380 | match_movement_ai.c:15232 | Tackle-field schema registration (19× atom) | SUPPORTING (schema) |
| FUN_00405530 | (helper, 100s of callers) | string→atom intern (static-init guarded) | SUPPORTING (runtime) |
| FUN_00413550 | (helper) | serialize key/value field | SUPPORTING (serialization) |
| FUN_004f90f0 / FUN_00892570 | (helpers) | std::map find / insert | SUPPORTING (container) |

No traced function implements continuous player-movement physics, pass/shot/tackle
resolution, or decision thresholds.

---

## UNKNOWN list (biggest gaps — require a different code region / the threaded sim / data files)

1. **Player movement physics** — position/velocity fields, speed clamps, acceleration,
   pursuit/target-seek. Not in corpus (runs in threaded sim collected by ProcessData).
2. **Ball possession transfer** — the actual has-ball flag and change-of-possession rules.
3. **Pass selection & trajectory** — receiver choice, pass power/angle, interception.
4. **Shot resolution** — power/placement/keeper-save probability.
5. **Tackle resolution** — success vs foul vs card probability and its attribute weighting.
6. **Marking/closing-down decision** — who marks whom and the trigger distance (only the
   `MarkingDistanceScale` *accessor* + default `_DAT_00b5e8c4` found; numeric value lives in
   the PlayerData config DB, not in code — DO NOT INVENT).
7. **Predicate/axiom EVALUATION** — the per-tick code that tests `GCMatchPredicate*` and
   fires events / increments stats (only registration found).
8. **GameState FSM transitions** — the predicates that drive `*Spotted → *Activated →
   InPlay` (only the enum and its stringifier found).
9. **Numeric tuning constants** — generally data-driven via "PlayerData" config keys +
   `.data` fallbacks (e.g. `_DAT_00b5e8c4`); values not present in decompiled code.
10. **Semantics of view-object offsets** `+0x128..0x138` in `FUN_009ba5e0` and the
    `FUN_009b5cf0` state enum {0,1,2,4} (display states, not confirmed to be movement).

### Recommended next trace targets (outside this corpus)
- The callees of `GCMatchModule::ProcessData` (`FUN_0077d800`, `FUN_0077cf20`,
  `FUN_0056b600`) → the per-fixture result-reading path, which should point at the sim's
  output buffers.
- The dispatch target behind the predicate registry vtable `+0x48` (set in `FUN_008a7770`)
  → the predicate evaluator.
- The `FUN_009b5*` / `FUN_009b8*` / `FUN_009b9*` cluster called from `FUN_009ba5e0` → to
  confirm whether any hold real entity-position updates vs pure display.
