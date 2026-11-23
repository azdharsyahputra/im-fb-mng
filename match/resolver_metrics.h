/*
 * Match Resolver performance metrics — reconstructed from CM2008.exe data
 * (MatchResolverDistributions.1..6.dat). These are the per-player match
 * statistics the quick-match statistical resolver produces by sampling named
 * probability distributions. Names are Observed (factual vocabulary extracted
 * from the distribution files). NO distribution data (the proprietary float
 * tables) is reproduced here. See docs/findings/match-resolver.md.
 */
#ifndef CM2008_MATCH_RESOLVER_METRICS_H
#define CM2008_MATCH_RESOLVER_METRICS_H

typedef enum cm_resolver_metric {
    CM_MR_ASSISTS,
    CM_MR_CHALLENGES_AVOIDED,
    CM_MR_CHALLENGES_MADE,
    CM_MR_CLEARANCES,
    CM_MR_COMPLETED_PASSES,
    CM_MR_CONDITION,
    CM_MR_CONFIDENCE,
    CM_MR_CORNERS,
    CM_MR_CROSSES,
    CM_MR_CROSSES_COMPLETED,
    CM_MR_CROSSES_FUMBLED,
    CM_MR_CROSSES_PUNCHES,
    CM_MR_CROSSES_TAKEN,
    CM_MR_CYNICAL_FOULS,
    CM_MR_DIRTY_FOULS,
    CM_MR_FAILED_TO_CONTROL,
    CM_MR_FOULS,
    CM_MR_FOULS_COMMITED,
    CM_MR_FOULS_RECEIVED,
    CM_MR_FREE_KICK_DIRECT,
    CM_MR_FREE_KICK_INDIRECT,
    CM_MR_FREE_KICKS,
    CM_MR_GOAL_KICKS,
    CM_MR_GOALS_CONCEDED,
    CM_MR_GOALS_SCORED,
    CM_MR_GROUND_COVERED,
    CM_MR_HEADERS,
    CM_MR_HEADERS_KEY,
    CM_MR_HEADERS_WON,
    CM_MR_INJURED,
    CM_MR_INJURY_LEVEL,
    CM_MR_INTERCEPTIONS,
    CM_MR_KEPT_POSSESSION,
    CM_MR_KEY_ACTIONS,
    CM_MR_LOST_POSSESSION,
    CM_MR_MAJOR_INJURIES,
    CM_MR_MINOR_FOULS,
    CM_MR_MINOR_INJURIES,
    CM_MR_MISS_CONTROLLED,
    CM_MR_MORALE,
    CM_MR_NUMBER_RED_CARDS,
    CM_MR_NUMBER_YELLOW_CARDS,
    CM_MR_OFFSIDES,
    CM_MR_OWN_GOALS_SCORED,
    CM_MR_PASSES_COMPLETED,
    CM_MR_PASSES_FAILED,
    CM_MR_PASSES_KEY,
    CM_MR_PASSES_RECEIVED,
    CM_MR_PEN_GOALS_SCORED,
    CM_MR_PENALTIES,
    CM_MR_PERFORMANCE,
    CM_MR_POOR_CROSSES,
    CM_MR_RUNS_MADE,
    CM_MR_RUNS_MADE_KEY,
    CM_MR_SCALED_PERFORMANCE,
    CM_MR_SHOTS_FUMBLED,
    CM_MR_SHOTS_HELD,
    CM_MR_SHOTS_OFF_TARGET,
    CM_MR_SHOTS_ON_GOAL,
    CM_MR_SHOTS_ON_TARGET,
    CM_MR_SHOTS_PARRIED,
    CM_MR_SHOTS_TIPPED,
    CM_MR_SUBSTITUTION_COUNT,
    CM_MR_TACKLES_KEY,
    CM_MR_TACKLES_LOST,
    CM_MR_TACKLES_WON,
    CM_MR_THROW_INS,
    CM_MR_TOUCHES,
    CM_MR_TRICKS,
    CM_MR_WARNINGS,
    CM_MR_METRIC_COUNT
} cm_resolver_metric;

extern const char *const CM_RESOLVER_METRIC_NAMES[CM_MR_METRIC_COUNT];
const char *cm_resolver_metric_name(cm_resolver_metric m);
int cm_resolver_metric_from_name(const char *name);  /* -1 if unknown */

#endif
