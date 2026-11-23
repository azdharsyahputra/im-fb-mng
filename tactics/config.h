/*
 * Tactical/match tunables — reconstructed from CM2008.exe.
 *
 * Observed pattern (e.g. FUN_0083ea10 "MarkingDistanceScale"): each tunable is a
 * named field looked up in a data source ("PlayerData") at first use and cached;
 * if the field is absent the exe returns a HARDCODED default. So the exe carries
 * fallback defaults even though primary values are data-driven (see
 * docs/findings/tactics-config.md and the data-driven formula finding).
 *
 * `cm_data_source` abstracts that lookup so these getters can be reconstructed
 * 1:1 and unit-tested without the game data cabs.
 */
#ifndef CM2008_TACTICS_CONFIG_H
#define CM2008_TACTICS_CONFIG_H

/* Returns 1 and sets *out if `name` exists in the source, else 0. */
typedef int (*cm_data_get_float_fn)(void *ctx, const char *name, float *out);

typedef struct cm_data_source {
    cm_data_get_float_fn get_float;
    void *ctx;
} cm_data_source;

/* FUN_0083ea10: "MarkingDistanceScale" from PlayerData; default 0.02 (Observed). */
float cm_marking_distance_scale(const cm_data_source *src);

#endif /* CM2008_TACTICS_CONFIG_H */
