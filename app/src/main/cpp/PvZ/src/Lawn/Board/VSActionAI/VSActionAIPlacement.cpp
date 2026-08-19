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

#include "VSActionAIPlacement.h"

#include "VSActionAILanePolicy.h"

#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Common/GameConstants.h"

#include <cmath>
#include <algorithm>
#include <array>
#include <limits>

namespace vsai::detail {

namespace {

    bool ReservesRearColumnForCarry(const VSGameState &state) {
        for (const VSCardState &card : state.seedBanks[0]) {
            switch (static_cast<SeedType>(card.seedType)) {
                case SeedType::SEED_SCAREDYSHROOM:
                case SeedType::SEED_SNOWPEA:
                case SeedType::SEED_REPEATER:
                case SeedType::SEED_BLOOMERANG:
                case SeedType::SEED_THREEPEATER:
                case SeedType::SEED_STARFRUIT:
                case SeedType::SEED_CABBAGEPULT:
                case SeedType::SEED_KERNELPULT:
                case SeedType::SEED_MELONPULT:
                case SeedType::SEED_WINTERMELON:
                case SeedType::SEED_SPORESHROOM:
                    return true;
                default:
                    break;
            }
        }
        return false;
    }

    int IncomeColumnFor(const VSGameState &state, int index) {
        static constexpr int kDefaultColumns[] = {0, 1, 2};
        static constexpr int kRearCarryColumns[] = {1, 2, 0};
        return (ReservesRearColumnForCarry(state) ? kRearCarryColumns : kDefaultColumns)[index];
    }

    int IncomeOpeningRowPriority(int rows, int row) {
        // The recordings open on three separated rows before filling the two
        // middle lanes. This keeps one cheap probe from immediately chewing
        // through every early Sunflower in a single line.
        if (rows == 5) {
            static constexpr int kFiveRowOrder[] = {2, 0, 4, 1, 3};
            for (int priority = 0; priority < 5; ++priority) {
                if (kFiveRowOrder[priority] == row) {
                    return priority;
                }
            }
        }
        if (rows == 6) {
            static constexpr int kSixRowOrder[] = {2, 3, 0, 5, 1, 4};
            for (int priority = 0; priority < 6; ++priority) {
                if (kSixRowOrder[priority] == row) {
                    return priority;
                }
            }
        }
        return row;
    }

} // namespace

bool IsPlantableVSTile(const VSGameState &state, VSGridPosition position) {
    return position.row >= 0 && position.row < state.rows && position.row < static_cast<int>(state.basePlantableCells.size()) && position.col >= 0 && position.col < 6
        && state.basePlantableCells[static_cast<std::size_t>(position.row)][static_cast<std::size_t>(position.col)];
}

VSGridPosition FindPlantCellInColumns(const VSGameState &state, int preferredRow, int firstColumn, int lastColumn) {
    preferredRow = std::clamp(preferredRow, 0, std::max(0, state.rows - 1));
    firstColumn = std::clamp(firstColumn, 0, 5);
    lastColumn = std::clamp(lastColumn, firstColumn, 5);
    for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
        const int row = (preferredRow + rowOffset) % state.rows;
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (IsPlantableVSTile(state, position) && !HasPlantAt(state, position) && !HasGridItemAt(state, position)) {
                return position;
            }
        }
    }
    return {};
}

VSGridPosition FindPlantCellInExactRow(const VSGameState &state, int row, int firstColumn, int lastColumn) {
    if (row < 0 || row >= state.rows) {
        return {};
    }
    firstColumn = std::clamp(firstColumn, 0, 5);
    lastColumn = std::clamp(lastColumn, firstColumn, 5);
    for (int column = firstColumn; column <= lastColumn; ++column) {
        const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
        if (IsPlantableVSTile(state, position) && !HasPlantAt(state, position) && !HasGridItemAt(state, position)) {
            return position;
        }
    }
    return {};
}

namespace {

    float PlantCellCenterX(VSGridPosition position) {
        return static_cast<float>(LAWN_XMIN + static_cast<int>(position.col) * 80 + 40);
    }

    bool IsMeleePlant(SeedType seed) {
        return seed == SeedType::SEED_BONK_CHOY || seed == SeedType::SEED_CELERY_STALKER || seed == SeedType::SEED_CHOMPER;
    }

} // namespace

bool IsPlantPlacementSafe(const VSGameState &state, SeedType seed, VSGridPosition position) {
    if (!IsPlantableVSTile(state, position)) {
        return false;
    }
    // A dead zombie target makes this route unwinnable for the zombie side.
    // Keep it available for Sunflowers, but never consume its empty cells
    // with a continuous attacker that can no longer pressure an economy.
    if (IsSustainedOutputSeed(seed) && !HasLiveZombieTargetInRow(state, position.row)) {
        return false;
    }
    // Instant counters and support/defensive overlays intentionally target an
    // occupied or threatened cell. Their callers have separate target rules.
    const bool isIncomePlant = seed == SeedType::SEED_SUNFLOWER || seed == SeedType::SEED_TWINSUNFLOWER || seed == SeedType::SEED_SUNSHROOM;
    if (!isIncomePlant && !IsPlantCombatSeed(static_cast<std::uint16_t>(seed)) && !IsSustainedOutputSeed(seed)) {
        return true;
    }

    const float cellCenterX = PlantCellCenterX(position);
    const float minimumGap = isIncomePlant ? 180.0f : (IsMeleePlant(seed) ? 45.0f : ((seed == SeedType::SEED_SPIKEWEED || seed == SeedType::SEED_SPIKEROCK) ? 60.0f : 125.0f));
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != position.row) {
            continue;
        }
        const float gap = zombie.positionX - cellCenterX;
        // Zombies move toward decreasing X. Once they are at or behind the
        // candidate cell, the plant is being offered directly to them.
        if (gap < minimumGap || (zombie.eating && gap < minimumGap + 100.0f)) {
            return false;
        }
    }
    return true;
}

VSGridPosition FindPuffshroomCell(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows) {
        return {};
    }

    const VSZombieState *closest = FindClosestZombie(state, row);
    for (int column = 5; column >= 2; --column) {
        const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
        if (!IsPlantableVSTile(state, position) || HasPlantAt(state, position) || HasGridItemAt(state, position)) {
            continue;
        }
        if (closest != nullptr) {
            // Puff-shroom attacks from its front edge for roughly 230 pixels.
            // Put it in the foremost safe cell that can actually engage the
            // approaching zombie instead of prebuilding a rear short-range gun.
            const float attackEndX = static_cast<float>(LAWN_XMIN + column * 80 + 60 + 230);
            if (closest->positionX > attackEndX) {
                continue;
            }
        } else if (column > 4) {
            // With no live target, leave the red-line cell available for a
            // blocker while still keeping the Puff forward enough to pressure.
            continue;
        }
        if (IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, position)) {
            return position;
        }
    }
    return {};
}

bool IsRangedOutputTradeUnfavorable(const VSGameState &state, int row) {
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row) {
            continue;
        }
        // A door or trashcan is designed to win a straight projectile trade.
        // Put new repeatable fire on a different grave route and use a real
        // answer for this lane when it becomes urgent.
        switch (static_cast<ZombieType>(zombie.zombieType)) {
            case ZombieType::ZOMBIE_TRASHCAN:
            case ZombieType::ZOMBIE_DOOR:
            case ZombieType::ZOMBIE_WALLNUT_HEAD:
            case ZombieType::ZOMBIE_TALLNUT_HEAD:
                return true;
            default:
                break;
        }
        if (zombie.eating || zombie.positionX < 640.0f) {
            return true;
        }
    }
    return false;
}

VSGridPosition FindSafeIncomeCell(const VSGameState &state, int preferredRow) {
    preferredRow = std::clamp(preferredRow, 0, std::max(0, state.rows - 1));
    std::array<int, 6> rowIncome{};
    int incomeCount = 0;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || !IsPlantEconomySeed(state, plant.seedType)) {
            continue;
        }
        ++incomeCount;
        if (plant.position.row >= 0 && plant.position.row < state.rows) {
            ++rowIncome[static_cast<std::size_t>(plant.position.row)];
        }
    }

    // Open three separated rear-lane flowers, then move one column forward
    // instead of waiting to fill every row in a single exposed column. This
    // matches the replay openings and leaves the early probe routes distinct.
    const int openingColumnCapacity = std::min(3, std::max(1, state.rows));
    const int phase = std::min(2, incomeCount / openingColumnCapacity);
    std::array<int, 3> columnOrder{phase, (phase + 1) % 3, (phase + 2) % 3};
    if (phase == 2) {
        columnOrder = {2, 1, 0};
    }

    // Once the zombie-side target has fallen, this lane cannot receive any
    // more meaningful zombie or grave investment. Reuse an empty safe tile
    // there before spending on a contested lane; the target survives in the
    // GridItem array through its death animation, so health zero is enough.
    for (const int columnIndex : columnOrder) {
        const int column = IncomeColumnFor(state, columnIndex);
        for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
            const int row = (preferredRow + rowOffset) % state.rows;
            if (!HasDestroyedZombieTargetInRow(state, row)) {
                continue;
            }
            const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (IsPlantableVSTile(state, position) && !HasPlantAt(state, position) && !HasGridItemAt(state, position) && IsPlantPlacementSafe(state, SeedType::SEED_SUNFLOWER, position)) {
                return position;
            }
        }
    }

    for (const int columnIndex : columnOrder) {
        const int column = IncomeColumnFor(state, columnIndex);
        int bestRow = -1;
        int bestRowScore = std::numeric_limits<int>::max();
        for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
            const int row = (preferredRow + rowOffset) % state.rows;
            const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (!IsPlantableVSTile(state, position) || HasPlantAt(state, position) || HasGridItemAt(state, position) || !IsPlantPlacementSafe(state, SeedType::SEED_SUNFLOWER, position)) {
                continue;
            }
            const int score = rowIncome[static_cast<std::size_t>(row)] * 100 + IncomeOpeningRowPriority(state.rows, row) * 5 + rowOffset;
            if (bestRow < 0 || score < bestRowScore) {
                bestRow = row;
                bestRowScore = score;
            }
        }
        if (bestRow >= 0) {
            return {static_cast<std::int8_t>(column), static_cast<std::int8_t>(bestRow)};
        }
    }
    return {};
}

VSGridPosition FindIcebergLettuceCell(const VSGameState &state, int row) {
    const VSZombieState *closest = FindClosestZombie(state, row);
    if (closest == nullptr) {
        return {};
    }
    const int closestColumn = std::clamp(static_cast<int>((closest->positionX - static_cast<float>(LAWN_XMIN)) / 80.0f), 0, 5);
    int frontNutColumn = -1;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row
            || (plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_WALLNUT) && plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_TALLNUT))) {
            continue;
        }
        frontNutColumn = std::max(frontNutColumn, static_cast<int>(plant.position.col));
    }
    // The first candidate is the zombie's actual foot cell. Only use a
    // neighbouring cell when the exact one is already occupied.
    for (const int offset : {0, -1, 1, -2, 2}) {
        const int column = closestColumn + offset;
        if (column < 0 || column > 5) {
            continue;
        }
        // The nut is the front barrier. A lettuce behind it is not exposed
        // to the zombie's feet and wastes the freeze; if the zombie has
        // already crossed the nut, defer instead of freezing the rear side.
        if (frontNutColumn >= 0 && column < frontNutColumn) {
            continue;
        }
        const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
        if (IsPlantableVSTile(state, position) && !HasPlantAt(state, position) && !HasGridItemAt(state, position)) {
            return position;
        }
    }
    return {};
}

VSGridPosition FindWallnutCell(const VSGameState &state, int row, int firstColumn, int lastColumn) {
    if (row < 0 || row >= state.rows) {
        return {};
    }
    // Giants flatten walls, Zamboni destroys them and leaves an ice trail,
    // and a close Catapult shoots over the investment. Save nut-class cards
    // for a lane where they can actually establish a front line.
    if (IsNutBypassZombieApproaching(state, row)) {
        return {};
    }
    firstColumn = std::clamp(firstColumn, 0, 5);
    lastColumn = std::clamp(lastColumn, firstColumn, 5);
    const VSZombieState *closest = FindClosestZombie(state, row);
    if (closest == nullptr) {
        return {};
    }
    // Zombies travel from right to left. A Wall-nut may be planted in the
    // zombie's current cell to intercept immediately, or in a cell it has
    // not reached yet. Planting to its right would leave the wall behind it.
    const int closestColumn = std::clamp(static_cast<int>((closest->positionX - static_cast<float>(LAWN_XMIN)) / 80.0f), 0, 5);
    for (int column = std::min(lastColumn, closestColumn); column >= firstColumn; --column) {
        const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
        if (!IsPlantableVSTile(state, position) || HasPlantAt(state, position) || HasGridItemAt(state, position)) {
            continue;
        }
        return position;
    }
    return {};
}

VSGridPosition FindSpikeweedCell(const VSGameState &state, int row) {
    if (row < 0 || row >= state.rows || IsMowerInMotion(state, row) || IsMowerAboutToTrigger(state, row)) {
        return {};
    }

    const VSPlantState *frontBarrier = nullptr;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row
            || (plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_WALLNUT) && plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_TALLNUT)
                && plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_PUMPKINSHELL))) {
            continue;
        }
        if (frontBarrier == nullptr || plant.position.col > frontBarrier->position.col) {
            frontBarrier = &plant;
        }
    }

    const VSZombieState *zamboni = nullptr;
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.row != row || zombie.zombieType != static_cast<std::uint16_t>(ZombieType::ZOMBIE_ZAMBONI)) {
            continue;
        }
        if (zamboni == nullptr || zombie.positionX < zamboni->positionX) {
            zamboni = &zombie;
        }
    }
    if (zamboni != nullptr && zamboni->positionX <= 760.0f) {
        const int zamboniColumn = std::clamp(static_cast<int>((zamboni->positionX - static_cast<float>(LAWN_XMIN)) / 80.0f), 0, 5);
        const int barrierColumn = frontBarrier == nullptr ? -1 : static_cast<int>(frontBarrier->position.col);
        // The tile one step along the vehicle's travel path is the front
        // response. Never spend Spikeweed behind a Wall-nut or Pumpkin just
        // because the Zomboni's ice trail made the correct front tile fail.
        for (const int offset : {-1, -2}) {
            const int column = zamboniColumn + offset;
            // Spikeweed is a front-line control card. Once the Zomboni has
            // crossed beyond the two front tiles, do not chase it back into
            // the plant formation; leave that response to instant counters.
            if (column <= barrierColumn || column < 4) {
                continue;
            }
            const VSGridPosition target{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (IsPlantableVSTile(state, target) && !HasPlantAt(state, target) && !HasGridItemAt(state, target)) {
                return target;
            }
        }
    }

    const VSZombieState *closest = FindClosestZombie(state, row);
    if (closest == nullptr || closest->positionX > 760.0f) {
        return {};
    }

    // Do not derive the target from a rear Wall-nut. In VS that placed
    // Spikeweed in column 3 whenever the nut was in column 2. Its useful
    // position is at the redline, so only consider columns 5 and 4. If both
    // are unavailable, wait for the next legal front-line opportunity.
    for (int column = 5; column >= 4; --column) {
        const VSGridPosition target{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
        const float cellCenterX = PlantCellCenterX(target);
        const bool willBeCrossed = std::any_of(state.zombies.begin(), state.zombies.end(), [row, cellCenterX](const VSZombieState &zombie) {
            return !zombie.dead && !zombie.mindControlled && zombie.row == row && zombie.positionX >= cellCenterX - 20.0f;
        });
        if (willBeCrossed && IsPlantableVSTile(state, target) && !HasPlantAt(state, target) && !HasGridItemAt(state, target) && IsPlantPlacementSafe(state, SeedType::SEED_SPIKEWEED, target)) {
            return target;
        }
    }
    return {};
}

int ZombiePlacementColumn(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_ZOMBIE_DANCER:
            // The Dancer needs room to summon its line of backups.
            return 7;
        case SeedType::SEED_ZOMBIE_CATAPULT:
        case SeedType::SEED_ZOMBIE_BALLOON:
            // Ranged/flying pressure is more useful when it starts protected.
            return 8;
        default:
            // Plants occupy 0..5; column 6 is the zombie front line.
            return 6;
    }
}

VSGridPosition FindZombieCell(const VSGameState &state, SeedType seed, int row) {
    row = std::clamp(row, 0, std::max(0, state.rows - 1));
    if (EvaluateZombieLanePolicy(state, row).deploymentBlocked) {
        return {};
    }
    return {static_cast<std::int8_t>(ZombiePlacementColumn(seed)), static_cast<std::int8_t>(row)};
}

bool CanInvestZombieEconomyInRow(const VSGameState &state, int row) {
    return EvaluateZombieLanePolicy(state, row).allowsEconomy;
}

VSGridPosition FindZombieEconomyCell(const VSGameState &state, int preferredRow) {
    preferredRow = std::clamp(preferredRow, 0, std::max(0, state.rows - 1));
    auto HasBlockingGridItem = [&](VSGridPosition position) {
        return std::any_of(state.gridItems.begin(), state.gridItems.end(), [position](const VSGridItemState &item) {
            // VS creates a target marker at column 8 for every row. The
            // engine permits a grave on that marker, so it must not consume
            // the back-column economy slot in the AI's board snapshot.
            return !item.dead && item.position.col == position.col && item.position.row == position.row && item.gridItemType != static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE);
        });
    };
    for (int column = 8; column >= 6; --column) {
        for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
            const int row = (preferredRow + rowOffset) % state.rows;
            if (!CanInvestZombieEconomyInRow(state, row)) {
                continue;
            }
            const VSGridPosition position{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (!HasPlantAt(state, position) && !HasBlockingGridItem(position)) {
                return position;
            }
        }
    }
    return {};
}

VSGridPosition FindZombieMoundCell(const VSGameState &state, int row) {
    if (!CanInvestZombieEconomyInRow(state, row)) {
        return {};
    }
    const VSGridItemState *bestItem = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || item.position.row != row) {
            continue;
        }

        const int score = MoundUpgradePriorityAt(state, item.position);
        if (score > bestScore) {
            bestItem = &item;
            bestScore = score;
        }
    }
    return bestItem == nullptr || bestScore <= 0 ? VSGridPosition{} : bestItem->position;
}

int MoundUpgradeCostAt(const VSGameState &state, VSGridPosition position) {
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || item.position.col != position.col || item.position.row != position.row) {
            continue;
        }
        if (item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_GRAVESTONE)) {
            // Converting a basic grave creates level zero, which still uses
            // the base mound card cost.
            return 75;
        }
        if (item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_BURIAL_MOUND)) {
            switch (item.level) {
                case 0:
                    return 150;
                case 1:
                    return 225;
                case 2:
                    return 300;
                case 3:
                    return 450;
                default:
                    return std::numeric_limits<int>::max();
            }
        }
    }
    return std::numeric_limits<int>::max();
}

int MoundUpgradePriorityAt(const VSGameState &state, VSGridPosition position) {
    if (!CanInvestZombieEconomyInRow(state, position.row)) {
        return std::numeric_limits<int>::min();
    }
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || item.position.col != position.col || item.position.row != position.row) {
            continue;
        }

        int extraBrains = 0;
        if (item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_GRAVESTONE)) {
            // Basic grave -> level 0 adds one small brain.
            extraBrains = 25;
        } else if (item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_BURIAL_MOUND)) {
            switch (item.level) {
                case 0:
                case 2:
                    // Level 0 -> 1 and 2 -> 3 each add a full brain.
                    extraBrains = 50;
                    break;
                case 1:
                    // Level 1 -> 2 adds a small brain.
                    extraBrains = 25;
                    break;
                default:
                    // Level 3 -> 4 only improves durability. It is not an
                    // income purchase while cheaper productive upgrades exist.
                    return std::numeric_limits<int>::min();
            }
        } else {
            return std::numeric_limits<int>::min();
        }

        const int upgradeCost = MoundUpgradeCostAt(state, position);
        return upgradeCost == std::numeric_limits<int>::max() ? std::numeric_limits<int>::min() : extraBrains * 12 - upgradeCost;
    }
    return std::numeric_limits<int>::min();
}

bool IsReadyCard(const VSCardState &card, int resource);

bool IsCardReadyForZombieTarget(const VSCardState &card, const VSGameState &state, VSGridPosition target) {
    if (card.seedType != static_cast<std::uint16_t>(SeedType::SEED_ZOMBIE_MOUND)) {
        return IsReadyCard(card, state.zombieBrains);
    }
    return !card.matchRestricted && card.active && !card.refreshing && card.refreshCounter <= 0 && MoundUpgradeCostAt(state, target) <= state.zombieBrains;
}

int CountZombieEconomy(const VSGameState &state) {
    return static_cast<int>(std::count_if(state.gridItems.begin(), state.gridItems.end(), [](const VSGridItemState &item) {
        return !item.dead && (item.gridItemType == GridItemType::GRIDITEM_GRAVESTONE || item.gridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND);
    }));
}

int HeavyZombieEconomyThreshold(const VSGameState &state) {
    // The zombie economy occupies its three rear columns.  Keep at most two
    // positions open before committing to an expensive game-ending zombie.
    const int economyTarget = std::max(0, state.rows * 3);
    return std::max(0, economyTarget - std::min(2, std::max(0, state.rows - 1)));
}

int StrategyBucket(int value) {
    return value <= 0 ? 0 : value <= 2 ? 1 : value <= 4 ? 2 : 3;
}

int CountLivePlants(const VSGameState &state) {
    return static_cast<int>(std::count_if(state.plants.begin(), state.plants.end(), [](const VSPlantState &plant) { return !IsDeadOrOutside(plant); }));
}

int CountPlantIncome(const VSGameState &state) {
    return CountPlantType(state, SeedType::SEED_SUNFLOWER) + (state.isNight ? CountPlantType(state, SeedType::SEED_SUNSHROOM) : 0);
}

} // namespace vsai::detail
