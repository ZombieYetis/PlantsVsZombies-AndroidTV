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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_LANE_POLICY_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_LANE_POLICY_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIThreat.h"

namespace vsai::detail {

struct ZombieLanePolicy {
    bool deploymentBlocked = true;
    bool hasLiveTarget = false;
    bool mowerless = false;
    bool strongMowerlessPlantLane = false;
    bool conversionRoute = false;
    bool allowsAttack = false;
    bool allowsEconomy = false;
};

ZombieLanePolicy EvaluateZombieLanePolicy(const VSGameState &state, int row);
int MowerlessLaneAttackScoreBonus(const VSGameState &state, const ZombieLanePolicy &policy, int row, int zombieCount);
int MowerlessLaneDistributionAdjustment(const ZombieLanePolicy &policy, int zombieCount);
int MowerlessLaneCommitmentBonus(const VSGameState &state, const ZombieLanePolicy &policy, int row, int zombieCount);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_LANE_POLICY_H
