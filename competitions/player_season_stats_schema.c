/* Player season statistics schema — reconstructed from CM2008.exe FUN_00a6d620. */
#include "player_season_stats_schema.h"
#include <string.h>

const char *const CM_PSTAT_FIELD_NAMES[CM_PSTAT_FIELD_COUNT] = {
    "PlayerNextEntity", "CumulativeRating", "IsCurrentRecord", "TransferID",
    "ShotsOnTargetOrShotsHeld", "IsOutfieldPlayerRecord", "CompetitionNameID", "CompetitionSeasonID",
    "CurrentStatsMonth", "DomesticSeasonID", "CleanSheetOrAssistances", "CumulativeRatingThisMonth",
    "TacklesWonOrShotsFumbled", "CrossesFumbled", "FoulsMade", "PassesCompleted",
    "ShotsOrShotsTipped", "FoulsAgainst", "DribblesOrCrossesTaken", "TacklesAttemptedOrShotsParried",
    "KindOfCompetition", "AppearancesThisMonth", "PassesAttemptedOrCrossesPunches", "ManOfTheMatch",
    "Appearances", "GoalsOrConcededGoals", "SubAppearances", "YellowCards",
    "RedCards",
};

const char *cm_player_season_stats_schema_name(int i) {
    if (i < 0 || i >= CM_PSTAT_FIELD_COUNT) return (const char *)0;
    return CM_PSTAT_FIELD_NAMES[i];
}

int cm_player_season_stats_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_PSTAT_FIELD_COUNT; i++)
        if (strcmp(name, CM_PSTAT_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
