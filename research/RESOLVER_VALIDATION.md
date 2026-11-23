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
