#include "../VSActionAILanePolicy.h"
#include "ZombieAIPlanning.h"

#include <algorithm>

namespace vsai::detail {

ZombieDecisionContext ZombieAIPlanning::BuildDecisionContext(const VSGameState &state) const {
    ZombieDecisionContext context{.tempo = GetZombieTempoPolicy()};
    context.livePlantCount = CountLivePlants(state);
    context.hasPlants = context.livePlantCount > 0;
    context.plantHasMagnet = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_MAGNETSHROOM) || CountPlantType(state, SeedType::SEED_MAGNETSHROOM) > 0;
    context.plantHasPeaCarry = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PEASHOOTER) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_REPEATER)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_THREEPEATER);
    context.plantHasShortPult = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_FUMESHROOM) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM);
    context.plantHasLobbedCard = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_CABBAGEPULT) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_KERNELPULT)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_MELONPULT) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_WINTERMELON)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_FUMESHROOM)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUFFSHROOM);
    context.plantHasNutCard = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_WALLNUT) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_PUMPKINSHELL);
    context.plantHasHighValueCarryCard = HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_MELONPULT) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_SPORESHROOM)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_STARFRUIT) || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_REPEATER)
        || HasActiveDeckCard(state, VSSide::Plants, SeedType::SEED_THREEPEATER);
    context.peaHeadCount = static_cast<int>(std::count_if(
        state.zombies.begin(), state.zombies.end(), [](const VSZombieState &zombie) { return !zombie.dead && zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_PEA_HEAD); }));
    for (int row = 0; row < state.rows && row < static_cast<int>(context.rowScoreFacts.size()); ++row) {
        ZombieRowScoreFacts &facts = context.rowScoreFacts[static_cast<std::size_t>(row)];
        facts.hasSnowPea = HasPlantTypeInRow(state, SeedType::SEED_SNOWPEA, row);
        facts.hasBonkChoy = HasPlantTypeInRow(state, SeedType::SEED_BONK_CHOY, row);
        facts.hasWallnut = HasPlantTypeInRow(state, SeedType::SEED_WALLNUT, row) || HasPlantTypeInRow(state, SeedType::SEED_TALLNUT, row);
        facts.hasPumpkinShell = HasPlantTypeInRow(state, SeedType::SEED_PUMPKINSHELL, row);
        facts.plantCount = CountPlantsInRow(state, row);
        facts.zombieCount = CountZombiesInRow(state, row);
        facts.graveProjectileThreat = StraightProjectileThreatScore(state, row);
        facts.lobbedProjectileThreat = LobbedProjectileThreatScore(state, row);
        facts.hasLobbedPlant = HasLobbedPlantInRow(state, row);
        facts.graveScreenDeficit = ZombieGraveScreenDeficit(state, row);
        facts.hasGraveGuard = HasZombieGraveGuardInRow(state, row);
        facts.sustainedOutput = SustainedOutputScoreInRow(state, row);
        facts.economyValue = PlantEconomyValueInRow(state, row);
        facts.lane = AssessPlantLane(state, row);
        facts.areaCounterExposure = PlantAreaCounterExposure(state, row);
    }
    context.actualEconomyCount = CountZombieEconomy(state);
    context.economyCount = context.tempo.EffectiveEconomyCount(context.actualEconomyCount);
    context.activePressureRows = CountActiveZombieRows(state);
    context.economyTarget = state.isSuddenDeath ? context.economyCount : context.tempo.EconomyTarget(std::max(state.rows * 2, state.rows * 3), state.rows, context.activePressureRows);
    context.economyDeficit = std::max(0, context.economyTarget - context.economyCount);
    context.heavyEconomyThreshold = HeavyZombieEconomyThreshold(state);
    context.templateProfile = DetectZombieTemplateProfile(state);
    for (const VSGridItemState &item : state.gridItems) {
        if (item.gridItemType != static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE)) {
            continue;
        }
        ++context.targetMarkersOnBoard;
        context.zeroHealthTargetMarkers += !item.dead && item.health <= 0 ? 1 : 0;
    }
    const int historicalTargetLosses = std::max(0, state.rows - state.liveZombieTargetCount);
    context.targetDefenseEmergency = context.targetMarkersOnBoard > 0 && historicalTargetLosses + context.zeroHealthTargetMarkers >= 2;
    if (context.targetDefenseEmergency) {
        int finalTargetThreat = std::numeric_limits<int>::min();
        for (const VSGridItemState &item : state.gridItems) {
            if (item.gridItemType != static_cast<std::uint16_t>(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) || item.dead || item.health <= 0 || item.position.row < 0 || item.position.row >= state.rows) {
                continue;
            }
            const int row = item.position.row;
            const int threat = ProtectableGraveThreatScore(state, row) * 3 + ZombieGraveScreenDeficit(state, row) * 2 + ZombieFrontlineValueInRow(state, row);
            if (context.criticalTargetRow < 0 || threat > finalTargetThreat) {
                context.criticalTargetRow = row;
                finalTargetThreat = threat;
            }
        }
    }
    context.graveDefenseRow = context.criticalTargetRow >= 0 ? context.criticalTargetRow : MostThreatenedEconomyRow(state);
    context.graveDefenseScore = ProtectableGraveThreatScore(state, context.graveDefenseRow);
    context.graveStraightThreat = StraightProjectileThreatScore(state, context.graveDefenseRow);
    context.graveLobbedThreat = LobbedProjectileThreatScore(state, context.graveDefenseRow);
    context.graveScreenDeficit = ZombieGraveScreenDeficit(state, context.graveDefenseRow);
    context.hasGraveGuard = HasZombieGraveGuardInRow(state, context.graveDefenseRow);
    const bool proactiveGraveScreen = NeedsProactiveGraveScreen(state, context.graveDefenseRow);
    context.graveDefenseUrgent = context.targetDefenseEmergency || context.graveDefenseScore >= 50 || context.graveStraightThreat >= 55 || context.graveLobbedThreat >= 70
        || context.graveScreenDeficit >= 55 || proactiveGraveScreen;
    context.graveDefenseReinforcement = context.graveDefenseUrgent && (context.targetDefenseEmergency || !context.hasGraveGuard || context.graveScreenDeficit >= 55);
    context.heavyZombieReserve = HeavyZombieReserve(state);
    context.bankForHeavy = context.heavyZombieReserve >= 100 && context.economyCount >= context.tempo.HeavyBankEconomyThreshold(state.rows, context.heavyEconomyThreshold)
        && context.tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows) && CountLivePlants(state) >= state.rows && context.graveDefenseScore < 100;
    context.minimumOpeningEconomy = context.tempo.OpeningEconomyFloor(std::min(2, std::max(1, state.rows)));
    context.desiredOpeningRows = context.tempo.OpeningPressureRowTarget(std::min(3, state.rows), state.rows);
    context.hasReadyFrontlineProbe = HasReadyFrontlineProbe(state);
    context.hasReadyEarlyHeavyCommit = HasReadyEarlyHeavyCommit(state, context);
    context.armoredNormalRushTemplate = context.templateProfile.Has(ZombieTemplate::ArmoredNormalRush);
    const bool impPailSundayTemplate = context.templateProfile.Has(ZombieTemplate::ImpSledSunday);
    const bool zamboniPoleOpeningTemplate = context.templateProfile.Has(ZombieTemplate::ZamboniPole);
    context.openingPressureEconomyFloor = (impPailSundayTemplate || zamboniPoleOpeningTemplate) ? context.tempo.OpeningEconomyFloor(std::min(3, state.rows)) : context.minimumOpeningEconomy;
    context.firstGraveProbe = context.armoredNormalRushTemplate && context.actualEconomyCount == 1 && context.activePressureRows == 0 && context.hasReadyFrontlineProbe
        && mLastPressureEconomyCount < context.actualEconomyCount;
    context.openingPressureEconomyCeiling = context.tempo.OpeningEconomyCeiling(state.rows + 1);
    context.openingPressureCadence = context.economyCount >= context.openingPressureEconomyFloor && context.economyCount <= context.openingPressureEconomyCeiling
        && context.activePressureRows < context.desiredOpeningRows && mLastPressureEconomyCount < context.actualEconomyCount;
    context.enhancedPressureRecovery = context.tempo.ShouldExtendPressure(context.economyCount, context.activePressureRows, state.rows);
    context.forceOpeningPressure = context.firstGraveProbe || (context.hasReadyFrontlineProbe && (context.openingPressureCadence || context.enhancedPressureRecovery));
    context.preservePressureDuringRepair = context.economyCount >= context.minimumOpeningEconomy && context.economyDeficit <= context.tempo.PressureRepairDeficitTolerance()
        && context.activePressureRows > 0 && context.hasReadyFrontlineProbe;
    context.survivingFrontRow = context.criticalTargetRow >= 0 ? context.criticalTargetRow : MostValuableZombieFrontRow(state);
    context.survivingFrontValue = ZombieFrontlineValueInRow(state, context.survivingFrontRow);
    context.preserveSurvivingFront = context.criticalTargetRow >= 0 || (context.economyCount >= state.rows && context.activePressureRows == 1 && context.survivingFrontValue >= 90);
    context.survivingFrontGuarded = HasZombieGraveGuardInRow(state, context.survivingFrontRow);
    context.economicRow = context.economyCount < state.rows * 2 ? LeastCommittedZombieRow(state) : LeastThreatenedEconomyRow(state);
    context.restorationCanProceed = !context.graveDefenseReinforcement || context.hasGraveGuard;
    context.restorationOutweighsFront = context.economyDeficit >= 2 || context.graveDefenseScore < 100 || context.hasGraveGuard;
    context.economyRepairIsUrgent = context.economyCount < context.minimumOpeningEconomy || context.economyDeficit >= context.tempo.EconomyRepairDeficitThreshold()
        || (context.tempo.IsEnhanced() && context.economyDeficit > 0 && context.activePressureRows >= std::min(2, state.rows));
    context.hasReadyTemplateCommit = HasReadyZombieTemplateCommit(state, context.templateProfile, context.tempo, context.actualEconomyCount, context.activePressureRows);
    for (int row = 0; row < state.rows; ++row) {
        if (EvaluateZombieLanePolicy(state, row).conversionRoute) {
            context.canConvertMowerlessTargetRoute = true;
            break;
        }
    }
    return context;
}

} // namespace vsai::detail
