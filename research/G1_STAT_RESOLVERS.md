# G1 — the match's stat-producing resolvers (the only 3 FUN_005cd020 callers)

All match STATS are produced by exactly three functions (image-wide, the only callers of the packed
2×5-bit stat counter FUN_005cd020). Everything else (the 9efaXX class handlers) is the 2D viewer.
Decompiled from the crack.

| resolver | size | 005cd020 bumps | LCG draws | role |
|---|---|---|---|---|
| FUN_0077c710 | 890 B | 2 (bits21-25 side A, 26-30 side B) | 4+ | the stat TALLY — repeated GEOMETRIC bumps to counters A/B with shares 0.5·A/(A+B), 0.5·B/(A+B); plus an aggregation mode. NOT a 1-winner duel — see DUEL_RESOLVER_GEOMETRIC_TALLY.md (Go `contest.Resolve` binary port is NOT 1:1) |
| FUN_00a029c0 | 2044 B | 3 | 9 | a complex multi-outcome EVENT resolver (3 sub-outcomes, 9 draws) — e.g. a goal-mouth/shot sequence |
| FUN_00a023f0 | 1360 B | 3 | 6 | the PENALTY SHOOTOUT resolver (ported shape in Go `contest.Shootout`) |

## Shared machinery (VERIFIED by call pattern)
- `FUN_005a9770` / `FUN_005a9750` (7 calls each in a029c0/a023f0): boolean STATE queries — read a
  condition/flag with a bit-arg (8/4/2/0x10…) before each sub-outcome branch.
- `FUN_005ccff0` / `FUN_005ce3e0`: resolve WHICH stat dword to bump (the counter address getters).
- `FUN_005cf2a0` / `FUN_005a6140` / `FUN_005a6910`: context/actor accessors (get the event's
  participants + stat block).
- Each resolver draws the global match RNG (0x19660d) per sub-outcome and compares against a
  probability, then bumps the selected counter — the same p=A/(A+B) shape as the duel, generalised to
  N sub-outcomes.

## What this nails down
- The gameplay/stat layer is **3 resolver atoms** (duel + a029c0 + shootout), each a short
  RNG-vs-probability sequence over real participant attributes. No other code writes match stats.
- So a faithful match = a SEQUENCE of these resolver atoms (not distribution sampling). The duel atom
  is ported; a029c0's exact 3-outcome arithmetic (the 9 draws → which 3 counters, with what
  probabilities) is the next port target, and the event GENERATOR (which atom fires when, between
  whom) remains the open G1 core.

## a029c0 mechanism — DECODED + SHARPENED (integer scale-and-compare gates) — VERIFIED
Full disasm (0xa02cb0–0xa0304c) shows each gate is the SAME `(draw*scale)>>32` integer-uniform idiom
as the PK resolver FUN_00a023f0 — NOT a float probability. Per gate:
```
ecx = [state]; ecx = ecx*0x19660d + 0x3c6ef35f; [state] = ecx   ; LCG next (draw)
[esp+0x34 or 0x38] = scale                                       ; scale operand
eax = draw; mul [scale]                                          ; EDX:EAX = draw*scale
value = EDX = (draw*scale) >> 32                                 ; integer uniform in [0, scale-1]
cmp value, threshold_byte ; jbe/ja                               ; threshold = a BYTE (attr-derived)
→ FUN_005cd020 bump counter 0/1
```
The `[esp+0x38]`/`[esp+0x34]` slots are a REUSED scratch for the `mul` SCALE operand (not a stored
probability). Observed scales across the gates: **0x7d=125** (same as the PK shootout), **0x7f=127**
(the 7-bit attr range), **0xa=10**, and two attribute-derived dynamic scales (ebp, eax). The
thresholds are BYTES read from stack slots ([esp+0x40], [esp+0x17], …), attribute-derived. So a029c0 =
a chain of `value=(draw*scale)>>32; value<=>threshold` gates (3 counter bumps), integer math
throughout — matching PK exactly, and the general stat-layer shape.

## Status
All 3 stat-resolvers' MECHANISMS are VERIFIED, and all use the SAME integer idiom
`value = (draw*scale)>>32; compare value vs a threshold` (NO float probabilities):
- PK shootout FUN_00a023f0: scale=125, threshold=50 (`value>50 → score`). VERIFIED, ported.
- a029c0: scales {125,127,10,dyn}, byte thresholds (attr-derived), 3 chained gates. VERIFIED mechanism.
- duel FUN_0077c710: a repeated GEOMETRIC tally with float shares 0.5·A/(A+B) (the ONE float-share
  resolver), see DUEL_RESOLVER_GEOMETRIC_TALLY.md — NOT the same integer idiom, and the Go binary port
  is NOT 1:1 (user-flagged, not ported).
Exact per-gate thresholds of a029c0 (which attribute → which byte) not all traced. Event GENERATOR
(which atom fires when/between whom) remains the large open core (~190-fn subsystem, see G1_MODEL.md).

## a029c0 condition flags — FUN_005a9770 / FUN_005a9750 (VERIFIED)
The a029c0 outcome-path selection (quick 0x40000/0x50000 vs the main 3-stage best-of-6) is gated by
EVENT-OBJECT condition bits, read via two tiny bit-test helpers (ecx = the event object):
- `FUN_005a9770(mask)` = `(([event+0x44] & mask) & 0x1ff) != 0` — tests the 9-bit flag field at
  event+0x44 (used on the "detailed" path, when `[ctx+0xc] & 0x1e000000 == 0x2000000`).
- `FUN_005a9750(mask)` = `((([event+0x40] >> 0x15) & mask) & 0x1ff) != 0` — tests bits 21+ of
  event+0x40 (the non-detailed path).
a029c0 reads them with masks 8/4/2/0x10/0x40/0x80/0x100 into local_29/28/cVar1/2a/26/27/25; these pick
the resolve path: cVar1 (bit2) / local_2a (bit0x10) enable the gates, local_28/29 drive the single-gate
0x40000/0x50000 outcomes, local_26/27 the balance guards. So the "which outcome kind" is set by the
event descriptor's own condition bitfield (+0x40 / +0x44); the exact real-world meaning of each bit is
the event-type definition (INFERRED). Completes the a029c0 decode.
