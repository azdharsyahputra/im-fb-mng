/* Quick statistical match resolver — pooled moment sampling. Clean-room. */
#include "resolver.h"
#include "resolver_eval.h"   /* cm_lcg_unit */
#include <stdio.h>
#include <string.h>
#include <math.h>

int cm_quick_load(cm_quick_resolver *R, const char *dir, uint32_t seed){
    memset(R,0,sizeof *R); R->rng = seed? seed : 1;
    for(int i=1;i<=CM_QR_MAX_FILES;i++){
        char path[1024];
        snprintf(path,sizeof path,"%s/MatchResolverDistributions.%d.dat",dir,i);
        if(cm_resolver_load(&R->files[R->n_files], path)) R->n_files++;
    }
    return R->n_files;
}

void cm_quick_free(cm_quick_resolver *R){
    for(int i=0;i<R->n_files;i++) cm_resolver_free(&R->files[i]);
    R->n_files=0;
}

/* pool (N, sumX, sumX2) over every file and every group's total triple */
int cm_quick_moments(const cm_quick_resolver *R, int section, const char *metric,
                     double *mean, double *var, uint32_t *n){
    double N=0, SX=0, SX2=0;
    for(int f=0; f<R->n_files; f++){
        const cm_dist_record *rec = cm_resolver_find(&R->files[f], section, metric);
        if(!rec) continue;
        for(int g=0; g<rec->n_groups; g++){
            cm_cdf_group G = rec->groups[g];
            if(G.n<=0) continue;
            cm_triple t = rec->triples[G.start+G.n-1];  /* group totals */
            N += (double)t.cum; SX += (double)t.lo; SX2 += (double)t.hi;
        }
    }
    if(N<=0) return 0;
    double m = SX/N, v = SX2/N - m*m; if(v<0) v=0;
    if(mean)*mean=m; if(var)*var=v; if(n)*n=(uint32_t)N;
    return 1;
}

float cm_quick_sample(cm_quick_resolver *R, int section, const char *metric){
    double m,v; uint32_t n;
    if(!cm_quick_moments(R,section,metric,&m,&v,&n)) return 0.0f;
    float u1=cm_lcg_unit(&R->rng), u2=cm_lcg_unit(&R->rng);
    if(u1<1e-7f) u1=1e-7f;
    double z = sqrt(-2.0*log(u1))*cos(6.28318530718*u2);
    double val = m + z*sqrt(v);
    if(val<0) val=0;
    return (float)val;
}

float cm_quick_sample_empirical(cm_quick_resolver *R, int section, const char *metric){
    if(R->n_files<=0) return 0.0f;
    /* pick a context file (uniform) — stands in for the obfuscated context index */
    int f = (int)(cm_lcg_unit(&R->rng) * (float)R->n_files);
    if(f<0) f=0; if(f>=R->n_files) f=R->n_files-1;
    const cm_dist_record *rec = cm_resolver_find(&R->files[f], section, metric);
    if(!rec){ for(int i=0;i<R->n_files && !rec;i++) rec=cm_resolver_find(&R->files[i],section,metric); }
    if(!rec) return 0.0f;
    return cm_resolver_sample_empirical(rec, &R->rng);
}

float cm_quick_sample_ctx(cm_quick_resolver *R, int section, const char *metric, int ctx_hint){
    if(R->n_files<=0) return 0.0f;
    int f = ctx_hint; if(f<0) f=0; if(f>=R->n_files) f=R->n_files-1;  /* deterministic, input-driven */
    const cm_dist_record *rec = cm_resolver_find(&R->files[f], section, metric);
    if(!rec){ for(int i=0;i<R->n_files && !rec;i++) rec=cm_resolver_find(&R->files[i],section,metric); }
    if(!rec) return 0.0f;
    return cm_resolver_sample_empirical(rec, &R->rng);
}
