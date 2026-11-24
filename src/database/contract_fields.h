/*
 * contract_fields.h — Contract.dat (on-disk, byte-aligned) accessors.
 *
 * No Contract schema-registration function was found in the committed dumps, so field
 * NAMES are UNKNOWN/INFERRED (research/BITPACKED_FIELD_MAP.md §5.1 and §7). The record
 * STRUCTURE below is VERIFIED against the owner's Contract.dat (109484 x 29 B): a u32
 * id, two u8/u8/u16 date triplets (start <= end in 100% of valid pairs), two optional
 * u16s, three u32 entity links, and a near-always-zero u8 flag.
 */
#ifndef CM2008_DATABASE_CONTRACT_FIELDS_H
#define CM2008_DATABASE_CONTRACT_FIELDS_H

#include "cm_dat.h"

typedef struct {
    uint32_t record_id;   /* +0   VERIFIED struct (ascending key) */
    cm_date  start_date;  /* +4   VERIFIED date; "start" INFERRED (<= end_date) */
    uint16_t opt_a;       /* +8   UNKNOWN (16% nonzero) */
    cm_date  end_date;    /* +10  VERIFIED date; "end" INFERRED (>= start_date) */
    uint16_t opt_b;       /* +14  UNKNOWN (16% nonzero) */
    uint32_t link0;       /* +16  INFERRED id-like (<=11381) */
    uint32_t link1;       /* +20  INFERRED id-like (<=159290, most distinct) */
    uint32_t link2;       /* +24  INFERRED id-like (<=11381) */
    uint8_t  flag;        /* +28  INFERRED IsDeleted-like (0 in 99.9%) */
    /* TODO(UNKNOWN): bind semantic names to link0..2 / opt_a / opt_b — needs the
     * on-disk loader or a Contract schema function (neither in committed dumps). */
} cm_contract_ondisk;

void cm_contract_decode_ondisk(const uint8_t *rec, cm_contract_ondisk *out);

#endif /* CM2008_DATABASE_CONTRACT_FIELDS_H */
