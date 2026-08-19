#include "ZombieAI.h"

#include "../VSActionAILanePolicy.h"

#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

bool ZombieAIPlanning::HasLobbedPlantInRow(const VSGameState &state, int row) {
    return std::any_of(state.plants.begin(), state.plants.end(), [row](const VSPlantState &plant) {
        if (IsDeadOrOutside(plant) || plant.position.row != row) {
            return false;
        }
        switch (static_cast<SeedType>(plant.seedType)) {
            case SeedType::SEED_CABBAGEPULT:
            case SeedType::SEED_KERNELPULT:
            case SeedType::SEED_MELONPULT:
            case SeedType::SEED_WINTERMELON:
            case SeedType::SEED_COBCANNON:
            case SeedType::SEED_SPORESHROOM:
                return true;
            default:
                return false;
        }
    });
}

const VSCardState *ZombieAIPlanning::FindReadyCard(const VSGameState &state, SeedType seedType) const {
    for (const VSCardState &card : state.seedBanks[1]) {
        if (IsSlotBlocked(card.slot) || card.seedType != static_cast<std::uint16_t>(seedType)) {
            continue;
        }
        if (seedType != SeedType::SEED_ZOMBIE_MOUND && IsReadyCard(card, state.zombieBrains)) {
            return &card;
        }
        if (seedType == SeedType::SEED_ZOMBIE_MOUND) {
            for (int row = 0; row < state.rows; ++row) {
                const VSGridPosition target = FindZombieMoundCell(state, row);
                if (target.col >= 0 && target.row >= 0 && IsCardReadyForZombieTarget(card, state, target)) {
                    return &card;
                }
            }
        }
    }
    return nullptr;
}

int ZombieAIPlanning::HeavyZombieReserve(const VSGameState &state) const {
    int reserve = std::numeric_limits<int>::max();
    for (const VSCardState &card : state.seedBanks[1]) {
        if (IsSlotBlocked(card.slot) || card.matchRestricted || !card.active || card.seedType == static_cast<std::uint16_t>(SeedType::SEED_NONE)) {
            continue;
        }
        if (IsHeavyZombieSeed(static_cast<SeedType>(card.seedType))) {
            reserve = std::min(reserve, std::max(0, card.cost));
        }
    }
    return reserve == std::numeric_limits<int>::max() ? 0 : reserve;
}

bool ZombieAIPlanning::HasReadyFrontlineProbe(const VSGameState &state) const {
    return std::any_of(state.seedBanks[1].begin(), state.seedBanks[1].end(), [&](const VSCardState &card) {
        return !IsSlotBlocked(card.slot) && card.active && !card.matchRestricted && IsZombieFrontlineProbeSeed(static_cast<SeedType>(card.seedType)) && IsReadyCard(card, state.zombieBrains);
    });
}

bool ZombieAIPlanning::IsEarlyHeavyCommitCard(const VSGameState &state, SeedType seed, const ZombieDecisionContext &context) const {
    if (!IsHeavyZombieSeed(seed)) {
        return false;
    }
    const ZombieTemplateProfile &profile = context.templateProfile;
    const bool replayPoleTemplate = profile.Has(ZombieTemplate::LadderPole);
    const bool replayFanPoleTemplate = profile.Has(ZombieTemplate::NewspaperFanPole);
    const bool replayFlagGigaTemplate = profile.Has(ZombieTemplate::FlagSquash);
    const bool replayArmoredNormalTemplate = profile.Has(ZombieTemplate::ArmoredNormalRush);
    const int livePlants = CountLivePlants(state);
    // These recordings have exceptional, but not arbitrary, early
    // conversions. They still need a real plant board and either a
    // live probe or multiple spread probes before the heavy card may
    // interrupt the normal grave-building cadence.
    if (seed == SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER && replayPoleTemplate && context.economyCount >= 2 && context.activePressureRows >= 1 && livePlants >= 3) {
        return true;
    }
    if (seed == SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER && replayFanPoleTemplate && context.economyCount >= 3 && context.tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows)
        && livePlants >= state.rows) {
        return true;
    }
    if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR && replayFlagGigaTemplate && context.economyCount >= 8 && context.tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows)
        && livePlants >= state.rows) {
        return true;
    }
    // The Normal/Trashcan/Dog replay banks behind protected graves until
    // roughly eight income sources, then turns its broad cheap pressure
    // into the first Giga Gargantuar. This is earlier than the generic
    // finisher threshold, but still needs two live routes and a real board.
    if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR && replayArmoredNormalTemplate && context.economyCount >= 8 && context.tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows)
        && livePlants >= state.rows) {
        return true;
    }
    if (!context.tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows) || livePlants < state.rows) {
        return false;
    }
    const int minimumEconomy = seed == SeedType::SEED_ZOMBIE_GARGANTUAR ? context.tempo.CommitEconomyFloor(state.rows) : context.tempo.CommitEconomyFloor(std::max(state.rows * 2, state.rows + 3));
    return context.economyCount >= minimumEconomy;
}

bool ZombieAIPlanning::HasReadyEarlyHeavyCommit(const VSGameState &state, const ZombieDecisionContext &context) const {
    return std::any_of(state.seedBanks[1].begin(), state.seedBanks[1].end(), [&](const VSCardState &card) {
        const SeedType seed = static_cast<SeedType>(card.seedType);
        return !IsSlotBlocked(card.slot) && card.active && !card.matchRestricted && IsReadyCard(card, state.zombieBrains) && IsEarlyHeavyCommitCard(state, seed, context);
    });
}

std::optional<VSAction> ZombieAIPlanning::TryTemplateSundayRelease(const VSGameState &state, const ZombieDecisionContext &context) {
    const ZombieTemplateProfile &profile = context.templateProfile;
    const bool normalNewsImpSundayTemplate = profile.Has(ZombieTemplate::NormalNewsImpSunday);
    const bool impPailSledSundayTemplate = profile.Has(ZombieTemplate::ImpSledSunday);
    const bool peaHeadSundayTemplate = profile.Has(ZombieTemplate::PeaHeadSunday);
    const int peaHeadCount = static_cast<int>(std::count_if(
        state.zombies.begin(), state.zombies.end(), [](const VSZombieState &zombie) { return !zombie.dead && zombie.zombieType == static_cast<std::uint16_t>(ZombieType::ZOMBIE_PEA_HEAD); }));
    const int maximumCounterExposure = impPailSledSundayTemplate || peaHeadSundayTemplate ? 145 : 150;
    const ZombieTempoPolicy &tempo = context.tempo;
    const bool releaseWindow =
        (normalNewsImpSundayTemplate
         && IsZombieTemplatePhaseAvailable(profile, tempo, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, context.actualEconomyCount, context.activePressureRows, state.rows, ZombieTemplatePhase::Finisher))
        || (impPailSledSundayTemplate
            && IsZombieTemplatePhaseAvailable(profile, tempo, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, context.actualEconomyCount, context.activePressureRows, state.rows, ZombieTemplatePhase::Finisher))
        || (peaHeadSundayTemplate
            && IsZombieTemplatePhaseAvailable(profile, tempo, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, context.actualEconomyCount, context.activePressureRows, state.rows, ZombieTemplatePhase::Finisher)
            && peaHeadCount >= 2);
    const VSCardState *sundayEdition = FindReadyCard(state, SeedType::SEED_ZOMBIE_SUNDAY_EDITION);
    if (!releaseWindow || sundayEdition == nullptr) {
        return std::nullopt;
    }

    int bestRow = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int row = 0; row < state.rows; ++row) {
        const VSGridPosition target = FindZombieCell(state, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, row);
        if (EvaluateZombieLanePolicy(state, row).allowsAttack && target.col >= 0 && target.row >= 0 && IsCardReadyForZombieTarget(*sundayEdition, state, target)
            && PlantAreaCounterExposure(state, row) < maximumCounterExposure) {
            const int score = ZombieLaneAttackScore(state, row) + PlantEconomyValueInRow(state, row) + SustainedOutputScoreInRow(state, row) - PlantAreaCounterExposure(state, row);
            if (bestRow < 0 || score > bestScore) {
                bestRow = row;
                bestScore = score;
            }
        }
    }
    if (bestRow < 0) {
        return std::nullopt;
    }
    return MakePlayAction(VSSide::Zombies, *sundayEdition, FindZombieCell(state, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, bestRow), state.boardTick);
}

std::unique_ptr<IVSAgent> CreateZombieAI() {
    return std::make_unique<ZombieAI>();
}

} // namespace vsai::detail
