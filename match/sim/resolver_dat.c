/* MatchResolverDistributions (.dat) parser + inverse-CDF sampler.
 * Grammar is the validated one from research/resolver-format.md. Reads the
 * caller's own file; embeds no game data. */
#include "resolver_dat.h"
#include "resolver_eval.h"   /* cm_lcg_next, cm_lcg_unit */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct { const uint8_t *b; size_t n, p; } rd;
static int ru32(rd *r, uint32_t *o){ if(r->p+4>r->n) return 0;
    const uint8_t*q=r->b+r->p; *o=(uint32_t)q[0]|((uint32_t)q[1]<<8)|((uint32_t)q[2]<<16)|((uint32_t)q[3]<<24);
    r->p+=4; return 1; }
static int rf32(rd *r, float *o){ uint32_t u; if(!ru32(r,&u)) return 0; memcpy(o,&u,4); return 1; }

/* peek: do the bytes at p look like a record name header (u32 len in 3..47 then
 * that many ASCII letters)? */
static int looks_name(const rd *r, size_t p){
    if(p+4>r->n) return 0;
    const uint8_t*q=r->b+p;
    uint32_t n=(uint32_t)q[0]|((uint32_t)q[1]<<8)|((uint32_t)q[2]<<16)|((uint32_t)q[3]<<24);
    if(n<3||n>47||p+4+n>r->n) return 0;
    for(uint32_t i=0;i<n;i++){ uint8_t c=q[4+i]; if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z'))) return 0; }
    return 1;
}
/* read block0 into the record's context axes: u32 nAxes; each axis u32 binCount,
 * u32 nBreaks, f32[nBreaks]. (Earlier this was skipped as padding; it is actually
 * the context lookup table.) */
static int read_block0(rd *r, cm_dist_record *rec){
    uint32_t nb; if(!ru32(r,&nb)) return 0; if(nb>4096) return 0;
    if(nb>0){ rec->axes=calloc(nb,sizeof(cm_axis)); rec->dims=calloc(nb,sizeof(int)); }
    rec->n_axes=(int)nb;
    for(uint32_t i=0;i<nb;i++){ uint32_t c,nq; if(!ru32(r,&c)||!ru32(r,&nq)) return 0;
        (void)c;
        if(nq>65536) return 0; if(r->p+(size_t)nq*4>r->n) return 0;
        cm_axis *ax=&rec->axes[i]; ax->n_breaks=(int)nq;
        ax->breaks = nq? malloc(sizeof(float)*nq) : NULL;
        for(uint32_t q=0;q<nq;q++){ float v; if(!rf32(r,&v)) return 0; ax->breaks[q]=v; }
        rec->dims[i]=(int)nq+1;
    }
    return 1;
}
/* read triples into rec; stop rule depends on know_total */
static int read_triples(rd *r, cm_dist_record *rec, int know_total, uint32_t total){
    int cap=64; rec->triples=malloc(sizeof(cm_triple)*cap); rec->n_triples=0;
    for(;;){
        if(rec->n_triples>=cap){ cap*=2; rec->triples=realloc(rec->triples,sizeof(cm_triple)*cap); }
        if(!know_total && looks_name(r,r->p)) break;         /* next record (record 0 calibration) */
        if(r->p+12>r->n) break;                              /* EOF */
        cm_triple t; if(!ru32(r,&t.cum)||!rf32(r,&t.lo)||!rf32(r,&t.hi)) break;
        rec->triples[rec->n_triples++]=t;
        if(know_total && t.cum==total) break;                /* terminator */
    }
    rec->total = rec->n_triples? rec->triples[rec->n_triples-1].cum : 0;
    return 1;
}

/* split triples into self-contained CDF groups (cum resets between groups) */
static void build_groups(cm_dist_record *rec){
    rec->groups=NULL; rec->n_groups=0;
    if(rec->n_triples<=0) return;
    int cap=8; rec->groups=malloc(sizeof(cm_cdf_group)*cap);
    uint32_t prev=0; int start=0;
    for(int i=0;i<rec->n_triples;i++){
        uint32_t c=rec->triples[i].cum;
        if(c<prev){ /* close previous group at i-1 */
            if(rec->n_groups>=cap){cap*=2;rec->groups=realloc(rec->groups,sizeof(cm_cdf_group)*cap);}
            rec->groups[rec->n_groups++] = (cm_cdf_group){start,i-start,rec->triples[i-1].cum};
            start=i;
        }
        prev=c;
    }
    if(rec->n_groups>=cap){cap*=2;rec->groups=realloc(rec->groups,sizeof(cm_cdf_group)*cap);}
    rec->groups[rec->n_groups++] = (cm_cdf_group){start,rec->n_triples-start,rec->triples[rec->n_triples-1].cum};
}

int cm_resolver_load(cm_resolver_db *db, const char *path){
    memset(db,0,sizeof *db);
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    if(sz<=0){ fclose(f); return 0; }
    uint8_t *buf=malloc((size_t)sz);
    if(fread(buf,1,(size_t)sz,f)!=(size_t)sz){ fclose(f); free(buf); return 0; }
    fclose(f);
    rd r={buf,(size_t)sz,0};
    for(int s=0;s<3;s++){
        uint32_t cnt; if(!ru32(&r,&cnt)||cnt>100000){ free(buf); return 0; }
        cm_dist_section *sec=&db->section[s];
        sec->n_records=(int)cnt; sec->records=calloc(cnt?cnt:1,sizeof(cm_dist_record));
        uint32_t sec_total=0;
        for(uint32_t i=0;i<cnt;i++){
            cm_dist_record *rec=&sec->records[i];
            uint32_t nl; if(!ru32(&r,&nl)||nl>=sizeof(rec->name)||r.p+nl>r.n){ free(buf); return 0; }
            memcpy(rec->name,r.b+r.p,nl); rec->name[nl]=0; r.p+=nl;
            if(!read_block0(&r,rec)){ free(buf); return 0; }
            if(i==0){ read_triples(&r,rec,0,0); sec_total=rec->total; }
            else    { read_triples(&r,rec,1,sec_total); }
            build_groups(rec);
        }
    }
    free(buf);
    db->ok = (db->section[0].n_records>0);
    return db->ok;
}

void cm_resolver_free(cm_resolver_db *db){
    for(int s=0;s<3;s++){
        for(int i=0;i<db->section[s].n_records;i++){
            cm_dist_record *rec=&db->section[s].records[i];
            free(rec->triples); free(rec->groups);
            for(int a=0;a<rec->n_axes;a++) free(rec->axes[a].breaks);
            free(rec->axes); free(rec->dims);
        }
        free(db->section[s].records);
    }
    memset(db,0,sizeof *db);
}

const cm_dist_record *cm_resolver_find(const cm_resolver_db *db,int section,const char *name){
    if(section<0||section>2) return NULL;
    const cm_dist_section *sec=&db->section[section];
    for(int i=0;i<sec->n_records;i++) if(strcmp(sec->records[i].name,name)==0) return &sec->records[i];
    return NULL;
}

/* Moment-based sampling (recovered semantics): a group's last triple holds the
 * cumulative moments (count N, sumX, sumX^2) of that context's population, so
 * mean = sumX/N and var = sumX^2/N - mean^2. We draw a value from a distribution
 * with that mean/variance using the recovered LCG (Gaussian via Box-Muller,
 * clamped >= 0). The MOMENTS are VERIFIED (reconstruct to realistic football
 * stats for every metric); the choice of a Gaussian-from-moments draw is the
 * grounded MODELLING approximation (CM's exact per-value-level shape uses the
 * intermediate triples; not yet reproduced). */
void cm_resolver_group_moments(const cm_dist_record *rec, int group,
                               double *out_mean, double *out_var, uint32_t *out_n){
    *out_mean=0; *out_var=0; *out_n=0;
    if(!rec||rec->n_groups<=0) return;
    if(group<0) group=0; if(group>=rec->n_groups) group=rec->n_groups-1;
    cm_cdf_group g=rec->groups[group];
    if(g.n<=0) return;
    cm_triple t=rec->triples[g.start+g.n-1];   /* cumulative totals for the group */
    if(t.cum==0) return;
    double mean=(double)t.lo/(double)t.cum;
    double var=(double)t.hi/(double)t.cum - mean*mean;
    if(var<0) var=0;
    *out_mean=mean; *out_var=var; *out_n=t.cum;
}

float cm_resolver_sample_group(const cm_dist_record *rec, int group, uint32_t *lcg){
    double mean,var; uint32_t n;
    cm_resolver_group_moments(rec,group,&mean,&var,&n);
    if(n==0) return 0.0f;
    /* Box-Muller using the recovered LCG */
    float u1=cm_lcg_unit(lcg), u2=cm_lcg_unit(lcg);
    if(u1<1e-7f) u1=1e-7f;
    double z=sqrt(-2.0*log(u1))*cos(6.28318530718*u2);
    double v=mean + z*sqrt(var);
    if(v<0) v=0;
    return (float)v;
}

float cm_resolver_sample(const cm_dist_record *rec, uint32_t *lcg){
    return cm_resolver_sample_group(rec, 0, lcg);
}

/* GROUNDED empirical sampler: the picker (FUN_005b5100, interface-verified) does a
 * weighted inverse-CDF bucket draw. Each triple is a bucket (N, sumX, sumX2);
 * draw a bucket with weight N, return its mean (sumX/N) plus intra-bucket noise
 * from its own variance (via recovered LCG). Reproduces the real empirical
 * distribution SHAPE (not just the first two moments). */
float cm_resolver_sample_empirical(const cm_dist_record *rec, uint32_t *lcg){
    if(!rec||rec->n_triples<=0) return 0.0f;
    double tot=0; for(int i=0;i<rec->n_triples;i++) tot+=(double)rec->triples[i].cum;
    if(tot<=0) return 0.0f;
    double u=(double)cm_lcg_unit(lcg)*tot, acc=0; int k=rec->n_triples-1;
    for(int i=0;i<rec->n_triples;i++){ acc+=(double)rec->triples[i].cum; if(u<acc){k=i;break;} }
    cm_triple t=rec->triples[k];
    if(t.cum==0) return 0.0f;
    double mean=(double)t.lo/(double)t.cum, var=(double)t.hi/(double)t.cum-mean*mean;
    if(var<0) var=0;
    double val=mean;
    if(var>0){
        float u1=cm_lcg_unit(lcg), u2=cm_lcg_unit(lcg); if(u1<1e-7f)u1=1e-7f;
        val += sqrt(-2.0*log(u1))*cos(6.28318530718*u2)*sqrt(var);
    }
    if(val<0) val=0;
    return (float)val;
}

/* ---- Context grid API (recovered; see research/RESOLVER_CONTEXT_GRID.md) ---------- */

static const float k_draw_breaks[8] = {
    -20.904322f,-10.911437f,-5.036873f,-1.8810841f,1.8810754f,5.036864f,10.911417f,20.904284f
};

int cm_resolver_grid_cells(const cm_dist_record *rec){
    if(!rec||rec->n_axes<=0) return 0;
    int n=1; for(int a=0;a<rec->n_axes;a++) n*=rec->dims[a];
    return n;
}
int cm_resolver_has_grid(const cm_dist_record *rec){
    return rec && rec->n_axes>0 && rec->n_triples==cm_resolver_grid_cells(rec)+1;
}
static void moments_of(cm_triple t,double *mean,double *var,uint32_t *n){
    *mean=0;*var=0;*n=0;
    if(t.cum==0) return;
    double m=(double)t.lo/(double)t.cum, v=(double)t.hi/(double)t.cum-m*m;
    if(v<0)v=0; *mean=m;*var=v;*n=t.cum;
}
void cm_resolver_overall(const cm_dist_record *rec,double *mean,double *var,uint32_t *n){
    *mean=0;*var=0;*n=0;
    if(!rec||rec->n_triples<=0) return;
    moments_of(rec->triples[rec->n_triples-1],mean,var,n);
}
int cm_resolver_bin(const cm_dist_record *rec,int axis,double v){
    if(!rec||axis<0||axis>=rec->n_axes) return 0;
    const cm_axis *ax=&rec->axes[axis]; int i=0;
    while(i<ax->n_breaks && v>=(double)ax->breaks[i]) i++;
    return i;
}
int cm_resolver_cell_index(const cm_dist_record *rec,const int *coords,int n_coords){
    if(!rec||n_coords!=rec->n_axes) return -1;
    int idx=0;
    for(int a=0;a<rec->n_axes;a++){
        int c=coords[a]; if(c<0||c>=rec->dims[a]) return -1;
        idx=idx*rec->dims[a]+c;
    }
    if(idx<0||idx>=cm_resolver_grid_cells(rec)) return -1;
    return idx;
}
int cm_resolver_draw_axis_index(const cm_dist_record *rec){
    if(!rec) return -1;
    for(int a=0;a<rec->n_axes;a++){
        if(rec->axes[a].n_breaks!=8) continue;
        int match=1;
        for(int i=0;i<8;i++) if(rec->axes[a].breaks[i]!=k_draw_breaks[i]){match=0;break;}
        if(match) return a;
    }
    return -1;
}
int cm_resolver_n_context(const cm_dist_record *rec){
    if(!rec||rec->n_axes<=0) return 0;
    return rec->n_axes - (cm_resolver_draw_axis_index(rec)>=0 ? 1 : 0);
}
void cm_resolver_conditional_moments(const cm_dist_record *rec,
                                     const double *context_values,int n_values,
                                     double *mean,double *var,uint32_t *n){
    *mean=0;*var=0;*n=0;
    if(!cm_resolver_has_grid(rec)){ cm_resolver_overall(rec,mean,var,n); return; }
    int draw=cm_resolver_draw_axis_index(rec);
    if(draw<0 || n_values!=cm_resolver_n_context(rec)){ cm_resolver_overall(rec,mean,var,n); return; }
    int *coords=calloc(rec->n_axes,sizeof(int));
    /* map context values onto non-draw axes in order */
    int ci=0;
    for(int a=0;a<rec->n_axes;a++){ if(a==draw) continue; coords[a]=cm_resolver_bin(rec,a,context_values[ci++]); }
    uint64_t N=0; double sx=0,sx2=0;
    for(int b=0;b<rec->dims[draw];b++){
        coords[draw]=b;
        int idx=cm_resolver_cell_index(rec,coords,rec->n_axes);
        if(idx<0) continue;
        cm_triple t=rec->triples[idx];
        N+=t.cum; sx+=t.lo; sx2+=t.hi;
    }
    free(coords);
    if(N==0){ cm_resolver_overall(rec,mean,var,n); return; }
    double m=sx/(double)N, v=sx2/(double)N-m*m; if(v<0)v=0;
    *mean=m;*var=v;*n=(uint32_t)N;
}
float cm_resolver_sample_conditional(const cm_dist_record *rec,
                                     const double *context_values,int n_values,
                                     uint32_t *lcg){
    double mean,var; uint32_t n;
    cm_resolver_conditional_moments(rec,context_values,n_values,&mean,&var,&n);
    if(n==0) return 0.0f;
    double val=mean;
    if(var>0){
        float u1=cm_lcg_unit(lcg),u2=cm_lcg_unit(lcg); if(u1<1e-7f)u1=1e-7f;
        val += sqrt(-2.0*log(u1))*cos(6.28318530718*u2)*sqrt(var);
    }
    if(val<0)val=0;
    return (float)val;
}
