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

Data dir: `--dir PATH` or `$RESOLVER_DIR` (default: the canonical extracted path).

## Tests

    make test

## Status by phase
- Phase 1 (models load / inspect / compare + invariant tests): **done**.
- Phases 2–4 (runtime state + context engine + sampler + trace; experiments +
  differential stats; reporting + reproducibility): in progress.

See `../research/RESOLVER_CONTEXT_GRID.md`, `RESOLVER_CROSS_MODEL_ANALYSIS.md`,
`PICKER_CALLER_CONTEXT.md` for the recovered structure and its evidence.
