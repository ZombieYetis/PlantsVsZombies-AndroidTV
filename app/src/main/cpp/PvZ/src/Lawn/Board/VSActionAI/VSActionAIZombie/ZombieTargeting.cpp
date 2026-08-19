#include "ZombieAI.h"

#include "../VSActionAITacticalRules.h"

#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

#include "PvZ/Lawn/Board/Plant.h"

namespace vsai::detail {

int ZombieAIPlanning::LeastCommittedZombieRow(const VSGameState &state) const {
    int bestRow = 0;
    int bestScore = std::numeric_limits<int>::max();
    for (int row = 0; row < state.rows; ++row) {
        int economy = 0;
        for (const VSGridItemState &item : state.gridItems) {
            economy += !item.dead && item.position.row == row && IsZombieEconomyItem(item.gridItemType) ? 1 : 0;
        }
        // Strategy data is only a tie-break after the live-board economy
        // and grave-threat checks. It can guide an otherwise equivalent
        // grave position, but cannot force a threatened or stacked row.
        const int score = economy * 130 + CountZombiesInRow(state, row) * 85 + GraveThreatScore(state, row) * 2 - StrategyBonus(state, VSSide::Zombies, SeedType::SEED_ZOMBIE_GRAVESTONE, row) * 2;
        if (score < bestScore) {
            bestScore = score;
            bestRow = row;
        }
    }
    return bestRow;
}

int ZombieAIPlanning::BungeeTargetScore(const VSGameState &state, const VSPlantState &plant, int row) {
    if (!IsBungeeTargetEligible(state, plant)) {
        return std::numeric_limits<int>::min();
    }

    const SeedType seed = static_cast<SeedType>(plant.seedType);
    const int plantCost = std::max(0, Plant::GetCost(seed, SeedType::SEED_NONE));
    int score = plantCost * 5 + PlantValueScore(plant) + static_cast<int>(plant.position.col) * 8;
    if (IsPlantCombatSeed(plant.seedType)) {
        score += 260;
    }
    score += PlantLaneWeaknessScore(state, row) / 3;
    return score;
}

std::optional<VSGridPosition> ZombieAIPlanning::FindTargetForCard(const VSGameState &state, const VSCardState &card, int row) const {
    const SeedType seed = static_cast<SeedType>(card.seedType);
    if (seed == SeedType::SEED_ZOMBIE_GRAVESTONE) {
        const VSGridPosition target = FindZombieEconomyCell(state, row);
        return target.col >= 0 && target.row >= 0 ? std::optional<VSGridPosition>(target) : std::nullopt;
    }
    if (seed == SeedType::SEED_ZOMBIE_MOUND) {
        const VSGridPosition target = FindZombieMoundCell(state, row);
        return target.col >= 0 && target.row >= 0 ? std::optional<VSGridPosition>(target) : std::nullopt;
    }
    if (seed == SeedType::SEED_ZOMBIE_CATAPULT && HasMindControlledZombieInRow(state, row)) {
        return std::nullopt;
    }
    if (HasLobbedPlantInRow(state, row) && IsZombieLobbedScreenDonation(seed)) {
        return std::nullopt;
    }
    if (seed == SeedType::SEED_ZOMBIE_TRASHCAN || seed == SeedType::SEED_ZOMBIE_TALLNUT_HEAD) {
        const int screenDeficit = ZombieGraveScreenDeficit(state, row);
        if ((!NeedsProactiveGraveScreen(state, row) && ProtectableGraveThreatScore(state, row) < 100 && StraightProjectileThreatScore(state, row) < 100 && screenDeficit < 120)
            || (HasZombieGraveGuardInRow(state, row) && screenDeficit < 120)) {
            return std::nullopt;
        }
    }
    if (IsZombieTargetedSeed(seed)) {
        const VSPlantState *targetPlant = nullptr;
        int targetScore = std::numeric_limits<int>::min();
        for (const VSPlantState &plant : state.plants) {
            if (IsDeadOrOutside(plant) || plant.position.row != row) {
                continue;
            }
            const int plantScore = BungeeTargetScore(state, plant, row);
            if (plantScore == std::numeric_limits<int>::min()) {
                continue;
            }
            if (targetPlant == nullptr || plantScore > targetScore) {
                targetPlant = &plant;
                targetScore = plantScore;
            }
        }
        return targetPlant == nullptr ? std::nullopt : std::optional<VSGridPosition>(targetPlant->position);
    }
    const VSGridPosition target = FindZombieCell(state, seed, row);
    return target.col >= 0 && target.row >= 0 ? std::optional<VSGridPosition>(target) : std::nullopt;
}

} // namespace vsai::detail
