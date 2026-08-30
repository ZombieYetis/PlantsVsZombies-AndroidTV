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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_CARD_RULES_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_CARD_RULES_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIStrategy.h"

#include <cstdint>

namespace vsai::detail {

enum class ZombieTemplate : std::uint8_t {
    ZamboniPole,
    PeaHeadGiant,
    ImpSledSunday,
    ArmoredNormalRush,
    NewspaperDiggerGiga,
    NewspaperSledDiggerGiga,
    ConeImpFootballGiant,
    NormalNewsSled,
    NormalNewsImpSunday,
    LadderPole,
    NewspaperFanPole,
    PeaHeadSunday,
    PeaHeadZamboni,
    PeaHeadFlagBungee,
    MoundSkirmish,
    FlagSquash,
    FanImp,
    MoundTallnutSled,
    ImpLadderFootball,
    SledDogHeavy,
    DogSledPea,
    LadderBalloonZamboni,
    MoundBungeeFootball,
    NewspaperImpFootballGiant,
    PeaHeadZomblobGiant,
    ImpPailSledFootball,
    NewspaperScreenFootball,
    DogPeaFootball,
    NewspaperFootballPole,
    DancerRaid,
    PeaHeadRaid,
    PeaHeadDancerRaid,
    MoundPeaZomblobFootball,
    SundayLadderRaid,
    MoundNewspaperZamboni,
    MoundTallnutGuard,
    ConeSundayTallnut,
    NormalNewsImpGiga,
    NewspaperLadderZamboniJack,
};

enum class ZombieTemplatePhase : std::uint8_t {
    Opening,
    Conversion,
    Finisher,
};

struct ZombieTemplateProfile {
    std::uint64_t templates = 0;
    bool fastPressure = false;
    bool rangedSiege = false;
    bool sundayPressure = false;

    bool Has(ZombieTemplate value) const;
};

struct ZombieTemplateTacticalState {
    int economyCount = 0;
    int activePressureRows = 0;
    int attackCommitPressureRows = 0;
    int zombiesInRow = 0;
    int rows = 0;
    int peaHeadCount = 0;
    int plantCount = 0;
    int economyValue = 0;
    int sustainedOutput = 0;
    int areaCounterExposure = 0;
    bool hasWallnut = false;
    bool graveUnderDirectFire = false;
    bool plantHasNutCard = false;
    bool plantHasHighValueCarryCard = false;
};

class ZombieTempoPolicy;

ZombieTemplateProfile DetectZombieTemplateProfile(const VSGameState &state);
bool IsZombieTemplatePhaseSeed(const ZombieTemplateProfile &profile, SeedType seed, ZombieTemplatePhase phase);
bool IsZombieTemplatePhaseAvailable(
    const ZombieTemplateProfile &profile, const ZombieTempoPolicy &tempo, SeedType seed, int actualEconomyCount, int activePressureRows, int rows, ZombieTemplatePhase phase);
bool HasReadyZombieTemplateCommit(const VSGameState &state, const ZombieTemplateProfile &profile, const ZombieTempoPolicy &tempo, int actualEconomyCount, int activePressureRows);
int ZombieTemplatePhaseBonus(const ZombieTemplateProfile &profile, const ZombieTempoPolicy &tempo, SeedType seed, int actualEconomyCount, int activePressureRows, int zombiesInRow, int rows);
int ZombieTemplateTacticalBonus(const ZombieTemplateProfile &profile, SeedType seed, const ZombieTemplateTacticalState &state);

class ZombieTempoPolicy {
public:
    explicit ZombieTempoPolicy(bool enhanced)
        : mEnhanced(enhanced) {}

    bool IsEnhanced() const;
    int EffectiveEconomyCount(int actualCount) const;
    int EconomyTarget(int baseline, int rows, int activePressureRows) const;
    int OpeningEconomyFloor(int baseline) const;
    int OpeningEconomyCeiling(int baseline) const;
    int OpeningPressureRowTarget(int baseline, int rows) const;
    bool ShouldExtendPressure(int economyCount, int activePressureRows, int rows) const;
    int CommitPressureRowTarget(int baseline, int rows) const;
    int AttackCommitPressureRowTarget(int baseline, int rows) const;
    bool HasCommitPressure(int activePressureRows, int baseline, int rows) const;
    bool HasAttackCommitPressure(int activePressureRows, int baseline, int rows) const;
    int CommitEconomyFloor(int baseline) const;
    int HeavyBankEconomyThreshold(int rows, int heavyEconomyThreshold) const;
    int HeavyCommitEconomyThreshold(int rows, int heavyEconomyThreshold) const;
    int EconomyRepairDeficitThreshold() const;
    int PressureRepairDeficitTolerance() const;
    std::uint8_t LaneAttackCooldown(SeedType seed) const;

private:
    bool mEnhanced;
};

ZombieTempoPolicy GetZombieTempoPolicy();

bool IsZombieTargetedSeed(SeedType seed);
bool IsZombieEconomySeed(SeedType seed);
bool IsZombieFrontlineProbeSeed(SeedType seed);
bool IsZombieFastAttackSeed(SeedType seed);
bool IsZombieMetalGraveGuard(SeedType seed);
bool IsZombieLobbedScreenDonation(SeedType seed);
int ZombieGraveGuardPriority(SeedType seed);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_CARD_RULES_H
