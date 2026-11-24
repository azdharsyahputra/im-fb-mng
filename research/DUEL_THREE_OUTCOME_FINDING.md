# Finding: the real duel FUN_0077c710 is a 3-OUTCOME resolver (not binary) — contest.Resolve likely not 1:1

## What the crack shows (FUN_0077c710 @0x77c88a–0x77c9a0, VERIFIED disasm; semantics INFERRED)
The duel computes TWO shares from the two contestants, each scaled by the 0.5 scalar (DAT_0xbbb51c /
0xc31b38 = 0.5), using the 2^32 unsigned→float fixup (0xb3e9a8 = 4.29497e9):
- shareA, shareB ≈ 0.5 × A/(A+B) and 0.5 × B/(A+B), stored at [esp+0x18]/[esp+0x1c], then ×0.5 again
  into [esp+0x14]/[esp+0x20].
- A single uniform draw is compared against BOTH thresholds (fcomp [esp+0x20], then fcomp [esp+0x14]):
  - one side → `push 0; call FUN_005cd020` (counter 0 = side A)
  - other side → `push 1; call FUN_005cd020` (counter 1 = side B)
  - the MIDDLE band → neither counter bumped = a NEUTRAL outcome (stalemate / loose ball).

So the duel has THREE outcomes: A-win / NEUTRAL / B-win. For equal ratings (A=B) the win shares are
symmetric (~0.25 each) with a ~0.5 neutral band → 25% / 50% / 25%.

## The Go port (contest.Resolve) — likely NOT 1:1
`contest.Resolve` is BINARY: `shareA = scalar*A/(A+B); return unit<shareA ? 0 : 1`. For A=B with
scalar=0.5 this gives A-win 25%, B-win 75% — ASYMMETRIC and with NO neutral outcome. That contradicts
the real duel's symmetric 3-outcome shape. So the binary model is probably a simplification, not 1:1.

## Status — NOT fixed (deliberately)
Flagged, NOT changed. The exact comparison direction uses the x87 `fnstsw; test ah,5; jp/jnp` parity
idiom, which is easy to misread from static disasm alone; and contest.Resolve has several callers
(matchsim, events, possession). A wrong "fix" would corrupt the engine. The honest next step is to
EMULATE FUN_0077c710 on sample (A,B) inputs (or single-step it) to pin the exact 3-outcome thresholds
and the neutral-band rule, THEN port a 3-outcome contest.ResolveDuel and migrate callers. Confidence:
shares/scalars/two-threshold structure VERIFIED; the symmetric 25/50/25 reading INFERRED (needs
emulation to confirm the compare directions).
