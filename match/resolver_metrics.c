/* Match Resolver metric vocabulary — reconstructed from CM2008.exe data files. */
#include "resolver_metrics.h"
#include <string.h>

const char *const CM_RESOLVER_METRIC_NAMES[CM_MR_METRIC_COUNT] = {
    "Assists", "ChallengesAvoided", "ChallengesMade", "Clearances",
    "CompletedPasses", "Condition", "Confidence", "Corners",
    "Crosses", "CrossesCompleted", "CrossesFumbled", "CrossesPunches",
    "CrossesTaken", "CynicalFouls", "DirtyFouls", "FailedToControl",
    "Fouls", "FoulsCommited", "FoulsReceived", "FreeKickDirect",
    "FreeKickIndirect", "FreeKicks", "GoalKicks", "GoalsConceded",
    "GoalsScored", "GroundCovered", "Headers", "HeadersKey",
    "HeadersWon", "Injured", "InjuryLevel", "Interceptions",
    "KeptPossession", "KeyActions", "LostPossession", "MajorInjuries",
    "MinorFouls", "MinorInjuries", "MissControlled", "Morale",
    "NumberRedCards", "NumberYellowCards", "Offsides", "OwnGoalsScored",
    "PassesCompleted", "PassesFailed", "PassesKey", "PassesReceived",
    "PenGoalsScored", "Penalties", "Performance", "PoorCrosses",
    "RunsMade", "RunsMadeKey", "ScaledPerformance", "ShotsFumbled",
    "ShotsHeld", "ShotsOffTarget", "ShotsOnGoal", "ShotsOnTarget",
    "ShotsParried", "ShotsTipped", "SubstitutionCount", "TacklesKey",
    "TacklesLost", "TacklesWon", "ThrowIns", "Touches",
    "Tricks", "Warnings",
};

const char *cm_resolver_metric_name(cm_resolver_metric m) {
    if (m < 0 || m >= CM_MR_METRIC_COUNT) return (const char *)0;
    return CM_RESOLVER_METRIC_NAMES[m];
}
int cm_resolver_metric_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_MR_METRIC_COUNT; i++)
        if (strcmp(name, CM_RESOLVER_METRIC_NAMES[i]) == 0) return i;
    return -1;
}
