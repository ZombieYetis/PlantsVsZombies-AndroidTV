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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_STATE_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_STATE_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/VSActionAIDecision.h"

#include <cstdint>

namespace vsai::detail {

bool IsDeadOrOutside(const VSPlantState &plant);
bool HasPlantAt(const VSGameState &state, VSGridPosition position);
bool HasPlantTypeAt(const VSGameState &state, SeedType seedType, VSGridPosition position);
bool HasGridItemAt(const VSGameState &state, VSGridPosition position);
const VSZombieState *FindClosestZombie(const VSGameState &state, int row = -1);
int CountPlantsInRow(const VSGameState &state, int row);
int CountZombiesInRow(const VSGameState &state, int row);
int CountActiveZombies(const VSGameState &state);
int CountActiveZombieRows(const VSGameState &state);
int CountPlantType(const VSGameState &state, SeedType seedType);
bool HasPlantTypeInRow(const VSGameState &state, SeedType seedType, int row);
bool HasActiveDeckCard(const VSGameState &state, VSSide side, SeedType seedType);
bool IsHeavyZombie(std::uint16_t zombieType);
bool IsFastZombie(std::uint16_t zombieType);
bool IsDecisiveCounterZombie(std::uint16_t zombieType);
bool HasZombieTypeInRow(const VSGameState &state, int row, ZombieType zombieType);
bool HasMindControlledZombieInRow(const VSGameState &state, int row);
bool IsMowerInMotion(const VSGameState &state, int row);
// This deliberately does not depend on mower state. Once a zombie reaches
// column zero, any additional zombie-side deployment in the lane is wasted.
bool HasZombieInHomeColumn(const VSGameState &state, int row);
bool IsMowerAboutToTrigger(const VSGameState &state, int row);
bool IsNutBypassZombieApproaching(const VSGameState &state, int row);
int CountLivePlants(const VSGameState &state);
int CountPlantIncome(const VSGameState &state);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_STATE_H
