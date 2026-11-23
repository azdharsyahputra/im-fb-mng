# Per-entity geometry block — the WRITER of +0xc0/+0xd0 (reverse byte-search)

Static only. `/Volumes/CM2008/Crack JPN/cm2008.exe`, flat map `file_off = VA-0x400000`
(VERIFIED). capstone linear-decode + byte-search; Unicorn confirmation via
`tools/emulate_fn.py`. No game run, no Ghidra, no commits. Reverse method mirrors the
eGameState writer recovery (we knew the field offset, byte-searched the stores to it).

## 0. Entropy (debunks "protected")
64 KB windows across the image: code regions **H ≈ 6.0–6.6 bits/byte** (normal x86),
zero-padding H≈0, ONE embedded blob at VA~0x1b9a000 H=8.0 (a compressed data resource,
not code). **Not packed / not encrypted** — the sim writer was simply never dumped before.

## 1. HEADLINE — writer of the +0xc0/+0xd0 geometry fields = FUN_009f0090  (VERIFIED)
The per-entity ~1012-byte geometry block is **FLOAT** at +0xc0..+0xd4, and it is produced by

```
FUN_009f0090(block /*arg0 -> edi*/, attrSrc /*arg1 -> esi*/)      ; VA 0x009f0090
```

This is the per-entity **physics-block seeder**. Disassembly + emulation show it writes a
contiguous float record **+0x00 … +0x228** into arg0, INCLUDING the position/geometry
fields **+0xc0, +0xc4, +0xc8, +0xcc, +0xd0, +0xd4**, while reading **player-attribute
bytes +0x00..+0x70** from arg1.

Store bytes (VERIFIED):
```
0x9f00fc  fst  [edi+0xc0]      0x9f0108  fst  [edi+0xc8]      0x9f0114  fst [edi+0xd4]
0x9f0102  fst  [edi+0xc4]      0x9f010e  fst  [edi+0xd0]
(second branch, jnp 0x9f03a7:  0x9f0604 fstp[edi+0xc0] … 0x9f064c fstp[edi+0xd4])
```
Emulation (Unicorn, `emu_run2.py`): seeding arg0/arg1 as scratch structs, the block range
logged WRITES at every dword `0x0..0x228` incl. `0xc0 0xc4 0xc8 0xcc 0xd0 0xd4`, and the
attr range logged READS of bytes `0x00..0x70`. **VERIFIED(emulated).**

### Values written = player attributes → float, via FUN_005d6d20  (VERIFIED)
```
FUN_005d6d20(attrByte):  attrByte==0xff ? 1.0 : (float)attrByte * [0xb73954]   ; ret 4
```
A pure attribute-byte→float normalizer (0xff = "unknown" ⇒ 1.0; else byte × scale const
`_DAT_00b73954`). FUN_009f0090 calls it ~60× on attr indices 0x02,0x0d,0x0e,0x11,0x12,
0x17,0x18,0x27,0x49,0x63..0x70, storing results across +0x08..+0x228 (e.g. +0xd8,+0xdc,
+0xe0,+0xe4,+0xe8 ← attrs 0x02/0x0d/0x0e/0x11/0x12). Physics tuning constants written into
+0x94..+0xb8 / +0x1ec/+0x1f0: `fld [0xba170c]`, `fld [0xbbb51c]` (the SAME scalar the
contest math uses), `fld [0xb63280]`, `fld [0xb5b758]`, `fld [0xb5bb9c]`, `fld [0xbb1898]`.
So the attribute-derived limits (pace/accel ≈ "MaxSprintingSpeed" family) live at +0xd8..
+0xe8 and the dynamic state (position/velocity vectors) at +0xc0..+0xd4, init-seeded here.

### Call path (VERIFIED by E8/E9 xref scan)
```
<per-entity geometry build> 0x9f39dc..   ; sets [edi+0x230]=1, calls FUN_005d6dd0,
   0x9f3b44 push ebx(attrSrc); push edi(block); 0x9f3b46 call FUN_009f0090
also reached via thin forwarder 0x9ef040 (009ef accessor family).
Block later copied to a stack scratch by FUN_009b54b0 (mov ecx,0xfd; rep movsd = 253 dw),
called from 0x9ba6f3 (the view/draw path, FUN_009ba5e0 cluster).
```

### Other writers of the same fields (VERIFIED, classified)
| VA | Writes | Role |
|---|---|---|
| **0x009f0090** | +0xc0..+0xd4 (float) from attrs | **block seeder (primary writer)** |
| 0x009f4dd0 | +0xc0,+0xc4 (fstp) | loads physics/settings from global singleton `[0xc68428]` via getters 0x821xxx/0x822xxx (ResolverSettings-style config load) |
| 0x009f53cc / 0x009f53ec | +0xc8 / +0xd0,+0xd4 (fstp) | deserialize block fields from `[0xc68428]` getters |
| 0x868556, 0x90b24a, 0x94ea36, 0x959137, 0x9733ec, 0x949240 … | +0xc0..+0xd4 (int `mov [edi]→[esi]`) | **24-byte (6-dword) struct copy / clone / snapshot** of the geometry record (not computation) |

## 2. The continuous INTEGRATOR (pos += vel·dt, clamp) — NOT in the located corpus
Reverse search over the WHOLE code region (0x400000–0xb00000):
- 92 write-sites to `[reg+0xc0]`, 96 to `[reg+0xd0]` (GPR-based, stack excluded).
- For EACH, checked for a READ of the **same (disp,base)** before the store — the in-place
  `new = old + delta` idiom. **ZERO hits on both axes.**
- No sim-region function contains both `fld [reg+0xc0]` and `fstp [reg+0xc0]` (same base).
  Coord-reading funcs 0x00a06e40 (fld +0xc0..+0xd4 → serialize) and 0x00a00eba (fld +0xc8,
  fadd +0xd0 … vector-difference kinematics) only READ.

⇒ The per-frame position/velocity integrator does **not** write +0xc0/+0xd0 in place
anywhere statically reachable. It is **thread-private**, consistent with
`MOVEMENT_GEOMETRY_BUFFER.md`. Exact call path to it:
`match worker thread proc (not located) → processes the stack copy produced by
FUN_009b54b0 ← entity+0x134 under [pool+0x3004c] (DAT_00c784a8)`. The thread proc
(`_beginthreadex`/`CreateThread` site in the match module) is the one remaining unlocated
link; the integrator step/clamp constants are therefore **UNKNOWN(located)**.

## 3. Model/view DUALITY (resolves the integer-vs-float question)
Two distinct objects both use +0xc0/+0xd0:
- **SIM physics block** (this doc): **float** +0xc0..+0xd4, 1012-byte, seeded by 0x9f0090.
- **VIEW/projection object**: **integer** +0xc0/+0xd0 read by `fild [esi+0xc0]/[esi+0xd0]`
  in FUN_009a89b0 / FUN_009aa250 and projected ×(1/112.5 @0xb5e8c8) to screen.
The engine keeps a float physics model and an integer-fixed-point view coordinate; the
earlier "integer +0xc0/+0xd0 ×1/112.5" finding is the VIEW side, not the sim block.

## 4. The 4-waypoint polyline (pathfinding target points)
- Pending target **refs** = u16 at **+0xb4/+0xb6/+0xb8/+0xba** (0xffff = empty).
- **FUN_009f0c40** (reached via forwarder jmp @0x9ef073) reads those 4 refs; for each
  non-0xffff it does `lea ecx,[esi+0xd0]; push <ref>; call FUN_009fd680`, then calls
  FUN_009fd690, then **resets** the 4 refs to 0xffff and clears +0xd8/+0xcc/+0xce.
- **FUN_009fd680** = forwarder `mov ecx,[ecx+4]; jmp 0x65fe90`.
  **FUN_009fd690** (real body) loops **exactly 4×**, reading `(u16,u16)` pairs at
  **+0x12 stride 8** (`lea esi,[edi+0x12]; movzx word[esi-2]; movzx word[esi]; add esi,8`)
  and pushing each into the point container `[edi+4]` via **FUN_0065fe90**. VERIFIED.
- The upstream **filler** that SETS +0xb4..+0xba (the pathfinding target selection) is one
  of the movement-action writers of +0xb4; it was **not uniquely isolated** (the +0xb4
  write set is large and overlaps unrelated dword fields). **INFERRED / partial UNKNOWN.**

## 5. Classification
- `+0xc0..+0xd4` of the 1012-byte block are FLOAT geometry fields — **VERIFIED** (bytes+emu)
- **FUN_009f0090** writes +0xc0/+0xd0 (+whole +0x00..+0x228 block) from player attributes
  via FUN_005d6d20 — **VERIFIED(emulated)**  ← the requested writer
- Attribute→float law `attr*[0xb73954]` (0xff→1.0) — **VERIFIED**
- Config/settings writers 0x9f4dd0 / 0x9f53ec from `[0xc68428]` — **VERIFIED**
- Call path seeder←0x9f3b46 / 0x9ef040; block copy FUN_009b54b0←0x9ba6f3 — **VERIFIED**
- Continuous integrator (per-frame pos+=vel, max-speed clamp) writing +0xc0/+0xd0 —
  **UNKNOWN(located) / INFERRED(thread-private)**; zero in-place read-modify sites in the
  entire image; reached only through the unlocated match worker-thread proc.
- Waypoint consume/flush chain 0x9f0c40 → 0x9fd680/0x9fd690 → 0x65fe90, refs at
  +0xb4..+0xba, pairs at +0x12 stride 8 — **VERIFIED**; the ref-FILLER — **INFERRED/UNKNOWN**
