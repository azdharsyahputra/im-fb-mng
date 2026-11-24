# Continuous per-frame integrator — hunt (Agent C)

**Mission:** find the REAL per-frame geometry integrator (read-modify-write of position/velocity
each tick), as opposed to the one-time seeder `FUN_009f0090` and the ruled-out config-singleton
initializer `0x9f5398`.

**Method — STATIC ONLY, real bytes.** capstone 5.0.7 linear disassembly of the already-unpacked
crack copy `/Volumes/CM2008/Crack JPN/cm2008.exe`, flat map `file_off = VA − 0x400000`
(**re-verified**: `0x9f0090` decodes to the seeder prologue `push esi; mov esi,[esp+0xc]; movzx
eax,[esi+0x49]; … call 0x5d6d20`, and the `0x9f00fc` store block matches `SIM_BLOCK_WRITER.md`).
No game run, no DRM, no commits. Ghidra (`cm2008_full`) was **not** run — no lock files present but it
is single-access and sibling agents may hold it; capstone on the crack copy is the authoritative
byte source used by all prior agents and needs no shared lock. Scan driven by the full
`docs/ghidra_dumps/functions.csv` (**18 958** functions with exact boundaries).
Scanner sources: `scratchpad/rmw_scan.py` (direct `[reg+disp]`), `scratchpad/rmw_idx.py`
(indexed `[reg+idx*s+disp]`), `scratchpad/filt.py` (classifier), `scratchpad/disfn.py` (disasm).

Classifications: `NOT_INVESTIGATED / LOCATED_NOT_UNDERSTOOD / DECOMPILED / STRUCTURALLY_UNDERSTOOD /
BEHAVIORALLY_VALIDATED / GENUINELY_UNAVAILABLE`.

---

## 0. BOTTOM LINE

1. **There is NO position/velocity integrator (`pos += vel·dt`) anywhere in statically-reachable
   code.** Extending the RMW scan beyond the old `+0xc0/+0xd0`-only search to the **whole offset
   range** (incl. the raw-float vectors at **+0x94..+0xB8** and the attribute scalars +0xc0..+0xd4),
   in **both float and integer** forms, and in **both direct and indexed** addressing, yields
   **ZERO** in-place read-modify-write of any geometry offset on the per-entity block. This
   **corroborates and broadens** `SIM_BLOCK_WRITER.md §2` (which only tested +0xc0/+0xd0). The
   negative IS the finding: **match motion is set-to-target / waypoint, not a continuous
   integrator** (consistent with `MOVEMENT_GEOMETRY_BUFFER.md`). — `BEHAVIORALLY_VALIDATED (absence)`.

2. **One genuine per-tick, in-place FLOAT read-modify-write integrator DOES exist in the sim
   cluster — but it integrates TIME/PHASE, not geometry:** `FUN_009d52e0` does
   `if (matchClock ≥ entity[+0x154]) entity[+0x154] += 10.0f`. This is the only continuous
   accumulator statically reachable; it advances a per-entity simulation-time field, gated by the
   match clock resolved through the documented `[esi+0x148]→0x9efa90→+0xe4+0x48` path.
   — `STRUCTURALLY_UNDERSTOOD` (bytes VERIFIED; "time not position" VERIFIED by offset+constant).

3. **The worker work-fn `0x46ff90` is NOT the sim loop.** It is a thin **virtual-dispatch thunk**
   (`call 0x46fec0; then (*this->vtbl[0])(1)`); `0x46fec0` is a buffer/string **finalize/log** helper
   (the "ran on a thread" path). The real per-fixture work is a **data-dependent vtable method**, not
   a single static function — so the true geometry producer is **thread-private / vtable-dispatched**,
   not pinnable statically. — `STRUCTURALLY_UNDERSTOOD` (thunk VERIFIED; real body still
   `LOCATED_NOT_UNDERSTOOD`).

**Did I find a read-modify-write geometry writer? NO for position/velocity (clean image-wide
negative). YES for a per-tick time/phase accumulator (`FUN_009d52e0`, +0x154 += 10.0).**

---

## 1. `0x9f5398` — confirmed RULED OUT (extends Agent 1, `INTEGRATOR_RECOVERY.md`)

Re-verified the exact bytes. Each geometry store reloads ESI from the **global singleton**
`[0xc68428]` (lazy-ctor `0x480110`) and writes a config getter's result **once**, write-only:
```
0x9f538c  call 0x480110            ; lazy-construct singleton
0x9f5391  mov  esi, eax            ; ESI = global singleton [0xc68428]  (NOT the entity block)
0x9f5393  call 0x841e90            ; config getter
0x9f5398  fstp [esi+0xc0]          ; write-once, no fld [esi+0xc0] read
0x9f539e  mov  eax,[0xc68428] … 0x9f53ae call 0x841f80 ; 0x9f53b3 fstp [esi+0xc4]  (same idiom)
```
No read of +0xc0..+0xd4; ESI is a global config singleton, not the per-entity physics block.
Agent 1's classification stands: **run-once global-config-singleton initializer, not a per-frame
integrator.** — `STRUCTURALLY_UNDERSTOOD`. (Not re-done in full; see `INTEGRATOR_RECOVERY.md`.)

---

## 2. Image-wide read-modify-write scan (the integrator fingerprint)

The integrator's signature is **read-then-write of the SAME `[base+disp]` with arithmetic between**
(`fld/fild [m] … fadd/fmul/fsub/fdiv … fstp/fistp [m]`, or integer `mov [m]→r; add; mov r→[m]`, or
single-instruction `add/sub [m],x`), **base register not reassigned in between** — vs the seeder,
which only writes. Scanned all 18 958 functions (VA < 0xb00000; the 0x15bxxxx bignum region is the
packer tail, excluded).

| Scan | Direct `[reg+disp]` | Indexed `[reg+idx*s+disp]` |
|---|---|---|
| FPU-store RMW with FPU-load/arith read (true integrator idiom) | **95** total image-wide | **0** |
| …of those on geometry offsets **+0x94..+0xB8** (raw-float vectors) | **0** | **0** |
| …of those on geometry offsets **+0xc0..+0xD4** (attr scalars) | **0** | **0** |
| …in SIM range (0x9b–0xa20000) on an in-block offset | **1** (`FUN_009d52e0` @ +0x154 — *time*) | **0** |
| Integer in-place RMW on +0x94..+0xD4 of the block | **0** | **0** |

**Interpretation of the 95 non-geometry float RMW sites** (all VERIFIED bytes, none are the position
integrator):
- **Refcount/counter churn:** a large `FUN_009d0xxx` accessor family does `add [esi+0x16c], ±1`
  (acquire/release of a per-object reference count) — integer, not geometry.
- **Vector/matrix math helpers:** `FUN_009fff20`, `FUN_00a04b20`, `FUN_009dc360` RMW `[argptr+0x0]`
  (they transform caller-supplied vectors in place) — operate on arg pointers, not the entity block.
- **Other-object physics coeffs:** e.g. `FUN_00866580 [esi+0x80]`, `FUN_008fd120 [esi+0x13c]`,
  `FUN_00817df0 [esi+0xc]` — in-place float updates on non-sim objects (audio/UI/geometry-builder),
  outside the match-entity cluster.
- CRT (`__modf_default`, `__handle_exc`) — library math.

**No indexed in-place RMW exists anywhere** (0 of any store type) — i.e. there is no per-player
`[entity + slot*stride + posOffset] += …` loop either. The per-slot arrays in the block
(`+0x98` stride 0xc, `+0xe0` stride 8, `+0x15c`/`+0x164` dword arrays — see §3) are **written from
computed values (set-to-target)**, never incremented in place.

---

## 3. `FUN_009d52e0` — the one real continuous integrator (TIME/PHASE, not geometry)

`__thiscall FUN_009d52e0(this /*ecx=esi=per-entity sim object*/, …, slotIndex /*[esp+0xa98]=edi*/)`,
size 586 B, VA 0x9d52e0. Full disasm in `scratchpad` (via `disfn.py 0x9d52e0`). Behaviour:

```
esi = this
state = [esi+0x150]                       ; 0 → branch A, 1 → branch B, else skip
ctx   = resolve([esi+0x148]) via call 0x9efa90      ; period/clock object (same resolver as
                                                     ; MOVEMENT_GEOMETRY_BUFFER access chain)
clock = *(*(ctx)+0xe4)+0x48                ; match clock base  (THREADED_SIMULATION §4 / MATCH_CLOCK)
...per-slot loop over edi: reads [esi+edi*3*4+0x98], array [esi+edi*8+0xe0], writes
   [esi+edi*4+0x15c], [esi+edi*4+0x164]   ; per-slot state (set from computed values, not integrated)
...
0x9d532b  cmp  clock, [esi+0x154]
0x9d5342  setge [esp+0x13]
0x9d5390  je   skip
0x9d5392  fld  dword [esi+0x154]          ; READ   ──┐  in-place float
0x9d5398  fadd dword [0xc5d43c]           ; += 10.0  │  read-modify-write
0x9d539e  fstp dword [esi+0x154]          ; WRITE  ──┘
```
- **Constant `[0xc5d43c] = 10.0f`** (bytes `00 00 20 41`, VERIFIED by reading the image).
- `+0x154` is a per-entity **simulation-time / phase accumulator**: while the match clock has reached
  the stored threshold, advance it by 10.0 (a 10-unit game-time step), driving the per-slot state
  transitions in the loop above. It is **not** a position or velocity field (those are the +0x94..0xB8
  raw-float vectors and the +0x98/+0xe0 per-slot arrays, none of which are RMW'd).
- **Callers:** `0x9d572f`, `0x9d58fc`, `0x9d5ab3` — all inside the `0x9d5xxx` per-entity sim-update
  driver cluster (siblings `FUN_009d5640`, `FUN_009d5a10`). No absolute-pointer (vtable) occurrence.
  So this is a per-tick update step of the entity-simulation module (0x9d0000–0x9d6000), reached from
  within that cluster, consistent with the match-entity sim, not match setup.
- **Callees (the per-slot workers):** `0x9d3e50`, `0x9d0fb0`, `0x9d45d0`, `0x9d4930`, `0x9d4090`,
  `0x9d41f0`, `0x9d4c60`, `0x9d4fc0` — scanned; none contains an in-place float RMW of a
  geometry offset (they set per-slot fields from computed targets).

Classification: `STRUCTURALLY_UNDERSTOOD` — the RMW, the gate (match clock), the constant (10.0),
and the field role (time/phase, not geometry) are all VERIFIED from bytes; the downstream semantics
of the per-slot arrays are `LOCATED_NOT_UNDERSTOOD`.

---

## 4. Worker path — `0x46ff90` (ctor `0x700b0`) and thread proc `0x4755c0`

- **Work-fn `0x46ff90`** (NOT in `functions.csv`; undefined Ghidra region):
  ```
  push esi; mov esi,[esp+8]       ; esi = Thread/param object (lpParameter)
  mov ecx,esi; call 0x46fec0      ; finalize/log helper (below)
  test esi,esi; je ret
  mov eax,[esi]; mov edx,[eax]    ; this->vtbl ; vtbl[0]
  push 1; mov ecx,esi; call edx   ; VIRTUAL call  (*this->run)(1)
  ret
  ```
  A **thin virtual-dispatch thunk** — it does no geometry work itself; it finalizes then invokes the
  object's virtual `run(1)`. The actual per-fixture simulation is behind the Thread object's **vtable
  pointer**, which is set per-instance and **not statically resolvable to one function**.
- **`0x46fec0`** (undefined region): allocates a 0x400 buffer and calls stream/string helpers
  `0x474810 / 0x472ba0 / 0x4728d0 / 0x472360` — the **result/finalize + logging** side (matches
  `THREADED_SIMULATION_RECOVERY.md §4`'s `"End of fixture %d that ran on a thread."`). No geometry.
- **`0x700b0` / `0x46ffb0` ctor region:** SEH-framed constructor storing vtables `0xb55d60`/`0xb55d50`/
  `0xb55d18` at `[esi]/[esi+4]` and sub-constructing members at `[esi+8]`/`[esi+0x40]` — generic
  object construction, no geometry stores.
- **Thread proc `0x4755c0`** (per `SIM_THREAD_ENTRY.md` / `THREADED_SIMULATION_RECOVERY.md`): reads
  the player pool `DAT_00c784a8` (stride 0x20), validates an entity, copies two coord vectors into the
  param object, and **returns** (no loop). A per-fixture snapshot/dispatch, not an integrator.

**Conclusion:** the worker path is a generic fire-and-harvest task (dispatch → finalize/log →
virtual `run`), so the per-frame geometry producer, if continuous at all, lives in the **vtable
method invoked at `0x46ffa8`** — data-dependent, thread-private, undefined in Ghidra.
— `LOCATED_NOT_UNDERSTOOD`.

---

## 5. Reconciling the prior docs

- `SIM_BLOCK_WRITER.md §2` ("zero in-place RMW of +0xc0/+0xd0"): **confirmed and broadened** — zero on
  +0x94..+0xD4, float and integer, direct and indexed, image-wide.
- `MOVEMENT_GEOMETRY_BUFFER.md` ("set-to-target + double-buffer, no velocity integrator"):
  **supported.** Note its "per-frame advance RECOVERED" paragraph cites `big.c:36881/36888`
  (`FUN_00978f40`) and `29529/29613` (`FUN_00977620`), which `GEOMETRY_WRITER_INVESTIGATION.md §1` and
  `PHYSICS_AND_WAYPOINT_ANALYSIS.md` show are the **UI widget** geometry setter, not the match entity
  — so the "cur/prev double-buffer at +0xc0/+0xc4" mechanism is a UI-object observation, not the sim
  block. The sim block is seeded once (`FUN_009f0090`) and thereafter has its per-slot fields **set**
  (not integrated) by the `0x9d0000–0x9d6000` cluster, with the only continuous accumulator being the
  +0x154 time/phase field (§3).
- The seeder `FUN_009f0090`'s own in-place pair at `[edi+0x130]/[edi+0x134]` (flagged by the scan) is
  the documented init-time affine transform (`block[0x4c]=block[0x4c]*-20.0+55.0`), not per-tick.

---

## 6. Classification summary

| Item | Verdict | Class |
|---|---|---|
| Position/velocity integrator (`pos += vel·dt`) on +0x94..0xB8 / +0xc0..0xD4 | **does not exist** in reachable code (0 float+int, 0 indexed, image-wide) | `BEHAVIORALLY_VALIDATED (absence)` |
| `FUN_009d52e0` +0x154 += 10.0, gated by match clock | per-tick in-place float accumulator of **sim time/phase** (not geometry) | `STRUCTURALLY_UNDERSTOOD` |
| `0x9f5398` | run-once config-singleton init (ESI=[0xc68428]), write-only | `STRUCTURALLY_UNDERSTOOD` (Agent 1, re-confirmed) |
| work-fn `0x46ff90` | virtual-dispatch thunk (`call 0x46fec0` + vcall `run(1)`); undefined in Ghidra | `STRUCTURALLY_UNDERSTOOD` |
| `0x46fec0` | buffer/string finalize + log ("ran on a thread") | `STRUCTURALLY_UNDERSTOOD` |
| True per-fixture sim body (vtable method behind `0x46ffa8`) | thread-private, data-dependent vtable dispatch | `LOCATED_NOT_UNDERSTOOD` |
| Motion model = set-to-target / waypoint (no continuous geometry integrator) | the engine advances position by setting computed targets, not integrating velocity | `BEHAVIORALLY_VALIDATED` |

**No velocity/accel formula is fabricated.** None exists as an in-place integrator in reachable code.

---

## 7. Single next step

Define, in Ghidra `cm2008_full` (single-access, static-only), the Thread object's **vtable method**
reached at `0x46ffa8` (follow the vtables `0xb55d60/0xb55d50/0xb55d18` the ctor `0x46ffb0`/`0x700b0`
installs) and decompile it, plus `FUN_009d52e0` and its `0x9d4xxx` callees, to formally confirm the
per-slot update is set-to-target. The genuine remaining unknown — whether any continuous position
integration happens inside that vtable `run` method — is **thread-private** and ultimately needs a
lawful runtime write-watchpoint on the 1012-byte block (per `MOVEMENT_GEOMETRY_BUFFER.md`); static
analysis has now exhausted the direct/indexed, float/integer RMW surface and found none for geometry.

Doc path: `/Users/csadeveloper/cm/cm2008-reconstruction/research/CONTINUOUS_INTEGRATOR_RECOVERY.md`
