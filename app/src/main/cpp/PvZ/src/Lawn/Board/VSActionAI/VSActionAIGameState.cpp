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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIGameState.h"

#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/LawnMower.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"

#include <cstddef>
#include <algorithm>
#include <iterator>

namespace vsai::detail {
namespace {

    constexpr std::size_t SideIndex(VSSide side) {
        return static_cast<std::size_t>(side);
    }

    bool IsMatchPlaying(const Board *board) {
        return board != nullptr && board->mApp != nullptr && board->mApp->mGameScene == GameScenes::SCENE_PLAYING;
    }

} // namespace

VSGameState BuildGameStateSnapshot(Board *board) {
    VSGameState state{};
    if (board == nullptr) {
        return state;
    }

    state.boardTick = static_cast<std::uint32_t>(board->mMainCounter);
    state.rows = board->StageHas6Rows() ? 6 : 5;
    state.plantSun = board->mSunMoney1;
    state.zombieBrains = board->mDeathMoney;
    state.liveZombieTargetCount = board->GetMPTargetCount();
    state.isNight = board->StageIsNight();
    state.isSuddenDeath = board->mChallenge != nullptr && board->mChallenge->IsMPSuddenDeath();
    state.resourceProductionDisabled = state.isSuddenDeath && Challenge::gVSSuddenDeathMode <= 1;
    state.playing = IsMatchPlaying(board);
    state.paused = board->mPaused || requestPause;

    for (int row = 0; row < state.rows; ++row) {
        for (int column = 0; column < 6; ++column) {
            state.basePlantableCells[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = board->CanPlantAt(column, row, SeedType::SEED_PEASHOOTER) == PlantingReason::PLANTING_OK;
        }
    }

    for (LawnMower *mower = nullptr; board->IterateLawnMowers(mower);) {
        if (mower == nullptr || mower->mDead || mower->mRow < 0 || mower->mRow >= static_cast<int>(state.mowerAvailable.size())) {
            continue;
        }
        const std::size_t row = static_cast<std::size_t>(mower->mRow);
        state.mowerAvailable[row] = mower->mMowerState == LawnMowerState::MOWER_READY;
        state.mowerInMotion[row] = mower->mMowerState == LawnMowerState::MOWER_TRIGGERED;
    }

    for (SeedBank *seedBank : board->mSeedBank) {
        if (seedBank == nullptr) {
            continue;
        }

        std::vector<VSCardState> &cards = state.seedBanks[seedBank->mIsZombie ? SideIndex(VSSide::Zombies) : SideIndex(VSSide::Plants)];
        const int packetCount = std::clamp(seedBank->mNumPackets, 0, static_cast<int>(std::size(seedBank->mSeedPackets)));
        cards.reserve(static_cast<std::size_t>(packetCount));
        for (int slot = 0; slot < packetCount; ++slot) {
            const SeedPacket &packet = seedBank->mSeedPackets[slot];
            const bool suddenDeathCardDisabled = state.isSuddenDeath
                && (board->mChallenge->ISMPSeedSuddenDeathDisabled(seedBank->mIsZombie ? 1 : 0, packet.mPacketType)
                    || (state.resourceProductionDisabled && Challenge::IsMPResourceProducer(packet.mPacketType)));
            cards.push_back({
                .slot = static_cast<std::uint8_t>(slot),
                .seedType = static_cast<std::uint16_t>(packet.mPacketType),
                .imitaterType = static_cast<std::uint16_t>(packet.mImitaterType),
                .cost = board->GetCurrentPlantCost(packet.mPacketType, SeedType::SEED_NONE),
                .refreshCounter = packet.mRefreshCounter,
                .refreshTime = packet.mRefreshTime,
                .active = packet.mActive,
                .refreshing = packet.mRefreshing,
                .matchRestricted = suddenDeathCardDisabled,
            });
        }
    }

    for (Plant *plant = nullptr; board->mPlants.IterateNext(plant);) {
        state.plants.push_back({
            .id = board->mPlants.DataArrayGetID(plant),
            .seedType = static_cast<std::uint16_t>(plant->mSeedType),
            .state = static_cast<std::uint16_t>(plant->mState),
            .position = {static_cast<std::int8_t>(plant->mPlantCol), static_cast<std::int8_t>(plant->mRow)},
            .health = plant->mPlantHealth,
            .maxHealth = plant->mPlantMaxHealth,
            .asleep = plant->mIsAsleep,
            .dead = plant->mDead,
        });
    }

    for (Zombie *zombie = nullptr; board->mZombies.IterateNext(zombie);) {
        Plant *jalapenoContactPlant = zombie->mZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD ? zombie->FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW) : nullptr;
        Plant *jalapenoPreContactPlant = nullptr;
        if (zombie->mZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD && !zombie->mMindControlled) {
            // The engine starts the burn at 20 pixels of overlap. Give the
            // AI an additional five-pixel reaction window, without changing
            // the engine collision rule.
            constexpr int kJalapenoHeadEngineOverlap = 20;
            constexpr int kJalapenoHeadExtraWarning = 5;
            constexpr int kJalapenoHeadAiWarningOverlap = kJalapenoHeadEngineOverlap - kJalapenoHeadExtraWarning;
            const Sexy::Rect attackRect = zombie->GetZombieAttackRect();
            for (Plant *plant = nullptr; board->mPlants.IterateNext(plant);) {
                if (plant->mDead || plant->mRow != zombie->mRow || !zombie->CanTargetPlant(plant, ZombieAttackType::ATTACKTYPE_CHEW)
                    || GetRectOverlap(attackRect, plant->GetPlantRect()) < kJalapenoHeadAiWarningOverlap) {
                    continue;
                }
                jalapenoPreContactPlant = plant;
                break;
            }
        }
        state.zombies.push_back({
            .id = board->mZombies.DataArrayGetID(zombie),
            .zombieType = static_cast<std::uint16_t>(zombie->mZombieType),
            .row = static_cast<std::int8_t>(zombie->mRow),
            .positionX = zombie->mPosX,
            .positionY = zombie->mPosY,
            .bodyHealth = zombie->mBodyHealth,
            .bodyMaxHealth = zombie->mBodyMaxHealth,
            .shieldHealth = zombie->mShieldHealth,
            .eating = zombie->mIsEating,
            .mindControlled = zombie->mMindControlled,
            .canBeFrozen = zombie->CanBeFrozen(),
            .jalapenoContactPlantId = jalapenoContactPlant == nullptr ? 0U : board->mPlants.DataArrayGetID(jalapenoContactPlant),
            .jalapenoPreContactPlantId = jalapenoPreContactPlant == nullptr ? 0U : board->mPlants.DataArrayGetID(jalapenoPreContactPlant),
            .explorerTorchLit = zombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && zombie->mHasObject,
            .bungeeAtTarget =
                zombie->mZombieType == ZombieType::ZOMBIE_BUNGEE && (zombie->mZombiePhase == ZombiePhase::PHASE_BUNGEE_AT_BOTTOM || zombie->mZombiePhase == ZombiePhase::PHASE_BUNGEE_GRABBING),
            .dead = zombie->mDead,
        });
    }

    for (GridItem *gridItem = nullptr; board->IterateGridItems(gridItem);) {
        state.gridItems.push_back({
            .id = board->mGridItems.DataArrayGetID(gridItem),
            .gridItemType = static_cast<std::uint16_t>(gridItem->mGridItemType),
            .position = {static_cast<std::int8_t>(gridItem->mGridX), static_cast<std::int8_t>(gridItem->mGridY)},
            .health = gridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE ? gridItem->mVSTargetZombieHealth : gridItem->mVSGraveStoneHealth,
            .level = gridItem->mMoundLevel,
            .dead = gridItem->mDead,
        });
    }

    for (Coin *coin = nullptr; board->mCoins.IterateNext(coin);) {
        const bool isPlantResource = coin->IsSun();
        const bool isZombieResource = coin->IsDeath();
        if (!isPlantResource && !isZombieResource) {
            continue;
        }
        state.resources.push_back({
            .id = board->mCoins.DataArrayGetID(coin),
            .side = isPlantResource ? VSSide::Plants : VSSide::Zombies,
            .coinType = static_cast<std::uint16_t>(coin->mType),
            .value = coin->GetSunValue(),
            .positionX = coin->mPosX,
            .positionY = coin->mPosY,
            .beingCollected = coin->mIsBeingCollected || coin->mCoinMotion == CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR,
            .dead = coin->mDead,
        });
    }

    return state;
}

} // namespace vsai::detail
