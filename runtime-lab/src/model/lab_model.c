#include "lab_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *lab_model_default_dir(void) {
    const char *e = getenv("RESOLVER_DIR");
    if (e && *e) return e;
    return "/Users/csadeveloper/cm/extracted_game/DatabaseFiles";
}

int lab_model_load(LabModel *m, int id, const char *dir) {
    memset(m, 0, sizeof *m);
    if (id < 1 || id > LAB_N_MODELS) return 0;
    if (!dir) dir = lab_model_default_dir();
    snprintf(m->path, sizeof m->path, "%s/MatchResolverDistributions.%d.dat", dir, id);
    m->id = id;
    m->loaded = cm_resolver_load(&m->db, m->path);
    return m->loaded;
}

void lab_model_free(LabModel *m) {
    if (!m) return;
    if (m->loaded) cm_resolver_free(&m->db);
    m->loaded = 0;
}

int lab_model_validate(const LabModel *m, char *msg, int msglen) {
    if (!m || !m->loaded) { snprintf(msg, msglen, "model not loaded"); return 0; }
    const cm_resolver_db *db = &m->db;
    if (db->section[0].n_records <= 0) { snprintf(msg, msglen, "no section-0 records"); return 0; }
    int grid = 0, bad = 0;
    for (int s = 0; s < 3; s++) {
        for (int i = 0; i < db->section[s].n_records; i++) {
            const cm_dist_record *r = &db->section[s].records[i];
            if (!cm_resolver_has_grid(r)) continue; /* scalars/empty allowed */
            grid++;
            /* overall cell cum == sum of grid cells' cum */
            unsigned long long sum = 0;
            int cells = cm_resolver_grid_cells(r);
            for (int k = 0; k < cells; k++) sum += r->triples[k].cum;
            double mean, var; uint32_t n;
            cm_resolver_overall(r, &mean, &var, &n);
            if ((unsigned long long)n != sum) {
                bad++;
                snprintf(msg, msglen, "%s: overall N=%u != sum of cells %llu", r->name, n, sum);
                return 0;
            }
            if (r->n_triples != cells + 1) {
                bad++;
                snprintf(msg, msglen, "%s: n_triples %d != cells %d + 1", r->name, r->n_triples, cells);
                return 0;
            }
        }
    }
    snprintf(msg, msglen, "ok: %d/%d/%d records, %d grid records, overall-cell + cell-count invariants hold",
             db->section[0].n_records, db->section[1].n_records, db->section[2].n_records, grid);
    (void)bad;
    return 1;
}

void lab_model_inspect(const LabModel *m, int section, FILE *f) {
    if (!m || !m->loaded) { fprintf(f, "model %d not loaded\n", m ? m->id : -1); return; }
    if (section < 0 || section > 2) section = 0;
    const cm_dist_section *sec = &m->db.section[section];
    fprintf(f, "model %d  file=%s\n", m->id, m->path);
    fprintf(f, "sections: %d / %d / %d records; inspecting section %d (%d metrics)\n",
            m->db.section[0].n_records, m->db.section[1].n_records, m->db.section[2].n_records,
            section, sec->n_records);
    fprintf(f, "%-22s %-5s %-6s %-5s %-24s %-12s\n", "metric", "axes", "cells", "draw", "dims", "overall(mean/N)");
    for (int i = 0; i < sec->n_records; i++) {
        const cm_dist_record *r = &sec->records[i];
        char dims[128] = "[";
        for (int a = 0; a < r->n_axes; a++) {
            char t[16]; snprintf(t, sizeof t, "%d%s", r->dims[a], a + 1 < r->n_axes ? "," : "");
            strncat(dims, t, sizeof dims - strlen(dims) - 1);
        }
        strncat(dims, "]", sizeof dims - strlen(dims) - 1);
        double mean, var; uint32_t n;
        cm_resolver_overall(r, &mean, &var, &n);
        fprintf(f, "%-22s %-5d %-6d %-5d %-24s %.4f / %u\n",
                r->name, r->n_axes, cm_resolver_grid_cells(r), cm_resolver_draw_axis_index(r),
                dims, mean, n);
    }
}

static const cm_dist_record *find0(const LabModel *m, const char *name) {
    return cm_resolver_find(&m->db, 0, name);
}

int lab_model_compare(const LabModel *a, const LabModel *b, LabModelComparison *out, FILE *report) {
    memset(out, 0, sizeof *out);
    if (!a->loaded || !b->loaded) return 0;
    const cm_dist_section *sa = &a->db.section[0];
    if (report)
        fprintf(report, "compare model %d vs model %d (section 0)\n%-22s %-16s %-16s %-10s\n",
                a->id, b->id, "metric", "dims(A)", "dims(B)", "stats");
    for (int i = 0; i < sa->n_records; i++) {
        const cm_dist_record *ra = &sa->records[i];
        const cm_dist_record *rb = find0(b, ra->name);
        if (!rb) continue;
        out->shared_metrics++;
        /* dims comparison */
        int same_dims = (ra->n_axes == rb->n_axes);
        if (same_dims) for (int k = 0; k < ra->n_axes; k++) if (ra->dims[k] != rb->dims[k]) { same_dims = 0; break; }
        if (!same_dims) out->dims_differ++;
        /* sufficient-stats comparison (overall triple) */
        cm_triple ta = ra->n_triples ? ra->triples[ra->n_triples - 1] : (cm_triple){0,0,0};
        cm_triple tb = rb->n_triples ? rb->triples[rb->n_triples - 1] : (cm_triple){0,0,0};
        int stats_same = (ta.cum == tb.cum && ta.lo == tb.lo && ta.hi == tb.hi);
        if (stats_same) out->stats_identical++; else out->stats_differ++;
        if (report) {
            char da[32] = "[", db2[32] = "[";
            for (int k = 0; k < ra->n_axes; k++) { char t[12]; snprintf(t, sizeof t, "%d%s", ra->dims[k], k+1<ra->n_axes?",":""); strncat(da, t, sizeof da-strlen(da)-1);} strncat(da,"]",sizeof da-strlen(da)-1);
            for (int k = 0; k < rb->n_axes; k++) { char t[12]; snprintf(t, sizeof t, "%d%s", rb->dims[k], k+1<rb->n_axes?",":""); strncat(db2, t, sizeof db2-strlen(db2)-1);} strncat(db2,"]",sizeof db2-strlen(db2)-1);
            fprintf(report, "%-22s %-16s %-16s %s\n", ra->name, da, db2, stats_same ? "IDENTICAL" : "DIFFER");
        }
    }
    if (report)
        fprintf(report, "\nshared=%d  dims_differ=%d  stats_identical=%d  stats_differ=%d\n",
                out->shared_metrics, out->dims_differ, out->stats_identical, out->stats_differ);
    return 1;
}
