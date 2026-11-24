# CM2008 runtime compatibility matrix + Phase-1 findings

Produced by `cm2008-runtime` (headless, read-only). No game data committed; the retail exe is
read from the owner's own local extract. The ISO is never modified; cracked binaries are never
run.

## ISO inventory (DIRECTLY_OBSERVED)

- ISO: `/Users/csadeveloper/mygames/Championship Manager 2008.iso` — 297,062,400 bytes, ISO9660,
  volume `CM2008`. 89 entries.
- **Retail executable:** `Setup/Files/CM2008.exe` — 13,580,176 bytes,
  SHA-256 `5b14a40b796c6d1834af9d794a6dde0581c4c3ce7cd39fed09146c4f2bd37e81`.
- **Crack (NEVER run, excluded):** `Crack JPN/cm2008.exe`.
- Installer: InstallShield (`Setup/setup.exe`, `Setup/data1.cab/.hdr`, `engine32.cab`),
  `Autorun.exe`, `DirectX/` redist, `Setup/Files/{dbghelp.dll, NonAdminLauncher.exe, MediaCenter}`.

## Retail PE (DIRECTLY_OBSERVED)

- Machine: **x86 (32-bit i386)**; PE32; subsystem GUI; 9 sections.
- Sections: `.text .rdata .data .rsrc` + **`ars est artem celare .securom`**.
- **Protection: SecuROM 7.x (DERIVED).** The obfuscated section names spell SecuROM's motto
  *"ars est artem celare"*. The protected picker `FUN_005b5100` (`0x5b37xx–0x5b52xx`) is
  runtime-decrypted inside these sections, so the on-disk bytes are not the executed
  instructions — consistent with the static dead-end in `research/RESOLVER_INDEX_SELECTION.md`.

## Host

- Apple Silicon (ARM64), macOS 15.7. No Wine/CrossOver/QEMU/UTM/Frida installed (docker only —
  irrelevant for a 32-bit Windows GUI app).

## Compatibility matrix (DERIVED)

| environment | 32-bit x86 Win | SecuROM 7.x disc-check | anti-debug tolerated | protected-code instrumentation | verdict |
|---|---|---|---|---|---|
| Wine / CrossOver on Apple Silicon | partial (wow64 on Rosetta, experimental) | fails (mounted ISO ≠ optical drive; no SecuROM driver) | no | no (anti-debug kills it) | **cannot run the retail exe** |
| Windows-on-ARM VM + x86-32 emulation | yes (x86-32 emulated) | usually fails (VM + no original optical media) | no (SecuROM detects VM/debugger) | no | **blocked by SecuROM** |
| QEMU user/system (x86) | yes (CPU) | n/a / fails | no | partial CPU trace but SecuROM aborts | **blocked by SecuROM** |
| User's own lawfully-activated Windows x86 host | yes | passes (licensed install) | read-only debugger OK on one's own software | **yes (read-only)** | **the sanctioned path** |

## Conclusion (honest)

Running and instrumenting the SecuROM-protected retail picker **on this Apple-Silicon host is not
reachable within the project's hard rules** (no DRM bypass, no anti-debug circumvention, no
cracked binary, no ISO modification). This is a protection boundary, not a tooling gap — even
with Wine/VM installed, SecuROM 7.x's disc check + anti-debug would refuse, and reading the
hidden picker code is precisely what the protection prevents.

**The real runtime evidence is obtainable lawfully by the user**, on their own activated install
where SecuROM permits execution, with a read-only debugger, per
`research/RUNTIME_CAPTURE_HOWTO.md`. The resulting JSONL is solved by
`cm2008-lab trace ingest` (correlator validated end-to-end on synthetic traces). `cm2008-runtime`
prepares and routes that flow; it never fakes an observation and never defeats the protection.

## What this already contributes (previously not recorded)

- Confirmed the retail exe's identity/hash and **SecuROM 7.x** protection with the exact section
  layout — pinning *why* the picker region is static-opaque (runtime-decrypted protector sections).
- A reproducible headless front-end (`cm2008-runtime`) for ISO/PE/DRM inspection, environment
  capability assessment, and routing captures into `cm2008-lab`.
