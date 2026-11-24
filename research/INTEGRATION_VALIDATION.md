# Cross-Subsystem Integration Validation (WORKSTREAM E)

Standalone harness: `tests/test_integration.c` (own `main()`), built and run with
`make integration` (links only the subsystem `.c` it exercises + `-lm`; lives under
`tests/` so the main `test`/`all` globs never pull it in).

**These are NOT final-score similarity tests.** Each test wires two or more of the
independently reconstructed clean-room subsystems together and asserts on the
**intermediate values and state transitions** that flow across the seam — the thing
that is actually recoverable and falsifiable without the game binary.

Result (this run): **262 checks, 0 failures — `ALL INTEGRATION TESTS PASSED`.**

---

## What each test connects and the intermediate values it checks

### TEST 1 — clock advance -> eGameState FSM transition -> pending-event resolve gating
Subsystems: `detailed_engine` + `match_lifecycle` + `match_state`.
Chain: `cm_detailed_update` -> `cm_lifecycle_advance` (float clock + FSM) -> resolve
callback.

Intermediate state/values asserted:
- Initial state is `KICKOFF_WAITING`; first pump transitions `KICKOFF_WAITING -> IN_PLAY`.
- **Resolve gating (the verified order):** the resolve callback fires **only** when the
  post-advance state is `IN_PLAY` **and** `event_pending` is set. Verified across pumps:
  not-pending -> no call; pending+live -> exactly one call; pending bit cleared after.
- **Execution order** (clock advanced *before* resolve) proven by reading
  `life->clock.elapsed_s` *inside* the callback and asserting it already equals the
  accumulated dt (2.0s, then 4.0s).
- **Half transition:** elapsed reaching `half_seconds` yields `HALF_OVER`; because the
  post-advance state is no longer `IN_PLAY`, a pending resolve is **gated out and the
  pending bit is preserved** (not consumed).
- `HALF_OVER -> KICKOFF_WAITING` with `half==2`; then `-> IN_PLAY` where the preserved
  pending bit finally resolves.
- **Full-time / terminal:** elapsed reaching `full_seconds` yields `GAME_OVER`, which is
  terminal — further pumps stay `GAME_OVER` and resolve never fires even when pending.
- Every resolve call observed `IN_PLAY` (invariant flag).
- `match_state` helpers cross-checked: `is_spotted`, `paired_activation`
  (GoalSpotted -> GoalActivated; non-spotted -> `NUMS`).

### TEST 2 — resolver selection -> empirical sampled result -> match statistics
Subsystems: `resolver_dat` empirical sampler driven by the `resolver_eval` LCG.
Chain: in-memory `cm_dist_record` of 3 zero-variance buckets with known weights/means
-> `cm_resolver_sample_empirical` x N -> aggregate statistic.

Intermediate values asserted:
- **Per-bucket shape:** observed draw frequency of each bucket value converges to its
  weight share `N_i/total` (0.100/0.300/0.600) within 0.01 over 200k draws
  (measured 0.102/0.299/0.599).
- **Moment check:** sample mean converges to the weight-weighted bucket mean
  (expected 7.600, measured 7.588, tol 0.05).
- **Determinism:** same LCG seed reproduces a byte-identical sample stream; a different
  seed changes it (confirms the sampler actually consumes the LCG).

### TEST 3 — predicate weights -> rank threshold
Subsystem: `predicate_weights` (header-only table + helpers).
Chain: weight table -> `cm_predicate_weight_of` -> `cm_predicate_passes_rank`.

Intermediate values asserted:
- Table count is 84; threshold constant is `80.0f`.
- Threshold invariant across **every** entry: `passes_rank(x) <=> weight(x) >= 80`, and
  the lookup round-trips each stored weight.
- Exactly **18** predicates survive the 80.0 filter (1@100 + 6@92 + 10@90 + 1@85) — the
  four header-named survivors (TeamGoals, WeakDefence, PlayerHadStrongAttackingMatch,
  TeamDoesManyQuickAttacks) all pass; 70/35/unknown all fail (-1 for unknown).
- Ranking order is strictly monotone by weight (100 > 92 > 85 > 70).

### TEST 4 — formula node eval -> value
Subsystem: `formula` (expression tree + registry + rule conditions).

Intermediate values asserted:
- Operator/aggregate string vocabulary maps to the VERIFIED opcodes
  (add/div/mean/prod/greaterthan…).
- Binop `6/2 = 3`; **div-by-zero -> `CM_FEVAL_ERROR`** (guarded, not invented).
- Aggregates over {3,4,5}: PRODUCT=60, MEAN=4, SUM=12.
- Host-context variable resolution: `Attr(12) * Scale(2) = 24`.
- **Registry defaults resolve** from the recovered `.data` constants: `MaxSprintingSpeed`
  = 9.0, `KeyPassesPerformance` = 0.5.
- **Unresolved is reported, never invented:** unknown formula name and unknown variable
  both return `CM_FEVAL_UNRESOLVED` with `*out` left untouched.
- Condition engine AND semantics, fail-closed (one false -> whole set false).

### TEST 5 — LCG determinism shared across subsystems
Subsystems: `resolver_eval` LCG primitive, used identically by `resolver_dat` and the
node evaluator.

Intermediate values asserted:
- Recurrence uses the VERIFIED constants `1664525 / 1013904223 mod 2^32`.
- Same seed -> identical raw `cm_lcg_next` streams; `cm_lcg_unit` stays in `[0,1)` and is
  deterministic.
- **Cross-subsystem equality:** a `CM_NODE_RANDOM` eval in `resolver_eval` advances the
  exact same stream (and leaves the RNG state) that a manual `cm_lcg_unit` from the same
  seed produces — proving one shared RNG, not two divergent ones.
- CONST/BINOP node values are RNG-free and exact (3*4 = 12).

---

## Coverage matrix: Differentially validated vs only Implemented

| Cross-subsystem chain | Status |
|---|---|
| clock -> FSM transitions -> resolve gating/order (engine+lifecycle+state) | **Differentially validated** (intermediate transitions + order + terminal) |
| empirical sampler -> per-bucket shape + moments, LCG-deterministic | **Differentially validated** (mechanism + determinism; on synthetic buckets) |
| predicate weight -> 80.0 rank threshold + ordering | **Differentially validated** (full-table invariant) |
| formula AST -> value / defaults / unresolved reporting | **Differentially validated** (operators, defaults, unresolved) |
| LCG shared across resolver_dat + resolver_eval | **Differentially validated** (stream + state equality) |

"Differentially validated" here = the intermediate values/transitions across the seam
are asserted against the recovered specification and are falsifiable, independent of any
final match outcome. It is **not** a byte-for-byte diff against the original binary's
runtime (which would require the game executable — out of scope and not run here).

## Links that remain UNVALIDATED because a subsystem piece is still UNKNOWN

- **eGameState transition WRITER** (`SetGameState`, the real `+0xe4` sub-object method) is
  UNKNOWN. Test 1 validates the *main-line* progression this clean-room driver models;
  the spotted/activated set-piece transitions and stoppage time are **not** driven by the
  original writer, so that seam is modelled, not diffed.
- **Context/index picker** (`FUN_005b5100`, RECOVERED = key→index lookup, NO RNG — not obfuscated): which `.dat` file/group a given
  player maps to is APPROXIMATED. Test 2 validates the sampling *mechanism* on controlled
  buckets; the real attribute/tactic -> group selection is **not** validated.
- **Real `.dat` distribution shape at the per-value level:** only the recovered moments
  are VERIFIED; CM's exact intermediate-triple shape is not yet reproduced. Test 2 uses a
  synthetic record (no game data is embedded/redistributed per repo policy), so the
  sampler-vs-real-file shape remains unvalidated here.
- **Formula bodies (trees):** live in the encrypted `formula.xml.blowfish` (loader in a
  protected region, key not plaintext — see `BLOWFISH_DATA_RECOVERY.md`). Test 4 validates
  the *engine* and the recovered *defaults*; formula-ref chains into real bodies are
  reported UNRESOLVED by design and cannot be value-diffed.
- **Resolve body contents** (`FUN_0077cf20 -> FUN_0077c540 -> FUN_009e9570`): the detailed
  engine integrates resolve as a callback (order is the faithful part). The predicate
  score-filter-rank body is validated in isolation (Test 3) but its *wiring into the live
  resolve step* is not exercised end-to-end, because the real per-tick event payload and
  cadence/dt source are UNKNOWN.
- **Match cadence / dt source** (per-program-step pump): UNKNOWN; Test 1 supplies dt from
  the caller, so wall-clock-to-dt mapping is not validated.
