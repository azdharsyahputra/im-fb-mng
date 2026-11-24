# G1 pass 2 — tick orchestration + the spotting phase (FUN_0077ee80 ProcessData)

ProcessData (FUN_0077ee80, the purely-virtual per-tick entry, called via vtable) orchestrates the
whole match tick. Recovered from the crack. `cd40` (the decision tree from G1 pass 1) is just one call
in this sequence.

## Per-tick orchestration (call order inside ProcessData)
```
FUN_0077d800        @0x77ef01   // early phase — the SPOTTING candidate (sets event-class flags)
FUN_0077cf20        @0x77eff7   // cf20 — finalize/event-tree (activate queued events)
FUN_0077ccb0        @0x77f1ec   // ccb0 — gate (freeze/pause check)
FUN_0077cd40        @0x77f22e   // cd40 — DISPATCH/RESOLVE the active event-record (G1 pass 1)
FUN_0077e850        @0x77f27d   // e850 — clock boundary / GameOver
(pending-activation cluster @0x77f2ae–0x77f3d0: 9efa90/9efa40/9efa70, and TWO contestant resolves
   5a6170→593cc0 ×2 — the same duel inputs as FUN_0077c710, resolving a pending event)
FUN_0077c660        @0x77f3d3   // c660 — clock repaint (H:MM)
FUN_0077cf20        @0x77f45b   // cf20 again
```
(This extends the known ccb0→cd40→e850→cf20→c660 order with the real surrounding phases.)

## The pump loop (how records reach cd40)
- ProcessData iterates per-entity records with **stride 0x2768 (10088 bytes each)**, indexed by a 9-bit
  id (`word[rec] & 0x1ff`), `imul 0x2768`. These big per-entity blocks hold the entity's full match
  state (the 1012-byte geometry block is a small part).
- For each record `esi`: if the **pending-event bit `[ebp+0x14] & 0x1000000` is CLEAR**, it calls
  `cd40(esi)` to spot/resolve; if SET, it takes the pending path (checks `[esi+5]`, resolves via the
  5a6170/593cc0 duel cluster).
- cd40 then routes by the record's class flags `+4/+5/+6` (G1 pass 1).
- Note: ProcessData contains `cpuid;rdtsc` profiler scopes (like GCFormulaModule::Find) — Ghidra
  mis-decodes them; the crack bytes are clean.

## Where the class flags are set (spotting) — NEXT TARGET
No direct `mov byte [rec+4/5/6]` appears in ProcessData itself, so the event-class flags are set by a
sub-call — **FUN_0077d800** (the first phase, before cd40) is the prime candidate for the spotting /
event-scheduling that decides each entity's event class this tick (likely from ball state / position /
tactics + the global RNG). Decompiling FUN_0077d800 is the next G1 pass.

## Status
G1 pass 2 — tick orchestration + pump loop + pending/duel-activation cluster **DECOMPILED/VERIFIED**;
the spotting function (FUN_0077d800) **LOCATED, not yet decoded**. Next: decompile FUN_0077d800 to
recover how event classes are chosen (the heart of causal event selection).
