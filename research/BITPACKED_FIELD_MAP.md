# CM2008 DB Field Map — Player / Club / Contract

Clean-room, static only. Evidence = committed Ghidra dumps (`docs/ghidra_dumps/`)
+ validation reads of the owner's `extracted_game/DatabaseFiles/*.dat`. No binary
executed; REA `open_binary` not attempted — no PE is present, only committed dumps,
so every code claim below is read from the committed decompilation.

Tags: **VERIFIED** (read from clean disassembly, and/or reproduced against real
records) · **INFERRED** (structure consistent with data but not name-bound by an
accessor/string/schema) · **UNKNOWN**.

---

## 0. HEADLINE FINDING (changes the whole premise) — VERIFIED

The task brief and prior research (`docs/findings/game-database.md`) assume the
owner's `*.dat` records are **bit-packed**. They are **not**. The owner's
`Player.dat`, `Club.dat`, `Contract.dat` are **byte-aligned** records containing
little-endian ints, **IEEE-754 doubles**, and `u8/u8/u16` date triplets.

Proof (reproduced, see `scan_club.py` / `val_*.py`, all against the real files):
- `Club.dat`: an IEEE double `00 00 00 00 00 00 f0 bf` (= **-1.0**) sits at the
  **same byte offset 22** in **100 %** of 6571 records; `YearFounded` as a
  `u16`=2008/2009 at byte **120** in 88 %. A bit-packed stream cannot place a
  byte-identical 8-byte double at a fixed *byte* boundary in every record.
- `Contract.dat`: `u8 day / u8 month / u16 year` date triplets at byte **4** and
  byte **10**; start-date ≤ end-date in **100 %** of records where both are valid.
- `Player.dat`: sequential `u32` id at byte 0; IEEE double at byte **16** in 52 %.

Consequence: the **bit-packed accessors in the committed dumps describe the
RUNTIME in-memory object**, a *different* representation from the on-disk file.
The two do **not** coincide (decoding `Club.dat` with the runtime offsets yields
garbage — e.g. a constant "bank=33"). The committed dumps contain **no reader for
the byte-aligned on-disk format** (see §6, BLOCKED).

So this document has two layers, kept strictly separate:
- **Layer A** (§2–§4): the RUNTIME bit-packed layout — VERIFIED from accessor code,
  but **not** validatable against the owner's files.
- **Layer B** (§5): the ON-DISK byte-aligned layout — VERIFIED/validated against the
  owner's files, but field **names** are mostly UNKNOWN (no name-binding accessor).

---

## 1. Accessor / schema functions found

| addr | kind | table | evidence |
|---|---|---|---|
| `FUN_00a5d0e0` | schema registration (114 descriptors, 111 named) | **Club** | `raw/player_development.c:1`; names match `clubs/club_schema.c` |
| `FUN_00a2f330` | XML exporter w/ explicit bit accessors | **Club** (runtime struct) | `raw/player_development.c:3466` |
| `FUN_00a5f970` | schema (29 fields, byte-offset, doubles) | Club XML *summary* (export-only) | `raw/player_development.c:3234` |
| `FUN_00a84df0` | schema registration (41 fields) | **Player/Staff** attribute record | `raw/players.c:884` |
| `FUN_00a30d00` / `FUN_00a27ca0` / `FUN_00a28620` | exporters w/ 7-bit accessors | Player/Staff (runtime) | `raw/players.c:399 / 712 / 462` |
| `FUN_00a72db0` | schema registration (match replay) | *not* a DB table | `raw/players.c:1` (the brief's pattern example) |
| `FUN_00405530` | type-tag interner (`"uint32"`,`"sint32"`,`"GCStringID"`,…) | all schemas | called before every descriptor |

**Descriptor shape (VERIFIED, 24 bytes / 6 dwords)** at `FUN_00a5d0e0` base
`0xc69450`, stride `0x18`:
`+0x00` type handle · `+0x04` kind (1=byte-field, 2=bit-field, 3=sized) ·
`+0x08` name ptr · `+0x0c` **A** (declared bit width) · `+0x10` **B** (native size,
almost always 4) · `+0x14` **C** (in-memory byte offset for kind 1/3, else
`0xffffffff`). Parser: `scratchpad/parse_schema.py`. Σ(A)=1559 bits ≠ 1352 (=169 B);
this is **expected** — A is the runtime declared width, not the on-disk packing, and
date parts overlap one word (below).

No Contract schema-registration function was located in the committed dumps
(§6 BLOCKED).

---

## 2. Club — RUNTIME bit-packed accessor (Layer A) — VERIFIED from `FUN_00a2f330`

Pointer `obj` = runtime club struct. Each field = `*(u32*)(obj+off) >> shift & mask`.
Signed money fields use the `*(int*)x * 2 >> 1` idiom = sign-extend bits[0..30]
(bit31 is a separate flag); written `s31` below. **Every row read from
`raw/player_development.c` lines 3531-3694 (addr `0x00a2f330`).** Not validated vs
the owner's file (that file is byte-aligned, §5).

| field | byte off | shift | mask | width | signed | null sentinel | src line |
|---|---|---|---|---|---|---|---|
| (AreaID word) | 0x04 | 0 | 0xfff | 12 | u | — | 3531 |
| NationalClub | 0x04 | 12 | 0x1 | 1 | u | — | 3609 |
| Administration | 0x04 | 13 | 0x1 | 1 | u | — | 3611 |
| BankBalance | 0x08 | 0 | s31 | 31 | s | — | 3613 |
| ClubCoEfficient | 0x0c | 0 | s31 | 31 | s | -1 | 3615 |
| ClubStatusID | 0x10 | 0 | 0x7 | 3 | u | 7 | 3544 |
| GroundCost | 0x14 | 0 | s31 | 31 | s | — | 3624 |
| KitAwayColour1 | 0x18 | 0 | full | 32 | u | — | 3626 |
| KitAwayColour2 | 0x1c | 0 | full | 32 | u | — | 3628 |
| KitHomeColour1 | 0x20 | 0 | full | 32 | u | — | 3630 |
| KitHomeColour2 | 0x24 | 0 | full | 32 | u | — | 3632 |
| Merchandising | 0x28 | 0 | s31 | 31 | s | — | 3634 |
| OutstandingLoan | 0x30 | 0 | s31 | 31 | s | — | 3637 |
| (loan flag, bit31 of +0x30) | 0x30 | 31 | 0x1 | 1 | u | — | 3639 |
| RecordLeagueAttendance | 0x34 | 0 | 0x3ffff | 18 | u | 0x3ffff | 3641 |
| RecordLeagueClubID | 0x38 | 0 | 0xffff | 16 | u | 0xffff | 3542 |
| RLD_Day | 0x38 | 16 | 0x3f | 6 | u | — | 3584 |
| RLD_Month | 0x38 | 22 | 0x1f | 5 | u | — | 3584 |
| RLD_Year | 0x3c | 0 | 0xfff | 12 | u | 0xfff | 3583 |
| Reputation | 0x3c | 12 | 0x7f | 7 | u | — | 3651 |
| SponsorAmount | 0x48 | 0 | s31 | 31 | s | — | 3657 |
| SE_Day (SponsorEnd) | 0x4c | 0 | 0x3f | 6 | u | yr all-1 | 3589 |
| SE_Month | 0x4c | 6 | 0x1f | 5 | u | | 3589 |
| SE_Year | 0x4c | 11 | 0xfff | 12 | u | 0xfff | 3589 |
| SS_Day (SponsorStart) | 0x4c | 23 | 0x3f | 6 | u | yr all-1 | 3595 |
| SS_Month | 0x50 | 0 | 0x1f | 5 | u | | 3594 |
| SS_Year | 0x50 | 5 | 0xfff | 12 | u | 0xfff | 3594 |
| TicketPrice | 0x50 | 17 | s(11) | 11 | s | — | 3665 |
| Training | 0x54 | 0 | 0xff | 8 | u | — | 3667 |
| TVRevenue | 0x58 | 0 | s31 | 31 | s | — | 3668 |
| MostAppearances | 0x60 | 0 | 0x3fffffff | 30 | u | 0x3fffffff | 3673 |
| MostGoals | 0x68 | 0 | 0x3fffffff | 30 | u | 0x3fffffff | 3678 |
| YouthAccademy | 0x68 | 30 | 0x1 | 1 | u | — | 3682 |
| YouthDevelopment | 0x6c | 0 | 0x7f | 7 | u | 0x7f | 3684 |
| HomeMatchDayID | 0x6c | 7 | 0x3f | 6 | u | 0x3f | 3543/3688 |
| YearFounded | 0x6c | 13 | 0xfff | 12 | u | 0xfff | 3691 |
| SeasonTicket(count) | 0x70 | 0 | 0x3fffffff | 30 | u | — | 3603 |

**Field boundaries / overlaps (VERIFIED):** word `+0x38` packs
RecordLeagueClubID(0-15) · RLD_Day(16-21) · RLD_Month(22-26). Word `+0x3c` packs
RLD_Year(0-11) · Reputation(12-18). Word `+0x4c` packs SE date(0-22) + SS_Day(23-28).
Word `+0x6c` packs YouthDevelopment(0-6) · HomeMatchDayID(7-12) · YearFounded(13-24).
These overlapping bit ranges are the "overlap" the brief asked for.

Getter/setter pairs: only getters (read-side exporter) were recovered; the setter
side (bit-packer) was not located — tagged UNKNOWN. Caller chain: `FUN_00a2f330`
is the per-record XML row writer invoked by the table export walk (callers not in
the trimmed dump — UNKNOWN).

Fields 44-110 of the 111-field schema (TransferBudget, WageBudget, Income*,
Expenditure*, Board*, Scout*, Weak*, …) are **not** read by this exporter →
their runtime offsets are **UNKNOWN** (accessor not in dumps).

---

## 3. Club — schema names/order/width (Layer A) — VERIFIED from `FUN_00a5d0e0`

111 field names + order + type tag + declared bit width **A**, extracted by address
(`scratchpad/parse_schema.py`). Matches `clubs/club_schema.c` verbatim. Selected:

| idx | name | type | A (bits) |
|---|---|---|---|
| 0 | BankBalance | uint32 | 3 |
| 4 | NationalClub | sint32 | 31 |
| 21 | Reputation | GCStringID(kind1) | 4 (byte) |
| 33 | TVRevenue | uint32 | 30 |
| 41 | YearFounded | uint32 | 12 |
| 43 | TransferBudget | uint32 | 31 |
| 44 | WageBudget | uint32 | 31 |
| 93 | Defunct | kind3 | 16 (native 2) |
| 105 | IsDeleted | uint32 | 21 |

Full 111-row table is `CM_CLUB_FIELD_NAMES` in `src/database/club_fields.c`.
Note A is the *declared* width and is **not** a usable on-disk offset (it does not
match the runtime exporter widths in §2 either — e.g. YearFounded A=12 matches, but
Reputation A=4 vs runtime 7). Treat A as advisory, VERIFIED-as-registered only.

---

## 4. Player / Staff — RUNTIME 7-bit accessors (Layer A) — VERIFIED

Player/staff attribute records store 7-bit attributes packed 3-per-dword and read as
`in_ECX[word] >> shift & 0x7f`, sentinel `0x7f`. VERIFIED examples:

| field | expr | src |
|---|---|---|
| PotentialAbility | `in_ECX[5] & 0x7f` | `raw/players.c:423` |
| (attr) | `in_ECX[5] >> 7 & 0x7f` | 425 |
| (attr) | `in_ECX[5] >> 0xe & 0x7f` | 429 |
| CurrentAbility | `in_ECX[0xc] & 0x7f` | 543 |
| (attr) | `in_ECX[0xc] >> 7 & 0x7f` | 545 |
| (attr) | `in_ECX[0xc] >> 0xe & 0x7f` | 549 |
| (attr) | `in_ECX[0xc] >> 0x15 & 0x7f` | 553 |

Schema `FUN_00a84df0` (41 fields, `raw/players.c:884`): PersonID, TrainingStyleID,
FormationID, MarkingStyleID, PlayingMentality, PassingStyleID, PressingStyleID,
PlayOffsideTrap, PlayRoamingPlayers, Determination … CurrentAbility, PotentialAbility
(full list in `DATA_MODEL_CATALOG.md`). Declared widths (A): PersonID=0xc,
TrainingStyleID=3, FormationID=3, PlayingMentality=3, PlayOffsideTrap=7, etc.
These are the runtime attribute values; the *exact* word/shift per named attribute
beyond CurrentAbility/PotentialAbility is **UNKNOWN** (exporters interleave several
record variants; name↔shift binding not resolved).

---

## 5. ON-DISK byte-aligned layout (Layer B) — VERIFIED / validated vs owner files

Container (VERIFIED, all three): `"DATa"` magic + `u32 count` (LE) + `count` fixed
records, record size `= (filesize-8)/count`. Player=41 B/85035, Club=169 B/6571,
Contract=29 B/109484 — exact division confirmed. Field **names** here are UNKNOWN
unless marked (no byte-format accessor exists in the dumps to bind names), so
semantic names are deliberately withheld per the brief's rule.

### 5.1 Contract.dat (29 B) — structure VERIFIED, names INFERRED
| off | type | content | confidence | validation |
|---|---|---|---|---|
| 0 | u32 | record id (ascending: 9,25,27,31…) | VERIFIED struct | monotone across file |
| 4 | u8,u8,u16 | **date A** (day,month,year) — contract **start** | VERIFIED date; "start" INFERRED | day∈1-31,mon∈1-12,yr∈1980-2030; A≤B in 100 % |
| 8 | u16 | optional (16 % nonzero) | UNKNOWN | — |
| 10 | u8,u8,u16 | **date B** (day,month,year) — contract **end** | VERIFIED date; "end" INFERRED | 47 % valid; B≥A |
| 14 | u16 | optional (16 % nonzero) | UNKNOWN | — |
| 16 | u32 | entity link (≤11381) | INFERRED (id-like) | 1148 distinct |
| 20 | u32 | entity link (≤159290, 7711 distinct) | INFERRED (person-like) | — |
| 24 | u32 | entity link (≤11381) | INFERRED (id-like) | 1193 distinct |
| 28 | u8 | flag (0 in 99.9 %, else 1) | INFERRED (IsDeleted-like) | — |

### 5.2 Player.dat (41 B) — structure VERIFIED, names UNKNOWN
| off | type | content | confidence |
|---|---|---|---|
| 0 | u32 | record id (sequential) | VERIFIED struct |
| 4 | u32 | small enum / `0xffffffff` sentinel | VERIFIED struct |
| 8 | u32 | small value (≤~50) / sentinel | VERIFIED struct |
| 12 | u32 | small value / sentinel | VERIFIED struct |
| 16 | f64 | IEEE double (often 1.0 / -1.0) | VERIFIED struct |
| 24 | u32 | small / sentinel | VERIFIED struct |
| 28 | u32 | small / sentinel | VERIFIED struct |
| 32 | u8 | flag / `0xff` | VERIFIED struct |
| 33 | u32 (unaligned) | value / sentinel | VERIFIED struct |
| 37 | u8[4] | four small bytes (~50-63, rating-like) | INFERRED |

### 5.3 Club.dat (169 B) — doubles/years located, full map partial
| off | type | content | confidence |
|---|---|---|---|
| 0 | u32 | record id/string id | VERIFIED struct |
| 8 | u32 | small enum (0x21=33 common) | VERIFIED struct |
| 14 | f64 | financial double (e.g. 4,836,062.0) | VERIFIED struct |
| 22 | f64 | constant **-1.0** (reserved/unused) | VERIFIED (100 %) |
| 38 | f64 | financial double | VERIFIED struct |
| 62 | f64 | financial double | VERIFIED struct |
| 74 | f64 | financial double | VERIFIED struct |
| 110 | f64 | financial double | VERIFIED struct |
| 120 | u16 | **YearFounded** (2008/2009) | VERIFIED value; name INFERRED by range+uniqueness |
| 130 | f64 | small double (~17-18, coefficient-like) | INFERRED |
| 139 | f64 | -1.0 (95 %) reserved | VERIFIED |
Offsets between the located doubles hold mixed 1/2/4-byte fields not individually
mapped — **UNKNOWN** (needs the byte-format accessor, §6).

---

## 6. Correction of the Go decoders (`super-manager-engine`)

- `club/club.go`: container + 111 names/order are correct (VERIFIED). But its
  `readLE` (uint32 LE, width 1/2/4) and the premise "records are bit-packed … no
  recovered widths" are **both wrong for the owner's file**: the file is byte-aligned
  and its money fields are **IEEE doubles**, not uint32 — a uint32 read of
  `BankBalance` returns nonsense. The field *indices* (`IdxBankBalance=4` …) are
  positions in the name list, not byte offsets, and must not be used as offsets.
- `contract/contract.go`: FieldNames were explicitly guessed. Structurally the file
  *does* carry two dates + flag (so "ExpiryDay/Month/Year" and "IsDeleted" have real
  counterparts), but `WeeklyWage` as field idx 2 does not match — the three u32s sit
  at bytes 16/20/24 and read as entity links, and the dates are byte/byte/u16, not
  separate fields. Order/widths remain INFERRED.
- `player/player.go`: a formula model, not a `.dat` decoder — unaffected.

## 7. BLOCKED

- **On-disk ↔ runtime correspondence: cannot validate.** The committed accessors
  (`FUN_00a2f330`, player exporters) target the runtime bit-packed struct; the owner's
  files are byte-aligned. Reading the files with the runtime offsets yields garbage.
- **Byte-aligned on-disk reader not in dumps.** No function in the committed dumps
  reads doubles/dates at the owner-file offsets, so on-disk field *names* cannot be
  name-bound (only structure is VERIFIED). Resolving names needs either that loader
  decompiled, or runtime observation (Windows) — out of scope (static only, no exec).
- **Setters / bit-packers:** not located (read-side exporters only).
- **Contract schema-registration function:** not found in the committed dumps.
- **REA:** no PE available (committed Ghidra dumps only); `open_binary` not run.
