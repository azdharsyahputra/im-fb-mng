# CM2008 Formula Coefficient / Identifier Registry

Clean-room static-evidence registry for the CM2008 formula system. Built by **reading** the shipped binary and the decompiled closure only — Ghidra was **not** re-run, nothing was executed, and the encrypted `formula.xml.blowfish` payload was **not** decrypted. Numeric values are read directly from the PE image; proprietary balancing trees are **not** reproduced (they are not in the exe — see the Unresolved section).

Sources: decompiled closure `scratchpad/formula.log` (829 formula-reachable funcs), binary `/Volumes/CM2008/Crack JPN/cm2008.exe`, grammar `research/FORMULA_XML_GRAMMAR.md`, `match/resolver_metrics.h`. The 84 predicate ranking weights (FUN_009e5160) are NOT duplicated here — see `match/sim/predicate_weights.h` / `research/MATCH_PREDICATE_WEIGHTS.md`.

## Methods

- **PE mapping (VERIFIED):** parsed the PE section table. For every normal section (`.text`,`.rdata`,`.data`,`.rsrc`) the raw-offset == RVA, so `file_offset = VA - 0x400000` holds exactly (only the SecuROM `.memory` section differs; it is irrelevant here).

- **Float constants:** parsed each function block in `formula.log`, collected every value-load `DAT_00b######` reference (excluding `&DAT`, the `_atexit`/`_func_` destructor table which the crack fills with `0xCC`), then read the 4-byte float at `VA-0x400000` with `struct.unpack_from('<f', exe, off)`. The `float10` casts in the code are x87 extensions of 4-byte `.rdata` floats — the f32 reading is clean; the f64/double reading is garbage, confirming 4-byte width.

- **Formula names:** each getter lazily resolves a named formula through `FUN_00662cf0` = `GCFormulaModule::Find(name, group)` (RTTI string at 0x00b87e6c), caches the `GCFormula*` handle in a `.data` slot, and `return (float10)DAT_const` as a **fallback default** when the formula is absent from `formula.xml`. Names/groups were extracted as the string-literal arguments; defaults read from the binary.

- **Grammar/operator vocabulary:** read verbatim from the `.rdata` string pool at 0x00b6b42c–0x00b6b550.

## Registry summary

- **88 distinct numeric float constants** recovered (VERIFIED values).

- **810 registered formula-name getters** across **39 groups** (810 distinct names).

- **1 formula `calc` operator vocabulary** (9 operators) recovered VERIFIED from strings.

- Formula-engine identifiers (classes, debug cmds, payload path, grammar tags) VERIFIED.


## 1. Formula-engine identifiers & structures (VERIFIED)

| name | kind | evidence VA (foff+0x400000) | note |
|---|---|---|---|
| `GCFormulaModule` | singleton class (CLSingleton) | 0x00b87660 / RTTI 0x00c259ec | owns the formula registry |
| `GCFormulaModule::Find` | method (= FUN_00662cf0) | 0x00b87e6c | (name,group)->GCFormula* ; every getter calls it |
| `GCFormula` | class (GCDataModule<GCFormula>) | RTTI 0x00c1c0bc / 0x00c259c0 | one parsed formula tree |
| `GCFFormulae` | class | RTTI 0x00c1c04c | formula collection |
| `GCFormulaeXMLHandler` | XML parser (CLSingletonAutoCreate) | RTTI 0x00c37918 | parses formula.xml into trees |
| `ReloadFormulae` | debug command | 0x00b87528 / 0x00c45258 | 'Debug Formulae|ReloadFormulae' |
| `TestXMLParserFormulae` | debug command | 0x00b87e84 / 0x00c44d1c | validates the XML parse |
| `#DataXML#/formula.xml.blowfish` | payload path | 0x00b87eac | Blowfish-encrypted formula definitions |
| `FUN_00818120` | evaluator entry | call in every getter | walks the formula tree, writes result to fStack_8 |
| `FUN_004020d0` | CLString ctor | 477 callsites | builds the (name)/(group) key strings |

## 2. Formula `calc` operator vocabulary (VERIFIED, .rdata string pool)

Fills the 'operator vocabulary — pending' gap noted in FORMULA_XML_GRAMMAR.md. Contiguous string block 0x00b6b42c–0x00b6b550 (also contains every grammar element/attribute tag).

| operator string | VA | role | applies to (`calc` on) |
|---|---|---|---|
| `sum` | 0x00b6b4d4 | add terms | result / value |
| `sub` | 0x00b6b4c8 | subtract | result / value |
| `prod` | 0x00b6b4cc | multiply | result / value |
| `div` | 0x00b6b4c4 | divide | result / value |
| `mean` | 0x00b6b438 | average | result / value |
| `equals` | 0x00b6b458 | == | condition |
| `notequals` | 0x00b6b44c | != | condition |
| `lessthan` | 0x00b6b460 | < | condition |
| `greaterthan` | 0x00b6b46c | > | condition |

Grammar element/attribute tags confirmed present as literals in the same block (VERIFIED the parser consumes them): `formrule, formula, rule, result, term, value, constant, const, input, variable, usevalue, formulae, function, condition, conditionset, effect, effectset, callrule, description` + attrs `name, group, type, calc`. The exact binop/agg opcode numbering (binop 1..5 / agg 0/1/2) is INFERRED — see `research/FORMULA_COEFFICIENTS_EVIDENCE.md` and `match/sim/resolver_eval.c`.


## 3. Numeric float constants (88) — VERIFIED values

All are 4-byte floats in `.rdata`. Value is VERIFIED (read from binary). Semantic ROLE is INFERRED: each is the fallback default returned by one or more formula-name getters (Section 4) when the named formula is missing from `formula.xml`; a few are compiler idioms (noted).

| DAT (VA) | value (f32) | u32 | #ref funcs | role / note |
|---|---|---|---|---|
| DAT_00b1801c | -1 | 0xbf800000 | 1 | -1.0 (generic) |
| DAT_00b18028 | 2 | 0x40000000 | 14 | 2.0 (generic) |
| DAT_00b3e9a8 | 4.29497e+09 | 0x4f800000 | 1 | = 2^32; unsigned->float conversion fixup in FUN_00817df0 (NOT a coefficient) |
| DAT_00b3f2e0 | 64 | 0x42800000 | 1 | formula-getter default / tuning constant |
| DAT_00b3f894 | 1000 | 0x447a0000 | 23 | default=1000 — WageLimits/ClubInfo getters |
| DAT_00b59ce8 | 33.75 | 0x42070000 | 1 | formula-getter default / tuning constant |
| DAT_00b59d7c | 0.01 | 0x3c23d70a | 1 | formula-getter default / tuning constant |
| DAT_00b5b10c | 80 | 0x42a00000 | 6 | formula-getter default / tuning constant |
| DAT_00b5b114 | 0.9 | 0x3f666666 | 6 | formula-getter default / tuning constant |
| DAT_00b5b190 | 200 | 0x43480000 | 1 | formula-getter default / tuning constant |
| DAT_00b5b198 | 0.65 | 0x3f266666 | 1 | formula-getter default / tuning constant |
| DAT_00b5b1d8 | 60 | 0x42700000 | 4 | formula-getter default / tuning constant |
| DAT_00b5b234 | 0.03 | 0x3cf5c28f | 1 | formula-getter default / tuning constant |
| DAT_00b5b284 | 25 | 0x41c80000 | 1 | formula-getter default / tuning constant |
| DAT_00b5b388 | 0.2 | 0x3e4ccccd | 8 | formula-getter default / tuning constant |
| DAT_00b5b5d4 | 0.7 | 0x3f333333 | 1 | formula-getter default / tuning constant |
| DAT_00b5b5ec | 0.3 | 0x3e99999a | 14 | formula-getter default / tuning constant |
| DAT_00b5b5f4 | 9 | 0x41100000 | 4 | formula-getter default / tuning constant |
| DAT_00b5b600 | 20 | 0x41a00000 | 18 | formula-getter default / tuning constant |
| DAT_00b5b6b4 | 40 | 0x42200000 | 3 | formula-getter default / tuning constant |
| DAT_00b5b6c4 | 0.05 | 0x3d4ccccd | 3 | formula-getter default / tuning constant |
| DAT_00b5b6cc | 8 | 0x41000000 | 20 | formula-getter default / tuning constant |
| DAT_00b5b704 | 90 | 0x42b40000 | 3 | formula-getter default / tuning constant |
| DAT_00b5b754 | 12 | 0x41400000 | 5 | formula-getter default / tuning constant |
| DAT_00b5b758 | 0.1 | 0x3dcccccd | 12 | formula-getter default / tuning constant |
| DAT_00b5b75c | 50 | 0x42480000 | 4 | formula-getter default / tuning constant |
| DAT_00b5b760 | 3 | 0x40400000 | 12 | formula-getter default / tuning constant |
| DAT_00b5b764 | 4 | 0x40800000 | 16 | formula-getter default / tuning constant |
| DAT_00b5b76c | 5 | 0x40a00000 | 14 | formula-getter default / tuning constant |
| DAT_00b5b798 | 0.25 | 0x3e800000 | 3 | formula-getter default / tuning constant |
| DAT_00b5b79c | -30 | 0xc1f00000 | 2 | formula-getter default / tuning constant |
| DAT_00b5bac0 | 0.4 | 0x3ecccccd | 4 | formula-getter default / tuning constant |
| DAT_00b5bad8 | 0.002 | 0x3b03126f | 1 | formula-getter default / tuning constant |
| DAT_00b5baf0 | -0.02 | 0xbca3d70a | 1 | formula-getter default / tuning constant |
| DAT_00b5baf4 | 85 | 0x42aa0000 | 1 | formula-getter default / tuning constant |
| DAT_00b5baf8 | 35 | 0x420c0000 | 1 | formula-getter default / tuning constant |
| DAT_00b5bb74 | 7 | 0x40e00000 | 2 | formula-getter default / tuning constant |
| DAT_00b5bb8c | 65 | 0x42820000 | 3 | formula-getter default / tuning constant |
| DAT_00b5bb94 | -0.1 | 0xbdcccccd | 2 | formula-getter default / tuning constant |
| DAT_00b5bbc4 | 36 | 0x42100000 | 1 | formula-getter default / tuning constant |
| DAT_00b5c0e8 | -0.2 | 0xbe4ccccd | 2 | formula-getter default / tuning constant |
| DAT_00b5e8c4 | 0.02 | 0x3ca3d70a | 3 | formula-getter default / tuning constant |
| DAT_00b5e8dc | -5 | 0xc0a00000 | 2 | formula-getter default / tuning constant |
| DAT_00b5e8e8 | 16 | 0x41800000 | 2 | formula-getter default / tuning constant |
| DAT_00b6016c | 10 | 0x41200000 | 26 | formula-getter default / tuning constant |
| DAT_00b60170 | -2 | 0xc0000000 | 3 | formula-getter default / tuning constant |
| DAT_00b60190 | 0.6 | 0x3f19999a | 5 | formula-getter default / tuning constant |
| DAT_00b60a10 | -0.5 | 0xbf000000 | 2 | formula-getter default / tuning constant |
| DAT_00b611e8 | 0.15 | 0x3e19999a | 1 | formula-getter default / tuning constant |
| DAT_00b61970 | 10000 | 0x461c4000 | 1 | formula-getter default / tuning constant |
| DAT_00b625a4 | 150 | 0x43160000 | 1 | formula-getter default / tuning constant |
| DAT_00b62960 | 75 | 0x42960000 | 4 | formula-getter default / tuning constant |
| DAT_00b63280 | 30 | 0x41f00000 | 4 | formula-getter default / tuning constant |
| DAT_00b73990 | 70 | 0x428c0000 | 2 | formula-getter default / tuning constant |
| DAT_00b8bb40 | 2.5 | 0x40200000 | 1 | formula-getter default / tuning constant |
| DAT_00b95908 | -10 | 0xc1200000 | 1 | formula-getter default / tuning constant |
| DAT_00b99dd4 | 0.66 | 0x3f28f5c3 | 1 | formula-getter default / tuning constant |
| DAT_00ba1708 | 15 | 0x41700000 | 10 | formula-getter default / tuning constant |
| DAT_00ba170c | 45 | 0x42340000 | 1 | formula-getter default / tuning constant |
| DAT_00ba1710 | -4 | 0xc0800000 | 1 | formula-getter default / tuning constant |
| DAT_00ba1714 | 100 | 0x42c80000 | 150 | default=100 — dominant default (player attrs / percentages), 150 getters |
| DAT_00ba9b50 | 0.45 | 0x3ee66666 | 1 | formula-getter default / tuning constant |
| DAT_00baba44 | 0.33 | 0x3ea8f5c3 | 1 | formula-getter default / tuning constant |
| DAT_00baca60 | 0.458 | 0x3eea7efa | 1 | formula-getter default / tuning constant |
| DAT_00bae644 | 700 | 0x442f0000 | 2 | formula-getter default / tuning constant |
| DAT_00bae660 | 81 | 0x42a20000 | 1 | formula-getter default / tuning constant |
| DAT_00bae67c | 22.5 | 0x41b40000 | 1 | formula-getter default / tuning constant |
| DAT_00bae6b4 | 28 | 0x41e00000 | 2 | formula-getter default / tuning constant |
| DAT_00bae80c | 625 | 0x441c4000 | 1 | formula-getter default / tuning constant |
| DAT_00baeb88 | 1.75 | 0x3fe00000 | 1 | formula-getter default / tuning constant |
| DAT_00baec40 | 0.004 | 0x3b83126f | 1 | formula-getter default / tuning constant |
| DAT_00baeda4 | -0.7 | 0xbf333333 | 1 | formula-getter default / tuning constant |
| DAT_00baedbc | -0.05 | 0xbd4ccccd | 1 | formula-getter default / tuning constant |
| DAT_00baee08 | -0.6 | 0xbf19999a | 1 | formula-getter default / tuning constant |
| DAT_00baee20 | -0.4 | 0xbecccccd | 3 | formula-getter default / tuning constant |
| DAT_00baf540 | 0.003 | 0x3b449ba6 | 1 | formula-getter default / tuning constant |
| DAT_00baf57c | 0.28 | 0x3e8f5c29 | 2 | formula-getter default / tuning constant |
| DAT_00baf614 | 9.5 | 0x41180000 | 1 | formula-getter default / tuning constant |
| DAT_00baf650 | 0.025 | 0x3ccccccd | 1 | formula-getter default / tuning constant |
| DAT_00baf774 | -3 | 0xc0400000 | 1 | formula-getter default / tuning constant |
| DAT_00bb086c | 540 | 0x44070000 | 1 | formula-getter default / tuning constant |
| DAT_00bb14e4 | 95 | 0x42be0000 | 1 | formula-getter default / tuning constant |
| DAT_00bb1898 | 55 | 0x425c0000 | 2 | formula-getter default / tuning constant |
| DAT_00bbae9c | 1.5 | 0x3fc00000 | 6 | formula-getter default / tuning constant |
| DAT_00bbb51c | 0.5 | 0x3f000000 | 13 | formula-getter default / tuning constant |
| DAT_00bd6bf8 | 1.1 | 0x3f8ccccd | 3 | formula-getter default / tuning constant |
| DAT_00bd7260 | 6 | 0x40c00000 | 21 | formula-getter default / tuning constant |
| DAT_00bd7268 | 0.75 | 0x3f400000 | 1 | formula-getter default / tuning constant |

## 4. Registered formula-name registry (810 getters, 39 groups)

Each row: a `<formula>`/`<rule>` **name** looked up via `GCFormulaModule::Find(name, group)`, its cached `GCFormula*` handle slot in `.data`, the getter function, and the **default** value returned when `formula.xml` lacks the entry (`-` = computed, no literal fallback). Names & groups are VERIFIED (string literals); the actual formula TREE behind each name is **NOT** in the exe (Section 6).

### Group taxonomy

| group | #getters |
|---|---|
| PlayerData | 187 |
| PlayerConstant | 101 |
| ProspectiveManagers | 77 |
| MatchResolver | 51 |
| PlayerState | 41 |
| PlayerHappiness | 37 |
| BoardRequest | 37 |
| SquadData | 36 |
| TrainingFacilities | 30 |
| ResolverConstants | 26 |
| TeamSelection | 26 |
| WageLimits | 19 |
| OfficialState | 18 |
| ManagerSacking | 17 |
| PersonHappiness | 17 |
| Reputation | 10 |
| TransferModule | 9 |
| Budget | 9 |
| MatchReport | 6 |
| PhysioInjuries | 5 |
| PhysioReport | 5 |
| CoachReport | 5 |
| PlayerReputationPostMatchProgression | 5 |
| ClubInfo | 4 |
| ManagerDiscipline | 4 |
| CompetitionIncome | 3 |
| TransferListNotify | 3 |
| PostMatchMutation | 3 |
| FinancialCosts | 3 |
| NewPlayerSalary | 3 |
| PersonAgeing | 2 |
| RollForwardData | 2 |
| PlayerDegradeable | 2 |
| TrainingModule | 2 |
| PlayerFluid | 1 |
| (key = name itself: multi-formula group) | 1 |
| MatchModule | 1 |
| PlayerProfessionalism | 1 |
| BoardAI | 1 |

### 4a. MatchResolver (player movement physics defaults) — 51 names

| name | default | default DAT | handle slot | getter |
|---|---|---|---|---|
| BackingAccelerationMultiplier | 0.9 | DAT_00b5b114 | DAT_00c8fb18 | FUN_00830be0 |
| BackingDecelerationMultiplier | 0.2 | DAT_00b5b388 | DAT_00c8fb20 | FUN_00830cd0 |
| DivingLeftAccelerationMultiplier | 6 | DAT_00bd7260 | DAT_00c8fb78 | FUN_00831720 |
| DivingLeftDecelerationMultiplier | 8 | DAT_00b5b6cc | DAT_00c8fb80 | FUN_00831810 |
| DivingRightAccelerationMultiplier | 6 | DAT_00bd7260 | DAT_00c8fb98 | FUN_00831ae0 |
| DivingRightDecelerationMultiplier | 8 | DAT_00b5b6cc | DAT_00c8fba0 | FUN_00831bd0 |
| JoggingAccelerationMultiplier | 1.5 | DAT_00bbae9c | DAT_00c8fae8 | FUN_00830640 |
| JoggingDecelerationMultiplier | - | - | DAT_00c8faf0 | FUN_00830730 |
| MaxBackingSpeed | 6 | DAT_00bd7260 | DAT_00c8fb30 | FUN_00830eb0 |
| MaxBaseAcceleration | 3 | DAT_00b5b760 | DAT_00c8fa70 | FUN_0082f830 |
| MaxBaseDeceleration | -5 | DAT_00b5e8dc | DAT_00c8fa80 | FUN_0082fa10 |
| MaxDivingLeftSpeed | 9 | DAT_00b5b5f4 | DAT_00c8fb90 | FUN_008319f0 |
| MaxDivingRightSpeed | 9 | DAT_00b5b5f4 | DAT_00c8fbb0 | FUN_00831db0 |
| MaxJoggingSpeed | 5 | DAT_00b5b76c | DAT_00c8faa0 | FUN_0082fdd0 |
| MaxRunningSpeed | 7 | DAT_00b5bb74 | DAT_00c8fab0 | FUN_0082ffb0 |
| MaxSidlingLeftSpeed | 6 | DAT_00bd7260 | DAT_00c8fb50 | FUN_00831270 |
| MaxSidlingRightSpeed | 6 | DAT_00bd7260 | DAT_00c8fb70 | FUN_00831630 |
| MaxSlidingSpeed | 9 | DAT_00b5b5f4 | DAT_00c8fbd0 | FUN_00832170 |
| MaxSpeedToTurningCircleRadius | 0.3 | DAT_00b5b5ec | DAT_00c8fa60 | FUN_0082f650 |
| MaxSprintingSpeed | 9 | DAT_00b5b5f4 | DAT_00c8fac0 | FUN_00830190 |
| MaxWalkingSpeed | 3 | DAT_00b5b760 | DAT_00c8fa90 | FUN_0082fbf0 |
| MinBackingSpeed | 5 | DAT_00b5b76c | DAT_00c8fb28 | FUN_00830dc0 |
| MinBaseAcceleration | 2 | DAT_00b18028 | DAT_00c8fa68 | FUN_0082f740 |
| MinBaseDeceleration | -4 | DAT_00ba1710 | DAT_00c8fa78 | FUN_0082f920 |
| MinDivingLeftSpeed | 8 | DAT_00b5b6cc | DAT_00c8fb88 | FUN_00831900 |
| MinDivingRightSpeed | 8 | DAT_00b5b6cc | DAT_00c8fba8 | FUN_00831cc0 |
| MinJoggingSpeed | 3 | DAT_00b5b760 | DAT_00c8fa98 | FUN_0082fce0 |
| MinRunningSpeed | 6 | DAT_00bd7260 | DAT_00c8faa8 | FUN_0082fec0 |
| MinSidlingLeftSpeed | 5 | DAT_00b5b76c | DAT_00c8fb48 | FUN_00831180 |
| MinSidlingRightSpeed | 5 | DAT_00b5b76c | DAT_00c8fb68 | FUN_00831540 |
| MinSlidingSpeed | 8 | DAT_00b5b6cc | DAT_00c8fbc8 | FUN_00832080 |
| MinSpeedToTurningCircleRadius | 0.15 | DAT_00b611e8 | DAT_00c8fa58 | FUN_0082f560 |
| MinSprintingSpeed | 8 | DAT_00b5b6cc | DAT_00c8fab8 | FUN_008300a0 |
| MinWalkingSpeed | 2 | DAT_00b18028 | DAT_00c8fa88 | FUN_0082fb00 |
| RunningAccelerationMultiplier | 2.5 | DAT_00b8bb40 | DAT_00c8faf8 | FUN_00830820 |
| RunningDecelerationMultiplier | - | - | DAT_00c8fb00 | FUN_00830910 |
| RunningFatigueModifier | 0.002 | DAT_00b5bad8 | DAT_00c8fa48 | FUN_0082f380 |
| SidlingLeftAccelerationMultiplier | 1.5 | DAT_00bbae9c | DAT_00c8fb38 | FUN_00830fa0 |
| SidlingLeftDecelerationMultiplier | 2 | DAT_00b18028 | DAT_00c8fb40 | FUN_00831090 |
| SidlingRightAccelerationMultiplier | 1.5 | DAT_00bbae9c | DAT_00c8fb58 | FUN_00831360 |
| SidlingRightDecelerationMultiplier | 2 | DAT_00b18028 | DAT_00c8fb60 | FUN_00831450 |
| SlidingAccelerationMultiplier | 6 | DAT_00bd7260 | DAT_00c8fbb8 | FUN_00831ea0 |
| SlidingDecelerationMultiplier | 8 | DAT_00b5b6cc | DAT_00c8fbc0 | FUN_00831f90 |
| SprintingAccelerationMultiplier | 5 | DAT_00b5b76c | DAT_00c8fb08 | FUN_00830a00 |
| SprintingDecelerationMultiplier | - | - | DAT_00c8fb10 | FUN_00830af0 |
| SprintingFatigueModifier | 0.02 | DAT_00b5e8c4 | DAT_00c8fa50 | FUN_0082f470 |
| StandingAccelerationMultiplier | - | - | DAT_00c8fac8 | FUN_00830280 |
| StandingDecelerationMultiplier | - | - | DAT_00c8fad0 | FUN_00830370 |
| UpdateDegradableStatsPeriod | 60 | DAT_00b5b1d8 | DAT_00c8fa40 | FUN_0082f290 |
| WalkingAccelerationMultiplier | - | - | DAT_00c8fad8 | FUN_00830460 |
| WalkingDecelerationMultiplier | - | - | DAT_00c8fae0 | FUN_00830550 |

### 4b. ResolverConstants (ball physics — all computed) — 26 names

| name | default | default DAT | handle slot | getter |
|---|---|---|---|---|
| AccurateShotAngleVariance | - | - | DAT_00c8f428 | FUN_00823480 |
| AccurateShotRangeVariance | - | - | DAT_00c8f420 | FUN_00823390 |
| BallElasticity | - | - | DAT_00c8f418 | FUN_008232a0 |
| BaseBallKickHeightDirectional | - | - | DAT_00c8f3b8 | FUN_00822760 |
| BaseBallKickHeightFlat | - | - | DAT_00c8f3a8 | FUN_00822580 |
| BaseBallKickHeightHead | - | - | DAT_00c8f3b0 | FUN_00822670 |
| BaseBallKickHeightHigh | - | - | DAT_00c8f3c8 | FUN_00822940 |
| BaseBallKickHeightMaximum | - | - | DAT_00c8f3d0 | FUN_00822a30 |
| BaseBallKickHeightOverhead | - | - | DAT_00c8f3c0 | FUN_00822850 |
| BaseBallSpeedFirm | - | - | DAT_00c8f388 | FUN_008221c0 |
| BaseBallSpeedFirmer | - | - | DAT_00c8f390 | FUN_008222b0 |
| BaseBallSpeedGentle | - | - | DAT_00c8f370 | FUN_00821ef0 |
| BaseBallSpeedMaximum | - | - | DAT_00c8f3a0 | FUN_00822490 |
| BaseBallSpeedMedium | - | - | DAT_00c8f378 | FUN_00821fe0 |
| BaseBallSpeedMediumFirm | - | - | DAT_00c8f380 | FUN_008220d0 |
| BaseBallSpeedStrong | - | - | DAT_00c8f398 | FUN_008223a0 |
| BaseKickReactionTimeFirm | - | - | DAT_00c8f3f0 | FUN_00822df0 |
| BaseKickReactionTimeFirmer | - | - | DAT_00c8f3f8 | FUN_00822ee0 |
| BaseKickReactionTimeGentle | - | - | DAT_00c8f3d8 | FUN_00822b20 |
| BaseKickReactionTimeMaximum | - | - | DAT_00c8f408 | FUN_008230c0 |
| BaseKickReactionTimeMedium | - | - | DAT_00c8f3e0 | FUN_00822c10 |
| BaseKickReactionTimeMediumFirm | - | - | DAT_00c8f3e8 | FUN_00822d00 |
| BaseKickReactionTimeStrong | - | - | DAT_00c8f400 | FUN_00822fd0 |
| BaseThrowReactionTime | - | - | DAT_00c8f410 | FUN_008231b0 |
| InaccurateShotAngleVariance | - | - | DAT_00c8f438 | FUN_00823660 |
| InaccurateShotRangeVariance | - | - | DAT_00c8f430 | FUN_00823570 |

### 4c. PlayerData (match-engine physics + *Performance metrics) — 187 names

| name | default | default DAT | handle slot | getter |
|---|---|---|---|---|
| ActiveRangeScale | 2 | DAT_00b18028 | DAT_00c90290 | FUN_0083ece0 |
| AntiWidthBias | 4 | DAT_00b5b764 | DAT_00c90230 | FUN_0083e1a0 |
| AssistsPerformance | 0.6 | DAT_00b60190 | DAT_00c900d0 | FUN_0083b860 |
| AttackerGoalPerformance | 4 | DAT_00b5b764 | DAT_00c90070 | FUN_0083ad20 |
| AttackHeaderDistance | 20 | DAT_00b5b600 | DAT_00c901a8 | FUN_0083d1b0 |
| BadCrossesPerformance | 0.05 | DAT_00b5b6c4 | DAT_00c900c0 | FUN_0083b680 |
| BadPassesPerformance | -0.2 | DAT_00b5c0e8 | DAT_00c90040 | FUN_0083a780 |
| BadShotsPerformance | -0.1 | DAT_00b5bb94 | DAT_00c90058 | FUN_0083aa50 |
| BadTacklesPerformance | -0.05 | DAT_00baedbc | DAT_00c90088 | FUN_0083aff0 |
| BallSpeedEffect | 0.004 | DAT_00baec40 | DAT_00c90008 | FUN_0083a0f0 |
| BehindBallScale | 0.5 | DAT_00bbb51c | DAT_00c8fd58 | FUN_00835050 |
| BestFeint | 3 | DAT_00b5b760 | DAT_00c8feb8 | FUN_00837990 |
| BlockDeflection | - | - | DAT_00c8ff00 | FUN_00838200 |
| BlockEarliestLost | - | - | DAT_00c8ff18 | FUN_008384d0 |
| BlockEarliestWon | - | - | DAT_00c8ff20 | FUN_008385c0 |
| BlockTackleRandom | - | - | DAT_00c8fef8 | FUN_00838110 |
| CatchDistance | - | - | DAT_00c90248 | FUN_0083e470 |
| CleanSheetKeeperDefenderPerformance | 4 | DAT_00b5b764 | DAT_00c90170 | FUN_0083cb20 |
| CleanSheetMidfielderPerformance | - | - | DAT_00c90178 | FUN_0083cc10 |
| ClearancePerformance | 0.1 | DAT_00b5b758 | DAT_00c900f8 | FUN_0083bd10 |
| CloseDive | 2 | DAT_00b18028 | DAT_00c90250 | FUN_0083e560 |
| CloseInMarking | 30 | DAT_00b63280 | DAT_00c90280 | FUN_0083eb00 |
| CloseKick | - | - | DAT_00c90258 | FUN_0083e650 |
| ClosePassDistance | 8 | DAT_00b5b6cc | DAT_00c8fda8 | FUN_008359b0 |
| CollisionInjuryChance | 0.4 | DAT_00b5bac0 | DAT_00c8fd10 | FUN_008347e0 |
| CommitToRun | 0.1 | DAT_00b5b758 | DAT_00c8fd50 | FUN_00834f60 |
| CompletedPassesPerformance | 0.1 | DAT_00b5b758 | DAT_00c90038 | FUN_0083a690 |
| CrossAbilityScale | 200 | DAT_00b5b190 | DAT_00c8ffd8 | FUN_00839b50 |
| CrossAreaWidth | 15 | DAT_00ba1708 | DAT_00c8ffb0 | FUN_008396a0 |
| CrossesFumbledPerformance | -0.4 | DAT_00baee20 | DAT_00c90138 | FUN_0083c490 |
| CrossesPunchedPerformance | 0.2 | DAT_00b5b388 | DAT_00c90130 | FUN_0083c3a0 |
| CrossesTakenPerformance | 0.3 | DAT_00b5b5ec | DAT_00c90128 | FUN_0083c2b0 |
| CutInDistance | 28 | DAT_00bae6b4 | DAT_00c8fd90 | FUN_008356e0 |
| CynicalPerformance | -0.4 | DAT_00baee20 | DAT_00c900a8 | FUN_0083b3b0 |
| DeepAreaLength | 40 | DAT_00b5b6b4 | DAT_00c8ffb8 | FUN_00839790 |
| DeepOwnHalfRun | -30 | DAT_00b5b79c | DAT_00c8fe70 | FUN_00837120 |
| DirtyBallDeflection | - | - | DAT_00c8ff80 | FUN_00839100 |
| DirtyBallEffect | - | - | DAT_00c8ff88 | FUN_008391f0 |
| DirtyFoulResult | - | - | DAT_00c8ff48 | FUN_00838a70 |
| DirtyPerformance | -0.6 | DAT_00baee08 | DAT_00c900a0 | FUN_0083b2c0 |
| DiveDistance | - | - | DAT_00c8fe58 | FUN_00836e50 |
| DummyAngle1 | - | - | DAT_00c8ffa8 | FUN_008395b0 |
| EaseOfDribble | 45 | DAT_00ba170c | DAT_00c8fda0 | FUN_008358c0 |
| EasyPass | - | - | DAT_00c8fdb0 | FUN_00835aa0 |
| ElbowBallRange | 6 | DAT_00bd7260 | DAT_00c8fee8 | FUN_00837f30 |
| ElbowEffect | - | - | DAT_00c8ff08 | FUN_008382f0 |
| ElbowingTime | - | - | DAT_00c8fef0 | FUN_00838020 |
| ElbowRange | 4 | DAT_00b5b764 | DAT_00c8fee0 | FUN_00837e40 |
| ExtraHeadRange | 8 | DAT_00b5b6cc | DAT_00c901d0 | FUN_0083d660 |
| ExtraSaveReach | 2 | DAT_00b18028 | DAT_00c90240 | FUN_0083e380 |
| FailedControlPerformance | -0.2 | DAT_00b5c0e8 | DAT_00c900e0 | FUN_0083ba40 |
| FeedCloseRange | 40 | DAT_00b5b6b4 | DAT_00c8fdf8 | FUN_00836310 |
| FeedScale | - | - | DAT_00c8fdf0 | FUN_00836220 |
| FeintRange | - | - | DAT_00c8fdd0 | FUN_00835e60 |
| FeintRangeStart | 5 | DAT_00b5b76c | DAT_00c8fdc8 | FUN_00835d70 |
| FreeKickOffsideLineScale | 0.45 | DAT_00ba9b50 | DAT_00c8fd30 | FUN_00834ba0 |
| GoalConcededDefenderPerformance | -0.5 | DAT_00b60a10 | DAT_00c90168 | FUN_0083ca30 |
| GoalConcededKeeperPerformance | -0.5 | DAT_00b60a10 | DAT_00c90160 | FUN_0083c940 |
| GoallineClearance | 4 | DAT_00b5b764 | DAT_00c901e0 | FUN_0083d840 |
| GoodCrossesPerformance | 0.2 | DAT_00b5b388 | DAT_00c900b8 | FUN_0083b590 |
| GroundCoveredPerformance | - | - | DAT_00c900c8 | FUN_0083b770 |
| HalfVolleyHeight | 0.6 | DAT_00b60190 | DAT_00c90208 | FUN_0083dcf0 |
| HeadAgility | 0.25 | DAT_00b5b798 | DAT_00c8fff8 | FUN_00839f10 |
| HeaderFinishSpan | 2 | DAT_00b18028 | DAT_00c901a0 | FUN_0083d0c0 |
| HeadersPerformance | 0.1 | DAT_00b5b758 | DAT_00c90100 | FUN_0083be00 |
| HeaderTimeSpan | - | - | DAT_00c90198 | FUN_0083cfd0 |
| HeaderTooClose | 16 | DAT_00b5e8e8 | DAT_00c90238 | FUN_0083e290 |
| HeadFxScale | 700 | DAT_00bae644 | DAT_00c8fd68 | FUN_00835230 |
| HighCrossHeight | 1.75 | DAT_00baeb88 | DAT_00c8ffc8 | FUN_00839970 |
| HighGoodControl | - | - | DAT_00c8fe98 | FUN_008375d0 |
| HighPassAngleRange | - | - | DAT_00c8fe38 | FUN_00836a90 |
| HighPassDifficulty | 33.75 | DAT_00b59ce8 | DAT_00c8fd80 | FUN_00835500 |
| HookAheadTarget | - | - | DAT_00c8ff68 | FUN_00838e30 |
| HookBallDeflection | - | - | DAT_00c8ff70 | FUN_00838f20 |
| HookBallEffect | - | - | DAT_00c8ff78 | FUN_00839010 |
| HookFoulResult | 0.3 | DAT_00b5b5ec | DAT_00c8ff50 | FUN_00838b60 |
| HookTackleBallArea | 6 | DAT_00bd7260 | DAT_00c8ff58 | FUN_00838c50 |
| HuntRangeScale | - | - | DAT_00c90298 | FUN_0083edd0 |
| InterceptionsPerformance | 0.2 | DAT_00b5b388 | DAT_00c900e8 | FUN_0083bb30 |
| KeeperDefenderGoalPerformance | 6 | DAT_00bd7260 | DAT_00c90060 | FUN_0083ab40 |
| KeptPossessionPerformance | 0.1 | DAT_00b5b758 | DAT_00c900d8 | FUN_0083b950 |
| KeyHeadersPerformance | 0.3 | DAT_00b5b5ec | DAT_00c90108 | FUN_0083bef0 |
| KeyPassesPerformance | 0.5 | DAT_00bbb51c | DAT_00c90048 | FUN_0083a870 |
| KeyTacklesPerformance | 0.3 | DAT_00b5b5ec | DAT_00c90098 | FUN_0083b1d0 |
| KickFxScale | 700 | DAT_00bae644 | DAT_00c8fd60 | FUN_00835140 |
| LayOffRange | 10 | DAT_00b6016c | DAT_00c901f0 | FUN_0083da20 |
| LollyPopAngle1 | - | - | DAT_00c8ffa0 | FUN_008394c0 |
| LongRangeBicycle | 15 | DAT_00ba1708 | DAT_00c90210 | FUN_0083dde0 |
| LongRangeDivingHeader | 15 | DAT_00ba1708 | DAT_00c901b0 | FUN_0083d2a0 |
| LongRangeHeader | 15 | DAT_00ba1708 | DAT_00c901b8 | FUN_0083d390 |
| LongRangeVolley | 15 | DAT_00ba1708 | DAT_00c90200 | FUN_0083dc00 |
| LooseMarking | 8 | DAT_00b5b6cc | DAT_00c90270 | FUN_0083e920 |
| LostPossessionPerformance | -0.1 | DAT_00b5bb94 | DAT_00c90120 | FUN_0083c1c0 |
| LowCrossHeight | 0.5 | DAT_00bbb51c | DAT_00c8ffc0 | FUN_00839880 |
| LowGoodControl | 0.3 | DAT_00b5b5ec | DAT_00c8fe90 | FUN_008374e0 |
| LungeDistance | 4 | DAT_00b5b764 | DAT_00c901d8 | FUN_0083d750 |
| MarkingDistanceScale | 0.02 | DAT_00b5e8c4 | DAT_00c90278 | FUN_0083ea10 |
| MaxBlockTackleResult | 35 | DAT_00b5baf8 | DAT_00c8fea8 | FUN_008377b0 |
| MaxDistanceFromAreaCrossTarget | 12 | DAT_00b5b754 | DAT_00c8ffe8 | FUN_00839d30 |
| MaxDistanceFromManCrossTarget | 8 | DAT_00b5b6cc | DAT_00c8ffe0 | FUN_00839c40 |
| MaxHighPassSafety | 10 | DAT_00b6016c | DAT_00c8fe18 | FUN_008366d0 |
| MaxPassSafety | 6 | DAT_00bd7260 | DAT_00c8fe08 | FUN_008364f0 |
| MaxReactionTime | - | - | DAT_00c90260 | FUN_0083e740 |
| MaxRoamDistance | 80 | DAT_00b5b10c | DAT_00c902a0 | FUN_0083eec0 |
| MaxSpacePassSafety | 10 | DAT_00b6016c | DAT_00c8fe20 | FUN_008367c0 |
| MentalFatigue | 0.4 | DAT_00b5bac0 | DAT_00c8fd20 | FUN_008349c0 |
| MidfielderGoalPerformance | 5 | DAT_00b5b76c | DAT_00c90068 | FUN_0083ac30 |
| MinBlockTackleResult | 10 | DAT_00b6016c | DAT_00c8fea0 | FUN_008376c0 |
| MinDeflection | 0.5 | DAT_00bbb51c | DAT_00c8feb0 | FUN_008378a0 |
| MinDistanceToCrossTarget | 20 | DAT_00b5b600 | DAT_00c8fff0 | FUN_00839e20 |
| MinHighSpacePassDistance | 625 | DAT_00bae80c | DAT_00c8fe30 | FUN_008369a0 |
| MinimumDribbleCaution | 8 | DAT_00b5b6cc | DAT_00c8fdd8 | FUN_00835f50 |
| MinimumHeadRange | 8 | DAT_00b5b6cc | DAT_00c901c8 | FUN_0083d570 |
| MinimumPassRangeSquared | 81 | DAT_00bae660 | DAT_00c8fd70 | FUN_00835320 |
| MinPassSafety | 4 | DAT_00b5b764 | DAT_00c8fe10 | FUN_008365e0 |
| MinShot | 20 | DAT_00b5b600 | DAT_00c8fd98 | FUN_008357d0 |
| MinSpacePassAngle | - | - | DAT_00c8fe48 | FUN_00836c70 |
| MinSpacePassDistance | 100 | DAT_00ba1714 | DAT_00c8fe28 | FUN_008368b0 |
| MinValidPassSpeed | 10 | DAT_00b6016c | DAT_00c8fe88 | FUN_008373f0 |
| MustShootRange | 15 | DAT_00ba1708 | DAT_00c8fd88 | FUN_008355f0 |
| OffsideLineScale | 0.6 | DAT_00b60190 | DAT_00c8fd38 | FUN_00834c90 |
| OffsideLineScaleMax | 0.9 | DAT_00b5b114 | DAT_00c8fd48 | FUN_00834e70 |
| OffsideLineScaleMin | 0.3 | DAT_00b5b5ec | DAT_00c8fd40 | FUN_00834d80 |
| OffsidePerformance | -0.02 | DAT_00b5baf0 | DAT_00c90110 | FUN_0083bfe0 |
| OnTargetShotsPerformance | 0.3 | DAT_00b5b5ec | DAT_00c90050 | FUN_0083a960 |
| OwnGoalsPerformance | -0.7 | DAT_00baeda4 | DAT_00c90080 | FUN_0083af00 |
| OwnHalfRun | -30 | DAT_00b5b79c | DAT_00c8fe68 | FUN_00837030 |
| PassDifficulty | 22.5 | DAT_00bae67c | DAT_00c8fd78 | FUN_00835410 |
| PenaltyMissedPerformance | -2 | DAT_00b60170 | DAT_00c90188 | FUN_0083cdf0 |
| PenaltySavedPerformance | 5 | DAT_00b5b76c | DAT_00c90180 | FUN_0083cd00 |
| PenGoalsPerformance | 0.65 | DAT_00b5b198 | DAT_00c90078 | FUN_0083ae10 |
| PerformanceBalancing | 1.1 | DAT_00bd6bf8 | DAT_00c90190 | FUN_0083cee0 |
| PhysicalFatigue | 0.4 | DAT_00b5bac0 | DAT_00c8fd18 | FUN_008348d0 |
| PoorAngleBias | 0.5 | DAT_00bbb51c | DAT_00c8fde8 | FUN_00836130 |
| PressureJumpDistance | 4 | DAT_00b5b764 | DAT_00c901c0 | FUN_0083d480 |
| PullShirtFrequency | 0.05 | DAT_00b5b6c4 | DAT_00c8fed8 | FUN_00837d50 |
| PullShirtTime | - | - | DAT_00c8fed0 | FUN_00837c60 |
| RedCardPerformance | -2 | DAT_00b60170 | DAT_00c90020 | FUN_0083a3c0 |
| RunShotDifficulty | - | - | DAT_00c8fe78 | FUN_00837210 |
| RunsPerformance | 0.1 | DAT_00b5b758 | DAT_00c900f0 | FUN_0083bc20 |
| SetupPower | 64 | DAT_00b3f2e0 | DAT_00c8fd00 | FUN_00834600 |
| ShirtPullingBallRange | 6 | DAT_00bd7260 | DAT_00c8fec8 | FUN_00837b70 |
| ShirtPullingEffect | - | - | DAT_00c8ff10 | FUN_008383e0 |
| ShirtPullingRange | 4 | DAT_00b5b764 | DAT_00c8fec0 | FUN_00837a80 |
| ShortPassChoiceDamper | 0.25 | DAT_00b5b798 | DAT_00c8fe00 | FUN_00836400 |
| ShotsFumbledPerformance | -0.4 | DAT_00baee20 | DAT_00c90158 | FUN_0083c850 |
| ShotsHeldPerformance | 0.3 | DAT_00b5b5ec | DAT_00c90140 | FUN_0083c580 |
| ShotsParriedPerformance | 0.2 | DAT_00b5b388 | DAT_00c90148 | FUN_0083c670 |
| ShotsTippedPerformance | 0.2 | DAT_00b5b388 | DAT_00c90150 | FUN_0083c760 |
| SidePass | - | - | DAT_00c8fdc0 | FUN_00835c80 |
| SkillFatigue | 0.2 | DAT_00b5b388 | DAT_00c8fd28 | FUN_00834ab0 |
| SkipDelay | 2 | DAT_00b18028 | DAT_00c8fd08 | FUN_008346f0 |
| SlideAheadTarget | - | - | DAT_00c8ff90 | FUN_008392e0 |
| SlideBallDeflection | - | - | DAT_00c902c8 | FUN_0083f370 |
| SlideBallEffect | - | - | DAT_00c902d0 | FUN_0083f460 |
| SlideFoulResult | 0.6 | DAT_00b60190 | DAT_00c90220 | FUN_0083dfc0 |
| SlideTackleBallArea | 6 | DAT_00bd7260 | DAT_00c90218 | FUN_0083ded0 |
| SlideTimeSpan | - | - | DAT_00c90228 | FUN_0083e0b0 |
| SmotherBallDeflection | - | - | DAT_00c90010 | FUN_0083a1e0 |
| SmotherBallEffect | - | - | DAT_00c90018 | FUN_0083a2d0 |
| SmotherFoulResult | 0.6 | DAT_00b60190 | DAT_00c90000 | FUN_0083a000 |
| SpacePassAngleRange | - | - | DAT_00c8fe40 | FUN_00836b80 |
| SpinAngle1 | - | - | DAT_00c8ff98 | FUN_008393d0 |
| SprintDistance | 20 | DAT_00b5b600 | DAT_00c8fe60 | FUN_00836f40 |
| SprintShotDifficulty | - | - | DAT_00c8fe80 | FUN_00837300 |
| StabDistance | 6 | DAT_00bd7260 | DAT_00c901f8 | FUN_0083db10 |
| StandardDribbleCaution | 12 | DAT_00b5b754 | DAT_00c8fde0 | FUN_00836040 |
| StraightPass | - | - | DAT_00c8fdb8 | FUN_00835b90 |
| TacklesAvoidedPerformance | 0.1 | DAT_00b5b758 | DAT_00c900b0 | FUN_0083b4a0 |
| TacklesWonPerformance | 0.1 | DAT_00b5b758 | DAT_00c90090 | FUN_0083b0e0 |
| TightAngleBicycle | - | - | DAT_00c902c0 | FUN_0083f280 |
| TightAngleDivingHeader | - | - | DAT_00c902b0 | FUN_0083f0a0 |
| TightAngleHeader | - | - | DAT_00c902a8 | FUN_0083efb0 |
| TightAngleVolley | - | - | DAT_00c902b8 | FUN_0083f190 |
| TightMarking | 3 | DAT_00b5b760 | DAT_00c90268 | FUN_0083e830 |
| TooCloseToCurl | 15 | DAT_00ba1708 | DAT_00c8fe50 | FUN_00836d60 |
| TouchesPerformance | 0.01 | DAT_00b59d7c | DAT_00c90030 | FUN_0083a5a0 |
| TouchlineAreaLength | 12 | DAT_00b5b754 | DAT_00c8ffd0 | FUN_00839a60 |
| TrickDistance | 15 | DAT_00ba1708 | DAT_00c8ff40 | FUN_00838980 |
| TricksPerformance | 0.03 | DAT_00b5b234 | DAT_00c90118 | FUN_0083c0d0 |
| TripBallRange | 6 | DAT_00bd7260 | DAT_00c8ff30 | FUN_008387a0 |
| TripEffect | - | - | DAT_00c8ff60 | FUN_00838d40 |
| TrippingTime | - | - | DAT_00c8ff38 | FUN_00838890 |
| TripRange | 4 | DAT_00b5b764 | DAT_00c8ff28 | FUN_008386b0 |
| UnderPressure | 6 | DAT_00bd7260 | DAT_00c901e8 | FUN_0083d930 |
| UninterestedRangeScale | 3 | DAT_00b5b760 | DAT_00c90288 | FUN_0083ebf0 |
| YellowCardPerformance | -1 | DAT_00b1801c | DAT_00c90028 | FUN_0083a4b0 |

### 4d. PlayerConstant (player attributes) — 101 names

| name | default | default DAT | handle slot | getter |
|---|---|---|---|---|
| ActingAbility | 100 | DAT_00ba1714 | DAT_00c8efa8 | FUN_00819890 |
| Adaptability | 100 | DAT_00ba1714 | DAT_00c8f1f0 | FUN_0081ea10 |
| Aggression | 100 | DAT_00ba1714 | DAT_00c8f1d0 | FUN_0081e590 |
| Alertness | 100 | DAT_00ba1714 | DAT_00c8ef90 | FUN_00819530 |
| Ambition | 100 | DAT_00ba1714 | DAT_00c8f1e0 | FUN_0081e7d0 |
| AttackingMidAbility | 100 | DAT_00ba1714 | DAT_00c8f248 | FUN_0081f670 |
| BallControl | 100 | DAT_00ba1714 | DAT_00c8f160 | FUN_0081d5d0 |
| BlockTackleStrength | 100 | DAT_00ba1714 | DAT_00c8ef88 | FUN_00819410 |
| BodyBalance | 100 | DAT_00ba1714 | DAT_00c8efd0 | FUN_00819e30 |
| BodyStrength | 100 | DAT_00ba1714 | DAT_00c8efc8 | FUN_00819d10 |
| CentralAbility | 100 | DAT_00ba1714 | DAT_00c8f268 | FUN_0081faf0 |
| CentralDefenderAbility | 100 | DAT_00ba1714 | DAT_00c8f220 | FUN_0081f0d0 |
| ClearanceAgilityLeft | 100 | DAT_00ba1714 | DAT_00c8f0a8 | FUN_0081bbf0 |
| ClearanceAgilityRight | 100 | DAT_00ba1714 | DAT_00c8f0b0 | FUN_0081bd10 |
| ClearancePower | 100 | DAT_00ba1714 | DAT_00c8f0a0 | FUN_0081bad0 |
| Composure | 100 | DAT_00ba1714 | DAT_00c8ef08 | FUN_00818210 |
| Consistency | 100 | DAT_00ba1714 | DAT_00c8f1c0 | FUN_0081e350 |
| CrossAccuracy | 100 | DAT_00ba1714 | DAT_00c8f1f8 | FUN_0081eb30 |
| CrossAgilityLeft | 100 | DAT_00ba1714 | DAT_00c8f090 | FUN_0081b890 |
| CrossAgilityRight | 100 | DAT_00ba1714 | DAT_00c8f098 | FUN_0081b9b0 |
| CrossRangeRight | 100 | DAT_00ba1714 | DAT_00c8f088 | FUN_0081b770 |
| CurlAbilityLeft | 100 | DAT_00ba1714 | DAT_00c8f168 | FUN_0081d6f0 |
| CurlAbilityRight | 100 | DAT_00ba1714 | DAT_00c8f170 | FUN_0081d810 |
| DeadballTakingSkill | 100 | DAT_00ba1714 | DAT_00c8ef68 | FUN_00818f90 |
| DefensiveMidAbility | 100 | DAT_00ba1714 | DAT_00c8f230 | FUN_0081f310 |
| DegreesControlStepLeft | - | - | DAT_00c8f030 | FUN_0081abb0 |
| DivingHeader | 100 | DAT_00ba1714 | DAT_00c8f150 | FUN_0081d390 |
| DribbleAgility | 100 | DAT_00ba1714 | DAT_00c8efa0 | FUN_00819770 |
| DribbleCaution | 100 | DAT_00ba1714 | DAT_00c8ef18 | FUN_00818450 |
| Fitness | 100 | DAT_00ba1714 | DAT_00c8f028 | FUN_0081aa90 |
| ForwardAbility | 100 | DAT_00ba1714 | DAT_00c8f260 | FUN_0081f9d0 |
| FullBackAbility | 100 | DAT_00ba1714 | DAT_00c8f218 | FUN_0081efb0 |
| GameAwareness | 100 | DAT_00ba1714 | DAT_00c8f120 | FUN_0081ccd0 |
| Handball | 100 | DAT_00ba1714 | DAT_00c8f158 | FUN_0081d4b0 |
| HardTackleFrequency | 100 | DAT_00ba1714 | DAT_00c8efe0 | FUN_0081a070 |
| HeadAccuracy | 100 | DAT_00ba1714 | DAT_00c8f148 | FUN_0081d270 |
| HeadRange | 100 | DAT_00ba1714 | DAT_00c8f140 | FUN_0081d150 |
| Height | 100 | DAT_00ba1714 | DAT_00c8f130 | FUN_0081cf10 |
| KeeperAgility | 100 | DAT_00ba1714 | DAT_00c8f0d8 | FUN_0081c2b0 |
| KeeperBravery | 100 | DAT_00ba1714 | DAT_00c8f0b8 | FUN_0081be30 |
| KeeperCoordinateDefence | 100 | DAT_00ba1714 | DAT_00c8f118 | FUN_0081cbb0 |
| KeeperCrosses | 100 | DAT_00ba1714 | DAT_00c8f0f8 | FUN_0081c730 |
| KeeperEccentricity | 100 | DAT_00ba1714 | DAT_00c8f128 | FUN_0081cdf0 |
| KeeperGrip | 100 | DAT_00ba1714 | DAT_00c8f0e8 | FUN_0081c4f0 |
| KeeperHandStrength | 100 | DAT_00ba1714 | DAT_00c8f0f0 | FUN_0081c610 |
| KeeperKicking | 100 | DAT_00ba1714 | DAT_00c8f100 | FUN_0081c850 |
| KeeperPunching | 100 | DAT_00ba1714 | DAT_00c8f110 | FUN_0081ca90 |
| KeeperReach | 100 | DAT_00ba1714 | DAT_00c8f0d0 | FUN_0081c190 |
| KeeperRecovery | 100 | DAT_00ba1714 | DAT_00c8f0e0 | FUN_0081c3d0 |
| KeeperRunningOut | 100 | DAT_00ba1714 | DAT_00c8f0c0 | FUN_0081bf50 |
| KeeperThrowing | 100 | DAT_00ba1714 | DAT_00c8f108 | FUN_0081c970 |
| Leadership | 100 | DAT_00ba1714 | DAT_00c8f1c8 | FUN_0081e470 |
| LeftFoot | 100 | DAT_00ba1714 | DAT_00c8f208 | FUN_0081ed70 |
| LeftSideAbility | 100 | DAT_00ba1714 | DAT_00c8f270 | FUN_0081fc10 |
| Marking | 100 | DAT_00ba1714 | DAT_00c8f1b8 | FUN_0081e230 |
| MidfieldAbility | 100 | DAT_00ba1714 | DAT_00c8f250 | FUN_0081f790 |
| Mobility | 100 | DAT_00ba1714 | DAT_00c8ef10 | FUN_00818330 |
| OverAggression | 100 | DAT_00ba1714 | DAT_00c8eff8 | FUN_0081a3d0 |
| PassCaution | 100 | DAT_00ba1714 | DAT_00c8ef98 | FUN_00819650 |
| PassCreativity | 100 | DAT_00ba1714 | DAT_00c8f178 | FUN_0081d930 |
| PassingAccuracy | 100 | DAT_00ba1714 | DAT_00c8f1b0 | FUN_0081e110 |
| PassingAgilityLeft | 100 | DAT_00ba1714 | DAT_00c8ef30 | FUN_008187b0 |
| PassingAgilityRight | 100 | DAT_00ba1714 | DAT_00c8ef38 | FUN_008188d0 |
| PassingRangeLeft | 100 | DAT_00ba1714 | DAT_00c8ef20 | FUN_00818570 |
| PassingRangeRight | 100 | DAT_00ba1714 | DAT_00c8ef28 | FUN_00818690 |
| PenaltyTakingSkill | 100 | DAT_00ba1714 | DAT_00c8ef60 | FUN_00818e70 |
| PhysicalPlay | 100 | DAT_00ba1714 | DAT_00c8efb0 | FUN_008199b0 |
| PlayerAbility | 100 | DAT_00ba1714 | DAT_00c8efc0 | FUN_00819bf0 |
| Professionalism | 100 | DAT_00ba1714 | DAT_00c8f1d8 | FUN_0081e6b0 |
| Resilience | 100 | DAT_00ba1714 | DAT_00c8f020 | FUN_0081a970 |
| RightFoot | 100 | DAT_00ba1714 | DAT_00c8f210 | FUN_0081ee90 |
| RightSideAbility | 100 | DAT_00ba1714 | DAT_00c8f278 | FUN_0081fd30 |
| Roaming | 100 | DAT_00ba1714 | DAT_00c8f190 | FUN_0081dc90 |
| RunFrequency | 100 | DAT_00ba1714 | DAT_00c8f198 | FUN_0081ddb0 |
| ShirtPulling | 100 | DAT_00ba1714 | DAT_00c8efb8 | FUN_00819ad0 |
| ShootingBrain | 100 | DAT_00ba1714 | DAT_00c8f180 | FUN_0081da50 |
| ShootingFinesse | 100 | DAT_00ba1714 | DAT_00c8f188 | FUN_0081db70 |
| ShotAccuracy | 100 | DAT_00ba1714 | DAT_00c8f200 | FUN_0081ec50 |
| ShotAgilityLeft | 100 | DAT_00ba1714 | DAT_00c8ef50 | FUN_00818c30 |
| ShotAgilityRight | 100 | DAT_00ba1714 | DAT_00c8ef58 | FUN_00818d50 |
| ShotComposureLeft | 100 | DAT_00ba1714 | DAT_00c8ef70 | FUN_008190b0 |
| ShotComposureRight | 100 | DAT_00ba1714 | DAT_00c8ef78 | FUN_008191d0 |
| ShotRangeLeft | 100 | DAT_00ba1714 | DAT_00c8ef40 | FUN_008189f0 |
| ShotRangeRight | 100 | DAT_00ba1714 | DAT_00c8ef48 | FUN_00818b10 |
| SmotheringAbility | 100 | DAT_00ba1714 | DAT_00c8f0c8 | FUN_0081c070 |
| Spring | 100 | DAT_00ba1714 | DAT_00c8f138 | FUN_0081d030 |
| StrikerAbility | 100 | DAT_00ba1714 | DAT_00c8f258 | FUN_0081f8b0 |
| SweeperAbility | 100 | DAT_00ba1714 | DAT_00c8f228 | FUN_0081f1f0 |
| TackleAvoidance | 100 | DAT_00ba1714 | DAT_00c8f000 | FUN_0081a4f0 |
| TackleCaution | 100 | DAT_00ba1714 | DAT_00c8eff0 | FUN_0081a2b0 |
| TackleChaseRange | 100 | DAT_00ba1714 | DAT_00c8ef80 | FUN_008192f0 |
| TackleFrequency | 100 | DAT_00ba1714 | DAT_00c8efd8 | FUN_00819f50 |
| TacklingAbility | 100 | DAT_00ba1714 | DAT_00c8efe8 | FUN_0081a190 |
| ThrowAccuracy | 100 | DAT_00ba1714 | DAT_00c8f1a8 | FUN_0081dff0 |
| ThrowPower | 100 | DAT_00ba1714 | DAT_00c8f1a0 | FUN_0081ded0 |
| TrickDelivery | 100 | DAT_00ba1714 | DAT_00c8f018 | FUN_0081a850 |
| TrickReading | 100 | DAT_00ba1714 | DAT_00c8f010 | FUN_0081a730 |
| Trickyness | 100 | DAT_00ba1714 | DAT_00c8f008 | FUN_0081a610 |
| Versatility | 100 | DAT_00ba1714 | DAT_00c8f1e8 | FUN_0081e8f0 |
| WingBackAbility | 100 | DAT_00ba1714 | DAT_00c8f238 | FUN_0081f430 |
| WingerAbility | 100 | DAT_00ba1714 | DAT_00c8f240 | FUN_0081f550 |

### 4e. All remaining groups (complete)

| name | group | default | default DAT | handle slot | getter |
|---|---|---|---|---|---|
| PlayerDegradeable | (multi) | 100 | DAT_00ba1714 | DAT_00c8f288 | FUN_0081ff70 |
| BoardInvestAmount | BoardAI | 10000 | DAT_00b61970 | DAT_00c8fc50 | FUN_00833130 |
| BoardRequestExpandStadiumBoardAmbitionThreshold | BoardRequest | - | - | DAT_00c909a8 | FUN_0084c210 |
| BoardRequestExpandStadiumBoardOpinionThreshold | BoardRequest | - | - | DAT_00c90998 | FUN_0084c030 |
| BoardRequestExpandStadiumEEOYBThreshold | BoardRequest | - | - | DAT_00c909b0 | FUN_0084c300 |
| BoardRequestExpandStadiumHigherDivisionThreshold | BoardRequest | 0.9 | DAT_00b5b114 | DAT_00c909c8 | FUN_0084c5d0 |
| BoardRequestExpandStadiumHoneymoonPeriodMonthsThreshold | BoardRequest | - | - | DAT_00c909a0 | FUN_0084c120 |
| BoardRequestExpandStadiumSameDivisionThreshold | BoardRequest | - | - | DAT_00c909c0 | FUN_0084c4e0 |
| BoardRequestExpandStadiumSeasonGateThreshold | BoardRequest | 0.9 | DAT_00b5b114 | DAT_00c909b8 | FUN_0084c3f0 |
| BoardRequestExtraRebuildTimeBoardOpinionThresholdHigh | BoardRequest | - | - | DAT_00c90a08 | FUN_0084cd50 |
| BoardRequestExtraRebuildTimeBoardOpinionThresholdLow | BoardRequest | - | - | DAT_00c90a10 | FUN_0084ce40 |
| BoardRequestExtraRebuildTimeHoneymoonPeriodMonthsThreshold | BoardRequest | - | - | DAT_00c90a18 | FUN_0084cf30 |
| BoardRequestExtraTransferFundsBoardAmbitionThreshold | BoardRequest | - | - | DAT_00c90958 | FUN_0084b8b0 |
| BoardRequestExtraTransferFundsBoardOpinionThreshold | BoardRequest | - | - | DAT_00c90930 | FUN_0084b400 |
| BoardRequestExtraTransferFundsBudgetLeagueAverageThreshold | BoardRequest | 0.3 | DAT_00b5b5ec | DAT_00c90948 | FUN_0084b6d0 |
| BoardRequestExtraTransferFundsBudgetUsedThreshold | BoardRequest | 0.7 | DAT_00b5b5d4 | DAT_00c90940 | FUN_0084b5e0 |
| BoardRequestExtraTransferFundsEEOYBThreshold | BoardRequest | - | - | DAT_00c90938 | FUN_0084b4f0 |
| BoardRequestExtraTransferFundsHoneymoonPeriodMonthsThreshold | BoardRequest | - | - | DAT_00c90960 | FUN_0084b9a0 |
| BoardRequestExtraTransferFundsNumberPlayersThreshold | BoardRequest | 0.3 | DAT_00b5b5ec | DAT_00c90950 | FUN_0084b7c0 |
| BoardRequestExtraWageFundsAtWeeklySalaryCapThreshold | BoardRequest | - | - | DAT_00c90990 | FUN_0084bf40 |
| BoardRequestExtraWageFundsBoardAmbitionThreshold | BoardRequest | - | - | DAT_00c90980 | FUN_0084bd60 |
| BoardRequestExtraWageFundsBoardOpinionThreshold | BoardRequest | - | - | DAT_00c90968 | FUN_0084ba90 |
| BoardRequestExtraWageFundsEEOYBThreshold | BoardRequest | - | - | DAT_00c90970 | FUN_0084bb80 |
| BoardRequestExtraWageFundsHoneymoonPeriodMonthsThreshold | BoardRequest | - | - | DAT_00c90988 | FUN_0084be50 |
| BoardRequestExtraWageFundsNumberPlayersThreshold | BoardRequest | 0.3 | DAT_00b5b5ec | DAT_00c90978 | FUN_0084bc70 |
| BoardRequestImproveFacilitiesBoardAmbitionThreshold | BoardRequest | - | - | DAT_00c909e0 | FUN_0084c8a0 |
| BoardRequestImproveFacilitiesBoardOpinionThreshold | BoardRequest | - | - | DAT_00c909d0 | FUN_0084c6c0 |
| BoardRequestImproveFacilitiesDivisionAverageThreshold | BoardRequest | 0.3 | DAT_00b5b5ec | DAT_00c909f0 | FUN_0084ca80 |
| BoardRequestImproveFacilitiesEEOYBThreshold | BoardRequest | - | - | DAT_00c909e8 | FUN_0084c990 |
| BoardRequestImproveFacilitiesHappyEnoughLevel | BoardRequest | - | - | DAT_00c90a00 | FUN_0084cc60 |
| BoardRequestImproveFacilitiesHoneymoonPeriodMonthsThreshold | BoardRequest | - | - | DAT_00c909d8 | FUN_0084c7b0 |
| BoardRequestImproveFacilitiesUnhappinessThreshold | BoardRequest | 0.3 | DAT_00b5b5ec | DAT_00c909f8 | FUN_0084cb70 |
| BoardUltimatumConditionBoardOpinionOnManagerThreshold | BoardRequest | - | - | DAT_00c90a28 | FUN_0084d110 |
| BoardUltimatumConditionEstimatedEndOfYearBalanceThreshold | BoardRequest | - | - | DAT_00c90a30 | FUN_0084d200 |
| BoardUltimatumConditionHoneymoonPeriodThreshold | BoardRequest | - | - | DAT_00c90a20 | FUN_0084d020 |
| BoardUltimatumFailureBoardOpinionReasonBoardOpinionChange | BoardRequest | - | - | DAT_00c90a48 | FUN_0084d4d0 |
| BoardUltimatumFailureEEOYBReasonBoardOpinionChange | BoardRequest | - | - | DAT_00c90a50 | FUN_0084d5c0 |
| BoardUltimatumFailureHoneymoonPeriodReasonBoardOpinionChange | BoardRequest | - | - | DAT_00c90a40 | FUN_0084d3e0 |
| BoardUltimatumSuccessBoardOpinionChange | BoardRequest | - | - | DAT_00c90a38 | FUN_0084d2f0 |
| EstimatedEndOfYearBalance | Budget | - | - | DAT_00c8f550 | FUN_00825cd0 |
| LoanThresholdBalance | Budget | - | - | DAT_00c8f578 | FUN_00826200 |
| TransferBudgetBalancePart | Budget | 100 | DAT_00ba1714 | DAT_00c8f540 | FUN_00825a90 |
| TransferBudgetIncomePart | Budget | 100 | DAT_00ba1714 | DAT_00c8f548 | FUN_00825bb0 |
| TransferBudgetStartGame | Budget | 100 | DAT_00ba1714 | DAT_00c8f538 | FUN_00825970 |
| WageBudgetMax | Budget | 100 | DAT_00ba1714 | DAT_00c8f530 | FUN_00825850 |
| WageBudgetMin | Budget | 100 | DAT_00ba1714 | DAT_00c8f528 | FUN_00825730 |
| WageBudgetNegativeBalance | Budget | 100 | DAT_00ba1714 | DAT_00c8f520 | FUN_00825610 |
| WageBudgetPositiveBalance | Budget | 100 | DAT_00ba1714 | DAT_00c8f518 | FUN_008254f0 |
| ClubFinanceIsBankrupt | ClubInfo | 1000 | DAT_00b3f894 | DAT_00c8fc48 | FUN_00833010 |
| ClubFinanceIsOkay | ClubInfo | 1000 | DAT_00b3f894 | DAT_00c8fc40 | FUN_00832ef0 |
| ClubFinanceIsRich | ClubInfo | 1000 | DAT_00b3f894 | DAT_00c8fc30 | FUN_00832cb0 |
| ClubFinanceIsSecure | ClubInfo | 1000 | DAT_00b3f894 | DAT_00c8fc38 | FUN_00832dd0 |
| CoachThresholdAdmires | CoachReport | - | - | DAT_00c8f8b8 | FUN_0082c440 |
| CoachThresholdAverage | CoachReport | - | - | DAT_00c8f8c8 | FUN_0082c620 |
| CoachThresholdGood | CoachReport | - | - | DAT_00c8f8c0 | FUN_0082c530 |
| CoachThresholdPoor | CoachReport | - | - | DAT_00c8f8d0 | FUN_0082c710 |
| CoachThresholdSuperlative | CoachReport | - | - | DAT_00c8f8b0 | FUN_0082c350 |
| ContinentalTVRevenue | CompetitionIncome | 100 | DAT_00ba1714 | DAT_00c8f500 | FUN_00825190 |
| CupEarlyRoundTVRevenue | CompetitionIncome | 100 | DAT_00ba1714 | DAT_00c8f508 | FUN_008252b0 |
| CupLateRoundTVRevenue | CompetitionIncome | 100 | DAT_00ba1714 | DAT_00c8f510 | FUN_008253d0 |
| Dividends | FinancialCosts | 100 | DAT_00ba1714 | DAT_00c8f4d8 | FUN_00824cb0 |
| LoanMonthlyInterestPercent | FinancialCosts | 0.458 | DAT_00baca60 | DAT_00c8f4f0 | FUN_00824fb0 |
| TaxPercent | FinancialCosts | - | - | DAT_00c8f4e0 | FUN_00824dd0 |
| ManagerDisciplineBaseMoodiness | ManagerDiscipline | 0.5 | DAT_00bbb51c | DAT_00c90650 | FUN_00845d60 |
| ManagerDisciplineMistakeProbability | ManagerDiscipline | 0.1 | DAT_00b5b758 | DAT_00c90648 | FUN_00845c70 |
| ManagerDisciplineThreshold | ManagerDiscipline | 90 | DAT_00b5b704 | DAT_00c90660 | FUN_00845f40 |
| ManagerDisciplineViolenceThreshold | ManagerDiscipline | 60 | DAT_00b5b1d8 | DAT_00c90658 | FUN_00845e50 |
| ManagerSackingBaseWeightDomesticBoardOpinionOfManager | ManagerSacking | 10 | DAT_00b6016c | DAT_00c90d50 | FUN_008531e0 |
| ManagerSackingBaseWeightDomesticLoyalty | ManagerSacking | 0.9 | DAT_00b5b114 | DAT_00c90d58 | FUN_008532d0 |
| ManagerSackingBaseWeightNationalBoardOpinionOfManager | ManagerSacking | 10 | DAT_00b6016c | DAT_00c90d60 | FUN_008533c0 |
| ManagerSackingBaseWeightNationalLoyalty | ManagerSacking | 0.9 | DAT_00b5b114 | DAT_00c90d68 | FUN_008534b0 |
| ManagerSackingSackabilityNationalMainPhaseThresholdHigh | ManagerSacking | - | - | DAT_00c90db8 | FUN_00853f30 |
| ManagerSackingSackabilityNationalMainPhaseThresholdLow | ManagerSacking | - | - | DAT_00c90dc0 | FUN_00854040 |
| ManagerSackingSackabilityNationalMainToQualifyingPhaseThresholdHigh | ManagerSacking | - | - | DAT_00c90dc8 | FUN_00854150 |
| ManagerSackingSackabilityNationalMainToQualifyingPhaseThresholdLow | ManagerSacking | - | - | DAT_00c90dd0 | FUN_00854260 |
| ManagerSackingSackabilityNationalQualifyingPhaseThresholdHigh | ManagerSacking | - | - | DAT_00c90d98 | FUN_00853af0 |
| ManagerSackingSackabilityNationalQualifyingPhaseThresholdLow | ManagerSacking | - | - | DAT_00c90da0 | FUN_00853c00 |
| ManagerSackingSackabilityNationalQualifyingToMainPhaseThresholdHigh | ManagerSacking | - | - | DAT_00c90da8 | FUN_00853d10 |
| ManagerSackingSackabilityNationalQualifyingToMainPhaseThresholdLow | ManagerSacking | - | - | DAT_00c90db0 | FUN_00853e20 |
| ManagerSackingSackabilityThresholdDomestic | ManagerSacking | - | - | DAT_00c90d90 | FUN_008539e0 |
| ManagerSackingWarningDomesticFormGamesObservedThreshold | ManagerSacking | - | - | DAT_00c90d88 | FUN_008538d0 |
| ManagerSackingWarningDomesticFormScoreThreshold | ManagerSacking | - | - | DAT_00c90d80 | FUN_008537c0 |
| ManagerSackingWarningDomesticThresholdHigh | ManagerSacking | - | - | DAT_00c90d70 | FUN_008535a0 |
| ManagerSackingWarningDomesticThresholdLow | ManagerSacking | - | - | DAT_00c90d78 | FUN_008536b0 |
| SimulatedResolverGoalFrequency | MatchModule | 0.5 | DAT_00bbb51c | DAT_00c8fcf0 | FUN_00834420 |
| MediaReportPerformanceThreshold | MatchReport | 85 | DAT_00b5baf4 | DAT_00c90628 | FUN_008458b0 |
| MediaReportReputationThreshold | MatchReport | 30 | DAT_00b63280 | DAT_00c90620 | FUN_008457c0 |
| MediaReportScoutedPlayerGoodPerformanceThreshold | MatchReport | 80 | DAT_00b5b10c | DAT_00c90638 | FUN_00845a90 |
| MediaReportScoutedPlayerPoorPerformanceThreshold | MatchReport | 30 | DAT_00b63280 | DAT_00c90640 | FUN_00845b80 |
| MediaReportSignificantPerformanceThreshold | MatchReport | 90 | DAT_00b5b704 | DAT_00c90630 | FUN_008459a0 |
| MediaReportSignificantScoreThreshold | MatchReport | 3 | DAT_00b5b760 | DAT_00c90618 | FUN_008456d0 |
| NewPlayerSalaryHighCap | NewPlayerSalary | - | - | DAT_00c910a0 | FUN_00859c00 |
| NewPlayerSalaryLowCap | NewPlayerSalary | - | - | DAT_00c91098 | FUN_00859b10 |
| NewPlayerSalaryWageBudgetPercent | NewPlayerSalary | - | - | DAT_00c910a8 | FUN_00859cf0 |
| CardDistance | OfficialState | 2 | DAT_00b18028 | DAT_00c90428 | FUN_00841cb0 |
| CynicalFoulRating | OfficialState | - | - | DAT_00c90410 | FUN_008419e0 |
| DirtyFoulRating | OfficialState | - | - | DAT_00c90418 | FUN_00841ad0 |
| FastGoalSpeed | OfficialState | 28 | DAT_00bae6b4 | DAT_00c90448 | FUN_00842070 |
| HighGoalHeight | OfficialState | 1.5 | DAT_00bbae9c | DAT_00c90438 | FUN_00841e90 |
| LowGoalHeight | OfficialState | 0.5 | DAT_00bbb51c | DAT_00c90430 | FUN_00841da0 |
| MaximumWallPlayers | OfficialState | - | - | DAT_00c903f8 | FUN_00841710 |
| MidHeightScore | OfficialState | 4 | DAT_00b5b764 | DAT_00c90480 | FUN_00842700 |
| MidPassScore | OfficialState | 4 | DAT_00b5b764 | DAT_00c90470 | FUN_00842520 |
| MidRangeScore | OfficialState | 8 | DAT_00b5b6cc | DAT_00c90460 | FUN_00842340 |
| MidSoloScore | OfficialState | 10 | DAT_00b6016c | DAT_00c90478 | FUN_00842610 |
| MidSpeedScore | OfficialState | 20 | DAT_00b5b600 | DAT_00c90468 | FUN_00842430 |
| MinorFoulRating | OfficialState | - | - | DAT_00c90408 | FUN_008418f0 |
| NudgeFoulRating | OfficialState | - | - | DAT_00c90400 | FUN_00841800 |
| ProffessionalFoulAngle | OfficialState | - | - | DAT_00c90420 | FUN_00841bc0 |
| SlowGoalSpeed | OfficialState | 8 | DAT_00b5b6cc | DAT_00c90440 | FUN_00841f80 |
| WideGoalBottom | OfficialState | 3 | DAT_00b5b760 | DAT_00c90458 | FUN_00842250 |
| WideGoalTop | OfficialState | -3 | DAT_00baf774 | DAT_00c90450 | FUN_00842160 |
| WeeksUntilStaffFluent | PersonAgeing | 50 | DAT_00b5b75c | DAT_00c90718 | FUN_008474d0 |
| WeeksUntilStaffSpoken | PersonAgeing | 20 | DAT_00b5b600 | DAT_00c90720 | FUN_008475f0 |
| EventHomesickDurationInterpFromMax | PersonHappiness | 540 | DAT_00bb086c | DAT_00c908f0 | FUN_0084ac80 |
| EventHomesickDurationInterpFromMin | PersonHappiness | - | - | DAT_00c908e8 | FUN_0084ab90 |
| EventHomesickMaxRange | PersonHappiness | - | - | DAT_00c908a8 | FUN_0084a410 |
| EventHomesickMinRange | PersonHappiness | - | - | DAT_00c908b0 | FUN_0084a500 |
| EventHomesickStartTimeOffset | PersonHappiness | - | - | DAT_00c908b8 | FUN_0084a5f0 |
| EventHomesickThresholdInterpFromMaxAge | PersonHappiness | 40 | DAT_00b5b6b4 | DAT_00c908c8 | FUN_0084a7d0 |
| EventHomesickThresholdInterpFromMinAge | PersonHappiness | 16 | DAT_00b5e8e8 | DAT_00c908c0 | FUN_0084a6e0 |
| EventHomesickThresholdInterpToMaxAdaptability | PersonHappiness | 70 | DAT_00b73990 | DAT_00c908d8 | FUN_0084a9b0 |
| EventHomesickThresholdInterpToMinAdaptability | PersonHappiness | - | - | DAT_00c908d0 | FUN_0084a8c0 |
| EventHomesickThresholdNumberOfSuccessiveDaysInArea | PersonHappiness | - | - | DAT_00c908e0 | FUN_0084aaa0 |
| EventPraiseDuration | PersonHappiness | - | - | DAT_00c90778 | FUN_00848070 |
| EventPraiseMaxOffsetOfAveragePerformance | PersonHappiness | - | - | DAT_00c90780 | FUN_00848160 |
| EventPraiseMaxRange | PersonHappiness | - | - | DAT_00c90760 | FUN_00847da0 |
| EventPraiseMinOffsetOfAveragePerformance | PersonHappiness | - | - | DAT_00c90788 | FUN_00848250 |
| EventPraiseMinRange | PersonHappiness | - | - | DAT_00c90768 | FUN_00847e90 |
| EventPraiseRepeatEffectFactor | PersonHappiness | 0.5 | DAT_00bbb51c | DAT_00c90790 | FUN_00848340 |
| EventPraiseStartTimeOffset | PersonHappiness | - | - | DAT_00c90770 | FUN_00847f80 |
| DailyConditionRecovery | PhysioInjuries | 10 | DAT_00b6016c | DAT_00c8f8f0 | FUN_0082cad0 |
| DailyInjuryConditionReduction | PhysioInjuries | 10 | DAT_00b6016c | DAT_00c8f8f8 | FUN_0082cbc0 |
| GeneralInjuryChance | PhysioInjuries | - | - | DAT_00c8f8d8 | FUN_0082c800 |
| InjuryFrequency | PhysioInjuries | 5 | DAT_00b5b76c | DAT_00c8f8e8 | FUN_0082c9e0 |
| TrainingInjuryChance | PhysioInjuries | - | - | DAT_00c8f8e0 | FUN_0082c8f0 |
| FitnessThresholdNoConcerns | PhysioReport | - | - | DAT_00c8f890 | FUN_0082bf10 |
| FitnessThresholdProne | PhysioReport | - | - | DAT_00c8f8a8 | FUN_0082c240 |
| FitnessThresholdRatherProne | PhysioReport | - | - | DAT_00c8f898 | FUN_0082c020 |
| FitnessThresholdSomewhatProne | PhysioReport | - | - | DAT_00c8f8a0 | FUN_0082c130 |
| InjuryRecurenceThreshold | PhysioReport | - | - | DAT_00c8f888 | FUN_0082be00 |
| Acceleration | PlayerDegradeable | 100 | DAT_00ba1714 | DAT_00c8f290 | FUN_00820090 |
| Deceleration | PlayerDegradeable | 100 | DAT_00ba1714 | DAT_00c8f298 | FUN_008201b0 |
| Reaction | PlayerFluid | 100 | DAT_00ba1714 | DAT_00c8f280 | FUN_0081fe50 |
| AwayFromHomeNation | PlayerHappiness | - | - | DAT_00c8f988 | FUN_0082dca0 |
| AwayFromHomeNationNoLanguage | PlayerHappiness | - | - | DAT_00c8f990 | FUN_0082dd90 |
| AwayFromHomeNationNoManagerLanguage | PlayerHappiness | - | - | DAT_00c8f998 | FUN_0082de80 |
| ClubFormHappy | PlayerHappiness | - | - | DAT_00c8f978 | FUN_0082dac0 |
| ClubFormUnhappy | PlayerHappiness | - | - | DAT_00c8f980 | FUN_0082dbb0 |
| ConfidenceRecoveryRate | PlayerHappiness | - | - | DAT_00c8fa08 | FUN_0082eba0 |
| GoodTrainingCondiitons | PlayerHappiness | - | - | DAT_00c8f9a0 | FUN_0082df70 |
| HappinessAmbitionThreshold | PlayerHappiness | 75 | DAT_00b62960 | DAT_00c8f918 | FUN_0082cf80 |
| HappinessMediaStatementThreshold | PlayerHappiness | - | - | DAT_00c8f9f8 | FUN_0082e9c0 |
| HappinessStatementThreshold | PlayerHappiness | - | - | DAT_00c8f9f0 | FUN_0082e8d0 |
| HappinessStateThreshold | PlayerHappiness | 5 | DAT_00b5b76c | DAT_00c8f908 | FUN_0082cda0 |
| HappinessStateThresholdMultiplier | PlayerHappiness | 1.5 | DAT_00bbae9c | DAT_00c8f910 | FUN_0082ce90 |
| HappinessTrainingThreshold | PlayerHappiness | 3 | DAT_00b5b760 | DAT_00c8f920 | FUN_0082d070 |
| HappinessUpdateFrequency | PlayerHappiness | 5 | DAT_00b5b76c | DAT_00c8f900 | FUN_0082ccb0 |
| MajorConcernThreshold | PlayerHappiness | -10 | DAT_00b95908 | DAT_00c8f928 | FUN_0082d160 |
| MonthsTillHomesick | PlayerHappiness | 3 | DAT_00b5b760 | DAT_00c8f938 | FUN_0082d340 |
| MonthsTillNaturalised | PlayerHappiness | 36 | DAT_00b5bbc4 | DAT_00c8f940 | FUN_0082d430 |
| MoraleRecoveryRate | PlayerHappiness | - | - | DAT_00c8fa00 | FUN_0082eab0 |
| NotSelectedLastFixture | PlayerHappiness | - | - | DAT_00c8f970 | FUN_0082d9d0 |
| PlayedLastFixture | PlayerHappiness | - | - | DAT_00c8f960 | FUN_0082d7f0 |
| PlayedLastFixtureSubbed | PlayerHappiness | - | - | DAT_00c8f968 | FUN_0082d8e0 |
| PlayerCriticism | PlayerHappiness | - | - | DAT_00c8f9b8 | FUN_0082e240 |
| PlayerFine | PlayerHappiness | - | - | DAT_00c8f9e0 | FUN_0082e6f0 |
| PlayerPraise | PlayerHappiness | - | - | DAT_00c8f9b0 | FUN_0082e150 |
| PlayerTransferListed | PlayerHappiness | - | - | DAT_00c8f9d0 | FUN_0082e510 |
| PlayerUndeservedCriticism | PlayerHappiness | - | - | DAT_00c8f9c8 | FUN_0082e420 |
| PlayerUndeservedFine | PlayerHappiness | - | - | DAT_00c8f9e8 | FUN_0082e7e0 |
| PlayerUndeservedPraise | PlayerHappiness | - | - | DAT_00c8f9c0 | FUN_0082e330 |
| PlayerUnemployed | PlayerHappiness | - | - | DAT_00c8f948 | FUN_0082d520 |
| PoorTrainingCondiitons | PlayerHappiness | - | - | DAT_00c8f9a8 | FUN_0082e060 |
| SlightConcernThreshold | PlayerHappiness | -5 | DAT_00b5e8dc | DAT_00c8f930 | FUN_0082d250 |
| StarPlayerAbilityRatio | PlayerHappiness | 1.1 | DAT_00bd6bf8 | DAT_00c8fa10 | FUN_0082ec90 |
| StarPlayerSold | PlayerHappiness | - | - | DAT_00c8f9d8 | FUN_0082e600 |
| SubPlayedLastFixture | PlayerHappiness | - | - | DAT_00c8f958 | FUN_0082d700 |
| SubWatchedLastFixture | PlayerHappiness | - | - | DAT_00c8f950 | FUN_0082d610 |
| WeeksUntilFluent | PlayerHappiness | 50 | DAT_00b5b75c | DAT_00c8fa18 | FUN_0082ed80 |
| WeeksUntilSpoken | PlayerHappiness | 20 | DAT_00b5b600 | DAT_00c8fa20 | FUN_0082eea0 |
| PlayerProfessionalismModifierFine | PlayerProfessionalism | - | - | DAT_00c90668 | FUN_00846030 |
| MinimumTimePlayedForReputationUpdate | PlayerReputationPostMatchProgression | - | - | DAT_00c90fe0 | FUN_00858480 |
| PercentageHomeReputationIncrease | PlayerReputationPostMatchProgression | - | - | DAT_00c90fd0 | FUN_008582a0 |
| PercentageWorldReputationIncrease | PlayerReputationPostMatchProgression | - | - | DAT_00c90fd8 | FUN_00858390 |
| TargetRangeLowerOffset | PlayerReputationPostMatchProgression | - | - | DAT_00c90ff0 | FUN_008586a0 |
| TargetRangeUpperOffset | PlayerReputationPostMatchProgression | - | - | DAT_00c90fe8 | FUN_00858590 |
| BowlFuture | PlayerState | - | - | DAT_00c90538 | FUN_00843c90 |
| CelebrationDistance | PlayerState | 3 | DAT_00b5b760 | DAT_00c904c0 | FUN_00842e80 |
| CornerRunUp | PlayerState | 15 | DAT_00ba1708 | DAT_00c904a8 | FUN_00842bb0 |
| DefaultRisingFreeKick | PlayerState | 2 | DAT_00b18028 | DAT_00c90510 | FUN_008437e0 |
| DummyAngle2 | PlayerState | - | - | DAT_00c90590 | FUN_008446e0 |
| ElbowAngle | PlayerState | - | - | DAT_00c905b8 | FUN_00844b90 |
| FallingBackwards | PlayerState | -2 | DAT_00b60170 | DAT_00c904d8 | FUN_00843150 |
| FallingForwards | PlayerState | 3 | DAT_00b5b760 | DAT_00c904d0 | FUN_00843060 |
| FreeKickRunUp | PlayerState | 4 | DAT_00b5b764 | DAT_00c90490 | FUN_008428e0 |
| GoalKickRunUpScale | PlayerState | 15 | DAT_00ba1708 | DAT_00c904b0 | FUN_00842ca0 |
| HandsKickRunUp | PlayerState | 4 | DAT_00b5b764 | DAT_00c904b8 | FUN_00842d90 |
| HookAngle | PlayerState | - | - | DAT_00c905c0 | FUN_00844c80 |
| KeeperCollision | PlayerState | 2 | DAT_00b18028 | DAT_00c904c8 | FUN_00842f70 |
| KeeperWidthBias | PlayerState | 2 | DAT_00b18028 | DAT_00c90520 | FUN_008439c0 |
| LollyPopAngle2 | PlayerState | - | - | DAT_00c90580 | FUN_00844500 |
| LongRangeFreeKick | PlayerState | 30 | DAT_00b63280 | DAT_00c90550 | FUN_00843f60 |
| MaxDefendDistance | PlayerState | 8 | DAT_00b5b6cc | DAT_00c904f8 | FUN_00843510 |
| MaxExtraComposure | PlayerState | 10 | DAT_00b6016c | DAT_00c904e8 | FUN_00843330 |
| MaxTackleChaseRange | PlayerState | 12 | DAT_00b5b754 | DAT_00c90560 | FUN_00844140 |
| MinComposure | PlayerState | 10 | DAT_00b6016c | DAT_00c904e0 | FUN_00843240 |
| MinDefendDistance | PlayerState | 5 | DAT_00b5b76c | DAT_00c904f0 | FUN_00843420 |
| MinimumKeeperUpdate | PlayerState | 0.1 | DAT_00b5b758 | DAT_00c90518 | FUN_008438d0 |
| MinTackleChaseRange | PlayerState | 6 | DAT_00bd7260 | DAT_00c90558 | FUN_00844050 |
| PenaltyAreaCaution | PlayerState | 0.05 | DAT_00b5b6c4 | DAT_00c90500 | FUN_00843600 |
| PenaltyRunUp | PlayerState | 6 | DAT_00bd7260 | DAT_00c90498 | FUN_008429d0 |
| PhysicalCautionDistance | PlayerState | - | - | DAT_00c90508 | FUN_008436f0 |
| RollFuture | PlayerState | 0.5 | DAT_00bbb51c | DAT_00c90540 | FUN_00843d80 |
| RunAdjusterDegrees | PlayerState | - | - | DAT_00c905a0 | FUN_008448c0 |
| RunOutDistance | PlayerState | 50 | DAT_00b5b75c | DAT_00c90530 | FUN_00843ba0 |
| SetPieceFreeKickRunUp | PlayerState | 8 | DAT_00b5b6cc | DAT_00c904a0 | FUN_00842ac0 |
| ShirtPullAngle | PlayerState | - | - | DAT_00c905b0 | FUN_00844aa0 |
| ShortCornerDistance | PlayerState | - | - | DAT_00c90570 | FUN_00844320 |
| SlideAngle | PlayerState | - | - | DAT_00c905c8 | FUN_00844d70 |
| SlideCautionDistance | PlayerState | 4 | DAT_00b5b764 | DAT_00c90548 | FUN_00843e70 |
| SmotherDistance | PlayerState | 8 | DAT_00b5b6cc | DAT_00c90528 | FUN_00843ab0 |
| SpinAngle2 | PlayerState | - | - | DAT_00c90588 | FUN_008445f0 |
| ThrowRunUp | PlayerState | 6 | DAT_00bd7260 | DAT_00c90488 | FUN_008427f0 |
| TightAngleFreeKick | PlayerState | - | - | DAT_00c90578 | FUN_00844410 |
| TightCollisionAngle | PlayerState | - | - | DAT_00c90598 | FUN_008447d0 |
| TripAngle | PlayerState | - | - | DAT_00c905a8 | FUN_008449b0 |
| ValidPassApproachSpeed | PlayerState | 12 | DAT_00b5b754 | DAT_00c90568 | FUN_00844230 |
| MutateConditionScale | PostMatchMutation | 0.5 | DAT_00bbb51c | DAT_00c90920 | FUN_0084b220 |
| MutateConfidenceScale | PostMatchMutation | 0.4 | DAT_00b5bac0 | DAT_00c90928 | FUN_0084b310 |
| MutateMoraleScale | PostMatchMutation | 0.1 | DAT_00b5b758 | DAT_00c90918 | FUN_0084b130 |
| ProspectiveManagersAffordableSalaryRemainingWageBudgetFactor | ProspectiveManagers | 0.5 | DAT_00bbb51c | DAT_00c90b00 | FUN_0084ea80 |
| ProspectiveManagersBaseWeightDomesticComponentAmbition | ProspectiveManagers | - | - | DAT_00c90bc8 | FUN_008503f0 |
| ProspectiveManagersBaseWeightDomesticComponentClubAreaPersonNationContrast | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90bf0 | FUN_008508a0 |
| ProspectiveManagersBaseWeightDomesticComponentCurrentAbility | ProspectiveManagers | 55 | DAT_00bb1898 | DAT_00c90b90 | FUN_0084fd60 |
| ProspectiveManagersBaseWeightDomesticComponentDetermination | ProspectiveManagers | - | - | DAT_00c90bc0 | FUN_00850300 |
| ProspectiveManagersBaseWeightDomesticComponentHomeReputation | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90ba8 | FUN_00850030 |
| ProspectiveManagersBaseWeightDomesticComponentLanguageCompetency | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90bd8 | FUN_008505d0 |
| ProspectiveManagersBaseWeightDomesticComponentLoyalty | ProspectiveManagers | - | - | DAT_00c90ba0 | FUN_0084ff40 |
| ProspectiveManagersBaseWeightDomesticComponentPotentialAbility | ProspectiveManagers | - | - | DAT_00c90b98 | FUN_0084fe50 |
| ProspectiveManagersBaseWeightDomesticComponentPreviousClub | ProspectiveManagers | 20 | DAT_00b5b600 | DAT_00c90bd0 | FUN_008504e0 |
| ProspectiveManagersBaseWeightDomesticComponentProfessionalism | ProspectiveManagers | - | - | DAT_00c90bb8 | FUN_00850210 |
| ProspectiveManagersBaseWeightDomesticComponentRivalry | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90be0 | FUN_008506c0 |
| ProspectiveManagersBaseWeightDomesticComponentUnemployment | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90be8 | FUN_008507b0 |
| ProspectiveManagersBaseWeightDomesticComponentWorldReputation | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90bb0 | FUN_00850120 |
| ProspectiveManagersBaseWeightNationalComponentAmbition | ProspectiveManagers | - | - | DAT_00c90c30 | FUN_00851020 |
| ProspectiveManagersBaseWeightNationalComponentClubAreaPersonNationContrast | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90c58 | FUN_008514d0 |
| ProspectiveManagersBaseWeightNationalComponentCurrentAbility | ProspectiveManagers | 55 | DAT_00bb1898 | DAT_00c90bf8 | FUN_00850990 |
| ProspectiveManagersBaseWeightNationalComponentDetermination | ProspectiveManagers | - | - | DAT_00c90c28 | FUN_00850f30 |
| ProspectiveManagersBaseWeightNationalComponentHomeReputation | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90c10 | FUN_00850c60 |
| ProspectiveManagersBaseWeightNationalComponentLanguageCompetency | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90c40 | FUN_00851200 |
| ProspectiveManagersBaseWeightNationalComponentLoyalty | ProspectiveManagers | - | - | DAT_00c90c08 | FUN_00850b70 |
| ProspectiveManagersBaseWeightNationalComponentPotentialAbility | ProspectiveManagers | - | - | DAT_00c90c00 | FUN_00850a80 |
| ProspectiveManagersBaseWeightNationalComponentPreviousClub | ProspectiveManagers | 20 | DAT_00b5b600 | DAT_00c90c38 | FUN_00851110 |
| ProspectiveManagersBaseWeightNationalComponentProfessionalism | ProspectiveManagers | - | - | DAT_00c90c20 | FUN_00850e40 |
| ProspectiveManagersBaseWeightNationalComponentRivalry | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90c48 | FUN_008512f0 |
| ProspectiveManagersBaseWeightNationalComponentUnemployment | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90c50 | FUN_008513e0 |
| ProspectiveManagersBaseWeightNationalComponentWorldReputation | ProspectiveManagers | 10 | DAT_00b6016c | DAT_00c90c18 | FUN_00850d50 |
| ProspectiveManagersComponentValueDisfavourManagerAtRivalClubDoesntExist | ProspectiveManagers | - | - | DAT_00c90c98 | FUN_00851c50 |
| ProspectiveManagersComponentValueDisfavourManagerAtRivalClubExists | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90c90 | FUN_00851b60 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpSrcMax | ProspectiveManagers | 80 | DAT_00b5b10c | DAT_00c90cd8 | FUN_008523d0 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpSrcMin | ProspectiveManagers | 20 | DAT_00b5b600 | DAT_00c90cd0 | FUN_008522e0 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpTrgMax | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90ce8 | FUN_008525b0 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastDiffInterpTrgMin | ProspectiveManagers | - | - | DAT_00c90ce0 | FUN_008524c0 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpSrcMax | ProspectiveManagers | 80 | DAT_00b5b10c | DAT_00c90cb8 | FUN_00852010 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpSrcMin | ProspectiveManagers | 20 | DAT_00b5b600 | DAT_00c90cb0 | FUN_00851f20 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpTrgMax | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90cc8 | FUN_008521f0 |
| ProspectiveManagersComponentValueDomesticClubAreaPersonNationContrastSameInterpTrgMin | ProspectiveManagers | - | - | DAT_00c90cc0 | FUN_00852100 |
| ProspectiveManagersComponentValueDomesticLanguageCompetencyFluent | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90c60 | FUN_008515c0 |
| ProspectiveManagersComponentValueDomesticLanguageCompetencyNone | ProspectiveManagers | - | - | DAT_00c90c70 | FUN_008517a0 |
| ProspectiveManagersComponentValueDomesticLanguageCompetencySlight | ProspectiveManagers | 65 | DAT_00b5bb8c | DAT_00c90c68 | FUN_008516b0 |
| ProspectiveManagersComponentValueFavourUnemployedManagerWhenEmployed | ProspectiveManagers | - | - | DAT_00c90ca8 | FUN_00851e30 |
| ProspectiveManagersComponentValueFavourUnemployedManagerWhenUnemployed | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90ca0 | FUN_00851d40 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpSrcMax | ProspectiveManagers | 80 | DAT_00b5b10c | DAT_00c90d18 | FUN_00852b50 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpSrcMin | ProspectiveManagers | 20 | DAT_00b5b600 | DAT_00c90d10 | FUN_00852a60 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpTrgMax | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90d28 | FUN_00852d30 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastDiffInterpTrgMin | ProspectiveManagers | - | - | DAT_00c90d20 | FUN_00852c40 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpSrcMax | ProspectiveManagers | 80 | DAT_00b5b10c | DAT_00c90cf8 | FUN_00852790 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpSrcMin | ProspectiveManagers | 20 | DAT_00b5b600 | DAT_00c90cf0 | FUN_008526a0 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpTrgMax | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90d08 | FUN_00852970 |
| ProspectiveManagersComponentValueNationalClubAreaPersonNationContrastSameInterpTrgMin | ProspectiveManagers | - | - | DAT_00c90d00 | FUN_00852880 |
| ProspectiveManagersComponentValueNationalLanguageCompetencyFluent | ProspectiveManagers | 100 | DAT_00ba1714 | DAT_00c90c78 | FUN_00851890 |
| ProspectiveManagersComponentValueNationalLanguageCompetencyNone | ProspectiveManagers | - | - | DAT_00c90c88 | FUN_00851a70 |
| ProspectiveManagersComponentValueNationalLanguageCompetencySlight | ProspectiveManagers | 65 | DAT_00b5bb8c | DAT_00c90c80 | FUN_00851980 |
| ProspectiveManagersComponentWeightDomesticHomeReputationFactorWhenForeignManager | ProspectiveManagers | - | - | DAT_00c90d38 | FUN_00852f10 |
| ProspectiveManagersComponentWeightDomesticWorldReputationFactorWhenHomeManager | ProspectiveManagers | - | - | DAT_00c90d30 | FUN_00852e20 |
| ProspectiveManagersComponentWeightNationalHomeReputationFactorWhenForeignManager | ProspectiveManagers | - | - | DAT_00c90d48 | FUN_008530f0 |
| ProspectiveManagersComponentWeightNationalWorldReputationFactorWhenHomeManager | ProspectiveManagers | - | - | DAT_00c90d40 | FUN_00853000 |
| ProspectiveManagersContinentalContrastBelievabilityEconomicRatingThreshold | ProspectiveManagers | - | - | DAT_00c90b80 | FUN_0084fb40 |
| ProspectiveManagersContinentalContrastBelievabilityReputationThreshold | ProspectiveManagers | - | - | DAT_00c90b88 | FUN_0084fc50 |
| ProspectiveManagersFixtureImminentPrestigiousnessThreshold | ProspectiveManagers | 95 | DAT_00bb14e4 | DAT_00c90b10 | FUN_0084ec80 |
| ProspectiveManagersManagerSearchingDesperatenessFactorMultiplier | ProspectiveManagers | - | - | DAT_00c90b78 | FUN_0084fa30 |
| ProspectiveManagersManagerUnapproachableAsProspectiveManagerPeriodInMonths | ProspectiveManagers | - | - | DAT_00c90b08 | FUN_0084eb70 |
| ProspectiveManagersNumMonthsRestrictingPreviousManagerReturning | ProspectiveManagers | - | - | DAT_00c90b18 | FUN_0084ed70 |
| ProspectiveManagersObscureManagerlessClubFrequencyMaxDays | ProspectiveManagers | 150 | DAT_00b625a4 | DAT_00c90ae8 | FUN_0084e790 |
| ProspectiveManagersObscureManagerlessClubFrequencyMinDays | ProspectiveManagers | 7 | DAT_00b5bb74 | DAT_00c90af0 | FUN_0084e880 |
| ProspectiveManagersObscureManagerlessClubReputationObservationThreshold | ProspectiveManagers | - | - | DAT_00c90af8 | FUN_0084e970 |
| ProspectiveManagersReputationBandOffsetDomestic | ProspectiveManagers | - | - | DAT_00c90b20 | FUN_0084ee80 |
| ProspectiveManagersReputationBandOffsetNational | ProspectiveManagers | - | - | DAT_00c90b28 | FUN_0084ef90 |
| ProspectiveManagersReputationBandOffsetNationalBoostSameArea | ProspectiveManagers | - | - | DAT_00c90b30 | FUN_0084f0a0 |
| ProspectiveManagersReputationBaseOffsetDomestic | ProspectiveManagers | - | - | DAT_00c90b38 | FUN_0084f1b0 |
| ProspectiveManagersReputationBaseOffsetNational | ProspectiveManagers | - | - | DAT_00c90b40 | FUN_0084f2c0 |
| ProspectiveManagersReputationSafetyBaseOffsetDomesticMax | ProspectiveManagers | - | - | DAT_00c90b50 | FUN_0084f4e0 |
| ProspectiveManagersReputationSafetyBaseOffsetDomesticMin | ProspectiveManagers | - | - | DAT_00c90b48 | FUN_0084f3d0 |
| ProspectiveManagersReputationSafetyBaseOffsetNationalMax | ProspectiveManagers | - | - | DAT_00c90b60 | FUN_0084f700 |
| ProspectiveManagersReputationSafetyBaseOffsetNationalMin | ProspectiveManagers | - | - | DAT_00c90b58 | FUN_0084f5f0 |
| ProspectiveManagersUnemployedAdvantageDomestic | ProspectiveManagers | - | - | DAT_00c90b68 | FUN_0084f810 |
| ProspectiveManagersUnemployedAdvantageNational | ProspectiveManagers | - | - | DAT_00c90b70 | FUN_0084f920 |
| ReputationManagerAnnualAward | Reputation | - | - | DAT_00c906e0 | FUN_00846e40 |
| ReputationManagerMonthlyAward | Reputation | - | - | DAT_00c906c8 | FUN_00846b70 |
| ReputationPlayerAnnualAward | Reputation | - | - | DAT_00c906e8 | FUN_00846f30 |
| ReputationPlayerDecreaseClubTransfer | Reputation | - | - | DAT_00c906b0 | FUN_008468a0 |
| ReputationPlayerIncreaseClubTransfer | Reputation | - | - | DAT_00c906a8 | FUN_008467b0 |
| ReputationPlayerManOfMatch | Reputation | - | - | DAT_00c906c0 | FUN_00846a80 |
| ReputationPlayerMonthlyAward | Reputation | - | - | DAT_00c906d0 | FUN_00846c60 |
| ReputationWorldRatio | Reputation | - | - | DAT_00c906b8 | FUN_00846990 |
| ReputationYouthPlayerAnnualAward | Reputation | - | - | DAT_00c906f0 | FUN_00847020 |
| ReputationYouthPlayerMonthlyAward | Reputation | - | - | DAT_00c906d8 | FUN_00846d50 |
| RollForwardPercentageChance | RollForwardData | - | - | DAT_00c90ed0 | FUN_00856460 |
| RollForwardWeeks | RollForwardData | - | - | DAT_00c90ec8 | FUN_00856350 |
| ActiveRadius | SquadData | 8 | DAT_00b5b6cc | DAT_00c90330 | FUN_0083ffa0 |
| AddedImpetus | SquadData | 0.02 | DAT_00b5e8c4 | DAT_00c903c8 | FUN_00841170 |
| AttackingFreeKick | SquadData | 60 | DAT_00b5b1d8 | DAT_00c90318 | FUN_0083fcd0 |
| AttackingThrow | SquadData | 60 | DAT_00b5b1d8 | DAT_00c90300 | FUN_0083fa00 |
| BowlDistance | SquadData | 65 | DAT_00b5bb8c | DAT_00c902d8 | FUN_0083f550 |
| CloseFreeKickAngle | SquadData | - | - | DAT_00c903e8 | FUN_00841530 |
| CloseFreeKickDistance | SquadData | 6 | DAT_00bd7260 | DAT_00c903a8 | FUN_00840db0 |
| CloseToFreeKickTarget | SquadData | 20 | DAT_00b5b600 | DAT_00c90320 | FUN_0083fdc0 |
| Cluster | SquadData | - | - | DAT_00c903e0 | FUN_00841440 |
| DisruptWallDistance | SquadData | 9.5 | DAT_00baf614 | DAT_00c903b8 | FUN_00840f90 |
| FreeKickBackXScale | SquadData | 0.66 | DAT_00b99dd4 | DAT_00c90398 | FUN_00840bd0 |
| FreeKickBackZScale | SquadData | 0.33 | DAT_00baba44 | DAT_00c90390 | FUN_00840ae0 |
| FreeKickStayForwardZScale | SquadData | 0.28 | DAT_00baf57c | DAT_00c903a0 | FUN_00840cc0 |
| FrustrationScale | SquadData | 0.1 | DAT_00b5b758 | DAT_00c903c0 | FUN_00841080 |
| GoForwardXScale | SquadData | 0.28 | DAT_00baf57c | DAT_00c90380 | FUN_00840900 |
| GoForwardZScale | SquadData | 0.25 | DAT_00b5b798 | DAT_00c90378 | FUN_00840810 |
| InitialDesperation | SquadData | 0.2 | DAT_00b5b388 | DAT_00c902f8 | FUN_0083f910 |
| LosingDesperation | SquadData | 0.75 | DAT_00bd7268 | DAT_00c90338 | FUN_00840090 |
| LosingImpetus | SquadData | 0.025 | DAT_00baf650 | DAT_00c903d0 | FUN_00841260 |
| MaxKickWidth | SquadData | 50 | DAT_00b5b75c | DAT_00c90328 | FUN_0083feb0 |
| MaxPressure | SquadData | 25 | DAT_00b5b284 | DAT_00c90358 | FUN_00840450 |
| MaxThrowInDistance | SquadData | 20 | DAT_00b5b600 | DAT_00c90308 | FUN_0083faf0 |
| MinSetPieceDistance | SquadData | - | - | DAT_00c902f0 | FUN_0083f820 |
| MinThrowDistance | SquadData | 5 | DAT_00b5b76c | DAT_00c902e8 | FUN_0083f730 |
| MinThrowInDistance | SquadData | 20 | DAT_00b5b600 | DAT_00c90310 | FUN_0083fbe0 |
| NoPressureExtraTimeMinutes | SquadData | 75 | DAT_00b62960 | DAT_00c90350 | FUN_00840360 |
| NoPressureTimeMinutes | SquadData | 70 | DAT_00b73990 | DAT_00c90348 | FUN_00840270 |
| OverFreeKickAngle | SquadData | - | - | DAT_00c903f0 | FUN_00841620 |
| OverFreeKickDistance | SquadData | 10 | DAT_00b6016c | DAT_00c903b0 | FUN_00840ea0 |
| PressureScale | SquadData | 0.003 | DAT_00baf540 | DAT_00c90360 | FUN_00840540 |
| RollDistance | SquadData | 20 | DAT_00b5b600 | DAT_00c902e0 | FUN_0083f640 |
| RollTooClose | SquadData | 20 | DAT_00b5b600 | DAT_00c903d8 | FUN_00841350 |
| TackleEasy | SquadData | 0.5 | DAT_00bbb51c | DAT_00c90368 | FUN_00840630 |
| TackleHard | SquadData | 1.5 | DAT_00bbae9c | DAT_00c90370 | FUN_00840720 |
| ThrowInClose | SquadData | 10 | DAT_00b6016c | DAT_00c90388 | FUN_008409f0 |
| WinningMargin | SquadData | - | - | DAT_00c90340 | FUN_00840180 |
| AttackingMidfielderCenterBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f320 | FUN_00821470 |
| AttackingMidfielderLeftBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f318 | FUN_00821350 |
| AttackingMidfielderRightBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f328 | FUN_00821590 |
| CentralDefenderBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2e0 | FUN_00820b70 |
| ClubReputationAbilityFractionReserves | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f360 | FUN_00821d10 |
| ClubReputationAbilityFractionSeniors | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f358 | FUN_00821c20 |
| ClubReputationAbilityFractionYouth | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f368 | FUN_00821e00 |
| DefaultMatchFitnessThreshold | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f350 | FUN_00821b30 |
| DefensiveMidfielderCenterBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2f0 | FUN_00820db0 |
| DefensiveMidfielderLeftBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2e8 | FUN_00820c90 |
| DefensiveMidfielderRightBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2f8 | FUN_00820ed0 |
| ForwardCenterBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f340 | FUN_008218f0 |
| FullBackLeftBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2d0 | FUN_00820930 |
| FullBackRightBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2d8 | FUN_00820a50 |
| GoalKeeperBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2b0 | FUN_008204b0 |
| MidfielderCenterBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f308 | FUN_00821110 |
| MidfielderLeftBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f300 | FUN_00820ff0 |
| MidfielderRightBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f310 | FUN_00821230 |
| NeedsRestThreshold | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2a8 | FUN_008203c0 |
| StrikerBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f348 | FUN_00821a10 |
| SweeperBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2b8 | FUN_008205d0 |
| TiredThreshold | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2a0 | FUN_008202d0 |
| WingBackLeftBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2c0 | FUN_008206f0 |
| WingBackRightBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f2c8 | FUN_00820810 |
| WingerLeftBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f330 | FUN_008216b0 |
| WingerRightBias | TeamSelection | 100 | DAT_00ba1714 | DAT_00c8f338 | FUN_008217d0 |
| TrainingFacilitiesUpdateCostLevel0 | TrainingFacilities | - | - | DAT_00c90dd8 | FUN_00854370 |
| TrainingFacilitiesUpdateCostLevel1 | TrainingFacilities | - | - | DAT_00c90de0 | FUN_00854480 |
| TrainingFacilitiesUpdateCostLevel2 | TrainingFacilities | - | - | DAT_00c90de8 | FUN_00854590 |
| TrainingFacilitiesUpdateCostLevel3 | TrainingFacilities | - | - | DAT_00c90df0 | FUN_008546a0 |
| TrainingFacilitiesUpdateCostLevel4 | TrainingFacilities | - | - | DAT_00c90df8 | FUN_008547b0 |
| TrainingFacilitiesUpdateCostLevel5 | TrainingFacilities | - | - | DAT_00c90e00 | FUN_008548c0 |
| TrainingFacilitiesUpdateCostLevel6 | TrainingFacilities | - | - | DAT_00c90e08 | FUN_008549d0 |
| TrainingFacilitiesUpdateCostLevel7 | TrainingFacilities | - | - | DAT_00c90e10 | FUN_00854ae0 |
| TrainingFacilitiesUpdateCostLevel8 | TrainingFacilities | - | - | DAT_00c90e18 | FUN_00854bf0 |
| TrainingFacilitiesUpdateCostLevel9 | TrainingFacilities | - | - | DAT_00c90e20 | FUN_00854d00 |
| TrainingFacilitiesUpdateRangeLevel0Max | TrainingFacilities | - | - | DAT_00c90e30 | FUN_00854f20 |
| TrainingFacilitiesUpdateRangeLevel0Min | TrainingFacilities | - | - | DAT_00c90e28 | FUN_00854e10 |
| TrainingFacilitiesUpdateRangeLevel1Max | TrainingFacilities | - | - | DAT_00c90e40 | FUN_00855140 |
| TrainingFacilitiesUpdateRangeLevel1Min | TrainingFacilities | - | - | DAT_00c90e38 | FUN_00855030 |
| TrainingFacilitiesUpdateRangeLevel2Max | TrainingFacilities | - | - | DAT_00c90e50 | FUN_00855360 |
| TrainingFacilitiesUpdateRangeLevel2Min | TrainingFacilities | - | - | DAT_00c90e48 | FUN_00855250 |
| TrainingFacilitiesUpdateRangeLevel3Max | TrainingFacilities | - | - | DAT_00c90e60 | FUN_00855580 |
| TrainingFacilitiesUpdateRangeLevel3Min | TrainingFacilities | - | - | DAT_00c90e58 | FUN_00855470 |
| TrainingFacilitiesUpdateRangeLevel4Max | TrainingFacilities | - | - | DAT_00c90e70 | FUN_008557a0 |
| TrainingFacilitiesUpdateRangeLevel4Min | TrainingFacilities | - | - | DAT_00c90e68 | FUN_00855690 |
| TrainingFacilitiesUpdateRangeLevel5Max | TrainingFacilities | - | - | DAT_00c90e80 | FUN_008559c0 |
| TrainingFacilitiesUpdateRangeLevel5Min | TrainingFacilities | - | - | DAT_00c90e78 | FUN_008558b0 |
| TrainingFacilitiesUpdateRangeLevel6Max | TrainingFacilities | - | - | DAT_00c90e90 | FUN_00855be0 |
| TrainingFacilitiesUpdateRangeLevel6Min | TrainingFacilities | - | - | DAT_00c90e88 | FUN_00855ad0 |
| TrainingFacilitiesUpdateRangeLevel7Max | TrainingFacilities | - | - | DAT_00c90ea0 | FUN_00855e00 |
| TrainingFacilitiesUpdateRangeLevel7Min | TrainingFacilities | - | - | DAT_00c90e98 | FUN_00855cf0 |
| TrainingFacilitiesUpdateRangeLevel8Max | TrainingFacilities | - | - | DAT_00c90eb0 | FUN_00856020 |
| TrainingFacilitiesUpdateRangeLevel8Min | TrainingFacilities | - | - | DAT_00c90ea8 | FUN_00855f10 |
| TrainingFacilitiesUpdateRangeLevel9Max | TrainingFacilities | - | - | DAT_00c90ec0 | FUN_00856240 |
| TrainingFacilitiesUpdateRangeLevel9Min | TrainingFacilities | - | - | DAT_00c90eb8 | FUN_00856130 |
| TrainingTruancyFrequency | TrainingModule | 5 | DAT_00b5b76c | DAT_00c8fce0 | FUN_00834240 |
| TrainingTruancyProfessionalismThreshold | TrainingModule | 75 | DAT_00b62960 | DAT_00c8fce8 | FUN_00834330 |
| BigClubNotifyQualityThreshold | TransferListNotify | - | - | DAT_00c91080 | FUN_008597e0 |
| PlayerNotifyAbilityThreshold | TransferListNotify | - | - | DAT_00c91070 | FUN_008595c0 |
| PlayerNotifyPotentialThreshold | TransferListNotify | - | - | DAT_00c91078 | FUN_008596d0 |
| DefenderCountSellThreshold | TransferModule | 6 | DAT_00bd7260 | DAT_00c905f0 | FUN_00845220 |
| DesiredSellPriceToBaseValueTrialThreshold | TransferModule | - | - | DAT_00c90610 | FUN_008455e0 |
| ForwardCountSellThreshold | TransferModule | 4 | DAT_00b5b764 | DAT_00c90600 | FUN_00845400 |
| KeeperCountSellThreshold | TransferModule | 2 | DAT_00b18028 | DAT_00c905e8 | FUN_00845130 |
| MidfielderCountSellThreshold | TransferModule | 8 | DAT_00b5b6cc | DAT_00c905f8 | FUN_00845310 |
| MinimumNegotiatePercentage | TransferModule | 90 | DAT_00b5b704 | DAT_00c905d8 | FUN_00844f50 |
| MinimumTransferListedNegotiatePercentage | TransferModule | 75 | DAT_00b62960 | DAT_00c905e0 | FUN_00845040 |
| NeededPlayerSellPriceMultipler | TransferModule | - | - | DAT_00c90608 | FUN_008454f0 |
| StarPlayerReputationRatio | TransferModule | 1.1 | DAT_00bd6bf8 | DAT_00c905d0 | FUN_00844e60 |
| AssistantWage | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f488 | FUN_00824170 |
| CoachWage | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f490 | FUN_00824290 |
| FirstTeam | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f450 | FUN_00823990 |
| FirstTeamPostRequestSuccess | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4b0 | FUN_00824710 |
| FringePlayer | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f460 | FUN_00823bd0 |
| HotProspect | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f468 | FUN_00823cf0 |
| HotProspectPostRequestSuccess | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4c0 | FUN_00824950 |
| IndispensablePlayer | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f440 | FUN_00823750 |
| KeyPlayer | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f448 | FUN_00823870 |
| KeyPlayerPostRequestSuccess | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4a8 | FUN_008245f0 |
| ManagerWage | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f480 | FUN_00824050 |
| NotNeeded | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f478 | FUN_00823f30 |
| NotNeededPostRequestSuccess | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4d0 | FUN_00824b90 |
| PhysiosWage | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f498 | FUN_008243b0 |
| Rotation | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f458 | FUN_00823ab0 |
| RotationPostRequestSuccess | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4b8 | FUN_00824830 |
| ScoutWage | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4a0 | FUN_008244d0 |
| Youngster | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f470 | FUN_00823e10 |
| YoungsterPostRequestSuccess | WageLimits | 1000 | DAT_00b3f894 | DAT_00c8f4c8 | FUN_00824a70 |

## 5. Cross-reference: formula names <-> resolver_metrics.h / player fields

The `*Performance` formula names (group `PlayerData`) compute the per-player performance components; they align with `CM_MR_*` in `match/resolver_metrics.h` and the `Player*` output fields (field name cluster at 0x00bd6e5c–0x00bd72a8, VERIFIED). Representative mapping:

| formula name (default) | resolver metric | player field |
|---|---|---|
| AssistsPerformance (0.6) | CM_MR_ASSISTS | PlayerAssists |
| CompletedPassesPerformance (0.1) | CM_MR_COMPLETED_PASSES/PASSES_COMPLETED | PlayerCompletedPasses |
| BadPassesPerformance (-0.2) | CM_MR_PASSES_FAILED | - |
| KeyPassesPerformance | CM_MR_PASSES_KEY | PlayerKeyPasses |
| InterceptionsPerformance | CM_MR_INTERCEPTIONS | PlayerInterceptions |
| GroundCoveredPerformance (computed) | CM_MR_GROUND_COVERED | PlayerGroundCovered |
| HeadersPerformance (0.1) | CM_MR_HEADERS | - |
| KeyHeadersPerformance | CM_MR_HEADERS_KEY | PlayerKeyHeaders |
| TacklesWonPerformance | CM_MR_TACKLES_WON | - |
| KeyTacklesPerformance | CM_MR_TACKLES_KEY | PlayerKeyTackles |
| ClearancePerformance (0.1) | CM_MR_CLEARANCES | - |
| KeptPossessionPerformance | CM_MR_KEPT_POSSESSION | PlayerKeptPossession |
| LostPossessionPerformance | CM_MR_LOST_POSSESSION | PlayerLostPossession |
| CrossesTakenPerformance (0.3) | CM_MR_CROSSES_TAKEN | - |
| OffsidePerformance | CM_MR_OFFSIDES | PlayerCaughtOffside |
| RedCardPerformance | CM_MR_NUMBER_RED_CARDS | NumberRedCards/RedCardFlag |
| YellowCardPerformance | CM_MR_NUMBER_YELLOW_CARDS | NumberYellowCards/YellowCardFlag |
| OwnGoalsPerformance | CM_MR_OWN_GOALS_SCORED | OwnGoalsScored |
| PenGoalsPerformance | CM_MR_PEN_GOALS_SCORED | PenGoalsScored |
| (ScaledPerformance 0x00bd7294) | CM_MR_SCALED_PERFORMANCE | PlayerScaledPerformance |
| (Performance 0x00bd7164) | CM_MR_PERFORMANCE | m_uPerformance |

Full per-player output field list (VERIFIED, 0x00bd6e00–0x00bd72d0): PlayerMorale, PlayerKeyActions, PlayerMinorFouls, PlayerCynicalFouls, PlayerCrossesCompleted, PlayerCrosses, PlayerShotsOffTarget, PlayerMissControlled, PlayerLostPossession, PlayerFoulsReceived, PlayerFoulsCommited, PlayerShotsOnGoal, PlayerShotsAttempted, PlayerAssists, PlayerCaughtOffside, PlayerRunsKey, PlayerRuns, PlayerInterceptions, PlayerKeyHeaders, PlayerKeyTackles, PlayerKeyPasses, PlayerCompletedPasses, and the shared fields PenaltiesSaved/Taken, GoalsScored{Attack,Midfield,Defence}, FreeKick{Direct,Indirect}, KeptPossession, ChallengesAvoided/Made, OwnGoalsScored, PenGoalsScored, DirtyFouls, Warnings, KeyActions, Tricks, FailedToControl, PassesReceived, PoorCrosses, CompletedPasses, TacklesLost, SubstitutionCount, NumberRedCards, ScaledPerformance.


## 6. Unresolved — lives only in the encrypted formula.xml (NOT recovered)

The following are **absent from the exe** and are deliberately **not fabricated**:

- **The formula TREES themselves.** For every one of the 810 registered names, the exe holds only the *name*, *group*, *handle slot* and a scalar *default*. The real definition (which inputs, per-input weights, position/duty/tactical modifiers, nested `value`/`formulae` lookups, condition/effect bodies) is parsed at runtime from `#DataXML#/formula.xml.blowfish` (Blowfish-encrypted, ~482 KB). UNKNOWN.

- **The Blowfish key / mode / IV** used by `GCFormulaeXMLHandler`. Not registered here (content-protection circumvention; out of scope). See `research/BLOWFISH_DATA_RECOVERY.md`.

- **Exact binop/agg opcode numbers** (the integer encoding of sum/sub/prod/div/mean and the aggregation selector). The operator *vocabulary* is VERIFIED (Section 2); the numeric opcode mapping is INFERRED from `resolver_eval.c` and unproven here — marked INFERRED/UNKNOWN.

- **Defaults marked `-`** (296 getters, e.g. all of ResolverConstants, GroundCoveredPerformance, the physics ball-speed/kick-height tables): these getters compute via the tree with no scalar literal fallback, so no in-exe constant exists to register. UNKNOWN until formula.xml is provided.

- **84 predicate ranking weights (FUN_009e5160):** already recovered elsewhere — `match/sim/predicate_weights.h`. Not re-registered here.


## Confidence legend

- **VERIFIED:** read directly from the PE image or decompiled closure (all numeric values, all names/groups, operator vocab, identifiers, handle slots).

- **INFERRED:** semantic role of a constant (default-vs-coefficient), opcode numbering.

- **UNKNOWN:** anything in Section 6 (formula trees, weights, key, `-` defaults).

