# Tooling Setup

## Disassembler: Ghidra 12.1.4 (already present on this machine)

A fully-built Ghidra 12.1.4 already exists from an earlier setup (2026-11-20):

- Install dir: `/Users/csadeveloper/tools/ghidra_12.1.4_PUBLIC`
- **mac_arm_64 native decompiler + sleigh already compiled** (Mach-O arm64),
  built via `gradle buildNatives` (`~/tools/buildnatives.log`: BUILD SUCCESSFUL).
  The official Ghidra release ships only Linux/Windows natives, so macOS requires
  this local build — it was already done.
- JDK 21: `/Users/csadeveloper/tools/jdk-21.0.13.1+2/Contents/Home` (Temurin 21.0.13.1).

> Note: a fresh download/compile attempted earlier in this session was redundant
> and its files under `~/tools/ghidra/`, `~/tools/dl/` were purged by the system.
> The pre-existing install above is the one in use.

## REA wiring (verified)

`~/.claude.json` → `mcpServers.rea.env`:

```json
"env": {
  "GHIDRA_INSTALL_DIR": "/Users/csadeveloper/tools/ghidra_12.1.4_PUBLIC",
  "JAVA_HOME": "/Users/csadeveloper/tools/jdk-21.0.13.1+2/Contents/Home"
}
```

`rea-agents doctor` with these values reports the **ghidra provider
available: true** — all checks pass (configuration, installation, version
12.1.4, headless, native_decompiler, java 21). Backup: `~/.claude.json.bak-*`.

## Activation (the remaining step)

The REA MCP server reads `env` only when it spawns. Resuming a session reuses the
existing server process, so the config change is not yet live in this session.
**Fully quit and reopen the Claude desktop app** (a session resume is not enough)
to respawn REA with the new environment. After that, REA's `auto` provider
selects `ghidra` and `open_binary` / decompilation work.

## Other artifacts found in ~/tools (context, not used by this task)

- `~/tools/ghidra_proj/cm0304.rep` — a prior Ghidra project named `cm0304`
  (a different Championship Manager title). Our task analyzes the retail
  `CM2008.exe`; this project is noted but not assumed relevant.
- `~/tools/d_*.log`, `~/tools/dump_*.log`, `~/tools/ghidra_scripts/` — prior
  headless-analysis logs/scripts.
