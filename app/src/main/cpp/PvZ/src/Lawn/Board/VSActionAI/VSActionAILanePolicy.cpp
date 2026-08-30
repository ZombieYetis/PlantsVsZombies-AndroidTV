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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAILanePolicy.h"

#include <algorithm>

namespace vsai::detail {

ZombieLanePolicy EvaluateZombieLanePolicy(const VSGameState &state, int row) {
    ZombieLanePolicy policy{};
    if (row < 0 || row >= state.rows) {
        return policy;
    }

    policy.deploymentBlocked = IsMowerInMotion(state, row) || HasZombieInHomeColumn(state, row) || IsMowerAboutToTrigger(state, row);
    policy.hasLiveTarget = HasLiveZombieTargetInRow(state, row);
    policy.mowerless = row < static_cast<int>(state.mowerAvailable.size()) && !state.mowerAvailable[static_cast<std::size_t>(row)] && !IsMowerInMotion(state, row);
    policy.strongMowerlessPlantLane = IsMowerlessStrongPlantLane(state, row);
    policy.conversionRoute = !policy.deploymentBlocked && policy.mowerless && policy.hasLiveTarget;
    policy.allowsAttack = !policy.deploymentBlocked && policy.hasLiveTarget && (!policy.strongMowerlessPlantLane || policy.conversionRoute);
    policy.allowsEconomy = !policy.deploymentBlocked && policy.hasLiveTarget && (!policy.strongMowerlessPlantLane || AllMowersSpent(state));
    return policy;
}

int MowerlessLaneAttackScoreBonus(const VSGameState &state, const ZombieLanePolicy &policy, int row, int zombieCount) {
    if (!policy.conversionRoute) {
        return 0;
    }
    const bool hasAttackPlant =
        std::any_of(state.plants.begin(), state.plants.end(), [row](const VSPlantState &plant) { return !IsDeadOrOutside(plant) && plant.position.row == row && IsPlantCombatSeed(plant.seedType); });
    if (!hasAttackPlant) {
        // A live target behind a spent mower and a row containing only
        // Sunflowers/support has no practical recovery. This must beat the
        // normal lane cooldown, while the separate Ash stack rules still
        // prevent feeding several ordinary bodies into one blast cell.
        return zombieCount == 0 ? 2860 : 2360;
    }
    return (zombieCount > 0 ? 1420 : 1280) + (AllMowersSpent(state) ? 240 : 0);
}

int MowerlessLaneDistributionAdjustment(const ZombieLanePolicy &policy, int zombieCount) {
    if (!policy.conversionRoute) {
        return 0;
    }
    if (zombieCount == 0) {
        return 620;
    }
    if (zombieCount == 1) {
        return 180;
    }
    return -15 - (zombieCount - 1) * 45;
}

int MowerlessLaneCommitmentBonus(const VSGameState &state, const ZombieLanePolicy &policy, int row, int zombieCount) {
    if (!policy.conversionRoute) {
        return 0;
    }
    const bool hasAttackPlant =
        std::any_of(state.plants.begin(), state.plants.end(), [row](const VSPlantState &plant) { return !IsDeadOrOutside(plant) && plant.position.row == row && IsPlantCombatSeed(plant.seedType); });
    if (!hasAttackPlant) {
        return zombieCount == 0 ? 3020 : 2640;
    }
    return zombieCount == 0 ? 1450 : 1700;
}

} // namespace vsai::detail
