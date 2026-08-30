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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_THREAT_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_THREAT_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIState.h"

namespace vsai::detail {

struct PlantLaneAssessment {
    int row = 0;
    int danger = 0;
    int rawDanger = 0;
    int defense = 0;
    int plantCount = 0;
    bool hasHeavy = false;
    bool hasFast = false;
    const VSZombieState *closest = nullptr;
};

struct PlantLaneFirepower {
    int row = 0;
    int dps = 0;
    int incomingHealth = 0;
    int nearHealth = 0;
    int closestDistance = 0;
    int secondsToContact = 0;
    int damageBeforeContact = 0;
    int deficit = 0;
    bool canHold = true;
};

int LargestZombieStackInRow(const VSGameState &state, int row);
int LargestCherryBombClusterInRow(const VSGameState &state, int row);
int ZombieThreatWeight(std::uint16_t zombieType);
// Trashcan zombies are deliberately slow grave screens. Their raw health is
// useful for lane survival, but they should not outweigh active attackers
// when choosing an ash target or comparing lanes.
int ZombieEffectiveThreatHealth(const VSZombieState &zombie);
bool IsMowerlessThirdColumnEmergency(const VSGameState &state, int row);
// On standard VS boards, a destroyed target marks a route which can no
// longer contribute to a zombie victory. Boards without target markers keep
// every row eligible for compatibility with the original VS variants.
bool HasLiveZombieTargetInRow(const VSGameState &state, int row);
// A target stays in the GridItem array briefly while its death animation
// plays. Zero health is already a permanent breakthrough for this row.
bool HasDestroyedZombieTargetInRow(const VSGameState &state, int row);
// Returns true only after every tracked plant mower has been spent. A single
// lost mower still deserves the normal conversion caution; the all-lost
// state is a different phase where zombie pressure must not stall.
bool AllMowersSpent(const VSGameState &state);
// A spent mower is not automatically a free attack route. When the lane has
// a developed, sustainable defense, feeding it only gives the plant side an
// efficient ash/firepower trade.
bool IsMowerlessStrongPlantLane(const VSGameState &state, int row);
int CounterPressureScoreInRow(const VSGameState &state, int row);
int MostUrgentCounterRow(const VSGameState &state);
int ZombieFrontlineValueInRow(const VSGameState &state, int row);
int MostValuableZombieFrontRow(const VSGameState &state);
int ZombiePressureInRow(const VSGameState &state, int row);
int PlantDefenseValue(const VSPlantState &plant);
int PlantDamagePerSecond(SeedType seedType);
PlantLaneFirepower AssessPlantLaneFirepower(const VSGameState &state, int row);
PlantLaneAssessment AssessPlantLane(const VSGameState &state, int row);
PlantLaneAssessment MostThreatenedPlantLane(const VSGameState &state);
bool IsPlantEconomySeed(const VSGameState &state, std::uint16_t seedType);
int LeastDevelopedPlantRow(const VSGameState &state);
int PlantValueScore(const VSPlantState &plant);
// Umbrella Leaf protects its surrounding 3x3 grid area from Bungee theft.
// Keep target-selection AI from spending a targeted card on a reflected drop.
bool IsPlantProtectedByUmbrella(const VSGameState &state, VSGridPosition position);
bool IsPlantCombatSeed(std::uint16_t seedType);
bool IsSustainedOutputSeed(SeedType seedType);
int SustainedOutputValue(SeedType seedType);
int CountSustainedOutputPlants(const VSGameState &state);
int SustainedOutputScoreInRow(const VSGameState &state, int row);
int PlantEconomyValueInRow(const VSGameState &state, int row);
bool HasSustainedOutputSeed(const VSGameState &state);
bool IsZombieEconomyItem(std::uint16_t gridItemType);
int EstimatedEconomyMaxHealth(const VSGridItemState &item);
int StraightProjectileThreatToEconomy(const VSPlantState &plant, const VSGridItemState &economy);
int PlantThreatToEconomy(const VSPlantState &plant, const VSGridItemState &economy);
int StraightProjectileThreatScore(const VSGameState &state, int row);
int LobbedProjectileThreatScore(const VSGameState &state, int row);
bool NeedsProactiveGraveScreen(const VSGameState &state, int row);
int ZombieGraveScreenDeficit(const VSGameState &state, int row);
int GraveThreatScore(const VSGameState &state, int row);
int ProtectableGraveThreatScore(const VSGameState &state, int row);
int ZombieEconomyAssetValue(const VSGridItemState &item);
int ZombieEconomyAttackOpportunity(const VSGameState &state, int row);
int SeedEconomyPressureOpportunity(const VSGameState &state, SeedType seed, int row);
int MostVulnerableZombieEconomyRow(const VSGameState &state);
int MostThreatenedEconomyRow(const VSGameState &state);
int LeastThreatenedEconomyRow(const VSGameState &state);
int PlantLaneWeaknessScore(const VSGameState &state, int row);
int EconomyPlantsInRow(const VSGameState &state, int row);
int ZombieLaneAttackScore(const VSGameState &state, int row);
int MostVulnerablePlantRow(const VSGameState &state);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_THREAT_H
