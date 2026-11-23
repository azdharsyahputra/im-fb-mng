/*
 * MatchResolverDistributions reader (verified grammar layer).
 * Reads a caller-provided buffer; floats are skipped, never materialised —
 * this code carries no game data of its own.
 */
#include "resolver_format.h"
#include <string.h>

static int rd_u32(cm_resolver_reader *r, uint32_t *out) {
    if (r->pos + 4 > r->len) return 0;
    const uint8_t *p = r->buf + r->pos;
    *out = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); /* LE */
    r->pos += 4;
    return 1;
}

int cm_resolver_open(cm_resolver_reader *r, const uint8_t *buf, size_t len) {
    if (!r || !buf) return 0;
    r->buf = buf; r->len = len; r->pos = 0;
    uint32_t ver;
    if (!rd_u32(r, &ver) || ver != CM_RESOLVER_VERSION) { r->pos = 0; return 0; }
    return 1;
}

static int looks_like_name(cm_resolver_reader *r) {
    if (r->pos + 4 > r->len) return 0;
    uint32_t n = (uint32_t)r->buf[r->pos] | ((uint32_t)r->buf[r->pos+1] << 8) |
                 ((uint32_t)r->buf[r->pos+2] << 16) | ((uint32_t)r->buf[r->pos+3] << 24);
    if (n < 3 || n > 24 || r->pos + 4 + n > r->len) return 0;
    const uint8_t *c = r->buf + r->pos + 4;
    if (!((c[0] >= 'A' && c[0] <= 'Z'))) return 0;
    for (uint32_t i = 0; i < n; i++) {
        uint8_t b = c[i];
        if (!((b>='A'&&b<='Z')||(b>='a'&&b<='z')||(b>='0'&&b<='9'))) return 0;
    }
    return 1;
}

int cm_resolver_next_record(cm_resolver_reader *r, char *name, int cap) {
    if (!looks_like_name(r)) return 0;
    uint32_t n; rd_u32(r, &n);
    int k = (int)n < cap - 1 ? (int)n : cap - 1;
    memcpy(name, r->buf + r->pos, (size_t)k);
    name[k] = '\0';
    r->pos += n;
    return (int)n;
}

int cm_resolver_read_block_header(cm_resolver_reader *r, uint32_t *n_buckets) {
    return rd_u32(r, n_buckets);
}

int cm_resolver_read_bucket(cm_resolver_reader *r, uint32_t *key, uint32_t *n_values) {
    if (!rd_u32(r, key) || !rd_u32(r, n_values)) return 0;
    /* skip the float payload without reading values */
    size_t skip = (size_t)(*n_values) * 4u;
    if (r->pos + skip > r->len) return 0;
    r->pos += skip;
    return 1;
}
