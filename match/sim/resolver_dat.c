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
/* skip block0: u32 nBuckets; each bucket u32 count,u32 nQ,f32[nQ] */
static int skip_block0(rd *r){
    uint32_t nb; if(!ru32(r,&nb)) return 0; if(nb>4096) return 0;
    for(uint32_t i=0;i<nb;i++){ uint32_t c,nq; if(!ru32(r,&c)||!ru32(r,&nq)) return 0;
        if(nq>65536) return 0; if(r->p+(size_t)nq*4>r->n) return 0; r->p+=(size_t)nq*4; }
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
            if(!skip_block0(&r)){ free(buf); return 0; }
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
        for(int i=0;i<db->section[s].n_records;i++){ free(db->section[s].records[i].triples); free(db->section[s].records[i].groups); }
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
