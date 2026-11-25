# CM2008 per-player match FSM — state inventory (VERIFIED)

The match's 2D engine drives each player through a finite state machine. State names are stored as plain
strings in `.rdata` (0xb5be14–0xb5e9f8), recovered via `search_strings("GC(Player|Ball|Match)State…")`.
Each state is a class `GCPlayerState<Name>`; its per-tick handler (a virtual method) advances that player's
normalized position (body+0x08/+0x10) and animation. The "position integrator" is distributed across the
LOCOMOTION handlers below — there is no single integrator (see MATCH_VTABLE_DISPATCH_MATRIX.md §7).

These are game-internal class identifiers (no club/player names); safe reference data. Handler addresses
are reachable per state via the RTTI vtable walk or the FSM's `{handler_ptr[], name_ptr[]}` enum arrays.

## Locomotion / positioning — the distributed "mover" (speed tiers ↔ formula.xml / movephysics)
Standing, Tactical, RunningToBall, FootOnBall, Dribble, SprintDribble, Milling, HalfMilling, Skipping,
Leaving, NeedsBath, WaitingForTouch, WaitingForTeamTouch, Wall, CollectBallOnly.

## Turning circle
DribbleTurn1R, DribbleTurn2R, DribbleTurn3R, DribbleTurn4, DribbleTurn1L, DribbleTurn2L, DribbleTurn3L.

## Feints / tricks
SpinLeft, SpinRight, LollypopRight, LollypopLeft, DummyRight, DummyLeft, DragBackRight, DragBackLeft,
Backheeled, DribbleBackheel, RunBackheel.

## Receiving / collecting
Receiving, SpaceReceiving, ReceivingHigh, ReceivingBackpass, CollectingBall, CollectingThrow,
CollectingCorner, CollectingSetPieceFreeKickBall, KickOffReceive, RecoverBallAfterGoal.

## Passing
Pass, PassRight, PassLeft, RunningPass, RunPassRight, RunPassLeft, DribblePass, DribblePassRight,
DribblePassLeft, Layoff, CrossedLeft, CrossedRight, KickOffPass, HookPassRight, HookPassLeft.

## Shooting
Shot, ShotRight, ShotLeft, RunningShot, RunShotRight, RunShotLeft, DribbleShot, DribbleShotRight,
DribbleShotLeft, SideFootShotLeft, SideFootShotRight, RunSideFootShotLeft, RunSideFootShotRight,
CurlShotLeft, CurlShotRight, RunCurlShotLeft, RunCurlShotRight, HookShotRight, HookShotLeft, SlideShot,
VolleyLeft, VolleyRight, RunVolleyLeft, RunVolleyRight, SprintVolleyLeft, SprintVolleyRight,
DefVolleyLeft, DefVolleyRight, DribbleDefVolleyLeft/Right, RunDefVolleyLeft/Right, OverheadKicking,
DiveHeading. Generic kick family: Kick, DribbleKick, RunKick, KickLeft/Right (+Dribble/Run variants),
Whack, DribbleWhack, RunWhack, WhackLeft/Right (+Dribble/Run variants).

## Heading / chest
JumpingChest, Chesting, CushionHeading, DefensiveHeading, PowerHeading, DownwardHeading, BackHeading,
DirectionalHeading, LoopingHeading.

## Fouls / physical
BlockTackle, SlideTackle, HookTackle, DirtyTackle, Tripping, Elbowing, PullShirt, Handball, TakesDive,
Stumble, ShirtPulled, FallenBackwards, FallenForwards, Climbing, ClimbedOn, KeeperCollision,
ReactLeft/Right, ReactHighLeft/Right, Aggro, AfterAggro, RefMoan, RefereeArguement.

## Set pieces
CornerKickPrepare/WalkBack/Runup/Signalling/Release/Short, CornerDefence, CollectingCorner,
ThrowPrepare/WalkBack/Release/Short, FreeKickWalkBack/Short, SetPieceFreeKickWalkBack,
PreparingFreeKick/PreparingSetPieceFreeKick, AwaitingFreeKick, TakingFreeKick, TakingSetPieceFreeKick,
GoalKickShort, AttackSetPiece, TakingPenalty, AwaitingPenalty, PreparingPenalty, PenaltyWalkBack,
ShootoutWait.

## Reactions / celebrations / admin
Celebration, PenCelebration, RunToScorer, Disapointed, SubbedOff, SubbedOn.

## Keeper states (GCPlayerStateKeeper*)
Watching, Clutching, Smothering, SmotherClaim, Blocking, LegBlockLeft/Right, Jumping, Stooping, Tipping,
Punching, Clearing, Bowling, Rolling, Dismay, CaughtHigh/Low/Mid, ReadyForHandsKick, PreparingHandsKick,
HandsKickWalkBack, HandsKickRelease, CollectingGoalKick, GoalKickWalkBack, GoalKicking, ShootoutReceiver,
ShootoutNonReceiver, SaveRecovery, DiveRecovery, RecoverBallAfterGoal.
Dive matrix: DiveLeft/Right, DiveLowLeft/Right, DiveHighLeft/Right, DiveCloseLeft/Right,
DiveLowCloseLeft/Right, DiveHighCloseLeft/Right. Push matrix: PushLeft/Right (+Low/High/Close/LowClose/
HighClose ×Left/Right). Save matrix: SaveLeft/Right (+Low/High/Close/LowClose/HighClose ×Left/Right).

## Official/referee states (GCOfficialState*, 0x498 region; handler FUN_00498ac0)
RefereeHalfMilling (FUN_00498d40), RefereeAfterShootout, RefereePlayerArguement,
OfficialSignallingSubstitution, AwaitingCornerKick (+ more by the same table walk).

---
This inventory makes the dispatch structure and per-handler semantics explicit without decompiling any
handler body — the per-state position/animation behaviour is named. Extracting a literal per-tick
`pos += speed·dir·dt` is one decompile of a chosen locomotion handler.
