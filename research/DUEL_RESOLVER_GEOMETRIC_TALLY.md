# FUN_0077c710 — CORRECTED mechanism: a repeated GEOMETRIC stat tally (not a 1-winner duel)

**Supersedes `DUEL_THREE_OUTCOME_FINDING.md`.** That earlier note read the x87 branches as a single
3-outcome (A / NEUTRAL / B) winner. A full instruction-level trace of the whole function (0x77c710–
0x77c9ce, VERIFIED disasm; float constants read from the image) shows that reading was **wrong**, and
so is the Go port's binary `p=A/(A+B)` winner. The real function is a **repeated-draw geometric tally
into two saturating 5-bit counters**, with a second (aggregation) mode.

## Float constants (read from the crack, VERIFIED)
| VA | value | role |
|---|---|---|
| 0xbbb51c | 0.5 | share scalar S |
| 0xc31b38 | 0.5 | share scalar S (second copy, multiplied in) |
| 0xb3e9a8 | 4.2949673e9 = 2^32 | unsigned→float fixup for (A+B) |
| 0xb18024 | 1.0 | normaliser: u_norm = u_bits − 1.0 |

## The two 7-bit inputs (VERIFIED)
`ebx`, `ebp` = the two contestant records (resolved via 5a6170→593cc0). 
- A = `(*(u32)(ebx+0x3c) >> 12) & 0x7f`  (0..127)
- B = `(*(u32)(ebp+0x3c) >> 12) & 0x7f`

## The shares (VERIFIED, 0x77c887–0x77c90b)
If A+B == 0: shareA = shareB = 0.5 (defaults, no division).
Else:
- shareA = **0.5 · A/(A+B)**   → stored at [esp+0x20]
- shareB = **0.5 · B/(A+B)**   → stored at [esp+0x14]

(The division uses the 2^32 fixup only if (A+B) were negative — it never is, A,B ≤ 127.) So at A=B,
shareA = shareB = 0.25.

## Mode split (VERIFIED, 0x77c7df–0x77c7e6)
`al = FUN_005ccf30(edi)`; `cmp al,2`:
- **al < 2 → 0x77c887**: the PROBABILITY-DRAW tally (below). This is the stochastic path.
- **al ≥ 2 → 0x77c7ec**: an AGGREGATION outer loop (below). No fresh RNG; it sums existing counters.

## Probability path (al<2) — the geometric tally — VERIFIED
Uniform per draw: `u_bits = ((state*0x19660d+0x3c6ef35f) >> 9) | 0x3f800000` (a float in [1,2)),
then `u = u_bits − 1.0` ∈ [0,1) — i.e. exactly `lcg.Unit()`.

Phase A (counter 0 = side A), 0x77c91b–0x77c969:
```
draw u; if u >= shareA: goto PhaseB            ; 0 bumps
loop:   bump counter0 (FUN_005cd020 arg 0)     ; push 0; ecx=edi; call 0x5cd020
        draw u; if u <  shareA: goto loop       ; jnp 0x77c930  (keep bumping)
        else: goto PhaseB
```
Phase B (counter 1 = side B), 0x77c96b–0x77c9cc: identical structure with shareB, bumping
`FUN_005cd020(arg 1)`, `jnp 0x77c995`.

So each call adds to counter 0 a **Geometric** count `k_A` with P(k_A = k) = shareA^k·(1−shareA)
(number of consecutive opening draws below shareA), and independently to counter 1 a Geometric `k_B`
with parameter shareB. Means: E[k_A] = shareA/(1−shareA), E[k_B] = shareB/(1−shareB).
At A=B (shareA=shareB=0.25): E[k_A] = E[k_B] = 0.25/0.75 = **1/3** each — symmetric, and NOT a single
winner. The counters saturate at 5 bits (FUN_005cd020 caps; see G1_STAT_RESOLVERS.md).

## Aggregation path (al≥2) — VERIFIED shape, semantics INFERRED
0x77c7ec–0x77c877: outer loop `[esp+0x28]` from 0 to 1 (runs twice). For each record `esi` walked via
5cdcf0/9fdd60, it reads the packed counter at `[esi+0x38]`: three 5-bit fields
(`>>0x1b &0x1f`, `>>0x16 &0x1f`, `[esi+0x1b] &0x1f`), sums them into `dl`, and calls FUN_005cd020
`dl` times (`add bl,1; cmp bl,dl; jb`). I.e. it **copies/accumulates** already-tallied 5-bit counts
onto the target counter `edi`. No RNG. (Role: roll up per-record stat counters into an aggregate —
e.g. summing per-minute tallies into a match total. Exact target semantics INFERRED.)

## What this corrects
1. **The Go `contest.Resolve` is not 1:1.** It returns a single binary winner (`u<shareA ? A : B`,
   asymmetric 25/75 at A=B). The original produces a *pair of geometric event counts* (symmetric 1/3,
   1/3 at A=B) that feed saturating stat counters — a different distribution and a different output
   type (counts, not a winner).
2. **My earlier `DUEL_THREE_OUTCOME_FINDING.md` (3-outcome A/NEUTRAL/B) is also wrong.** The "middle
   band" I inferred is actually the *stop* condition of a repeat loop, not a neutral outcome; and the
   second compare is the loop's continue-test, not a third branch.

## VERIFIED vs INFERRED
- VERIFIED (instruction-level): the float constants; A/B extraction; shareA/shareB = 0.5·A/(A+B),
  0.5·B/(A+B); the uniform = lcg.Unit(); the geometric repeat-bump structure of both phases; the
  mode split on FUN_005ccf30; the aggregation loop's 5-bit field extraction + sum-bump.
- INFERRED: that counters 0/1 are specific match stats; the gameplay meaning of the two modes and of
  the aggregation target; whether a caller invokes this once per event or per tick.

## Status — PORTED ✅ (super-manager-engine commit 47aa8b2)
The faithful 1:1 atom is now `contest.TallyDuel` (+ `DuelShares`, `MaxTally`): two independent
`for lcg.Unit() < share { count++ }` loops over shareA/shareB, each saturating at 5 bits (31) while
the loop keeps consuming draws (RNG-stream faithful). VERIFIED against the derived geometric
distribution — A==B: E[countA]=E[countB]=1/3 symmetric, P(count=0)=0.75 (measured 0.333/0.335/0.751);
100 vs 20: E=0.714/0.091 (measured 0.717/0.090); determinism + cap tested; `match duel` CLI demos it.
The old binary `contest.Resolve` is relabelled a clean-room projection for the SIDELINED narrative
engines only (not the 1:1 atom); its callers are unchanged and the canonical realstats engine never
used it. OPEN (not ported, lower value): the aggregation mode (al≥2, sums existing 5-bit fields) —
documented in G1_STAT_RESOLVERS.md; no caller context for it yet.
