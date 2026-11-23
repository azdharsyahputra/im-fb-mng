/*
 * MatchResolverDistributions file format — reconstructed grammar (CM2008).
 *
 * VERIFIED layer (from structural analysis of the extracted files; NO
 * proprietary distribution values are reproduced here):
 *
 *   file    = u32 version(=64) , record*
 *   record  = u32 nameLen , char[nameLen] name , block+      (until next name/EOF)
 *   block   = u32 nBuckets , bucket[nBuckets]
 *   bucket  = u32 key , u32 nValues , f32[nValues]
 *
 * The first block of each record parses cleanly with this grammar; deeper within
 * a record there are block variants (e.g. many empty buckets) not yet fully
 * pinned — see docs/findings/match-resolver.md. This header declares a reader
 * for the VERIFIED layer that operates on a caller-provided buffer (the user's
 * own extracted file) and never embeds game data.
 */
#ifndef CM2008_MATCH_RESOLVER_FORMAT_H
#define CM2008_MATCH_RESOLVER_FORMAT_H
#include <stddef.h>
#include <stdint.h>

#define CM_RESOLVER_VERSION 64

typedef struct cm_resolver_reader {
    const uint8_t *buf;
    size_t len;
    size_t pos;
} cm_resolver_reader;

/* Init over a caller buffer; returns 1 and sets pos past the version word if the
 * leading version == 64, else 0. */
int cm_resolver_open(cm_resolver_reader *r, const uint8_t *buf, size_t len);

/* Read the next record name into name[cap]; returns name length (>0) and leaves
 * pos at the record's first block, or 0 if the next bytes are not a valid name. */
int cm_resolver_next_record(cm_resolver_reader *r, char *name, int cap);

/* Read one block header: nBuckets. Returns 1 on success. */
int cm_resolver_read_block_header(cm_resolver_reader *r, uint32_t *n_buckets);

/* Read one bucket header: key + nValues, then SKIP its floats (values are not
 * materialised). Returns 1 on success. */
int cm_resolver_read_bucket(cm_resolver_reader *r, uint32_t *key, uint32_t *n_values);

#endif /* CM2008_MATCH_RESOLVER_FORMAT_H */
