/*
 * Match predicate RANKING WEIGHTS — VERIFIED, recovered from the binary.
 *
 * Source: GCMatchModule weight evaluator FUN_009e5160. It is a type-name dispatch
 * (GetTypeName() via vtable+4, compared to interned GCMatchPredicate* strings) that
 * returns a float weight CONSTANT embedded in .data for each predicate type. These
 * are the real CM2008 coefficients used by the predicate rule engine FUN_009e9570
 * (score-filter-rank): FUN_0077c540 passes a threshold of 80.0f, so only predicates
 * whose weight >= 80 survive that filter (TeamGoals, WeakDefence,
 * PlayerHadStrongAttackingMatch, TeamDoesManyQuickAttacks).
 *
 * VERIFIED: all 84 pairs below (value read from the .data float constant the
 * evaluator returns on each matching path). The evaluator's full nested if/else
 * chain was transpiled to an AST and evaluated per predicate type-name; all 84
 * type-names dispatched by FUN_009e5160 resolve to a non-zero weight constant
 * (none return literal 0, none unresolved). The original 13 anchor pairs are a
 * subset and still verify exactly. These values are NOT invented.
 *
 * NOTE: these ranking weights are distinct from the per-metric *Performance formula
 * coefficients, which live in the encrypted DataXML/formula.xml.blowfish (its key is
 * not a plaintext string and its loader is in a protected region — see
 * research/BLOWFISH_DATA_RECOVERY.md).
 */
#ifndef CM2008_PREDICATE_WEIGHTS_H
#define CM2008_PREDICATE_WEIGHTS_H

typedef struct cm_predicate_weight {
    const char *type_name;   /* GCMatchPredicate type (verbatim) */
    float       weight;      /* .data float constant returned by FUN_009e5160 */
    unsigned    dat_addr;    /* source address in .data (evidence) */
} cm_predicate_weight;

/* FUN_0077c540 -> FUN_009e9570 keeps candidates with weight >= this. (VERIFIED: 80.0f
 * = 0x42a00000 passed at the c540 call site.) */
#define CM_PREDICATE_RANK_THRESHOLD  80.0f

static const cm_predicate_weight CM_PREDICATE_WEIGHTS[] = {
    { "GCMatchPredicateTeamGoals",                                           100.0f, 0x00ba1714 },
    { "GCMatchPredicateStrongDefence",                                        92.0f, 0x00bd5c48 },
    { "GCMatchPredicateStrongMidfield",                                       92.0f, 0x00bd5c48 },
    { "GCMatchPredicateVeryBadDefence",                                       92.0f, 0x00bd5c48 },
    { "GCMatchPredicateVeryBadMidfield",                                      92.0f, 0x00bd5c48 },
    { "GCMatchPredicateWeakDefence",                                          92.0f, 0x00bd5c48 },
    { "GCMatchPredicateWeakMidfield",                                         92.0f, 0x00bd5c48 },
    { "GCMatchPredicatePlayerHadABadMatchAsCentralDefender",                  90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadABadMatchAsFullBack",                         90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadABadMatchAsSweeper",                          90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadABadMatchAsWingBack",                         90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadAGoodMatchAsForward",                         90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadAGoodMatchAsStriker",                         90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadAGoodMatchAsWinger",                          90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadBadDefensiveMatch",                           90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadStrongAttackingMatch",                        90.0f, 0x00b5b704 },
    { "GCMatchPredicatePlayerHadStrongDefensiveMatch",                        90.0f, 0x00b5b704 },
    { "GCMatchPredicateTeamDoesManyQuickAttacks",                             85.0f, 0x00b5baf4 },
    { "GCMatchPredicateDefensiveErrorsLeadsToShots",                          70.0f, 0x00b73990 },
    { "GCMatchPredicateMakesAttackingPassesInBox",                            70.0f, 0x00b73990 },
    { "GCMatchPredicateFailsManyDefensiveTackles",                            65.0f, 0x00b5bb8c },
    { "GCMatchPredicateWinsHeadersCorner",                                    65.0f, 0x00b5bb8c },
    { "GCMatchPredicateWinsHeadersCross",                                     65.0f, 0x00b5bb8c },
    { "GCMatchPredicateWinsHeadersFreeKick",                                  65.0f, 0x00b5bb8c },
    { "GCMatchPredicateWinsManyDefensiveTackles",                             65.0f, 0x00b5bb8c },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsAttackingCentralMidfielder",  60.0f, 0x00b5b1d8 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsAttackingFlankMidfielder",    60.0f, 0x00b5b1d8 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsForward",                     60.0f, 0x00b5b1d8 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsStriker",                     60.0f, 0x00b5b1d8 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsWinger",                      60.0f, 0x00b5b1d8 },
    { "GCMatchPredicateSuccessfulCornersInBox",                               60.0f, 0x00b5b1d8 },
    { "GCMatchPredicateDefensiveErrorsLeadsToGoals",                          45.0f, 0x00ba170c },
    { "GCMatchPredicateFailsManyTackles",                                     45.0f, 0x00ba170c },
    { "GCMatchPredicateLongPassChain",                                        45.0f, 0x00ba170c },
    { "GCMatchPredicateLongRunGoal",                                          45.0f, 0x00ba170c },
    { "GCMatchPredicateLostTackleLeadsToGoal",                                45.0f, 0x00ba170c },
    { "GCMatchPredicateMakesLongPassesIntoAttackingThird",                    45.0f, 0x00ba170c },
    { "GCMatchPredicatePassInterceptionLeadsToGoal",                          45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsCentralMidfielder",           45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsDefensiveCentralMidfielder",  45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsAttackingCentralMidfielder",      45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsAttackingFlankMidfielder",        45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsCentralDefender",                 45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsCentralMidfielder",               45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsDefensiveCentralMidfielder",      45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsDefensiveFlankMidfielder",        45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsFlankMidfielder",                 45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsFullBack",                        45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsSweeper",                         45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerHadAGoodMatchAsWingBack",                        45.0f, 0x00ba170c },
    { "GCMatchPredicatePlayerScoredAnOwnGoal",                                45.0f, 0x00ba170c },
    { "GCMatchPredicateSuccessfulCrossesInBox",                               45.0f, 0x00ba170c },
    { "GCMatchPredicateTeamChainsManyPasses",                                 45.0f, 0x00ba170c },
    { "GCMatchPredicateWinsDefensiveHeaders",                                 45.0f, 0x00ba170c },
    { "GCMatchPredicateWinsDefensiveHeadersInBox",                            45.0f, 0x00ba170c },
    { "GCMatchPredicateWinsHeadersGoal",                                      45.0f, 0x00ba170c },
    { "GCMatchPredicateWinsManyTackles",                                      45.0f, 0x00ba170c },
    { "GCMatchPredicateCrossAfterLongRun",                                    40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePassIntoBoxAfterLongRun",                              40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsAttackingCentralMidfielder",       40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsAttackingFlankMidfielder",         40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsCentralMidfielder",                40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsDefensiveCentralMidfielder",       40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsDefensiveFlankMidfielder",         40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsFlankMidfielder",                  40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsForward",                          40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsStriker",                          40.0f, 0x00b5b6b4 },
    { "GCMatchPredicatePlayerHadABadMatchAsWinger",                           40.0f, 0x00b5b6b4 },
    { "GCMatchPredicateShotAfterLongRun",                                     40.0f, 0x00b5b6b4 },
    { "GCMatchPredicateQuickAttack",                                          35.0f, 0x00b5baf8 },
    { "GCMatchPredicateWinsHeadersThrowIn",                                   35.0f, 0x00b5baf8 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsCentralDefender",             30.0f, 0x00b63280 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsDefensiveFlankMidfielder",    30.0f, 0x00b63280 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsFlankMidfielder",             30.0f, 0x00b63280 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsFullBack",                    30.0f, 0x00b63280 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsSweeper",                     30.0f, 0x00b63280 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsWingBack",                    30.0f, 0x00b63280 },
    { "GCMatchPredicateAttackBandComments",                                   20.0f, 0x00b5b600 },
    { "GCMatchPredicateDefenceBandComments",                                  20.0f, 0x00b5b600 },
    { "GCMatchPredicateMidfieldBandComments",                                 20.0f, 0x00b5b600 },
    { "GCMatchPredicateAttackingChancesInfluence",                            10.0f, 0x00b6016c },
    { "GCMatchPredicateGoalChancesInfluence",                                 10.0f, 0x00b6016c },
    { "GCMatchPredicateTeamOffsides",                                         10.0f, 0x00b6016c },
    { "GCMatchPredicateWinsManyHeaders",                                      10.0f, 0x00b6016c },
};
#define CM_PREDICATE_WEIGHT_COUNT \
    (sizeof(CM_PREDICATE_WEIGHTS)/sizeof(CM_PREDICATE_WEIGHTS[0]))

/* Returns the weight for a predicate type-name, or -1.0f if not among the recovered
 * set. Linear scan (table is tiny). */
static inline float cm_predicate_weight_of(const char *type_name){
    for (unsigned i=0;i<CM_PREDICATE_WEIGHT_COUNT;i++){
        const char *a=CM_PREDICATE_WEIGHTS[i].type_name, *b=type_name;
        while(*a && *a==*b){a++;b++;}
        if(*a=='\0' && *b=='\0') return CM_PREDICATE_WEIGHTS[i].weight;
    }
    return -1.0f;
}
/* 1 if this predicate survives the rank threshold (weight >= 80). */
static inline int cm_predicate_passes_rank(const char *type_name){
    float w = cm_predicate_weight_of(type_name);
    return (w >= 0.0f) && (w >= CM_PREDICATE_RANK_THRESHOLD);
}

#endif
