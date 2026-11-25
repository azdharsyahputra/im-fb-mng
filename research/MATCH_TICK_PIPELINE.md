# Match tick pipeline — spatial duel → side aggregate → balance tier (0x4b/0x4e–0x4f)

Traced in response to "Trace tick fisika". This documents the recovered per-player → per-side →
team-balance resolution chain in the 0x4e–0x4f match-engine module, the entity memory layout it walks,
and how it connects to (but is distinct from) the formula-driven physics **config** object in 0x9f.

All addresses are VA (crack cm2008.exe, Ghidra 12.1.4). Labels: **VERIFIED** = read directly from the
decompiled body; **INFERRED** = structure deduced, not byte-proven; **UNKNOWN** = not yet traced.

---

## 1. The physics config is a load-once global singleton (0x9f) — NOT a per-tick object

Re-confirmed this pass, correcting the earlier hunt that treated the 0x9f readers as tick candidates:

- `DAT_00c682f4` — global pointer to the physics-config object (4-byte slot; the object itself is
  allocated by `FUN_004d0790`). **VERIFIED.**
- `FUN_009f4730` — the lazy getter (`GetPhysicsConfig`): allocates + constructs on first call, registers
  the destructor via `_atexit(&LAB_009f2190)`, returns the pointer. Its **only caller is the initializer
  `FUN_009f4dd0` itself** (`procedure_callers` = [0x9f4dd0], complete). **VERIFIED.**
- `FUN_009f4dd0` — the 10,447-byte initializer (range 0x9f4dd0–0x9f769e): 359 formula.xml accessor calls
  → 363 struct-field writes. Runs once at config construction. **VERIFIED** (size/role).
- `LAB_009f2190` / `0x9f21a2` — the atexit **destructor** label (why `resolve_containing_procedure`
  returned "not_in_procedure": it is an atexit thunk, not a tick). **VERIFIED.**

Consequence: the config singleton is populated once from formula.xml and then **read directly through the
global** by consumers. It is static per match, not integrated per frame. The per-frame work is elsewhere
(the 0x4e–0x4f module below). The genuine external consumer of the config region resolved to
`FUN_004b8ef0` (0x4b8ef0), which is the per-player step of that module.

---

## 2. Match entity memory layout (VERIFIED from the walkers)

Two independent aggregators (`FUN_004eff60`, `FUN_004efef0`) walk the same array with the same stride,
which pins the layout:

- **Entity stride = `0x34c` (844 bytes).** Proven two ways: `FUN_004efef0` advances its pointer by
  `0xd3` ints (0xd3·4 = 0x34c), and `FUN_004eff60` indexes `i * 0x34c`.
- **23 entities per side** (`0x17` loop count in both walkers) — full matchday squad incl. bench.
- **Side block size = `0x4bd4` (19,412 = 23 · 844).** `FUN_004efef0` bases its side at
  `side * 0x4bd4 + 0x8d8 + matchstate[2]`.
- **Side index** = `*(char *)(*matchstate + 0x6d)` (0 or 1). **VERIFIED.**
- **Entity array base** = `matchstate[2]` (+ a per-walker field offset: 0x7e8 in FUN_004eff60, 0x8d8 in
  FUN_004efef0 — the two read different sub-fields of the same records). **INFERRED** (base identity).
- Each entity has a **"body" sub-object pointer at `entity+0xf0`**. Body fields seen:

| Body offset | Type | Meaning | Evidence |
|---|---|---|---|
| `+0x08` | float | **continuous X position** | `FUN_004b8ef0` reads, ×100 → byte |
| `+0x10` | float | **continuous Y position** | `FUN_004b8ef0` reads, ×100 → byte |
| `+0xd0` | byte (bit0) | active flag (bit0 set = skip) | both walkers gate on `& 1` |
| `+0x174` | ptr | own-entity detail record (has +0x594 dword, +0x20 byte) | FUN_004b8ef0 |
| `+0x180` | uint | side index (XOR 1 selects opponent) | FUN_004b8ef0 |
| `+0x189` | byte | contest parameter | FUN_004b8ef0 |
| `+0x1a8` | — | contest accumulator (passed to FUN_004b0aa0) | FUN_004b8ef0 |
| `+0x1b1` | byte | active sentinel (0xFF = inactive) | FUN_004b8ef0 |
| `+0x1b6` | byte | **local-duel result** (written per tick) | FUN_004b8ef0 writes; FUN_004eff60 reads |
| `+0x1b8` | byte (0..100) | **condition / fitness** attribute | FUN_004efef0 reads |
| `+0x1ba` | byte | quantized X (= X·100) | FUN_004b8ef0 writes |
| `+0x1bb` | byte | quantized Y (= Y·100) | FUN_004b8ef0 writes |

The float X/Y at body+0x08/+0x10 are the **smoking gun that CM2008 keeps continuous per-player
positions** — the match is a spatial simulation, not a view-only animation. (This closes out the
retracted "movement is view-only" error for good.)

---

## 3. The resolution chain (bottom-up)

### 3a. `FUN_004b8ef0` — per-player: commit position + local duel (VERIFIED)

thiscall, ECX = entity. For an active entity (`body+0x1b1 != 0xFF`):
1. Select the opponent: `oppIdx = body+0x180 ^ 1`, indexed through `entity+0xc` table at +0x4c.
2. Resolve a local duel: `FUN_004b0aa0(body+0x1a8, own+0x594, opp+0x594, own+0x20, opp+0x20, body+0x189)`
   → stored to **body+0x1b6**.
3. Quantize the continuous position: `body+0x1ba = (byte)(body+0x08 · 100.0)`,
   `body+0x1bb = (byte)(body+0x10 · 100.0)` (via the `+8388864.0`/`SUB41` float→int magic).

So this step **reads** the already-updated continuous position and **resolves** the player's local
contest. It does NOT integrate position (no `pos += vel·dt` here).

### 3b. `FUN_004eff60` — per-side: mean local-duel outcome (VERIFIED)

Loops the side's 23 entities; for each active one calls `FUN_004b8ef0` then accumulates its `body+0x1b6`;
returns the **mean** over active players. This is side A's (or B's) aggregate attacking/contest strength
for the phase.

### 3c. `FUN_004efef0` — per-side: mean condition (VERIFIED)

Same 23-entity walk; accumulates `body+0x1b8` (the 0..100 condition/fitness byte); returns the mean.

### 3d. `FUN_004f1570` — team-balance ratio → 5-tier outcome (VERIFIED constants)

Combines both sides' aggregates into a single balance ratio and buckets it:

```
strengthMult(side) = (meanDuelOutcome(side) - 55.0) * 0.5 + 1.0      // pivot 55, slope 0.5, base 1.0
conditionTerm(side) ≈ base * (1 - (100 - meanCondition(side)) * 0.01) // fitness drag, scale 0.01

ratio = strengthMult(A) * conditionTerm(A)
      / ( strengthMult(B) * conditionTerm(B) )

tier = 0   if ratio < 0.30
       1   if ratio < 0.65
       2   if ratio <= 1.30
       3   if ratio <= 3.00
       4   if ratio >  3.00
```

**VERIFIED numeric constants:** rating pivot **55.0**, strength slope **0.5**, base **1.0**, condition
scale **0.01 / 100.0**, tier thresholds **0.30, 0.65, 1.30, 3.00** → tiers {0,1,2,3,4}.

The pivot **55** is consistent with the 7-bit condensed-rating family (0..127, "average" player ≈ 55–64);
each rating point above/below average shifts the strength multiplier by 0.5. The exact algebra of the two
`conditionTerm` sides is slightly obscured by x87 ST0/ST1 register artifacts in the decompile (the two
sides are not written symmetrically), so the condition-term shape is **INFERRED**; the strength term and
all the literal constants/thresholds are **VERIFIED**.

### 3e. Callers of `FUN_004f1570`

`procedure_callers(0x4f1570)` → the two sibling drivers `0x4f1570`/`0x4f16d0` are themselves called from
higher in the 0x4f module (the per-phase step). The 5-tier return value is the macro event-outcome bucket
the phase consumes. **UNKNOWN:** the tier→event mapping (what tier 0..4 each means — clear chance, half
chance, even midfield, etc.) and the phase scheduler above it.

---

## 4. What is now VERIFIED vs still UNKNOWN

**VERIFIED this pass:**
- Config is a load-once global singleton (DAT_00c682f4), not a per-tick object; getter/init/destructor
  identified.
- Entity array layout: 23/side, stride 0x34c, side block 0x4bd4, side index at matchstate+0x6d, body at
  entity+0xf0.
- Continuous per-player float positions at body+0x08 (X) / +0x10 (Y), quantized ×100 to body+0x1ba/+0x1bb.
- The per-player → per-side → balance-tier resolution chain and all its literal constants (pivot 55,
  slope 0.5, thresholds 0.30/0.65/1.30/3.00).

**STILL UNKNOWN (honest backlog):**
- **The position INTEGRATION itself** — the upstream function that *writes* body+0x08/+0x10 from
  velocity/heading each tick (steering, acceleration tiers, ball flight, collision). FUN_004b8ef0 only
  *reads* the committed position. This is the remaining "true physics integration" target; it consumes
  the 0x9f config constants (accel 3.5–5.5, speed tiers, turning circle) but its exact VA is not yet
  resolved.
- `FUN_004b0aa0` internals (the local-duel kernel) — not yet decompiled.
- The condition-term algebra in FUN_004f1570 (x87-obscured) and the tier→event mapping.
- The per-phase scheduler above FUN_004f1570/FUN_004f16d0.

---

## 4b. Second behaviour config + the marking pass (traced while hunting the integrator)

- **`DAT_00c68428`** — a SECOND match config singleton, 0x5e0 (1504) bytes, constructed by
  `FUN_004e4e80`, destructor `LAB_004800f0`, lazy getter `FUN_00480110`. It is read from **several
  hundred sites** across 0x49–0x4f (and is also populated inside the big initializer FUN_009f4dd0, so
  that 10KB routine seeds BOTH config objects). This is the master behaviour/physics parameter block the
  live match math reads. Its field +0xe8 is a float scale used in the reachability term below.
  **VERIFIED** (identity/size/constructor); field meanings mostly **UNKNOWN**.
- **`FUN_004f4b10`** — the opponent **man-marking / assignment** pass. Loops the opposing side's 23
  entities (side `^1`), commits each position via `FUN_004b8ef0`, tracks several "best candidate"
  extrema, then writes assignment codes {1,2,3,4} into `matchstate+0x250 + slot*8` with a target id at
  `+0x24c` (up to 0x62=98 slots). Recovered decision inputs:
  - **reachability term** `(1 − rng · body+0x0c · DAT_00c68428+0xe8) · entity+0x17c` — compared against
    **0.5**; `body+0x0c` behaves as a SPEED/closing-rate term, `entity+0x17c` as a cap. **VERIFIED**
    shape; semantics INFERRED.
  - role/position gate `body+0x74` with thresholds **>9, <7, >6**; duel-rating gate `local_27 < 0x4c`
    (76). **VERIFIED** constants.
  This is marking logic (who picks up whom), not position integration — it READS positions.

### Extra body fields seen this pass

| Body offset | Type | Meaning | Evidence |
|---|---|---|---|
| `+0x0c` | float | speed / closing-rate (×config+0xe8 in reachability) | FUN_004f4b10 |
| `+0x14` | float | threat / secondary position metric (tracked as a max) | FUN_004f4b10 |
| `+0x1b7` | byte | secondary per-player metric (tracked as a max) | FUN_004f4b10 |
| `+0x74` | int | role / position id (gated >9 / <7 / >6) | FUN_004f4b10 |

The four consecutive floats at body **+0x08, +0x0c, +0x10, +0x14** are consistent with a
position/velocity pair — pos = (+0x08 X, +0x10 Y), with +0x0c / +0x14 as the matching rate/velocity
components — but that pairing is **INFERRED**, not yet proven by the writer.

### The four FUN_004b8ef0 callers are all READERS

`FUN_004eff60` (side duel-mean), `FUN_004f09b0`/`FUN_004f0a10` (thin "commit N times" wrappers, count
from `matchstate[2]+0xd8 + 0x48e + side`), `FUN_004f4b10` (marking). None writes body+0x08/+0x10 — they
all consume the already-updated continuous position. So the integrator is in a separate subsystem
(reached before this consumer cluster runs), not yet located by static call-graph walking.

## 4c. Decision / targeting layer + the nature of the positions (integrator hunt, pass 2)

Climbing the call graph from the consumer cluster reached the match-tick top and a second, parallel
DECISION subsystem. Both read positions; neither writes body+0x08/+0x10.

**Match-tick top = `FUN_00498ac0`** (set-piece / phase handler): branches on match-state enum
`*state+0x34 == 6`, swaps the attacking/defending side pointers (`+0xcc/+0xd0/+0xd4/+0xd8`), and invokes
the **tactical phase `FUN_004f8b90`** (once or twice, gated on config flags +0x4b8/+0x4b9). The tactical
phase calls `FUN_004f1040`, `FUN_004f1070`, `FUN_004f2180`, `FUN_004f4b10` (marking), `FUN_004f8a30`.

**`FUN_004f2180` — team-posture selector (VERIFIED, and it pins the coordinate system):** averages four
per-team metrics over the **11 active starters** (`×0.09090909 = 1/11`, bench skipped via body+0xd0 bit):
- `Σ(1.0 − body+0x08)` — mean of (1 − X)
- `Σ(1.0 − body+0x10)` — mean of (1 − Y)
- `Σ(1.0 − body+0x1b8·0.01)` — mean of (1 − condition/100)
- `Σ body+0x14` — mean threat metric

then takes the dominant metric → posture code {1,2,3,4} and applies it to all players via
`FUN_004ef5c0` / `FUN_004ef490` / `FUN_004f1860` / `FUN_004ef530`. Because it computes `1.0 − body+0x08`
and `1.0 − body+0x10` as "distance from the far end", the **positions are NORMALIZED pitch fractions in
0..1** (0 = own end, 1 = opponent end), not metres — which is why FUN_004b8ef0 quantizes them ×100 into a
0..100 "percent of pitch" byte. **VERIFIED.** (A sentinel `float 1000.0` = 0x447a0000 is used as an
init-max elsewhere in the same function.)

**`FUN_004ef490` — per-player zone target from X (VERIFIED coeffs):** reads the player's X (body+0x08),
clamps it at `entity+0x268`, and builds a target box as four linear functions of X:
`{24·X − 12, −20·X + 20, −6·X + 6, −2·X + 2}`, passed to `FUN_004a8160`.

**`FUN_004a8160` — candidate scoring / selection (not a position write):** adjusts the zone bounds by
`entity+0x260`, scales by `(12 − tier)/11`, weights them through `FUN_004a7590/76e0/7670/7600` (×0.1 and
×0.0075), forms a composite score `(p2 + p3 + p4) − p1`, and accumulates it into the player's
option-selection struct at `body+0x174` (running sum +0x14, count +0x1c, max +0x10, min +0x18, with the
chosen option id taken from `body+0x1a8`). This is argmax/argmin DECISION-MAKING over candidate targets,
feeding the selection struct — it writes scores and the picked option, never the continuous position.

## 4d. Conclusion of the integrator hunt (honest)

Two full call-graph trees were traced to their roots:
1. **Resolution** — FUN_004b8ef0 → FUN_004eff60/FUN_004efef0 → FUN_004f1570 → FUN_0048cc10 (duel → side
   aggregate → balance tier).
2. **Decision/targeting** — FUN_00498ac0 → FUN_004f8b90 → FUN_004f2180 → FUN_004ef490 → FUN_004a8160
   (posture → zone target → candidate scoring).

**Every function in both trees READS `body+0x08`/`body+0x10`; none writes them.** So the position
INTEGRATOR (the per-tick writer that advances the normalized X/Y from velocity/heading) is architecturally
SEPARATE from — and upstream of — the entire match-logic module (0x4b–0x4f). It is not reachable by
call-graph walking from the logic that consumes positions.

What this establishes as VERIFIED: CM2008 maintains continuous normalized (0..1) per-player pitch
positions, and the match LOGIC (both the statistical resolver and the tactical/decision layer) is driven
by reading those positions plus the condensed ratings and the two behaviour-config singletons. What
remains UNKNOWN: the exact function that writes body+0x08/+0x10 each tick. Pinning it needs a different
technique than call-graph walking — either (a) identifying the body object's class via its allocator /
constructor (the entity-array initializer that stores the body pointer at entity+0xf0) and reading its
update method, or (b) runtime observation (watch writes to a body's +0x08). Static call-graph tracing
from the consumer side has been exhausted.

## 4e. Allocator hunt — the body is a POLYMORPHIC object; the mover is a VIRTUAL method

Pursuing the body-object class, the lineup/role-setup path was mapped (all readers, no position write):
- `FUN_004ef8a0` — lineup accessor: `*((matchstate[2]+0xd8)+0x114 + (side·11 + idx)·4)` — a table of **11
  starter pointers per side** (pointers into the 23-entity array), separate from the physical array at
  `matchstate[2]+0x7e8`.
- `FUN_004f8a30` — per-starter lineup/marking setup: clears body+0x7c / +0x214, assigns the marking
  target `body+0x8c`, accumulates `entity+0x2bc` into `matchstate+0x59c`, then cascades to six per-phase
  steps (FUN_004f4eb0/12c0/efaa0/f1350/ef8d0/ef9b0).
- `FUN_004efaa0` — assigns each starter a role/zone id (`body+0x74 = FUN_004a7f20(matchstate+0x5fc)`).
- `FUN_004aebe0` — thin forwarder to `FUN_004ae3f0(body+0xd8, *(body+0x174)+0x5fc)`.

**The decisive structural finding:** across ~17 traced functions, **`body+0x00` is never read as data** —
it is only ever the base for `+0x08`, `+0x10`, etc. That is the signature of a **C++ vtable pointer**: the
body is a polymorphic object. Its per-tick position update is therefore almost certainly a **virtual
method, invoked through an indirect call** `(*body->vtbl[n])(body)`. Ghidra's call graph resolves only
direct calls (it states plainly: "unresolved computed or indirect calls remain unknown"), so a
virtual-dispatched mover is **invisible** to the resolved-call-graph walking used throughout this trace —
which is exactly why every reachable function reads the position and none writes it.

### Recovered body-object field map (body = *(entity+0xf0))

| Offset | Type | Meaning | Status |
|---|---|---|---|
| +0x00 | ptr | **vtable pointer** (never read as data) | INFERRED |
| +0x08 | float | X position, normalized 0..1 | VERIFIED |
| +0x0c | float | speed / closing-rate | INFERRED |
| +0x10 | float | Y position, normalized 0..1 | VERIFIED |
| +0x14 | float | threat / forward metric | INFERRED |
| +0x3c | byte | role index (<11) | VERIFIED (read) |
| +0x74 | int | role / zone id (assigned by FUN_004efaa0) | VERIFIED |
| +0x7c | u32 | cleared at setup | VERIFIED |
| +0x8c | ptr | assigned marking opponent | VERIFIED |
| +0xd0 | byte | flags; bit0 = inactive/bench | VERIFIED |
| +0xd8 | ptr/u32 | passed to FUN_004ae3f0 | VERIFIED (read) |
| +0x174 | ptr | candidate/option selection struct | VERIFIED |
| +0x180 | uint | side index (XOR 1 = opponent) | VERIFIED |
| +0x189 | byte | contest parameter | VERIFIED |
| +0x18e | byte | cleared on role change | VERIFIED |
| +0x1a8 | u32 | contest / option accumulator id | VERIFIED |
| +0x1b1 | byte | active sentinel (0xFF = inactive) | VERIFIED |
| +0x1b6 | byte | local-duel result | VERIFIED |
| +0x1b7 | byte | secondary per-player metric | VERIFIED |
| +0x1b8 | byte | condition / fitness (0..100) | VERIFIED |
| +0x1ba | byte | quantized X (= X·100) | VERIFIED |
| +0x1bb | byte | quantized Y (= Y·100) | VERIFIED |
| +0x214 | u32 | cleared at setup | VERIFIED |

### Terminal conclusion of the integrator hunt

The position-integration WRITER cannot be reached by resolved-call-graph walking because it is a
**virtual method** on the polymorphic body object. Threading it statically now requires either:
1. **vtable archaeology** — find the body's constructor (the `mov [obj+0], <vtable_addr>` store, which
   needs a byte/offset-write search the current REA call-graph tools do not provide) and read the vtable's
   function pointers; or
2. an **indirect-call / data-flow resolver** that maps the virtual dispatch to its concrete target.

Runtime observation (watchpoint on a body's +0x08) is **excluded** by the project's rule against executing
the cracked binary. So the movement writer is, for now, the documented static-analysis plateau; everything
that CONSUMES the positions (the entire match-logic module) is mapped, and the positions' nature
(normalized 0..1, 11 active starters, quantized ×100) is VERIFIED.

## 4f. vtable archaeology attempt — virtual dispatch CONFIRMED

Pursued the vtable directly. `procedure_callers(FUN_00498ac0)` returns **no resolved callers** (2
unresolved), i.e. even the top match-tick/phase handler is invoked **indirectly**. `xrefs(0x498ac0)`
points at `0x498d8f` and `0x498e39`; both `resolve_containing_procedure` to **not_in_procedure**, and a
raw byte read (read_bytes @0x498d00) decodes them as `e8 2c fd ff ff` / `e8 82 fc ff ff` = **`call
FUN_00498ac0`** sitting inside two tiny functions near 0x498d80 / 0x498e00 that Ghidra never registered
(a disassembly gap). Those are vtable thunks of the match-state object.

This upgrades the mover finding from inference to **confirmation**: the CM2008 match engine is a
**vtable-driven virtual state machine** — handlers (and the per-player body mover) are reached through
indirect calls that the static resolved call graph does not connect, and some of the dispatch thunks are
not even registered as functions. The movement WRITER therefore cannot be reached by call-graph walking
by construction. Recovering it would require manually reconstructing the body object's vtable from its
constructor (itself reached only through the equally virtual match-setup path) or a fragile x87/SSE
float-store byte-signature search for the `[body+0x08]`/`[body+0x10]` write — both high-cost,
low-certainty, and neither adds a VERIFIED gameplay constant beyond what is already recovered. Runtime
dispatch resolution is excluded (no exe execution). This is the honest terminus of the static integrator
hunt; the consumer side, the position semantics, and the balance-tier atom are the solid, shippable
results.

## 5. Codeable atom

The balance-tier classifier (§3d) is fully constant-verified and can be ported 1:1 as a resolver atom
(strength multiplier with pivot 55 / slope 0.5, and the 0.30/0.65/1.30/3.00 tier buckets). The condition
term should be marked INFERRED until the x87 algebra is disambiguated. The position integration must stay
UNKNOWN (not fabricated) until §4's integration function is traced.
