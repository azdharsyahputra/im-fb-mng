# Waypoint Provider Recovery — the real match target/waypoint source (Agent D)

Static only. No game run, no DRM bypass. Evidence base:
- `docs/ghidra_dumps/big/CM2008.exe_big.c` (curated 150-fn decompile) and `docs/ghidra_dumps/*/functions.csv`.
- **Fresh single-access re-export of the unpacked DB** `/Users/csadeveloper/tools/cm2008_full`
  (program `CM2008.exe`, SecuROM-unpacked analysis DB — the same DB the committed dumps came from).
  Project was **unlocked, no running Ghidra** at the time; I ran `reexport.sh` + a custom
  `FindFiller.java` scan (read-only, `-noanalysis`). Scan outputs retained in the session scratchpad.
- Cross-check: `research/PHYSICS_AND_WAYPOINT_ANALYSIS.md` (Agent 3), `research/SIM_BLOCK_WRITER.md`,
  `research/GEOMETRY_WRITER_INVESTIGATION.md`, `research/DETAILED_MATCH_ENGINE_CALLGRAPH.md`.

Classification vocabulary: NOT_INVESTIGATED / LOCATED_NOT_UNDERSTOOD / DECOMPILED /
STRUCTURALLY_UNDERSTOOD / BEHAVIORALLY_VALIDATED / GENUINELY_UNAVAILABLE. **Nothing below is invented;
coordinates are NOT fabricated. The headline is a verified negative with a located recovery path.**

All addresses are VAs into `CM2008.exe` (flat map `file_off = VA - 0x400000`).

---

## 0. TL;DR

- **FUN_00a1fdf0 is confirmed a serializer, NOT a waypoint/target-picker** — and its one caller
  **FUN_00724790 is the `dumpplayerstats` debug/stats command handler**, reached only through a
  function-pointer/vtable table entry. The long-standing "`00724790 → 00a1fdf0` = movement/positioning"
  claim in the older docs is **REFUTED**: that edge is a stats dump, it reaches the serializer, and it
  produces **no world coordinates and does not touch the match entity's +0xb4..+0xba refs.**
- **The consume side is fully DECOMPILED & cross-referenced to the match tick** (below).
- **The real FILLER (what writes non-0xffff target refs into +0xb4..+0xba) is NOT in the dumped
  closure.** A fingerprint scan of the whole image for u16 stores to `[reg+0xb4/0xb6/0xb8/0xba]`
  returns **exactly two** writers, and **both only store `0xffff` (empty/reset)** — the per-tick flush
  (`FUN_009f0c40`) and an entity reset (`FUN_009f0fe0`). No static site stores a real ref value. The
  selector therefore lives in the **thread-private match-AI worker** (same un-dumped region as the
  per-frame integrator in `SIM_BLOCK_WRITER.md §2`), or writes through a non-literal pointer path.
  **This is the clean answer, not a failure.**

---

## 1. FUN_00a1fdf0 refutation — CONFIRMED serializer (closing the conflation)

**Status: STRUCTURALLY_UNDERSTOOD. Agent 3's refutation is independently re-confirmed.**

- `big.c` #55 @0x00a1fdf0 (3624 B, 81 callees, **1 caller**). Body (lines 40656-41330) walks a linked
  roster of **0x3c-byte** entries (`next = base + (entry[0] & 0xfff)*0x3c`, validity bit
  `(*(uint*)(entry+0x3a) >> 0x14) & 1`) and, per valid player, **registers the full attribute-NAME
  dictionary** (Morale, Condition, Acceleration, Agility, Anticipation, DecisionMaking, Positioning,
  WorkRate, … all ~60 names) via `FUN_00413550` and serializes each via `FUN_005f2b40`/`FUN_00584650`.
  The movement-sounding attribute names appear here only because the function emits the **entire
  dictionary** — not a selectively-read movement subset. VERIFIED.
- The `0x14`/`0x44`-stride "candidate list" that was mistaken for coordinates belongs to the **next**
  function **FUN_007fa450** (`GCTransferModule` schema registrar). Its "coords" are
  `FUN_00a0d5b0(0x16, <hash>)` return pairs. **I decompiled FUN_00a0d5b0:**
  ```c
  void __thiscall FUN_00a0d5b0(undefined4 *this, undefined4 p2, undefined4 p3)
  { *this = p2; this[1] = p3; return; }   // a 2-field (tag,hash) pair initializer
  ```
  So `FUN_00a0d5b0(0x16, 0x4df0a5fc)` just stores the pair **(typeTag=0x16, fieldHash=0x4df0a5fc)** —
  a **serialization field descriptor**, NOT a pitch coordinate. Conflation closed. VERIFIED.

**Conclusion: neither FUN_00a1fdf0 nor FUN_007fa450 produces world coordinates.** (matches Agent 3.)

---

## 2. FUN_00724790 — what it actually reaches = `dumpplayerstats` (NOT movement)

**Status: DECOMPILED (fresh re-export). The "movement trampoline" label is REFUTED.**

Re-exported body (`__fastcall FUN_00724790(int param_1)`, 382 B, thiscall `this=param_1`):
- Walks a roster list of **0x34-byte** entries off `param_1+0x2c` with field mask `& 0x1fffff` and bit
  `>> 0x1e & 1`, calling `FUN_005fb4f0()` + `FUN_00721c10(entry)` per entry.
- **Builds the std::string literal `s___dumpplayerstats_00b9acac`** (the symbol name literally contains
  **"dumpplayerstats"**; it copies 0x12 bytes of it into a local string via `FUN_00401f80`/
  `FUN_007e0f10`), then **iff a count `local_1c != 0` calls `FUN_00a1fdf0()`** (the serializer), then
  `FUN_00620a20()`.
- **Callers (re-export): exactly ONE reference, and it is a DATA entry at `0x00b9b32c`** (a
  function-pointer / command-dispatch table), with **zero direct CALL sites**. This matches
  `match-pipeline-deep.md`'s observation that the three "wrappers" have 0 static callers and are
  invoked indirectly — but the correct reading is that `00724790` is a **table-registered debug/stats
  command** (`dumpplayerstats`) whose action is to serialize player stats via `00a1fdf0`.

**So FUN_00724790 reaches the SERIALIZER, not any movement/positioning code.** It does not read or
write the entity geometry block or the +0xb4..+0xba target refs. The edge
`00724790 → 00a1fdf0 = "POSITIONING / MOVEMENT"` in `match-pipeline-deep.md`,
`DETAILED_MATCH_ENGINE_FUNCTION_MAP.md` (line 149), `RESOLVER_FUNCTION_MAP.md`, `ARCHITECTURE.md`,
`MATCH_ENGINE_MASTER_STATUS.md` is **wrong on semantics** (the call-graph *edge* is real; its *role*
is a stats dump). Those docs should be corrected.

---

## 3. Consume side — DECOMPILED and tied to the match tick (VERIFIED)

Fresh re-exports (the four functions are absent from `big.c`; sizes from `functions.csv`):

### FUN_009f0c40 — per-tick waypoint FLUSH + reset (182 B) — the consumer
```c
void __fastcall FUN_009f0c40(int this) {          // this = match entity (ESI)
  if (*(short*)(this+0xb4) != -1) FUN_009fd680(*(short*)(this+0xb4), 0);
  if (*(short*)(this+0xb8) != -1) FUN_009fd680(*(short*)(this+0xb8), 0);
  if (*(short*)(this+0xb6) != -1) FUN_009fd680(*(short*)(this+0xb6), 0);
  if (*(short*)(this+0xba) != -1) FUN_009fd680(*(short*)(this+0xba), 0);
  FUN_009fd690(0);
  *(u16*)(this+0xb4)=0xffff; *(u16*)(this+0xb8)=0xffff;   // RESET refs to empty
  *(u16*)(this+0xb6)=0xffff; *(u16*)(this+0xba)=0xffff;
  *(u8*)(this+0xd8)=0; *(u16*)(this+0xcc)=0; *(u16*)(this+0xce)=0;
}
```
- The 4 refs are **signed `short`**, `0xffff (=-1)` = empty. Processed in order b4, b8, b6, ba.
- `FUN_009fd680` = thin forwarder (`mov ecx,[ecx+4]; jmp 0x65fe90`).
- `FUN_0065fe90` = interlocked list push: `LOCK; *(int*)(node+0x7c)=node; UNLOCK; FUN_0065fe39()`.
- `FUN_009fd690(this2, char mode)`: loops **exactly 4×**, `ptr=this2+0x12`, reads `(*ptr, ptr[-1])`
  = the **(u16,u16) pair at +0x12, stride 8** (`ptr += 4` shorts), pushing each via `FUN_0065fe90`
  (mode!=0 uses the pair, mode==0 uses `DAT_00b3f808`). Matches Agent 3 exactly. VERIFIED.

### Call path — flush runs per-tick in the match EXECUTE phase (VERIFIED cross-ref)
```
FUN_0077cf20 (per-tick EXECUTE/finalize, "cf20" in the ccb0→cd40→e850→cf20→c660 tick chain)
   → FUN_009efc30 (forwarder → 0x9f0c40)
      → FUN_009f0c40  [reads+flushes+resets +0xb4..+0xba]
         → FUN_009fd680 ×(≤4) → FUN_0065fe90
         → FUN_009fd690 → FUN_0065fe90
```
`FUN_0077cf20` = the EXECUTE step is VERIFIED in `MATCH_CLOCK_AND_UPDATE_LOOP.md` /
`DETAILED_MATCH_ENGINE_CALLGRAPH.md` (line 151 already records this flush chain). So the waypoint
polyline is **consumed and cleared once per tick during event execution**. VERIFIED.
(`FUN_009fd690` is also reached via `FUN_009efc00` from UI paths `009441f0/00942960/0094a110` — a
second, non-match consumer of the same point-container type.)

---

## 4. CHASING THE FILLER — fingerprint scan result (the headline)

**Method (coordinator's signature/fingerprint trick, same as FindGeom for +0xc0/+0xd0):** scanned the
**entire image** for `MOV ... [reg + 0xb4/0xb6/0xb8/0xba]` and classified by operand size and base.
(`research/ghidra_reexport/find/` style script, run read-only; mine kept in scratchpad to avoid
editing project files.)

**Result — u16 (`word ptr`) stores to the ESI-based quad exist in EXACTLY TWO functions, and both
write `0xffff` (empty):**

| VA | Function | Writes | src | Meaning |
|---|---|---|---|---|
| 0x9f0cbe..0x9f0cd3 | **FUN_009f0c40** | `word [ESI+0xb4/b8/b6/ba]` | DI=0xffff | per-tick flush **reset** (§3) |
| 0x9f115f..0x9f1174 | **FUN_009f0fe0** | `word [ESI+0xb4/b8/b6/ba]` | AX=0xffff | **entity reset/re-init** |

Re-exported `FUN_009f0fe0` to confirm the second one's semantics: it zero-inits a large entity region
(a 0x16×0xb grid of 8-byte records off `*(this+0x1e8)`), then **guarded by `mode==0` sets
`+0xb4/+0xb6/+0xb8/+0xba = 0xffff`, `+0xd8=0`, `+0xcc=0`, `+0xce=0`** — the identical field set
`FUN_009f0c40` clears — and seeds physics floats at `*(this+0x1e8)+0x4a0..0x4ac` from
`*(this+500)+0x48` (+ consts `_DAT_00b5b330`, `_DAT_00b18024`). It is an **entity reset / kickoff
re-initializer** (callers `FUN_009f7d90`, `FUN_009f9400` = entity setup paths), **not** a target filler.

**No other store in the whole image writes the +0xb4..+0xba u16 quad.** The byte-ptr quad writers at
`0x87aba0 / 0x87bb30 / 0x9ce300` write 4 *bytes* at those offsets on a **different object layout**
(flag array), not the u16 ref block — ruled out. The dword writers (`0x868400`, `0x90b160`, `0x973300`,
…) target other objects' +0xb4 dword fields — ruled out.

### Interpretation (VERIFIED negative + located next step)
Because the consumer (§3) demonstrably reads **non-`0xffff`** refs at runtime, a filler must store real
values, yet **no static literal-displacement u16 store does so.** Therefore the real target/waypoint
SELECTOR is one of:
1. **Thread-private match-AI worker (most likely).** The same un-dumped region that holds the per-frame
   position integrator (`SIM_BLOCK_WRITER.md §2`, `GEOMETRY_WRITER_INVESTIGATION.md §3`: match worker
   thread entry unlocated; only `CreateThread` as an import thunk at 0x0156b873). The decision/RESOLVE
   phase that *selects* targets runs there; the main-thread EXECUTE phase (`cf20`) only *flushes* them.
2. **A non-literal pointer write** (e.g. `lea edi,[esi+0xb4]` then `mov [edi],ax; mov [edi+4],ax`, or a
   `rep movs`/struct copy from a scratch buffer) — invisible to a displacement fingerprint scan.

Both are **un-dumped-code / dataflow limits, NOT protection** (image is unpacked, entropy-normal per
`SIM_BLOCK_WRITER.md §0`). **The filler/selector is NOT isolated.** Classification:
**LOCATED_NOT_UNDERSTOOD** (write offsets + consume chain known; producer un-located).

### Recovery path (not run here — needs the worker thread or dataflow)
- **Locate the match worker-thread proc** (analyze the `0x0156xxxx` `CreateThread` thunk refs / the
  thread-pool creation site in the match module, then re-export its loop). This yields BOTH this filler
  AND the per-frame integrator (`SIM_BLOCK_WRITER.md §2` item). Highest value.
- OR **dataflow trace** of `lea`/pointer writes to `this+0xb4` within the sim cluster (0x9b–0x9f):
  scan for `lea r,[esi+0xb4]` (and `+0xb6/b8/ba`) followed by a `mov [r],*` store — catches the
  non-literal path my displacement scan misses.
- OR **runtime capture** (out of static scope): breakpoint-on-write to entity+0xb4 during a match.

---

## 5. Classification summary

| Finding | Status |
|---|---|
| FUN_00a1fdf0 = roster attribute-NAME serializer (not a picker) | STRUCTURALLY_UNDERSTOOD (re-confirmed) |
| FUN_00a0d5b0 = `(tag,hash)` pair initializer; FUN_007fa450 "coords" = field descriptors, not pitch | DECOMPILED / VERIFIED |
| **FUN_00724790 = `dumpplayerstats` debug/stats command → calls serializer 00a1fdf0** | **DECOMPILED** — "movement trampoline" label REFUTED |
| FUN_00724790 reached only via DATA/table entry @0xb9b32c (no direct callers) | VERIFIED |
| Consume chain FUN_009f0c40 → 009fd680/009fd690 → 0065fe90; refs u16 @+0xb4/b6/b8/ba (0xffff=empty); pairs @+0x12 stride 8 | DECOMPILED / VERIFIED |
| Consume runs per-tick: `cf20 (FUN_0077cf20) → 009efc30 → 009f0c40` (EXECUTE phase) | VERIFIED (cross-ref) |
| FUN_009f0fe0 = entity reset/re-init (also writes 0xffff to the quad), NOT a filler | DECOMPILED |
| **Real filler (writes non-0xffff target refs to +0xb4..+0xba)** | **LOCATED_NOT_UNDERSTOOD** — only two static writers exist and both store 0xffff; selector is thread-private / non-literal-write, un-dumped |
| Target x/y world coordinates | NOT produced by any dumped function (not fabricated) |

**Net:** The waypoint *provider/filler* is **not located** in the static closure. What is firmly
established: it is NOT `a1fdf0`, NOT `7fa450`, and NOT reachable through the `00724790` trampoline
(that path is a stats dump). The entity's +0xb4..+0xba u16 refs are **consumed+reset per-tick in the
EXECUTE phase**, so the selector runs earlier in the **decision/RESOLVE phase of the un-dumped match
worker thread** — the same recovery blocker as the per-frame integrator. Next step: locate the match
worker-thread entry (via the `0x0156xxxx` `CreateThread` thunk), then the filler and the integrator
fall out together.

Doc path: `/Users/csadeveloper/cm/cm2008-reconstruction/research/WAYPOINT_PROVIDER_RECOVERY.md`
