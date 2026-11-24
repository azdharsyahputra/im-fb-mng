/*
 * player_fields.h — Player record field accessors (CM2008).
 *
 * LAYER A (runtime bit-packed attributes): VERIFIED from the player/staff exporters
 *   (raw/players.c:399/543/...): 7-bit attributes packed 3-per-dword, read as
 *   `word >> shift & 0x7f`, sentinel 0x7f. CurrentAbility/PotentialAbility are
 *   name-bound; the remaining attribute<->shift bindings are UNKNOWN (TODO).
 *   The 41-field schema (names/order/declared widths) is FUN_00a84df0.
 *
 * LAYER B (on-disk Player.dat, byte-aligned): structure VERIFIED vs the owner's file
 *   (85035 x 41 B); field NAMES UNKNOWN (no name-binding on-disk accessor in dumps).
 */
#ifndef CM2008_DATABASE_PLAYER_FIELDS_H
#define CM2008_DATABASE_PLAYER_FIELDS_H

#include "cm_dat.h"

/* ---------- LAYER A: runtime 7-bit attribute reads (VERIFIED idiom) ---------- */
#define CM_PLAYER_ATTR_SENTINEL 0x7fu
/* Read the attribute in word `word_index` at bit `shift`. 0x7f means "absent".
 * VERIFIED pattern (raw/players.c): e.g. PotentialAbility = word[5] & 0x7f. */
static inline uint32_t cm_player_attr7(const uint8_t *obj, size_t word_index, unsigned shift) {
    return cm_bits(obj, word_index * 4u, shift, 0x7fu);
}
/* Name-bound examples (VERIFIED). NB: which word holds Current/Potential differs by
 * record variant in the dumps; these match the FUN_00a30d00 variant (raw/players.c:423/543). */
static inline uint32_t cm_player_potential_ability(const uint8_t *obj) { return cm_player_attr7(obj, 5,  0); }
static inline uint32_t cm_player_current_ability  (const uint8_t *obj) { return cm_player_attr7(obj, 0xc, 0); }
/* TODO(UNKNOWN): bind the other 7-bit attributes (Passing, Shot, Tackling, ...) to
 * their (word,shift) — the exporters interleave variants; not resolved statically. */

/* ---------- LAYER B: on-disk Player.dat (byte-aligned, VERIFIED structure) ------- */
typedef struct {
    uint32_t record_id;   /* +0  u32 sequential (VERIFIED struct) */
    uint32_t f4;          /* +4  u32 small/sentinel (UNKNOWN name) */
    uint32_t f8;          /* +8  u32 small/sentinel (UNKNOWN name) */
    uint32_t f12;         /* +12 u32 small/sentinel (UNKNOWN name) */
    double   d16;         /* +16 f64 (UNKNOWN name; often +/-1.0) */
    uint32_t f24;         /* +24 u32 small/sentinel (UNKNOWN name) */
    uint32_t f28;         /* +28 u32 small/sentinel (UNKNOWN name) */
    uint8_t  f32;         /* +32 u8 flag/0xff (UNKNOWN name) */
    uint32_t f33;         /* +33 u32 (unaligned) value/sentinel (UNKNOWN name) */
    uint8_t  tail[4];     /* +37 four small bytes, rating-like (INFERRED) */
    /* TODO(UNKNOWN): bind names — the on-disk Player loader is not in committed dumps. */
} cm_player_ondisk;

void cm_player_decode_ondisk(const uint8_t *rec, cm_player_ondisk *out);

#endif /* CM2008_DATABASE_PLAYER_FIELDS_H */
