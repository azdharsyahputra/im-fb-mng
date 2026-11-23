# Reconstruction Roadmap

## The gate (must clear before any behavioral RE)

Reconstructing *behavior* means reading the algorithms inside the 168 `GC*`
modules. That requires a disassembler/decompiler. None is currently installed.
REA probed for `ghidra`, `hopper`, `ida`, `kuna` — **all absent**.

**Unblock options (pick one):**
1. **Ghidra** (free, Apache-2.0). Install an extracted Ghidra 12.1.x release and
   set `GHIDRA_INSTALL_DIR`. REA then drives it headless for decompilation.
2. **IDA Pro** + the upstream IDA MCP (`REA_IDA_MCP_CONFIG`).
3. **Hopper** at `/Applications/Hopper Disassembler.app`.

Until one is configured, work is limited to symbol/metadata archaeology (done)
and clean-room design — but design must not invent algorithms and claim fidelity.

## Phase plan

- **Phase 0 — Surface analysis.** ✅ Done. ISO inventory, PE/resource parse,
  SecuROM ID, 1610 RTTI symbols, tech stack, Game Explorer metadata. See `docs/EVIDENCE.md`.
- **Phase 1 — Enable disassembly.** 🔒 Install a provider (above). Then rebuild
  `binary_overview`, function list, and call graph of `CM2008.exe`.
- **Phase 2 — Data formats first.** Analyze the InstallShield `data2.cab` /
  `data3.cab` player database and the save-game format. Economics and match math
  are meaningless without the data schema they operate on.
- **Phase 3 — Core math.** Decompile `GCFormulaModule`, player attribute/ageing,
  finance, and competition-coefficient calculations. These are deterministic and
  the most testable against the original.
- **Phase 4 — Match engine.** `GCMatchModule` + `GCMatchInputModule` +
  `GCMatchEventModule`. Hardest; reconstruct the event model and tactics inputs.
- **Phase 5 — Simulation loop & scheduling.** Calendar, fixtures, transfers,
  board processing — the day-advance cycle that ties modules together.
- **Phase 6 — UI & game flow.** Reimplement the `FE*Screen` navigation graph on
  an original renderer (no D3D9/original-asset dependency).
- **Phase 7 — Persistence, audio, localization, polish.**

Each phase: RESEARCH → REVERSE ENGINEER → IMPLEMENT → TEST → VALIDATE → DOCUMENT,
with verified vs inferred behavior kept separate in `COMPATIBILITY_REPORT.md`.

## Validation strategy

Behavioral comparison against the original requires *running* the original with
controlled inputs. The only runnable copy here is the forbidden crack, and it is
Windows-only. Real validation therefore needs either a licensed retail install
on Windows or disassembly-derived reference vectors. Documented, not hand-waved.
