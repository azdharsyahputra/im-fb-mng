/* Competition record schema impl — reconstructed from CM2008.exe FUN_00a61c90. */
#include "competition_schema.h"
#include <string.h>

const char *const CM_COMP_FIELD_NAMES[CM_COMP_FIELD_COUNT] = {
    "auHostAreaID","SponsorStringID","AnnualSalaryCap","GamePointsID",
    "FirstFixtureId","CompetitionNameID","LastFixtureId","EligibilityGlobalRegionID",
    "BreakEndDayTimeID","BreakStartDayTimeID","SeasonID","Status","U21InSquad",
    "MustHaveSubGoalKeeper","ReserveSchedulingCompId","YouthSchedulingCompId",
    "CurrencyID","PreviousCompetition","FriendlySchedulingCompId","FixtureSpreadMin",
    "NextCompetition","TransferWindowID","LoanWindowID","IsAggregate","IsPool",
    "IsNonCompetitive","uCurrentRoundNumber","uStartRoundNumber","uEndRoundNumber",
    "uOriginalEndRoundNumber","MaxNonGlobalRegionInMatchSquad","MaxNonGlobalRegionAtClub",
    "MaxNonGlobalRegionBoughtPerSeason","MaxTeamsPerCountry","uAverageTrainingFacilityValue",
    "MaxLoanPlayerUsed","RefereeAgeLimit","MaxSquadSize","IsRoutingReady",
    "IsRoutingToUpdated","IsRoutingFromUpdated","ClearDisciplineCards","NoSubstitutesNamed",
    "NoSubstitutesUsed","MinNumRegisteredGK","MinNumRegisteredYTH","MaxNumMarqueePlayers",
    "bPlacingUpdated","ExcludeSameNationRef"
};

const char *cm_comp_field_name(int i) {
    if (i < 0 || i >= CM_COMP_FIELD_COUNT) return (const char *)0;
    return CM_COMP_FIELD_NAMES[i];
}
int cm_comp_field_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_COMP_FIELD_COUNT; i++)
        if (strcmp(name, CM_COMP_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
