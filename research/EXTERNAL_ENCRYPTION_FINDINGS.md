# EXTERNAL ENCRYPTION FINDINGS — CM2008 DataXML / formula.xml.blowfish

**Agent:** formula-xml-osint · **Date:** 2026-11-23

Focus: everything the public internet offers on the *encryption* of CM2008's DataXML, and whether any of it unlocks `formula.xml.blowfish`.

---

## HEADLINE

**No external source yields the `formula.xml` Blowfish key, key-derivation, or an equivalent decryptor for any BGS-era Championship Manager title.** No candidate key was found, therefore the verified decryptor (`tools/decrypt_blowfish.py`) was **not** invoked (nothing to test). The `formula.xml` key remains UNKNOWN after an extensive public search.

---

## KEY-CANDIDATE TEST RESULTS

| Candidate key | Source | Tested against decryptor? | Result |
|---|---|---|---|
| *(none)* | — | — | No candidate key surfaced anywhere online. |

The verified decryptor remains the project's tool of record:
`python3 -I tools/decrypt_blowfish.py "<cm2008.exe>" formula.xml.blowfish "<KEY>" out.xml` → success iff output begins with `<?xml`. **It was not run during this OSINT pass because no external key candidate exists to try.** Cipher is confirmed Blowfish ECB big-endian (project test-vector verification stands; no external source contradicts it).

---

## WHAT THE SEARCH CONFIRMED ABOUT THE ENCRYPTION

1. **The two recovered keys are not public.** `f8956ju8y9g54tiou` (MetParams.dat) and `Q1ofsktoPkjS89jdnJeqIO` (ProgramStep.xml) each appear in exactly 2 GitHub files — both in the user's own repo (`azdharsyahputra/im-fb-mng`). They were neither leaked publicly nor obtained from an external source; they are original to this project. This also means the `formula.xml` key is unlikely to be sitting in any public corpus either.

2. **No third-party ever decrypted BGS-era DataXML.** Code searches for `formula.xml.blowfish`, `DataXML`+blowfish, `champman.xsd`, `MetParams.dat`, `ProgramStep.xml`+blowfish return **only** the user's own repo (plus unrelated wordlist/dictionary noise). The entire public record of CM2008 DataXML reverse-engineering *is this project*.

3. **The protected-loader finding is corroborated by the sibling engine.** `MagnificentS/cm2010-run` proves that CM2010 (`cm2010.exe`, same BGS "GC" engine) is **SecuROM-virtualised** on its central dispatch/compare code — static decompile yields `swi(0xCD)`/`in(0x77)`/`halt_baddata` and the real logic is unrecoverable statically; they had to confirm it against a **live process**. This is the same class of protection that puts the CM2008 `formula.xml` key loader out of static reach. (Protection is *selective*: other functions like `FUN_00b81900` decompile as clean x86, so not every path is virtualised.)

4. **Not all BGS XML is encrypted.** CM2010's `FrontEndScreens/XML/Compiled` are **plaintext** XML (namespace `urn:BGStudios:FELayout`), parsed directly by cm2010-run's `parse.py`. Only the `DataXML` layer (formula.xml et al.) is Blowfish-wrapped. Useful boundary: the encryption is applied to the gameplay-data XML, not the UI-layout XML.

---

## PATTERNS FROM GENERIC GAME-BLOWFISH PRIOR ART (not CM, background only)

- **Key embedded/derived in the binary, extracted at runtime** — DDON ("Blowfish key is compiled into the application"). Matches the hypothesis that CM2008's key is produced by code in a protected region.
- **Key validated by the game** — CnCNet C&C mix files reject a random key; there is a signature/pattern the loader expects. Implication: a brute-forced or guessed `formula.xml` key must still satisfy the loader's own checks; recovering the *real* key (or the decrypted buffer) is safer than guessing.
- **Simple key transforms before Blowfish** — CatSystem2 XORs the key with 0xCD before key-setup. Worth keeping in mind: the CM2008 `formula.xml` key may be a literal *passed through a transform*, unlike the two already-recovered literal keys.
- **Endianness** — repeatedly the #1 cause of "decrypts to noise." The project already uses big-endian ECB (verified), so this is settled here.
- **Blowfish→XML→DOM is a known shipping pattern** (CodeGuru MSXML2 example): decrypt Blowfish payload, then hand the plaintext to an XML DOM — architecturally identical to what CM2008 does with DataXML.

---

## ASSESSMENT: DOES ANYTHING EXTERNAL UNLOCK formula.xml?

**No.** The public internet contains no key, no decryptor, and no decrypted formula content for CM2008 (or any BGS CM title). The only actionable external contribution is **strategic**, from `cm2010-run`:

- Static recovery of the key path is blocked by SecuROM virtualization (independently demonstrated on the sibling build) → **pursue the key dynamically**.
- Proven runtime approach to copy for CM2008: attach to a running `cm2008.exe` (debugger such as `x32dbg`, or an injected read-only probe DLL + mem-read bridge), and at **DataXML load time** capture either:
  - the Blowfish **key string / scheduled P-array + S-boxes** at the key-setup call, or
  - the **decrypted `formula.xml` buffer** in memory immediately after decryption (bypasses the need for the key entirely — gives the plaintext/expression trees directly).
- **Do not reuse CM2010 addresses** — they are per-build. Re-anchor on the CM2008 binary (RTTI/vtable walk, as cm2010-run did for CM2010).

This complements, and does not replace, the project's existing clean-room evaluator + recovered grammar/coefficients: runtime capture of the decrypted `formula.xml` would supply the missing expression trees even if the literal key is never named.
