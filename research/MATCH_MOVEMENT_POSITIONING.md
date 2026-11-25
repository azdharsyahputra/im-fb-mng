> ⚠️ RETRACTED IN PART by MATCH_MOVEMENT_PHYSICS.md: the "no per-second movement sim / view-only"
> conclusion is WRONG. CM2008 HAS a physics movement model for 22 players (speeds/acceleration/turning/
> diving/sliding), configured by the MatchResolver group in the decrypted formula.xml. Keep this doc for
> the stat-atom / FieldPosition findings, but read the physics doc for the movement truth.

# Movement / positioning — is CM2008 a per-second 22-player positional sim? (NO — VERIFIED)

Question: does the match engine simulate 22 players moving each second (positions driving outcomes),
or is it an event/attribute/zone model with the 2D view as cosmetic animation? Investigated with the
decompiler. Answer: **event/attribute/zone model; the 2D movement is a separate view layer that does
NOT feed the stat engine.**

## Evidence

### 1. The stat engine never reads positions — only attributes (VERIFIED, from the atom ports)
All match stats come from the 3 resolver atoms (duel FUN_0077c710, best-of-6 FUN_00a029c0, penalty
FUN_00a023f0). Every one reads the participants' **7-bit attributes** (person+0x3c >>12 &0x7f) and the
global LCG — never an (x,y) position. The dispatcher FUN_0077cd40 routes on the event's class flags
(desc+4/5/6), which are the resolver's own OUTCOME, not spatial state. So outcomes are
attribute+event-driven; no coordinate is an input to any resolver.

### 2. FieldPosition.dat is a position-DEFINITION table, not coordinates (VERIFIED)
`DATa`, count 268, recsize 32. Each record = an id/hash u32 + several small integer fields
(0/2/3/5/7/10/13…). These are position ROLE/type definitions (268 of them — role variants/adjacency),
NOT per-player x/y pixel coordinates and NOT a per-tick movement grid.

### 3. The 2D viewer is a separate presentation layer (VERIFIED)
The class handlers 9efa10/20/40/70 are 8-byte thunks into the match VIEWER:
- `FUN_009efa20 → FUN_009f9400` (scene setup): operates a huge view context (in_ECX + 0x10000+ bytes,
  ~64 KB), sets `__controlfp`, assigns team colours (DAT_00c67eb0..ec0), and calls per-side
  `FUN_009f7f00(side 0/1)`.
- `FUN_009f7f00` (per-side view update): maintains per-side/per-player VIEW records (handles, 0x164 and
  0x5c-stride view structs at DAT_00c801e8/DAT_00c800e8), using the same null sentinels (0x100000/
  0x1fffff/0x4000/0x7fff) as the engine, then calls sprite/scene functions (009f3220/009f4180/
  009f2e70/009efe40/009f28a0). It animates the scene from event/handle state; it does not compute
  player trajectories that feed back into any resolver.

## Honest conclusion
CM2008's match is a **statistical event engine over player attributes and a 3-zone (Attack/Midfield/
Defence) abstraction**, not a continuous per-second 22-player positional/physics simulation. The 2D
match view animates players as a PRESENTATION of the resolved event stream; those on-screen positions
are view-layer state and are never read by the stat resolvers. Therefore:
- There is no per-second movement model to port into the engine — the gameplay-determining layer is
  the attribute/zone/event system, which the `realstats` (output) and `match causal` (rate-anchored +
  atom-resolved) engines already capture.
- "22 players moving every second" is a property of the VIEWER, not the gameplay sim.

## Not exhaustively traced (honest scope)
The exact sprite interpolation inside the view layer (whether the view stores continuous x/y and how it
tweens dots between events) was not fully decompiled — it is cosmetic and irrelevant to outcomes, so it
does not affect the reconstruction. The precise pump ACTIVATION cadence (how often an entity fires an
event) was also not pinned to a constant; it is unnecessary because the engine anchors event COUNTS to
the real output distributions rather than re-deriving the tick rate.

## Correction (from MATCH_TACTICS_FORMATIONS.md): FieldPosition.dat IS a pitch grid
A later tactics pass showed the 268×32 FieldPosition.dat records are `id + namehash + 3×(Column,Row)`
= base / attack-phase / defend-phase grid CELLS (GK id=1=(0,7); Column=depth, Row=width; -1 sentinel).
So positions ARE grid cells per formation phase — but they are STATIC formation slots (base/attack/
defend), NOT per-second continuous trajectories, and the stat resolvers still read only the condensed
rating (STAT_GAMEPLAY_LINKAGE.md), not these cells. The "no per-second 22-player movement sim"
conclusion stands; the "not coordinates" wording is corrected to "static formation grid cells, 3 phases".
