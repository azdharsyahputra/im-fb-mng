# CM2008 Player Attribute Mapping — WORKSTREAM 2

Clean-room, **static only**. No binary executed, no DRM bypass. Sources: committed
Ghidra dumps (`docs/ghidra_dumps/`), the owner-decrypted plaintext
`extracted_game/DataXML/formula.xml`, the owner `PlayerProperties.dat`, and the owner
Go loader `super-manager-engine/staff/staff.go`. Every claim tagged
VERIFIED / INFERRED / UNKNOWN. No semantic name is inferred from an offset alone.

## TL;DR

- The **105 attribute getters** harvested by `FUN_005d6dd0` are **recovered** (names,
  addresses, class, derived/stored) — **VERIFIED**.
- The getter -> accessor -> offset -> column_index chain is **BLOCKED past the getter**:
  the per-attribute leaf accessors are not decompiled in any committed dump and
  `CM2008.exe` is absent, so no **column index** can be VERIFIED.
- **94 / 105** getters are **DERIVED** (computed from 2-3 stored leaves via `formula.xml`)
  and therefore map to **no single column**. Only **9** are single-leaf stored reads.
- A **72-name stored-attribute set** is recovered (67 double-confirmed by two independent
  plaintext sources); the on-disk **column order is UNKNOWN** (no code anchor).

## 1. VERIFIED facts (byte / plaintext level)

### 1.1 On-disk format — `PlayerProperties.dat` (VERIFIED)
- File: `extracted_game/DatabaseFiles/PlayerProperties.dat`, 12,016,066 bytes.
- 8-byte header: `[0:4]` magic/tag, `[4:8]` = record count **76051**.
- Then 76051 records x **158 bytes** = `u32 player_id + 77 x u16` (LE, byte-aligned).
- Sentinel `0xFFFF` = not-set; valid values observed in **[0..100]** (0 cols exceed 100).
- Cross-checks: `(12016066-8)/76051 == 158`; owner loader `staff.go` `PropAttrCount=77`,
  `AttrVec{ID uint32; Attrs [77]uint16}`.

### 1.2 The 105 getters & the harvester (VERIFIED)
- `FUN_005d6dd0` (`__thiscall`, 2nd arg = `Player*`), size **56 bytes** in
  `functions.csv` — a small **table-driven loop**, not 105 inline calls.
- Its 105 harvested getters = the `PlayerConstant`(101) + `PlayerDegradeable`(2) +
  `PlayerFluid`(1) + meta(1) formulas routed to it, per `FORMULA_CONSUMER_MAP.md`
  (source `scratchpad/getters_full.txt`, 810 rows, defaults byte-checked). Getter
  addresses recovered (e.g. `FUN_00819e30` BodyBalance, `FUN_00820090` Acceleration).
- **Class/ownership**: all 105 take a `Player*`; their formula **group** is the class
  (PlayerConstant / PlayerDegradeable / PlayerFluid). Full list in
  `PLAYER_ATTRIBUTE_MAPPING.csv` (`getter_addr,getter_name,class`).

### 1.3 Attribute-name enumeration (VERIFIED, decompiled)
A debug CSV-exporter in `docs/ghidra_dumps/match_player/decompiled.c` (lines ~6250-7820)
dumps every per-player attribute **in enum order with its literal name**, each paired
with a consecutive 1-byte stack slot. Two contiguous arrays:
- **Block A** (idx 0..60, `uStack_183c`↓, 61 entries): outfield view — `eHeading,
  eTackling, ... eForward, eDeadBalls`.
- **Block B** (idx 61..104, `uStack_29d0`↓, 44 entries): goalkeeper view — shares
  mental/physical, swaps outfield tech for `eHandling, eKicking, eOneOnOnes,
  eShotStopping, eThrowing, eCrosses, eCoordinatingDefense, eEccentricity`.
- 61 + 44 = **105**, matching the harvester count. This is the authoritative
  **attribute name + order** evidence (this in-memory layout != the on-disk u16 record).

### 1.4 Derived vs stored (VERIFIED from `formula.xml`)
`formula.xml` defines each PlayerConstant getter as a `<result>` over `<term>`s whose
leaves are `<variable ... type="GCPlayer"><variable name="X" type="int"/>`. The leaf
`X` names are the **stored** attributes; multi-leaf results are **DERIVED**.
- **DERIVED: 94/105** (e.g. `BodyBalance = f(Balance,Dribbling)`,
  `BlockTackleStrength = f(Bravery,Determination,Strength)`,
  `KeeperGrip = f(Concentration,Handling)`).
- **STORED (single leaf == own name): 9/105** harvested: Acceleration, Adaptability,
  Aggression, Ambition, Height, LeftFoot, Professionalism, RightFoot, Versatility.
  (`formula.xml` has 11 such single-leaf attrs overall; Confidence & Pace are routed to
  a different consumer, not `FUN_005d6dd0`.)
- **COMPUTE-ONLY: 2/105** (no int leaf — locomotion/physics, e.g. `DegreesControlStepLeft`).

## 2. BLOCKED: getter -> accessor -> offset -> column_index

The required chain cannot be completed past the getter. Missing artifacts:
- **Per-attribute leaf accessors** (the `FUN_0081xxxx` bodies that read `*(u16*)(Player
  + offset)`): present only as `addr,name,size` rows in every `functions.csv`; **no
  decompiled body** in any `.c` dump (`big/`, `raw/`, `match_player/`, `resolver*/`).
- **The 77-column field registrar**: not in the committed dumps (unlike the person/player
  parent records, recovered from `FUN_00a84df0`/`FUN_00a7c540`). The only name-
  registration sequences present are training categories, not the attribute columns.
- **`CM2008.exe`**: absent (gitignored; only a `DotNetInstaller.exe` placeholder on disk),
  so REA cannot open the PE to decompile the accessors on demand. **REA PE route = blocked.**

Therefore: `accessor_addr`, `offset`, `bit_range`, `column_index` = **UNKNOWN** for all
rows. The enum dump (1.3) proves names/order in an in-memory layout whose **62-byte
stride / 1-byte fields differ from the on-disk 158-byte / u16 record**, so its order
does NOT establish on-disk column indices. Per directive: no name is pinned to an index.

## 3. INFERRED: on-disk column structure (data route)

Per-column distribution over all 76051 records (sentinel-excluded):

| Region | Cols | Populated | Signature | INFERRED meaning |
|---|---|---|---|---|
| R1 | 0-17 | 48-86%% | heterogeneous scales; col1 mean~90, col7 mean~1 (max79), col13 near-continuous mean~6 | mixed meta + footedness/position/universal fields |
| R2 | 18-60 | ~17.4%% (13236), **94.9-100%% co-populated (atomic)** | 43 uniform cols, 0-100, means 50-65 | the **43 core shared attributes** (tech+mental+physical+hidden); count matches enum idx 0-42 |
| R3 | 61-76 | 0.2-1.2%% | cols 63-76 **max=15** (1-15/1-20 scale); 61-62 to 100 | sparse specialist (goalkeeper / special) candidates |

- R2 being one atomic 43-column block (a player either has all or none) is the strongest
  structural signal: **INFERRED** = the 43 non-positional core attributes. Internal
  **order within R2 is UNKNOWN**.
- Weak single-column INFERENCE: col 1 (mean ~90, consistent with most players strongly
  right-footed) is a **RightFoot** candidate — **INFERRED-low**, not pinned.

## 4. Stored attribute NAME set (evidence-backed; index UNKNOWN)

72 distinct stored-attribute names, triangulated from two independent plaintext sources.
`confirmed` = present in BOTH the decompiled enum dump AND a `formula.xml` GCPlayer int
leaf (67 names). These are the candidate `PlayerProperties.dat` columns (names only).

| # | stored name | provenance | #derived getters consuming it |
|--:|---|---|--:|
| 0 | Heading | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 1 | Tackling | VERIFIED-name (dump-enum + formula-leaf) | 6 |
| 2 | Dribbling | VERIFIED-name (dump-enum + formula-leaf) | 4 |
| 3 | Crossing | VERIFIED-name (dump-enum + formula-leaf) | 5 |
| 4 | Finishing | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 5 | Marking | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 6 | FirstTouch | VERIFIED-name (dump-enum + formula-leaf) | 4 |
| 7 | ThrowIns | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 8 | Passing | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 9 | Anticipation | VERIFIED-name (dump-enum + formula-leaf) | 6 |
| 10 | Creativity | VERIFIED-name (dump-enum + formula-leaf) | 5 |
| 11 | DecisionMaking | VERIFIED-name (dump-enum + formula-leaf) | 6 |
| 12 | Movement | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 13 | Positioning | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 14 | TeamWork | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 15 | WorkRate | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 16 | Concentration | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 17 | Vision | VERIFIED-name (dump-enum + formula-leaf) | 4 |
| 18 | FitnessStat | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 19 | Acceleration | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 20 | Balance | VERIFIED-name (dump-enum + formula-leaf) | 8 |
| 21 | Jumping | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 22 | Pace | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 23 | Stamina | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 24 | Strength | VERIFIED-name (dump-enum + formula-leaf) | 5 |
| 25 | LongShots | VERIFIED-name (dump-enum + formula-leaf) | 2 |
| 26 | InjuryResistance | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 27 | Agility | VERIFIED-name (dump-enum + formula-leaf) | 7 |
| 28 | InjuryRecoveryRate | dump-enum only | 0 |
| 29 | Determination | VERIFIED-name (dump-enum + formula-leaf) | 4 |
| 30 | Professionalism | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 31 | Pressure | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 32 | Loyalty | dump-enum only | 0 |
| 33 | Sportsmanship | VERIFIED-name (dump-enum + formula-leaf) | 5 |
| 34 | Consistency | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 35 | Flair | VERIFIED-name (dump-enum + formula-leaf) | 4 |
| 36 | Leadership | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 37 | Aggression | VERIFIED-name (dump-enum + formula-leaf) | 5 |
| 38 | Bravery | VERIFIED-name (dump-enum + formula-leaf) | 4 |
| 39 | Confidence | VERIFIED-name (dump-enum + formula-leaf) | 7 |
| 40 | Versatility | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 41 | Adaptability | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 42 | Ambition | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 43 | LeftFoot | VERIFIED-name (dump-enum + formula-leaf) | 12 |
| 44 | RightFoot | VERIFIED-name (dump-enum + formula-leaf) | 10 |
| 45 | RightSide | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 46 | LeftSide | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 47 | Central | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 48 | GoalKeeper | dump-enum only | 0 |
| 49 | Sweeper | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 50 | CentralDefender | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 51 | DefensiveMidfielder | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 52 | Midfielder | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 53 | AttackingMidfielder | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 54 | Striker | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 55 | WingBack | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 56 | Roaming | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 57 | FullBack | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 58 | Winger | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 59 | Forward | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 60 | DeadBalls | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 61 | Crosses | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 62 | CoordinatingDefense | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 63 | Eccentricity | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 64 | Handling | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 65 | Kicking | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 66 | OneOnOnes | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 67 | ShotStopping | VERIFIED-name (dump-enum + formula-leaf) | 3 |
| 68 | Throwing | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 69 | PenaltyTaking | VERIFIED-name (dump-enum + formula-leaf) | 1 |
| 70 | CurrentAbility | formula-leaf only | 4 |
| 71 | Height | formula-leaf only | 1 |

Notes:
- `dump-enum only` (GoalKeeper, InjuryRecoveryRate, Loyalty): stored/enumerated but not
  used as a formula leaf.
- `formula-leaf only` (CurrentAbility, Height): CurrentAbility is a parent-record
  aggregate (see `docs/DATA_MODEL_CATALOG.md`), likely not a PlayerProperties column.
- 77 columns - ~72 names ≈ **5 unaccounted** columns (candidates: PotentialAbility,
  Home/World reputation, or flags) — **UNKNOWN**.

## 5. Task (6)/(7) — classification sets

- **Stored-column getters (direct): 9** — Acceleration, Adaptability, Aggression,
  Ambition, Height, LeftFoot, Professionalism, RightFoot, Versatility.
- **Derived/computed getters (NO direct column): 96** (94 DERIVED + 2 compute-only).
  Each maps to its formula leaves, not a column (see CSV `semantic_name`).
- **Columns with NO getter**: the ~5 unaccounted columns above + any stored field never
  referenced as a formula leaf (e.g. parent-record mirrors) — **UNKNOWN set**, bounded
  by 77 minus the 72 named leaves.
- **Getters with NO column**: the 96 derived/compute-only getters.

## 6. Ambiguities
- Two distinct "105" sets coincide: (a) the 105 **computed** getters harvested by
  `FUN_005d6dd0` (mostly DERIVED), and (b) the 105-entry **enum dump** of stored/view
  attributes. They are different layers (outputs vs inputs); do not conflate.
- Block A (outfield) vs Block B (goalkeeper) are two **views** of one player record; the
  on-disk record stores the **union**, which is why ~72 names span both.
- On-disk column order is independent evidence from the in-memory enum order; no artifact
  links them. Any index<->name claim beyond the region hypotheses would violate the
  "never infer a name from an offset alone" rule.

## 7. To unblock (future work)
- Decompile the `FUN_0081xxxx` leaf accessors (needs `CM2008.exe` + Ghidra/REA): each
  reads `*(u16*)(Player + off)`; map `off` -> load order from the PlayerProperties
  loader to pin column indices. This converts the 72 names from INFERRED to VERIFIED.
- Or find the field registrar for the 77 columns (pattern of `FUN_00405530("Name")`).

## Files
- `research/PLAYER_ATTRIBUTE_MAPPING.csv` — 105 getter rows (chain, UNKNOWN past getter).
- `src/database/player_attribute_schema.h` — 77-column schema, all `COLUMN_NN_UNKNOWN`.
