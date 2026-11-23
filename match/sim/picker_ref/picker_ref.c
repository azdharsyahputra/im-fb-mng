/*
 * picker_ref.c — candidate reductions for the obfuscated resolver picker
 * FUN_005b5100.  Build/run standalone:
 *
 *     cc -std=c99 -O2 -Wall -Wextra -o /tmp/test_picker_ref \
 *        match/sim/picker_ref/picker_ref.c match/sim/picker_ref/test_picker_ref.c \
 *     && /tmp/test_picker_ref
 *
 * (or just `make -C match/sim/picker_ref test`.)
 *
 * EVERY algorithm below is a HYPOTHESIS. The picker's real reduction is UNKNOWN
 * (anti-disassembly). See the header and research/RUNTIME_PICKER_ALGORITHM.md.
 */
#include "picker_ref.h"
#include <stdlib.h> /* malloc/free for large tables */

const char *pr_candidate_name(pr_candidate cand) {
    switch (cand) {
        case PR_CAND_FLOAT_CDF: return "FLOAT_CDF (H-B, weighted inverse-CDF, float idiom)";
        case PR_CAND_MOD_CDF:   return "MOD_CDF (H-A, weighted inverse-CDF, modulo)";
        case PR_CAND_UNIFORM_N: return "UNIFORM_N (H-C, unweighted uniform pick)";
        case PR_CAND_ARGMAX:    return "ARGMAX (H-D, deterministic max weight, RNG tie-break)";
    }
    return "?";
}

/* Build a cumulative-sum view over the weights into caller-supplied cum[] (length
 * n). Returns the total. Handles both raw and already-cumulative inputs. Negative
 * weights are clamped to 0 (a weight table must be non-negative; the .dat cum[] is a
 * monotone non-negative count sequence). */
static double build_cumulative(const pr_weights *wt, double *cum) {
    double running = 0.0;
    for (int i = 0; i < wt->n; i++) {
        if (wt->cumulative) {
            double c = wt->w[i];
            if (c < running) c = running; /* enforce monotonicity defensively */
            cum[i] = c;
            running = c;
        } else {
            double w = wt->w[i];
            if (w < 0.0) w = 0.0;
            running += w;
            cum[i] = running;
        }
    }
    return running; /* == total weight */
}

/* Smallest 1-based index i such that cum[i-1] > target (strict), i.e. the first
 * bucket whose cumulative boundary exceeds the draw. This is the inverse-CDF step.
 * Buckets with zero width (cum unchanged) are skipped automatically because the
 * strict compare never lands inside them. */
static int cdf_lookup(const double *cum, int n, double target) {
    for (int i = 0; i < n; i++) {
        if (target < cum[i]) return i + 1; /* 1-based */
    }
    return n; /* fp edge: target == total rounds to the last bucket */
}

int pr_pick(const pr_weights *wt, uint32_t *state, pr_candidate cand) {
    if (wt == NULL || wt->n <= 0 || state == NULL) return 0; /* empty-group sentinel */

    const int n = wt->n;

    if (cand == PR_CAND_UNIFORM_N) {
        /* H-C: ignore weights entirely; one LCG step, modulo N, 1-based. */
        uint32_t r = pr_lcg_next(state);
        return (int)(r % (uint32_t)n) + 1;
    }

    /* Candidates that need the cumulative distribution. */
    double stackcum[64];
    double *cum = stackcum;
    double *heapcum = NULL;
    if (n > (int)(sizeof stackcum / sizeof stackcum[0])) {
        heapcum = (double *)malloc((size_t)n * sizeof(double));
        if (heapcum == NULL) return 0;
        cum = heapcum;
    }
    double total = build_cumulative(wt, cum);

    int result;
    if (total <= 0.0) {
        /* All-zero weights: no probability mass. Degenerate; still consume one LCG
         * step (so determinism/consumption tests see a uniform step) and fall back
         * to a uniform pick over N. Labelled behaviour, not recovered. */
        uint32_t r = pr_lcg_next(state);
        result = (int)(r % (uint32_t)n) + 1;
    } else if (cand == PR_CAND_FLOAT_CDF) {
        /* H-B: float-uniform idiom (VERIFIED to exist engine-wide) scaled to total.*/
        float u = pr_lcg_unit(state);          /* [0,1), one LCG step */
        double target = (double)u * total;
        result = cdf_lookup(cum, n, target);
    } else if (cand == PR_CAND_MOD_CDF) {
        /* H-A: integer reduction. Map the raw 32-bit state onto [0,total) by
         * modulo. Only well-defined when total is integral (cumulative counts),
         * which the .dat cum[] are; for non-integral totals this truncates. */
        uint32_t r = pr_lcg_next(state);       /* one LCG step */
        double target;
        if (total >= 1.0 && total < 4294967296.0) {
            target = (double)(r % (uint32_t)total);
        } else {
            /* total not representable as a clean modulus: fall back to a scaled
             * reduction over the full 32-bit range. */
            target = ((double)r / 4294967296.0) * total;
        }
        result = cdf_lookup(cum, n, target);
    } else { /* PR_CAND_ARGMAX, H-D */
        /* Deterministic: pick the max-weight bucket; draw one unit() ONLY to break
         * an exact tie between equal maxima (so RNG consumption is data-dependent
         * here — a distinguishing, testable property vs the CDF candidates). */
        /* reconstruct per-bucket weight from cumulative */
        int best = 0;
        double bestw = cum[0];
        int ties = 1;
        for (int i = 1; i < n; i++) {
            double wi = cum[i] - cum[i - 1];
            if (wi > bestw) { bestw = wi; best = i; ties = 1; }
            else if (wi == bestw) { ties++; }
        }
        if (ties > 1) {
            /* break the tie among all buckets equal to bestw */
            uint32_t r = pr_lcg_next(state);
            int k = (int)(r % (uint32_t)ties);
            int seen = 0;
            for (int i = 0; i < n; i++) {
                double wi = (i == 0) ? cum[0] : cum[i] - cum[i - 1];
                if (wi == bestw) {
                    if (seen == k) { best = i; break; }
                    seen++;
                }
            }
        }
        result = best + 1; /* 1-based */
    }

    if (heapcum) free(heapcum);
    return result;
}

int pr_pick_raw(const double *w, int n, uint32_t *state, pr_candidate cand) {
    pr_weights wt;
    wt.w = w;
    wt.n = n;
    wt.cumulative = 0;
    return pr_pick(&wt, state, cand);
}
