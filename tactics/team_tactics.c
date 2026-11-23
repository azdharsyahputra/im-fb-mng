/* Team tactics schema impl — reconstructed from CM2008.exe FUN_00a76870. */
#include "team_tactics.h"
#include <string.h>

const char *const CM_TEAM_TACTIC_NAMES[CM_TT_FIELD_COUNT] = {
    "Passing","Tackling","ClosingDown","Mentality",
    "CornersLeft","CornersRight","FreeKicksLeft","FreeKicksRight",
    "UserTactics","ThrowInLeft","ThrowInRight",
    "OffsideTrap","OffsideTrapDepth","CounterAttack"
};

void cm_team_tactics_init(cm_team_tactics *t) {
    if (!t) return;
    for (int i = 0; i < CM_TT_FIELD_COUNT; i++) t->field[i] = CM_TT_UNSET;
}

const char *cm_team_tactic_name(cm_team_tactic_field f) {
    if (f < 0 || f >= CM_TT_FIELD_COUNT) return (const char *)0;
    return CM_TEAM_TACTIC_NAMES[f];
}

int cm_team_tactic_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_TT_FIELD_COUNT; i++)
        if (strcmp(name, CM_TEAM_TACTIC_NAMES[i]) == 0) return i;
    return -1;
}
