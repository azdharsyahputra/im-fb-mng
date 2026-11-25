# Match clock & event cadence (P6) — recovered static from the crack

## Clock units — VERIFIED (FUN_0077c660, 170 b, "clock repaint")
Internal match time is in **SECONDS**. c660 reads two time parts from the match-time object
([esi+0x98]→FUN_009efa90→+0xe4, then +0x44 and +0x48), sums them, and formats to a packed H:MM
display dword at [obj+0x30]:
- `fmul [0xba28c0]` where `0xba28c0 = 0.000277778 = 1/3600` → seconds → **hours**, taken `& 0x1f`
  (5-bit hour field) `<< 6`.
- remainder `fmul [0xba28bc] (=3600)` / `fmul [0xb5b1d4] (=0.0166667 = 1/60)` → **minutes**, `& 0x3f`
  (6-bit, 0..59).
- packed `hour(5) | min(6)` into [obj+0x30] (mask 0xffffc007).
This VERIFIES the Go `match` note "continuous float clock, hours = s/3600": base unit = seconds.

## Event cadence — VERIFIED (FUN_009d52e0, "per-entity time accumulator")
`if (matchClock >= entity[+0x154]) entity[+0x154] += 10.0f` (const `0xc5d43c = 10.0`). With the clock
in seconds, each entity's next-action time advances by **10 seconds of match time**. So every entity
gets an action-check roughly **once per 10 sim-seconds** → ~540 checks/entity across a 90-minute match
(5400 s / 10). This is the real dt/cadence basis: the native engine's per-entity event interval is 10 s,
not an arbitrary tick. (The Go `match` pump currently takes an arbitrary dt; to mirror the original,
an entity acts on a 10-s cadence. Documented, not yet wired — engine numbers unchanged.)

## SetGameState writer — PARTIAL (FUN_0077e850, 1283 b, "boundary / GameOver")
e850 is the clock-boundary / GameOver trigger. Its many `mov byte ptr [reg], 0|1` stores are BOOLEAN
FLAGS (pending-event / half bits), not the 32-value eGameState enum. It also sets a global time
threshold (`fld [0xba28bc]=3600 → fstp [0xc84c7c]`). The exact eGameState enum STORE (writing
InPlay=0 / HalfOver=0x0b / GameOver=0x0c to the state field) is NOT pinned among these byte stores —
it is written elsewhere (follow-up). The lifecycle itself is already modelled VERIFIED in the Go
`state`/`match` packages (InPlay→HalfOver→GameOver); only the original writer's offset is unresolved.

## Classifications
- Clock base unit = seconds; H:MM packing (÷3600 / ÷60) — **VERIFIED** (FUN_0077c660).
- Per-entity event cadence = +10.0 s (FUN_009d52e0) — **VERIFIED**.
- eGameState writer offset/site — **PARTIAL / UNKNOWN** (flags found; enum store not pinned here).

## Use for the engine
The native match tick can mirror the original by giving each entity a 10-s action cadence (dt-derived),
which turns the clean-room EventRatePerMin into a recovered interval. Left as a documented wiring step
(no engine numbers changed here, per the "real numbers only" rule).

---

## Clock pack + end-of-period boundary (decompiler, VERIFIED)

### FUN_0077c660 — seconds → H:MM display pack
`total = ctx[+0xe4][+0x48] + ctx[+0xe4][+0x44]` (elapsed seconds, two float components summed).
hours = int(total/3600) (const 0.00027777778 = 1/3600), minutes = int((total − hours*3600)/60)
(const 0.016666668 = 1/60). Packed `((hours & 0x1f) << 6 | minutes & 0x3f) << 3` into the display
field at `[DAT_00c809d4+0x30]` preserving other bits (mask 0xffffc007). The 8388864.0 bias is the
standard float→int truncation trick (2^23 + 256). Confirms the recovered seconds→H:MM clock.

### FUN_0077e850 — period boundary / match finalize
Gate: if `ctx[+0xe4][+0x4c] < 3600.0` (DAT_00c84c7c = 0x45610000 = 3600.0f) AND
`ctx[+0xd8][+0x4b4] < 500000` → keep running (flag 1). Otherwise the period ENDS and it finalizes:
builds the scoreline string via `sprintf("%d-%d-%d", …)`, CRC32-hashes it (FUN_00405530 — the same
commentary/text GID hasher), logs/reports (FUN_00411c10, FUN_0077ddc0 clears a byte), then — gated on
match state `ctx+0x1061c == 1 || == 3` and FUN_0048b480 — runs the end-of-match transition
(FUN_004918c0 / FUN_009efb40). So the "GameOver"/boundary step is a clock-threshold finalize that emits
the scoreline as a text key and advances the match lifecycle state. (The 3600.0 threshold is a period
unit; exact half/full-time mapping not pinned here.)
