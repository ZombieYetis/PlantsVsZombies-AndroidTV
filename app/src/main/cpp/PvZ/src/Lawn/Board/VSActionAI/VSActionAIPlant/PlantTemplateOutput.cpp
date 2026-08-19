#include "PlantAI.h"

#include <limits>
#include <optional>

namespace vsai::detail {

std::optional<VSAction> PlantAIPlanning::TryTemplateSustainedOutput(const VSGameState &state, int preferredRow, int protectedSun, TemplateOutputPolicy policy) {
    if (policy.seed == SeedType::SEED_NONE || policy.targetCount <= 0 || CountPlantType(state, policy.seed) >= policy.targetCount) {
        return std::nullopt;
    }

    const VSCardState *card = FindReadyCard(state, policy.seed);
    const int cost = card == nullptr ? std::numeric_limits<int>::max() : (policy.useEffectiveCost ? EffectivePlantPlayCost(state, *card) : card->cost);
    if (card == nullptr || cost == std::numeric_limits<int>::max() || state.plantSun - cost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, policy.seed, row) || ShouldYieldLaneToMower(state, row) || (policy.requireFavorableRangedTrade && IsRangedOutputTradeUnfavorable(state, row))) {
            continue;
        }
        const VSGridPosition target = FindSustainedOutputCell(state, policy.seed, row);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, policy.seed, target)) {
            continue;
        }

        const VSZombieState *closest = FindClosestZombie(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = SeedEconomyPressureOpportunity(state, policy.seed, row) * policy.economyPressureWeight;
        score += PlantEconomyValueInRow(state, row) * 2 + firepower.deficit * policy.firepowerDeficitWeight;
        score += closest == nullptr ? policy.noZombieScore : (closest->positionX > policy.distantZombieThreshold ? policy.distantZombieScore : policy.closeZombieScore);
        score += row == preferredRow ? policy.preferredRowBonus : 0;
        score += StrategyBonus(state, VSSide::Plants, policy.seed, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick));
}

} // namespace vsai::detail
