/* Unit tests for the reconstructed data models. Verifies the recovered
 * structure compiles and the name<->index mappings round-trip. */
#include "../finances/income_categories.h"
#include "../players/attributes.h"
#include "../match/events.h"
#include "../match/predicates.h"
#include "../tactics/config.h"
#include "../tactics/team_tactics.h"
#include "../staff/person_schema.h"
#include "../competitions/competition_schema.h"
#include "../clubs/club_schema.h"
#include "../match/match_schema.h"
#include "../tactics/squad_tactics_schema.h"
#include "../players/player_schema.h"
#include "../competitions/season_schema.h"
#include "../match/match_record_schema.h"
#include "../competitions/standings_schema.h"
#include "../competitions/player_season_stats_schema.h"
#include "../competitions/competition_def_schema.h"
#include "../scouting/scout_report_schema.h"
#include "../tactics/position_tactics_schema.h"
#include "../staff/assistant_delegation_schema.h"
#include "../players/person_name_schema.h"
#include "../clubs/club_finance_schema.h"
#include "../database/table_counts.h"
#include <stdio.h>
#include <string.h>

int mock_get(void *ctx, const char *name, float *out) {
    (void)ctx;
    if (strcmp(name, "MarkingDistanceScale") == 0) { *out = 0.5f; return 1; }
    return 0;
}

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL: %s (line %d)\n", #cond, __LINE__); fails++; } } while (0)

int main(void) {
    /* Finance income taxonomy — Observed order from FUN_00907f50 */
    CHECK(CM_INCOME_CATEGORY_COUNT == 10);
    CHECK(CM_INCOME_PRIZE_MONEY == 5);
    CHECK(strcmp(cm_income_name(CM_INCOME_GATE_RECEIPTS), "GateReceipts") == 0);
    CHECK(cm_income_from_name("Sponsorship") == CM_INCOME_SPONSORSHIP);
    CHECK(cm_income_from_name("NopeNotReal") == -1);

    /* Player attributes */
    CHECK(CM_AT_COUNT == 53);
    CHECK(cm_attribute_from_name("Finishing") == CM_AT_FINISHING);
    CHECK(cm_attribute_from_name(cm_attribute_name(CM_AT_PACE)) == CM_AT_PACE);
    CHECK(cm_attribute_effective(14, 2) == 16); /* INFERRED combine */

    /* Match events (axioms) */
    CHECK(CM_AX_COUNT == 34);
    CHECK(strcmp(cm_axiom_name(CM_AX_GOAL), "Goal") == 0);
    CHECK(strcmp(cm_axiom_name(CM_AX_RED_CARD), "RedCard") == 0);

    /* every name table entry is non-null and non-empty */
    for (int i = 0; i < CM_AT_COUNT; i++) CHECK(cm_attribute_name(i) && cm_attribute_name(i)[0]);
    for (int i = 0; i < CM_AX_COUNT; i++) CHECK(cm_axiom_name(i) && cm_axiom_name(i)[0]);

    /* Match predicate evaluator — the recovered recursive tree mechanism */
    cm_match_stats st = {0};
    st.passes = 40; st.passes_ok = 34; st.tackles = 6; st.tackles_ok = 5;
    st.goals = 1; st.possession_portion = 58;

    cm_predicate_node p_pass = { CM_PRED_SUCCESSFUL_PASSES, 30, NULL, 0 };
    cm_predicate_node p_goal = { CM_PRED_PLAYER_SCORED_A_GOAL, 1, NULL, 0 };
    cm_predicate_node p_poss = { CM_PRED_TEAM_POSSESSION_PORTION, 60, NULL, 0 };
    CHECK(cm_predicate_eval(&p_pass, &st) == 1); /* 34 >= 30 */
    CHECK(cm_predicate_eval(&p_goal, &st) == 1); /* scored */
    CHECK(cm_predicate_eval(&p_poss, &st) == 0); /* 58 < 60 */

    /* composite: (SuccessfulPasses>=30) AND (ScoredGoal) -> true */
    cm_predicate_node *kids_and[2] = { &p_pass, &p_goal };
    cm_predicate_node p_and = { CM_PRED_AND, 0, kids_and, 2 };
    CHECK(cm_predicate_eval(&p_and, &st) == 1);
    /* NOT(possession>=60) -> true */
    cm_predicate_node *kids_not[1] = { &p_poss };
    cm_predicate_node p_not = { CM_PRED_NOT, 0, kids_not, 1 };
    CHECK(cm_predicate_eval(&p_not, &st) == 1);
    /* OR with both false -> false */
    cm_predicate_node *kids_or[2] = { &p_poss, &p_poss };
    cm_predicate_node p_or = { CM_PRED_OR, 0, kids_or, 2 };
    CHECK(cm_predicate_eval(&p_or, &st) == 0);
    CHECK(strcmp(cm_predicate_name(CM_PRED_SUCCESSFUL_TACKLE), "SuccessfulTackle") == 0);

    /* Tactics getter reconstruction (FUN_0083ea10): default 0.02 when absent */
    CHECK(cm_marking_distance_scale(NULL) == 0.02f);
    { /* present in data -> returns the data value */
        cm_data_source ds = { mock_get, NULL };
        CHECK(cm_marking_distance_scale(&ds) == 0.5f);
    }

    /* Team tactics schema (FUN_00a76870) */
    CHECK(CM_TT_FIELD_COUNT == 14);
    CHECK(cm_team_tactic_from_name("ClosingDown") == CM_TT_CLOSING_DOWN);
    CHECK(cm_team_tactic_from_name("OffsideTrapDepth") == CM_TT_OFFSIDE_TRAP_DEPTH);
    { cm_team_tactics tt; cm_team_tactics_init(&tt);
      CHECK(tt.field[CM_TT_MENTALITY] == CM_TT_UNSET); }

    /* Staff/manager person schema (FUN_00a84df0) */
    CHECK(CM_PF_FIELD_COUNT == 40);
    CHECK(CM_PF_PERSON_ID == 0);
    CHECK(cm_person_field_from_name("Physiotherapy") == CM_PF_PHYSIOTHERAPY);
    CHECK(cm_person_field_from_name("Sportmanship") == CM_PF_SPORTMANSHIP); /* original typo */
    CHECK(cm_person_field_from_name("CurrentAbility") == CM_PF_CURRENT_ABILITY);

    /* Competition record schema (FUN_00a61c90) */
    CHECK(CM_COMP_FIELD_COUNT == 49);
    CHECK(cm_comp_field_from_name("TransferWindowID") >= 0);
    CHECK(cm_comp_field_from_name("MaxSquadSize") >= 0);
    CHECK(cm_comp_field_from_name("Nonexistent") == -1);

    /* Generated schemas — counts + a representative lookup each */
    CHECK(CM_CLUB_FIELD_COUNT == 111);
    CHECK(cm_club_schema_from_name("TransferBudget") >= 0);
    CHECK(cm_club_schema_from_name("BankBalance") >= 0);
    CHECK(CM_MATCH_FIELD_COUNT == 32);
    CHECK(CM_SQT_FIELD_COUNT == 50);
    CHECK(cm_squad_tactics_schema_from_name("FreeKick") >= 0);
    CHECK(CM_PLAYER_FIELD_COUNT == 39);
    CHECK(cm_player_schema_from_name("AskingPrice") >= 0);
    CHECK(CM_SEASON_FIELD_COUNT == 52);
    CHECK(cm_club_schema_from_name("ZzzNope") == -1);

    /* Batch 2 schemas — count sanity + a lookup each */
    CHECK(CM_MREC_FIELD_COUNT == 31);
    CHECK(cm_match_record_schema_from_name("MatchSeed") >= 0);
    CHECK(CM_STAND_FIELD_COUNT == 30);
    CHECK(cm_standings_schema_from_name("BonusPoints") >= 0);
    CHECK(CM_PSTAT_FIELD_COUNT == 29);
    CHECK(cm_player_season_stats_schema_from_name("CumulativeRating") >= 0);
    CHECK(CM_CDEF_FIELD_COUNT == 27);
    CHECK(CM_SCOUT_FIELD_COUNT == 22);
    CHECK(CM_PTAC_FIELD_COUNT == 21);
    CHECK(cm_position_tactics_schema_from_name("Marking") >= 0);
    CHECK(CM_ASST_FIELD_COUNT == 31);
    CHECK(CM_PNAME_FIELD_COUNT == 28);
    CHECK(cm_person_name_schema_from_name("DB_Year") >= 0);

    CHECK(CM_CFIN_FIELD_COUNT == 29);
    CHECK(cm_club_finance_schema_from_name("BankBalance") >= 0);
    /* Verified DB record counts (extracted .dat tables) */
    CHECK(CM_DB_PLAYER_COUNT == 85035);
    CHECK(CM_DB_CLUB_COUNT == 6571);
    CHECK(CM_DB_PERSON_COUNT == 102990);

    if (fails == 0) { printf("ALL TESTS PASSED\n"); return 0; }
    printf("%d TEST(S) FAILED\n", fails); return 1;
}
