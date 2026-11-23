# CM2008 — Player `*Performance` calculation: reverse-engineering report

**Question answered:** how do attributes + condition + tactics + context become the
`<Metric>Performance` float fields that the resolver reads (e.g. `TouchesPerformance`,
`AssistsPerformance`, `TacklesWonPerformance`, `GroundCoveredPerformance`)?

**Headline result (VERIFIED):** there is **no function that computes and stores a
`<Metric>Performance` float**. Each `<Metric>Performance` is a **named formula node**
registered under the data source `"PlayerData"` and **evaluated live, on every read**,
by the node‑tree evaluator `FUN_00818120`. The "getters" the resolver calls *are* the
evaluation. The numeric structure of each formula (which inputs, which weights, which
operators) lives in **game data loaded by `GCFormulaModule`**, not in the executable.
The executable gives the **evaluator, the node grammar, the lookup/registry, the input
field layout, and the consumer**; it does **not** give the coefficients.

Binary: `CM2008.exe`, Ghidra project `~/tools/cm2008_full` (`CM2008`). All addresses
are VAs. Evidence dumps regenerated into
`/private/tmp/claude-501/.../scratchpad/agentC2_out/` (`eval_core.c`, `eval_helpers.c`,
`operand.c`, `operand2.c`, `range.log`, `combiner.c`).

> Note on confidence: many of the small leaf accessors (`FUN_005b3770`, `FUN_005b4880`,
> `FUN_005b4900`, `FUN_005b3ba0`, `FUN_005b52f0`, `FUN_005b5150`, `FUN_005b3760`,
> `FUN_00480110`) are **anti‑disassembled** (overlapping/garbage bytes:
> `Control flow encountered bad instruction data`). Their *roles* are recovered from
> their clean callers; their internals are not byte‑decoded. This is flagged per item.

---

## 1. The read path — proof the values are computed live, not stored

The resolver reads a metric through a per‑metric getter. All getters share one shape
(`resolver2/decompiled.c`, confirmed by fresh decompile):

Example `TouchesPerformance` = `FUN_0083a5a0` @ `0x0083a5a0`:

1. **One‑time** (guarded by bit in `_DAT_00c90034`): intern the two strings
   `source="PlayerData"`, `key="TouchesPerformance"` and resolve the formula node:
   `DAT_00c90030 = FUN_00662cf0(&source, &key)`. `DAT_00c90030` is the **cached node
   handle**, *not* a value.
2. If the handle is 0 (formula absent) → return a **hard‑coded default float**
   (`TouchesPerformance` → `_DAT_00b59d7c`).
3. Otherwise → **`FUN_00818120()` runs on every call** (it is *outside* the one‑time
   guard) and the result float `fStack_8` (= `output+0x14`) is returned.

→ **VERIFIED:** step 3 re‑evaluates the tree on each read; nothing is memoised except
the node pointer. There is no stored float field and therefore no writer to find.

Per‑metric default fallbacks observed (VA of the default global):
| Getter | Metric | default |
|---|---|---|
| `FUN_0083a3c0` | `RedCardPerformance` | `-2.0` |
| `FUN_0083a5a0` | `TouchesPerformance` | `_DAT_00b59d7c` |
| `FUN_0083b0e0` | `TacklesWonPerformance` | `_DAT_00b5b758` |
| `FUN_0083b770` | `GroundCoveredPerformance` | `0.0` |
| `FUN_0083b860` | `AssistsPerformance` | `_DAT_00b60190` |
| `FUN_00a72db0` | `HomePerformanceLevel` / `AwayPerformanceLevel` | (see fn) |

There is a whole **contiguous family of these getters** `0x0083a3c0 … 0x0083ba40`
(one per metric), each: `FUN_00662cf0(source="PlayerData", key="<Metric>Performance")`
then `FUN_00818120()`. (`operand.c`, `combiner.c`, `range.log`.)

---

## 2. The lookup + registry (`FUN_00662cf0`)

`FUN_00662cf0` @ `0x00662cf0` = **`(source, key) → node*`** against a module registry.

- Its own body is anti‑disassembled, but its **clean caller `FUN_008181b0`** shows the
  call convention exactly (ASM @ `0x008181c4`):
  `MOV ECX,[0x00c77668]; PUSH key; PUSH source; CALL 0x00662cf0` →
  **`__thiscall`, `ECX = registry singleton DAT_00c77668`, args `(source, key)`,
  returns node\***. (VERIFIED from ASM.)
- The registry at **`0x00c77668`** is the `GCFormulaModule` instance. Each *source* is a
  namespace; each *key* a named formula. Source namespaces observed being passed to
  `FUN_00662cf0` across the dumps (`local_5c` assignments): **`PlayerData` (28×),
  `ProspectiveManagers` (28×), `SquadData`, `ManagerSacking`, `PlayerHappiness`,
  `TrainingModule`, `PlayerState`, `PlayerReputationPostMatchProgression`,
  `TransferModule`, `PhysioInjuries`.** (VERIFIED.)

---

## 3. The evaluator grammar (`FUN_00818120` chain) — VERIFIED

Recovered from clean decompiles + ASM of `FUN_00818120`, `FUN_00818080`,
`FUN_00817df0`, `FUN_00817fe0`, `FUN_00817f20`, `FUN_008181b0`.

### 3.1 Node evaluate — `FUN_00818120` @ `0x00818120` (`__thiscall`, ECX = node, arg = output ctx)
A node has three parts and is evaluated in this order:
1. **Children vector** at `[node+0x38]..[node+0x3c]` (ptr array, count `=(end-begin)>>2`):
   for each child → `FUN_005b3770` (child‑evaluate recursion; anti‑disasm internals).
2. **Term list** (count `FUN_005b4870`): for each term `t = FUN_005b4880(i)` →
   `FUN_00818080(ECX=t)` computes the term's binary op.
3. **Aggregate** descriptor at `[node+0x74]` → `FUN_00817df0` folds all term results.

### 3.2 Binary‑op term — `FUN_00818080` @ `0x00818080` (ECX = term)
- `left  = FUN_00817fe0(ECX=[term+0x28])`
- `right = FUN_00817fe0(ECX=[term+0x2c])` (skipped when op==5)
- `switch([term+0x24])` → store result into `[term+0x30]`:

| `[term+0x24]` | operation | result |
|---|---|---|
| 1 | ADD | left + right |
| 2 | SUB | left − right |
| 3 | MUL | left × right |
| 4 | DIV | left ÷ right |
| 5 | VALUE | left (single operand, no right) |
| other | — | 0.0 |

(VERIFIED from the FP jump table at `0x0081810c`.)

### 3.3 Operand resolve — `FUN_00817fe0` @ `0x00817fe0` (ECX = operand)
`switch(FUN_005b3ba0())` (operand kind), jump table `0x00818068`, index = kind−1:

| kind | handler | meaning |
|---|---|---|
| 1 | `FUN_00817f20(ECX=[op+0x10])` | **nested selector / sub‑expression** (recursive; uses obfuscated index‑picker `FUN_005b5100` — see §3.5) |
| 2 | `FUN_005b52f0([op+0x14])` → `FUN_005b5150(key,ctx)` → `FUN_005b3760` | **external named value** — look a name up in the eval context and read its float (this is the channel for *inputs*: raw match stats / attributes / condition) |
| 3 | `FUN_008181b0(ECX=[op+0x1c])` | **named‑ref to another formula node** (recurses via `FUN_00662cf0`, §3.4) |
| 5 | `FUN_005b52f0([op+0x20])` → `FUN_005b4900(key,ctx)` → return `[term+0x30]` | **reference to a sibling term's computed result** |
| 4 / default | — | 0.0 |

(VERIFIED from the jump table + clean caller dispatch. The `FUN_005b52f0/5b5150/5b3760/5b4900`
leaf bodies are anti‑disassembled, roles inferred from the dispatch.)

### 3.4 Named‑ref node — `FUN_008181b0` @ `0x008181b0` (VERIFIED)
`if([op+0x48]==0) [op+0x48] = FUN_00662cf0(source=[op+0x0c], key=[op+0x24])` (ECX=registry
`0x00c77668`), then build a sub‑context `FUN_00817da0(_,[op+0x3c],ctx)` and
`FUN_00818120(ECX=[op+0x48])`. → A formula operand can name **another (source,key)
formula** and pull its evaluated value. The resolved node is cached in `[op+0x48]`.

### 3.5 Aggregate fold — `FUN_00817df0` @ `0x00817df0` (ECX = aggregate node)
`switch([aggnode+0x20])`, iterating terms (`FUN_005b4db0`=count, `FUN_005b4900`=term i,
value = `term+0x30`), writes to `[node+0x0c]` **and to `output+0x14`** (the float the
getter returns):

| `[aggnode+0x20]` | aggregate |
|---|---|
| 0 | SUM of term values |
| 1 | MEAN (SUM ÷ count) |
| 2 | PRODUCT (init 1.0, ×) |
| other | 0.0 |

(VERIFIED.)

### 3.6 Selector / random path — `FUN_00817f20` (operand kind 1) — INFERRED/HYPOTHESIS
`FUN_00817f20` @ `0x00817f20` picks a branch via **`FUN_005b5100(ctx)`** (an obfuscated
index picker) and recurses, dispatching through a small vtable on tags `DAT_00c6dc90` /
`DAT_00c6dc94`. This is the same machinery the prior reconstruction
(`match/sim/resolver_eval.{h,c}`) models as the **TABLE / RANDOM** node with the recovered
LCG (`state = state*1664525 + 1013904223`, VERIFIED constants there). Whether any given
`PlayerData/*Performance` formula actually uses this (i.e. whether player performance has
a **random factor**) is **data‑dependent and UNKNOWN from the executable** — it depends on
the loaded formula tree, which is game data.

---

## 4. Inputs — what feeds the formulas (field layout) — VERIFIED (layout), UNKNOWN (which per metric)

The per‑player **match‑stat record** is the input bank. Its byte layout is recovered from
the CSV exporter `FUN_00a25730` @ `0x00a25730` (`match_movement_ai.c:14380`), whose header
branch names every column and whose value branch reads each offset:

- `[+0x0e]` = **`Performance`** (overall match rating byte), `[+0x10]` = **`Condition`**,
  `[+0x11]` = Tricks, `[+0x12]` = Morale, `[+0x13]` = Confidence, `[+0x14]` (u16) = `Touches`,
  `[+0x16]` = CompletedPasses, `[+0x17]` = Passes, `[+0x1b..0x1d]` summed = GoalsScored,
  `[+0x21]` = ChallengesMade, `[+0x22]` = TacklesWon, `[+0x23]` = TacklesKey,
  `[+0x29]` = Crosses, `[+0x36]` (u16) = GroundCovered, `[+0x38]` = Assists,
  `[+0x39]` = KeptPossession, `[+0x3b]` = FailedToControl, `[+0x4e]` = LostPossession,
  `[+0x50]` (u16) = ShotsOffTarget … (full list in the exporter). Counters are
  **8/16‑bit integers**.

These raw counters are what a `*Performance` formula pulls in via **operand kind 2**
(external named lookup, §3.3): e.g. `TouchesPerformance` scales the `Touches` counter.
**Which** counters, attributes (CA, technique, …), `Condition`, tactics or context each
formula references, and with **what weights/positional‑duty modifiers**, is encoded in the
formula tree data — **not in the exe**, so it is **UNKNOWN / not recoverable here**. Do not
invent these. (The attribute → raw‑counter influence happens earlier, in match‑event
generation, a separate subsystem.)

Metric keys confirmed present under source `PlayerData`: `RedCardPerformance`,
`TouchesPerformance`, `TacklesWonPerformance`, `GroundCoveredPerformance`,
`AssistsPerformance`, `BadTacklesPerformance`, `KeyTacklesPerformance`,
`TacklesAvoidedPerformance`, `KeptPossessionPerformance`, `OffsidePerformance`,
`LostPossessionPerformance`, plus `HomePerformanceLevel` / `AwayPerformanceLevel`
(`FUN_00a72db0`). (VERIFIED from string refs.)

---

## 5. Consumer — the overall match rating aggregator — VERIFIED (structure)

A single large routine at **≈`0x009f70f0`** (currently in an un‑analyzed/obfuscated code
region; `getFunctionContaining` returns none) calls **the entire getter family in
sequence**: `… 0x0083a3c0, 0x0083a4b0, 0x0083a5a0(Touches), 0x0083a690 … 0x0083b0e0
(TacklesWon) … 0x0083b770(GroundCovered), 0x0083b860(Assists) … 0x0083ba40`, each preceded
by a helper call (`0x00480110`, anti‑disassembled). (VERIFIED, `range.log`.) This is the
match‑rating accumulator that folds the per‑metric `*Performance` floats into the overall
**`Performance` byte** `[record+0x0e]`.

**Final precision/conversion (VERIFIED):** the exporter computes
`ScaledPerformance = (Performance + 5) / 10` (`FUN_00a25730` @ `0x00a2.., line 14529`),
i.e. the 0–~100 `Performance` byte rounds to a 0–10 match rating. The per‑metric
`*Performance` channels are IEEE‑754 **32‑bit floats** throughout the evaluator
(`FST [term+0x30]`, `output+0x14`).

---

## 6. Where the formula DATA lives (and the loader) — VERIFIED (mechanism), partial (exact file)

Formulas are **data‑driven**, matching the architectural note in
`docs/ghidra_dumps/raw/README.md`:

- **`GCFormulaModule` loader/registration dispatcher** = `FUN_008f90f0` @ `0x008f90f0`
  (13446 bytes, dump seed `formulae`; `__thiscall`; 8769‑byte on‑stack parse buffer;
  ~160 one‑time `FUN_00405530` string‑interns). It builds the node trees into the registry
  `0x00c77668`. Driven by the data‑reload hooks `ReloadFormulae` / `TestXMLParserFormulae`
  (seed strings). Its 13 KB body is heavily guarded/obfuscated; byte‑level recovery of the
  parse was **not** attempted (would require the data schema to be meaningful).
- **Weighting structure** is named, e.g. the `…BaseWeight…Component…` key families
  (`ProspectiveManagersBaseWeightDomesticComponent{Ambition,Loyalty,PotentialAbility,…}`,
  `ManagerSackingBaseWeight…`) — i.e. each formula = base weight × component terms, exactly
  the SUM/MEAN/PRODUCT‑of‑binops grammar of §3. The **numeric weights are data**.
- **Related distribution data** loads via `FUN_00a098a0` @ `0x00a098a0` from
  `#Database#/MatchResolverDistributions.%i.dat` (6 files, `%i`=1..6; see
  `research/resolver-loader.md`). The `PlayerData` formula source is loaded by
  `GCFormulaModule` from the game's XML data (per README: shipped in the installer cabinets
  `Setup/data2.cab` / `data3.cab`, VFS prefix `#DataXML#`). These data files are **not in the
  dumps and not in the exe**, so the actual coefficients/weights per metric are the blocker.

---

## 7. Attributes + condition + tactics + context → Performance (end‑to‑end)

```
player attributes + condition + tactics + context
        │  (match‑event generation — separate subsystem; produces events)
        ▼
per‑player match‑stat record  (raw integer counters: Touches, TacklesWon,
        │                       GroundCovered, Assists, …; Condition @ +0x10)
        │  resolver read of "<Metric>Performance"
        ▼
FUN_00662cf0(registry 0x00c77668, "PlayerData", "<Metric>Performance") → node*
        │
        ▼
FUN_00818120 evaluate node tree  (VERIFIED grammar):
     • operand kind 2  → pull raw counter / named input  (FUN_005b5150)
     • operand kind 3  → pull another named formula       (FUN_008181b0→FUN_00662cf0)
     • term op 1..5    → ADD/SUB/MUL/DIV/VALUE  (weights are DATA constants)
     • aggregate 0/1/2 → SUM / MEAN / PRODUCT
     • (optional kind‑1 selector/random via FUN_005b5100 + LCG — data‑dependent)
        │  float32, returned each call (no caching of the value)
        ▼
aggregator ≈0x009f70f0 reads every <Metric>Performance → overall Performance byte [+0x0e]
        ▼
ScaledPerformance = (Performance + 5) / 10          (0–10 match rating)
```

---

## 8. Classification summary

| Item | Address | Status |
|---|---|---|
| `*Performance` are live‑evaluated formula nodes, not stored floats (no writer) | getters `0x0083a3c0…0x0083ba40` | **VERIFIED** |
| Getter re‑evaluates on every read; only node ptr cached | `0x0083a5a0` etc. | **VERIFIED** |
| `(source,key)→node*` lookup; registry singleton | `FUN_00662cf0` / `DAT_00c77668` | **VERIFIED** (conv. from caller ASM) |
| Node grammar: children + terms + aggregate | `FUN_00818120` | **VERIFIED** |
| Binop codes 1=ADD 2=SUB 3=MUL 4=DIV 5=VALUE | `FUN_00818080` @ `0x0081810c` | **VERIFIED** |
| Operand kinds 1/2/3/5 (nested / external‑named / named‑ref / term‑ref) | `FUN_00817fe0` @ `0x00818068` | **VERIFIED** |
| Named‑ref recursion via `FUN_00662cf0` | `FUN_008181b0` | **VERIFIED** |
| Aggregate 0=SUM 1=MEAN 2=PRODUCT; writes `output+0x14` | `FUN_00817df0` | **VERIFIED** |
| Inputs = raw int match counters; record layout | `FUN_00a25730` @ `0x00a25730` | **VERIFIED** |
| `ScaledPerformance=(Performance+5)/10`; floats are f32 | `FUN_00a25730:14529` | **VERIFIED** |
| Rating aggregator calls all getters | `≈0x009f70f0` | **VERIFIED** (structure; region not auto‑analyzed) |
| `GCFormulaModule` loader/registration | `FUN_008f90f0` | **VERIFIED** it registers; parse internals **not decoded** (obfuscated) |
| Distribution data file | `FUN_00a098a0`, `MatchResolverDistributions.%i.dat` | **VERIFIED** (separate from PlayerData formulas) |
| Selector / **random factor** in `*Performance` | `FUN_00817f20`/`FUN_005b5100` + LCG | **HYPOTHESIS / data‑dependent** |
| Which attributes/condition/tactics each formula uses; **weights, position/duty/tactical modifiers** | formula tree data | **UNKNOWN** — in `#DataXML#` game data, not in exe |

## 9. The exact blocker

The **evaluator, grammar, lookup, registry, input layout, consumer, and output precision
are fully recovered from the binary.** The **per‑metric formula definitions** — exactly
which inputs each `<Metric>Performance` combines, the attribute/condition weights, and any
position/duty/tactical modifiers or random component — are **not in `CM2008.exe`**. They are
loaded at runtime by `GCFormulaModule` (`FUN_008f90f0`) from the game's XML formula data
(installer cabinets `data2.cab`/`data3.cab`, VFS `#DataXML#`). Recovering the coefficients
requires extracting and parsing that data; inventing them from football intuition is out of
scope and explicitly avoided here.

## UPDATE (orchestrator) — formula coefficients are ENCRYPTED; mechanism recovered, data not extracted

The formula definitions live in the install as **`DataXML/formula.xml.blowfish`**
(~482 KB, **Blowfish-encrypted**), parsed at runtime by GCFormulaModule
("ReloadFormulae"/"TestXMLParserFormulae").

Status:
- VERIFIED (and implemented): the formula EVALUATOR + grammar — node tree,
  binop codes (1=ADD 2=SUB 3=MUL 4=DIV 5=VALUE), operand kinds (1 nested / 2
  external-input / 3 named-ref / 5 sibling), aggregate (0=SUM 1=MEAN 2=PRODUCT),
  registry singleton DAT_00c77668, input record offsets, ScaledPerformance=(P+5)/10.
  `match/sim/resolver_eval.{h,c}` now uses the verified op-codes.
- NOT RECOVERED (deliberately): the per-metric coefficients/weights/modifiers.
  They are inside the encrypted `formula.xml.blowfish`. Decrypting the game's
  protected data to extract its proprietary balancing is content-protection
  circumvention + reproduction of proprietary data — out of scope and not done.

Consequence: the engine's formula layer can evaluate formula trees, but the
coefficient DATA is the user's own (to obtain legally). This is a hard evidence
boundary, not a gap I can close by more static analysis of the exe.
