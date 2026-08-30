/*
 * Copyright (C) 2023-2026 PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIThreat.h"

#include "PvZ/Lawn/Board/VSActionAI/VSActionAILanePolicy.h"

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIStrategy.h"

#include <cmath>
#include <algorithm>
#include <limits>

namespace vsai::detail {

bool IsZombieEconomyItem(std::uint16_t gridItemType) {
    return gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_GRAVESTONE) || gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_BURIAL_MOUND);
}

int EstimatedEconomyMaxHealth(const VSGridItemState &item) {
    if (item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_BURIAL_MOUND)) {
        return 350 + 70 * (std::clamp(item.level, 0, 4) + 1);
    }
    return 350;
}

int StraightProjectileThreatToEconomy(const VSPlantState &plant, const VSGridItemState &economy) {
    const int rowDistance = std::abs(static_cast<int>(plant.position.row) - static_cast<int>(economy.position.row));
    if (plant.position.col >= economy.position.col) {
        return 0;
    }

    const SeedType seed = static_cast<SeedType>(plant.seedType);
    const bool reachesEconomyRow = rowDistance == 0 || (seed == SeedType::SEED_THREEPEATER && rowDistance == 1);
    if (!reachesEconomyRow) {
        return 0;
    }

    switch (seed) {
        case SeedType::SEED_GATLINGPEA:
            return 190;
        case SeedType::SEED_REPEATER:
            return 165;
        case SeedType::SEED_BLOOMERANG:
            return 135;
        case SeedType::SEED_SNOWPEA:
            return 150;
        case SeedType::SEED_SCAREDYSHROOM:
            return 135;
        case SeedType::SEED_THREEPEATER:
            return 135;
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_CACTUS:
            return 120;
        default:
            return 0;
    }
}

int PlantThreatToEconomy(const VSPlantState &plant, const VSGridItemState &economy) {
    if (IsDeadOrOutside(plant) || plant.position.row < 0 || economy.position.row < 0) {
        return 0;
    }

    const int rowDistance = std::abs(static_cast<int>(plant.position.row) - static_cast<int>(economy.position.row));
    const SeedType seed = static_cast<SeedType>(plant.seedType);
    if (seed == SeedType::SEED_STARFRUIT) {
        // Starfruit fires in five directions and is the one plant that can
        // threaten a grave from an adjacent row as well as its own row.
        return rowDistance == 0 ? 145 : (rowDistance == 1 ? 75 : 0);
    }
    if (seed == SeedType::SEED_GRAVEBUSTER) {
        return rowDistance == 0 && plant.position.col == economy.position.col ? 250 : 0;
    }
    if (const int projectileThreat = StraightProjectileThreatToEconomy(plant, economy); projectileThreat > 0) {
        return projectileThreat;
    }
    // Pults and mushrooms can lock onto VS graves. Keep this list explicit so
    // melee plants (Bonk Choy, Celery Stalker, Chomper) never inflate grave
    // threat when they cannot reach the zombie economy.
    if (rowDistance != 0 || plant.position.col >= economy.position.col) {
        return 0;
    }
    switch (seed) {
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_SPORESHROOM:
            return 75;
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_BLOOMERANG:
            return 90;
        case SeedType::SEED_MELONPULT:
            return 115;
        case SeedType::SEED_WINTERMELON:
            return 135;
        case SeedType::SEED_GLOOMSHROOM:
            return 100;
        default:
            return 0;
    }
}

int StraightProjectileThreatScore(const VSGameState &state, int row) {
    int score = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || !IsZombieEconomyItem(item.gridItemType) || item.position.row != row) {
            continue;
        }
        for (const VSPlantState &plant : state.plants) {
            if (!IsDeadOrOutside(plant)) {
                score += StraightProjectileThreatToEconomy(plant, item);
            }
        }
    }
    return score;
}

int LobbedProjectileThreatScore(const VSGameState &state, int row) {
    int score = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || !IsZombieEconomyItem(item.gridItemType) || item.position.row != row) {
            continue;
        }
        for (const VSPlantState &plant : state.plants) {
            if (IsDeadOrOutside(plant) || plant.position.row != row) {
                continue;
            }
            const SeedType seed = static_cast<SeedType>(plant.seedType);
            switch (seed) {
                case SeedType::SEED_CABBAGEPULT:
                case SeedType::SEED_KERNELPULT:
                case SeedType::SEED_MELONPULT:
                case SeedType::SEED_WINTERMELON:
                    score += PlantThreatToEconomy(plant, item);
                    break;
                default:
                    break;
            }
        }
    }
    return score;
}

bool NeedsProactiveGraveScreen(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows || HasZombieGraveGuardInRow(state, row)) {
        return false;
    }

    int economyAssets = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (!item.dead && item.position.row == row && IsZombieEconomyItem(item.gridItemType)) {
            ++economyAssets;
        }
    }
    // A live straight shooter has already acquired the rear economic lane.
    // Do not wait for the first tombstone to be half dead before assigning a
    // Trashcan, Door, Pail, or head as the screen.
    const int directThreat = StraightProjectileThreatScore(state, row);
    return economyAssets > 0 && directThreat >= (economyAssets >= 2 ? 55 : 75);
}

int ZombieGraveScreenDeficit(const VSGameState &state, int row) {
    const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
    if (firepower.dps <= 0 || CountZombiesInRow(state, row) == 0) {
        return 0;
    }

    int screenHealth = 0;
    for (const VSZombieState &zombie : state.zombies) {
        if (!zombie.dead && zombie.row == row) {
            screenHealth += ZombieEffectiveThreatHealth(zombie);
        }
    }
    const int horizon = std::max(5, firepower.secondsToContact + 3);
    return std::max(0, firepower.dps * horizon - screenHealth);
}

int GraveThreatScore(const VSGameState &state, int row) {
    int score = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || !IsZombieEconomyItem(item.gridItemType) || item.position.row != row) {
            continue;
        }

        const int maxHealth = std::max(1, EstimatedEconomyMaxHealth(item));
        const int health = std::clamp(item.health, 0, maxHealth);
        score += std::max(0, (maxHealth - health) * 100 / maxHealth);
        score += health <= maxHealth / 3 ? 100 : (health <= maxHealth / 2 ? 45 : 0);
        for (const VSPlantState &plant : state.plants) {
            score += PlantThreatToEconomy(plant, item);
        }
    }
    return score;
}

int ProtectableGraveThreatScore(const VSGameState &state, int row) {
    int score = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || !IsZombieEconomyItem(item.gridItemType) || item.position.row != row) {
            continue;
        }

        const int maxHealth = std::max(1, EstimatedEconomyMaxHealth(item));
        const int health = std::clamp(item.health, 0, maxHealth);
        score += std::max(0, (maxHealth - health) * 100 / maxHealth);
        score += health <= maxHealth / 3 ? 100 : (health <= maxHealth / 2 ? 45 : 0);
        for (const VSPlantState &plant : state.plants) {
            // Gravebuster is already consuming this exact grave. Placing a
            // slow screen in front cannot save it, so it must not bait a
            // Trashcan or Door away from a real projectile threat.
            if (!IsDeadOrOutside(plant) && plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_GRAVEBUSTER)) {
                score += PlantThreatToEconomy(plant, item);
            }
        }
    }
    return score;
}

int ZombieEconomyAssetValue(const VSGridItemState &item) {
    if (!IsZombieEconomyItem(item.gridItemType)) {
        return 0;
    }
    if (item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_BURIAL_MOUND)) {
        return 135 + std::clamp(item.level, 0, 4) * 90;
    }
    return 110;
}

int ZombieEconomyAttackOpportunity(const VSGameState &state, int row) {
    int score = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || item.position.row != row || !IsZombieEconomyItem(item.gridItemType)) {
            continue;
        }

        const int assetValue = ZombieEconomyAssetValue(item);
        const int maxHealth = std::max(1, EstimatedEconomyMaxHealth(item));
        int existingPressure = 0;
        for (const VSPlantState &plant : state.plants) {
            existingPressure += PlantThreatToEconomy(plant, item);
        }
        // A fresh grave/mound is worth opening a firing lane for. Once it is
        // already under fire, finishing it remains useful but needs fewer
        // additional resources than a completely untouched income source.
        score += existingPressure > 0 ? assetValue : assetValue * 3;
        if (item.health <= maxHealth / 2) {
            score += assetValue / 2;
        }
    }
    return score;
}

int SeedEconomyPressureOpportunity(const VSGameState &state, SeedType seed, int row) {
    int score = 0;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || !IsZombieEconomyItem(item.gridItemType)) {
            continue;
        }
        if (seed == SeedType::SEED_GRAVEBUSTER && item.gridItemType != static_cast<std::uint16_t>(GridItemType::GRIDITEM_GRAVESTONE)) {
            continue;
        }

        const int rowDistance = std::abs(row - static_cast<int>(item.position.row));
        int pressure = 0;
        if (seed == SeedType::SEED_GRAVEBUSTER) {
            pressure = rowDistance == 0 ? 4 : 0;
        } else if (seed == SeedType::SEED_STARFRUIT) {
            pressure = rowDistance == 0 ? 3 : (rowDistance == 1 ? 2 : 0);
        } else if (seed == SeedType::SEED_THREEPEATER) {
            pressure = rowDistance <= 1 ? 2 : 0;
        } else if (IsSustainedOutputSeed(seed)) {
            pressure = rowDistance == 0 ? 2 : 0;
        }
        score += pressure * ZombieEconomyAssetValue(item);
    }
    return score;
}

int MostVulnerableZombieEconomyRow(const VSGameState &state) {
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        const int score = ZombieEconomyAttackOpportunity(state, row);
        if (score > bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int MostThreatenedEconomyRow(const VSGameState &state) {
    const auto IsLiveZombieTargetRow = [&state](int row) {
        return std::any_of(state.gridItems.begin(), state.gridItems.end(), [row](const VSGridItemState &item) {
            return !item.dead && item.health > 0 && item.position.row == row && item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE);
        });
    };
    const bool hasZombieTargets = std::any_of(
        state.gridItems.begin(), state.gridItems.end(), [](const VSGridItemState &item) { return item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE); });
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        // A lost target route cannot win the game back. On ordinary VS boards
        // keep every defensive decision on a surviving target row instead of
        // paying for a grave screen in a route that is already gone.
        if (hasZombieTargets && !IsLiveZombieTargetRow(row)) {
            continue;
        }
        // Pick the economic row which can still be screened, not merely the
        // one with the largest amount of historical damage. Direct shooters
        // and pults must both pull a guard toward their current firing lane.
        const int protectableThreat = ProtectableGraveThreatScore(state, row);
        const int straightThreat = StraightProjectileThreatScore(state, row);
        const int lobbedThreat = LobbedProjectileThreatScore(state, row);
        const int screenDeficit = ZombieGraveScreenDeficit(state, row);
        int score = protectableThreat * 2 + straightThreat + lobbedThreat + screenDeficit;
        if (hasZombieTargets) {
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            // Target survival is decided by the firing lane rather than only
            // the health of a grave that happens to be in front of it. Favor
            // high plant DPS, a developed output line, and an unscreened
            // target route so the first guard appears before a third target
            // can be lost.
            score += firepower.dps * 4 + SustainedOutputScoreInRow(state, row) * 3 + screenDeficit * 3;
            score += HasZombieGraveGuardInRow(state, row) ? 0 : 180;
            score += CountZombiesInRow(state, row) == 0 ? 90 : 0;
        }
        if (NeedsProactiveGraveScreen(state, row)) {
            score += 160;
        }
        if (score > bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int LeastThreatenedEconomyRow(const VSGameState &state) {
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::max();
    for (int row = 0; row < state.rows; ++row) {
        const int score = GraveThreatScore(state, row);
        if (score < bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int PlantLaneWeaknessScore(const VSGameState &state, int row) {
    const PlantLaneAssessment assessment = AssessPlantLane(state, row);
    const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
    if (assessment.plantCount == 0) {
        return -20;
    }

    int economyPlants = 0;
    int combatPlants = 0;
    int highValuePlants = 0;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row) {
            continue;
        }
        economyPlants += IsPlantEconomySeed(state, plant.seedType) ? 1 : 0;
        combatPlants += IsPlantCombatSeed(plant.seedType) ? 1 : 0;
        highValuePlants += PlantValueScore(plant) >= 100 ? 1 : 0;
    }

    // A line with multiple Sunflowers is a real investment. It must outrank
    // a merely sparse line so zombies keep opening distinct economic fronts.
    int economyScore = economyPlants * 95 + std::max(0, economyPlants - 1) * 45;
    // A Sunflower-only row is a cheap breakthrough opportunity. Count its
    // income twice until the plant side commits real firepower to the lane.
    if (combatPlants == 0 && economyPlants > 0) {
        economyScore *= 2;
    }
    int score = assessment.plantCount * 14 + economyScore + highValuePlants * 24;
    score += std::max(0, 120 - assessment.defense);
    score += combatPlants == 0 ? 35 : 0;
    score += assessment.rawDanger / 4;
    // This lane score feeds the zombie chooser. Prefer a sunflower row
    // whose actual output cannot clear a current push over a visually sparse
    // row that already has sufficient DPS.
    score += std::max(0, 34 - firepower.dps) * 3;
    score += firepower.deficit * 6;
    score += !firepower.canHold && firepower.nearHealth > 0 ? 95 : 0;
    return score;
}

int EconomyPlantsInRow(const VSGameState &state, int row) {
    return static_cast<int>(std::count_if(state.plants.begin(), state.plants.end(), [&state, row](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.position.row == row && IsPlantEconomySeed(state, plant.seedType);
    }));
}

int ZombieLaneAttackScore(const VSGameState &state, int row) {
    const ZombieLanePolicy policy = EvaluateZombieLanePolicy(state, row);
    // Lane scoring is also consumed by special grave-defense actions. A
    // strong mowerless plant lane suppresses ordinary bodies through
    // `allowsAttack`, but it must retain a finite score so a valid guard can
    // still be evaluated there. Only a lost target or a hard deployment stop
    // makes the entire row unusable.
    if (policy.deploymentBlocked || !policy.hasLiveTarget) {
        return std::numeric_limits<int>::min() / 4;
    }
    const PlantLaneAssessment assessment = AssessPlantLane(state, row);
    const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
    const int zombieCount = CountZombiesInRow(state, row);
    const int economyPlants = EconomyPlantsInRow(state, row);
    const int graveThreat = GraveThreatScore(state, row);
    int score = PlantLaneWeaknessScore(state, row);

    // Sunflowers and other economy plants are the most efficient pressure
    // targets. Empty rows are still useful for forcing the plant player to
    // spend resources, but are less valuable than a developed economy lane.
    // Separate Sunflower lanes are pressure targets in their own right. A
    // fresh economy lane should outrank feeding a second cheap zombie into a
    // defended lane that one Ash card can erase.
    score += economyPlants * 210 + std::max(0, economyPlants - 1) * 90;
    score += assessment.plantCount == 0 ? 28 : 0;
    score += assessment.defense < 100 ? 35 : 0;
    score += graveThreat * 3;
    score += std::max(0, 32 - firepower.dps) * 3;
    score += firepower.deficit * 5;
    score += !firepower.canHold && firepower.nearHealth > 0 ? 80 : 0;

    score += MowerlessLaneAttackScoreBonus(state, policy, row, zombieCount);
    if (zombieCount == 0) {
        score += policy.conversionRoute ? MowerlessLaneDistributionAdjustment(policy, zombieCount) : 150;
    } else if (zombieCount == 1) {
        score += policy.conversionRoute ? MowerlessLaneDistributionAdjustment(policy, zombieCount) : -115;
    } else {
        score += policy.conversionRoute ? MowerlessLaneDistributionAdjustment(policy, zombieCount) : -95 - (zombieCount - 1) * 175;
    }
    score -= ZombiePressureInRow(state, row) / 3;
    return score;
}

int MostVulnerablePlantRow(const VSGameState &state) {
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        const int score = PlantLaneWeaknessScore(state, row);
        if (score > bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

} // namespace vsai::detail
