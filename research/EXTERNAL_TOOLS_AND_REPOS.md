# EXTERNAL TOOLS & REPOS — CM2008 / BGS-engine OSINT

**Agent:** formula-xml-osint · **Date:** 2026-11-23

Scope: external tools, repositories and communities relevant (or adjacent) to decrypting/understanding CM2008's `DataXML` (`formula.xml.blowfish`, `MetParams.dat`, `ProgramStep.xml`) and its match-engine formula layer. Verification fields recorded per item. **No downloaded tool or binary was executed.**

---

## TIER A — Independent, same-engine (HIGH VALUE, but no file decryption)

### `MagnificentS/cm2010-run`
- **URL:** https://github.com/MagnificentS/cm2010-run
- **What:** Multi-mission reverse-engineering of **Championship Manager 2010** (`cm2010.exe`), the direct sibling of CM2008 on the same **Beautiful Game Studios "GC"/"CL" engine**. Python, public, created/pushed 2026-05-23, ~266 MB, 0 stars, no license.
- **Date / author:** 2026-05, GitHub user `MagnificentS`.
- **Technical findings:**
  - Engine classes `GCProgramStep` / `GCProgramStepInt` / `GCProgramStepManager`, `CLObject`/`CLReceiver`/`CLMessageFunction`/`CLDataWrapper`/`CLSingleton<T>`. UI XML namespace `urn:BGStudios:FELayout`.
  - Command-step dispatch: `GCProgramStep::Execute` = vtable slot 18 (`0x007B56D0` / `GCProgramStepInt` `0x007B5690`); step fields `+0x28` target, `+0x2C` message_id (0x40F push / 0x410 pop / 0x40A clock…), `+0x30` payload; dispatch via fixed vtable slot 3 (`[vtable+0xC]`) to a **universal receiver `0x004C2700`** shared by 381 vtables; `ProgramStep.xml` loaded by `GCProgramStepManager::Process` (`FUN_00BA21A0`, insertion-sort).
  - **Binary protection:** dispatch/compare body (`FUN_004C2600`→`0x004C2700`) is **SecuROM-virtualised** (static decompile shows `swi(0xCD)`/`in(0x77)`/`halt_baddata`); the literal compare is unrecoverable statically. Protection is selective (`FUN_00b81900` is clean x86).
  - **Toolchain / methodology:** static = **Ghidra MCP** (port 8192); live = **`x32dbg`** rebase + injected **`LiveProbe` inner DLL** + **`probe-cli`** mem-read over named pipe (p99 ≈ 69 µs), with a module-range read guard. Static VAs confirmed live byte-for-byte.
  - **FE-XML-PARSER:** `parse.py` (stdlib) parses 166 **plaintext** FrontEndScreens XML (`…/FrontEndScreens/XML/Compiled`) into a screen graph — proves some BGS XML is unencrypted, but it **never touches the encrypted DataXML / formula.xml layer**.
- **Independently verifiable:** Yes (public repo; static VAs + live byte captures documented).
- **Specifically CM2008?** No — CM2010 (sibling build). Architecture transfers; **addresses do not**.
- **Yields key/algorithm/format/tool?** No key, no DataXML/Blowfish work. Yields: architecture validation + a runtime key-capture methodology.

---

## TIER B — Wrong game / wrong engine (no DataXML value)

### FMRTE family — *Football Manager* (Sports Interactive) live memory editors
- `marcosvilhena87/FMRTE2008.reverse_engineering`, `marcosvilhena87/FMRTE.2008`, `HyeonJung/FMRTE`, `HyeonJung/FMRTE_backend`, `QuantumFlibble/FMRTE24-Scout-Knowledge-Max`.
- **What:** "FMRTE 2008" = Football Manager 2008 real-time **memory** editor (Cheat-Engine-style). SI engine, in-RAM editing.
- **CM2008?** No. **Key/format?** No — different game, no on-disk DataXML decryption.

### Sports-Interactive-era CM editors (pre-2003 split, different codebase)
- `nckstwrt/CM0102Patcher`, `nckstwrt/CM0102Loader`, `maxevans60/cm0102-function-map` ("map of every function in cm0102.exe"), `maxevans60/cm0102-bosman-edition`, `lifebeyondfife/Cm93`, `leandrolsouza/cm0304-save-editor`, `archibalduk/CM_Shadow_Editor` (CM2001/02, abandoned), `nckstwrt/CM9798TacticsEditor`, `agevak/CM0102SaveGameTacticsEditor`, `CM0102-Rust/Megazord-Toolkit`, `cm0304-modern/cm0304-modern`, `YouKnowHtoP/CM0304_*`, `ben-clarke/championship-manager-93-tools`.
- **What:** save-game / database / tactics editors and function maps for CM93 / CM0102 / CM0304 (Sports Interactive engines, pre-BGS).
- **CM2008?** No — different (pre-split) codebase. **Key/format?** No Blowfish, no DataXML, no formula.xml.

### CM2008 official database editor
- "Editor Read Me v2.0" (Mark Forrester) — https://www.scribd.com/document/554261288/Editor-Read-Me-v2-0
- **What:** club/player DB editor usage; install paths; read-only-attribute handling. **No** encryption or formula-layer detail.

---

## TIER C — Generic Blowfish-in-games prior art (pattern background only; none are CM)

| Item | URL | Pattern relevant to us |
|---|---|---|
| CnCNet "Encrypted mix file reading" | https://forums.cncnet.org/topic/2088-encrypted-mix-file-reading/page/2/ | Blowfish key protected by RSA; game *validates* the key (random key rejected); endianness pitfall noted. |
| DDON extractor — Crypto & Archives | https://ddon-research.github.io/ddon-extractor/docs/architecture/crypto-and-archives | "Blowfish key is compiled into the application" → extract from binary. |
| CatSystem2 V_CODEs | https://github-wiki-see.page/m/trigger-segfault/TriggersTools.CatSystem2/wiki/V_CODEs | KEY_CODE XOR 0xCD, then Blowfish → key-derivation variant to keep in mind. |
| `Tatsh/bfcodec` | https://github.com/Tatsh/bfcodec | BFCodec Blowfish content tools; needs correct key. |
| CodeGuru: Blowfish → MSXML2 DOM | https://www.codeguru.com/dotnet/loading-blowfish-encrypted-data-into-an-msxml2-dom-object/ | Documents the exact pattern "Blowfish-encrypted XML decrypted then loaded into a DOM" — architecturally what CM2008 does with DataXML. |

**Takeaway from Tier C:** the recurring design is "key embedded in or derived inside the binary, used at runtime; random keys rejected." Consistent with the user's finding that the formula.xml key loader lives in a protected region → favor **runtime extraction**.

---

## TIER D — Methodological interest only (different engines)
- FM23 bit-for-bit match-engine replication: https://sortitoutsi.net/content/77224/fm23-looking-for-a-partner-bit-for-bit-c-replication-of-the-native-match-engine (modern SI engine; LCG RNG; geometric/deterministic).
- EA FIFA Manager `script.bin` ↔ XML editor: https://soccergaming.com/forums/threads/script-editor-script-bin.6470075/ (HTTP 403 to automated fetch; EA engine, not CM2008).

---

## COMMUNITIES CHECKED
sortitoutsi.net, fmscout.com, soccergaming.com, championshipmanager.fandom.com, Steam/SteamDB (app 8060), MobyGames, Wikipedia. **None** carry CM2008 DataXML/formula.xml decryption, the key, or the formula trees. Active CM modding energy is concentrated on the **pre-2003 SI titles** (CM0102/CM0304), not the BGS-era CM2008.
