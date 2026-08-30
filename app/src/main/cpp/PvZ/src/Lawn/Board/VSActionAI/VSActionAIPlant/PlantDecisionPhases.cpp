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

#include <optional>

namespace vsai::detail {

PlantDecisionResult PlantAI::TryOpeningEconomyPhase(const VSGameState &state) {
    if (state.isSuddenDeath || mOpeningEconomyPlaced) {
        return {};
    }

    const SeedType openingSeed = state.isNight ? SeedType::SEED_SUNSHROOM : SeedType::SEED_SUNFLOWER;
    const VSCardState *card = FindReadyCard(state, openingSeed);
    const VSGridPosition target = FindSafeIncomeCell(state, LeastDevelopedPlantRow(state));
    if (card != nullptr && target.col >= 0 && target.row >= 0 && state.plantSun >= card->cost) {
        return {.handled = true, .action = MakePlayAction(VSSide::Plants, *card, target, state.boardTick)};
    }
    return {.handled = true};
}

std::optional<VSAction> PlantAI::TryImmediateMaintenancePhase(const VSGameState &state) {
    if (std::optional<VSAction> action = TryEvadeJalapenoHead(state)) {
        return action;
    }
    for (const VSResourceState &resource : state.resources) {
        if (resource.side == VSSide::Plants && !resource.dead && !resource.beingCollected) {
            return MakeCollectResourceAction(VSSide::Plants, resource.id);
        }
    }
    if (std::optional<VSAction> action = TryBlover(state, MostUrgentCounterRow(state))) {
        return action;
    }
    return TryRemoveLadderedNut(state);
}

PlantDecisionResult PlantAI::TryOpeningOutputPhase(const VSGameState &state, const PlantDecisionContext &context) {
    if (context.openingNeedsFirepower) {
        if (context.hasSustainedOutputSeed) {
            if (std::optional<VSAction> action = TrySustainedOutputPlant(
                    state, context.firepowerRow, {.protectedSun = context.protectedSun, .allowLowCostCombat = true, .requirePreferredRow = true, .allowEmergencyTrade = true})) {
                return {.handled = true, .action = action};
            }
        }
        if (std::optional<VSAction> action = TryWakeableMushroomOutput(state, context.firepowerRow, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (std::optional<VSAction> action = TryWakeSleepingMushroom(state, context.danger.row)) {
        return {.handled = true, .action = action};
    }
    return {};
}

std::optional<VSAction> PlantAI::TryFirstTemplateTactic(const VSGameState &state, const PlantDecisionContext &context, std::initializer_list<TemplateTacticStep> tactics) {
    for (const TemplateTacticStep step : tactics) {
        const int preferredRow = step.useFirepowerRow ? context.firepowerRow : context.zombieEconomyStrikeRow;
        if (std::optional<VSAction> action = (this->*step.tactic)(state, preferredRow, context.protectedSun)) {
            return action;
        }
    }
    return std::nullopt;
}

PlantDecisionResult PlantAI::TryTemplatePressurePhase(const VSGameState &state, const PlantDecisionContext &context) {
    const bool mustFundMainCarry = context.hasIncomeSeed && context.actualIncomePlantCount < context.openingIncomeTarget && !context.immediateCounterThreat && !context.openingNeedsFirepower
        && !context.pressureOutrunsFirepower && context.danger.danger < 145 && (context.counterFirepower.canHold || context.weakestFirepower.closestDistance > 760);
    if (mustFundMainCarry) {
        if (std::optional<VSAction> action = TryIncomePlant(state, LeastDevelopedPlantRow(state), context.protectedSun)) {
            return {.handled = true, .action = action};
        }
        return {.handled = true};
    }
    if (!context.immediateCounterThreat && !context.openingNeedsFirepower && CountZombieEconomy(state) > 0) {
        if (context.economyZombieDeck) {
            if (std::optional<VSAction> action = TryGraveBuster(state, context.protectedSun)) {
                return {.handled = true, .action = action};
            }
        }
        if (std::optional<VSAction> action = TryFirstTemplateTactic(state,
                                                                    context,
                                                                    {
                                                                        {&PlantAI::TryMelonScaredySupport, false},
                                                                        {&PlantAI::TryScaredyCoffeeTempo, false},
                                                                        {&PlantAI::TryPeaPuffTempoOpening, false},
                                                                        {&PlantAI::TryPeaCeleryAshTempo, false},
                                                                        {&PlantAI::TrySporePuffTempoPressure, false},
                                                                        {&PlantAI::TryPeaCabbageTorchTempo, false},
                                                                    })) {
            return {.handled = true, .action = action};
        }
    }
    if (!context.immediateCounterThreat && !context.openingNeedsFirepower) {
        if (std::optional<VSAction> action = TryFirstTemplateTactic(state,
                                                                    context,
                                                                    {
                                                                        {&PlantAI::TryFumeDoomPressure, false},
                                                                        {&PlantAI::TrySporeShellPressure, false},
                                                                        {&PlantAI::TryBoomerangControlPressure, false},
                                                                        {&PlantAI::TryBoomerangGarlicFormation, false},
                                                                        {&PlantAI::TryThreepeaterPuffFormation, true},
                                                                        {&PlantAI::TrySnowpeaPuffMagnetPressure, true},
                                                                        {&PlantAI::TrySnowpeaBonkFormation, false},
                                                                        {&PlantAI::TryPeaDoomTempoPressure, false},
                                                                        {&PlantAI::TryStarfruitCrossfireFormation, true},
                                                                        {&PlantAI::TryCactusSpikeweedCore, true},
                                                                        {&PlantAI::TryKernelCeleryFormation, false},
                                                                        {&PlantAI::TryRepeaterCeleryTempo, false},
                                                                        {&PlantAI::TryMelonMineTempo, false},
                                                                        {&PlantAI::TryRepeaterTempoPressure, false},
                                                                        {&PlantAI::TryMagnetShroom, true},
                                                                        {&PlantAI::TryScaredyMelonSupport, true},
                                                                        {&PlantAI::TryScaredyPuffDoomPressure, true},
                                                                    })) {
            return {.handled = true, .action = action};
        }
        if (context.hasActiveZombie) {
            if (std::optional<VSAction> action = TryFirstTemplateTactic(state,
                                                                        context,
                                                                        {
                                                                            {&PlantAI::TryStarfruitPuffPressure, true},
                                                                            {&PlantAI::TryPeaPuffPressure, true},
                                                                        })) {
                return {.handled = true, .action = action};
            }
        }
    }
    if (context.hasActiveZombie && !context.immediateCounterThreat) {
        if (std::optional<VSAction> action = TrySporePuffPressure(state, context.firepowerRow, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.hasActiveZombie && !context.immediateCounterThreat && (context.largestFirepowerDeficit > 0 || context.counterLane.danger >= 95)) {
        if (std::optional<VSAction> action = TryWakeableMushroomOutput(state, context.firepowerRow, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.hasActiveZombie && !context.immediateCounterThreat) {
        if (std::optional<VSAction> action = TryStarfruitGarlicFormation(state, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    return {};
}

PlantDecisionResult PlantAI::TryEconomyConversionPhase(const VSGameState &state, const PlantDecisionContext &context) {
    if (context.highSunCombatPressure && !context.immediateCounterThreat) {
        if (std::optional<VSAction> action = TrySustainedOutputPlant(state, context.firepowerRow, {.protectedSun = context.protectedSun, .allowLowCostCombat = true})) {
            return {.handled = true, .action = action};
        }
    }
    if (context.hasActiveZombie && !context.immediateCounterThreat && context.hasSustainedOutputSeed && state.plantSun - context.protectedSun >= 200
        && (context.danger.danger >= 80 || context.largestFirepowerDeficit > 0)) {
        if (std::optional<VSAction> action = TrySustainedOutputPlant(state, context.firepowerRow, {.protectedSun = context.protectedSun, .allowLowCostCombat = true, .allowEmergencyTrade = true})) {
            return {.handled = true, .action = action};
        }
    }
    if (context.canStrikeZombieEconomy) {
        if (std::optional<VSAction> action = TryGraveBuster(state, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
        if (context.hasSustainedOutputSeed) {
            if (std::optional<VSAction> action = TrySustainedOutputPlant(state, context.zombieEconomyStrikeRow, {.protectedSun = context.protectedSun})) {
                return {.handled = true, .action = action};
            }
        }
    }
    const bool daytimePadTemplate = !state.isNight && context.hasSunshroomFiller && !context.hasIncomeSeed;
    if (!state.isNight && context.hasSunshroomFiller && !context.immediateCounterThreat && (context.hasActiveZombie || daytimePadTemplate)
        && (context.incomePlantCount >= context.minimumIncomeBeforeOutput || context.sustainedOutputCount > 0 || daytimePadTemplate)) {
        if (std::optional<VSAction> action = TrySunshroomFiller(state, context.danger.row, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.zombieCluster && context.areaCounterReserve > 0 && state.plantSun < context.areaCounterReserve) {
        return {.handled = true};
    }
    if (!context.immediateCounterThreat && context.danger.danger < 105 && context.incomePlantCount >= 6 && context.sustainedOutputCount >= 3) {
        if (std::optional<VSAction> action = TryTorchwoodSupport(state, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    const bool safeIncomeShortfall = context.incomePlantCount < context.openingIncomeTarget && context.danger.danger < 105 && !context.pressureOutrunsFirepower;
    if (context.hasIncomeSeed && context.incomePlantCount < context.openingIncomeTarget && context.danger.danger < 150 && (!context.highSunCombatPressure || safeIncomeShortfall)) {
        if (context.incomePlantCount >= context.minimumIncomeBeforeOutput && context.needsSustainedOutput) {
            if (std::optional<VSAction> action = TrySustainedOutputPlant(state, LeastDevelopedPlantRow(state), {.protectedSun = context.protectedSun})) {
                return {.handled = true, .action = action};
            }
        }
        if (std::optional<VSAction> action = TryIncomePlant(state, LeastDevelopedPlantRow(state), context.protectedSun)) {
            return {.handled = true, .action = action};
        }
        if (!context.hasActiveZombie || context.danger.danger < 90) {
            return {.handled = true};
        }
    }
    const bool canExpandIncome = context.mayExpandIncomePastOpening && (context.danger.danger < 105 || (context.counterCombatPlants > 0 && context.danger.danger < 140))
        && (context.counterFirepower.canHold || context.weakestFirepower.closestDistance > 760);
    if (context.hasIncomeSeed && context.hasActiveZombie && context.incomePlantCount < context.incomeExpansionTarget && !context.immediateCounterThreat && canExpandIncome
        && (!context.highSunCombatPressure || (context.midGame && context.incomePlantCount < context.incomeExpansionTarget)) && !context.outputTempoHasPriority) {
        if (context.needsSustainedOutput) {
            if (std::optional<VSAction> action = TrySustainedOutputPlant(state, context.firepowerRow, {.protectedSun = context.protectedSun})) {
                return {.handled = true, .action = action};
            }
        }
        if (std::optional<VSAction> action = TryIncomePlant(state, LeastDevelopedPlantRow(state), context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.hasActiveZombie) {
        return {};
    }

    const int buildRow = LeastDevelopedPlantRow(state);
    if (context.hasIncomeSeed && context.incomePlantCount < context.minimumIncomeBeforeOutput) {
        if (std::optional<VSAction> action = TryIncomePlant(state, buildRow, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.needsSustainedOutput) {
        if (std::optional<VSAction> action = TrySustainedOutputPlant(state, buildRow, {.protectedSun = context.protectedSun})) {
            return {.handled = true, .action = action};
        }
    }
    if (context.hasIncomeSeed && context.incomePlantCount < context.incomeExpansionTarget
        && (!context.highSunCombatPressure || (context.midGame && context.incomePlantCount < context.incomeExpansionTarget)) && !context.outputTempoHasPriority && context.mayExpandIncomePastOpening) {
        if (std::optional<VSAction> action = TryIncomePlant(state, buildRow, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.incomePlantCount >= 6 && context.sustainedOutputCount >= 3) {
        if (std::optional<VSAction> action = TryTorchwoodSupport(state, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (std::optional<VSAction> action = TrySustainedOutputPlant(state, buildRow, {.protectedSun = context.protectedSun})) {
        return {.handled = true, .action = action};
    }
    return {.handled = true, .action = TryFallbackPlant(state, context.danger, buildRow)};
}

PlantDecisionResult PlantAI::TryLaneDefensePhase(const VSGameState &state, const PlantDecisionContext &context) {
    const bool hasRangedHarasser = HasZombieTypeInRow(state, context.danger.row, ZombieType::ZOMBIE_PEA_HEAD) || HasZombieTypeInRow(state, context.danger.row, ZombieType::ZOMBIE_SUNDAY_EDITION);
    if (context.danger.danger >= 85 && (hasRangedHarasser || SustainedOutputScoreInRow(state, context.danger.row) >= 55)) {
        if (std::optional<VSAction> action = TryPumpkinShell(state, context.danger.row, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    const bool yieldDangerLane = ShouldYieldLaneToMower(state, context.danger.row);
    if (context.danger.danger >= 105 && !yieldDangerLane) {
        if (!IsRangedOutputTradeUnfavorable(state, context.danger.row) && !HasPlantTypeInRow(state, SeedType::SEED_SNOWPEA, context.danger.row)) {
            if (std::optional<VSAction> action = TryPlant(state, SeedType::SEED_SNOWPEA, context.danger.row, 1, 2)) {
                return {.handled = true, .action = action};
            }
        }
        if (!HasPlantTypeInRow(state, SeedType::SEED_BONK_CHOY, context.danger.row)) {
            if (std::optional<VSAction> action = TryPlant(state, SeedType::SEED_BONK_CHOY, context.danger.row, 3, 3)) {
                return {.handled = true, .action = action};
            }
        }
        if (std::optional<VSAction> action = TryPumpkinShell(state, context.danger.row, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
        if (ShouldDeployWallnut(state, context.danger.row)) {
            if (std::optional<VSAction> action = TryPlant(state, SeedType::SEED_WALLNUT, context.danger.row, 3, 5)) {
                return {.handled = true, .action = action};
            }
        }
        if (std::optional<VSAction> action = TrySpikeweed(state, context.danger.row, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    const int buildRow = LeastDevelopedPlantRow(state);
    if (!context.immediateCounterThreat && context.incomePlantCount >= 6 && context.sustainedOutputCount >= 3) {
        if (std::optional<VSAction> action = TryTorchwoodSupport(state, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (std::optional<VSAction> action = TrySustainedOutputPlant(state, buildRow, {.protectedSun = context.protectedSun})) {
        return {.handled = true, .action = action};
    }
    if (context.hasActiveZombie && !ShouldYieldLaneToMower(state, buildRow) && !HasPlantTypeInRow(state, SeedType::SEED_BONK_CHOY, buildRow)) {
        if (std::optional<VSAction> action = TryPlant(state, SeedType::SEED_BONK_CHOY, buildRow, 3, 3)) {
            return {.handled = true, .action = action};
        }
    }
    if (!yieldDangerLane && ShouldDeployWallnut(state, context.danger.row)) {
        if (std::optional<VSAction> action = TryPlant(state, SeedType::SEED_WALLNUT, context.danger.row, 3, 5)) {
            return {.handled = true, .action = action};
        }
    }
    if (!yieldDangerLane) {
        if (std::optional<VSAction> action = TrySpikeweed(state, context.danger.row, context.protectedSun)) {
            return {.handled = true, .action = action};
        }
    }
    if (context.hasIncomeSeed && context.incomePlantCount < context.incomeExpansionTarget
        && (!context.highSunCombatPressure || (context.midGame && context.incomePlantCount < context.incomeExpansionTarget)) && !context.outputTempoHasPriority && context.mayExpandIncomePastOpening) {
        return {.handled = true, .action = TryIncomePlant(state, buildRow, context.protectedSun)};
    }
    return {};
}

PlantDecisionResult PlantAI::TryFallbackPhase(const VSGameState &state, const PlantDecisionContext &context) {
    const int buildRow = LeastDevelopedPlantRow(state);
    if (context.hasActiveZombie && state.plantSun - context.protectedSun >= 125) {
        if (std::optional<VSAction> action = TrySustainedOutputPlant(state, context.firepowerRow, {.protectedSun = context.protectedSun, .allowLowCostCombat = true, .allowEmergencyTrade = true})) {
            return {.handled = true, .action = action};
        }
    }
    return {.handled = true, .action = TryFallbackPlant(state, context.danger, buildRow)};
}

} // namespace vsai::detail
