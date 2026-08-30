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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_DECISION_CONTEXT_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_DECISION_CONTEXT_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIZombie/ZombieCardRules.h"

#include <array>

namespace vsai::detail {

struct ZombieRowScoreFacts {
    PlantLaneAssessment lane;
    int plantCount = 0;
    int zombieCount = 0;
    int graveProjectileThreat = 0;
    int lobbedProjectileThreat = 0;
    int graveScreenDeficit = 0;
    int sustainedOutput = 0;
    int economyValue = 0;
    int areaCounterExposure = 0;
    bool hasSnowPea = false;
    bool hasBonkChoy = false;
    bool hasWallnut = false;
    bool hasPumpkinShell = false;
    bool hasLobbedPlant = false;
    bool hasGraveGuard = false;
};

struct ZombieDecisionContext {
    ZombieTempoPolicy tempo;
    ZombieTemplateProfile templateProfile;
    int actualEconomyCount = 0;
    int economyCount = 0;
    int economyTarget = 0;
    int economyDeficit = 0;
    int activePressureRows = 0;
    int heavyEconomyThreshold = 0;
    int targetMarkersOnBoard = 0;
    int zeroHealthTargetMarkers = 0;
    int criticalTargetRow = -1;
    int graveDefenseRow = -1;
    int graveDefenseScore = 0;
    int graveStraightThreat = 0;
    int graveLobbedThreat = 0;
    int graveScreenDeficit = 0;
    int heavyZombieReserve = 0;
    int minimumOpeningEconomy = 0;
    int desiredOpeningRows = 0;
    int openingPressureEconomyFloor = 0;
    int openingPressureEconomyCeiling = 0;
    int survivingFrontRow = -1;
    int survivingFrontValue = 0;
    int economicRow = -1;
    int livePlantCount = 0;
    int peaHeadCount = 0;
    std::array<ZombieRowScoreFacts, 6> rowScoreFacts{};
    bool hasPlants = false;
    bool plantHasMagnet = false;
    bool plantHasPeaCarry = false;
    bool plantHasShortPult = false;
    bool plantHasLobbedCard = false;
    bool plantHasNutCard = false;
    bool plantHasHighValueCarryCard = false;
    bool targetDefenseEmergency = false;
    bool graveDefenseUrgent = false;
    bool graveDefenseReinforcement = false;
    bool hasGraveGuard = false;
    bool bankForHeavy = false;
    bool hasReadyFrontlineProbe = false;
    bool hasReadyEarlyHeavyCommit = false;
    bool armoredNormalRushTemplate = false;
    bool firstGraveProbe = false;
    bool openingPressureCadence = false;
    bool enhancedPressureRecovery = false;
    bool forceOpeningPressure = false;
    bool preservePressureDuringRepair = false;
    bool preserveSurvivingFront = false;
    bool survivingFrontGuarded = false;
    bool restorationCanProceed = false;
    bool restorationOutweighsFront = false;
    bool economyRepairIsUrgent = false;
    bool hasReadyTemplateCommit = false;
    bool canConvertMowerlessTargetRoute = false;
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_DECISION_CONTEXT_H
