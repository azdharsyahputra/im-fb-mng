/* Demo: run a full clean-room match and print the result + stats.
 * Build:  make demo   ->   ./match/sim/cmdemo [seed]
 */
#include "sim.h"
#include <stdio.h>
#include <stdlib.h>

/* A simple team template: base skill with per-role tweaks. Clean-room sample
 * data (NOT from the game) just so the engine has something to simulate. */
static void make_team(cm_player_attr out[CM_PLAYERS_PER_TEAM], int base, uint64_t *s){
    for(int r=0;r<CM_PLAYERS_PER_TEAM;r++){
        cm_player_attr a;
        int b = base;
        /* tiny deterministic jitter */
        *s ^= *s<<13; *s^=*s>>7; *s^=*s<<17;
        int j = (int)((*s>>40)%5) - 2;
        int v = b + j; if(v<1)v=1; if(v>20)v=20;
        a.pace=v; a.acceleration=v; a.stamina=14; a.strength=v; a.agility=v;
        a.marking=v; a.tackling=v; a.positioning=v; a.anticipation=v; a.decisions=v;
        a.passing=v; a.dribbling=v; a.finishing=v; a.technique=v; a.first_touch=v;
        a.off_the_ball=v; a.work_rate=14; a.teamwork=v; a.bravery=v; a.composure=v;
        a.handling=1; a.reflexes=1;
        if(r==CM_ROLE_GK){ a.handling=v; a.reflexes=v; a.finishing=2; }
        if(r==CM_ROLE_SR||r==CM_ROLE_SL){ a.finishing=v+2>20?20:v+2; a.off_the_ball=v+2>20?20:v+2; }
        out[r]=a;
    }
}

int main(int argc, char **argv){
    uint64_t seed = argc>1 ? strtoull(argv[1],0,10) : 12345ULL;
    cm_player_attr home[CM_PLAYERS_PER_TEAM], away[CM_PLAYERS_PER_TEAM];
    uint64_t s1=seed^0xABCD, s2=seed^0x1234;
    make_team(home, 15, &s1);   /* slightly stronger home side */
    make_team(away, 13, &s2);

    cm_tactics th = { .mentality=12, .closing_down=12, .marking=12, .passing=8, .tempo=12 };
    cm_tactics ta = { .mentality=9,  .closing_down=10, .marking=14, .passing=10,.tempo=10 };

    cm_match m;
    cm_match_init(&m, seed, home, away, th, ta);
    cm_match_kickoff(&m, 0);
    cm_match_run(&m, 90*60);   /* 90 minutes @ ~60 ticks/min */

    /* aggregate team stats */
    int tshots[2]={0,0}, tsot[2]={0,0}, tpass[2]={0,0}, tpok[2]={0,0}, tposs[2]={0,0};
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        int t=m.players[i].team;
        tshots[t]+=m.stats[i].shots; tsot[t]+=m.stats[i].shots_on_target;
        tpass[t]+=m.stats[i].passes; tpok[t]+=m.stats[i].passes_ok;
        tposs[t]+=m.stats[i].possession_ticks;
    }
    int totposs = tposs[0]+tposs[1]; if(totposs==0) totposs=1;
    printf("=== CM-style match (clean-room engine), seed=%llu ===\n", (unsigned long long)seed);
    printf("FULL TIME:  HOME %d - %d AWAY\n", m.score[0], m.score[1]);
    printf("Possession: %d%% - %d%%\n", 100*tposs[0]/totposs, 100*tposs[1]/totposs);
    printf("Shots (on target): HOME %d (%d)  -  AWAY %d (%d)\n",
           tshots[0], tsot[0], tshots[1], tsot[1]);
    printf("Passes (completed): HOME %d (%d)  -  AWAY %d (%d)\n",
           tpass[0], tpok[0], tpass[1], tpok[1]);
    printf("Minutes simulated: %d\n", m.minute);
    return 0;
}
