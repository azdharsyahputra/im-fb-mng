/* Player record schema — reconstructed from CM2008.exe FUN_00a7c540. */
#include "player_schema.h"
#include <string.h>

const char *const CM_PLAYER_FIELD_NAMES[CM_PLAYER_FIELD_COUNT] = {
    "AskingPrice", "PlaysNationalTeam", "PersonID", "AvailabilityID",
    "DescriptionID", "MediaPersonalityID", "Morale", "CareerPersonalityID",
    "RoleID", "InternationalGoals", "ContractID", "InternationalCaps",
    "InjuryID", "FinancialPersonalityID", "Condition", "HistoryPlayer",
    "InternationalInstruction", "TrainingShedule", "PlayerProperties", "RetiredFromInternationals",
    "OtherCupTiedFlags", "WeeksFine", "RegistrationFlags", "Improvement",
    "ThisCupTiedFlags", "UnavailableReason", "UnavailableCounter", "ShortlistID",
    "OfferedToUser", "MatchFitness", "IsPhantom", "IsImmortal",
    "IsFake", "IsDeleted", "CurrentAbility", "PotentialAbility",
    "HomeReputation", "WorldReputation", "TendencyTrainingOutfielder",
};

const char *cm_player_schema_name(int i) {
    if (i < 0 || i >= CM_PLAYER_FIELD_COUNT) return (const char *)0;
    return CM_PLAYER_FIELD_NAMES[i];
}

int cm_player_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_PLAYER_FIELD_COUNT; i++)
        if (strcmp(name, CM_PLAYER_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
