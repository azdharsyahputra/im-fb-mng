# GAMEPLAY_1TO1_TASKS.md — the CAUSAL 1:1 gameplay workstream

The component/data layer is faithful (real distributions + recovered math: LCG, picker, duel
p=A/(A+B), formula evaluator, record layouts, 10s cadence). What is NOT yet 1:1 is the **causal
gameplay engine** — the logic that makes CM2008 play like CM2008. These are large tasks (essentially
porting the match AI + tactics + macro). Honest scope flags below. Work them in order.

## G1 — Causal match-engine logic (the per-event decision tree)  [~] atoms mapped; generator is a huge subsystem
PROGRESS (VERIFIED, research/G1_*.md): the tick SKELETON (ProcessData orchestration, pump loop over
0x2768 per-entity records, dispatcher cd40 routing by desc+4/5/6, activate cf20, clock+cadence), the
descriptor model (0x20B, 2 contestants→7-bit attrs, event-type handle), and that the 9efaXX class
handlers are the 2D VIEWER (presentation, no stats). All match STATS come from 3 resolver ATOMS —
duel FUN_0077c710 (ported), FUN_00a029c0 (3-stage sequential gate), penalty shootout FUN_00a023f0 —
each a `draw×prob>>32` gate over real participant attributes. Mechanisms understood.
OPEN (the hard core): the event GENERATOR — which atom fires when, between whom, from game state/
tactics. The event-object pool is touched by ~190 functions (hub FUN_006bee40); this is a large
multi-week port, NOT an incremental find. Without it the atoms can't be assembled into a causal match.
DECISION NEEDED (user): commit to the big generator port, or keep realstats (1:1 output) as canonical
and treat the causal generator as out of scope.
`realstats` SAMPLES the output distributions; it does NOT run the original's per-event decision tree.
G1 = recover and port the real per-tick logic so the engine DECIDES which events happen (from state,
not from an aggregate draw).
- Entry: ProcessData FUN_0077ee80 → dispatcher FUN_0077cd40 (per-tick; event-record flags [ebx+4/5/6];
  global RNG draw @0x77cd84) → event resolvers (duel FUN_0077c710, FUN_0077be30, …) → stat counters.
- Deliverable: a decompiled decision tree (what cd40 reads, how it picks/sequences events) → a native
  port that produces a match causally, with `realstats` distributions as the validation target.
- Scope: LARGE. Expect multiple passes (dispatcher → each resolver → the state it reads).

## G2 — Micro: tactics/positioning inputs to the decision tree
How the engine uses formation, player positions, roles, and the 14 "…OrX…" tendency fields (cols
63–76) to weight event selection/outcomes. Depends on G1 (the tree) + A1 (named attributes).
- Scope: LARGE. Needs the attribute binding (A1, coordinator) to feed real tactic inputs.

## G3 — Tactics & AI: formations, roles, team instructions, opponent manager AI
The manager-level decisions (team selection, formation, in-match instructions, opponent AI). Mostly
unexplored; separate subsystem from the match tick.
- Scope: VERY LARGE / mostly UNKNOWN. Later.

## G4 — Macro: transfer / finance / training / morale / scouting / board
Native packages exist (management/finance/transfer/scouting/league/…) but they are RECONSTRUCTIONS/
approximations, not verified 1:1 ports of the original logic. G4 = verify each against the real
routines and close the gaps.
- Scope: LARGE, per-subsystem. Later.

## Honest status line
Done (1:1 components + faithful OUTPUT distributions): P1–P7, realstats canonical engine, 77 attribute
names. NOT done (causal gameplay): G1–G4. Full micro+macro gameplay is NOT yet copied — these tasks
are that copy. Starting G1.
