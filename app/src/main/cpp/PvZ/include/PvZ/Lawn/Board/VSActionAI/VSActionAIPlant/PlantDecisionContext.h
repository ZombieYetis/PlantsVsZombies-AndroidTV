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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_DECISION_CONTEXT_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_DECISION_CONTEXT_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIThreat.h"

#include <optional>

namespace vsai::detail {

// All values are derived from one VSGameState snapshot before action
// priorities are evaluated. Decision phases must treat this as immutable.
struct PlantDecisionContext {
    PlantLaneAssessment danger;
    PlantLaneAssessment counterLane;
    PlantLaneFirepower counterFirepower;
    PlantLaneFirepower weakestFirepower;
    int actualIncomePlantCount = 0;
    int incomePlantCount = 0;
    int openingIncomeTarget = 0;
    int minimumIncomeBeforeOutput = 0;
    int sustainedOutputCount = 0;
    int counterRow = 0;
    int firepowerRow = 0;
    int counterFirstColumn = 4;
    int areaCounterReserve = 0;
    int incomeExpansionTarget = 0;
    int protectedSun = 0;
    int zombieEconomyStrikeRow = 0;
    int maximumOutputCount = 0;
    int desiredOutputCount = 0;
    int largestFirepowerDeficit = 0;
    int counterCombatPlants = 0;
    bool metalZombieDeck = false;
    bool economyZombieDeck = false;
    bool rangedZombieDeck = false;
    bool hasIncomeSeed = false;
    bool hasSunshroomFiller = false;
    bool hasSustainedOutputSeed = false;
    bool hasActiveZombie = false;
    bool mowerlessThirdColumnEmergency = false;
    bool zombieCluster = false;
    bool squashThreat = false;
    bool impPearThreat = false;
    bool midGame = false;
    bool pressureOutrunsFirepower = false;
    bool immediateCounterThreat = false;
    bool canStrikeZombieEconomy = false;
    bool needsSustainedOutput = false;
    bool highSunCombatPressure = false;
    bool readySustainedOutput = false;
    bool openingNeedsFirepower = false;
    bool outputTempoHasPriority = false;
    bool mayExpandIncomePastOpening = false;
    const VSZombieState *counterClosest = nullptr;
    int zamboniRow = -1;
    int impactThreatRow = -1;
};

struct PlantDecisionResult {
    bool handled = false;
    std::optional<VSAction> action;
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_DECISION_CONTEXT_H
