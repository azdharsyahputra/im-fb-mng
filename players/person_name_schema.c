/* Person name/DOB base record schema — reconstructed from CM2008.exe FUN_00a7a880. */
#include "person_name_schema.h"
#include <string.h>

const char *const CM_PNAME_FIELD_NAMES[CM_PNAME_FIELD_COUNT] = {
    "FirstNameID", "LastNameID", "FavouriteClubID", "DB_Day",
    "DB_Month", "DB_Year", "AreaID", "DataID",
    "ReplaySpeed", "ReplayHighlightSpeed", "MatchSpeed", "MatchHighlightSpeed",
    "m_uAnalysisMatchSpeed", "m_uAnalysisAlternativeWindowX", "m_uAnalysisAlternativeWindowY", "m_uHighLightsFlags",
    "m_uMatchCameraViewHash", "m_bMatchPlayerNames", "m_bMatchPlayerNumbers", "m_bMatchBallTrace",
    "m_bMatchGoalReplays", "m_bMatchCustomHighlightOptions", "MatchHighlights", "ReplayHighlights",
    "m_bAnalysisPlayerNames", "m_bAnalysisPlayerNumbers", "m_bAnalysisBallTrace", "m_bAnalysisDisplayAlternateCameraView",
};

const char *cm_person_name_schema_name(int i) {
    if (i < 0 || i >= CM_PNAME_FIELD_COUNT) return (const char *)0;
    return CM_PNAME_FIELD_NAMES[i];
}

int cm_person_name_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_PNAME_FIELD_COUNT; i++)
        if (strcmp(name, CM_PNAME_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
