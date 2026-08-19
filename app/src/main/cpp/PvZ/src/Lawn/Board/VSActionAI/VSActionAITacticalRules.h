#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_TACTICAL_RULES_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_TACTICAL_RULES_H

#include "VSActionAIThreat.h"

namespace vsai::detail {

bool IsPlantOneShotSeed(SeedType seed);
bool IsPlantImmediateCounterSeed(SeedType seed);
bool CanPumpkinShellTarget(SeedType seed);
bool IsSquashTargetZombie(const VSZombieState &zombie);
bool CanChillyPepperAffect(const VSZombieState &zombie);
bool IsBungeeTargetEligible(const VSGameState &state, const VSPlantState &plant);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_TACTICAL_RULES_H
