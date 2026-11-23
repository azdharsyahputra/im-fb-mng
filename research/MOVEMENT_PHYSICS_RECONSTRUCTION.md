# CM2008 Match Engine — WORKSTREAM A: Movement Physics DATA LIFECYCLE

Clean-room reconstruction from decompilation + raw binary bytes. No binary executed,
no Ghidra opened. Decompiled text (`wave3.log`) and binary bytes treated as DATA only.
Builds on `MOVEMENT_ACTION_MAP.md` (the reader/view layer); this document maps the
**data lifecycle**: entity arrays, struct offsets, reader/writer per field, the
match RNG, the discrete sim resolvers found *in* this corpus, and the thread boundary.

Sources:
- `scratchpad/wave3.log` (231-fn decompile of `/Volumes/CM2008/Crack JPN/cm2008.exe`).
- Raw bytes of `cm2008.exe` (`file_offset = VA − 0x400000`; verified identity mapping).
- Prior: `research/MOVEMENT_ACTION_MAP.md`.

Confidence tags: **VERIFIED** (read in a clean body or decoded from bytes),
**INFERRED** (deduced from pattern/cross-reference), **APPROXIMATE** (math read,
semantics uncertain), **UNKNOWN**.

---

## 0. TWO HEADLINE CORRECTIONS to the prior layer model

### 0a. `wave3.log` decompiler output is UNRELIABLE for ~30% of functions — it is NOT packing. VERIFIED
Many functions in `wave3.log` decompile to garbage (`swi(3)`, `halt_baddata()`,
`in()/out()`, "instruction overlaps", "bad instruction data"): e.g. `FUN_00405530`,
`FUN_0056b600`, and the whole `FUN_005a60xx`/`FUN_005a6xxx` accessor cluster.
Reading the **raw bytes** shows these are perfectly valid, ordinary x86 — the binary is
**not packed or obfuscated**; the DumpWave3 Ghidra pass simply produced broken output for
them (wrong auto-analysis convention / failed flow). Proof by bytes:
- `0x405530`: `8b542404 0fb60a 83c8ff 84c9 741e … 33c8 81e1ff000000 c1e808 33048dd059b300 … c20400`
  → a **table-driven string hash** `hash(char*) ret 4` (xor/shift/table at `0xb359d0`), **not** `swi(3)`. VERIFIED.
- `0x5a6040`: `5356 57 8b7c2410 8bf1 8bcf 83cbff e8… 663b06 7508 … 33c0 c20400` → a record **type-compare**. VERIFIED.
- `0x56b600`, `0x5a6c30`, `0x5a6320` all decode to clean code (below).

**Consequence:** the entity-field accessor bodies ARE statically recoverable from the
binary; they are just missing from `wave3.log`. Field offsets below were recovered by
**decoding bytes directly**, not from the broken pseudocode.

### 0b. Discrete match-sim resolution DOES live in this corpus (not "only on worker threads"). VERIFIED
The prior note concluded all physics/resolution runs on worker threads. Byte/clean-code
analysis shows the **viewed match's discrete event + contest resolution runs here**,
synchronously, driven by a shared LCG RNG (§3). What runs on background threads is the
*other league fixtures* (ProcessData, `"ran on a thread"` — prior §7c). The two reconcile:
foreground match = this module; background fixtures = threaded. Continuous **position/
velocity integration** (the per-tick x/y/z update) is still **not in this corpus**.

---

## 1. ENTITY POOLS & HANDLE MODEL — VERIFIED

Entities are referenced by **integer handles**, resolved to record pointers through a
pool manager object. Four distinct pools were identified, each with its own manager
global, base-pointer offset, element stride, and null-handle sentinels (a reserved
top value 2^(n-1) and the all-ones 2^n−1):

| Pool | Manager global | Base-ptr field | Stride | Handle bits | Null sentinels | Resolve expr |
|---|---|---|---|---|---|---|
| **A** (primary match entity) | `DAT_00c784a8` | `+0x2c` | `0x20` (32) | 20-bit | `0x80000`, `0xfffff` | `h*0x20 + *(DAT_00c784a8+0x2c)` |
| **B** (ball / 3-D object) | `DAT_00c779ac` | `+0x24` | `0x2c` (44) | 16-bit | `0x8000`, `0xffff` | `h*0x2c + *(DAT_00c779ac+0x24)` |
| **C** (event / incident sub-record) | `DAT_00c803d8` | `+0x2c` | `0x14` (20) | 18-bit | `0x20000`, `0x3ffff` | `h*0x14 + *(DAT_00c803d8+0x2c)` |
| **D** (A-parallel cold data) | `DAT_00c7ffb8` | `+0x2c` | `0x10` (16) | 20-bit (shares A's h) | `0x80000`, `0xfffff` | `h*0x10 + *(DAT_00c7ffb8+0x2c)` |

- **Forward resolve** (handle→ptr): inlined everywhere, e.g. `FUN_009ba5e0:64-83`. VERIFIED.
- **Reverse resolve** (ptr→handle): `FUN_005a6320` / `FUN_005a6330`
  (`0x5a6320` bytes `8bc1 8b0da884c700 2b412c c1f805 c3`) =
  `handle = (ptr − *(DAT_00c784a8+0x2c)) >> 5` (÷0x20). VERIFIED.
- `FUN_005a6330` additionally re-maps a validated A-handle into pool **D**:
  `shl eax,4; add eax,[DAT_00c7ffb8+0x2c]` → `h*0x10 + base_D`. VERIFIED. Confirms A and D
  are **parallel arrays keyed by the same handle** (hot/cold split).
- **A↔B link**: `FUN_005a6c30` (`0x5a6c30`) reads `ushort [recordA+2]`, treats it as a
  pool-B handle (`movzx; imul …,0x2c; add [DAT_00c779ac+0x24]`), with B's `0x8000/0xffff`
  nulls. So **each A-record carries a pool-B reference at `+2`.** VERIFIED.

**Assignment of A vs B to player vs ball — INFERRED.** The match-view singleton tracks one
A-handle (`+0xac`, int) and one B-handle (`+0xc0`, ushort). B's 44-byte stride suits a
3-D mover (x,y,z + vx,vy,vz + spin/height); A's 32-byte stride suits a 2-D pitch actor.
Most consistent reading: **A = player pitch-entity, B = ball.** Not byte-proven.

---

## 2. ENTITY RECORD LAYOUT (pool A / match-participant record) — field table

Offsets VERIFIED by byte-decode or clean-body read; **meanings** mostly INFERRED. The
continuous-geometry fields (x/y/z, v) are NOT among the offsets any in-corpus function
touches directly (they are read inside the vtable draw calls and the geometry accessors
whose bodies are the broken-decompile set — recoverable from bytes but not yet decoded).

| Offset | Type | Reader(s) | Writer(s) | Meaning (confidence) |
|---|---|---|---|---|
| `+0x00` | ushort | `FUN_005a6040`, `FUN_005a6c30`, `FUN_0077c710:12813` (`&0x1fffff`, sentinel `0x100000`) | — (sim thread) | type/kind id + packed state (VERIFIED offset; INFERRED meaning) |
| `+0x02` | ushort | `FUN_005a6c30`, `FUN_005a6040` | — | **handle ref into pool B** (ball link) (VERIFIED) |
| `+0x04` | uint32 | view (`FUN_0077cd40`) | `FUN_0077cd40` (`&0xffff3fff\|0x3000`), `FUN_0077cf20` (`&0xfef7ffff`), `FUN_00a023f0` (bits16-18 `\|0x10000/0x20000/0x30000`) | **action/movement STATE + result flags** (VERIFIED offset+writes; bit meanings INFERRED) |
| `+0x18` | uint32 | `FUN_005a6330` region (`&0xfffff`) | — | 20-bit **handle self/peer ref** (VERIFIED) |
| `+0x1b` | byte | `FUN_0077c710:12816` (`&0x1f`) | — | 5-bit count (VERIFIED offset) |
| `+0x1c` | uint32 | `FUN_0077e850:14326` (bit-packed date y/m/d) | — | packed metadata (VERIFIED, context-specific) |
| `+0x38` | uint32 | `FUN_0077c710:12815` (`>>0x1b`, `>>0x16 &0x1f`) | — | two packed 5-bit counts (VERIFIED offset) |
| `+0x3c` | uint32 | `FUN_0077c710:12830-31` (`>>0xc &0x7f`) | — | **7-bit contest/duel rating** used in weighted resolution (VERIFIED offset; INFERRED = an attribute) |

Separate **pool-A manager** field: `*(DAT_00c784a8 + 0x3004c)` read by `FUN_009d5c20:464`
and cached in the view object — a global manager field (count or shared record), not a
per-entity field. VERIFIED read; meaning UNKNOWN.

**Position / velocity / acceleration / direction numeric offsets: UNKNOWN** in-corpus.
They are consumed by (a) the view's per-entity vtable draw `(**(piVar2+0x5c))` loop
(`FUN_009ba5e0:135-146`) and helpers `FUN_008c10c0`, `FUN_00a0d5b0/00a0f880`, and
(b) the geometry accessors in the broken-decompile set. Recoverable only by decoding
those bodies from bytes (not done here).

---

## 3. THE MATCH RNG — VERIFIED (the one piece of real "physics math" recovered)

A single **linear congruential generator** drives all discrete resolution:

```
state' = state * 0x0019660D + 0x3C6EF35F        // 1664525, 1013904223  (Numerical-Recipes "quick" LCG)
uniform_float ∈ [0,1)  =  (float)(state' >> 9 | 0x3F800000) - 1.0     // top 23 bits → [1,2) − 1
uniform_int   ∈ [0,N)  =  (uint)(((uint64)state' * N) >> 32)          // fixed-point scale
```

- **State location:** `FUN_007e7c10` = `lea eax,[ecx+0x2c]; ret` → the seed `uint` lives at
  **match-context + 0x2c**; also reached as `**(uint**)(ctx+8)` in `FUN_00a023f0`. VERIFIED.
- Used in `FUN_0077cd40:12447`, `FUN_0077c710` (×4), `FUN_00a023f0` (×8). VERIFIED.
- `_DAT_00c84958` is a derived per-match seed (`state>>8`) cached on first use
  (`FUN_0077cd40:12445-49`). VERIFIED.

These LCG constants ARE in the code and ARE reusable; the *gameplay* tuning scalars
(`_DAT_00c31b38`, `_DAT_00bbb51c`, `_DAT_00b18024`) are `.data` floats read by value — do
not invent them (consistent with the config-driven model in the prior note).

---

## 4. DISCRETE SIM RESOLVERS FOUND IN-CORPUS — the player-to-ball / contest layer

### 4a. `FUN_0077c710` — attribute-weighted two-sided contest (loose-ball / duel / tackle). VERIFIED math
Reachable: match module → `FUN_0077cd40` → `FUN_0077c710`.
```
r0 = (record_sideA + 0x3c) >> 0xc & 0x7f      // 7-bit rating, side 0  (iVar from FUN_00593cc0)
r1 = (record_sideB + 0x3c) >> 0xc & 0x7f      // 7-bit rating, side 1
s  = r0 + r1
p0 = _DAT_00c31b38 * (r0/s)                    // scaled probability share, side 0
p1 = _DAT_00c31b38 * (r1/s)                    // side 1
while (rng_uniform() < p0) apply_outcome(side=0)   // FUN_005cd020(0)
while (rng_uniform() < p1) apply_outcome(side=1)   // FUN_005cd020(1)
… then FUN_00483dc0(… many floats …); cross-multiply ratio test via two FUN_00476350() float10s
```
Reads **two entities' `+0x3c` ratings**, normalizes to probability shares, scales by a
`.data` coefficient, RNG-resolves, and applies outcomes to a per-side list. This is the
in-engine **tackle/collision/loose-ball contest**. VERIFIED arithmetic; "loose-ball duel"
label INFERRED. It also iterates pool-C-style records (`FUN_005cdcf0`/`FUN_009fdd60`) with
counts at `+0x38`/`+0x1b`.

### 4b. `FUN_00a023f0` — best-of-5 + sudden-death resolution (penalty shootout). VERIFIED math
```
per-trial success ≈ (uint64)rng * 0x7D >> 32  compared vs 0x33   // rand*125; threshold 51 → p≈0.408
loops count successes out of 5 for each side; ties → "while (c0==c1)" sudden death
monotone correction table (5↔3, 4↔1) biases bl-owouts
result → entity+4 bits 16-18 : |0x10000 (side0), |0x20000 (side1), |0x30000 (other)
```
Also uses `*0x7f` (127) scaling elsewhere. Classic **penalty-shootout** shape. VERIFIED
arithmetic; shootout label INFERRED from the best-of-5 + sudden-death structure.

### 4c. Outcome appliers — `FUN_005cd020(side)`, `FUN_005ccff0()`, `FUN_00a06920()`
Push/commit resolved outcomes to per-side containers; not fully decoded. INFERRED role.

---

## 5. EVENT DISPATCH & STATE TRANSITIONS — VERIFIED structure

### `FUN_0077cd40` — match-event → entity-state + view bridge
`param_2` = event descriptor: handle at `+0`, flag bytes at `+4,+5,+6`. It:
1. `h = ptr→handle (FUN_005a6320)`; `FUN_006a00f0(h)` → writes result to `view+0x98`.
2. Branches on the event flags; emits effect messages `FUN_009efa20/efa10(code, seed, …)`.
3. On the "contest" branch: `FUN_0077c710()` then sets **entity+4 bits 12-13** (`|0x3000`).
4. Drives the view: `FUN_009d5c20(h)` (track entity), issues animation/message IDs
   `0x495, 0x412, 0x414, 0x43a` via `FUN_0040adf0(code, h)` and vtable `(*(view+4+?))(code,h)`.
VERIFIED.

### `FUN_0077cf20` — per-entity tick/update
Reads the focused-target global `DAT_00c31b34` (`0xffffffff` = none); writes
`*(DAT_00c9b114+0x40)=param_3`; clears **entity+0x18 bits 19,24** (`&0xfef7ffff`); runs a
chain of predicate checks (`FUN_0069a610`, `FUN_00661270`) gating sub-updates
(`FUN_005cfc40`, `FUN_0077c540`, `FUN_0077c5e0`, `FUN_00955980`); ends with
`FUN_0040adf0(0x414, h)`. VERIFIED. This is the state-machine driver per entity.

### Message/animation ID codes (dispatched to the view) — VERIFIED values, meanings INFERRED
`0x412, 0x414, 0x43a, 0x495` and effect codes `0,1,3` (`FUN_009efa20/efa10/efa30`).

---

## 6. READER (VIEW) PATH — refines prior §1

`FUN_009ba5e0` (view per-frame entity update) is the top reader. It reads the **view
singleton `DAT_00c94cf8`**: `+0xac` (tracked A-handle, int), `+0xc0` (tracked B-handle,
ushort), `+0x04` (sub-object whose vtable `+4` takes `(msgcode,h)`), `+0x12a` (byte flag).
It resolves both handles, caches the record ptrs/derived values into the view object, then
runs the state machine and draw cluster:

```
state = FUN_009b5cf0() ∈ {0,1,2,4}            // from bit tests FUN_008e9e10/008e9f90/008ea090
  (0: !test1 | 1: test1&!test2 | 2: test1&test2&!test3 | 4: all three)
state gates: FUN_009b54b0 / 009b5b50 / 009b9490 / 009b8dc0 / 009b5690 / 009b5bd0 /
             009c8c70 / 009b86d0 / 009b6e60 / 009b55d0 / 009ba490
```
`FUN_008e9f90/008ea090` are **bit-array queries**: `bit = state_obj[*(obj+0x3b8/0x3cc) + FUN_0061d430()&0xffff]`.
VERIFIED. Config atom `"AllowVerticalMovement"` interned at `FUN_009ba5e0:132`.

View cache offsets (into the view object `param_1`): `+0x128`=A-record ptr, `+0x12c`=B-record
ptr, `+0x130`=`FUN_005a6c30(B)`, `+0x134`=`FUN_005a6040(B)`, `+0x138`=`(A==0)`,
`+0xbc`=`FUN_0077cc30()` singleton, `+0x144`=tracked A-record (`FUN_009d5c20`), `+0x148`=
`*(DAT_00c784a8+0x3004c)`, `+0x14c`=`FUN_005a6330()`, `+0x140`=valid flag, `+0x150`=view
sub-state {0,1,2}, `+0x98`=`FUN_006a00f0` result. VERIFIED offsets.

**Every access to pools A/B/C in this corpus is a READ** (grep-verified: `DAT_00c784a8`,
`DAT_00c779ac`, `DAT_00c803d8` appear only in resolve-and-read contexts). The entity
**writers of position** are not present — only state-word (`+4`) writers are. VERIFIED.

---

## 7. THREAD BOUNDARY & SYNC — INFERRED / UNKNOWN

- **No thread/sync API is visible in this corpus**: no `CreateThread`, `_beginthreadex`,
  `EnterCriticalSection`, `Interlocked*`, `WaitFor*`. The `LOCK()/UNLOCK()` tokens in
  `wave3.log` are **artifacts of the broken decompiles** (stray `lock` prefixes in
  mis-disassembled bodies), not real atomics. VERIFIED (absence).
- `FUN_0077d800`/`FUN_0077d340` are a **std::map / red-black tree** lower_bound + insert
  (node = 20 B: `+4` parent, `+8` left, `+0xc` right, `+0x10` key, `+0` color), **NOT a
  thread dispatcher** — correcting the prior "FUN_0077d800 dispatches sim onto threads".
  This is the container `GCMatchModule::ProcessData` walks. VERIFIED.
- Model (INFERRED): the **foreground (viewed) match** resolves events synchronously on the
  main/match thread (§4–5) and the **view reads the same pools** (§6) in the same per-tick
  sequence — resolve → write `+4` state → render. **Background league fixtures** are the
  ones simmed on worker threads and committed by `ProcessData` (prior §7c). No
  double-buffer or lock was found for the foreground path.
- **UNKNOWN:** whether the viewed match's physics step and the render step are truly on one
  thread or a producer/consumer pair; the hand-off (if any) is set up by CRT thread
  wrappers and the sim loop, neither of which is in this 231-fn corpus. **Static is
  insufficient** here: the thread-creation and the lock/seqlock (if any) live outside the
  dumped set, and the only way to confirm is to decode `GCMatchModule::ProcessData`'s
  callees (`FUN_0077d800` is just the map) and the sim entry, or to observe at runtime.

---

## 8. FUNCTION DEPENDENCY GRAPH (traced)

```
match tick
  └─ FUN_0077cf20 (per-entity state update)
       ├─ reads DAT_00c31b34 (focus target), writes entity+0x18, DAT_00c9b114+0x40
       ├─ FUN_0069a610 / FUN_00661270  (predicate gates)
       ├─ FUN_005cfc40 / FUN_0077c540 / FUN_0077c5e0 / FUN_00955980  (sub-updates)
       └─ FUN_0040adf0(0x414, h)  → view message
  └─ FUN_0077cd40 (event handler)
       ├─ FUN_005a6320 (ptr→handle) ─ FUN_006a00f0 → view+0x98
       ├─ RNG seed cache (_DAT_00c84958)
       ├─ FUN_0077c710  ── CONTEST ──┐
       │     ├─ FUN_007e7c10 → RNG state @ctx+0x2c   (LCG)
       │     ├─ reads side0/side1 +0x3c ratings (FUN_00593cc0)
       │     ├─ _DAT_00c31b38 scale ; FUN_005cd020(0/1) apply
       │     └─ FUN_00483dc0 / FUN_00476350 (float ratio test)
       ├─ FUN_00a023f0  ── SHOOTOUT ── RNG best-of-5 → entity+4 bits16-18
       ├─ FUN_005a6330 → entity+4 |0x3000 (state)
       └─ FUN_009d5c20 (view: track entity) ; FUN_0040adf0(0x495/0x412/0x414/0x43a)

render tick
  └─ FUN_009ba5e0 (view per-entity)
       ├─ reads DAT_00c94cf8 +0xac (A-handle) / +0xc0 (B-handle)
       ├─ resolve A via DAT_00c784a8 ; B via DAT_00c779ac ; cache +0x128..+0x138
       ├─ FUN_009b5cf0 → state{0,1,2,4}  (bit tests FUN_008e9e10/8e9f90/8ea090)
       └─ draw cluster FUN_009b54b0/5b50/9490/8dc0/5690/5bd0/c8c70/86d0/6e60/55d0/ba490
            (consume pool C via DAT_00c803d8 ×0x14; emit via vtable +0x5c)
```

Pool reverse-map helpers: `FUN_005a6320/6330` (ptr→A-handle, →pool D). Pool A↔B link:
`FUN_005a6c30` (`A+2`→B). String-atom hash: `FUN_00405530` (table hash, `ret 4`).

---

## 9. PER-FIELD READER/WRITER SUMMARY

| Field | Written by (layer) | Read by (layer) | Lifecycle |
|---|---|---|---|
| entity `+0` type/state | sim (out-of-corpus) | `FUN_005a6040/6c30`, `FUN_0077c710` | sim writes, all layers read |
| entity `+2` ball ref | sim/setup (OOC) | `FUN_005a6c30/6040` | link A→B |
| entity `+4` action/result flags | `FUN_0077cd40`, `FUN_0077cf20`, `FUN_00a023f0` (**this module**) | view `FUN_0077cd40` | resolver writes, view reads → **movement-state transition record** |
| entity `+0x18` handle/peer | `FUN_0077cf20` (clears bits) | `FUN_005a6330` region | per-tick update |
| entity `+0x38/+0x3c` ratings/counts | sim/attribute import (OOC) | `FUN_0077c710` | read-only in contests |
| **position/velocity/accel/dir** | **sim integrator (OOC / not dumped)** | view draw vtable `+0x5c`, geometry accessors (bytes, not decoded) | **UNKNOWN offsets** |
| RNG seed `ctx+0x2c` | all resolvers (LCG) | all resolvers | shared mutable state |
| view singleton `DAT_00c94cf8` +0xac/+0xc0 | input/selection (OOC) | `FUN_009ba5e0`, `FUN_0077cd40` | picks tracked player/ball |

---

## 10. IMPLEMENTATION NOTES (after the data-flow map, per instructions)

- **Reproduce the RNG exactly**: `state = state*1664525 + 1013904223` (uint32 wrap), with
  the two derived forms in §3. This governs every contest/shootout outcome and must match
  bit-for-bit for determinism. VERIFIED — safe to implement as-is.
- **Contest template (§4a)** is directly implementable from VERIFIED arithmetic once the
  `+0x3c` rating semantics and `_DAT_00c31b38` value are known (rating = a player attribute;
  coefficient = a `.data`/PlayerData tuning float — **read it, don't invent**).
- **Shootout (§4b)**: best-of-5, per-kick p≈51/125≈0.408 (VERIFIED), tie→sudden death,
  blow-out correction table. Implementable; confirm which side maps to `0x10000`/`0x20000`.
- **Do NOT implement continuous movement yet**: position/velocity/accel offsets and the
  integrator are UNKNOWN. Next step = decode the geometry accessors from bytes (the
  broken-decompile set `0x5a6xxx`, `0x8c1xxx`, `0x00a0d5b0`, `0x006a00f0`) with a correct
  disassembly, and the vtable-`+0x5c` draw targets, to recover the x/y/z field offsets.

---

## 11. UNKNOWNs & why static is insufficient

1. **Position/velocity/acceleration/direction field offsets** — consumed only inside the
   view's vtable draw calls and the geometry accessors, whose bodies are the
   broken-decompile set. Recoverable from clean bytes but require a correct disassembly
   pass not provided by `wave3.log`.
2. **The continuous integrator** (the `pos += vel*dt; vel += accel*dt; speed clamp to
   MaxSprintingSpeed=9`) — not in this 231-fn corpus; lives in the sim loop / threaded path.
   `formula.log` (MatchResolver defaults: `MaxSprintingSpeed=9`, accel/decel multipliers)
   gives the *coefficients* but not the *code path* that applies them.
3. **Thread boundary & sync of the foreground match** — no thread/lock API in-corpus;
   cannot confirm single-thread-sequential vs producer/consumer statically.
4. **`+0x3c`/`+0x38` rating semantics** — offsets VERIFIED, but which attribute each packed
   field holds requires the attribute-import/schema code (prior note's serializers).
5. **Tuning scalars** `_DAT_00c31b38`, `_DAT_00bbb51c`, `_DAT_00b18024` — read by value from
   `.data`; numeric values not in the instruction stream (data-driven; do not invent).
6. **Passing/receiving/shooting trajectory** — still absent; only the discrete contest and
   shootout resolvers were found, plus the event-dispatch message IDs.
