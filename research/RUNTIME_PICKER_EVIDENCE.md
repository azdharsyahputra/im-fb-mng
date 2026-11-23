# Runtime Picker Evidence — FUN_005b5100

## 0. Status: NO LEGITIMATE RUNTIME EVIDENCE COLLECTED

**No runtime evidence for `FUN_005b5100` was or could be collected in this
environment.** There is no legitimate, unmodified CM2008 runtime in this sandbox;
the only binary present is the cracked `CM2008.exe` on the ISO, which this
investigation is forbidden to execute (DRM / anti-debugging). Executing it is also
the only way the protector would decrypt the picker's real instructions, so the
block is fundamental, not incidental.

Consequently this file contains **a schema and a collection protocol only**. Every
data cell below is a **placeholder/EXAMPLE SHAPE**, explicitly NOT an observation.
No register value, RNG state, return index, or weight tuple in this document is
real. Nothing here may be cited as measured behavior.

This is the expected, acceptable outcome per the investigation's hard rules:
document the block precisely rather than fabricate runtime data.

---

## 1. Corpus schema (one row per picker invocation)

Captured read-only at the sole clean caller `FUN_00817f20`, call site `0x00817f3c`
(`CALL FUN_005b5100`) and the instruction that consumes the return as
`*(pool + idx*4 - 4)`.

| field | type | source | meaning |
|---|---|---|---|
| `call_seq` | u64 | counter | monotonic invocation index within the match |
| `match_seed` | u32 | `DAT_00c84958` / `--matchseed` | fixes the whole match's noise |
| `pool_ptr` | u32 | `ECX` | distribution/pool object address |
| `cursor` | u32 | `[ECX+0x18]` | current-selection cursor (0 ⇒ sampling branch) |
| `state_before` | u32 | `[ECX+0x1c]` | LCG state at entry |
| `state_after` | u32 | `[ECX+0x1c]` after return | LCG state at exit |
| `n_entries` | u16 | walk entry array at `ECX+0x00` (bounded by cursor offset 0x18) | number of candidate buckets |
| `weight_block_ptr` | u32 | stack arg = `node+0x0c` | embedded weight/param block |
| `weights[]` | f32/u32[] | deref weight_block / entry attrs | raw or cumulative weights the reduction reads |
| `cum[]` | u32[] | from the matched `.dat` record/group | VERIFIED cumulative counts for cross-check |
| `return_index` | u16 | `AX & 0xFFFF` | the 1-based index returned |
| `selected_ptr` | u32 | `*(pool + idx*4 - 4)` | resolved entry (sanity: in entry array) |
| `file` | u8 | resolver context | which `MatchResolverDistributions.N.dat` |
| `section` | u8 | 0/1/2 | player-fine / player-coarse / team |
| `metric` | str | record name | e.g. Corners, PassesCompleted |
| `group` | u16 | CDF group index within the record | which context CDF |
| `position` | enum | EPositionValue | player position |
| `duty` | enum | EDutyValue | player duty |
| `squad` | enum | ESquadValue | squad tier |
| `performance_level` | u8 | Home/AwayPerformanceLevel | — |
| `mentality` | u8 | Home/AwayPlayingMentality | — |

Suggested on-disk form: newline-delimited JSON or CSV, one row per hit, arrays
hex-encoded. Keep raw `weights[]`/`cum[]` so the reduction fit is reproducible.

### EXAMPLE ROW SHAPE (NOT REAL DATA — all values are placeholders)
```
call_seq=<u64>  match_seed=<u32>  pool_ptr=<addr>  cursor=0
state_before=<u32>  state_after=<u32>  n_entries=<N>
weights=[...]  cum=[...]  return_index=<1..N>  selected_ptr=<addr>
file=<1..6> section=<0..2> metric=<name> group=<g>
position=<P> duty=<D> squad=<S> performance_level=<L> mentality=<M>
```

---

## 2. Collection protocol (read-only; no patching, no DRM bypass)

1. Attach an external debugger (x64dbg / WinDbg / Frida) to a **lawfully-owned,
   unmodified** CM2008 install on a Windows host. The protector decrypts
   `FUN_005b5100` in memory at runtime; do **not** dump/patch/modify it.
2. Set a breakpoint at `0x00817f3c` (module-base-relative). On hit: read `ECX`,
   `[ECX+0x18]`, `[ECX+0x1c]`, the stack arg (`node+0x0c`); step over the `CALL`;
   read `AX` and `[ECX+0x1c]` again; emit one schema row. Continue.
3. Drive matches with controlled inputs to fill the diversity axes (§3).
4. Correlate each row's `weights[]`/`cum[]` with the user's own extracted `.dat`
   via the VERIFIED parser (`match/sim/resolver_dat.c`).

---

## 3. Sampling plan (contexts to cover)

| axis | values to sweep | purpose |
|---|---|---|
| RNG | ≥10 distinct `MatchSeed`s | map state→index; confirm weight-tracking |
| determinism | each seed replayed ≥2× (`--repeatmatchseed`) | bit-identical index sequences |
| position | GK/DF/MF/FW + EPositionValue | confirm context-agnostic reduction |
| duty | full EDutyValue set | " |
| squad | full ESquadValue tiers | " |
| performance level | low/med/high | " |
| mentality/tactics | PlayingMentality, Passing/Pressing/Offside | distribution-selection only |
| metric shape | Corners, Penalties, PassesCompleted, empty (nBuckets=0) | boundary coverage |

Target corpus size: enough per `(metric, group)` cell that the `return_index`
histogram is stable to ≤1 percentage point (≈10⁴–10⁵ hits per distinct weight
table) — matching the precision the synthetic self-consistency test already uses.

---

## 4. EXACT additional evidence required to close each UNKNOWN

| UNKNOWN | evidence that resolves it |
|---|---|
| index-reduction math (FLOAT_CDF vs MOD_CDF vs UNIFORM_N vs ARGMAX) | regression of `return_index` on `(state_before, cum[])` over the corpus (§2.4 of RUNTIME_PICKER_INVESTIGATION) |
| LCG steps consumed per pick | `(state_after − state_before)` delta pattern: =1 step, =k steps, or data-dependent |
| whether `pool+0x1c` is the RNG source | observe `[ECX+0x1c]` actually changes across the call |
| unweighted vs weighted (rules out UNIFORM_N) | index histogram vs `cum[]` shape at fixed inputs |
| determinism / seed-fixes-noise | identical rows for replayed seed; changed rows when one tactical bit flips |
| context→index decomposition | join rows on (position,duty,squad,level) vs (file,section,metric,group) |

Without this corpus, the candidates in `match/sim/picker_ref/` remain
**HYPOTHESIS** and the picker remains **NOT recovered**.
