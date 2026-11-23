# CM2008 Movement Field Offsets — EMULATION RECOVERY

Clean-room offset recovery by **isolated CPU emulation** (Unicorn) of individual readable
functions from `/Volumes/CM2008/Crack JPN/cm2008.exe`, plus a capstone byte-disassembly
cross-check. No game program was executed; `tools/emulate_fn.py` single-steps one function's
bytes over controlled memory and logs the struct offsets it reads/writes (map-on-fault keeps
it running over zero memory). `file_offset == VA − 0x400000` (verified identity mapping).

Method: seed the pool-base globals (`DAT_00c784a8`/`DAT_00c779ac`), the view-source
singleton (`DAT_00c94cf8`) and the manager `+0x2c`/`+0x24` base fields so a **valid**
player(A)/ball(B) handle resolves into a **tracked scratch entity array**, then record every
record offset the view + draw cluster touches. Scripts:
`scratchpad/drive_view.py`, `scratchpad/drive_draws.py`, `scratchpad/disfn.py`,
`scratchpad/scan_floats.py` (all RE aids).

Confidence tags: **VERIFIED(emulated)** (observed in an emulated access trace and/or decoded
from clean bytes), **INFERRED**, **UNKNOWN**. Per instructions, **no offset is invented** —
only observed accesses are reported.

---

## 0. HEADLINE RESULT

The entity **position / velocity / acceleration / direction numeric offsets remain
UNKNOWN** — they are **not read anywhere in the emulated view/draw/handle corpus**. Two
emulation facts pin this down:

1. The 32-byte **pool-A match-entity record is read ONLY at `+0x00` and `+0x02`** by the
   entire view path (`FUN_009ba5e0` + all 13 draw helpers). Once the scratch array is placed
   clear of the manager region, the draw helpers read **zero** further record offsets.
2. There are **no x87 float stores/loads against any entity-record pointer** in the handle
   cluster (`0x5a60xx–0x5a6xxx`), the string/geometry helpers (`0x8c1xxx`, `0xa0d5b0`), or
   the view/draw cluster (`0x9b5xxx–0x9bax`). The one geometry float in range is
   `fild dword ptr [esi+0xd0]` in the offside overlay (`FUN_009aa250`) — an **integer**
   loaded and converted to float. **Positions are integer fixed-point, not IEEE floats**,
   which is why no float-field scan locates them.

Consistent with `MOVEMENT_PHYSICS_RECONSTRUCTION.md §11`: the continuous geometry lives in
the manager state buffer (`*(DAT_00c784a8 + 0x3004c)`, a ~1012-dword block copied by
`rep movsd` in `FUN_009b54b0`) and/or the out-of-corpus threaded sim output — not in the
small per-entity record that this corpus resolves and reads.

---

## 1. HANDLE → POINTER RESOLVE — VERIFIED(emulated)

Decoded from `FUN_009ba5e0` head and reproduced in emulation:

```
pool A (player): rec = handle*0x20 + *(*(0x00c784a8) + 0x2c)   ; null handle 0x80000 / 0xfffff
pool B (ball):   rec = handle*0x2c + *(*(0x00c779ac) + 0x24)   ; null handle 0x8000  / 0xffff
```

`drive_view.py` seeded `*(0xc784a8)=MGRA`, `MGRA+0x2c=ARRAYA`, `VIEWSRC+0xac=2` (handle);
the emulator computed `recA = ARRAYA + 2*0x20 = ARRAYA+0x40` and the trace then showed reads
at `ARRAYA+0x40` / `ARRAYA+0x42` = **record +0x00 / +0x02**. Reverse-resolve helpers
(`FUN_005a6320 = (ptr-base)>>5`, `FUN_005a6330` → pool D `*0x10`, `FUN_005a6c30` A+2→B) were
re-emulated and confirm the strides/sentinels above. VERIFIED(emulated).

## 2. ENTITY POOL-A RECORD — fields ACTUALLY touched by the emulated corpus

| Offset | Sz | Access | Evidence | Meaning |
|---|---|---|---|---|
| `+0x00` | u16 | read | `drive_view` ARRAYA+0x40 read; `FUN_005a6040/6c30` | type/kind + packed state (VERIFIED(emulated) offset) |
| `+0x02` | u16 | read | `drive_view` ARRAYA+0x42 read; `FUN_005a6c30` imul 0x2c → pool-B | ball(B) handle ref (VERIFIED(emulated)) |

**No other pool-A record offset is read by the view/draw cluster** (emulated; draw-helper
union = ∅ once the array is clear of the manager at `+0x3004c`). The `+0x04` state word and
`+0x3c` rating cited elsewhere are written/read by the **resolver** path, not the view:

| Offset | Sz | Where | Evidence |
|---|---|---|---|
| `+0x3c` | u32 | `FUN_0077c710` contest, **both sides** | bytes `mov eax,[ebx+0x3c]`, `mov ecx,[ebp+0x3c]`, `shr …,0xc; and …,0x7f` → 7-bit rating. NB `+0x3c` > pool-A stride 0x20 → this rating lives on the **60-byte player record** (stride 0x3c, pool `DAT_00c78384`), not the 32-byte match entity. VERIFIED(bytes) |
| `+0x04` | u32 | `FUN_00a023f0` shootout writes `\|0x10000/0x20000/0x30000` | bytes `or eax,0x10000` … (bits 16-18). VERIFIED(bytes) |

## 3. VIEW / CAMERA OBJECT geometry fields (NOT entity records) — INFERRED

`drive_view.py` logged these reads on the **view object** (`this`) during the per-entity
update, before it writes its cache (`+0xf0 … +0x138`):

```
view +0xb0,+0xb4   (dword pair)   ┐
view +0xc0,+0xc4   (dword pair)   ├─ candidate view-space coordinate pairs (2-D points)
view +0xd8,+0xdc   (dword pair)   ┘
view +0xd0         (u32, fild)    ─ pitch coord → ×0.008888889×{56.25,16.2} px (FUN_009aa250)
```

These are **view/camera** fields (presentation), INFERRED to be screen/pitch coordinates;
they are not the per-entity physics state. Offset `+0xd0` is VERIFIED(bytes) as an integer
pitch coordinate (the `fild` + pitch→screen scalers in the offside overlay).

## 4. RECLASSIFICATION of the doc's "geometry accessors" — VERIFIED(emulated+bytes)

Emulation + disassembly show the four addresses named as "geometry accessors" are **not**
geometry at all:

| Addr | Real behavior (evidence) | Verdict |
|---|---|---|
| `FUN_006a00f0` | `std::map::find` on manager `+0x30058`, returns node `+0x14` (`call 0x4f90f0`; reads `this+0x30058`) | registry lookup, **not** geometry |
| `FUN_008c10c0` | `std::string` member op — SSO buffer at `+0x64`, cap `+0x78`, bool `+0x90`; alloc/copy helpers | container op, **not** geometry |
| `FUN_00a0d5b0` | 2-dword point/pair ctor: `this[0]=arg1; this[4]=arg2; return this (ret 8)` | generic `{+0,+4}` pair |
| `FUN_005a6xxx` | handle/pool resolve helpers — read only record `+0,+2,+0x18` | handle math, **not** geometry |

The `FUN_00a0d5b0` `{+0x00,+0x04}` pair is the only plausible *coordinate container* among
them, but it is a generic 8-byte value object, not an entity field layout.

## 5. CONTEST / SHOOTOUT math used for the clean-room code — VERIFIED(bytes)

Confirmed directly in the instruction stream (grounds `match/movement/contest.c`):

- **LCG** `imul eax,eax,0x19660D; add eax,0x3C6EF35F` (×4 in contest, ×many in shootout) +
  the `or eax,0x3f800000` float-in-[1,2) idiom. VERIFIED(bytes).
- **Contest** `FUN_0077c710`: two `+0x3c` ratings → `>>0xc & 0x7f`; `fld [0xbbb51c]` tuning
  scalar; `fdiv st(1)` normalize `r/s`; `fcomp` uniform vs scaled share. VERIFIED(bytes).
- **Shootout** `FUN_00a023f0`: `mov [esp+..],0x7d` (125), LCG, fixed-point scale, compare vs
  `0x33` (51) ⇒ p≈51/125≈0.408; best-of-5 then sudden death; result `or …,0x10000/0x20000/
  0x30000` into entity `+0x04` bits 16-18. VERIFIED(bytes).

The tuning scalar value `_DAT_00bbb51c` is a `.data` float read by value (not in the
instruction stream) — **not invented**; passed as a parameter to the reimplementation.

---

## 6. SUMMARY — VERIFIED(emulated) vs UNKNOWN

**VERIFIED(emulated / bytes):**
- Pool strides + null sentinels + handle↔ptr resolve for all 4 pools (A 0x20, B 0x2c, C 0x14, D 0x10).
- Pool-A record fields read by the view corpus: **`+0x00` (type/state u16), `+0x02` (ball handle u16)** — and nothing else.
- Contest rating at `+0x3c` (60-byte player record), state word at `+0x04`.
- View-object coordinate candidates `+0xb0/+0xb4`, `+0xc0/+0xc4`, `+0xd8/+0xdc`, pitch `+0xd0` (int).
- Contest + shootout RNG math (LCG, 7-bit rating normalize, 51/125 kick, flag bits).

**UNKNOWN (not observed — not invented):**
- Entity position / velocity / acceleration / direction numeric offsets. Not read by any
  emulated path; the 32-byte record exposes only `+0/+2`; geometry is integer fixed-point in
  the manager state buffer (`manager+0x3004c`) / out-of-corpus sim, unreachable over zero
  memory because the field-reading branches are FSM-gated and the state buffer is not in a
  resolvable per-entity record.
- The continuous integrator (`pos += vel*dt`, clamp to `MaxSprintingSpeed`).
- Numeric value of tuning scalar `_DAT_00bbb51c` (data-driven).
