/* Club finance snapshot schema — reconstructed from CM2008.exe FUN_00a5f970. */
#include "club_finance_schema.h"
#include <string.h>

const char *const CM_CFIN_FIELD_NAMES[CM_CFIN_FIELD_COUNT] = {
    "Training", "BankBalance", "LoansOutstanding", "TicketPrice",
    "Merchandising", "GroundCost", "SponsorshipPerYear", "Attendence",
    "StadiumCapacity", "Reputation", "SeasonTicketSales", "FirstTeamPlayerPotential",
    "FirstTeamPlayerSalary", "ManagerSalary", "NumPlayersInFirstSquad", "NumPlayersInReserveSquad",
    "NumPlayersInYouthSquad", "NumCoaches", "NumScouts", "NumPhysios",
    "NumBoardMembers", "FirstTeamPlayerGlobalRep", "FirstTeamPlayerDomesticRep", "FirstTeamPlayerAbility",
    "StaffAbility", "BoardAbility", "RefereeAbility", "ClubID",
    "IsDeleted",
};

const char *cm_club_finance_schema_name(int i) {
    if (i < 0 || i >= CM_CFIN_FIELD_COUNT) return (const char *)0;
    return CM_CFIN_FIELD_NAMES[i];
}

int cm_club_finance_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_CFIN_FIELD_COUNT; i++)
        if (strcmp(name, CM_CFIN_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
