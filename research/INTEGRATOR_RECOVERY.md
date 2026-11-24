# 0x9f5398 investigation — candidate per-frame integrator → it is NOT one

**Verdict:** `0x9f5398` is **one store instruction inside a run-once global-config-singleton
initializer** (`FUN_009f4dd0`, undefined in Ghidra). It is **NOT a per-frame integrator** — it
writes the geometry field `+0xc0` exactly once, from a config getter, with no read-modify-write.
**Classification: STRUCTURALLY_UNDERSTOOD.**

Static only. Method: capstone linear disassembly of the already-unpacked crack copy
`/Volumes/CM2008/Crack JPN/cm2008.exe`, flat map `file_off = VA - 0x400000` (same copy/method as
`SIM_BLOCK_WRITER.md`; mapping re-verified — VA `0x9f0090` decodes to the known seeder prologue
`push esi; mov esi,[esp+0xc]; …; call 0x5d6d20`). No game run, no DRM touched. The retail ISO exe
is SecuROM-packed; this crack copy is not (entropy per `SIM_BLOCK_WRITER.md` §0), so its bytes
disassemble cleanly. Ghidra was **not** run (project locked / single-access).

---

## 1. Is 0x9f5398 a defined function? — NO (undefined region)

`functions.csv` (full image, not just big.c's 150) has a gap:
```
009f4bd0  FUN_009f4bd0  size 503  -> ends 0x9f4dc7
         << GAP: 0x9f4dc7 .. 0x9f76a0  (0x28d9 bytes, NO defined function) >>
009f76a0  FUN_009f76a0  size 137
```
`0x9f5398` and `0x9f5404` fall inside this gap. No `009f5*`/`009f6*` entry exists in any index, and
**no defined function anywhere in the dumps references `009f5xxx`** (grep over big.c + all per-module
`decompiled.c` = 0 hits). This is why big.c never contained it — Ghidra never created the function.
(Same situation as the `0x6dd000` enter-handler cluster in `GEOMETRY_WRITER_INVESTIGATION.md` §4.)

## 2. Function boundaries (recovered by disassembly)

The undefined function containing `0x9f5398` is:
```
START  0x9f4dd0   push ecx                         (after int3 pad at 0x9f4dcf)
END    0x9f769e   ret        (int3 at 0x9f769f; next func 0x9f76a0)
size   ~0x28ce bytes
```
Designate it **FUN_009f4dd0**. (`SIM_BLOCK_WRITER.md` §1's separately byte-found sites `0x9f4dd0`,
`0x9f53cc`, `0x9f53ec` are all interior stores of THIS one function.)

## 3. Control flow — a run-once straight-line initializer

Prologue + **init guard** (runs the body only once):
```
009f4dd0  push ecx
009f4dd1  mov  ecx,[0xc68428]          ; global singleton ptr
009f4dd7  test ecx,ecx / je .. / call 0x480110   ; lazy-construct singleton if null
009f4dea  cmp  byte [eax+0x5dc], 0     ; <-- init flag
009f4df1  jne  0x9f769d                ; already initialized -> pop ecx; ret   (EARLY-OUT)
...
009f769e  ret
```
Body = **390 repetitions** of one idiom, writing consecutive singleton fields:
```
mov  eax,[0xc68428]      ; reload singleton ptr
test eax,eax / jne / call 0x480110   ; lazy-init guard (per field)
mov  esi,eax             ; ESI = the singleton object
call 0x841XXX            ; config getter -> value in st0 (or eax)
fstp dword [esi+0xNN]    ; store value into field 0xNN  (written ONCE)
```
No loop, no back-edge over the stores (one linear pass). Tail (0x9f767x) sets `[eax+0x5d8]` from
`[0xc9b084]`, checks `[0xc682f4]`, then `jmp 0x9f4730` (tail-call to a sibling init helper).

## 4. The geometry stores (+0xc0..+0xd4) — ESI = global singleton, not the physics block

Exact store sites and their value sources (each a distinct getter, +0x90 apart):
```
0x9f5398  fstp [esi+0xc0]  <- call 0x841e90
0x9f53b3  fstp [esi+0xc4]  <- call 0x841f80
0x9f53ce  fstp [esi+0xc8]  <- call 0x842070
0x9f53e9  fstp [esi+0xcc]  <- call 0x842160
0x9f5404  fstp [esi+0xd0]  <- call 0x842250
0x9f541f  fstp [esi+0xd4]  <- call 0x842340
```
Each field written **exactly once**. Automated scan of the whole body `0x9f4dd0..0x9f769e` for any
`fld/fadd/fmul/fild [esi+0xc0..0xd4]` (a read of these fields that would make it an in-place
`new = old + vel*dt` integrator): **NONE**. This matches `SIM_BLOCK_WRITER.md` §2's image-wide
"zero read-modify-write" result.

- **ESI points at the global singleton `[0xc68428]`** (reloaded from that global before every store,
  lazily constructed by `0x480110`). It is **NOT** the per-entity ~1012-byte physics block
  (that block is passed as arg0/EDI into the VERIFIED seeder `FUN_009f0090`). Same field offsets
  `+0xc0..+0xd4`, **different object**: a global settings/config singleton.
- The value getter `0x841e90` begins `mov eax,1; sub esp,0x4c; test byte [0xc9043c],al; jne ..;
  or [0xc9043c],al` = a **run-once static-initializer / config-default provider**, consistent with a
  settings bank (same shape as the `0x821xxx/0x822xxx` getters `SIM_BLOCK_WRITER.md` §1 noted for
  this region). So `+0xc0..+0xd4` here are being seeded with **config/default scalars**, not motion.

## 5. Callers / callees (static reachability — YES, but at match setup, not per tick)

Call graph into the function (byte-scan for E8/E9 rel32 + absolute-ptr):
```
0x77cbf0  (undefined match-setup routine)
   |  call 0x834420; fstp [0xc31b38]
   |  cmp [0xc31b34], -1 ; je ...          ; DAT_00c31b34 = SELECTED-FIXTURE global
   |                                        ; (see MATCH_CLOCK_AND_UPDATE_LOOP.md)
   |  0x77cc24  jmp 0x9efc70  ------.       (tail-call)
                                    v
0x9efc70  jmp 0x9f4dd0  (thin thunk, int3-padded)   <- ONLY reference to 0x9f4dd0
                                    v
0x9f4dd0  FUN_009f4dd0  (this initializer)
```
- Only **one** reference to `0x9f4dd0` in the whole image (the thunk `0x9efc70`); the thunk has one
  reference (the tail-jump at `0x77cc24`). No vtable/absolute-pointer occurrences of either.
- `0x77cbf0` is itself an undefined routine (between `FUN_0077cb80` end `0x77cbec` and `FUN_0077cc30`)
  in the **match module**, and it keys off the selected-fixture global `DAT_00c31b34` — i.e. this
  fires at **match/fixture setup**, not every simulation tick.
- Callees: `0x480110` (singleton lazy-ctor), the getter bank `0x841e90..0x842340+` (config
  accessors), plus the tail `jmp 0x9f4730`.

## 6. Per-frame integrator? — NO

Three independent reasons it is a **one-time writer**, not a per-tick integrator:
1. **Run-once guard** at entry (`[singleton+0x5dc]` init flag → early `ret`).
2. **No read-modify-write** of `+0xc0..+0xd4` (values come from getters, not from prior field + Δ).
3. **Reached at match setup** via the fixture-selection path, behind a thunk, with a single caller —
   not from the per-tick chain (`FUN_0077ee80 → cf20/cd40/c540/c660`).

This **corroborates and supersedes** the tentative `SIM_BLOCK_WRITER.md` §1 note that
`0x9f53cc/0x9f53ec` "deserialize block fields from `[0xc68428]` getters" — refined: the entire
`0x9f4dd0` body is a **global-config-singleton initializer**, and `0x9f5398` is part of it. The
earlier "strongest per-frame-integrator candidate" guess in `GEOMETRY_WRITER_INVESTIGATION.md` is
**ruled out**. The real per-frame integrator remains unlocated/thread-private (SIM_BLOCK_WRITER §2).

## 7. Classification & unresolved dependencies

| Item | Classification |
|---|---|
| `0x9f5398` is in an undefined Ghidra region (no function) | **STRUCTURALLY_UNDERSTOOD** (it is, and why) |
| `FUN_009f4dd0` boundaries `0x9f4dd0..0x9f769e`, run-once guard, 390-field init | **STRUCTURALLY_UNDERSTOOD** |
| ESI = global singleton `[0xc68428]`; `+0xc0..+0xd4` seeded once from getters `0x841e90..0x842340` | **STRUCTURALLY_UNDERSTOOD** |
| Caller chain `0x77cbf0 → 0x9efc70 → 0x9f4dd0`, keyed on fixture global `DAT_00c31b34` | **STRUCTURALLY_UNDERSTOOD** |
| "`0x9f5398` is NOT the per-frame integrator" | **STRUCTURALLY_UNDERSTOOD** (no RMW, run-once, setup-time) |
| Exact config value each getter `0x841e90..0x842340` returns into `+0xc0..+0xd4` | **LOCATED_NOT_UNDERSTOOD** (getters not decompiled) |
| The TRUE per-frame pos+=vel·dt integrator | **GENUINELY_UNAVAILABLE / thread-private** per SIM_BLOCK_WRITER §2 (not in this function) |

**Unresolved dependencies (exact):**
1. Decompile getter bank `0x841e90, 0x841f80, 0x842070, 0x842160, 0x842250, 0x842340` to name the
   config/default values written to `+0xc0..+0xd4` (and the meaning of singleton `[0xc68428]`).
2. Decompile the match-setup caller `0x77cbf0` and sibling tail `0x9f4730` for full setup semantics.
3. The per-frame integrator is still the open item from `SIM_BLOCK_WRITER.md` §2 — thread-private,
   reached only through the unlocated match worker-thread proc; not addressed by this function.

**Next concrete step:** In Ghidra (`cm2008_full`, single-access) **create/define functions** at
`0x9f4dd0`, `0x77cbf0`, `0x480110`, and getters `0x841e90..0x842340`, run auto-analysis on them
(headless `-noanalysis` / `reexport.sh` will report "NO FUNCTION" for `0x9f5398` because the region
is undefined — defining it first is mandatory), then re-export `FUN_009f4dd0` to confirm the field
semantics. This is a static DB pass only — no game run, no DRM.
