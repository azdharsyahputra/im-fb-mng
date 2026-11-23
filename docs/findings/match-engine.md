# Finding: Match Engine model (events, predicates, in-match attributes)

**Status:** reverse-engineered (structure) · **Confidence:** Observed (decompiled + symbol strings)
**Target:** retail `CM2008.exe` (SHA-256 `5b14a40b…37e81`), Ghidra project `CM2008`.
**Evidence:** `docs/ghidra_dumps/match_player/decompiled.c` (FUN_008a7770, FUN_00a82320, FUN_00a7f650…), symbol dump.

CM2008's match engine is an **event-driven model with three layers**:

```
  Match Axioms (atomic events)  ->  Match Predicates (derived performance)  ->  player ratings / commentary
```

## Layer 1 — Match Axioms (atomic events)

Registered by name in `FUN_008a7770` (lazy-interned via `FUN_00405530`, guarded
by bitmask `_DAT_00c92c24`). Complete recovered set (Observed):

`BackPass, BallOut, CausedFoul, Clearances, CornerSetup, Corners, Cross,
FreeKicks, Goal, GoalKick, GoalKickSetup, HandsKick, Header, KeeperTouch,
MadeInterception, MadeTackle, NewTouch, Offside, Pass, PassReceived, Portion,
Post, RedCard, Runs, ShotAtTarget, ShotFreekick, ShotHeader, ThrowIn,
ThrowInSetup, WasFouled, WasIntercepted, WasTackled, YellowCard`

`Portion` = a time-slice of the match (the engine advances in "portions"); the
`…Setup` axioms (CornerSetup, GoalKickSetup, ThrowInSetup) are set-piece
pre-states. `New Touch`/`PassReceived`/`WasTackled`/`WasIntercepted` are the
passive counterparts of the active events — i.e. events are recorded for both the
acting and receiving player.

## Layer 2 — Match Predicates (derived performance, ~90)

Predicates are boolean/counted conditions evaluated over axioms, used for player
ratings and the commentary/analysis system. Recovered families (names truncated
by Ghidra's label length but unambiguous):

- **Actions:** Pass, MadeTackle, Header, Cross, Corners, Clearances, FreeKicks,
  GoalKick, ThrowIn, Foul, Shot, Rebound, Move
- **Success/failure pairs:** Successful/Unsuccessful × Tackle, Pass, Header,
  Shot(sFrom…), Cross; WinsDefensiveHeader, WinsHeaders(Corner/Cross/Freekick/
  Goal/Throw), WinsManyHeaders/Tackles, FailsManyTackles/DefensiveActions
- **Goal provenance:** GoalFromCorner, GoalFromFreekick, GoalFromPenalty,
  GoalFromThrowIn, GoalsFromShotsInside / …Outside, LongRunGoal, OwnGoal
- **Chains & runs:** LongPassChain, MediumPassChain, LongRunEndingInAttack,
  CrossAfterLongRun, PassIntoBoxAfter…, QuickAttack, TeamChainsManyPasses
- **Team/band summaries:** TeamPossession(Portion), TeamGoals, TeamOffsides,
  Attack/Midfield/Defence BandCommentary, Strong/Weak/VeryBad × Defence/Midfield
- **Player verdicts (rating inputs):** PlayerHadAGoodMatch, PlayerHadABadMatch,
  PlayerHadStrongAttack/Defence, PlayerHadBadDefence, PlayerScoredAGoal,
  PlayerScoredAnOwnGoal, PlayerDoesNotGet…, SinglePlayerSuccessful/Unsuccessful/Offside

Full list: symbol dump (`grep GCMatchPredicate`).

## Layer 3 — In-match player attribute vector

`FUN_00a82320` / `FUN_00a7f650` register the per-player in-match fields as
`uint32` records `(type="uint32", name, precision=7|4, size=4, default=-1)`. Each
gameplay attribute appears as a **`<Attr>Value`** (effective in-match value) and
**`<Attr>Offset`** (situational modifier), i.e.:

```
effective_in_match[Attr] = base[Attr] + Offset[Attr]      (Observed pattern; exact combine fn not yet decompiled)
```

Derived/aggregate in-match values also present: `MovementValue`,
`PositioningValue`, `CreativityValue`, `DecisionMakingValue`, `TeamWorkValue`,
`WorkRateValue`, `ConcentrationValue`, `AnticipationValue`. See
`docs/findings/player-attributes.md` for the full attribute set.

## Unknowns (the simulation math)

- How axioms are **generated** each `Portion` from the two teams' effective
  attributes + tactics + RNG — the core simulation functions (large, unnamed) are
  not yet pinned. This is the biggest remaining gap.
- The exact `base (+/combine) Offset` function and the meaning of the precision
  constant (7 vs 4).
- Predicate → numeric rating weighting.
