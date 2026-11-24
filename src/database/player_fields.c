/*
 * player_fields.c — Player.dat on-disk decoder + runtime attribute helpers.
 * See player_fields.h and research/BITPACKED_FIELD_MAP.md §4/§5.2.
 * On-disk structure VERIFIED vs the owner's Player.dat (85035 x 41 B).
 */
#include "player_fields.h"

void cm_player_decode_ondisk(const uint8_t *rec, cm_player_ondisk *o) {
    o->record_id = cm_rd_u32(rec, 0);
    o->f4  = cm_rd_u32(rec, 4);
    o->f8  = cm_rd_u32(rec, 8);
    o->f12 = cm_rd_u32(rec, 12);
    o->d16 = cm_rd_f64(rec, 16);
    o->f24 = cm_rd_u32(rec, 24);
    o->f28 = cm_rd_u32(rec, 28);
    o->f32 = cm_rd_u8 (rec, 32);
    o->f33 = cm_rd_u32(rec, 33);   /* unaligned on disk (VERIFIED) */
    for (int i = 0; i < 4; i++) o->tail[i] = rec[37 + (size_t)i];
}
