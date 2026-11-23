# Runtime Picker Algorithm — FUN_005b5100 (candidate hypotheses)

**This document presents CANDIDATE algorithms, not a recovered one.** The picker's
real reduction is **UNKNOWN** (anti-disassembly, see
`RUNTIME_PICKER_INVESTIGATION.md §1.6`). Each candidate is implemented and
differentially tested in `match/sim/picker_ref/` over **synthetic** inputs only; no
candidate is validated against CM2008 because no legitimate runtime evidence exists
here (`RUNTIME_PICKER_EVIDENCE.md`).

Confidence tags: **VERIFIED**, **INFERRED**, **APPROXIMATE**, **UNKNOWN**.

---

## 1. Fixed constraints every candidate must satisfy (VERIFIED/INFERRED)

From the caller `FUN_00817f20` and the `.dat` format, any correct picker must:

1. Return a **1-based index** in `[1, n]`, consumed as `*(pool + idx*4 - 4)`.
   **VERIFIED** (prior static pass).
2. Select **one entry/bucket** of the pool's candidate array (not a section, not a
   raw value). **INFERRED** (strong).
3. Be the **only RNG consumer** in the quick resolver. **VERIFIED by absence**
   (0 inline LCG in `resolver/`, `match_player/` dumps).
4. Operate on **cumulative-count buckets / moment triples** from the VERIFIED `.dat`
   format (`cum` monotone within a group). **VERIFIED** (format).
5. Be **context-agnostic**: position/duty/squad/level/tactics choose *which*
   distribution is handed in; the picker only sees that distribution + the RNG.
   **INFERRED** (whole-pipeline consistent).

The reference enforces 1 and 2 by construction and tests 3–4's shape; 5 is a
property of the surrounding wiring, not of the reduction.

---

## 2. Candidate algorithms (all HYPOTHESIS)

Implemented in `match/sim/picker_ref/picker_ref.c` as `pr_candidate`:

### H-B `FLOAT_CDF` — weighted inverse-CDF via the float idiom  (PRIMARY candidate)
```
u01 = ((state>>9)|0x3f800000) as float − 1.0     // VERIFIED idiom, one LCG step
target = u01 * total                              // total = last cum
return smallest 1-based i with cum[i] > target
```
- **Why primary:** the float-uniform idiom is the one **VERIFIED to appear openly
  elsewhere** in the engine (`match_engine.c:6027/6032`, 26 sites game-wide). That
  is weak evidence — it shows the idiom is *in the codebase*, **not** that the
  picker uses it. Confidence: **HYPOTHESIS**.
- Consumes **exactly one** LCG step.

### H-A `MOD_CDF` — weighted inverse-CDF via integer modulo
```
r = lcg_next(state)                               // one LCG step
target = r % total          (total integral; .dat cum[] are integer counts)
return smallest 1-based i with cum[i] > target
```
- **Why plausible:** the `.dat` cumulative values are **integer counts**, and
  `r % total` is the natural integer inverse-CDF. The 24-bit integer idiom
  `state >> 8` is also VERIFIED in use (match-seed derivation), showing integer
  reductions occur. Confidence: **HYPOTHESIS**.
- Consumes **exactly one** LCG step. Biased for `total` not dividing 2³² (classic
  modulo bias) — distinguishable from H-B at the tails given enough samples.

### H-C `UNIFORM_N` — unweighted uniform pick (alternative reading a)
```
r = lcg_next(state); return r % n + 1
```
- Ignores weights entirely. Listed in `RESOLVER_INDEX_SELECTION.md §4` as a
  still-open lower-confidence reading. The VERIFIED cumulative-count structure of
  the `.dat` makes an *unweighted* pick unlikely (why store counts then ignore
  them?), but §2 of that doc does not by itself rule it out. Confidence:
  **HYPOTHESIS (low)**. Trivially falsified by a runtime index histogram that
  tracks weights.

### H-D `ARGMAX` — deterministic max-weight, RNG tie-break (alternative reading b)
```
pick bucket with max weight; if ties, draw one unit() to choose among the maxima
```
- A deterministic selector with RNG only as a tie-breaker. Distinctive, testable
  **RNG-consumption signature**: zero draws when the max is unique, one draw on a
  tie — unlike H-A/H-B/H-C which always draw once. Confidence: **HYPOTHESIS (low)**;
  retained precisely because its consumption pattern is the easiest to confirm or
  refute from `(state_before → state_after)` deltas.

---

## 3. RNG consumption analysis

| candidate | LCG steps per pick | how a corpus confirms/refutes |
|---|---|---|
| FLOAT_CDF (H-B) | exactly 1 | `state_after == step(state_before)` on every hit |
| MOD_CDF (H-A) | exactly 1 | same delta as H-B; separated by the index-vs-state fit, not the delta |
| UNIFORM_N (H-C) | exactly 1 | same delta; separated by index-vs-weight histogram |
| ARGMAX (H-D) | 0 (unique max) or 1 (tie) | data-dependent delta: `state` unchanged on unique-max hits |

**Open consumption questions (UNKNOWN):**
- Whether the picker makes **one** draw or **several** per invocation (e.g. a
  rejection loop like the engine's `while (u<p)` geometric draws at
  `match_engine.c:6027-6032`). Only `(state_after − state_before)` deltas across the
  corpus settle this. The reference assumes one draw for the CDF candidates — an
  **APPROXIMATE** assumption, clearly labelled.
- Whether `pool+0x1c` is truly the state read (HYPOTHESIS; engine convention
  VERIFIED).

---

## 4. Differential-test results over SYNTHETIC inputs

`cc -std=c99 -O2 -Wall -Wextra -o /tmp/t picker_ref.c test_picker_ref.c && /tmp/t`
(or `make -C match/sim/picker_ref test`). Result: **28427 checks passed, 0 failed.**

| test | what it proves (about the REFERENCE, not CM2008) |
|---|---|
| T0 LCG | `pr_lcg_next` matches the VERIFIED constants; `unit()` ∈ [0,1) over 1000 draws |
| T1 determinism | same `(state, weights)` → same index; identical 32-pick sequences; state advances identically — for all 4 candidates |
| T2 range | every result is a 1-based index in `[1,n]` over 5000 draws × 4 candidates |
| T3 consumption | FLOAT_CDF/MOD_CDF/UNIFORM_N consume exactly one LCG step; ARGMAX draws 0 on unique max, 1 on a tie |
| T4 boundaries | empty group → sentinel 0; single bucket → always 1; all-zero weights → in-range fallback (no div-by-0); FLOAT_CDF never selects zero-mass buckets |
| T5 encoding | raw weights and precomputed cumulative inputs agree (2000 draws) |
| T6 synthetic replay | over 200k draws, FLOAT_CDF shares `0.102/0.200/0.299/0.399` and MOD_CDF `0.100/0.199/0.300/0.401` track the weights `{10,20,30,40}` within 1pt; UNIFORM_N stays ~flat (distinguishing H-C) |

**These results are self-consistency properties of the reference.** T6 is a
**SYNTHETIC** replay — it is NOT a claim about CM2008's observed behaviour and must
never be cited as runtime evidence. The tests cannot and do not select which
candidate is correct; that requires the §2.4 regression in
`RUNTIME_PICKER_INVESTIGATION.md` over a real corpus.

---

## 5. Confidence levels (summary)

| claim | confidence |
|---|---|
| returns 1-based index into pool entry array | VERIFIED |
| selects a bucket/record entry (role) | INFERRED (strong) |
| is the resolver's only RNG consumer | VERIFIED (by absence) |
| indexes cumulative-count / moment buckets | VERIFIED (format) |
| context-agnostic (inputs pick distribution, not index) | INFERRED |
| RNG source = pool+0x1c | HYPOTHESIS (convention VERIFIED) |
| reduction = weighted inverse-CDF (H-A/H-B) | HYPOTHESIS |
| float vs modulo vs uniform vs argmax | UNKNOWN (undecided) |
| exactly one draw per pick | APPROXIMATE (assumed; needs corpus) |
| exact context→index decomposition | UNKNOWN |

---

## 6. Unresolved assumptions (carried by the reference until runtime closes them)

1. **One LCG draw per pick.** Assumed for the CDF candidates; could be a rejection
   loop. (APPROXIMATE)
2. **`pool+0x1c` is the RNG.** Natural from the engine convention but read inside the
   obfuscated body. (HYPOTHESIS)
3. **Strict vs non-strict CDF boundary** (`cum[i] > target` vs `>= target`) and
   whether the index is the first-exceeding or last-not-exceeding bucket — the
   reference uses first-strictly-exceeding; the exact comparator and rounding are
   **UNKNOWN** and only matter at bin edges.
4. **All-zero-weight behaviour** (empty `nBuckets=0` records): the reference falls
   back to a uniform pick; the original's handling is **UNKNOWN** (it may never be
   called on an empty record — the caller may short-circuit via the cursor/`+0x18`).
5. **Modulo bias in H-A** vs unbiased scaling: the exact 32-bit→`[0,total)` map is
   **UNKNOWN**.
6. **Context→index decomposition** (`contextBase` factoring into
   position×duty×squad×level): VERIFIED that a clean `contextBase` exists per file
   (`resolver-format.md §3`), its named-dimension factoring is **UNKNOWN**.

---

## 7. Bottom line

The picker is **NOT recovered**. Its interface, role, RNG dependency and data are
established; its exact index-reduction math, per-pick draw count, and context→index
mapping are **UNKNOWN**. Four labelled candidates are implemented and pass
self-consistency tests over synthetic inputs, but **differential validation against
the real `FUN_005b5100` is impossible here** because no legitimate runtime is
available (the only build is the forbidden cracked ISO). The exact additional
evidence needed is the breakpoint corpus in `RUNTIME_PICKER_EVIDENCE.md`,
captured read-only from a lawfully-owned unmodified install.
