# FORMULA.XML OSINT REPORT — Championship Manager 2008

**Agent:** formula-xml-osint
**Date:** 2026-11-23
**Mission:** Search the public internet for prior art, keys, tools, format specs, or reverse-engineering of CM2008 (Eidos/BGS, 2007) and especially its encrypted `DataXML/formula.xml.blowfish`.

---

## BOTTOM LINE (read first)

- **No working `formula.xml` key was found anywhere on the public internet.** No candidate key was recovered, so nothing was run through the verified decryptor.
- **The trail for CM2008 `DataXML`/`formula.xml` specifically is COLD.** There is *zero* third-party prior art. Every GitHub hit for `formula.xml.blowfish`, `champman.xsd`, `MetParams.dat`, `ProgramStep.xml`, and the two recovered keys resolves to **the user's own repository** (`azdharsyahputra/im-fb-mng`, pushed from this project). No external party has published CM2008 DataXML decryption, the key, or the formula expression trees.
- **The recovered keys are unique to this project.** `f8956ju8y9g54tiou` and `Q1ofsktoPkjS89jdnJeqIO` each return only 2 GitHub hits, both in the user's own repo — confirming they were not borrowed from, or leaked to, any public source.
- **One genuinely valuable independent lead exists:** `MagnificentS/cm2010-run` — a reverse-engineering project for **Championship Manager 2010**, the direct sibling title on the **same Beautiful Game Studios "GC" engine**. It does **not** decrypt DataXML/formula.xml (no Blowfish work at all), but it independently (a) confirms the engine architecture the user reconstructed, (b) confirms the binary is **SecuROM-virtualised** — explaining why the formula.xml key loader sits in a protected region, and (c) demonstrates a **live-process probe methodology** that is the clearest path to capturing the key at runtime.

---

## PRIORITIZED DISCOVERIES

### Priority 1 — Directly decrypts formula.xml
**NONE.** No tool, key, or writeup anywhere decrypts CM2008's `formula.xml.blowfish` (or any BGS-era DataXML). The only matches on earth are the user's own files.

### Priority 2 — Reconstructs the formula trees (alternative source for expression trees)
**NONE for CM2008.** No external dump, parse, or transcription of CM2008's formula expression trees exists. `MagnificentS/cm2010-run` parses CM2010's *FrontEndScreens* UI XML (plaintext, namespace `urn:BGStudios:FELayout`) but never touches the encrypted DataXML layer, so it provides **no** formula expression content.

### Priority 3 — Understands the engine (HIGH VALUE — independent corroboration)
**`MagnificentS/cm2010-run`** (GitHub, created 2026-05-23, Python, ~266 MB, public, 0 stars, no license). A multi-mission static+dynamic RE of `cm2010.exe`. Key corroborating facts for CM2008 work:

- **Same engine class family.** `GCProgramStep`, `GCProgramStepInt`, `GCProgramStepManager`, `CLObject`, `CLReceiver`, `CLMessageFunction`, `CLDataWrapper`, `CLSingleton<T>` — the exact "GC"/"CL" command-step + message-dispatch architecture. UI XML namespace is literally `urn:BGStudios:FELayout` (Beautiful Game Studios), confirming the shared authorship across CM2008/CM2010.
- **Command-step dispatch confirmed (independently of this project).** `GCProgramStep::Execute` is vtable slot 18; a step carries `+0x28` target ptr, `+0x2C` message_id (e.g. 0x40F=1039 push, 0x410=1040 pop, 0x40A clock), `+0x30` int payload. Dispatch is a fixed vtable slot (`[target->vtable+0xC]`, slot 3) landing on a **universal receiver** `0x004C2700` shared by 381 vtables; one frame deeper a `CLMessageFunction`-keyed per-class message map is consulted. `ProgramStep.xml` is loaded + insertion-sorted by `GCProgramStepManager::Process` (`FUN_00BA21A0`).
- **Binary is protected (critical).** The dispatch/compare body (`FUN_004C2600` → `0x004C2700`) is **SecuROM-virtualised** — static decompile shows `swi(0xCD)`/`in(0x77)`/`halt_baddata`; the literal message-id compare **cannot be recovered statically** and had to be confirmed at runtime. This is the exact phenomenon behind the user's finding that the `formula.xml` key loader "is in a protected region." (Note: not every function is virtualised — `FUN_00b81900` reads as clean x86; protection is selective.)
- **Runtime methodology (the actionable part).** They attach to the running `cm2010.exe` with a `LiveProbe` inner DLL + `probe-cli` (mem-read over a named pipe, sub-ms round-trip), rebase via `x32dbg`, and confirm static VAs live (`GCProgramStepManager` slot-3 bytes `00 27 4c 00` = `0x004C2700`, etc.). A module-range guard rejects out-of-range reads. Static tool is Ghidra MCP on port 8192.

**Relevance / limits:** CM2010 ≠ CM2008 — **addresses are per-build and NOT portable**, and if the formula.xml key is derived (non-literal) it may differ between the two titles. What transfers is the architecture validation and the strategy: because the key-loading/decrypt path is likely inside (or reached from) a virtualised region, the realistic route to the `formula.xml` key is **dynamic** — attach to a running `cm2008.exe`, and capture either the Blowfish round-keys/P-array+S-boxes after key schedule, or the plaintext `formula.xml` buffer right after the engine decrypts it, at DataXML load time.

### Priority 4 — Historical / no decryption value
- **FMRTE / FMRTE2008** (`marcosvilhena87/FMRTE2008.reverse_engineering`, `marcosvilhena87/FMRTE.2008`, `HyeonJung/FMRTE`, etc.): "FMRTE 2008" = **Football Manager 2008** (Sports Interactive), a **live memory** real-time editor — a different game and a different (in-RAM) approach, not file decryption. No DataXML relevance.
- **SI-era CM editors** (`nckstwrt/CM0102Patcher`, `maxevans60/cm0102-function-map`, `maxevans60/cm0102-bosman-edition`, `leandrolsouza/cm0304-save-editor`, `archibalduk/CM_Shadow_Editor`, `CM0102-Rust/Megazord-Toolkit`, `cm0304-modern/cm0304-modern`, `ben-clarke/championship-manager-93-tools`): all target **pre-2003 Sports Interactive** CM engines (CM93 / CM0102 / CM0304) — a *different codebase* from the post-split BGS CM2008. Save/DB/memory editing only; no Blowfish, no DataXML, no formula.xml.
- **CM2008 official editor** ("Editor Read Me v2.0" by Mark Forrester, hosted on Scribd): a club/player database editor; documents install paths and read-only-attribute handling, **not** the match-engine formula layer or any encryption.
- **Generic Blowfish-in-games prior art** (not CM): CnCNet C&C mix files (Blowfish key itself protected by RSA; game validates the key, so a random key fails); DDON ("Blowfish key is compiled into the application"); CatSystem2 V_CODEs (KEY_CODE XOR 0xCD then Blowfish); `Tatsh/bfcodec`. Useful only as *pattern background* — a common design is "key embedded in / derived inside the binary, extracted at runtime," consistent with the user's protected-loader finding. Endianness reminder also recurs (the user already uses Blowfish ECB **big-endian**, verified vs. test vectors).
- **FM23 match-engine replication** (`sortitoutsi.net/content/77224`): a bit-for-bit C replication effort on the modern **SI** FM engine (LCG RNG, geometric/deterministic). Different engine lineage; methodological interest only.
- **SoccerGaming `script.bin` editor thread** (Cloudflare-403 to automated fetch): EA **FIFA Manager** `script.bin` ↔ XML tooling — a different publisher/engine; not CM2008.

---

## CROSS-REFERENCE AGAINST ESTABLISHED PROJECT FACTS

| Project fact | External corroboration found? |
|---|---|
| Cipher = Blowfish ECB big-endian | No external CM2008 source; generic game-Blowfish prior art is consistent (endianness is a known gotcha). |
| MetParams.dat key `f8956ju8y9g54tiou` | Unique to user's repo (2 hits, both user's). Not public elsewhere. |
| ProgramStep.xml key `Q1ofsktoPkjS89jdnJeqIO` | Unique to user's repo (2 hits, both user's). Not public elsewhere. |
| formula.xml key UNKNOWN, loader in protected region | **Corroborated indirectly:** cm2010-run proves the sibling engine's dispatch/compare code is SecuROM-virtualised and unrecoverable statically. No one has the key. |
| GCProgramStep command-tree, 810 formulas / 39 groups / 9 ops, clean-room evaluator | **Architecture independently validated** by cm2010-run (GCProgramStep/Manager, message-id dispatch, ProgramStep.xml load path). No external *formula content* exists to cross-check the 810 names / coefficients. |

**Does any external discovery resolve the formula.xml key or provide an alternative source for the formula trees?**
**No.** Nothing external yields the key, and nothing external contains CM2008 formula expression trees. The best external contribution is strategic: confirmation that the key must be obtained **dynamically** from a live process (the sibling-engine project proves static recovery is blocked by SecuROM virtualization), plus independent validation that the reconstructed command-step architecture is correct.

---

## RECOMMENDED NEXT STEP (derived from the one useful lead)
Pursue the key **at runtime**, not statically: attach to a running `cm2008.exe` (debugger or injected probe) and, at DataXML load time, capture either (a) the Blowfish key string / scheduled P-array+S-boxes passed to the key-setup routine, or (b) the decrypted `formula.xml` buffer immediately after decryption. This mirrors `cm2010-run`'s LiveProbe + probe-cli + Ghidra-MCP workflow and sidesteps the virtualised/protected static loader. Addresses from cm2010 are **not** reusable; re-anchor on the CM2008 build.

---

## LOG OF QUERIES RUN & SOURCES CHECKED

**Web searches (WebSearch, standard mode):**
1. `Championship Manager 2008 formula.xml blowfish decrypt reverse engineering` — no CM2008 format hits.
2. `CM2008 DataXML encrypted blowfish key modding tools` — no CM2008 hits.
3. `Championship Manager 2008 database editor formula match engine modding` — editor docs only.
4. `github Championship Manager 2008 formula.xml blowfish decrypt DataXML` — no hits.
5. `"Championship Manager 2008" OR "CM2008" github extractor decrypt data files` — unrelated games only.
6. `GCFormula GCProgramStep "#DataXML#" Eidos game engine blowfish` — no hits (generic Blowfish docs only).
7. `Beautiful Game Studios Championship Manager match engine formula xml schema reverse engineer` — FM/CM93 only.
8. `Championship Manager 2008 sortitoutsi OR fmscout editor data files formula modding` — editor docs only.
9. `"formula.xml" OR "ProgramStep.xml" OR "MetParams.dat" game blowfish key decrypt` — other games only.
10. `Championship Manager 2008 champman.xsd schema formula database structure` — no schema hits.
11. `Championship Manager 2008 no-cd crack data file format unpack archive tool` — no format hits.
12. `"Championship Manager 2008" match engine ratings decompile disassembly research` — reviews only.
13. `FMRTE2008 reverse engineering football manager editor memory` — FM08 memory editor, not CM2008.

**GitHub repository search (API):** `championship manager`(292), `championship manager editor`(5), `fmrte`(13), `cm0304 editor`(3), `eidos championship manager`(0), plus `cm2008 decrypt`/`formula.xml blowfish`/`DataXML blowfish eidos`(all 0). No CM2008 (BGS-era) data-format repo exists.

**GitHub code search (authenticated via `gh`):** `formula.xml.blowfish`(15 → all user's repo), `champman.xsd`(4 → all user's), `MetParams.dat`(4 → 3 user's + 1 unrelated), `ProgramStep.xml blowfish`(3 → all user's), `DataXML blowfish`(846 → user's repo + wordlist noise), `cm2008 formula`(251 → user's repo + noise), `GCProgramStep`(27 → **cm2010-run** + noise), `GCFormula`(209 → all unrelated: KULeuven IDP, OpenPapyrus, expired-domains), `f8956ju8y9g54tiou`(2 → user's), `Q1ofsktoPkjS89jdnJeqIO`(2 → user's), repo-scoped `blowfish`/`DataXML` in cm2010-run (**0**), `formula`/`decrypt`/`GCProgramStep` in cm2010-run (present, runtime context only).

**Fetched / read:** cm2010-run `RE-001/dispatcher.md`, `RE-000/address_anchor.md`, `RE-DYNAMIC-LIVE/summary.md`, `FE-XML-PARSER/parse.py`, `FE-XML-PARSER/gate_decision.md`. (SoccerGaming script.bin thread returned HTTP 403 to automated fetch; GitHub web code-search UI requires login — used the `gh` API instead.)

**Not reachable / not pursued:** SoccerGaming forum (Cloudflare 403); no-CD/crack binaries (not needed and out of scope — not downloaded/executed). No downloaded binary or third-party tool was executed at any point.
