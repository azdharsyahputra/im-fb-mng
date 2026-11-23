#!/usr/bin/env python3
"""
Clean-room decryptor for Championship Manager 2008 *.blowfish data files.

CM2008 ships several data files encrypted with standard Blowfish in ECB mode,
big-endian block words (verified against official Blowfish test vectors and by
decrypting MetParams/ProgramStep to valid plaintext). This tool reverses that so
the owner of a legitimate copy can read their own data files for interoperability.

It does NOT run the game, patch it, or defeat any DRM. The Blowfish init tables are
read from a CM2008 binary you supply (they are the standard public pi-derived
constants); the per-file keys below were recovered from the binary's own strings.

Recovered keys (VERIFIED by producing valid plaintext):
    DataMisc/MetParams.dat.blowfish   -> b"f8956ju8y9g54tiou"
    DataXML/ProgramStep.xml.blowfish  -> b"Q1ofsktoPkjS89jdnJeqIO"
                                         (4 fragments nJeqIO|jS89jd|ktoPk|Q1ofs
                                          concatenated in reverse order)
    DataXML/formula.xml.blowfish      -> b"deliverance" (VERIFIED: decrypts to valid XML,
                                         1349 <formula> elements). The key is DERIVED, not
                                         a literal: 11 single chars are sliced from the decoy
                                         pangram "the quick brown fox jumped over the lazy
                                         dog." at indices [29,7,14,37,11,24,28,6,36,2,25] and
                                         reverse-concatenated -> "deliverance". That is why a
                                         printable-string brute never found it. See
                                         research/BLOWFISH_KEY_DATAFLOW.md.

Usage:
    python3 decrypt_blowfish.py <cm2008.exe> <infile.blowfish> <key> <outfile>
The Blowfish P/S init constants are read from <cm2008.exe> at the first occurrence
of the standard table (little-endian P0 = 0x243f6a88).
"""
import sys, struct

MASK = 0xffffffff

def load_tables(exe_path):
    exe = open(exe_path, "rb").read()
    off = exe.find(bytes.fromhex("886a3f24"))   # LE 0x243f6a88 = Blowfish P[0]
    if off < 0:
        raise SystemExit("Blowfish init table not found in binary")
    vals = list(struct.unpack_from("<%dI" % (18 + 4*256), exe, off))
    P0 = vals[:18]
    S0 = [vals[18+256*k:18+256*(k+1)] for k in range(4)]
    assert P0[0] == 0x243f6a88 and S0[0][0] == 0xd1310ba6, "unexpected table"
    return P0, S0

def make_cipher(key, P0, S0):
    P = list(P0); S = [s[:] for s in S0]
    def Ffn(x):
        return ((((S[0][(x>>24)&0xff] + S[1][(x>>16)&0xff]) & MASK)
                 ^ S[2][(x>>8)&0xff]) + S[3][x&0xff]) & MASK
    def enc(L, R):
        for i in range(16): L ^= P[i]; R ^= Ffn(L); L, R = R, L
        L, R = R, L; R ^= P[16]; L ^= P[17]; return L & MASK, R & MASK
    def dec(L, R):
        for i in range(17, 1, -1): L ^= P[i]; R ^= Ffn(L); L, R = R, L
        L, R = R, L; R ^= P[1]; L ^= P[0]; return L & MASK, R & MASK
    j = 0
    for i in range(18):
        d = 0
        for _ in range(4): d = ((d << 8) | key[j]) & MASK; j = (j+1) % len(key)
        P[i] ^= d
    L = R = 0
    for i in range(0, 18, 2): L, R = enc(L, R); P[i], P[i+1] = L, R
    for s in S:
        for i in range(0, 256, 2): L, R = enc(L, R); s[i], s[i+1] = L, R
    return enc, dec

def decrypt_ecb(data, dec):
    out = bytearray()
    for o in range(0, len(data) - 7, 8):
        A, B = struct.unpack_from(">II", data, o)
        l, r = dec(A, B)
        out += struct.pack(">II", l & MASK, r & MASK)
    return bytes(out)

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(__doc__); raise SystemExit(2)
    exe, infile, key, outfile = sys.argv[1], sys.argv[2], sys.argv[3].encode(), sys.argv[4]
    P0, S0 = load_tables(exe)
    _, dec = make_cipher(key, P0, S0)
    pt = decrypt_ecb(open(infile, "rb").read(), dec)
    open(outfile, "wb").write(pt)
    head = pt[:64]
    print("wrote %s (%d bytes); head=%r" % (outfile, len(pt), head))
