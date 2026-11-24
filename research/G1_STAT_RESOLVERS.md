# G1 — the match's stat-producing resolvers (the only 3 FUN_005cd020 callers)

All match STATS are produced by exactly three functions (image-wide, the only callers of the packed
2×5-bit stat counter FUN_005cd020). Everything else (the 9efaXX class handlers) is the 2D viewer.
Decompiled from the crack.

| resolver | size | 005cd020 bumps | LCG draws | role |
|---|---|---|---|---|
| FUN_0077c710 | 890 B | 2 (bits21-25 side A, 26-30 side B) | 4 | the DUEL — p=A/(A+B) between two contestants' 7-bit attrs (ported in Go `contest`) |
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

## Status
3 stat-resolvers identified + their draw/counter/helper structure — VERIFIED. a029c0's exact outcome
arithmetic — LOCATED, not yet fully decoded. Event generator — still open.
