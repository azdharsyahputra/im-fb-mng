# Resolver architecture (synthesis of 5-agent RE)

## On-disk format (VERIFIED, 134/134 records parse to exact EOF, all 6 files)
```
file    = section[3]
section = u32 recordCount, record[recordCount]          # counts [64,64,6]=134
record  = u32 nameLen, name, block0, triple[], terminator
block0  = u32 nBuckets, bucket[nBuckets]
bucket  = u32 count, u32 nQ, f32 quant[nQ]              # per-metric quantile grid
triple  = u32 cum, f32 lo, f32 hi                       # inverse-CDF point (12 bytes)
terminator = triple with cum == sectionTotal
```
- Leading 64 = record count (NOT a version). sec0/1 = 64 player metrics (fine / 10x-coarse); sec2 = 6 team metrics (FreeKicks,Penalties,Corners,ThrowIns,GoalKicks,SubstitutionCount).
- `triple` = empirical inverse-CDF: (cumulativeCount, valueLo, valueHi). Sampling = draw u in [0,total), find cum>=u, interpolate lo..hi.
- 6 files differ by block0 nBuckets (5/7/6/5/5/4) => 6 contexts (position/tier) [H].
- Section totals (4,121,340 / 412,134) constant, NOT stored -> loader hard-codes or derives (loader FUN_00a098a0).

## Runtime shape (VERIFIED)
CM's match logic = recursive typed-node-tree interpreters (same shape across three subsystems): command/axiom tree (FUN_008f90f0), predicate DAG (FUN_009e8f60), distribution sampler (FUN_00818120). PRNG = recovered LCG.

## Reconstructed (implemented, clean-room)
- `match/sim/resolver_dat.{h,c}` — the validated parser + inverse-CDF sampler (reads user's own .dat; verified 6/6 files parse).
- `match/sim/resolver_eval.{h,c}` — exact LCG + node evaluator.
