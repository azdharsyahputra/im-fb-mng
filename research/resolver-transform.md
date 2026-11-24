# Finding: MatchResolverDistributions triple semantics — SOLVED

**Status:** SOLVED · **Confidence:** VERIFIED (numeric reconstruction)
**Method:** numeric hypothesis test on real bytes (no exe decompile needed).

## Result

Each 12-byte `triple` is a **cumulative moment summary**, not a value range:

```
triple = { uint32 N, float sumX, float sumX2 }       # count, Σx, Σx²
```

A group's **last triple** holds that context's population moments:

```
mean = sumX / N
var  = sumX2 / N - mean²          (>= 0 for every metric/group tested)
```

## Evidence (VERIFIED — realistic reconstruction across all metrics)

Interpreting the earlier "lo/hi" as (Σx, Σx²) makes every metric reconstruct to
realistic football statistics (file 1):

| metric | N (grp0) | mean | std |
|---|--:|--:|--:|
| Corners | 12,309 | 1.64 | 1.48 |
| FreeKicks | — | 2.9–4.0 | 1.2–1.9 |
| Penalties | 2,572 | 0.023 | 0.15 |
| GoalsScored | — | 0.001–0.009 | ~0.06 |
| ShotsOnTarget | 18,584 | 0.022–0.042 | ~0.18 |
| Fouls | 11,192 | 0.20–0.29 | ~0.46 |
| PassesCompleted | 1,241 | 15.8–21.0 | 8.4–11.5 |

(Values are per sampling unit — portion/player.) Variance ≥ 0 everywhere; count
metrics are near-Poisson (var≈mean), passes overdispersed (var>mean) — exactly as
real match events behave. This resolves the prior "lo/hi up to 7e6" puzzle: those
were Σx/Σx² over large N, not a value axis.

## Why this supersedes the inverse-CDF hypothesis

The inverse-CDF `lerp(lo,hi)` reading produced nonsense (Corners mean ≈ 40,268)
because `lo/hi` are sums, not bounds. The moment reading produces Corners mean
≈ 1.6. The 12-byte stride and group structure (from the validated format) are
unchanged; only the field meaning is corrected.

## Implemented

`match/sim/resolver_dat.c`:
- `cm_resolver_group_moments()` — returns (mean, var, N) for a group [VERIFIED].
- `cm_resolver_sample_group()` — draws from that mean/var (Gaussian-from-moments
  via the recovered LCG, clamped ≥0) [moments VERIFIED; draw shape = grounded
  approximation].

## Still open
- Intermediate triples within a group (the per-value-level cumulative moments)
  encode the exact distribution shape — using only group totals is the current
  approximation; the full shape is recoverable from them next.
- Which group/context a given player maps to (attribute→index) — still the
  picker (FUN_005b5100) — RECOVERED, key→index lookup, NO RNG (not obfuscated; RESOLVER_PICKER_CLUSTER_RECOVERY.md).
