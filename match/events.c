/*
 * Match event (axiom) names — reconstructed from CM2008.exe GCMatchAxiom* set
 * (FUN_008a7770). Names Observed. See docs/findings/match-engine.md.
 */
#include "events.h"
#include <string.h>

const char *const CM_AXIOM_NAMES[CM_AX_COUNT] = {
    "Portion","NewTouch","Pass","PassReceived","BackPass","MadeTackle",
    "WasTackled","MadeInterception","WasIntercepted","Header","Cross","Runs",
    "ShotAtTarget","ShotHeader","ShotFreekick","Goal","Post","KeeperTouch",
    "HandsKick","KeeperPunch","Clearances","Corners","CornerSetup","FreeKicks",
    "GoalKick","GoalKickSetup","ThrowIn","ThrowInSetup","Offside","CausedFoul",
    "WasFouled","YellowCard","RedCard","BallOut"
};

const char *cm_axiom_name(cm_match_axiom a) {
    if (a < 0 || a >= CM_AX_COUNT) return (const char *)0;
    return CM_AXIOM_NAMES[a];
}
