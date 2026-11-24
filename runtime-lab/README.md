# cm2008-lab — CM2008 resolver runtime experiment lab

A clean-room **experiment bench** for the Championship Manager 2008 match resolver. It is
**not** a reimplementation of the original engine: it loads the six recovered resolver
models (`MatchResolverDistributions.{1..6}.dat`), builds *hypothesised* runtime context,
samples the recovered conditional distributions, and runs perturbation / cross-model
experiments with reproducible traces. Every result carries a confidence tag
(**VERIFIED / INFERRED / APPROXIMATE / UNKNOWN**).

It **reuses** the repo's VERIFIED resolver parser + LCG (`../match/sim/resolver_dat.c`,
`resolver_eval.h`); it never executes the game and embeds no game data.

## Build (native macOS ARM64 / x86_64, C11)

    cd runtime-lab && make

## Run

    ./cm2008-lab --help
    ./cm2008-lab models list
    ./cm2008-lab models inspect --id 1
    ./cm2008-lab models compare --a 1 --b 6
    ./cm2008-lab player inspect
    ./cm2008-lab simulate --model 1 --metric GoalsScored --position ST --duty Attack \
                 --skill 18 --hypothesis combined --samples 20000 --seed 123
    ./cm2008-lab experiment position-perturbation --samples 10000 --seed 7
    ./cm2008-lab experiment duty-perturbation
    ./cm2008-lab experiment stamina-perturbation      # explicit NULL result (no mapping)
    ./cm2008-lab experiment skill-perturbation
    ./cm2008-lab experiment cross-model
    ./cm2008-lab experiment axis-correlation --model 1
    ./cm2008-lab experiment all
    ./cm2008-lab trace  inspect  --id skill-perturbation
    ./cm2008-lab report generate --id skill-perturbation
    ./cm2008-lab report axis-mapping --model 1 --metric GoalsScored   # Solver A + C
    ./cm2008-lab solve  capture-spec                                   # Solver B recipe
    ./cm2008-lab solve  picker --trace YOURCAPTURE.jsonl              # Solver B correlator
    ./cm2008-lab trace  ingest --file YOURCAPTURE.jsonl              # ingest the howto-schema capture

Runtime capture (Solver B, LAWFUL install only — the lab never runs the game): capture the
picker at VA `0x00817f3c` (rebase to `module_base + 0x417f3c` under ASLR) read-only per
`../research/RUNTIME_CAPTURE_HOWTO.md`, then `trace ingest --file` it. The correlator confirms
the LCG step count, identifies the reduction (FLOAT_CDF/MOD_CDF/UNIFORM_N/ARGMAX), and reports
the per-position tie. Validate the pipeline without the game via `trace gen-exact --cand C` then
`trace ingest`.

Data dir: `--dir PATH` or `$RESOLVER_DIR` (default: the canonical extracted path).
Traces go to `traces/<id>.jsonl`; reproducibility manifests to `experiments/<id>.json`.

## Tests

    make test

## What is VERIFIED vs INFERRED vs UNKNOWN

- **VERIFIED**: the .dat envelope + the context-grid structure; the LCG; the six files
  re-bin ONE population (bit-identical grand-total N/sumX/sumX2); the ±20.9 draw axis.
- **INFERRED (strong)**: the categorical context axis = the game's **7-bin player-position
  spine** (b0 Central Defender … b6 Striker; GK absent; b2 = the rare ~3% Sweeper). The lab
  identifies it as the one context axis with a non-uniform population marginal and maps a
  player's position onto its literal bin (see `../research/RESOLVER_AXIS_MAPPING.md`). Use the
  `position` or `combined` hypothesis.
- **INFERRED**: each remaining context axis's *role* (effect size + breakpoint symmetry).
- **APPROXIMATE**: the Gaussian-from-moments draw SHAPE; the `involvement` and `quality`
  projections of a hypothesised runtime quantity onto an axis range.
- **UNKNOWN**: the literal engine quantity behind the *continuous* (equal-frequency) context
  axes (read internally by the obfuscated iterators — not caller-supplied, see
  `PICKER_CALLER_CONTEXT.md`); the engine's model-selection logic; the exact picker reduction.
  The lab never fabricates these.

Hypotheses for `simulate`/experiments: `neutral` (all mid), `position` (literal spine bin from
the player's position), `involvement` (APPROXIMATE position+duty proxy onto the position axis),
`quality` (skill onto the differential axis), `combined` (position + quality).

## Status by phase
All four phases **done**: (1) models load/inspect/compare; (2) runtime state + context
engine + sampler + trace; (3) perturbation + cross-model + differential stats; (4) report +
reproducibility + regression. `make test` = 119 checks.

See `../research/RESOLVER_CONTEXT_GRID.md`, `RESOLVER_CROSS_MODEL_ANALYSIS.md`,
`PICKER_CALLER_CONTEXT.md` for the recovered structure and its evidence.
