/* League standings (squad-in-competition) schema — reconstructed from CM2008.exe FUN_00a667f0. */
#include "standings_schema.h"
#include <string.h>

const char *const CM_STAND_FIELD_NAMES[CM_STAND_FIELD_COUNT] = {
    "SquadID", "GroupID", "GroupNumber", "Status",
    "BonusPoints", "PreviousPreviousSeasonAggregatePoints", "PreviousSeasonAggregatePoints", "HomeWon",
    "HomeDrawn", "HomeLost", "HomeFor", "HomeAgainst",
    "HomeYellowCards", "HomeRedCards", "AwayWon", "AwayDrawn",
    "AwayLost", "AwayFor", "AwayAgainst", "AwayYellowCards",
    "AwayRedCards", "ShootoutFor", "GamesPlayed", "PlayedThisMonth",
    "RoundPosition", "WonThisMonth", "LostThisMonth", "IsRouted",
    "IsDeleted", "fPoints",
};

const char *cm_standings_schema_name(int i) {
    if (i < 0 || i >= CM_STAND_FIELD_COUNT) return (const char *)0;
    return CM_STAND_FIELD_NAMES[i];
}

int cm_standings_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_STAND_FIELD_COUNT; i++)
        if (strcmp(name, CM_STAND_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
