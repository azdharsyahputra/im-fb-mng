/*
 * Clean-room reconstruction of the CM2008 match-init "ResolverSettings" harvest.
 * See resolver_settings.h for the evidence basis. This file contains LAYOUT facts
 * (name/group/offset) only; it embeds NO recovered game-data value constants — the
 * struct is populated from the formula registry at runtime.
 */
#include "resolver_settings.h"
#include <string.h>

#define F(name, group) { #name, group, (unsigned short)CM_RS_OFF_##name }

/* The recovered FUN_009f4bd0 harvest slots across the three byte-exact regions.
 * Column = the `fstp [esi+disp]` displacement. Values are NOT here. */
static const cm_rs_field RS_FIELDS[] = {
    /* ---- Movement / gait (MatchResolver, 0x9c..0x1b8) ---- */
    F(UpdateDegradableStatsPeriod,        "MatchResolver"),
    F(MinSpeedToTurningCircleRadius,      "MatchResolver"),
    F(MaxSpeedToTurningCircleRadius,      "MatchResolver"),
    F(MinBaseAcceleration,                "MatchResolver"),
    F(MaxBaseAcceleration,                "MatchResolver"),
    F(MinBaseDeceleration,                "MatchResolver"),
    F(MaxBaseDeceleration,                "MatchResolver"),
    F(MinWalkingSpeed,                    "MatchResolver"),
    F(MaxWalkingSpeed,                    "MatchResolver"),
    F(MinJoggingSpeed,                    "MatchResolver"),
    F(MaxJoggingSpeed,                    "MatchResolver"),
    F(MinRunningSpeed,                    "MatchResolver"),
    F(MaxRunningSpeed,                    "MatchResolver"),
    F(MinSprintingSpeed,                  "MatchResolver"),
    F(MaxSprintingSpeed,                  "MatchResolver"),
    F(StandingAccelerationMultiplier,     "MatchResolver"),
    F(WalkingAccelerationMultiplier,      "MatchResolver"),
    F(JoggingAccelerationMultiplier,      "MatchResolver"),
    F(SprintingAccelerationMultiplier,    "MatchResolver"),
    F(RunningAccelerationMultiplier,      "MatchResolver"),
    F(StandingDecelerationMultiplier,     "MatchResolver"),
    F(WalkingDecelerationMultiplier,      "MatchResolver"),
    F(JoggingDecelerationMultiplier,      "MatchResolver"),
    F(SprintingDecelerationMultiplier,    "MatchResolver"),
    F(RunningDecelerationMultiplier,      "MatchResolver"),
    F(BackingAccelerationMultiplier,      "MatchResolver"),
    F(BackingDecelerationMultiplier,      "MatchResolver"),
    F(MinBackingSpeed,                    "MatchResolver"),
    F(MaxBackingSpeed,                    "MatchResolver"),
    F(SidlingLeftAccelerationMultiplier,  "MatchResolver"),
    F(SidlingLeftDecelerationMultiplier,  "MatchResolver"),
    F(MinSidlingLeftSpeed,                "MatchResolver"),
    F(MaxSidlingLeftSpeed,                "MatchResolver"),
    F(SidlingRightAccelerationMultiplier, "MatchResolver"),
    F(SidlingRightDecelerationMultiplier, "MatchResolver"),
    F(MinSidlingRightSpeed,               "MatchResolver"),
    F(MaxSidlingRightSpeed,               "MatchResolver"),
    F(DivingLeftAccelerationMultiplier,   "MatchResolver"),
    F(DivingLeftDecelerationMultiplier,   "MatchResolver"),
    F(MinDivingLeftSpeed,                 "MatchResolver"),
    F(MaxDivingLeftSpeed,                 "MatchResolver"),
    F(DivingRightAccelerationMultiplier,  "MatchResolver"),
    F(DivingRightDecelerationMultiplier,  "MatchResolver"),
    F(MinDivingRightSpeed,                "MatchResolver"),
    F(MaxDivingRightSpeed,                "MatchResolver"),
    F(SlidingAccelerationMultiplier,      "MatchResolver"),
    F(SlidingDecelerationMultiplier,      "MatchResolver"),
    F(MinSlidingSpeed,                    "MatchResolver"),
    F(MaxSlidingSpeed,                    "MatchResolver"),
    F(RunningFatigueModifier,             "MatchResolver"),
    F(SprintingFatigueModifier,           "MatchResolver"),

    /* ---- Ball physics (ResolverConstants, 0x28..0x98) ---- */
    F(BaseBallSpeedGentle,            "ResolverConstants"),
    F(BaseBallSpeedMedium,            "ResolverConstants"),
    F(BaseBallSpeedMediumFirm,        "ResolverConstants"),
    F(BaseBallSpeedFirm,              "ResolverConstants"),
    F(BaseBallSpeedFirmer,            "ResolverConstants"),
    F(BaseBallSpeedStrong,            "ResolverConstants"),
    F(BaseBallSpeedMaximum,           "ResolverConstants"),
    F(BaseKickReactionTimeGentle,     "ResolverConstants"),
    F(BaseKickReactionTimeMedium,     "ResolverConstants"),
    F(BaseKickReactionTimeMediumFirm, "ResolverConstants"),
    F(BaseKickReactionTimeFirm,       "ResolverConstants"),
    F(BaseKickReactionTimeFirmer,     "ResolverConstants"),
    F(BaseKickReactionTimeStrong,     "ResolverConstants"),
    F(BaseKickReactionTimeMaximum,    "ResolverConstants"),
    F(BaseBallKickHeightFlat,         "ResolverConstants"),
    F(BaseBallKickHeightHead,         "ResolverConstants"),
    F(BaseBallKickHeightDirectional,  "ResolverConstants"),
    F(BaseBallKickHeightOverhead,     "ResolverConstants"),
    F(BaseBallKickHeightHigh,         "ResolverConstants"),
    F(BaseBallKickHeightMaximum,      "ResolverConstants"),
    F(BaseThrowReactionTime,          "ResolverConstants"),
    F(BallElasticity,                 "ResolverConstants"),
    F(AccurateShotRangeVariance,      "ResolverConstants"),
    F(AccurateShotAngleVariance,      "ResolverConstants"),
    F(InaccurateShotRangeVariance,    "ResolverConstants"),
    F(InaccurateShotAngleVariance,    "ResolverConstants"),

    /* ---- Player-rating weights (PlayerData "...Performance", 0x51c..0x5d4) ---- */
    F(RedCardPerformance,                 "PlayerData"),
    F(YellowCardPerformance,              "PlayerData"),
    F(TouchesPerformance,                 "PlayerData"),
    F(CompletedPassesPerformance,         "PlayerData"),
    F(BadPassesPerformance,               "PlayerData"),
    F(KeyPassesPerformance,               "PlayerData"),
    F(OnTargetShotsPerformance,           "PlayerData"),
    F(BadShotsPerformance,                "PlayerData"),
    F(KeeperDefenderGoalPerformance,      "PlayerData"),
    F(MidfielderGoalPerformance,          "PlayerData"),
    F(AttackerGoalPerformance,            "PlayerData"),
    F(PenGoalsPerformance,                "PlayerData"),
    F(OwnGoalsPerformance,                "PlayerData"),
    F(BadTacklesPerformance,              "PlayerData"),
    F(TacklesWonPerformance,              "PlayerData"),
    F(KeyTacklesPerformance,              "PlayerData"),
    F(DirtyPerformance,                   "PlayerData"),
    F(CynicalPerformance,                 "PlayerData"),
    F(TacklesAvoidedPerformance,          "PlayerData"),
    F(GoodCrossesPerformance,             "PlayerData"),
    F(BadCrossesPerformance,              "PlayerData"),
    F(GroundCoveredPerformance,           "PlayerData"),
    F(AssistsPerformance,                 "PlayerData"),
    F(KeptPossessionPerformance,          "PlayerData"),
    F(FailedControlPerformance,           "PlayerData"),
    F(InterceptionsPerformance,           "PlayerData"),
    F(RunsPerformance,                    "PlayerData"),
    F(ClearancePerformance,               "PlayerData"),
    F(HeadersPerformance,                 "PlayerData"),
    F(KeyHeadersPerformance,              "PlayerData"),
    F(OffsidePerformance,                 "PlayerData"),
    F(TricksPerformance,                  "PlayerData"),
    F(LostPossessionPerformance,          "PlayerData"),
    F(CrossesTakenPerformance,            "PlayerData"),
    F(CrossesPunchedPerformance,          "PlayerData"),
    F(CrossesFumbledPerformance,          "PlayerData"),
    F(ShotsHeldPerformance,               "PlayerData"),
    F(ShotsParriedPerformance,            "PlayerData"),
    F(ShotsTippedPerformance,             "PlayerData"),
    F(ShotsFumbledPerformance,            "PlayerData"),
    F(GoalConcededKeeperPerformance,      "PlayerData"),
    F(GoalConcededDefenderPerformance,    "PlayerData"),
    F(CleanSheetKeeperDefenderPerformance,"PlayerData"),
    F(CleanSheetMidfielderPerformance,    "PlayerData"),
    F(PenaltySavedPerformance,            "PlayerData"),
    F(PenaltyMissedPerformance,           "PlayerData"),
    F(PerformanceBalancing,               "PlayerData"),
};
#undef F

#define RS_FIELD_COUNT (sizeof(RS_FIELDS) / sizeof(RS_FIELDS[0]))

const cm_rs_field *cm_rs_field_table(size_t *n_out) {
    if (n_out) *n_out = RS_FIELD_COUNT;
    return RS_FIELDS;
}

void cm_resolver_settings_init(cm_resolver_settings *rs) {
    size_t i;
    if (!rs) return;
    for (i = 0; i < CM_RS_SIZE_FLOATS; i++) {
        rs->raw[i] = CM_RS_UNRESOLVED;
        rs->resolved[i] = 0;
    }
}

void cm_rs_set_off(cm_resolver_settings *rs, unsigned off, float v) {
    if (!rs || off + 4u > CM_RS_SIZE_BYTES || (off & 3u) != 0u) return;
    rs->raw[off / 4u] = v;
    rs->resolved[off / 4u] = 1;
}

float cm_rs_get_off(const cm_resolver_settings *rs, unsigned off) {
    if (!rs || off + 4u > CM_RS_SIZE_BYTES || (off & 3u) != 0u) return CM_RS_UNRESOLVED;
    return rs->raw[off / 4u];
}

int cm_rs_is_resolved_off(const cm_resolver_settings *rs, unsigned off) {
    if (!rs || off + 4u > CM_RS_SIZE_BYTES || (off & 3u) != 0u) return 0;
    return rs->resolved[off / 4u] ? 1 : 0;
}

size_t cm_resolver_settings_fill(cm_resolver_settings *rs,
                                 const cm_formula_registry *reg,
                                 const cm_fcontext *ctx,
                                 size_t *out_unresolved) {
    size_t i, resolved = 0;
    if (!rs) { if (out_unresolved) *out_unresolved = 0; return 0; }
    cm_resolver_settings_init(rs);
    for (i = 0; i < RS_FIELD_COUNT; i++) {
        float v = 0.0f;
        cm_feval_status st = cm_formula_resolve(reg, RS_FIELDS[i].name,
                                                RS_FIELDS[i].group, ctx, &v);
        if (st == CM_FEVAL_OK) {
            cm_rs_set_off(rs, RS_FIELDS[i].off, v);
            resolved++;
        }
        /* else: leave sentinel + resolved=0; it is COUNTED, not invented. */
    }
    if (out_unresolved) *out_unresolved = RS_FIELD_COUNT - resolved;
    return resolved;
}

unsigned cm_rs_offset_of(const char *name, int *ok) {
    size_t i;
    if (name) {
        for (i = 0; i < RS_FIELD_COUNT; i++) {
            if (strcmp(RS_FIELDS[i].name, name) == 0) {
                if (ok) *ok = 1;
                return RS_FIELDS[i].off;
            }
        }
    }
    if (ok) *ok = 0;
    return 0;
}

float cm_rs_get(const cm_resolver_settings *rs, const char *name, int *found) {
    int ok = 0;
    unsigned off = cm_rs_offset_of(name, &ok);
    if (!ok) { if (found) *found = 0; return CM_RS_UNRESOLVED; }
    if (found) *found = cm_rs_is_resolved_off(rs, off);
    return cm_rs_get_off(rs, off);
}

/* ================= Per-player constants (FUN_005d6dd0) ================= */

/* Documented subset of the 101 "PlayerConstant" getters (RE-fact names). */
static const char *const PC_NAMES[] = {
    /* locomotion / physical */
    "BodyBalance", "BodyStrength", "Fitness", "Height", "Mobility", "Spring",
    "DribbleAgility", "DribbleCaution", "TrickDelivery", "TrickReading", "Trickyness",
    "ClearanceAgilityLeft", "ClearanceAgilityRight", "CrossAgilityLeft", "CrossAgilityRight",
    "PassingAgilityLeft", "PassingAgilityRight", "ShotAgilityLeft", "ShotAgilityRight",
    /* passing / crossing / throwing */
    "CrossAccuracy", "CrossRangeRight", "PassCaution", "PassCreativity", "PassingAccuracy",
    "PassingRangeLeft", "PassingRangeRight", "ThrowAccuracy", "ThrowPower",
    /* shooting / set-piece */
    "BallControl", "CurlAbilityLeft", "CurlAbilityRight", "DeadballTakingSkill", "DivingHeader",
    "HeadAccuracy", "HeadRange", "PenaltyTakingSkill", "ShootingBrain", "ShootingFinesse",
    "ShotAccuracy", "ShotComposureLeft", "ShotComposureRight", "ShotRangeLeft", "ShotRangeRight",
    /* defensive / tackling */
    "Aggression", "BlockTackleStrength", "CentralDefenderAbility", "ClearancePower", "Handball",
    "HardTackleFrequency", "Marking", "OverAggression", "ShirtPulling", "SmotheringAbility",
    "TackleAvoidance", "TackleCaution", "TackleChaseRange", "TackleFrequency", "TacklingAbility",
    /* goalkeeping */
    "KeeperAgility", "KeeperBravery", "KeeperCoordinateDefence", "KeeperCrosses",
    "KeeperEccentricity", "KeeperGrip", "KeeperHandStrength", "KeeperKicking", "KeeperPunching",
    "KeeperReach", "KeeperRecovery", "KeeperRunningOut", "KeeperThrowing",
    /* mental / positioning */
    "ActingAbility", "Adaptability", "Alertness", "Ambition", "Composure", "Consistency",
    "GameAwareness", "Leadership", "LeftFoot", "RightFoot", "PhysicalPlay", "PlayerAbility",
    "Professionalism", "Resilience", "Roaming", "RunFrequency", "Versatility",
    /* positional ability ratings */
    "AttackingMidAbility", "CentralAbility", "DefensiveMidAbility", "ForwardAbility",
    "FullBackAbility", "LeftSideAbility", "MidfieldAbility", "RightSideAbility",
    "StrikerAbility", "SweeperAbility", "WingBackAbility", "WingerAbility",
};
#define PC_COUNT (sizeof(PC_NAMES) / sizeof(PC_NAMES[0]))

size_t cm_pc_count(void) { return PC_COUNT; }

const char *cm_pc_name(size_t idx) {
    return (idx < PC_COUNT) ? PC_NAMES[idx] : (const char *)0;
}

void cm_player_constants_init(cm_player_constants *pc) {
    size_t i;
    if (!pc) return;
    pc->attr = pc->_attr;
    pc->resolved = pc->_resolved;
    pc->n = PC_COUNT;
    for (i = 0; i < PC_COUNT; i++) {
        pc->_attr[i] = CM_RS_UNRESOLVED;
        pc->_resolved[i] = 0;
    }
}

size_t cm_player_constants_fill(cm_player_constants *pc,
                                const cm_formula_registry *reg,
                                const cm_fcontext *ctx,
                                size_t *out_unresolved) {
    size_t i, resolved = 0;
    if (!pc) { if (out_unresolved) *out_unresolved = 0; return 0; }
    cm_player_constants_init(pc);
    for (i = 0; i < PC_COUNT; i++) {
        float v = 0.0f;
        cm_feval_status st = cm_formula_resolve(reg, PC_NAMES[i], "PlayerConstant", ctx, &v);
        if (st == CM_FEVAL_OK) {
            pc->_attr[i] = v;
            pc->_resolved[i] = 1;
            resolved++;
        }
    }
    if (out_unresolved) *out_unresolved = PC_COUNT - resolved;
    return resolved;
}

float cm_pc_get(const cm_player_constants *pc, const char *name, int *found) {
    size_t i;
    if (pc && name) {
        for (i = 0; i < PC_COUNT; i++) {
            if (strcmp(PC_NAMES[i], name) == 0) {
                if (found) *found = pc->_resolved[i] ? 1 : 0;
                return pc->_attr[i];
            }
        }
    }
    if (found) *found = 0;
    return CM_RS_UNRESOLVED;
}
