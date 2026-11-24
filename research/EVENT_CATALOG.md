# CM2008 Match Event Catalog — recovered from the GCMatchPredicate string table

**Status: event NAMES VERIFIED** from a contiguous `GCMatchPredicate*` string table in the retail/
crack image (VA **0xb5650c .. 0xb5992c**, 267 distinct names). These are the match engine's own
event/metric identifiers — the taxonomy the predicate (axiom) layer tests and the duel resolver
feeds. No names invented; no DRM touched (static string read + regex over the owned image).

## How the names tie to the recovered mechanism (VERIFIED + INFERRED)
- Each base metric (e.g. `SuccessfulTackles…`, `UnsuccessfulHeaders…`, `TeamGoals`) is a COUNTER an
  event increments. The recovered event is the duel `FUN_0077c710` (p = A/(A+B) over 7-bit attrs,
  LCG uniform) whose outcome calls `FUN_005cd020` to bump a saturating stat counter — see
  RESIDUALS_RESOLUTION.md. (mechanism VERIFIED; the exact predicate→counter-bit wiring per name is
  INFERRED, not individually traced.)
- `GCMatchPredicate*` is the predicate/axiom DAG (`FUN_009e9570…`, match-predicates.md) that reads
  those counters and fires reactions. So: duel → counter (base event) → predicate (named condition).

## Event taxonomy (families, with counts of distinct predicate names)

**Base in-play events** (each: Successful/Unsuccessful × zone Attack/Midfield/Defence × side Left/
Centre/Right, and a team + single-player variant):
- **Tackles** (51 names) — `(Un)SuccessfulTacklesIn{Attack,Midfield,Defence}{Left,Centre,Right}Side`,
  `Wins/FailsManyTackles`, `Wins/FailsManyDefensiveTackles`, `MadeTackle`, `LostTackleLeadsTo{Shot,Goal}`.
- **Passes** (42) — `(Un)SuccessfulPassesIn{zone}{side}`, `TeamChainsManyPasses`, `Medium/LongPassChain`,
  `MakesLongPassesIntoAttackingThird`, `AttackingPassInBox`, `PassIntoBoxAfterLongRun`,
  `PassInterceptionLeadsTo{Shot,Goal}`.
- **Headers** (41) — `(Un)SuccessfulHeadersIn{zone}{side}`, `WinsDefensiveHeaders[InBox]`,
  `WinsHeaders{Cross,Corner,ThrowIn,FreeKick,Goal}`, `WinsManyHeaders`.
- **Shots** (21) — `(Un)SuccessfulShotsFrom{Left,Centre,Right}Side`, `GoalsFromShotsInsideTheBox`,
  `GoalsFromShotsOutsideTheBox{side}`, `ShotAfterLongRun`, `DefensiveActionLeadsToShot`,
  `DefensiveErrorsLeadsToShots`, `PassInterceptionLeadsToShot`.
- **Crosses** (11) — `(Un)SuccessfulCrossesFrom{Left,Right}Side`, `SuccessfulCrossesInBox`,
  `CrossAfterLongRun`.

**Goals & chances** (15): `TeamGoals`, `PlayerScoredAGoal`, `PlayerScoredAnOwnGoal`,
`GoalFrom{PenaltySpot,Corner,Freekick,ThrowIn}`, `LongRunGoal`, `DefensiveActionLeadsToGoal`,
`DefensiveErrorsLeadsToGoals`, `GoalChancesInfluence`, `SingleGoalChanceInfluence` (+ goal-by-source
in the Shots family: inside / outside-box-by-side).

**Set pieces**: Corners (5: `GoalFromCorner`, `SuccessfulCornersInBox`, `CornerSetup`, `Corners`,
`PeriodCorner`), ThrowIn (3), FreeKick (`FreeKicks`), GoalKick (`GoalKick`, `GoalKickSetup`,
`PeriodGoalkick`).

**Discipline / misc**: `Foul`, `PeriodFoul`, `TeamOffsides`, `SinglePlayerOffsides`, `Clearances`,
`Shot`/`Header`/`Pass`/`Cross`/`Corners` (base totals).

**Possession & tempo**: `TeamPossession`, `TeamPossessionPortion`, `PeriodChangePossession`,
`TeamDoesManyQuickAttacks`, `QuickAttack`, `PeriodPortion`.

**Match-rating reactions** (68, "Other"): per-position `PlayerHadA{Good,Bad}MatchAs{Striker,Forward,
Winger,...CentralDefender,FullBack,WingBack,Sweeper,...Midfielder}`, `PlayerDoesNotGetTheBallAs{pos}`,
team-band comments (`Strong/Weak/VeryBad{Defence,Midfield}`), `PlayerHad{Strong,Bad}{Attacking,
Defensive}Match`. These are POST-match reactions over the accumulated counters, not in-play events.

## Spatial model (VERIFIED from the names)
The pitch is modelled as **3 thirds × 3 lanes**: zone ∈ {Attack, Midfield, Defence} × side ∈ {Left,
Centre, Right}. Shots/crosses use side only ({Left,Centre,Right} / {Left,Right}). This is the engine's
own zonal grid — it confirms a statistical ZONE model, not continuous (x,y) physics (consistent with
the seed-once geometry + no-integrator root).

## What this unlocks for the native engine
The event-loop (match/events.go) currently models shot/goal/save/foul/card/corner. The real taxonomy
is: **Pass, Tackle, Header, Cross, Shot** (each succ/unsucc, zoned) → **Goal** (by source) → set
pieces (Corner/ThrowIn/FreeKick/GoalKick) → Offside/Foul/Clearance, with Possession/tempo context.
Each is a duel `p=A/(A+B)` outcome bumping its named counter. Expanding EventModel to this taxonomy +
emitting named events is the next build step; the per-event RATES stay the only APPROXIMATE part.

## Classifications
- The 267 `GCMatchPredicate*` NAMES — **VERIFIED** (string table @0xb5650c..0xb5992c).
- Base event = duel → `FUN_005cd020` counter — **VERIFIED mechanism** (RESIDUALS_RESOLUTION.md).
- Which predicate name maps to which exact counter bit/dispatch flag — **INFERRED** (families are
  unambiguous by name; per-name byte wiring not individually traced).
- Zone(3)×side(3) spatial model — **VERIFIED** (from the name enumeration).
