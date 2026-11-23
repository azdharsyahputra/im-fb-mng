#include "match_state.h"
static const char *const NM[CM_GS_COUNT]={
 "KickoffWaiting","InPlay","HalfActivated","HalfOver","SecondHalf","GameOver",
 "Goal","Foul","Offside","Corner","ThrowIn","Penalty","Substitution"};
const char *cm_game_state_name(cm_game_state s){ return (s<0||s>=CM_GS_COUNT)?0:NM[s]; }
