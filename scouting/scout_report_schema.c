/* Scout report schema — reconstructed from CM2008.exe FUN_007b5e50. */
#include "scout_report_schema.h"
#include <string.h>

const char *const CM_SCOUT_FIELD_NAMES[CM_SCOUT_FIELD_COUNT] = {
    "TDM_StaffID", "scoutname", "ScoutedLeague", "player1strength2",
    "player3strength2", "player2weakness2", "player1strength1", "player2weakness3",
    "player2name", "player3name", "player1weakness3", "player2weakness1",
    "player3weakness2", "player2generaldescription", "player1name", "player3generaldescription",
    "player3weakness1", "player2strength1", "player1generaldescription", "player1weakness2",
    "player3strength3", "player2strength3",
};

const char *cm_scout_report_schema_name(int i) {
    if (i < 0 || i >= CM_SCOUT_FIELD_COUNT) return (const char *)0;
    return CM_SCOUT_FIELD_NAMES[i];
}

int cm_scout_report_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_SCOUT_FIELD_COUNT; i++)
        if (strcmp(name, CM_SCOUT_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
