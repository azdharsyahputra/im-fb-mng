/* Match/replay record schema — reconstructed from CM2008.exe FUN_00a72db0. */
#include "match_record_schema.h"
#include <string.h>

const char *const CM_MREC_FIELD_NAMES[CM_MREC_FIELD_COUNT] = {
    "MatchSeed", "HomePersonality", "AwayPersonality", "MatchInputsID",
    "MakeUnReplayableAfterAnalysis", "IsDeleted", "HomePossession", "HomeTeamDutiesID",
    "HomePassingStyle", "HomePressingStyle", "HomeOffsideTrap", "HomePlayingMentality",
    "HomeModifiedUp", "HomeManagerNameID", "HomeLikelyResult", "HomePerformanceLevel",
    "HomePlayerModified", "HomeTalkType", "AwayTeamDutiesID", "AwayPassingStyle",
    "AwayPressingStyle", "AwayOffsideTrap", "AwayPlayingMentality", "AwayModifiedUp",
    "AwayManagerNameID", "AwayLikelyResult", "AwayPerformanceLevel", "AwayPlayerModified",
    "AwayTalkType", "HomePlayerPersonalStatsID", "AwayPlayerPersonalStatsID",
};

const char *cm_match_record_schema_name(int i) {
    if (i < 0 || i >= CM_MREC_FIELD_COUNT) return (const char *)0;
    return CM_MREC_FIELD_NAMES[i];
}

int cm_match_record_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_MREC_FIELD_COUNT; i++)
        if (strcmp(name, CM_MREC_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
