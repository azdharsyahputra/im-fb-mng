/*
 * Player attribute names + lookup — reconstructed from CM2008.exe.
 * Names Observed (symbol <Attr>Value/<Attr>Offset fields). The lookup helpers
 * are straightforward reconstruction over the recovered set.
 * See docs/findings/player-attributes.md.
 */
#include "attributes.h"
#include <string.h>

const char *const CM_ATTRIBUTE_NAMES[CM_AT_COUNT] = {
    /* Technical */
    "Crossing","Dribbling","Finishing","FirstTouch","Heading","LongShots",
    "Marking","Passing","PenaltyTaking","Tackling","DeadBalls","Crosses","ThrowIns",
    /* Mental */
    "Aggression","Anticipation","Bravery","Concentration","Creativity",
    "DecisionMaking","Determination","Flair","Leadership","Movement","Positioning",
    "Pressure","Professionalism","TeamWork","Vision","WorkRate","Consistency","Confidence",
    /* Physical */
    "Acceleration","Agility","Balance","Jumping","Pace","Stamina","Strength","Fitness",
    /* Goalkeeping */
    "Handling","Kicking","OneOnOnes","ShotStopping","Throwing","Roaming",
    /* Hidden / personality */
    "Adaptability","Ambition","Eccentricity","Loyalty","Sportsmanship","Versatility",
    /* Preferred foot */
    "LeftFoot","RightFoot"
};

const char *cm_attribute_name(cm_attribute a) {
    if (a < 0 || a >= CM_AT_COUNT) return (const char *)0;
    return CM_ATTRIBUTE_NAMES[a];
}

int cm_attribute_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_AT_COUNT; i++)
        if (strcmp(name, CM_ATTRIBUTE_NAMES[i]) == 0) return i;
    return -1;
}

/*
 * Effective in-match attribute value.
 * Observed model (see docs/findings): the match data carries, per attribute, a
 * base value and a situational "Offset". The exact combine function is NOT yet
 * decompiled; this is the documented first-order model (addition), flagged
 * inferred. Do not treat the combine as verified until the combine function is
 * reverse-engineered.
 */
int cm_attribute_effective(int base, int offset) {
    return base + offset; /* INFERRED — pending decompilation of the combine fn */
}
