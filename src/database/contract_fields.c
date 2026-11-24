/*
 * contract_fields.c — Contract.dat on-disk decoder. See contract_fields.h and
 * research/BITPACKED_FIELD_MAP.md §5.1. Structure VERIFIED vs owner's Contract.dat.
 */
#include "contract_fields.h"

void cm_contract_decode_ondisk(const uint8_t *rec, cm_contract_ondisk *o) {
    o->record_id  = cm_rd_u32(rec, 0);
    o->start_date = cm_rd_date(rec, 4);
    o->opt_a      = cm_rd_u16(rec, 8);
    o->end_date   = cm_rd_date(rec, 10);
    o->opt_b      = cm_rd_u16(rec, 14);
    o->link0      = cm_rd_u32(rec, 16);
    o->link1      = cm_rd_u32(rec, 20);
    o->link2      = cm_rd_u32(rec, 24);
    o->flag       = cm_rd_u8(rec, 28);
}
