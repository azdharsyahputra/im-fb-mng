# eGameState +0x30 Writer — Consumer Trace (statewriter.log, 154 funcs)

Clean-room, read-only. Source dump: `scratchpad/statewriter.log` = the event-execution consumer
closure (FUN_0077cf20, FUN_009f2410, FUN_009f9760, FUN_009f9400, FUN_009f7d90, FUN_0077ccb0 + 2
levels of callees). Cross-checked against `scratchpad/wave3.log`, `research/EGAMESTATE_COMPLETE.md`,
`research/MATCH_STATE_EFFECTS.md`. Decompiled text treated as DATA.

Target field (VERIFIED, prior work):
```
matchCtx   = *(module+4) + 0x110
subObj     = *(int*)(matchCtx + 0xe4)     // clock/state sub-object
eGameState = *(int*)(subObj + 0x30)       // raw int 0..0x1f (31 states)
```

---

## 1. +0x30 store verdict — **bounded UNKNOWN (NOT FOUND in these 154 funcs)**  VERIFIED

Exhaustive grep of the consumer dump:

- **`+0xe4` appears exactly ONCE as a data access** (line 2380, inside `FUN_00a271a0`):
  `*(float *)(*(int *)(iVar4 + 0xe4) + 0xac)` — a **READ of the `+0xac` float** inside a debug
  `_fprintf` dump line (`iVar4 = FUN_009efa90()` = matchCtx). It reads `+0xac`, not `+0x30`, and
  stores nothing. (The only other `0xe4` hit, line 7521 `0xe476f7cf`, is an RNG/hash constant.)
- **No store of the form `[reg+0x30] = x` where `reg` came from `[x+0xe4]` exists.** All 31 `0x30`
  occurrences are FP control words (`0x30000`/`0x300` masks), bitfield packs (`|0x3000`), or two
  unrelated `+0x30` stores whose base is NOT `+0xe4`:
  - line 4605 `FUN_009f4180`: `*(char*)(*(int*)(param_1+0x3c)+0x30+param_1)=...` — base `[param_1+0x3c]`, a per-side scratch byte.
  - line 4776 `FUN_009f4a90`: `FUN_009f3b00(iVar1*0x234 + 0x30 + param_2, ...)` — a computed array-element address passed as an arg.
  Neither touches the `+0xe4` sub-object.
- `wave3.log` confirms the only `(…+0xe4)+0x30` site binary-wide is the **READ** at wave3:14372
  (feeds `eGameState::ToString` FUN_0077ddc0). No store.

**Therefore the SetGameState writer is NOT in the event-execution consumer closure either.** This
refines the prior boundary: the writer is one level *deeper* than this dump reaches (see §4).
Classification: **UNKNOWN, precisely bounded.**

---

## 2. The `+0x1072c` field = GCMatchModule match/period lifecycle state (NOT eGameState)  VERIFIED behavior / INFERRED labels

`+0x1072c` is a field on the **GCMatchModule** (`this` = module), distinct from the per-tick
eGameState (`subObj+0x30`). It is a **coarse session/period mode** with values {1,2,3,4}; eGameState
is the fine-grained 31-state event machine on the clock sub-object. `+0x1072c` **gates** whether the
per-event executor runs (which in turn commits eGameState) — it does not hold eGameState.

### Writers of `+0x1072c`
| Line | Function | Store | Meaning |
|---|---|---|---|
| 2520 | FUN_009f9400 | `= param_2` | period-setup installs mode (param_2 ∈ {0,1} special-cased) |
| 3341 | FUN_009f7f00 | `= 4` | when team-setup call returns 0 → **abort/terminal** |
| 7345 | FUN_009f7d90 | `= param_2` | load/restore path, only when saved-state tag `FUN_005ccf30()=='\x03'` |
| 7375 | FUN_009f7d90 | `= 4` | saved-state tag ≠ 3 → **invalid/abort** |
| 7630 | FUN_009f4bd0 | `= 4` | when `FUN_005cd9a0()==0` → **abort/terminal** |

### Readers of `+0x1072c`
| Line | Function | Test | Consequence |
|---|---|---|---|
| 2092 | FUN_009f2410 (consume) | `1 < v && v < 4` (→ 2 or 3) | run executor `FUN_009f2220()` |
| 2569 | FUN_009f9400 | `v != 1 && DAT==0` | set module+0x10801 = (live-input flag) |
| 4251 | FUN_009f2e70 | `v < 2 || 3 < v` (→ 0,1,≥4) | non-active branch |
| 4896 | FUN_009f7a50 | `v < 2 || 3 < v` | non-active branch |
| 5828/5833/5841 | FUN_009f7730 (setup) | `==1` vs `==3` vs `1\|3` | v=1 init via bit `>>5`, v=3 via `>>6`, else `FUN_00493e40(0)` |
| 6390 | FUN_009f0fe0 | `==1 \| ==3` | enable live path |
| 7371 | FUN_009f7d90 | `v == 3` | module+0x10801 = (v==3) |

### Value → meaning (branch behavior VERIFIED; labels INFERRED)
- **1** = live period, fresh/first-half start (kickoff) — primary play mode (distinct init bit `>>5`).
- **2** = active play (events execute; grouped with 3 in consume gate 2<... check).
- **3** = live period, restored/resumed (saved-state tag 3 in FUN_009f7d90; init bit `>>6`); also event-executing + input-enabled.
- **4** = ended / aborted / invalid — **terminal**, written by 3 distinct failure guards.

Relationship to eGameState: `+0x1072c ∈ {2,3}` is the precondition under which the consume stage
(FUN_009f2410) invokes the executor that writes `subObj+0x30`. `+0x1072c` is the *enclosing* state;
eGameState is the *inner* per-event state.

Companion queue fields (VERIFIED): `+0x10818` = pending flag (set 2516 FUN_009f9400=param_4, 5785
FUN_009f7730=param_3; cleared 2096 FUN_009f2410=0 after consume); `+0x1081c` = result/time index
(set 2518 FUN_009f9400, 5787 FUN_009f7730 from `FUN_005a6320()`).

---

## 3. Event data-flow: queue → execute → (commit is out of scope)  VERIFIED to the boundary

```
FUN_0077cf20 (per-tick driver)
  ├─ FUN_009efa70 → FUN_009f9760   execute/render: resets per-entity flags (+0x1ac=0x100000,
  │                                +0x1a8=0x20000 over player arrays); NO +0xe4 / +0x30 write
  ├─ FUN_009efa40 → FUN_009f2410   CONSUME: if (+0x10818!=0 && +0x1072c∈{2,3})
  │        └─ FUN_009f2220()   <-- EXECUTOR, **NOT in dump** (called line 2094 only)   ← BOUNDARY
  │        └─ then +0x10818 = 0 (clear pending, line 2096); FUN_009f9e60() (also undefined)
  └─ FUN_009efc30 → FUN_009f0c40   report/finalize

Posting paths (write the queue, never +0x30):
  FUN_009f9400  (via FUN_009efa20)  writes +0x10818,+0x1081c,+0x1072c; then FUN_009f7730 (period reinit)
  FUN_009f7d90  (via FUN_009efa10)  load/restore: writes +0x1072c; gated on saved tag==3
```

Every reachable function in this closure writes only **GCMatchModule** fields (`module+0x107xx /
+0x108xx / +0x1072x`) and per-entity slabs (`+0x1e8`, `+0x2d8`, `+0x1250c`, `+0x12510`). **None
dereferences `matchCtx+0xe4` to store** — the sub-object pointer is not even materialized in this
closure except the single `+0xac` debug read. The restart-setup helpers (`FUN_009f7f00`,
`FUN_009f4bd0`) operate on `param_1+0x2d8+...` team records, not the clock sub-object.

---

## 4. Remaining boundary — exact unknown

The eGameState `subObj+0x30` commit lives **one call deeper than this dump**, in:

- **`FUN_009f2220`** — the per-event executor invoked by the consume gate (FUN_009f2410 line 2094)
  exactly when `+0x10818` pending is set and `+0x1072c ∈ {2,3}`. It is referenced but **has no
  definition** in `statewriter.log` or `wave3.log`. This is the prime suspect: it turns the queued
  request (`+0x1081c` index) into the applied eGameState.
- Secondary undefined callees on the same path: `FUN_009f9e60` (2097), and the `FUN_005cxxxx`
  render/apply helpers fanned out from `FUN_009f9760` (e.g. `FUN_005cdd70`, `FUN_005cf0a0`,
  `func_0x005cc520`) that were not expanded to leaf depth.

Consistent with prior work (`MATCH_STATE_EFFECTS.md §1.3`): binary-wide there is **ZERO** instance
of the idiom *load `[reg+0xe4]` then store `[sameReg+0x30]`*. The writer therefore receives the
sub-object pointer as a **passed-in `this`/param** and executes a generic `mov [this+0x30], <state>`
with no `+0xe4` reload — statically indistinguishable from thousands of `mov [this+0x30],x` stores
without disassembling `FUN_009f2220`'s body and its callees.

**Runtime evidence that would name it:** a hardware **write-watchpoint on
`*(*(matchCtx+0xe4)+0x30)`** during a live match; the faulting instruction's enclosing function (and
the `this` it writes) names SetGameState. Statically: dump/disassemble `FUN_009f2220` (VA 0x9f2220)
and its descendants and search for a `mov/c7 [this+0x30], imm∈0..0x1f` where `this` is the param
threaded from `FUN_009f2410`'s `subObj`.

---

## Classification summary
| Claim | Status |
|---|---|
| No `+0x30`-of-`+0xe4` store in the 154-func consumer closure | **VERIFIED** |
| Only `+0xe4` access in closure is a `+0xac` READ (line 2380, debug fprintf) | **VERIFIED** |
| `+0x1072c` writers/readers map (lines above) | **VERIFIED** |
| `+0x1072c` = module period-lifecycle {1,2,3,4}, gates executor; ≠ eGameState | **VERIFIED (behavior)** |
| `+0x1072c` value labels (1=kickoff,2=play,3=resumed,4=terminal) | **INFERRED** |
| Queue→consume→execute flow to `FUN_009f2220` boundary | **VERIFIED** |
| `FUN_009f2220` (+ descendants) is the actual `subObj+0x30` committer | **INFERRED / UNKNOWN body** |
