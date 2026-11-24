# Match RNG lifecycle (CM2008) — seed → state → draws → reset

**Scope:** the PRNG behind both match paths — the **quick resolver**
(MatchResolver, picker-based) and the **detailed command-tree engine**
(`FUN_009e8ca0`/`FUN_0077c710`…). Covers seed source, init timing, state storage,
call sites + order, engine sharing, draw conditionality, reset policy, and
deterministic tests.

Confidence: **VERIFIED** (read from clean disassembly), **INFERRED**,
**HYPOTHESIS**, **UNKNOWN**.

---

## 0. The PRNG (VERIFIED)

A single inlined LCG is used game-wide (Numerical-Recipes constants):

```
state = state * 0x19660d + 0x3c6ef35f   (mod 2^32)    ; 1664525, 1013904223
```

- Inlined at **95+ sites** in the full dump, 6 in `match_engine.c`. Never a shared
  function — each consumer expands it. (VERIFIED, grep.)
- Uniform-float idiom (VERIFIED, 26 sites): `(float)(state>>9 | 0x3f800000) - 1.0`
  → `u ∈ [0,1)` (take top 23 mantissa bits, set exponent to 1.0, subtract 1).
  Centered variants: `((u)-0.5)*K` → symmetric perturbation (e.g.
  `…*20.0` = ±10, `big` dump `0x16551`).
- Integer idiom (VERIFIED): `state >> 8` → 24-bit value (used for the generated
  match seed, `FUN_0077cd40`).

---

## 1. Seed source & init timing

### 1a. Per-match seed = `MatchSeed` input field (VERIFIED)
`GCMatchInputs` schema `FUN_00a72db0` registers **`MatchSeed`** as a `uint32` at
struct offset 8 (`_DAT_00c70da0 = "MatchSeed"`), alongside the tactical inputs
(`Home/AwayPlayingMentality`, `…TeamDutiesID`, `…PassingStyle`, `…PressingStyle`,
`…OffsideTrap`, `…PerformanceLevel`, `HomePossession`, …). So every match carries
its own seed in its input record — this is what makes a match **replayable /
reproducible**. (VERIFIED.)

### 1b. Command-line overrides (VERIFIED strings + binding)
`FUN_00a2c3d0` (the CLI/settings parser; references all three) and
`FUN_0077d890` bind options to globals:

| option | global | role | conf |
|---|---|---|---|
| `--matchseed`   | `DAT_00c84958` | forces the match seed value | VERIFIED |
| `--repeatmatchseed` | `DAT_00c84954` | reuse-seed flag (replay same match) | INFERRED |
| `--gameseed`    | (a2c3d0 binding) | forces the global/game seed | VERIFIED (str) |
| `--ReRandomiseOnLoad` | (in `FUN_007f4fa0`) | re-seed on save load | VERIFIED (str) |
| `--randomtactics`, `RandomSeed`, `RandomBG` | — | related test knobs | VERIFIED (str) |

Other relevant strings: `m_uGameSeed` (saved-game global seed; errors
`"Error reading/writing m_uGameSeed"`), handled by load `FUN_007f4fa0` /
write `FUN_007f3130`. (VERIFIED strings; bodies of a2c3d0/7f4fa0/7f3130 are large
and partly stack-obfuscated, so exact assignment lines are INFERRED.)

### 1c. Generated fallback seed (VERIFIED)
Match kickoff `FUN_0077cd40`:
```
if (DAT_00c84958 == 0) {                         ; no seed supplied
    p = FUN_007e7c10();                          ; &globalSingleton->0x2c  (global RNG)
    v = *p * 0x19660d + 0x3c6ef35f;  *p = v;      ; advance GLOBAL rng
    DAT_00c84958 = v >> 8;                        ; derive a 24-bit match seed
}
… FUN_009efa20(…, DAT_00c84958, …)               ; hand seed to match start
```
So **init timing**: the match seed is resolved **at match start**, from (priority)
CLI `--matchseed` → the `MatchSeed` input field → else freshly drawn from the
global RNG. (VERIFIED for the fallback branch; priority order INFERRED.)

### 1d. Global/game seed init (INFERRED)
`m_uGameSeed` seeds the **global** RNG (singleton at `DAT_00c800a0`/`DAT_00c84958`
family); on new game it is randomised, on load it is restored unless
`--ReRandomiseOnLoad`. (INFERRED from string roles; the write path is
`FUN_007f3130`, read path `FUN_007f4fa0`.)

---

## 2. State storage locations (VERIFIED)

Two distinct storage shapes coexist:

| state | location | accessor | used by | conf |
|---|---|---|---|---|
| per-object stream | `object + 0x1c` | inline | detailed engine (`FUN_009e8ca0 @0x009e8d…`, `+0x1c *= …`) only. The picker does NOT touch +0x1c (REFUTED — RESOLVER_PICKER_CLUSTER_RECOVERY.md) | VERIFIED (engine); picker use REFUTED |
| global stream base | `singleton + 0x2c` | `FUN_007e7c10` → `return this+0x2c` | match kickoff `FUN_0077cd40`, global draws | VERIFIED |
| global stream array | `*(DAT_00c800a0 + 0x2c) + index*0x40` | `FUN_009fdd60` | per-stream draws in detailed engine (`FUN_0077c710`) | VERIFIED |

`FUN_009fdd60` (VERIFIED) returns `*(this+0x28)*0x40 + *(DAT_00c800a0+0x2c)` with
two sentinel guards (`0x20000000`, `0x3fffffff` → return 0). So the global RNG is
actually an **array of 0x40-byte stream slots** selected by an index at
`this+0x28` — i.e. the engine keeps **multiple independent sub-streams** off one
base, not a single global counter. (VERIFIED.)

---

## 3. Call sites & ORDER

### 3a. Detailed engine, per sim step (VERIFIED — `FUN_0077c710 @0x6025`)
Canonical event-generation block, in this order:
```
p = FUN_009fdd60()                           ; fetch this step's stream slot
share_home = possByte_home/(h+a); share_away = …   ; from +0x3c possession bytes
draw u; while (u < K*share_home) { FUN_005cd020(0); draw u; }   ; home events, geometric-style
draw u; while (u < K*share_away) { FUN_005cd020(1); draw u; }   ; away events
… (further draws for event subtype)
```
Order is **deterministic**: home branch fully drains before away; each draw is the
next LCG step of the *same* slot. (VERIFIED.)

### 3b. Detailed engine, resolution object (VERIFIED — `FUN_009e8ca0 @0x009e8d…`)
Single `+0x1c` advance, **inside a conditional branch** (only when the engine
takes the "non-trivial" path, guarded by `iVar1 != in_ECX+4`), then builds the
event. So this object's stream advances **once per qualifying node**. (VERIFIED.)

### 3c. Quick resolver (INFERRED/HYPOTHESIS)
The resolver (`resolver/`, `match_player/` dumps) has **0 inline LCG**. All draws
NOT happen inside `FUN_005b5100` — it is a deterministic key→index lookup with NO RNG (REFUTED, RESOLVER_PICKER_CLUSTER_RECOVERY.md). The resolver draw (if any) is upstream. [stale text kept:] ~~happen inside the obfuscated picker `FUN_005b5100`~~ (one draw per selector-leaf
evaluation) — see RESOLVER_INDEX_SELECTION.md. Order = tree-evaluation order of
the formula nodes (`FUN_00818120` iterates children `[+0x38,+0x3c)` in address
order; `FUN_00817df0` aggregates in iterator order). So draw order is **the
deterministic DFS order of the resolver node tree**. (INFERRED; the per-draw
mechanism is UNKNOWN due to obfuscation.)

---

## 4. Do the two engines share RNG state? (INFERRED — they do NOT share live state)

- They are **alternative paths** for the same fixture: quick resolver (instant
  result) vs detailed command-tree engine (watched match). Only one runs per
  match. (INFERRED from architecture.)
- They use the **same algorithm** (§0) and the **same seed source** (`MatchSeed`,
  §1), but **separate state instances**: the resolver advances a pool object's
  `+0x1c`; the detailed engine advances `singleton+0x2c`-based stream slots and
  per-object `+0x1c`. No code path threads one engine's live state into the
  other. (INFERRED.)
- **Within** an engine, state is **propagated by copy** on context fork — VERIFIED
  at `FUN_009e8850 @0x9165`:
  `*(child+0x18)=*(parent+0x18); *(child+0x1c)=*(parent+0x1c);` (copies cursor +
  RNG when spawning a sub-resolution). So sub-contexts **inherit** the parent's
  RNG snapshot rather than sharing a pointer — forks are reproducible but then
  diverge. (VERIFIED.)

Net: **same seed ⇒ same result for a given engine**, but a fixture resolved
quick vs watched will generally give **different** detailed event streams (same
seed, different consumption pattern). Final scorelines may still match only if the
distributions are calibrated to — that is a game-design question, UNKNOWN here.

---

## 5. Are draws conditional? (VERIFIED — yes, pervasively)

- Fallback match seed drawn **only if** `DAT_00c84958 == 0` (§1c). VERIFIED.
- `FUN_009e8ca0` `+0x1c` advance gated by a branch condition. VERIFIED.
- `FUN_0077c710` uses **loops** (`while(u < p)`) → a *variable* number of draws
  per step, dependent on earlier draws and on possession. VERIFIED.
- `FUN_009fdd60` returns 0 (no stream) for sentinel indices → some steps draw
  nothing. VERIFIED.

Consequence for reproducibility: **draw count is data-dependent**, so you cannot
predict the Nth global state without replaying the exact branch/loop history.

---

## 6. Reset policy between matches / sims (INFERRED)

- **Per match:** state is (re)seeded from that match's `MatchSeed` at start
  (§1), so each match begins from a defined point; `--repeatmatchseed`
  re-uses the previous seed to replay. (INFERRED from seed plumbing.)
- **Global stream:** persists across matches within a session (it is the
  singleton `DAT_00c800a0+0x2c`); it is what *generates* fresh match seeds, and
  it is itself seeded from `m_uGameSeed` / re-randomised per `--ReRandomiseOnLoad`.
  (INFERRED.)
- No evidence of a per-match reset of the *global* stream (that would defeat its
  role as seed generator). UNKNOWN whether the resolver pool's `+0x1c` is
  re-seeded per formula or carried across formulas within one match — depends on
  pool lifetime (UNKNOWN; obfuscation-blocked).

---

## 7. Proposed deterministic tests

### T1 — Validate the LCG itself (VERIFIED math; unit test)
Given `s0`, assert `s1 = (s0*1664525 + 1013904223) & 0xffffffff` and
`u = ((s>>9)|0x3f800000) reinterpreted as float) - 1.0 ∈ [0,1)`. Cross-check the
26 float-draw sites reproduce known deltas (e.g. `((u-0.5)*20)` ∈ [-10,10)).

### T2 — Generated match-seed derivation (VERIFIED path)
Set `DAT_00c84958=0`, global state = `G`. Assert kickoff yields match seed
`(G*1664525+1013904223)>>8` and advances the global state to
`G*1664525+1013904223`. (Breakpoint `FUN_0077cd40`, log before/after.)

### T3 — Per-output validation of the picker (recovers the UNKNOWN)
Runtime capture at `0x00817f3c` (`CALL FUN_005b5100`): log `ECX` (pool ptr),
`[ECX+0x1c]` (state in), the stack arg (`node+0x0c`), and `AX` (index out), plus
`[ECX+0x1c]` after. Across many calls:
- confirm `[ECX+0x1c]` advances by exactly the LCG step per call (proves it is the
  RNG consumer and how many draws it makes);
- regress `AX` against `(state, node+0x0c weights)` to identify the reduction
  (modulo vs inverse-CDF vs float-uniform×N). This is the only way to close §4/§5
  of RESOLVER_INDEX_SELECTION.md.

### T4 — State-progression / determinism (end-to-end)
Resolve the same fixture twice with identical `MatchSeed`; assert **bit-identical**
resolver outputs (scoreline + every `*Performance` stat). Then flip one input bit
(e.g. `HomePlayingMentality`) and assert the output changes — proves inputs route
through the distribution selection, not the RNG stream, i.e. seed alone fixes the
noise. Use `--repeatmatchseed` to confirm replay equivalence.

### T5 — Fork inheritance (VERIFIED path)
At `FUN_009e8850` fork, assert `child[0x1c]==parent[0x1c]` at copy time, then that
child and parent diverge on subsequent draws (independent streams post-copy).

### T6 — Engine divergence (INFERRED hypothesis test)
Resolve fixture quick vs detailed with the same `MatchSeed`; log both state
trajectories. Expect **different** consumption (different draw counts/order),
confirming separate live state despite shared seed (§4).

---

## 8. Open / UNKNOWN
- ~~Exact reduction inside `FUN_005b5100` (obfuscated) — needs T3.~~ RESOLVED: no reduction exists — `FUN_005b5100` is a key→index lookup, NO RNG (RESOLVER_PICKER_CLUSTER_RECOVERY.md). T3 is moot for the picker; the open item is WHERE the resolver draw lives (upstream match engine, not here).
- Exact assignment of `m_uGameSeed` → global state register (bodies of
  `FUN_00a2c3d0`/`007f4fa0`/`007f3130` are large/stack-obfuscated).
- Resolver pool lifetime / whether `+0x1c` re-seeds per formula within a match.
- Whether `FUN_005cd020` (event emitter in the draw loops) itself draws further
  RNG (not yet traced).

### Source artifacts
`agentD2_out/rng_disasm.txt` (`009e8ca0`, `0077cd40`, schema `00a72db0`, seed fns),
`agentD2_out/rng2_disasm.txt` (`007e7c10`, `009efa20`),
`agentD2_out/acc_disasm.txt` (`009fdd60`),
`agentD2_out/seedxref/xref.c` (`00a2c3d0`,`0077d890`,`007f4fa0`,`00a2ace0`),
`docs/ghidra_dumps/raw/match_engine.c` (LCG sites, state-copy `0x9165`),
project `~/tools/cm2008_full`.
