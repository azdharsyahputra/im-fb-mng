/* Match lifecycle FSM over the continuous float clock — clean-room. */
#include "match_lifecycle.h"

void cm_lifecycle_init_len(cm_lifecycle *L, float half_seconds){
    L->clock.elapsed_s = 0.0f;
    L->clock.offset_s  = 0.0f;
    L->state = CM_GS_KICKOFF_WAITING;
    L->half  = 1;
    L->half_seconds = half_seconds;
    L->full_seconds = half_seconds * 2.0f;
}
void cm_lifecycle_init(cm_lifecycle *L){ cm_lifecycle_init_len(L, CM_HALF_SECONDS); }

cm_game_state cm_lifecycle_advance(cm_lifecycle *L, float dt){
    if(dt > 0.0f) L->clock.elapsed_s += dt;
    float t = L->clock.elapsed_s;
    switch(L->state){
    case CM_GS_KICKOFF_WAITING:
        L->state = CM_GS_IN_PLAY;                 /* kickoff taken */
        break;
    case CM_GS_IN_PLAY:
        if(L->half==1 && t >= L->half_seconds){
            L->state = CM_GS_HALF_OVER;           /* HalfActivated->HalfOver */
        } else if(L->half==2 && t >= L->full_seconds){
            L->state = CM_GS_GAME_OVER;
        }
        break;
    case CM_GS_HALF_OVER:
        L->half = 2;
        L->state = CM_GS_KICKOFF_WAITING;         /* second-half restart -> kickoff */
        break;
    case CM_GS_GAME_OVER:
    default:
        break;
    }
    return L->state;
}

int cm_lifecycle_minute(const cm_lifecycle *L){ return cm_clock_minute(&L->clock); }
int cm_lifecycle_is_live(const cm_lifecycle *L){ return L->state == CM_GS_IN_PLAY; }
