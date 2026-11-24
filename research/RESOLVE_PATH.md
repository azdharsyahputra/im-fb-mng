# CM2008 Detailed Match Engine — Resolve Path (Priority 2)

Clean-room behavioral recovery from Ghidra decompilation dumps. No binary executed, no
Ghidra run. All decompiled text treated as DATA.

Sources:
- `docs/ghidra_dumps/raw/match_engine.c` (depth-1 closure of the resolve chain)
- `docs/ghidra_dumps/big/CM2008.exe_big.c` (predicate sub-engine bodies)

Verified chain:
`FUN_0077ee80` (GCMatchModule::Update/ProcessData)
→ `FUN_0077cf20` (resolve entry)
→ `FUN_0077c540` (weight-80.0 step)
→ `FUN_009e9570` (predicate rule engine)
plus stochastic sub-events `FUN_0077cd40` and `FUN_0077c710`.

Evidence classification per claim: **VERIFIED** (line quoted), **INFERRED** (derived from
offsets/constants), **APPROXIMATE** (shape clear, exact arithmetic obfuscated),
**UNKNOWN** (body absent / unrecoverable).

Convention note: Ghidra recovers the implicit `this` as `in_ECX` in these thiscall
functions. Across the chain the match-state/resolve-subject object is referenced through
`in_ECX`; its identity is INFERRED (see Open Questions re: the +0x14 vs +0x18 base offset).

---

## 1. FUN_0077ee80 — GCMatchModule::Update / ProcessData  (match_engine.c:596)

Top-level per-tick driver. `in_ECX` = GCMatchModule (`this`); `param_1` = pointer to the
per-tick event/control struct. Debug name confirms identity:
- VERIFIED name string: `pcStack_44 = "GCMatchModule::ProcessData";` (line 638).

### 1.1 INPUTS
- `*param_1` (line 649) — mode selector: `0` = end-of-fixture batch finalize path;
  non-zero = per-tick resolve path. VERIFIED.
- `param_1[1]` / `(char)param_1[1]` (lines 816, 845, 850) — a "resolved/terminal" flag of
  the tick struct. VERIFIED read.
- `*(char*)((int)param_1 + 5)` (lines 804, 859) — "is-highlight / needs-detail" flag.
  VERIFIED read.
- `in_ECX + 0x14` — GCMatchModule flags word (see pending bit below). VERIFIED.
- Globals `DAT_00c84950/54/58/48/4c` — match-run mode/config gates (detail vs quick). VERIFIED reads.
- rdtsc/cpuid framing (lines 645-648, 884-906) — profiling counters only
  (`_DAT_00c84c94`, `_DAT_00c84ca0..`); not match logic. VERIFIED.

### 1.2 PREDICATES controlling resolution
- `if (*param_1 == 0)` (649): batch-finalize branch iterates two intrusive red-black trees
  (`in_ECX+0x58..0x60` then `in_ECX+0x70..0x78`) and calls `FUN_0077cf20` per fixture
  (line 698). VERIFIED.
- Per-tick branch (`else`, 786):
  - `cVar6 = FUN_0077ccb0();` (788) — fast-path gate; if non-zero, early return after
    profiling. VERIFIED.
  - `if ((*(uint*)(in_ECX+0x14) & 0x1000000) == 0)` (800) → `cVar6 = FUN_0077cd40();`
    (802). The pending bit being **clear** triggers a fresh stochastic sub-event roll.
    VERIFIED.
  - Final resolve dispatch condition (845): `((FUN_0048c4a0 != 0) || bVar19 || (cVar6 != 0)
    || (param_1[1] != 0))` → calls `FUN_0077cf20()` (855). VERIFIED.

### 1.3 STATE MODIFIED (writes)
- `*(uint*)(in_ECX+0x14) |= 0x80000` (731, 858) — set "resolution-pending/in-progress"
  (bit 19). VERIFIED.
- `*(uint*)(in_ECX+0x14) &= 0xfff7ffff` (783) — clear bit 19. VERIFIED.
- `*(uint*)(in_ECX+0x14) &= 0xfeffffff` (832) — clear pending bit 0x1000000 (bit 24) on the
  "match finished / all consumed" path. VERIFIED.
- Batch path resets tree heads: `in_ECX+0x60=iVar13; in_ECX+0x5c=0; in_ECX+0x64=iVar13;
  in_ECX+0x68=0` (680-683) and the +0x74..+0x80 set (725-728). VERIFIED (list teardown).

### 1.4 EVENTS / RNG / SUB-SUBSYSTEMS
- No direct event emit here; delegates. Sub-event generation = `FUN_0077cd40` (802),
  resolution = `FUN_0077cf20` (698, 855), injuries/subs post-processing via `FUN_0077c660`
  + `FUN_0061cfb0` (865-878). VERIFIED calls.
- No LCG in this body.

---

## 2. FUN_0077cf20 — resolve entry  (match_engine.c:2511)

Signature `(undefined4 param_1, undefined4 param_2, char param_3)`; `in_ECX` = resolve
subject. This is where a pending event is **consumed and committed**, then the pending bit
is cleared.

### 2.1 INPUTS
- `param_2` stored to `*(DAT_00c9b114 + 0x40)` (2524) — publishes the fixture/context
  pointer to a global match singleton (likely for the UI/reporting side). VERIFIED.
- `param_1` → `FUN_00a2eb00(param_1)` (2528), gated by `DAT_00c31b34` network/seat id
  match (2523-2527): network-sync of the fixture id. VERIFIED (semantics INFERRED:
  DAT_00c31b34 == local-seat id).
- `param_3` (2532): if non-zero, **skip** the whole resolution body and jump straight to
  the pending-bit clear (`LAB_0077d084`). So `param_3` = "already-resolved / passive"
  flag. VERIFIED.

### 2.2 PREDICATES / resolution body (only when param_3 == 0)
- `FUN_005d0460(3,…)` (2533), `FUN_009efa70()` (2534) — enter resolution critical section
  on the match-state object. VERIFIED.
- Two `FUN_00661270()` checks guarded by `FUN_005a6170/FUN_00593cc0` (team selectors) →
  `FUN_005cfc40()` (2546): per-side precondition. VERIFIED (body of 005cfc40 is
  corrupted/obfuscated — UNKNOWN internals).
- `cVar1 = FUN_0069a610(); if (cVar1 != 0) FUN_0077c540();` (2548-2551) — **the weight-80.0
  resolution step runs only when FUN_0069a610 returns true.** VERIFIED. `FUN_0069a610` =
  "is this the detailed/active match needing full resolution" predicate (INFERRED).
- Second `FUN_0069a610` gate (2553) → two `FUN_0077c5e0()` applies (per side, 2559/2565)
  then `FUN_00955980()` (2567). VERIFIED. `FUN_0077c5e0` = commit side sub-result
  (INFERRED).
- `FUN_009efc30()` (2569) — leave resolution section. VERIFIED.

### 2.3 STATE MODIFIED
- `*(uint*)(in_ECX+0x18) &= 0xfef7ffff` (2571) — **clears BOTH pending bit 0x1000000 (bit
  24) and the in-progress bit 0x80000 (bit 19)** in one mask (`~0x01080000`). This is the
  authoritative pending-bit clear at end of a resolved tick. VERIFIED.

### 2.4 EVENTS / RNG / SUB-SUBSYSTEMS
- Emits nothing itself; the committed events were produced by `FUN_0077c540`/`FUN_0077c5e0`
  into the match-state event list, then flushed by `FUN_005cfd00()` (2572) and the
  `FUN_0040adf0(0x414,…)` reporting hook (2577). VERIFIED calls.
- No LCG.

---

## 3. FUN_0077c540 — weight-80.0 step  (match_engine.c:5739)

The decisive "run the predicate rule engine with weight 80.0" step. Signature `(void)`;
operates on the match-state object returned by `FUN_009efa90`.

### 3.1 INPUTS
- `iVar3 = FUN_009efa90();` (5748) — match-state accessor.
- `piVar1 = *(int**)(iVar3 + 0x180);` (5749) — pointer to the **candidate/event
  rule-list** head (an intrusive list; `*piVar1 == piVar1` means empty). VERIFIED.

### 3.2 PREDICATE / CALL into the rule engine
- `if ((int*)*piVar1 != piVar1)` (5750) — proceed only if the rule-list is non-empty.
  VERIFIED.
- `pvVar5 = (void*)FUN_009e9570(1, uVar4, piVar1, 0x42a00000);` (5758). **`0x42a00000` is
  the IEEE-754 float `80.0`** — the weight/threshold passed as the 4th argument. VERIFIED
  (constant decode). `uVar4 = FUN_005a6330()` (5752) = match-state/team context handle.
  `param_1 = 1` = mode flag. VERIFIED.
- `pvVar5` is a 12-byte result object (`operator_new(0xc)`, 5753) populated by the engine
  (see §4.1). VERIFIED.

### 3.3 STATE MODIFIED / EVENTS
- `cVar2 = FUN_005cdcb0(0); if (cVar2) FUN_009e43b0(0,uVar4,pvVar5);` (5760-5763) and the
  same for side `1` (5764-5767). `FUN_009e43b0(side, ctx, result)` = **apply/emit the
  resolved event to that team's match state** (the actual mutation/event emission).
  `FUN_005cdcb0(side)` gates whether that side is affected. VERIFIED calls; FUN_009e43b0
  internals APPROXIMATE (body at big:42660, not fully traced here).
- Cleanup: `FUN_009e8be0(); _free(pvVar5);` (5769-5770) — the 0xc result is transient.
  VERIFIED.

### 3.4 RNG
- None in this body; randomness is upstream in `FUN_0077c710`/`FUN_0077cd40` and
  (UNKNOWN) possibly inside `FUN_009e9570`/`FUN_009e5160`.

---

## 4. FUN_009e9570 — predicate / axiom rule engine  (big/CM2008.exe_big.c:75756)

Signature recovered as `(undefined4 param_1, undefined4 param_2, undefined4 *param_3)`; the
4th stack arg (`80.0` from §3) is **NOT** in the recovered signature. `in_ECX` = output
object pointer (the 0xc buffer from `FUN_0077c540`).

### 4.1 INPUTS / OUTPUT init
- `*in_ECX = 0; in_ECX[1] = 0; in_ECX[2] = 0;` (75817-75819) — zero-initialise the 12-byte
  result object. VERIFIED (matches `operator_new(0xc)` in §3).
- `param_3` = candidate rule-list (`piVar1` / match-state+0x180); copied to
  `local_d4/e0/e8`, `local_ec = *param_3` (75826-75834). VERIFIED.

### 4.2 BEHAVIOR (structure VERIFIED; arithmetic APPROXIMATE/UNKNOWN)
- Two `FUN_009e58d0()` passes (75832/75835) build working lists/trees from the input.
- Main predicate walk: red-black-tree / intrusive-list traversal
  (`LAB_009e9667` 75837-75853, and the balanced-tree successor walks 75855-75878).
- **Weight evaluation**: per node `fVar11 = (float10)FUN_009e5160();` (75898) and
  `if ((float10)0 < fVar11)` (75899) collects nodes with **positive weight**, storing
  `-fVar11` and an id `iVar1 = *(int*)(puVar7+0x10)` into a candidate node
  (`piVar8[3]=iVar1; piVar8[2]=(int)(-fVar11)`, 75904-75905). This is a weighted candidate
  filter. VERIFIED structurally.
- Mode split on `cStack_171` (75889): `== 0` → scoring/collection path that returns the
  filtered weighted set; `!= 0` → allocates 0x50-byte objects via
  `FUN_009e92b0()` (75957-75962) and pushes them into a vector at `iStack_170+4`
  (75964-75972) — i.e. materialise predicate-result objects. VERIFIED structurally.
- Stack-cookie protected (`__security_check_cookie`, 75949). Cleanup frees temp lists
  (75936-75948). VERIFIED.

### 4.3 RETURN (how it feeds FUN_0077c540)
- Populates the 0xc `in_ECX` object (chosen id + weight) that §3 then hands to
  `FUN_009e43b0`. Exact field layout of the 0xc object is APPROXIMATE (3 dwords: likely
  {selected-id, weight/score, flags}).

### 4.4 UNKNOWN
- `FUN_009e5160` (per-node weight/formula evaluator) body is **absent** from both dumps —
  only the call site exists. The exact weighting arithmetic and how/whether the `80.0`
  threshold is applied inside are **UNKNOWN** (do not invent).

---

## 5. FUN_0077cd40 — stochastic sub-event roll  (match_engine.c:2613)

Called from `FUN_0077ee80:802` when the pending bit is clear. Rolls whether a new
sub-event occurs and **sets the pending bit**. Returns `uVar7` (event-occurred flag).
`in_ECX` = GCMatchModule/match object.

### 5.1 INPUTS
- `param_1` = tick struct (`uVar4 = *param_1`, 2627). `*(char*)((int)param_1+6)` (2637),
  `*(char*)(param_1+1)` (2642/2655), `*(char*)((int)param_1+5)` (2638/2665/2681/2690) —
  highlight/terminal/side flags. VERIFIED reads.
- Config gates `DAT_00c84950/54/58`. VERIFIED.

### 5.2 RNG (VERIFIED — the LCG)
- `uVar6 = *puVar3 * 0x19660d + 0x3c6ef35f; *puVar3 = uVar6; DAT_00c84958 = uVar6 >> 8;`
  (2633-2635). **`0x19660d = 1664525`, `0x3c6ef35f = 1013904223`** — the classic LCG
  `state*1664525 + 1013904223`. `puVar3 = FUN_007e7c10()` = global RNG state. Seeds
  `DAT_00c84958` (a per-event random seed, top 24 bits). VERIFIED.

### 5.3 STATE MODIFIED / EVENTS
- `*(undefined4*)(in_ECX+0x98) = uVar2;` (2630) where `uVar2 = FUN_006a00f0(FUN_005a6320())`
  — store current resolved match-time/handle at +0x98. VERIFIED.
- `*(undefined4*)(in_ECX+0x8c) = 0;` (2640) — reset a counter on the "scheduled-event"
  branch. VERIFIED.
- **`*(uint*)(in_ECX+0x18) |= 0x1000000;` (2663) — SETS the event-pending bit (bit 24).**
  VERIFIED. This is the producer of the pending bit.
- Dispatch of the roll: either `FUN_009efa20(1/0, DAT_00c84958, uVar4, 0)` (2639/2643) —
  schedule event using the random seed — or `FUN_0077c710(uVar4)` (2646) for the
  highlight/detailed path, followed by `*(uint*)(iVar5+4) = (…&0xffff3fff)|0x3000` (2648)
  setting a 2-bit state field (0x3000) on the camera/highlight object `FUN_005a6330`.
  VERIFIED.
- Final: if pending bit still clear (2694) calls the reporting hook
  `(**(code**)(iVar5+4))(0x43a,…)` (2697). VERIFIED.

---

## 6. FUN_0077c710 — stochastic highlight/sub-event applier  (match_engine.c:5920)

The per-player highlight resolver (shots/dribbles); heavy RNG. Signature `(void)`.

### 6.1 INPUTS
- `uVar4 = FUN_005a6330()` (5954) highlight ctx; `puVar5 = FUN_007e7c10()` (5955) = RNG
  state; team selectors `FUN_005a6170(0/1)`→`FUN_00593cc0()` (5957-5962). VERIFIED.
- Mode gate `DAT_00c8494c` (5963: `==0`, 6045: `!=2`, else branch 6071). VERIFIED.

### 6.2 RNG (VERIFIED — same LCG, drives event counts)
- `uVar8 = *puVar5 * 0x19660d + 0x3c6ef35f; *puVar5 = uVar8;` (6025-6026, 6030-6031,
  6034-6035, 6040-6041) — same LCG constants. VERIFIED.
- Uniform float draw: `(float)(uVar8 >> 9 | 0x3f800000) - 1.0` → uniform in [0,1)
  (6027, 6036). VERIFIED (standard "mantissa trick" [1.0,2.0)−1.0).
- Weight derivation: per-side attribute byte
  `bVar2 = (byte)(*(uint*)(iVar6+0x3c) >> 0xc) & 0x7f` (6014) and the same at `iVar7+0x3c`
  (6016); normalised into `fVar17/fVar18` shares (6018-6022) then scaled by
  `_DAT_00c31b38` into thresholds `fVar19/fVar16` (6023-6024). **The draw is compared
  against these thresholds in `while` loops that repeatedly call `FUN_005cd020(0)` /
  `FUN_005cd020(1)`** (6028-6033, 6038-6043) — i.e. generate N sub-events for each side
  while `draw < threshold`. VERIFIED. (The `0x3c` field holds a packed attribute;
  bits>>0xc & 0x7f = 7-bit rating — INFERRED meaning.)

### 6.3 STATE MODIFIED / EVENTS
- `*puVar5` (RNG state) advanced. VERIFIED.
- The "many players" branch (`1 < bVar2`, 5990) iterates both sides and calls
  `FUN_005cd020(iVar7)` a count derived from packed attribute bytes
  `puVar5[0xe]>>0x1b + (puVar5[0xe]>>0x16 & 0x1f) + (*(byte*)(puVar5+0x1b) & 0x1f)`
  (5998-6004) — per-player sub-event emission. VERIFIED structurally; arithmetic
  APPROXIMATE.
- `FUN_005cd020(side)` = the sub-event emitter/applier (body not in dumps — INFERRED).
- `DAT_00c8494c == 2` branch → `FUN_00a023f0(uVar4)` (6071). Comparison branch at 6045-6069
  uses `FUN_00476350()` draws and `FUN_00483dc0(5,…)`/`FUN_005ccff0()` to pick between two
  equal-rated candidates. VERIFIED calls; semantics APPROXIMATE.

---

## 7. FUN_0077ccb0 — fast-path resolver  (match_engine.c:3010)

Called first in the per-tick branch (`FUN_0077ee80:788`). When the match is in quick/
non-detailed mode (`DAT_00c8494c == 3` and the config gates, 3017-3019) it resolves the
whole tick immediately:
- `FUN_0077c710(*param_1)` (3020) — run the stochastic applier directly.
- `*(uint*)(in_ECX+0x18) &= 0xfef7ffff` (3021) — **clears pending bit 0x1000000 + 0x80000**
  (same mask as §2.3). VERIFIED.
- Sets highlight state `(iVar1+4 … | 0x3000)` (3023) and fires reporting hook
  `FUN_0040adf0(0x414,…)` (3028); returns 1 → caller early-returns. VERIFIED.

## 8. Match clock (matchCtx+0xe4) and event timestamp — FUN_0077c660 (match_engine.c:2757)

- Match-state accessor `FUN_009efa90()` returns the match-state object; its **+0xe4 is the
  clock sub-object pointer**. Clock fields: `+0x44` (period base seconds), `+0x48`
  (seconds within period), `+0x4c` (compared to threshold `DAT_00c84c7c = 0x45610000 =
  3600.0f`, see FUN_0077e850:3138), `+0x30` (a time/handle dword, 3262). VERIFIED.
- `FUN_0077c660` packs the clock into an event timestamp bitfield: reads `clock+0x48` and
  `clock+0x44`, sums, multiplies by `0.00027777778` (=1/3600) and `0.016666668` (=1/60) to
  derive hours/minutes, packs into `*param_1` combined with `DAT_00c809d4+0x30` flags
  (2765-2771). VERIFIED. This is the timestamp attached when the injury/sub events from
  `FUN_0077ee80:865-878` (via `FUN_0061cfb0`) are pushed.

---

## Offset / Mutation Table

| Object | Offset | Access | Meaning (confidence) | Line |
|---|---|---|---|---|
| GCMatchModule (`in_ECX` of ee80) | +0x14 | R/W bit 0x1000000 | pending-event gate (check & final clear) — VERIFIED site; base-identity INFERRED | 800,832 |
| GCMatchModule | +0x14 | W bit 0x80000 | resolution in-progress (set/clear) VERIFIED | 731,783,858 |
| resolve subject (`in_ECX` of cf20/cd40/ccb0) | +0x18 | W |0x1000000 | **SET pending bit** (cd40) VERIFIED | 2663 |
| resolve subject | +0x18 | W &0xfef7ffff | **CLEAR pending+inprogress** (cf20, ccb0) VERIFIED | 2571,3021 |
| resolve subject | +0x18 | R bit 0x1000000 | re-check pending after roll VERIFIED | 2694 |
| GCMatchModule | +0x98 | W | resolved match-time/handle (cd40) VERIFIED | 2630 |
| GCMatchModule | +0x8c | W=0 | scheduled-event counter reset (cd40) VERIFIED | 2640 |
| GCMatchModule | +0x58..+0x68 | W | fixture rb-tree-A head reset (batch) VERIFIED | 680-683 |
| GCMatchModule | +0x74..+0x80 | W | fixture rb-tree-B head reset (batch) VERIFIED | 725-728 |
| match-state (FUN_009efa90) | +0x180 | R | candidate/event rule-list head VERIFIED | 5749 |
| match-state | +0xe4 → clock | R | clock sub-object ptr VERIFIED | 2765,6090 |
| clock | +0x44,+0x48,+0x4c,+0x30 | R | period-base / in-period secs / threshold cmp / handle VERIFIED | 2765-2767,3138,3262 |
| highlight ctx (FUN_005a6330) | +4 | W (…\|0x3000) | highlight 2-bit state field VERIFIED | 2648,3023 |
| rule-engine out obj (0xc) | +0,+4,+8 | W=0 then filled | {selected-id, weight, flags} (layout APPROX) | 75817-75819 |
| RNG state (FUN_007e7c10) | *(ptr) | R/W | LCG state 1664525/1013904223 VERIFIED | 2633,6025 |
| player attr struct | +0x3c | R | packed rating (>>0xc & 0x7f) for shot weight INFERRED | 6014,6016 |

---

## Pending-bit lifecycle (fixture/resolve-subject +0x18, bit 0x1000000)

1. **Gate (read):** `FUN_0077ee80:800` — per tick, if `in_ECX+0x14 & 0x1000000 == 0`
   (no event currently pending) the engine rolls for a new sub-event via `FUN_0077cd40`.
   (ee80 reads it at `+0x14`; cd40/cf20 write it at `+0x18` — see Open Questions on the
   4-byte base discrepancy; semantically the same flag word.)
2. **Produce (set):** `FUN_0077cd40:2663` — `in_ECX+0x18 |= 0x1000000`. A stochastic event
   has been scheduled/created (seed `DAT_00c84958` from the LCG). The in-progress bit
   `0x80000` is separately set by `FUN_0077ee80` (731/858).
3. **Consume / commit (clear):** two sites, both masking `&0xfef7ffff` (clears bit24 +
   bit19 together):
   - `FUN_0077cf20:2571` — normal detailed resolution: after `FUN_0077c540` (rule engine)
     and `FUN_0077c5e0` side-commits have run.
   - `FUN_0077ccb0:3021` — quick-mode fast path: after `FUN_0077c710` runs directly.
   The module-level mirror also clears at `FUN_0077ee80:832` (`+0x14 &0xfeffffff`) on the
   match-finished path.
4. **Re-check:** `FUN_0077cd40:2694` — if after processing the bit is still clear, fire the
   `0x43a` reporting callback (event fully flushed).

Net: cd40 is the sole **producer**; cf20 and ccb0 are the **consumers/clearers**; ee80
holds the gate and a module-level mirror. The 0x80000 "in-progress" bit always travels with
the pending bit on the clear mask.

---

## Open questions / UNKNOWN

1. **+0x14 vs +0x18 base discrepancy (INFERRED).** `FUN_0077ee80` checks/clears the pending
   bit at `in_ECX+0x14`, while `FUN_0077cd40`/`FUN_0077cf20`/`FUN_0077ccb0` set/clear it at
   `in_ECX+0x18`. Both words also carry the 0x80000 in-progress bit. Most likely the Ghidra
   `in_ECX` recovery differs by one field (4 bytes) between the outer driver and the inner
   resolvers (i.e. they point at adjacent sub-objects of the same GCMatchModule), so it is
   physically one flags word. Not provable from the dumps alone — needs the struct
   definition / a register-accurate trace. Do not assume they are identical without it.
2. **`FUN_009e5160`** (per-node weight/formula evaluator) — body **absent** from both
   dumps. The exact predicate-weighting arithmetic is UNKNOWN.
3. **The `80.0` (0x42a00000) 4th argument to `FUN_009e9570`** — passed but not present in
   the recovered 3-arg signature; where/how it is consumed inside the engine is UNKNOWN
   (likely a weight ceiling or normalisation constant — NOT verified).
4. **0xc result-object field layout** — zeroed then filled; the three dwords are inferred as
   {selected-id, weight/score, flags} but exact meaning is APPROXIMATE.
5. **`FUN_005cd020`** (sub-event emitter called from the RNG loops) and **`FUN_009e43b0`**
   (per-side event apply/emit) bodies not traced here — the actual event push mechanics
   downstream are INFERRED.
6. **`FUN_005cfc40`** (per-side precondition in cf20) decompiles to corrupted/overlapping
   instructions (many "Removing unreachable block" / "Bad instruction" warnings) — internals
   UNKNOWN.
7. **`FUN_0069a610`** gate (controls whether `FUN_0077c540` runs) — returns a char; inferred
   "is-active-detailed-match" but body not inspected.
8. **Formula eval `00818120` / picker `005b5100`** named in the task brief were **not
   reached** on this resolve path in the dumped closure (no call sites found in the chain
   bodies read). Whether they are invoked indirectly (vtable/`FUN_009e5160`) is UNKNOWN.
   (NB: the picker BODY is now recovered — key→index lookup, NO RNG, RESOLVER_PICKER_CLUSTER_RECOVERY.md;
   only its reachability on THIS path is the open item here.)
