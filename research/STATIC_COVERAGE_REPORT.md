# Static coverage expansion — progress report (metrics, not doc count)

One integration pass over 4 parallel static-RE workstreams (committed dumps only; no exe run,
no DRM bypass). Measured by work done, per the directive.

## Metrics

| metric | result |
|---|---|
| Functions analyzed (match-engine reachable set) | 657 reached from `FUN_0077ee80`; 373 with bodies; **172 match-relevant** labelled by subsystem |
| Accessor/schema functions identified (DB) | 7 (Club schema `FUN_00a5d0e0`, Club runtime `FUN_00a2f330`, Player/Staff schema `FUN_00a84df0`, player 7-bit exporters `FUN_00a30d00/a27ca0/a28620`, type interner `FUN_00405530`) |
| Attribute getters recovered | **105/105** (names+addr+class): 101 PlayerConstant, 2 Degradeable, 1 Fluid, 1 meta |
| Attribute semantic names recovered | **72** stored-attribute names, **67 double-confirmed** (enum dump ∧ formula.xml leaf) |
| Getter kind split | 9 STORED · 94 DERIVED · 2 compute-only (VERIFIED from formula.xml bodies) |
| DB fields mapped VERIFIED | Club runtime bit-fields ~36 (incl. 4 overlapping-bit words); Club schema names 111; Club on-disk structural 8; Contract on-disk 9; Player on-disk structural 10 |
| Accessors verified (clean-room, tested) | `src/database/{club,player,contract}_fields.c` — `tests/test_database_fields.c` all pass vs real files |
| Subsystems surveyed | 11 (training, dev/ageing, injury, fatigue, morale, form, squad-selection, manager-AI, tactics, transfers, contracts) — 73 address-cited rows |
| Tests passing | DB field test (synthetic round-trip + real-file invariants) ✅; full existing suites still green |

## Headline correction (VERIFIED)

On-disk `.dat` records are **byte-aligned, not bit-packed**. The Ghidra bit accessors describe the
**runtime in-memory** object, a different layout. Proven against real files (Club double −1.0 @ +22
in 100% of 6571 records; Contract date triplets; Player ascending ids). This invalidates the prior
"records are bit-packed" note and the Go `club`/`contract` decoders (money is IEEE double, not u32;
contract offsets wrong) — flagged for correction.

## Ready-to-implement clean-room (WS5 queue, from formula.xml plaintext bodies)

Fatigue, Form, Morale (value+mutation scales), ManagerSackability, wage caps/budgets,
contract-length (120mo), TeamSelection ratings, training-facility curves, injury-chance/recovery,
reputation progression, and every data-model schema. Delivered this pass: the **byte-aligned DB
accessor layer** (database clean-room component). The behavioural formulas are gated on the
attribute column→name mapping (below) for their player-attribute inputs.

## Bounded blockers (what each needs; none is "impossible" except the picker)

1. **Attribute column index → name** (0/77 VERIFIED): the per-attribute leaf accessors
   (`FUN_0081xxxx`) have no decompiled body in the committed dumps, there is no 77-column
   registrar, and `CM2008.exe` is not on disk → REA can't open the PE. Unblock: re-export those
   leaf-accessor bodies from the existing Ghidra project, or point REA at the project DB.
2. **On-disk DB field names**: structure VERIFIED, but no byte-aligned on-disk *loader* in the
   dumps to name-bind the fields. Unblock: decompile that loader.
3. **AI tactic/formation choice, transfer-valuation getter functions, some driver bodies**:
   un-dumped or corrupt decompiles → thin/UNKNOWN.
4. **Picker cluster** `0x5b37xx–0x5b52xx`: genuine protector barrier — runtime-only (user's lawful
   install capture), kept isolated per WS6.

## Next actions (priority order)

1. Unblock #1/#2 by re-exporting the missing accessor/loader bodies from the Ghidra project (the
   functions are listed in functions.csv; only their bodies are absent from the committed dumps).
2. Correct the Go `club`/`contract` decoders to the verified byte-aligned layout.
3. Clean-room implement the ready formula.xml subsystems once attribute inputs are column-bound.

Detail: `BITPACKED_FIELD_MAP.md`, `PLAYER_ATTRIBUTE_MAPPING.md`/`.csv`,
`DETAILED_MATCH_ENGINE_FUNCTION_MAP.md`, `DETAILED_MATCH_ENGINE_CALLGRAPH.md`,
`SUBSYSTEM_RECOVERY_MATRIX.md`.
