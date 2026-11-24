# CM2008 Reconstruction — Remaining Work

What is **done**, and what is **not yet done** and why. Honest status: VERIFIED (proven against the
owner's real bytes/files), INFERRED (strong, evidence-backed), UNKNOWN (not recovered). Nothing is
fabricated — where logic/data isn't recovered it's left as an explicit gap, not invented.

Last updated from the running sessions; regenerate structural parts with `cmd/dbscan` / `cmd/mgmtdeps`.

---

## ✅ Done (foundation — solid)

- **Crypto/format**: formula.xml Blowfish cracked (key `deliverance`); DATa `.dat` envelope; MatchResolver grid.
- **MatchResolver**: context grid recovered (draw + position + booster/suppressor axes); position bins mapped; VERIFIED.
- **Formula layer**: 1189 formulas parse, 0 dangling refs; management subsystems evaluated from real bodies.
- **Management (ungated)**: Budget, WageLimits, CompetitionIncome, BoardInvest (input-driven) + ManagerSacking, ContractInterest, NewPlayerSalary, ContractualDesires, PlayerReputation, TransferModule, FinancialCosts, ClubInfo, BoardExpectation (incl. ManagerSackability). All from real formula.xml.
- **Database structure**: 88 DATa tables scanned (id/f64/date fields, 370 FK candidates) → `research/DATABASE_SCHEMA_MAP.md`. Club/Contract byte-aligned decoders corrected + tested.
- **Engine**: resolver sampler, league/season, squad strength from real attributes, LCG, contest, picker interface.

---

## ⏳ A. Gated — waiting on a CAPTURE from you (tooling is READY)

| # | Item | Blocker | Unblock |
|---|---|---|---|
| A1 | 77 PlayerProperties columns → attribute **names** | column→name map not in static data | run `cmd/attrmap` on a `capture.json` (player_id + named attrs from your game) — see `research/ATTRIBUTE_CAPTURE_HOWTO.md` |
| A2 | Picker internal reduction + **literal axis names** | logic is in the SecuROM-protected picker, runtime-only | `cm2008-native trace ingest` on a read-only debugger capture from your lawful install — see `research/RUNTIME_CAPTURE_HOWTO.md` |
| A3 | Attribute-driven formula groups: PlayerConstant (113), PlayerDegradeable, PlayerFluid, PlayerHappiness, TeamSelection (56) | need A1 (player attribute inputs) | build after A1 lands |

## 🔨 B. In progress — Lab session (`local_4973aa86`, match-engine/geometry)

| # | Item | Status |
|---|---|---|
| B1 | Detailed match engine real execution (per-actor contest c710 wired; pass/shot/tackle/GK math) | partial |
| B2 | Movement integrator writer (`FUN_009f0090` seeder VERIFIED; integrator hunt) | Milestone-2 Agent C running |
| B3 | Event **cadence/timing** (threaded sim) | UNKNOWN statically; 1 labeled calibration param until a runtime capture |

## 🗂️ C. Database — structure done, NAMES not

| # | Item | Blocker | Unblock |
|---|---|---|---|
| C1 | Field **names** for the 88 tables (offsets/types/FKs mapped; names unknown except Club's 111) | no byte-aligned on-disk loader / field registrar in the dumps | decompile the loader, or cross-ref A1-style |
| C2 | FK **disambiguation** (small-id tables overlap → multiple candidates) | id-range overlap | tighten by value-range / cardinality heuristics |

## 🧠 D. Behavioural systems — NOT built (need recovered logic, not just constants)

| # | Item | Blocker |
|---|---|---|
| D1 | Native **decision logic** combining constants+inputs: sack decision (beyond score), transfer-valuation combination, contract-offer generation, squad selection | bodies native, un-dumped/corrupt |
| D2 | **Multi-season loop**: promotion/relegation, cups, transfers actually executing, finances evolving season-to-season, training/development/injury as running systems | not built (formulas/constants exist; driving loop doesn't) — **buildable now from `league`+`mgmt` without fabricating** |
| D3 | **AI manager** (lineup / tactics / transfer choices) | thin / UNKNOWN |

## 🧱 E. True wall

| # | Item | Why |
|---|---|---|
| E1 | Picker internal reduction math | SecuROM runtime-decrypted; static = junk (verified). Only via A2 runtime capture. |

---

## Suggested next (no fabrication required)

1. **D2 multi-season loop** — compose the existing `league` (fixtures/table) + `mgmt` (budget/finance/board) into a season-over-season sim. Real inputs, real formulas; the only approximations are clearly-labeled scheduling choices.
2. **C2 FK disambiguation** — cheap refinement of the DB map.
3. Everything else waits on **your captures (A1/A2)** or the **lab (B)**.
