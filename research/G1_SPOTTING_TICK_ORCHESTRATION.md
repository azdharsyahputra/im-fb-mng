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

## Status
Tick orchestration + pump loop — VERIFIED. Spotting phase — **NOT YET FOUND** (0077d800 ruled out).
Next: decompile cf20 (FUN_0077cf20) and check whether it writes rec+4/5/6; and clarify whether +4/5/6
are per-tick event classes or static role bits (fingerprint-scan byte stores to the 0x2768-stride
record's +4/+5/+6). This is honest slow going — a multi-function trace with one lead already ruled out.
