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
 * VERIFIED: the 13 pairs below (value read from the .data float constant the
 * evaluator returns on each matching path). The evaluator covers ~84 predicate
 * type-names total; the remaining pairings were not resolved cleanly by the static
 * instruction-order pass (shared constants / deeply nested branches) and are tracked
 * as pending in research/MATCH_PREDICATE_WEIGHTS.md. These values are NOT invented.
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
    { "GCMatchPredicateTeamGoals",                                   100.0f, 0x00ba1714 },
    { "GCMatchPredicateWeakDefence",                                  92.0f, 0x00bd5c48 },
    { "GCMatchPredicatePlayerHadStrongAttackingMatch",               90.0f, 0x00b5b704 },
    { "GCMatchPredicateTeamDoesManyQuickAttacks",                     85.0f, 0x00b5baf4 },
    { "GCMatchPredicateDefensiveErrorsLeadsToShots",                  70.0f, 0x00b73990 },
    { "GCMatchPredicateWinsManyDefensiveTackles",                     65.0f, 0x00b5bb8c },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsAttackingFlankMidfielder", 60.0f, 0x00b5b1d8 },
    { "GCMatchPredicateMakesLongPassesIntoAttackingThird",           45.0f, 0x00ba170c },
    { "GCMatchPredicateShotAfterLongRun",                            40.0f, 0x00b5b6b4 },
    { "GCMatchPredicateQuickAttack",                                 35.0f, 0x00b5baf8 },
    { "GCMatchPredicatePlayerDoesNotGetTheBallAsSweeper",            30.0f, 0x00b63280 },
    { "GCMatchPredicateAttackBandComments",                          20.0f, 0x00b5b600 },
    { "GCMatchPredicateWinsManyHeaders",                             10.0f, 0x00b6016c },
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
