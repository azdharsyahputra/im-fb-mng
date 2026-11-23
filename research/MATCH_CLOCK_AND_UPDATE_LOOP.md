# CM2008 Match Clock & Outer Update Loop

Traces who drives the recursive dispatcher `FUN_008f90f0`, and — because that turns
out to be the game-flow dispatcher, not the match loop — locates the **real match
clock and update model** in the `GCMatch*` engine. Evidence from the live Ghidra
DB (`CM2008.exe`) via `analyzeHeadless` (`ListCallers`, `ListXrefsTo`,
`DumpVtable`, `DumpListingRefs`) and `docs/ghidra_dumps/raw/match_engine.c`.
Classifications: **VERIFIED / INFERRED / HYPOTHESIS / UNKNOWN**.

---

## 1. The outer caller of `FUN_008f90f0` is NOT a match clock (VERIFIED)

`FUN_008f90f0` has **127 distinct callers** (Ghidra `getCallingFunctions`). It is a
**generic program-step / command executor**, invoked once per UI/program-step
event — a screen handler builds a root command node and calls
`FUN_008f90f0(root, outA, outB)`. There is **no single tick loop** here.

Representative callers (VERIFIED, from `scratchpad/agentB2_out/callers.txt`):
`FUN_00930350` (2530 b, 33 callees), `FUN_00943120` (1793 b, 39 callees),
`FUN_0096ac80` (2265 b), `FUN_00875b10` (2488 b), `FUN_009788f0` (842 b). These
are screen/dialog/program-step handlers, not a simulation loop. Internally the
dispatcher recurses with hard-coded roots such as `FUN_008f90f0(&DAT_00c93f10,
&DAT_00c93f4c, 0)` (VERIFIED, `match_movement_ai.c` line 16326).

**Cadence:** per program-step / UI event, i.e. driven by the game's main
message/program-step pump (the registered command `DisplayProgramStep` /
`DumpScreenActivates` confirm a program-step model). Exact pump function =
UNKNOWN; not a fixed time tick. The match-flow arms (`ViewHalfTimeTeamTalks`,
`SwitchMatchScreen`, …) fire only when the match state machine (§3) requests those
screens. (INFERRED.)

---

## 2. The real match update unit: `GCMatchModule` (VERIFIED)

The detailed match is a **polymorphic module object** updated through a vtable, not
a direct-called tick:

- vtable `@ 0x00ba2904` (.rdata). Constructors `FUN_0077d890` (@ `0x0077d890`,
  allocates via `operator_new`) and `FUN_0077d410` (@ `0x0077d410`) store this
  vtable into the object (xrefs to the vtable base from `0x0077d8d8` /
  `0x0077d422`). (VERIFIED, `DumpVtable`.)
- **Update method = vtable slot `+0xa8` = `FUN_0077ee80`** (@ `0x0077ee80`, 1621 b,
  the `GCMatchModule` seed). The *only* reference to `FUN_0077ee80` in the whole
  image is that vtable slot (`ListXrefsTo` → 1 ref, from `0x00ba29ac` DATA). So it
  is invoked **purely virtually** — `module->vtable[0xa8]()` — by the game's module
  manager. (VERIFIED.)
- Other vtable methods include dtor `+0x0 → 0x0077d7e0`, `+0xb8 → FUN_0077ed60`,
  `+0x9c → FUN_0077e520`, etc. (VERIFIED.)

### What Update (`FUN_0077ee80`) does (VERIFIED)
Iterates **two linked lists of running fixtures** held on the module
(`ctx+0x58..0x68` and `ctx+0x70..0x80`), processes each, and logs
`"End of fixture %d that ran on a thread."` — i.e. fixtures can be simulated on
worker threads. Uses 64-bit math (`__allmul`/`__alldiv`). Per fixture it calls
`FUN_0077cf20` (@ `0x0077cf20`, 413 b). (VERIFIED.)

### Per-fixture → rule engine chain (VERIFIED)
```
GCMatchModule::Update  FUN_0077ee80   (vtable +0xa8)
  └─ per-fixture        FUN_0077cf20   (checks selected fixture DAT_00c31b34)
       └─              FUN_0077c540
            └─         FUN_009e9570    (match-analysis predicate RULE driver)
                 └─    FUN_009e8f60 / 009e5ea0 / 009e5160   (predicate DAG eval)
```
`FUN_009e9570` references `"--disable_match_analysis_rule_filtering"`; it is the
rule/predicate engine described in `COMMAND_TREE_EXECUTION.md` §6. (VERIFIED
call edges via `ListCallers`/`ListXrefsTo`.)

---

## 3. The match clock is CONTINUOUS FLOAT TIME, not a fixed tick (VERIFIED)

`FUN_0077c660` (@ `0x0077c660`, 170 b) is the **match-clock formatter**. It reads a
**float elapsed-time accumulator** from the match context and converts to
hours/minutes:

```
fVar1 = *(float*)(*(ctx+0xe4) + 0x48)                    // base/offset seconds
fVar1 = *(float*)(*(ctx+0xe4) + 0x44) + fVar1            // + elapsed seconds
hours   = fVar1 * 0.00027777778   (= 1/3600)             // seconds -> hours
minutes = (fVar1 - hours*3600.0) * 0.016666668 (= 1/60)  // remainder -> minutes
// packed into a display bitfield
```
(VERIFIED, `match_engine.c` lines 2760–2772.) Implications:

- The clock is a **floating-point seconds accumulator** stored at
  `matchContext->[+0xe4]->+0x44` (elapsed) / `+0x48` (offset/base). It is advanced
  continuously by the per-step update, **not** incremented in fixed integer
  minute/second ticks. The task's "do not assume fixed-tick" is confirmed.
- The AI samples it as `"LastPerceivedMatchTime"` (interned stat key, VERIFIED) —
  i.e. AI decisions read a snapshot of continuous match time.
- `"NumMinutesPlayed"` (interned via `FUN_00a6d620`) and `"MinutesPerGoals"` are
  derived minute counters, not the base clock. (VERIFIED strings.)

---

## 4. Match state machine & half / full-time handling (VERIFIED enum, INFERRED driver)

The in-match phases are an `eGameState` enum (VERIFIED — 31 names, mapped to
strings by `FUN_0077ddc0` @ `0x0077ddc0`, the state→name debug mapper; called by
`FUN_0077e850` match-analysis logger):

- Phase/clock states: `eGameStateKickoffWaiting`, `eGameStateKickOffSpotted`,
  `eGameStateInPlay`, `eGameStateHalfActivated`, **`eGameStateHalfOver`**,
  **`eGameStateGameOver`**, `eGameStatePenaltyShootout`, `eGameStateApresGoal`,
  `eGameStateSubstitutionHappening`, `eGameStateSpare`, `eGameStateNums`.
- Event sub-FSM (each a *Spotted → Activated* pair): Goal, Foul, Offside,
  ThrowIn, GoalKick, CornerKick, FreeKick, SetPieceFreeKick, HandsKick,
  Backpass, Penalty.

**Half / full-time model (INFERRED from the above):** the per-step match update
compares the continuous float clock (§3) against the half/match duration and
transitions `eGameState… → eGameStateHalfOver` (end of a half) and ultimately
`eGameStateGameOver`. These transitions are what trigger the game-flow commands
`ViewHalfTimeTeamTalks` / `ViewFullTimeTeamTalks` / `ResultsPending` in
`FUN_008f90f0` (§1). The clock is event/phase-gated: during set-pieces,
goals-apres, and substitutions the `InPlay` clock is suspended by the sub-FSM.
(INFERRED — states and clock verified; the explicit comparison/write not yet
decompiled.)

---

## 5. Update frequency / model summary

| question | answer | conf |
|----------|--------|------|
| Is `FUN_008f90f0` the match loop? | **No** — it is the program-step/screen command dispatcher (127 callers) | VERIFIED |
| What is the match update unit? | `GCMatchModule::Update` = `FUN_0077ee80` (vtable `0x00ba2904`+0xa8) | VERIFIED |
| How is it invoked? | **Virtually** by the game module manager (only xref is the vtable slot) | VERIFIED |
| Fixed tick? | **No** — continuous float seconds accumulator (`ctx+0xe4→+0x44/+0x48`), formatted ÷60/÷3600 by `FUN_0077c660` | VERIFIED |
| Half/full time | `eGameStateHalfOver` / `eGameStateGameOver` states gate team-talk/result screens | enum VERIFIED, trigger INFERRED |
| Rules per step | Update → `FUN_0077cf20` → `FUN_0077c540` → `FUN_009e9570` predicate rule engine per fixture | VERIFIED |

---

## 6. Exact missing functions (named dependencies, UNKNOWN)

1. **Module-manager update loop** — the function that issues the virtual call
   `matchModule->vtable[0x00ba2904 + 0xa8]()` each program-step. It is reached by
   an indirect `call [reg+0xa8]`, so not statically resolvable to one caller;
   the module object is created by **`FUN_0077d890`** (`operator_new`, 0 direct
   callers ⇒ instantiated via a factory/registry). *To resolve:* runtime trace, or
   find who stores a `0x00ba2904`-vtable object into the module manager's list and
   iterates it calling `+0xa8`.
2. **`eGameState` transition writer** — the function that writes the match
   state field and decides `HalfOver`/`GameOver` from the float clock (§3/§4). The
   states and clock are located; the single writer is not yet decompiled.
   *Candidate region:* callees of `FUN_0077ee80` → `FUN_0077cf20` →
   `FUN_0077cd40` (@ `0x0077cd40`, 472 b) / `FUN_0077c710` (@ `0x0077c710`, 890 b).
3. **Real-time cadence** — whether Update runs per render frame or per fixed
   sim-substep, and the dt fed into the float clock. UNKNOWN (needs the module
   manager / dt source).

---

## 7. One-line reconciliation

The caller's premise ("`FUN_008f90f0` = match engine recursive command-tree
interpreter, outer loop = match clock") does not hold: `FUN_008f90f0` is the
**game program-step command tree**; the **detailed match engine** is the
`GCMatchModule` (`FUN_0077ee80`, vtable `0x00ba2904`) driving a float-time,
event-gated `eGameState` FSM and the `GCMatchPredicate`/`GCMatchAxiom` rule DAG.
