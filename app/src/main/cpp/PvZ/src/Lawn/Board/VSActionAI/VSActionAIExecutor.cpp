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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIExecutor.h"

#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"

#include <algorithm>
#include <iterator>

namespace vsai::detail {
namespace {

    constexpr std::size_t kSideCount = 2;

    bool IsValidSide(VSSide side) {
        return static_cast<std::size_t>(side) < kSideCount;
    }

    bool IsTickBefore(std::uint32_t tick, std::uint32_t deadline) {
        return static_cast<std::int32_t>(tick - deadline) < 0;
    }

    bool IsActionExpired(const VSAction &action, std::uint32_t tick) {
        return action.expiresAtTick != 0 && IsTickBefore(action.expiresAtTick, tick);
    }

    bool IsActionDeferred(const VSAction &action, std::uint32_t tick) {
        return action.notBeforeTick != 0 && IsTickBefore(tick, action.notBeforeTick);
    }

    GamepadControls *FindControlsForSide(Board *board, VSSide side) {
        if (board == nullptr) {
            return nullptr;
        }

        const bool wantsZombieControls = side == VSSide::Zombies;
        for (GamepadControls *controls : board->mGamepadControls) {
            if (controls != nullptr && controls->mIsZombie == wantsZombieControls) {
                return controls;
            }
        }
        return nullptr;
    }

    SeedBank *FindSeedBankForSide(Board *board, VSSide side) {
        if (board == nullptr) {
            return nullptr;
        }

        const bool wantsZombieBank = side == VSSide::Zombies;
        for (SeedBank *seedBank : board->mSeedBank) {
            if (seedBank != nullptr && seedBank->mIsZombie == wantsZombieBank) {
                return seedBank;
            }
        }
        return nullptr;
    }

    bool IsValidGridTarget(const Board *board, VSGridPosition target) {
        if (board == nullptr) {
            return false;
        }

        const int rowCount = board->StageHas6Rows() ? 6 : 5;
        return target.col >= 0 && target.col < 9 && target.row >= 0 && target.row < rowCount;
    }

    void SetCursorForSeed(Board *board, GamepadControls *controls, const SeedPacket &packet, std::uint8_t seedSlot, VSGridPosition target) {
        const int gridX = static_cast<int>(target.col);
        const int gridY = static_cast<int>(target.row);
        controls->mCursorPositionX = static_cast<float>(board->GridToPixelX(gridX, gridY) + board->GridCellWidth(gridX, gridY) / 2);
        controls->mCursorPositionY = static_cast<float>(board->GridToPixelY(gridX, gridY) + board->GridCellHeight(gridX, gridY) / 2);
        controls->mGridCenterPositionX = controls->mCursorPositionX;
        controls->mGridCenterPositionY = controls->mCursorPositionY;
        controls->mSelectedSeedIndex = static_cast<int>(seedSlot);
        controls->mSelectedSeedType = packet.mPacketType == SeedType::SEED_IMITATER ? packet.mImitaterType : packet.mPacketType;
        controls->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;

        CursorObject *cursor = board->mCursorObject[controls->mPlayerIndex];
        if (cursor != nullptr) {
            cursor->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
            cursor->mSelectedIndex = static_cast<int>(seedSlot);
            cursor->mType = packet.mPacketType;
            cursor->mImitaterType = packet.mImitaterType;
        }
    }

    VSActionResult ExecutePlaySeed(Board *board, const VSAction &action) {
        if (!IsValidGridTarget(board, action.target)) {
            return VSActionResult::RejectedInvalidTarget;
        }

        GamepadControls *controls = FindControlsForSide(board, action.side);
        SeedBank *seedBank = FindSeedBankForSide(board, action.side);
        if (controls == nullptr || seedBank == nullptr || controls->GetSeedBank() != seedBank) {
            return VSActionResult::RejectedUnsupported;
        }

        const int packetCount = std::clamp(seedBank->mNumPackets, 0, static_cast<int>(std::size(seedBank->mSeedPackets)));
        if (action.seedSlot >= static_cast<std::uint8_t>(packetCount)) {
            return VSActionResult::RejectedInvalidCard;
        }

        SeedPacket &packet = seedBank->mSeedPackets[action.seedSlot];
        if (action.expectedSeedType != kAnySeedType && action.expectedSeedType != static_cast<std::uint16_t>(packet.mPacketType)) {
            return VSActionResult::RejectedInvalidCard;
        }
        if (!packet.CanPickUp()) {
            return VSActionResult::RejectedCardUnavailable;
        }

        const int gridX = static_cast<int>(action.target.col);
        const int gridY = static_cast<int>(action.target.row);
        const SeedType selectedSeed = packet.mPacketType == SeedType::SEED_IMITATER ? packet.mImitaterType : packet.mPacketType;
        if (selectedSeed == SeedType::SEED_UMBRELLA && board->FindUmbrellaPlant(gridX, gridY) != nullptr) {
            return VSActionResult::RejectedInvalidTarget;
        }
        if (packet.mPacketType == SeedType::SEED_ZOMBIE_MOUND) {
            SetCursorForSeed(board, controls, packet, action.seedSlot, action.target);
        }
        const int cost = board->GetCurrentPlantCost(packet.mPacketType, SeedType::SEED_NONE);
        if (action.side == VSSide::Plants) {
            if (!board->CanTakeSunMoney(cost, 0)) {
                return VSActionResult::RejectedInsufficientResource;
            }
        } else if (!board->CanTakeDeathMoney(cost)) {
            return VSActionResult::RejectedInsufficientResource;
        }
        if (board->HasLevelAwardDropped() || board->CanPlantAt(gridX, gridY, packet.mPacketType) != PlantingReason::PLANTING_OK) {
            return VSActionResult::RejectedInvalidTarget;
        }

        const int resourceBefore = action.side == VSSide::Plants ? board->mSunMoney1 : board->mDeathMoney;
        SetCursorForSeed(board, controls, packet, action.seedSlot, action.target);
        controls->OnButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, controls->mPlayerIndex, 0);

        const int resourceAfter = action.side == VSSide::Plants ? board->mSunMoney1 : board->mDeathMoney;
        return resourceBefore != resourceAfter || !packet.CanPickUp() ? VSActionResult::Applied : VSActionResult::RejectedInvalidTarget;
    }

    VSActionResult ExecuteShovel(Board *board, const VSAction &action) {
        if (action.side != VSSide::Plants) {
            return VSActionResult::RejectedUnsupported;
        }
        if (!IsValidGridTarget(board, action.target)) {
            return VSActionResult::RejectedInvalidTarget;
        }

        const int gridX = static_cast<int>(action.target.col);
        const int gridY = static_cast<int>(action.target.row);
        const int pixelX = board->GridToPixelX(gridX, gridY) + board->GridCellWidth(gridX, gridY) / 2;
        const int pixelY = board->GridToPixelY(gridX, gridY) + board->GridCellHeight(gridX, gridY) / 2;
        Plant *plant = board->ToolHitTest(pixelX, pixelY);
        if (plant == nullptr || plant->mDead) {
            return VSActionResult::RejectedInvalidTarget;
        }

        const SeedType seedType = plant->mSeedType;
        const int plantCol = plant->mPlantCol;
        const int plantRow = plant->mRow;
        board->mApp->PlayFoley(FoleyType::FOLEY_USE_SHOVEL);
        plant->Die();
        if (seedType == SeedType::SEED_CATTAIL && board->GetTopPlantAt(plantCol, plantRow, PlantPriority::TOPPLANT_ONLY_PUMPKIN) != nullptr) {
            board->NewPlant(plantCol, plantRow, SeedType::SEED_LILYPAD, SeedType::SEED_NONE, -1);
        }
        return VSActionResult::Applied;
    }

    VSActionResult ExecuteFireCobCannon(Board *board, const VSAction &action) {
        if (action.side != VSSide::Plants) {
            return VSActionResult::RejectedUnsupported;
        }
        if (action.objectId == 0 || !IsValidGridTarget(board, action.target)) {
            return VSActionResult::RejectedInvalidTarget;
        }

        Plant *plant = board->mPlants.DataArrayTryToGet(action.objectId);
        if (plant == nullptr || plant->mDead || plant->mSeedType != SeedType::SEED_COBCANNON || plant->mState != PlantState::STATE_COBCANNON_READY) {
            return VSActionResult::RejectedUnsupported;
        }

        const int gridX = static_cast<int>(action.target.col);
        const int gridY = static_cast<int>(action.target.row);
        const int pixelX = board->GridToPixelX(gridX, gridY) + board->GridCellWidth(gridX, gridY) / 2;
        const int pixelY = board->GridToPixelY(gridX, gridY) + board->GridCellHeight(gridX, gridY) / 2;
        plant->CobCannonFire(pixelX, pixelY);
        return plant->mState == PlantState::STATE_COBCANNON_FIRING ? VSActionResult::Applied : VSActionResult::RejectedUnsupported;
    }

    VSActionResult ExecuteCollectResource(Board *board, const VSAction &action) {
        if (action.objectId == 0) {
            return VSActionResult::RejectedInvalidTarget;
        }

        Coin *coin = board->mCoins.DataArrayTryToGet(action.objectId);
        if (coin == nullptr || coin->mDead || coin->mIsBeingCollected || coin->mCoinMotion == CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR) {
            return VSActionResult::RejectedInvalidTarget;
        }
        const bool isPlantResource = coin->IsSun();
        const bool isZombieResource = coin->IsDeath();
        if ((action.side == VSSide::Plants && !isPlantResource) || (action.side == VSSide::Zombies && !isZombieResource)) {
            return VSActionResult::RejectedUnsupported;
        }

        GamepadControls *controls = FindControlsForSide(board, action.side);
        if (controls == nullptr) {
            return VSActionResult::RejectedUnsupported;
        }
        coin->GamepadCursorOver(controls->mPlayerIndex);
        return coin->mIsBeingCollected || coin->mCoinMotion == CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR ? VSActionResult::Applied : VSActionResult::RejectedInvalidTarget;
    }

    VSActionResult ExecuteConcede(Board *board, const VSAction &action) {
        if (action.side == VSSide::Plants) {
            board->mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_ZOMBIE_WON);
            board->mApp->mGameScene = GameScenes::SCENE_ZOMBIES_WON;
        } else {
            board->mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_PLANT_WON);
            board->mApp->mGameScene = GameScenes::SCENE_PLANTS_WON;
        }
        return VSActionResult::Applied;
    }

} // namespace

VSActionResult ExecuteBoardAction(Board *board, const VSAction &action, VSActionExecutionContext context) {
    if (!IsValidSide(action.side)) {
        return VSActionResult::RejectedInvalidSide;
    }
    if (!context.replayExecution && !context.localVSMatch) {
        return VSActionResult::RejectedNotLocalVS;
    }
    if (!context.matchPlaying) {
        return VSActionResult::RejectedMatchNotPlaying;
    }
    if (context.matchPaused) {
        return VSActionResult::RejectedMatchPaused;
    }

    const std::uint32_t boardTick = static_cast<std::uint32_t>(board->mMainCounter);
    if (!context.replayExecution && IsActionExpired(action, boardTick)) {
        return VSActionResult::RejectedStale;
    }
    if (!context.replayExecution && IsActionDeferred(action, boardTick)) {
        return VSActionResult::Deferred;
    }

    switch (action.kind) {
        case VSActionKind::PlaySeed:
            return ExecutePlaySeed(board, action);
        case VSActionKind::Shovel:
            return ExecuteShovel(board, action);
        case VSActionKind::FireCobCannon:
            return ExecuteFireCobCannon(board, action);
        case VSActionKind::CollectResource:
            return ExecuteCollectResource(board, action);
        case VSActionKind::Concede:
            return ExecuteConcede(board, action);
    }
    return VSActionResult::RejectedUnsupported;
}

} // namespace vsai::detail
