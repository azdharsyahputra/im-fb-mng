/* MatchResolverDistributions (.dat) parser + inverse-CDF sampler.
 * Grammar is the validated one from research/resolver-format.md. Reads the
 * caller's own file; embeds no game data. */
#include "resolver_dat.h"
#include "resolver_eval.h"   /* cm_lcg_next */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        }
    }
    free(buf);
    db->ok = (db->section[0].n_records>0);
    return db->ok;
}

void cm_resolver_free(cm_resolver_db *db){
    for(int s=0;s<3;s++){
        for(int i=0;i<db->section[s].n_records;i++) free(db->section[s].records[i].triples);
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

/* inverse-CDF sample: u in [0,total); first triple with cum>=u; interpolate lo..hi */
float cm_resolver_sample(const cm_dist_record *rec, uint32_t *lcg){
    if(!rec||rec->n_triples<=0||rec->total==0) return 0.0f;
    uint32_t u = cm_lcg_next(lcg) % rec->total;
    uint32_t prev=0;
    for(int i=0;i<rec->n_triples;i++){
        uint32_t c=rec->triples[i].cum;
        if(c<prev) prev=0;                         /* new CDF group: reset base */
        if(u < c){
            uint32_t span = c-prev;
            float frac = span? (float)(u-prev)/(float)span : 0.0f;
            return rec->triples[i].lo + frac*(rec->triples[i].hi - rec->triples[i].lo);
        }
        prev=c;
    }
    return rec->triples[rec->n_triples-1].hi;
}
