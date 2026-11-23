# Championship Manager 2008 — Behavioral Reconstruction

Clean-room reconstruction effort targeting **Championship Manager 2008**
(Beautiful Game Studios / Eidos, 2007; Windows x86). The original retail
executable is treated as the behavioral specification.

## What this repository is — and is not

- **Clean-room reconstruction.** No original source code was recovered. No
  original source is present here. Nothing in this repo is copied from the
  game; recovered artifacts are limited to **symbol names, strings, and
  metadata** statically extracted from the shipped executable.
- **Not** a crack, patch, loader, or DRM bypass. The copy-protected and the
  pirated "crack" executables are analyzed statically only and are never run.
- **Not** a redistribution of game assets or data. No art, audio, text
  database, or player data from the original is included.

## Current honest status (2026-11-23)

| Dimension | State |
|---|---|
| Target identity | **Verified** — CM2008, retail exe `CM2008.exe`, SHA-256 `5b14a40b…37e81` |
| Architecture (module/screen names) | **Recovered by name** — 168 `GC*` modules, 84 `FE*` screens (MSVC RTTI retained in the retail exe) |
| Tech stack | **Observed** — Direct3D 9 (`d3d9.dll`, `d3dx9_33.dll`), DirectInput8, DirectSound, Winsock |
| Function/algorithm behavior | **NOT recovered** — requires a disassembler (none installed); see `UNKNOWN_BEHAVIOR.md` |
| Implemented code | **None** — directory scaffold only; no module implemented yet |

> The hard blocker is tooling: without a disassembler (Ghidra/IDA/Hopper), the
> actual algorithms inside each module cannot be read, so no behavior can be
> faithfully reconstructed yet. See `docs/EVIDENCE.md` for exactly what is and
> is not known.

## Layout

Source directories (`engine/`, `match/`, `players/`, …) are scaffolding. Each
maps to one or more recovered modules documented in `FUNCTION_MAPPING.md`. They
are intentionally empty of implementation until the corresponding behavior is
reverse-engineered — per project rule, no placeholder is marked "done".

## Tracking docs

- `IMPLEMENTATION_STATUS.md` — per-subsystem status matrix
- `RECONSTRUCTION_ROADMAP.md` — ordered plan and the tooling gate
- `FUNCTION_MAPPING.md` — recovered symbols → target modules
- `COMPATIBILITY_REPORT.md` — behavioral comparison (empty until there is code to compare)
- `UNKNOWN_BEHAVIOR.md` — the open questions blocked on disassembly
- `docs/EVIDENCE.md` — raw recovered evidence and how it was obtained
