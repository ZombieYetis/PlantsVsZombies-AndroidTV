#include "ZombieAI.h"

#include "PvZ/Lawn/Board/Plant.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace vsai::detail {
int ZombieAIPlanning::CardScore(const VSCardState &card, const VSGameState &state, const ZombieDecisionContext &context, int targetRow, int effectiveCost) {
    const SeedType seed = static_cast<SeedType>(card.seedType);
    const ZombieTempoPolicy &tempo = context.tempo;
    const int economyCount = context.economyCount;
    const ZombieRowScoreFacts &rowFacts = context.rowScoreFacts[static_cast<std::size_t>(targetRow)];
    const bool hasPlants = context.hasPlants;
    const bool hasSnowPea = rowFacts.hasSnowPea;
    const bool hasBonkChoy = rowFacts.hasBonkChoy;
    const bool hasWallnut = rowFacts.hasWallnut;
    const bool hasPumpkinShell = rowFacts.hasPumpkinShell;
    const int plantCount = rowFacts.plantCount;
    const int zombieCount = rowFacts.zombieCount;
    const int graveProjectileThreat = rowFacts.graveProjectileThreat;
    const int lobbedProjectileThreat = rowFacts.lobbedProjectileThreat;
    const bool hasLobbedPlant = rowFacts.hasLobbedPlant;
    const int graveScreenDeficit = rowFacts.graveScreenDeficit;
    const bool hasGraveGuard = rowFacts.hasGraveGuard;
    const int economyTarget = context.economyTarget;
    const int heavyEconomyThreshold = context.heavyEconomyThreshold;
    const int sustainedOutput = rowFacts.sustainedOutput;
    const int economyValue = rowFacts.economyValue;
    const PlantLaneAssessment &targetLane = rowFacts.lane;
    const int areaCounterExposure = rowFacts.areaCounterExposure;
    const bool plantHasMagnet = context.plantHasMagnet;
    const bool plantHasPeaCarry = context.plantHasPeaCarry;
    const bool plantHasShortPult = context.plantHasShortPult;
    const bool plantHasLobbedCard = context.plantHasLobbedCard;
    const bool plantHasNutCard = context.plantHasNutCard;
    const bool plantHasHighValueCarryCard = context.plantHasHighValueCarryCard;
    const ZombieTemplateProfile &profile = context.templateProfile;
    const int peaHeadCount = context.peaHeadCount;
    const int templatePhaseBonus = ZombieTemplatePhaseBonus(profile, tempo, seed, context.actualEconomyCount, context.activePressureRows, zombieCount, state.rows);
    const ZombieTemplateTacticalState templateState{
        .economyCount = economyCount,
        .activePressureRows = context.activePressureRows,
        .attackCommitPressureRows = tempo.AttackCommitPressureRowTarget(2, state.rows),
        .zombiesInRow = zombieCount,
        .rows = state.rows,
        .peaHeadCount = peaHeadCount,
        .plantCount = plantCount,
        .economyValue = economyValue,
        .sustainedOutput = sustainedOutput,
        .areaCounterExposure = areaCounterExposure,
        .hasWallnut = hasWallnut,
        .graveUnderDirectFire = graveProjectileThreat >= 55,
        .plantHasNutCard = plantHasNutCard,
        .plantHasHighValueCarryCard = plantHasHighValueCarryCard,
    };
    const int templateTacticalBonus = ZombieTemplateTacticalBonus(profile, seed, templateState);

    int score = 20 + ZombieLaneAttackScore(state, targetRow);
    const int graveThreat = ProtectableGraveThreatScore(state, targetRow);
    // A grave is the zombie player's income source. Any available
    // pressure is deliberately biased toward a lane that is shooting it.
    score += graveThreat * 2;
    if (IsZombieFastAttackSeed(seed) && economyCount >= 1 && economyCount <= state.rows + 2) {
        // The quick-attack recordings use low-cost bodies to make the
        // plant player defend several income rows before heavy cards are
        // affordable. Reward an unoccupied economy lane, not a pileup.
        score += 145 + economyValue / 2;
        score += zombieCount == 0 ? 110 : -90;
    }
    switch (seed) {
        case SeedType::SEED_ZOMBIE_BOBSLED:
            // After the opening graves, the replay's first proactive pressure is Bobsled into a held lane.
            score += 95 + plantCount * 16 + sustainedOutput / 2 + economyValue / 3 + (hasSnowPea ? 190 : 0) + (hasBonkChoy ? 120 : 0);
            // A sled team is already an area-counter magnet. Keep its first
            // commitment on an empty route and use it to open a second lane,
            // instead of stacking four riders into one Cherry/Squash cell.
            score += zombieCount == 0 ? 300 : -420;
            break;
        case SeedType::SEED_ZOMBIE_WALLNUT_HEAD:
            score += 80 + plantCount * 12 + sustainedOutput / 3 + (hasSnowPea ? 115 : 0) + (hasWallnut ? 80 : 0);
            break;
        case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
            // Mound games protect the upgraded economic asset with a
            // durable head rather than treating Trashcan as the only
            // viable grave screen.
            score += graveProjectileThreat > 0 && !hasGraveGuard ? 440 + graveProjectileThreat * 2 : -95;
            score += graveThreat >= 100 ? 115 : 0;
            break;
        case SeedType::SEED_ZOMBIE_PAIL:
            score += 65 + plantCount * 14 + sustainedOutput / 2 + economyValue / 4 + (hasSnowPea ? 135 : 0) + (hasBonkChoy ? 100 : 0);
            score += plantHasPeaCarry ? 70 : 0;
            // Magnet is a card-level matchup signal even before the magnet
            // is planted. Prefer a non-metal guard when the opponent has
            // committed that answer package.
            score -= plantHasMagnet ? 190 : 0;
            // A Pea Head opening first establishes ranged pressure in empty
            // lanes. Keep the first pail for a real screen decision rather
            // than letting it displace that formation by raw durability.
            break;
        case SeedType::SEED_ZOMBONI:
            // The ice trail makes Zomboni a strong answer to protected,
            // developed lanes, matching the second replay's breakthrough.
            score += 115 + plantCount * 18 + sustainedOutput / 2 + economyValue / 3 + ((hasWallnut || plantHasNutCard) ? 135 : 0) + (hasSnowPea ? 90 : 0);
            break;
        case SeedType::SEED_ZOMBIE_TRASHCAN:
            // Trashcan is deliberately a slow front-line shield: a
            // single one in the lane blocks pea-family fire before it
            // reaches the graves behind it.
            score += graveProjectileThreat > 0 && !hasGraveGuard ? 425 + graveProjectileThreat * 2 : -250;
            score -= lobbedProjectileThreat * 2;
            score += graveScreenDeficit * 2;
            score += graveThreat >= 100 ? 90 : 0;
            score -= plantHasMagnet ? 240 : 0;
            score -= plantHasShortPult ? 80 : 0;
            // A pult carry attacks over this slow screen. Penalize the
            // card-level matchup too, before the first pult is planted.
            score -= plantHasLobbedCard ? 380 : 0;
            break;
        case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
            // Giga Polevaulter needs a substantial economy, but it may
            // still be the mid-game release card once two lanes are live.
            {
                const bool hasBreakthroughTarget = plantCount >= 3 || hasWallnut || hasPumpkinShell || sustainedOutput >= 80 || economyValue >= 120;
                const bool earlyHeavyCommit = economyCount >= tempo.HeavyCommitEconomyThreshold(state.rows, heavyEconomyThreshold)
                    && tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows) && context.livePlantCount >= state.rows && areaCounterExposure < 120;
                // This exact ladder/pole recording releases a Giga Pole
                // notably earlier than the generic finisher plan, but only
                // after two low-cost lanes are live and a nut line gives it
                // a meaningful jump target.
                score += (economyCount >= heavyEconomyThreshold || earlyHeavyCommit) ? ((hasBreakthroughTarget || earlyHeavyCommit) ? 250 : 15) : -240;
                score += earlyHeavyCommit ? 110 : 0;
                score += plantCount * 16 + sustainedOutput / 2 + economyValue / 3;
                score += (hasWallnut ? 145 : 0) + (hasPumpkinShell ? 105 : 0) + (hasSnowPea ? 70 : 0);
                score += targetLane.defense >= 120 ? 75 : 0;
                score -= areaCounterExposure / 3;
                score -= zombieCount >= 2 ? 145 : 0;
            }
            break;
        case SeedType::SEED_ZOMBIE_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
            // Heavy cards are release cards, not automatic reinforcements.
            // A human-like commit seeks a defended economic line to force
            // several answers, and avoids walking a giant into a formed
            // Ash cluster merely because friendly zombies are already there.
            {
                const bool hasBreakthroughTarget = plantCount >= 3 || hasWallnut || sustainedOutput >= 100 || economyValue >= 150;
                const bool hasBoardInvestment = context.livePlantCount >= state.rows;
                const int earlyEconomyFloor = seed == SeedType::SEED_ZOMBIE_GARGANTUAR ? state.rows : std::max(state.rows * 2, state.rows + 3);
                const bool earlyHeavyCommit =
                    economyCount >= earlyEconomyFloor && tempo.HasAttackCommitPressure(context.activePressureRows, 2, state.rows) && hasBoardInvestment && areaCounterExposure < 120;
                // Standard Gargantuars can convert a five-grave opening
                // into pressure; the more expensive variants wait for a
                // broader rear field. Both still need a real board state
                // to commit into instead of becoming an opening all-in.
                const bool hasMidGameHeavyEconomy = economyCount >= tempo.HeavyCommitEconomyThreshold(state.rows, heavyEconomyThreshold) && hasBreakthroughTarget;
                const bool canCommitHeavy = economyCount >= heavyEconomyThreshold || hasMidGameHeavyEconomy || earlyHeavyCommit;
                score += canCommitHeavy ? ((hasBreakthroughTarget || earlyHeavyCommit) ? 285 : 35) : -220;
                score += earlyHeavyCommit ? 125 : 0;
                score += plantCount * 18 + sustainedOutput / 2 + economyValue / 3;
                score += (hasWallnut ? 135 : 0) + (hasPumpkinShell ? 110 : 0) + (hasBonkChoy ? 100 : 0) + (hasSnowPea ? 75 : 0);
                score += targetLane.defense >= 150 ? 90 : 0;
                score -= areaCounterExposure / 2;
                score -= zombieCount >= 2 ? 125 : 0;
            }
            break;
        case SeedType::SEED_ZOMBIE_PEA_HEAD:
            // Pea Head is the persistent half of the ranged-siege replay.
            // Start it on a developed plant route, then let a separate
            // grave-guard decision preserve the rear economy. It should not
            // shadow a cheap probe merely because that probe reached a row
            // first.
            score += plantCount * 10 + sustainedOutput / 3 + economyValue / 4 + (hasSnowPea ? 120 : 0);
            score += zombieCount == 0 ? 85 : -110;
            break;
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
            score += plantCount * 10 + sustainedOutput / 3 + economyValue / 4 + (hasSnowPea ? 120 : 0);
            score -= hasLobbedPlant ? 900 : 0;
            score -= plantHasLobbedCard ? 380 : 0;
            score -= plantHasMagnet ? 165 : 0;
            break;
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
            score += 45 + plantCount * 10 + sustainedOutput / 4 + economyValue / 5;
            score += hasSnowPea ? 70 : 0;
            break;
        case SeedType::SEED_ZOMBIE_LADDER:
            // Ladders are only a worthwhile commitment against an
            // established nut line; otherwise a cheaper probe is better.
            score += hasWallnut ? 275 : (plantHasNutCard ? 220 : -65);
            score += plantCount * 8 + sustainedOutput / 3 + economyValue / 4;
            break;
        case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
            // Sunday Edition is a release card, but the fast-pressure
            // recordings do not wait for a full fifteen-grave bank. Once
            // several cheap routes are already taxing the plant player it
            // converts a four-grave opening into a real second-wave threat.
            // Without that multi-lane setup it still waits for the mature
            // economy so it is not an isolated expensive donation.
            { score += economyCount >= heavyEconomyThreshold ? 145 : -170; }
            score += plantCount * 14 + sustainedOutput / 2 + economyValue / 3;
            score += targetLane.defense >= 120 ? 70 : 0;
            score -= areaCounterExposure / 3;
            break;
        case SeedType::SEED_ZOMBIE_IMP:
        case SeedType::SEED_ZOMBIE_DIGGER:
            score += plantCount * 8 + sustainedOutput / 4 + economyValue / 2 + (hasWallnut ? 90 : 0);
            break;
        case SeedType::SEED_ZOMBIE_FOOTBALL:
        case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
            // Football is the mid-game runner behind a Pea Head firing
            // spread. Giga Football additionally tackles its first plant,
            // but its strategic deployment remains the same as Football.
            // Before that spread exists, keep the brains for graves and a
            // second ranged lane instead of donating an isolated rush.
            score += plantCount * 15 + sustainedOutput / 2 + economyValue / 3 + (hasSnowPea ? 95 : 0);
            score += zombieCount == 0 ? 80 : -170;
            score -= areaCounterExposure / 3;
            break;
        case SeedType::SEED_ZOMBIE_NORMAL:
        case SeedType::SEED_ZOMBIE_DOGWALKER:
        case SeedType::SEED_ZOMBIE_FLAG:
            // Normal/Dog/Flag recordings win tempo by touching several
            // Sunflower lanes while graves are still being built. Treat
            // these as a network of cheap probes, never as a second body
            // behind an already answered zombie.
            score += 120 + economyValue * 2 / 3 + plantCount * 7;
            score += zombieCount == 0 ? 145 : -220;
            score += CountZombiesInRow(state, targetRow) == 0 && EconomyPlantsInRow(state, targetRow) > 0 ? 85 : 0;
            break;
        case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
            // Cheap fast pressure should fan out through under-defended
            // sunflower lanes, not shadow an existing zombie stack.
            score += 105 + plantCount * 9 + sustainedOutput / 3 + economyValue / 2;
            score += zombieCount == 0 ? 110 : -90;
            break;
        case SeedType::SEED_ZOMBIE_SQUASH_HEAD:
            score += 95 + plantCount * 11 + sustainedOutput / 3 + economyValue / 3;
            score += zombieCount == 0 ? 90 : -75;
            break;
        case SeedType::SEED_ZOMBIE_BUNGEE:
            // Bungee needs a real high-value carry to steal. A deck-level
            // carry is enough to reserve the card, but never let it outrank
            // a useful frontline probe when the plant deck is only pads.
            score += hasPlants ? 220 : -80;
            score += plantHasHighValueCarryCard ? 260 : -180;
            score += (hasWallnut || hasBonkChoy) ? 85 : 0;
            break;
        case SeedType::SEED_ZOMBIE_GRAVESTONE:
            if (economyCount < economyTarget) {
                // Replay construction spans the full three zombie-side
                // columns. The fixed four-grave opening was too small.
                score += 450 + (economyTarget - economyCount) * 35;
            } else {
                score -= 180;
            }
            score += plantCount * 4;
            break;
        case SeedType::SEED_ZOMBIE_MOUND:
            // A mound is an upgrade to a grave economy, not a substitute
            // for the initial rear field.
            score += economyCount >= std::max(3, state.rows / 2) ? 220 : -220;
            score += graveThreat;
            break;
        case SeedType::SEED_ZOMBIE_DANCER:
            score += 75 + plantCount * 12 + (graveThreat > 0 ? 35 : 0);
            break;
        case SeedType::SEED_ZOMBIE_CATAPULT:
            // Unlike a Door or Newspaper, the Catapult can pressure a
            // pult line without presenting its screen to the lobbed
            // projectiles. Commit it to a developed firing lane, unless
            // FindTarget rejects that lane for a hypnotized zombie.
            score += 155 + plantCount * 14 + sustainedOutput / 2 + economyValue / 3;
            score += hasLobbedPlant ? 145 : 0;
            score += (hasSnowPea ? 90 : 0) + (hasPumpkinShell ? 55 : 0);
            score -= areaCounterExposure / 4;
            break;
        case SeedType::SEED_ZOMBIE_BALLOON:
            score += 65 + plantCount * 8 + (hasSnowPea ? 75 : 0);
            break;
        case SeedType::SEED_ZOMBIE_ZOMBLOB:
            score += plantCount * 12 + sustainedOutput / 2 + economyValue / 3;
            break;
        default:
            score += plantCount * 7 + sustainedOutput / 4 + economyValue / 4;
            break;
    }
    score += templatePhaseBonus;
    score += templateTacticalBonus;
    if (seed != SeedType::SEED_ZOMBIE_TRASHCAN && IsZombieGraveGuardSeed(seed)) {
        // The replay with Screen Door has no Trashcan.  A Door, Pail or
        // Wall-nut Head must still be allowed to screen direct fire from
        // the zombie-side economy instead of treating Trashcan as unique.
        score += graveProjectileThreat > 0 && !hasGraveGuard ? 260 + graveProjectileThreat : -35;
        score += lobbedProjectileThreat > 0 && !hasGraveGuard ? 180 + lobbedProjectileThreat : 0;
    }
    const bool isEconomyOrTargetedSeed = IsZombieEconomySeed(seed) || IsZombieTargetedSeed(seed);
    const bool isEmergencyGraveGuard = IsZombieGraveGuardSeed(seed) && graveProjectileThreat > 0 && !hasGraveGuard;
    if (zombieCount > 0 && !isEconomyOrTargetedSeed && !IsHeavyZombieSeed(seed) && !isEmergencyGraveGuard) {
        // A cheap/medium zombie is a probe, not a reason to feed the
        // same Ash target.  After one probe, opening another line with
        // Sunflowers is more valuable than reinforcing this line.
        score -= 340 + (zombieCount - 1) * 210;
        score -= areaCounterExposure;
        if (EconomyPlantsInRow(state, targetRow) == 0) {
            score -= 90;
        }
    }
    score += StrategyBonus(state, VSSide::Zombies, seed, targetRow);
    score -= effectiveCost / 50;
    return score;
}

} // namespace vsai::detail
