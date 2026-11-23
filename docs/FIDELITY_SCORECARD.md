# CM2008 Match-Engine Fidelity Scorecard

Honest per-mechanic classification of the current clean-room engine (`match/sim/`)
against recovered CM2008 evidence.

Legend: **FAITHFUL** = backed by recovered structure/parameter · **APPROX** =
CM-derived structure, details are our design · **INVENTED** = our own, no CM basis.

| # | Mechanic | Our impl | Classification | Evidence / why |
|--:|---|---|---|---|
| 1 | Match update loop | `cm_match_tick` (60 ticks/min) | **APPROX** | CM has a real dispatcher `FUN_008f90f0` (135 callees); its call sequence + tick rate are NOT yet decoded, so ours is a plausible structure only |
| 2 | Formation / positions | 4-4-2 hard-coded coords | **INVENTED** | CM stores formations as data (`TeamTacticsFormation` schema, FUN_00a84df0 FormationID); our coordinates are our own |
| 3 | Player movement | `speed=0.55+0.055*pace`; move-to-target | **INVENTED** | the formula is ours; only the attribute (pace) is real. CM's mover is `FUN_00a1fdf0` (located, not decoded) |
| 4 | Marking | standoff scaled by `marking` | **APPROX** | sliders `Marking`, `MarkingLine/Offset/Style`, `MarkingDistanceScale=0.02` are REAL (FUN_0083ea10); the geometry is ours |
| 5 | Closing down / pressing | pressers from `closing_down` | **APPROX** | `ClosingDown`/`TeamClosingDown` are real sliders; the trigger math is ours |
| 6 | Offside | 2nd-last-defender line | **APPROX** | CM has `GCMatchAxiomOffside`; our line computation is standard-rule, our own code |
| 7 | Pass decision + completion | option pick + `0.88+0.006*passing…` | **INVENTED** | event type real (`Pass`/`PassReceived` axioms); the probabilities are ours |
| 8 | Tackle | `0.30+0.03*(tackling-dribbling)` | **INVENTED** | axioms `MadeTackle`/`WasTackled` real; the win formula is ours |
| 9 | Shot + GK save | `finishing`/`handling` curves | **INVENTED** | axioms `ShotAtTarget`/`Goal` real; curves are ours |
| 10 | Ball physics (flight, control/intercept radius) | our constants | **INVENTED** | CM models ball interactions (located in 0x008f cluster); our physics is our own |
| 11 | Restarts (throw/corner/goal-kick) | our handlers | **APPROX** | CM has the axioms (`ThrowIn`,`Corners`,`GoalKick`,`CornerSetup`…); our logic is our own |
| 12 | Match stats (shots, passes, tackles, possession…) | counted in sim | **FAITHFUL (vocabulary)** | the metric set matches CM's `GCMatchPredicate*` + MatchResolver 70 metrics; the VALUES come from our invented sim |
| 13 | **Statistical resolver (CDF sampling)** | **NOT INTEGRATED** | **MISSING** | the mechanism that makes CM *CM* — attribute→distribution→metric sampling via `MatchResolverDistributions` — is absent from our engine |

## Bottom line

- **Vocabulary / parameters / event taxonomy / data schemas**: FAITHFUL (recovered).
- **All gameplay math (movement, pass/tackle/shot probabilities, ball physics)**:
  INVENTED — plausible football, not CM's algorithms.
- **The core fidelity mechanism is not wired in**: CM resolves matches by reading
  attributes/tactics through `FUN_00662cf0`/`FUN_00818120` and sampling the
  `MatchResolverDistributions` CDF tables. Our engine uses hand-written formulas
  instead. **Until the engine samples the resolver, it is CM-shaped but not
  CM-faithful.**

## Path to real fidelity (ordered)

1. Decode the resolver CDF indexing: how an attribute/context maps to a (block,
   bucket) and how a metric is sampled (needs reading the resolver sampler; the
   getters FUN_00662cf0/FUN_00818120 are the consumers).
2. Drive per-player match metrics by sampling the resolver from the user's own
   extracted distribution files (not our formulas).
3. Decode FUN_008f90f0's call sequence to fix tick order + frequency.
4. Decode FUN_00a1fdf0 to replace invented movement with CM's positioning.
5. Validate with the fidelity test framework (distributions, not just scores).
