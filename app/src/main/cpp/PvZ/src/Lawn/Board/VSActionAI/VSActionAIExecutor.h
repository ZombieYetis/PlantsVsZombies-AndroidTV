#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_EXECUTOR_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_EXECUTOR_H

#include "PvZ/Lawn/VSActionSystem.h"

class Board;

namespace vsai::detail {

struct VSActionExecutionContext {
    bool replayExecution = false;
    bool localVSMatch = false;
    bool matchPlaying = false;
    bool matchPaused = false;
};

VSActionResult ExecuteBoardAction(Board *board, const VSAction &action, VSActionExecutionContext context);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_EXECUTOR_H
