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

#ifndef PVZ_LAWN_WIDGET_VS_ACTION_AI_DRAFT_POLICY_H
#define PVZ_LAWN_WIDGET_VS_ACTION_AI_DRAFT_POLICY_H

#include "PvZ/Lawn/Common/ConstEnums.h"

#include <cstddef>
#include <cstdint>
#include <span>

class LawnApp;

namespace vsai::draft {

enum class BanDatabaseLoadState : std::uint8_t {
    Uninitialized,
    Unavailable,
    Invalid,
    Loaded,
};

// Replay Ban data is advisory. Callers retain their own baseline and matchup
// scores, while this loader only exposes a validated priority for one seed.
int BanDatabasePriority(bool targetsZombies, SeedType seed, std::uint32_t tick);
BanDatabaseLoadState GetBanDatabaseLoadState();
void ResetBanDatabase();

// Draft roles and Ban priorities rank selectable cards before the chooser
// applies its live engine legality and UI state checks.
int BanBaseThreat(bool targetsZombies, SeedType seed);
std::span<const SeedType> PlantBanPriority();
std::span<const SeedType> ZombieBanPriority();

// This session owns only cross-chooser template selection. The chooser owns
// engine legality, input routing, animations, and applying the selected seed.
struct BuiltinAIDraftSession {
    LawnApp *app = nullptr;
    int plantProfile = -1;
    int zombieProfile = -1;
    int plantMainPickSlot = -1;
    bool usePlantTemplate = true;
    bool useZombieTemplate = true;
};

BuiltinAIDraftSession &GetBuiltinAIDraftSession();
void ResetBuiltinAIDraftSession();

// This optional history intentionally survives one chooser session so
// consecutive local matches do not repeat an archetype when RNG is seeded
// identically. It is not current-match draft state.
struct BuiltinAIDraftHistory {
    int lastPlantProfile = -1;
    int lastZombieProfile = -1;
};

BuiltinAIDraftHistory &GetBuiltinAIDraftHistory();

// These are draft composition rules, not runtime plant roles or replay deck
// archetypes. See VSActionAICardRules and VSActionAIStrategy respectively.
bool IsPlantTempoMushroom(SeedType seed);
bool IsPlantCarrySeed(SeedType seed);
bool IsPeaMainDamageSeed(SeedType seed);
bool IsCoffeeDependentPlant(SeedType seed);
bool IsMagnetTargetZombieSeed(SeedType seed);

template <typename IsEligible>
SeedType FindRotatedEligibleSeed(std::span<const SeedType> seeds, std::size_t firstIndex, IsEligible &&isEligible) {
    if (seeds.empty()) {
        return SeedType::SEED_NONE;
    }
    firstIndex %= seeds.size();
    for (std::size_t offset = 0; offset < seeds.size(); ++offset) {
        const SeedType seed = seeds[(firstIndex + offset) % seeds.size()];
        if (isEligible(seed)) {
            return seed;
        }
    }
    return SeedType::SEED_NONE;
}

} // namespace vsai::draft

#endif // PVZ_LAWN_WIDGET_VS_ACTION_AI_DRAFT_POLICY_H
