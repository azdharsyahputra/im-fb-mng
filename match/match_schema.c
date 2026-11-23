/* Match record schema — reconstructed from CM2008.exe FUN_0077ddc0. */
#include "match_schema.h"
#include <string.h>

const char *const CM_MATCH_FIELD_NAMES[CM_MATCH_FIELD_COUNT] = {
    "eGameStateInPlay", "eGameStateHandsKickActivated", "eGameStateHandsKickSpotted", "eGameStateThrowInSpotted",
    "eGameStateGoalKickSpotted", "eGameStateCornerKickSpotted", "eGameStateGoalSpotted", "eGameStateSpare",
    "eGameStateBackpassSpotted", "eGameStateKickOffSpotted", "eGameStateSubstitutionHappening", "eGameStateHalfOver",
    "eGameStateGameOver", "eGameStateApresGoal", "eGameStateKickoffWaiting", "eGameStatePenaltySpotted",
    "eGameStateThrowInActivated", "eGameStateGoalKickActivated", "eGameStateCornerKickActivated", "eGameStateGoalActivated",
    "eGameStatePenaltyActivated", "eGameStatePenaltyShootout", "eGameStateCloseFKActivated", "eGameStateBackpassActivated",
    "eGameStateHalfActivated", "eGameStateFreeKickActivated", "eGameStateFreeKickSpotted", "eGameStateFoulSpotted",
    "eGameStateOffsideSpotted", "eGameStateSetPieceFreeKickActivated", "eGameStateSetPieceFreeKickSpotted", "eGameStateNums",
};

const char *cm_match_schema_name(int i) {
    if (i < 0 || i >= CM_MATCH_FIELD_COUNT) return (const char *)0;
    return CM_MATCH_FIELD_NAMES[i];
}

int cm_match_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_MATCH_FIELD_COUNT; i++)
        if (strcmp(name, CM_MATCH_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
