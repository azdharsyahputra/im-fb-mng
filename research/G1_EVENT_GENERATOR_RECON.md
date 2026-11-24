# G1 event GENERATOR — recon pass 1 (the selection skeleton)

Start of the event-generator workstream (the causal "which atom fires when, between whom"). All
VERIFIED from the crack (capstone) unless noted. This pass maps the per-entity scheduler → predicate
eval thread; it does NOT yet port the generator (a large distributed subsystem — honest scope below).

## Correction: FUN_006bee40 is NOT the generator hub
Decompiled (21 insns): it takes an object, calls 5a1090(obj,0x800)→5a4a20 to get a 16-bit key, then
linearly scans a 3-entry table 0xc293d8..0xc29444 (stride 0x24) for a matching word, returning the
index or 0x800. It is a small **lookup helper** (hence its many call sites), not the event pool hub.
The earlier "hub FUN_006bee40, 137 refs" note is withdrawn.

## Subsystem scale (byte-search of the image for the global immediates)
- event descriptor-array ptr `0xc784a8`: **372** reference sites
- event pool `0xc77df8`: **394**
- contestant table `0xc779ac`: 297 ; person table `0xc77778`: 456
Confirms the generator is a large, distributed subsystem — not siftable by hand; a full port needs the
decompiler (Ghidra) + xref typing to separate writers from readers.

## The per-entity SCHEDULER — FUN_009d52e0 (VERIFIED)
Called from 3 sites (0x9d572f/0x58fc/0x5ab3, same module). `ecx=esi=entity record`. Per tick:
- reads the match clock `[[esi+?].0xe4 +0x48]` and compares it to the entity's next-action time
  `[esi+0x154]`; `setge [esp+0x13]` = "cadence fired" flag.
- walks a per-entity action VECTOR selected by a caller index `edi` (= [esp+0xa98]):
  `begin = [esi + edi*12 + 0x98]`, `end = [esi + edi*12 + 0x9c]` (a {begin,end} pair per index), and
  for each element `ebp` (stride 4): `FUN_009d3e50(ebp)` then `FUN_009d0fb0(this=esi, edi, flag)`,
  storing the returned byte into the element's slot.
- **cadence advance (VERIFIED):** `if [esp+0x13]: [esi+0x154] += 10.0f` (const 0xc5d43c = 10.0) — the
  recovered 10-sim-second entity action tick, GATED by the clock having passed the entity's timer.

## The per-action PREDICATE eval — FUN_009d0fb0 (VERIFIED shape)
Called ONLY by the scheduler (0x9d5374). Returns a **bool** (eligibility). For the action (ebp) + index
(edi): resolves the entity's handle `[esi+0x148]` (via 9efa90) and runs a chain of predicate queries —
`FUN_0048c8c0(h, ebp, edi, &flag)` → `FUN_009d0a00(this,&x)` → if true `FUN_004956e0(ebp,edi)` →
`FUN_0048c470(h, ebp, edi)` — combining their booleans to return 1 (fire) or 0 (skip). These 48c8c0/
4956e0/48c470 are the **GCMatchPredicate** evaluation layer — the same rank-weight predicates whose
table + 80.0 threshold are verified in the Go `predicate` pkg (FUN_0077c540 `fld [0xb5b10c]`=80.0).

## Emerging model (INFERRED from the above, to confirm)
```
per tick, per entity (FUN_009d52e0):
  if matchClock >= entity.nextActionTime[+0x154]:
    for action in entity.actionVector[index edi, +0x98..+0x9c]:
      if FUN_009d0fb0 evaluates the action's GCMatchPredicate as eligible:   # 48c8c0/4956e0/48c470
        → the action fires → (descriptor in the pool) → dispatcher cd40 → resolver atom (duel/a029c0/PK)
    entity.nextActionTime += 10.0
```
So event SELECTION = a per-entity, clock-gated walk of predicate-guarded action slots. The index `edi`
(from the caller) likely selects a phase/zone (cf. the 3×3 GCMatchPredicate zone model). This ties the
cadence (recovered), the predicate table (verified), and the dispatcher/resolvers (ported) into one
causal loop — the first end-to-end sketch of the generator.

## Honest scope / next targets
This is the SELECTION SKELETON, not the full generator. Still open: (1) decompile the predicate-eval
trio FUN_0048c8c0 / 004956e0 / 0048c470 to pin what each action's predicate tests (game state →
eligible event type); (2) find where the eligible action WRITES a descriptor (desc+8 type, desc+4/5/6
class) into the pool — the actual event creation; (3) how the action vector (+0x98) is populated per
entity/zone. Items (1)-(3) span the ~190-fn subsystem and want the Ghidra decompiler (currently slow to
analyze this 11 MB image) for a tractable multi-pass port. Capstone recon continues on the trio next.
