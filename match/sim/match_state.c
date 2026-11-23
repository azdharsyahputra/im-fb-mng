#include "match_state.h"
const char *cm_game_state_name(cm_game_state s){
    switch(s){
    case CM_GS_IN_PLAY:         return "InPlay";
    case CM_GS_HALF_OVER:       return "HalfOver";
    case CM_GS_GAME_OVER:       return "GameOver";
    case CM_GS_APRES_GOAL:      return "ApresGoal";
    case CM_GS_KICKOFF_WAITING: return "KickoffWaiting";
    case CM_GS_HALF_ACTIVATED:  return "HalfActivated";
    case CM_GS_NUMS:            return "Nums";
    default:                    return (const char *)0;
    }
}
