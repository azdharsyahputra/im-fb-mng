/* Squad/position tactics schema — reconstructed from CM2008.exe FUN_00a76dc0. */
#include "squad_tactics_schema.h"
#include <string.h>

const char *const CM_SQT_FIELD_NAMES[CM_SQT_FIELD_COUNT] = {
    "NameID", "TeamPassing", "TeamTackling", "TeamClosingDown",
    "TeamMentality", "CornersLeft", "CornersRight", "FreeKicksLeft",
    "FreeKicksRight", "UserTactics", "OffsideTrap", "CounterAttack",
    "MenBehindBall", "ThrowInLeft", "ThrowInRight", "OffsideTrapDepth",
    "IsDeleted", "Passing", "ClosingDown", "Tackling",
    "Marking", "Mentality", "CrossFrom", "CrossAim",
    "CrossHeight", "Distribution", "DefendFreeKicks", "AttackFreeKicks",
    "DefendCorner", "AttackCorner", "AttackThrowInLeft", "AttackThrowInRight",
    "LongShots", "ForwardRuns", "RunWithBall", "FreeRole",
    "HoldUpBall", "MarkingPosition", "Column", "AttackRunsRow",
    "AttackRunsColumn", "DefendRunsRow", "DefendRunsColumn", "FeedsRow1",
    "FeedsColumn1", "FeedsRow2", "FeedsColumn2", "ExplicitRole",
    "Penalty", "FreeKick",
};

const char *cm_squad_tactics_schema_name(int i) {
    if (i < 0 || i >= CM_SQT_FIELD_COUNT) return (const char *)0;
    return CM_SQT_FIELD_NAMES[i];
}

int cm_squad_tactics_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_SQT_FIELD_COUNT; i++)
        if (strcmp(name, CM_SQT_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
