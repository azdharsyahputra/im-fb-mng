# G1 — Match Event Handlers (dispatcher / activate phase)

Clean-room static RE. Target: `/Volumes/CM2008/Crack JPN/cm2008.exe` (image base `0x400000`,
`file_off = VA − 0x400000`). All disassembly via capstone 5.0.7 on the crack bytes
(Ghidra scope profiler artifacts ignored). Nothing was executed; ISO/DRM untouched.

Confidence tags: **VERIFIED** (read directly from crack bytes), **INFERRED** (semantic label
from structure/call pattern), **STRUCTURALLY_UNDERSTOOD** (mechanics certain, purpose partial),
**UNKNOWN**.

---

## Headline result

**None of the six `FUN_009efa*/FUN_009efc*` event handlers bump any stat counter.**
They are the **match-viewer / highlight presentation layer**. The stat counter
`FUN_005cd020` is **not** called by any of them, nor by the dispatcher `FUN_0077cd40`,
nor by the activate phase `FUN_0077cf20`. (VERIFIED — byte scan below.)

The packed stat counters are bumped only inside the **duel `FUN_0077c710`**, which the
dispatcher calls on its own **flag4** branch, *separately* from these handlers.
So the real chain is:

```
class/flag4 event ─► FUN_0077c710 (duel, RNG + p=A/(A+B)) ─► FUN_005cd020(this=statDword, 0|1)
class 0/1/3 event ─► FUN_009efa10 / FUN_009efa20 (presentation only, NO stat write)
```

---

## 1. The handlers are forwarding thunks (VERIFIED)

Every target is an 8–9-byte stub. Six are identical adapter thunks:

```asm
FUN_009efa10:  mov ecx, [ecx+4]      ; switch `this` → wrapped inner object at +4
               jmp  <impl>
```

| Thunk (csv size) | Bytes | Real impl | impl size |
|---|---|---|---|
| FUN_009efa10 (8) | `8B4904 E978830000` | **FUN_009f7d90** | 356 |
| FUN_009efa20 (8) | `8B4904 E9D8990000` | **FUN_009f9400** | 639 |
| FUN_009efa40 (8) | `8B4904 E9C8290000` | **FUN_009f2410** | 71 |
| FUN_009efa70 (8) | `8B4904 E9E89C0000` | **FUN_009f9760** | 1043 |
| FUN_009efc30 (8) | `8B4904 E908100000` | **FUN_009f0c40** | 182 |
| FUN_009efc90 (8) | `8B4904 E918480000` | **FUN_009f44b0** | 359 |

`mov ecx,[ecx+4]` means the outer object wraps the real controller at `+4`. All analysis
below is on the *impl* with `this` = that inner controller (a large object; fields up to
`+0x12574` are referenced).

### FUN_009efa90 — the shared getter (VERIFIED)

```asm
mov eax, [ecx+4]        ; same inner object at +4
add eax, 0x110          ; → sub-struct at +0x110
ret
```
Returns `&inner->field_0x110`. This is the event's timeline/animation sub-object that
`efa10/efa20/efa70` all populate (`lea … [esi+0x110]`). The dispatcher uses it directly
(`FUN_0077cd40 @0x77ce37 → efa90 → FUN_0048ba30`) to test that sub-object. **Confirmed
shared getter**, not a counter.

---

## 2. Dispatcher class routing (VERIFIED from FUN_0077cd40)

`this`=controller (`edi`); `ebx`=event descriptor; `esi=[ebx]`=actor/subject entity.
Byte flags on the descriptor select the class:

| Condition | Call site | Handler | Class arg |
|---|---|---|---|
| `[ebx+6] != 0` (flag6) | `0x77cdb3` `push 3; call 9efa10` | **efa10** | **3** |
| `[0xc84954]==0` & (`[ebx+5]` (flag5) \| `[0xc84950]`) | `0x77cdde` `push 1; call 9efa20` | **efa20** | **1** |
| else | `0x77ce26` `push 0; call 9efa20` | **efa20** | **0** |
| `[ebx+4] != 0` (flag4) | `0x77cdf8` `call 0077c710` | **duel** | (stat path) |

- The class value (3 / 1 / 0) is stored by each impl into `inner[+0x1072c]` (VERIFIED:
  `efa10 @0x9f7dce`, `efa20 @0x9f94b1`). When `[ebx+4]&0xf000==0`, efa10 writes `4` and bails.
- Dispatcher **draws the 0x19660d LCG itself** (`@0x77cd84`, `imul …,0x19660d; add 0x3c6ef35f;
  store; shr 8`) to seed `[0xc84958]` — a random gate for highlight selection, *not* a stat.
- Activate phase `FUN_0077cf20` calls **efa70** (`@0x77cf86`), **efa40** (`@0x77cff6`),
  **efc30** (`@0x77d07f`). efc90 is called by `FUN_0077ee80`, not by the activate phase. (VERIFIED)

---

## 3. Per-handler role + effects

### efa10 → FUN_009f7d90  —  class 3 (flag6) highlight builder  (STRUCTURALLY_UNDERSTOOD)
`ret 0xc` (3 args: actor, `5a6330(desc)`, 3). Guard: `[desc+4]&0xf000`; else writes
`[+0x1072c]=4`, returns. Calls `__controlfp`, `FUN_005ccf30(desc)` (checks type==3), stores
class 3 into `[+0x1072c]`, then builds the on-screen event: timeline sub-obj via
`FUN_009ea050([+0x110])`, side records `FUN_009f4bd0(side 0/1)`, graphics slots
`FUN_0048bec0(idx 0..3, globals 0xc67eb0..c0)`, `FUN_009f13d0`/`FUN_009f0fe0`.
Writes `[+0x10801] = ([+0x1072c]==3)`. **No stat counter. No RNG.**

### efa20 → FUN_009f9400  —  class 0/1 main event builder  (STRUCTURALLY_UNDERSTOOD)
`ret 0x10` (4 args: class(0|1), actor, desc, 0). Stores class into `[+0x1072c]`. Builds the
event display: `FUN_009f9e40`, `FUN_009f7730`, timeline `FUN_009ea050`, per-side text
`FUN_009f7f00(0/1)`, team crests `FUN_005a6170→594020→9f3130(0/1)`, label records
`FUN_005a61f0→9f3330(idx 2,1,3)`, graphics `FUN_0048bec0(0..3)`. Sets a 3-bit display-state
field on `5a6330(desc)` (`and 0xffff2fff; or 0x2000` = value 2). Writes
`[+0x10802]=global[0xc9a4e9]` and `[+0x10801] = ([+0x1072c]==1 || global!=0)`.
**No stat counter. No RNG.**

### efa40 → FUN_009f2410  —  activate-phase commit/close  (STRUCTURALLY_UNDERSTOOD)
Guard `[+0x10818]` (pending-event flag); if 0, return. If `FUN_00493930([+0x118])` false and
`[+0x1072c]` in `[2,3]`, call `FUN_009f2220`. Clear `[+0x10818]=0`; tail-jmp
`FUN_009f9e60([+8])`. Commits/finalizes the current event. **No stat counter. No RNG.**

### efa70 → FUN_009f9760  —  activate-phase scene build  (STRUCTURALLY_UNDERSTOOD)
`ret 4`. Guard `FUN_00493e00([+0x118])`. Sets display flags on arg event (`and 0xffff3fff;
or 0x3000` = value 3). Populates the 22-player scene: loops 0xC side entries (stride 0x34c,
player render structs at +0x1a8 reset to `0x100000/0x20000`), per entry `FUN_0077c200`
(view helper) + `FUN_009f2460` + setters `FUN_005cd970/5cf0a0/5cf110/5cf1b0/5cde10` on the
draw sink. Resolves two team rosters (`[+0x2d8]`,`[+0x2e4]`) via `FUN_009efd30`, ball/ids via
`FUN_005cdc40(0x100000 sentinel, side 0/1)`, camera `FUN_005cef20`. **No stat counter.
No RNG** (the LCG-looking constants are all inside callees, not here).

### efc30 → FUN_009f0c40  —  activate-phase graphics teardown  (VERIFIED mechanics)
Releases up to 4 sprite/label handles `[+0xb4]/[+0xb6]/[+0xb8]/[+0xba]` (sentinel `0xffff`)
via `FUN_009fd680([+0xd0], id, 0)`, then `FUN_009fd690([+0xd0],0)`; resets the 4 handles to
`0xffff`, clears `[+0xd8]=0`, `[+0xcc]=0`, `[+0xce]=0`. **No stat counter. No RNG.**

### efc90 → FUN_009f44b0  —  commentary / caption text builder  (STRUCTURALLY_UNDERSTOOD)
Stack-cookie protected (0x7ec buffers). Formats two person names (`[+0x1250c]`,`[+0x1250e]`;
sentinels `0x4000/0x7fff`; person array `[0xc800e8]+0x24`, stride 0x10) via `FUN_009f1770`,
and two club names (`[+0x12510]`,`[+0x12514]`; sentinels `0x100000/0x1fffff`; club array
`[0xc801e8]+0x2c`, stride 0x164) via `FUN_009f4180`, into stack buffers, then emits the line
via `FUN_00a091f0(&inner[+0x110], [+0x10818], &[+0x10820], &[+0xa160], …)`.
Caller: `FUN_0077ee80`. **No stat counter. No RNG.**

---

## 4. The stat counter and where it is actually bumped (VERIFIED)

### FUN_005cd020 — packed two-field saturating counter
`this`(ecx)=pointer to one packed dword; arg[esp+4]=`0` or `1`. Increments one 5-bit
sub-field by 1 with saturation:

| arg | field mask | step | saturates at |
|---|---|---|---|
| 0 | bits 21–25 (`0x03E00000`) | `0x200000` | `0x03E00000` (31) |
| 1 | bits 26–30 (`0x7C000000`) | `0x04000000` | `0x7C000000` (31) |

Matches the stated spec exactly.

### Who calls FUN_005cd020 (full byte-scan of the image — only 3 callers):
`FUN_0077c710` (the duel), `FUN_00a023f0`, `FUN_00a029c0`. **None of the efa*/efc* handlers.**

### Duel FUN_0077c710 (dispatcher flag4 branch) bumps the counter:
`this`=controller (`mov ecx,edi; call 0077c710` from dispatcher). `edi` inside = the stat
dword; `esi` = RNG state (`[esi]` is the 0x19660d LCG seed).

- `@0x77c834` `mov ecx,edi; push [esp+0x28]; call 005cd020` — bump field selected by a prior value.
- `@0x77c930` `push 0; mov ecx,edi; call 005cd020` → **arg0 (bits21-25)**, inside a probability
  loop: draw `0x19660d` LCG `@0x77c93d`, convert to float in `[1.0,2.0)` (`shr 9; or 0x3f800000`),
  compare — this is the `p = A/(A+B)` contest.
- `@0x77c995` `push 1; mov ecx,edi; call 005cd020` → **arg1 (bits26-30)**, paired with LCG draw
  `@0x77c9a0`.

So the two 5-bit saturating sub-counters are the **two contestants' running tally** (side A →
arg0, side B → arg1), tallied during probabilistic duel resolution. `FUN_005d0460` (listed as a
candidate) is a tiny accessor (`push esi; mov esi,ecx; mov eax,[esi+4]`), **not** a counter.

---

## 5. Summary table (class → handler → role → counter)

| Class | Flag | Handler (thunk → impl) | Role | Stat counter bumped | RNG 0x19660d |
|---|---|---|---|---|---|
| 3 | flag6 | efa10 → 9f7d90 | highlight event build (display) | **none** | no |
| 1 | flag5 | efa20 → 9f9400 | main event build, class=1 | **none** | no |
| 0 | else | efa20 → 9f9400 | main event build, class=0 | **none** | no |
| (4) | flag4 | *duel* 0077c710 | probabilistic contest p=A/(A+B) | **005cd020(0)=bits21-25, (1)=bits26-30** | **yes (×3+)** |
| — act | — | efa70 → 9f9760 | activate: 22-player scene build | **none** | no (callees only) |
| — act | — | efa40 → 9f2410 | activate: commit/close event | **none** | no |
| — act | — | efc30 → 9f0c40 | activate: graphics teardown | **none** | no |
| — | — | efc90 → 9f44b0 | commentary/caption text | **none** | no |
| — | — | efa90 → getter | returns `inner+0x110` (shared) | **none** | no |

**Confidence:** thunk resolution, class routing, counter non-involvement, 005cd020 caller set,
and 005cd020 semantics are **VERIFIED** from crack bytes. Per-handler *role labels*
(highlight/commentary/scene) are **STRUCTURALLY_UNDERSTOOD / INFERRED** from call patterns and
field offsets; the mechanical reads/writes listed are VERIFIED.

**Key correction to the G1 premise:** the efa* handlers are presentation, not scoring. Stat
counters are bumped by the duel `FUN_0077c710` on the flag4 branch, not by the class 0/1/3
presentation handlers.
