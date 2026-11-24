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

## a029c0 mechanism — DECODED (3-stage sequential probability gate)
Each of its 3 sub-outcomes is: `FUN_005ce3e0` (get the target stat dword) → `imul 0x19660d` (LCG draw)
→ `mul [esp+0x38]` (scale the draw by a probability value held at [esp+0x38]) → compare → `FUN_005cd020`
(bump counter 0 or 1). So a029c0 = the duel's `draw×prob>>32` gate, chained for 3 sequential outcomes.
The probability `[esp+0x38]` is computed earlier from the participants' attributes (same A/(A+B)
family). Portable shape: a sequence of probability gates over real attrs — not distribution sampling.

## Status
All 3 stat-resolvers' MECHANISMS are now understood (VERIFIED): duel = 2-outcome gate; a029c0 =
3-stage sequential gate; shootout = kick gates. Each is `draw×prob>>32` over real participant attrs.
The per-gate probability sources ([esp+0x38] etc.) are attribute-derived (exact formula per gate not
all traced). Event GENERATOR (which atom fires when/between whom) remains the large open core (~190-fn
subsystem, see G1_MODEL.md).
