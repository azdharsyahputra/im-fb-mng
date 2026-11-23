#include "match_state.h"
/* Names recovered verbatim from eGameState::ToString (FUN_0077ddc0). */
const char *cm_game_state_name(cm_game_state s){
    switch(s){
    case CM_GS_IN_PLAY:                return "InPlay";
    case CM_GS_HANDS_KICK_ACTIVATED:  return "HandsKickActivated";
    case CM_GS_HANDS_KICK_SPOTTED:    return "HandsKickSpotted";
    case CM_GS_THROW_IN_SPOTTED:      return "ThrowInSpotted";
    case CM_GS_GOAL_KICK_SPOTTED:     return "GoalKickSpotted";
    case CM_GS_CORNER_KICK_SPOTTED:   return "CornerKickSpotted";
    case CM_GS_GOAL_SPOTTED:          return "GoalSpotted";
    case CM_GS_SPARE:                 return "Spare";
    case CM_GS_BACKPASS_SPOTTED:      return "BackpassSpotted";
    case CM_GS_KICKOFF_SPOTTED:       return "KickOffSpotted";
    case CM_GS_SUBSTITUTION_HAPPENING:return "SubstitutionHappening";
    case CM_GS_HALF_OVER:             return "HalfOver";
    case CM_GS_GAME_OVER:             return "GameOver";
    case CM_GS_APRES_GOAL:            return "ApresGoal";
    case CM_GS_KICKOFF_WAITING:       return "KickoffWaiting";
    case CM_GS_PENALTY_SPOTTED:       return "PenaltySpotted";
    case CM_GS_THROW_IN_ACTIVATED:    return "ThrowInActivated";
    case CM_GS_GOAL_KICK_ACTIVATED:   return "GoalKickActivated";
    case CM_GS_CORNER_KICK_ACTIVATED: return "CornerKickActivated";
    case CM_GS_GOAL_ACTIVATED:        return "GoalActivated";
    case CM_GS_PENALTY_ACTIVATED:     return "PenaltyActivated";
    case CM_GS_PENALTY_SHOOTOUT:      return "PenaltyShootout";
    case CM_GS_CLOSE_FK_ACTIVATED:    return "CloseFKActivated";
    case CM_GS_BACKPASS_ACTIVATED:    return "BackpassActivated";
    case CM_GS_HALF_ACTIVATED:        return "HalfActivated";
    case CM_GS_FREE_KICK_ACTIVATED:   return "FreeKickActivated";
    case CM_GS_FREE_KICK_SPOTTED:     return "FreeKickSpotted";
    case CM_GS_FOUL_SPOTTED:          return "FoulSpotted";
    case CM_GS_OFFSIDE_SPOTTED:       return "OffsideSpotted";
    case CM_GS_SETPIECE_FK_ACTIVATED: return "SetPieceFreeKickActivated";
    case CM_GS_SETPIECE_FK_SPOTTED:   return "SetPieceFreeKickSpotted";
    case CM_GS_NUMS:                  return "Nums";
    default:                          return "Unknown MatchState";
    }
}

int cm_game_state_is_spotted(cm_game_state s){
    switch(s){
    case CM_GS_HANDS_KICK_SPOTTED: case CM_GS_THROW_IN_SPOTTED:
    case CM_GS_GOAL_KICK_SPOTTED:  case CM_GS_CORNER_KICK_SPOTTED:
    case CM_GS_GOAL_SPOTTED:       case CM_GS_BACKPASS_SPOTTED:
    case CM_GS_KICKOFF_SPOTTED:    case CM_GS_PENALTY_SPOTTED:
    case CM_GS_FREE_KICK_SPOTTED:  case CM_GS_FOUL_SPOTTED:
    case CM_GS_OFFSIDE_SPOTTED:    case CM_GS_SETPIECE_FK_SPOTTED:
        return 1;
    default: return 0;
    }
}

/* Spotted -> Activated pairing (INFERRED from the name-pairing + tick order; the
 * actual writer is UNKNOWN). Returns CM_GS_NUMS when there is no paired activation. */
cm_game_state cm_game_state_paired_activation(cm_game_state s){
    switch(s){
    case CM_GS_HANDS_KICK_SPOTTED:   return CM_GS_HANDS_KICK_ACTIVATED;
    case CM_GS_THROW_IN_SPOTTED:     return CM_GS_THROW_IN_ACTIVATED;
    case CM_GS_GOAL_KICK_SPOTTED:    return CM_GS_GOAL_KICK_ACTIVATED;
    case CM_GS_CORNER_KICK_SPOTTED:  return CM_GS_CORNER_KICK_ACTIVATED;
    case CM_GS_GOAL_SPOTTED:         return CM_GS_GOAL_ACTIVATED;
    case CM_GS_PENALTY_SPOTTED:      return CM_GS_PENALTY_ACTIVATED;
    case CM_GS_BACKPASS_SPOTTED:     return CM_GS_BACKPASS_ACTIVATED;
    case CM_GS_FREE_KICK_SPOTTED:    return CM_GS_FREE_KICK_ACTIVATED;
    case CM_GS_SETPIECE_FK_SPOTTED:  return CM_GS_SETPIECE_FK_ACTIVATED;
    default:                         return CM_GS_NUMS;
    }
}
