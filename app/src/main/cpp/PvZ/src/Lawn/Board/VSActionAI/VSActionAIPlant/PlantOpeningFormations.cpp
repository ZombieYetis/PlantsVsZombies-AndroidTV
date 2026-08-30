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

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {
std::optional<VSAction> PlantAIPlanning::TryBoomerangControlPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // Replay tactics match the cards that define their formation, rather
    // than a full recorded deck. Counter and economy slots may be Ban
    // replacements without disabling the carry's normal opening.
    const bool boomerangControlTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_BLOOMERANG);
    if (!boomerangControlTemplate || EffectivePlantEconomyCount(state) < 4 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const VSCardState *boomerang = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_BLOOMERANG);
    const int totalCost = boomerang == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *boomerang);
    if (boomerang == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    const int firingLineTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 3));
    if (CountPlantType(state, SeedType::SEED_BLOOMERANG) >= firingLineTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_BLOOMERANG, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_BLOOMERANG, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_BLOOMERANG, target)) {
            continue;
        }

        const VSZombieState *closest = FindClosestZombie(state, row);
        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_BLOOMERANG, row) * 9;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 12;
        score += closest == nullptr ? 70 : (closest->positionX > 630.0f ? 95 : -130);
        score += lane.danger < 105 ? 80 : -110;
        score += row == preferredRow ? 40 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_BLOOMERANG, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *boomerang, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryBoomerangGarlicFormation(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool boomerangGarlicTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_BLOOMERANG) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_GARLIC);
    if (!boomerangGarlicTemplate || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const int income = EffectivePlantEconomyCount(state);
    const VSCardState *boomerang = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_BLOOMERANG);
    const int boomerangCost = boomerang == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *boomerang);
    const int firingTarget = std::min(state.rows, std::max(1, income - 2));
    if (boomerang != nullptr && boomerangCost != std::numeric_limits<int>::max() && state.plantSun - boomerangCost >= protectedSun && CountPlantType(state, SeedType::SEED_BLOOMERANG) < firingTarget) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            if (HasPlantTypeInRow(state, SeedType::SEED_BLOOMERANG, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
                continue;
            }
            const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_BLOOMERANG, row);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_BLOOMERANG, target)) {
                continue;
            }
            const VSZombieState *closest = FindClosestZombie(state, row);
            const PlantLaneAssessment lane = AssessPlantLane(state, row);
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_BLOOMERANG, row) * 9;
            score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 12;
            score += closest == nullptr ? 70 : (closest->positionX > 640.0f ? 85 : -120);
            score += lane.danger < 110 ? 75 : -100;
            score += row == preferredRow ? 35 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_BLOOMERANG, row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0 && bestTarget.row >= 0) {
            return MakePlayAction(VSSide::Plants, *boomerang, bestTarget, state.boardTick);
        }
    }

    const VSCardState *garlic = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_GARLIC);
    if (garlic == nullptr || state.plantSun - garlic->cost < protectedSun || CountPlantType(state, SeedType::SEED_BLOOMERANG) == 0) {
        return std::nullopt;
    }
    int bestRow = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        if (HasPlantTypeInRow(state, SeedType::SEED_GARLIC, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || (!closest->eating && closest->positionX > 620.0f)) {
            continue;
        }
        const VSGridPosition target = FindPlantCellInExactRow(state, row, 4, 4);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_GARLIC, target)) {
            continue;
        }
        int adjacentBoomerangs = 0;
        for (const int adjacentRow : {row - 1, row + 1}) {
            if (adjacentRow >= 0 && adjacentRow < state.rows && HasPlantTypeInRow(state, SeedType::SEED_BLOOMERANG, adjacentRow)) {
                ++adjacentBoomerangs;
            }
        }
        if (adjacentBoomerangs == 0 && !HasPlantTypeInRow(state, SeedType::SEED_BLOOMERANG, row)) {
            continue;
        }
        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = adjacentBoomerangs * 220 + lane.danger * 2 + firepower.deficit * 10;
        score += closest->eating ? 125 : 0;
        score += closest->positionX < 560.0f ? 70 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_GARLIC, row);
        if (bestRow < 0 || score > bestScore) {
            bestRow = row;
            bestScore = score;
        }
    }
    return bestRow < 0 ? std::nullopt : PlantAIPlanning::TryPlantExactRow(state, SeedType::SEED_GARLIC, bestRow, 4, 4);
}

std::optional<VSAction> PlantAIPlanning::TryThreepeaterPuffFormation(const VSGameState &state, int preferredRow, int protectedSun) {
    // Puff and Potato Mine buy the early runway in this recording. At four
    // producers the first Threepeater belongs in an inner row, where all
    // three shots cover live economy routes instead of becoming an edge gun.
    const bool threepeaterPuffTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_THREEPEATER);
    if (!threepeaterPuffTemplate || EffectivePlantEconomyCount(state) < 4 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const VSCardState *threepeater = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_THREEPEATER);
    const int totalCost = threepeater == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *threepeater);
    if (threepeater == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    const int formationTarget = std::min(3, std::max(1, EffectivePlantEconomyCount(state) - 3));
    if (CountPlantType(state, SeedType::SEED_THREEPEATER) >= formationTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (row == 0 || row == state.rows - 1 || HasPlantTypeInRow(state, SeedType::SEED_THREEPEATER, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)
            || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_THREEPEATER, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_THREEPEATER, target)) {
            continue;
        }

        int coveredPressure = 0;
        int coveredDeficit = 0;
        for (int coveredRow = row - 1; coveredRow <= row + 1; ++coveredRow) {
            coveredPressure += PlantEconomyValueInRow(state, coveredRow);
            coveredDeficit += AssessPlantLaneFirepower(state, coveredRow).deficit;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = coveredPressure * 3 + coveredDeficit * 10;
        score += SeedEconomyPressureOpportunity(state, SeedType::SEED_THREEPEATER, row) * 6;
        score += closest == nullptr ? 65 : (closest->positionX > 620.0f ? 75 : -125);
        score += row == preferredRow ? 35 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_THREEPEATER, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *threepeater, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySnowpeaPuffMagnetPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool snowpeaMagnetTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SNOWPEA) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM)
        && (state.isNight || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE));
    if (!snowpeaMagnetTemplate) {
        return std::nullopt;
    }

    // The Snow-pea/Puff/Magnet recording opens its first safe route with a
    // Coffee-backed Puff before committing the 175-sun carry.  The Puff is
    // capped at two and cannot be placed into a close zombie, so this is a
    // tempo bridge rather than a replacement main damage plan.
    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    const int puffCost = puff == nullptr ? std::numeric_limits<int>::max() : puff->cost + (coffee == nullptr ? 0 : coffee->cost);
    if (EffectivePlantEconomyCount(state) >= 1 && EffectivePlantEconomyCount(state) < 3 && puff != nullptr && (state.isNight || coffee != nullptr)
        && CountPlantType(state, SeedType::SEED_PUFFSHROOM) < 2 && state.plantSun - puffCost >= protectedSun) {
        VSGridPosition openingTarget{};
        int openingScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, row);
            if (HasPlantTypeInRow(state, SeedType::SEED_PUFFSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)
                || (closest != nullptr && (closest->eating || closest->positionX < 720.0f))) {
                continue;
            }
            const VSGridPosition target = FindPuffshroomCell(state, row);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, target)) {
                continue;
            }
            int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_PUFFSHROOM, row) * 5;
            score += PlantEconomyValueInRow(state, row) * 2;
            score += closest == nullptr ? 80 : 25;
            score += row == preferredRow ? 25 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
            if (openingTarget.col < 0 || score > openingScore) {
                openingTarget = target;
                openingScore = score;
            }
        }
        if (openingTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *puff, openingTarget, state.boardTick);
        }
    }

    // The replay opens under pressure with one Coffee-backed Puff per close
    // route. It is temporary control, so it is considered before a costly
    // Snow-pea only while the target is actually within Puff range.
    const int snowpeaCount = CountPlantType(state, SeedType::SEED_SNOWPEA);
    const int puffSupportLimit = snowpeaCount + 2;
    if (puff != nullptr && CountPlantType(state, SeedType::SEED_PUFFSHROOM) < puffSupportLimit && (state.isNight || coffee != nullptr) && state.plantSun - puffCost >= protectedSun) {
        VSGridPosition bestPuffTarget{};
        int bestPuffScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, row);
            if (closest == nullptr || closest->positionX > 720.0f || HasPlantTypeInRow(state, SeedType::SEED_PUFFSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
                continue;
            }
            const VSGridPosition target = FindPuffshroomCell(state, row);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, target)) {
                continue;
            }
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            int score = firepower.deficit * 18 + (!firepower.canHold ? 120 : 0);
            score += PlantEconomyValueInRow(state, row) * 2 + (closest->positionX < 630.0f ? 95 : 0);
            score += row == preferredRow ? 30 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
            if (bestPuffTarget.col < 0 || score > bestPuffScore) {
                bestPuffTarget = target;
                bestPuffScore = score;
            }
        }
        if (bestPuffTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *puff, bestPuffTarget, state.boardTick);
        }
    }

    if (EffectivePlantEconomyCount(state) < 3) {
        return std::nullopt;
    }
    const VSCardState *snowpea = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SNOWPEA);
    const int snowpeaCost = snowpea == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *snowpea);
    if (snowpea == nullptr || snowpeaCost == std::numeric_limits<int>::max() || state.plantSun - snowpeaCost < protectedSun) {
        return std::nullopt;
    }
    const int snowpeaTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 2));
    if (snowpeaCount >= snowpeaTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SNOWPEA, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SNOWPEA, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SNOWPEA, target)) {
            continue;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SNOWPEA, row) * 7;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 14;
        score += closest == nullptr ? 55 : (closest->positionX > 630.0f ? 70 : -115);
        score += row == preferredRow ? 35 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SNOWPEA, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *snowpea, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryPeaPuffTempoOpening(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool peaPuffTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM)
        && (state.isNight || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE));
    if (!peaPuffTemplate || state.isSuddenDeath || EffectivePlantEconomyCount(state) < 1 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const auto IsAffordable = [&](const VSCardState *card) {
        if (card == nullptr) {
            return false;
        }
        const int cost = PlantAIPlanning::EffectivePlantPlayCost(state, *card);
        return cost != std::numeric_limits<int>::max() && state.plantSun - cost >= protectedSun;
    };
    const auto OpeningScore = [&](int row) {
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_PEASHOOTER, row) * 6;
        score += PlantEconomyValueInRow(state, row) * 2;
        score += closest == nullptr ? 75 : (closest->positionX > 680.0f ? 55 : -130);
        score += row == preferredRow ? 30 : 0;
        return score;
    };

    const int puffCount = CountPlantType(state, SeedType::SEED_PUFFSHROOM);
    const int peaCount = CountPlantType(state, SeedType::SEED_PEASHOOTER);
    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    if (puffCount == 0 && peaCount == 0 && IsAffordable(puff)) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            const VSGridPosition target = FindPuffshroomCell(state, row);
            if (PlantAIPlanning::ShouldYieldLaneToMower(state, row) || target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, target)) {
                continue;
            }
            const int score = OpeningScore(row) + StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *puff, bestTarget, state.boardTick);
        }
    }

    const VSCardState *pea = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PEASHOOTER);
    if (puffCount == 0 || peaCount > 0 || !IsAffordable(pea)) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const bool hasAwakePuff = std::any_of(state.plants.begin(), state.plants.end(), [row](const VSPlantState &plant) {
            return !IsDeadOrOutside(plant) && plant.position.row == row && plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_PUFFSHROOM) && !plant.asleep;
        });
        if (!hasAwakePuff || HasPlantTypeInRow(state, SeedType::SEED_PEASHOOTER, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = FindPlantCellInExactRow(state, row, 1, 2);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_PEASHOOTER, target)) {
            continue;
        }
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        const int score = OpeningScore(row) + firepower.deficit * 12 + (firepower.canHold ? 0 : 100) + StrategyBonus(state, VSSide::Plants, SeedType::SEED_PEASHOOTER, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *pea, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryPeaCeleryAshTempo(const VSGameState &state, int preferredRow, int protectedSun) {
    // The Peashooter-versus-Dog recording establishes a compact rear pea
    // line after three producers. Celery is a close-range relief card; the
    // three Ash cards remain available to the normal counter planner.
    const bool peaCeleryAshTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CELERY_STALKER);
    if (!peaCeleryAshTemplate || state.isSuddenDeath || EffectivePlantEconomyCount(state) < 3 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const auto IsAffordable = [&](const VSCardState *card) {
        if (card == nullptr) {
            return false;
        }
        const int cost = PlantAIPlanning::EffectivePlantPlayCost(state, *card);
        return cost != std::numeric_limits<int>::max() && state.plantSun - cost >= protectedSun;
    };

    const VSCardState *pea = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PEASHOOTER);
    if (IsAffordable(pea)) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, row);
            if (HasPlantTypeInRow(state, SeedType::SEED_PEASHOOTER, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
                continue;
            }

            const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_PEASHOOTER, row);
            if (target.col < 0 || target.row < 0 || target.col > 2 || !IsPlantPlacementSafe(state, SeedType::SEED_PEASHOOTER, target)) {
                continue;
            }

            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_PEASHOOTER, row) * 9;
            score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 12;
            score += closest == nullptr ? 85 : (closest->positionX > 660.0f ? 55 : -135);
            score += row == preferredRow ? 35 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PEASHOOTER, row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *pea, bestTarget, state.boardTick);
        }
    }

    const VSCardState *celery = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_CELERY_STALKER);
    if (!IsAffordable(celery) || CountPlantType(state, SeedType::SEED_PEASHOOTER) == 0) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || closest->mindControlled || closest->eating || closest->positionX > 650.0f || HasPlantTypeInRow(state, SeedType::SEED_CELERY_STALKER, row)
            || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }

        const VSGridPosition target = FindPlantCellInExactRow(state, row, 4, 5);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_CELERY_STALKER, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = lane.danger * 2 + firepower.deficit * 15 + PlantEconomyValueInRow(state, row);
        score += !firepower.canHold ? 120 : 0;
        score += HasPlantTypeInRow(state, SeedType::SEED_PEASHOOTER, row) ? 125 : 0;
        score += row == preferredRow ? 25 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_CELERY_STALKER, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *celery, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySporePuffTempoPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // This replay opens a safe lane with one or two Coffee-backed Puffs,
    // then converts the fourth producer into a rear Spore-shroom line. The
    // temporary Puffs cannot become a second carry or consume the slots
    // that the pult needs to threaten the zombie grave economy.
    const bool sporePuffTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM)
        && (state.isNight || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE));
    const int incomeCount = EffectivePlantEconomyCount(state);
    if (!sporePuffTemplate || state.isSuddenDeath || incomeCount < 1 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    const int puffCost = puff == nullptr ? std::numeric_limits<int>::max() : puff->cost + (coffee == nullptr ? 0 : coffee->cost);
    const int puffTarget = std::min(2, incomeCount);
    if (puff != nullptr && CountPlantType(state, SeedType::SEED_PUFFSHROOM) < puffTarget && (state.isNight || coffee != nullptr) && state.plantSun - puffCost >= protectedSun) {
        VSGridPosition bestPuffTarget{};
        int bestPuffScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, row);
            if (HasPlantTypeInRow(state, SeedType::SEED_PUFFSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)
                || (closest != nullptr && (closest->eating || closest->positionX < 720.0f))) {
                continue;
            }
            const VSGridPosition target = FindPuffshroomCell(state, row);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, target)) {
                continue;
            }
            int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_PUFFSHROOM, row) * 5;
            score += PlantEconomyValueInRow(state, row) * 2;
            score += closest == nullptr ? 75 : 20;
            score += row == preferredRow ? 25 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
            if (bestPuffTarget.col < 0 || score > bestPuffScore) {
                bestPuffTarget = target;
                bestPuffScore = score;
            }
        }
        if (bestPuffTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *puff, bestPuffTarget, state.boardTick);
        }
    }

    if (incomeCount < 4) {
        return std::nullopt;
    }
    const VSCardState *spore = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SPORESHROOM);
    const int sporeCost = spore == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *spore);
    const int sporeTarget = std::min(state.rows, std::max(1, incomeCount - 3));
    if (spore == nullptr || sporeCost == std::numeric_limits<int>::max() || state.plantSun - sporeCost < protectedSun || CountPlantType(state, SeedType::SEED_SPORESHROOM) >= sporeTarget) {
        return std::nullopt;
    }

    VSGridPosition bestSporeTarget{};
    int bestSporeScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SPORESHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SPORESHROOM, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SPORESHROOM, target)) {
            continue;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SPORESHROOM, row) * 8;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 13;
        score += closest == nullptr ? 60 : (closest->positionX > 620.0f ? 70 : -95);
        score += row == preferredRow ? 30 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SPORESHROOM, row);
        if (bestSporeTarget.col < 0 || score > bestSporeScore) {
            bestSporeTarget = target;
            bestSporeScore = score;
        }
    }
    return bestSporeTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *spore, bestSporeTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryPeaCabbageTorchTempo(const VSGameState &state, int preferredRow, int protectedSun) {
    // The Peashooter/Cabbagepult/Torchwood recording opens after three
    // producers with one rear Peashooter, then a Cabbagepult on a second
    // route. Torchwood and Pumpkin are held for a formed firing lane, not
    // spent before either pressure line exists.
    const bool peaCabbageTorchTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CABBAGEPULT)
        && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_TORCHWOOD);
    if (!peaCabbageTorchTemplate || EffectivePlantEconomyCount(state) < 3 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const int incomeCount = EffectivePlantEconomyCount(state);
    const int peaCount = CountPlantType(state, SeedType::SEED_PEASHOOTER);
    const int cabbageCount = CountPlantType(state, SeedType::SEED_CABBAGEPULT);
    // Peashooter remains this deck's main carry. The recording opens one
    // Cabbagepult as a second line, but does not replace the rear pea core.
    const int peaTarget = std::min(2, std::max(1, incomeCount - 2));
    const int cabbageTarget = 1;
    SeedType openingSeed = SeedType::SEED_NONE;
    if (peaCount < peaTarget) {
        openingSeed = SeedType::SEED_PEASHOOTER;
    } else if (cabbageCount < cabbageTarget) {
        openingSeed = SeedType::SEED_CABBAGEPULT;
    }
    if (openingSeed == SeedType::SEED_NONE) {
        // Torchwood is the conversion after two firing lines exist. The
        // helper still requires a real pea line behind the target cell.
        if (peaCount >= 2 && cabbageCount >= 1 && incomeCount >= 5) {
            return PlantAIPlanning::TryTorchwoodSupport(state, protectedSun);
        }
        return std::nullopt;
    }

    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, openingSeed);
    const int totalCost = card == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *card);
    if (card == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, openingSeed, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)
            || (openingSeed == SeedType::SEED_PEASHOOTER && IsRangedOutputTradeUnfavorable(state, row))) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, openingSeed, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, openingSeed, target)) {
            continue;
        }

        const VSZombieState *closest = FindClosestZombie(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, openingSeed, row) * 8;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 12;
        score += closest == nullptr ? 70 : (closest->positionX > 640.0f ? 75 : -120);
        // The recorded Cabbagepult deliberately opens a second firing row;
        // it can still use the Peashooter row when that is the only safe
        // deficit, but a two-lane pressure shape is the default.
        score += openingSeed == SeedType::SEED_CABBAGEPULT && !HasPlantTypeInRow(state, SeedType::SEED_PEASHOOTER, row) ? 120 : 0;
        score += row == preferredRow ? 35 : 0;
        score += StrategyBonus(state, VSSide::Plants, openingSeed, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySnowpeaBonkFormation(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool snowpeaBonkTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SNOWPEA) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_BONK_CHOY);
    if (!snowpeaBonkTemplate || state.isSuddenDeath || EffectivePlantEconomyCount(state) < 3 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const auto IsAffordable = [&](const VSCardState *card) {
        if (card == nullptr) {
            return false;
        }
        const int cost = PlantAIPlanning::EffectivePlantPlayCost(state, *card);
        return cost != std::numeric_limits<int>::max() && state.plantSun - cost >= protectedSun;
    };
    const auto IsSafeFormationRow = [&](int row) {
        const VSZombieState *closest = FindClosestZombie(state, row);
        return !PlantAIPlanning::ShouldYieldLaneToMower(state, row) && (closest == nullptr || (!closest->eating && closest->positionX > 660.0f));
    };
    const auto FormationScore = [&](int row) {
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SNOWPEA, row) * 6;
        score += PlantEconomyValueInRow(state, row) * 2;
        score += closest == nullptr ? 80 : 40;
        score += row == preferredRow ? 35 : 0;
        return score;
    };

    // The Snow-pea/Bonk replay does not open with an exposed Snow-pea. Its
    // first controlled route is a single, safe Wall-nut plus Bonk shell;
    // later lanes can receive the ranged carry directly. This remains one
    // starter shell, rather than a five-lane Wall-nut turtle.
    const VSCardState *wallnut = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_WALLNUT);
    const VSCardState *bonk = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_BONK_CHOY);
    const int wallnutCount = CountPlantType(state, SeedType::SEED_WALLNUT);
    const int bonkCount = CountPlantType(state, SeedType::SEED_BONK_CHOY);
    if (wallnutCount == 0 && bonkCount == 0 && IsAffordable(wallnut)) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            if (!IsSafeFormationRow(row)) {
                continue;
            }
            const VSGridPosition target = FindWallnutCell(state, row, 4, 5);
            if (target.col < 0 || target.row < 0) {
                continue;
            }
            const int score = FormationScore(row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *wallnut, bestTarget, state.boardTick);
        }
    }

    if (wallnutCount > 0 && bonkCount == 0 && IsAffordable(bonk)) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            if (!HasPlantTypeInRow(state, SeedType::SEED_WALLNUT, row) || HasPlantTypeInRow(state, SeedType::SEED_BONK_CHOY, row) || !IsSafeFormationRow(row)) {
                continue;
            }
            const VSGridPosition target = FindPlantCellInExactRow(state, row, 3, 3);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_BONK_CHOY, target)) {
                continue;
            }
            const int score = FormationScore(row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0) {
            return MakePlayAction(VSSide::Plants, *bonk, bestTarget, state.boardTick);
        }
    }

    const VSCardState *snowpea = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SNOWPEA);
    const int snowpeaTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 2));
    if (!IsAffordable(snowpea) || CountPlantType(state, SeedType::SEED_SNOWPEA) >= snowpeaTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SNOWPEA, row) || !IsSafeFormationRow(row) || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SNOWPEA, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SNOWPEA, target)) {
            continue;
        }
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = FormationScore(row) + firepower.deficit * 12;
        score += !firepower.canHold ? 105 : 0;
        score += HasPlantTypeInRow(state, SeedType::SEED_WALLNUT, row) ? 115 : 0;
        score += HasPlantTypeInRow(state, SeedType::SEED_BONK_CHOY, row) ? 90 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SNOWPEA, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *snowpea, bestTarget, state.boardTick));
}

} // namespace vsai::detail
