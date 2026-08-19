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

#include "VSActionAICardRules.h"
#include "VSActionAIThreat.h"

#include <cmath>
#include <algorithm>
#include <limits>

#include "PvZ/Lawn/Common/GameConstants.h"

namespace vsai::detail {

namespace {

    bool IsSlowTrashcan(const VSZombieState &zombie) {
        return static_cast<ZombieType>(zombie.zombieType) == ZombieType::ZOMBIE_TRASHCAN;
    }

    int ScaleZombieThreat(const VSZombieState &zombie, int value) {
        return IsSlowTrashcan(zombie) && value > 0 ? std::max(1, value / 5) : value;
    }

} // namespace

int LargestZombieStackInRow(const VSGameState &state, int row) {
    constexpr float kGridCellWidth = 80.0f;
    int largestStack = 0;
    for (const VSZombieState &anchor : state.zombies) {
        if (anchor.dead || anchor.row != row) {
            continue;
        }

        int stackSize = 0;
        for (const VSZombieState &zombie : state.zombies) {
            if (zombie.dead || zombie.row != row) {
                continue;
            }
            const float distance = zombie.positionX - anchor.positionX;
            if (distance > -kGridCellWidth && distance < kGridCellWidth) {
                ++stackSize;
            }
        }
        largestStack = std::max(largestStack, stackSize);
    }
    return largestStack;
}

int LargestCherryBombClusterInRow(const VSGameState &state, int row) {
    constexpr float kCherryBombRadius = 115.0f;
    int largestCluster = 0;
    for (const VSZombieState &anchor : state.zombies) {
        if (anchor.dead || anchor.row != row) {
            continue;
        }

        int clusterSize = 0;
        for (const VSZombieState &zombie : state.zombies) {
            if (!zombie.dead && zombie.row == row && std::abs(zombie.positionX - anchor.positionX) <= kCherryBombRadius) {
                ++clusterSize;
            }
        }
        largestCluster = std::max(largestCluster, clusterSize);
    }
    return largestCluster;
}

int ZombieThreatWeight(std::uint16_t zombieType) {
    switch (static_cast<ZombieType>(zombieType)) {
        // Trashcan is a slow grave screen, not an attacking body. Keep its
        // contribution at one fifth of an ordinary threat during planning.
        case ZombieType::ZOMBIE_TRASHCAN:
            return 6;
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
            return 115;
        case ZombieType::ZOMBIE_BOBSLED:
        case ZombieType::ZOMBIE_ZAMBONI:
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_WALLNUT_HEAD:
        case ZombieType::ZOMBIE_SQUASH_HEAD:
            return 80;
        case ZombieType::ZOMBIE_PAIL:
        case ZombieType::ZOMBIE_DIGGER:
        case ZombieType::ZOMBIE_POLEVAULTER:
        case ZombieType::ZOMBIE_SUPER_FAN_IMP:
            return 55;
        default:
            return 30;
    }
}

int ZombieEffectiveThreatHealth(const VSZombieState &zombie) {
    const int health = std::max(0, zombie.bodyHealth) + std::max(0, zombie.shieldHealth);
    if (static_cast<ZombieType>(zombie.zombieType) == ZombieType::ZOMBIE_TRASHCAN) {
        return health > 0 ? std::max(1, health / 5) : 0;
    }
    return health;
}

bool HasLiveZombieTargetInRow(const VSGameState &state, int row) {
    bool hasTargetMarkers = false;
    for (const VSGridItemState &item : state.gridItems) {
        if (item.gridItemType != static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE)) {
            continue;
        }
        hasTargetMarkers = true;
        if (!item.dead && item.health > 0 && item.position.row == row) {
            return true;
        }
    }
    return !hasTargetMarkers;
}

bool HasDestroyedZombieTargetInRow(const VSGameState &state, int row) {
    return std::any_of(state.gridItems.begin(), state.gridItems.end(), [row](const VSGridItemState &item) {
        return item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) && item.position.row == row && (item.dead || item.health <= 0);
    });
}

bool AllMowersSpent(const VSGameState &state) {
    if (state.rows <= 0 || state.mowerAvailable.size() < static_cast<std::size_t>(state.rows) || state.mowerInMotion.size() < static_cast<std::size_t>(state.rows)) {
        return false;
    }
    for (int row = 0; row < state.rows; ++row) {
        const std::size_t index = static_cast<std::size_t>(row);
        // MOWER_TRIGGERED clears the entire lane but is not a spent mower yet.
        if (state.mowerAvailable[index] || state.mowerInMotion[index]) {
            return false;
        }
    }
    return true;
}

bool IsMowerlessStrongPlantLane(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows || row >= static_cast<int>(state.mowerAvailable.size()) || state.mowerAvailable[static_cast<std::size_t>(row)] || IsMowerInMotion(state, row)
        || CountPlantsInRow(state, row) == 0) {
        return false;
    }

    const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
    // An empty mowerless lane is already a poor place to spend a grave. If
    // attackers are present, suppress only when the existing DPS can hold it.
    return CountZombiesInRow(state, row) == 0 || firepower.canHold || (firepower.dps >= 45 && firepower.deficit <= 20);
}

bool IsMowerlessThirdColumnEmergency(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows || row >= static_cast<int>(state.mowerAvailable.size()) || state.mowerAvailable[static_cast<std::size_t>(row)] || IsMowerInMotion(state, row)) {
        return false;
    }

    // Plant columns are zero based. Once a zombie crosses the right edge of
    // column two, it has reached the third plant column with no mower left.
    constexpr float kThirdColumnBoundary = static_cast<float>(LAWN_XMIN + 3 * 80);
    return std::any_of(state.zombies.begin(), state.zombies.end(), [row](const VSZombieState &zombie) { return !zombie.dead && zombie.row == row && zombie.positionX < kThirdColumnBoundary; });
}

int CounterPressureScoreInRow(const VSGameState &state, int row) {
    int score = 0;
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row) {
            continue;
        }
        score += ZombieThreatWeight(zombie.zombieType);
        score += ScaleZombieThreat(zombie, IsDecisiveCounterZombie(zombie.zombieType) ? 150 : 0);
        score += ScaleZombieThreat(zombie, zombie.eating ? 90 : 0);
        score += ScaleZombieThreat(zombie, std::clamp((880 - static_cast<int>(zombie.positionX)) / 8, 0, 70));
        if (zombie.bodyMaxHealth > 0 && zombie.bodyHealth * 100 / zombie.bodyMaxHealth >= 70) {
            score += IsHeavyZombie(zombie.zombieType) ? 45 : 0;
        }
    }
    // A genuine pileup is more urgent than the same number of separated
    // zombies, but the stack must fit inside one lawn cell.
    score += LargestZombieStackInRow(state, row) * 90;
    // Losing a mower turns an intruder in column three into a board-loss
    // risk. This must outrank every economy and cross-lane opportunity.
    score += IsMowerlessThirdColumnEmergency(state, row) ? 2000 : 0;
    return score;
}

int MostUrgentCounterRow(const VSGameState &state) {
    int bestRow = 0;
    int bestScore = 0;
    for (int row = 0; row < state.rows; ++row) {
        const int score = CounterPressureScoreInRow(state, row);
        if (score > bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int ZombieFrontlineValueInRow(const VSGameState &state, int row) {
    int score = 0;
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row) {
            continue;
        }
        score += ZombieThreatWeight(zombie.zombieType);
        score += ScaleZombieThreat(zombie, IsHeavyZombie(zombie.zombieType) ? 70 : 0);
        score += ScaleZombieThreat(zombie, zombie.shieldHealth > 0 ? 20 : 0);
        score += ScaleZombieThreat(zombie, zombie.eating ? 35 : 0);
        score += ScaleZombieThreat(zombie, zombie.positionX < 760.0f ? 25 : 0);
    }
    return score;
}

int MostValuableZombieFrontRow(const VSGameState &state) {
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        const int score = ZombieFrontlineValueInRow(state, row);
        if (score > bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int ZombiePressureInRow(const VSGameState &state, int row) {
    int pressure = 0;
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row) {
            continue;
        }
        pressure += ScaleZombieThreat(zombie, 55 + std::clamp((900 - static_cast<int>(zombie.positionX)) / 10, 0, 65));
        pressure += ScaleZombieThreat(zombie, IsHeavyZombie(zombie.zombieType) ? 25 : 0);
        pressure += ScaleZombieThreat(zombie, zombie.eating ? 40 : 0);
    }
    return pressure;
}

int PlantDefenseValue(const VSPlantState &plant) {
    const int healthRatio = plant.maxHealth > 0 ? std::clamp(plant.health * 100 / plant.maxHealth, 0, 100) : 50;
    int score = 0;
    switch (static_cast<SeedType>(plant.seedType)) {
        case SeedType::SEED_WALLNUT:
        case SeedType::SEED_TALLNUT:
        case SeedType::SEED_PUMPKINSHELL:
            score = 110;
            break;
        case SeedType::SEED_SNOWPEA:
            score = 75;
            break;
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_SPLITPEA:
            score = 45;
            break;
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_BLOOMERANG:
            score = 65;
            break;
        case SeedType::SEED_THREEPEATER:
            score = 75;
            break;
        case SeedType::SEED_MELONPULT:
            score = 95;
            break;
        case SeedType::SEED_WINTERMELON:
            score = 115;
            break;
        case SeedType::SEED_GATLINGPEA:
        case SeedType::SEED_GLOOMSHROOM:
            score = 110;
            break;
        case SeedType::SEED_BONK_CHOY:
        case SeedType::SEED_CELERY_STALKER:
            score = 65;
            break;
        case SeedType::SEED_IMP_PEAR:
            score = 55;
            break;
        case SeedType::SEED_STARFRUIT:
        case SeedType::SEED_SPORESHROOM:
            score = 70;
            break;
        case SeedType::SEED_CHOMPER:
            score = 60;
            break;
        default:
            score = 25;
            break;
    }
    return score * healthRatio / 100;
}

int PlantDamagePerSecond(SeedType seedType) {
    // These values mirror the relative damage/cadence of the VS plants. The
    // agent needs a stable tactical estimate rather than animation-perfect
    // frame prediction, so values are rounded to whole damage per second.
    switch (seedType) {
        case SeedType::SEED_GATLINGPEA:
            return 56;
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_WINTERMELON:
            return 48;
        case SeedType::SEED_REPEATER:
            return 28;
        case SeedType::SEED_FUMESHROOM:
            return 24;
        case SeedType::SEED_BLOOMERANG:
            return 22;
        case SeedType::SEED_GLOOMSHROOM:
            return 45;
        case SeedType::SEED_CABBAGEPULT:
            return 26;
        case SeedType::SEED_KERNELPULT:
            return 17;
        case SeedType::SEED_SNOWPEA:
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_THREEPEATER:
            return 14;
        case SeedType::SEED_PUFFSHROOM:
            return 10;
        case SeedType::SEED_SCAREDYSHROOM:
            return 16;
        case SeedType::SEED_STARFRUIT:
            return 18;
        case SeedType::SEED_SPORESHROOM:
            return 16;
        case SeedType::SEED_BONK_CHOY:
        case SeedType::SEED_CELERY_STALKER:
            return 22;
        case SeedType::SEED_CHOMPER:
            return 30;
        default:
            return 0;
    }
}

PlantLaneFirepower AssessPlantLaneFirepower(const VSGameState &state, int row) {
    PlantLaneFirepower assessment{};
    assessment.row = row;
    if (row < 0 || row >= state.rows) {
        return assessment;
    }

    float closestX = std::numeric_limits<float>::max();
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row) {
            continue;
        }
        const int health = ZombieEffectiveThreatHealth(zombie);
        assessment.incomingHealth += health;
        if (zombie.positionX <= 700.0f || zombie.eating) {
            assessment.nearHealth += health;
        }
        closestX = std::min(closestX, zombie.positionX);
    }
    if (closestX == std::numeric_limits<float>::max()) {
        return assessment;
    }

    assessment.closestDistance = std::max(0, static_cast<int>(closestX));
    // A row's actual forward-most plant is the meaningful contact point.
    // Treating every lane as if contact began at a fixed rear coordinate
    // makes a zombie in column five look harmless while it is already about
    // to chew a forward income plant or a defensive line.
    int frontPlantColumn = -1;
    for (const VSPlantState &plant : state.plants) {
        if (!IsDeadOrOutside(plant) && plant.position.row == row) {
            frontPlantColumn = std::max(frontPlantColumn, static_cast<int>(plant.position.col));
        }
    }
    const int contactX = frontPlantColumn < 0 ? LAWN_XMIN + 120 : LAWN_XMIN + frontPlantColumn * 80 + 40;
    assessment.secondsToContact = std::clamp((assessment.closestDistance - contactX) / 42, 1, 16);
    for (const VSPlantState &plant : state.plants) {
        if (!IsDeadOrOutside(plant) && plant.position.row == row) {
            switch (static_cast<SeedType>(plant.seedType)) {
                case SeedType::SEED_WALLNUT:
                case SeedType::SEED_TALLNUT:
                    assessment.secondsToContact += 7;
                    break;
                case SeedType::SEED_PUMPKINSHELL:
                    assessment.secondsToContact += 4;
                    break;
                default:
                    break;
            }
        }
    }
    assessment.secondsToContact = std::min(24, assessment.secondsToContact);

    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.asleep) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(plant.seedType);
        const int baseDps = PlantDamagePerSecond(seed);
        if (baseDps == 0) {
            continue;
        }

        const int rowDistance = std::abs(static_cast<int>(plant.position.row) - row);
        int contribution = 0;
        if (plant.position.row == row) {
            contribution = baseDps;
            // Melee output becomes real only after an intruder reaches its
            // attack zone. It cannot be used to justify a distant lane.
            if ((seed == SeedType::SEED_BONK_CHOY || seed == SeedType::SEED_CELERY_STALKER) && closestX > 560.0f) {
                contribution = 0;
            } else if (seed == SeedType::SEED_CHOMPER && closestX > 500.0f) {
                contribution = 0;
            }
        } else if (seed == SeedType::SEED_THREEPEATER && rowDistance == 1) {
            contribution = baseDps;
        } else if (seed == SeedType::SEED_STARFRUIT && rowDistance == 1) {
            contribution = baseDps * 2 / 3;
        } else if (seed == SeedType::SEED_GLOOMSHROOM && rowDistance == 1 && closestX < 480.0f) {
            contribution = baseDps / 2;
        }
        if (contribution == 0) {
            continue;
        }

        const int healthRatio = plant.maxHealth > 0 ? std::clamp(plant.health * 100 / plant.maxHealth, 0, 100) : 50;
        assessment.dps += contribution * healthRatio / 100;
    }

    // Distant units can still be addressed after the nearest contact; near
    // health is the immediate requirement that decides whether a new shooter
    // is needed before another economy plant.
    const int requiredHealth = assessment.nearHealth > 0 ? assessment.nearHealth : assessment.incomingHealth;
    assessment.damageBeforeContact = assessment.dps * assessment.secondsToContact;
    const int requiredDps = (requiredHealth + assessment.secondsToContact - 1) / assessment.secondsToContact;
    assessment.deficit = std::max(0, requiredDps - assessment.dps);
    assessment.canHold = requiredHealth == 0 || assessment.damageBeforeContact >= requiredHealth;
    return assessment;
}

PlantLaneAssessment AssessPlantLane(const VSGameState &state, int row) {
    PlantLaneAssessment assessment{};
    assessment.row = row;
    assessment.closest = FindClosestZombie(state, row);
    int frontPlantColumn = -1;
    for (const VSPlantState &plant : state.plants) {
        if (!IsDeadOrOutside(plant) && plant.position.row == row) {
            frontPlantColumn = std::max(frontPlantColumn, static_cast<int>(plant.position.col));
        }
    }
    const float frontPlantX = frontPlantColumn < 0 ? static_cast<float>(LAWN_XMIN + 6 * 80) : static_cast<float>(LAWN_XMIN + frontPlantColumn * 80 + 40);
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row) {
            continue;
        }
        const int advance = std::clamp((850 - static_cast<int>(zombie.positionX)) / 3, 0, 240);
        assessment.rawDanger += ZombieThreatWeight(zombie.zombieType) + ScaleZombieThreat(zombie, advance);
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.eating ? 135 : 0);
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.positionX < 680.0f ? 35 : 0);
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.positionX < 600.0f ? 65 : 0);
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.positionX < 520.0f ? 100 : 0);
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.positionX < 400.0f ? 160 : 0);
        // Crossing the actual forward plant is a tactical break point. The
        // resulting score outranks economy opportunities in other rows.
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.positionX <= frontPlantX + 30.0f ? 140 : 0);
        assessment.rawDanger += ScaleZombieThreat(zombie, zombie.positionX <= frontPlantX - 50.0f ? 220 : 0);
        const int shieldThreat = std::max(0, zombie.shieldHealth) / 30;
        assessment.rawDanger += static_cast<ZombieType>(zombie.zombieType) == ZombieType::ZOMBIE_TRASHCAN ? shieldThreat / 5 : std::min(40, shieldThreat);
        assessment.hasHeavy = assessment.hasHeavy || IsHeavyZombie(zombie.zombieType);
        assessment.hasFast = assessment.hasFast || IsFastZombie(zombie.zombieType);
    }

    for (const VSPlantState &plant : state.plants) {
        if (!IsDeadOrOutside(plant) && plant.position.row == row) {
            assessment.defense += PlantDefenseValue(plant);
            ++assessment.plantCount;
        } else if (!IsDeadOrOutside(plant) && plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_STARFRUIT) && std::abs(static_cast<int>(plant.position.row) - row) == 1) {
            // Starfruit's diagonal shots support both adjacent lanes. Treat
            // that fire as partial cover instead of repeatedly overbuilding
            // a lane next to an established Starfruit pattern.
            assessment.defense += PlantDefenseValue(plant) / 2;
        }
    }
    const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
    // Unit counts alone hide the difference between a bucket zombie far away
    // and one that reaches a sunflower before the available DPS can remove
    // it. Surface that shortfall to all existing lane-choice callers.
    assessment.rawDanger += std::min(170, firepower.deficit * 5);
    if (!firepower.canHold && firepower.nearHealth > 0) {
        assessment.rawDanger += 35;
    }
    assessment.danger = std::max(0, assessment.rawDanger - assessment.defense / 2);
    return assessment;
}

PlantLaneAssessment MostThreatenedPlantLane(const VSGameState &state) {
    PlantLaneAssessment best{};
    best.danger = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        const PlantLaneAssessment assessment = AssessPlantLane(state, row);
        if (assessment.danger > best.danger) {
            best = assessment;
        }
    }
    return best;
}

int LeastDevelopedPlantRow(const VSGameState &state) {
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::max();
    for (int row = 0; row < state.rows; ++row) {
        const PlantLaneAssessment assessment = AssessPlantLane(state, row);
        const int incomeCount = static_cast<int>(std::count_if(state.plants.begin(), state.plants.end(), [&state, row](const VSPlantState &plant) {
            return !IsDeadOrOutside(plant) && plant.position.row == row && IsPlantEconomySeed(state, plant.seedType);
        }));
        const int score = assessment.defense + assessment.plantCount * 12 + incomeCount * 15;
        if (score < bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int PlantValueScore(const VSPlantState &plant) {
    // Health is intentionally capped: a full Wall-nut should be a worthwhile target,
    // not erase every other lane from the zombie agent's comparison.
    int score = std::clamp(plant.health / 10, 10, 80);
    switch (static_cast<SeedType>(plant.seedType)) {
        case SeedType::SEED_SUNFLOWER:
        case SeedType::SEED_SUNSHROOM:
            score += 35;
            break;
        case SeedType::SEED_SNOWPEA:
            score += 90;
            break;
        case SeedType::SEED_GATLINGPEA:
        case SeedType::SEED_WINTERMELON:
        case SeedType::SEED_GLOOMSHROOM:
            score += 135;
            break;
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_REPEATER:
            score += 95;
            break;
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_BLOOMERANG:
            score += 75;
            break;
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_SPLITPEA:
            score += 55;
            break;
        case SeedType::SEED_BONK_CHOY:
        case SeedType::SEED_CELERY_STALKER:
            score += 75;
            break;
        case SeedType::SEED_STARFRUIT:
            score += 100;
            break;
        case SeedType::SEED_CHOMPER:
            score += 95;
            break;
        case SeedType::SEED_SPORESHROOM:
            score += 85;
            break;
        case SeedType::SEED_WALLNUT:
        case SeedType::SEED_TALLNUT:
        case SeedType::SEED_PUMPKINSHELL:
            score += 55;
            break;
        default:
            score += 45;
            break;
    }
    return score;
}

bool IsPlantProtectedByUmbrella(const VSGameState &state, VSGridPosition position) {
    if (position.col < 0 || position.row < 0) {
        return false;
    }
    return std::any_of(state.plants.begin(), state.plants.end(), [position](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_UMBRELLA) && std::abs(static_cast<int>(plant.position.col) - static_cast<int>(position.col)) <= 1
            && std::abs(static_cast<int>(plant.position.row) - static_cast<int>(position.row)) <= 1;
    });
}

bool IsPlantEconomySeed(const VSGameState &state, std::uint16_t seedType) {
    return seedType == static_cast<std::uint16_t>(SeedType::SEED_SUNFLOWER) || seedType == static_cast<std::uint16_t>(SeedType::SEED_TWINSUNFLOWER)
        || (state.isNight && seedType == static_cast<std::uint16_t>(SeedType::SEED_SUNSHROOM));
}

bool IsPlantCombatSeed(std::uint16_t seedType) {
    return HasPlantCardRole(static_cast<SeedType>(seedType), VSCardRole::PlantCombat);
}

bool IsSustainedOutputSeed(SeedType seedType) {
    return HasPlantCardRole(seedType, VSCardRole::PlantSustainedOutput);
}

int SustainedOutputValue(SeedType seedType) {
    switch (seedType) {
        case SeedType::SEED_GATLINGPEA:
        case SeedType::SEED_WINTERMELON:
        case SeedType::SEED_GLOOMSHROOM:
            return 130;
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_STARFRUIT:
            return 100;
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_BLOOMERANG:
        case SeedType::SEED_SNOWPEA:
        case SeedType::SEED_SPORESHROOM:
            return 80;
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_SPLITPEA:
            return 55;
        default:
            return 0;
    }
}

int CountSustainedOutputPlants(const VSGameState &state) {
    return static_cast<int>(
        std::count_if(state.plants.begin(), state.plants.end(), [](const VSPlantState &plant) { return !IsDeadOrOutside(plant) && IsSustainedOutputSeed(static_cast<SeedType>(plant.seedType)); }));
}

int SustainedOutputScoreInRow(const VSGameState &state, int row) {
    int score = 0;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row) {
            continue;
        }

        const SeedType seed = static_cast<SeedType>(plant.seedType);
        int plantScore = SustainedOutputValue(seed);
        if (seed == SeedType::SEED_BONK_CHOY || seed == SeedType::SEED_CELERY_STALKER) {
            plantScore = 55;
        } else if (seed == SeedType::SEED_CHOMPER) {
            plantScore = 65;
        }
        if (plantScore == 0) {
            continue;
        }
        const int healthRatio = plant.maxHealth > 0 ? std::clamp(plant.health * 100 / plant.maxHealth, 0, 100) : 50;
        // A sleeping mushroom is an investment, but does not yet hold a lane.
        score += plantScore * (plant.asleep ? 25 : healthRatio) / 100;
    }
    return score;
}

int PlantEconomyValueInRow(const VSGameState &state, int row) {
    int score = 0;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row || !IsPlantEconomySeed(state, plant.seedType)) {
            continue;
        }
        const int healthRatio = plant.maxHealth > 0 ? std::clamp(plant.health * 100 / plant.maxHealth, 0, 100) : 50;
        score += 70 * healthRatio / 100;
        // A rear economy plant takes longer to replace than a disposable
        // front filler and is a better route to protect with lasting fire.
        score += std::max(0, 3 - static_cast<int>(plant.position.col)) * 8;
    }
    return score;
}

bool HasSustainedOutputSeed(const VSGameState &state) {
    return std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) { return IsSustainedOutputSeed(static_cast<SeedType>(card.seedType)); });
}

} // namespace vsai::detail
