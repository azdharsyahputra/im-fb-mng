/*
 * Clean-room reconstruction of the CM2008 match-init "ResolverSettings" model.
 *
 * EVIDENCE BASIS (all RE facts: offsets, field names, groupings — VERIFIED):
 *   - research/FORMULA_BODY_RECOVERY.md + research/FORMULA_CONSUMER_MAP.md
 *   - At match init (after __controlfp), FUN_009f4bd0 (__thiscall, esi = this)
 *     harvests ~359 global `void` formula getters into one ~0x5e0-byte struct via
 *     `fstp dword [esi+disp32]`. The disp32 is the field OFFSET recovered byte-exactly
 *     from scratchpad/harvester_9f4bd0_offsets.txt (300 stores).
 *   - FUN_005d6dd0(Player*) harvests 101 PlayerConstant getters (+degradable/fluid)
 *     into a per-player scaled-attribute array (no ResolverSettings offset; input=Player).
 *   - Byte-exact offset REGIONS modelled here:
 *       movement/gait (MatchResolver)      esi+0x9c .. esi+0x1b8
 *       ball physics (ResolverConstants)   esi+0x28 .. esi+0x98
 *       player-rating weights (PlayerData "...Performance") esi+0x51c .. esi+0x5d4
 *
 * COPYRIGHT NOTE: this header models the LAYOUT only (offsets/names/groups = RE facts).
 * The field VALUES are NOT stored here — the struct is POPULATED AT RUNTIME from the
 * formula system (defaults / formula.xml) via cm_resolver_settings_fill(). No bulk
 * recovered game-data constants are embedded.
 */
#ifndef CM2008_RESOLVER_SETTINGS_H
#define CM2008_RESOLVER_SETTINGS_H

#include <stddef.h>
#include "../formula/formula.h"

/* Full harvest-target size (FUN_009f4bd0 struct). All stores are 4-byte floats. */
#define CM_RS_SIZE_BYTES  0x5e0u
#define CM_RS_SIZE_FLOATS (CM_RS_SIZE_BYTES / 4u)   /* 376 float slots */

/* Value left at an offset whose formula did not resolve (no body + no default).
 * Distinct, non-plausible: the shipped engine never produces this. Callers must
 * consult the resolved[] flag / cm_rs_is_resolved_off(), not compare to this. */
#define CM_RS_UNRESOLVED (-1.0e30f)

/* ---- Recovered field OFFSETS (bytes from `this`/esi) — VERIFIED RE facts ---- */

/* Movement / gait region — group "MatchResolver" (esi+0x9c .. esi+0x1b8). */
#define CM_RS_OFF_UpdateDegradableStatsPeriod        0x9cu
#define CM_RS_OFF_MinSpeedToTurningCircleRadius      0xf4u
#define CM_RS_OFF_MaxSpeedToTurningCircleRadius      0xf8u
#define CM_RS_OFF_MinBaseAcceleration                0xfcu
#define CM_RS_OFF_MaxBaseAcceleration                0x100u
#define CM_RS_OFF_MinBaseDeceleration                0x104u
#define CM_RS_OFF_MaxBaseDeceleration                0x108u
#define CM_RS_OFF_MinWalkingSpeed                    0x10cu
#define CM_RS_OFF_MaxWalkingSpeed                    0x110u
#define CM_RS_OFF_MinJoggingSpeed                    0x114u
#define CM_RS_OFF_MaxJoggingSpeed                    0x118u
#define CM_RS_OFF_MinRunningSpeed                    0x11cu
#define CM_RS_OFF_MaxRunningSpeed                    0x120u
#define CM_RS_OFF_MinSprintingSpeed                  0x124u
#define CM_RS_OFF_MaxSprintingSpeed                  0x128u
#define CM_RS_OFF_StandingAccelerationMultiplier     0x12cu
#define CM_RS_OFF_WalkingAccelerationMultiplier      0x130u
#define CM_RS_OFF_JoggingAccelerationMultiplier      0x134u
#define CM_RS_OFF_SprintingAccelerationMultiplier    0x138u
#define CM_RS_OFF_RunningAccelerationMultiplier      0x13cu
#define CM_RS_OFF_StandingDecelerationMultiplier     0x140u
#define CM_RS_OFF_WalkingDecelerationMultiplier      0x144u
#define CM_RS_OFF_JoggingDecelerationMultiplier      0x148u
#define CM_RS_OFF_SprintingDecelerationMultiplier    0x14cu
#define CM_RS_OFF_RunningDecelerationMultiplier      0x150u
#define CM_RS_OFF_BackingAccelerationMultiplier      0x154u
#define CM_RS_OFF_BackingDecelerationMultiplier      0x158u
#define CM_RS_OFF_MinBackingSpeed                    0x15cu
#define CM_RS_OFF_MaxBackingSpeed                    0x160u
#define CM_RS_OFF_SidlingLeftAccelerationMultiplier  0x164u
#define CM_RS_OFF_SidlingLeftDecelerationMultiplier  0x168u
#define CM_RS_OFF_MinSidlingLeftSpeed                0x16cu
#define CM_RS_OFF_MaxSidlingLeftSpeed                0x170u
#define CM_RS_OFF_SidlingRightAccelerationMultiplier 0x174u
#define CM_RS_OFF_SidlingRightDecelerationMultiplier 0x178u
#define CM_RS_OFF_MinSidlingRightSpeed               0x17cu
#define CM_RS_OFF_MaxSidlingRightSpeed               0x180u
#define CM_RS_OFF_DivingLeftAccelerationMultiplier   0x184u
#define CM_RS_OFF_DivingLeftDecelerationMultiplier   0x188u
#define CM_RS_OFF_MinDivingLeftSpeed                 0x18cu
#define CM_RS_OFF_MaxDivingLeftSpeed                 0x190u
#define CM_RS_OFF_DivingRightAccelerationMultiplier  0x194u
#define CM_RS_OFF_DivingRightDecelerationMultiplier  0x198u
#define CM_RS_OFF_MinDivingRightSpeed                0x19cu
#define CM_RS_OFF_MaxDivingRightSpeed                0x1a0u
#define CM_RS_OFF_SlidingAccelerationMultiplier      0x1a4u
#define CM_RS_OFF_SlidingDecelerationMultiplier      0x1a8u
#define CM_RS_OFF_MinSlidingSpeed                    0x1acu
#define CM_RS_OFF_MaxSlidingSpeed                    0x1b0u
#define CM_RS_OFF_RunningFatigueModifier             0x1b4u
#define CM_RS_OFF_SprintingFatigueModifier           0x1b8u

/* Ball physics region — group "ResolverConstants" (esi+0x28 .. esi+0x98).
 * (All compute-only in the exe: bodies live in the encrypted formula.xml.) */
#define CM_RS_OFF_BaseBallSpeedGentle             0x28u
#define CM_RS_OFF_BaseBallSpeedMedium             0x2cu
#define CM_RS_OFF_BaseBallSpeedMediumFirm         0x30u
#define CM_RS_OFF_BaseBallSpeedFirm               0x34u
#define CM_RS_OFF_BaseBallSpeedFirmer             0x38u
#define CM_RS_OFF_BaseBallSpeedStrong             0x3cu
#define CM_RS_OFF_BaseBallSpeedMaximum            0x40u
#define CM_RS_OFF_BaseKickReactionTimeGentle      0x48u
#define CM_RS_OFF_BaseKickReactionTimeMedium      0x4cu
#define CM_RS_OFF_BaseKickReactionTimeMediumFirm  0x50u
#define CM_RS_OFF_BaseKickReactionTimeFirm        0x54u
#define CM_RS_OFF_BaseKickReactionTimeFirmer      0x58u
#define CM_RS_OFF_BaseKickReactionTimeStrong      0x5cu
#define CM_RS_OFF_BaseKickReactionTimeMaximum     0x60u
#define CM_RS_OFF_BaseBallKickHeightFlat          0x68u
#define CM_RS_OFF_BaseBallKickHeightHead          0x6cu
#define CM_RS_OFF_BaseBallKickHeightDirectional   0x70u
#define CM_RS_OFF_BaseBallKickHeightOverhead      0x74u
#define CM_RS_OFF_BaseBallKickHeightHigh          0x78u
#define CM_RS_OFF_BaseBallKickHeightMaximum       0x7cu
#define CM_RS_OFF_BaseThrowReactionTime           0x84u
#define CM_RS_OFF_BallElasticity                  0x88u
#define CM_RS_OFF_AccurateShotRangeVariance       0x8cu
#define CM_RS_OFF_AccurateShotAngleVariance       0x90u
#define CM_RS_OFF_InaccurateShotRangeVariance     0x94u
#define CM_RS_OFF_InaccurateShotAngleVariance     0x98u

/* Player-rating weights region — group "PlayerData" "...Performance"
 * (esi+0x51c .. esi+0x5d4). */
#define CM_RS_OFF_RedCardPerformance                 0x51cu
#define CM_RS_OFF_YellowCardPerformance              0x520u
#define CM_RS_OFF_TouchesPerformance                 0x524u
#define CM_RS_OFF_CompletedPassesPerformance         0x528u
#define CM_RS_OFF_BadPassesPerformance               0x52cu
#define CM_RS_OFF_KeyPassesPerformance               0x530u
#define CM_RS_OFF_OnTargetShotsPerformance           0x534u
#define CM_RS_OFF_BadShotsPerformance                0x538u
#define CM_RS_OFF_KeeperDefenderGoalPerformance      0x53cu
#define CM_RS_OFF_MidfielderGoalPerformance          0x540u
#define CM_RS_OFF_AttackerGoalPerformance            0x544u
#define CM_RS_OFF_PenGoalsPerformance                0x548u
#define CM_RS_OFF_OwnGoalsPerformance                0x54cu
#define CM_RS_OFF_BadTacklesPerformance              0x550u
#define CM_RS_OFF_TacklesWonPerformance              0x554u
#define CM_RS_OFF_KeyTacklesPerformance              0x558u
#define CM_RS_OFF_DirtyPerformance                   0x55cu
#define CM_RS_OFF_CynicalPerformance                 0x560u
#define CM_RS_OFF_TacklesAvoidedPerformance          0x564u
#define CM_RS_OFF_GoodCrossesPerformance             0x568u
#define CM_RS_OFF_BadCrossesPerformance              0x56cu
#define CM_RS_OFF_GroundCoveredPerformance           0x570u
#define CM_RS_OFF_AssistsPerformance                 0x574u
#define CM_RS_OFF_KeptPossessionPerformance          0x578u
#define CM_RS_OFF_FailedControlPerformance           0x57cu
#define CM_RS_OFF_InterceptionsPerformance           0x580u
#define CM_RS_OFF_RunsPerformance                    0x584u
#define CM_RS_OFF_ClearancePerformance               0x588u
#define CM_RS_OFF_HeadersPerformance                 0x58cu
#define CM_RS_OFF_KeyHeadersPerformance              0x590u
#define CM_RS_OFF_OffsidePerformance                 0x594u
#define CM_RS_OFF_TricksPerformance                  0x598u
#define CM_RS_OFF_LostPossessionPerformance          0x59cu
#define CM_RS_OFF_CrossesTakenPerformance            0x5a0u
#define CM_RS_OFF_CrossesPunchedPerformance          0x5a4u
#define CM_RS_OFF_CrossesFumbledPerformance          0x5a8u
#define CM_RS_OFF_ShotsHeldPerformance               0x5acu
#define CM_RS_OFF_ShotsParriedPerformance            0x5b0u
#define CM_RS_OFF_ShotsTippedPerformance             0x5b4u
#define CM_RS_OFF_ShotsFumbledPerformance            0x5b8u
#define CM_RS_OFF_GoalConcededKeeperPerformance      0x5bcu
#define CM_RS_OFF_GoalConcededDefenderPerformance    0x5c0u
#define CM_RS_OFF_CleanSheetKeeperDefenderPerformance 0x5c4u
#define CM_RS_OFF_CleanSheetMidfielderPerformance    0x5c8u
#define CM_RS_OFF_PenaltySavedPerformance            0x5ccu
#define CM_RS_OFF_PenaltyMissedPerformance           0x5d0u
#define CM_RS_OFF_PerformanceBalancing               0x5d4u

/*
 * The ResolverSettings struct. Modelled as a flat float array indexed by the byte
 * offsets above (mirrors `fstp [esi+disp]`). `resolved[i]` is 1 iff slot i was filled
 * from a resolved formula during harvest (vs. left at CM_RS_UNRESOLVED).
 */
typedef struct cm_resolver_settings {
    float          raw[CM_RS_SIZE_FLOATS];
    unsigned char  resolved[CM_RS_SIZE_FLOATS];
} cm_resolver_settings;

/* One recovered harvest slot: (formula name, group, byte offset). RE facts only;
 * NO value is stored here — the value is resolved at runtime. */
typedef struct cm_rs_field {
    const char    *name;
    const char    *group;
    unsigned short off;   /* byte offset into ResolverSettings (4-aligned) */
} cm_rs_field;

/* The recovered harvest field table (movement + ball + performance regions). */
const cm_rs_field *cm_rs_field_table(size_t *n_out);

/* Initialise: every slot = CM_RS_UNRESOLVED, every resolved flag = 0. */
void cm_resolver_settings_init(cm_resolver_settings *rs);

/*
 * Harvest: for each known (name, group, offset), resolve the formula via
 * cm_formula_resolve(reg, name, group, ctx) and store the result at the offset.
 * Where a formula is unresolved (no body + no default / absent), the slot keeps its
 * CM_RS_UNRESOLVED sentinel and is counted, never invented.
 * Returns the number of slots resolved; *out_unresolved (if non-NULL) gets the rest.
 */
size_t cm_resolver_settings_fill(cm_resolver_settings *rs,
                                 const cm_formula_registry *reg,
                                 const cm_fcontext *ctx,
                                 size_t *out_unresolved);

/* ---- Accessors by offset ---- */
float cm_rs_get_off(const cm_resolver_settings *rs, unsigned off);
int   cm_rs_is_resolved_off(const cm_resolver_settings *rs, unsigned off);
void  cm_rs_set_off(cm_resolver_settings *rs, unsigned off, float v); /* marks resolved */

/* ---- Accessor by name (searches the field table for the offset) ----
 * Returns the stored value; *found (if non-NULL) is 1 if the name is a known field
 * AND it was resolved, else 0. Unknown name -> CM_RS_UNRESOLVED, *found=0. */
float cm_rs_get(const cm_resolver_settings *rs, const char *name, int *found);

/* Offset for a known field name, or 0 and *ok=0 if unknown (0 is not a valid slot). */
unsigned cm_rs_offset_of(const char *name, int *ok);

/* ================= Per-player constants (FUN_005d6dd0) ================= */

/* Representative set of the 101 per-player "PlayerConstant" getters harvested by
 * FUN_005d6dd0(Player*). Names/group are RE facts; values resolve at runtime (the
 * exe default for essentially all is the attribute-scale base, supplied by the
 * registry/context, never hardcoded here). This is a documented subset of the 101. */
typedef struct cm_player_constants {
    float          *attr;        /* length = cm_pc_count() */
    unsigned char  *resolved;    /* length = cm_pc_count() */
    size_t          n;
    /* inline storage so callers need not allocate */
    float          _attr[128];
    unsigned char  _resolved[128];
} cm_player_constants;

size_t      cm_pc_count(void);
const char *cm_pc_name(size_t idx);

void cm_player_constants_init(cm_player_constants *pc);

/* Harvest the per-player set via cm_formula_resolve(reg, name, "PlayerConstant", ctx).
 * Unresolved getters keep CM_RS_UNRESOLVED and are counted. Returns #resolved. */
size_t cm_player_constants_fill(cm_player_constants *pc,
                                const cm_formula_registry *reg,
                                const cm_fcontext *ctx,
                                size_t *out_unresolved);

float cm_pc_get(const cm_player_constants *pc, const char *name, int *found);

#endif /* CM2008_RESOLVER_SETTINGS_H */
