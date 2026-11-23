# CM2008 Encrypted Data Recovery (Blowfish)

Goal: read the game's own encrypted data files for interoperability with the
reconstruction. No binary is executed; no DRM is bypassed. The installer ISO's
InstallShield cabs were extracted with `unshield`; the cipher/keys were recovered by
static analysis of the binary.

## Cipher (VERIFIED)
Standard **Blowfish, ECB mode, big-endian block words**. Confirmed two ways:
1. The implementation reproduces the official Blowfish ECB test vectors exactly
   (key 0x0000000000000000 / pt 0 -> ct 0x4EF997456198DD78, etc.).
2. Decrypting `MetParams.dat.blowfish` and `ProgramStep.xml.blowfish` with the keys
   below yields valid plaintext.

The Blowfish P/S init constants are the standard pi-derived public values, present in
the binary at VA 0x809b50 (LE). Decryptor: `tools/decrypt_blowfish.py`.

## Encrypted files (in installer `Setup/data1.cab`, group DataXML / DataMisc)
| file | size (enc) | status |
|---|---|---|
| `DataXML/formula.xml.blowfish` | 482112 | **DECRYPTED** -> valid XML, 1349 <formula> |
| `DataXML/ProgramStep.xml.blowfish` | 44800 | **DECRYPTED** -> valid XML |
| `DataMisc/MetParams.dat.blowfish` | 48 | **DECRYPTED** -> metrics URL |

## Recovered keys (VERIFIED — each produces valid plaintext)
| file | key | how it is stored |
|---|---|---|
| MetParams.dat | `f8956ju8y9g54tiou` | single ASCII string adjacent to the filename |
| ProgramStep.xml | `Q1ofsktoPkjS89jdnJeqIO` | 4 fragments (`nJeqIO` `jS89jd` `ktoPk` `Q1ofs`) adjacent to the filename, concatenated in **reverse** order (loader FUN_00861970 appends them via 4x FUN_00404c90) |

`ProgramStep.xml` decrypts to `<?xml version="1.0" encoding="iso-8859-1"?><dataroot>
...<Program> Type="GCProgramStep"...` — i.e. the game-flow command-tree definitions
that the UI interpreter FUN_008f90f0 executes. `MetParams.dat` is just the Eidos
metrics endpoint URL.

## formula.xml key — SOLVED (static, VERIFIED): key = `deliverance`
`formula.xml.blowfish` decrypts with the Blowfish ECB BE key **`deliverance`** (11 bytes)
-> valid XML, 1349 `<formula>` elements, root `<formrule>` -> champman.xsd. Recovered by
tracing the key handoff (no runtime, no DRM bypass): see research/BLOWFISH_KEY_DATAFLOW.md.

How it was hidden (why the earlier brute missed it):
- The string adjacent to the filename is the pangram "the quick brown fox jumped over the
  lazy dog." — a DECOY (the `TestXMLParserFormulae` self-test string); it fails as a key.
- The real key is DERIVED at load time: the formula loader (FUN_006641a0, ordinary
  disassemblable — only the shared decrypt FUN_0042c6d0/FUN_00404c90 internals are
  SecuROM-mangled, and they aren't needed) slices 11 single characters out of the pangram
  at fixed indices [29,7,14,37,11,24,28,6,36,2,25] = "ecnareviled", then reverse-concatenates
  them (the same idiom as ProgramStep's key) -> **"deliverance"**. The literal never appears
  in the binary, which is exactly why a printable-string brute could not find it.
- Shared decrypt convention: FUN_0042c6d0(filename, output, key_string, mode); the Blowfish
  init table (VA 0xc09b50) has one xref -> context-init FUN_0042c3b4; FUN_0042c6d0 has
  exactly 3 callers (formula 0x6647bc, MetParams 0x801bbf, ProgramStep 0x861b2a).

Decrypt it with `tools/decrypt_blowfish.py <exe> formula.xml.blowfish deliverance formula.xml`.
(The decrypted formula.xml is the owner's copyrighted game data: kept local / out of the
repo, never committed.)

## Classification
- Cipher = Blowfish ECB BE — **VERIFIED**
- MetParams / ProgramStep keys + plaintext — **VERIFIED**
- formula.xml key = `deliverance` (derived; static) + plaintext — **VERIFIED**
