# STAT → GAMEPLAY LINKAGE — what feeds person+0x3c (the contest attribute)

Clean-room, STATIC only (capstone 5.0.7 over the file; no exe run, no DRM touch, no file edits).
Target: `/Volumes/CM2008/Crack JPN/cm2008.exe` (PE x86, dumped-as-mapped; file_off = VA − 0x400000;
.text at VA 0x401000 / file 0x1000, size 0x710000). Builds on G1_STAT_RESOLVERS.md,
DUEL_RESOLVER_GEOMETRIC_TALLY.md, G1_EVENT_GENERATOR_RECON.md, PLAYERPROPERTIES_SCHEMA_NAMES.md.

Labels: **VERIFIED** (read instruction-level from the crack) · **INFERRED** · **UNKNOWN**.

---

## HEADLINE ANSWER (changes the premise of the open question)

The open question assumed the 7-bit value at `person+0x3c` is **selected by event type** (shot→Finishing,
tackle→Tackling, aerial→Heading, save→GoalKeeper). **That selection does NOT exist in the match
engine.** — VERIFIED.

`person+0x3c` bits 12-18 is a **single, pre-stored 7-bit per-player rating** that lives in the match
person record. It is written by exactly **one** clean bitfield setter, populated primarily by record
copy / deserialization, read **identically for both contestants** by every resolver, and read in **162
places** across the whole match engine (movement, positioning, presentation, resolvers). There is **no
event-type → property-index switch or table** anywhere on the path that writes it. The contest reads one
fixed field regardless of whether the event is a shot, tackle, dribble, header or save.

So the "which of the 77 properties per event" map the task asked for is **not present** — the engine
abstracts each player to a small family of 7-bit ratings (see §5) and the duel/best-of-6/penalty atoms
all consume the same one.

---

## 1. The setter that WRITES person+0x3c — VERIFIED

**`FUN_00581ce0`** (file_off 0x181ce0) — a one-argument `__thiscall` bitfield-merge setter:

```
00581ce0: mov eax, [esp+4]        ; arg = new 7-bit value
00581ce4: shl eax, 0xc            ; << 12
00581ce7: xor eax, [ecx+0x3c]     ; ^ old dword
00581cea: and eax, 0x7f000        ; mask = 0x7f << 12  (bits 12-18, exactly the contest attr)
00581cef: xor [ecx+0x3c], eax     ; dst ^= ((dst ^ (val<<12)) & 0x7f000)
00581cf2: ret 4
```
i.e. `person->f3c = (person->f3c & ~0x7f000) | ((val & 0x7f) << 12)`. The mask `0x7f000` matches the
resolver read `(*(u32)(person+0x3c) >> 12) & 0x7f` bit-for-bit. `ecx` is the person record: its only
"populate" caller resolves `ecx = [0xc77778]+0x24 + id*0x14c` — the **person table 0xc77778, stride
0x14c (332 bytes)**, so this is unambiguously the record the duel reads. VERIFIED.

This setter has **only two call sites** image-wide (byte-searched E8 rel32 → 0x581ce0):

- **0x71eeb2** (in `FUN_0071ee40`): sets the field to the **constant 0xa = 10** on a special match
  entity (id from `[0xc2c6b8]`, then match-context setup via `[0xc80464]`, `FUN_007282c0`…). A default
  initializer for a non-player/ball/special actor — **not** a player-attribute load. VERIFIED.
- **0x7472dd**: `push <val>; mov ecx,<record>; call 0x581ce0`, where `val = FUN_00747050(record,
  [ebp+8])`. `FUN_00747050` **re-reads the record's already-present `+0x3c>>0xc&0x7f`**, walks a group
  of related records reading each one's same `+0x3c` field, and keeps the **maximum** (`mov [esp+0x28],
  al` only `if al > current`). So this caller **aggregates (max over a group)** an attribute that is
  already stored — it does **not** choose a property by event type, and does no 0-100→7-bit scaling.
  VERIFIED.

Neither caller selects a property by event type; neither loads a raw PlayerProperties column. ⇒ the
primary population of `+0x3c` is upstream of these (§3).

Paired accessors on the same struct (same 0x581xxx family, VERIFIED): `FUN_00581cd0` gets `+0xb0`
bits 18-24 (another 7-bit field); `0x581c92`/`0x581ca1`/`0x581cb4` set `+0xb4` 7-bit subfields. The
getter for `+0x3c` bits 12-18 is **inlined** at the 162 read sites, not a standalone function.

## 2. Is attribute selection per-event-type? → NO (generic single field) — VERIFIED

- Only one masked writer (0x581ce0) touches bits 12-18; a full linear resync-sweep of .text for every
  `[reg+0x3c]` store (2502 of them) found **no** other instruction that merges a 7-bit value at shift
  12 with mask 0x7f000 into a person record. No switch/jump-table keyed on an event-type byte feeds it.
- The two setter callers branch on entity/default, not on event type.
- `+0x3c` is a struct-resident field copied wholesale by record clones and the DB snapshot memcpy (§3),
  then read uniformly. There is no map {event type → one of 77 properties}. **UNKNOWN does not apply —
  the mechanism is positively generic.**

**No event-type → attribute-index table was found, because the engine does not use one.** (Reported per
the task's instruction not to fabricate a mapping.)

## 3. How person+0x3c is actually filled (the real population path) — VERIFIED

`person+0x3c` (dword index 15 of the 0x14c record) is populated by **bulk copy**, not computed per
event:

- **`FUN_00593cc0`** (0x193cc0), the contestant→person resolver used by the duel: resolves
  `person = [0xc77778]+0x24 + id*0x14c`, then `mov ecx,0x19; rep movsd` — a **25-dword (100-byte)
  memcpy** from a DB-resolved cached snapshot (looked up via cache `[0xc775b8]` → `FUN_00661710` →
  `FUN_0061e440`) into the caller's buffer. `+0x3c` is inside that 100-byte block, so the contest attr
  is copied verbatim from the per-player snapshot. VERIFIED.
- **`FUN_0058c970`** is a **deserializer**: it unpacks word fields (`+0x18/+0x1c/+0x20/+0x24`) through
  `FUN_005850d0` (a 16-bit swap), reads `+0x3c>>0xc&0x7f`, and **if the value == 0x7f (127) forces it to
  1** (`and 0xfff81fff; or 0x1000`). So 127 is a "no value" **sentinel** clamped to 1; valid range is
  0-126. This proves `+0x3c` is a **stored/serialized** field, not a match-time computation. VERIFIED.
- Object clones copy it wholesale too (e.g. `FUN_00873514`: `mov edx,[edi+0x3c]; mov [esi+0x3c],edx`).

⇒ The property→rating decision happens **once, at squad/player load** (DB → per-player match snapshot),
**upstream of the match engine**, and is baked into `+0x3c` before any contest runs.

## 4. Scaling raw property → 7-bit — VERIFIED (within engine) / UNKNOWN (DB origin)

- **Within the match engine: no scaling.** The setter `0x581ce0` masks `& 0x7f` with no multiply/divide;
  the aggregate `FUN_00747050` copies 7-bit values directly; the deserializer stores directly (only the
  127→1 sentinel clamp). The value is used as a raw 0-126 integer. VERIFIED.
- **DB → 7-bit:** the exact transform from the on-disk PlayerProperties u16 (0-100 / 0-20) to this
  stored 7-bit value happens in the upstream snapshot builder (the writer of the cached block behind
  `[0xc775b8]`), which was **not located on these match-engine paths**. Since 0-100 already fits in 7
  bits (0-127), it is plausibly a direct store or a derived-rating computation, but the precise formula
  is **UNKNOWN** from this static pass.

## 5. What person+0x3c most likely IS (INFERRED)

The person record carries a **family of 7-bit rating fields** — `+0x3c` bits 12-18, `+0xb0` bits 18-24,
`+0xb4` bits 8-14 — each with its own accessor. Combined with: a single `+0x3c` value read in **162**
engine sites, a **max-over-group** aggregate (`FUN_00747050`), and a **127→1 sentinel**, the contest
field reads as a **derived single per-player ability/current-rating for this record**, not as any one of
the 77 named PlayerProperties selected by context. This matches the CM/FM design where match contests
resolve on a condensed per-player rating rather than on individual raw attributes. **INFERRED.**

## 6. Attacker / defender asymmetry → NONE at the field level — VERIFIED

Duel top (`FUN_0077c710`, 0x77c710):
```
push 0; mov ecx,ebp; call 0x5a6170   ; esi = contestant-0 record
push 1; mov ecx,ebp; call 0x5a6170   ; ebx = contestant-1 record
mov ecx,ebx; call 0x593cc0 ; ebx = PERSON A (0x14c)
mov ecx,ebp; call 0x593cc0 ; ebp = PERSON B (0x14c)
...
A = (*(u32)(ebx+0x3c) >> 12) & 0x7f
B = (*(u32)(ebp+0x3c) >> 12) & 0x7f
```
**Both contestants read the identical offset+mask** (`+0x3c` bits 12-18). There is no "attacker reads
Finishing, defender reads GoalKeeper/Marking" — both sides contribute the **same kind** of stored 7-bit
rating; the asymmetry is only in the *values*, never in *which field* is read. The best-of-6 resolver
`FUN_00a029c0` is likewise symmetric: side A and B read the same byte-attr formula (`idx*0x34 +
[0xc80464+0x2c] + 0x2c`, `& 0x7f`, indices 0/1 via `FUN_005ce3e0`; see G1_STAT_RESOLVERS.md pass 5).
VERIFIED.

## 7. One event traced end-to-end — VERIFIED

**Duel / aerial / tackle-class event** (any event the dispatcher routes to the duel atom):
```
event fires → dispatcher FUN_0077cd40 (routes by desc+4/5/6)
  → duel FUN_0077c710
      → FUN_005a6170(0)/(1)          resolve the two contestant records
      → FUN_00593cc0 (×2)            → PERSON A, PERSON B (0x14c, table 0xc77778)
      → A = personA->f3c >>12 &0x7f   (the ONE stored contest rating; same for B)
      → shareA = 0.5·A/(A+B), shareB = 0.5·B/(A+B)
      → geometric tally: for lcg.Unit()<share { bump 5-bit counter }  (DUEL_RESOLVER_GEOMETRIC_TALLY.md)
```
The **same person contest rating** feeds the contest no matter the event type; `person+0x3c` was filled
upstream by the DB snapshot copy (§3). There is no point on this path where an event type selects a
different property. VERIFIED.

---

## VERIFIED vs INFERRED vs UNKNOWN

**VERIFIED**
- Setter of person+0x3c bits 12-18: `FUN_00581ce0` (file_off 0x181ce0), merge `(val&0x7f)<<12`, mask
  0x7f000; operates on person records `[0xc77778]+0x24 + id*0x14c`.
- Only 2 callers: constant-10 init (special entity) and a max-over-group aggregate (FUN_00747050);
  neither selects by event type.
- No event-type → property-index switch/table feeds person+0x3c (confirmed by full .text sweep of all
  2502 `[reg+0x3c]` writes; only 0x581ce0 masks bits 12-18).
- person+0x3c filled by 25-dword memcpy (FUN_00593cc0) from a DB snapshot, and loaded by a deserializer
  (FUN_0058c970) with a 127→1 sentinel; copied by clones.
- No scaling inside the engine (raw 7-bit, 0-126 valid).
- Both duel contestants read the identical +0x3c field; best-of-6 is likewise symmetric → no
  attacker/defender field asymmetry.
- 162 inline read sites for person+0x3c>>0xc&0x7f across the engine.

**INFERRED**
- person+0x3c is a derived single per-player ability/current rating (member of a 7-bit rating family
  with +0xb0, +0xb4), not one of the 77 named properties picked by context.
- The DB property→7-bit value is likely a direct store or a current-ability derivation.

**UNKNOWN**
- The exact upstream function (behind cache `[0xc775b8]` / snapshot builder) that first writes
  person+0x3c from the DB PlayerProperties, and therefore the precise DB-column → contest-rating formula
  and which of the 77 properties (or what blend) produces it. It is not on the match-engine paths and
  was not reached in this static pass; it belongs to the squad/player-load layer.

## Key addresses (VA)
| what | VA | note |
|---|---|---|
| person+0x3c contest-attr SETTER | 0x00581ce0 | `(val&0x7f)<<12`, mask 0x7f000, ret 4 |
| setter caller — const-10 init | 0x0071eeb2 (fn 0x0071ee40) | special entity, not a player attr |
| setter caller — max-over-group | 0x007472dd | val = FUN_00747050 (reads existing +0x3c) |
| aggregate recompute | 0x00747050 | max over a group of records' +0x3c; no scaling |
| contestant→person resolver + memcpy | 0x00593cc0 | person = [0xc77778]+0x24+id*0x14c; `rep movsd 0x19` |
| deserializer + 127→1 sentinel | 0x0058c970 (write 0x58c9ee) | stored field proof |
| duel resolver (reads A,B) | 0x0077c710 | both sides read +0x3c bits 12-18 |
| +0xb0 7-bit getter (sibling rating) | 0x00581cd0 | rating family evidence |
| a029c0 contestant index getter | 0x005ce3e0 | best-of-6 attr indexing (sym., &0x7f) |
| person table base | 0xc77778 | 0x14c stride | 
| contestant table base | 0xc779ac | 0x2c stride |

## B2 RESOLVED — person+0x3c IS the stored CurrentAbility column; CA→attributes, not attributes→CA

The one untraced upstream piece (DB → contest rating) is resolved. person+0x3c bits12-18 is NOT a
mean/aggregate of the 77 attributes — it is the **stored 7-bit CurrentAbility DB column, copied verbatim**
(domain 0..100, 127=unknown→clamped to 1). Proofs:
- **FUN_005f0540 @0x5f0540** registers the person projection column list verbatim: "Nationality",
  "HomeReputation","WorldReputation",**"CurrentAbility","PotentialAbility"** — CA/PA are stored DB columns.
- **FUN_00667b80 @0x667b80** (per-person pass in PostImport FUN_006687c0): reads `CA=(person+0x3c>>0xc)&0x7f`,
  clamps to 100, then for each UNSET attribute field DERIVES it as **`attr = min + (max−min)·CA/100`**
  (applied to person+8,+0x14,+0x28,+0x30,+0x48,+0x4c,+0x50,+0xd4). So the data flow is **CA → attributes**,
  refuting "CA = mean(attributes)". (This is also the non-detailed-player attribute GENERATION formula —
  the mechanism behind the ~83% sentinel skill blocks: a non-detailed player's shown attributes are
  generated from CA via min+(max−min)·CA/100.)
- The only computed transforms ever applied to +0x3c are two runtime post-load adjusters (setter
  FUN_00581ce0 @0x581ce0 has exactly two callers): (1) FUN_0071ee40 @0x71eeb2 hard-codes CA=10 for ~46
  synthetic/placeholder persons; (2) FUN_007471f0/FUN_00747050 @0x7472dd/0x747050 raises a squad member's
  CA to the peer-group lower quartile: `CA = max(own, sorted_peer_CA[floor(0.25·n)])`.
- The verbatim column→+0x3c load itself is a reflective/indirect DB-field write (computed destination
  offset) that the resolved call graph doesn't capture (46 omitted person-table refs); last concrete
  masked instructions are the FUN_00581ce0 merge and the FUN_0058c970 @0x58c9ee 127→1 clamp.
Port note: the match contest rating per player = the stored CurrentAbility (0..100), NOT derived from the
77 attributes; and non-detailed players' attributes = `min+(max−min)·CA/100` per attribute's [min,max].
