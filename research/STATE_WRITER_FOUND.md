# eGameState writer — FOUND (static, VERIFIED)

Resolves the open item from STATE_WRITER_TRACE.md. The "SetGameState writer" is NOT a
single parameterised function — it is a FAMILY of per-state "enter-state" handlers, each
a `__thiscall` method on the state sub-object (`this == *(matchCtx+0xe4)`), living in the
match-period/flow module at **VA 0x6dd000–0x6ed000** — a region OUTSIDE every earlier dump
(those were seeded from the 0x77xxxx match-sim module), which is why the prior trace could
only bound it. It matches agent D's prediction exactly: the write site receives the
sub-object as `this`, with no `+0xe4` reload (binary-wide the load-+0xe4/store-+0x30 idiom
is 0 hits — because the two are in different functions).

## How it was found
Byte-search for `mov dword [reg+0x30], <eGameState imm>` (C7 /0 encodings) over the whole
image: 29 sites, all `mov [ecx+0x30], <state>` in the 0x6dd000–0x6ed000 cluster — ecx is
the thiscall `this` = the clock/state sub-object. Confirmed by disassembly.

## The writer shape (VERIFIED from two handlers)
Each enter-state handler writes `+0x30 = <state>` and resets the sub-object's associated
fields in one go. Example GameOver (ends `ret` @0x6ecd4e):
```
[ecx+0x20] = 5          ; signed counter/duration
[ecx+0x24] = -5
[ecx+0x30] = 0x0c       ; eGameState = GameOver   <-- the state write
[ecx+0x04] , [ecx+0x08] = <timing constants>
[ecx+0x18]=[ecx+0x19]=[ecx+0x39] = 1   ; active/dirty flag bytes
```
KickoffWaiting (ends `ret` @0x6dde29):
```
[ecx+0x24] = 0
[ecx+0x2c] = 0          ; cursor reset
[ecx+0x20] = -10
[ecx+0x30] = 0x0e       ; eGameState = KickoffWaiting
[ecx+0x08] = <const>
[ecx+0x18]=[ecx+0x19]=[ecx+0x38]=[ecx+0x39] = 1
[ecx+0x3c] = 100 , [ecx+0x40] = 5      ; timers
```

## Sub-object field map (INFERRED from the setters)
| off | meaning |
|---|---|
| +0x04,+0x08 | per-state timing constants (packed) |
| +0x18,+0x19,+0x38,+0x39 | active/dirty flag bytes (set to 1 on enter) |
| +0x20 | signed counter/duration for the state |
| +0x24 | aux counter |
| +0x2c | cursor (reset on kickoff) |
| **+0x30** | **eGameState** (VERIFIED) |
| +0x3c,+0x40 | per-state timers (e.g. 100, 5 for kickoff) |

## Observed state writers (sample, VERIFIED by immediate value)
KickoffWaiting (0x0e) ×many, GameOver (0x0c) ×several, PenaltyShootout (0x15),
PenaltyActivated (0x14), HalfActivated (0x18) — across the 0x6dd000–0x6ed000 module. The
transition TRIGGERS (which handler runs when) are the callers in that same match-flow
module; the writers themselves are now located and characterised.

## Classification
- eGameState lives at sub-object +0x30 — **VERIFIED**
- writers = per-state thiscall enter-handlers in 0x6dd000–0x6ed000, each sets +0x30 +
  resets the state fields — **VERIFIED** (byte-pattern + disassembly of two handlers)
- the full field-reset semantics (+0x20/+0x24/+0x3c/+0x40 units) — **INFERRED**
- Not runtime-only: fully static. (The earlier "runtime watchpoint needed" verdict is
  superseded — the module was simply never dumped.)
