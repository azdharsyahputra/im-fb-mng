/* Season/table record schema — reconstructed from CM2008.exe FUN_00a647b0. */
#include "season_schema.h"
#include <string.h>

const char *const CM_SEASON_FIELD_NAMES[CM_SEASON_FIELD_COUNT] = {
    "uStatus", "RoundOrder", "NoOfGroups", "RoundNo",
    "StartDayNoID", "StartDayID", "StartMonthID", "EndDayNoID",
    "EndDayID", "EndMonthID", "CompetitionID", "NoOfSquads",
    "TimesPlayed", "RoundPosition", "FixturesGenerated", "RoundTypeID",
    "ChangeKitID", "ClearDisciplineCards", "DrawDayTimeID", "EndDayTimeID",
    "HomeGateReceiptPercent", "NeutralVenue", "NoSwapPlayers", "PlayerRegistrationDayTimeID",
    "ReplayEndDayTimeID", "ReplayVenueChangeID", "StartDayTimeID", "TVMoney",
    "UniqueFixture", "VenueQuality", "RegistrationNumBeforeStart", "DrawNumAfterPrevStart",
    "StartNumAfterPrevStart", "EndNumAfterStart", "ReplayNumAfterEnd", "Group",
    "DrawNumAfterPrevEnd", "StartNumAfterPrevEnd", "OfficialLevelID", "MatchRulesMask",
    "ReplayMatchRulesMask", "IsFinalGroupGamePlay", "ContainsTitleHolder", "IsRoutingUpdated",
    "bPlacingUpdated", "IsDeleted", "MaxPlayed", "MaxHomeGamesBroadcast",
    "RoundNameStringID", "uNumberOfFixtures", "uNumberOfPopulatedFixtures", "OriginalNoOfSquads",
};

const char *cm_season_schema_name(int i) {
    if (i < 0 || i >= CM_SEASON_FIELD_COUNT) return (const char *)0;
    return CM_SEASON_FIELD_NAMES[i];
}

int cm_season_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_SEASON_FIELD_COUNT; i++)
        if (strcmp(name, CM_SEASON_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
