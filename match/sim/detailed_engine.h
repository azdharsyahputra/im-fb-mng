/*
 * Detailed match engine update — integrates the float-clock/eGameState lifecycle
 * with the recovered per-update RESOLVE order. Models the VERIFIED call structure of
 * GCMatchModule::ProcessData/Update (FUN_0077ee80):
 *   per program-step pump -> repaint clock (FUN_0077c660) -> advance state ->
 *   if a fixture has the event-pending flag: resolve (FUN_0077cf20 -> FUN_0077c540
 *   -> predicate rule engine FUN_009e9570) -> emit ordered events -> clear flag.
 * The engine does NOT advance the clock float itself (the clock sub-object does);
 * here dt is supplied by the caller (the real cadence/dt source is UNKNOWN —
 * per-program-step, see HARD_BOUNDARIES.md).
 *
 * The resolve step is a CALLBACK so the (separately recovered) resolver/predicate
 * layer plugs in without the engine hard-wiring an approximation. Execution ORDER
 * is the faithful part; the resolve body is supplied by the caller.
 */
#ifndef CM2008_DETAILED_ENGINE_H
#define CM2008_DETAILED_ENGINE_H
#include "match_lifecycle.h"

typedef void (*cm_resolve_fn)(void *ctx, const cm_lifecycle *life);

typedef struct cm_detailed_engine {
    cm_lifecycle life;
    int resolve_calls;      /* diagnostic: how many resolve steps ran */
    int event_pending;      /* mirrors fixture+0x18 bit 0x1000000 */
} cm_detailed_engine;

void cm_detailed_init(cm_detailed_engine *E, float half_seconds);

/* One Update pump. Order (verified): advance clock+FSM, then — only while live and
 * an event is pending — run the resolve callback. Returns the game state. */
cm_game_state cm_detailed_update(cm_detailed_engine *E, float dt,
                                 cm_resolve_fn resolve, void *ctx);

#endif
