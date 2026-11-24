/*
 * club_fields.h — Club record field accessors (CM2008).
 *
 * LAYER A (runtime bit-packed struct): VERIFIED from FUN_00a2f330
 *   (docs/ghidra_dumps/raw/player_development.c:3466). Reads a RUNTIME club object,
 *   NOT the on-disk Club.dat (which is byte-aligned — see LAYER B and
 *   research/BITPACKED_FIELD_MAP.md §0). Offsets/shifts/masks come straight from the
 *   exporter disassembly; they have NOT been validated against the owner's file
 *   because the two layouts differ.
 *
 * LAYER B (on-disk Club.dat, byte-aligned): structure VERIFIED against the owner's
 *   file; most field NAMES are UNKNOWN (no name-binding accessor in the dumps), so
 *   only data-validated structural fields are exposed; the rest are explicit TODO.
 */
#ifndef CM2008_DATABASE_CLUB_FIELDS_H
#define CM2008_DATABASE_CLUB_FIELDS_H

#include "cm_dat.h"

#define CM_CLUB_FIELD_COUNT 111
extern const char *const CM_CLUB_FIELD_NAMES[CM_CLUB_FIELD_COUNT]; /* VERIFIED schema */
int         cm_club_field_index(const char *name);                /* -1 if absent */
const char *cm_club_field_name(int idx);                          /* NULL if oob */

/* ---------- LAYER A: runtime bit-packed accessors (VERIFIED from code) ---------- */
/* `obj` points at the runtime club struct (the object FUN_00a2f330 exported). */
typedef struct {
    uint32_t national_club;      /* +4 b12, 1  */
    uint32_t administration;     /* +4 b13, 1  */
    int32_t  bank_balance;       /* +8  s31    */
    int32_t  club_coefficient;   /* +0xc s31 (null -1) */ int coef_valid;
    uint32_t club_status_id;     /* +0x10 b0..2 (null 7) */ int status_valid;
    int32_t  ground_cost;        /* +0x14 s31  */
    uint32_t kit_away1, kit_away2, kit_home1, kit_home2; /* +0x18/+0x1c/+0x20/+0x24 */
    int32_t  merchandising;      /* +0x28 s31  */
    int32_t  outstanding_loan;   /* +0x30 s31  */
    uint32_t loan_flag;          /* +0x30 b31  */
    uint32_t record_league_att;  /* +0x34 b0..17 (null 0x3ffff) */ int rla_valid;
    uint32_t record_league_club; /* +0x38 b0..15 (null 0xffff)  */ int rlc_valid;
    cm_date  record_league_date; /* +0x38/+0x3c packed */
    uint32_t reputation;         /* +0x3c b12..18, 7 */
    int32_t  sponsor_amount;     /* +0x48 s31 */
    cm_date  sponsor_end;        /* +0x4c packed */
    cm_date  sponsor_start;      /* +0x4c/+0x50 packed */
    int32_t  ticket_price;       /* +0x50 b17..27 signed, 11 */
    uint32_t training;           /* +0x54 b0..7, 8 */
    int32_t  tv_revenue;         /* +0x58 s31 */
    uint32_t most_appearances;   /* +0x60 b0..29 (null 0x3fffffff) */ int ma_valid;
    uint32_t most_goals;         /* +0x68 b0..29 (null 0x3fffffff) */ int mg_valid;
    uint32_t youth_academy;      /* +0x68 b30, 1 */
    uint32_t youth_development;  /* +0x6c b0..6 (null 0x7f) */ int yd_valid;
    uint32_t home_match_day_id;  /* +0x6c b7..12 (null 0x3f) */ int hmd_valid;
    uint32_t year_founded;       /* +0x6c b13..24 (null 0xfff) */ int yf_valid;
    uint32_t season_ticket;      /* +0x70 b0..29 */
} cm_club_runtime;

/* Decode every VERIFIED runtime field from a runtime club object. */
void cm_club_decode_runtime(const uint8_t *obj, cm_club_runtime *out);

/* ---------- LAYER B: on-disk Club.dat (byte-aligned, VERIFIED structure) --------- */
typedef struct {
    uint32_t record_id;      /* +0   u32  (VERIFIED struct) */
    uint32_t status_enum;    /* +8   u32  (VERIFIED struct; name UNKNOWN) */
    double   financial0;     /* +14  f64  (VERIFIED struct; name UNKNOWN, e.g. bank) */
    double   financial1;     /* +38  f64  (VERIFIED struct; name UNKNOWN) */
    double   financial2;     /* +62  f64  (VERIFIED struct; name UNKNOWN) */
    double   financial3;     /* +74  f64  (VERIFIED struct; name UNKNOWN) */
    double   financial4;     /* +110 f64  (VERIFIED struct; name UNKNOWN) */
    uint16_t year_founded;   /* +120 u16  (VERIFIED value; name INFERRED by range)  */
    /* TODO(UNKNOWN): ~100 remaining byte-aligned fields between the located doubles;
     * needs the on-disk loader decompiled (not in committed dumps). */
} cm_club_ondisk;

void cm_club_decode_ondisk(const uint8_t *rec, cm_club_ondisk *out);

#endif /* CM2008_DATABASE_CLUB_FIELDS_H */
