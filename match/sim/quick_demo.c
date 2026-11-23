/* Quick resolver demo: print CM's recovered population stats per metric + a
 * sample, from the user's own extracted .dat dir. Usage: cmquick <dir> [seed] */
#include "resolver.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){
    const char*dir = argc>1?argv[1]:".";
    uint32_t seed = argc>2?(uint32_t)strtoul(argv[2],0,10):777u;
    cm_quick_resolver R;
    int nf=cm_quick_load(&R,dir,seed);
    printf("loaded %d distribution files from %s\n",nf,dir);
    if(nf==0){ printf("no files found\n"); return 1; }
    struct { int sec; const char*m; } ms[] = {
        {0,"GoalsScored"},{0,"ShotsOnTarget"},{0,"ShotsOnGoal"},{0,"PassesCompleted"},
        {0,"TacklesWon"},{0,"Interceptions"},{0,"Fouls"},{0,"Headers"},
        {2,"Corners"},{2,"FreeKicks"},{2,"Penalties"},{2,"ThrowIns"},
    };
    printf("%-18s %-8s %-8s %-8s  sample\n","metric","mean","std","N");
    for(unsigned i=0;i<sizeof ms/sizeof ms[0];i++){
        double mean,var; uint32_t n;
        if(!cm_quick_moments(&R,ms[i].sec,ms[i].m,&mean,&var,&n)){ printf("%-18s (not found)\n",ms[i].m); continue; }
        float s=cm_quick_sample(&R,ms[i].sec,ms[i].m);
        printf("%-18s %-8.3f %-8.3f %-8u  %.2f\n",ms[i].m,mean,var>0?__builtin_sqrtf((float)var):0.0f,n,s);
    }
    cm_quick_free(&R);
    return 0;
}
