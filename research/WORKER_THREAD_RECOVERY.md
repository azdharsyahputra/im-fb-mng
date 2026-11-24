# CM2008 Worker-Thread `run()` Recovery — Milestone 3

Static analysis only. No game run, no DRM bypass, no ISO modification.

**Two complementary static sources** (verified independently; the single-access unpacked
Ghidra DB `cm2008_full` was unlocked, I was the only analyzer):
- **Crack copy** `/Volumes/CM2008/Crack JPN/cm2008.exe` (ImageBase 0x400000). Its `.text`
  (0x401000–0x711000) is UNENCRYPTED and is the authoritative source for the thread/task
  code at **0x46xxxx / 0x47xxxx**. Flat map `file_off = VA − 0x400000` **re-verified**:
  VA 0x46ff90 → file 0x6ff90 = `56 8b 74 24 08 …` (a clean function prologue). Disassembly
  via capstone (`CS_ARCH_X86 / CS_MODE_32`).
- **Unpacked DB** `cm2008_full` (Ghidra 12, `reexport.sh`/`Decomp.java` recipe + two
  read-only scan scripts kept in the scratchpad). Authoritative for the vtable **data**
  (0xb55xxx) and the simulation **code** (0x77xxxx / 0x9bxxxx–0x9fxxxx / 0xa1xxxx).

**Critical image note (VERIFIED):** the two images are *complementary*, not identical.
In `cm2008_full` the 0x46xxxx–0x4702xx bytes are **corrupt/mutated** (byte at 0x46ff90 =
`0x75`, vs crack `0x56`; Ghidra decompiles the whole region to `swi(3)`/`halt_baddata`
garbage with overlapping instructions). Conversely the crack's `.text` ends at 0x711000,
so 0x77xxxx/0x9fxxxx are `.rdata` there. **⇒ thread/task code read from the crack; vtables
and sim code read from `cm2008_full`.** The vtable pointers in `cm2008_full` (0xb55d50→
0x4700b0, etc.) land exactly on clean crack code, so the two agree on the code VA space.

Classification vocabulary: VERIFIED / INFERRED / STRUCTURALLY_UNDERSTOOD /
LOCATED_NOT_UNDERSTOOD / UNKNOWN. **A clean VERIFIED NEGATIVE is reported as such; no
integrator formula, velocity/accel law, coordinate, or selector is fabricated.**

---

## 0. BOTTOM LINE

1. **The thread entry resolves fully (VERIFIED).** CreateThread site #2 → `Thread::start`
   (0x475620) → thread proc **0x4755d0** → `(*this->vtbl[1])()`. For the worker object
   (runnable-interface vtable **0xb55d50**) that is **`run()` = 0x4700b0**.
2. **The lead's premise is REFUTED by the bytes.** `FUN_0046ff90` is NOT the worker loop
   and `vtbl[0]` is NOT the real work: **`vtbl[0]` is the C++ destructor**, and
   `FUN_0046ff90` is the **completion/finalize functor** (`build "…ran on a thread" log`
   via 0x46fec0, then `(*vtbl[0])(1)` = *delete this*). The thread dispatch uses
   **`vtbl[1]`**, not `vtbl[0]`.
3. **`run()` (0x4700b0) is dispatch/harvest/finalize plumbing — it contains NO geometry and
   NO waypoint access.** Its full 78-function direct-call closure has **zero** operands on
   +0x94..0xD4 or +0xb4..0xBA and makes **zero** calls into GCMatchModule (0x77xxxx) or the
   sim-geometry cluster (0x9bxxxx–0x9fxxxx).
4. **Integrator verdict — VERIFIED NEGATIVE.** The hypothesis that a per-frame position
   integrator "falls out of run()" is false (run() has no geometry). An image-cluster scan
   (0x77 / 0x9b–0x9f / 0xa1) finds **0** read-modify-write sites on +0x94..0xD4 (x87 **or**
   SSE). The geometry block is **only ever SET**, by the single seeder `FUN_009f0090`.
   Motion is set-to-target; there is no continuous velocity/accel integrator in statically
   reachable code.
5. **Filler verdict — VERIFIED NEGATIVE (thread-private).** The non-0xffff waypoint-ref
   writer is not in the thread and not via any literal or `lea`-based store to the entity's
   u16 quad +0xb4..0xBA in the sim cluster. This independently re-confirms Agent D's
   image-wide negative. It is **also not in the picker cluster** (0x5b3000–0x5b5300, which
   `run()` consults at 0x5b3ba0): that cluster is statically readable from the crack (plain
   accessors, no RNG — see `PICKER_RECOVERY.md` / `RESOLVER_PICKER_CLUSTER_RECOVERY.md`) and
   a full-range scan shows **0** geometry/waypoint operands. The selector is therefore
   applied upstream in the match engine or via a struct/`rep movs` copy, outside every
   region examined here.

---

## 1. Thread entry → `run()` resolution (VERIFIED)

### 1.1 The real thread procedure (crack, 0x4755d0)
`SIM_THREAD_ENTRY.md`'s disassembly of this proc (player-pool reads, vector copies) was a
**mis-aligned read** (it started at 0x4755d3). The real proc is a 5-instruction virtual
trampoline; 0x4755c0 is a 3-byte stub (`mov cl,0; ret`).

```
0x4755d0  mov ecx,[esp+4]    ; this = lpParameter (the worker's runnable subobject)
0x4755d4  mov eax,[ecx]      ; eax = vtable
0x4755d6  mov edx,[eax+4]    ; edx = vtbl[1]      <-- SLOT 1, not slot 0
0x4755d9  call edx           ; (*this->vtbl[1])()  = run()
0x4755db  xor eax,eax; ret 4
```
`Thread::start` push `0x4755d0` as `lpStartAddress` is at crack VA **0x47562b** (prior docs'
"0x7562b/0x75620/0x700b0" dropped the leading `4` / mixed VA with file-offset; the true VAs
are 0x47562b / 0x475620 / 0x4700b0).

### 1.2 The worker object (multiple inheritance) — ctor 0x46ffb0 (crack, VERIFIED)
```
0x46ffb0  … SEH prologue …
0x46ffd2  mov esi,ecx                 ; this
0x46ffe0  mov [esi+4],0xb55d18        ; (transient during construction)
0x46fff4  mov [esi],  0xb55d60        ; PRIMARY base vtable  @ this+0
0x46fffa  mov [esi+4],0xb55d50        ; RUNNABLE base vtable @ this+4
0x470001  lea ecx,[esi+8];  call 0x476260   ; member ctor
0x470013  lea ecx,[esi+0x40]; call 0x475af0 ; member ctor
0x47002a  ret 8
```
The thread is handed the **+4 subobject** (vtable **0xb55d50**), so `vtbl[1]` =
`*(0xb55d50+4)` = **0x4700b0**. This is a textbook `IRunnable`: slot0 = destructor,
slot1 = `run()`.

### 1.3 vtable map (read from `cm2008_full`, cross-checked against crack file 0x755xxx)
| vtable | slot0 (+0) | slot1 (+4) | slot2 (+8) | role |
|---|---|---|---|---|
| **0xb55d60** (primary base) | 0x470190 *(deleting dtor)* | `be4eb0`=RTTI | – | base interface, dtor only |
| **0xb55d50** (runnable base) | 0x470160 *(adjustor→dtor)* | **0x4700b0 = `run()`** | 0x470150 *(adjustor thunk)* | **the thread interface** |
| 0xb55d18 | 0x46fdd0 *(deleting dtor, installs 0xb55d18)* | 0xa3e322 *(shared stub/purecall)* | 0xa3e322 | abstract base |
| 0xb55d28 | 0x470170 *(deleting dtor)* | — (rodata "text/plain…") | — | the 0x190 result/log object |

Adjustor thunks confirm the MI layout: `0x470160: sub ecx,4; jmp 0x470190` (re-base the
runnable subobject to the primary before the real deleting dtor); `0x470150: add ecx,0x3c;
jmp 0x475cf0`.

### 1.4 Why the lead's "vtbl[0] = real work" is wrong (VERIFIED from bytes)
All three slot-0 methods are **destructors** (standard MSVC deleting-destructor shape
`if (flag&1) operator delete(this)`):
```
0x46fdd0  test [esp+4],1; mov esi,ecx; mov [esi],0xb55d18; je .; push esi; call 0xa3cd30(delete); .: mov eax,esi; ret 4
0x470190  mov esi,ecx; call 0x470030(base dtor); test [esp+8],1; je .; push esi; call 0x4749b0(delete); .: ret 4
0x470160  sub ecx,4; jmp 0x470190      ; adjustor → same deleting dtor
```
`FUN_0046ff90` therefore calls `0x46fec0` (the work/finalize) and then `(*vtbl[0])(1)` =
**the deleting destructor with the delete flag set** = "run the finalize, then delete the
object". This exactly matches the harvest side in `THREADED_SIMULATION_RECOVERY.md §4(a)`
(`"End of fixture %d that ran on a thread."` + `(**node+0x10)(1)` release).

---

## 2. Decompiled `run()` body and its callees (VERIFIED, crack disassembly)

### 2.1 `run()` = 0x4700b0 — dispatch / harvest / post-finalize
```
0x4700b0  … SEH …; mov esi,ecx (this); mov edi,[esp+0x20] (fixture ctx arg)
0x4700d9  mov ecx,edi; call 0x5b3ba0 ; test eax; je done   ; fixture predicate (picker cluster)
0x4700e4  lea ecx,[esi+0x3c]; call 0x475bf0 ; ebx=result    ; construct a child worker Thread
0x4700ee  test ebx; je done
0x4700f2  mov ecx,edi; call 0x472350
0x4700f9  push 0x190; call 0x4749d0 (operator new); add esp,4 ; allocate 0x190-byte object
0x47011d  …; mov ecx,eax; call 0x46fdf0                        ; construct result obj (edi, esi+4)
0x470127  push 0x46ff90                                        ; completion/finalize functor
0x47012e  mov ecx,ebx; call 0x475760                           ; enqueue (callback=0x46ff90, arg=obj)
0x47013b  done: … ret 4
```
`run()` validates the fixture, spins up a child `Thread` on `[this+0x3c]`, wraps the fixture
into a 0x190-byte result/log object, and **enqueues the finalize callback**. It does **not**
itself touch any geometry entity.

- **0x475760 (enqueue)** `[q+0x10]=obj; [q+0xc]=0x46ff90; add ecx,0x18; call 0x472ef0` — stores
  completion `(callback,arg)` then signals. VERIFIED.
- **0x46fdf0 (result-obj ctor)** `mov [esi],0xb55d28; [esi+4]=arg1; [esi+8]=arg2;
  call 0x473d50` — installs the result vtable and stores a fixture id + ptr + a std::string
  member (0xc). Holds results/log data, **no geometry fields**. VERIFIED.

### 2.2 `FUN_0046ff90` — the completion/finalize functor (VERIFIED)
```
0x46ff90  push esi; mov esi,[esp+8]; mov ecx,esi; call 0x46fec0 ; build log string
0x46ff9c  test esi,esi; je .
0x46ffa0  mov eax,[esi]; mov edx,[eax]; push 1; mov ecx,esi; call edx ; vtbl[0](1)=delete this
0x46ffaa  .: pop esi; ret
```
Only reference to 0x46ff90 in the whole image is the `push 0x46ff90` at 0x470128 (the
enqueue above). VERIFIED (byte-search: 1 hit).

### 2.3 `FUN_0046fec0` — the "…ran on a thread" log builder (VERIFIED)
SEH + 0x400-byte stack buffer; reads result-obj members [esi+4]/[esi+8]/[esi+0xc]; formats
with the literal at **0xb55d2c** via std::string ops (0x5b4110/0x473500/0x5c3fa0/0x4747b0).
This is the finalize/log, matching the GCMatchModule harvest string. Not the simulation.

### 2.4 Thread closure scan (crack, seeds {0x4700b0, 0x46ff90, 0x46fec0, 0x46fdf0})
Direct-call BFS over `.text`, 78 functions reached:
- **Geometry operands (+0x94..0xD4): 0.**
- **Waypoint operands (+0xb4..0xBA): 0.**
- Calls leaving `.text`: 18, **all** into CRT/exception/string helpers (0xa3cd40 free,
  0xa58b**/0xa42700 SEH/string). **None** into 0x77xxxx or 0x9bxxxx–0x9fxxxx or 0xa1xxxx.

---

## 3. Integrator verdict — **VERIFIED NEGATIVE** (no per-frame RMW integrator)

Two independent findings:

**(a) Not in the thread.** `run()`'s closure has zero geometry access (§2.4).

**(b) No RMW anywhere in the statically-reachable sim.** Read-only scan of every defined
function in 0x77xxxx / 0x9bxxxx–0x9fxxxx / 0xa1xxxx for the integrator signature
(load `FLD/FILD/MOVSS/…` from `[reg+disp]` **and** store `FSTP/FISTP/MOVSS/…` to the same
`[reg+disp]`, disp ∈ +0x94..0xD4):
- **RMW sites: 0.** (x87 and SSE both covered; no SSE float store to these offsets exists at all.)
- Float stores to the geometry block (+0x94..0xD4) via a register base: **31 — all inside the
  single seeder `FUN_009f0090`** (two init paths; writes +0x94..+0xB8 raw vectors and
  +0xc0..+0xD4 attribute scalars). No other function writes the block.

⇒ The geometry block is **only seeded (set), never read-modify-written**. There is no
continuous position/velocity/accel integrator in statically reachable code; movement is
**set-to-target** (consistent with the waypoint-polyline consume model in
`WAYPOINT_PROVIDER_RECOVERY.md`). The "per-frame integrator" assumed by earlier docs
**does not exist as an RMW**; the per-frame *position update* is a set-to-target write whose
decision input comes from the picker. Classification: **VERIFIED NEGATIVE** for a
thread-private RMW integrator and for any RMW integrator in the scanned sim cluster.

*Residual (honest caveat):* a position write performed purely through a `rep movs`/struct
copy would not appear as a `[reg+disp]` store, and this scan covered only 0x77 / 0x9b–0x9f /
0xa1. (The picker cluster 0x5b3000–0x5b5300 is *not* a boundary — it is statically readable
and was scanned: 0 geometry operands.) So the residual is a scan-scope/representation gap,
not a protection boundary.

---

## 4. Filler verdict — **VERIFIED NEGATIVE** (thread-private); re-confirms Agent D

- **Not in the thread.** §2.4: zero +0xb4..0xBA operands in `run()`'s closure.
- **No non-0xffff u16 store in the sim cluster**, by either path:
  - *Literal displacement:* the only `word`-sized stores to `[reg+0xb4..0xBA]` are the two
    known resets — `FUN_009f0c40` (per-tick flush) and `FUN_009f0fe0` (entity reset), both
    writing **0xffff**. (This matches Agent D's image-wide fingerprint.)
  - *Non-literal `lea`:* the register-based `lea r,[base+0xb4]` sites
    (`FUN_009c3fe0 / 009c4860 / 009c5420 / 009c8480`) operate on a **different object
    layout** — there +0xb4/+0xb8/+0xbc are `std::vector<u16>` begin/end/cap pointers
    (serialization clone, `push_back`, `memmove`), **not** the entity's four scalar u16 refs.
    Ruled out (as Agent D already ruled out the dword +0xb4 objects).

⇒ No function writes a real (non-empty) waypoint ref into the match-entity quad in any
statically reachable code. The selector/filler is **not thread-private**. Given `run()`
*does* call into the picker cluster (0x5b3ba0 ∈ 0x5b37xx–0x5b52xx) only as a boolean
predicate, the actual ref selection most plausibly lives inside that **SecuROM-protected
picker (runtime-only)** or is applied via a struct/`rep movs` copy. Classification:
**VERIFIED NEGATIVE** (thread-private) + **LOCATED_NOT_UNDERSTOOD** (producer; picker is a
protection boundary, per `research/RUNTIME_CAPTURE_HOWTO.md`).

---

## 5. Classifications + corrections to prior leads

| item | addr | verdict | class |
|---|---|---|---|
| Thread proc (virtual trampoline → vtbl[1]) | 0x4755d0 | `mov ecx,[esp+4]; mov eax,[ecx]; mov edx,[eax+4]; call edx` | VERIFIED |
| Worker object ctor (MI; installs 0xb55d60@+0, 0xb55d50@+4) | 0x46ffb0 | runnable interface = 0xb55d50 | VERIFIED |
| **`run()`** (vtbl[1] of 0xb55d50) | **0x4700b0** | dispatch/harvest/post-finalize; no geometry | VERIFIED / DECOMPILED |
| vtbl[0] of 0xb55d60/50/18/28 | 0x470190/0x470160/0x46fdd0/0x470170 | **destructors** (lead's "real work" refuted) | VERIFIED |
| Finalize functor | 0x46ff90 | `log(0x46fec0); delete this` | VERIFIED |
| "…ran on a thread" log builder | 0x46fec0 | string build, literal @0xb55d2c | VERIFIED |
| Thread closure (78 fns) | — | 0 geometry, 0 waypoint, 0 sim calls | VERIFIED |
| Per-frame RMW integrator | — | **does not exist** (0 RMW; seeder-only) | VERIFIED NEGATIVE |
| Geometry seeder (only block writer) | FUN_009f0090 | sets +0x94..+0xD4 (x87) | VERIFIED |
| Waypoint filler (non-0xffff) | — | **not thread-private; absent from sim cluster AND picker cluster** | VERIFIED NEGATIVE / LOCATED_NOT_UNDERSTOOD |
| Picker predicate called by run() | 0x5b3ba0 | `mov eax,[ecx+0xc]; ret` (find-by-key accessor, no geometry) | VERIFIED |
| Waypoint reset/flush | FUN_009f0c40 / FUN_009f0fe0 | write 0xffff only | VERIFIED |

**Corrections recorded:** (1) prior docs' `0x700b0`/`0x75620`/`0x7562b` are the VAs
`0x4700b0`/`0x475620`/`0x47562b`. (2) The thread dispatch is through **vtbl[1]**, and
**vtbl[0] is the destructor** — the "data-dependent vtbl[0] = real work" lead is refuted.
(3) `SIM_THREAD_ENTRY.md`'s player-pool body for 0x4755d0 is a mis-aligned disassembly; the
real proc is the 5-instruction trampoline above. (4) `cm2008_full`'s 0x46xxxx–0x4702xx bytes
are corrupt — use the crack `.text` for this region.

## 6. Next step (to close the two remaining LOCATED_NOT_UNDERSTOOD items)
The integrator and filler are **not** in the worker thread, **not** as RMW/literal-store in
the sim cluster, and **not** in the picker cluster (all three statically scanned here). Two
concrete static follow-ups remain, both in reach (no DRM boundary):
1. **Widen the geometry/waypoint-write scan beyond 0x77/0x9b–0x9f/0xa1** — the per-frame
   set-to-target writer of +0x94..+0xB8 and the non-0xffff +0xb4 selector were not found in
   any region examined, so scan the rest of `.text` (esp. 0x6xxxxx match logic and 0xa2xxxx)
   for register-based stores to those offsets and for `rep movs`/struct copies whose
   destination is the entity (`entity+0x134` block). This is the most likely home given the
   consumer `FUN_009f0c40` demonstrably reads non-0xffff refs at runtime.
2. **Weigh the statistical-engine hypothesis.** Only the seeder `FUN_009f0090` writes the
   block in the scanned sim, and no RMW integrator exists — consistent with CM's heavily
   documented FORMULA_*/RESOLVER_* predicate engine resolving fixtures statistically rather
   than by continuous spatial integration. If the wider scan (step 1) also finds no per-frame
   position writer, that is itself the answer: the "geometry integrator" is a seed-once view
   artifact, not a physics loop.
A runtime write-watchpoint on entity+0xb4 and the +0x94..+0xB8 floats
(`research/RUNTIME_CAPTURE_HOWTO.md`) remains the fastest confirmation but is no longer the
*only* path — the picker being clean removes the former protection blocker.
```
```
Scripts used (kept in session scratchpad, not committed): `disx.py`, `closure.py`
(capstone over the crack); `Recover.java`, `FindRMW.java`, `FindPos.java` (read-only over
`cm2008_full`).
