#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_AI_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_AI_H

#include "ZombieAIPlanning.h"

namespace vsai::detail {

class ZombieAI final : public ZombieAIPlanning {
public:
    std::optional<VSAction> Decide(const VSGameState &state) override;
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_AI_H
