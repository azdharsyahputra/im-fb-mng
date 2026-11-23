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
All 84 predicate type-names dispatched by FUN_009e5160 are resolved. Method: the
evaluator's full nested if/else chain was transpiled to an AST and evaluated once per
predicate type-name (all `GetTypeName()` calls return the same name pointer), so each
predicate is paired with the exact `.data` constant its matching path returns. The
inverted `if (p != X) { deeper } RET_X` structure is handled correctly (X's return is
the statement following the block's close brace). The 13 pre-confirmed anchors are a
subset and verify exactly. Weights are shared across groups of predicates (several
type-names map to the same `.data` float), which is why distinct names repeat an addr.

| predicate type | weight | .data addr |
|---|---:|---|
| GCMatchPredicateTeamGoals | 100 | 0x00ba1714 |
| GCMatchPredicateStrongDefence | 92 | 0x00bd5c48 |
| GCMatchPredicateStrongMidfield | 92 | 0x00bd5c48 |
| GCMatchPredicateVeryBadDefence | 92 | 0x00bd5c48 |
| GCMatchPredicateVeryBadMidfield | 92 | 0x00bd5c48 |
| GCMatchPredicateWeakDefence | 92 | 0x00bd5c48 |
| GCMatchPredicateWeakMidfield | 92 | 0x00bd5c48 |
| GCMatchPredicatePlayerHadABadMatchAsCentralDefender | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadABadMatchAsFullBack | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadABadMatchAsSweeper | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadABadMatchAsWingBack | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadAGoodMatchAsForward | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadAGoodMatchAsStriker | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadAGoodMatchAsWinger | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadBadDefensiveMatch | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadStrongAttackingMatch | 90 | 0x00b5b704 |
| GCMatchPredicatePlayerHadStrongDefensiveMatch | 90 | 0x00b5b704 |
| GCMatchPredicateTeamDoesManyQuickAttacks | 85 | 0x00b5baf4 |
| GCMatchPredicateDefensiveErrorsLeadsToShots | 70 | 0x00b73990 |
| GCMatchPredicateMakesAttackingPassesInBox | 70 | 0x00b73990 |
| GCMatchPredicateFailsManyDefensiveTackles | 65 | 0x00b5bb8c |
| GCMatchPredicateWinsHeadersCorner | 65 | 0x00b5bb8c |
| GCMatchPredicateWinsHeadersCross | 65 | 0x00b5bb8c |
| GCMatchPredicateWinsHeadersFreeKick | 65 | 0x00b5bb8c |
| GCMatchPredicateWinsManyDefensiveTackles | 65 | 0x00b5bb8c |
| GCMatchPredicatePlayerDoesNotGetTheBallAsAttackingCentralMidfielder | 60 | 0x00b5b1d8 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsAttackingFlankMidfielder | 60 | 0x00b5b1d8 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsForward | 60 | 0x00b5b1d8 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsStriker | 60 | 0x00b5b1d8 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsWinger | 60 | 0x00b5b1d8 |
| GCMatchPredicateSuccessfulCornersInBox | 60 | 0x00b5b1d8 |
| GCMatchPredicateDefensiveErrorsLeadsToGoals | 45 | 0x00ba170c |
| GCMatchPredicateFailsManyTackles | 45 | 0x00ba170c |
| GCMatchPredicateLongPassChain | 45 | 0x00ba170c |
| GCMatchPredicateLongRunGoal | 45 | 0x00ba170c |
| GCMatchPredicateLostTackleLeadsToGoal | 45 | 0x00ba170c |
| GCMatchPredicateMakesLongPassesIntoAttackingThird | 45 | 0x00ba170c |
| GCMatchPredicatePassInterceptionLeadsToGoal | 45 | 0x00ba170c |
| GCMatchPredicatePlayerDoesNotGetTheBallAsCentralMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerDoesNotGetTheBallAsDefensiveCentralMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsAttackingCentralMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsAttackingFlankMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsCentralDefender | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsCentralMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsDefensiveCentralMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsDefensiveFlankMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsFlankMidfielder | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsFullBack | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsSweeper | 45 | 0x00ba170c |
| GCMatchPredicatePlayerHadAGoodMatchAsWingBack | 45 | 0x00ba170c |
| GCMatchPredicatePlayerScoredAnOwnGoal | 45 | 0x00ba170c |
| GCMatchPredicateSuccessfulCrossesInBox | 45 | 0x00ba170c |
| GCMatchPredicateTeamChainsManyPasses | 45 | 0x00ba170c |
| GCMatchPredicateWinsDefensiveHeaders | 45 | 0x00ba170c |
| GCMatchPredicateWinsDefensiveHeadersInBox | 45 | 0x00ba170c |
| GCMatchPredicateWinsHeadersGoal | 45 | 0x00ba170c |
| GCMatchPredicateWinsManyTackles | 45 | 0x00ba170c |
| GCMatchPredicateCrossAfterLongRun | 40 | 0x00b5b6b4 |
| GCMatchPredicatePassIntoBoxAfterLongRun | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsAttackingCentralMidfielder | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsAttackingFlankMidfielder | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsCentralMidfielder | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsDefensiveCentralMidfielder | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsDefensiveFlankMidfielder | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsFlankMidfielder | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsForward | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsStriker | 40 | 0x00b5b6b4 |
| GCMatchPredicatePlayerHadABadMatchAsWinger | 40 | 0x00b5b6b4 |
| GCMatchPredicateShotAfterLongRun | 40 | 0x00b5b6b4 |
| GCMatchPredicateQuickAttack | 35 | 0x00b5baf8 |
| GCMatchPredicateWinsHeadersThrowIn | 35 | 0x00b5baf8 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsCentralDefender | 30 | 0x00b63280 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsDefensiveFlankMidfielder | 30 | 0x00b63280 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsFlankMidfielder | 30 | 0x00b63280 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsFullBack | 30 | 0x00b63280 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsSweeper | 30 | 0x00b63280 |
| GCMatchPredicatePlayerDoesNotGetTheBallAsWingBack | 30 | 0x00b63280 |
| GCMatchPredicateAttackBandComments | 20 | 0x00b5b600 |
| GCMatchPredicateDefenceBandComments | 20 | 0x00b5b600 |
| GCMatchPredicateMidfieldBandComments | 20 | 0x00b5b600 |
| GCMatchPredicateAttackingChancesInfluence | 10 | 0x00b6016c |
| GCMatchPredicateGoalChancesInfluence | 10 | 0x00b6016c |
| GCMatchPredicateTeamOffsides | 10 | 0x00b6016c |
| GCMatchPredicateWinsManyHeaders | 10 | 0x00b6016c |

Only predicates with weight >= 80 survive the c540 threshold: **TeamGoals (100),
the StrongDefence/StrongMidfield/VeryBad*/Weak* group (92), the PlayerHad*Match
group at 90, and TeamDoesManyQuickAttacks (85)**.

These values are implemented verbatim in `match/sim/predicate_weights.h` (84 rows,
sorted by weight descending).

## Pending (not invented)
None. All 84 predicate type-names dispatched by FUN_009e5160 are resolved to a
non-zero `.data` weight constant. None of the matching paths return a literal 0, and
no predicate was left ambiguous. Note: `GCMatchPredicateTeamGoals` additionally
returns 0 when `mode == 1`, but its real weight (mode != 1) is 100.

## Relationship to formula.xml
These RANKING weights are separate from the per-metric `*Performance` formula
coefficients (which are in the encrypted formula.xml.blowfish — key not recovered,
see BLOWFISH_DATA_RECOVERY.md). The ranking weights do NOT depend on that file.

## Classification
- Evaluator mechanism + 80.0 threshold linkage — **VERIFIED**
- all 84 predicate weight values — **VERIFIED** (13 anchors + 71 newly recovered)
- unresolved / zero-returning predicates — **NONE**
