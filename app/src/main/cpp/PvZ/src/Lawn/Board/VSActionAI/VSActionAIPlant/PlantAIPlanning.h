#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_PLANNING_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_PLANNING_H

#include "../VSActionAIStrategy.h"
#include "PlantDecisionContext.h"

#include <limits>
#include <optional>

namespace vsai::detail {

bool IsLobbedOutputSeed(SeedType seed);

class PlantAIPlanning : public BuiltinVSAgent {
protected:
    struct SustainedOutputPolicy {
        int protectedSun = 0;
        bool allowLowCostCombat = false;
        bool requirePreferredRow = false;
        bool allowEmergencyTrade = false;
    };

    struct PlantPlacementRange {
        int preferredRow = -1;
        int firstColumn = 0;
        int lastColumn = 5;
        bool requireExactRow = false;
    };

    // Shared policy for template tactics that differ only in their carry
    // card, economy gate, and score coefficients. Deck-specific gates remain
    // in their tactics so this cannot replace formation-specific behavior.
    struct TemplateOutputPolicy {
        SeedType seed = SeedType::SEED_NONE;
        int targetCount = 0;
        int economyPressureWeight = 0;
        int firepowerDeficitWeight = 0;
        int noZombieScore = 0;
        float distantZombieThreshold = 0.0f;
        int distantZombieScore = 0;
        int closeZombieScore = 0;
        int preferredRowBonus = 0;
        bool requireFavorableRangedTrade = false;
        bool useEffectiveCost = false;
    };

    // Match-local memory: Reset clears the opening gate and the short ash
    // history; OnActionResult advances each only after an applied action.
    bool mOpeningEconomyPlaced = false;
    VSGridPosition mRecentAshTarget{-1, -1};
    std::uint32_t mRecentAshTick = 0;
    bool mHasRecentAsh = false;

    struct AshTarget {
        VSGridPosition position{-1, -1};
        int hitCount = 0;
        int totalHealth = 0;
        int highValueCount = 0;
        int giantCount = 0;
        int pailCount = 0;
        float frontMostX = std::numeric_limits<float>::max();
        bool mowerlessThirdColumn = false;
        bool mowerlessHomeColumn = false;
        int score = std::numeric_limits<int>::min();
    };

    static bool IsDaytimeCoffeeMushroom(SeedType seed);
    static bool IsSquashClusterZombie(std::uint16_t zombieType);
    static bool IsSquashHighValueZombie(std::uint16_t zombieType);
    static int LargestSquashTargetStackInRow(const VSGameState &state, int row);
    PlantDecisionContext BuildDecisionContext(const VSGameState &state) const;
    PlantDecisionResult TryEmergencyPolicy(const VSGameState &state, const PlantDecisionContext &context);
    const VSCardState *FindReadyCard(const VSGameState &state, SeedType seedType) const;
    std::optional<VSAction> TryBlover(const VSGameState &state, int preferredRow);
    std::optional<VSAction> TryEvadeJalapenoHead(const VSGameState &state);
    int EffectivePlantPlayCost(const VSGameState &state, const VSCardState &card) const;
    std::optional<VSAction> TryClearDaytimeSunshroomForPlanting(const VSGameState &state, SeedType replacementSeed, PlantPlacementRange range);
    std::optional<VSAction> TryPlantInRange(const VSGameState &state, SeedType seedType, PlantPlacementRange range);
    std::optional<VSAction> TryPlant(const VSGameState &state, SeedType seedType, int row, int firstColumn, int lastColumn);
    std::optional<VSAction> TryPlantExactRow(const VSGameState &state, SeedType seedType, int row, int firstColumn, int lastColumn);
    std::optional<VSAction> TryRemoveLadderedNut(const VSGameState &state);
    std::optional<VSAction> TryCounterPlant(const VSGameState &state, SeedType seedType, int row, int firstColumn);

    static int ZombieColumn(const VSZombieState &zombie);
    static int ZombieEffectiveHealth(const VSZombieState &zombie);
    static bool IsHypnoshroomTarget(const VSZombieState &zombie);
    static int PotatoMineArmingLead(const VSZombieState &zombie);
    AshTarget FindBestAshTarget(const VSGameState &state, SeedType seedType) const;
    static bool IsAshTargetWorthPlaying(const VSGameState &state, SeedType seedType, const AshTarget &target);
    std::optional<VSAction> TryAshCounter(const VSGameState &state, SeedType seedType, int protectedSun);
    std::optional<VSAction> TryBestAshCounter(const VSGameState &state, int protectedSun);
    std::optional<VSAction> TryPotatoMine(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySnowpeaBonkPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryStarfruitChomperPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryKernelCeleryPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryMagnetShroom(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryHypnoshroom(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryWakeSleepingDoomshroom(const VSGameState &state);
    std::optional<VSAction> TryUmbrellaDefense(const VSGameState &state, int protectedSun);
    std::optional<VSAction> TryStarfruitGarlicFormation(const VSGameState &state, int protectedSun);
    std::optional<VSAction> TryIcebergLettuce(const VSGameState &state, int row, int protectedSun, bool forceEmergencyControl = false);
    std::optional<VSAction> TryTorchwoodSupport(const VSGameState &state, int protectedSun);
    std::optional<VSAction> TryIncomePlant(const VSGameState &state, int row, int protectedSun);
    std::optional<VSAction> TrySunshroomFiller(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryGraveBuster(const VSGameState &state, int protectedSun);

    VSGridPosition FindSustainedOutputCell(const VSGameState &state, SeedType seed, int row) const;
    bool HasReadySustainedOutputCard(const VSGameState &state, int protectedSun) const;
    std::optional<VSAction> TryRecycleIncomeForOutput(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySustainedOutputPlant(const VSGameState &state, int row, SustainedOutputPolicy policy);
    std::optional<VSAction> TryTemplateSustainedOutput(const VSGameState &state, int preferredRow, int protectedSun, TemplateOutputPolicy policy);
    bool HasIncomeSeed(const VSGameState &state) const;
    bool HasSunshroomSeed(const VSGameState &state) const;
    static int EffectivePlantEconomyCount(const VSGameState &state);
    bool HasEconomyPressurePlan(const VSGameState &state) const;
    int PrimaryOutputCost(const VSGameState &state) const;
    int MainCarryIncomeTarget(const VSGameState &state) const;
    int EconomyPressureIncomeTarget(const VSGameState &state) const;
    std::optional<VSAction> TryBoomerangControlPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryBoomerangGarlicFormation(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryThreepeaterPuffFormation(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySnowpeaPuffMagnetPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryPeaPuffTempoOpening(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryPeaCeleryAshTempo(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySporePuffTempoPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryPeaCabbageTorchTempo(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySnowpeaBonkFormation(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryPeaDoomTempoPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryStarfruitCrossfireFormation(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryCactusSpikeweedCore(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryKernelCeleryFormation(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryRepeaterCeleryTempo(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryMelonMineTempo(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryRepeaterTempoPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySporeShellPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryFumeDoomPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryMelonScaredySupport(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryScaredyCoffeeTempo(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryScaredyMelonSupport(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryScaredyPuffDoomPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryStarfruitPuffPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryPeaPuffPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TrySporePuffPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryWakeableMushroomOutput(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryWakeSleepingMushroom(const VSGameState &state, int preferredRow);

    std::optional<VSAction> TryPumpkinShell(const VSGameState &state, int row, int protectedSun);
    bool ShouldDeployWallnut(const VSGameState &state, int row) const;
    bool ShouldYieldLaneToMower(const VSGameState &state, int row) const;
    std::optional<VSAction> TrySpikeweed(const VSGameState &state, int row, int protectedSun);
    std::optional<VSAction> TryCactusSpikeweedPressure(const VSGameState &state, int preferredRow, int protectedSun);
    std::optional<VSAction> TryImpactDistraction(const VSGameState &state, int row, int protectedSun);
    int AreaCounterReserve(const VSGameState &state) const;
    std::optional<VSAction> TryFallbackPlant(const VSGameState &state, const PlantLaneAssessment &danger, int buildRow);

public:
    void Reset() override {
        BuiltinVSAgent::Reset();
        mOpeningEconomyPlaced = false;
        mRecentAshTarget = {};
        mRecentAshTick = 0;
        mHasRecentAsh = false;
    }

    void OnActionResult(const VSAction &action, VSActionResult result) override {
        BuiltinVSAgent::OnActionResult(action, result);
        if (result == VSActionResult::Applied && action.side == VSSide::Plants && action.kind == VSActionKind::PlaySeed
            && (action.expectedSeedType == static_cast<std::uint16_t>(SeedType::SEED_SUNFLOWER) || action.expectedSeedType == static_cast<std::uint16_t>(SeedType::SEED_SUNSHROOM))) {
            mOpeningEconomyPlaced = true;
        }
        if (result == VSActionResult::Applied && action.side == VSSide::Plants && action.kind == VSActionKind::PlaySeed) {
            switch (static_cast<SeedType>(action.expectedSeedType)) {
                case SeedType::SEED_SQUASH:
                case SeedType::SEED_CHERRYBOMB:
                case SeedType::SEED_JALAPENO:
                case SeedType::SEED_CHILLY_PEPPER:
                case SeedType::SEED_DOOMSHROOM:
                    mRecentAshTarget = action.target;
                    mRecentAshTick = action.notBeforeTick;
                    mHasRecentAsh = true;
                    break;
                default:
                    break;
            }
        }
    }
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_PLANNING_H
