# Movement continuous geometry — located (access path VERIFIED; exact x/y offsets pending)

The one genuine remaining gap (player position/velocity/acceleration numeric offsets) is
now narrowed from "an unknown buffer" to a precise per-entity block + access path.

## Verified access chain (disassembly)
The player position/velocity does NOT live in the small 32-byte pool-A entity record
(that exposes only +0x00 type/state and +0x02 ball handle). The continuous geometry is a
PER-ENTITY 253-dword block, reached as:
```
id     = *(DAT_00c784a8 /*player pool*/ + 0x3004c)   ; a period/frame object id (scalar,
                                                       ; compared vs entity+0x98; NOT the buffer)
period = FUN_009efa90(id)                             ; resolve id -> period object
src    = FUN_009efb70(period, *(entity+0x134))        ; per-entity geometry source pointer
```
`FUN_009b54b0` then does `mov ecx,0xfd; rep movsd` — copies **253 dwords = 1012 bytes**
from `src` into a stack buffer, then processes it (FUN_009efb80 / FUN_009efba0 / FUN_009b41e0).
So each entity's continuous geometry is a 1012-byte block keyed by `entity+0x134`, held
under the `[pool+0x3004c]` period object.

VERIFIED: the block size (253 dwords), the key (entity+0x134), the resolver functions
(009efa90/009efb70), and that `[pool+0x3004c]` is a scalar id (not the buffer).

## What is still UNKNOWN (not invented)
The exact dword indices WITHIN the 1012-byte block that hold x, y, velocity, acceleration,
and facing. A3's emulation showed positions are INTEGER fixed-point (a lone `fild` on a
view coord) and that the projection scalers (56.25 / 112.5 / deg2rad) are referenced from
UI/camera/overlay code (presentation), not the sim's world-coord reader. The dwords are
consumed by (a) the view-build cluster that reads the copied stack buffer in FUN_009b54b0's
continuation (0x9b41e0 …) and (b) the continuous integrator, which is in the threaded sim
(writes the block) — not in any dump so far.

## Chase result — the read side is thin forwarders; the writer is the threaded sim
Followed the processing chain: `FUN_009b41e0 -> FUN_009cffa0` (`mov ecx,[ecx+0x148]; jmp`)
`-> FUN_009efaa0` (`mov ecx,[ecx+4]; jmp`) `-> FUN_009f0d30 ...` — a cascade of thin
`__thiscall` forwarders in the 009ef accessor family, each just re-deref + tail-jump. A
scan of the entire view/geometry cluster (0x9b0000–0x9d0000) for fixed-point reads
(`fild`/`fistp`) found essentially NONE (a couple at +0x34/+0x70/+0xe4) — i.e. the
world-coordinate conversion is NOT in the view code. So the 1012-byte block is POPULATED by
the continuous integrator in the worker-thread sim (the block's producer); the view side
only forwards/reads pre-computed values.

Therefore the exact x/y/velocity dword indices are in the THREADED sim writer, which no
module-seeded dump has reached (it is not protected — just never located). The two ways to
get them:
1. Static: find the match worker-thread entry (the `_beginthreadex`/`CreateThread` site in
   the match module) and dump the sim loop that writes the 1012-byte block — then the store
   offsets are the field indices.
2. Runtime: a write-watchpoint on the block in a legit unmodified copy names them directly.

This is the ONLY match behavior still < "Algorithm recovered". Everything else (lifecycle,
state-writer, resolver, picker, formula engine+trees, predicate weights, contest/shootout,
match-loop) is recovered/implemented. NOT a protection boundary — a not-yet-dumped region.

## COORDINATE OFFSETS RECOVERED (anomaly method: follow the projection scaler)
Instead of chasing the thin-forwarder read chain, I followed the pitch→screen projection
SCALER constant 0.008888889 (= 1/112.5, VA 0xb5e8c8) to its consumers. Two projection
functions (0x9a89b0, 0x9aa250) do, VERIFIED by disassembly:
```
fild dword ptr [obj+0xd0]      ; world coordinate (INTEGER fixed-point)
fmul dword ptr [0xb5e8c8]      ; * 1/112.5  (pitch units -> normalized)
fmul <screen-size const>       ; * 56.25 / etc -> pixels
```
and the overlay 0x9aa250 reads BOTH axes of a point:
```
fild [esi+0xd0] * (1/112.5) * 56.25        ; axis A  -> screen
      [esi+0xc0] (fild) * [0xbd2c88]        ; axis B  -> screen
      [esi+0x160]/[esi+0x164]/[esi+0x168]   ; second point / vector
```
So the engine's world coordinates are **integer fixed-point at object offset +0xc0 and
+0xd0** (a 2D point; one is the pitch-length axis, the other pitch-width), each projected
by ×(1/112.5). This matches A3's observed +0xb0/+0xc0/+0xd0 view-coordinate cluster.

- world X / Y = **+0xc0 / +0xd0**, integer fixed-point, projection ×(1/112.5) — **VERIFIED**
  (projection disassembly) on the engine coordinate object.
- that this coordinate object IS the per-entity geometry block (so player positions live at
  block+0xc0/+0xd0) — **INFERRED** (strong: it is the only fild-coordinate layout in the
  view cluster, and the block is the per-entity object). Velocity is the adjacent/second
  cluster (+0xc4/+0xd4 or the +0x160.. triple) — INFERRED, not yet isolated.

## Classification
- per-entity geometry = 1012-byte block via entity+0x134 under [pool+0x3004c] — **VERIFIED**
- resolver chain 009efa90/009efb70 + rep-movsd copy — **VERIFIED**
- world X/Y = +0xc0/+0xd0 integer fixed-point, ×(1/112.5) projection — **VERIFIED** (projection);
  mapping onto the entity block — **INFERRED**
- velocity/accel exact offsets — **INFERRED/UNKNOWN** (adjacent cluster; needs the threaded
  writer or a per-axis confirmation)
