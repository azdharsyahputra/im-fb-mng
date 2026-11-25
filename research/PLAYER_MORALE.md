# CM2008 Player Morale / Happiness model (GCHappy subsystem) — static RE from the crack

Target: `/Volumes/CM2008/Crack JPN/cm2008.exe` (PE x86, dumped-as-mapped). `file_off = VA - 0x400000`.
Method: capstone 5.0.7 + raw string/byte-ref scans. No execution, no DRM bypass, no file edits.
Labels: **VERIFIED** = read directly from the binary; **INFERRED** = deduced from code shape; **UNKNOWN** = not resolvable statically.

> Morale is **runtime state**, not stored in the shipped `DatabaseFiles/*.dat`. The subsystem persists to two
> internal resources `#Database#/HappyMemory.dat` and `#Database#/HappyRecords.dat` (VERIFIED string refs),
> which are not present in the extracted data dir. The static binary gives us the *structure, reason inventory,
> config knobs and update code paths*, but the *numeric per-effect magnitudes* live in those runtime resources
> / in objects built at load time (see UNKNOWN).

---

## 1. Architecture (VERIFIED — RTTI + class strings)

The subsystem is object-oriented C++. Singletons / data modules (RTTI `.?AV...`):

| Class | Role |
|---|---|
| `GCHappinessModule` | top-level morale/happiness driver (`.\Modules\Process\GCHappinessModule.cpp`, assert str 0xba3d0c) |
| `GCHappyRecordModule` / `GCHappyRecord` | per-person happiness record (one per player) |
| `GCHappyMemoryModule` / `GCHappyMemory` | individual time-decaying "memory" events/niggles |
| `GCHappyMedia` | turns morale state into manager-facing mail / concern / transfer-request responses |
| `GCHappyEffectData` + ~60 effect subclasses | polymorphic per-reason effect objects (e.g. `GCPraise`, `GCFined`, `GCHomesick`, `GCBeingDroppedToTheReserves`, `GCTransferListed`) |

~119 happy/transfer/award RTTI classes total; **~70 are `GCHappy*`/effect/response classes** (the "~70 class strings" in the brief). Full RTTI list dumped in session notes.

Each reason has a base effect class plus **QuietChat / Promise response variants** (manager's reply options), e.g.:
`GCBeingDroppedToTheReserves` + `_QuietChatIgnore1` / `_QuietChatNotReady2` / `_QuietChatDoesNotFit3` / `_QuietChatHisTimeWillCome4` / `_QuietChatReservesWillDoYouGood5`;
`GCNotPlayingFirstTeamFootball_PromiseBePatient2` / `_PromiseChanceByEndOfSeason4` / etc.

---

## 2. GCHappyEffect reason inventory (VERIFIED)

### 2a. The effect name/media table — VA range
A static descriptor array of **45 entries × 0x20 bytes** at **VA `0x00B8EBB8` → `0x00B8F158`** (file_off `0x78EBB8`).
Each 32-byte entry = `{ char* effectName; char* localizedText[0..6]; }`. The name pointer is at entry offset 0;
the remaining slots point to **empty (runtime-localized) strings** — so **this table carries NO numeric weights**.
Reason-name string blob itself: `0x00B8E5B0`–`0x00B8EBB0`.

Parallel runtime arrays indexed by the same effect enum:
- name/media table, **stride 0x20**, base held at `*[0xC784A8]+0x2C` (used by fixture analysis).
- effect-object array `GCHappyEffectData*`, **stride 0x34**, base held at `*[0xC803D0]+0x2C` (used by value update).

### 2b. The 45 effect enum entries (table order; NULL + 44 real reasons + 5 meta)

Grouped into reason families:

- **Selection / playing time / role**: `NotInTopTeam`, `DroppedFromPlaying`, `DroppedToReserves`,
  `PlayingOutOfPosition`, `RoleNotBeingFulfilled`, `ChangeToWorseRole`, `PlayerTopTeamFootball`,
  `UnexpectedInclusion`, `BetterRole`, `PlayerPromoted`
- **Match results / team form**: `TeamPerformingWell`, `TeamPerformingBadly`
- **Contract / valuation**: `NewContractNotBeingOffered`, `ContractAgreed`, `ContractDisagreement`,
  `ValueNotRecognised`, `OverEstimatingPersonsValue`, `ClubLowerRepThanPlayer`
- **Transfer / squad churn**: `TxListed`, `TxRequestTurnedDown`, `TakenOffTransferList`, `LoanListed`,
  `GettingGoodOffersTurnedDown`, `StarPlayerSold`, `BuyingPlayersInTheSamePosition`, `PressSpeculating`
- **Club ambition / facilities / recruitment**: `ClubLackingAmbition`, `ClubWinningAwards`,
  `BadTrainingFacilities`, `GoodTrainingFacilities`, `SignedQualityPlayer`
- **Personal / social (other players)**: `Homesick`, `LanguageBarrier`, `OtherPlayersHappy`,
  `OtherPlayerSympathise`, `OtherPlayerTreatedUnfairly`, `OtherPlayerPraise`, `OtherPlayerCriticise`
- **Direct manager interaction**: `Praise`, `Criticise`, `Fined`, `TeamTalk`, `Sympathise`, `WonAward`
- **Meta / system (not reasons)**: `NULL`, `ScheduledMonthlyUpdate`, `IsEffectSetupRequired`,
  `SetupEffect`, `IsEffectMemorable Niggle`, `IsEffectMemorable Event` (strings at `0x00B93BD8`+)

---

## 3. Data structures (VERIFIED — from debug-dump format strings)

### GCHappyMemory (one time-decaying effect instance; dump block @ `0x00B6B640`)
Fields: `Happy Memory ID`, `Start Time`, `Duration (Days)`, **`Effect` (the enum id above)**,
`Decay Delay`, `Gradient`, `Count`, `Break Count`, `State`, `Additional Data 1`, `Additional Data 2`.
→ Each memory carries an **effect id + strength value** (dumped as `Effect: '%d %s'`, %d=value, %s=name)
and the fields to make its contribution **ramp/decay over time** (`Start Time`+`Decay Delay`+`Gradient`+`Duration`).

### GCHappyRecord (per person; dump block @ `0x00B6B7D8`)
Fields: `Happy Record ID`, `Person ID & Name`, `Time Transfer Canceled`, `Time Transfer Requested`,
`Time Concern Voiced`, a list of **Event** memories and a list of **Niggle** memories (`uaNiggle` array),
plus `PromiseID`, `uHappyRecordIndex`, `m_uInitialMorale`.

### The scalar morale value (VERIFIED)
In the person/player runtime struct the baseline morale is a **single byte**:
- `record+0x5C` = Initial Fatigue, `record+0x5D` = **Initial Morale**, `record+0x5E` = Initial Up-For-It
  (seen in dump fn @ `0x005D93F0`–`0x005D9470`).
- Encoding: `0xFF` = sentinel → treated as **1.0** (full, `fld1`); otherwise `byte * 0.003921568 (=1/255)`
  → a **0.0–1.0 float** (scale const @ `0x00B73954` = `0.00392156`). So morale is a 0–255 byte shown as 0–1.

---

## 4. Compute / update logic (function VAs)

| Function (from its log string) | VA | What it does |
|---|---|---|
| **`GCHappyRecordModule::HandleHappyValueUpdate`** | **`0x006DC7F0`** (log push @0x6DC80F) | **Core recompute**: walks the person's HappyMemory linked list (node stride **0x64**; node[0]=next-index masked `0x1FFFFF`, node+0xA bit0 = has-next), resolves each `GCHappyEffectData` object (`id*0x34 + *[0xC803D0]+0x2C`), checks active/expired flags, calls per-effect predicate `0x6DB270(record)`, then applies/removes each effect (`0x6DC0F0`, `0x6DB2F0`, name lookup `0x6D0620`). |
| `GCHappyRecordModule::HandleFixturePostAnalysis` | ~`0x006DD3xx` (log @0x6DD465) | **Match-result → morale**: resolves the just-played fixture (`id<<5 + *[0xC784A8]+0x2C`), iterates squad/effects, builds an effect-id work list (consts 9,0xA,3,5,0x12 pushed), applies post-match effects (`TeamPerformingWell/Badly`). |
| `GCHappyRecordModule::HandleFixturePreAnalysis` | ~ (log @0x6DD15F) | pre-match expectation setup |
| `GCHappyRecordModule::HandleAddHappyEvent[ToClub]` | `0x006DC420` region | inject a new event memory |
| `GCHappyRecordModule::HandleAddHappyNiggle` | `0x006DC66F` region | inject a niggle memory |
| `GCHappyRecordModule::HandleHappyNiggleMemoryUpdate` | `0x006DCA50` region | decay/age niggle memories |
| `GCHappinessModule` init / effect registration | **~`0x006D0600`–`0x006D1830`** | builds the 45-effect table (35 refs to `0x00B8EBB8` here); handles `ScheduledMonthlyUpdate` (@0x6D0A0C), `IsEffectSetupRequired`, `SetupEffect`, `IsEffectMemorable`. |
| `GCHappinessModule::HandleMailResponse` | region w/ log @`0x0078B5BF` | applies manager's QuietChat/Promise reply |

### Inputs that feed morale (VERIFIED from the above)
Match results (fixture pre/post analysis), **team selection / squad status** (dropped, reserves, out-of-position),
**contract & valuation** state, **transfer** events (listed, request turned down, star player sold),
**club ambition / training facilities / awards**, **homesickness & language**, **other players' mood**,
and **direct manager actions** (praise/criticise/fine/team-talk/promise). A `ScheduledMonthlyUpdate` drives
periodic decay/recompute.

### Is morale a weighted sum? (INFERRED)
Yes — **time-decayed weighted accumulation**. Morale = `InitialMorale` baseline mutated by the set of active
`GCHappyMemory` effects, each contributing its stored strength scaled by its decay curve
(`Start Time`+`Decay Delay`+`Gradient`+`Duration`+`Count`/`Break Count`). `HandleHappyValueUpdate` is the
aggregator that iterates the list and applies each effect object's virtual contribution. After each match,
`PostMatchMutation` applies `MutateMoraleScale` (and `MutateConditionScale`, `MutateConfidenceScale`), and
morale drifts back toward baseline at `MoraleRecoveryRate` (the "recovery" over time).

---

## 5. Config knobs (VERIFIED — attribute-registration strings)

Registered under attribute group **`PlayerHappiness`** (reg site @`0x0082EADC`) and **`PostMatchMutation`** (@`0x0084B15C`):

`HappinessUpdateFrequency`, `HappinessStateThreshold`, `HappinessStateThresholdMultiplier`,
`HappinessAmbitionThreshold`, `HappinessTrainingThreshold`, **`MajorConcernThreshold`**,
**`SlightConcernThreshold`**, `MonthsTillHomesick`, `MonthsTillNaturalised`, **`MoraleRecoveryRate`**,
and (PostMatchMutation) **`MutateMoraleScale`**, `MutateConditionScale`, `MutateConfidenceScale`,
`ThresholdMaxMonths`. These are the tunables; their numeric values come from the attributes/config resource
at runtime (not a static const block found in `.rdata`).

---

## 6. Traced path: a reason → morale delta → management (one path)

**Dropped to reserves → unhappiness → transfer request.** (VERIFIED code structure; INFERRED magnitudes)
1. Manager drops a player; game raises `GCHappyEffect::DroppedToReserves` → `GCHappyRecordModule::HandleAddHappyEvent`
   (`0x6DC420` region) creates a `GCHappyMemory{ Effect=DroppedToReserves, Start Time=now, Gradient/Duration set }`
   on that person's `GCHappyRecord`, via effect class `GCBeingDroppedToTheReserves`.
2. `HandleHappyValueUpdate` (`0x6DC7F0`) later walks the memory list, resolves each effect object, and folds its
   (negative) decayed strength into the person's morale scalar (`record+0x5D`, 0–255/0xFF).
3. When morale crosses `SlightConcernThreshold` / `MajorConcernThreshold`, `GCHappyMedia` emits a concern:
   **`ConcernNegativePlayerLowMorale`** (type registered @ `0x00A8F3B0`, string `0x00B943A4`) and, if persistent,
   **`ConcernNegativePlayerTransferRequestLowMorale`** (@ `0x00A91B72`) — a manager mail; `Time Concern Voiced` /
   `Time Transfer Requested` are stamped on the record. The manager answers via a `_QuietChat*`/`_Promise*`
   variant (`HandleMailResponse`), which pushes a counter-effect memory (e.g. `_QuietChatHisTimeWillCome4`).

---

## 7. Linkage to gameplay / management (VERIFIED refs, INFERRED weighting)

- **Management mail / player concerns**: low morale → `ConcernNegativePlayerLowMorale` and
  `ConcernNegativePlayerTransferRequestLowMorale` mails (above). Morale directly drives **player discontent and
  transfer requests**.
- **Match engine**: `PostMatchMutation` mutates **Morale, Condition and Confidence** together each match
  (`MutateMoraleScale`/`MutateConditionScale`/`MutateConfidenceScale`). Confidence & condition feed match
  performance, so morale is coupled to form through the shared post-match mutation step. (Coupling VERIFIED;
  exact in-match performance multiplier NOT traced.)
- **UI**: `PlayerMorale` display string (@`0x00BD6E5C`, ref `0x00AE7508`); morale also printed as a 0–1 float.
- **Homesickness / settling**: `MonthsTillHomesick` / `MonthsTillNaturalised` gate `Homesick`/`LanguageBarrier`
  reasons over time.

---

## 8. UNKNOWN / not recovered statically

- **Exact per-effect numeric magnitudes** (how many morale points `Fined` vs `Praise` vs `DroppedToReserves`
  are worth). The static `0x20`-stride table at `0xB8EBB8` holds only name + (empty) text pointers — **no
  weights**. The values live in the `GCHappyEffectData` objects constructed at load (array at `*[0xC803D0]+0x2C`)
  and/or in `#Database#/HappyMemory.dat` / `HappyRecords.dat` (internal resources, absent from the extracted
  `DatabaseFiles/`). Not resolvable from the static image alone.
- **Exact decay formula coefficients** (how `Gradient`/`Decay Delay`/`Duration`/`Count` combine into the
  instantaneous contribution). Code path identified (`HandleHappyValueUpdate` + effect vtables) but the FP
  arithmetic inside the per-effect virtual methods was not fully decompiled.
- **Numeric values of the `PlayerHappiness` thresholds** (`SlightConcernThreshold` etc.) — names VERIFIED,
  values are runtime config.
- **The integer enum value of each effect** — table *order* is known (Praise=index0 … NewContractNotBeingOffered
  =index44 in the `0xB8EBB8` array), but the canonical enum ids used in `GCHappyMemory.Effect` are assigned in
  the registration function (`~0x6D0600`) and were not individually decoded.
- `GCHappinessModule.cpp` assert refs at `0x0078AD2D/0x0078B092/0x0078B332/0x0078B4D1` mark additional module
  methods not individually traced.

---

### Appendix — key VAs
- Effect name/media table: `0x00B8EBB8`–`0x00B8F158` (45 × 0x20)
- Reason-name string blob: `0x00B8E5B0`+ and `0x00B93BD8`+ (meta)
- `HandleHappyValueUpdate`: `0x006DC7F0`
- `HandleFixturePostAnalysis`: log `0x006DD465`; `HandleFixturePreAnalysis`: log `0x006DD15F`
- `HandleAddHappyEventToClub`: `0x006DC420`; `HandleAddHappyNiggle`: `0x006DC66F`;
  `HandleHappyNiggleMemoryUpdate`: `0x006DCA50`
- Module init / effect registration: `~0x006D0600`–`0x006D1830`
- Record dump (morale byte @ record+0x5D): `0x005D93F0`; morale scale const `0x00B73954` = 1/255
- Config: `PlayerHappiness` reg `0x0082EADC`, `PostMatchMutation` reg `0x0084B15C`
- Linkage: `ConcernNegativePlayerLowMorale` `0x00A8F3B0`, `...TransferRequestLowMorale` `0x00A91B72`,
  `PlayerMorale` UI ref `0x00AE7508`
