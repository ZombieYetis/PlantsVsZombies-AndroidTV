#include "ZombieAI.h"

#include "../VSActionAILanePolicy.h"

#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

std::optional<VSAction> ZombieAIPlanning::TryBuildEconomy(const VSGameState &state, int row) {
    if (state.isSuddenDeath) {
        return std::nullopt;
    }
    const VSCardState *card = FindReadyCard(state, SeedType::SEED_ZOMBIE_GRAVESTONE);
    if (card == nullptr) {
        return std::nullopt;
    }
    const VSGridPosition target = FindZombieEconomyCell(state, row);
    if (target.col < 0 || target.row < 0) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Zombies, *card, target, state.boardTick);
}

std::optional<VSAction> ZombieAIPlanning::TryProtectEconomy(const VSGameState &state, int row, bool force) {
    const ZombieLanePolicy lane = EvaluateZombieLanePolicy(state, row);
    if (!lane.hasLiveTarget || lane.deploymentBlocked || (!force && lane.strongMowerlessPlantLane && CountZombiesInRow(state, row) == 0 && !AllMowersSpent(state))) {
        return std::nullopt;
    }
    const int protectableThreat = ProtectableGraveThreatScore(state, row);
    const int straightThreat = StraightProjectileThreatScore(state, row);
    const int lobbedThreat = LobbedProjectileThreatScore(state, row);
    const int screenDeficit = ZombieGraveScreenDeficit(state, row);
    const bool proactiveScreen = NeedsProactiveGraveScreen(state, row);
    if ((HasZombieGraveGuardInRow(state, row) && screenDeficit < 55) || (!force && !proactiveScreen && protectableThreat < 50 && straightThreat < 55 && lobbedThreat < 70 && screenDeficit < 55)) {
        return std::nullopt;
    }

    const VSCardState *bestCard = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    const bool plantHasMagnet = std::any_of(state.seedBanks[0].begin(),
                                            state.seedBanks[0].end(),
                                            [](const VSCardState &card) { return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_MAGNETSHROOM); })
        || CountPlantType(state, SeedType::SEED_MAGNETSHROOM) > 0;
    for (const VSCardState &card : state.seedBanks[1]) {
        const SeedType seed = static_cast<SeedType>(card.seedType);
        if (IsSlotBlocked(card.slot) || !IsZombieGraveGuardSeed(seed) || !IsReadyCard(card, state.zombieBrains)) {
            continue;
        }
        // A pult, including Spore-shroom, attacks over a slow screen.
        // Do not turn an urgent grave-defense branch into a free
        // Trashcan, Door, or Newspaper donation.
        if (HasLobbedPlantInRow(state, row) && IsZombieLobbedScreenDonation(seed)) {
            continue;
        }
        const VSGridPosition target = FindZombieCell(state, seed, row);
        if (!IsCardReadyForZombieTarget(card, state, target)) {
            continue;
        }

        int score = ZombieGraveGuardPriority(seed) + protectableThreat * 2 + (proactiveScreen ? 220 : 0) + (force ? 420 : 0);
        // Trashcan is the direct-fire shield from the recordings. Lobbed
        // projectiles pass over every metal screen, so the row filter
        // above leaves durable non-screen heads as the only protection
        // candidates in those lanes.
        if (seed == SeedType::SEED_ZOMBIE_TRASHCAN) {
            score += straightThreat * 3 - lobbedThreat * 2;
        } else {
            score += straightThreat > 0 ? 120 : 0;
            score += lobbedThreat > 0 ? 170 : 0;
        }
        if (plantHasMagnet) {
            score += IsZombieMetalGraveGuard(seed) ? -420 : 260;
        }
        score += screenDeficit * 2;
        score += StrategyBonus(state, VSSide::Zombies, seed, row);
        score -= card.cost / 4;
        if (bestCard == nullptr || score > bestScore) {
            bestCard = &card;
            bestScore = score;
        }
    }

    if (bestCard == nullptr) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Zombies, *bestCard, FindZombieCell(state, static_cast<SeedType>(bestCard->seedType), row), state.boardTick);
}

std::optional<VSAction> ZombieAIPlanning::TryCounterLobbedGravePressure(const VSGameState &state, const ZombieDecisionContext &context, int row) {
    const VSCardState *catapult = FindReadyCard(state, SeedType::SEED_ZOMBIE_CATAPULT);
    if (catapult == nullptr || !EvaluateZombieLanePolicy(state, row).allowsAttack || context.economyCount < state.rows || HasMindControlledZombieInRow(state, row)
        || LobbedProjectileThreatScore(state, row) < 70) {
        return std::nullopt;
    }

    // A Catapult is the replay-derived answer to a developed pult or
    // Spore firing lane. Unlike a Door, Newspaper, or Trashcan, it does
    // not donate a slow metal screen to a projectile that arcs over it.
    const VSGridPosition target = FindZombieCell(state, SeedType::SEED_ZOMBIE_CATAPULT, row);
    if (target.col < 0 || target.row < 0 || !IsCardReadyForZombieTarget(*catapult, state, target)) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Zombies, *catapult, target, state.boardTick);
}

} // namespace vsai::detail
