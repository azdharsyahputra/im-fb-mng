# Resolver indexing: Player/Team state → index → distribution → sampled outcome

**Goal of this doc.** Pin, with binary evidence, the chain
`attributes + position + condition + ability + tactics + context + opponent →
Resolver Index → Distribution (bucket) → sampled performance metric`.
Each link is classified **VERIFIED** (decompiled/structural proof),
**STRONGLY INFERRED** (multiple consistent signals, one inferential step),
**HYPOTHESIS** (plausible, not yet pinned), or **UNKNOWN**. Where the chain
breaks, the exact missing function/data is named.

Binary: `CM2008.exe` (Ghidra project `/Users/csadeveloper/tools/cm2008_full`).
No proprietary distribution float values are reproduced here.

---

## 0. TL;DR — recovered chain and where it breaks

```
 [attributes, condition, ability, tactics, opponent]
        │  (A) GCFormulaModule formulas  — UNKNOWN function
        ▼
 PlayerData.<Metric>Performance   (continuous float, per player per aspect)   VERIFIED (read path)
        │  (B) discretise + combine with position/duty/squad/context index
        ▼
 Resolver Index  =  f(Performance, EPositionValue, EDutyValue, ESquadValue, PerformanceLevel)   STRONGLY INFERRED (dimensions) / HYPOTHESIS (formula)
        │  (C) index the named distribution record (dims 5,9,8 → CDF)          STRONGLY INFERRED (data), UNKNOWN (sampler fn)
        ▼
 sample with MatchSeed RNG → discrete count                                    HYPOTHESIS
        │  (D)
        ▼
 per-player output record (0x34c bytes) → FE match stats                        VERIFIED (output path)
```

The **two ends are decompiled and solid** (input-field read path; output-record
marshalling path). The **middle numeric core** (links B/C/D — the function that
computes the index and samples the CDF) is behind the resolver module's
struct-offset / register-`this` boundary and is **not yet pinned**. The precise
missing artifacts are named in §6.

---

## 1. The resolver is a data-reflection machine (VERIFIED)

All resolver config/inputs/metrics are read through one accessor pair:
- `FUN_00662cf0` — resolve a named field in a named data source → field handle.
- `FUN_00818120` — read the typed value for a handle from the current record.

The getter idiom (cache handle once via a `&1` init flag, then read value each
call) is identical across the whole cluster. Data sources observed as the first
argument to `FUN_00662cf0`: **`"ResolverConstants"`, `"MatchResolver"`,
`"PlayerData"`**.

Evidence (getter bodies, this run and `docs/ghidra_dumps/resolver*/decompiled.c`):
- `FUN_0083b860` `"AssistsPerformance"` ← `"PlayerData"` → returns `float`.
- `FUN_0083a5a0` `"TouchesPerformance"`, `FUN_0083b0e0` `"TacklesWonPerformance"`,
  `FUN_0083b770` `"GroundCoveredPerformance"` — same shape.
- `FUN_0082f290` `"UpdateDegradableStatsPeriod"` ← `"MatchResolver"`,
  `FUN_00831090` `"SidlingLeftDecelerationMultiplier"` ← `"MatchResolver"`,
  `FUN_00834420` `"SimulatedResolverGoalFrequency"` ← `"MatchResolver"` — these
  are **engine tuning scalars**, not per-player.
- The ~26 `FUN_0082xxxx` `"ResolverConstants"` getters and ~52 `FUN_0082f–00832`
  `"MatchResolver"` getters are **all this same scalar-getter pattern** (verified
  by reading representatives; uniform 235-byte bodies).

**Consequence for tracing:** these getters carry no algorithm and, in Ghidra,
have **0 resolved callers** (checked via caller-closure: `0083b860/0083a5a0/
0083b0e0/0083b770` → 0 callers). They are invoked indirectly through the
reflection/current-record dispatch, so the index computation cannot be reached
by direct xref from them. This is the central reason the numeric core is hard to
pin.

---

## 2. Left side of the chain — the resolver's per-player INPUTS (VERIFIED read path)

### 2a. `PlayerData.<Metric>Performance` — continuous per-aspect ratings
Fields read from the `"PlayerData"` source by dedicated getters (addresses are
the getter entry points):

| field | getter | returns |
|---|---|---|
| `AssistsPerformance` | `FUN_0083b860` | float |
| `TouchesPerformance` | `FUN_0083a5a0` | float |
| `TacklesWonPerformance` | `FUN_0083b0e0` | float |
| `GroundCoveredPerformance` | `FUN_0083b770` | float |
| plus `OffsidePerformance`, `KeptPossessionPerformance`, `LostPossessionPerformance`, `KeyTacklesPerformance`, `BadTacklesPerformance`, `TacklesAvoidedPerformance`, `ScaledPerformance` | (sibling getters, same region) | float |

These are **continuous** values (the getters return `float`). They are the
per-player, per-aspect "performance level" fed into the resolver. **VERIFIED**
they are read; **UNKNOWN** which function *writes* them (see §6 link A).

### 2b. Context inputs (VERIFIED they exist)
`HomePerformanceLevel` / `AwayPerformanceLevel` (`FUN_00a72db0` registers the
GCMatchInputs schema: also `MatchSeed`, `Home/AwayPersonality`,
`Home/AwayLikelyResult`, `Home/AwayPlayingMentality`, passing/pressing styles,
offside trap, etc.). These are **team-level context** + the RNG seed.
`CurrentAbility`/`PotentialAbility`/`Condition` are read by player getters
(`FUN_0080a9f0` etc.) — the raw ability/condition inputs.

### 2c. Tactical index dimensions — `GCResolverTactics` (VERIFIED)
The resolver's tactical input is three keyed value-sets (struct-field reflection
registrations):
- `GCResolverTactics::ESquadValue` — `FUN_00a9ed90` — `{eType, uValue}`.
- `GCResolverTactics::EPositionValue` — `FUN_00a9ee40` — `{eType, uPlayerIndex, uValue}` → **position is an explicit per-player dimension**.
- `GCResolverTactics::EDutyValue` — `FUN_00a9ef40` — `{eType(sTeamDutiesData::EOperation), uIndex, uValue}` → **duty** dimension.

So the resolver is told, per team: squad-level values, per-position/per-player
values, and per-duty values. **This is the position/duty/squad side of the
index.** VERIFIED as inputs; their *use* as array subscripts is §5.

### 2d. The resolver command/packet (VERIFIED)
- `GCResolverInput` — `FUN_00a9f090` — `{m_iResolveStep:int32, m_InputData:sInputData, m_eInputType:eResolverInputType, m_iTeamIndex:ESquadType}`. The resolver is a **step-driven** module fed serialized inputs.
- Serialization/identity types present: `TDM_ResolverExternalID`,
  `GCResolverExternalTypes`, `ResolverStatus`, `ResolverVersion`,
  `m_iResolveStep`.

---

## 3. Right side of the chain — the resolver OUTPUTS (VERIFIED output path)

### 3a. Event stream — `GCResolverEvent` (VERIFIED)
`FUN_00a9e850` registers `GCResolverEvent` =
`{m_uMins:uint32, m_iTeamIndex:ESquadType, m_eEventType:eResolverEventType,
m_PersonNameID:TDM_ResolverExternalID, m_fWorldTimeOffset:float}`.
→ Resolver output is a **timestamped event stream** (event type, which team,
which player, match minute).

### 3b. Per-player output stat records + marshalling (VERIFIED)
`MatchResolverComponent`, the FE bridge, cluster `0x009d4xxx–0x009d5xxx`:
- `FUN_009d5640` ("Match interface (resolver), getting PersonNameID…") — builds
  the player↔squad roster (2 squads, loop `iVar5 < 2`) handed to the resolver.
- `FUN_009d52e0` — per-squad resolve step: for each player gets an output slot
  `FUN_009d3e50(slot)`, computes a per-player byte via `FUN_009d0fb0(squad,
  playerIdx)`, then `FUN_009d45d0`/`FUN_009d4930` marshal the stats and
  `FUN_009d4090` stores them.
- `FUN_009d4090` ("Storing (from Resolver, squadnum %i) player output data…") —
  iterates per-player records and copies each into the FE match interface.
  Record stride seen: `0x34c` bytes/player, `0x17` (23) players/squad.
- `FUN_009d45d0` (851b) / `FUN_009d4930` (812b) — read **~30 per-player scalar
  accessors** `FUN_00495460…FUN_004956c0` (each returns a `short`/`char` count)
  and write them into the FE stats struct via `FUN_0061fa70`.

**Type signature of the chain is thus confirmed:** continuous float
`*Performance` inputs (§2a) → **discrete short/char counts** out (§3b). That is
exactly a sample-from-distribution operation.

Caveat: the `FUN_00495xxx` accessors are register-`this` thiscalls that Ghidra
mis-decompiles (bad/overlapping instruction data, e.g. `FUN_004955b0`). They
read fields of the already-filled `0x34c` output record — i.e. the sampling has
already happened upstream of this cluster.

---

## 4. A disambiguation that removes a false lead (VERIFIED)

The code symbols containing "Distribution" are **ball-distribution tactics in the
visual match AI, NOT the resolver's statistical CDF distributions**:
`GCMatchActionDistributionImplementation` (RTTI @ `0x00b564a0`),
`GainedPossessionDistribution`/`LostPossessionDistribution` (@ `0x00b5fd24/44`),
`"Wing / Middle distribution PLAYER/TEAM"`, `DistributionPreference`
(`FUN_009def10` registers passing-preference enum: BombAround, CutInside,
DistributeToWings…). None of these is the resolver sampler. The resolver's
statistical distributions (the ~45 MB `MatchResolverDistributions.*.dat`) have
**no dedicated named class or vftable in the symbol table** — they are read
generically. This is why keyword/xref hunts for "Distribution" do not land on
the sampler.

---

## 5. The index itself — what is known (STRONGLY INFERRED) vs assumed (HYPOTHESIS)

**STRONGLY INFERRED — the index dimensions.** The distribution records begin
with small integer dimension counts (prior finding: leading ints `5, 9, 8`),
then ~5-point monotonic CDF runs. The resolver's declared index inputs are
exactly discrete, low-cardinality, keyed dimensions: `ESquadValue`,
`EPositionValue` (per player/position), `EDutyValue` (per duty), plus the
3-valued `Home/AwayPerformanceLevel` context. A nested
`[squad/context][position][duty]`-style grid indexing a per-metric CDF is
consistent with both the `5,9,8` header and these inputs. (I could **not** bind
a specific enum's cardinality to a specific header integer — the enum
*cardinalities* were not recovered, only the field layouts — so this stays
inferred, not verified.)

**HYPOTHESIS — the index formula.** The continuous `<Metric>Performance` float
(e.g. `AssistsPerformance`) is discretised into a bucket and combined with the
position/duty/squad/context subscripts to pick the CDF column; a `MatchSeed`-fed
RNG draw against that CDF yields the discrete count written to the `0x34c`
record. No decompiled multiply-add / table-offset expression has been captured
to confirm this.

---

## 6. Where the chain breaks — exact missing artifacts

| link | status | missing artifact |
|---|---|---|
| **A** attributes → `PlayerData.<Metric>Performance` | UNKNOWN | the GCFormulaModule function(s) that **write** the `*Performance` floats. **Checked this run:** xref of the cached handle globals `DAT_00c900d0` (Assists), `DAT_00c90030` (Touches), `DAT_00c90090` (TacklesWon), `DAT_00c900c8` (GroundCovered) finds **only the four getters** — no store site. ⇒ the values are computed **game-side (GCFormulaModule)** and serialized into the `PlayerData` record handed to the resolver, not written through these globals. The formula remains unlocated. |
| **B** performance+position/duty → index | HYPOTHESIS | the function computing `index = f(performance, EPositionValue, EDutyValue, ESquadValue, PerformanceLevel)`. Operates on struct offsets inside the resolver module; no string anchor. |
| **C** index → distribution CDF column | UNKNOWN (sampler fn) / STRONGLY INFERRED (data) | (1) the **distribution loader** that maps the named records into memory (no filename string in the exe; loaded via the generic data-source layer, likely keyed by the resolver-metric enum ordinal, not per-metric string literals); (2) the **CDF indexer** that reads the `nBuckets/key/nValues` grammar. |
| **D** CDF + `MatchSeed` → count | HYPOTHESIS | the RNG draw + CDF inverse-lookup. `MatchSeed` is a confirmed GCMatchInputs field (`FUN_00a72db0`); the consuming RNG is not pinned. |
| **E** count → `0x34c` output record | PARTIAL | the **writer** of the `0x34c` per-player record (the engine entry consuming `GCResolverInput`). The readers (`FUN_00495xxx`, `FUN_009d45d0/4930`) are known; the writer is engine-side and decoupled from the FE component (**checked:** `FUN_009d1470`, the component's pre-results call, is a 165-byte state-sync, not the engine). The record slab lives at **offset `0x8d8` of the per-match context** (base `*(ctx+0x8d8 + FUN_009efa90())`, stride `0x34c`, `0x17`=23 players × 2 squads), per `FUN_009d4090`. Find the engine by xref to a **store** into that slab. |

### Recommended next probes (highest yield first)
1. **Writer of the `0x34c` record / engine entry.** Caller-closure *upward* from
   `FUN_009d3e50` (output-slot allocator) and find who writes the same slab the
   `FUN_00495xxx` accessors read; that function consumes `GCResolverInput` and
   contains links B–D.
2. **Formula A (done-negative):** handle-global xref found no store ⇒ look in
   GCFormulaModule / the `PlayerData` serializer that fills `*Performance` before
   the resolver is called, not at these globals.
3. **Resolver-metric enum** (`eDistribution` @ `0x00bbcecc`, and the resolver
   metric vocabulary already recovered in `docs/findings/match-resolver.md`): if
   the loader keys distributions by ordinal, the enum registration function
   (same pattern as `FUN_00a9ed90`) reveals the record order and count, letting
   the `5,9,8` dims be bound to specific dimensions (would upgrade link C to
   VERIFIED).
4. Fix decompilation of the `FUN_00495xxx` thiscalls (set calling convention /
   `this` type in Ghidra) to read the exact `0x34c` field offsets = the sampled
   metric layout.

---

## 7. Function/address index (anchors established this run)

| role | function(s) |
|---|---|
| field lookup / typed read | `FUN_00662cf0`, `FUN_00818120` |
| per-player `*Performance` getters | `FUN_0083b860` (Assists), `FUN_0083a5a0` (Touches), `FUN_0083b0e0` (TacklesWon), `FUN_0083b770` (GroundCovered); handles `DAT_00c900d0/00c90030/00c90090/00c900c8` |
| MatchResolver tuning getters | `FUN_0082f290`, `FUN_00831090`, `FUN_00834420` (+~50 siblings `0x0082f–00832`) |
| ResolverConstants getters | `FUN_008220d0`…`FUN_00823660` (~26) |
| GCResolverTactics index dims | `FUN_00a9ed90` (ESquadValue), `FUN_00a9ee40` (EPositionValue), `FUN_00a9ef40` (EDutyValue) |
| resolver input packet | `FUN_00a9f090` (GCResolverInput), `FUN_00a71b90` (typename), `FUN_00a71d60` (ResolverStatus) |
| resolver output event | `FUN_00a9e850` (GCResolverEvent), `FUN_00a71a30` (typename) |
| MatchResolverComponent (output bridge) | `FUN_009d5c20` (init/dispatch), `FUN_009d5640` (feed roster), `FUN_009d57b0` (report path), `FUN_009d5a10` (run), `FUN_009d52e0` (per-squad step), `FUN_009d45d0`/`FUN_009d4930` (marshal), `FUN_009d4090` (store), `FUN_009d3e50` (output slot), `FUN_009d0fb0` (per-player byte) |
| per-player output stat accessors | `FUN_00495460`…`FUN_004956c0` (~30; register-this, mis-decompiled) |
| context inputs schema | `FUN_00a72db0` (GCMatchInputs: MatchSeed, Home/AwayPerformanceLevel, mentalities, styles…) |
| ball-distribution tactics (NOT the resolver CDFs) | `FUN_009def10` (DistributionPreference enum); RTTI `GCMatchActionDistributionImplementation` @ `0x00b564a0` |

*Status:* input and output halves VERIFIED; index dimensions STRONGLY INFERRED;
index formula + CDF sampler HYPOTHESIS/UNKNOWN. Missing functions named in §6.
