# Finding: The Match Resolver (data-driven statistical quick-match engine)

**Status:** format identified + metric vocabulary recovered · **Confidence:** Observed
**Source:** `MatchResolverDistributions.1..6.dat` (extracted game data, ~45 MB
total) + resolver constant getters in the exe (FUN_00822xxx cluster,
`ResolverConstants`/`ResolverVersion`/`AssistsPerformance`).

## What it is

Alongside the visual match engine, CM2008 has a **statistical match resolver**
(the "quick match" path). It produces a realistic set of **per-player match
statistics** by sampling named **probability distributions** held in the six
`MatchResolverDistributions` files. The exe side is the evaluator + a large set
of data-driven `ResolverConstants`; the distributions themselves are data.

## Recovered metric vocabulary (70 metrics — Observed, factual)

The per-player outputs the resolver produces (names extracted from the
distribution files, appearing across ≥2 of the 6 files):

Assists, Challenges(Avoided/Made), Clearances, CompletedPasses, Condition,
Confidence, Corners, Crosses(+Completed/Fumbled/Punches/Taken), Cynical/Dirty/
Minor Fouls, FailedToControl, Fouls(+Commited/Received), FreeKick(Direct/Indirect),
FreeKicks, GoalKicks, Goals(Scored/Conceded/OwnGoalsScored), GroundCovered,
Headers(+Key/Won), Injured, InjuryLevel, Interceptions, KeptPossession/
LostPossession, KeyActions, Major/MinorInjuries, MissControlled, Morale,
Number(Red/Yellow)Cards, Offsides, Passes(Completed/Failed/Key/Received),
PenGoalsScored, Penalties, Performance, ScaledPerformance, PoorCrosses,
RunsMade(+Key), Shots(OnTarget/OnGoal/OffTarget/Held/Parried/Tipped/Fumbled),
SubstitutionCount, Tackles(Won/Lost/Key), ThrowIns, Touches, Tricks, Warnings.

Reconstructed as `match/resolver_metrics.h/.c` (enum + name table + lookup),
tested.

## File format (Observed structure)

Leading `uint32` (= 64), then a sequence of named records: `uint32 nameLen`,
ASCII `name` (e.g. "Assists"), then dimension counts (e.g. 5, 9, 8) and a nested
array of IEEE-754 floats forming the distribution for that metric. The six files
partition the distributions (by context/position tier). Records are large and
hierarchically nested.

## Deliberately NOT reproduced

The distribution **float values are proprietary game data** and are **not**
copied into this repository. A reconstructed resolver would load them from the
user's own extracted files at runtime. Only the format and the (factual) metric
vocabulary are captured here.

## Open

- Exact nested layout within each named record (the dimension semantics and the
  sampling index derived from player ratings/tactics) is only partially decoded.
- How the resolver maps a player's effective attributes → distribution indices
  → sampled stats is the remaining behavioral piece (ties into GCFormulaModule).
