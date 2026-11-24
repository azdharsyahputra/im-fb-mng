# G1 — Match Event Descriptor / Record Data Model

Clean-room static RE of Championship Manager 2008.
Target: `cm2008.exe` (crack, `/Volumes/CM2008/Crack JPN/cm2008.exe`), `file_off = VA − 0x400000`.
Method: capstone 5.0.7, `CS_MODE_32`, disassembly from crack bytes (Ghidra mis-decodes the
cpuid/rdtsc profiler scopes; crack bytes are clean here). No execution, no DRM touched, ISO untouched.

Confidence tags: **VERIFIED** (read directly from instruction bytes), **INFERRED** (strongly implied
by caller usage), **STRUCTURALLY_UNDERSTOOD** (shape known, exact semantics from parent context not
re-derived here), **UNKNOWN**.

---

## 0. Executive summary

The accessors in the `FUN_005a61xx / FUN_005a63xx` family operate on a compact **event descriptor**
that is a **0x20-byte (32-byte) array element**, NOT the 0x2768 record directly. The 0x2768 "per-entity
record" from the G1 context is the **actor** that a descriptor resolves to via a `std::map` lookup
(§6). Every field inside the descriptor is a packed **handle** (id) into a separate global "database
table"; each getter is the canonical CM `id → pointer` resolver:

```
movzx eax, word[ecx+F]      ; read handle
cmp ax, NULL_SENTINEL  -> return 0
cmp ax, INVALID_SENTINEL -> return 0
mov reg, [GLOBAL_TABLE]      ; table manager
imul eax, eax, STRIDE        ; id * element size
add  eax, [reg+0x24]         ; + contiguous array base
ret                          ; -> &table[id]
```

Resolved tables found:

| Global      | Stride | Base field | Meaning (role in this model)                    |
|-------------|--------|------------|-------------------------------------------------|
| `0xc779ac`  | 0x2c   | `[+0x24]`  | **Contestant** record (desc+0 / desc+2)         |
| `0xc77df8`  | 0xb8   | `[+0x24]`  | **Event** object (desc+8 low-13; holds type id) |
| `0xc77778`  | 0x14c  | `[+0x24]`  | **Person/player** record (contestant+0)         |
| `0xc7fc08`  | 0x28   | `[+0x24]`  | secondary handle (desc+8 bits 13..26)           |
| `0xc802d4`  | 0x14   | `[+0x2c]`  | handle (desc+0xc, 21-bit)                        |
| `0xc78768`  | 0x5c   | `[+0x24]`  | handle (desc+0x10 word)                          |
| `0xc784a8`  | 0x20   | `[+0x2c]`  | **Descriptor array manager** (id = index)       |

---

## 1. Accessor bodies (VERIFIED — disassembly)

### FUN_005a6320  — descriptor → id (no bounds check)   `file_off 0x1a6320`
```asm
mov eax, ecx                 ; eax = descriptor ptr
mov ecx, [0xc784a8]          ; descriptor-array manager
sub eax, [ecx+0x2c]          ; eax -= array base
sar eax, 5                   ; / 0x20  -> element index = descriptor id
ret
```
**VERIFIED.** Descriptor id = `(desc − [[0xc784a8]+0x2c]) / 0x20`. Confirms the descriptor
array has **stride 0x20** and base at `[0xc784a8]+0x2c`.

### FUN_005a6330  — descriptor → id (with null check)    `file_off 0x1a6330`
```asm
mov eax, ecx
mov ecx, [0xc784a8]
sub eax, [ecx+0x2c]
sar eax, 5
cmp eax, 0x80000             ; null-id sentinel
jne .ok
xor eax, eax                 ; -> 0
.ok: ret
```
**VERIFIED.** Identical to `5a6320` plus a null-id sentinel `0x80000`. (The G1-context remark that
`5a6330`'s result "+4 holds an action-state field masked `0xffff3fff|0x3000`" is a
**read-modify-write performed by callers**, not by this getter — this getter only returns the id.
→ STRUCTURALLY_UNDERSTOOD, see §3 desc+4.)

### FUN_006a00f0  — (manager, id) → actor               `file_off 0x2a00f0`
```asm
sub esp,0x1c ; push esi; push edi
mov edi, ecx                 ; ecx = manager = [0xc784a8]
lea esi, [edi+0x30058]       ; map root/header inside the manager
lea eax, [esp+0x28]          ; &arg0 (the id/key from 5a6320)
lea ecx, [esp+0xc]           ; &result iterator
push eax ; push ecx ; mov ecx, esi
call 0x4f90f0                ; std::map::find(key) -> result
mov eax, [esp+8]             ; result node
cmp eax, esi                 ; == end/header ?
je  .null
mov eax, [eax+0x14]          ; node->mapped_value
.null: pop edi; pop esi; add esp,0x1c
ret 4
```
**VERIFIED.** A `std::map` lookup rooted at `manager+0x30058`, keyed by the descriptor id,
returning `node+0x14` (the mapped value = **actor pointer**) or null.

### FUN_005a6170  — descriptor → contestant 0/1          `file_off 0x1a6170`
```asm
cmp [esp+4], 0               ; arg: which contestant
jne .one
  movzx eax, word[ecx]       ; desc+0x00 : contestant[0] handle
  cmp ax,0x8000 -> ret 0
  cmp ax,0xffff -> ret 0
  mov ecx,[0xc779ac]; imul eax,eax,0x2c; add eax,[ecx+0x24]; ret 4
.one:
  movzx eax, word[ecx+2]     ; desc+0x02 : contestant[1] handle
  cmp ax,0x8000 -> ret 0
  cmp ax,0xffff -> ret 0
  mov edx,[0xc779ac]; imul eax,eax,0x2c; add eax,[edx+0x24]; ret 4
```
**VERIFIED.** `arg==0` → handle at **desc+0**, `arg!=0` → handle at **desc+2**; both resolved
through table `0xc779ac` (stride **0x2c**). Null sentinels `0x8000` / `0xffff`.

### FUN_005a61c0  — descriptor → event object            `file_off 0x1a61c0`
```asm
mov ax, word[ecx+8]          ; desc+0x08 (low 16 bits)
and ax, 0x1fff               ; 13-bit handle
movzx eax, ax
cmp ax,0x1000 -> ret 0       ; null sentinel
cmp ax,0x1fff -> ret 0       ; invalid sentinel
mov ecx,[0xc77df8]; imul eax,eax,0xb8; add eax,[ecx+0x24]; ret
```
**VERIFIED.** Reads **desc+8 bits 0..12 (13-bit)** → table `0xc77df8` (stride **0xb8**).

### FUN_005a06c0  — event object → event TYPE id          `file_off 0x1a06c0`
```asm
mov eax, [ecx+0x10]          ; (event object)+0x10
shr eax, 0x14               ; >> 20  -> top 12 bits
ret
```
**VERIFIED.** Operates on the **`5a61c0` result** (the 0xb8-stride event object), not on the
descriptor. Event type id = `[event+0x10] >> 20` (12-bit). See §4 for the `0xc31b34` comparison.

### FUN_00593cc0  — contestant → person/player record     `file_off 0x193cc0`
```asm
movzx eax, word[ecx]         ; contestant+0x00 : person handle
cmp ax,0x8000 -> ret 0
cmp ax,0xffff -> ret 0
mov ecx,[0xc77778]; imul eax,eax,0x14c; add eax,[ecx+0x24]; ret
```
**VERIFIED.** This is an **id→pointer resolver** (table `0xc77778`, stride **0x14c**), *not* the
"word>>12 & 0x7f" reader. The G1-context description compressed the chain: `593cc0` returns the
person record; the **7-bit attribute read happens in the caller** at `0x5a644c` (§5).

---

## 2. Cross-reference verification (the real pipelines)

### 2a. Event-type filter pipeline — `FUN_0077cf20` (VERIFIED)
```asm
mov  ecx, esi                ; esi = descriptor
call 0x5a61c0               ; desc+8 (13-bit) -> event object (0xb8 table)
mov  ecx, eax
call 0x5a06c0               ; [event+0x10] >> 20  = 12-bit event type id
movzx edx, ax
cmp  edx, [0xc31b34]         ; compare to global selected/filter event-type id
jne  .skip
...                          ; (act when the event type matches the filter)
mov  ecx, esi
call 0x5a6330               ; descriptor -> id (null-checked)
```
`0xc31b34` is a **global filter holding the currently-selected event-type id** (initialised to `-1`
= "none"; see `FUN_0077cbf0`: `cmp [0xc31b34], -1`). This VERIFIES the `5a61c0 → 5a06c0 → type id →
cmp 0xc31b34` chain exactly as stated in the G1 context.

### 2b. Actor resolution — `FUN_0077cd40` (VERIFIED)
```asm
mov  ebp, [0xc784a8]         ; descriptor-array manager
mov  esi, [ebx]              ; esi = descriptor ptr
mov  ecx, esi
call 0x5a6320               ; -> descriptor id
push eax
mov  ecx, ebp               ; manager = this
call 0x6a00f0               ; map lookup id -> actor
mov  [edi+0x98], eax         ; store actor at (caller object)+0x98
```
VERIFIES: `descriptor → 5a6320 → id → 6a00f0 map → actor`, actor cached at **object+0x98**
(exactly the "actor stored at module+0x98" from the G1 context). The **actor is the larger record**
(the 0x2768-stride per-entity record referenced in G1 context — INFERRED from role, not re-measured).

### 2c. Attribute scoring routine — `FUN_005a63xx` (VERIFIED)
```asm
...
mov  ecx, edi               ; edi = descriptor
push esi                    ; which contestant
call 0x5a6170               ; -> contestant ptr (0x2c table)
mov  ecx, eax
call 0x593cc0               ; -> person record (0x14c table)
mov  eax, [eax+0x3c]         ; person+0x3c
shr  eax, 0xc               ; >> 12
and  al, 0x7f               ; & 0x7f  -> 7-bit attribute
...
movzx eax, word[edi+0x10]    ; desc+0x10 handle
cmp ax,0x8000 / 0xffff -> 0
mov ecx,[0xc78768]; imul eax,eax,0x5c; add eax,[ecx+0x24]  ; stride 0x5c
mov edx,[eax+0x20]; shr edx,7; test dl,1                   ; (entity+0x20) bit7 flag
```
VERIFIES the **7-bit attribute** lives at **person_record+0x3c** as `(dword >> 12) & 0x7f`, reached
through `5a6170 → 593cc0`. Also reveals **desc+0x10** is a handle into table `0xc78768` (stride 0x5c).

---

## 3. EVENT DESCRIPTOR struct (0x20 bytes) — array @ `[0xc784a8]+0x2c`

| Offset | Size | Field | Resolver | Confidence |
|--------|------|-------|----------|------------|
| +0x00 | u16 | **contestant[0] handle** → table `0xc779ac` (stride 0x2c). Null `0x8000`/`0xffff` | `5a6170(,0)` | **VERIFIED** |
| +0x02 | u16 | **contestant[1] handle** → table `0xc779ac` (stride 0x2c). Null `0x8000`/`0xffff` | `5a6170(,1)` | **VERIFIED** |
| +0x04 | u8? | class flag / **action-state** field (masked `&0xffff3fff | 0x3000` by callers) | callers of `5a6330` | STRUCTURALLY_UNDERSTOOD |
| +0x05 | u8? | class flag (per G1 context) | — | UNKNOWN |
| +0x06 | u8? | class flag (per G1 context) | — | UNKNOWN |
| +0x08 | u32 | **packed handles**: bits 0..12 = **event handle** → `0xc77df8` (0xb8), null `0x1000`/`0x1fff`; bits 13..26 = handle → `0xc7fc08` (0x28), null `0x2000`/`0x3fff` | `5a61c0`, `5a6550` | **VERIFIED** |
| +0x0c | u32 | packed handle: bits 0..20 (21-bit) → table `0xc802d4` (stride 0x14, base `[+0x2c]`); null `0x100000`/`0x1fffff` | `5a61f0` | **VERIFIED** |
| +0x10 | u16 | handle → table `0xc78768` (stride 0x5c). Null `0x8000`/`0xffff` | scoring fn `0x5a64be` | **VERIFIED** |
| +0x12..+0x1f | — | remaining bytes of the 0x20 element — not touched by these accessors | — | UNKNOWN |

**Descriptor identity:** `id = (desc − [[0xc784a8]+0x2c]) / 0x20`; null-id sentinel `0x80000`
(`5a6320`/`5a6330`). **VERIFIED.**

> Note on the 0x2768 figure: these accessors manipulate the **0x20-byte descriptor**, which is a
> compact handle. The 0x2768 "per-entity record" from the G1 context is the **actor** object reached
> through the §6 map, not the thing `5a6320`/`sar 5` divides. (INFERRED.)

---

## 4. EVENT object (table `0xc77df8`, stride 0xb8) — resolved from desc+8

| Offset | Size | Field | Confidence |
|--------|------|-------|------------|
| +0x10 | u32 | bits 20..31 (12-bit) = **event TYPE id** (`5a06c0`), compared to global filter `[0xc31b34]` | **VERIFIED** |

Other 0xb8 fields: UNKNOWN (out of scope).

---

## 5. CONTESTANT record (table `0xc779ac`, stride 0x2c) — resolved from desc+0/+2

| Offset | Size | Field | Confidence |
|--------|------|-------|------------|
| +0x00 | u16 | **person handle** → person table `0xc77778` (stride 0x14c) via `593cc0`. Null `0x8000`/`0xffff` | **VERIFIED** |

Other 0x2c fields: UNKNOWN.

## 5b. PERSON / player record (table `0xc77778`, stride 0x14c)

| Offset | Size | Field | Confidence |
|--------|------|-------|------------|
| +0x3c | u32 | bits 12..18 = **7-bit attribute** `(v>>12)&0x7f` (read at `0x5a644c`) | **VERIFIED** |

---

## 6. ACTOR resolution (how an "actor" is obtained from a descriptor)

```
descriptor ptr
  └─ 5a6320 ──────────► id = (desc − [[0xc784a8]+0x2c]) / 0x20
       └─ 6a00f0(manager=[0xc784a8], id):
            std::map::find at manager+0x30058, key=id
            └─► node+0x14  = actor pointer (or null)
                 └─ caller stores it at  (its object)+0x98
```
**VERIFIED** end-to-end (§2b). The actor is a separate, larger object (the per-entity record; its
0x2768 stride is from the G1 context — INFERRED here, not re-measured).

## 6b. How a "contestant" is obtained from a descriptor
```
descriptor ptr
  └─ 5a6170(desc, side∈{0,1}):
       handle = word[desc + (side?2:0)]           (null 0x8000/0xffff)
       └─► &table_0xc779ac[handle]  (stride 0x2c) = contestant record
            └─ 593cc0: person = &table_0xc77778[ word[contestant+0] ]  (stride 0x14c)
                 └─ attribute = (dword[person+0x3c] >> 12) & 0x7f       (7-bit)
```
**VERIFIED** end-to-end (§2c).

---

## 7. Confidence roll-up

- **VERIFIED:** all 7 accessor bodies; descriptor fields +0/+2/+8/+0xc/+0x10; descriptor id formula
  and sentinel; the `5a61c0→5a06c0→[0xc31b34]` event-type pipeline; the `5a6320→6a00f0→object+0x98`
  actor pipeline; the `5a6170→593cc0→person+0x3c` 7-bit-attribute pipeline; the seven table globals
  and their strides/base-offsets/sentinels.
- **INFERRED:** actor == the larger (0x2768) per-entity record; semantic labels "event / contestant /
  person" (from role in the pipelines).
- **STRUCTURALLY_UNDERSTOOD:** desc+4 action-state field — getter only returns the id; the
  `&0xffff3fff|0x3000` masking is a caller-side read-modify-write not decompiled in this pass.
- **UNKNOWN:** desc+5/+6 class flags; desc+0x12..0x1f; interior layout of the 0xb8/0x2c/0x14c/0x28/
  0x14/0x5c tables beyond the fields above.

## 8. Negative results (valid)
- `FUN_00593cc0` does **not** itself compute `word>>12 & 0x7f`; it is a plain id→pointer resolver.
  The attribute math is in the caller. (Corrects the compressed G1-context description.)
- `FUN_005a06c0` does **not** read the descriptor; it reads the `5a61c0`-resolved event object
  (`+0x10 >> 20`).
- `FUN_005a6330` does **not** read/mask desc+4; it only returns the null-checked id. The action-state
  masking is caller-side.
- The descriptor stride these accessors use is **0x20**, not 0x2768; 0x2768 belongs to the actor
  record reached via the map.

---
*Static-only. No execution, no DRM bypass, no ISO modification. Disassembly from crack bytes via
capstone; file offsets = VA − 0x400000.*
