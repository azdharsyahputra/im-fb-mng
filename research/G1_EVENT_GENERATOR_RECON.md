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

---

# Pass 2 — decompiler-driven (Ghidra now working; RTTI+CallConvID disabled, 3G heap)

## The scheduler FUN_009d52e0(group) — DECOMPILED (VERIFIED)
`in_ECX` = match context; `param_1` = a GROUP index. Three callers (0x9d5640 / 0x9d57b0 / 0x9d5a10)
⇒ param_1 ∈ {0,1,2} = **3 zones** (INFERRED: Attack/Midfield/Defence, matching the 3×3 GCMatchPredicate
zone model). Two modes on `ctx+0x150`:

### Mode 0 (ctx+0x150==0) — normal per-tick generation
```
slot = 0
begin = *(ctx + 0x98 + group*0xc)              // action vector {begin,end} per zone
end   = *(ctx + 0x9c + group*0xc)
cadenceFired = (ctx+0x154 <= clock[*(ctx+0x148→).0xe4 +0x48])
for action in [begin,end) stride 4:
    slotResult = FUN_009d3e50(action)           // dest byte for this slot
    *slotResult = FUN_009d0fb0(group, slot)     // eligibility (predicate eval)
    slot++
if cadenceFired: ctx+0x154 += 10.0f             // +10s entity cadence (VERIFIED const 0xc5d43c)
// event BUILDERS (turn eligibility → events):
FUN_0048dbd0(buf); FUN_009d45d0(buf,group); FUN_009d4930(buf,group);
FUN_009d4090(group); FUN_009d41f0(group)
ctx+0x15c+group*4 = FUN_0048c790(group)
ctx+0x164+group*4 = FUN_0048c7c0(group)
```

### Mode 1 (ctx+0x150==1) — record-walk variant
Walks records (FUN_005cdcf0 → FUN_009fdd60), eligibility from record flags (`rec+0x10==-1 ||
rec+0x11!=-1`), state `rec[0x2c]&0x3ffff` (skip 0x20000). Then builders FUN_009d4c60/9d4fc0/9d4090/
9d41f0. (A different source of eligibility; same builder tail.)

## The predicate eval FUN_009d0fb0(group, slot) — DECOMPILED (VERIFIED)
```
FUN_009efa90()                                   // resolve ctx+0x148 handle
c = FUN_0048c8c0(group, slot, &out)              // predicate A: slot-lookup, bool
FUN_009d0a00(buf)
if (c != 0 && FUN_004956e0(group,slot)==0)       // predicate B: u16 state == 0
    FUN_0048c470(group, slot)                    // predicate C: object byte +0x1b3
```
The per-(group,slot) record model (VERIFIED from the trio):
- records are **0x34c (844) bytes**, indexed `group*0x17 + slot` (0x17 = **23 slots/zone**).
- `FUN_0048c8c0`: searches a pointer table `*(ctx+0xd8) + 0x114 + (i + group*0xb)*4` for the record
  address `ctx + (group*23+slot)*0x34c + 0x7e8`; count at `*(ctx+0xd8)+0x48e+group`. Returns the hit
  index → the slot's eligibility key.
- `FUN_004956e0`: reads u16 `*(ctx + 0x760 + (group*23+slot)*2)` — a per-slot STATE/counter; eval
  requires it `== 0`.
- `FUN_0048c470`: follows `*(ctx + (group*23+slot)*0x34c + 0x8d8)` → reads byte `+0x1b3` of that obj.

## What pass 2 nails (VERIFIED) + what's open
NAILED: the generator is a per-tick, per-ZONE (×3) walk of ≤23 action slots; each slot's eligibility is
a 3-predicate test over an 844-byte per-slot record (pointer-table membership + a u16 state==0 + an
object flag); the +10s cadence advances per zone. The 3×3 zone model is now concrete in the data layout
(3 zones × 23 slots).
OPEN (pass 3 — the heart): the **event BUILDERS** FUN_009d45d0 / FUN_009d4930 / FUN_009d4090 /
FUN_009d41f0 (+ FUN_0048c790 / FUN_0048c7c0 result computers) — these consume the eligibility bytes and
WRITE the actual event descriptors (desc+8 type, desc+4/5/6 class) into the pool, i.e. decide which
event fires between whom. Decompiling these four is the next pass. With the decompiler working this is
now tractable (no longer a capstone needle-hunt), though still a multi-pass port.

---

# Pass 3 — CORRECTION: the "builders" are the STATS-EXPORT layer, not the generator

Decompiling FUN_009d45d0 / 9d4930 / 9d4090 / 9d41f0 shows they do NOT create events. They **copy
resolved per-player match stats into the front-end (FE) match interface** for display. Proof — the
literal strings in FUN_009d4090:
> "Storing (from Resolver, squadnum %i) player output data in FE match interface for PersonNameID %u"
> "Storing (from Report, squadnum %i) player output data in FE match interface for PersonNameID %u"

What they actually do (VERIFIED):
- **FUN_009d45d0**: reads ~33 per-player stat getters (FUN_004954xx–004956xx) and stores each into an
  FE field slot via FUN_0061fa70(&DAT_00c99bXX). These DAT_00c99bXX are the named match-stat fields.
- **FUN_009d4930**: computes percentage stats and registers field NAMES via FUN_00405530 —
  "PassesFailedAsPercentage", "TacklesLostAsPercentage" (ratio = made/attempts*100, with the 2^32
  unsigned→float fixup). Same schema-registration idiom as PlayerProperties.
- **FUN_009d4090**: iterates the squad's player handles (PersonNameIDs) and stores each player's output
  block into the FE interface ("squadnum", "PersonNameID"). Mode 1 = "from Report" (saved), mode 0 =
  "from Resolver" (live).
- **FUN_009d41f0**: per-player, picks a value (0x16 = default, else an attribute via the ctx+0xd8
  pointer table) into the FE field.

## Correction to pass 2
FUN_009d52e0 is the **per-squad / per-player STATS-REPORTING scheduler** (publishes resolver output to
the UI), NOT the event generator. Consequently:
- `ctx+0x98 + squad*0xc` is a **squad's vector of player handles** (PersonNameIDs), not an "action
  vector"; the ≤23 slots are **squad players** (11 + subs), not event slots.
- `param_1` is a **squad number** (the 3 callers are reporting contexts), not a pitch "zone"; the
  "3×3 zone" reading in pass 2 is withdrawn for this function.
- FUN_009d0fb0 / 48c8c0 / 4956e0 / 48c470 test whether a player **has resolver output to export**
  (eligibility for reporting), not event eligibility.
- The event pool 0xc77df8 is NOT touched here — confirming this is the OUTPUT/reporting side.

## Still-useful artifact from pass 3 (VERIFIED)
The **844-byte (0x34c) per-player match-stat record** (indexed squad*23+player) and the FE stat-field
vocabulary (percentage stats like PassesFailedAsPercentage/TacklesLostAsPercentage, plus the ~33
getters) — i.e. the per-player match-stats model the UI consumes.

## Redirect (where the real generator is)
The export pulls "from Resolver" → the per-tick event generation lives UPSTREAM in the match tick we
mapped in pass 1: ProcessData FUN_0077ee80 → dispatcher FUN_0077cd40 (RNG draw @0x77cd84, routes by
desc+4/5/6 to the duel/presentation) → the resolver atoms. The honest next target is the code that
FILLS the descriptor pool before cd40 runs — i.e. who sets desc+8 (event type) / desc+4/5/6 — reached
from the ProcessData pump and the event-object pool 0xc77df8, NOT from this reporting scheduler.
