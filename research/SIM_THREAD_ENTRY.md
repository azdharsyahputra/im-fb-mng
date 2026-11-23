# CM2008 Match Worker-Thread Entry

Target: `/Volumes/CM2008/Crack JPN/cm2008.exe` (SecuROM-wrapped PE32, ImageBase
`0x400000`). Static only; bytes read directly; PE parsed by hand; disassembly via
capstone (`CS_ARCH_X86 / CS_MODE_32`). No Ghidra, no execution.

**Bottom line (VERIFIED):** there is **no separate "match-sim" worker thread**. The
continuous match simulation is **pumped cooperatively** — `GCMatchModule::Update`
is invoked *virtually* (`vtable+0xa8`) once per program-step by the module
manager/main pump (see `MATCH_CLOCK_AND_UPDATE_LOOP.md`), not via `CreateThread`.
The game spawns exactly **two** `CreateThread` threads; only one touches match
data, and it is the engine's **generic worker-`Thread` class**, used (per the
`GCMatchModule` log string) to run **background/off-screen fixtures "on a thread"** —
not the watched-match tick.

---

## 0. PE / import facts (VERIFIED)

- Sections (flat `file_off == VA-0x400000` holds for all except `.memory`):
  `.text 0x1000–0x711000`, `.rdata 0x711000–0x808000`, `.data 0x808000–0x89f000`,
  `.rsrc`, then SecuROM payload sections `ars / est / artem / celare / .securom`
  (`.securom` entropy 7.22 = encrypted; `.text` sample entropy ~6.0 = normal code).
- Import: `KERNEL32.dll!CreateThread`. **IAT slot VA = `0xb110dc`.** Name string
  `"CreateThread"` at **`0x806f30`** (flat) — toolkit value confirmed.
- **No `_beginthread` / `_beginthreadex`** import present (byte-searched; absent).
  (`CreateRemoteThread` text appears only inside the SecuROM blob at `0x198057e`,
  not game code.)
- Call pattern `FF 15 DC 10 B1 00` (`call dword [0xb110dc]`): **exactly 2 sites** in
  `.text` — `0x314f5` and `0x75633`. No other reference (mov/push) to `0xb110dc`.

---

## 1. CreateThread call site #1 — `0x314f5`  (thread proc `0x4314b0`) — NOT match

Thin stdcall wrapper `0x314e0`:
```
0x314e0  mov  ecx,[esp+4]
0x314e4  lea  eax,[esp+4]
0x314e8  push eax            ; lpThreadId
0x314e9  push 0              ; dwCreationFlags
0x314eb  push ecx            ; lpParameter = caller arg
0x314ec  push 0x4314b0       ; lpStartAddress  <-- thread proc #1
0x314f1  push 0              ; dwStackSize
0x314f3  push 0              ; lpThreadAttributes
0x314f5  call dword [0xb110dc]
0x314fb  test eax,eax; setne al; ret 8
```
**Thread proc #1 = `0x4314b0`.** Body reads globals `[0xc77668]`, `[0xc8fb60]`,
`[0xc8fb6c]`; calls `0x262cf0`; does string/buffer teardown (`0x1fd0`=free,
`0x418120`); returns a **float**. No reference to any match structure.
Wrapper `0x314e0` has **zero** pointer references anywhere in the image; `0x4314b0`
is referenced only by the push at `0x314ed`.
**Identity:** runtime/CRT-style init or housekeeping thread. **VERIFIED not-match;**
exact role **INFERRED**.

---

## 2. CreateThread call site #2 — `0x75633`  (thread proc `0x4755d0`) — generic Thread class, match-data worker

C++ method `Thread::start` at `0x75620` (thiscall, `this`→esi):
```
0x75620  push esi
0x75621  mov  esi,ecx            ; this
0x75623  lea  eax,[esi+8]
0x75626  push eax                ; lpThreadId  = &this[8]
0x75627  push 0                  ; dwCreationFlags
0x75629  push esi                ; lpParameter = this
0x7562a  push 0x4755d0           ; lpStartAddress (trampoline)  <-- thread proc #2
0x7562f  push 0                  ; dwStackSize
0x75631  push 0                  ; lpThreadAttributes
0x75633  call dword [0xb110dc]
0x75639  mov  [esi+4],eax        ; store handle
```
Call chain: `0x700b0` (SEH frame; stores work-fn `0x46ff90`, calls Thread methods
`0x72350/0x6fdf0/0x75760`) → `Thread` ctor `0x75bf0` (SEH) → `Thread::start`
`0x75620`. The only pointer ref to `0x4755d0` is the push at `0x7562b`.

**Thread proc #2 = `0x4755d0`.** The static pointer lands **16 bytes inside**
function `0x4755c0` — i.e. the 16-byte prologue is **SecuROM-mutated** (`0x4755c0`
has no refs; the real entry is obscured). The body from `0x4755d3` is genuine code
(entropy ~6.0) and is unambiguously **match code**:
```
0x4755c0  mov  eax,[0xc94cf8]
0x4755c7  mov  edi,[eax+0xac]          ; player index (from a match/view ctx)
0x4755cd  cmp  edi,0x80000 ... 0xfffff ; sentinel/bounds checks
0x4755e7  mov  ecx,[0xc784a8]          ; *** player pool DAT_00c784a8 ***
0x4755ed  shl  edi,5                   ; *** stride 0x20 ***
0x4755f0  add  edi,[ecx+0x2c]          ; &pool.array[index]
0x4755f4  call 0x5e0050                ; entity validity check
            ...  0x1a6170 / 0x193cc0 / 0x261710 / 0x21d480  (match accessors)
0x47565c  call 0x1ee2b0  (this+0x4c,+0x54)   ; copy coord/vector field
0x47566b  call 0x1ee2b0  (this+0x4c,+0x58)   ; copy coord/vector field
0x475674  ret                                 ; <-- RETURNS, no per-frame loop
```
**VERIFIED:** touches the player pool `DAT_00c784a8` with stride `0x20`, indexes an
entity, validates it, and copies two coordinate/transform vectors into the thread
param object. **It is a short routine that returns — not a continuous tick loop.**
**Identity (INFERRED):** the engine's **general-purpose `Thread` class trampoline**;
this instance reads a tracked match player's world transform — a **view / background
worker**, and (per §3) the vehicle for simulating fixtures off the main pump.

---

## 3. Where the continuous match sim actually runs (cooperative pump)

From `MATCH_CLOCK_AND_UPDATE_LOOP.md` (team VERIFIED on the match engine):
- The detailed match is a polymorphic `GCMatchModule`; **`Update` =
  `FUN_0077ee80`**, invoked **purely virtually** via `vtable+0xa8` (only xref is the
  vtable slot), **once per program-step** by the module manager. → the watched-match
  sim is **pumped cooperatively**, there is no dedicated sim thread.
- `GCMatchModule::Update` logs **`"End of fixture %d that ran on a thread."`**
  (C-string at **`0x7a2e60`**, followed by `"GCMatchModule"`). This is the only
  thread link in the match engine: **background fixtures** (other matches on the
  match-day) are offloaded to the `Thread` class of §2; the clock itself is a
  continuous float accumulator advanced per step, not a tick.

**CAVEAT (VERIFIED discrepancy):** in *this* file `.text` ends at `0x711000`, so the
team's `0x77xxxx` `GCMatchModule` *code* addresses (e.g. `0x77ee80`) and vtable
`0xba2904` fall in `.rdata`/data here and read as non-code (the `+0xa8` slot =
`0xca2b1c0c`). Those code VAs come from a differently-mapped image. The **data**
addresses do match this file: player pool `0xc784a8` is referenced 372× across
`.text` (clusters `0x18xxxx–0x2axxxx`, `0x37xxxx`), ball pool `0xc779ac` 297×,
period field `+0x3004c` 30× — none inside either CreateThread proc's region, i.e.
the bulk of match logic runs in the pumped (non-thread) code paths.

---

## 4. Classification summary

| item | VA | verdict | conf |
|------|----|---------|------|
| CreateThread IAT slot | `0xb110dc` | import call target | VERIFIED |
| CreateThread call sites | `0x314f5`, `0x75633` | the only two in `.text` | VERIFIED |
| Thread proc #1 | `0x4314b0` | runtime/CRT init helper, NOT match | VERIFIED (not-match) / INFERRED (role) |
| Thread proc #2 | `0x4755d0` (fn `0x4755c0`, SecuROM-mutated prologue) | generic `Thread` class; reads match player-pool transform; **not a loop** | VERIFIED (touches `0xc784a8` stride 0x20) / INFERRED (view/background-fixture worker) |
| Continuous match sim | `GCMatchModule::Update` (virtual `vtable+0xa8`) | **pumped per program-step, not threaded** | VERIFIED (cross-ref) |
| Match-day fixtures "on a thread" | via `Thread::start` `0x75620` → proc `0x4755d0` | background offload | INFERRED |

**Answer:** No separate match-sim thread. The sim is pumped from
`GCMatchModule::Update`. The only match-touching `CreateThread` thread is the generic
`Thread` class (entry `0x75633`, proc `0x4755d0`), used for background-fixture /
view work — not the watched-match tick.
