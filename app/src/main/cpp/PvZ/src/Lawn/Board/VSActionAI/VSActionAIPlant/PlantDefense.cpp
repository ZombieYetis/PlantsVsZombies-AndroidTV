#include "PlantAI.h"

#include "../VSActionAITacticalRules.h"

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

std::optional<VSAction> PlantAIPlanning::TryPumpkinShell(const VSGameState &state, int row, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_PUMPKINSHELL);
    const VSZombieState *closest = FindClosestZombie(state, row);
    if (card == nullptr || closest == nullptr || IsMowerInMotion(state, row) || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }
    // A wall and a pumpkin are alternative front-line investments for a
    // normal push. Do not answer the same lane with both across adjacent
    // think ticks; the heavier breakthrough branches retain their ash or
    // control answers instead.
    if (HasPlantTypeInRow(state, SeedType::SEED_WALLNUT, row) || HasPlantTypeInRow(state, SeedType::SEED_TALLNUT, row)) {
        return std::nullopt;
    }

    if (IsNutBypassZombieApproaching(state, row)) {
        return std::nullopt;
    }
    const float triggerDistance = IsHeavyZombie(closest->zombieType) || IsDecisiveCounterZombie(closest->zombieType) ? 660.0f : 580.0f;
    if (!closest->eating && closest->positionX > triggerDistance) {
        return std::nullopt;
    }

    const VSPlantState *bestPlant = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row || plant.seedType == static_cast<std::uint16_t>(SeedType::SEED_PUMPKINSHELL)
            || (!IsPlantCombatSeed(plant.seedType) && !IsPlantEconomySeed(state, plant.seedType)) || !CanPumpkinShellTarget(static_cast<SeedType>(plant.seedType)) || plant.position.col < 3
            || HasPlantTypeAt(state, SeedType::SEED_PUMPKINSHELL, plant.position)) {
            continue;
        }
        const int score =
            PlantValueScore(plant) + (IsPlantCombatSeed(plant.seedType) ? 170 : 0) + static_cast<int>(plant.position.col) * 95 + ZombieDeckCounterBonus(state, SeedType::SEED_PUMPKINSHELL, row);
        if (bestPlant == nullptr || score > bestScore) {
            bestPlant = &plant;
            bestScore = score;
        }
    }
    return bestPlant == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *card, bestPlant->position, state.boardTick));
}

bool PlantAIPlanning::ShouldDeployWallnut(const VSGameState &state, int row) const {
    if (row < 0 || row >= state.rows || HasPlantTypeInRow(state, SeedType::SEED_WALLNUT, row) || HasPlantTypeInRow(state, SeedType::SEED_TALLNUT, row)
        || HasPlantTypeInRow(state, SeedType::SEED_PUMPKINSHELL, row)) {
        return false;
    }
    if (IsNutBypassZombieApproaching(state, row)) {
        return false;
    }

    const VSZombieState *closest = FindClosestZombie(state, row);
    if (closest == nullptr) {
        return false;
    }

    const bool hasIcebergControl = HasPlantTypeInRow(state, SeedType::SEED_ICEBERG_LETTUCE, row);
    bool hasProtectedInvestment = hasIcebergControl;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row || plant.position.col > 3) {
            continue;
        }
        if (IsPlantEconomySeed(state, plant.seedType) || IsPlantCombatSeed(plant.seedType)) {
            hasProtectedInvestment = true;
            break;
        }
    }
    const bool mowerlessEmergency = IsMowerlessThirdColumnEmergency(state, row);
    if (!hasProtectedInvestment && !mowerlessEmergency) {
        return false;
    }

    // The nut belongs at the front only after an actual intruder reaches
    // the middle lawn.  A distant pail or trashcan is a reason to build
    // firepower, not to lock 50 sun into a premature wall.
    const bool decisive = IsDecisiveCounterZombie(closest->zombieType);
    const float triggerDistance = IsHeavyZombie(closest->zombieType) || decisive ? 620.0f : 550.0f;
    const PlantLaneAssessment lane = AssessPlantLane(state, row);
    const int firstColumn = mowerlessEmergency ? 0 : 3;
    return (mowerlessEmergency || closest->eating || (hasIcebergControl && closest->positionX <= 720.0f) || (closest->positionX <= triggerDistance && lane.danger >= (decisive ? 80 : 105)))
        && FindWallnutCell(state, row, firstColumn, 5).col >= 0;
}

bool PlantAIPlanning::ShouldYieldLaneToMower(const VSGameState &state, int row) const {
    if (row < 0 || row >= state.rows || row >= static_cast<int>(state.mowerAvailable.size()) || !state.mowerAvailable[static_cast<std::size_t>(row)]) {
        return false;
    }
    const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
    if (CountZombiesInRow(state, row) < 3 || firepower.canHold || firepower.nearHealth <= 0) {
        return false;
    }
    const bool areaAnswerWeak = ReadyPlantAreaCounterCount(state) == 0 || PlantAreaCounterExposure(state, row) < 120;
    const bool dpsCannotCatchUp = firepower.deficit > std::max(80, firepower.dps * 2);
    return areaAnswerWeak || dpsCannotCatchUp;
}

std::optional<VSAction> PlantAIPlanning::TrySpikeweed(const VSGameState &state, int row, int protectedSun) {
    const VSCardState *card = PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SPIKEWEED);
    const VSGridPosition target = FindSpikeweedCell(state, row);
    if (card == nullptr || target.col < 0 || target.row < 0 || state.plantSun - card->cost < protectedSun) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Plants, *card, target, state.boardTick);
}

std::optional<VSAction> PlantAIPlanning::TryCactusSpikeweedPressure(const VSGameState &state, int preferredRow, int protectedSun) {
    const bool hasCactusCard = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return card.active && !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_CACTUS);
    });
    if (!hasCactusCard || CountPlantType(state, SeedType::SEED_CACTUS) == 0 || PlantAIPlanning::FindReadyCard(state, SeedType::SEED_SPIKEWEED) == nullptr) {
        return std::nullopt;
    }

    // The Cactus recordings use Spikeweed as a front trigger after the
    // carry is established. Do not spend it on an empty lane or put it
    // behind the carry; TrySpikeweed resolves the mower/Zomboni path and
    // keeps the tile on the zombie-facing side of a nut.
    int bestRow = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < state.rows; ++offset) {
        const int row = (preferredRow + offset) % state.rows;
        if (HasPlantTypeInRow(state, SeedType::SEED_SPIKEWEED, row) || FindClosestZombie(state, row) == nullptr || PlantAIPlanning::ShouldYieldLaneToMower(state, row)) {
            continue;
        }
        const VSGridPosition target = FindSpikeweedCell(state, row);
        if (target.col < 0 || target.row < 0) {
            continue;
        }
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        int score = firepower.deficit * 10 + PlantEconomyValueInRow(state, row) * 2;
        score += HasPlantTypeInRow(state, SeedType::SEED_CACTUS, row) ? 280 : 0;
        score += StrategyBonus(state, VSSide::Plants, SeedType::SEED_SPIKEWEED, row);
        score += ZombieDeckCounterBonus(state, SeedType::SEED_SPIKEWEED, row);
        score += row == preferredRow ? 25 : 0;
        if (bestRow < 0 || score > bestScore) {
            bestRow = row;
            bestScore = score;
        }
    }
    return bestRow < 0 ? std::nullopt : PlantAIPlanning::TrySpikeweed(state, bestRow, protectedSun);
}

std::optional<VSAction> PlantAIPlanning::TryImpactDistraction(const VSGameState &state, int row, int protectedSun) {
    const VSZombieState *impactZombie = nullptr;
    for (const VSZombieState &zombie : state.zombies) {
        const ZombieType type = static_cast<ZombieType>(zombie.zombieType);
        if (zombie.dead || zombie.row != row || (type != ZombieType::ZOMBIE_SQUASH_HEAD && type != ZombieType::ZOMBIE_GIGA_FOOTBALL)) {
            continue;
        }
        if (impactZombie == nullptr || zombie.positionX < impactZombie->positionX) {
            impactZombie = &zombie;
        }
    }
    if (impactZombie == nullptr || impactZombie->positionX > 720.0f) {
        return std::nullopt;
    }

    const int impactColumn = std::clamp(static_cast<int>((impactZombie->positionX - static_cast<float>(LAWN_XMIN)) / 80.0f), 0, 5);
    int protectedColumn = -1;
    for (const VSPlantState &plant : state.plants) {
        const SeedType seed = static_cast<SeedType>(plant.seedType);
        const bool valuableOutput = IsSustainedOutputSeed(seed) || PlantValueScore(plant) >= 120;
        if (IsDeadOrOutside(plant) || plant.position.row != row || plant.position.col >= impactColumn || !valuableOutput) {
            continue;
        }
        protectedColumn = std::max(protectedColumn, static_cast<int>(plant.position.col));
    }
    if (protectedColumn < 0) {
        return std::nullopt;
    }

    // A front plant already absorbs the next Squash Head or Giga Football
    // tackle. Adding another disposable body behind it spends a card without
    // buying any additional time for the protected carry.
    const bool hasExistingPad = std::any_of(state.plants.begin(), state.plants.end(), [&](const VSPlantState &plant) {
        return !IsDeadOrOutside(plant) && plant.position.row == row && plant.position.col > protectedColumn && plant.position.col <= impactColumn;
    });
    if (hasExistingPad) {
        return std::nullopt;
    }

    for (const SeedType seed : {SeedType::SEED_SUNSHROOM, SeedType::SEED_PUFFSHROOM, SeedType::SEED_SUNFLOWER}) {
        const VSCardState *card = PlantAIPlanning::FindReadyCard(state, seed);
        const bool disposablePad = seed == SeedType::SEED_SUNSHROOM || seed == SeedType::SEED_PUFFSHROOM;
        if (card == nullptr || card->cost > 75 || state.plantSun < card->cost || (!disposablePad && state.plantSun - card->cost < protectedSun)) {
            continue;
        }
        for (int column = impactColumn; column > protectedColumn; --column) {
            if (std::optional<VSAction> action = PlantAIPlanning::TryPlantExactRow(state, seed, row, column, column)) {
                return action;
            }
        }
    }
    return std::nullopt;
}

int PlantAIPlanning::AreaCounterReserve(const VSGameState &state) const {
    int reserve = std::numeric_limits<int>::max();
    for (const VSCardState &card : state.seedBanks[0]) {
        if (IsSlotBlocked(card.slot) || !card.active || card.seedType == static_cast<std::uint16_t>(SeedType::SEED_NONE)) {
            continue;
        }
        if (IsAreaCounterSeed(static_cast<SeedType>(card.seedType))) {
            reserve = std::min(reserve, std::max(0, card.cost));
        }
    }
    return reserve == std::numeric_limits<int>::max() ? 0 : reserve;
}

std::optional<VSAction> PlantAIPlanning::TryFallbackPlant(const VSGameState &state, const PlantLaneAssessment &danger, int buildRow) {
    const bool hasActiveZombie = CountActiveZombies(state) > 0;
    const VSCardState *bestCard = nullptr;
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    for (const VSCardState &card : state.seedBanks[0]) {
        if (IsSlotBlocked(card.slot) || !IsReadyCard(card, state.plantSun)) {
            continue;
        }

        const SeedType seed = static_cast<SeedType>(card.seedType);
        if (seed == SeedType::SEED_SUNFLOWER || seed == SeedType::SEED_SUNSHROOM || seed == SeedType::SEED_IMP_PEAR || seed == SeedType::SEED_INSTANT_COFFEE || seed == SeedType::SEED_PUMPKINSHELL
            || seed == SeedType::SEED_ICEBERG_LETTUCE || seed == SeedType::SEED_TORCHWOOD || seed == SeedType::SEED_GARLIC || seed == SeedType::SEED_HYPNOSHROOM || seed == SeedType::SEED_POTATOMINE
            || seed == SeedType::SEED_SWEET_POTATO || seed == SeedType::SEED_SPIKEWEED || seed == SeedType::SEED_SPIKEROCK || seed == SeedType::SEED_CHILLY_PEPPER || seed == SeedType::SEED_UMBRELLA) {
            continue;
        }
        const bool emergencySeed =
            seed == SeedType::SEED_SQUASH || seed == SeedType::SEED_CHERRYBOMB || seed == SeedType::SEED_JALAPENO || seed == SeedType::SEED_ICESHROOM || seed == SeedType::SEED_DOOMSHROOM;
        const int totalCost = PlantAIPlanning::EffectivePlantPlayCost(state, card);
        if (totalCost == std::numeric_limits<int>::max() || state.plantSun < totalCost) {
            continue;
        }
        if (emergencySeed && (!hasActiveZombie || danger.danger < 150)) {
            continue;
        }

        // Instant counters are selected by their dedicated target logic;
        // they must never become generic emergency fillers in another lane.
        if (seed == SeedType::SEED_SQUASH || seed == SeedType::SEED_CHERRYBOMB || seed == SeedType::SEED_JALAPENO || seed == SeedType::SEED_ICESHROOM || seed == SeedType::SEED_DOOMSHROOM) {
            continue;
        }

        if (seed == SeedType::SEED_SNOWPEA && HasPlantTypeInRow(state, seed, danger.danger >= 105 ? danger.row : buildRow)) {
            continue;
        }

        if ((seed == SeedType::SEED_WALLNUT || seed == SeedType::SEED_TALLNUT) && !ShouldDeployWallnut(state, danger.row)) {
            continue;
        }
        if (seed == SeedType::SEED_CHOMPER && (!hasActiveZombie || danger.closest == nullptr || danger.danger < 90)) {
            continue;
        }

        int row = danger.danger >= 105 ? danger.row : buildRow;
        if (ShouldYieldLaneToMower(state, row)) {
            for (int offset = 1; offset <= state.rows; ++offset) {
                const int candidateRow = (row + offset) % state.rows;
                if (!ShouldYieldLaneToMower(state, candidateRow)) {
                    row = candidateRow;
                    break;
                }
            }
            if (ShouldYieldLaneToMower(state, row)) {
                continue;
            }
        }
        if (IsSustainedOutputSeed(seed) && !IsLobbedOutputSeed(seed) && IsRangedOutputTradeUnfavorable(state, row)) {
            continue;
        }
        int firstColumn = 2;
        int lastColumn = 3;
        if (emergencySeed) {
            firstColumn = 4;
            lastColumn = 5;
        } else if (seed == SeedType::SEED_WALLNUT || seed == SeedType::SEED_TALLNUT) {
            firstColumn = 3;
            lastColumn = 5;
        } else if (seed == SeedType::SEED_CHOMPER) {
            firstColumn = lastColumn = 4;
        } else if (seed == SeedType::SEED_BONK_CHOY || seed == SeedType::SEED_CELERY_STALKER) {
            const VSZombieState *closest = FindClosestZombie(state, row);
            if (closest == nullptr || (!closest->eating && closest->positionX > 660.0f)) {
                continue;
            }
            firstColumn = lastColumn = 4;
        }

        const bool requiresExactRow = emergencySeed || seed == SeedType::SEED_CHOMPER;
        const VSGridPosition target = (seed == SeedType::SEED_WALLNUT || seed == SeedType::SEED_TALLNUT)
            ? FindWallnutCell(state, row, firstColumn, lastColumn)
            : (IsSustainedOutputSeed(seed) ? PlantAIPlanning::FindSustainedOutputCell(state, seed, row)
                                           : (requiresExactRow ? FindPlantCellInExactRow(state, row, firstColumn, lastColumn) : FindPlantCellInColumns(state, row, firstColumn, lastColumn)));
        if (target.col >= 0 && target.row >= 0 && (!IsPlantCombatSeed(static_cast<std::uint16_t>(seed)) || IsPlantPlacementSafe(state, seed, target))) {
            if (!state.isNight && (seed == SeedType::SEED_ICESHROOM || seed == SeedType::SEED_DOOMSHROOM)) {
                const int zombiesOnPlantCell = static_cast<int>(std::count_if(state.zombies.begin(), state.zombies.end(), [&target](const VSZombieState &zombie) {
                    return !zombie.dead && !zombie.mindControlled && zombie.row == target.row && PlantAIPlanning::ZombieColumn(zombie) == target.col;
                }));
                if (zombiesOnPlantCell >= 2) {
                    continue;
                }
            }
            // This is intentionally the final stage after all placement,
            // cost, cooldown, mower, ice-trail and close-zombie filters
            // above. Replay data can therefore rank two legal fallback
            // moves, but can never turn an invalid card into a command.
            int score = StrategyBonus(state, VSSide::Plants, seed, target.row) * 3 - card.cost / 4;
            score += ZombieDeckCounterBonus(state, seed, target.row);
            score += IsSustainedOutputSeed(seed) ? 95 : 0;
            score += IsPlantCombatSeed(static_cast<std::uint16_t>(seed)) ? 30 : 0;
            score += target.row == buildRow ? 15 : 0;
            if (bestCard == nullptr || score > bestScore) {
                bestCard = &card;
                bestTarget = target;
                bestScore = score;
            }
        }
    }
    return bestCard == nullptr ? std::nullopt : std::optional<VSAction>(MakePlayAction(VSSide::Plants, *bestCard, bestTarget, state.boardTick));
}

} // namespace vsai::detail
