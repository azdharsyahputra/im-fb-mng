/*
 * Standalone test for the ResolverSettings match-constants model + its harvest.
 *
 * Uses a SYNTHETIC cm_formula_registry (made-up values, NOT recovered game data) to
 * prove: (1) fill() stores each resolved value at the right byte offset; (2) by-name
 * and by-offset accessors agree; (3) formulas with no body + no default (compute-only)
 * and names absent from the registry are left at the sentinel and COUNTED, not invented.
 */
#include "../match/model/resolver_settings.h"
#include "../match/formula/formula.h"
#include <stdio.h>
#include <math.h>

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", (msg)); failures++; } \
    else         { printf("  ok:   %s\n", (msg)); } \
} while (0)

static int feq(float a, float b) { return fabsf(a - b) < 1e-6f; }

/* --- Synthetic registry: arbitrary values, two compute-only (no default) rows. --- */
static const cm_formula_def SYN_DEFS[] = {
    /* name, group, body, default, has_default */
    { "MaxSprintingSpeed",   "MatchResolver", 0, 7.5f,  1 },  /* -> 0x128 */
    { "MinSprintingSpeed",   "MatchResolver", 0, 6.0f,  1 },  /* -> 0x124 */
    { "KeyPassesPerformance","PlayerData",    0, 0.25f, 1 },  /* -> 0x530 */
    { "PerformanceBalancing","PlayerData",    0, 1.0f,  1 },  /* -> 0x5d4 */
    { "BallElasticity",      "ResolverConstants", 0, 0.0f, 0 },/* compute-only: unresolved */
    /* per-player (group PlayerConstant) */
    { "Fitness",             "PlayerConstant", 0, 95.0f, 1 },
    { "BallControl",         "PlayerConstant", 0, 88.0f, 1 },
    { "KeeperReach",         "PlayerConstant", 0, 0.0f,  0 }, /* compute-only: unresolved */
};
static const cm_formula_registry SYN_REG = { SYN_DEFS, sizeof(SYN_DEFS)/sizeof(SYN_DEFS[0]) };

int main(void) {
    cm_resolver_settings rs;
    cm_player_constants  pc;
    size_t n_fields = 0, unresolved = 0, resolved = 0;
    int found = 0, ok = 0;

    printf("== ResolverSettings model + harvest ==\n");

    (void)cm_rs_field_table(&n_fields);
    printf("  (modelled harvest slots: %zu)\n", n_fields);
    CHECK(n_fields > 100, "field table covers movement+ball+performance regions (>100)");

    /* Offset facts are wired correctly. */
    CHECK(cm_rs_offset_of("MaxSprintingSpeed", &ok) == 0x128u && ok, "MaxSprintingSpeed @ 0x128");
    CHECK(cm_rs_offset_of("MinSprintingSpeed", &ok) == 0x124u && ok, "MinSprintingSpeed @ 0x124");
    CHECK(cm_rs_offset_of("BaseBallSpeedGentle", &ok) == 0x28u && ok, "BaseBallSpeedGentle @ 0x28");
    CHECK(cm_rs_offset_of("KeeperDefenderGoalPerformance", &ok) == 0x53cu && ok,
          "KeeperDefenderGoalPerformance @ 0x53c");
    CHECK(cm_rs_offset_of("PerformanceBalancing", &ok) == 0x5d4u && ok, "PerformanceBalancing @ 0x5d4");
    CHECK(cm_rs_offset_of("NoSuchField", &ok) == 0 && !ok, "unknown name reported, not invented");

    /* Harvest from the synthetic registry. */
    resolved = cm_resolver_settings_fill(&rs, &SYN_REG, NULL, &unresolved);
    printf("  (resolved=%zu unresolved=%zu of %zu)\n", resolved, unresolved, n_fields);

    /* Exactly the 4 registered-with-default ResolverSettings rows resolve. */
    CHECK(resolved == 4, "exactly 4 slots resolved from synthetic registry");
    CHECK(resolved + unresolved == n_fields, "resolved + unresolved == total fields");

    /* Values landed at the correct byte offsets. */
    CHECK(feq(cm_rs_get_off(&rs, 0x128u), 7.5f), "0x128 holds 7.5 (MaxSprintingSpeed)");
    CHECK(feq(cm_rs_get_off(&rs, 0x124u), 6.0f), "0x124 holds 6.0 (MinSprintingSpeed)");
    CHECK(feq(cm_rs_get_off(&rs, 0x530u), 0.25f), "0x530 holds 0.25 (KeyPassesPerformance)");
    CHECK(feq(cm_rs_get_off(&rs, 0x5d4u), 1.0f), "0x5d4 holds 1.0 (PerformanceBalancing)");

    /* By-name accessor agrees and reports resolved state. */
    CHECK(feq(cm_rs_get(&rs, "MaxSprintingSpeed", &found), 7.5f) && found,
          "by-name MaxSprintingSpeed == 7.5 (found)");
    CHECK(feq(cm_rs_get(&rs, "KeyPassesPerformance", &found), 0.25f) && found,
          "by-name KeyPassesPerformance == 0.25 (found)");

    /* Compute-only / absent: sentinel, not resolved, and COUNTED as unresolved. */
    CHECK(!cm_rs_is_resolved_off(&rs, 0x88u), "BallElasticity (compute-only) not resolved");
    CHECK(feq(cm_rs_get_off(&rs, 0x88u), CM_RS_UNRESOLVED), "BallElasticity slot holds sentinel");
    (void)cm_rs_get(&rs, "BaseBallSpeedMaximum", &found);
    CHECK(!found, "absent-from-registry field reported unresolved (found=0)");
    /* A known movement slot that was NOT in the registry stays sentinel. */
    CHECK(!cm_rs_is_resolved_off(&rs, 0x134u), "JoggingAccelerationMultiplier (absent) not resolved");

    printf("\n== Per-player PlayerConstants (FUN_005d6dd0) harvest ==\n");
    size_t pc_n = cm_pc_count();
    printf("  (modelled per-player getters: %zu)\n", pc_n);
    CHECK(pc_n > 50, "per-player set modelled (>50 of the 101)");

    resolved = cm_player_constants_fill(&pc, &SYN_REG, NULL, &unresolved);
    printf("  (resolved=%zu unresolved=%zu of %zu)\n", resolved, unresolved, pc_n);
    CHECK(resolved == 2, "exactly 2 per-player getters resolved (Fitness, BallControl)");
    CHECK(resolved + unresolved == pc_n, "per-player resolved + unresolved == total");

    CHECK(feq(cm_pc_get(&pc, "Fitness", &found), 95.0f) && found, "Fitness == 95.0 (found)");
    CHECK(feq(cm_pc_get(&pc, "BallControl", &found), 88.0f) && found, "BallControl == 88.0 (found)");
    (void)cm_pc_get(&pc, "KeeperReach", &found);
    CHECK(!found, "KeeperReach (compute-only) unresolved, not invented");
    (void)cm_pc_get(&pc, "Marking", &found);
    CHECK(!found, "Marking (absent from registry) unresolved");

    printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
