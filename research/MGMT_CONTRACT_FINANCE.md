# CM2008 MANAGEMENT core — contracts, finance, transfers, board

Static reverse-engineering (clean-room). Sources: crack `cm2008.exe` (PE x86, dumped-as-mapped,
`file_off = VA − 0x400000`) + extracted `.dat` files. No exe execution, no DRM touched, no files modified.
Method: capstone 5.0.7 disassembly + raw string/byte-ref scans. Builds on the already-VERIFIED
Club.dat (recsize 169) and Contract.dat (29B) rows in `PORT_VS_CRACK_VERIFICATION.md` — not re-verified here.

Confidence tags: **VERIFIED** = proven by byte math / cross-ref / disasm; **INFERRED** = strong evidence,
not proven; **UNKNOWN** = not determined. Field offsets in `.dat` are FILE-record offsets; offsets with
`[esi+..]` are IN-MEMORY object offsets (different layout from the file).

---

## 1. CONTRACT `.dat` layouts

All contract tables share the **Contract id** key (`Contract.dat @0`, u32, range 9..251758).
`filesize = 8 + count*recsize` holds for every table below.

### Contract.dat — 109484 × 29B  (employment record; prior-VERIFIED, repeated for context)
`id@0:u32 | StartDate@4 | EndDate@10 | Club@16:u32 | Person@20:u32 | Squad@24:u32 | Flag@28:u8`
Date encoding (VERIFIED by decode): `day:u8, month:u8, year:u16, pad:u16` (6B). e.g. rec0 Start=`01 07 d3 07`=1/7/2003.
Link refs confirmed by schema map: @16→Club.dat, @20→Person.dat, @24→Squad.dat. Flag set 1.48%.

### ContractJob.dat — 109663 × 8B  **VERIFIED**
`contractId@0:u32 | job@4:u32`
- @0 = Contract id (distinct 109484 = exactly Contract.dat count; 179 dup rows → a few contracts hold 2 jobs).
- @4 = **Job.dat id**, observed range 0..18 = exactly the 19 rows of `Job.dat`. Distribution: value 3 dominates
  (83574, = "player"), 8 (9821), 0 (3749), 2, 7, 11… So this table assigns each contract its ROLE
  (player / manager / coach / physio / …). Which numeric id = which role is in `Job.dat` (see §1.6). **VERIFIED** structure.

### ContractDetail.dat — 101659 × 51B  (optional contract extras; VERIFIED types, INFERRED semantics)
`id@0:u32 | f_small@4:f64 | f_big@12:f64 | @20:u32 | link0@24 link1@28 link2@32 link3@36 :u32 | @40:u32 | @44:u16 @46:u16 @48:u16 @50:u8`
- @0 = Contract id (distinct 101659; a subset of contracts — only those with detail rows).
- **@4 f64** = money, −1.0 sentinel in 96.9% of rows; non-sentinel 3189 rows, range 22.5 .. 131000, median 2763.
  Scale = weekly/periodic figure → **weekly wage (or appearance money)**. INFERRED.
- **@12 f64** = money, −1.0 sentinel in 96.8%; non-sentinel 1279 rows, range 0.9 .. **103,629,899**, median 750k.
  Large lump sum → **signing-on fee / total value / minimum-fee-release clause value**. INFERRED.
- **@24/@28/@32/@36** = four u32 slots, ≈100% = `0xffffffff` (empty); rare real small ints (1,2,3).
  → four optional **clause/bonus link or option slots** (almost always unused). INFERRED.
- @20 small enum {0,0xffffffff,1,6,4,2}; @40 small enum (10 distinct incl. sentinel); @44:u16 (0xffff default,
  else small ints 1..~13); @46:u16 enum {0,1,2,8}; @50:u8 bool (729 set). Exact meanings **UNKNOWN**.
  (@40 is NOT a double — f64 reading gives denormals.)

### ContractClause.dat — 12 × 16B  **VERIFIED types**
`linkId@0:u32(asc) | clauseType@4:u32 | value@8:f64`
- @0 ascending sparse (8554, 8740, 13731, 94198, 108215, 118545 …) → contract/clause link id.
- @4 = clause-type enum (seen 1 and 4).
- @8 = f64 value (fee/amount), distinct 12. Only 12 clause rows exist in this DB (clauses are rare).

### ContractBonus.dat — 4 × 16B  **VERIFIED types**
`linkId@0:u32 | bonusType@4:u32 | amount@8:f64`
- rec examples: (134794, 2, 0.0), (200452, 1, 1000.0), (227703, 1, 24M-ish), (227703, 2, …).
- @4 = bonus-type enum (1,2). @8 = f64 amount/threshold. Only 4 bonus rows in this DB.

### ContractLoan.dat — 1172 × 17B  **VERIFIED types, INFERRED flag semantics**
`contractId@0:u32 | flagA@4:u8 | flagB@5:u8 | flagC@6:u8 | fee@7:f64 | wagePct@15:u16`
- @0 = Contract id (distinct 1172 = record count).
- @4 bool (912 set), @5 bool (733 set), @6 bool (62 set, rare) → loan terms flags
  (e.g. can-be-recalled / can-play-vs-parent / future-fee-agreed — exact mapping INFERRED).
- **@7 f64** = loan fee, −1.0 sentinel for 1150; 22 real values 69 .. 8,500,000, median 758k. **Loan fee**. INFERRED.
- **@15 u16** = percentage, 0xffff default; real values **100, 50, 75, 25, 10** → **% of wage paid by loaning club**. INFERRED.

### Job.dat — 19 × 9B
`id@0:u32(0..18) | nameHash@4:u32 | cat@8:u8`. 19 job roles; ContractJob.@4 indexes this. @8 is a 1-byte
category/group (values 1,2,3…). Role names are not stored as ASCII here (hashed/localised). **VERIFIED** structure.

---

## 2. CLUB FINANCE

### 2a. Club.dat file finance slots (6571 × 169B) — INFERRED by value-range
The 6 doubles labelled `Finance[6]` in the Go port sit at file offsets @14,22,38,62,74,110 (non-contiguous;
other club-relation bytes interleave). 210 clubs are all −1 (non-playable placeholders). Measured:

| off | kind | range (non-sentinel) | median | reading |
|----|----|----|----|----|
| @14 | f64 | **−2,753,480 .. 1e8** (CAN BE NEGATIVE) | 12,931 | **Bank balance / cash** (only negative-capable slot = overdraft). INFERRED-strong |
| @22 | f64 | always −1.0 | — | sentinel (Go labels it so). VERIFIED |
| @38 | f64 | 200 .. 6,000,000 | 829 | a budget/small reserve. UNKNOWN which |
| @62 | f64 | 22 .. 10,000,000 | 3,524 | budget. UNKNOWN which |
| @74 | f64 | 2 .. 46,000,000 | 11,676 | large budget (transfer-budget scale). INFERRED |
| @98 | f64 | 1 .. 70,000 | 105 | small count/figure (not a big-money slot) |
| @110| f64 | 766 .. 80,000,000 | 6,611 | large budget (transfer/wage scale). INFERRED |
| @130| f64 | 0.6 .. 110 | 9 | ratio/multiplier (NOT money), 72% sentinel |
| @139| f64 | 2,753 .. 90,000,000 | — | sparse (164 set) big money — debt / takeover value? UNKNOWN |

File stores finance as doubles; the engine keeps live budgets as **integers** (see 2b), so a 1:1 file-offset→label
map cannot be fully proven from values alone. @14 = Bank Balance is the one strong call (negative-capable).

### 2b. In-memory GCClub finance — **VERIFIED from the club debug-dumper**  (func ~`0x0058c970`)
The club text-dump function reads these in-memory offsets against labelled `printf` formats (format VAs in
.rdata). This is a direct, proven field→meaning map of the runtime club object (`esi` = GCClub):

| in-mem field | format string (VA) | meaning |
|----|----|----|
| `[esi+8]` (int) | `Bank Balance: %d` (0xb6319c) | **Bank Balance** |
| `[esi+0x74]` (int) | `Transfer Budget: %d` (0xb63178) | **Transfer Budget** |
| `[esi+0x78]` (int) | `Wage Budget: %d` (0xb63154) | **Wage Budget** |
| `[esi+0x3c] >> 12 & 0x7f` | `Reputation: %d` (0xb63130) | **Reputation** (7-bit, 0..127) |
| `[esi+4] & 0xfff` | `Area ID & Name` (0xb631ec) | Area/nation id (12-bit) |
| `[esi+0x103] & 0xf` | `Financial Status: '%d %s'` (0xb631c0) | **Financial Status** enum (4-bit: rich…bankrupt) |
| `[esi+0xe6] & 1` | `Is Playable` (0xb63104) | playable flag |
| `[esi+4] >> 12 & 1` | `Is National Club` (0xb630e0) | national-team flag |

→ Operationally the finance model is **three money fields — Bank Balance, Transfer Budget, Wage Budget —
plus a Financial-Status bracket and a 7-bit club Reputation.** VERIFIED by the dumper's byte reads.

Finance config schema (wage model) is registered in a reflection table at func `0x00a62de0` (string ctors via
`0x405530`), which binds the config keys **`MinimumSalary`, `SalaryRangeExponent`, `SalaryRangeMultiplier`,
`AnnualSalaryCap`, `ManagerSalary`, `FirstTeamPlayerSalary`, `MarqueePlayerWeeklyWageBudget`,
`PercentTransferRevenue`** (strings @0xb67260..0xb678a0) to typed record offsets. These drive the wage/budget
generation (form `wage ≈ Minimum + Multiplier · base^Exponent`, capped by `AnnualSalaryCap`) — the formula is
data-driven from this record. The exact arithmetic site is **UNKNOWN** (not traced to a single inline expression).

---

## 3. TRANSFERS

### Function VAs (resolved via __FUNCTION__-string push sites → enclosing prologue)
| function | VA | role |
|----|----|----|
| `GCManagerAI::MakePlayerTransferOffer` | **0x0056c330** | AI builds a transfer bid |
| `GCManagerAI::MakeLoanOffer` | **0x0056ab90** | AI builds a loan offer |
| `GCManagerAI::AssessSquad` | **0x00571590** | AI squad-needs assessment (drives offers) |
| `GCTransferModule::ProcessOffer` | **0x007d60c0** | process/accept/reject an incoming offer |
| `GCManagerTransferListModule::ConductPlayerTransferListing` | **0x007ac3f0** | decide transfer-listing |
| `GCManagerTransferListModule::ProcessPotentialListCandidate` | **0x007ac3f0** (same obj) | list-candidate eval |
| `GCContractMonitorModule::ProcessData` | **0x007a49c0** | periodic contract monitor |

### Traced: `MakePlayerTransferOffer` (0x0056c330)  — **VERIFIED control flow, INFERRED money math**
- Entry decodes three packed handles from the arg block: clubA `[esp+0x14c]`, clubB `[esp+0x144]`,
  player `[esp+0x148]` (u16). Each uses the CM null-handle guards (`cmp 0x100000/0x1fffff`, `0x10000/0x1ffff`,
  `0x8000/0xffff`) then `imul idx,stride; add [table+0x2c]` to turn an id into a pointer
  (club stride 0x34, player stride **0x14c**=332). VERIFIED.
- Calls a chain of player/club getters (`0x580740, 0x580a60, 0x580c00, 0x5882c0, 0x5f2850, 0x5e5260, 0x661270,
  0x5e4f30 …`) and at **0x56c524 executes `imul ecx,ecx,0x19660d`** — the **LCG multiplier 1664525** already
  VERIFIED in `PORT_VS_CRACK_VERIFICATION.md`. → the AI bid applies the game LCG to add **random variation**
  to the computed offer. VERIFIED (RNG use); the full fee expression is spread across the getters, **not** a
  single inline formula, so the precise number is **UNKNOWN** without multi-function tracing.

### Traced: `GCTransferModule::ProcessOffer` (0x007d60c0) — **VERIFIED it is a type dispatcher**
A state machine over offer-record fields read via `0x613xxx` accessor getters (GCTransferOffer: asking fee,
offered fee, clause flags). It branches on offer/role type codes (`cmp eax,6/7/8/0x14`) and packed handle tags
(`cmp 0x8000`, `0x40000`, `0x38000`), then scores accept/reject. No single money comparison is the whole
decision; the asking-vs-offer test is inside the getters. Exact accept threshold **UNKNOWN**.

### TransferHistory.dat — 48488 × 35B (ledger, context)
`@8 date (100%), @14→Club (from), @18→Club (to), @23 f64 (fee, distinct 63)`. Records completed transfers.

---

## 4. BOARD

### Board.dat — 5386 × 32B  (per-club board setup, mostly default)
`id@0:u32(asc, all unique) | @4.. = 28 bytes, ~95% 0xff (unset)`
- Only ~250/5386 rows carry any non-sentinel data in @4+; the rest are all-`0xff` → **most clubs use
  DEFAULT board expectations computed at runtime** (from Reputation; see board logic below), not stored here.
- @0 is an ascending unique id in its own space (min 1155, max 163772); it does **not** cleanly map to Club,
  Group or Squad ids (599/1241/849 of 5386 coincidental overlaps). Board records are referenced FROM the club
  side. @4 non-sentinel values (218) are **not** Person ids. Exact field encoding of the populated rows
  (packed expectation targets / takeover flags) is **UNKNOWN** — the table is too sparse to decode by range.

### Board logic — `GCBoardProcessModule::OnCompetitionComplete` (**0x007471f0**) — **VERIFIED control flow**
- Fired when a competition completes. Outer loop walks the competition's clubs (iterators `0x5aa1f0`/`0x5aa3e0`),
  inner loop walks members (`0x5af010`/`0x5af040`), and for each club calls the per-club handler **`0x00747050`**
  (`club, competition`) → returns a bool.
- `0x00747050` unpacks a **7-bit club attribute `[club+0x3c] >> 12 & 0x7f`** — the SAME field the finance
  dumper labels **Reputation** (§2b). It collates these reputation values across the competition's members
  (building a sorted list) to judge a club's finishing position **against the board's reputation-based
  expectation**. VERIFIED that reputation is the board's yardstick; the exact confidence-delta write
  (where a numeric board-confidence value is incremented/decremented) is **UNKNOWN** (not isolated to one store).
- A float threshold constant at `0xb62960` is `fcomp`-compared against the reputation in the dumper path.

### Related board/relationship functions (VAs for follow-up)
| function | VA / string |
|----|----|
| `GCClubAssessProspectiveManagersModule::MakeBoardTransferOfferForUserManager` | str 0x007a5810 |
| `GCClubCollateProspectiveManagersModule::CollateProspectiveManagers` | str 0x007a5938 |
| `GCContractModule::GiveUserNewContract` | str 0xb99ddb (no direct push xref found — likely vtable-dispatched) |
| board/happiness memory classes | `GCContractAgreed::IsMemorable`, `GCClubLackingAmbition::IsMemorable`, `GCNotAgreeingContractTerms::IsMemorable`, `GCTransferListed::IsMemorable` |

Config keys for board/finance behavior seen in .rdata: `Sugar Daddy Bonus`, `--LogSugarDaddyBonuses`,
`NewSeasonTransferBudgetExists/None`, `PercentTransferRevenue`, `BWageBudgetRemaining`, `TransferFund`.

---

## 5. Honest UNKNOWN / not-determined

- **Exact wage formula arithmetic** — config fields (MinimumSalary/SalaryRangeMultiplier/Exponent/AnnualSalaryCap)
  are registered (func 0x00a62de0) and clearly feed a `Min + Mult·base^Exp` model, but the inline math site was
  not traced. UNKNOWN.
- **Precise AI bid fee value** and **ProcessOffer accept threshold** — both distributed across many `0x5exxxx`/
  `0x61xxxx` getters + LCG; no single closed-form captured. UNKNOWN.
- **Club.dat file finance slots @38/@62/@74/@98/@110/@130/@139 individual labels** — only @14 (Bank Balance) and
  @22 (sentinel) are firm; the rest are budget/ratio by range but not individually proven. The runtime
  in-memory budgets (Transfer @+0x74, Wage @+0x78, Balance @+8) ARE proven, but their file-offset sources aren't
  mapped 1:1. UNKNOWN.
- **ContractDetail @4 vs @12 exact labels** (wage vs signing-on vs release) — by scale only. INFERRED, not proven.
- **ContractLoan flag bits @4/@5/@6 and wagePct owner**, **ContractClause/Bonus type enums**, **Board.dat packed
  expectation encoding**, **board confidence-delta store site** — all UNKNOWN.
- **ContractDetail tail enums** @20/@40/@44/@46/@50 — UNKNOWN.

## Function VA quick index
```
GCManagerAI::MakePlayerTransferOffer   0x0056c330
GCManagerAI::MakeLoanOffer             0x0056ab90
GCManagerAI::AssessSquad               0x00571590
GCClub::AddPerson                      0x0058bc70
GCClub debug finance dumper            0x0058c970   (Balance@+8 / TransferBudget@+0x74 / WageBudget@+0x78 / Rep=[+0x3c]>>12&0x7f)
GCTransferModule::ProcessOffer         0x007d60c0
GCManagerTransferListModule::Conduct…  0x007ac3f0
GCContractMonitorModule::ProcessData   0x007a49c0
GCBoardProcessModule::OnCompetitionComplete 0x007471f0  -> per-club 0x00747050
Finance/wage config schema registrar   0x00a62de0
```
