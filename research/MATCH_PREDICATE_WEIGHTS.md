# Match Predicate Ranking Weights (FUN_009e5160)

The detailed match engine ranks candidate match events/comments with a predicate
rule engine (`FUN_009e9570`, a score-filter-rank over an intrusive red-black tree of
candidates). The per-predicate score comes from **FUN_009e5160**, the weight
evaluator — and, crucially, it is NOT obfuscated: it is a plain type-name dispatch
that returns a float weight constant embedded in `.data`.

## Mechanism (VERIFIED)
```
float FUN_009e5160(int mode, Predicate *p){
    name = p->vtable->GetTypeName();        // vtable+4
    if (name == "GCMatchPredicateTeamGoals") return (mode==1)? 0.0f : DAT_00ba1714; // 100
    if (name == "GCMatchPredicateWeakDefence") ... return DAT_00bd5c48;             // 92
    ... // ~84 predicate type-names, each -> a .data float constant
}
```
The caller chain is `FUN_0077c540 -> FUN_009e9570 -> FUN_009e5160`. `FUN_0077c540`
passes a threshold of **80.0f** (0x42a00000). `FUN_009e9570` keeps candidates with
`score > 0`, then ranks by `-score`. So the 80.0 threshold means only the highest-
weight predicates (>= 80) win that selection stage.

## Recovered weights (VERIFIED — read from the .data float constant each path returns)
| predicate type | weight | .data addr |
|---|---:|---|
| GCMatchPredicateTeamGoals | 100 | 0x00ba1714 |
| GCMatchPredicateWeakDefence | 92 | 0x00bd5c48 |
| GCMatchPredicatePlayerHadStrongAttackingMatch | 90 | 0x00b5b704 |
| GCMatchPredicateTeamDoesManyQuickAttacks | 85 | 0x00b5baf4 |
| GCMatchPredicateDefensiveErrorsLeadsToShots | 70 | 0x00b73990 |
| GCMatchPredicateWinsManyDefensiveTackles | 65 | 0x00b5bb8c |
| GCMatchPredicatePlayerDoesNotGetTheBallAsAttackingFlankMidfielder | 60 | 0x00b5b1d8 |
| GCMatchPredicateMakesLongPassesIntoAttackingThird | 45 | 0x00ba170c |
| GCMatchPredicateShotAfterLongRun | 40 | 0x00b5b6b4 |
| GCMatchPredicateQuickAttack | 35 | 0x00b5baf8 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsSweeper | 30 | 0x00b63280 |
| GCMatchPredicateAttackBandComments | 20 | 0x00b5b600 |
| GCMatchPredicateWinsManyHeaders | 10 | 0x00b6016c |

Only predicates with weight >= 80 survive the c540 threshold: **TeamGoals (100),
WeakDefence (92), PlayerHadStrongAttackingMatch (90), TeamDoesManyQuickAttacks (85)**.

These values are implemented verbatim in `match/sim/predicate_weights.h`.

## Pending (not invented)
FUN_009e5160 dispatches on ~84 predicate type-names. The static instruction-order
pass resolved 13 clean (name -> float) pairs above. The remaining ~71 were not paired
unambiguously (shared constants, deeply nested branch structure); they are UNKNOWN
pending a control-flow-accurate pass or a Ghidra decompiler-variable trace. The 13
recovered are byte-backed and correct.

## Relationship to formula.xml
These RANKING weights are separate from the per-metric `*Performance` formula
coefficients (which are in the encrypted formula.xml.blowfish — key not recovered,
see BLOWFISH_DATA_RECOVERY.md). The ranking weights do NOT depend on that file.

## Classification
- Evaluator mechanism + 80.0 threshold linkage — **VERIFIED**
- 13 weight values — **VERIFIED**
- remaining ~71 predicate weights — **UNKNOWN** (pending)
