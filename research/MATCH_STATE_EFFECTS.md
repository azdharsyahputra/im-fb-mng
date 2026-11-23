# Match State Writer, Dispatch & Effect Targets (Workstream D)

Clean-room, **static only**. No Ghidra run, no binary execution. Evidence = the STATE
cluster closure (`scratchpad/wave3.log`), the formula/rule closure
(`scratchpad/formula.log`), and **raw bytes** of `/Volumes/CM2008/Crack JPN/cm2008.exe`
(file_offset = VA − 0x400000, read with `dd`/`xxd` + hand-decoded x86; capstone/objdump
raw-blob disassembly unavailable, so writer bytes were decoded manually and by targeted
byte-pattern scans).

Classification: **VERIFIED** (bytes/decompile prove it), **INFERRED** (structure implies),
**APPROXIMATE** (located, meaning partial), **UNKNOWN** (not statically recoverable).
Supersedes §3 of `EGAMESTATE_COMPLETE.md` (whose "009ef family is the SetGameState writer"
hypothesis is **REFUTED** below).

---

## 0. TL;DR

- The eGameState int `*(*(matchCtx+0xe4)+0x30)` is **only ever READ** in the entire
  match-engine region (0x77c600–0x77f200). No writer exists in scope. **VERIFIED.**
- The prior prime suspect — the `FUN_009ef*` match-context family — is **NOT** the writer.
  Byte-decoded: every member and every deep target writes the **GCMatchModule event-request
  queue** (`+0x1072c`, `+0x10818`, `+0x1081c`), never `+0x30`, never touches `+0xe4`.
  **VERIFIED (by byte scan of all 8 deep targets).**
- Binary-wide byte scan for the canonical `SetGameState` idiom — *load `[reg+0xe4]` then
  store `[sameReg+0x30]`* — returns **ZERO** hits anywhere in 36 MB. **VERIFIED.**
- State transitions are driven by a **two-stage event-request queue** (request in resolve
  `FUN_0077cd40`; execute in finalize `FUN_0077cf20`), not a direct setter and not a vtable
  call. The visible `+0x30` int is committed by the queue's **execution consumer, which is
  out of the dumped closure**. Writer verdict = **UNKNOWN but precisely bounded** (a thiscall
  method of the clock/state sub-object class). **INFERRED.**
- `function`/`callrule` effects: the recovered closure writes **only** computed floats to
  node scratch (`output+0x14`, `term+0x30`). **No** writes to the match-stat slab
  (`+0xe` Performance / `+0x10` Condition), player attributes, tactics, or match state.
  Effect game-data targets live in un-recovered named-native-function callees → **UNKNOWN**;
  observed keys are **management/AI weighting**, not real-time match mutation. **VERIFIED by
  absence in closure + INFERRED from key strings.**

---

## 1. The eGameState WRITER — verdict & evidence

### 1.1 Field recap (VERIFIED, from `EGAMESTATE_COMPLETE.md`)
```
matchCtx   = *(module+4) + 0x110            // FUN_009efa90: return *(this+4)+0x110
subObj     = *(int*)(matchCtx + 0xe4)       // clock/state sub-object
eGameState = *(int*)(subObj + 0x30)         // raw int 0..0x1f
```
Clock fields on the same subObj: `+0x44` elapsed(f), `+0x48` base/offset(f), `+0x4c` freeze
counter.

### 1.2 The 009ef family is REFUTED as the writer (VERIFIED)

The `FUN_009ef*` members are **thin thiscall wrappers** forwarding to `0x9fxxxx` bodies
(decompile, `wave3.log`):

| wrapper | forwards to | role (byte-decoded) |
|---|---|---|
| `FUN_009efa10` | `FUN_009f7d90` | post request; writes module `+0x1072c` (`89 86 2c 07 01 00`) |
| `FUN_009efa20` | `FUN_009f9400` | post request; writes module `+0x10818` (`89 be 18 08 01 00`) & `+0x1081c` (`89 86 1c 08 01 00`) |
| `FUN_009efa40` | `FUN_009f2410` | consume; tests `+0x10818!=0`, validates `+0x1072c∈[2,3]`, clears `+0x10818=0` (`c7 86 18 08 01 00 0`) |
| `FUN_009efa70` | `FUN_009f9760` | execute/render |
| `FUN_009efa30` | `FUN_009f9680` | (event helper) |
| `FUN_009efa60` | `FUN_009f21b0` | (event helper) |
| `FUN_009efc30` | `FUN_009f0c40` | report/finalize |
| `FUN_009efb40` | `FUN_009effb0` | (event helper) |
| `FUN_009efb10` | — (inline) | **getter**: `return *(*(this+4)+0x1081c)` (event-result index) |
| `FUN_009efa90` | — (inline) | **getter**: `return *(this+4)+0x110` (matchCtx) |
| `FUN_009efab0` | `FUN_0048c4a0` | predicate helper |

**Byte-scan of all 8 deep targets** (`0x9f7d90, 0x9f9400, 0x9f2410, 0x9f9760, 0x9f9680,
0x9f21b0, 0x9f0c40, 0x9effb0`) for any `+0xe4` access and any `+0x30` store:

```
FUN_9f7d90 : +0xe4 access = []   +0x30 stores = []
FUN_9f9400 : +0xe4 access = []   +0x30 stores = []
FUN_9f2410 : +0xe4 access = []   +0x30 stores = []
FUN_9f9760 : +0xe4 access = []   +0x30 stores = []
FUN_9f9680 : +0xe4 access = []   +0x30 stores = []
FUN_9f21b0 : +0xe4 access = []   +0x30 stores = []
FUN_9f0c40 : +0xe4 access = []   +0x30 stores = []
FUN_9effb0 : +0xe4 access = []   +0x30 stores = []
```
→ **None** of the 009ef family writes the eGameState. They operate on a **separate**
event-request record embedded in the GCMatchModule (`this`), not on the `ctx+0xe4`
sub-object. **VERIFIED.** This refutes `EGAMESTATE_COMPLETE.md §3`'s prime-suspect list.

### 1.3 Binary-wide: the SetGameState idiom does not exist (VERIFIED)

Byte-pattern scan of the full 36 MB for *load `[reg+0xe4]` (disp32) then store
`[sameReg+0x30]`* (tracking the load's destination register across a 64-byte window;
store = `89`/`c7`/`88`):
```
e4-load -> store [sameReg+0x30] : 0
```
There is **no** place in the binary where the sub-object pointer is freshly loaded from
`+0xe4` and immediately written at `+0x30`. The writer therefore holds the sub-object
pointer **as a passed-in `this`/param**, and writes `[this+0x30]` with no `+0xe4` reload.

### 1.4 Inside the state region +0x30 is READ-only (VERIFIED)

Scan of 0x77c600–0x77f200:
| site | bytes | meaning |
|---|---|---|
| `0x77c6ed` | `8b 50 30` | `mov edx,[eax+0x30]` — READ (clock path) |
| `0x77ebc0` | `8b 91 e4.. ; 8b 72 30` | `mov edx,[ecx+0xe4]; mov esi,[edx+0x30]` — the **eGameState READ** feeding ToString `FUN_0077ddc0` (`wave3.log:14372-14373`) |
| `0x77d429` | `c7 47 30 f4 28 ba 00` | store **pointer** `0xBA28F4` to `[edi+0x30]` — vtable/type install on a *list/iterator node*, not the state int |
| `0x77d7aa/8cb/8de` | `c7 46 30 <ptr>` | same: install `0xBA27F0`/`0xBA28F4` (VAs, not state values 0..0x1f) |

The only `+0x30` *stores* in the region write **code/data pointers** into other
constructed objects (confirmed: immediates are `0xBAxxxx` VAs), and their base regs are
never fed from `+0xe4`. So the eGameState `+0x30` is **never written in the state region**.
**VERIFIED.**

### 1.5 Writer verdict: **UNKNOWN (precisely bounded)**

The writer is a **method of the clock/state sub-object class** (`this == *(matchCtx+0xe4)`),
living in the **event-execution consumer** that is *outside* the dumped STATE/formula
closures. It is statically indistinguishable from thousands of generic `mov [this+0x30],x`
stores without disassembling every candidate method (blocked: no Ghidra, no capstone,
objdump cannot disassemble a raw blob). **No vtable slot** for a state setter appears in
scope — the 009ef dispatch is via **direct calls to flat C wrappers**, not virtual
dispatch; the sub-object itself shows no observed vtable use in the cluster.

**To resolve:** a runtime **write-watchpoint on `*(*(matchCtx+0xe4)+0x30)`**, or disassembly
of the event-execution consumer reached from `FUN_0077cf20 → FUN_009efa40(→9f2410)` /
`FUN_009efa70(→9f9760)` where the queued request (`+0x1081c` index) is turned into the
visible restart.

| reader/writer | address | op | status |
|---|---|---|---|
| **Reader** (ToString feed) | `0x77ebc0`/`0x77e850` → `FUN_0077ddc0` | `mov [ctx+0xe4]; [edx+0x30]` | **VERIFIED** |
| **Reader** (clock path) | `0x77c6ed` (`FUN_0077c660`) | `mov edx,[eax+0x30]` | VERIFIED |
| **Writer** | — | thiscall method of subObj, `[this+0x30]=state` | **UNKNOWN** (out of scope; bounded) |
| ~~009ef family~~ | 0x9f7d90/9400/2410/… | write `+0x1072c/+0x10818/+0x1081c` | **NOT the writer (VERIFIED)** |

---

## 2. The indirect state-dispatch mechanism — two-stage event queue (VERIFIED structure)

State is **not** set by a `SetGameState(x)` call. It flows through an **event-request queue**
on the GCMatchModule, decoupling *detection* from *execution*:

**Stage 1 — REQUEST (in resolve `FUN_0077cd40`, per tick):** after event resolution
(`FUN_0077c710`, LCG `*0x19660d+0x3c6ef35f`), it posts a request:
```c
if (*(param_2+6) == 0) {
  if (DAT_00c84954==0 && (*(param_2+5) || DAT_00c84950))
        FUN_009efa20(1, DAT_00c84958, eventCode, 0);   // 9f9400: +0x10818=pending, +0x1081c=result
  else if (*(param_2+1)==0)
        FUN_009efa20(0, DAT_00c84958, eventCode, 0);   // 9f9400 (normal/no-force)
  else { FUN_0077c710(eventCode); ... }                // direct resolve
} else {
  FUN_009efa10(3, id);                                 // 9f7d90: +0x1072c = type, under [ebp+4]&0xf000 & type==3
}
```
**Stage 2 — EXECUTE (in finalize `FUN_0077cf20`):**
```c
FUN_009efa70();   // 9f9760  execute
FUN_009efa40();   // 9f2410  consume: if(+0x10818) && +0x1072c in [2,3] -> handler; +0x10818 = 0
FUN_009efc30();   // 9f0c40  report
... FUN_0040adf0(0x414, ...);   // emit render/report command 0x414
```
**Readback:** `FUN_0077e850` calls `FUN_009efb10()` → `*(module+4)+0x1081c` (the event-result
index) and maps it `iVar6*0x20 + *(DAT_00c784a8+0x2c)` into an event table
(`wave3.log:14950-14958`).

So the "indirect dispatch" is: **resolve queues an event code → finalize executes it →
the execution consumer commits the visible `+0x30` state.** The 009ef layer is the queue
API (direct calls through thiscall wrappers), **not** the state mutator. **VERIFIED
(request/execute wiring); the final +0x30 commit site is out of scope.**

Fields of the queue (module `this`): `+0x1072c` request-type, `+0x10818` pending-flag,
`+0x1081c` event-result index. **VERIFIED by byte-decode.**

---

## 3. State-mutation lifecycle — Spotted vs Activated (INFERRED)

Mapping the two-stage queue onto the name-table Spotted/Activated pairs
(`EGAMESTATE_COMPLETE.md §1`):

| phase | engine step | who/when | sets |
|---|---|---|---|
| base | — | kickoff/in-play | `InPlay 0x00` |
| **Spotted** | **resolve `FUN_0077cd40`→`FUN_0077c710`** detects an event and **queues** a request (`FUN_009efa20`/`efa10`) | per tick, when an event fires | the `*Spotted` state (0x02–0x06, 0x08, 0x0f, 0x1a–0x1e) |
| **Activated** | **finalize `FUN_0077cf20`→`FUN_009efa40`** **consumes/executes** the queued request | the tick the restart is taken | the paired `*Activated` state (0x01,0x10–0x14,0x16–0x19,0x1d) |
| back to play | execution consumer | restart complete | `InPlay 0x00` (or `ApresGoal 0x0d`→`KickoffWaiting 0x0e` after a goal) |
| boundary | **clock `FUN_0077e850`** freeze: `(clk+0x4c>=3600.0) \|\| (aux+0x4b4>=500000)` | no-event ticks | `HalfOver 0x0b` / `GameOver 0x0c` |

**Who sets Spotted vs Activated:** Spotted is the *queue-time* value written in the **resolve**
path; Activated is the *execute-time* value written in the **finalize** path. Both commits
land in the out-of-scope execution consumer, so the exact `+0x30` edge is **INFERRED**, not
byte-confirmed. The Spotted→Activated→InPlay two-phase pattern matches the queue's
request→execute→clear lifecycle 1:1. **INFERRED.**

Clock boundary trigger is **VERIFIED** (`DAT_00c84c7c = 3600.0f`, lazy-init; AND of two
continue-conditions ⇒ OR freeze); HalfOver-vs-GameOver selector (half counter) is
**UNKNOWN** in scope.

---

## 4. Effect targets — what `function`/`callrule` actually do

### 4.1 The rule/effect system is SEPARATE from the match engine (VERIFIED)
- `formula.log` (829-func closure) → match engine: **3** incidental calls only
  (`FUN_0077c400` ×2, `FUN_009effe0` ×1 — shared utility/container helpers, **not** the
  `+0x30` path, **not** 009ef queue members). **0** refs to matchCtx layout
  (`0x110`/`0x1081c`/`0x1072c`). **0** refs to predicate engine (`FUN_009e9570`) or axiom
  registry (`FUN_008a7770`). **VERIFIED** → no static bridge from rule effects into match
  state.

### 4.2 What the recovered closure actually writes (VERIFIED)
The **only** data writes in the formula/rule closure are the compute path's float results:
| writer | target | op | source |
|---|---|---|---|
| `FUN_00818080` (binop) | `term+0x30` | `*(float*)(p+0x30) = add/sub/mul/div/unary` | `formula.log:145-157` |
| `FUN_00817df0` (agg) | `node+0x0c`, `output+0x14` | SUM/MEAN/PRODUCT fold | §2 `FORMULA_CONDITION_EFFECT_ENGINE.md` |
| compute output | `output+0x14` | `*(p+0x14) = *(p+0xc)` | `formula.log:214/231/235` |

**No** write to match-stat slab `+0xe` (Performance) / `+0x10` (Condition), player attribute
records, tactics, or match state appears anywhere in the closure. **VERIFIED by absence.**

### 4.3 Effect dispatch (APPROXIMATE / UNKNOWN)
- The effect-application walk passes through **15 `halt_baddata`** anti-disassembly stubs;
  the boolean fold and effect loop are **not cleanly decompiled**. **VERIFIED (obfuscated).**
- Dispatch is via the **formula-node vtable** (indirect calls at node vtable slots
  `+0x40`, `+0x44`, `+0x11c`, `+0x130`; `formula.log:51085…52854`) and the **name registry**
  `FUN_00662cf0` (builds `DAT_00c8xxxx` named-rule globals; `formula.log:292…850+`).
- `callrule name + data+` = invoke another named rule via the registry (rule composition).
  Bind mechanics **UNKNOWN**.
- `function name + variable? + const+` = call a **named native function** with const args —
  the hook that would reach outside the arithmetic tree. **The named functions and what they
  mutate are NOT in the closure → UNKNOWN.**
- Observed registry keys are **management/AI weighting** (`ProspectiveManagersBaseWeight*`
  {Ambition, Loyalty, CurrentAbility, PotentialAbility, Reputation, …},
  `ManagerSacking*`). → the subsystem most plausibly **selects/weights which formula
  applies to a decision**, i.e. produces **AI weights/values**, not real-time match-state or
  player-attribute mutations. **INFERRED from key strings.**

### 4.4 Effect-target map

| effect | mechanism | concrete reader/writer | what it mutates | status |
|---|---|---|---|---|
| compute (`term`/`result`/`value`) | node-tree eval | W: `term+0x30`, `output+0x14` (float) | formula scratch/output | **VERIFIED** |
| `function` | registry name→native fn + const args | dispatch via node vtable; **target fn out of closure** | UNKNOWN (likely AI weight value) | **UNKNOWN** |
| `callrule` | registry name→rule + `data+` | `FUN_00662cf0` registry; recursive node walk | another rule's output (composition) | **INFERRED struct / UNKNOWN bind** |
| → match state / attributes / tactics | — | **none in closure** | **none found** | **UNKNOWN (no static bridge)** |

A runtime bridge from a `function`-effect into match state cannot be *positively excluded*
(it would be via an un-recovered native target), but **no static path exists** in the
recovered evidence.

---

## 5. Coverage matrix

| target | Identified | Data-flow mapped | Algorithm recovered | Implemented | Validated |
|---|---|---|---|---|---|
| **State writer (+0x30)** | Bounded (subObj method, out of scope); 009ef **excluded** | Reads fully mapped; write site not in scope | No | No | No |
| **Indirect dispatch** | Yes (2-stage event queue, 009ef API) | Yes (request `cd40`→`9f9400/9f7d90`; execute `cf20`→`9f2410`; readback `efb10`) | Request/execute wiring recovered; final commit out of scope | Partial (queue shape) | No |
| **Effect targets** | Compute path yes; `function`/`callrule` game-data targets no | Compute writes mapped; effect-mutation targets not in closure | Compute recovered; effect loop obfuscated | No | No |
| **Mutation lifecycle** | Yes (Spotted=queue / Activated=execute) | Mapped onto queue stages | INFERRED edges; clock boundary VERIFIED | No | No |

### Still needs runtime
1. **Write-watchpoint on `*(*(matchCtx+0xe4)+0x30)`** → names the actual SetGameState method
   and its call site (the only way to close the writer UNKNOWN).
2. **Trace the event-execution consumer** reached from `FUN_0077cf20→FUN_009efa40(9f2410)`
   / `efa70(9f9760)` that turns the `+0x1081c` queued index into the visible `+0x30` commit.
3. **Breakpoint/log a `function` effect** to capture the named-native-function target and
   confirm whether any effect ever mutates attributes/performance/tactics/match-state (vs
   AI weights only).
4. **HalfOver-vs-GameOver selector** (half-counter field) — not in scope.

---

### Byte-evidence appendix (all from raw `cm2008.exe`, VA = file_offset + 0x400000)
- `0x9f9400+0x6e`: `89 be 18 08 01 00` = `mov [esi+0x10818],edi`; `89 86 1c 08 01 00` =
  `mov [esi+0x1081c],eax` — event-request store (NOT state).
- `0x9f7d90+0x3a`: `89 86 2c 07 01 00` = `mov [esi+0x1072c],eax` — request-type store.
- `0x9f2410+0x20`: `83 f8 02 7c .. 83 f8 03 7f` = `cmp eax,2 / cmp eax,3`; `+0x32`:
  `c7 86 18 08 01 00 00 00 00 00` = `mov dword [esi+0x10818],0` — consume/clear.
- `0x77ebc0`: `8b 91 e4 00 00 00 ; 8b 72 30` = eGameState **READ** (`[ctx+0xe4]`→`[edx+0x30]`).
- `0x77d429`: `c7 47 30 f4 28 ba 00` = `mov [edi+0x30], 0x00ba28f4` — **pointer** install on
  another object (not the 0..0x1f state int).
- Global scan: `e4-load → [sameReg+0x30]-store` pairs = **0**.
