# EXTERNAL FORMULA/DB RESEARCH — CM2008 data-editor / DataXML OSINT

**Agent:** external-formula-db-research · **Date:** 2026-11-23
**Mission (narrow):** Find historical **data-editor / database tools** for Championship Manager 2008 (and same-engine CM2009/CM2010) and determine whether **any touches the encrypted `DataXML` / `formula.xml.blowfish` layer** — which would imply it embeds or derives the Blowfish key. Distinct from the earlier (cold) key search: this is about the *editor tools* themselves.
**Rule observed:** No downloaded binary or tool was executed. No personal data placed in queries.

---

## BOTTOM LINE (read first)

- **No CM2008 / CM2009 / CM2010 data editor found touches the encrypted DataXML or `formula.xml` layer.** Every editor located operates on the **plaintext pre-game player/club/staff/competition database only**. None demonstrates any knowledge of the formula.xml Blowfish key, the encrypted-DataXML format, or match-engine coefficients.
- **No tool demonstrates DataXML decryption.** Zero evidence anywhere. The encrypted-formula trail remains **cold for external tooling** — consistent with the prior `FORMULA_XML_OSINT_REPORT.md`.
- All GitHub hits for `formula.xml.blowfish`, `champman.xsd`, `GCFormula`, `GCProgramStep` resolve to **the user's own repo** (`azdharsyahputra/im-fb-mng`) or **unrelated non-CM projects** (KULeuven IDP, OpenPapyrus ERP, expired-domain lists). No third party.
- **No actionable lead** was produced by this editor-focused pass. The only strategic takeaway remains the prior one: the key must be captured **at runtime** (the editors cannot help — they never read the encrypted layer).

---

## TOOLS FOUND

| Tool | URL / source | Author · date | What it edits | Touches encrypted DataXML/formula? | Evidence of key/format knowledge? |
|---|---|---|---|---|---|
| **Official CM2008 Editor** ("Editor Read Me v2.0") | scribd.com/document/554261288 | Mark Forrester · CM2008 era | Plain DB: Club Data, Squad, Player Data (9 main buttons); install points at the game data folder via a path text file | **No** — no mention of DataXML/formula.xml/blowfish/encryption; names no encrypted file | **No** |
| **GM / Getmanaging CM08 Editor v2.3** (also v1.6) | Steam app/8060 discussions (file "GM CM08 Editor v 2.3.zip", Aug 2024 repost); links dead as of 2025 | "Getmanaging" · unknown | Plain DB: players, names, free-agent status, "skins"; v1.6 can't rename/free-agent, v2.3 can | **No** — encryption/DataXML/formula never mentioned in-thread | **No** |
| **cmeditors** (SourceForge) | sourceforge.net/projects/cmeditors | nygreen · reg. 2003, last update 2013; GPLv2; Delphi/Kylix | **Championship Manager 4** pre-game DB editor (`CM4Pregame-v0.10-source.zip`, 203 kB). Open-source. | **No** — CM4-era pregame DB; no DataXML/formula/blowfish anywhere on the page | **No** (source available but wrong/earlier data layer) |
| **cm2010-run** (same BGS engine, RE not editor) | github.com/MagnificentS/cm2010-run | MagnificentS · 2026-05 | N/A — reverse-engineering of `cm2010.exe`; parses **plaintext** FrontEndScreens UI XML only | **No** — explicitly never touches the encrypted DataXML layer (see prior report) | **No key**; confirms engine + SecuROM virtualization |
| CM93/CM0102/CM0304 editors (FMRTE, CM0102Patcher, cm0304-save-editor, CM_Shadow_Editor, …) | GitHub (see prior `EXTERNAL_TOOLS_AND_REPOS.md`) | various | Pre-2003 **Sports Interactive** save/DB/memory editing | **No** — different (pre-BGS) codebase; no Blowfish/DataXML/formula | **No** |

### champman0102.net (named in mission)
- `champman0102.net` **301-redirects to `champman0102.co.uk`** (now 403 to automated fetch). It is the **CM 01/02** community (Sports Interactive era). Prior report already confirms it carries **no** CM2008 DataXML/formula tooling. Its modding energy is on 01/02, not the BGS CM2008.

---

## DOES ANY EDITOR TOUCH THE ENCRYPTED FORMULA LAYER?

**No — none.** The consistent picture across every tool located:

- All CM2008 editors (official Forrester editor; GM/Getmanaging editor) edit the **plaintext pre-game database** (players, clubs, staff, competitions, names, free-agent flags, skins). This is the same `data` folder that also contains `DataXML/`, but the editors read/write the **unencrypted DB records**, never the `formula.xml.blowfish` / `MetParams.dat` / `ProgramStep.xml` encrypted payloads.
- No editor's documentation, thread, or source mentions `DataXML`, `formula.xml`, `champman.xsd`, Blowfish, encryption, coefficients, or match-engine formulas.
- The open-source one (`cmeditors`, Delphi, GPLv2) targets **CM4 (2003)** pre-game data — an earlier data layer — and still shows no formula/encryption handling.
- GitHub code search for the actual encrypted-layer identifiers returns **only the user's own repo** or unrelated software. Confirms no external party has published code that reads the encrypted layer.

**Implication:** CM2008's encrypted `formula.xml` was a developer-internal, runtime-only asset. The modding community edited the open database and never needed (or obtained) the formula-layer key. No editor is a shortcut to the key or the formula trees.

---

## PRIORITIZED VERDICT

1. **No lead from editor tooling.** No CM2008/09/10 data editor handles the encrypted DataXML/formula layer; none embeds or derives the Blowfish key. This pass does not advance the key hunt.
2. **Best (unchanged) path = runtime capture.** Per prior `FORMULA_XML_OSINT_REPORT.md`: attach to a live `cm2008.exe` and capture the Blowfish key string / scheduled P-array+S-boxes, or the decrypted `formula.xml` buffer at DataXML load time. The editors confirm there is no static/third-party alternative.
3. **Low-value curiosity only:** `cmeditors` (CM4 pregame, open Delphi source) could, at most, illuminate the *pre-game DB* format lineage — **not** the encrypted formula layer. Not worth downloading for this goal.

---

## QUERY LOG

**WebSearch (standard):**
1. `Championship Manager 2008 data editor DataXML tool` — official editor (Scribd) only; no DataXML tool.
2. `Championship Manager 2008 editor champman0102.net` — 01/02 community only; no CM2008 editor on that site.
3. `CM2008 formula editor GCFormula Blowfish` — generic formula-editor docs only; nothing CM.
4. `Championship Manager 2008 editor download sortitoutsi data update` — Scribd readme + FM-era data; no CM2008 formula tool.
5. `CM2010 editor Championship Manager 2010 data editor download` — surfaced SourceForge **cmeditors** (= CM4); no CM2010 DataXML editor.
6. `Championship Manager 2008 "formula" edit match engine modding tool` — DB editor only; no match-formula tool.
7. `champman0102 forum Championship Manager 2008 editor data files` — surfaced **GM CM08 Editor v2.3** (Steam); DB edit only.
8. `Championship Manager 2010 editor DataXML xml data files decrypt fmscout` — FM2010 editor + old CmEd (save-decode, pre-XML); no CM2010 DataXML decrypt.
9. `"GM CM08 Editor" Getmanaging Championship Manager 2008 editor` — confirms file exists; no feature/encryption detail.

**WebFetch:**
- sourceforge.net/projects/cmeditors → CM4 pregame editor, nygreen, GPLv2 Delphi, last 2013; no DataXML/formula/blowfish.
- steamcommunity.com/app/8060/discussions/0/6324828960571800138 → GM/Getmanaging editor edits players/names/free-agents/skins; **no** encryption/DataXML/formula mention.
- scribd.com/document/554261288 (official editor readme) → edits Club/Squad/Player Data; **no** encrypted-file/formula mention.
- champman0102.net → 301 → champman0102.co.uk → **HTTP 403** (automated fetch blocked). 01/02 community per prior report.

**GitHub (`gh search`):**
- repos `championship manager` / `cm2008 editor` / `cm08 editor` / `getmanaging` / `championship manager 2009` — only CM0102/CM93/CM0304 (SI-era) editors; **no BGS CM2008/09/10 data-editor repo**.
- code `formula.xml.blowfish` — **all hits = user's own repo** (`azdharsyahputra/im-fb-mng`).
- code `champman.xsd` — **all hits = user's own repo**.
- code `GCFormula` — user's repo + unrelated (KULeuven IDP, OpenPapyrus, expired domains, ML scripts). No CM editor.
- code `formula.xml championship` / `DataXML eidos` / `"DataXML" blowfish championship` — **0 results**.

**Not reachable:** champman0102.co.uk (403). **Not executed:** any downloaded editor/binary (GM CM08 editor, cmeditors source, etc.) — none run, per mission rule.
