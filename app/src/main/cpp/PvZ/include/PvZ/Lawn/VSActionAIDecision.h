#ifndef PVZ_LAWN_VS_ACTION_AI_DECISION_H
#define PVZ_LAWN_VS_ACTION_AI_DECISION_H

#include "PvZ/Lawn/VSActionSystem.h"

#include <memory>

namespace vsai {

// Internal factory for the replay-informed local VS agents. The agents use
// only the snapshot API in VSActionSystem.h and cannot mutate Board directly.
std::unique_ptr<IVSAgent> CreateBuiltinVSAgent(VSSide side);

} // namespace vsai

#endif // PVZ_LAWN_VS_ACTION_AI_DECISION_H
