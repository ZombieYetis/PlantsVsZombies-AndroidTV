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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_PLACEMENT_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_PLACEMENT_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIThreat.h"

namespace vsai::detail {

VSGridPosition FindPlantCellInColumns(const VSGameState &state, int preferredRow, int firstColumn, int lastColumn);
VSGridPosition FindPlantCellInExactRow(const VSGameState &state, int row, int firstColumn, int lastColumn);
VSGridPosition FindPuffshroomCell(const VSGameState &state, int row);
bool IsPlantableVSTile(const VSGameState &state, VSGridPosition position);
bool IsPlantPlacementSafe(const VSGameState &state, SeedType seed, VSGridPosition position);
bool IsRangedOutputTradeUnfavorable(const VSGameState &state, int row);
VSGridPosition FindSafeIncomeCell(const VSGameState &state, int preferredRow);
VSGridPosition FindIcebergLettuceCell(const VSGameState &state, int row);
VSGridPosition FindWallnutCell(const VSGameState &state, int row, int firstColumn, int lastColumn);
VSGridPosition FindSpikeweedCell(const VSGameState &state, int row);
int ZombiePlacementColumn(SeedType seed);
VSGridPosition FindZombieCell(const VSGameState &state, SeedType seed, int row);
bool CanInvestZombieEconomyInRow(const VSGameState &state, int row);
VSGridPosition FindZombieEconomyCell(const VSGameState &state, int preferredRow);
VSGridPosition FindZombieMoundCell(const VSGameState &state, int row);
int MoundUpgradeCostAt(const VSGameState &state, VSGridPosition position);
int MoundUpgradePriorityAt(const VSGameState &state, VSGridPosition position);
bool IsReadyCard(const VSCardState &card, int resource);
bool IsCardReadyForZombieTarget(const VSCardState &card, const VSGameState &state, VSGridPosition target);
int CountZombieEconomy(const VSGameState &state);
int HeavyZombieEconomyThreshold(const VSGameState &state);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_PLACEMENT_H
