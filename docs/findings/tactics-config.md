# Finding: Tactical tunables are data-driven with exe-side defaults

**Status:** reverse-engineered (one getter 1:1) · **Confidence:** Observed
**Evidence:** clean decompile `docs/ghidra_dumps/raw/tactics.c` — FUN_0083ea10.

## Mechanism (Observed)

Tactical/match tunables are **named fields read from a data source** (`"PlayerData"`)
at first use and cached; if a field is absent the exe falls back to a **hardcoded
default**. FUN_0083ea10 is the getter for `MarkingDistanceScale`:

1. lazy-init: resolve field `"MarkingDistanceScale"` in `"PlayerData"`, cache the handle;
2. if the handle is 0 (field not found) → `return 0.02f`;
3. else read and return the field's float value.

This both **confirms the data-driven design** and recovers a **real default
constant: `MarkingDistanceScale = 0.02`**. Many sibling getters exist
(`ClosingDown`, `TeamClosingDown`, `MarkingLine`, `MarkingOffset`, …) following
the same pattern; their defaults are read off their own getters as each is
reconstructed.

## Reconstructed 1:1

`tactics/config.c : cm_marking_distance_scale()` reproduces the observable
behaviour (data value if present, else 0.02), tested both ways in
`tests/test_reconstruction.c`. The `cm_data_source` interface stands in for the
"PlayerData" lookup so the getter is testable without the game data cabs.

## TeamTactics schema (FUN_00a76870) — Observed

The team-level tactics record is registered field-by-field (uint32, precision 7,
default `0xFFFFFFFF`). Recovered field set and order (verbatim):

`Passing, Tackling, ClosingDown, Mentality, CornersLeft, CornersRight,
FreeKicksLeft, FreeKicksRight, UserTactics, ThrowInLeft, ThrowInRight,
OffsideTrap, OffsideTrapDepth, CounterAttack`

Reconstructed in `tactics/team_tactics.h/.c` (struct + name table + init to
UNSET), tested. Value ranges are INFERRED (precision 7).

## Note

The exe-side default (0.02) is the fallback; the *actual* in-game value comes from
the shipped data (data cabs). Faithful balancing still needs those parsed — but
the exe defaults are genuine recovered constants and worth capturing.
