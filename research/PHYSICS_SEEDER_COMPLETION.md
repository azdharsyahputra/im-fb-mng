# Physics-Block Seeder — Completion (Agent B)

Static only. No game run, no DRM bypass. Builds on `PHYSICS_AND_WAYPOINT_ANALYSIS.md` (Agent 3) and
`SIM_BLOCK_WRITER.md`. New evidence here: the **normalizer SCALE constant is recovered** by reading the
retail image at the mapped file offset, plus source-struct / normalizer-body / SeedProvider confirmation.

Evidence vocabulary: NOT_INVESTIGATED / LOCATED_NOT_UNDERSTOOD / DECOMPILED / STRUCTURALLY_UNDERSTOOD /
BEHAVIORALLY_VALIDATED / GENUINELY_UNAVAILABLE.

Image used (read-only): `…/scratchpad/iso-extract/Setup/Files/CM2008.exe` (13,580,176 bytes, 2007-10-15).
Flat map `file_off = VA − 0x400000` (VERIFIED in SIM_BLOCK_WRITER §0).

---

## 1. NORMALIZER SCALE @ 0xb73954 = **1/255** — RESOLVED

**Value: `0.00392156886` = 1.0f/255.0f.  Classification: BEHAVIORALLY_VALIDATED (bytes read from retail .rdata).**

- **Section check (anti-SecuROM):** the retail exe IS SecuROM-packed — its PE section table carries the
  giveaway names `ars` / `artem` / `celare` (SecuROM's motto *"ars celare artem"*) plus a `.securom`
  section at **VA 0x1565000 / rawptr 0xc82000**. **But offset 0x773954 lands in `.rdata`**
  (VA 0xb11000–0xc07800, rawptr 0x711000–0x808000, characteristics 0x40000040 = read / initialized data),
  which is **not** an encrypted SecuROM section and is far from `.securom`. SecuROM encrypts parts of
  `.text`; `.rdata` constants are intact — exactly as expected. So this is a genuine owner-byte read, not
  junk. **Reported as VERIFIED, not invented.**
- Read 4 bytes at file offset `0x773954` (= VA 0xb73954 − 0x400000): `81 80 80 3b` (LE).
- As float32 = `0x3b808081` = **0.003921568859** . Exact `1/255 = 0.003921568627`; agreement to 7 sig figs
  (nearest float to 1/255 is `0x3b808081` — i.e. this *is* the round-trip of 1/255). Not 1/256, not 1/100.
- **Not SecuROM junk.** The offset sits in an intact `.rdata` constant/string table. Hex context:

  ```
  0x773948  7d 00 00 00            = 125 (int)
  0x77394c  0a d7 23 3c            = 0.01f
  0x773950  00 00 c8 42            = 100.0f          (_DAT_00b73950)
  0x773954  81 80 80 3b            = 1/255f          (_DAT_00b73954  <-- SCALE)
  0x773958  25 49 92 3d            = 0.07142857f (=1/14) 
  0x77395c  "Tendencies not setup, using mid-range"   (ASCII)
  nearby:   "...Personality", "HomePersonality", "MatchSeed", "Tendencia"
  ```

- **Corroboration:** the constant is immediately followed by the literal string
  *"Tendencies not setup, using mid-range"* and surrounded by `…Personality` / `MatchSeed` / `Tendencia`
  strings — i.e. this is the match-seed / personality-tendency constant block. That both confirms the
  region is genuine (readable ASCII, clean floats) and reframes the +0xC0..+0xD4 fields as
  **behavioral/tendency coefficients** (see §2), not pitch coordinates.
- Agent 3's prior guess (1/100 or 1/200) is **superseded** — 100.0f is the *adjacent* constant at
  0xb73950; the actual normalizer scale is 1/255 (attribute byte 0..254 → [0, 0.9961]; 0xFF → 1.0).

Normalizer law (now fully numeric):
```
FUN_005d6d20(attrByte):  return (attrByte == 0xFF) ? 1.0f : attrByte * (1.0f/255.0f)
```

---

## 2. attr[0x26..0x2b] / attr[0x49] → named attribute

**Byte indices: VERIFIED. Index→NAME mapping: GENUINELY_UNAVAILABLE (statically undetermined).**

- The six physics floats are seeded (Branch A, attr[0x49] normalizes to 0) from the six **consecutive**
  attribute bytes `param_2[0x26..0x2b]`; `+0xCC` is `attr[0x29]` in both branches; `attr[0x49]` is the
  GK/outfield branch selector. These indices are read directly from the decompile — **VERIFIED**.
- To name them we need arg1's byte-layout order. The only recovered orderings are:
  (a) the on-disk 77×u16 `PlayerProperties.dat` record — **column order UNKNOWN** (Agent 2, no code anchor);
  (b) the `match_player` debug CSV-exporter enum (name order VERIFIED), but that exporter uses **two
  separate stack views** (outfield Block A idx 0..60, GK Block B idx 61..104), not arg1's buffer.
- **Alignment test (falsifies the naive enum map):** FUN_009f0090 seeds the *GK-attribute family*
  `block[0x7d..0x8a]` from `param_2[0x63..0x70]` (bytes 99..112). In the debug exporter the GK-specific
  names (eHandling/eKicking/eOneOnOnes/eShotStopping/eThrowing) sit at ~94..98, while 99..112 are
  eAgility/eLeftFoot/eRightFoot/… — **not** GK. So **arg1's order ≠ the debug-exporter order**; the
  exporter cannot be used to name arg1 indices.
- Therefore a *verified* name for attr[0x26..0x2b]/attr[0x49] is **not obtainable from the located corpus**.
  If (unproven) arg1 were enum-aligned, 0x26..0x2b would fall in the personality band
  (Bravery/Confidence/Versatility/Adaptability/Ambition/LeftFoot) — consistent with the adjacent
  *"Tendencies…Personality"* strings and with these floats being **behavioral-tendency coefficients** —
  but the GK-range contradiction above means this is **INFERRED-WEAK, not evidence**. No name is asserted.

Net: the +0xC0..+0xD4 seeding is index-exact and scale-exact; the human-readable attribute names behind
those indices are an open item requiring the arg1 filler (see §3) to be decompiled or the buffer observed.

---

## 3. Source struct arg1 (ESI / attrSrc)

**STRUCTURALLY_UNDERSTOOD (indices verified; filler not decompiled).**

- Call site (SIM_BLOCK_WRITER §1, VERIFIED): per-entity geometry build `…0x9f39dc` sets `[edi+0x230]=1`,
  calls `FUN_005d6dd0`, then `0x9f3b44 push ebx(attrSrc); push edi(block); 0x9f3b46 call FUN_009f0090`.
  So **arg1 = EBX = a per-entity attribute byte buffer**; arg0 = EDI = the 1012-byte float physics block.
- `FUN_005d6dd0` (56 bytes, `__thiscall(Player*)`, table-driven loop; Agent 2 VERIFIED) is the
  **105-getter harvester** that fills a per-player attribute buffer immediately before the seeder runs —
  the most likely producer of arg1. Its body does not spell out the write order, so the exact
  index→getter correspondence stays **INFERRED**.
- Raw-vector copy **CONFIRMED** in `CM2008.exe_big.c` (lines 54696-54705): the seeder copies **10 raw
  float32s** `block[0x25..0x2e]` (= **+0x94..+0xB8**) directly from `*(float*)(param_2 + 0x74 .. +0x98)`
  — no normalization. These are the genuine raw vector/geometry fields (Branch B instead writes the
  `45.0/0.5/30.0` constants here). Note arg1 is read as both **bytes** (attr indices ≤ ~0x70, via
  FUN_005d6d20) and **floats** (the +0x74..+0x98 vector) — it is a mixed record, min size ≥ 0x9C.

---

## 4. FUN_005d6d20 body

**49-byte size VERIFIED (functions.csv, 4 dumps). Body: LOCATED_NOT_UNDERSTOOD here; LAW BEHAVIORALLY_VALIDATED (SIM_BLOCK_WRITER, Unicorn).**

- Listed at `005d6d20,FUN_005d6d20,49` in match_player/match_finance/resolver/resolver2 functions.csv.
- **Its decompiled body is absent from every committed dump** (grep of docs/ returns only call-sites).
- The 49-byte shape fits the validated law exactly: compare `al,0xFF` → branch → (`fld1`) or
  (`movzx`/`cvtsi2ss` or `fild` ; `fmul [0xb73954]`) → `ret`. The behavior was emulation-confirmed in
  SIM_BLOCK_WRITER §1; re-export via `reexport.sh` was **not run** (not needed — SCALE is now a direct
  byte read and the law is already validated). If a disassembly listing is later wanted, that is the
  single-access path; it would not change the result.

---

## 6. The whole write set + ResolverSettings cross-reference (NEW — the field identifier)

**STRUCTURALLY_UNDERSTOOD / BEHAVIORALLY_VALIDATED.** Dumping *every* float store in FUN_009f0090 (not
just +0xC0..+0xD4) and aligning it against the **named** MatchResolver movement constants settles what
block this is.

- FUN_009f0090 writes **125 distinct float slots**, contiguous **+0x2c … +0x228**.
- The **named ResolverSettings movement constants** (harvested from `formula.xml`, `FORMULA_CONSUMER_MAP.md`
  lines 42-142) are read back by getter **FUN_009f4bd0** at `esi+0xf8 … esi+0x1b0` — **every one of those
  offsets is inside FUN_009f0090's write set.** Same struct. FUN_009f0090 = the **per-entity seeder**;
  FUN_009f4bd0 = the per-field **named getter**. This NAMES ~24 of the seeded physics fields:

  | +off | ResolverSettings name | seeder writes |
  |---|---|---|
  | +0xf8 | MaxSpeedToTurningCircleRadius | `0.1` literal |
  | +0xfc | MinBaseAcceleration | `0.1` |
  | +0x100 | MaxBaseAcceleration | `0.1` |
  | +0x110 | MaxWalkingSpeed | `0.1` |
  | +0x118 | MaxJoggingSpeed | norm(attr) |
  | +0x120 | MaxRunningSpeed | norm(attr) |
  | **+0x128** | **MaxSprintingSpeed** | **norm(attr)** |
  | +0x12c | StandingAccelerationMultiplier | norm(attr) |
  | +0x130 | WalkingAccelerationMultiplier | norm(attr) → tail affine ×−20+55 |
  | +0x134 | JoggingAccelerationMultiplier | norm(attr) → tail affine ×−20+55 |
  | +0x138 | SprintingAccelerationMultiplier | norm(attr) |
  | +0x13c | RunningAccelerationMultiplier | norm(attr) |
  | +0x154 | BackingAccelerationMultiplier | norm(attr) |
  | +0x160 | MaxBackingSpeed | norm(attr) |
  | +0x164/0x170 | SidlingLeft Accel / MaxSpeed | norm(attr) |
  | +0x174/0x180 | SidlingRight Accel / MaxSpeed | norm(attr) |
  | +0x184/0x190 | DivingLeft Accel / MaxSpeed | norm(attr) |
  | +0x194/0x1a0 | DivingRight Accel / MaxSpeed | norm(attr) |
  | +0x1a4/0x1b0 | Sliding Accel / MaxSpeed | norm(attr) |

- **Consequence for §2:** the named movement band is **+0xf8..+0x1b0**. The six fields **+0xC0..+0xD4 lie
  BELOW it** (no named getter reads < +0xf8 in the whole harvester table), so they are **not** the
  MaxSprinting/accel constants — they are an earlier **behavioral/default-state** region of the same
  struct (consistent with the "Tendencies…Personality" strings beside the SCALE const). The
  cross-reference thus *names 24 fields* but confirms **+0xC0..+0xD4 remain unnamed** — strengthening the
  §2 conclusion rather than overturning it.
- **Cross-layer note (not over-claimed):** `formula.xml`/FUN_009f4bd0 defaults are large (MaxSprintingSpeed
  **9**, MaxBaseAcceleration **3**); the seeder writes small `0.1` literals or normalized **[0,1]**
  values. So the per-entity seeder emits **normalized multipliers/seeds** that a later resolver stage
  combines with the global ResolverSettings defaults — two layers, not a contradiction. Exact combination
  rule is out of scope here (UNKNOWN, resolver side).

---

## 5. Go `geometry.SeedProvider` requirements

To faithfully reproduce FUN_009f0090's +0xC0..+0xD4 (and the raw-vector +0x94..+0xB8) seeding, the
provider must supply, **per entity**, the raw attribute bytes of the arg1 buffer, and the reconstruction
must apply:

**Normalizer (exact):** `norm(b) = 1.0 if b == 0xFF else float32(b) * (1.0/255.0)`.

**+0xC0..+0xD4 seed (Branch A, when `norm(attr[0x49]) == 0`):**
| dest float (byte) | source byte |
|---|---|
| +0xC0 | norm(attr[0x26]) |
| +0xC4 | norm(attr[0x27]) |
| +0xC8 | norm(attr[0x28]) |
| +0xCC | norm(attr[0x29])  *(both branches)* |
| +0xD0 | norm(attr[0x2a]) |
| +0xD4 | norm(attr[0x2b]) |

**Branch B (`norm(attr[0x49]) != 0`):** +0xC0←norm(attr[0x27]); +0xC4,+0xC8,+0xD0,+0xD4 all ← norm(attr[0x27]);
+0xCC ← norm(attr[0x29]).  (Collapse to the single attr[0x27] value; +0xCC unchanged.)

**Raw vector +0x94..+0xB8 (Branch A):** copy 10 float32 verbatim from source offsets +0x74,+0x78,…,+0x98
(no normalization). **Branch B:** write the literals `45,45,0.5,0.5,45,45,30,30,30,30`.

**Tail affine (always):** `block[0x4c] = block[0x4c]*-20.0 + 55.0; block[0x4d] = block[0x4d]*-20.0 + 55.0`.

**SeedProvider contract (minimum):** for each entity, expose the **raw attribute byte array** (indices at
least 0x02..0x70 for the byte reads, plus the 0x74..0x98 float window) in the **native arg1 order**, and
a flag/byte at index **0x49** that selects outfield(0)/alt branch. Order note: the index→semantic-name
mapping is not statically verified (§2), so the provider must preserve the **original CM2008 attribute
byte order** (as produced by FUN_005d6dd0) rather than any logical/display order. Scale = **1/255**,
0xFF-sentinel → 1.0. Those two numeric facts are fully pinned down; only the human labels of indices
0x26..0x2b/0x49 remain open.

---

## Classifications summary
| Finding | Classification |
|---|---|
| SCALE @0xb73954 = 1/255 (0x3b808081), confirmed in .rdata (not .securom) | BEHAVIORALLY_VALIDATED (retail .rdata byte read) |
| Seeder writes +0x2c..+0x228 (125 slots); band +0xf8..+0x1b0 = named ResolverSettings (MaxSprintingSpeed@+0x128 …) | STRUCTURALLY_UNDERSTOOD / cross-referenced |
| Normalizer law (0xFF→1.0 else b*1/255) | BEHAVIORALLY_VALIDATED |
| +0xC0..+0xD4 ← attr[0x26..0x2b], flag attr[0x49] (indices) | BEHAVIORALLY_VALIDATED |
| attr index → human name (0x26..0x2b/0x49) | GENUINELY_UNAVAILABLE (arg1 order unanchored) |
| arg1 = per-entity attr buffer from FUN_005d6dd0; +0x74..+0x98 raw-float copy | STRUCTURALLY_UNDERSTOOD / copy VERIFIED |
| FUN_005d6d20 body (49 bytes) | LOCATED_NOT_UNDERSTOOD (law validated elsewhere) |
| SeedProvider spec | DECOMPILED/STRUCTURALLY_UNDERSTOOD (complete for faithful seeding) |
