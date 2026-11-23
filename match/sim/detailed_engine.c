/* Detailed engine update pump — clean-room model of the VERIFIED call order. */
#include "detailed_engine.h"

void cm_detailed_init(cm_detailed_engine *E, float half_seconds){
    cm_lifecycle_init_len(&E->life, half_seconds);
    E->resolve_calls = 0;
    E->event_pending = 0;
}

cm_game_state cm_detailed_update(cm_detailed_engine *E, float dt,
                                 cm_resolve_fn resolve, void *ctx){
    /* Verified order (FUN_0077ee80):
     *   (1) repaint clock + advance eGameState FSM first, THEN
     *   (2) only while live (IN_PLAY) and an event is pending, run resolve. */
    cm_game_state st = cm_lifecycle_advance(&E->life, dt);

    if(st == CM_GS_IN_PLAY && E->event_pending && resolve){
        resolve(ctx, &E->life);   /* FUN_0077cf20 -> FUN_0077c540 -> FUN_009e9570 */
        E->resolve_calls++;
        E->event_pending = 0;     /* clear fixture+0x18 event bit */
    }
    return st;
}
