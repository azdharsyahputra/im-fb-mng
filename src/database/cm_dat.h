/*
 * cm_dat.h — CM2008 "DATa" table container + bit/byte field helpers.
 *
 * Container format (VERIFIED, docs/findings/game-database.md, confirmed against the
 * owner's Player/Club/Contract.dat): 4-byte magic "DATa", u32 record count (LE),
 * then `count` fixed-size records. Record size = (filesize - 8) / count, exact.
 *
 * Two field layouts exist and MUST NOT be mixed (see research/BITPACKED_FIELD_MAP.md):
 *   Layer A — RUNTIME bit-packed struct (the exe's in-memory object). The bit
 *             accessors recovered from FUN_00a2f330 etc. read THIS. Helpers: cm_bits*.
 *   Layer B — ON-DISK byte-aligned record (what the owner's .dat actually stores:
 *             LE ints, IEEE doubles, u8/u8/u16 dates). Helpers: cm_rd_*.
 *
 * -std=c11 -Wall -Wextra clean. No I/O policy here; callers mmap/read the file.
 */
#ifndef CM2008_DATABASE_CM_DAT_H
#define CM2008_DATABASE_CM_DAT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---- container ---- */
typedef struct {
    const uint8_t *data;   /* whole file bytes (borrowed, not owned) */
    size_t         size;
    uint32_t       count;  /* record count from header */
    size_t         rec_size;
} cm_dat_table;

/* Parse a DATa envelope. Returns 0 on success, <0 on error. VERIFIED format. */
static inline int cm_dat_open(cm_dat_table *t, const uint8_t *buf, size_t n) {
    if (!t || !buf || n < 8) return -1;
    if (memcmp(buf, "DATa", 4) != 0) return -2;
    uint32_t count = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) |
                     ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
    size_t body = n - 8;
    if (count == 0) { t->data = buf; t->size = n; t->count = 0; t->rec_size = 0; return 0; }
    if (body % count != 0) return -3;        /* not divisible -> wrong record model */
    t->data = buf; t->size = n; t->count = count; t->rec_size = body / count;
    return 0;
}

/* Pointer to record i (no bounds beyond count). */
static inline const uint8_t *cm_dat_record(const cm_dat_table *t, uint32_t i) {
    return (i < t->count) ? t->data + 8 + (size_t)i * t->rec_size : NULL;
}

/* ---- Layer B: on-disk byte-aligned reads (little-endian) ---- */
static inline uint32_t cm_rd_u32(const uint8_t *r, size_t off) {
    return (uint32_t)r[off] | ((uint32_t)r[off+1] << 8) |
           ((uint32_t)r[off+2] << 16) | ((uint32_t)r[off+3] << 24);
}
static inline int32_t  cm_rd_s32(const uint8_t *r, size_t off) { return (int32_t)cm_rd_u32(r, off); }
static inline uint16_t cm_rd_u16(const uint8_t *r, size_t off) { return (uint16_t)(r[off] | (r[off+1] << 8)); }
static inline uint8_t  cm_rd_u8 (const uint8_t *r, size_t off) { return r[off]; }
static inline double   cm_rd_f64(const uint8_t *r, size_t off) {
    uint64_t v = 0; for (int i = 0; i < 8; i++) v |= (uint64_t)r[off+(size_t)i] << (8*i);
    double d; memcpy(&d, &v, sizeof d); return d;
}
/* u8 day / u8 month / u16 year date triplet (VERIFIED encoding in Contract.dat). */
typedef struct { uint8_t day, month; uint16_t year; } cm_date;
static inline cm_date cm_rd_date(const uint8_t *r, size_t off) {
    cm_date d; d.day = r[off]; d.month = r[off+1]; d.year = cm_rd_u16(r, off+2); return d;
}
static inline int cm_date_valid(cm_date d) {
    return d.day >= 1 && d.day <= 31 && d.month >= 1 && d.month <= 12 &&
           d.year >= 1900 && d.year <= 2100;
}

/* ---- Layer A: runtime bit-packed reads (word at byte off, shift, mask) ---- */
static inline uint32_t cm_bits(const uint8_t *obj, size_t off, unsigned shift, uint32_t mask) {
    return (cm_rd_u32(obj, off) >> shift) & mask;
}
/* The exe's "*(int*)x * 2 >> 1" idiom: 31-bit value in bits[0..30], sign-extended
 * from bit30 (bit31 is a separate flag). VERIFIED (FUN_00a2f330). */
static inline int32_t cm_bits_s31(const uint8_t *obj, size_t off) {
    uint32_t raw = cm_rd_u32(obj, off);
    int32_t shifted = (int32_t)(raw << 1);   /* drop bit31 into the sign position */
    return shifted >> 1;                      /* arithmetic: sign-extend from old bit30 */
}
/* Sign-extend a `width`-bit field at [shift..shift+width-1] (e.g. TicketPrice). */
static inline int32_t cm_bits_signed(const uint8_t *obj, size_t off, unsigned shift, unsigned width) {
    uint32_t raw = cm_rd_u32(obj, off);
    unsigned top = 32u - (shift + width);
    int32_t v = (int32_t)(raw << top);        /* field -> top bits */
    return v >> (32u - width);                /* arithmetic down -> sign-extended */
}

#endif /* CM2008_DATABASE_CM_DAT_H */
