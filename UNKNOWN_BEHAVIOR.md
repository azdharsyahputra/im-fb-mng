# Unknown Behavior

Open questions that cannot be answered with the current tooling (static strings +
PE/resource parsing, no disassembler, original never executed). Ordered by how
much the reconstruction depends on them.

## Blocked on disassembly

1. **`GCFormulaModule` contents.** Almost certainly the central calculation
   engine (ratings, probabilities, derived attributes). Entirely opaque.
2. **Match engine model.** How `GCMatchModule` turns tactics + squad + ratings
   into events (`GCMatchEventModule`) and a result. Tick rate, RNG source and
   seeding, event taxonomy — all unknown.
3. **Player attribute model.** Attribute list, ranges, how `GCAgeingModule` and
   development feedback mutate them over time.
4. **Transfer & contract valuation.** The `GCPlayerTransferPaymentModule` /
   `GCContract*` formulas for fees, wages, clauses, auction dynamics.
5. **Finance model.** Income/expenditure categories and the board/finance
   feedback loop (`GCBoardProcessModule`, `GCClubIncome/ExpenditureModule`).
6. **Competition structure.** How coefficients, groups, and round positions drive
   qualification, seeding, and prize money.
7. **Class layouts.** Fields, sizes, and relationships behind every `GC*` name.

## Blocked on data-format analysis (separate from disassembly)

8. **Player/world database schema.** Ships inside InstallShield `data2.cab` /
   `data3.cab`; not yet extracted or parsed. This is the actual game content.
9. **Save-game format.** Structure, versioning, compression unknown.

## Known-but-out-of-scope

10. **SecuROM specifics.** Identified (7.x). Intentionally **not** reverse-
    engineered or reproduced — DRM bypass is out of scope by project rule.

## Resolution path

See `RECONSTRUCTION_ROADMAP.md` Phase 1–2. Items 1–7 unblock when a disassembler
is attached; items 8–9 unblock by extracting and parsing the InstallShield
cabinets (feasible without a disassembler, and a good next step either way).
