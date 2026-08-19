#include "PlantAI.h"

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {
std::optional<VSAction> PlantAIPlanning::TryMelonScaredySupport(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *melon = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_MELONPULT);
    const VSCardState *scaredy = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SCAREDYSHROOM);
    if (melon == nullptr || scaredy == nullptr || EffectivePlantEconomyCount(state) < 4 || CountPlantType(state, SeedType::SEED_SCAREDYSHROOM) >= 1) {
        return std::nullopt;
    }

    const int scaredyCost = PlantAIPlanning::EffectivePlantPlayCost(state, *scaredy);
    if (scaredyCost == std::numeric_limits<int>::max() || state.plantSun - scaredyCost < protectedSun) {
        return std::nullopt;
    }

    // In the Melon/Scaredy recording, a compact four-Sunflower opening is
    // followed by one cheap long-range Scaredy-shroom, then the player saves for the first
    // Melon-pult. A second early Scaredy delays that breakpoint and turns a
    // Melon carry into a low-damage mushroom line; the post-Melon support
    // branch grows any later layer one row at a time.
    if (state.plantSun - scaredyCost >= melon->cost + protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SCAREDYSHROOM, row);
        if (target.col < 0 || target.row < 0 || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || !IsPlantPlacementSafe(state, SeedType::SEED_SCAREDYSHROOM, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SCAREDYSHROOM, row) * 6;
        score += PlantEconomyValueInRow(state, row) * 2;
        score += firepower.deficit * 12;
        score += !firepower.canHold && firepower.nearHealth > 0 ? 90 : 0;
        score += lane.danger < 105 ? 45 : -110;
        score += row == preferredRow ? 20 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM, row);
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_MELONPULT, row) / 2;
        if (score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *scaredy, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryScaredyCoffeeTempo(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool scaredyCoffeeTemplate =
        HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM) && (state.isNight || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE));
    if (!scaredyCoffeeTemplate || CountZombieEconomy(state) == 0 || EffectivePlantEconomyCount(state) < 2 || CountPlantType(state, SeedType::SEED_SCAREDYSHROOM) >= state.rows) {
        return std::nullopt;
    }

    const VSCardState *scaredy = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SCAREDYSHROOM);
    const int totalCost = scaredy == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *scaredy);
    if (scaredy == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SCAREDYSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SCAREDYSHROOM, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SCAREDYSHROOM, target)) {
            continue;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SCAREDYSHROOM, row) * 8;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 14;
        score += closest == nullptr ? 50 : (closest->positionX > 620.0f ? 80 : -125);
        score += lane.danger < 110 ? 70 : -105;
        score += row == preferredRow ? 30 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *scaredy, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryScaredyMelonSupport(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *scaredy = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SCAREDYSHROOM);
    const int scaredyCost = scaredy == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *scaredy);
    if (scaredy == nullptr || CountPlantType(state, SeedType::SEED_MELONPULT) == 0 || scaredyCost == std::numeric_limits<int>::max() || state.plantSun - scaredyCost < protectedSun
        || EffectivePlantEconomyCount(state) < 4) {
        return std::nullopt;
    }

    // In the Melon/Scaredy replay Scaredy-shroom is a cheap rear layer, not
    // a second carry. Grow it one row at a time after Melon exists, and cap
    // the layer so it cannot consume the firing cells or the whole economy.
    const int targetCount = std::min(state.rows * 2, state.rows + EffectivePlantEconomyCount(state) / 2);
    if (CountPlantType(state, SeedType::SEED_SCAREDYSHROOM) >= targetCount) {
        return std::nullopt;
    }

    const VSCardState *bestCard = nullptr;
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (PlantAIPlanning::ShouldYieldLaneToMower(state, row) || (closest != nullptr && closest->positionX < 620.0f)) {
            continue;
        }

        // Scaredy-shroom is a rear firing layer. Do not let this template
        // consume column one after the back cell fills: waiting is better
        // than turning its low health into a forward disposable plant.
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SCAREDYSHROOM, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SCAREDYSHROOM, target)) {
            continue;
        }
        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = firepower.deficit * 14 + SeedEconomyPressureOpportunity(state, SeedType::SEED_SCAREDYSHROOM, row) * 5;
        score += PlantEconomyValueInRow(state, row) * 2 + (closest == nullptr ? 35 : 0);
        score += row == preferredRow ? 25 : 0;
        score -= lane.danger >= 105 ? 100 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM, row);
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_MELONPULT, row) / 2;
        if (bestCard == nullptr || score > bestScore) {
            bestCard = scaredy;
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestCard == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *bestCard, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryScaredyPuffDoomPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // This replay family has Scaredy-shroom as the only durable carry. Puff
    // is a short-range Coffee-backed layer and Doom is reserved for the
    // real multi-zombie break, not an excuse to omit the firing core.
    if (!HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM) || !HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM)
        || (!state.isNight && !HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE)) || EffectivePlantEconomyCount(state) < 4) {
        return std::nullopt;
    }

    const VSCardState *scaredy = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SCAREDYSHROOM);
    const int scaredyCost = scaredy == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *scaredy);
    const int scaredyTarget = std::min(state.rows, std::max(3, EffectivePlantEconomyCount(state) - 1));
    if (scaredy != nullptr && scaredyCost != std::numeric_limits<int>::max() && state.plantSun - scaredyCost >= protectedSun && CountPlantType(state, SeedType::SEED_SCAREDYSHROOM) < scaredyTarget) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            if (HasPlantTypeInRow(state, SeedType::SEED_SCAREDYSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
                continue;
            }
            const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SCAREDYSHROOM, row);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SCAREDYSHROOM, target)) {
                continue;
            }
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SCAREDYSHROOM, row) * 5;
            score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 12;
            score += firepower.canHold ? 0 : 95;
            score += row == preferredRow ? 35 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM, row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0 && bestTarget.row >= 0) {
            return MakePlayAction(VSSide::Plants, *scaredy, bestTarget, state.boardTick);
        }
    }

    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    const int puffCost = puff == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *puff);
    if (puff == nullptr || puffCost == std::numeric_limits<int>::max() || state.plantSun - puffCost < protectedSun || CountPlantType(state, SeedType::SEED_SCAREDYSHROOM) == 0) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
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
        int score = HasPlantTypeInRow(state, SeedType::SEED_SCAREDYSHROOM, row) ? 280 : 90;
        score += firepower.deficit * 17 + (firepower.canHold ? 0 : 120);
        score += PlantEconomyValueInRow(state, row) * 2;
        score += closest->positionX < 620.0f ? 85 : 0;
        score += row == preferredRow ? 30 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *puff, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryStarfruitPuffPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    if (puff == nullptr || (!state.isNight && coffee == nullptr) || CountPlantType(state, SeedType::SEED_STARFRUIT) == 0) {
        return std::nullopt;
    }
    const int totalCost = puff->cost + (coffee == nullptr ? 0 : coffee->cost);
    if (state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    // The Starfruit/Puff recordings put one short-range Puff in the same
    // rows as the staggered Starfruit band, then wake it with Coffee. This
    // keeps the cheap pressure aligned with cross-lane Starfruit fire.
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_PUFFSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || closest->positionX > 720.0f) {
            continue;
        }
        const VSGridPosition target = FindPuffshroomCell(state, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, target)) {
            continue;
        }
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = HasPlantTypeInRow(state, SeedType::SEED_STARFRUIT, row) ? 280 : 90;
        score += firepower.deficit * 18 + SeedEconomyPressureOpportunity(state, SeedType::SEED_PUFFSHROOM, row) * 4;
        score += closest->positionX < 620.0f ? 80 : 0;
        score += row == preferredRow ? 25 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_STARFRUIT, row) / 2;
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *puff, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryPeaPuffPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // This is the short-range pressure package from the newer recordings:
    // Peashooter supplies the durable carry, while one Coffee-backed Puff
    // buys time in a threatened lane. It is intentionally separate from the
    // Spore/Starfruit branches so Puff is not selected as a second carry.
    const bool hasPeaFamilyCarry = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_REPEATER)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_THREEPEATER);
    const int peaFamilyPlants = CountPlantType(state, SeedType::SEED_PEASHOOTER) + CountPlantType(state, SeedType::SEED_REPEATER) + CountPlantType(state, SeedType::SEED_THREEPEATER);
    const bool earlyPuffResponse = CountActiveZombies(state) > 0 && EffectivePlantEconomyCount(state) >= 2;
    if (!hasPeaFamilyCarry || !HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM) || (peaFamilyPlants == 0 && !earlyPuffResponse)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_STARFRUIT) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM)) {
        return std::nullopt;
    }
    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    if (puff == nullptr || (!state.isNight && coffee == nullptr)) {
        return std::nullopt;
    }
    const int totalCost = puff->cost + (coffee == nullptr ? 0 : coffee->cost);
    if (state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
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
        const bool hasPeaFamilyInRow =
            HasPlantTypeInRow(state, SeedType::SEED_PEASHOOTER, row) || HasPlantTypeInRow(state, SeedType::SEED_REPEATER, row) || HasPlantTypeInRow(state, SeedType::SEED_THREEPEATER, row);
        int adjacentThreepeaterCount = 0;
        for (const int supportRow : {row - 1, row + 1}) {
            if (supportRow >= 0 && supportRow < state.rows && HasPlantTypeInRow(state, SeedType::SEED_THREEPEATER, supportRow)) {
                ++adjacentThreepeaterCount;
            }
        }
        int score = hasPeaFamilyInRow ? 300 : (peaFamilyPlants == 0 ? 185 : 90);
        // Threepeater supports both neighbouring rows. A Puff placed in one
        // of those lanes is still part of the recorded cross-lane pressure
        // package, rather than an unrelated forward mushroom.
        score += adjacentThreepeaterCount * 150;
        score += firepower.deficit * 16 + (firepower.canHold ? 0 : 110);
        score += PlantEconomyValueInRow(state, row) * 2;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
        score += closest->positionX < 620.0f ? 80 : 0;
        score += row == preferredRow ? 25 : 0;
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *puff, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySporePuffPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool hasSporeCarry = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_SPORESHROOM);
    });
    const VSCardState *puff = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUFFSHROOM);
    if (!hasSporeCarry || puff == nullptr || EffectivePlantEconomyCount(state) < 3) {
        return std::nullopt;
    }

    const int puffCost = PlantAIPlanning::EffectivePlantPlayCost(state, *puff);
    if (puffCost == std::numeric_limits<int>::max() || state.plantSun - puffCost < protectedSun) {
        return std::nullopt;
    }

    const VSCardState *bestCard = nullptr;
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || closest->positionX > 720.0f || HasPlantTypeInRow(state, SeedType::SEED_PUFFSHROOM, row)) {
            continue;
        }

        // The Spore/Puff recordings use Puff-shroom as a Coffee-backed
        // close-range pressure layer while Spore-shroom remains the
        // long-range carry. Keep it at the foremost safe firing cell that
        // reaches the current zombie, never as a fixed rear placement.
        const VSGridPosition target = FindPuffshroomCell(state, row);
        if (target.col < 0 || target.row < 0 || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || !IsPlantPlacementSafe(state, SeedType::SEED_PUFFSHROOM, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = lane.danger * 2 + firepower.deficit * 18;
        score += !firepower.canHold ? 105 : 0;
        score += PlantEconomyValueInRow(state, row);
        score += row == preferredRow ? 30 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM, row);
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SPORESHROOM, row) / 2;
        if (bestCard == nullptr || score > bestScore) {
            bestCard = puff;
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestCard == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *bestCard, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryWakeableMushroomOutput(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    const bool fumeDoomTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_FUMESHROOM);
    const bool starfruitPuffTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_STARFRUIT) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM);
    const VSCardState *bestCard = nullptr;
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (const SeedType seed : {SeedType::SEED_PUFFSHROOM, SeedType::SEED_SCAREDYSHROOM, SeedType::SEED_FUMESHROOM}) {
        const VSCardState *card = PlantAIPlanning::FindReadyCard(state, seed);
        if (card == nullptr || (!state.isNight && coffee == nullptr)) {
            continue;
        }
        if (seed == SeedType::SEED_SCAREDYSHROOM && EffectivePlantEconomyCount(state) < 4) {
            continue;
        }
        const int totalCost = card->cost + (coffee == nullptr ? 0 : coffee->cost);
        if (state.plantSun - totalCost < protectedSun) {
            continue;
        }

        for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
            const int row = (preferredRow + rowOffset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, row);
            // Scaredy-shroom is a normal long-range straight shooter.
            // Only Puff/Fume need a nearby zombie before this branch
            // commits Coffee and a forward firing position.
            if (closest == nullptr || (seed != SeedType::SEED_SCAREDYSHROOM && closest->positionX > 720.0f)) {
                continue;
            }

            const VSGridPosition target = seed == SeedType::SEED_SCAREDYSHROOM ? PlantAIPlanning::FindSustainedOutputCell(state, seed, row)
                                                                               : (seed == SeedType::SEED_PUFFSHROOM ? FindPuffshroomCell(state, row) : FindPlantCellInExactRow(state, row, 2, 3));
            if (target.col < 0 || target.row < 0 || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || !IsPlantPlacementSafe(state, seed, target)) {
                continue;
            }

            const PlantLaneAssessment lane = AssessPlantLane(state, row);
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            int score = lane.danger * 2 + firepower.deficit * 16;
            score += !firepower.canHold ? 120 : 0;
            score += PlantEconomyValueInRow(state, row);
            score += row == preferredRow ? 35 : 0;
            score += fumeDoomTemplate && seed == SeedType::SEED_FUMESHROOM ? 260 : 0;
            score += starfruitPuffTemplate && seed == SeedType::SEED_PUFFSHROOM ? 180 : 0;
            score += StrategyBonus(state, VSSide::Plants, seed, row);
            if (bestCard == nullptr || score > bestScore) {
                bestCard = card;
                bestTarget = target;
                bestScore = score;
            }
        }
    }
    return bestCard == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *bestCard, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryWakeSleepingMushroom(const VSGameState &state, int preferredRow) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    if (card == nullptr) {
        return std::nullopt;
    }

    const VSPlantState *bestPlant = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || !plant.asleep || plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_SUNSHROOM)) {
            continue;
        }
        int score = PlantValueScore(plant) + (IsPlantCombatSeed(plant.seedType) ? 220 : 0);
        if (plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_MAGNETSHROOM)) {
            // Magnet is a matchup card, not generic mushroom filler. Wake it
            // immediately when its row contains removable equipment.
            for (const VSZombieState &zombie : state.zombies) {
                if (!zombie.dead && zombie.row == plant.position.row
                    && (zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_PAIL) || zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_DOOR)
                        || zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_FOOTBALL) || zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_TRASHCAN)
                        || zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_LADDER))) {
                    score += 420;
                    break;
                }
            }
        }
        score += plant.position.row == preferredRow ? 70 : 0;
        if (bestPlant == nullptr || score > bestScore) {
            bestPlant = &plant;
            bestScore = score;
        }
    }
    return bestPlant == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestPlant->position, state.boardTick));
}

} // namespace vsai::detail
