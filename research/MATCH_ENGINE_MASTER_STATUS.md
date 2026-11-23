# CM2008 Match Engine — Master Status

> **CRITICAL CORRECTION (verified):** `FUN_008f90f0` is the top-level UI/game-flow
> **command-tree** interpreter (89 recovered command names: SaveGame, SwitchMatchScreen,
> ViewHalfTimeTeamTalks, Tactics…), NOT the match update. The REAL detailed match engine
> is **`GCMatchModule::Update` = `FUN_0077ee80`** (vtable 0x00ba2904 slot +0xa8), invoked
> virtually by the module manager; it iterates fixtures → FUN_0077cf20→FUN_0077c540→
> FUN_009e9570 (predicate rule engine). Match **clock = continuous float seconds**
> (FUN_0077c660), NOT fixed-tick. Half/full-time = **eGameState FSM, 31 states**
> (FUN_0077ddc0). See research/COMMAND_TREE_*.md, MATCH_CLOCK_AND_UPDATE_LOOP.md.

Central index of the reverse-engineering + reconstruction. Categories reported
separately: **VERIFIED** (binary/byte-backed + validated), **INFERRED** (strong,
not conclusively demonstrated), **APPROXIMATE** (deliberate substitute),
**UNKNOWN**. Target: retail `CM2008.exe` SHA-256 `5b14a40b…37e81`.

## Two independent match engines
1. **Quick statistical resolver** — `MatchResolverDistributions.*.dat` + sampler.
2. **Detailed command-tree engine** — `FUN_008f90f0` recursive node interpreter.
They are not reachable from each other (VERIFIED, pipeline RE).

## Architecture (VERIFIED)
Recursive typed-node-tree interpreters are the shared shape:
- command/axiom tree `FUN_008f90f0` (96-way dispatch, 36 recursion sites)
- predicate DAG `FUN_009e9570→009e8f60/92b0/9200`
- distribution/formula node eval `FUN_00818120→00817fe0/f20/df0/080`

## Function map (see research/RESOLVER_FUNCTION_MAP.md for full table)
| addr | role | conf |
|---|---|---|
| FUN_00a098a0 | .dat loader (`#Database#/MatchResolverDistributions.%i.dat`, 6 files) | VERIFIED |
| FUN_00662cf0 | named-field lookup (obfuscated body) | INFERRED |
| FUN_00818120 (+00817fe0/f20/df0/080) | node-tree sampler/evaluator | VERIFIED |
| FUN_005b5100 | index/bucket picker (obfuscated) | UNKNOWN |
| FUN_008f90f0 | UI/game-flow command-tree interpreter (NOT the match engine) | VERIFIED |
| FUN_0077ee80 | GCMatchModule::Update — REAL detailed match engine (vtable 0x00ba2904+0xa8) | VERIFIED |
| FUN_0077c660 | continuous float match clock (elapsed @ matchCtx+0xe4+0x44) | VERIFIED |
| FUN_0077ddc0 | eGameState FSM (31 states: Kickoff/InPlay/HalfOver/GameOver+events) | VERIFIED |
| 00724790→00a1fdf0 / 009b2600→009aecc0 / 0093c7f0→008a7770 | movement / offside-duties / axiom leaves | VERIFIED(edge) |
| GCFormulaModule *Performance writer | attributes→performance | UNKNOWN |
| outer match clock/tick loop | submits trees to FUN_008f90f0 | UNKNOWN |
| LCG (inlined) | state=state*1664525+1013904223 | VERIFIED |

## .dat format + transform (VERIFIED)
`file=section[3]{u32 count, record[]}`; `record={u32 nameLen,name,block0,triple[],term}`;
`block0={u32 nBuckets, bucket{u32 count,u32 nQ,f32 quant[nQ]}}`;
`triple=12B {u32 N, f32 sumX, f32 sumX2}` = **cumulative moment summary**.
Group last triple ⇒ mean=sumX/N, var=sumX2/N−mean². Validated: all metrics
reconstruct to realistic stats. Sections [64,64,6]=134 records; 70 metrics.
Parser `match/sim/resolver_dat.c` (6/6 files parse to EOF). See RESOLVER_TRANSFORM.

## Reconstructed & buildable (tests pass)
| layer | file | status |
|---|---|---|
| .dat parser | resolver_dat.c | VERIFIED structure+moments |
| moment sampler | resolver_dat.c (Gaussian-from-moments) | moments VERIFIED; draw-shape APPROXIMATE |
| quick resolver (pooled) | resolver.c | VERIFIED moments; context pooling APPROXIMATE |
| node evaluator + LCG | resolver_eval.c | LCG VERIFIED; evaluator INFERRED |
| detailed engine (tick) | sim_core/movement/defending/events | APPROXIMATE (architecturally superseded by command-tree) |

## APPROXIMATE (isolated, flagged, NOT presented as original)
- Detailed tick engine gameplay formulas (movement speed, pass/tackle/shot probs).
- Gaussian-from-moments draw shape (vs CM's exact per-value-level shape in intermediate triples).
- Pooled-context sampling (vs per-player attribute→context index).

## UNKNOWN (open, with named dependencies)
- `.dat` per-value-level exact shape (intermediate triples) — recoverable next.
- Attribute→context index / picker `FUN_005b5100` (obfuscated).
- `GCFormulaModule` writer of `<Metric>Performance`.
- Outer match clock / update-loop that drives `FUN_008f90f0`; tick frequency.
- Command-tree full node-type vocabulary (~75) + handler map.
- RNG lifecycle: seed source/timing, shared vs per-engine state, reset policy.

## Evidence trail
research/: RESOLVER_ARCHITECTURE, DATA_FLOW, FUNCTION_MAP, VALIDATION,
resolver-format, resolver-consumers, resolver-indexing, resolver-loader,
resolver-transform, independent-validation, ARCHITECTURE, match-pipeline-deep.
docs/findings/: match-engine, match-predicates, match-pipeline, player-attributes,
tactics-config, staff-schema, finance-model, game-database.
