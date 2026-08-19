#include "ZombieAI.h"

#include "../VSActionAILanePolicy.h"

#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {

void ZombieAIPlanning::Reset() {
    BuiltinVSAgent::Reset();
    mLastAttackRow = -1;
    mLastPressureEconomyCount = -1;
    mLaneAttackCooldown.fill(0);
    mOpeningEconomyPlaced = false;
}

void ZombieAIPlanning::OnActionResult(const VSAction &action, VSActionResult result) {
    BuiltinVSAgent::OnActionResult(action, result);
    if (result == VSActionResult::Applied && action.side == VSSide::Zombies && action.kind == VSActionKind::PlaySeed
        && action.expectedSeedType == static_cast<std::uint16_t>(SeedType::SEED_ZOMBIE_GRAVESTONE)) {
        mOpeningEconomyPlaced = true;
    }
}

std::optional<VSAction> ZombieAI::Decide(const VSGameState &state) {
    AdvanceBlockedSlots();
    // Priority is fixed for one board snapshot: opening economy, resources,
    // target and grave safety, template commitments, economy recovery, then
    // per-card lane scoring. Earlier safety decisions intentionally preempt
    // pressure so a local match cannot lose its final target to a score tie.
    // Keep the first zombie action independent from the current grave count:
    // target markers and replay state must not let a template open with a
    // body before its first Gravestone is actually on the board.
    if (!state.isSuddenDeath && !mOpeningEconomyPlaced) {
        if (std::optional<VSAction> action = ZombieAIPlanning::TryBuildEconomy(state, ZombieAIPlanning::LeastCommittedZombieRow(state))) {
            return action;
        }
        return std::nullopt;
    }
    for (std::uint8_t &cooldown : mLaneAttackCooldown) {
        if (cooldown > 0) {
            --cooldown;
        }
    }
    for (const VSResourceState &resource : state.resources) {
        if (resource.side == VSSide::Zombies && !resource.dead && !resource.beingCollected) {
            return MakeCollectResourceAction(VSSide::Zombies, resource.id);
        }
    }

    const int actualEconomyCount = CountZombieEconomy(state);
    if (mLastPressureEconomyCount > actualEconomyCount) {
        // A destroyed grave re-opens the pressure cadence. Rebuilding
        // from a smaller base should not force a full 15-grave rebuild
        // before the next low-cost probe is allowed.
        mLastPressureEconomyCount = actualEconomyCount - 1;
    }
    const ZombieDecisionContext context = BuildDecisionContext(state);
    const ZombieTempoPolicy &tempo = context.tempo;
    const int economyDeficit = context.economyDeficit;
    const bool targetDefenseEmergency = context.targetDefenseEmergency;
    const int graveDefenseRow = context.graveDefenseRow;
    const int graveStraightThreat = context.graveStraightThreat;
    const int graveLobbedThreat = context.graveLobbedThreat;
    const bool graveDefenseUrgent = context.graveDefenseUrgent;
    const bool graveDefenseReinforcement = context.graveDefenseReinforcement;
    if (targetDefenseEmergency) {
        if (std::optional<VSAction> action = ZombieAIPlanning::TryProtectEconomy(state, graveDefenseRow, true)) {
            return action;
        }
    }
    if (graveLobbedThreat >= 70 && graveStraightThreat < 70) {
        if (std::optional<VSAction> action = ZombieAIPlanning::TryCounterLobbedGravePressure(state, context, graveDefenseRow)) {
            return action;
        }
    }
    if (graveDefenseReinforcement) {
        if (std::optional<VSAction> action = ZombieAIPlanning::TryProtectEconomy(state, graveDefenseRow, targetDefenseEmergency)) {
            return action;
        }
    }
    const int activePressureRows = context.activePressureRows;
    const int heavyZombieReserve = context.heavyZombieReserve;
    const bool bankForHeavy = context.bankForHeavy;
    const bool hasReadyFrontlineProbe = context.hasReadyFrontlineProbe;
    const bool hasReadyEarlyHeavyCommit = context.hasReadyEarlyHeavyCommit;
    const int desiredOpeningRows = context.desiredOpeningRows;
    const int criticalTargetRow = context.criticalTargetRow;
    if (std::optional<VSAction> action = ZombieAIPlanning::TryTemplateSundayRelease(state, context)) {
        return action;
    }
    const bool canConvertMowerlessTargetRoute = context.canConvertMowerlessTargetRoute;
    const bool forceOpeningPressure = context.forceOpeningPressure;
    const bool preservePressureDuringRepair = context.preservePressureDuringRepair;
    const int survivingFrontRow = context.survivingFrontRow;
    const bool preserveSurvivingFront = context.preserveSurvivingFront;
    const bool survivingFrontGuarded = context.survivingFrontGuarded;
    const int economicRow = context.economicRow;
    const bool restorationCanProceed = context.restorationCanProceed;
    const bool restorationOutweighsFront = context.restorationOutweighsFront;
    const bool economyRepairIsUrgent = context.economyRepairIsUrgent;
    const bool hasReadyTemplateCommit = context.hasReadyTemplateCommit;
    if (economyDeficit > 0 && restorationCanProceed && !forceOpeningPressure && (!canConvertMowerlessTargetRoute || !hasReadyFrontlineProbe) && !hasReadyEarlyHeavyCommit && !hasReadyTemplateCommit
        && (!bankForHeavy || (tempo.IsEnhanced() && economyDeficit >= 2)) && (economyRepairIsUrgent || !preservePressureDuringRepair) && (!preserveSurvivingFront || restorationOutweighsFront)) {
        if (std::optional<VSAction> action = ZombieAIPlanning::TryBuildEconomy(state, economicRow)) {
            return action;
        }
    }

    const bool saveForHeavy = bankForHeavy && state.zombieBrains < heavyZombieReserve;

    const VSCardState *bestCard = nullptr;
    int targetRow = graveDefenseUrgent ? graveDefenseRow : MostVulnerablePlantRow(state);
    int bestScore = std::numeric_limits<int>::min();
    const bool plantHasReadyAsh = ReadyPlantAreaCounterCount(state) > 0;
    int unpressuredEconomyRows = 0;
    for (int row = 0; row < state.rows; ++row) {
        const ZombieLanePolicy lane = EvaluateZombieLanePolicy(state, row);
        if (lane.allowsAttack && CountZombiesInRow(state, row) == 0 && EconomyPlantsInRow(state, row) > 0) {
            ++unpressuredEconomyRows;
        }
    }
    for (const VSCardState &card : state.seedBanks[1]) {
        if (IsSlotBlocked(card.slot) || !card.active || card.refreshing || card.refreshCounter > 0) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        // Sudden death removes zombie-side economy actions.  Filter them
        // before scoring so an otherwise attractive grave cannot stall
        // the agent on a target the mode rejects.
        if (state.isSuddenDeath && IsZombieEconomySeed(seed)) {
            continue;
        }
        if (graveDefenseUrgent && card.seedType == static_cast<std::uint16_t>(SeedType::SEED_ZOMBIE_GRAVESTONE)) {
            continue;
        }
        for (int row = 0; row < state.rows; ++row) {
            const ZombieLanePolicy lane = EvaluateZombieLanePolicy(state, row);
            const std::optional<VSGridPosition> target = FindTargetForCard(state, card, row);
            if (!target.has_value() || !IsCardReadyForZombieTarget(card, state, *target)) {
                continue;
            }
            // The mower sweep makes every zombie-side action in this row
            // disposable. Do not spend a body, Bungee, grave, or screen
            // while it is moving, nor after an invader reaches column 0.
            if (lane.deploymentBlocked) {
                continue;
            }
            const int effectiveCost = static_cast<SeedType>(card.seedType) == SeedType::SEED_ZOMBIE_MOUND ? MoundUpgradeCostAt(state, *target) : card.cost;
            const bool isEconomyAction = IsZombieEconomySeed(seed);
            const bool isTargetedAction = IsZombieTargetedSeed(seed);
            const bool isProtectedGuard = IsZombieGraveGuardSeed(seed) && graveDefenseUrgent;
            const int zombiesInRow = CountZombiesInRow(state, row);
            const bool pursueBrokenMowerRow = lane.conversionRoute;
            // A destroyed zombie target cannot be recovered by spending more
            // bodies in its row. The normal marker-less VS boards retain all
            // rows through HasLiveZombieTargetInRow's compatibility path.
            if (!isEconomyAction && !lane.hasLiveTarget) {
                continue;
            }
            if (lane.strongMowerlessPlantLane && !lane.conversionRoute && !isEconomyAction && !isTargetedAction && !isProtectedGuard) {
                continue;
            }

            const bool isLaneAttack = !isEconomyAction && !isTargetedAction && !isProtectedGuard;
            // When the plant's broad answer is ready, a third ordinary body
            // in one row is precisely the stack it is waiting to erase.
            // Spread to a live, unpressured economy row first; once those
            // routes are all occupied, the existing score penalties still
            // permit a deliberate late-game commitment.
            if (plantHasReadyAsh && isLaneAttack && !IsHeavyZombieSeed(seed) && zombiesInRow >= 2 && unpressuredEconomyRows > 0 && !pursueBrokenMowerRow) {
                continue;
            }

            int score = ZombieAIPlanning::CardScore(card, state, context, row, effectiveCost);
            if (seed == SeedType::SEED_ZOMBIE_MOUND) {
                // The target already passed the per-mound affordability
                // check. Add its marginal income return so level 0/2
                // upgrades beat an expensive level 1/3 tunnel vision.
                score += MoundUpgradePriorityAt(state, *target);
            }
            const bool isEarlyHeavyCandidate = ZombieAIPlanning::IsEarlyHeavyCommitCard(state, seed, context);
            if ((forceOpeningPressure && !IsZombieFrontlineProbeSeed(seed) && !isEarlyHeavyCandidate)
                || (hasReadyEarlyHeavyCommit && !forceOpeningPressure && !isEarlyHeavyCandidate && isEconomyAction) || (preservePressureDuringRepair && !forceOpeningPressure && isEconomyAction)) {
                continue;
            }
            if (isLaneAttack && !IsHeavyZombieSeed(seed)) {
                // A row remains on cooldown for a few decisions after a
                // probe. This prevents alternating two lanes forever
                // while another Sunflower route remains untouched. A spent
                // mower is different: that live target lane is a conversion
                // route, so only a small cooldown applies.
                score -= static_cast<int>(mLaneAttackCooldown[static_cast<std::size_t>(row)]) * (pursueBrokenMowerRow ? 35 : 155);
            }
            if (plantHasReadyAsh && zombiesInRow > 0 && !isEconomyAction && !isTargetedAction && !isProtectedGuard) {
                // One ready Cherry/Squash/Jalapeno/Doomshroom is a reason
                // to fan out, not to build a one-row pile. Heavy cards can
                // still be a deliberate finisher, but are penalized much
                // harder once two bodies already share the blast cell.
                score -= IsHeavyZombieSeed(seed) ? (zombiesInRow >= 2 ? 720 : 260) : (zombiesInRow >= 2 ? 900 : 520);
            }
            if (!isEconomyAction && !isTargetedAction && !isProtectedGuard && !IsHeavyZombieSeed(seed) && activePressureRows < desiredOpeningRows) {
                // The new recordings use cheap cones, imps and normal
                // zombies to establish several live probes before any
                // lane receives a second body. This also denies one Ash
                // counter an entire zombie-side wave.
                score += zombiesInRow == 0 ? 210 : -280;
            }
            if (graveDefenseUrgent && row == graveDefenseRow) {
                score += isProtectedGuard ? 410 : 120;
            }
            if (preserveSurvivingFront && row == survivingFrontRow) {
                const SeedType seed = static_cast<SeedType>(card.seedType);
                if (IsZombieGraveGuardSeed(seed) && !survivingFrontGuarded) {
                    // After two attack lanes have been cleared, keep the
                    // remaining valuable front alive before restarting
                    // economic expansion on an empty route.
                    score += criticalTargetRow >= 0 ? 560 : 320;
                } else if (!IsHeavyZombieSeed(seed) && !IsZombieEconomySeed(seed)) {
                    score += criticalTargetRow >= 0 ? 110 : 75;
                }
            }
            if (saveForHeavy && !IsHeavyZombieSeed(static_cast<SeedType>(card.seedType))) {
                // Continue inexpensive probes and grave guards, but do
                // not repeatedly spend the giant timing on medium cards.
                // This keeps 100/200-brain finishers reachable without
                // leaving every grave route unprotected.
                if (!isProtectedGuard) {
                    const int lowCostPenalty = tempo.IsEnhanced() ? -120 : -45;
                    const int mediumCostPenalty = tempo.IsEnhanced() ? -330 : -190;
                    score += card.cost <= std::max(50, heavyZombieReserve / 4) ? lowCostPenalty : mediumCostPenalty;
                }
            }
            if (isLaneAttack && !IsHeavyZombieSeed(seed) && zombiesInRow > 0 && unpressuredEconomyRows > 0 && !pursueBrokenMowerRow) {
                score -= (zombiesInRow == 1 ? 250 : 460) * std::min(2, unpressuredEconomyRows);
            }
            if (!graveDefenseUrgent && !preserveSurvivingFront && row == mLastAttackRow && !pursueBrokenMowerRow) {
                // Do not keep feeding the same lane while another lane can
                // accept a zombie. This penalty is intentionally skipped
                // during urgent grave defense.
                score -= tempo.HasAttackCommitPressure(activePressureRows, 2, state.rows) ? 210 : 125;
            }
            if (pursueBrokenMowerRow && !IsZombieEconomySeed(seed) && !IsZombieTargetedSeed(seed)) {
                // A cleared mower lane is a live conversion route. Keep
                // pressure there while the independent grave-defense path
                // continues to protect the zombie economy. This must also
                // overcome the ordinary multi-lane spreading bias: that
                // bias is correct before a mower falls, but not when the
                // next successful push wins through this still-live target.
                score += MowerlessLaneCommitmentBonus(state, lane, row, zombiesInRow);
            }
            if (bestCard == nullptr || score > bestScore) {
                bestCard = &card;
                targetRow = row;
                bestScore = score;
            }
        }
    }
    if (bestCard == nullptr) {
        return std::nullopt;
    }

    const std::optional<VSGridPosition> target = FindTargetForCard(state, *bestCard, targetRow);
    if (!target.has_value()) {
        return std::nullopt;
    }
    const SeedType chosenSeed = static_cast<SeedType>(bestCard->seedType);
    if (IsZombieFrontlineProbeSeed(chosenSeed)) {
        mLastPressureEconomyCount = std::max(mLastPressureEconomyCount, actualEconomyCount);
    }
    if (!IsZombieEconomySeed(chosenSeed) && !IsZombieTargetedSeed(chosenSeed)) {
        mLastAttackRow = targetRow;
        if (targetRow >= 0 && targetRow < static_cast<int>(mLaneAttackCooldown.size()) && !(IsZombieGraveGuardSeed(chosenSeed) && graveDefenseUrgent)) {
            mLaneAttackCooldown[static_cast<std::size_t>(targetRow)] = tempo.LaneAttackCooldown(chosenSeed);
        }
    }
    return MakePlayAction(VSSide::Zombies, *bestCard, *target, state.boardTick);
}

} // namespace vsai::detail
