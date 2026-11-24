# G1 pass 2–3 — tick orchestration + spotting search (FUN_0077ee80 ProcessData)

ProcessData (FUN_0077ee80, purely-virtual per-tick entry) orchestrates the whole match tick. `cd40`
(the G1-pass-1 decision tree) is one call in this sequence.

## Per-tick orchestration (call order inside ProcessData) — VERIFIED
```
FUN_0077d800        @0x77ef01   // a std::map/BST FIND (see correction below) — NOT the spotter
FUN_0077cf20        @0x77eff7   // cf20 — finalize/event-tree (activate queued events)
FUN_0077ccb0        @0x77f1ec   // ccb0 — gate (freeze/pause)
FUN_0077cd40        @0x77f22e   // cd40 — DISPATCH/RESOLVE the active record (G1 pass 1)
FUN_0077e850        @0x77f27d   // e850 — clock boundary / GameOver
(pending-activation cluster @0x77f2ae–0x77f3d0: 9efa90/9efa40/9efa70, two contestant resolves
   5a6170→593cc0 ×2 — resolves a pending event, same duel inputs as FUN_0077c710)
FUN_0077c660        @0x77f3d3   // c660 — clock repaint (H:MM)
FUN_0077cf20        @0x77f45b   // cf20 again
```

## Pump loop (how records reach cd40) — VERIFIED
- Iterates per-entity records, **stride 0x2768 (10088 B)**, indexed by a 9-bit id (`word[rec]&0x1ff`).
- For each record `esi`: if the **pending bit `[ebp+0x14]&0x1000000` is CLEAR**, call `cd40(esi)`
  (resolve); if SET, take the pending path (5a6170/593cc0 duel cluster).
- cd40 routes by the record's class flags `+4/+5/+6` (pass 1). ProcessData has cpuid/rdtsc profiler
  scopes (Ghidra mis-decodes; crack clean).

## CORRECTION (pass 3): FUN_0077d800 is NOT the spotting phase
Decompiled FUN_0077d800 (140 B): it is a **binary-search-tree / std::map lookup** — traverse from root
`[ebp+4]`, compare key `[ebx]` vs node `[node+0x10]`, go left `[node+8]` / right `[node+0xc]`, until
null; return the node + a found/insert bool written as `[result+4]=0|1`. That `+4` byte is the
iterator's found flag, NOT an event-class flag. So the pass-2 guess "0077d800 = spotting" was WRONG and
is withdrawn. ProcessData calls it early just to LOOK UP a record/entry by id.

## Where the class flags (rec+4/5/6) are actually set — STILL UNLOCATED
Not in ProcessData directly (no `mov byte [rec+4/5/6]` there) and not in 0077d800 (a map find). Open
candidates, by likelihood:
1. **FUN_0077cf20** (cf20, called twice per tick as finalize/activate) — may set up the next tick's
   event classes when it activates/clears events.
2. The **cadence path** — FUN_009d52e0 (`entity[+0x154]+=10s`) gating a per-entity scheduler that
   assigns an event class when an entity's timer fires.
3. The flags may be **static per-entity role bits** (active/on-pitch/special) rather than per-tick
   chosen — in which case cd40 routes by entity TYPE and the "event selection" is the pump iteration +
   the handlers' internal RNG, not a separate spotter.

## cf20 (FUN_0077cf20, 413B) — the event ACTIVATE phase — DECOMPILED (not the flag-setter)
cf20 ACTIVATES/resolves an already-classified event and clears the pending bit:
- optional report if the event type `(5a61c0→5a06c0)` matches `[0xc31b34]` → `FUN_00a2eb00` (reporting).
- `ebx = 5a6330(esi)` (the event's stat/actor block); `5d0460(ebx,3)`; handlers `9efa70 / 9efa40 /
  9efc30`.
- resolves the TWO contestants: `5a6170(esi,0/1) → 593cc0` (each one's attribute) → `661270` (test);
  gated by `69a610([0xc784a8],esi)` (the detailed-match-active gate) → detailed sub-handlers
  `77c540 / 77c5e0`.
- **`and [ebp+0x18], 0xfef7ffff`** — CLEARS the pending-event bit (0x1000000) and bit 0x80000. Matches
  the Go pump ("pending cleared at cf20"). ✓
- final report `5a6320→40adf0(id 0x414)`.
So cf20 consumes + clears events; it does NOT set the class flags rec+4/5/6 either.

## HONEST G1 ASSESSMENT (after 4 passes)
Recovered (the tick SKELETON, VERIFIED): orchestration order, the pump loop (0x2768-stride per-entity
records, 9-bit id), the dispatch router cd40 (routes by rec+4/5/6; sets pending), the activate phase
cf20 (resolves contestants + clears pending), the duel FUN_0077c710 (p=A/(A+B), already ported), the
clock (seconds→H:MM) and the +10s cadence.
STILL NOT recovered — the EVENT-SELECTION CORE: which function writes rec+4/5/6 (the event class per
entity per tick) and how it chooses from game state/tactics. It is NOT 0077d800 (map find), NOT cf20
(activate), NOT in ProcessData directly. It lives behind the heavy indirection (vtable handlers
9efa10/20/40/70, accessors 5a6170/5a6320/5a6330/5a61c0, the scheduler) — a large distributed subsystem.
Pinning + porting it is a deep multi-week effort (the full match AI), not a few more passes. The honest
status: G1 SKELETON done; G1 event-selection CORE is a large open subsystem.
