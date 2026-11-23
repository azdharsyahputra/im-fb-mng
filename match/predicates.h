/*
 * Match predicate system — reconstructed MECHANISM from CM2008.exe.
 *
 * Observed (decompiled FUN_009e8f60, FUN_009e92b0, FUN_009e9200): the match
 * engine's judgment layer is an EXPRESSION TREE of GCMatchPredicate nodes held
 * in child containers and evaluated recursively (a node is processed, then its
 * children are iterated/recursed). Predicates are derived from the atomic match
 * axioms (events) tallied per player/team. See docs/findings/match-engine.md and
 * docs/findings/match-predicates.md.
 *
 * What is Observed: the predicate TAXONOMY (names) and the tree/recursion
 * MECHANISM. What is INFERRED (and marked so): the exact leaf computations,
 * composite operators, and any thresholds — those are data-driven
 * (GCFormulaModule / XML) and not recovered from the exe.
 */
#ifndef CM2008_MATCH_PREDICATES_H
#define CM2008_MATCH_PREDICATES_H

/* Per-player/team axiom tallies a leaf predicate reads (from the axiom set). */
typedef struct cm_match_stats {
    int passes, passes_ok;
    int tackles, tackles_ok;
    int headers, headers_ok;
    int shots, shots_on_target;
    int crosses, crosses_ok;
    int clearances, interceptions;
    int fouls, yellow_cards, red_cards, offsides;
    int goals, own_goals;
    int possession_portion;   /* team possession, 0..100 */
} cm_match_stats;

/* Predicate node types. Leaf = reads stats; composite = combines children.
 * Leaf names are Observed (GCMatchPredicate* taxonomy). */
typedef enum cm_predicate_type {
    /* composites (operators — INFERRED set) */
    CM_PRED_AND, CM_PRED_OR, CM_PRED_NOT,
    /* leaves (Observed names) */
    CM_PRED_SUCCESSFUL_PASSES, CM_PRED_UNSUCCESSFUL_PASS,
    CM_PRED_SUCCESSFUL_TACKLE,  CM_PRED_UNSUCCESSFUL_TACKLE,
    CM_PRED_SUCCESSFUL_HEADER,  CM_PRED_UNSUCCESSFUL_HEADER,
    CM_PRED_SUCCESSFUL_CROSSES, CM_PRED_UNSUCCESSFUL_CROSS,
    CM_PRED_WINS_DEFENSIVE_HEADER,
    CM_PRED_PASS_INTERCEPTION,
    CM_PRED_PLAYER_SCORED_A_GOAL, CM_PRED_PLAYER_SCORED_OWN_GOAL,
    CM_PRED_PLAYER_HAD_A_GOOD_MATCH, CM_PRED_PLAYER_HAD_A_BAD_MATCH,
    CM_PRED_TEAM_POSSESSION_PORTION,
    CM_PRED_TYPE_COUNT
} cm_predicate_type;

/* Expression-tree node. `threshold` is used by leaf predicates (INFERRED). */
typedef struct cm_predicate_node {
    cm_predicate_type type;
    int threshold;                        /* INFERRED (data-driven in original) */
    struct cm_predicate_node **children;  /* composite children */
    int child_count;
} cm_predicate_node;

/* Recursive evaluation — the Observed tree-walk mechanism. Returns 0/1. */
int cm_predicate_eval(const cm_predicate_node *node, const cm_match_stats *s);

const char *cm_predicate_name(cm_predicate_type t);

#endif /* CM2008_MATCH_PREDICATES_H */
