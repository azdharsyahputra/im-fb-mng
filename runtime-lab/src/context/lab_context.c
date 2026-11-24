#include "lab_context.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

const char *lab_conf_str(lab_confidence c) {
    switch (c) { case CONF_VERIFIED: return "VERIFIED"; case CONF_INFERRED: return "INFERRED";
                 case CONF_APPROXIMATE: return "APPROXIMATE"; default: return "UNKNOWN"; }
}
const char *lab_prov_str(lab_provenance p) {
    switch (p) { case PROV_RECONSTRUCTED: return "RECONSTRUCTED"; case PROV_EXPERIMENTAL: return "EXPERIMENTAL";
                 default: return "UNKNOWN"; }
}

static const char *POS[POS__N] = { "GK","DC","DL","DR","DM","MC","ML","MR","AMC","AML","AMR","ST" };
const char *lab_position_str(lab_position p) { return (p >= 0 && p < POS__N) ? POS[p] : "?"; }
int lab_position_parse(const char *s, lab_position *out) {
    for (int i = 0; i < POS__N; i++) if (strcasecmp(s, POS[i]) == 0) { *out = (lab_position)i; return 1; }
    return 0;
}
static const char *DUTY[DUTY__N] = { "Defend","Support","Attack" };
const char *lab_duty_str(lab_duty d) { return (d >= 0 && d < DUTY__N) ? DUTY[d] : "?"; }
int lab_duty_parse(const char *s, lab_duty *out) {
    for (int i = 0; i < DUTY__N; i++) if (strcasecmp(s, DUTY[i]) == 0) { *out = (lab_duty)i; return 1; }
    return 0;
}

void lab_state_default(lab_runtime_state *s) {
    memset(s, 0, sizeof *s);
    s->player.id = 1; s->player.team_id = 1;
    s->player.position = POS_MC; s->player.duty = DUTY_SUPPORT;
    s->player.skill = 15; s->player.stamina = 80; s->player.condition = 90; s->player.morale = 70;
    s->match.match_id = 1; s->match.minute = 45; s->match.possession = 50; s->match.attacking_team = 0;
}

/* Attacking-involvement proxy (HYPOTHESIS, APPROXIMATE): forward + attack duty => high;
 * defender + defend duty => low. Returns 0..1. */
double lab_involvement_proxy(lab_position pos, lab_duty duty) {
    double base;
    switch (pos) {
        case POS_ST:  base = 0.95; break;
        case POS_AMC: case POS_AML: case POS_AMR: base = 0.80; break;
        case POS_MC:  case POS_ML:  case POS_MR:  base = 0.55; break;
        case POS_DM:  base = 0.40; break;
        case POS_DC:  case POS_DL:  case POS_DR:  base = 0.25; break;
        case POS_GK:  base = 0.05; break;
        default:      base = 0.5;  break;
    }
    double dadj = (duty == DUTY_ATTACK) ? 0.12 : (duty == DUTY_DEFEND ? -0.12 : 0.0);
    double v = base + dadj;
    if (v < 0) v = 0; if (v > 1) v = 1;
    return v;
}

/* ---- recovered 7-bin position spine (see RESOLVER_AXIS_MAPPING.md) ------------------ */
static const char *SPINE[7] = {
    "Central Defender", "Full-back", "Sweeper", "Defensive Mid", "Midfielder",
    "Attacking Mid", "Striker"
};
const char *lab_spine_bin_name(int bin) { return (bin >= 0 && bin < 7) ? SPINE[bin] : "?"; }

int lab_position_spine_bin(lab_position p) {
    switch (p) {
        case POS_GK: return -1;                 /* absent from outfield-metric axes */
        case POS_DC: return 0;
        case POS_DL: case POS_DR: return 1;     /* full-back/stopper */
        case POS_DM: return 3;                  /* bin2 = Sweeper (no lab enum) */
        case POS_MC: case POS_ML: case POS_MR: return 4;
        case POS_AMC: case POS_AML: case POS_AMR: return 5;
        case POS_ST: return 6;
        default: return -1;
    }
}

/* marginal maxDev from uniform for one axis (how categorical it is) */
static double axis_marginal_maxdev(const cm_dist_record *rec, int a) {
    if (!cm_resolver_has_grid(rec) || a < 0 || a >= rec->n_axes) return 0;
    int cells = cm_resolver_grid_cells(rec), dim = rec->dims[a];
    int stride = 1; for (int b = a + 1; b < rec->n_axes; b++) stride *= rec->dims[b];
    double *marg = calloc(dim, sizeof(double)); double tot = 0;
    for (int k = 0; k < cells; k++) { int c = (k / stride) % dim; marg[c] += rec->triples[k].cum; tot += rec->triples[k].cum; }
    double uni = 1.0 / dim, maxdev = 0;
    if (tot > 0) for (int b = 0; b < dim; b++) { double d = marg[b] / tot - uni; if (d < 0) d = -d; if (d > maxdev) maxdev = d; }
    free(marg);
    return maxdev;
}

int lab_identify_position_axis(const cm_dist_record *rec) {
    if (!cm_resolver_has_grid(rec)) return -1;
    int draw = cm_resolver_draw_axis_index(rec);
    int best = -1; double bestdev = 0.05; /* threshold: uniform axes sit at ~0 */
    for (int a = 0; a < rec->n_axes; a++) {
        if (a == draw) continue;
        double dev = axis_marginal_maxdev(rec, a);
        if (dev > bestdev) { bestdev = dev; best = a; }
    }
    return best;
}

/* actual axis index for context-order index ci (file order, excluding draw axis) */
static int ctx_actual(const cm_dist_record *rec, int draw, int ci) {
    int k = 0;
    for (int a = 0; a < rec->n_axes; a++) { if (a == draw) continue; if (k == ci) return a; k++; }
    return -1;
}
/* a value that bins to `bin` on axis `a` */
static double value_for_bin(const cm_dist_record *rec, int a, int bin) {
    const cm_axis *ax = &rec->axes[a];
    if (ax->n_breaks <= 0) return 0;
    if (bin <= 0) return (double)ax->breaks[0] - 1.0;
    if (bin >= ax->n_breaks) return (double)ax->breaks[ax->n_breaks - 1] + 1.0;
    return ((double)ax->breaks[bin - 1] + (double)ax->breaks[bin]) * 0.5;
}

int lab_resolve_axis_candidates(const cm_dist_record *rec, lab_axis_candidates *out) {
    memset(out, 0, sizeof *out);
    if (!rec || !cm_resolver_has_grid(rec)) { out->draw_axis = -1; out->n = 0; return 0; }
    int draw = cm_resolver_draw_axis_index(rec);
    out->draw_axis = draw;
    int nctx = cm_resolver_n_context(rec);
    out->n = nctx;
    int maxi = -1; double maxratio = 0;

    for (int ci = 0; ci < nctx; ci++) {
        int a = ctx_actual(rec, draw, ci);
        lab_axis_candidate *c = &out->items[ci];
        c->axis_index = a;
        const cm_axis *ax = &rec->axes[a];
        c->lo = ax->n_breaks ? ax->breaks[0] : 0;
        c->hi = ax->n_breaks ? ax->breaks[ax->n_breaks - 1] : 0;
        c->symmetric = (ax->n_breaks > 0 && ax->breaks[0] < 0 && ax->breaks[ax->n_breaks - 1] > 0);

        /* effect ratio: sweep this axis over its bins, others at mid, aggregate draw axis */
        double lo = 1e300, hi = -1e300;
        double ctxvals[LAB_MAX_AXES];
        for (int cj = 0; cj < nctx; cj++) {
            int aj = ctx_actual(rec, draw, cj);
            ctxvals[cj] = value_for_bin(rec, aj, rec->dims[aj] / 2);
        }
        for (int b = 0; b < rec->dims[a]; b++) {
            ctxvals[ci] = value_for_bin(rec, a, b);
            double mean, var; uint32_t n;
            cm_resolver_conditional_moments(rec, ctxvals, nctx, &mean, &var, &n);
            if (n < 100) continue;
            if (mean < lo) lo = mean;
            if (mean > hi) hi = mean;
        }
        c->effect_ratio = (lo > 0 && lo < 1e299) ? hi / lo : (hi > 0 ? 1e9 : 1.0);
        if (c->effect_ratio > maxratio) { maxratio = c->effect_ratio; maxi = ci; }
    }

    /* the categorical axis = the recovered player-position spine (non-uniform marginal) */
    int pos_axis = lab_identify_position_axis(rec);

    /* assign roles from the evidence (data-derived, NOT game variable names) */
    for (int ci = 0; ci < nctx; ci++) {
        lab_axis_candidate *c = &out->items[ci];
        if (c->axis_index == pos_axis) {
            c->role = "player-position spine b0 CDef..b6 Striker (INFERRED-strong)";
            c->candidate_source = "position on the 7-value *Value spine (RESOLVER_AXIS_MAPPING.md)";
            c->confidence = CONF_INFERRED;
            c->alternatives = "role/duty grouping vs raw position; either way def<->att categorical";
        } else if (c->symmetric && c->effect_ratio > 1.5) {
            c->role = "differential/quality-matchup (INFERRED)";
            c->candidate_source = "skill/ability vs opponent (HYPOTHESIS)";
            c->confidence = CONF_APPROXIMATE;
            c->alternatives = "could be score/strength differential; literal name UNKNOWN";
        } else if (ci == maxi && c->effect_ratio > 2.0) {
            c->role = "dominant continuous booster (INFERRED)";
            c->candidate_source = "quantile-binned player/match quantity (HYPOTHESIS)";
            c->confidence = CONF_APPROXIMATE;
            c->alternatives = "uniform marginal => continuous; literal name UNKNOWN";
        } else {
            c->role = "secondary continuous modifier (INFERRED-weak)";
            c->candidate_source = "quantile-binned quantity (HYPOTHESIS)";
            c->confidence = CONF_UNKNOWN;
            c->alternatives = "insufficient effect to characterise";
        }
    }
    return 1;
}

static int find_role_axis(const lab_axis_candidates *c, int want_symmetric) {
    int best = -1; double bestr = 0;
    for (int i = 0; i < c->n; i++) {
        if (want_symmetric && !c->items[i].symmetric) continue;
        if (!want_symmetric) { /* dominant = max effect */
            if (c->items[i].effect_ratio > bestr) { bestr = c->items[i].effect_ratio; best = i; }
        } else {
            if (c->items[i].effect_ratio > bestr) { bestr = c->items[i].effect_ratio; best = i; }
        }
    }
    return best;
}

int lab_build_context(const lab_runtime_state *s, const cm_dist_record *rec,
                      const char *hypothesis, lab_context *out) {
    memset(out, 0, sizeof *out);
    if (!rec || !cm_resolver_has_grid(rec)) return 0;
    lab_axis_candidates cand;
    if (!lab_resolve_axis_candidates(rec, &cand)) return 0;
    int draw = cand.draw_axis, nctx = cand.n;
    out->n = nctx;
    snprintf(out->hypothesis, sizeof out->hypothesis, "%s", hypothesis ? hypothesis : "neutral");

    /* start neutral: every context axis at its middle bin */
    for (int ci = 0; ci < nctx; ci++) {
        int a = ctx_actual(rec, draw, ci);
        out->values[ci] = value_for_bin(rec, a, rec->dims[a] / 2);
    }
    if (!hypothesis || strcmp(hypothesis, "neutral") == 0) return 1;

    /* context-order index of the recovered position axis (if any) */
    int pos_axis = lab_identify_position_axis(rec);
    int pos_ci = -1;
    for (int ci = 0; ci < nctx; ci++) if (ctx_actual(rec, draw, ci) == pos_axis) { pos_ci = ci; break; }
    int qual_ci = find_role_axis(&cand, 1);      /* symmetric/quality candidate */

    double quality = (s->player.skill - 1) / 19.0; if (quality < 0) quality = 0; if (quality > 1) quality = 1;

    /* "position"/"volume"/"combined" drive the position axis via the LITERAL spine mapping;
     * "involvement" drives the same axis via the older APPROXIMATE position+duty proxy (so duty
     * sensitivity is visible); "quality"/"combined" drive the differential axis via skill. */
    int do_pos  = (strcmp(hypothesis, "position") == 0 || strcmp(hypothesis, "volume") == 0 || strcmp(hypothesis, "combined") == 0);
    int do_invol = (strcmp(hypothesis, "involvement") == 0);
    int do_qual = (strcmp(hypothesis, "quality") == 0 || strcmp(hypothesis, "combined") == 0);

    if (do_invol && pos_ci >= 0) {
        double involvement = lab_involvement_proxy(s->player.position, s->player.duty);
        double lo = cand.items[pos_ci].lo, hi = cand.items[pos_ci].hi, e = (hi - lo) * 0.02 + 0.01;
        out->values[pos_ci] = (lo - e) + involvement * ((hi + e) - (lo - e));
    }
    if (do_pos && pos_ci >= 0) {
        int spine = lab_position_spine_bin(s->player.position);   /* 0..6 or -1 (GK) */
        int a = pos_axis, dim = rec->dims[a];
        int bin;
        if (spine < 0) bin = 0;                                   /* GK -> defence-most proxy */
        else if (dim == 7) bin = spine;                           /* literal 7-bin spine */
        else bin = (int)((double)spine / 6.0 * (dim - 1) + 0.5);  /* scale if collapsed */
        if (bin < 0) bin = 0; if (bin >= dim) bin = dim - 1;
        out->values[pos_ci] = value_for_bin(rec, a, bin);
    }
    if (do_qual && qual_ci >= 0 && qual_ci != pos_ci) {
        double lo = cand.items[qual_ci].lo, hi = cand.items[qual_ci].hi, e = (hi - lo) * 0.02 + 0.01;
        out->values[qual_ci] = (lo - e) + quality * ((hi + e) - (lo - e));
    }
    return 1;
}

int lab_cmd_player(int argc, char **argv) {
    (void)argc; (void)argv;
    lab_runtime_state s; lab_state_default(&s);
    printf("cm2008-lab runtime state template (defaults; all EXPERIMENTAL unless noted)\n\n");
    printf("PLAYER\n");
    printf("  %-14s %-8s %-14s %s\n", "field", "value", "provenance", "note");
    printf("  %-14s %-8d %-14s %s\n", "id", s.player.id, lab_prov_str(PROV_EXPERIMENTAL), "lab identifier");
    printf("  %-14s %-8d %-14s %s\n", "team_id", s.player.team_id, lab_prov_str(PROV_EXPERIMENTAL), "lab identifier");
    printf("  %-14s %-8s %-14s %s\n", "position", lab_position_str(s.player.position), lab_prov_str(PROV_EXPERIMENTAL), "lab code; CM2008 internal id UNKNOWN");
    printf("  %-14s %-8s %-14s %s\n", "duty", lab_duty_str(s.player.duty), lab_prov_str(PROV_EXPERIMENTAL), "lab code");
    printf("  %-14s %-8d %-14s %s\n", "skill", s.player.skill, lab_prov_str(PROV_EXPERIMENTAL), "1..20 aggregate ability");
    printf("  %-14s %-8d %-14s %s\n", "stamina", s.player.stamina, lab_prov_str(PROV_EXPERIMENTAL), "0..100");
    printf("  %-14s %-8d %-14s %s\n", "condition", s.player.condition, lab_prov_str(PROV_EXPERIMENTAL), "0..100");
    printf("  %-14s %-8d %-14s %s\n", "morale", s.player.morale, lab_prov_str(PROV_EXPERIMENTAL), "0..100");
    printf("\nMATCH\n");
    printf("  %-14s %-8d %-14s %s\n", "match_id", s.match.match_id, lab_prov_str(PROV_EXPERIMENTAL), "lab identifier");
    printf("  %-14s %-8d %-14s %s\n", "minute", s.match.minute, lab_prov_str(PROV_EXPERIMENTAL), "0..90");
    printf("  %-14s %-8d %-14s %s\n", "possession", s.match.possession, lab_prov_str(PROV_EXPERIMENTAL), "0..100 home");
    printf("\nUNKNOWN (not represented; literal CM2008 internals not recovered):\n");
    printf("  - which runtime quantity feeds each context axis (see PICKER_CALLER_CONTEXT.md)\n");
    printf("  - exact bin-boundary semantics; exact picker reduction\n");
    return 0;
}
