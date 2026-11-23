# CM2008 vs CM2010 — Loader / Encryption Comparison

**Question:** Does the sibling title CM2010 (same Beautiful Game Studios "GC" engine,
repo `MagnificentS/cm2010-run` @ `/Users/csadeveloper/cm/cm2010-run-inspect`) RE any
loader / key-generation logic that reveals how CM2008 derives its `formula.xml` Blowfish
key?

**Method:** Read-only. Grep/read the CM2010 RE repo + the CM2008 findings
(`BLOWFISH_DATA_RECOVERY.md`, `FORMULA_EVALUATOR_FUNCTION_MAP.md`,
`FORMULA_XML_GRAMMAR.md`, `FORMULA_XML_RUNTIME_PLAN.md`,
`EXTERNAL_ENCRYPTION_FINDINGS.md`) + the CM2008 loader logs
(`scratchpad/bfkey.log`, `scratchpad/formula.log`). No binary executed; no Ghidra run.

---

## HEADLINE (honest yes/no)

**NO.** CM2010 does **not** yield any usable key-derivation lead for CM2008's
`formula.xml`. The CM2010 repo contains **zero** DataXML / Blowfish / formula / key-setup
work — it reverse-engineers the FrontEnd UI (GCProgramStep message dispatch) of a running
`cm2010.exe`. It names **no** key, **no** key source, and **no** key-derivation for any
data layer in either title. No shared-key claim is made (there is no CM2010 key to test,
and GC-engine addresses/constants are per-build in any case).

The one genuinely transferable contribution is **strategic, not a key**: CM2010
independently proves that the GC engine *selectively* SecuROM-virtualizes its central
dispatch/compare code — the same class of protection that puts CM2008's `formula.xml`
key-setup out of static reach — which corroborates (does not unblock) the "recover it
dynamically" conclusion already in `FORMULA_XML_RUNTIME_PLAN.md`.

---

## Side-by-side loader / encryption table

| Aspect | CM2008 (this project) | CM2010 (`cm2010-run-inspect`) |
|---|---|---|
| What was RE'd | Gameplay DataXML loaders + formula evaluator (static, from binary bytes/decomp dumps) | FrontEnd UI runtime: message dispatch, GCProgramStep execution, vtable identity (live process, LiveProbe DLL) |
| Formula subsystem | `GCFormulaModule` / `GCFormulaeXMLHandler`; evaluator `FUN_00818120` + 812 accessor stubs; resolver `GCFormulaModule::Find`=`FUN_00662cf0` | **ABSENT.** No `GCFormula*` class anywhere. "Formula" in the repo = "rebase formula"/"formulate" only — not the gameplay formula engine |
| ProgramStep loader class | `GCProgramStep*` loader `FUN_00861970` (calls Blowfish `FUN_0080bde0`); handles `#DataXML#/...` | `GCProgramStepManager::Process` = `FUN_00BA21A0`: installs `GCProgramStepXMLHandler` vtables (`0x012C4B40`/`0x012C4B24`), load+insertion-sort of ProgramStep.xml. **No decrypt step documented** |
| GCProgramStep executor | (per-step exec not the focus; loader + evaluator are) | `GCProgramStep::Execute` `0x007B56D0`, `GCProgramStepInt::Execute` `0x007B5690` (vtable slot 18); fully mapped live (RE-001/RE-002/LP-006.1) |
| `ProgramStep.xml` at rest | **Blowfish-encrypted** (`ProgramStep.xml.blowfish`, 44800 B) — key `Q1ofsktoPkjS89jdnJeqIO` recovered (4 reversed fragments) | Treated as **plaintext**: `remake/program_loader.py` parses `data/source/ProgramStep.xml` directly with `xml.etree.ElementTree`. No decryption in the pipeline |
| `formula.xml` at rest | **Blowfish-encrypted** (`formula.xml.blowfish`, 482112 B) — **key UNKNOWN** | Not present / not handled at all |
| FrontEnd UI XML | (not this project's layer) | **Plaintext** — `FrontEndScreens/XML/Compiled/*.xml`, ns `urn:BGStudios:FELayout`, `FELayout.xsd`; parsed by `FE-XML-PARSER/parse.py` |
| Cipher | Blowfish, ECB, big-endian — VERIFIED (test vectors + 2 files decrypted) | None referenced. `grep -l blowfish` over the whole repo = **0 files** |
| Key storage (recovered keys) | ASCII fragments adjacent to filename, concatenated (ProgramStep: reversed; MetParams: single literal) | **No key logic of any kind** — repo never touches a key setup or a Blowfish schedule |
| Key-setup routine | Shared `FUN_0080bde0` / string helper `FUN_00404c90` — **SecuROM-corrupt on disk** (overlapping insns, `bad instruction data`); `formula.xml` filename xref at `0x00664793` sits in a virtualized region | **Not RE'd.** CM2010's own data-layer encryption (if any) is never investigated |
| Protection evidence | formula key-setup unreadable statically (SecuROM-virtualized region) | Independently shows GC-engine central dispatch `FUN_004C2700`/compare code is SecuROM-virtualized (`swi(0xCD)`, `in(0x77)`, `halt_baddata`); had to confirm live (RE-002) |
| Recovery posture | Static exhausted → runtime capture of key or decrypted buffer (owner-run) | Runtime/live-process method proven for the UI layer (LiveProbe inline hooks) — a *method* analogue, not applied to any crypto |

---

## What CM2010 DOES provide

1. **Confirmation of the encrypted-vs-plaintext layering of the GC engine.** CM2010's
   UI layer (`FrontEndScreens/XML/Compiled`) ships as plaintext XML; encryption, where it
   exists, is applied to the gameplay `DataXML` layer, not the UI-layout layer. This
   matches CM2008's split and is a useful boundary — but it is corroboration, not a key.
2. **Independent proof of selective SecuROM virtualization in the GC engine.** The real
   dispatch/compare logic (`FUN_004C2700`, message-id table) decompiles to junk on disk
   and was only resolved against a **live** process; other functions (e.g.
   `FUN_00B81900`) decompile clean. Same protection class as CM2008's formula key-setup.
3. **A proven live-process methodology to copy** (not addresses): attach a read-only
   probe to the running, legitimately-owned binary and observe what the game itself
   produces — exactly the approach `FORMULA_XML_RUNTIME_PLAN.md` prescribes for grabbing
   CM2008's decrypted `formula.xml` buffer or the key at key-setup.
4. **Sibling loader shape for ProgramStep** (`GCProgramStepManager::Process`
   installs an `XMLHandler` vtable pair, then load+sort) — architecturally consistent
   with CM2008's `FUN_00861970`, but with **no** decryption stage documented.

## What CM2010 does NOT provide

- **No `formula.xml` key, no key fragments, no key-derivation, no decryptor** — for
  CM2010 or CM2008. It never decrypts anything.
- **No `GCFormulaModule` / `GCFormulaeXMLHandler`** and no formula evaluator work at all.
- **No key-setup / Blowfish-schedule RE** — the crypto path is simply outside its scope.
- **No portable addresses/constants** — GC-engine VAs are per-build; CM2010's are useless
  for CM2008 and were never intended to transfer.
- **No evidence CM2010's own DataXML is or isn't encrypted** — the repo works from an
  already-plaintext `data/source/ProgramStep.xml` and never examines the at-rest form, so
  it cannot even establish CM2010's data-layer key, let alone CM2008's.

---

## Verdict

CM2010 gives a **strategic corroboration** ("the GC engine virtualizes its key path →
recover dynamically") and confirms the **plaintext-UI / encrypted-gameplay-data** split,
but **no key material, no key-derivation, and nothing testable** against
`tools/decrypt_blowfish.py`. The CM2008 `formula.xml` key remains **UNKNOWN**; the
open path is unchanged — a read-only runtime capture on a legitimate, unmodified CM2008
install (decrypted buffer preferred, key at key-setup as fallback), per
`FORMULA_XML_RUNTIME_PLAN.md`. **No shared key is claimed or implied.**
