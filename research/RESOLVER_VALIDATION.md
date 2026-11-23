# Resolver validation (independent + cross-checks)

## VERIFIED (byte/parse/decompile-backed)
- File format: 134/134 records to exact EOF in all 6 files (own C parser + agent python agree).
- Leading u32 == 64 in all files; sections [64,64,6]; 70 metric names.
- LCG constants 1664525 / 1013904223 (mod 2^32) — read from inlined sites.
- Sampler chain FUN_00818120 + node ops (binop/aggregate/named-ref) — decompiled.
- Resolver I/O structs (GCResolverInput/Event), index dims (ESquad/EPosition/EDuty), 0x34c stat slab, FE marshalling — decompiled.

## CORRECTED / ruled out
- Floats are a VALUE axis (z-score-like), NOT probabilities (earlier claim withdrawn).
- "64" is a count, not a version (earlier claim withdrawn).
- `*Distribution`-named symbols = ball-distribution tactics in the VISUAL AI, NOT the resolver CDFs (false lead ruled out).
- Getters FUN_00662cf0/FUN_00818120 read scalar named fields; the `.dat` loader is separate (FUN_00a098a0).

## OPEN (honest, named dependencies)
- U: GCFormulaModule writer of *Performance (attribute->performance float).
- U: the index/bucket picker FUN_005b5100 (obfuscated) -> exact context/value-level selection.
- U: section-total source (hard-coded vs derived) — needs loader FUN_00a098a0 body.
- U: outer match clock/tick loop that submits node trees.
- H: the 6 files == 6 position/tier contexts.

## UPDATE — .dat value-semantics are UNRESOLVED (demonstrated)

The parser STRUCTURE is verified, but a direct differential test exposed that the
**triple field semantics are not yet correct**:
- On real `MatchResolverDistributions.1.dat`, record "Corners" (section 2, total
  412,134): `lo/hi` are integer-valued floats ranging `[0 .. ~7,004,008]`, always
  `lo<=hi`; first group is a single triple `cum=12309, lo=20212, hi=60152`.
- These are neither a 0..N metric value axis (corners are 0..~20) nor z-scores
  (±~21). So the inverse-CDF `lerp(lo,hi)` hypothesis yields nonsensical samples
  (Corners mean ≈ 40,268). CONCLUSION: `triple=(cum,lo,hi)` stride is right
  (terminator aligns), but WHAT the fields mean and the transform to a metric
  value is **UNKNOWN**.
- Likely the sampler applies `block0` (the per-metric quantile/transform grid,
  bucket0 = 9 symmetric breakpoints) and/or a further mapping; and the
  node-tree evaluator (FUN_00818120) may belong to the formula/constants path,
  NOT the `.dat` sampler. These are now the top open items.

Status split (honest):
- VERIFIED: file structure, record count, 12-byte triple stride, terminator,
  metric names, LCG constants.
- UNKNOWN: triple field meaning, block0 transform role, the actual `.dat`→metric
  value sampler, the attribute→context index.

The in-repo sampler (`cm_resolver_sample_group`) is therefore EXPERIMENTAL and is
NOT wired into the match engine. The synthetic unit test covers the inverse-CDF
*mechanism* only, not CM fidelity.
