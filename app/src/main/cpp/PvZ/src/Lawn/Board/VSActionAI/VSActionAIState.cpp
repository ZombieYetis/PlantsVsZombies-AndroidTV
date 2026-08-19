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

#include "VSActionAIState.h"

#include "PvZ/Lawn/Common/GameConstants.h"

#include <cstddef>
#include <algorithm>

namespace vsai::detail {

bool IsDeadOrOutside(const VSPlantState &plant) {
    return plant.dead || plant.position.row < 0 || plant.position.col < 0;
}

bool HasPlantAt(const VSGameState &state, VSGridPosition position) {
    return std::any_of(state.plants.begin(), state.plants.end(), [position](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.position.col == position.col && plant.position.row == position.row;
    });
}

bool HasPlantTypeAt(const VSGameState &state, SeedType seedType, VSGridPosition position) {
    return std::any_of(state.plants.begin(), state.plants.end(), [seedType, position](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.seedType == static_cast<std::uint16_t>(seedType) && plant.position.col == position.col && plant.position.row == position.row;
    });
}

bool HasGridItemAt(const VSGameState &state, VSGridPosition position) {
    return std::any_of(
        state.gridItems.begin(), state.gridItems.end(), [position](const VSGridItemState &item) { return !item.dead && item.position.col == position.col && item.position.row == position.row; });
}

const VSZombieState *FindClosestZombie(const VSGameState &state, int row) {
    const VSZombieState *closest = nullptr;
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row < 0 || (row >= 0 && zombie.row != row)) {
            continue;
        }
        if (closest == nullptr || zombie.positionX < closest->positionX) {
            closest = &zombie;
        }
    }
    return closest;
}

int CountPlantsInRow(const VSGameState &state, int row) {
    return static_cast<int>(std::count_if(state.plants.begin(), state.plants.end(), [row](const VSPlantState &plant) { return !IsDeadOrOutside(plant) && plant.position.row == row; }));
}

int CountZombiesInRow(const VSGameState &state, int row) {
    return static_cast<int>(std::count_if(state.zombies.begin(), state.zombies.end(), [row](const VSZombieState &zombie) { return !zombie.dead && zombie.row == row; }));
}

int CountActiveZombies(const VSGameState &state) {
    return static_cast<int>(std::count_if(state.zombies.begin(), state.zombies.end(), [](const VSZombieState &zombie) { return !zombie.dead && zombie.row >= 0; }));
}

int CountActiveZombieRows(const VSGameState &state) {
    int count = 0;
    for (int row = 0; row < state.rows; ++row) {
        if (CountZombiesInRow(state, row) > 0) {
            ++count;
        }
    }
    return count;
}

int CountPlantType(const VSGameState &state, SeedType seedType) {
    return static_cast<int>(
        std::count_if(state.plants.begin(), state.plants.end(), [seedType](const VSPlantState &plant) { return !IsDeadOrOutside(plant) && plant.seedType == static_cast<std::uint16_t>(seedType); }));
}

bool HasPlantTypeInRow(const VSGameState &state, SeedType seedType, int row) {
    return std::any_of(state.plants.begin(), state.plants.end(), [seedType, row](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.position.row == row && plant.seedType == static_cast<std::uint16_t>(seedType);
    });
}

bool HasActiveDeckCard(const VSGameState &state, VSSide side, SeedType seedType) {
    const std::size_t sideIndex = side == VSSide::Plants ? 0 : 1;
    return std::any_of(state.seedBanks[sideIndex].begin(), state.seedBanks[sideIndex].end(), [seedType](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(seedType);
    });
}

bool IsHeavyZombie(std::uint16_t zombieType) {
    switch (static_cast<ZombieType>(zombieType)) {
        case ZombieType::ZOMBIE_PAIL:
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_BOBSLED:
        case ZombieType::ZOMBIE_ZAMBONI:
        case ZombieType::ZOMBIE_CATAPULT:
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_WALLNUT_HEAD:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            return true;
        default:
            return false;
    }
}

bool IsFastZombie(std::uint16_t zombieType) {
    switch (static_cast<ZombieType>(zombieType)) {
        case ZombieType::ZOMBIE_BOBSLED:
        case ZombieType::ZOMBIE_ZAMBONI:
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
        case ZombieType::ZOMBIE_POLEVAULTER:
        case ZombieType::ZOMBIE_DIGGER:
        case ZombieType::ZOMBIE_IMP:
        case ZombieType::ZOMBIE_SUPER_FAN_IMP:
            return true;
        default:
            return false;
    }
}

bool IsDecisiveCounterZombie(std::uint16_t zombieType) {
    switch (static_cast<ZombieType>(zombieType)) {
        case ZombieType::ZOMBIE_BOBSLED:
        case ZombieType::ZOMBIE_ZAMBONI:
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_POLEVAULTER:
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            return true;
        default:
            return false;
    }
}

bool HasZombieTypeInRow(const VSGameState &state, int row, ZombieType zombieType) {
    return std::any_of(state.zombies.begin(), state.zombies.end(), [row, zombieType](const VSZombieState &zombie) {
        return !zombie.dead && zombie.row == row && zombie.zombieType == static_cast<std::uint16_t>(zombieType);
    });
}

bool HasMindControlledZombieInRow(const VSGameState &state, int row) {
    return std::any_of(state.zombies.begin(), state.zombies.end(), [row](const VSZombieState &zombie) { return !zombie.dead && zombie.row == row && zombie.mindControlled; });
}

bool IsMowerInMotion(const VSGameState &state, int row) {
    return row >= 0 && row < state.rows && row < static_cast<int>(state.mowerInMotion.size()) && state.mowerInMotion[static_cast<std::size_t>(row)];
}

bool HasZombieInHomeColumn(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows) {
        return false;
    }

    constexpr float kHomeColumnX = static_cast<float>(LAWN_XMIN + 80);
    return std::any_of(
        state.zombies.begin(), state.zombies.end(), [row](const VSZombieState &zombie) { return !zombie.dead && !zombie.mindControlled && zombie.row == row && zombie.positionX <= kHomeColumnX; });
}

bool IsMowerAboutToTrigger(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows || row >= static_cast<int>(state.mowerAvailable.size()) || !state.mowerAvailable[static_cast<std::size_t>(row)]) {
        return false;
    }

    // Do not feed a fresh zombie into a lane once an invader has entered the
    // first plant column. The ready mower will clear the whole lane shortly.
    return HasZombieInHomeColumn(state, row);
}

bool IsNutBypassZombieApproaching(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows) {
        return false;
    }

    constexpr float kApproachingHeavyX = 760.0f;
    return std::any_of(state.zombies.begin(), state.zombies.end(), [row](const VSZombieState &zombie) {
        if (zombie.dead || zombie.mindControlled || zombie.row != row) {
            return false;
        }
        switch (static_cast<ZombieType>(zombie.zombieType)) {
            case ZombieType::ZOMBIE_ZAMBONI:
                // An ice trail makes walls a losing response even before the
                // vehicle enters the normal close-range threshold.
                return true;
            case ZombieType::ZOMBIE_JALAPENO_HEAD:
            case ZombieType::ZOMBIE_JACK_IN_THE_BOX:
                // Both explode on contact. A nut-class barrier spends sun
                // without protecting the firing line behind it.
                return true;
            case ZombieType::ZOMBIE_GARGANTUAR:
            case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            case ZombieType::ZOMBIE_CATAPULT:
                return zombie.positionX <= kApproachingHeavyX;
            default:
                return false;
        }
    });
}

} // namespace vsai::detail
