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

## Nested grammar (verified layer)

Structural analysis (counts only — no values reproduced) gives:

```
file    = u32 version(=64) , record*
record  = u32 nameLen , name , block+        (blocks until the next name / EOF)
block   = u32 nBuckets , bucket[nBuckets]
bucket  = u32 key , u32 nValues , f32[nValues]
```

The first block of each record parses cleanly (e.g. Assists' first block =
5 buckets with keys 9/34/31/32/27 and 8/6/6/5/4 values). Reconstructed as a
buffer reader in `match/resolver_format.h/.c` that walks this grammar and
**skips the float payloads** (values are never materialised), tested on
synthetic data in `tests/`.

Still open: deeper blocks within a record show variants (many empty buckets,
alternate keying) that the single grammar above does not yet consume to the exact
record boundary; and the mapping from a player's effective attributes/tactics to
the (block,bucket) index used for sampling. Those remain the behavioral frontier.

## Deeper blocks = quantised distributions (5-point monotonic threshold tables)

Structural analysis of the blocks after the first shows the data is dominated by
**short monotonically-increasing integer runs, predominantly length 5**, each
separated by zero words with floats interleaved. Semantically this is a
**quantile / CDF encoding**: every distribution is stored as a small set of
cumulative threshold breakpoints (≈5 per curve) plus associated float weights —
a compact way to sample a performance metric given a context index. (Only the
*structure* is characterised here; the threshold/weight numbers are proprietary
game data and are not reproduced.)

The exact byte framing of these threshold tables (the zero-padding and the
float-interleave order) is not yet pinned to the record boundary by structural
analysis alone; the definitive layout needs the distribution *loader* function
from the exe (the FUN_00822xxx cluster are the constant getters, not the file
parser) — locating and decompiling that loader is the clear next step.

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
