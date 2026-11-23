/* Match lifecycle FSM over the continuous float clock — clean-room. */
#include "match_lifecycle.h"

void cm_lifecycle_init(cm_lifecycle *L){
    L->clock.elapsed_s = 0.0f;
    L->clock.offset_s  = 0.0f;
    L->state = CM_GS_KICKOFF_WAITING;
    L->half  = 1;
}

cm_game_state cm_lifecycle_advance(cm_lifecycle *L, float dt){
    if(dt > 0.0f) L->clock.elapsed_s += dt;
    float t = L->clock.elapsed_s;
    switch(L->state){
    case CM_GS_KICKOFF_WAITING:
        L->state = CM_GS_IN_PLAY;                 /* kickoff taken */
        break;
    case CM_GS_IN_PLAY:
        if(L->half==1 && t >= CM_HALF_SECONDS){
            L->state = CM_GS_HALF_OVER;           /* HalfActivated->HalfOver */
        } else if(L->half==2 && t >= CM_FULL_SECONDS){
            L->state = CM_GS_GAME_OVER;
        }
        break;
    case CM_GS_HALF_OVER:
        /* second-half restart */
        L->half = 2;
        L->state = CM_GS_SECOND_HALF;
        break;
    case CM_GS_SECOND_HALF:
        L->state = CM_GS_IN_PLAY;                 /* second half kickoff */
        break;
    case CM_GS_GAME_OVER:
    default:
        break;
    }
    return L->state;
}

int cm_lifecycle_minute(const cm_lifecycle *L){ return cm_clock_minute(&L->clock); }
int cm_lifecycle_is_live(const cm_lifecycle *L){ return L->state == CM_GS_IN_PLAY; }
