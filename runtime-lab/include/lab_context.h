/*
 * lab_context — runtime player/match state + the context engine.
 *
 * HONESTY (the crux of this lab): research/PICKER_CALLER_CONTEXT.md shows the within-record
 * context axes are NOT supplied by callers — they are read internally by the obfuscated
 * iterators, so the literal engine quantity behind each axis is UNKNOWN. This engine therefore
 * NEVER hardcodes an axis name. It instead:
 *   - derives each context axis's ROLE from the data itself (effect size + breakpoint symmetry
 *     on the chosen metric) -> INFERRED;
 *   - projects a hypothesised runtime quantity onto an axis's breakpoint range to form a
 *     context value -> APPROXIMATE;
 *   - keeps every candidate mapping tagged with confidence + evidence + alternatives.
 * The draw/quantile axis (slot of the universal +-20.9 grid) is excluded from context.
 */
#ifndef CM2008_LAB_CONTEXT_H
#define CM2008_LAB_CONTEXT_H

#include "resolver_dat.h"

#define LAB_MAX_AXES 8

/* ---- provenance / confidence ------------------------------------------------------- */
typedef enum { PROV_RECONSTRUCTED, PROV_EXPERIMENTAL, PROV_UNKNOWN } lab_provenance;
typedef enum { CONF_VERIFIED, CONF_INFERRED, CONF_APPROXIMATE, CONF_UNKNOWN } lab_confidence;
const char *lab_conf_str(lab_confidence c);
const char *lab_prov_str(lab_provenance p);

/* ---- runtime state ----------------------------------------------------------------- */
/* Positions (subset; codes are the lab's own, NOT CM2008's internal ids — those are
 * UNKNOWN). */
typedef enum { POS_GK, POS_DC, POS_DL, POS_DR, POS_DM, POS_MC, POS_ML, POS_MR,
               POS_AMC, POS_AML, POS_AMR, POS_ST, POS__N } lab_position;
typedef enum { DUTY_DEFEND, DUTY_SUPPORT, DUTY_ATTACK, DUTY__N } lab_duty;

const char *lab_position_str(lab_position p);
int         lab_position_parse(const char *s, lab_position *out);
const char *lab_duty_str(lab_duty d);
int         lab_duty_parse(const char *s, lab_duty *out);

/* A player field's value never claims an unverified CM2008 internal; each is tagged. */
typedef struct {
    int           id, team_id;          /* EXPERIMENTAL identifiers */
    lab_position  position;             /* EXPERIMENTAL input */
    lab_duty      duty;
    int           skill;                /* 1..20 aggregate ability (EXPERIMENTAL input) */
    int           stamina;              /* 0..100 */
    int           condition;            /* 0..100 */
    int           morale;               /* 0..100 */
} lab_player;

typedef struct {
    int    match_id;
    int    minute;                      /* 0..90 */
    int    possession;                  /* 0..100 home possession (EXPERIMENTAL) */
    int    attacking_team;              /* 0/1 */
} lab_match;

typedef struct { lab_player player; lab_match match; } lab_runtime_state;

void lab_state_default(lab_runtime_state *s);

/* ---- context engine ---------------------------------------------------------------- */
/* A derived candidate for one context axis of a record. role/candidate_source are
 * hypotheses; confidence reflects the strength of the data evidence, never certainty. */
typedef struct {
    int            axis_index;          /* index into record->axes */
    double         effect_ratio;        /* max/min conditional mean along this axis (evidence) */
    int            symmetric;           /* breakpoints straddle 0 (differential-like) */
    double         lo, hi;              /* breakpoint range */
    const char    *role;                /* INFERRED role label (data-derived, not a game name) */
    const char    *candidate_source;    /* HYPOTHESISED runtime source field */
    lab_confidence confidence;
    const char    *alternatives;        /* other plausible readings */
} lab_axis_candidate;

typedef struct {
    int                draw_axis;       /* index of the ±20.9 draw axis, or -1 */
    int                n;               /* number of context axes */
    lab_axis_candidate items[LAB_MAX_AXES];
} lab_axis_candidates;

/* Derive the axis candidates for one metric record (needs a loaded db for effect sizing). */
int lab_resolve_axis_candidates(const cm_dist_record *rec, lab_axis_candidates *out);

/* A built context: one value per CONTEXT axis (file order, excluding the draw axis). */
typedef struct {
    int    n;
    double values[LAB_MAX_AXES];
    char   hypothesis[48];
} lab_context;

/* Build a context vector from runtime state under a named hypothesis:
 *   "neutral"  — every context axis at its middle bin (no claim; APPROXIMATE baseline)
 *   "quality"  — project skill onto the quality-candidate axis; others neutral
 *   "volume"   — project position/duty involvement onto the volume-candidate axis; others neutral
 *   "combined" — both of the above
 * Returns 1 on success. The mapping is APPROXIMATE/INFERRED by construction. */
int lab_build_context(const lab_runtime_state *s, const cm_dist_record *rec,
                      const char *hypothesis, lab_context *out);

/* 0..1 attacking-involvement proxy from position+duty (HYPOTHESIS, APPROXIMATE). */
double lab_involvement_proxy(lab_position pos, lab_duty duty);

/* ---- recovered categorical axis = the 7-bin player-position spine ------------------- */
/* research/RESOLVER_AXIS_MAPPING.md (other session): exactly one context axis is categorical
 * (non-uniform fixed marginal); its defence->attack gradient maps onto the game's 7-value
 * vertical position spine (bin0 Central Defender .. bin6 Striker; GK absent from outfield
 * metrics; bin2 = the rare ~3% Sweeper). INFERRED-strong, not byte-proven. */

/* Return the actual axis index of the categorical/position axis (the context axis whose
 * population marginal is non-uniform), or -1 if none is clearly categorical. */
int lab_identify_position_axis(const cm_dist_record *rec);

/* Map a lab position to its 0..6 bin on the 7-value spine, or -1 for GK (absent here).
 * DC->0, DL/DR->1, (Sweeper 2 has no lab enum), DM->3, MC/ML/MR->4, AMC/AML/AMR->5, ST->6. */
int lab_position_spine_bin(lab_position p);
const char *lab_spine_bin_name(int bin);   /* "Central Defender" .. "Striker" */

int lab_cmd_player(int argc, char **argv);

#endif /* CM2008_LAB_CONTEXT_H */
