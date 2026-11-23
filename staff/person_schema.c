/* Staff/manager person schema impl — reconstructed from CM2008.exe FUN_00a84df0.
 * Names preserve the original spellings exactly (incl. the game's own typos). */
#include "person_schema.h"
#include <string.h>

const char *const CM_PERSON_FIELD_NAMES[CM_PF_FIELD_COUNT] = {
    "PersonID",
    "TrainingStyleID","FormationID","MarkingStyleID","PlayingMentality",
    "PassingStyleID","PressingStyleID","PlayOffsideTrap","PlayRoamingPlayers",
    "Determination","Professionalism","Pressure","Loyalty",
    "Sportmanship","Adaptibility","Ambition","Superstition",
    "CoachingGoalkeepers","CoachingOutfieldPlayers","Management",
    "Physiotherapy","Tactics","CoachingYouthPlayers",
    "TransferAcumen","LevelOfTrust","Generosity",
    "ScoutingOnAbility","ScoutingOnPotential","Motivating",
    "Strictness","Realism",
    "ContractID","PlaysNationalTeam","Shortlist",
    "ScoutingOrderID","ManagerHistoryID","SeekingEmployment",
    "MarkedForAIBackroomStaffManagementTransfer","IsDeleted","CurrentAbility"
};

const char *cm_person_field_name(cm_person_field f) {
    if (f < 0 || f >= CM_PF_FIELD_COUNT) return (const char *)0;
    return CM_PERSON_FIELD_NAMES[f];
}

int cm_person_field_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_PF_FIELD_COUNT; i++)
        if (strcmp(name, CM_PERSON_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
