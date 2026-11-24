# RECONSTRUCTION_ROADMAP.md — CM2008 native ARM64 (Phase 0 output)

Supersedes the pre-audit roadmap. Component model (per the master directive):
**A = native runtime (Go `super-manager-engine`)**, **B = lab (`runtime-lab`/`runtime-research`
+ Go `cmd/xmodel|axisid`)**, **C = knowledge base (`research/`, `docs/ghidra_dumps`)**.

## Subsystem dependency graph

```
            data model (WS1) ──────────────┐
                 │                          │
        formula (WS2)   predicate (WS3)     │
                 │           │              │
                 └────┬──────┘              │
                      ▼                     ▼
            quick resolver (WS4)     detailed match engine (WS5)
                      │                     │
                      └─────────┬───────────┘
                                ▼
                   other systems (WS6: training/injury/transfer/
                   contracts/AI/competition/finance/save-load)
                                ▼
                      full integration (Phase 6)
```

Data model underpins everything; formula+predicate feed both resolvers; resolvers feed the match
layer; match + data feed the management systems.

## Priorities & sequence (evidence-weighted)

1. **Consolidate on Go (A).** Stop adding duplicate subsystems in C. New runtime code lands in
   `super-manager-engine`; C `match/` is the *reference* to port, not a parallel runtime.
2. **WS1 Data Foundation (CRITICAL, evidence-rich NOW).** Wire the VERIFIED byte-aligned schema
   into the Go runtime; `data validate` + record-inspection CLI + round-trip/integrity tests.
   Blocked sub-item (column→name map) stays UNKNOWN, not fabricated.
3. **WS2 Formula + WS3 Predicate (CRITICAL/HIGH).** Already in Go; add runtime-semantics
   validation vs the decrypted datasets + the 84 weights; differential tests in B.
4. **WS4 Quick Resolver (CRITICAL).** Grid done; keep `NativePickerBackend` abstraction (Go
   `picker` + candidates); the true reduction stays HYPOTHESIS until the user's capture → Solver B.
5. **WS5 Detailed Match Engine (CRITICAL).** Port the C tick lifecycle
   (ccb0→cd40→e850→cf20→c660, GCMatchModule::Update, GameOver) into a Go `match` package with
   deterministic ticks + replay + instrumentation. SetGameState writer stays UNKNOWN until found.
6. **WS6 Other systems.** Deepen finance/transfer/contract/scouting/league/training/injury;
   save-load last.
7. **Phase 6 integration + divergence tooling** (first-divergence compare across two runs).

## Validation requirements (every workstream)
- Parser/round-trip tests on VERIFIED envelopes; no test that only asserts "doesn't crash".
- Formula/predicate: evaluate against the decrypted datasets; record divergences.
- Resolver: cross-model invariants (already locked) + candidate ranking (Solver C) + Solver B
  readiness.
- Match: deterministic replay (same seed+inputs → identical tick trace); event-ordering tests.
- Every reconstructed item carries an evidence label (see RECONSTRUCTION_COVERAGE.md).

## Hard boundaries (unchanged)
Native runtime never depends on Ghidra/PE/the retail exe. No DRM bypass, no anti-debug defeat,
no cracked binary, no ISO modification. Picker reduction + literal axis fields are recoverable
only via the user's lawful runtime capture (`research/RUNTIME_CAPTURE_HOWTO.md` → Solver B).
