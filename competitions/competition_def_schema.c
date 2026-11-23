/* Competition definition schema — reconstructed from CM2008.exe FUN_00a62de0. */
#include "competition_def_schema.h"
#include <string.h>

const char *const CM_CDEF_FIELD_NAMES[CM_CDEF_FIELD_COUNT] = {
    "NameStringID", "AreaID", "IsLeague", "CupTiedGroupID",
    "DisciplineGroupID", "PositionsRecorded", "FrequencyInMonths", "Ranking",
    "TopClubsInterested", "CompNameType", "IsPlayable", "FixtureRatingsByArea",
    "Scope", "AbbreviationStringID", "CompetitionNameOrder", "FirstTeams",
    "RevervesTeams", "YouthTeams", "LatestCompetition", "IsInternationalQualifying",
    "YearsUntilNextPlayed", "SpansSeasons", "RelatedCompNameID", "MaximumAverageAttendance",
    "MinimumAverageAttendance", "SalaryRangeMultiplier", "SalaryRangeExponent",
};

const char *cm_competition_def_schema_name(int i) {
    if (i < 0 || i >= CM_CDEF_FIELD_COUNT) return (const char *)0;
    return CM_CDEF_FIELD_NAMES[i];
}

int cm_competition_def_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_CDEF_FIELD_COUNT; i++)
        if (strcmp(name, CM_CDEF_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
