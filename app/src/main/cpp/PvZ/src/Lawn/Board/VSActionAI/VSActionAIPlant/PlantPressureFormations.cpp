#include "PlantAI.h"

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {
std::optional<VSAction> PlantAIPlanning::TryPeaDoomTempoPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // This recording's Peashooter is an early economic threat. Doom and
    // Chilly are held to answer a later swarm; they must not make the AI
    // wait for four flowers before its first low-cost firing lane exists.
    const bool peaDoomTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER);
    if (!peaDoomTemplate || EffectivePlantEconomyCount(state) < 1 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const int peaTarget = EffectivePlantEconomyCount(state) < 4 ? 1 : std::min(state.rows, EffectivePlantEconomyCount(state) - 2);
    return TryTemplateSustainedOutput(state,
                                      preferredRow,
                                      protectedSun,
                                      {
                                          .seed = SeedType::SEED_PEASHOOTER,
                                          .targetCount = peaTarget,
                                          .economyPressureWeight = 8,
                                          .firepowerDeficitWeight = 13,
                                          .noZombieScore = 70,
                                          .distantZombieThreshold = 640.0f,
                                          .distantZombieScore = 80,
                                          .closeZombieScore = -120,
                                          .preferredRowBonus = 35,
                                          .requireFavorableRangedTrade = true,
                                      });
}

std::optional<VSAction> PlantAIPlanning::TryStarfruitCrossfireFormation(const VSGameState &state, int preferredRow, int protectedSun) {
    // Starfruit recordings build from the inner rows so each plant applies
    // crossfire to three lanes. Puff/Chomper are local answers and must not
    // postpone that central pressure formation.
    const bool starfruitTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_STARFRUIT);
    if (!starfruitTemplate || EffectivePlantEconomyCount(state) < 4 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }
    const VSCardState *starfruit = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_STARFRUIT);
    if (starfruit == nullptr || state.plantSun - starfruit->cost < protectedSun) {
        return std::nullopt;
    }
    const int crossfireTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 3));
    if (CountPlantType(state, SeedType::SEED_STARFRUIT) >= crossfireTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (row == 0 || row == state.rows - 1 || HasPlantTypeInRow(state, SeedType::SEED_STARFRUIT, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)
            || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_STARFRUIT, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_STARFRUIT, target)) {
            continue;
        }
        int crossfireValue = 0;
        int crossfireDeficit = 0;
        for (int coveredRow = row - 1; coveredRow <= row + 1; ++coveredRow) {
            crossfireValue += PlantEconomyValueInRow(state, coveredRow);
            crossfireDeficit += AssessPlantLaneFirepower(state, coveredRow).deficit;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = crossfireValue * 3 + crossfireDeficit * 9;
        score += SeedEconomyPressureOpportunity(state, SeedType::SEED_STARFRUIT, row) * 6;
        score += closest == nullptr ? 60 : (closest->positionX > 640.0f ? 70 : -115);
        score += row == preferredRow ? 30 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_STARFRUIT, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *starfruit, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryCactusSpikeweedCore(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool cactusSpikeweedTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CACTUS) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPIKEWEED);
    if (!cactusSpikeweedTemplate || EffectivePlantEconomyCount(state) < 4 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }
    const int cactusTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 3));
    return TryTemplateSustainedOutput(state,
                                      preferredRow,
                                      protectedSun,
                                      {
                                          .seed = SeedType::SEED_CACTUS,
                                          .targetCount = cactusTarget,
                                          .economyPressureWeight = 7,
                                          .firepowerDeficitWeight = 13,
                                          .noZombieScore = 60,
                                          .distantZombieThreshold = 640.0f,
                                          .distantZombieScore = 75,
                                          .closeZombieScore = -125,
                                          .preferredRowBonus = 30,
                                          .requireFavorableRangedTrade = true,
                                      });
}

std::optional<VSAction> PlantAIPlanning::TryKernelCeleryFormation(const VSGameState &state, int preferredRow, int protectedSun) {
    // Kernel-pult is the durable lane pressure in the Kernel/Celery replay.
    // Celery reacts at the front only after this lobbed firing core exists.
    const bool kernelCeleryTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_KERNELPULT) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CELERY_STALKER);
    if (!kernelCeleryTemplate || EffectivePlantEconomyCount(state) < 6 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }
    const int kernelTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 5));
    return TryTemplateSustainedOutput(state,
                                      preferredRow,
                                      protectedSun,
                                      {
                                          .seed = SeedType::SEED_KERNELPULT,
                                          .targetCount = kernelTarget,
                                          .economyPressureWeight = 8,
                                          .firepowerDeficitWeight = 12,
                                          .noZombieScore = 55,
                                          .distantZombieThreshold = 620.0f,
                                          .distantZombieScore = 70,
                                          .closeZombieScore = -85,
                                          .preferredRowBonus = 30,
                                      });
}

std::optional<VSAction> PlantAIPlanning::TryRepeaterCeleryTempo(const VSGameState &state, int preferredRow, int protectedSun) {
    // This replay spends its first three producers on a two-lane Repeater
    // core. Celery is a close-range response only; letting it consume the
    // early firing budget turns the deck into a fragile melee opening.
    const bool repeaterCeleryTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_REPEATER) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CELERY_STALKER);
    if (!repeaterCeleryTemplate || EffectivePlantEconomyCount(state) < 3 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const int firingLineTarget = std::min(state.rows, std::max(2, EffectivePlantEconomyCount(state) - 1));
    return TryTemplateSustainedOutput(state,
                                      preferredRow,
                                      protectedSun,
                                      {
                                          .seed = SeedType::SEED_REPEATER,
                                          .targetCount = firingLineTarget,
                                          .economyPressureWeight = 8,
                                          .firepowerDeficitWeight = 13,
                                          .noZombieScore = 70,
                                          .distantZombieThreshold = 640.0f,
                                          .distantZombieScore = 80,
                                          .closeZombieScore = -135,
                                          .preferredRowBonus = 35,
                                          .requireFavorableRangedTrade = true,
                                          .useEffectiveCost = true,
                                      });
}

std::optional<VSAction> PlantAIPlanning::TryMelonMineTempo(const VSGameState &state, int preferredRow, int protectedSun) {
    // In the pure Melon-pult recording, Potato Mine and Wall-nut buy the
    // first firing window. There is no Scaredy-shroom support layer: once
    // the saved sun reaches Melon cost, pressure the grave economy directly.
    const bool melonMineTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_MELONPULT);
    if (!melonMineTemplate || EffectivePlantEconomyCount(state) < 3 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const int firingLineTarget = std::min(state.rows, std::max(1, EffectivePlantEconomyCount(state) - 2));
    return TryTemplateSustainedOutput(state,
                                      preferredRow,
                                      protectedSun,
                                      {
                                          .seed = SeedType::SEED_MELONPULT,
                                          .targetCount = firingLineTarget,
                                          .economyPressureWeight = 9,
                                          .firepowerDeficitWeight = 12,
                                          .noZombieScore = 60,
                                          .distantZombieThreshold = 620.0f,
                                          .distantZombieScore = 70,
                                          .closeZombieScore = -80,
                                          .preferredRowBonus = 35,
                                          .useEffectiveCost = true,
                                      });
}

std::optional<VSAction> PlantAIPlanning::TryRepeaterTempoPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // The Repeater/Sun-shroom recordings use two actual Sunflowers plus a
    // disposable Sun-shroom pad, then immediately turn the first affordable
    // 200 sun into a rear firing lane. Daytime Sun-shrooms do not count as
    // income, but they do make the two-producer breakpoint safe enough to
    // start grave pressure before the generic filler can spend it.
    const bool repeaterTempoTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_REPEATER);
    const int incomeCount = EffectivePlantEconomyCount(state);
    const bool hasSunshroomPad = CountPlantType(state, SeedType::SEED_SUNSHROOM) > 0;
    if (!repeaterTempoTemplate || incomeCount < (hasSunshroomPad ? 2 : 3) || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const VSCardState *repeater = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_REPEATER);
    const int totalCost = repeater == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *repeater);
    if (repeater == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    // Spread the opening repeaters across safe grave lanes. Later generic
    // output logic may deepen a completed route, but the first pass must not
    // hand a Cherry or Squash a stack of the only durable plant-side carry.
    const int openingTarget = std::min(state.rows, std::max(1, (incomeCount + 1) / 2));
    if (CountPlantType(state, SeedType::SEED_REPEATER) >= openingTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_REPEATER, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }

        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_REPEATER, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_REPEATER, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_REPEATER, row) * 8;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 14;
        score += closest == nullptr ? 55 : (closest->positionX > 620.0f ? 80 : -115);
        score += lane.danger < 105 ? 65 : -120;
        score += row == preferredRow ? 35 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_REPEATER, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *repeater, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySporeShellPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // The Pumpkin/Squash Spore recordings establish a small rear firing
    // spread before spending either defensive card. Pumpkin protects a
    // developed carry and Squash clears a real breakthrough; neither is an
    // opening substitute for the lobbed pressure that can damage graves
    // through a slow zombie screen.
    const bool sporeShellTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM);
    if (!sporeShellTemplate || EffectivePlantEconomyCount(state) < 4 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const VSCardState *spore = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SPORESHROOM);
    const int totalCost = spore == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *spore);
    if (spore == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    const int openingTarget = std::min(3, std::max(1, EffectivePlantEconomyCount(state) - 3));
    if (CountPlantType(state, SeedType::SEED_SPORESHROOM) >= openingTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SPORESHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }

        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SPORESHROOM, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SPORESHROOM, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_SPORESHROOM, row) * 7;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 12;
        score += closest == nullptr ? 45 : (closest->positionX > 620.0f ? 75 : -80);
        score += lane.danger < 120 ? 65 : -90;
        score += row == preferredRow ? 35 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SPORESHROOM, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *spore, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryFumeDoomPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    // In the Fume/Doom replay, Doom and Chilly are the one-shot release
    // valves. The actual board advantage comes from a compact Fume firing
    // line in columns two and three, built immediately after the initial
    // sun base instead of treating Sun-shroom padding as another producer.
    const bool fumeDoomTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_FUMESHROOM) && (state.isNight || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE));
    if (!fumeDoomTemplate || EffectivePlantEconomyCount(state) < 6 || CountZombieEconomy(state) == 0) {
        return std::nullopt;
    }

    const VSCardState *fume = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_FUMESHROOM);
    const int totalCost = fume == nullptr ? std::numeric_limits<int>::max() : PlantAIPlanning::EffectivePlantPlayCost(state, *fume);
    if (fume == nullptr || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    const int firingLineTarget = std::min(state.rows, std::max(2, EffectivePlantEconomyCount(state) - 5));
    if (CountPlantType(state, SeedType::SEED_FUMESHROOM) >= firingLineTarget) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_FUMESHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }

        const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_FUMESHROOM, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_FUMESHROOM, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        const VSZombieState *closest = FindClosestZombie(state, row);
        int score = SeedEconomyPressureOpportunity(state, SeedType::SEED_FUMESHROOM, row) * 6;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * 14;
        score += closest == nullptr ? 50 : (closest->positionX > 560.0f ? 95 : -100);
        score += lane.danger < 120 ? 75 : -85;
        score += row == preferredRow ? 30 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_FUMESHROOM, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *fume, bestTarget, state.boardTick));
}

} // namespace vsai::detail
