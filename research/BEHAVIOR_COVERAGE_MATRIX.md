# CM2008 Match Behavior — Coverage Matrix

Status per behavior, not a fidelity %. Stages: **Identified** -> **Data-flow mapped**
-> **Algorithm recovered** -> **Implemented** -> **Differentially validated**.
"Validated" here = against the recovered specification (clean-room tests), NOT a
runtime diff vs the original exe (that needs a lawful runtime; see §Runtime).

## Match lifecycle / clock / state
| behavior | status | evidence |
|---|---|---|
| eGameState value set (31+Nums) | Validated | ToString FUN_0077ddc0; match_state.* + tests |
| continuous float clock | Implemented | FUN_0077c660; match_lifecycle.* |
| per-tick order (ccb0→cd40→e850→cf20→c660) | Implemented | detailed_engine.c; integration test 1 |
| pending-event gate/lifecycle | Validated | cd40 set / cf20+ccb0 clear; integration test 1 |
| GameOver trigger | Algorithm recovered | FUN_0077e850 (clk+0x4c≥3600 OR aux+0x4b4≥500000) |
| HalfOver-vs-GameOver selector | Identified | half-counter field UNKNOWN |
| **SetGameState writer** | Data-flow mapped (write not found) | bounded: thiscall method of *(matchCtx+0xe4), writes [this+0x30] in out-of-scope event-execution consumer; 009ef family REFUTED; 0 writer-idiom hits in 36MB → **needs runtime watchpoint** |
| Spotted→Activated lifecycle | Data-flow mapped (edges INFERRED) | Spotted=queue-time(cd40), Activated=execute-time(cf20) |

## Resolver / selection / RNG
| behavior | status | evidence |
|---|---|---|
| .dat format + moment triples | Validated | parser + tests |
| LCG PRNG (seed @matchctx+0x2c) | Validated | bit-exact; integration test 5 |
| empirical sampler | Validated (vs recovered spec) | integration test 2 (200k draws, moment check) |
| predicate ranking weights (84) | Validated | FUN_009e5160; predicate_weights.h; test 3 |
| rank threshold 80.0 | Validated | FUN_0077c540; 18 survive |
| **picker FUN_005b5100** | Implemented (VERIFIED) | NOT anti-disassembly (false alarm) — disassembled: a deterministic LINEAR find-by-key (1-based index of first entry whose *(entry+0xc)==*target), NO RNG. match/sim/picker_real.h + tests. See PICKER_RECOVERY.md |

## Formula system
| behavior | status | evidence |
|---|---|---|
| formula.xml grammar | Validated | champman.xsd; FORMULA_XML_GRAMMAR.md |
| evaluator (FUN_00818120) + opcodes | Implemented | binop 1-5, agg 0-2; formula_eval.c; test 4 |
| calc operator vocabulary (9) | Validated | {sum,mean,prod,div,sub,add,mul,unary}+{gt,lt,eq,ne} |
| 88 default constants / 810 names | Data-flow mapped | FORMULA_COEFFICIENT_REGISTRY.md |
| **harvest path** (two centralizers) | Algorithm recovered | FUN_005d6dd0 (101 PlayerConstant per-player) + FUN_009f4bd0 (360 globals → ResolverSettings struct ~0x5e0B, byte-exact offsets) |
| **formula trees (bodies)** | Algorithm recovered (DATA) | formula.xml **DECRYPTED** (key `deliverance`, derived statically): 1349 formula trees + all coefficients now in the clear. Parse → wire into the evaluator. (Game data kept local.) |
| condition/effect engine | Data-flow mapped | rule=conditionset(AND)→effectset; 4 compares; parser FUN_005b4db0 |
| **effect targets (function/callrule)** | Identified | produce AI weights (ProspectiveManagers/ManagerSacking), NOT real-time match mutation; native targets UNKNOWN |

## Movement / player actions
| behavior | status | evidence |
|---|---|---|
| entity pools (player/ball/event/cold) | Data-flow mapped | A=DAT_00c784a8 stride0x20, B=DAT_00c779ac stride0x2c, C/D; handle=(ptr-base)>>5 |
| movement state word (entity+4) | Data-flow mapped | writers cd40/cf20/a023f0; reader=view |
| tackling / loose-ball contest | Algorithm recovered | FUN_0077c710 attribute-weighted 2-sided contest (rating@+0x3c) |
| penalty shootout | Algorithm recovered | FUN_00a023f0 best-of-5+SD, p≈0.408 |
| player pos / velocity / accel / direction | Identified (offsets UNKNOWN) | consumed in view draw + geometry accessors whose decompile is broken → re-disasm needed |
| ball pos/velocity | Data-flow mapped | pool B via view+0xc0; offsets UNKNOWN |
| passing / receiving / shooting | Identified | message-ids/atoms only; math in sim loop (out of corpus) |
| continuous integrator (pos+=vel*dt, speed clamp=9) | Identified | coefficient known (MaxSprintingSpeed=9); code path in sim loop, not in corpus |

## Cross-subsystem validation (Workstream E)
262 checks, all passing. Differentially validated (vs recovered spec): clock→FSM→
pending gating; resolver sampler distribution + LCG determinism; weight→rank threshold;
formula eval/defaults/unresolved; one shared RNG stream. `make integration`.

## Runtime (what remains, and why)
Static analysis is exhausted for four items, all blocked by the SAME two causes:
1. **SecuROM virtualization** (independently corroborated on the sibling title CM2010,
   MagnificentS/cm2010-run): picker body, formula-key loader, comparison dispatch, the
   state-writer/effect-execution consumer are protector-junk on disk.
2. **Encrypted payload / sim-loop code not in the dumped corpus**: formula trees (in
   formula.xml.blowfish), the continuous movement integrator.

Closeable ONLY by read-only observation of a lawful, unmodified CM2008 runtime:
- SetGameState writer → write-watchpoint on *(*(matchCtx+0xe4)+0x30).
- picker → SOLVED statically (was a false alarm; disassembles to a deterministic
  find-by-key, no RNG). See PICKER_RECOVERY.md / match/sim/picker_real.h.
- formula.xml → SOLVED statically (key `deliverance` derived from the loader; no runtime
  needed). Formula trees + coefficients are decryptable with tools/decrypt_blowfish.py.
- movement offsets → correct re-disassembly of the geometry accessors (clean bytes).
No DRM bypass, no cracked-exe execution.
