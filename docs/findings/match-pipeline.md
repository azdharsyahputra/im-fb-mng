# Finding: Original CM2008 match-simulation pipeline (dependency graph)

**Status:** reverse-engineered (call-graph) · **Confidence:** Observed (static call graph)
**Evidence:** call graph built from `docs/ghidra_dumps/match_ai/match_movement_ai.c`
(765 decompiled functions) + per-function caller/callee counts.

## Update dispatcher — `FUN_008f90f0`

The match update core. In-cluster fan-out = 121 (135 direct callees total); called
by only 11. It is the top of the simulation pipeline: one function that drives the
per-update subsystem calls. Reachability from it = **318 functions**.

Region distribution of the reachable set (where the match code lives):

| region | funcs | role |
|---|--:|---|
| 0x008f_xxxx | 46 | match-engine core (dispatcher's own block) |
| 0x008e_xxxx | 40 | match-engine core (adjacent block) |
| 0x0040_xxxx,0x0041_xxxx | 37 | CRT/allocator/string primitives |
| 0x009d_xxxx,0x0090_xxxx | 31 | containers / vector ops |
| 0x007e_xxxx,0x0072_xxxx | 20 | data access + helpers |

→ The **match-engine proper is the ~86 functions in 0x008e_xxxx–0x008f_xxxx**,
driven by FUN_008f90f0.

## Second tier (subsystem drivers, called by the dispatcher)

| addr | size | in-cluster callees | likely role (by neighbourhood/refs) |
|---|--:|--:|---|
| FUN_00a1fdf0 | 3624 | 66 | positioning/movement driver (seed: positioning|movement|attacking) |
| FUN_009aecc0 | 2757 | 36 | offside/defensive-phase driver (seed: offside) |
| FUN_00729660 | 5800 | 22 | large sub-step (0x0072 data/logic block) |
| FUN_008f7f10 | 2226 | 19 | match sub-update |
| FUN_008f6540 | 724 | 21 | match sub-update |

## Data-driven access path (how attributes/tactics/distributions feed the sim)

Two getters dominate fan-in and are called by **42** match functions:
- `FUN_00662cf0` — data-source field lookup (resolves a named field in a data
  source such as "PlayerData"); 42 callers.
- `FUN_00818120` — typed value reader (reads the field's value given the handle);
  42 callers, 3 callees.

This is the single mechanism by which the match engine reads **player attributes,
tactical sliders, and the MatchResolver distributions** — all accessed as named
fields through the same getter pair (confirmed by the accessor pattern in
FUN_0083b860 "AssistsPerformance" / FUN_0083ea10 "MarkingDistanceScale"). The
match engine is therefore **data-driven end to end**: behaviour = generic logic +
data read through FUN_00662cf0/FUN_00818120.

## Function-level traceability (anchors established)

| mechanic | original function(s) | status |
|---|---|---|
| match update dispatcher | FUN_008f90f0 | located (algorithm not fully traced) |
| positioning/movement driver | FUN_00a1fdf0 | located |
| offside/defensive driver | FUN_009aecc0 | located |
| event/axiom registry | FUN_008a7770 | decompiled (names/taxonomy) |
| predicate evaluator | FUN_009e8f60 / 009e92b0 / 009e9200 | decompiled (recursive tree mechanism) |
| data field lookup | FUN_00662cf0 | located (consumer of all data) |
| typed value reader | FUN_00818120 | located |
| tactic getter pattern | FUN_0083ea10 (MarkingDistanceScale=0.02) | reconstructed 1:1 |
| resolver metric getter pattern | FUN_0083b860 (AssistsPerformance) | pattern confirmed |

## Not yet traced (honest)

- The exact per-update **call sequence** inside FUN_008f90f0 (which sub-driver
  runs in what order; tick frequency; half/full-time handling) — needs reading
  the dispatcher body, which is dense/inlined.
- How FUN_00a1fdf0 turns attributes+tactics into target positions (the movement
  algorithm) — located, not decoded.
- How the predicate tree connects to event emission and to the resolver sampling.
