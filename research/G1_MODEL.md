# G1 consolidated model — the causal match tick (what's recovered, what's open)

Consolidates G1 passes 1–4 + the parallel web decompile (dispatcher, activate, handlers, descriptor,
accessors). All VERIFIED from the crack unless noted.

## The tick (FUN_0077ee80 ProcessData)
Orchestration: `d800`(std::map find) → `cf20`(activate) → `ccb0`(gate) → `cd40`(dispatch) →
`e850`(boundary/GameOver) → pending-duel cluster → `c660`(clock H:MM) → `cf20`. The pump iterates
per-entity ACTOR records (stride 0x2768, 9-bit id), calling `cd40` per record gated by the pending
bit `module+0x18 & 0x1000000`.

## The event descriptor (0x20 B; array @ [0xc784a8]+0x2c, id=index) — VERIFIED (agent C)
- `+0x00/+0x02` u16 = contestant[0]/[1] handles → contestant tbl 0xc779ac → PERSON tbl 0xc77778 →
  **person+0x3c bits12..18 = the 7-bit match attribute** used by the duel.
- `+0x04` = class / action-state field; `cd40` reads bytes `+4/+5/+6` to ROUTE (see below). Caller
  masks `&0xffff3fff | 0x3000`.
- `+0x08` = packed event handles → EVENT obj (tbl 0xc77df8, 0xb8) whose `+0x10 bits20..31 = event
  TYPE id`.
- `+0x0c/+0x10` = further linked-object handles.

## Dispatch routing (FUN_0077cd40) — VERIFIED (pass 1)
```
if desc.flag6: handler 9efa10 (class 3)      // PRESENTATION
elif gate & (desc.flag5 | global): 9efa20 (class 1)   // PRESENTATION
elif desc.flag4: DUEL FUN_0077c710            // ← the only STATS path
else: 9efa20 (class 0)                         // PRESENTATION
// duel path sets module+0x18 |= 0x1000000 (pending); cf20 clears it
```

## Handlers are PRESENTATION, not gameplay — VERIFIED (agent B)
The 6 class handlers (9efa10/20/40/70, 9efc30/90) are 8-byte thunks to a match-VIEWER layer
(highlight/scene/commentary/graphics-teardown). They bump NO stat counters. Image-wide, the stat
counter `FUN_005cd020` (packed 2×5-bit) has **only 3 callers**: the duel `FUN_0077c710`, the penalty
shootout `FUN_00a023f0`, and `FUN_00a029c0`. So **all match STATS come from the duel (+ shootout +
a029c0)**; classes 0/1/3 are the 2D viewer.

## The stat-tally atom FUN_0077c710 — VERIFIED (mechanism CORRECTED)
Reads the two contestants' 7-bit attrs (person+0x3c), and in its stochastic mode (FUN_005ccf30 < 2)
runs a **repeated GEOMETRIC tally**: draw u=lcg.Unit(); while u<shareA bump counter 0 (side A), then
while u<shareB bump counter 1 (side B), with shareA=0.5·A/(A+B), shareB=0.5·B/(A+B). It also has an
aggregation mode (FUN_005ccf30 ≥ 2) that sums existing 5-bit counter fields. It is **NOT** a single
p=A/(A+B) winner — the Go `contest.Resolve` binary port is NOT 1:1. See
DUEL_RESOLVER_GEOMETRIC_TALLY.md (supersedes DUEL_THREE_OUTCOME_FINDING.md).

## OPEN CORE (what's NOT yet recovered)
1. **Event GENERATION / selection**: who creates descriptors in the pool and sets the event TYPE
   (desc+8 → event-obj type id) + the class flags desc+4/5/6 — i.e. what decides "a tackle/shot
   between players X and Y happens now", from game state/positions/tactics. The class flags look
   DERIVED from the event type, so the real question is who generates events of each type. This is
   the deepest layer (the match AI) and is still UNKNOWN. (Agent A's hunt failed on a network error.)
2. **FUN_00a029c0** — the 3rd stat-bumper (besides duel + shootout) — not yet decompiled.

## Implication for a 1:1 engine
Stats are produced ONLY by the duel/shootout atoms over real player attributes — so a faithful engine
is a SEQUENCE of duels, not distribution sampling. The missing piece is the event GENERATOR (what
sequence of duels/events the match produces). Porting the duel atom is done; porting the generator is
the open G1 core.
