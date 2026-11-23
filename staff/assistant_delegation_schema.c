/* Assistant-manager delegation settings schema — reconstructed from CM2008.exe FUN_00a896c0. */
#include "assistant_delegation_schema.h"
#include <string.h>

const char *const CM_ASST_FIELD_NAMES[CM_ASST_FIELD_COUNT] = {
    "PersonalityID", "AssistantTakesDomesticTraining", "AssistantBuysDomesticPlayers", "AssistantOffersDomesticContracts",
    "AssistantHiresAndFiresDomesticCoachingStaff", "AssistantArrangesDomesticFriendlyMatches", "AssistantControlsDomesticFirstTeam", "AssistantSelectsDomesticFirstTeam",
    "AssistantSelectsDomesticFirstTeamTactics", "AssistantControlsDomesticReserveTeam", "AssistantSelectsDomesticReserveTeam", "AssistantSelectsDomesticReserveTeamTactics",
    "AssistantControlsDomesticYouthTeam", "AssistantSelectsDomesticYouthTeam", "AssistantSelectsDomesticYouthTeamTactics", "SugarDaddy",
    "AsstSelectDomesticUnfitSeniors", "uBoardRequestStateExtraTransferFunds", "uBoardRequestStateExtraWageFunds", "uBoardRequestStateExpandStadium",
    "uBoardRequestStateImproveFacilities", "uBoardRequesStatetExtraRebuildTime", "FinedPlayers", "DisciplinedPlayers",
    "PendingUserSacking", "BoardRequestCounterExtraTransferFunds", "BoardRequestCounterExtraWageFunds", "BoardRequestCounterExpandStadium",
    "BoardRequestCounterImproveFacilities", "BoardRequestCounterExtraRebuildTime", "IsDeleted",
};

const char *cm_assistant_delegation_schema_name(int i) {
    if (i < 0 || i >= CM_ASST_FIELD_COUNT) return (const char *)0;
    return CM_ASST_FIELD_NAMES[i];
}

int cm_assistant_delegation_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_ASST_FIELD_COUNT; i++)
        if (strcmp(name, CM_ASST_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
