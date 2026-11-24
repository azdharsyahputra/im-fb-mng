# CM2008 — Residuals Resolution + Corrected Full-Range LCG Map

Clean-room, **static analysis only**. The game was never run; no DRM/anti-debug was
touched; the ISO was not modified. Every byte below was read from the crack image
`/Volumes/CM2008/Crack JPN/cm2008.exe` (dumped-as-mapped, `file_off = VA − 0x400000`,
filesize `0x2254cc0`) and disassembled with capstone 5.0.7 (x86-32). Function names
are from `docs/ghidra_dumps/functions.csv` (18,957 fns). Scanner scripts live in the
session scratchpad (`.../scratchpad/re/`), not committed.

Confidence tags: **VERIFIED** (read from clean disasm) · **INFERRED** ·
**STRUCTURALLY_UNDERSTOOD** · **LOCATED_NOT_UNDERSTOOD** · **VERIFIED-NEGATIVE** ·
**UNKNOWN**.

---

## TL;DR

1. **Geometry position writer (Residual 1): VERIFIED-NEGATIVE — the per-entity
   geometry block is SEED-ONCE. There is no per-frame position integrator and no
   per-tick set-to-target view-copy.** The float position/vector fields
   (block `+0x94..0xB8`) are written by exactly one field-level float writer — the
   known seeder `FUN_009f0090` (1 direct call site, on an init/reset lifecycle) —
   plus one whole-block `rep movsd` **reset-to-default** path (`FUN_009f4bd0`, source
   is the default-block constructor `FUN_004ec590`). No function RMW-integrates the
   float position vector via a heap pointer anywhere in `.text`; the detailed match
   engine (`0x77xxxx`) writes **zero** floats to that range.

2. **Per-event draw→distribution (Residual 2): VERIFIED chain pinned.**
   `FUN_0077ee80` *GCMatchModule::ProcessData* → `FUN_0077cd40` (event dispatcher,
   draws the global match RNG, dispatches by event-record flags) → `FUN_0077c710`
   (two-player **contested-duel** resolver). Distribution `p = A/(A+B)` is computed
   from two match-entities' 7-bit packed attribute; the LCG uniform is drawn and
   `fcomp`-compared to `p`; the outcome increments a saturating 5-bit tally. All
   addresses below. (A fully self-contained second pin, `FUN_0077be30`, uses the
   integer idiom `(state*N)>>32`: a 10 % trigger then a 50/50 split.)

3. **`FUN_00662cf0` (Residual 3): VERIFIED — Ghidra false alarm.** It is
   *GCFormulaModule::Find*, a deterministic named-field **binary-search** lookup
   wrapped in a hand-rolled `cpuid`/`rdtsc` profiler scope. The "obfuscated body"
   is Ghidra choking on the `cpuid`/`rdtsc` intrinsics (anomaly method #1). No RNG.

4. **Corrected full-range LCG scan:** **508** confirmed `imul …,0x19660d` sites
   (prior capped scan reported 283). The additive constant `0x3c6ef35f` also appears
   exactly **508** times (1:1 cross-validation). Region breakdown below. The formula
   engine (`0x817–0x81a`) and the quick-match resolver (`0x822xxx`) contain **0**
   inline draws (corroborating "formula evaluator = deterministic").

---

## Residual 1 — Geometry position writer / integrator

**Verdict: VERIFIED-NEGATIVE. Seed-once. No per-frame position writer.**

### Method
Three successive scans over all mapped functions (`0 < size < 0x8000`):

- **Pitfall A corrected:** capstone's `op.access` does **not** flag x87 FPU stores
  (`fst`/`fstp`) as writes — the all-`fst` seeder first showed as writing nothing.
  Role detection was redone by **mnemonic** (`fst/fstp/fist/fistp`,`movss`-to-mem =
  store; `fld/fadd[mem]/…` = load). VERIFIED.
- **Pitfall B corrected:** the disp range `0x94..0xd4` collides with unrelated
  structs. Final scan restricted to **float stores** via a **heap base register**
  (excludes `[esp±]`/`[ebp±]` stack locals). This is what isolates the real block.

### Findings (VERIFIED)
- **Float stores to position range `+0x94..0xB8` via a heap pointer, in the entire
  binary: only 6 functions.** In match/sim/seed regions: **one** — the seeder
  `FUN_009f0090` (writes all 10 slots `0x94,0x98,0x9c,0xa0,0xa4,0xa8,0xac,0xb0,0xb4,0xb8`;
  **no RMW**). The other 5 (`FUN_00494b20`, `FUN_00493e40`, `FUN_007f9060/90c0/9d80`)
  write only 1–2 slots on unrelated structs.
- **The detailed match engine `0x77xxxx` writes zero floats to `+0x94..0xB8`.**
- **No float RMW integrator** (`pos += vel`) exists for the vector via a heap base
  anywhere in `.text`.
- **Seeder lifecycle:** `FUN_009f0090` has **exactly one** direct caller,
  `FUN_009f3b00` @ `0x9f3b46`. `FUN_009f3b00` is a per-entity **setup/reset**
  (`mov byte [edi+0x230],1`; teardown path sets `[eax+0x22c]=0x20000` sentinel),
  called from `FUN_009f4a90`. This is an init path, not a physics tick.

### The struct-copy path (anomaly method #3 — the task's explicit check)
The block is **253 dwords (0x3F4 bytes) at `entity+0x5fc`**. Three `rep movsd`
with `ecx = 0xfd` (253) exist, all in the sim region:
- `FUN_009f20b0` @ `0x9f20f4`, `FUN_009f4420` @ `0x9f446f` — copy `[esi+0x5fc] →
  stack` (block **readers**; 0 direct callers → reached indirectly/by-value).
- `FUN_009f4bd0` @ `0x9f4c0c` — copies a **source → `[edi+0x5fc]`** (block
  **writer**). The source (`call 0x4ec590; mov esi,eax`) is **not** a per-frame
  target: `FUN_004ec590` is a **constructor** — it zeroes `+0xc,+0x1c,…,+0x7c`
  (stride 0x10) and stamps 11 records of stride `0x4c` to constant `0x16`, returning
  the freshly-built **default** block. So `FUN_009f4bd0` = **reset block to default**
  (formation-setup path `FUN_009f7d90 → FUN_009f9680`), not a per-frame set-to-target.

### Why the earlier noisy hits were false
- `FUN_0077cd40`'s `+0x98` is an **integer object pointer** (`mov [edi+0x98],eax`,
  `eax` = call return), a different (match-state) struct — not a float position.
- `FUN_009f81f0`'s `+0x94..0xB8` "RMW" were **`[esp+…]` stack accumulators** (it
  `and esp,0xffffffc0; sub esp,0x1b4` then sums entity data into a stack frame; it is
  a **consumer/aggregator** that *reads* entity positions, gated by the same `0x20000`
  sentinel). Excluded once stack bases were filtered.

### Conclusion
The geometry block is **initialized once** (default-reset `FUN_009f4bd0` + field
seeder `FUN_009f0090`) and thereafter **not** moved: no physics integrator (RMW),
no per-tick float set-to-target view-copy. Consistent with a **statistical match
engine that produces events, not continuous motion**.

**Residual scan-scope gap (named precisely):** the search is exhaustive for
*direct* (`[reg+disp]`, `lea[reg+disp]`, `rep movsd`) float/bulk writes over
`VA 0x401000–0xa50000` through mapped functions. It cannot statically exclude a
writer reached *only* through an indirect/vtable call that computes the block base
without a literal `+0x94..0xB8` displacement; however the only block-mover thunks
found (`0x9efaXX → 0x9f7d90/0x9f9400/0x9f9680`) route to the same init/reset
subsystem, not to a per-tick integrator.

---

## Residual 2 — Per-event draw → distribution mapping

**Verdict: VERIFIED. One event's draw-meets-probability chain pinned end to end.**

### Call chain (VERIFIED)
```
FUN_0077ee80  GCMatchModule::ProcessData   (profiler label @ rodata 0xba2e88;
                                             also "End of fixture %d that ran on a thread.")
   └─ @0x77f22e → FUN_0077cd40             event dispatcher
        ├─ inline global-RNG draw @0x77cd84 (coin cache at 0xc84958)
        ├─ dispatch by event-record flags [ebx+4], [ebx+5], [ebx+6]
        │     → sim appliers 0x9efa10 / 0x9efa20 (type-coded thunks into 0x9fxxxx)
        └─ @0x77cdf8 → FUN_0077c710        two-player contested-duel resolver
   (also FUN_0077ccb0 @0x77cce8 → FUN_0077c710)
```

### The draw-meets-probability site — `FUN_0077c710` (VERIFIED)
Global match RNG object fetched via `mov ecx,[0xc8d5d4]; call 0x7e7c10` (state at
`[esi]`). Two contesting match-entities `ebx`, `ebp` (from match-player accessors
`0x5a6170`/`0x593cc0`).

- **(a) Distribution — `0x77c887–0x77c8e8`:** extract two 7-bit packed attributes
  `A = [ebx+0x3c] >> 0xc & 0x7f`, `B = [ebp+0x3c] >> 0xc & 0x7f`; compute the
  normalized probability `p = A/(A+B)` (`fild`, `fdiv`; `0xb3e9a8 = 4294967296.0`
  unsigned-correction; midpoint constants `0xbbb51c = 0xc31b38 = 0.5`). This is the
  distribution/formula result.
- **(b) Draw — `0x77c8f0` (and `0x77c93d`, `0x77c96d`, `0x77c9a0`):**
  `imul eax,eax,0x19660d; add eax,0x3c6ef35f; mov [esi],eax` then the float-uniform
  idiom `eax>>9; or eax,0x3f800000` → `u∈[1,2)`, `fld1; fsub` → `u∈[0,1)`.
- **(c) Compare → outcome — `0x77c925` / `0x77c960` / `0x77c98a`:**
  `fcomp; fnstsw ax; test ah,5; jp/jnp` compares `u` against `p`; on success calls
  `0x5cd020(0)` or `0x5cd020(1)` in a rejection/trial loop.
- **Outcome recorder `FUN_005cd020`:** a **saturating 5-bit counter increment** on a
  packed stat dword — arg 0 bumps bits 21–25 (mask `0x3e00000`), arg 1 bumps bits
  26–30 (mask `0x7c000000`). Two outcome tallies.

### Second, self-contained pin — `FUN_0077be30` (VERIFIED)
Integer-range uniform idiom `bucket = (uint64)state * N >> 32`:
- `0x77be45` draw → `mul 0xa` → `edx = bucket∈[0,10)`; `cmp edx,0; jne` end
  (**10 % trigger**).
- `0x77be73` draw → `mul 2` → `edx∈{0,1}` (**50/50**) → `call 0x77bb30` (outcome 0)
  or `call 0x77b570` (outcome 1).

### Event naming
The mechanism is **STRUCTURALLY_UNDERSTOOD**: a two-player contested duel with an
attribute-derived probability and a two-way saturating outcome tally — i.e. a
tackle/50-50/aerial-class match event resolved inside the detailed engine. The
**exact UI label** (tackle vs header vs shot) is **LOCATED_NOT_UNDERSTOOD**: the
`+0x3c` bit-slot read here is on a *match-engine entity*, not the DB player record
(the DB uses that same bit-slot for a 7-bit "Reputation", per
`BITPACKED_FIELD_MAP.md`), and the duel→label binding lives in the event/axiom
registry enum (`FUN_0093c7f0 → 008a7770`), not followed here. The draw-meets-
probability requirement is fully met regardless.

Note: the global match-RNG pointer `0xc8d5d4` is referenced **449×** across `.text`
(the `mov ecx,[0xc8d5d4]; call 0x7e7c10` access pattern) — a single pervasive
global RNG, consistent with "match = statistical engine".

---

## Residual 3 — `FUN_00662cf0` "obfuscated body"

**Verdict: VERIFIED — Ghidra false alarm (anomaly method #1). It is
`GCFormulaModule::Find`, a deterministic named-field lookup. No RNG.**

### Evidence
- The function is a plain x86 C++ method; the only thing that confuses Ghidra is a
  hand-rolled **profiler scope** built from `cpuid`/`rdtsc` intrinsics (the
  decompiler can't model them, truncates the body, and reports garbage). The crack
  bytes disassemble cleanly to a `ret 8`/`ret 8` with `cc` padding at `0x662f2c`.
- **Profiler prologue (`0x662cf0–0x662d3e`):** run-once init guard `[0xc77698]`
  (registers a profiling record `0xc77670`; atexit via `0xa3dc19`; ctor `0x403150`
  with the label string **`"GCFormulaModule::Find"` @ `0xb87e6c`**); increments the
  active-call counter `[0xc7767c]`; captures an entry timestamp with
  `xor eax,eax; cpuid; rdtsc` → `esi:edi`.
- **Payload (`0x662d3e–0x662dd7`):** reads two string-like args `[esp+0x40]` and
  `[esp+0x44]` (category, name), builds a combined key via `call 0x93b1c0`, then
  `call FUN_00662970` over the container at `edi+0x3c / edi+0x40`, returning the
  found object at `[eax+4]`. `FUN_00662970` is a **binary search** over 16-byte
  sorted `{key,…}` records (`sar …,4` record count, `sar …,1` midpoint,
  `cmp/ja/jb`) — deterministic.
- **Profiler epilogue (two copies, one per return path, `0x662e3c`/`0x662ec3`):**
  `cpuid; rdtsc` exit timestamp; elapsed = exit − start; decrement `[0xc7767c]`;
  when the outermost call returns, accumulate 64-bit total cycles
  `[0xc77688:0x8c]`, track max `[0xc77690:0x94]`, bump 64-bit call-count
  `[0xc77680:0x84]`. Returns the found object (`ebp`→`eax`) or 0.

The profiler stat block `0xc77670–0xc77698` and the `cpuid/rdtsc` idiom are the
entire "obfuscation". Behavior recovered in full; no anti-debug, no self-modifying
code, no RNG.

---

## Corrected full-range LCG scan

**Scope corrected:** prior scan was capped at `file_off 0x311000` (VA `0x711000`),
so the match engine, formula engine and sim were never scanned. This scan covers
**VA `0x401000 – 0xa50000`** (full code range).

**Method:** byte-search the 4-byte LE immediate `0d 66 19 00` (= `0x19660d`); each
hit verified by disassembling across alignments and confirming an instruction whose
immediate operand equals `0x19660d`. Cross-checked against the additive constant.

### Totals (VERIFIED)
- **508** confirmed `imul r32, r32, 0x19660d` sites (0 rejected; mnemonic histogram
  = `{imul: 508}`).
- Additive constant `0x3c6ef35f` immediate appears **508** times → **1:1** with the
  multiplies (strong integrity check: every LCG step is `state = state*0x19660d +
  0x3c6ef35f`).
- **79** of these sites fall inside a mapped function; **279** sites fall in
  `functions.csv` gaps (unmapped/auto-analysis-missed code — the map is incomplete
  in the tail and the corrupt `0x46xxxx` region; all 279 are still confirmed
  `imul 0x19660d` by operand).
- **222** confirmed sites have the float-uniform constant `0x3f800000` within +0x80
  bytes (the `u = ((state>>9)|0x3f800000) − 1.0` idiom) — prior capped count 168.

### Sites by region (VERIFIED)
| Region | LCG `imul 0x19660d` sites |
|---|---:|
| game logic `0x4–0x6xxxxx` | **282** |
| `0x70–0x76xxxx` (per-domain resolver band) | **144** |
| `0xa0–0xa4xxxx` | 36 |
| `0x78–0x816xxx` | 33 |
| **detailed match engine `0x77xxxx`** | **7** |
| `0x824–0x9dxxxx` | 3 |
| sim/seeder `0x9fxxxx` | 2 |
| sim `0x9exxxx` | 1 |
| **formula engine `0x817–0x81a`** | **0** |
| **quick-match resolver `0x822xxx`** | **0** |
| **TOTAL** | **508** |

### Interpretation (tests the "match = statistical engine" root)
- The **match engine draws**, but the 7 *inline* draws in the thin `0x77xxxx` tick
  layer are only the tip: the resolver band `0x70–0x76xxxx` (144) and game logic
  (282) hold the bulk, and the single global match RNG at `0xc8d5d4` is reached
  449× — so match outcomes are statistical, drawn pervasively through shared
  resolvers rather than concentrated in the dispatcher. **Confirmed.**
- **Formula engine = 0 draws** corroborates the established "deterministic evaluator"
  finding. **Quick-match resolver `0x822xxx` = 0 inline draws** → it relies on the
  shared RNG helper and/or the precomputed `MatchResolverDistributions` tables, not
  inline LCG.

### The 7 match-engine draw sites (VERIFIED)
| draw @ | function |
|---|---|
| `0x77be45`, `0x77be73` | `FUN_0077be30` (10%→50/50 event, Residual 2 pin #2) |
| `0x77c8f0`, `0x77c93d`, `0x77c96d`, `0x77c9a0` | `FUN_0077c710` (contested-duel, Residual 2 pin #1) |
| `0x77cd84` | `FUN_0077cd40` (event dispatcher coin, global RNG + `[0xc84958]` cache) |

---

## Appendix — key addresses

| Thing | VA | Confidence |
|---|---|---|
| Geometry field seeder (seed-once) | `FUN_009f0090` | VERIFIED |
| Seeder's only caller (setup/reset) | `FUN_009f3b00` @call `0x9f3b46` | VERIFIED |
| Whole-block default-reset (rep movsd 0xfd) | `FUN_009f4bd0` @`0x9f4c0c` | VERIFIED |
| Default-block constructor (source of reset) | `FUN_004ec590` | VERIFIED |
| Block readers (rep movsd 0xfd → stack) | `FUN_009f20b0`, `FUN_009f4420` | VERIFIED |
| Geometry block location | `entity+0x5fc`, 253 dwords | VERIFIED |
| Match tick (profiler: GCMatchModule::ProcessData) | `FUN_0077ee80` | VERIFIED |
| Event dispatcher | `FUN_0077cd40` | VERIFIED |
| Contested-duel resolver | `FUN_0077c710` | VERIFIED |
| Duel distribution `p=A/(A+B)` | `0x77c887–0x77c8e8` | VERIFIED |
| Duel LCG draws | `0x77c8f0/0x77c93d/0x77c96d/0x77c9a0` | VERIFIED |
| Duel compare→outcome | `0x77c925/0x77c960/0x77c98a` | VERIFIED |
| Outcome tally (saturating 5-bit) | `FUN_005cd020` | VERIFIED |
| 10%→50/50 event (pin #2) | `FUN_0077be30` | VERIFIED |
| Global match RNG object ptr | `0xc8d5d4` (accessor `0x7e7c10`) | VERIFIED |
| `GCFormulaModule::Find` (Residual 3) | `FUN_00662cf0` | VERIFIED |
| Find payload (binary search) | `FUN_00662970` | VERIFIED |
| Profiler label string | `"GCFormulaModule::Find"` @ `0xb87e6c` | VERIFIED |
| Profiler stat block | `0xc77670–0xc77698` | VERIFIED |
| LCG recurrence | `state = state*0x19660d + 0x3c6ef35f` | VERIFIED |
| Float-uniform idiom | `u = ((state>>9)|0x3f800000) − 1.0` | VERIFIED |
