#ifndef AI5_ISAKU_SWITCH_H
#define AI5_ISAKU_SWITCH_H
#include <stdbool.h>
#include "ai5/game.h"

/* Opt-in build only. AI5_ISAKU_SWITCH_TEST is reserved for host regressions;
 * ordinary desktop builds never enable the Switch implementation. */
#if defined(AI5_ISAKU_SWITCH_BUILD) && (defined(__SWITCH__) || defined(AI5_ISAKU_SWITCH_TEST))
#define ISAKU_SWITCH_PORT 1
#endif

static inline bool isaku_switch_active(void)
{
#ifdef ISAKU_SWITCH_PORT
    return ai5_target_game == GAME_ISAKU;
#else
    return false;
#endif
}
#endif
