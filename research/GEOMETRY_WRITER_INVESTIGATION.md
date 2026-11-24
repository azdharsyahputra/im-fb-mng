# Geometry writer + waypoint source — evidence investigation

Static investigation of the four unresolved items, from the committed dumps (per-module +
`docs/ghidra_dumps/big/CM2008.exe_big.c`, a 150-function curated decompile spanning
0x6f15f0–0xae7600 — NOT whole-image). No game run, no DRM. Implementation-ready for the Lab
(`super-manager-engine/geometry` evidence model / interfaces).

Classification scale (as requested):
`NOT_INVESTIGATED` · `LOCATED_NOT_UNDERSTOOD` · `DECOMPILED` · `STRUCTURALLY_UNDERSTOOD` ·
`BEHAVIORALLY_VALIDATED` · `GENUINELY_UNAVAILABLE`.

---

## 1. Continuous movement integrator (writer of geometry +0xC0 / +0xD0)

**Result: the match geometry-entity writer is NOT in any available dump.** Classification:
**LOCATED_NOT_UNDERSTOOD** (region known = threaded-sim worker; no body dumped). NOT
GENUINELY_UNAVAILABLE — it is un-dumped, not protected.

- Whole-image-ish scan of `big/CM2008.exe_big.c` for `*(int*)(reg+0xc0)=`/`(reg+0xd0)=` finds INT
  writes only in the **UI/object-setup cluster**: `FUN_00977620` (@0x977620, lines 29529/29613:
  `*(int*)(in_ECX+0xc0)=FUN_00617170(); *(int*)(in_ECX+0xc4)=FUN_00617160()`) and `FUN_00978f40`
  (@0x978f40, lines 36881/36888) — both adjacent to `FUN_008f90f0` (the VERIFIED **UI interpreter**,
  not the match engine). These write a DIFFERENT object's +0xC0 (widget geometry), **ruled out** as
  the pitch-coordinate writer.
- The match geometry entity (read by the projection funcs `FUN_009a89b0`/`FUN_009aa250` via
  `fild [obj+0xd0]`, `fild [obj+0xc0]`) is written elsewhere: `0x9aa250` is only *called* in the
  big dump (line 58722), its body absent; `0x9a89b0` absent entirely. The 0x9b–0x9f sim-geometry
  functions present in the dump are resolver/predicate (`009e9570`, `009e43b0`, …), not the writer.
- **Representation (answered): INTEGER fixed-point.** The read side is `fild` (integer load) ×
  1/112.5 — VERIFIED (BEHAVIORALLY on the read path). No floating-point position field exists; and
  the recovered motion is set-to-target waypoints, so there is likely **no separate vx/vy field**
  (the earlier vx/vy hunt found none) — INFERRED.
- **Unresolved dependency:** the writer's address. Recipe to get it (not yet run): byte-scan the
  image for `fistp`/`mov [reg+0xc0]`/`[reg+0xd0]` INTEGER stores in the 0x9bxxxx–0x9fxxxx range
  (the sim-geometry cluster), or locate the match worker-thread entry (item 3) first and dump its
  loop. Hand the dumped body to `geometry.Integrator` (replacing `UnknownIntegrator`).

## 2. Waypoint source — target-picker `FUN_00a1fdf0`

**Result: body present and STRUCTURALLY_UNDERSTOOD.** (`big.c` #55 @0x00a1fdf0, 3624 B, 81 callees,
1 caller.)

- **Inputs (reads):** match roster via `+0x3c`/`+0x3a` strides; named player attributes
  `Acceleration, Agility, Anticipation, DecisionMaking, Positioning, WorkRate` (and more) — read by
  name, not assumed. R: roster base, attribute getters.
- **Outputs (writes):** a **0x14 (20-byte) stride candidate list** — each entry
  `*(obj + i*0x14) = u32 (coord A)`, `*(obj + 4 + i*0x14) = u32 (coord B)`, `*(obj + 0x1c + ...) = 0`
  (reset/flag), with a running count at `obj+8`. Also higher-level 0x44-stride per-player
  assignment records (value@+0x4c, active-flag@+0x48, counters@+0x50/+0x98/+0xe0…).
- **Interpretation:** this is **player-AI / movement positioning** — it consumes attributes +
  teammate/opponent geometry and emits per-player candidate target lists. The target COORDINATES are
  the first two u32 of each 0x14-stride entry (INFERRED: which is x vs y, and their world-coord
  frame, is **LOCATED_NOT_UNDERSTOOD**).
- **Caller/callee:** 1 caller (via the `FUN_00724790` movement trampoline per prior docs — body not
  in this dump); 81 callees (attribute getters + list builders).
- **Unresolved dependency:** the exact offset within the 0x44 record that the integrator consumes as
  the next waypoint, and the x/y assignment. Enough to implement a `TargetProvider` that returns the
  0x14-stride entry pair once that x/y mapping is pinned (one more targeted trace).

## 3. Threaded simulation

**Result: LOCATED_NOT_UNDERSTOOD** (worker entry not in any dump).

- `GCMatchModule::Update/ProcessData` (`FUN_0077ee80`, VERIFIED) branch (a) iterates two fixture
  lists and logs `"End of fixture %d that ran on a thread."` → fixtures are simulated on worker
  threads; branch (b) is the per-tick chain `ccb0→cd40→e850→cf20→c660` (VERIFIED).
- No `_beginthreadex`/`CreateThread` appears in the dumps (CRT thread-start not in the seeded
  closure). Scheduling: ProcessData pumps per program-step; the geometry writer runs in the worker
  (item 1); event processing is cd40 (queue) → cf20 (execute) within the pump. Synchronization
  primitives: NOT_INVESTIGATED (no body).
- **Unresolved dependency:** the thread-creation site + worker loop body (byte-scan for the match
  module's `_beginthreadex` call, then dump the worker). This also yields item 1's writer.

## 4. Bridge enter-handler (`0x6dd000` cluster)

**Result: LOCATED_NOT_UNDERSTOOD** (byte-located; undefined in the unpacked DB).

- The per-state `eGameState` enter-handlers (0x6dd000–0x6ed000, 29 `mov [ecx+0x30],<state>` sites,
  STATE_WRITER_FOUND.md) are **undefined functions** in `cm2008_full` (re-export returned
  "NO FUNCTION"). The `cf20 → 009efa40/efa70 → …(exec consumer)… → 0x6dd000` bridge is therefore
  **INFERRED**, not decompilable from this DB.
- **Unresolved dependency:** define/analyze the 0x6dd000 module in Ghidra (it was never auto-analyzed
  because seeding came from the 0x77xxxx module), then the exec-consumer→enter-handler edge is
  readable.

---

## Priority summary (what to get next, in order)
1. **Locate the match worker-thread entry** (`_beginthreadex` in the match module) → dump its loop
   → yields BOTH the geometry integrator writer (item 1) and the real event cadence.
2. **Pin the target x/y offset** inside `FUN_00a1fdf0`'s 0x14/0x44-stride output → a real
   `TargetProvider`.
3. Analyze the 0x6dd000 cluster to confirm the enter-handler bridge.

Items 1 & 3 are un-dumped regions of an unpacked DB (static-recoverable with more Ghidra passes),
NOT protection boundaries. The SecuROM picker `0x5b37xx–0x5b52xx` remains the only runtime-only
piece.
