/*
 * Copyright (C) 2023-2026 PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV. If not, see <https://www.gnu.org/licenses/>.
 */

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
