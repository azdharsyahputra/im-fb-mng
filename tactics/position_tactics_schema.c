/* Per-position tactics schema — reconstructed from CM2008.exe FUN_00a75ca0. */
#include "position_tactics_schema.h"
#include <string.h>

const char *const CM_PTAC_FIELD_NAMES[CM_PTAC_FIELD_COUNT] = {
    "PositionTactics", "Passing", "ClosingDown", "Tackling",
    "Marking", "Mentality", "CrossFrom", "CrossAim",
    "CrossHeight", "Distribution", "DefendFreeKicks", "AttackFreeKicks",
    "DefendCorner", "AttackCorner", "AttackThrowInLeft", "AttackThrowInRight",
    "LongShots", "ForwardRuns", "RunWithBall", "FreeRole",
    "HoldUpBall",
};

const char *cm_position_tactics_schema_name(int i) {
    if (i < 0 || i >= CM_PTAC_FIELD_COUNT) return (const char *)0;
    return CM_PTAC_FIELD_NAMES[i];
}

int cm_position_tactics_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_PTAC_FIELD_COUNT; i++)
        if (strcmp(name, CM_PTAC_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
