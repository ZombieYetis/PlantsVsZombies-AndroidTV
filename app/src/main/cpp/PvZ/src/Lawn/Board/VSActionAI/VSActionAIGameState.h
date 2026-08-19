#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_GAME_STATE_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_GAME_STATE_H

#include "PvZ/Lawn/VSActionSystem.h"

class Board;

namespace vsai::detail {

// Reads live Board state into the algorithm-facing value snapshot. The
// returned state has no ownership or mutation path back into Board.
VSGameState BuildGameStateSnapshot(Board *board);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_GAME_STATE_H
