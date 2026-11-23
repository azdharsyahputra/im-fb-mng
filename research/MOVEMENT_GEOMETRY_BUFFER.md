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

## Classification
- per-entity geometry = 1012-byte block via entity+0x134 under [pool+0x3004c] — **VERIFIED**
- resolver chain 009efa90/009efb70 + rep-movsd copy — **VERIFIED**
- x/y/vel/accel dword indices within the block — **UNKNOWN** (narrowed; next = 0x9b41e0 reader)
