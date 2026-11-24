# Match-Engine Reconstruction — Dependency & Fidelity Report

Scope: the match subsystem only (lifecycle clock + eGameState FSM, detailed-engine
update order, quick statistical resolver). Every row is classed **VERIFIED** /
**INFERRED** / **APPROXIMATE** / **UNKNOWN** against retail `CM2008.exe`
(SHA-256 `5b14a40b…37e81`). Nothing here is decrypted game data; no decompiled
dump or game asset is committed.

## 1. Reconstructed modules and what grounds them

| reconstructed unit | source file | fidelity | evidence |
|---|---|---|---|
| eGameState enum **values** | `match/sim/match_state.h` | **VERIFIED** | eGameState::ToString (FUN_0077ddc0) name table: InPlay=0, HalfOver=0x0b, GameOver=0x0c, ApresGoal=0x0d, KickoffWaiting=0x0e, HalfActivated=0x18, Nums=0x1f (31 states) |
| continuous float clock (seconds; +0x44/+0x48/+0x4c layout) | `match/sim/match_state.h` | **VERIFIED** | FUN_0077c660 (hours = s/3600); clock sub-object `*(matchCtx+0xe4)` |
| GameOver trigger (end-counter ≥ ~3600) | documented, FSM uses provisional elapsed≥full | **VERIFIED (trigger)** / **APPROXIMATE (wiring)** | FUN_0077e850 gate on clock+0x4c vs DAT_00c84c7c; the counter's exact accumulation semantics vs wall-clock are not pinned, so the FSM uses elapsed≥full as a stand-in |
| detailed-engine **update ORDER** (advance clock+FSM → resolve only when live+pending → clear event bit) | `match/sim/detailed_engine.c` | **VERIFIED (order)** / **APPROXIMATE (resolve body is a callback)** | FUN_0077ee80 → FUN_0077cf20 → FUN_0077c540 → FUN_009e9570; event bit = fixture+0x18 0x1000000 |
| LCG PRNG + uniform idiom | `match/sim/resolver_eval.c` | **VERIFIED** | `state*1664525+1013904223`; `((state>>9)|0x3f800000)-1.0`; seed @ MatchInputs+8 |
| .dat parser (section[3]; record; block0; triple) | `match/sim/resolver_dat.c`, `match/resolver_format.c` | **VERIFIED** | FUN_00a098a0 loader; 6/6 files, 134 records, 70 metrics parse clean |
| triple = cumulative moment summary (N, ΣX, ΣX²) | `resolver_dat.c` | **VERIFIED** | reconstructs realistic per-metric means/vars across all metrics |
| empirical weighted-bucket sampler | `cm_resolver_sample_empirical` | **INFERRED** | draws from the recovered moment buckets (weight N_i, value ΣX_i/N_i); reproduces the real distribution shape |
| formula node evaluator (binop/agg/operand grammar) | `resolver_eval.c` | **VERIFIED (grammar)** | FUN_00818120 chain: node+0x24 op 1..5, node+0x20 agg 0..2 |
| lifecycle FSM main-line transitions | `match/sim/match_lifecycle.c` | **VERIFIED (states)** / **APPROXIMATE (transition logic)** | states verified; the actual writer is UNKNOWN so transitions are our reconstruction |
| half length (2700/5400 s) | `CM_HALF_SECONDS` default | **PROVISIONAL** | data-driven in original (GCRoundPlayTimeModule + RoundPlayTime.dat field "PlayTime"); 2700/5400 appear once each as likely defaults |

## 2. Faithfully reproduced (VERIFIED)
- eGameState state set and numeric values.
- Continuous-float time model and clock memory layout.
- Detailed-engine call ORDER and the resolve gate (live + event-pending).
- LCG constants + float-uniform idiom (bit-exact).
- `.dat` binary format and the moment-summary semantics of the triple.
- Formula-evaluator operator/aggregate grammar.
- GameOver *trigger condition* (end-counter threshold).

## 3. Deliberate approximations (APPROXIMATE / INFERRED — labelled in code)
- **Resolve body**: `detailed_engine` reproduces the *order*, not the internal
  per-event arithmetic of FUN_0077cf20/c540 (the body plugs in via callback).
- **Context/index selection** for the resolver stays explicitly approximate
  (`cm_quick_sample_ctx`, ctx_hint 0..5). NOTE: FUN_005b5100 IS recovered (key→index lookup, NO RNG — RESOLVER_PICKER_CLUSTER_RECOVERY.md); it is not the sampler, so this dependency is on the resolver's actual draw (location UNKNOWN, likely upstream).
- **Empirical sampler** is grounded in the recovered buckets but intra-bucket
  shape is modelled, not byte-recovered.
- **Gaussian-from-moments** sampler is kept ONLY as a labelled experimental
  baseline; it is **not** wired as original CM2008 behaviour.
- **FSM transition logic** (not the state values) is our reconstruction because
  the writer is unknown.
- **Half length** default is provisional (real value is data-driven).

## 4. Unknown / hard boundaries (static analysis insufficient)
| boundary | why static fails | what would resolve it |
|---|---|---|
| **SetGameState writer** | method of the +0xe4 sub-object, indirect/virtual; not in dumps | runtime capture of writes to matchCtx+0xe4+0x30 |
| ~~**FUN_005b5100 picker** exact reduction~~ RESOLVED | ~~anti-disassembly~~ Ghidra false-alarm; body recovered from crack (key→index lookup, NO RNG) | none — static (RESOLVER_PICKER_CLUSTER_RECOVERY.md) |
| resolver stochastic DRAW (the real RNG) | not in the picker cluster (proven RNG-free) | trace upstream match-engine LCG that seeds the key/pool |
| **Formula coefficients** | encrypted `formula.xml.blowfish` — will not decrypt | legitimate key/runtime observation only |
| **Module-manager indirect dispatch** to FUN_0077ee80 | indirect call through vtable 0x00ba2904+0xa8 | runtime capture of the dispatch |
| **end-counter accumulation** vs wall clock | the ~3600 counter is separate from elapsed; semantics unclear from static | runtime trace of clock+0x4c |

See `HARD_BOUNDARIES.md` and `RUNTIME_OBSERVATION_PLAN.md` for the capture plan
(observation only — the binary is not modified).

## 5. Tests proving the data flow (not just compilation)
`tests/test_reconstruction.c` integration block asserts the recovered architecture:
- resolve runs **only** while `IN_PLAY` **and** an event is pending;
- resolve observes the clock **after** it was advanced (order check: minute==10);
- the event-pending bit clears after one resolve (no re-fire);
- no resolve during KICKOFF_WAITING / HALF_OVER / GAME_OVER;
- pending bit is preserved across the half-time break;
- terminal GAME_OVER never resolves.
Plus FSM transitions over the float clock with verified enum values, LCG
determinism, parser round-trip, and sampler distribution shape. **ALL TESTS PASSED.**
