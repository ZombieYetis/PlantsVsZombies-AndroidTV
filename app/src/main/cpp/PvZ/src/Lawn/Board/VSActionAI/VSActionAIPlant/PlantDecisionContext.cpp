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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIPlant/PlantAI.h"

#include <cmath>
#include <algorithm>
#include <limits>

namespace vsai::detail {

PlantDecisionContext PlantAIPlanning::BuildDecisionContext(const VSGameState &state) const {
    PlantDecisionContext context{};
    context.danger = MostThreatenedPlantLane(state);
    const std::uint16_t zombieDeckArchetype = DeckArchetype(state, VSSide::Zombies);
    const bool fastZombieDeck = (zombieDeckArchetype & kZombieDeckFastPressure) != 0;
    context.metalZombieDeck = (zombieDeckArchetype & kZombieDeckMetalScreen) != 0;
    const bool vehicleZombieDeck = (zombieDeckArchetype & kZombieDeckVehicle) != 0;
    context.economyZombieDeck = (zombieDeckArchetype & kZombieDeckEconomy) != 0;
    context.rangedZombieDeck = (zombieDeckArchetype & kZombieDeckRangedSiege) != 0;
    const bool heavyZombieDeck = (zombieDeckArchetype & kZombieDeckHeavy) != 0;
    const bool swarmZombieDeck = (zombieDeckArchetype & kZombieDeckSwarm) != 0;
    const bool deckNeedsEarlyFirepower = fastZombieDeck || vehicleZombieDeck || context.rangedZombieDeck || heavyZombieDeck || swarmZombieDeck;
    context.actualIncomePlantCount = CountPlantIncome(state);
    context.openingIncomeTarget = MainCarryIncomeTarget(state);
    context.incomePlantCount = context.actualIncomePlantCount;
    context.minimumIncomeBeforeOutput = context.openingIncomeTarget;
    context.sustainedOutputCount = CountSustainedOutputPlants(state);
    context.hasIncomeSeed = HasIncomeSeed(state);
    context.hasSunshroomFiller = HasSunshroomSeed(state);
    context.hasSustainedOutputSeed = HasSustainedOutputSeed(state);
    context.hasActiveZombie = CountActiveZombies(state) > 0;
    context.counterRow = MostUrgentCounterRow(state);
    context.mowerlessThirdColumnEmergency = IsMowerlessThirdColumnEmergency(state, context.counterRow);
    context.counterClosest = FindClosestZombie(state, context.counterRow);
    const int counterZombieCount = CountZombiesInRow(state, context.counterRow);
    const int counterStackCount = LargestSquashTargetStackInRow(state, context.counterRow);
    context.counterLane = AssessPlantLane(state, context.counterRow);
    context.counterFirepower = AssessPlantLaneFirepower(state, context.counterRow);
    context.firepowerRow = context.counterRow;
    int largestFirepowerDeficit = context.counterFirepower.deficit;
    int contestedZombieRows = 0;
    int unholdableZombieRows = 0;
    int incomingZombieHealth = 0;
    int damageBeforeZombieContact = 0;
    for (int row = 0; row < state.rows; ++row) {
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        if (firepower.deficit > largestFirepowerDeficit || (firepower.deficit == largestFirepowerDeficit && !firepower.canHold && row != context.counterRow)) {
            context.firepowerRow = row;
            largestFirepowerDeficit = firepower.deficit;
        }
        if (firepower.incomingHealth <= 0) {
            continue;
        }
        ++contestedZombieRows;
        incomingZombieHealth += firepower.incomingHealth;
        damageBeforeZombieContact += firepower.damageBeforeContact;
        if (!firepower.canHold || firepower.deficit > 0) {
            ++unholdableZombieRows;
        }
    }
    context.weakestFirepower = AssessPlantLaneFirepower(state, context.firepowerRow);
    context.largestFirepowerDeficit = largestFirepowerDeficit;
    context.counterCombatPlants = static_cast<int>(std::count_if(state.plants.begin(), state.plants.end(), [&context](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.position.row == context.counterRow && IsPlantCombatSeed(plant.seedType);
    }));
    const int combatPlantCount =
        static_cast<int>(std::count_if(state.plants.begin(), state.plants.end(), [](const VSPlantState &plant) { return !IsDeadOrOutside(plant) && IsPlantCombatSeed(plant.seedType); }));
    const bool hasCombatSeed =
        std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) { return card.active && !card.matchRestricted && IsPlantCombatSeed(card.seedType); });
    const bool hasGargantuar = HasZombieTypeInRow(state, context.counterRow, ZombieType::ZOMBIE_GARGANTUAR) || HasZombieTypeInRow(state, context.counterRow, ZombieType::ZOMBIE_GIGA_GARGANTUAR);
    const bool hasGigaPoleVaulter = HasZombieTypeInRow(state, context.counterRow, ZombieType::ZOMBIE_GIGA_POLEVAULTER);
    float closestZamboniX = std::numeric_limits<float>::max();
    float closestImpactThreatX = std::numeric_limits<float>::max();
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead) {
            continue;
        }
        if (zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_ZAMBONI) && zombie.positionX < closestZamboniX) {
            context.zamboniRow = zombie.row;
            closestZamboniX = zombie.positionX;
        }
        const ZombieType zombieType = static_cast<ZombieType>(zombie.zombieType);
        if ((zombieType == ZombieType::ZOMBIE_SQUASH_HEAD || zombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) && zombie.positionX < closestImpactThreatX) {
            context.impactThreatRow = zombie.row;
            closestImpactThreatX = zombie.positionX;
        }
    }
    const bool earlySingleBucket = state.boardTick < 32000 && counterZombieCount == 1 && HasZombieTypeInRow(state, context.counterRow, ZombieType::ZOMBIE_PAIL) && context.counterCombatPlants == 0
        && context.counterLane.plantCount <= 2 && context.counterClosest != nullptr && context.counterClosest->positionX < 700.0f && !context.counterFirepower.canHold;
    context.zombieCluster = context.hasActiveZombie && counterStackCount >= 2;
    context.squashThreat = earlySingleBucket;
    context.impPearThreat = (hasGargantuar || hasGigaPoleVaulter) && context.counterClosest != nullptr
        && (context.counterClosest->eating || context.counterClosest->positionX < 780.0f || context.counterLane.danger >= 160);
    context.counterFirstColumn = context.mowerlessThirdColumnEmergency ? 0 : 4;
    context.areaCounterReserve = AreaCounterReserve(state);
    const bool hasEconomyPressurePlan = HasEconomyPressurePlan(state);
    context.midGame = state.boardTick >= 16000 || CountZombieEconomy(state) >= std::max(2, (state.rows + 1) / 2);
    const int lateIncomeRecoveryTarget = !state.isSuddenDeath && context.midGame && context.actualIncomePlantCount < context.openingIncomeTarget ? context.openingIncomeTarget : 0;
    const int economyPressureIncomeTarget = EconomyPressureIncomeTarget(state);
    context.incomeExpansionTarget = state.isSuddenDeath ? 0 : std::max(economyPressureIncomeTarget, lateIncomeRecoveryTarget);
    context.pressureOutrunsFirepower = context.hasActiveZombie && (unholdableZombieRows > 0 || (contestedZombieRows >= 2 && damageBeforeZombieContact < incomingZombieHealth));
    context.immediateCounterThreat = context.squashThreat || context.impPearThreat || context.mowerlessThirdColumnEmergency;
    const bool mustHoldCounterReserve =
        context.areaCounterReserve > 0 && state.plantSun >= context.areaCounterReserve && (HasReadyZombieBreakthroughCard(state) || ((heavyZombieDeck || swarmZombieDeck) && context.hasActiveZombie));
    context.protectedSun = mustHoldCounterReserve ? context.areaCounterReserve : 0;
    context.zombieEconomyStrikeRow = MostVulnerableZombieEconomyRow(state);
    const int economyStrikeIncomeFloor = context.economyZombieDeck ? std::max(2, context.minimumIncomeBeforeOutput - 1) : context.minimumIncomeBeforeOutput;
    context.canStrikeZombieEconomy = (state.isSuddenDeath || context.incomePlantCount >= economyStrikeIncomeFloor) && hasEconomyPressurePlan && CountZombieEconomy(state) > 0
        && context.danger.danger < 150 && !context.immediateCounterThreat;
    const bool hasSporeCarry = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_SPORESHROOM);
    });
    context.maximumOutputCount = std::max(1, state.rows * (hasSporeCarry ? 3 : 2));
    context.desiredOutputCount = state.isSuddenDeath ? context.maximumOutputCount : std::min(context.maximumOutputCount, std::max(1, (context.incomePlantCount + 1) / 2));
    if (!state.isSuddenDeath && context.incomePlantCount >= context.openingIncomeTarget) {
        context.desiredOutputCount = std::max(context.desiredOutputCount, std::min(context.maximumOutputCount, state.rows + 1));
    }
    if (!state.isSuddenDeath && state.plantSun >= 125) {
        const int reserveOutputTarget = state.rows + (state.plantSun >= 350 ? 2 : 1);
        context.desiredOutputCount = std::max(context.desiredOutputCount, std::min(context.maximumOutputCount, reserveOutputTarget));
    }
    if (!state.isSuddenDeath && deckNeedsEarlyFirepower && context.incomePlantCount >= context.minimumIncomeBeforeOutput) {
        context.desiredOutputCount = std::max(context.desiredOutputCount, std::min(context.maximumOutputCount, state.rows));
    }
    if (!state.isSuddenDeath && hasSporeCarry) {
        const int sporeLayerTarget = state.plantSun >= 500 ? context.maximumOutputCount : (state.plantSun >= 350 ? state.rows * 2 + 2 : state.rows + (state.plantSun >= 200 ? 3 : 1));
        context.desiredOutputCount = std::max(context.desiredOutputCount, std::min(context.maximumOutputCount, sporeLayerTarget));
    }
    context.needsSustainedOutput = context.hasSustainedOutputSeed && (context.sustainedOutputCount < context.desiredOutputCount || (context.hasActiveZombie && context.largestFirepowerDeficit > 0));
    const int highSunCombatTarget = std::min(context.maximumOutputCount, state.rows + (state.plantSun >= 350 ? 2 : 1));
    context.highSunCombatPressure = !state.isSuddenDeath && state.plantSun >= 125 && context.incomePlantCount >= context.minimumIncomeBeforeOutput && (hasCombatSeed || context.hasSustainedOutputSeed)
        && (combatPlantCount < highSunCombatTarget || context.needsSustainedOutput || context.largestFirepowerDeficit > 0);
    context.readySustainedOutput = HasReadySustainedOutputCard(state, context.protectedSun);
    const bool deckOpeningPressure = deckNeedsEarlyFirepower && !context.hasActiveZombie && context.incomePlantCount >= context.minimumIncomeBeforeOutput && state.plantSun >= 100;
    context.openingNeedsFirepower = deckOpeningPressure
        || (context.hasActiveZombie
            && (context.counterFirepower.deficit > 0 || !context.counterFirepower.canHold
                || (context.counterClosest != nullptr && context.counterClosest->positionX < 720.0f && context.counterCombatPlants == 0)));
    context.outputTempoHasPriority = context.incomePlantCount >= context.openingIncomeTarget && context.needsSustainedOutput && context.readySustainedOutput
        && (context.pressureOutrunsFirepower || (context.midGame && context.incomePlantCount >= context.incomeExpansionTarget));
    context.mayExpandIncomePastOpening = context.incomePlantCount < context.openingIncomeTarget || !context.midGame
        || (!context.pressureOutrunsFirepower && (!context.needsSustainedOutput || !context.readySustainedOutput || context.incomePlantCount < context.incomeExpansionTarget));
    return context;
}

} // namespace vsai::detail
