# Behavioral Compatibility Report

Compares the reconstructed implementation against Championship Manager 2008.

## Status: no comparisons yet

There is no implemented code to compare, and no reference capture from the
original. This document exists so that every future claim of "matches the
original" is backed by a concrete, repeatable experiment rather than assertion.

## Required before any row can be filled

1. A reference oracle for the original behavior — one of:
   - decompiled algorithm (needs disassembler, see `RECONSTRUCTION_ROADMAP.md`), or
   - captured input→output vectors from a legitimate retail install on Windows.
2. A deterministic test harness in `tests/` with fixed seeds.

## Planned comparison axes (from the mission)

| Axis | Oracle source | Status |
|---|---|---|
| Match outcomes (fixed seed) | match-engine decompilation | ⬜ blocked |
| Player positioning / events | match-engine decompilation | ⬜ blocked |
| Player statistics | `GCPlayer*` decompilation | ⬜ blocked |
| Tactical behavior | `GCTactics*` decompilation | ⬜ blocked |
| Transfer calculations | `GCPlayerTransfer*` / `GCContract*` | ⬜ blocked |
| Financial calculations | `GCFinance*` | ⬜ blocked |
| Save/load round-trip | save format analysis | ⬜ blocked |

No axis can report "compatible" until its oracle exists. Nothing here will be
marked passing on inference alone.
