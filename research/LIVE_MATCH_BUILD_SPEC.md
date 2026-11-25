# Live 2D match — build blueprint (what's VERIFIED + the 3 gaps)

Synthesis of the existing research into a buildable spec for a real 22-player + ball live match that
feeds commentary from emergent events. Pulls together MATCH_* / EGAMESTATE_* / MATCH_PLAYER_FSM_STATES /
MATCH_VTABLE_DISPATCH_MATRIX / MATCH_RESOLVER_INTERNALS.

## Layer 1 — Match FLOW skeleton — VERIFIED, PORTED (matchflow package)
A match is an eGameState machine (31 states, EGAMESTATE_COMPLETE.md): InPlay(0x00) is open play; an
incident is SPOTTED (ThrowIn/GoalKick/Corner/Goal/Backpass/FreeKick/Penalty/Foul/Offside/KickOff) then
ACTIVATED (restart taken) then resumes. State lives at *(matchCtx+0xe4)+0x30; enter handlers @0x6dd000–
0x6ed000 (STATE_WRITER_FOUND.md); stepped by GCMatchModule::Update (vtable+0xa8 = FUN_0077ee80) with an
event-gated continuous (float) clock. Ported: super-manager-engine/matchflow (enum + spotted→activated).

## Layer 2 — Player MOVEMENT — VERIFIED, PORTED (movephysics / geometry / movement / formation)
Each player is a physics body: pos = body+0x08 (X) / body+0x10 (Y) normalized 0..1; per-tick advance by
the locomotion state's speed tier (Walking 1.45–2.45 … Sprinting 7.1–9.25 m/s), base accel 3.5–5.5 /
decel −3..−5, turning circle 0.10–0.13, fatigue modifiers — ALL from formula.xml MatchResolver (51) and
the physics config initializer FUN_009f4dd0. The integrator is FUN_004a0440 (s=v·t+½a·t², pos@+0x100,
dir@+0x10c, speed@+0x118). Start positions = formation cells (FieldPosition.dat / formation.grid).

## Layer 3 — Player FSM inventory — VERIFIED (states), the mover PINNED
~255 GCPlayerState* classes (MATCH_PLAYER_FSM_STATES.md): Standing, Tactical, RunningToBall, FootOnBall,
Dribble, SprintDribble, DribbleTurn*, Shoot/header/volley, Pass, Mark, keeper Dive/Save/Push, feints.
Per-state per-tick OnTick handler advances position; for RunningToBall it is PINNED: vtable @0xb5bea8 →
OnTick 0x4de470 → move-order 0x4a0ea0 → integrator FUN_004a0440 (MATCH_VTABLE_DISPATCH_MATRIX §9).

## Layer 4 — Contest/resolution — VERIFIED, PORTED (contest / matchrating)
Duel atom TallyDuel (geometric), ResolveBestOf6, balance tier; per-player effective rating 40–100
(matchrating.EffectiveRating, kernel FUN_004b0aa0). Resolver FUN_00a029c0 writes a 3-bit outcome code
(0 none,1-3 duel/foul,4 shot,5 split) → switch FUN_00a05c40 → Monitor observer vtable 0xbd6c74 surfaces
the event (MATCH_RESOLVER_INTERNALS.md).

## Layer 5 — Commentary — VERIFIED, PORTED (commentary + clubdb + names)
Event → Commentary.str line (21 event buckets) with real club/player names. Done.

## THE 3 GAPS (being detailed by a running subagent) — needed to connect Layers 2/3 into live play
1. BALL OBJECT + POSSESSION: the ball entity (pos/vel) and the "who has it" link / transfer on
   tackle-intercept-pass; how RunningToBall's OnTick reads the ball to steer. (physics constants known;
   the possession tracking + ball struct not yet pinned.)
2. FSM TRANSITIONS: the per-state conditions that switch a player (RunningToBall→FootOnBall when near
   ball; FootOnBall→Dribble/Shoot/Pass by distance-to-goal / pressure). Only RunningToBall's OnTick is
   decompiled; the state-setter + transition thresholds for the action states are the gap.
3. EVENT EMERGENCE: the hand-off where an FSM action state (Shoot/Pass/Tackle) raises the event the
   resolver grades (FSM→FUN_00a029c0→Monitor), i.e. events arising from positions, not dice.

## Build order once the gaps land
matchflow(flow) → seed formation positions → per tick: FSM decide (decision/zone) → set waypoint/speed →
integrator step (Layer 2) → ball update + possession (gap 1) → detect action from positions (gap 2/3) →
resolver grade (Layer 4) → commentary line (Layer 5) → maybe flow transition (Layer 1). Layers 1,2,4,5
are built; only the gap-1/2/3 glue remains.
