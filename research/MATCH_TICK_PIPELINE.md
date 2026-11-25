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

## 5. Codeable atom

The balance-tier classifier (§3d) is fully constant-verified and can be ported 1:1 as a resolver atom
(strength multiplier with pivot 55 / slope 0.5, and the 0.30/0.65/1.30/3.00 tier buckets). The condition
term should be marked INFERRED until the x87 algebra is disambiguated. The position integration must stay
UNKNOWN (not fabricated) until §4's integration function is traced.
