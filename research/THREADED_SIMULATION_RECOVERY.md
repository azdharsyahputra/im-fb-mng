# CM2008 Threaded Match-Simulation Recovery

**Workstream: Agent 2 — Thread & Scheduler Investigation. STATIC ONLY** (no
execution, no DRM bypass). Evidence from committed artifacts:
`docs/ghidra_dumps/raw/match_engine.c`, `research/SIM_THREAD_ENTRY.md`,
`research/MATCH_CLOCK_AND_UPDATE_LOOP.md`,
`research/DETAILED_MATCH_ENGINE_CALLGRAPH.md`, and the FindGeom hand-disasm scan
(`scratchpad/fg3.txt`). The re-export recipe was **not** run (single-access; the
existing artifacts fully answer the questions).

Classifications per finding use exactly:
`NOT_INVESTIGATED / LOCATED_NOT_UNDERSTOOD / DECOMPILED / STRUCTURALLY_UNDERSTOOD /
BEHAVIORALLY_VALIDATED / GENUINELY_UNAVAILABLE`.

---

## 0. BOTTOM LINE

**The watched match is a single-threaded cooperative pump, NOT a dedicated
match-sim worker thread.** `GCMatchModule::Update/ProcessData` (`FUN_0077ee80`) is
invoked *purely virtually* (vtable `0x00ba2904` slot `+0xa8`) once per program-step
by the module manager. There is no tick loop and no sim thread.

Threads *do* exist, but only two `CreateThread` sites in the whole image, and
neither is the watched-match tick:
- one is CRT/housekeeping (not match);
- one is a **generic `Thread` class** used to run **background / off-screen
  fixtures** "on a thread." `ProcessData` branch (a) is the **harvest/finalize**
  side of that generic worker — it collects *completed* background fixtures and
  applies their results each pump step. **No `_beginthreadex`; no synchronization
  primitives of any kind** (Mutex / CriticalSection / Event / WaitForSingleObject /
  Interlocked) are present anywhere in the dumps or the hand-disasm — consistent
  with a cooperative harvest model, not a producer/consumer lock protocol.

---

## 1. CreateThread sites + start routines + callback struct

**Important mapping caveat (VERIFIED):** the Ghidra dumps (`match_engine.c`, the
big dump) do **not** contain the `CreateThread` import, the string, or the thread
procs — the dump's `.text` ends at `0x711000` and the import/IAT region is mapped
differently. All CreateThread facts below come from the **hand-disassembly of the
actual `cm2008.exe`** recorded in `research/SIM_THREAD_ENTRY.md` (capstone,
ImageBase `0x400000`), corroborated by the FindGeom scan. Grep of the dumps for
`CreateThread|beginthread|Mutex|Event|WaitForSingleObject|Interlocked|
CriticalSection` returns **0 hits** — i.e. none of the thread/sync machinery is in
the decompiled match code, only the *result-harvest* logic is.

- **Import:** `KERNEL32.dll!CreateThread`, IAT slot VA `0xb110dc`, name string at
  `0x806f30`. **No `_beginthread/_beginthreadex`** (byte-searched, absent).
  Call pattern `FF 15 DC 10 B1 00` occurs at **exactly two** `.text` sites:
  `0x314f5` and `0x75633`. — `BEHAVIORALLY_VALIDATED` (import/byte evidence).

### Site #1 — `0x314f5`, thread proc `0x4314b0` — NOT match
Thin stdcall wrapper `0x314e0` pushes `lpStartAddress=0x4314b0`,
`lpParameter=caller arg`, `lpThreadId=&esp[4]`. Proc `0x4314b0` reads globals
`[0xc77668]/[0xc8fb60]/[0xc8fb6c]`, calls `0x262cf0`, does string/buffer teardown,
returns a float. No match structure touched. Wrapper has zero pointer refs.
**Identity:** CRT/runtime housekeeping thread. — `STRUCTURALLY_UNDERSTOOD`
(VERIFIED not-match; exact role INFERRED).

### Site #2 — `0x75633`, thread proc `0x4755d0` — generic `Thread` class, match worker
`Thread::start` (thiscall, `this`→ecx/esi) at `0x75620`:
```
0x75623 lea eax,[esi+8]      ; lpThreadId  = &this[8]
0x75629 push esi             ; lpParameter = this  <-- callback struct = the Thread object
0x7562a push 0x4755d0        ; lpStartAddress (trampoline)  <-- thread proc #2
0x75633 call dword [0xb110dc]
0x75639 mov [esi+4],eax      ; store HANDLE at this+4   (this+8 = thread id)
```
**Callback/argument structure = the `Thread` object itself**, passed as
`lpParameter`. Object layout (from the start stub + construction chain):
`this+0x4 = HANDLE`, `this+0x8 = thread-id`, plus a stored **work function**
(`0x46ff90`) set by the owner. Construction chain (VERIFIED, SEH frames):
`0x700b0` (stores work-fn `0x46ff90`; calls `0x72350/0x6fdf0/0x75760`) →
`Thread` ctor `0x75bf0` → `Thread::start 0x75620`. The only pointer ref to
`0x4755d0` is the push at `0x7562b`.

**Thread proc #2 `0x4755d0`** (static entry lands 16 bytes inside fn `0x4755c0`;
the 16-byte prologue is SecuROM-mutated, body from `0x4755d3` is genuine,
entropy ~6.0):
```
mov eax,[0xc94cf8]; mov edi,[eax+0xac]   ; player index from a match/view ctx
cmp edi,0x80000..0xfffff                 ; sentinel/bounds
mov ecx,[0xc784a8]                        ; *** player pool DAT_00c784a8 ***
shl edi,5 ; add edi,[ecx+0x2c]            ; stride 0x20 → &pool.array[index]
call 0x5e0050                             ; entity validity check
... 0x1a6170 / 0x193cc0 / 0x261710 / 0x21d480  (match accessors)
call 0x1ee2b0(this+0x4c,+0x54) ; call 0x1ee2b0(this+0x4c,+0x58) ; copy coord vecs
ret                                       ; <-- RETURNS: short routine, NOT a loop
```
**VERIFIED:** touches player pool `DAT_00c784a8` (stride `0x20`), validates an
entity, copies two transform vectors into the thread param object, and **returns**
(no per-frame tick loop). **Identity (INFERRED):** the engine's general-purpose
`Thread` trampoline; this instance is the per-fixture / view worker that simulates
a background fixture off the main pump. — `STRUCTURALLY_UNDERSTOOD`
(pool access VERIFIED; worker role INFERRED).

---

## 2. Possible worker entry point (the fixture-simulating function)

The function run on the worker thread is the `Thread`-class trampoline
**`0x4755d0`** (→ real fn `0x4755c0`), driven by a stored **work function
`0x46ff90`** set at construction site `0x700b0`. Because the proc returns after
reading/writing one entity's transform, the model is **one short job per
dispatch**, not a long-lived loop — i.e. a fixture is simulated by a (re)dispatched
job whose completed result object is later harvested by `ProcessData` branch (a).
— `LOCATED_NOT_UNDERSTOOD` for `0x46ff90`'s full body (outside the committed
dumps; would need re-export of the `0x46xxxx/0x70xxxx` region); the dispatch
plumbing itself is `STRUCTURALLY_UNDERSTOOD`.

---

## 3. Scheduler + synchronization

**No scheduler object and no synchronization primitives were found.** Grep across
`docs/ghidra_dumps/**` for `WaitForSingleObject | WaitForMultipleObjects |
CriticalSection | CreateMutex | CreateEvent | SetEvent | Interlocked |
ResumeThread | SuspendThread` → **0 hits**; SIM_THREAD_ENTRY's byte-search of the
binary likewise found no join/wait around the `Thread` class (only `CreateThread`
and the handle store at `this+4`). The only cross-thread handoff visible is the
**result list** consumed in `ProcessData` branch (a) (§4). This strongly implies a
**fire-and-harvest** model: background fixtures are dispatched, and finished ones
are drained on the next cooperative pump step without explicit locks (the harvest
runs on the main pump, serialized by the single-threaded Update). — the *absence*
is `BEHAVIORALLY_VALIDATED` (grep + byte-search); any hidden locking would be
`GENUINELY_UNAVAILABLE` under static-only w/ SecuROM.

---

## 4. Connecting worker execution to ProcessData (how fixtures are dispatched/harvested)

`FUN_0077ee80(int *param_1)` branches on `*param_1` (the dispatch arg):

### Branch (a) `*param_1 == 0` — BACKGROUND / THREADED-FIXTURE HARVEST
(`match_engine.c` lines 649–784). Two **red-black-tree / `std::map`-style** node
lists hang off the module context `in_ECX`:
- list 1: head `in_ECX+0x58`, iterator `in_ECX+0x60`, flag `+0x68`; per node calls
  `FUN_0077d800` (tree insert/rebalance) — the **enqueue/registration** side.
- list 2: head `in_ECX+0x70`, iterator `in_ECX+0x78`, flag `+0x80`; per node:
  ```
  uVar9 = FUN_005a6320();                       // fixture entity→handle (the %d)
  FUN_00411c10(0x1a,2,"End of fixture %d that ran on a thread.",uVar9);  // 0x7a2e60
  if (DAT_00c84944) { vcall(ctx+0x98,+4); FUN_009efc90(); vcall(ctx+0x98,+0xc); }
  iVar10 = *(int*)(iVar12+0x10);                // <-- the completed-fixture RESULT struct
  FUN_0077cf20(*(iVar10+8), iVar10+0xc, 0, *(byte*)(iVar10+0x19), *(iVar10+0x20)); // apply
  if (node+0x10) (**node+0x10)(1);              // release/destroy the result object
  ```
  This is the **harvest/finalize** side: each node carries a finished-fixture
  result object at `node+0x10` (fields `+0x8/+0xc/+0x19/+0x20`), which
  `FUN_0077cf20` consumes to apply the fixture outcome; `FUN_0056b600` then frees
  the list. **So fixtures are NOT dispatched from inside ProcessData** — dispatch
  is via the `Thread` class (§1–2) elsewhere; ProcessData branch (a) only
  **collects completed background fixtures**, logs them, and applies results.
  **The worker is effectively per-fixture** (one result node per fixture; the
  `Thread` proc is a short per-entity job, not a shared loop). — `DECOMPILED`.

### Branch (b) `*param_1 != 0` — DETAILED PER-TICK of the watched match
(lines 786–882). Confirmed call order **ccb0 → cd40 → e850 → cf20 → c660**:
1. `FUN_0077ccb0` — pause/abort-restart gate (early-return if set). `DECOMPILED`.
2. `FUN_0077cd40` — resolve / action-state dispatch; pulls RNG
   (`0x7e7c10`, LCG `x*0x19660d+0x3c6ef35f`) and **QUEUES** event requests
   (`+0x10818/+0x1081c`, `+0x1072c`). → the **enqueue** step. `DECOMPILED`.
3. `FUN_0077e850` — match-analysis / FREEZE logger + **clock-boundary check**:
   reads `*(ctx+0xe4)+0x4c` vs `3600.0f` (`DAT_00c84c7c`) → `eGameStateHalfOver /
   GameOver` trigger; `eGameState::ToString` = `FUN_0077ddc0`. `DECOMPILED`.
4. `FUN_0077cf20` — finalize / **event-EXECUTE**: `FUN_00a2eb00` per-fixture
   stat/report writer (guarded by `DAT_00c31b34` + active-team match via
   `005a06c0/005a61c0`), consumes queued events, commits `eGameState`, runs the
   predicate-rank driver `FUN_0077c540 → FUN_009e9570` (DAG ranker, threshold
   80.0f). `DECOMPILED`.
5. `FUN_0077c660` — **continuous float clock**: `*(ctx+0xe4)+0x44` (elapsed) +
   `+0x48` (base) → hours (`*1/3600`) / minutes (`*1/60`) bitfield. `DECOMPILED`.

**`cd40` queues, `cf20` executes** — confirmed. `FUN_0077cf20` is reused in both
branches: in (a) to *apply a finished background fixture*, in (b) as the
*per-tick event-execute* stage.

---

## 5. Event cadence + execution order

- **Cadence:** per **program-step / UI-message pump**, NOT a fixed time tick.
  `GCMatchModule::Update` is called virtually (vtable `0x00ba2904+0xa8`) by the
  module manager each step; the module-manager loop that issues the indirect
  `call [reg+0xa8]` is not statically resolvable to one caller (module created via
  factory `FUN_0077d890`, 0 direct callers). — `STRUCTURALLY_UNDERSTOOD`
  (manager loop itself `GENUINELY_UNAVAILABLE` under static-only).
- **Clock:** continuous **float seconds accumulator** (`matchCtx+0xe4 → +0x44
  elapsed / +0x48 base`), advanced per step and formatted ÷60/÷3600 — explicitly
  **not** integer minute/second ticks. — `BEHAVIORALLY_VALIDATED`.
- **Per-step order:** branch select on `*param_1` → (a) drain/finalize completed
  threaded fixtures, else (b) `ccb0 → cd40(queue) → e850(clock/FSM boundary) →
  cf20(execute+rules) → c660(advance clock)`.
- **Worker granularity:** per-fixture (one short dispatched job + one harvest node
  per background fixture); the watched match runs inline on the pump.

---

## 6. Summary table

| item | addr | verdict | classification |
|------|------|---------|----------------|
| CreateThread sites | `0x314f5`, `0x75633` | only two in image | BEHAVIORALLY_VALIDATED |
| Thread proc #1 | `0x4314b0` | CRT/housekeeping, not match | STRUCTURALLY_UNDERSTOOD |
| Thread proc #2 / `Thread::start` | `0x4755d0` / `0x75620` | generic Thread class; per-fixture/view worker; touches pool `0xc784a8` stride 0x20; returns (no loop) | STRUCTURALLY_UNDERSTOOD |
| Worker work-fn | `0x46ff90` (set at `0x700b0`) | stored job function | LOCATED_NOT_UNDERSTOOD |
| Callback struct | the `Thread` object (`lpParameter`) | `+0x4`=HANDLE, `+0x8`=tid, work-fn | STRUCTURALLY_UNDERSTOOD |
| Sync primitives | — | NONE present (grep + byte-search) | BEHAVIORALLY_VALIDATED (absence) |
| ProcessData branch (a) | `0077ee80` L649–784 | harvest/finalize completed threaded fixtures (2 RB-tree lists; result @ node+0x10) → `0077cf20` | DECOMPILED |
| Per-tick chain (b) | ccb0→cd40→e850→cf20→c660 | queue→clock/FSM→execute+rules→advance clock | DECOMPILED |
| Continuous match sim | `0077ee80` virtual `+0xa8` | cooperative pump, not threaded | BEHAVIORALLY_VALIDATED |

## 7. Next step
Re-export the `0x46xxxx` / `0x70xxxx` region (via `research/ghidra_reexport/
reexport.sh`, single-access) to decompile work-fn `0x46ff90` and construction site
`0x700b0`, and to confirm whether the `Thread` class is reused for anything besides
background fixtures — and to search that region for any IAT-adjacent Wait/handle
usage the current dumps omit. A runtime trace (out of scope here) would confirm the
per-fixture dispatch cardinality.
