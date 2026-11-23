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
| `DataXML/formula.xml.blowfish` | 482112 | key UNKNOWN (see below) |
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

## formula.xml key — HARD BOUNDARY (static)
`formula.xml.blowfish` holds the per-metric `*Performance` formula coefficients. Its
key was NOT recovered:
- The string adjacent to its filename is the pangram "the quick brown fox jumped over
  the lazy dog." — this belongs to the `TestXMLParserFormulae` debug self-test, and
  it is NOT the key (fails in every Blowfish mode/endianness).
- Brute over **every** printable ASCII string in the binary (as a Blowfish ECB key,
  testing block-0 == "<?xml") found NO hit, so the key is not a plaintext literal.
- The only code reference to the filename (VA 0x664793) lies in a SecuROM-protected
  region that disassembles to junk on disk (valid only in memory at runtime), so the
  key source cannot be read statically.

Conclusion: the formula key is either a non-ASCII/byte-array constant, built by
concatenation like ProgramStep's, or materialized only at runtime. Recovering it
needs a read-only runtime observation of a lawfully-owned, unmodified copy (dump the
key passed to the Blowfish setup, or the decrypted XML from memory). This is the same
class of boundary as the picker FUN_005b5100.

GOOD NEWS: the match *ranking* weights are NOT gated by this file — they are embedded
.data float constants recovered directly from the binary. See
`research/MATCH_PREDICATE_WEIGHTS.md`.

## Classification
- Cipher = Blowfish ECB BE — **VERIFIED**
- MetParams / ProgramStep keys + plaintext — **VERIFIED**
- formula.xml key — **UNKNOWN** (boundary; runtime-only)
