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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_POLICY_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_POLICY_H

#include "PvZ/Lawn/VSActionSystem.h"

#include <algorithm>

namespace vsai::detail {

// Shared policy boundary for the optional local AI enhancement.  Keeping the
// resource-count and production-rate rules together prevents strategy and
// engine-side production checks from drifting apart.
struct AIEnhancementPolicy {
    static constexpr int kEconomyLead = 1;
    static constexpr int kCooldownNumerator = 7;
    static constexpr int kCooldownDenominator = 10;

    bool enabled = false;

    int EffectiveEconomyCount(int actualCount) const {
        return enabled && actualCount > 0 ? actualCount + kEconomyLead : actualCount;
    }

    int ScaleProductionCooldown(int cooldown) const {
        if (!enabled || cooldown <= 0) {
            return cooldown;
        }
        return std::max(1, (cooldown * kCooldownNumerator + kCooldownDenominator - 1) / kCooldownDenominator);
    }
};

AIEnhancementPolicy GetAIEnhancementPolicy(VSSide side);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_POLICY_H
