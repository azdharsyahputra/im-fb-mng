# Finding: Match predicate evaluation (the match AI judgment layer)

**Status:** reverse-engineered (mechanism) · **Confidence:** mechanism Observed, leaf semantics Inferred
**Evidence:** clean decompilation `docs/ghidra_dumps/raw/match_engine.c` — FUN_009e8f60
(seed, 656b), FUN_009e92b0 (node constructor), FUN_009e9200 (recursive walk).

## What was recovered (Observed mechanism)

CM2008 judges match performance with an **expression tree of GCMatchPredicate
nodes**, evaluated recursively:

- `FUN_009e92b0` constructs a predicate node (`operator_new(0x50)`) and appends
  children to a vector (`param_1[0x12]/[0x13]` push-back) — i.e. nodes own a
  child list.
- `FUN_009e9200` is the recursive walk: it processes a node, then iterates a
  child pointer array (`ptr += 4`) and **recurses into itself** per child.
- `FUN_009e8f60` iterates a vector of 0x20-byte node records (`(end-begin)>>5`,
  `index*0x20`) through a custom small-object free-list allocator
  (`DAT_00c65ba0`).

So the judgment layer is a composable predicate DAG over the atomic match
**axioms** (events). This mechanism is reconstructed faithfully in
`match/predicate_eval.c` (recursive AND/OR/NOT + leaf dispatch).

## What is NOT recovered (Inferred / data-driven)

- The exact **leaf computations and thresholds**: the original resolves these via
  `GCFormulaModule`, whose coefficients ship in the data cabs (`data2/3.cab`),
  not the exe. The reconstructed leaves use the obvious stat each name implies
  with a placeholder `>=` threshold — marked INFERRED in code.
- The precise **composite operator set** the original uses (AND/OR/NOT is the
  reconstructed minimal set; the original tree may carry weighted/aggregating
  nodes).
- The full ~90-predicate taxonomy is Observed by name (symbol dump); the module
  implements the clearly-named subset and is extended as each is understood.

## Reconstructed artifacts

- `match/predicates.h`, `match/predicate_eval.c` — tree model + recursive evaluator
- tests in `tests/test_reconstruction.c` (leaf + AND/OR/NOT composites) — passing.
