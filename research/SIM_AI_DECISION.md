# CM2008 — Player Movement / AI Decision Pipeline (clean-room, static)

Clean-room, **static only**. No game executed, no Ghidra. `file_offset = VA − 0x400000`
(verified identity map). All byte reads from `/Volumes/CM2008/Crack JPN/cm2008.exe`,
disassembled with capstone. Decompiled text and bytes are treated as DATA.

This document unifies the movement/action-decision findings and re-verifies the load-bearing
facts directly from the instruction stream. Confidence tags: **VERIFIED** (read in bytes),
**INFERRED** (deduced from pattern/xref), **UNKNOWN** (bounded, not recovered).

Builds on: `MOVEMENT_ACTION_MAP.md`, `MOVEMENT_PHYSICS_RECONSTRUCTION.md`,
`MOVEMENT_GEOMETRY_BUFFER.md`, `MOVEMENT_OFFSETS_EMULATED.md`, `FORMULA_BODY_RECOVERY.md`,
`FORMULA_CONSUMER_MAP.md`, `MATCH_PREDICATE_WEIGHTS.md`, `STATE_WRITER_FOUND.md`,
`EGAMESTATE_COMPLETE.md`.

---

## 0. HEADLINE — the decision core is SPLIT across two layers

The AI "decision" is two distinct machines, and they live in different places:

1. **DISCRETE action selection + event resolution — IN CORPUS, RECOVERED.** The foreground
   viewed match resolves discrete actions synchronously in the `0x77xxxx` / `0x9exxxx` module:
   a per-tick event driver (`FUN_0077cf20`) runs an **attribute-weighted stochastic contest**
   (`FUN_0077c710`) to decide outcomes, tags each actor's **action-state word** (record `+4`,
   `| 0x3000`) via `FUN_0077cd40`, and ranks candidate match events with a **weighted predicate
   engine** (`FUN_0077c540 → FUN_009e9570 → FUN_009e5160`, threshold 80.0). All of this is
   byte-verified below.

2. **CONTINUOUS movement / positioning geometry — THREADED SIM, BOUNDED (not recovered).**
   The per-tick `pos += vel·dt`, gait selection, pursuit/target-seek, and who-marks-whom
   geometry are **not** in any module-seeded dump. They run in the worker-thread sim that
   populates the per-entity 1012-byte geometry block (`MOVEMENT_GEOMETRY_BUFFER.md`). What IS
   recovered for this layer is its **tuning data**: the full `ResolverSettings` struct layout,
   the gait speed/accel constants, `MarkingDistanceScale`, etc. — harvested at match-init by
   `FUN_009f4bd0` / `FUN_005d6dd0`. The **formula trees** that combine tuning × attribute are
   in the encrypted `formula.xml.blowfish` payload (key not recovered) = UNKNOWN.

So: **decision core = in-corpus for action/event selection; bounded-to-threaded for continuous
motion; data-driven for all numeric tuning.** The engine is data-driven exactly as the anomaly
predicted — the consumers of the movement-formula struct are the mover/resolver, but the mover
itself is thread-private while its *inputs* (the struct) are fully recovered.

---

## 1. THE PER-TICK DECISION PIPELINE (call graph, VERIFIED by xref)

Byte-level caller scan (E8 rel32 resolution over the whole image):

```
FUN_0077cf20  (per-tick event/decision driver; 1 caller region in match module)
  ├─ FUN_0077cd40   [called @0x77f22e → driver]   ACTION-RESOLUTION DISPATCHER
  │     ├─ branch on event-record flag bytes  [ebx+4],[ebx+5],[ebx+6]
  │     ├─ FUN_0077c710  [@0x77cdf8]              ATTRIBUTE-WEIGHTED CONTEST   ← decision math
  │     └─ writes ACTION STATE word: record+4 = (old & 0xffff3fff) | 0x3000
  └─ FUN_0077c540   [called @0x77cfed → inside FUN_0077cf20]   EVENT-RANK CONSUMER
        └─ FUN_009e9570  [@0x77c585, threshold fld[0xb5b10c]=80.0]  score→filter→rank
              └─ FUN_009e5160  [@0x9e99ba]        PER-PREDICATE WEIGHT (returns .data float)
```

Verified callers (byte scan):
- `FUN_0077cd40`: 1 caller `0x77f22e` (match-sim driver band). **VERIFIED.**
- `FUN_0077c710` (contest): 2 callers `0x77cce8` (sibling dispatcher `FUN_0077ccb0`) and
  `0x77cdf8` (inside `FUN_0077cd40`). **VERIFIED.**
- `FUN_0077c540` (predicate-rank consumer): 1 caller `0x77cfed`, i.e. **inside the per-tick
  driver `FUN_0077cf20`**. **VERIFIED** — this wires event ranking into the tick loop.
- `FUN_009e5160`: 1 caller `0x9e99ba` (inside the rank engine `FUN_009e9570`). **VERIFIED.**
- `FUN_009f4bd0` (settings harvest): 2 callers `0x9f7e56/0x9f7e61` (match setup). **VERIFIED.**

---

## 2. THE DECISION PRIMITIVE — attribute-weighted contest `FUN_0077c710` (VERIFIED by bytes)

This is the core "who wins / does this succeed" math. Disassembly `@0x77c887`:

```
mov  eax,[ebx+0x3c]         ; ratingWord of side A
fld  dword [0xbbb51c]       ; tuning scalar  _DAT_00bbb51c = 0.5   (VERIFIED value)
mov  ecx,[ebp+0x3c]         ; ratingWord of side B
shr  eax,0xc  ; and al,0x7f ; rA = (A>>12)&0x7f   7-bit rating (0..127)
shr  ecx,0xc  ; and cl,0x7f ; rB = (B>>12)&0x7f
add  eax,ecx               ; sum = rA+rB     ; je → avoid /0
fild [esp+0x20] (sum) ; fild [esp+0x14] (rA) ; fdiv st(1)   ; shareA = rA/sum
fild [esp+0x10] (rB) ; fdivrp                               ; shareB = rB/sum
imul eax,eax,0x19660d ; add eax,0x3C6EF35F                  ; LCG step (shared match RNG)
... or eax,0x3f800000 idiom → uniform u in [1,2)/[0,1)
fld1 ; fcomp st(2)/[esp+..]                                 ; compare u vs scaled share → winner
```

- Inputs: two **7-bit player ratings** at record `+0x3c` (`(word>>0xc)&0x7f`). Record `+0x3c`
  lives on the **60-byte player record** (stride `0x3c`, pool `DAT_00c78384`), i.e. the
  per-player attribute record — **VERIFIED** (offset > pool-A 32-byte stride).
- Normalisation: `shareA = rA/(rA+rB)` — a Bradley-Terry / proportional-odds win share.
- Tuning: a single `.data` scalar `_DAT_00bbb51c = 0.5` scales the share before the random
  compare. **VERIFIED value = 0.5.**
- Randomness: the engine-wide LCG `x = x*0x19660D + 0x3C6EF35F` (same constants used in the
  penalty shootout `FUN_00a023f0` and the action dispatcher's RNG advance). **VERIFIED.**

**This is the recovered decision kernel**: contested events (tackles, headers, 50/50s,
pass-interception, control) resolve by comparing a uniform random draw against the
attribute-derived win-share scaled by a data-driven constant. Which *specific* attribute word
`+0x3c` encodes per call-site, and which scalar beyond `0.5` applies per event type, are
selected by the caller/formula system (INFERRED) — the math shape is VERIFIED.

---

## 3. ACTION-STATE TAGGING — `FUN_0077cd40` (VERIFIED by bytes)

The dispatcher that turns a queued event record into an applied action on an entity. Disasm:

```
ebp = DAT_00c784a8 (player pool mgr) ; esi = [ebx] (event's entity handle)
reverse-resolve handle (FUN_005a6320) → map-find (FUN_006a00f0) → node @ edi+0x98
advance shared LCG: [0xc84958] = (x*0x19660D + 0x3C6EF35F) >> 8     ; RNG reseed
cmp [ebx+6],0  → if set: FUN_009efa10(3, ...)        ; event class 6 (type code 3)
elif [ebx+4..5]/globals → FUN_009efa20(1|0, ...)     ; event classes 4/5
if [ebx+4] != 0:                                     ; CONTESTED action
    push esi ; call FUN_0077c710                     ; ← RUN THE CONTEST
    resolve pool-D record (FUN_005a6330) → eax
    ecx = [eax+4]
    ecx &= 0xffff3fff        ; clear action-state bits 14-15
    ecx |= 0x3000            ; set action-state code (bits 12-13)
    [eax+4] = ecx            ; ← WRITE ACTION/MOVEMENT STATE WORD
```

- **Entity record `+4` is the action/movement state word.** The write `(& 0xffff3fff) | 0x3000`
  clears a 2-bit field (bits 14-15) and stamps code `0x3000` into bits 12-13. **VERIFIED.**
- Confirms the task's known fact: `FUN_0077cd40` is the `+4 | 0x3000` writer, and it does so
  **after** running the contest `FUN_0077c710`. The contest outcome drives which state is
  stamped (INFERRED from control flow; the `0x3000` path is the contested branch).
- The event record (`ebx`) carries flag bytes at `+4/+5/+6` that pre-classify the pending
  action; the dispatcher routes each class to a different resolver (`FUN_009efa10/20`) or to the
  contest. **VERIFIED (control flow).** Flag semantics (which class = pass/shot/tackle) = INFERRED.

---

## 4. EVENT RANKING — predicate engine `FUN_009e9570` / `FUN_009e5160` (VERIFIED)

Called every tick from the driver via `FUN_0077c540`. Mechanism (disasm `@0x9e5160`):

```
name = p->vtbl[1]()                 ; GetTypeName (vtable+4)
cmp eax,[0xc5f388] ; je → fld [0xba1714]  ret   ; TeamGoals → 100.0
cmp eax,[0xc5f3e4] ; je → fld [0xbd5c48]  ret   ; StrongDefence group → 92.0
... ~84 interned type-name compares, each → a .data float weight
```

- Each predicate's weight is a **`.data` float constant** returned by a flat type-name
  dispatch. **VERIFIED**: `0xba1714 = 100.0`, `0xbd5c48 = 92.0` (read from bytes).
- `FUN_0077c540` passes threshold **`fld [0xb5b10c]` = 80.0** (VERIFIED value) into
  `FUN_009e9570`, which keeps candidates with score > 0 and ranks by `-score`. Only predicates
  with weight ≥ 80 survive: `TeamGoals(100)`, the `Strong*/VeryBad*/Weak*` group (92), the
  `PlayerHad*Match*` group (90), `TeamDoesManyQuickAttacks(85)`. Full 84-row table in
  `MATCH_PREDICATE_WEIGHTS.md` (all weights VERIFIED).

**Scope caveat (clean-room honesty):** the 84 predicate names are match **narrative /
assessment** predicates (`PlayerHadAGoodMatchAsStriker`, `WeakDefence`, `TeamGoals`,
`DefensiveErrorsLeadsToShots`, …). This engine selects/ranks **which notable match event or
assessment fires** (commentary, chance generation, performance tagging) — it is **VERIFIED** as
a weighted ranking machine wired into the tick loop. Equating it to a per-player
"pass-vs-shoot-vs-dribble" chooser is **INFERRED and partly overstated**: the per-actor physical
action choice is the contest dispatcher (§2–3). The two cooperate — predicate ranking biases
*what kind of chance/event* the sim pursues; the contest resolves *whether the actor succeeds*.

---

## 5. INPUTS — attributes + ResolverSettings + ball/opponent position

### 5a. Per-player attributes (VERIFIED harvest, VERIFIED read at +0x3c)
- **`FUN_005d6dd0`** harvests all **101 `PlayerConstant`** ratings per player into a scaled
  0–100 attribute array at match-init (input = a Player; default 100). Covers movement
  (`Mobility, Acceleration, Deceleration, Spring, BodyBalance`), passing, shooting, tackling
  (`Marking, Tackling, HardTackleFrequency, Aggression`), awareness (`GameAwareness, Alertness,
  Roaming, RunFrequency`) and the positional-role **bias** family. **VERIFIED** (consumer map).
- Consumed by the contest as the 7-bit word at record `+0x3c` (§2). **VERIFIED.**

### 5b. ResolverSettings — the movement tuning struct (VERIFIED layout; reads bounded)
Harvested globally at match-init by **`FUN_009f4bd0`** (`esi` = settings struct). Byte-exact
offsets (from `fstp [esi+disp]` stores; see `FORMULA_BODY_RECOVERY.md` Appendix A):

| Field | default | slot | Field | default | slot |
|---|---|---|---|---|---|
| MinSprintingSpeed | 8 | esi+0x124 | **MaxSprintingSpeed** | **9** | esi+0x128 |
| MinRunningSpeed | 6 | esi+0x11c | MaxRunningSpeed | 7 | esi+0x120 |
| Min/Max BaseAcceleration | 2 / 3 | esi+0xfc/0x100 | Min/Max BaseDeceleration | −4 / −5 | esi+0x104/0x108 |
| Running/SprintingFatigueModifier | 0.002 / 0.02 | esi+0x1b4/0x1b8 | UpdateDegradableStatsPeriod | 60 | esi+0x9c |
| Min/MaxSpeedToTurningCircleRadius | 0.15 / 0.3 | esi+0xf4/0xf8 | (8-gait accel/decel block) | — | esi+0x12c..0x1b0 |

Full gait model (Standing/Walking/Jogging/Running/Sprinting/Backing/SidlingL+R/DivingL+R/
Sliding; Min/Max speed + accel/decel per gait) at `esi+0x9c..0x1b8`. **VERIFIED** struct +
defaults. The accel/decel **multipliers** for the base gaits are `(compute)` — no fallback, so
they are meaningful only when the formula payload is present = UNKNOWN numeric.

**Consumer reads:** the per-frame code that reads `[rs+0x124]/[rs+0x128]` etc. to clamp speed
and pick a gait is **NOT cleanly in-corpus**. A byte scan for `fild/fld [reg+0x128]` finds no
integer-load (fild) consumer and only generic float reads outside the match-sim band. This is
consistent with the continuous integrator being **thread-private** (§6). **Reads = UNKNOWN /
bounded to threaded sim; struct + defaults = VERIFIED.**

### 5c. Ball / opponent position (VERIFIED coordinate layout; decision use INFERRED)
- World coordinates are **integer fixed-point** at coordinate-object `+0xc0` (X) and `+0xd0`
  (Y), each projected to screen by `× (1/112.5)` (`_DAT_00b5e8c8 = 0.008888889`, VERIFIED
  value). `+0xd0` holds a sub-object that is a **4-point (u16,u16) waypoint polyline** (stride 8
  from `+0x12`) — motion is **integer waypoints, not a float velocity vector**. **VERIFIED**
  (`MOVEMENT_GEOMETRY_BUFFER.md`).
- Each pool-A match-entity record references its ball at record `+0x02` (pool-B handle).
  **VERIFIED.** No explicit per-player velocity field is exposed = **VERIFIED(absence)**.

---

## 6. OUTPUTS & the threaded boundary

- **Discrete output:** action-state word at entity record `+4` (`| 0x3000` etc., §3) and the
  `eGameState` FSM at clock sub-object `+0x30` (31 states; per-state enter-handlers in
  `0x6dd000–0x6ed000`, each `mov [this+0x30], <state>`; `STATE_WRITER_FOUND.md`). The FSM gates
  set-pieces; the committer is `FUN_009f2220` (one call deeper than any dump) = bounded UNKNOWN.
- **Continuous output:** the per-entity **1012-byte geometry block** (253 dwords), keyed by
  `entity+0x134` under `[pool+0x3004c]`, copied by `rep movsd` in `FUN_009b54b0`. It is
  **populated by the worker-thread sim** (the integrator), and the view side only forwards/reads
  it. The exact dword indices for x/y/vel/accel inside the block = **UNKNOWN** (threaded writer
  never located; not protected — just never dumped). **VERIFIED** block size/key/path.
- `GCMatchModule::ProcessData` (`FUN_0077ee80`) collects **background-league** fixtures on
  threads; the **foreground** viewed match resolves discretely in-corpus (§1–4). This reconciles
  the two: discrete decisions here, continuous geometry threaded. **VERIFIED.**

---

## 7. MARKING / CLOSING-DOWN / POSITIONING (tuning VERIFIED; geometry UNKNOWN)

- **`MarkingDistanceScale`** = `.data` default `_DAT_00b5e8c4 = 0.02` (**VERIFIED value**),
  stored at `rs+0x3b8`; accessor `FUN_0083ea10` under key `"PlayerData"/"MarkingDistanceScale"`.
  Siblings at `rs+0x3b0/0x3b4/0x3bc`: `TightMarking=3`, `LooseMarking=8`, `CloseInMarking=30`.
- Positioning tuning (VERIFIED defaults): `OffsideLineScale 0.6` (`rs+0x1c8`), `CommitToRun 0.1`
  (`rs+0x1d8`), `BehindBallScale 0.5` (`rs+0x1dc`), `MaxRoamDistance 80` (`rs+0x3cc`),
  `OwnHalfRun −30` (`rs+0x264`), `CrossAreaWidth 15`, `DeepAreaLength 40`, `AntiWidthBias 4`,
  plus per-player `*Bias` role-drift attributes (via `FUN_005d6dd0`).
- The **geometry** that turns these into a defender's target position (distance from marked
  opponent = `MarkingDistanceScale × f(opponent pos +0xc0/+0xd0, ball pos)`) is the formula
  tree in the encrypted payload + the threaded mover = **UNKNOWN**. The knobs and their
  defaults are VERIFIED; **do not invent the geometry** — it is data-driven.

---

## 8. CLASSIFICATION SUMMARY

| Element | Function(s) | Status |
|---|---|---|
| Per-tick decision driver wiring | FUN_0077cf20 → cd40 / c540 (xref) | **VERIFIED** |
| Attribute-weighted contest kernel (rA/(rA+rB), ×0.5, LCG, fcomp) | FUN_0077c710 | **VERIFIED (bytes)** |
| 7-bit rating read `(word>>0xc)&0x7f` at player record +0x3c | FUN_0077c710 | **VERIFIED (bytes)** |
| Action-state word = record+4, `(&0xffff3fff)\|0x3000` after contest | FUN_0077cd40 | **VERIFIED (bytes)** |
| Event-rank engine: 84 predicates → .data weights, threshold 80.0 | FUN_009e5160/9570/c540 | **VERIFIED (bytes+values)** |
| Predicate engine = per-actor pass/shoot/dribble chooser | — | **INFERRED (overstated); it ranks match events/assessments** |
| ResolverSettings movement struct layout + defaults | FUN_009f4bd0 | **VERIFIED** |
| Per-player attribute harvest (101 PlayerConstant) | FUN_005d6dd0 | **VERIFIED** |
| World coord = int fixed-point +0xc0/+0xd0, ×(1/112.5); waypoint polyline | geometry cluster | **VERIFIED** |
| MarkingDistanceScale=0.02 + positioning knobs | FUN_0083ea10 / FUN_009f4bd0 | **VERIFIED (defaults)** |
| eGameState FSM + set-piece gating | 0x6dd000–0x6ed000 enter-handlers | **VERIFIED** |
| Per-frame READS of ResolverSettings movement fields (speed clamp, gait pick) | threaded mover | **UNKNOWN (bounded)** |
| Continuous integrator (pos+=vel·dt), target-seek, marking geometry | threaded sim + payload | **UNKNOWN (bounded)** |
| Formula TREES combining tuning × attribute (gait/pass/shot/mark) | formula.xml.blowfish | **UNKNOWN (encrypted, key not recovered)** |
| eGameState committer FUN_009f2220 body | one call deeper than dumps | **UNKNOWN (bounded)** |

**What is data-driven vs hardcoded:**
- *Data-driven (formula.xml / PlayerData config):* every numeric tuning — gait speeds/accels,
  pass/shot physics, `MarkingDistanceScale` and positioning scales, performance weights, and
  the formula trees. Values live in `.data` defaults (recovered) with the real bodies in the
  encrypted payload (not recovered). **Do not invent thresholds — cite the formula source.**
- *Hardcoded (in code, recovered):* the contest **math shape** (proportional win-share × scalar
  × uniform-random compare), the LCG constants, the 7-bit rating decode, the action-state
  bit-encoding (`+4 | 0x3000`), the predicate-rank mechanism + 80.0 cut, and the FSM structure.
