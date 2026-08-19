#include "PlantAI.h"

#include "PvZ/Lawn/Common/GameConstants.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

int PlantAIPlanning::EffectivePlantEconomyCount(const VSGameState &state) {
    return EffectiveAIEconomyCount(VSSide::Plants, CountPlantIncome(state));
}

VSGridPosition PlantAIPlanning::FindSustainedOutputCell(const VSGameState &state, SeedType seed, int row) const {
    int lastOutputColumn = 3;
    for (const VSPlantState &plant : state.plants) {
        if (IsDeadOrOutside(plant) || plant.position.row != row || plant.seedType != static_cast<std::uint16_t>(SeedType::SEED_POTATOMINE)) {
            continue;
        }
        // A planted mine defines the front of this lane. Never add a
        // repeatable attacker on its zombie-facing side.
        lastOutputColumn = std::min(lastOutputColumn, static_cast<int>(plant.position.col) - 1);
    }
    if (lastOutputColumn < 0) {
        return {};
    }
    switch (seed) {
        case SeedType::SEED_STARFRUIT:
            // The replay builds a staggered center band (columns 1..3),
            // so every star also reaches the two neighboring lanes.
            return lastOutputColumn < 1 ? VSGridPosition{} : FindPlantCellInExactRow(state, row, 1, lastOutputColumn);
        case SeedType::SEED_BLOOMERANG:
            // Boomerang's replay formation also keeps a center firing band.
            // Leaving column zero for income/support prevents the carry from
            // being offered as the first disposable target.
            return lastOutputColumn < 1 ? VSGridPosition{} : FindPlantCellInExactRow(state, row, 1, lastOutputColumn);
        case SeedType::SEED_THREEPEATER:
            // Threepeater's value comes from its adjacent-row volleys.  The
            // recorded Puff/Threepeater games reserve column zero for the
            // sunflower band and build the cross-lane firing core in 1..3.
            // A frontmost Threepeater at column zero neither protects that
            // economy nor leaves room for the short-range Puff response.
            return lastOutputColumn < 1 ? VSGridPosition{} : FindPlantCellInExactRow(state, row, 1, lastOutputColumn);
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_GLOOMSHROOM:
            // Short-range mushroom carries need to be far enough forward
            // to engage, but never consume the emergency cells at 4/5.
            return lastOutputColumn < 2 ? VSGridPosition{} : FindPlantCellInExactRow(state, row, 2, lastOutputColumn);
        case SeedType::SEED_SCAREDYSHROOM:
            // The daytime Scaredy layout reserves the back column for
            // its carry. It must never become forward disposable padding.
            return FindPlantCellInExactRow(state, row, 0, 0);
        default:
            // The replay pults and boomerangs establish their firing line
            // behind the disposable front, including column zero when it
            // remains open beside the opening Sunflowers.
            return FindPlantCellInExactRow(state, row, 0, lastOutputColumn);
    }
}

bool PlantAIPlanning::HasReadySustainedOutputCard(const VSGameState &state, int protectedSun) const {
    return std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [&](const VSCardState &card) {
        if (IsSlotBlocked(card.slot) || !IsReadyCard(card, state.plantSun) || !IsSustainedOutputSeed(static_cast<SeedType>(card.seedType))) {
            return false;
        }
        const int totalCost = PlantAIPlanning::EffectivePlantPlayCost(state, card);
        return totalCost != std::numeric_limits<int>::max() && state.plantSun - totalCost >= protectedSun;
    });
}

std::optional<VSAction> PlantAIPlanning::TryRecycleIncomeForOutput(const VSGameState &state, int preferredRow, int protectedSun) {
    // Replays keep a compact income base, then replace only its exposed
    // front flower when it prevents a real firing line from being built.
    // Do not trade economy during the opening or merely to make space.
    const int compactIncomeBase = state.rows >= 6 ? 5 : 4;
    if (state.isSuddenDeath || EffectivePlantEconomyCount(state) < compactIncomeBase || !HasReadySustainedOutputCard(state, protectedSun)) {
        return std::nullopt;
    }

    const VSPlantState *bestPlant = nullptr;
    int bestScore = std::numeric_limits<int>::min();
    for (int rowOffset = 0; rowOffset < state.rows; ++rowOffset) {
        const int row = (preferredRow + rowOffset) % state.rows;
        if (!HasLiveZombieTargetInRow(state, row)) {
            continue;
        }
        const bool hasReadyLobbedOutput = std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [&](const VSCardState &card) {
            if (!IsReadyCard(card, state.plantSun) || !IsLobbedOutputSeed(static_cast<SeedType>(card.seedType))) {
                return false;
            }
            const int totalCost = PlantAIPlanning::EffectivePlantPlayCost(state, card);
            return totalCost != std::numeric_limits<int>::max() && state.plantSun - totalCost >= protectedSun;
        });
        if (IsRangedOutputTradeUnfavorable(state, row) && !hasReadyLobbedOutput) {
            continue;
        }

        int incomeInRow = 0;
        bool outputCellAvailable = false;
        for (const VSPlantState &plant : state.plants) {
            if (!IsDeadOrOutside(plant) && plant.position.row == row && IsPlantEconomySeed(state, plant.seedType)) {
                ++incomeInRow;
            }
        }
        for (const VSCardState &card : state.seedBanks[0]) {
            if (!IsReadyCard(card, state.plantSun) || !IsSustainedOutputSeed(static_cast<SeedType>(card.seedType))) {
                continue;
            }
            const int totalCost = PlantAIPlanning::EffectivePlantPlayCost(state, card);
            if (totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
                continue;
            }
            if (FindSustainedOutputCell(state, static_cast<SeedType>(card.seedType), row).col >= 0) {
                outputCellAvailable = true;
                break;
            }
        }
        if (outputCellAvailable || incomeInRow < 2) {
            continue;
        }

        for (const VSPlantState &plant : state.plants) {
            if (IsDeadOrOutside(plant) || plant.position.row != row || !IsPlantEconomySeed(state, plant.seedType) || plant.position.col < 2 || plant.position.col > 3) {
                continue;
            }
            const int healthRatio = plant.maxHealth > 0 ? plant.health * 100 / plant.maxHealth : 100;
            int score = static_cast<int>(plant.position.col) * 120 + (100 - std::clamp(healthRatio, 0, 100));
            score += row == preferredRow ? 35 : 0;
            if (bestPlant == nullptr || score > bestScore) {
                bestPlant = &plant;
                bestScore = score;
            }
        }
    }
    return bestPlant == nullptr ? std::nullopt : std::optional<VSAction>(MakeShovelAction(bestPlant->position, state.boardTick));
}

std::optional<VSAction> PlantAIPlanning::TrySustainedOutputPlant(const VSGameState &state, int row, SustainedOutputPolicy policy) {
    const int protectedSun = policy.protectedSun;
    const bool allowLowCostCombat = policy.allowLowCostCombat;
    const bool requirePreferredRow = policy.requirePreferredRow;
    const bool allowEmergencyTrade = policy.allowEmergencyTrade;
    const VSCardState *bestCard = nullptr;
    VSGridPosition bestTarget{};
    int bestScore = std::numeric_limits<int>::min();
    const bool peaCabbageTorchTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CABBAGEPULT)
        && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_TORCHWOOD);
    const bool threepeaterPuffTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_THREEPEATER) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM);
    const bool boomerangControlTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_BLOOMERANG);
    const bool sporeShellTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM);
    const bool melonMineTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_MELONPULT);
    const bool cactusSpikeTemplate = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CACTUS) && HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPIKEWEED);
    const int threepeaterCount = CountPlantType(state, SeedType::SEED_THREEPEATER);
    const int peaCount = CountPlantType(state, SeedType::SEED_PEASHOOTER);
    const int cabbageCount = CountPlantType(state, SeedType::SEED_CABBAGEPULT);
    for (const VSCardState &card : state.seedBanks[0]) {
        if (IsSlotBlocked(card.slot) || !IsReadyCard(card, state.plantSun)) {
            continue;
        }

        const SeedType seed = static_cast<SeedType>(card.seedType);
        const int totalCost = PlantAIPlanning::EffectivePlantPlayCost(state, card);
        if (totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
            continue;
        }
        // The dedicated Scaredy/Coffee replay tempo can start its rear
        // layer after two producers. Other decks retain the four-producer
        // guard so generic fallback never turns it into opening padding.
        const bool scaredyCoffeeTempoDeck =
            HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SCAREDYSHROOM) && (state.isNight || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_INSTANT_COFFEE));
        if (seed == SeedType::SEED_SCAREDYSHROOM && EffectivePlantEconomyCount(state) < (scaredyCoffeeTempoDeck ? 2 : 4)) {
            continue;
        }
        const bool lowCostCombat = allowLowCostCombat && card.cost <= 100 && (seed == SeedType::SEED_BONK_CHOY || seed == SeedType::SEED_CELERY_STALKER || seed == SeedType::SEED_CHOMPER);
        if (!IsSustainedOutputSeed(seed) && !lowCostCombat) {
            continue;
        }

        for (int rowOffset = 0; rowOffset < (requirePreferredRow ? 1 : state.rows); ++rowOffset) {
            const int targetRow = (row + rowOffset) % state.rows;
            const VSZombieState *closest = FindClosestZombie(state, targetRow);
            if (seed == SeedType::SEED_SNOWPEA && HasPlantTypeInRow(state, seed, targetRow)) {
                continue;
            }
            if (lowCostCombat && HasPlantTypeInRow(state, seed, targetRow)) {
                continue;
            }
            if (lowCostCombat && (closest == nullptr || (!closest->eating && closest->positionX > 660.0f))) {
                continue;
            }
            if (!lowCostCombat && !allowEmergencyTrade && !IsLobbedOutputSeed(seed) && IsRangedOutputTradeUnfavorable(state, targetRow)) {
                continue;
            }

            const VSGridPosition target = lowCostCombat ? FindPlantCellInExactRow(state, targetRow, 4, 4) : PlantAIPlanning::FindSustainedOutputCell(state, seed, targetRow);
            if (target.col < 0 || target.row < 0) {
                continue;
            }
            if (ShouldYieldLaneToMower(state, targetRow) || !IsPlantPlacementSafe(state, seed, target)) {
                continue;
            }

            const PlantLaneAssessment lane = AssessPlantLane(state, targetRow);
            const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, targetRow);
            const int existingOutput = SustainedOutputScoreInRow(state, targetRow);
            const int outputValue = IsSustainedOutputSeed(seed) ? SustainedOutputValue(seed) : 48;
            int score = outputValue * 3 - totalCost / 3;
            // The recordings invest firepower behind established sun
            // rows.  This protects sunk economy while avoiding a full
            // one-lane turtle: existing output lowers the next score.
            score += PlantEconomyValueInRow(state, targetRow) * 2;
            score += std::max(0, 110 - existingOutput) * 2;
            score -= existingOutput / 2;
            // A lane with enough plants on paper can still lose its
            // nearest zombie before those plants deal its health total.
            // Prioritize the concrete DPS shortfall over a second safe
            // economy row whenever that happens.
            score += firepower.deficit * 14;
            score += !firepower.canHold && firepower.nearHealth > 0 ? 135 : 0;
            // Once a firing lane reaches a grave, further shots convert
            // directly into lost zombie income.  That is worth more than
            // another safe Sunflower after the opening has stabilized.
            score += SeedEconomyPressureOpportunity(state, seed, targetRow) * 4;
            score += StrategyBonus(state, VSSide::Plants, seed, targetRow);
            // Replay priors identify the template; this bounded matchup
            // prior changes which legal carry is best against the live
            // zombie deck (metal screens, vehicles, ranged siege, etc.).
            score += ZombieDeckCounterBonus(state, seed, targetRow);
            // Local danger is handled by the counter branch. Durable
            // output belongs on a safe route where it can threaten the
            // grave economy instead of becoming a free blocker target.
            score -= !lowCostCombat && lane.danger >= 105 ? 80 : 0;
            score += targetRow == row ? 25 : 0;
            if (seed == SeedType::SEED_SNOWPEA) {
                score += 45;
            }
            if (seed == SeedType::SEED_STARFRUIT) {
                const int centerDistance = std::abs(targetRow * 2 - (state.rows - 1));
                score += 50 + (state.rows - centerDistance) * 8;
            }
            if (seed == SeedType::SEED_SPORESHROOM) {
                score += 35;
            }
            if (seed == SeedType::SEED_BLOOMERANG && boomerangControlTemplate) {
                // The Boomerang/Doom/Hypno replay uses Boomerang as the
                // stable grave-pressure layer. Hypno and Doom answer a
                // breakthrough separately, so do not wait for a crisis
                // before extending a safe firing lane into the mound field.
                score += 145 + SeedEconomyPressureOpportunity(state, seed, targetRow) * 2;
            }
            if (seed == SeedType::SEED_SPORESHROOM && sporeShellTemplate) {
                // This is still a one-carry deck: Pumpkin and Squash buy
                // time for Spore rather than replacing it with a second
                // output type. Prefer a fresh lane until every grave route
                // has a firing threat.
                score += 120 + (HasPlantTypeInRow(state, seed, targetRow) ? -90 : 55);
            }
            if (seed == SeedType::SEED_MELONPULT && melonMineTemplate) {
                // Potato Mine keeps the close route safe while Melon-pult
                // turns banked sun into broad grave pressure. Its placement
                // belongs on an exposed economic row, not merely the row
                // currently closest to a zombie.
                score += 105 + SeedEconomyPressureOpportunity(state, seed, targetRow) * 2;
            }
            if (seed == SeedType::SEED_CACTUS && cactusSpikeTemplate) {
                // Cactus is the durable carry; Spikeweed is its reactive
                // front trigger. Reward spreading cactus fire before a
                // second Spikeweed investment consumes the same lane.
                score += 85 + (HasPlantTypeInRow(state, seed, targetRow) ? -70 : 45);
            }
            if (seed == SeedType::SEED_THREEPEATER) {
                // A Threepeater covers its own lane plus both neighbours.
                // Score that complete firing band instead of evaluating it
                // as a narrow lane shooter, so the first two form the replay
                // opening at the inner rows and a later third fills the
                // middle only when it is still useful.
                int coveredLaneScore = 0;
                for (int coveredRow = std::max(0, targetRow - 1); coveredRow <= std::min(state.rows - 1, targetRow + 1); ++coveredRow) {
                    const PlantLaneFirepower coveredFirepower = AssessPlantLaneFirepower(state, coveredRow);
                    const int coveredOutput = SustainedOutputScoreInRow(state, coveredRow);
                    coveredLaneScore += std::max(0, 105 - coveredOutput) * 2;
                    coveredLaneScore += coveredFirepower.deficit * 7;
                    coveredLaneScore += PlantEconomyValueInRow(state, coveredRow);
                }
                score += coveredLaneScore;
                // Edge rows waste one of the three shots. They remain legal
                // when they are the only answer to a live deficit, but are
                // not a normal formation choice.
                score -= (targetRow == 0 || targetRow == state.rows - 1) ? 210 : 0;
                if (state.rows >= 3) {
                    const int firstCoreRow = state.rows / 2 - 1;
                    const int secondCoreRow = std::min(state.rows - 2, state.rows / 2 + 1);
                    if (threepeaterCount == 0) {
                        score += targetRow == firstCoreRow ? 180 : 0;
                    } else if (threepeaterCount == 1) {
                        score += targetRow == secondCoreRow ? 145 : 0;
                    } else {
                        score += targetRow == state.rows / 2 ? 85 : 0;
                    }
                }
                score += threepeaterPuffTemplate ? 135 : 0;
            }
            if (peaCabbageTorchTemplate) {
                // The Pea/Cabbage/Torch opening first establishes one pea
                // lane, then uses two lobbed lanes to punish the coming
                // Trashcan/Door screen before expanding the pea line. This
                // is a small opening ratio, not two competing carries: once
                // the support pair exists, Peashooter resumes its normal
                // numerical lead and Torchwood turns that line into the
                // template's main economic pressure.
                if (seed == SeedType::SEED_PEASHOOTER) {
                    score += peaCount == 0 ? 250 : (cabbageCount >= 2 ? 175 : 25);
                } else if (seed == SeedType::SEED_CABBAGEPULT) {
                    score += peaCount > 0 && cabbageCount < 2 ? 245 : 30;
                }
            }
            if (lowCostCombat) {
                // A melee plant is the cheap answer when a straight
                // projectile lane is screened by a Door or Trashcan.
                score += lane.danger >= 90 ? 95 : 20;
            }
            if (bestCard == nullptr || score > bestScore) {
                bestCard = &card;
                bestTarget = target;
                bestScore = score;
            }
        }
    }
    if (bestCard != nullptr) {
        return MakePlayAction(VSSide::Plants, *bestCard, bestTarget, state.boardTick);
    }

    // Daytime Sun-shrooms are front-line disposable pads. Sustained-output
    // placement computes its own firing cells and therefore does not pass
    // through TryPlantInRange; explicitly reclaim an occupied firing cell
    // before giving up on an otherwise legal carry placement.
    for (const VSCardState &card : state.seedBanks[0]) {
        if (IsSlotBlocked(card.slot) || !IsReadyCard(card, state.plantSun)) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        const int totalCost = PlantAIPlanning::EffectivePlantPlayCost(state, card);
        if (!IsSustainedOutputSeed(seed) || totalCost == std::numeric_limits<int>::max() || state.plantSun - totalCost < protectedSun) {
            continue;
        }

        int firstColumn = 0;
        int lastColumn = 3;
        switch (seed) {
            case SeedType::SEED_STARFRUIT:
            case SeedType::SEED_BLOOMERANG:
            case SeedType::SEED_THREEPEATER:
                firstColumn = 1;
                break;
            case SeedType::SEED_FUMESHROOM:
            case SeedType::SEED_GLOOMSHROOM:
                firstColumn = 2;
                break;
            case SeedType::SEED_SCAREDYSHROOM:
                lastColumn = 0;
                break;
            default:
                break;
        }
        if (std::optional<VSAction> action =
                TryClearDaytimeSunshroomForPlanting(state, seed, {.preferredRow = row, .firstColumn = firstColumn, .lastColumn = lastColumn, .requireExactRow = requirePreferredRow})) {
            return action;
        }
    }
    return allowLowCostCombat ? std::nullopt : PlantAIPlanning::TryRecycleIncomeForOutput(state, row, protectedSun);
}

bool PlantAIPlanning::HasIncomeSeed(const VSGameState &state) const {
    if (state.isSuddenDeath) {
        return false;
    }
    return std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [&state](const VSCardState &card) {
        return !card.matchRestricted
            && (card.seedType == static_cast<std::uint16_t>(SeedType::SEED_SUNFLOWER) || (state.isNight && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_SUNSHROOM)));
    });
}

bool PlantAIPlanning::HasSunshroomSeed(const VSGameState &state) const {
    return !state.isSuddenDeath && std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        return !card.matchRestricted && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_SUNSHROOM);
    });
}

bool PlantAIPlanning::HasEconomyPressurePlan(const VSGameState &state) const {
    return std::any_of(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &card) {
        if (card.matchRestricted || !card.active) {
            return false;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        return seed == SeedType::SEED_GRAVEBUSTER || IsSustainedOutputSeed(seed);
    });
}

int PlantAIPlanning::PrimaryOutputCost(const VSGameState &state) const {
    int primaryOutputCost = 0;
    for (const VSCardState &card : state.seedBanks[0]) {
        if (card.matchRestricted || !card.active) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        if (!IsSustainedOutputSeed(seed)) {
            continue;
        }
        int outputCost = std::max(0, card.cost);
        if (!state.isNight && PlantAIPlanning::IsDaytimeCoffeeMushroom(seed)) {
            const auto coffee = std::find_if(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &candidate) {
                return !candidate.matchRestricted && candidate.active && candidate.seedType == static_cast<std::uint16_t>(SeedType::SEED_INSTANT_COFFEE);
            });
            if (coffee == state.seedBanks[0].end()) {
                continue;
            }
            outputCost += std::max(0, coffee->cost);
        }
        primaryOutputCost = std::max(primaryOutputCost, outputCost);
    }
    return primaryOutputCost;
}

int PlantAIPlanning::MainCarryIncomeTarget(const VSGameState &state) const {
    const int compactTarget = state.rows >= 6 ? 5 : 4;
    const int primaryOutputCost = PlantAIPlanning::PrimaryOutputCost(state);
    if (primaryOutputCost <= 0) {
        return compactTarget;
    }

    // These are physical producers, not an enhanced-AI virtual economy. A
    // 200 Sun Melon-pult needs the eight-Sunflower bank that can actually
    // support repeated plays; a faster producer cooldown does not replace it.
    if (primaryOutputCost <= 50) {
        return std::max(compactTarget, 4);
    }
    if (primaryOutputCost <= 100) {
        return std::max(compactTarget, 5);
    }
    if (primaryOutputCost <= 150) {
        return std::max(compactTarget, 6);
    }
    if (primaryOutputCost <= 200) {
        return std::max(compactTarget, 8);
    }
    if (primaryOutputCost <= 250) {
        return std::max(compactTarget, 9);
    }
    return std::max(compactTarget, 10);
}

int PlantAIPlanning::EconomyPressureIncomeTarget(const VSGameState &state) const {
    bool hasGraveBuster = false;
    bool hasCrossLaneOutput = false;
    int cheapestOutputCost = std::numeric_limits<int>::max();
    const int primaryOutputCost = PlantAIPlanning::PrimaryOutputCost(state);
    for (const VSCardState &card : state.seedBanks[0]) {
        if (card.matchRestricted || !card.active) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        if (seed == SeedType::SEED_GRAVEBUSTER) {
            hasGraveBuster = true;
            continue;
        }
        if (!IsSustainedOutputSeed(seed)) {
            continue;
        }
        int outputCost = std::max(0, card.cost);
        if (!state.isNight && PlantAIPlanning::IsDaytimeCoffeeMushroom(seed)) {
            const auto coffee = std::find_if(state.seedBanks[0].begin(), state.seedBanks[0].end(), [](const VSCardState &candidate) {
                return !candidate.matchRestricted && candidate.active && candidate.seedType == static_cast<std::uint16_t>(SeedType::SEED_INSTANT_COFFEE);
            });
            if (coffee == state.seedBanks[0].end()) {
                continue;
            }
            outputCost += std::max(0, coffee->cost);
        }
        cheapestOutputCost = std::min(cheapestOutputCost, outputCost);
        hasCrossLaneOutput = hasCrossLaneOutput || seed == SeedType::SEED_STARFRUIT || seed == SeedType::SEED_THREEPEATER;
    }

    int contestedZombieRows = 0;
    int unholdableZombieRows = 0;
    int incomingZombieHealth = 0;
    int damageBeforeZombieContact = 0;
    for (int row = 0; row < state.rows; ++row) {
        const PlantLaneFirepower firepower = AssessPlantLaneFirepower(state, row);
        if (firepower.incomingHealth <= 0) {
            continue;
        }
        ++contestedZombieRows;
        incomingZombieHealth += firepower.incomingHealth;
        damageBeforeZombieContact += firepower.damageBeforeContact;
        if (!firepower.canHold || firepower.deficit > 0) {
            ++unholdableZombieRows;
        }
    }

    const int mainCarryTarget = MainCarryIncomeTarget(state);
    int target = mainCarryTarget;
    const int graveCount = CountZombieEconomy(state);
    const int phase = static_cast<int>(state.boardTick / 16000);
    // Recorded VS openings start rebuilding Sunflower income once the first
    // firing layer exists and the zombie side has established a few graves;
    // waiting for a fully developed grave board leaves expensive carries
    // permanently underfunded.
    const bool midGame = phase >= 1 || graveCount >= std::max(2, (state.rows + 1) / 2);
    const bool pressureOutrunsFirepower = unholdableZombieRows > 0 || (contestedZombieRows >= 2 && damageBeforeZombieContact < incomingZombieHealth);
    const bool boardCanSafelyGrow = contestedZombieRows == 0 || !pressureOutrunsFirepower;
    const int outputCount = CountSustainedOutputPlants(state);

    // The carry-cost target is an opening floor. Tactical pressure can defer
    // a safe Sunflower for a defense, but may never silently lower the bank
    // needed to start that main damage card.
    if (boardCanSafelyGrow && primaryOutputCost >= 225 && (!midGame || outputCount >= std::max(2, state.rows / 2))) {
        ++target;
    }
    if (boardCanSafelyGrow && !midGame && primaryOutputCost >= 225 && state.plantSun < primaryOutputCost) {
        ++target;
    }
    if (boardCanSafelyGrow && midGame && outputCount > 0 && !pressureOutrunsFirepower) {
        ++target;
    }
    if (cheapestOutputCost == std::numeric_limits<int>::max() && !hasGraveBuster && !hasCrossLaneOutput && graveCount <= state.rows && !pressureOutrunsFirepower) {
        ++target;
    }
    return std::clamp(target, mainCarryTarget, 11);
}

} // namespace vsai::detail
