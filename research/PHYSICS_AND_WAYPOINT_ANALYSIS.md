# Physics-Block Init & Waypoint Generation — Static Reconstruction (Agent 3)

Static only. Evidence base: `docs/ghidra_dumps/big/CM2008.exe_big.c` (Ghidra decompile of
`CM2008.exe`), cross-checked against `research/SIM_BLOCK_WRITER.md` (capstone + Unicorn byte-level),
`research/GEOMETRY_WRITER_INVESTIGATION.md`, `research/MOVEMENT_GEOMETRY_BUFFER.md`. No game run, no
DRM bypass, no re-export invoked. All line numbers are into `CM2008.exe_big.c`.

Classification vocabulary (per task): NOT_INVESTIGATED / LOCATED_NOT_UNDERSTOOD / DECOMPILED /
STRUCTURALLY_UNDERSTOOD / BEHAVIORALLY_VALIDATED / GENUINELY_UNAVAILABLE.

---

## 1. FUN_009f0090 — per-entity physics-block seeder  (VA 0x009f0090)

Signature in dump (line 54585): `void FUN_009f0090(undefined4 param_1, undefined1 *param_2)`.
- **param_2 (arg1 / ESI)** = player-attribute source struct; indexed per *byte* `param_2[k]`, each byte
  passed through the normalizer `FUN_005d6d20` (see §2).
- **Destination block (arg0 / EDI)** appears in the decompile as `float *unaff_retaddr` (the EDI block
  pointer; a decompiler calling-convention artifact). It is a contiguous **FLOAT** record; the
  decompile indexes it by *float index* `block[i]`, so **byte offset = i * 4**.
- 1 callee (FUN_005d6d20), 1 caller. Called from the per-entity geometry build at `0x9f3b46`
  (`push ebx(attrSrc); push edi(block)`), also via forwarder `0x9ef040` (SIM_BLOCK_WRITER §1, VERIFIED).

**Top-level control flow (VERIFIED from body 54591-54961):**
- Line 54591-54593: `fVar1 = FUN_005d6d20(param_2[0x49]); block[99] = fVar1;` then branch on
  `if ((float10)0 == fVar1)`.
  - **Branch A** (attr[0x49] normalizes to 0.0), lines 54594-54768: seeds the GK-attribute family
    `block[0x7d..0x8a]` from `param_2[0x63..0x70]`, copies a raw-float vector block
    `block[0x25..0x2e] ← *(float*)(param_2+0x74 .. +0x98)` (lines 54696-54705, direct float copy, NOT
    normalized), and seeds `block[0x2f..0x35]` from attribute bytes.
  - **Branch B** (attr[0x49] != 0), lines 54769-54885: sets `block[0x25..0x2e]` to literal constants
    (`45.0, 45.0, 0.5, 0.5, 45.0, 45.0, 30.0, 30.0, 30.0, 30.0`, lines 54828-54837) and collapses
    `block[0x2f..0x35]` (minus 0x33) to a single normalized value.
- **Common tail** (always runs), lines 54886-54961: seeds many more fields; sets `block[0x33]`
  (= +0xCC) from `attr[0x29]`; ends with two affine transforms (lines 54959-54960):
  `block[0x4c] = block[0x4c]*-20.0 + 55.0; block[0x4d] = block[0x4d]*-20.0 + 55.0;`.

Classification: **DECOMPILED + STRUCTURALLY_UNDERSTOOD** (full body present; write offsets and attr
sources read directly; the field *semantics* are inferred). Byte-level stores and the write set
`+0x00..+0x228` were independently **BEHAVIORALLY_VALIDATED** by Unicorn in SIM_BLOCK_WRITER.md §1.

---

## 2. FUN_005d6d20 — attribute-byte → float normalizer, and the scale constant 0xb73954

**Law (VERIFIED by capstone + Unicorn in SIM_BLOCK_WRITER.md §1, line 37):**
```
FUN_005d6d20(attrByte):  return (attrByte == 0xff) ? 1.0f : (float)attrByte * [0xb73954]
```
`0xff` is the "unknown/sentinel" attribute value → 1.0; every other byte is scaled by the global
`_DAT_00b73954`. Return size 4 bytes (float). FUN_009f0090 calls it ~60×.

**Numeric value of the scale at 0xb73954 = UNKNOWN (LOCATED_NOT_UNDERSTOOD).**
- `FUN_005d6d20` is listed in `functions.csv` (49 bytes) but **its body is not decompiled in any dump**
  (`docs/ghidra_dumps/*/functions.csv` list it; no `decompiled.c`/`big.c` contains its body).
- The data address `0xb73954` **does not appear in any dump or data file in the repo** (grep of
  `docs/` and `research/` returns only the formula citation in SIM_BLOCK_WRITER.md, not a value).
- The constant lives in the `.data`/`.rdata` of the image, which is not in the seeded decompile.
  **I did not invent a value.** It is very likely `1.0/100.0` or `1.0/200.0` (0..100 or 0..200
  attribute scale → 0..1 unit) — stated as **INFERRED / UNCONFIRMED**, not evidence.
- Recovery path (not run here; single-access re-export): dump 4 bytes at file offset
  `0xb73954 - 0x400000 = 0x773954` (flat VA→file map is VERIFIED per SIM_BLOCK_WRITER §0), or
  re-export FUN_005d6d20's body via `research/ghidra_reexport/reexport.sh`.

---

## 3. Field map +0xC0 … +0xD4  (float indices 0x30..0x35)

Record is `float*`; `+0xC0 = block[0x30]`, `+0xC4 = block[0x31]`, `+0xC8 = block[0x32]`,
`+0xCC = block[0x33]`, `+0xD0 = block[0x34]`, `+0xD4 = block[0x35]`.

| Byte off | float idx | Branch A source (attr[0x49]==0) | Branch B source (attr[0x49]!=0) | line(s) | source status |
|---|---|---|---|---|---|
| +0xC0 | 0x30 | `norm(attr[0x26])` | `fVar1 = norm(attr[0x27])` | 54668-69 / 54786 | VERIFIED |
| +0xC4 | 0x31 | `norm(attr[0x27])` | `norm(attr[0x27])` | 54670-71 / 54787 | VERIFIED |
| +0xC8 | 0x32 | `norm(attr[0x28])` | `norm(attr[0x27])` | 54672-73 / 54788 | VERIFIED |
| +0xCC | 0x33 | `norm(attr[0x29])` (common tail) | `norm(attr[0x29])` (common tail) | 54908-09 | VERIFIED |
| +0xD0 | 0x34 | `norm(attr[0x2a])` | `norm(attr[0x27])` | 54674-75 / 54789 | VERIFIED |
| +0xD4 | 0x35 | `norm(attr[0x2b])` | `norm(attr[0x27])` | 54676-77 / 54790 | VERIFIED |

**What is VERIFIED:** the six write offsets `+0xC0..+0xD4`; that all six are FLOAT; and that every one
is **seeded from a player-attribute byte via FUN_005d6d20** (Branch A maps the six consecutive attrs
`0x26,0x27,0x28,0x29,0x2a,0x2b` → the six consecutive floats; Branch B collapses five of them to a
single value). `+0xCC` is attr[0x29] in *both* branches.

**What is INFERRED (and weakly supported):** the "position X/Y cur/prev" label applied to these fields
in `research/DETAILED_MATCH_ENGINE_FUNCTION_MAP.md` (line 147: `+0xc0/+0xc4`=X cur/prev,
`+0xd0/+0xd4`=Y cur/prev). The decompile shows these are **attribute-derived scalars at init**, not raw
coordinates — a live position seeded from `attributeByte × scale` is physically implausible. The most
defensible reading: `+0xC0..+0xD4` are **per-axis physics/kinematic coefficients or default state**
that the (thread-private) per-frame integrator later overwrites in place with real position/velocity.
The *actual* raw-float geometry/vector fields in this record are `block[0x25..0x2e]` = **+0x94..+0xB8**
(copied raw from `attrSrc+0x74..+0x98` in Branch A, or set to the `45.0/30.0/0.5` constants in
Branch B) — these are the more likely position/velocity vectors. **The x-vs-y assignment for any of
these is INFERRED / not determinable statically.**

Classification of the +0xC0..+0xD4 map: write offsets + attr sources **BEHAVIORALLY_VALIDATED**
(bytes + Unicorn); semantic labels **LOCATED_NOT_UNDERSTOOD**.

---

## 4. FUN_00a1fdf0 — NOT a waypoint/target-picker (correction of a prior inference)

**Direct read of the body (lines 40656-41330) does not support the "waypoint/target-picker" label.**
What the function actually does:
- **Roster iteration** (lines 40778-40793, 41320-41324): walks a linked list of **0x3c-byte roster
  entries** — `next = base + (entry[0] & 0xfff) * 0x3c` — gated by a validity bit
  `(*(uint*)(entry+0x3a) >> 0x14) & 1` (byte +0x74). `+0x3c` = entry stride; `+0x3a` (ushort index) =
  the flag word. These are **roster-list mechanics, not pitch coordinates**. VERIFIED.
- For each valid player it **registers the full attribute-NAME dictionary** as string literals via
  `FUN_00413550` (lines 40826-41002): `Morale, Condition, TransferValue, BaseValue,
  InternationalScoredGoals, Acceleration, Adaptability, Aggression, Agility, Ambition, Anticipation,
  Balance, Bravery, Concentration, Confidence, Consistency, Creativity, DecisionMaking, Determination,
  FitnessStat, Flair, …, Positioning, Pressure, …, WorkRate, AttackingMidfielder, …, Winger` — i.e.
  **every** attribute/position name, then serializes each via `FUN_005f2b40`/`FUN_00584650`.
- The attrs named in the task (Acceleration/Agility/Anticipation/DecisionMaking/Positioning/WorkRate)
  appear here as **field names in a complete dictionary**, not as a selectively-read movement subset —
  which is exactly why *all* ~60 names are present.

**Conclusion: FUN_00a1fdf0 is a per-roster player-attribute SERIALIZER / exporter** (reflection-style
dump), **not** a movement picker. Classification: **STRUCTURALLY_UNDERSTOOD**.

### The 0x14 / 0x44-stride "candidate list" lives in FUN_007fa450, not FUN_00a1fdf0
FUN_00a1fdf0 ends at line 41330. The `i*0x14`-stride and `0x44`-section writers the task describes are
in the **next function, FUN_007fa450 @ 0x007fa450** (begins line 41727). Evidence (lines 41740-41816):
```
uVar3 = FUN_0065b090("GCTransferModule",0);          // module tag
puVar4 = FUN_00a0d5b0(0x16, 0x4df0a5fc);             // factory(typeTag=0x16, 32-bit hash)
puVar5 = FUN_00a0d5b0(0x16, 0x71349204);
*(obj + 0xc + i*0x14) = puVar5[0];  *(obj+0x10+i*0x14) = puVar5[1];   // "coord A/B"
*(obj +       i*0x14) = puVar4[0];  *(obj+ 4 +i*0x14) = puVar4[1];
*(obj + 0x1c + count*0x14) = 0;     count++ ;        // flag@+0x1c, running count@obj+8
... repeated at section bases +0x48/+0x50, +0x90/+0x98 (the 0x44-stride records)
```
- **Where the "coordinates" originate:** they are **`FUN_00a0d5b0(0x16, <hash>)` return values** —
  `(typeTag, 32-bit constant)` pairs, where the constants (`0x4df0a5fc, 0x71349204, 0x1a2a97a6, …`)
  are **string/field hashes**, under the module name **"GCTransferModule"**. These are
  **serialization / reflection field descriptors**, **not world/pitch waypoint coordinates.**
  `FUN_00a0d5b0` is an object factory (called pervasively, e.g. lines 4447/7405); its body is not in
  any dump → **LOCATED_NOT_UNDERSTOOD**, but it is clearly a type-factory, not a geometry source.
- **Stride facts (VERIFIED):** candidate entries are **0x14 (20-byte)** stride {u32@+0x0, u32@+0x4,
  …, flag@+0x1c}; section/record stride is **0x44 (68-byte)** with per-section count words at
  `+0x08`, `+0x50`, `+0x98` and value/active markers at `+0x4c`/`+0x48`.

**Therefore: the prior claim (`GEOMETRY_WRITER_INVESTIGATION.md §2`) that FUN_00a1fdf0 is a
movement target-picker emitting pitch coordinates is NOT supported by the decompiled evidence.** Two
adjacent serialization functions (an attribute serializer `a1fdf0` and a transfer-schema registrar
`7fa450`) were conflated, and 32-bit field hashes were read as coordinates. **No world coordinates are
produced by either function.**

### Where match waypoints actually come from (VERIFIED chain, per SIM_BLOCK_WRITER.md §4)
The real pathfinding waypoints are the **4-point polyline on the entity object**, unrelated to
`a1fdf0`:
- Pending target refs = **u16 at +0xb4/+0xb6/+0xb8/+0xba** (0xffff = empty).
- **FUN_009f0c40** (via forwarder `jmp @0x9ef073`) reads the 4 refs; per non-0xffff ref:
  `lea ecx,[esi+0xd0]; push <ref>; call FUN_009fd680; call FUN_009fd690`, then resets refs to 0xffff.
- **FUN_009fd690** loops **exactly 4×**, reading **(u16,u16) pairs at +0x12, stride 8**
  (`movzx word[esi-2]; movzx word[esi]; add esi,8`), pushing each into point container `[edi+4]` via
  `FUN_0065fe90`. VERIFIED. The ref **filler** (the true target-selection site) is **not isolated** —
  INFERRED / UNKNOWN. The per-frame integrator that consumes these is **thread-private** (match worker
  thread entry unlocated) — LOCATED_NOT_UNDERSTOOD (SIM_BLOCK_WRITER §2, MOVEMENT_GEOMETRY_BUFFER.md).

---

## 5. VERIFIED vs INFERRED summary

| Finding | Status |
|---|---|
| FUN_009f0090 full body, write set +0x00..+0x228, branch on `norm(attr[0x49])==0` | VERIFIED (DECOMPILED + Unicorn) |
| +0xC0..+0xD4 are FLOAT, seeded from attrs 0x26/0x27/0x28/0x29/0x2a/0x2b via FUN_005d6d20 | VERIFIED |
| +0xCC = norm(attr[0x29]) in both branches; Branch B collapses +0xC0/C8/D0/D4 to norm(attr[0x27]) | VERIFIED |
| Semantic "position X/Y cur/prev" for +0xC0..+0xD4 | INFERRED (weak; likely physics coeffs, real vectors at +0x94..+0xB8) |
| FUN_005d6d20 law `attr==0xff?1.0:attr*[0xb73954]` | VERIFIED (capstone+Unicorn) |
| Numeric value at 0xb73954 | UNKNOWN / LOCATED_NOT_UNDERSTOOD (not in any dump; not invented) |
| FUN_00a1fdf0 = roster player-attribute-name serializer | VERIFIED (STRUCTURALLY_UNDERSTOOD) |
| 0x14/0x44-stride candidate list belongs to FUN_007fa450 ("GCTransferModule"), coords = FUN_00a0d5b0(0x16,hash) field descriptors | VERIFIED — and NOT pitch coordinates |
| "FUN_00a1fdf0 is a movement waypoint/target-picker" (prior inference) | REFUTED by decompile |
| Real waypoints: 4-ref polyline +0xb4..+0xba → FUN_009f0c40 → FUN_009fd690 (pairs @+0x12 stride 8) | VERIFIED; ref-filler INFERRED/UNKNOWN |
| FUN_00a0d5b0 object factory body | LOCATED_NOT_UNDERSTOOD (not in dump) |

Doc path: `/Users/csadeveloper/cm/cm2008-reconstruction/research/PHYSICS_AND_WAYPOINT_ANALYSIS.md`
