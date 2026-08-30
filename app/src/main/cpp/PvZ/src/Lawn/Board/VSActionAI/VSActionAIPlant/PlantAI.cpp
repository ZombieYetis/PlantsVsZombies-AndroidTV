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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAITacticalRules.h"

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

bool PlantAIPlanning::IsDaytimeCoffeeMushroom(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_PUFFSHROOM:
        case SeedType::SEED_SCAREDYSHROOM:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_GLOOMSHROOM:
        case SeedType::SEED_SPORESHROOM:
        case SeedType::SEED_HYPNOSHROOM:
        case SeedType::SEED_ICESHROOM:
        case SeedType::SEED_DOOMSHROOM:
        case SeedType::SEED_MAGNETSHROOM:
            return true;
        default:
            return false;
    }
}

bool PlantAIPlanning::IsSquashClusterZombie(std::uint16_t zombieType) {
    // A bobsled team is one coordinated card and should be answered by
    // firepower or a dedicated control card, not by treating its riders
    // as an ordinary same-cell pile.
    return static_cast<ZombieType>(zombieType) != ZombieType::ZOMBIE_BOBSLED;
}

bool PlantAIPlanning::IsSquashHighValueZombie(std::uint16_t zombieType) {
    switch (static_cast<ZombieType>(zombieType)) {
        case ZombieType::ZOMBIE_DOOR:
        case ZombieType::ZOMBIE_TRASHCAN:
        case ZombieType::ZOMBIE_PAIL:
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_ZAMBONI:
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            return true;
        default:
            return false;
    }
}

int PlantAIPlanning::LargestSquashTargetStackInRow(const VSGameState &state, int row) {
    constexpr float kGridCellWidth = 80.0f;
    int largestStack = 0;
    for (const VSZombieState &anchor : state.zombies) {
        if (anchor.dead || anchor.row != row || !IsSquashClusterZombie(anchor.zombieType)) {
            continue;
        }
        int stackSize = 0;
        for (const VSZombieState &zombie : state.zombies) {
            if (zombie.dead || zombie.row != row || !IsSquashClusterZombie(zombie.zombieType)) {
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

const VSCardState *PlantAIPlanning::FindReadyCard(const VSGameState &state, SeedType seedType) const {
    for (const VSCardState &card : state.seedBanks[0]) {
        if (!IsSlotBlocked(card.slot) && card.seedType == static_cast<std::uint16_t>(seedType) && IsReadyCard(card, state.plantSun)) {
            return &card;
        }
    }
    return nullptr;
}

std::optional<VSAction> PlantAIPlanning::TryBlover(const VSGameState &state, int preferredRow) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_BLOVER);
    if (card == nullptr) {
        return std::nullopt;
    }

    int balloonRow = -1;
    float closestBalloonX = std::numeric_limits<float>::max();
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.zombieType != static_cast<std::uint16_t>(ZombieType::ZOMBIE_BALLOON)) {
            continue;
        }
        if (zombie.positionX < closestBalloonX) {
            balloonRow = zombie.row;
            closestBalloonX = zombie.positionX;
        }
    }
    if (balloonRow < 0) {
        return std::nullopt;
    }

    // Blover affects every Balloon Zombie. Its target tile is merely a
    // legal launch point, so use a rear free square instead of spending a
    // threatened frontline cell in the balloon's row.
    const VSGridPosition target = FindPlantCellInColumns(state, balloonRow >= 0 ? balloonRow : preferredRow, 0, 5);
    return target.col < 0 || target.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, target, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryEvadeJalapenoHead(const VSGameState &state) {
    const VSPlantState *bestPlant = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.mindControlled || zombie.row < 0 || zombie.row >= state.rows || zombie.zombieType != static_cast<std::uint16_t>(ZombieType::ZOMBIE_JALAPENO_HEAD)) {
            continue;
        }

        const VSPlantState *contactPlant = nullptr;
        for (const VSPlantState &plant : state.plants) {
            if (IsDeadOrOutside(plant) || plant.position.row != zombie.row) {
                continue;
            }
            if (plant.id == zombie.jalapenoContactPlantId || (zombie.jalapenoContactPlantId == 0U && plant.id == zombie.jalapenoPreContactPlantId)) {
                contactPlant = &plant;
            }
        }
        if (contactPlant == nullptr) {
            continue;
        }

        // Garlic and an awake Hypno-shroom deliberately stop this zombie's
        // burn trigger in Zombie::UpdateZombieJalapenoHead. Ash plants are
        // one-shot counters that must remain available. Keep all of them in
        // place rather than creating an opening behind the Jalapeno Head.
        const SeedType candidateSeed = static_cast<SeedType>(contactPlant->seedType);
        const bool isAshPlant = candidateSeed == SeedType::SEED_POTATOMINE || candidateSeed == SeedType::SEED_SQUASH || candidateSeed == SeedType::SEED_CHERRYBOMB
            || candidateSeed == SeedType::SEED_JALAPENO || candidateSeed == SeedType::SEED_CHILLY_PEPPER || candidateSeed == SeedType::SEED_DOOMSHROOM || candidateSeed == SeedType::SEED_ICESHROOM;
        if (candidateSeed == SeedType::SEED_GARLIC || candidateSeed == SeedType::SEED_ICEBERG_LETTUCE || (candidateSeed == SeedType::SEED_HYPNOSHROOM && !contactPlant->asleep) || isAshPlant) {
            continue;
        }

        // BuildGameState uses the engine's exact target first, then a
        // five-pixel AI-only warning window. The engine keeps its normal
        // 20-pixel burn trigger; the 15-pixel AI threshold only buys that
        // additional five pixels of shoveling time.
        const int score = PlantValueScore(*contactPlant) + static_cast<int>(contactPlant->position.col) * 10;
        if (bestPlant == nullptr || score > bestScore) {
            bestPlant = contactPlant;
            bestScore = score;
        }
    }
    return bestPlant == nullptr ? std::nullopt : std::optional<VSAction>(MakeShovelAction(bestPlant->position, state.boardTick));
}

int PlantAIPlanning::EffectivePlantPlayCost(const VSGameState &state, const VSCardState &card) const {
    const SeedType seed = static_cast<SeedType>(card.seedType);
    if (state.isNight || !IsDaytimeCoffeeMushroom(seed)) {
        return card.cost;
    }

    // A daytime mushroom is not a deployed defender until Coffee is
    // available as well. Treat the two clicks as one commitment so a
    // Spore-shroom carry is never left asleep after spending its sun.
    const VSCardState *coffee = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    return coffee == nullptr ? std::numeric_limits<int>::max() : card.cost + coffee->cost;
}

std::optional<VSAction> PlantAIPlanning::TryClearDaytimeSunshroomForPlanting(const VSGameState &state, SeedType replacementSeed, PlantPlacementRange range) {
    if (state.isNight || replacementSeed == SeedType::SEED_SUNSHROOM) {
        return std::nullopt;
    }
    range.firstColumn = std::clamp(range.firstColumn, 0, 5);
    range.lastColumn = std::clamp(range.lastColumn, range.firstColumn, 5);

    const VSPlantState *bestPad = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (int rowOffset = 0; rowOffset < (range.requireExactRow ? 1 : state.rows); ++rowOffset) {
        const int targetRow = range.requireExactRow ? range.preferredRow : (range.preferredRow + rowOffset) % state.rows;
        if (targetRow < 0 || targetRow >= state.rows || IsMowerInMotion(state, targetRow)) {
            continue;
        }
        for (const VSPlantState &plant : state.plants) {
            if (IsDeadOrOutside(plant) || plant.position.row != targetRow || plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_SUNSHROOM) || plant.position.col < range.firstColumn
                || plant.position.col > range.lastColumn) {
                continue;
            }
            // Do not clear a pad for a projectile plant that would still be
            // placed into a losing point-blank trade on the following turn.
            if (IsPlantCombatSeed(static_cast<std::uint16_t>(replacementSeed)) && !IsPlantPlacementSafe(state, replacementSeed, plant.position)) {
                continue;
            }
            const int score = static_cast<int>(plant.position.col) * 100 + (targetRow == range.preferredRow ? 25 : 0) - rowOffset;
            if (bestPad == nullptr || score > bestScore) {
                bestPad = &plant;
                bestScore = score;
            }
        }
    }
    return bestPad == nullptr ? std::nullopt : std::optional<VSAction>(MakeShovelAction(bestPad->position, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryPlantInRange(const VSGameState &state, SeedType seedType, PlantPlacementRange range) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, seedType);
    const int totalCost = card == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *card);
    if (card == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun < totalCost) {
        return std::nullopt;
    }
    if ((seedType == SeedType::SEED_WALLNUT || seedType == SeedType::SEED_TALLNUT || seedType == SeedType::SEED_PUMPKINSHELL) && IsNutBypassZombieApproaching(state, range.preferredRow)) {
        return std::nullopt;
    }
    const VSGridPosition target = seedType == SeedType::SEED_WALLNUT || seedType == SeedType::SEED_TALLNUT
        ? FindWallnutCell(state, range.preferredRow, range.firstColumn, range.lastColumn)
        : (range.requireExactRow ? FindPlantCellInExactRow(state, range.preferredRow, range.firstColumn, range.lastColumn)
                                 : FindPlantCellInColumns(state, range.preferredRow, range.firstColumn, range.lastColumn));
    if (target.col < 0 || target.row < 0) {
        return TryClearDaytimeSunshroomForPlanting(state, seedType, range);
    }
    if (target.col < 0 || target.row < 0 || (IsMowerInMotion(state, target.row) && IsPlantImmediateCounterSeed(seedType))
        || (ShouldYieldLaneToMower(state, target.row) && (!range.requireExactRow || !IsPlantImmediateCounterSeed(seedType)))) {
        return std::nullopt;
    }
    if (IsPlantCombatSeed(static_cast<std::uint16_t>(seedType)) && !IsPlantPlacementSafe(state, seedType, target)) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Plants, *card, target, state.boardTick);
}

std::optional<VSAction> PlantAIPlanning::TryPlant(const VSGameState &state, SeedType seedType, int row, int firstColumn, int lastColumn) {
    return PlantAIPlanning::TryPlantInRange(state, seedType, {.preferredRow = row, .firstColumn = firstColumn, .lastColumn = lastColumn});
}

std::optional<VSAction> PlantAIPlanning::TryPlantExactRow(const VSGameState &state, SeedType seedType, int row, int firstColumn, int lastColumn) {
    return PlantAIPlanning::TryPlantInRange(state, seedType, {.preferredRow = row, .firstColumn = firstColumn, .lastColumn = lastColumn, .requireExactRow = true});
}

std::optional<VSAction> PlantAIPlanning::TryRemoveLadderedNut(const VSGameState &state) {
    const VSPlantState *bestPlant = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || (plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_WALLNUT) && plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_TALLNUT))) {
            continue;
        }
        const bool laddered = std::any_of(state.gridItems.begin(), state.gridItems.end(), [&plant](const VSGridItemState &item) {
            return !item.dead && item.gridItemType == static_cast<std::uint16_t>(GridItemType::GRIDITEM_LADDER) && item.position.row == plant.position.row && item.position.col == plant.position.col;
        });
        if (!laddered) {
            continue;
        }

        // Remove the most valuable/most exposed laddered barrier first;
        // keeping an unladdered nut is still useful as a temporary wall.
        int score = PlantValueScore(plant) + static_cast<int>(plant.position.col) * 12;
        score += CountZombiesInRow(state, plant.position.row) * 35;
        if (bestPlant == nullptr || score > bestScore) {
            bestPlant = &plant;
            bestScore = score;
        }
    }
    return bestPlant == nullptr ? std::nullopt : std::optional<VSAction>(MakeShovelAction(bestPlant->position, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryCounterPlant(const VSGameState &state, SeedType seedType, int row, int firstColumn) {
    // Once the planner has deliberately yielded a ready-mower lane, an
    // immediate one-shot in that route cannot recover the sunk investment.
    // Keep Squash/Imp Pear for a lane that still needs to be held.
    if (ShouldYieldLaneToMower(state, row)) {
        return std::nullopt;
    }
    const VSZombieState *closest = FindClosestZombie(state, row);
    if (closest == nullptr) {
        return std::nullopt;
    }
    const int targetColumn = std::clamp(static_cast<int>((closest->positionX - static_cast<float>(LAWN_XMIN)) / 80.0f), 0, 5);
    // Answer the zombie's current cell first.  The old right-to-left
    // search put a late Squash or Imp Pear behind a third-column zombie.
    for (int column = targetColumn; column >= firstColumn; --column) {
        if (std::optional<VSAction> action = PlantAIPlanning::TryPlantExactRow(state, seedType, row, column, column)) {
            return action;
        }
    }
    for (int column = std::max(firstColumn, targetColumn + 1); column <= 5; ++column) {
        if (std::optional<VSAction> action = PlantAIPlanning::TryPlantExactRow(state, seedType, row, column, column)) {
            return action;
        }
    }
    return std::nullopt;
}


int PlantAIPlanning::ZombieColumn(const VSZombieState &zombie) {
    return std::clamp(static_cast<int>((zombie.positionX - static_cast<float>(LAWN_XMIN)) / 80.0f), 0, 5);
}

int PlantAIPlanning::ZombieEffectiveHealth(const VSZombieState &zombie) {
    return ZombieEffectiveThreatHealth(zombie);
}

bool PlantAIPlanning::IsHypnoshroomTarget(const VSZombieState &zombie) {
    switch (static_cast<ZombieType>(zombie.zombieType)) {
        case ZombieType::ZOMBIE_DOOR:
        case ZombieType::ZOMBIE_TRASHCAN:
        case ZombieType::ZOMBIE_PAIL:
        case ZombieType::ZOMBIE_NEWSPAPER:
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_WALLNUT_HEAD:
        case ZombieType::ZOMBIE_TALLNUT_HEAD:
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            return true;
        default:
            return false;
    }
}

int PlantAIPlanning::PotatoMineArmingLead(const VSZombieState &zombie) {
    // Three cells provide enough runway for normal and heavy walkers. Fast
    // units get only one extra cell rather than pushing a Mine into the rear
    // formation, where it no longer protects the threatened firing lane.
    constexpr int kThreeCells = 3 * 80;
    return IsFastZombie(zombie.zombieType) ? kThreeCells + 80 : kThreeCells;
}

PlantAIPlanning::AshTarget PlantAIPlanning::FindBestAshTarget(const VSGameState &state, SeedType seedType) const {
    const int rowRadius = seedType == SeedType::SEED_DOOMSHROOM ? 2 : ((seedType == SeedType::SEED_CHERRYBOMB) ? 1 : 0);
    const int columnRadius = seedType == SeedType::SEED_DOOMSHROOM ? 2 : ((seedType == SeedType::SEED_CHERRYBOMB) ? 1 : 0);
    AshTarget bestTarget;
    for (int row = 0; row < state.rows; ++row) {
        for (int column = 0; column < 6; ++column) {
            const VSGridPosition target{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (!IsPlantableVSTile(state, target) || HasPlantAt(state, target) || HasGridItemAt(state, target)) {
                continue;
            }
            if (!state.isNight && (seedType == SeedType::SEED_ICESHROOM || seedType == SeedType::SEED_DOOMSHROOM)) {
                const int zombiesOnPlantCell = static_cast<int>(std::count_if(state.zombies.begin(), state.zombies.end(), [&target](const VSZombieState &zombie) {
                    return !zombie.dead && !zombie.mindControlled && zombie.row == target.row && PlantAIPlanning::ZombieColumn(zombie) == target.col;
                }));
                // Daytime burst mushrooms need a second click for Coffee.
                // Do not plant into a double chew stack that will consume the
                // sleeping mushroom before that follow-up action can happen.
                if (zombiesOnPlantCell >= 2) {
                    continue;
                }
            }

            AshTarget candidate;
            candidate.position = target;
            candidate.score = 0;
            candidate.mowerlessThirdColumn = IsMowerlessThirdColumnEmergency(state, row);
            for (const VSZombieState &zombie : state.zombies) {
                if (zombie.dead || zombie.mindControlled) {
                    continue;
                }
                if (zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_BUNGEE) && !zombie.bungeeAtTarget) {
                    continue;
                }
                const int zombieColumn = PlantAIPlanning::ZombieColumn(zombie);
                const bool hitsWholeRow = (seedType == SeedType::SEED_JALAPENO || seedType == SeedType::SEED_CHILLY_PEPPER) && zombie.row == row;
                const bool hitsArea = std::abs(static_cast<int>(zombie.row) - row) <= rowRadius && std::abs(zombieColumn - column) <= columnRadius;
                if (!hitsWholeRow && !hitsArea) {
                    continue;
                }
                if (seedType == SeedType::SEED_SQUASH && !IsSquashTargetZombie(zombie)) {
                    continue;
                }
                if (seedType == SeedType::SEED_CHILLY_PEPPER && !CanChillyPepperAffect(zombie)) {
                    // Chilly Pepper only damages zombies which the engine
                    // can freeze at this instant. This excludes Zomboni,
                    // intact Bobsleds, airborne units and transient phases
                    // such as a tunneling Digger, preventing fake row hits.
                    continue;
                }

                const int health = PlantAIPlanning::ZombieEffectiveHealth(zombie);
                ++candidate.hitCount;
                candidate.totalHealth += health;
                candidate.highValueCount += PlantAIPlanning::IsSquashHighValueZombie(zombie.zombieType) ? 1 : 0;
                const ZombieType zombieType = static_cast<ZombieType>(zombie.zombieType);
                candidate.giantCount += zombieType == ZombieType::ZOMBIE_GARGANTUAR || zombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR;
                candidate.pailCount += zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_PAIL) ? 1 : 0;
                candidate.frontMostX = std::min(candidate.frontMostX, zombie.positionX);
                candidate.mowerlessHomeColumn = candidate.mowerlessHomeColumn || (candidate.mowerlessThirdColumn && zombieColumn == 0);
                candidate.score += ZombieThreatWeight(zombie.zombieType) * 6 + health / 3;
                candidate.score += zombie.eating ? 260 : 0;
                candidate.score += std::clamp((LAWN_XMIN + 6 * 80 - static_cast<int>(zombie.positionX)) / 3, 0, 220);
            }
            candidate.score += candidate.hitCount * 230 + candidate.highValueCount * 190;
            candidate.score += candidate.mowerlessThirdColumn ? 1200 : 0;
            // A mowerless zombie in column zero is the final possible
            // response window. Its Ash target must outrank every ordinary
            // cluster on the rest of the lawn.
            candidate.score += candidate.mowerlessHomeColumn ? 4000 : 0;
            // Replay priors only break ties between already legal blast
            // cells. They never make a weak Ash target pass the separate
            // health/count threshold below.
            candidate.score += StrategyBonus(state, VSSide::Plants, seedType, row);
            candidate.score += ZombieDeckCounterBonus(state, seedType, row);
            if (candidate.hitCount == 0) {
                continue;
            }
            if (candidate.score > bestTarget.score || (candidate.score == bestTarget.score && candidate.frontMostX < bestTarget.frontMostX)) {
                bestTarget = candidate;
            }
        }
    }
    return bestTarget;
}

bool PlantAIPlanning::IsAshTargetWorthPlaying(const VSGameState &state, SeedType seedType, const AshTarget &target) {
    if (target.hitCount <= 0) {
        return false;
    }
    const bool hasReachedThirdColumn = target.frontMostX < static_cast<float>(LAWN_XMIN + 3 * 80);
    const bool panic = target.mowerlessThirdColumn && hasReachedThirdColumn;
    if (target.mowerlessHomeColumn) {
        // The first plant column without a mower is a loss on the next
        // contact. Do not keep a ready Ash card for a better cluster.
        return true;
    }
    switch (seedType) {
        case SeedType::SEED_SQUASH: {
            // A lone early Buckethead is the exception to the usual
            // multi-target Squash rule. If the lane has no meaningful
            // repeatable fire, waiting for a cluster loses the economy.
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, target.position.row);
            const bool earlyUnansweredBucket = target.hitCount == 1 && target.pailCount == 1 && target.totalHealth >= 400 && target.frontMostX < 780.0f && (firepower.dps < 35 || !firepower.canHold);
            return earlyUnansweredBucket || (target.hitCount >= 2 && target.totalHealth >= 420)
                || (target.highValueCount > 0 && target.totalHealth >= (panic ? 260 : 480) && (target.frontMostX < 660.0f || panic));
        }
        case SeedType::SEED_CHERRYBOMB:
            return target.hitCount >= (panic ? 1 : 2) && target.totalHealth >= (panic ? 320 : 500);
        case SeedType::SEED_JALAPENO:
            return target.hitCount >= (panic ? 1 : 3) && target.totalHealth >= (panic ? 360 : 650);
        case SeedType::SEED_CHILLY_PEPPER: {
            // Chilly Pepper has a 100-tick wind-up.  A current snapshot can
            // contain valid targets which the existing firing line will
            // remove before its row blast occurs, so do not spend it on an
            // already-resolved fight unless this is the mowerless last call.
            if (target.mowerlessHomeColumn) {
                return true;
            }
            constexpr int kChillyWindupSeconds = 2;
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, target.position.row);
            const int survivingHealth = target.totalHealth - firepower.dps * kChillyWindupSeconds;
            return survivingHealth > 0 && target.hitCount >= (panic ? 1 : 3) && survivingHealth >= (panic ? 360 : 650);
        }
        case SeedType::SEED_DOOMSHROOM:
            return target.hitCount >= (panic ? 2 : 4) && target.totalHealth >= (panic ? 600 : 1100);
        default:
            return false;
    }
}


bool IsLobbedOutputSeed(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_WINTERMELON:
        case SeedType::SEED_COBCANNON:
        case SeedType::SEED_SPORESHROOM:
            return true;
        default:
            return false;
    }
}

std::optional<VSAction> PlantAIPlanning::TryUmbrellaDefense(const VSGameState &state, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_UMBRELLA);
    if (card == nullptr || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }

    bool hasRaidThreat = false;
    for (const VSCardState &zombieCard : state.seedBanks[1]) {
        if (!zombieCard.matchRestricted && zombieCard.active
            && (zombieCard.seedType == static_cast<std::uint16_t>(SeedType::SEED_ZOMBIE_BUNGEE) || zombieCard.seedType == static_cast<std::uint16_t>(SeedType::SEED_ZOMBIE_CATAPULT))) {
            hasRaidThreat = true;
            break;
        }
    }
    if (!hasRaidThreat) {
        hasRaidThreat = std::any_of(state.zombies.begin(), state.zombies.end(), [](const VSZombieState &zombie) {
            const ZombieType type = static_cast<ZombieType>(zombie.zombieType);
            return !zombie.dead && !zombie.mindControlled && (type == ZombieType::ZOMBIE_BUNGEE || type == ZombieType::ZOMBIE_CATAPULT);
        });
    }
    if (!hasRaidThreat) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = 0;
    for (int row = 0; row < state.rows; ++row) {
        for (int column = 0; column < 6; ++column) {
            const VSGridPosition target{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (!IsPlantableVSTile(state, target) || HasPlantAt(state, target) || HasGridItemAt(state, target) || IsPlantProtectedByUmbrella(state, target)) {
                continue;
            }

            int protectedValue = 0;
            int protectedPlants = 0;
            for (const VSPlantState &plant : state.plants) {
                if (IsDeadOrOutside(plant) || IsPlantOneShotSeed(static_cast<SeedType>(plant.seedType)) || IsPlantProtectedByUmbrella(state, plant.position)
                    || std::abs(static_cast<int>(plant.position.col) - column) > 1 || std::abs(static_cast<int>(plant.position.row) - row) > 1) {
                    continue;
                }
                const int value = PlantValueScore(plant);
                if (value >= 100) {
                    protectedValue += value;
                    ++protectedPlants;
                }
            }

            // Bungee or Catapult availability alone is not a placement
            // reason. The umbrella must cover a meaningful unprotected
            // investment and never overlap an existing umbrella radius.
            const int score = protectedValue + protectedPlants * 90 - std::abs(column - 2) * 12;
            if ((protectedPlants >= 2 || protectedValue >= 180) && score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick));
}

std::unique_ptr<IVSAgent> CreatePlantAI() {
    return std::make_unique<PlantAI>();
}

} // namespace vsai::detail
