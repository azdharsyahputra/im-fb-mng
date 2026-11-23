/*
 * Match predicate evaluator — reconstructed MECHANISM.
 *
 * The recursive tree-walk mirrors the Observed decompilation (FUN_009e9200:
 * process node then recurse over the child array; FUN_009e92b0: nodes carry a
 * child list). Composite operators (AND/OR/NOT) and leaf threshold comparisons
 * are INFERRED — the original evaluates data-driven formulae via GCFormulaModule,
 * whose coefficients ship in the data cabs, not the exe. Treat leaf thresholds
 * as placeholders, not verified values.
 */
#include "predicates.h"
#include <stddef.h>

static const char *const NAMES[CM_PRED_TYPE_COUNT] = {
    "AND","OR","NOT",
    "SuccessfulPasses","UnsuccessfulPass",
    "SuccessfulTackle","UnsuccessfulTackle",
    "SuccessfulHeader","UnsuccessfulHeader",
    "SuccessfulCrosses","UnsuccessfulCross",
    "WinsDefensiveHeader","PassInterception",
    "PlayerScoredAGoal","PlayerScoredAnOwnGoal",
    "PlayerHadAGoodMatch","PlayerHadABadMatch",
    "TeamPossessionPortion"
};

const char *cm_predicate_name(cm_predicate_type t) {
    if (t < 0 || t >= CM_PRED_TYPE_COUNT) return NULL;
    return NAMES[t];
}

/* Leaf evaluation: map the Observed predicate name to the stat it reads.
 * The >= threshold comparison is the INFERRED part. */
static int eval_leaf(const cm_predicate_node *n, const cm_match_stats *s) {
    int th = n->threshold;
    switch (n->type) {
    case CM_PRED_SUCCESSFUL_PASSES:       return s->passes_ok            >= th;
    case CM_PRED_UNSUCCESSFUL_PASS:       return (s->passes - s->passes_ok)       >= th;
    case CM_PRED_SUCCESSFUL_TACKLE:       return s->tackles_ok           >= th;
    case CM_PRED_UNSUCCESSFUL_TACKLE:     return (s->tackles - s->tackles_ok)     >= th;
    case CM_PRED_SUCCESSFUL_HEADER:       return s->headers_ok           >= th;
    case CM_PRED_UNSUCCESSFUL_HEADER:     return (s->headers - s->headers_ok)     >= th;
    case CM_PRED_SUCCESSFUL_CROSSES:      return s->crosses_ok           >= th;
    case CM_PRED_UNSUCCESSFUL_CROSS:      return (s->crosses - s->crosses_ok)     >= th;
    case CM_PRED_WINS_DEFENSIVE_HEADER:   return s->headers_ok           >= th;
    case CM_PRED_PASS_INTERCEPTION:       return s->interceptions        >= th;
    case CM_PRED_PLAYER_SCORED_A_GOAL:    return s->goals                >= (th > 0 ? th : 1);
    case CM_PRED_PLAYER_SCORED_OWN_GOAL:  return s->own_goals            >= (th > 0 ? th : 1);
    case CM_PRED_TEAM_POSSESSION_PORTION: return s->possession_portion   >= th;
    /* Aggregate verdicts — the original weights many inputs (INFERRED here). */
    case CM_PRED_PLAYER_HAD_A_GOOD_MATCH:
        return (s->goals*3 + s->passes_ok + s->tackles_ok + s->headers_ok) >= th;
    case CM_PRED_PLAYER_HAD_A_BAD_MATCH:
        return (s->own_goals*3 + (s->passes - s->passes_ok)
                + (s->tackles - s->tackles_ok) + s->red_cards*2) >= th;
    default: return 0;
    }
}

int cm_predicate_eval(const cm_predicate_node *node, const cm_match_stats *s) {
    if (!node || !s) return 0;
    switch (node->type) {
    case CM_PRED_AND:
        for (int i = 0; i < node->child_count; i++)
            if (!cm_predicate_eval(node->children[i], s)) return 0;
        return 1;
    case CM_PRED_OR:
        for (int i = 0; i < node->child_count; i++)
            if (cm_predicate_eval(node->children[i], s)) return 1;
        return 0;
    case CM_PRED_NOT:
        return node->child_count == 1 ? !cm_predicate_eval(node->children[0], s) : 0;
    default:
        return eval_leaf(node, s);
    }
}
