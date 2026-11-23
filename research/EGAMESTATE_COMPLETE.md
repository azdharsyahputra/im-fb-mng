# eGameState — Complete Recovery (Match Lifecycle, Priority 4)

Clean-room behavioral recovery from Ghidra decompilation dumps. Decompiled text treated as DATA.
Primary source: `docs/ghidra_dumps/raw/match_engine.c` (+ `big/CM2008.exe_big.c`).
Supersedes the partial `research/EGAMESTATE_MAPPING.md`.

Classification legend: **VERIFIED** (byte/line-confirmed), **INFERRED** (strong structural deduction),
**APPROXIMATE** (value confirmed, meaning/units not), **UNKNOWN** (not in provided dumps).

---

## 1. Complete 32-entry value → name table  (VERIFIED)

Source: `FUN_0077ddc0` @ `0077ddc0` (eGameState::ToString). A 32-case `switch` (cases `0`..`0x1f`),
each loading a literal name string. Default (out of range) = `"Unknown MatchState"` (line 6808).
The switch selector is passed in a register (decompiler rendered it `unaff_retaddr`); the value is the
raw eGameState int (see §4 for the call site that feeds it the state field).

| Value | Name | Evidence (line) |
|------:|------|-----------------|
| 0x00 | eGameStateInPlay | 6814 |
| 0x01 | eGameStateHandsKickActivated | 6821 |
| 0x02 | eGameStateHandsKickSpotted | 6828 |
| 0x03 | eGameStateThrowInSpotted | 6835 |
| 0x04 | eGameStateGoalKickSpotted | 6842 |
| 0x05 | eGameStateCornerKickSpotted | 6849 |
| 0x06 | eGameStateGoalSpotted | 6856 |
| 0x07 | eGameStateSpare | 6863 |
| 0x08 | eGameStateBackpassSpotted | 6870 |
| 0x09 | eGameStateKickOffSpotted | 6877 |
| 0x0a | eGameStateSubstitutionHappening | 6884 |
| 0x0b | eGameStateHalfOver | 6891 |
| 0x0c | eGameStateGameOver | 6898 |
| 0x0d | eGameStateApresGoal | 6905 |
| 0x0e | eGameStateKickoffWaiting | 6912 |
| 0x0f | eGameStatePenaltySpotted | 6919 |
| 0x10 | eGameStateThrowInActivated | 6926 |
| 0x11 | eGameStateGoalKickActivated | 6933 |
| 0x12 | eGameStateCornerKickActivated | 6940 |
| 0x13 | eGameStateGoalActivated | 6947 |
| 0x14 | eGameStatePenaltyActivated | 6954 |
| 0x15 | eGameStatePenaltyShootout | 6961 |
| 0x16 | eGameStateCloseFKActivated | 6968 |
| 0x17 | eGameStateBackpassActivated | 6975 |
| 0x18 | eGameStateHalfActivated | 6982 |
| 0x19 | eGameStateFreeKickActivated | 6989 |
| 0x1a | eGameStateFreeKickSpotted | 6996 |
| 0x1b | eGameStateFoulSpotted | 7003 |
| 0x1c | eGameStateOffsideSpotted | 7010 |
| 0x1d | eGameStateSetPieceFreeKickActivated | 7017 |
| 0x1e | eGameStateSetPieceFreeKickSpotted | 7024 |
| 0x1f | eGameStateNums *(sentinel / count)* | 7031 |

**Count: 31 real states (0x00–0x1e) + `eGameStateNums` sentinel (0x1f) = 32 cases.** VERIFIED.
This matches the known "31 states" count exactly.

### Spotted/Activated pairing (INFERRED from the name table)
The engine uses a two-phase pattern per event: a **Spotted** state (event detected, queued) and an
**Activated** state (event being executed/restart taken). Pairs read directly from the names:

| Event | Spotted | Activated |
|-------|---------|-----------|
| HandsKick | 0x02 | 0x01 |
| ThrowIn | 0x03 | 0x10 |
| GoalKick | 0x04 | 0x11 |
| CornerKick | 0x05 | 0x12 |
| Goal | 0x06 | 0x13 |
| Backpass | 0x08 | 0x17 |
| KickOff | 0x09 | (0x18 HalfActivated / 0x0e KickoffWaiting) |
| Penalty | 0x0f | 0x14 |
| FreeKick | 0x1a | 0x19 |
| SetPieceFreeKick | 0x1e | 0x1d |
| Foul | 0x1b | (→ FreeKick/CloseFK/Penalty Activated) |
| Offside | 0x1c | (→ FreeKick Activated) |
| Half | 0x0b HalfOver | 0x18 HalfActivated |
| CloseFK | — | 0x16 |

Unpaired singletons: `InPlay` (0x00, base), `Spare` (0x07, unused slot), `SubstitutionHappening`
(0x0a), `GameOver` (0x0c, terminal), `ApresGoal` (0x0d, post-goal reset), `KickoffWaiting` (0x0e),
`PenaltyShootout` (0x15). The exact Foul/Offside → restart mapping is INFERRED, not byte-confirmed.

---

## 2. The state field location  (VERIFIED)

The live eGameState is a raw `int` at offset **`+0x30` inside the match clock/state sub-object**,
which itself hangs off the match context at **`matchCtx + 0xe4`**:

```
state = *(int *)( *(int *)(matchCtx + 0xe4) + 0x30 )
```

Evidence — `FUN_0077e850` line 3262:
```c
uVar2 = *(undefined4 *)(*(int *)(puVar10 + 0xe4) + 0x30);     // read eGameState
puVar8 = (undefined4 *)FUN_0077ddc0(&puStack_210,uVar2);      // ToString(state)
...
FUN_00411c10(0x16,4,">>>SOAKDATA>>> MatchFreeze(%d) MatchFreezeText(%s) <<<SOAKDATA<<<",uVar2,puVar8);
```
The value is passed straight to `FUN_0077ddc0` (ToString, §1) and printed as both the numeric id and
its name — this simultaneously **confirms the +0x30 field is the eGameState** and that `FUN_0077ddc0`
is its ToString.

The same `matchCtx+0xe4` sub-object holds the clock fields (§5): `+0x44`, `+0x48`, `+0x4c`.

> **Not the state field:** `DAT_00c809d4 + 0x30` (seen in `FUN_0077c660` line 2771 and ~10 big-dump
> sites) is a **different object** — a packed time-of-day DISPLAY register (hour/minute bitfields via
> `>>9 & 0x1f`, `>>0x17`), masked `& 0xffffc007`. Do not confuse it with the eGameState.

---

## 3. SetGameState WRITER  — verdict: **UNKNOWN (not present in provided dumps)**

I searched both dumps exhaustively for any store to the state field:

- `grep -E "0xe4\) \+ 0x30"` → in `match_engine.c` the **only** hit is the READ at line 3262. In
  `big/CM2008.exe_big.c`: **no hits at all**.
- `grep -E "\+ 0x30\) *= "` (any store to offset 0x30) in `match_engine.c` → line 8335
  (`**(undefined1 **)(param_1 + 0x30) = 0`, a list/tree node reset, unrelated) only. In big → two hits
  (63828/63830) which are **player attribute bit-packing** (`& 0x7c00000`), unrelated.

**The two suspected candidates do NOT write +0x30:**
- `FUN_0077cd40` (lines 2611–2700) — the per-tick resolve step ("ProcessData" inner). Writes
  `in_ECX+0x98`, `+0x8c`, `+0x18` (flags on a different sub-object) and calls `FUN_0077c710`. No +0x30
  store. VERIFIED.
- `FUN_0077c710` (lines 5915–6073) — event-resolution routine (LCG RNG `*0x19660d + 0x3c6ef35f`,
  player/team selection, probability compares). Writes player/RNG state only. No +0x30 store. VERIFIED.

**Conclusion.** Within the extracted match-engine dumps the eGameState field is only ever **read**
(for the MatchFreeze log). No writer exists in scope. The writer is therefore **UNKNOWN**, and the
evidence points to it living in the **match-context accessor family `FUN_009ef*`**, whose bodies are
NOT in the dumps (only the 14-byte `FUN_009efab0` thunk is present; it just forwards to `FUN_0048c4a0`).

Why this family is the prime suspect (INFERRED):
- `FUN_009efa90()` returns the match context (its result is used as `ctx`, then `ctx+0xe4` dereferenced
  for clock/state — e.g. lines 2764/2766, 3124, 6088).
- Small-integer-first-arg siblings look like state/command setters:
  `FUN_009efa20(1, DAT_00c84958, state, 0)` and `FUN_009efa20(0, ...)` (lines 2639/2643),
  `FUN_009efa10(3, state)` (line 2653) — called from `FUN_0077cd40` exactly where a state change
  would be committed. These are the best leads but their definitions are absent, so the actual +0x30
  store cannot be confirmed.

**What is needed to resolve:** dump the bodies of `FUN_009efa10`, `FUN_009efa20`, `FUN_009efa30`,
`FUN_009efa40`, `FUN_009efa60`, `FUN_009efa70`, `FUN_009efb10`, `FUN_009efb40` (the `0x009efxxx`
match-context family) from the full binary, and search there for `mov [reg+0x30], <state>` where
`reg == *(matchCtx+0xe4)`. Alternatively a runtime write-watchpoint on `*(*(matchCtx+0xe4)+0x30)`.

---

## 4. Transition graph  (relationship-level; exact edges INFERRED)

Because the writer is out of scope, edges are reconstructed from (a) the name-table Spotted/Activated
pairing and (b) the tick ordering in `FUN_0077ee80`. No single edge is byte-confirmed as a `+0x30`
store; the **triggers** marked VERIFIED are the conditions evaluated in-scope that *gate* a transition.

| From | To | Trigger | Driver | Class |
|------|-----|---------|--------|-------|
| (match start) | KickoffWaiting 0x0e | match init | setup | INFERRED |
| KickoffWaiting 0x0e | KickOffSpotted 0x09 | kickoff taken | event | INFERRED |
| KickOffSpotted 0x09 | InPlay 0x00 | ball in play | event | INFERRED |
| InPlay 0x00 | *EventSpotted* (0x02–0x06,0x08,0x0f,0x1a–0x1e) | event detected during resolve (`FUN_0077cd40`→`FUN_0077c710`) | event | INFERRED |
| *Spotted* | *Activated* (paired, §1) | event begins execution | event | INFERRED |
| *Activated* | InPlay 0x00 | restart completed (`FUN_0077cf20` finalize) | event | INFERRED |
| GoalSpotted 0x06 | GoalActivated 0x13 | goal confirmed | event | INFERRED |
| GoalActivated 0x13 | ApresGoal 0x0d | goal celebration/reset | event | INFERRED |
| ApresGoal 0x0d | KickoffWaiting 0x0e | restart after goal | event | INFERRED |
| InPlay 0x00 | HalfOver 0x0b | **clock freeze cond true** (§5) at 1st-half end | **clock** | trigger VERIFIED / edge INFERRED |
| HalfOver 0x0b | HalfActivated 0x18 | 2nd-half kickoff | event | INFERRED |
| HalfActivated 0x18 | InPlay 0x00 | 2nd half in play | event | INFERRED |
| InPlay 0x00 | GameOver 0x0c | **clock freeze cond true** (§5) at full time | **clock** | trigger VERIFIED / edge INFERRED |
| (draw, cup rules) | PenaltyShootout 0x15 | after GameOver, tie | rules | INFERRED |
| any | SubstitutionHappening 0x0a | sub requested | event | INFERRED |

**Clock-driven vs event-driven:**
- **Clock-driven:** only the two half/full boundaries (→ HalfOver / GameOver), gated by the
  `FUN_0077e850` freeze condition (§5). Everything else is event-driven.
- **Event-driven:** every Spotted/Activated transition, decided inside the per-tick resolve
  (`FUN_0077cd40` → `FUN_0077c710`) and finalized by `FUN_0077cf20`.
- HalfOver vs GameOver is selected by the **half counter** (which half just ended) — that counter is
  NOT in these functions → UNKNOWN which field distinguishes them here.

---

## 5. Clock boundaries, half-time & full-time conditions

### 5a. Continuous clock  (VERIFIED)
`FUN_0077c660` @ `0077c660` composes the displayed match time from the `matchCtx+0xe4` sub-object:
```c
fVar1 = *(float *)(*(matchCtx+0xe4) + 0x48);          // base/offset component
fVar1 = *(float *)(*(matchCtx+0xe4) + 0x44) + fVar1;  // + running elapsed  (line 2767)
fVar2 = fVar1 * 0.00027777778 + 8388864.0;            // *(1/3600); 8388864≈2^23 float→int round bias
// packs hours = fVar1/3600, minutes = (fVar1 - hours*3600)/60 into DAT_00c809d4+0x30 display reg
```
- `+0x44` = **running elapsed clock (float)**. `+0x48` = additive base/offset (e.g. half start / added
  time). Units are **seconds** (INFERRED from the 1/3600 and 1/60 = `0.016666668` conversions, i.e.
  HH:MM). This is a time-of-day display; it does not itself gate transitions.
- `FUN_009d0060` line 6090 also reads `*(matchCtx+0xe4)+0x48` (feeds `FUN_00493b90`), corroborating
  +0x48 as a clock component.

### 5b. Half / Full freeze condition  (trigger VERIFIED; refined vs the task hypothesis)
`FUN_0077e850` @ `0077e850` is the per-tick boundary evaluator (the GameOver/MatchFreeze trigger). The
decisive block (lines 3134–3143):
```c
if ((_DAT_00c84c80 & 1) == 0) {         // one-time init of the threshold constant
    _DAT_00c84c80 |= 1;
    DAT_00c84c7c = 0x45610000;           // = 3600.0f   (VERIFIED: IEEE-754 0x45610000 == 3600.0)
}
if ( (*(int *)(*(matchCtx+0xe4) + 0x4c) <  DAT_00c84c7c)        // clock+0x4c < 3600.0
   && (*(int *)(*(matchCtx+0xd8) + 0x4b4) < 500000) ) {         // aux counter < 500000 (0x7a120)
    uStack_1ba = 1;                       // KEEP PLAYING (not frozen)
} else {
    uStack_1ba = 0;                       // FREEZE → half/full handling, builds MatchFreeze log (§2)
}
```
**Refinement of the task's statement:** the *continue-playing* test is an **AND**
`(clock+0x4c < 3600.0) && (aux+0x4b4 < 500000)`. By De Morgan, the **freeze / boundary trigger is the
OR**: `(clock+0x4c >= 3600.0) || (aux+0x4b4 >= 500000)`. (The task phrased it as a single AND with
`>=`; the engine actually ANDs the two *continue* conditions.)

- `DAT_00c84c7c == 3600.0f` — **VERIFIED** (constant 0x45610000 decodes exactly to 3600.0; lazily
  initialized, guarded by `_DAT_00c84c80 & 1`).
- The compare is written `*(int *)` but both operands are non-negative; for positive IEEE-754 floats a
  bitwise-int `<` is order-preserving, so this is effectively `clock+0x4c < 3600.0f`. INFERRED.
- `+0x4c` on the clock sub-object = the **"elapsed-against-limit" / freeze counter**, distinct from the
  display clock at +0x44. VERIFIED location; **units/meaning APPROXIMATE**.
- **3600 is NOT the half length.** Prior research (`EGAMESTATE_MAPPING.md`, `DETAILED_ENGINE_HANDLERS`)
  found half length is data-driven from `GCRoundPlayTimeModule` / `RoundPlayTime.dat` ("PlayTime",
  defaults 2700.0/5400.0 s). So **3600.0f here is a hard engine-internal freeze/animation cap** (max
  ticks before a forced boundary), not the configurable 45'/90'. Reconciliation INFERRED.
- `matchCtx+0xd8` is a second sub-object; `+0x4b4 < 500000` is a secondary guard (plain decimal
  500000 = 0x7a120 — likely a simulated-event/stoppage safety cap). Meaning PROVISIONAL.

### 5c. Interruption behavior  (VERIFIED structure)
When frozen (`uStack_1ba = 0`), `FUN_0077e850` (under `DAT_00c84954 != 0`) builds the
`>>>SOAKDATA>>> MatchFreeze(%d) MatchFreezeText(%s)` diagnostic from the current state (§2) and runs
cleanup (`FUN_0048ba60`, `FUN_0048f140`, `FUN_004918c0`, string frees). This is the freeze/interrupt
path; it reads but does not write the state (the actual state change to HalfOver/GameOver happens via
the out-of-scope writer, §3).

---

## 6. State ↔ resolve / command-tree relationship  (VERIFIED ordering)

Driver: `FUN_0077ee80` @ `0077ee80` = **`GCMatchModule::ProcessData`** (string at line 638), the
per-tick entry (SEED, depth 0). Per-tick order in the "match running" branch (`*param_1 != 0`):

1. `cVar6 = FUN_0077ccb0()` (line 788) — gate predicate (frozen/paused? checks `DAT_00c8494c==3`,
   `DAT_00c84954/0x50`). If true, early-out (no resolve this tick).
2. If not gated and the `0x1000000` flag on `in_ECX+0x14` is clear:
   `cVar6 = FUN_0077cd40()` (line 802) — **the per-tick RESOLVE / state-advance step** (calls
   `FUN_0077c710` event resolution). Returns whether an event fired.
3. `if (param_1[1]==0 && cVar6==0) cVar7 = FUN_0077e850()` (line 818) — **clock-boundary check**
   (§5b); only evaluated when no event fired this tick. `bVar19 = (cVar7==0)`.
4. If a terminating condition holds (`FUN_0048c4a0` and/or `bVar19`/`cVar6`/`param_1[1]`):
   `FUN_0077cf20(...)` (lines 828/855) — **command/event-tree executor / finalize** (fires render &
   report commands `0x414`, clears flags). Followed by `FUN_0077c660()` (line 870) — clock display
   update — and `FUN_00661710()` fixture-advance.

**Therefore:** resolve (`FUN_0077cd40`/`FUN_0077c710`) and the command tree (`FUN_0077cf20`) run
**every tick, gated by game state**, not once per state. Events are detected during resolve; the clock
boundary is only tested on ticks where no event fired; the command tree executes the chosen
event/finalize. The state field is consumed as context (and for the freeze log) — the commit of the
new state value is performed by the out-of-scope writer (§3).

Per-tick call chain (VERIFIED from call sites):
```
GCMatchModule::ProcessData  FUN_0077ee80
 ├─ FUN_0077ccb0            (gate: paused/frozen?)
 ├─ FUN_0077cd40            (resolve / state-advance)  ── calls ─▶ FUN_0077c710 (event RNG+selection)
 ├─ FUN_0077e850            (clock boundary → half/full freeze; reads state via ToString FUN_0077ddc0)
 ├─ FUN_0077cf20            (command/event-tree executor + finalize; emits cmd 0x414)
 └─ FUN_0077c660            (display clock update → DAT_00c809d4+0x30)
```

---

## 7. UNKNOWN list

1. **SetGameState writer** — no store to `*(matchCtx+0xe4)+0x30` in either dump. Prime suspects:
   match-context family `FUN_009efa10/20/30/40/60/70`, `FUN_009efb10/b40` (bodies not dumped). Need
   their decompilation or a runtime write-watchpoint. (§3)
2. **Exact transition edges** — all `from→to` edges are INFERRED from name pairing + tick order; none
   is byte-confirmed as a +0x30 store. (§4)
3. **HalfOver vs GameOver selector** — the half-counter field distinguishing 1st-half end from full
   time is not in these functions. UNKNOWN.
4. **`+0x4c` units** — confirmed as the freeze counter compared to 3600.0f, but its tick unit and how
   it relates to the config half length (RoundPlayTime.dat 2700/5400 s) is APPROXIMATE/UNKNOWN. (§5b)
5. **`matchCtx+0xd8 +0x4b4 < 500000`** — second freeze guard; meaning PROVISIONAL (likely event/
   stoppage cap). (§5b)
6. **`eGameStateSpare` (0x07)** and **`PenaltyShootout` (0x15)** entry conditions — not observed in
   scope. UNKNOWN.
7. **`FUN_0077ccb0` gate semantics** — exact pause/freeze meaning of `DAT_00c8494c==3`,
   `DAT_00c84954`, `DAT_00c84950` not fully resolved. INFERRED only.

---

### Threshold summary (byte-confirmation status)
| Constant | Value | Status |
|----------|-------|--------|
| `DAT_00c84c7c` (freeze clock cap) | `0x45610000` = **3600.0f** | **VERIFIED** |
| aux guard `matchCtx+0xd8 +0x4b4` | `< 500000` (0x7a120) | VERIFIED compare / meaning PROVISIONAL |
| display conversions | `1/3600` (0.00027777778), `1/60` (0.016666668) | VERIFIED |
| half length (45'/90') | 2700.0 / 5400.0 s | config-driven (prior research); NOT this 3600.0 |
