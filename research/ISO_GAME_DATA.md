# Game data on the ISO installer — extraction + decoded systems

The text/data the exe-only passes flagged as "external/UNKNOWN" ships inside the InstallShield
installer on the game ISO (`/Volumes/CM2008/Setup/data1.cab` + `data2/3.cab`, 2511 files). Extract per
file-group with `unshield` (installed):
  `unshield -g <Group> -d <out> x /Volumes/CM2008/Setup/data1.cab`
Groups include TextEnglish, TextUnlocalised, DataXML, DataMisc, DatabaseFiles, FrontendXMLCompiled.

## Text (.str) system — VERIFIED (see MATCH_COMMENTARY.md for commentary)
Every localized string table is a `.str` (TextEnglish/…): `"STR1"` header + records
`[u32 keyHash][u32 len][utf8]`, keyHash = the CRC32 the exe computes (FUN_00405530). Tables:
Commentary, News, stories, Media, Categories, Club, City, Injury, TeamTactic, Frontend, … The
unlocalised `.gid` files are the name→hash directories. Name tokens inside templates use the
`#<slot><Tn><Fn>#` grammar (commentary doc).

## Mail / News template system — VERIFIED (DataXML\Mails, 709 XML templates)
Plain XML, e.g. `BoardRequestAcceptedTransferFunds.xml`:
```
<Mail Category="Mail_Board" Name="..." Priority="Compulsory">
  <Headline><Line StringGroupID="3044392946">…</Line></Headline>
  <Body><Paragraph><Line Optional="False" StringGroupID="1315763766">
    <Parameters>
      <Parameter Index="1" Name="mostseniorboardmemberjobtitle" Type="Job"/>
      <Parameter Index="2" Name="mostseniorboardmembername" Type="BoardMember" Flags="CommonName"/>
      <Parameter Index="3" Name="newtransferbudget" Type="Money" Flags="3SigFig-Short"/>
      <Parameter Index="4" Name="userclubname" Type="Club"/>
    </Parameters>
  </Line></Paragraph></Body>
</Mail>
```
So a mail = Headline/Body/Paragraph/Line, each `Line` referencing a `StringGroupID` (a #Text# GID key
into News.str/stories.str) and a typed, flagged `<Parameter>` list (Type ∈ Job/BoardMember/Money/Club/…,
Flags like `3SigFig-Short`/`CommonName` control formatting). This is the same #Text# mechanism as
commentary, with structured parameters instead of inline `#…#` tokens. 709 templates cover board/
transfer/contract/injury/press/scout events. The exe's GCMailTemplateXMLHandler parses these.

## .tac formations — CORRECTION (VERIFIED)
The 25 `DatabaseFiles/Tactics/*.tac` are GCTactics serializations (`TMP1`/`DAT0`, hash-keyed records
`[u32 fieldHash][u32 …][value]`), 2037 B each. But **all 25 are byte-identical except ONE 4-byte dword
at offset ~0x525** (a NameID hash): 442 vs 343, 442 vs 4231, 442 vs 532_Sweeper each differ in exactly
those 4 bytes. So the shipped `.tac` do NOT encode distinct 11-slot grids — they are the same default
GCTactics template keyed by a formation NameID. The formation SHAPE is therefore name-keyed (the digit
name), which is exactly what `tactics.ZoneCounts` derives — it does not need per-.tac grid decoding.
(The in-game grid per formation is applied from the FieldPosition cells by NameID at runtime; that
runtime assignment is still UNKNOWN, but it is NOT stored distinctly in these files.)

## Encrypted data — noted
`DataXML/formula.xml.blowfish` (482 KB) and `DataXML/ProgramStep.xml.blowfish` are **Blowfish-encrypted**
(the match formula-evaluator config + program steps). Decrypting needs the key from the exe
(future item); the formula evaluator opcodes themselves are already recovered (FORMULA_EVALUATOR_*.md).
