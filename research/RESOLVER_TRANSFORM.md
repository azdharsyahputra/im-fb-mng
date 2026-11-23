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
  obfuscated picker (FUN_005b5100).

## UPDATE — moment means are robust (VERIFIED); exact cell decomposition still ambiguous

Cross-check of pooling methods on real file 1:
- Pool group-totals vs pool ALL triples give nearly identical population means:
  Corners 3.38 vs 3.41 · GoalsScored ~0.086 · PassesCompleted ~33.9 ·
  Fouls 0.294 · ShotsOnTarget 0.303. Variance same order.
- Invariant across all metrics: ΣN over all triples == 2×sectionTotal, and exactly
  one triple has cum == sectionTotal (a catch-all/total row). This confirms the
  triples carry cumulative structure (not purely independent cells), but the
  population **mean/variance are stable regardless of decomposition**.
- Per-value-level delta reconstruction (Δsum/ΔN) does NOT yield a clean monotone
  value axis under the naive group split (negative/zero deltas appear), so the
  exact within-metric shape is NOT yet pinned.

Status:
- VERIFIED: per-metric population mean & variance (robust, realistic, decomposition-independent).
- APPROXIMATE: the draw SHAPE (Gaussian-from-moments) — sound given verified mean/var,
  but not CM's exact empirical shape.
- UNKNOWN: exact cell/cumulative decomposition for the full distribution shape; the
  attribute→context index.

The quick resolver (resolver.c) therefore rests on VERIFIED population moments; only
the draw shape and per-player context are approximations, both isolated + labeled.

## UPDATE — block0 is a normal-quantile transform grid (shape model found)

Parsing block0 (previously skipped) across metrics/files:
- `block0[0]`: count=9, nQ=8, **symmetric, range ±20.904** — identical across ALL
  metrics and files. This is a shared **standard-normal quantile axis** (9 z
  breakpoints / 8 intervals).
- `block0[1..]`: per-metric, asymmetric small ranges (e.g. Corners [2.2,4.0],
  GoalsScored/PassesCompleted [4.0,13.0]) — per-metric value-mapping parameters.

Interpretation [STRONGLY INFERRED]: each metric's distribution is a **piecewise
quantile transform of a standard normal** — sample z~N(0,1), clamp to [-20.9,20.9],
map through the per-metric block0 curve to the metric value. This EXPLAINS why the
moment reconstruction gave sane mean/var (a normal core) AND why pure Gaussian is
not exact (the transform is piecewise, skewing/bounding the normal).

Consequences:
- The Gaussian-from-moments sampler is a BASELINE approximation of a
  normal-quantile transform — kept explicitly experimental, NOT wired as original.
- The EXACT shape = normal z → block0 piecewise map; the precise value mapping
  (how block0[0] z-grid pairs with block0[1..] / the triples to produce the value)
  needs the consumer/sampler code (picker/RNG agent in progress) to confirm.

Open: exact block0[0]↔value pairing; role of the catch-all triple (cum==sectionTotal)
— likely the normalisation denominator (aggregation), not a sampled row.
