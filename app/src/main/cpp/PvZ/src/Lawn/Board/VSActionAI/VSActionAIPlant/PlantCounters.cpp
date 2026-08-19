#include "PlantAI.h"

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

namespace {

    bool CanUseSunshroomFootPad(const VSZombieState &zombie) {
        if (static_cast<ZombieType>(zombie.zombieType) == ZombieType::ZOMBIE_EXPLORER && zombie.explorerTorchLit) {
            return false;
        }
        switch (static_cast<ZombieType>(zombie.zombieType)) {
            case ZombieType::ZOMBIE_GARGANTUAR:
            case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            case ZombieType::ZOMBIE_ZAMBONI:
            case ZombieType::ZOMBIE_CATAPULT:
                return false;
            default:
                return !zombie.dead && !zombie.mindControlled;
        }
    }

} // namespace

std::optional<VSAction> PlantAIPlanning::TryAshCounter(const VSGameState &state, SeedType seedType, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, seedType);
    if (card == nullptr) {
        return std::nullopt;
    }
    int requiredSun = card->cost;
    if (seedType == SeedType::SEED_DOOMSHROOM && !state.isNight) {
        const VSCardState *coffee = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
        if (coffee == nullptr) {
            return std::nullopt;
        }
        requiredSun += coffee->cost;
    }
    if (state.plantSun < requiredSun) {
        return std::nullopt;
    }
    const AshTarget target = PlantAIPlanning::FindBestAshTarget(state, seedType);
    if (target.position.row < 0 || IsMowerInMotion(state, target.position.row) || (ShouldYieldLaneToMower(state, target.position.row) && !target.mowerlessHomeColumn)) {
        return std::nullopt;
    }
    // A protected reserve exists to keep an answer ready, not to prevent
    // that same answer when a mowerless zombie has reached column zero.
    if ((!target.mowerlessHomeColumn && state.plantSun - requiredSun < protectedSun) || !IsAshTargetWorthPlaying(state, seedType, target)) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Plants, *card, target.position, state.boardTick);
}

std::optional<VSAction> PlantAIPlanning::TryBestAshCounter(const VSGameState &state, int protectedSun) {
    const SeedType candidates[] = {
        SeedType::SEED_DOOMSHROOM,
        SeedType::SEED_CHERRYBOMB,
        SeedType::SEED_JALAPENO,
        SeedType::SEED_CHILLY_PEPPER,
        SeedType::SEED_SQUASH,
    };
    const VSCardState *bestCard = nullptr;
    AshTarget bestTarget;
    int bestScore = std::numeric_limits<int>::min();
    for (const SeedType seed : candidates) {
        const VSCardState *card = PlantAIPlanning::FindReadyCard(state, seed);
        if (card == nullptr) {
            continue;
        }
        int requiredSun = card->cost;
        if (seed == SeedType::SEED_DOOMSHROOM && !state.isNight) {
            const VSCardState *coffee = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
            if (coffee == nullptr) {
                continue;
            }
            requiredSun += coffee->cost;
        }
        if (state.plantSun < requiredSun) {
            continue;
        }
        const AshTarget target = PlantAIPlanning::FindBestAshTarget(state, seed);
        if (target.position.row < 0 || IsMowerInMotion(state, target.position.row) || (ShouldYieldLaneToMower(state, target.position.row) && !target.mowerlessHomeColumn)
            || (!target.mowerlessHomeColumn && state.plantSun - requiredSun < protectedSun) || !IsAshTargetWorthPlaying(state, seed, target)) {
            continue;
        }

        // A prior blast gets time to resolve. Repeated Ash on the same
        // local fight is a waste unless a Gargantuar still demands another
        // answer; separated rows remain independently eligible.
        const bool overlapsRecentBlast = mHasRecentAsh && state.boardTick >= mRecentAshTick && state.boardTick - mRecentAshTick < 180 && target.position.row == mRecentAshTarget.row
            && std::abs(static_cast<int>(target.position.col) - static_cast<int>(mRecentAshTarget.col)) <= 2;
        if (overlapsRecentBlast && target.giantCount == 0) {
            continue;
        }

        int score = target.totalHealth + target.hitCount * 210 + target.highValueCount * 300 + target.giantCount * 1800;
        score += target.mowerlessThirdColumn ? 1400 : 0;
        score += target.mowerlessHomeColumn ? 4600 : 0;
        // Damage radius decides which card is the better answer. A row or
        // area counter that covers a real multi-body/health cluster should
        // win over Squash, leaving Squash available for the other lane.
        switch (seed) {
            case SeedType::SEED_DOOMSHROOM:
                score += target.hitCount * 180 + target.highValueCount * 220 - requiredSun / 2;
                break;
            case SeedType::SEED_CHERRYBOMB:
                score += target.hitCount * 130 + target.highValueCount * 140 - requiredSun / 3;
                break;
            case SeedType::SEED_JALAPENO:
                score += target.hitCount * 170 + target.highValueCount * 130 - requiredSun / 3;
                break;
            case SeedType::SEED_CHILLY_PEPPER:
                score += target.hitCount * 90 - requiredSun / 4;
                break;
            case SeedType::SEED_SQUASH:
                score += target.highValueCount * 180 + target.giantCount * 420 - 260;
                break;
            default:
                break;
        }
        if (bestCard == nullptr || score > bestScore) {
            bestCard = card;
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestCard == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *bestCard, bestTarget.position, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryPotatoMine(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_POTATOMINE);
    if (card == nullptr || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
        const int row = (preferredRow + rowOffset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || closest->mindControlled || IsMowerInMotion(state, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)
            || HasPlantTypeInRow(state, SeedType::SEED_POTATOMINE, row)) {
            continue;
        }

        int frontOutputColumn = -1;
        for (const VSPlantState &plant : state.plants) {
            if (!IsDeadOrOutside(plant) && plant.position.row == row && IsSustainedOutputSeed(static_cast<SeedType>(plant.seedType))) {
                frontOutputColumn = std::max(frontOutputColumn, static_cast<int>(plant.position.col));
            }
        }

        const int requiredLead = PlantAIPlanning::PotatoMineArmingLead(*closest);
        for (int column = 5; column > frontOutputColumn; --column) {
            const VSGridPosition target{static_cast<std::int8_t>(column), static_cast<std::int8_t>(row)};
            if (!IsPlantableVSTile(state, target) || HasPlantAt(state, target) || HasGridItemAt(state, target) || !IsPlantPlacementSafe(state, SeedType::SEED_POTATOMINE, target)) {
                continue;
            }

            const float cellCenter = static_cast<float>(LAWN_XMIN + column * 80 + 40);
            const int lead = static_cast<int>(closest->positionX - cellCenter);
            if (lead < requiredLead) {
                continue;
            }

            const PlantLaneAssessment lane = AssessPlantLane(state, row);
            int score = ZombieThreatWeight(closest->zombieType) * 12 + PlantAIPlanning::ZombieEffectiveHealth(*closest) / 3;
            score += lane.danger * 3 + (IsHeavyZombie(closest->zombieType) ? 140 : 0);
            // Prefer the forward-most mine that can still arm. A mine
            // planted too far back arms safely but leaves a long period
            // in which the plant side receives no defensive value.
            score -= (lead - requiredLead) / 3;
            score += row == preferredRow ? 35 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_POTATOMINE, row);
            score += ZombieDeckCounterBonus(state, SeedType::SEED_POTATOMINE, row);
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
    }
    return bestTarget.col < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySnowpeaBonkPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *snowpea = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SNOWPEA);
    const VSCardState *bonk = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_BONK_CHOY);
    const bool hasSnowpeaDeck = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_SNOWPEA);
    });
    const bool hasBonkDeck = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_BONK_CHOY);
    });
    if (!hasSnowpeaDeck || !hasBonkDeck) {
        return std::nullopt;
    }

    // Snow-pea/Bonk is a low-cost front-pressure template. Establish the
    // ranged slow first, then add exactly one Bonk in a lane whose intruder
    // has reached the plant half. This avoids donating either card directly
    // to a distant Screen Door or Trashcan.
    if (snowpea != nullptr && state.plantSun - snowpea->cost >= protectedSun) {
        VSGridPosition bestTarget{};
        int bestScore = std::numeric_limits<int>::min();
        for (int offset = 0; offset < state.rows; ++offset) {
            const int row = (preferredRow + offset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, row);
            if (closest == nullptr || HasPlantTypeInRow(state, SeedType::SEED_SNOWPEA, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || IsRangedOutputTradeUnfavorable(state, row)) {
                continue;
            }
            const VSGridPosition target = PlantAIPlanning::FindSustainedOutputCell(state, SeedType::SEED_SNOWPEA, row);
            if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_SNOWPEA, target)) {
                continue;
            }
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
            int score = firepower.deficit * 16 + PlantEconomyValueInRow(state, row) * 2;
            score += closest->positionX < 700.0f ? 100 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SNOWPEA, row);
            score += row == preferredRow ? 30 : 0;
            if (bestTarget.col < 0 || score > bestScore) {
                bestTarget = target;
                bestScore = score;
            }
        }
        if (bestTarget.col >= 0 && bestTarget.row >= 0) {
            return MakePlayAction(VSSide::Plants, *snowpea, bestTarget, state.boardTick);
        }
    }

    if (bonk == nullptr || state.plantSun - bonk->cost < protectedSun) {
        return std::nullopt;
    }
    int bestRow = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || HasPlantTypeInRow(state, SeedType::SEED_BONK_CHOY, row) || closest->positionX > 660.0f || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = firepower.deficit * 12 + PlantEconomyValueInRow(state, row) * 2;
        score += closest->eating ? 180 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_BONK_CHOY, row);
        score += row == preferredRow ? 35 : 0;
        if (bestRow < 0 || score > bestScore) {
            bestRow = row;
            bestScore = score;
        }
    }
    return bestRow < 0 ? std::nullopt : PlantAIPlanning::TryPlantExactRow(state, SeedType::SEED_BONK_CHOY, bestRow, 3, 4);
}

std::optional<VSAction> PlantAIPlanning::TryStarfruitChomperPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool hasStarfruitDeck = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_STARFRUIT);
    });
    const VSCardState *chomper = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_CHOMPER);
    if (!hasStarfruitDeck || chomper == nullptr || CountPlantType(state, SeedType::SEED_STARFRUIT) == 0 || state.plantSun - chomper->cost < protectedSun) {
        return std::nullopt;
    }

    // In the Starfruit/Chomper recording the Starfruit band owns pressure
    // across adjacent rows, while Chomper is spent only as a close-range
    // removal card. Keep one in the most threatened supported lane instead
    // of letting generic fallback plant it into an empty route.
    int bestRow = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || HasPlantTypeInRow(state, SeedType::SEED_CHOMPER, row) || closest->positionX > 620.0f || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int adjacentStarfruit = 0;
        for (const int adjacent : {row - 1, row, row + 1}) {
            if (adjacent >= 0 && adjacent < state.rows && HasPlantTypeInRow(state, SeedType::SEED_STARFRUIT, adjacent)) {
                ++adjacentStarfruit;
            }
        }
        int score = adjacentStarfruit * 190 + firepower.deficit * 12;
        score += closest->eating ? 160 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_CHOMPER, row);
        score += row == preferredRow ? 30 : 0;
        if (bestRow < 0 || score > bestScore) {
            bestRow = row;
            bestScore = score;
        }
    }
    return bestRow < 0 ? std::nullopt : PlantAIPlanning::TryPlantExactRow(state, SeedType::SEED_CHOMPER, bestRow, 4, 4);
}

std::optional<VSAction> PlantAIPlanning::TryKernelCeleryPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool hasKernelCarry = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_KERNELPULT);
    });
    const bool hasRepeaterCarry = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_REPEATER);
    });
    const VSCardState *celery = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_CELERY_STALKER);
    const bool hasRearCore = CountPlantType(state, SeedType::SEED_KERNELPULT) > 0 || CountPlantType(state, SeedType::SEED_REPEATER) > 0;
    if ((!hasKernelCarry && !hasRepeaterCarry) || !hasRearCore || celery == nullptr || state.plantSun - celery->cost < protectedSun) {
        return std::nullopt;
    }

    // Celery is the front support for an existing Kernel/Repeater firing
    // lane. Holding it until that core exists prevents a close-range opener
    // from consuming the tempo reserved for the recorded ranged plan.
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || closest->mindControlled || closest->positionX > 660.0f || HasPlantTypeInRow(state, SeedType::SEED_CELERY_STALKER, row)) {
            continue;
        }

        // Kernel/Celery recordings hold a close route with one Celery,
        // leaving the pult line behind it free to keep targeting graves.
        const VSGridPosition target = FindPlantCellInExactRow(state, row, 5, 5);
        if (target.col < 0 || target.row < 0 || PlantAIPlanning::ShouldYieldLaneToMower(state, row) || !IsPlantPlacementSafe(state, SeedType::SEED_CELERY_STALKER, target)) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = lane.danger * 2 + firepower.deficit * 14;
        score += !firepower.canHold ? 95 : 0;
        score += PlantEconomyValueInRow(state, row);
        // Repeater/Celery is the same front-support pattern as the
        // Kernel/Celery recording, but it is slightly more valuable in a
        // straight-fire lane because Celery closes the gap while Repeater
        // keeps shooting from the rear.
        score += hasRepeaterCarry && HasPlantTypeInRow(state, SeedType::SEED_REPEATER, row) ? 180 : 0;
        score += row == preferredRow ? 20 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_CELERY_STALKER, row);
        if (score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *celery, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryMagnetShroom(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *magnet = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_MAGNETSHROOM);
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    if (magnet == nullptr || (!state.isNight && coffee == nullptr)) {
        return std::nullopt;
    }

    const auto IsMetalZombie = [](std::uint16_t zombieType) {
        switch (static_cast<ZombieType>(zombieType)) {
            case ZombieType::ZOMBIE_PAIL:
            case ZombieType::ZOMBIE_DOOR:
            case ZombieType::ZOMBIE_FOOTBALL:
            case ZombieType::ZOMBIE_JACK_IN_THE_BOX:
            case ZombieType::ZOMBIE_DIGGER:
            case ZombieType::ZOMBIE_POGO:
            case ZombieType::ZOMBIE_LADDER:
            case ZombieType::ZOMBIE_TRASHCAN:
                return true;
            default:
                return false;
        }
    };

    const int totalCost = magnet->cost + (coffee == nullptr ? 0 : coffee->cost);
    if (state.plantSun - totalCost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_MAGNETSHROOM, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        int metalCount = 0;
        int metalHealth = 0;
        float closestX = std::numeric_limits<float>::max();
        for (const VSZombieState &zombie : state.zombies) {
            if (zombie.dead || zombie.row != row || !IsMetalZombie(zombie.zombieType)) {
                continue;
            }
            ++metalCount;
            metalHealth += ZombieEffectiveThreatHealth(zombie);
            closestX = std::min(closestX, zombie.positionX);
        }
        if (metalCount == 0 || closestX > 760.0f) {
            continue;
        }

        // The recording places Magnet in the forward plant band (usually
        // column five) so its pickup aura reaches the incoming equipment.
        // It is a deliberate exception to the rear-carry safety rule.
        VSGridPosition target = FindPlantCellInExactRow(state, row, 5, 5);
        if (target.col < 0 || target.row < 0) {
            target = FindPlantCellInExactRow(state, row, 4, 4);
        }
        if (target.col < 0 || target.row < 0 || HasGridItemAt(state, target)) {
            continue;
        }
        int score = metalCount * 300 + metalHealth / 2 + PlantEconomyValueInRow(state, row) * 2;
        score += closestX < 620.0f ? 120 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_MAGNETSHROOM, row);
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE, row) / 3;
        score += ZombieDeckCounterBonus(state, SeedType::SEED_MAGNETSHROOM, row);
        score += row == preferredRow ? 35 : 0;
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestTarget.col < 0 || bestTarget.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *magnet, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryHypnoshroom(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_HYPNOSHROOM);
    const VSCardState *coffee = state.isNight ? nullptr : PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    if (card == nullptr || (!state.isNight && coffee == nullptr) || state.plantSun - card->cost - (coffee == nullptr ? 0 : coffee->cost) < protectedSun) {
        return std::nullopt;
    }
    const bool boomerangControlTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_BLOOMERANG);
    const bool controlLineReady = CountPlantType(state, SeedType::SEED_BLOOMERANG) >= std::min(2, state.rows);

    const VSZombieState *bestZombie = nullptr;
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.mindControlled || !IsHypnoshroomTarget(zombie) || zombie.row < 0 || zombie.row >= state.rows || IsMowerInMotion(state, zombie.row) || zombie.positionX > 760.0f
            || HasMindControlledZombieInRow(state, zombie.row)) {
            continue;
        }

        int followers = 0;
        for (const VSZombieState &other : state.zombies) {
            if (!other.dead && !other.mindControlled && other.row == zombie.row && other.positionX > zombie.positionX + 45.0f && other.positionX < zombie.positionX + 500.0f) {
                ++followers;
            }
        }
        const PlantLaneAssessment lane = AssessPlantLane(state, zombie.row);
        if (followers == 0 && PlantAIPlanning::ZombieEffectiveHealth(zombie) < 650 && lane.danger < 150 && !zombie.eating) {
            continue;
        }
        // In the Boomerang/Doom/Hypno template, Hypno is a mid-game
        // conversion. Preserve it until the boomerang band has started,
        // unless a durable zombie has already reached a real break point.
        if (boomerangControlTemplate && !controlLineReady && !zombie.eating && zombie.positionX > 620.0f && PlantAIPlanning::ZombieEffectiveHealth(zombie) < 1200 && followers < 2) {
            continue;
        }

        const int zombieColumn = PlantAIPlanning::ZombieColumn(zombie);
        for (int column = zombieColumn; column >= std::max(0, zombieColumn - 1); --column) {
            const VSGridPosition target{static_cast<std::int8_t>(column), zombie.row};
            if (!IsPlantableVSTile(state, target) || HasPlantAt(state, target) || HasGridItemAt(state, target)) {
                continue;
            }
            if (!state.isNight) {
                const int zombiesOnPlantCell = static_cast<int>(std::count_if(state.zombies.begin(), state.zombies.end(), [&target](const VSZombieState &other) {
                    return !other.dead && !other.mindControlled && other.row == target.row && PlantAIPlanning::ZombieColumn(other) == target.col;
                }));
                // Like Ice-shroom and Doom-shroom, a daytime Hypno-shroom
                // needs a later Coffee click. Do not place it under a double
                // chew stack that can consume it before activation.
                if (zombiesOnPlantCell >= 2) {
                    continue;
                }
            }
            int score = ZombieThreatWeight(zombie.zombieType) * 13 + PlantAIPlanning::ZombieEffectiveHealth(zombie) / 2;
            score += followers * 280 + lane.danger * 3 + (zombie.eating ? 180 : 0);
            score += zombie.row == preferredRow ? 50 : 0;
            score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_HYPNOSHROOM, zombie.row);
            score -= std::abs(column - zombieColumn) * 30;
            if (bestZombie == nullptr || score > bestScore) {
                bestZombie = &zombie;
                bestTarget = target;
                bestScore = score;
            }
        }
    }
    return bestZombie == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryWakeSleepingDoomshroom(const VSGameState &state) {
    if (state.isNight) {
        return std::nullopt;
    }
    const VSCardState *coffee = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_INSTANT_COFFEE);
    if (coffee == nullptr) {
        return std::nullopt;
    }
    const VSPlantState *doomshroom = nullptr;
    for (const VSPlantState &plant : state.plants) {
        if (!IsDeadOrOutside(plant) && !IsMowerInMotion(state, plant.position.row) && plant.asleep && plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_DOOMSHROOM)) {
            doomshroom = &plant;
            break;
        }
    }
    return doomshroom == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *coffee, doomshroom->position, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryStarfruitGarlicFormation(const VSGameState &state, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_GARLIC);
    if (card == nullptr || state.plantSun - card->cost < protectedSun || CountPlantType(state, SeedType::SEED_STARFRUIT) < 2) {
        return std::nullopt;
    }

    int bestRow = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        if (HasPlantTypeInRow(state, SeedType::SEED_GARLIC, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest == nullptr || (!closest->eating && closest->positionX > 700.0f)) {
            continue;
        }
        const VSGridPosition target = FindPlantCellInExactRow(state, row, 4, 4);
        if (target.col < 0 || target.row < 0) {
            continue;
        }

        int adjacentStarfruit = 0;
        for (const int adjacentRow : {row - 1, row + 1}) {
            if (adjacentRow >= 0 && adjacentRow < state.rows && HasPlantTypeInRow(state, SeedType::SEED_STARFRUIT, adjacentRow)) {
                ++adjacentStarfruit;
            }
        }
        if (adjacentStarfruit == 0) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = adjacentStarfruit * 260 + lane.danger * 2 + firepower.deficit * 10;
        score += closest->eating ? 130 : 0;
        score += closest->positionX < 560.0f ? 80 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_GARLIC, row);
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_STARFRUIT, row) / 2;
        if (bestRow < 0 || score > bestScore) {
            bestRow = row;
            bestScore = score;
        }
    }
    return bestRow < 0 ? std::nullopt : PlantAIPlanning::TryPlantExactRow(state, SeedType::SEED_GARLIC, bestRow, 4, 4);
}

std::optional<VSAction> PlantAIPlanning::TryIcebergLettuce(const VSGameState &state, int row, int protectedSun, bool forceEmergencyControl) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_ICEBERG_LETTUCE);
    if (card == nullptr || IsMowerInMotion(state, row) || state.plantSun - card->cost < protectedSun || HasPlantTypeInRow(state, SeedType::SEED_ICEBERG_LETTUCE, row)) {
        return std::nullopt;
    }

    const VSZombieState *litExplorer = nullptr;
    for (const VSZombieState &zombie : state.zombies) {
        if (zombie.dead || zombie.mindControlled || zombie.row != row || zombie.zombieType != static_cast<std::uint16_t>(ZombieType::ZOMBIE_EXPLORER) || !zombie.explorerTorchLit) {
            continue;
        }
        if (litExplorer == nullptr || zombie.positionX < litExplorer->positionX) {
            litExplorer = &zombie;
        }
    }
    if (litExplorer != nullptr) {
        const VSGridPosition torchTarget{static_cast<std::int8_t>(ZombieColumn(*litExplorer)), static_cast<std::int8_t>(row)};
        if (IsPlantableVSTile(state, torchTarget) && !HasPlantAt(state, torchTarget) && !HasGridItemAt(state, torchTarget)) {
            return MakePlayAction(VSSide::Plants, *card, torchTarget, state.boardTick);
        }
    }

    const VSZombieState *closest = FindClosestZombie(state, row);
    if (closest == nullptr) {
        return std::nullopt;
    }

    const PlantLaneAssessment lane = AssessPlantLane(state, row);
    const bool needsControl = forceEmergencyControl || IsFastZombie(closest->zombieType) || IsHeavyZombie(closest->zombieType) || lane.danger >= 135;
    const float triggerDistance = IsFastZombie(closest->zombieType) ? 760.0f : 680.0f;
    if (!needsControl || closest->positionX > triggerDistance) {
        return std::nullopt;
    }
    const VSGridPosition target = FindIcebergLettuceCell(state, row);
    return target.col < 0 || target.row < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, target, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryTorchwoodSupport(const VSGameState &state, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_TORCHWOOD);
    if (card == nullptr || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }

    int bestRow = -1;
    VSGridPosition bestTarget{};
    int bestScore = 0;
    for (int row = 0; row < state.rows; ++row) {
        if (HasPlantTypeInRow(state, SeedType::SEED_TORCHWOOD, row) || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        // Recorded Pea/Cabbage/Torch boards place Torchwood in the first
        // open cell in front of the pea line. Column two is preferable when
        // available because it keeps a third-column slot for a later pad or
        // emergency blocker; column three remains the fallback.
        const VSGridPosition target = FindPlantCellInExactRow(state, row, 2, 3);
        if (target.col < 0 || target.row < 0 || !IsPlantPlacementSafe(state, SeedType::SEED_TORCHWOOD, target)) {
            continue;
        }

        int peaFamilyCount = 0;
        for (const VSPlantState &plant : state.plants) {
            if (IsDeadOrOutside(plant) || plant.position.row != row || plant.position.col >= target.col) {
                continue;
            }
            switch (static_cast<SeedType>(plant.seedType)) {
                case SeedType::SEED_PEASHOOTER:
                case SeedType::SEED_REPEATER:
                case SeedType::SEED_THREEPEATER:
                case SeedType::SEED_SPLITPEA:
                case SeedType::SEED_GATLINGPEA:
                    ++peaFamilyCount;
                    break;
                default:
                    break;
            }
        }
        const int score = peaFamilyCount * 180 + PlantEconomyValueInRow(state, row) / 2 + StrategyBonus(state, VSSide::Plants, SeedType::SEED_TORCHWOOD, row);
        if (peaFamilyCount > 0 && score > bestScore) {
            bestRow = row;
            bestTarget = target;
            bestScore = score;
        }
    }
    return bestRow < 0 ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TryIncomePlant(const VSGameState &state, int row, int protectedSun) {
    if (state.isSuddenDeath) {
        return std::nullopt;
    }
    const VSGridPosition target = FindSafeIncomeCell(state, row);
    if (target.col < 0 || target.row < 0 || PlantAIPlanning::ShouldYieldLaneToMower(state, target.row)) {
        return std::nullopt;
    }
    const VSCardState *bestCard = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const SeedType seedType : {SeedType::SEED_SUNFLOWER, SeedType::SEED_SUNSHROOM}) {
        if (seedType == SeedType::SEED_SUNSHROOM && !state.isNight) {
            continue;
        }
        if (const VSCardState *card = PlantAIPlanning::FindReadyCard(state, seedType); card != nullptr) {
            if (state.plantSun - card->cost < protectedSun) {
                continue;
            }
            const int score = StrategyBonus(state, VSSide::Plants, seedType, target.row);
            if (bestCard == nullptr || score > bestScore) {
                bestCard = card;
                bestScore = score;
            }
        }
    }
    return bestCard == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *bestCard, target, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySunshroomFiller(const VSGameState &state, int preferredRow, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SUNSHROOM);
    if (state.isSuddenDeath || state.isNight || card == nullptr || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }

    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
        const int row = (preferredRow + rowOffset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SUNSHROOM, row) || IsMowerInMotion(state, row)) {
            continue;
        }

        VSGridPosition target = FindPlantCellInExactRow(state, row, 5, 5);
        const VSZombieState *closest = FindClosestZombie(state, row);
        if (closest != nullptr && CanUseSunshroomFootPad(*closest)) {
            const VSGridPosition footTarget{static_cast<std::int8_t>(ZombieColumn(*closest)), static_cast<std::int8_t>(row)};
            if (IsPlantableVSTile(state, footTarget) && !HasPlantAt(state, footTarget) && !HasGridItemAt(state, footTarget)) {
                target = footTarget;
            }
        }
        if (target.col < 0 || target.row < 0) {
            continue;
        }

        const PlantLaneAssessment lane = AssessPlantLane(state, row);
        if (closest == nullptr && CountActiveZombies(state) == 0) {
            continue;
        }
        int score = lane.rawDanger * 2 + PlantEconomyValueInRow(state, row) * 2 + SustainedOutputScoreInRow(state, row);
        score += closest == nullptr ? 35 : (closest->positionX < 760.0f ? 140 : 0);
        score += closest != nullptr && target.col == ZombieColumn(*closest) ? 380 : 0;
        score += row == preferredRow ? 35 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SUNSHROOM, row);
        if (bestTarget.col < 0 || score > bestScore) {
            bestTarget = target;
            bestScore = score;
        }
    }
    if (bestTarget.col < 0 || bestTarget.row < 0) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Plants, *card, bestTarget, state.boardTick);
}

std::optional<VSAction> PlantAIPlanning::TryGraveBuster(const VSGameState &state, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_GRAVEBUSTER);
    if (card == nullptr || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }

    const VSGridItemState *bestItem = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const VSGridItemState &item : state.gridItems) {
        if (item.dead || IsMowerInMotion(state, item.position.row) || !IsZombieEconomyItem(item.gridItemType)) {
            continue;
        }
        const PlantLaneAssessment lane = AssessPlantLane(state, item.position.row);
        const int maxHealth = std::max(1, EstimatedEconomyMaxHealth(item));
        int score = ZombieEconomyAssetValue(item) * 4;
        score += item.health <= maxHealth / 2 ? 80 : 0;
        score -= lane.danger * 2;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_GRAVEBUSTER, item.position.row);
        if (bestItem == nullptr || score > bestScore) {
            bestItem = &item;
            bestScore = score;
        }
    }
    return bestItem == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestItem->position, state.boardTick));
}

} // namespace vsai::detail
