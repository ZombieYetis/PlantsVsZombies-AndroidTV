/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PvZ/Lawn/System/SaveGame.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/MessageWidget.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/NetPlay.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <cstring>
#include <algorithm>
#include <chrono>
#include <memory>

namespace {
enum class OnlineLoadState { Idle, AwaitingHost, Sending, Receiving, AwaitingReady, AwaitingResume };
OnlineLoadState gOnlineLoadState = OnlineLoadState::Idle;
bool gApplyingOnlineSaveGame = false;
std::vector<unsigned char> gOnlineSaveData;
std::size_t gOnlineSaveOffset = 0;
uint32_t gOnlineSaveSize = 0;
uint32_t gOnlineSaveHash = 0;
bool gOnlineSaveLoaded = false;
constexpr std::size_t kMaxOnlineSaveSize = 64 * 1024 * 1024;
auto gOnlineSaveProgress = std::chrono::steady_clock::now();

uint32_t SaveDataHash(const std::vector<unsigned char> &data) {
    uint32_t hash = 2166136261u;
    for (unsigned char byte : data) {
        hash = (hash ^ byte) * 16777619u;
    }
    return hash;
}

bool ApplyOnlineSave(LawnApp *app, SaveGameContext *context) {
    struct ApplyingSave {
        ApplyingSave() {
            gApplyingOnlineSaveGame = true;
        }
        ~ApplyingSave() {
            gApplyingOnlineSaveGame = false;
        }
    } applyingSave;

    app->KillSeedChooserScreen();
    app->PostEnterLevel();
    app->MakeNewBoard();
    app->SetSecondPlayer(1);
    if (!LawnLoadGame(app->mBoard, context)) {
        return false;
    }

    // The native PostLoadGame ends with ClearSecondPlayer. The scoped flag
    // preserves the online connection while retaining its other fixups.
    app->mBoard->PostLoadGame();
    app->SetSecondPlayer(1);
    for (int player = 0; player < 2; ++player) {
        auto *controls = app->mBoard->mGamepadControls[player];
        controls->mApp = app;
        controls->mBoard = app->mBoard;
        controls->mPlayerIndex = player;
        controls->mGamepadIndex = player;
        controls->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
        controls->mSelectedSeedIndex = 0;
        app->mBoard->ClearCursor(player);
    }
    app->mBoard->mPaused = false;
    app->mBoardResult = BoardResult::BOARDRESULT_NONE;
    app->mFirstTimeGameSelector = false;
    app->mBoard->MapLoadedNetplayIds();
    return true;
}

void AbortOnlineSave(LawnApp *app, bool notifyPeer) {
    if (notifyPeer && IsRemoteServer()) {
        BaseEvent event = {EVENT_SERVER_SAVEGAME_ABORT};
        netplay::PutEvent(event);
    }
    netplay::ResetSaveGameTransfer();
    app->mNeedLoadGame = false;
    app->KillSeedChooserScreen();
    app->ReturnToModeSelect();
    LOG_ERROR("[NETPLAY] coop save transfer failed; returned to mode selection");
}
} // namespace

bool netplay::IsApplyingOnlineSaveGame() noexcept {
    return gApplyingOnlineSaveGame;
}

bool netplay::IsSynchronizingSaveGame() noexcept {
    return gOnlineLoadState != OnlineLoadState::Idle;
}

void netplay::ResetSaveGameTransfer() noexcept {
    gOnlineLoadState = OnlineLoadState::Idle;
    gOnlineSaveData.clear();
    gOnlineSaveOffset = 0;
    gOnlineSaveSize = 0;
    gOnlineSaveHash = 0;
    gOnlineSaveLoaded = false;
}

bool netplay::StartCoopEndlessLoad(LawnApp *app) {
    ResetSaveGameTransfer();
    gOnlineSaveProgress = std::chrono::steady_clock::now();
    if (!IsRemoteServer()) {
        // Never consult the guest's local save. Wait for the host's decision.
        app->KillSeedChooserScreen();
        app->KillBoard();
        app->mGameScene = GameScenes::SCENE_LEVEL_INTRO;
        gOnlineLoadState = OnlineLoadState::AwaitingHost;
        return true;
    }

    if (!app->TryLoadGame()) {
        delete app->mSaveGame;
        app->mSaveGame = nullptr;
        BaseEvent event = {EVENT_SERVER_SAVEGAME_NEW_GAME};
        PutEvent(event);
        return false;
    }

    std::unique_ptr<SaveGameContext> context(app->mSaveGame);
    app->mSaveGame = nullptr;
    app->mNeedLoadGame = false; // This handshake replaces the local continue dialog.
    app->mSaveGameOperation = SaveGameOperation::SAVE_GAME_OPERATION_NONE;
    if (context->mBuffer.mData.size() < sizeof(SaveFileHeader) || context->mBuffer.mData.size() > kMaxOnlineSaveSize || !ApplyOnlineSave(app, context.get())) {
        AbortOnlineSave(app, true);
        return true;
    }

    gOnlineSaveData = std::move(context->mBuffer.mData);
    gOnlineSaveSize = static_cast<uint32_t>(gOnlineSaveData.size());
    gOnlineSaveHash = SaveDataHash(gOnlineSaveData);
    gOnlineLoadState = OnlineLoadState::Sending;
    U16UNI32UNI32_Event begin{};
    begin.type = EVENT_SERVER_SAVEGAME_BEGIN;
    begin.data1 = uint16_t(app->mGameMode);
    begin.data2.u32 = gOnlineSaveSize;
    begin.data3.u32 = gOnlineSaveHash;
    PutEvent(begin);
    LOG_INFO("[NETPLAY] sending coop save: {} bytes", gOnlineSaveSize);
    return true;
}

void netplay::UpdateSaveGameTransfer(LawnApp *app) {
    if (!IsSynchronizingSaveGame()) {
        return;
    }
    if (!IsOnlineModeActive() && !gIsReplayMode && !gIsServerModeSpectator) {
        ResetSaveGameTransfer();
        return;
    }
    if (!gIsReplayMode && std::chrono::steady_clock::now() - gOnlineSaveProgress > std::chrono::minutes(2)) {
        if (IsRemoteServer()) {
            AbortOnlineSave(app, true);
        } else {
            // Closing the stalled stream prevents late chunks reaching a new board.
            app->ClearSecondPlayer();
            AbortOnlineSave(app, false);
        }
        return;
    }
    if (gOnlineLoadState != OnlineLoadState::Sending || HasPendingSendData()) {
        return;
    }
    // Bound queued data so a save cannot flood the relay's pending-write queue.
    const std::size_t batchEnd = std::min(gOnlineSaveOffset + 16 * 1024, gOnlineSaveData.size());
    while (gOnlineSaveOffset < batchEnd) {
        U16U8x240_Event chunk{};
        chunk.type = EVENT_SERVER_SAVEGAME_CHUNK;
        chunk.count = static_cast<uint16_t>(std::min(sizeof(chunk.data), gOnlineSaveData.size() - gOnlineSaveOffset));
        std::memcpy(chunk.data, gOnlineSaveData.data() + gOnlineSaveOffset, chunk.count);
        PutEvent(chunk);
        gOnlineSaveOffset += chunk.count;
    }
    gOnlineSaveProgress = std::chrono::steady_clock::now();
    if (gOnlineSaveOffset == gOnlineSaveData.size()) {
        BaseEvent end = {EVENT_SERVER_SAVEGAME_END};
        PutEvent(end);
        gOnlineSaveData.clear();
        gOnlineLoadState = OnlineLoadState::AwaitingReady;
    }
}

bool netplay::HandleSaveGameEvent(LawnApp *app, const BaseEvent *event, bool fromHost) {
    if (event->type < EVENT_SERVER_SAVEGAME_BEGIN || event->type > EVENT_SERVER_SAVEGAME_ABORT) {
        return false;
    }
    if (app->mGameMode != GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS) {
        return true;
    }
    if (!fromHost) {
        if (event->type == EVENT_CLIENT_SAVEGAME_READY && IsRemoteServer() && (gOnlineLoadState == OnlineLoadState::Sending || gOnlineLoadState == OnlineLoadState::AwaitingReady)) {
            if (static_cast<const U8_Event *>(event)->data == 0) {
                AbortOnlineSave(app, true);
            } else if (gOnlineLoadState == OnlineLoadState::AwaitingReady) {
                BaseEvent resume = {EVENT_SERVER_SAVEGAME_RESUME};
                PutEvent(resume);
                ResetSaveGameTransfer();
            }
        }
        return true;
    }

    switch (event->type) {
        case EVENT_SERVER_SAVEGAME_NEW_GAME:
            if (gOnlineLoadState == OnlineLoadState::AwaitingHost) {
                ResetSaveGameTransfer();
                app->PostEnterLevel();
                app->NewGame();
            }
            break;
        case EVENT_SERVER_SAVEGAME_BEGIN: {
            const auto *begin = static_cast<const U16UNI32UNI32_Event *>(event);
            if (begin->data1 != uint16_t(GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS) || begin->data2.u32 < sizeof(SaveFileHeader) || begin->data2.u32 > kMaxOnlineSaveSize) {
                U8_Event ready = {{EVENT_CLIENT_SAVEGAME_READY}, 0};
                if (!gIsReplayMode && !gIsServerModeSpectator)
                    PutEvent(ready);
                break;
            }
            ResetSaveGameTransfer();
            gOnlineLoadState = OnlineLoadState::Receiving;
            gOnlineSaveSize = begin->data2.u32;
            gOnlineSaveHash = begin->data3.u32;
            gOnlineSaveData.reserve(gOnlineSaveSize);
            gOnlineSaveProgress = std::chrono::steady_clock::now();
            break;
        }
        case EVENT_SERVER_SAVEGAME_CHUNK: {
            if (gOnlineLoadState != OnlineLoadState::Receiving)
                break;
            const auto *chunk = static_cast<const U16U8x240_Event *>(event);
            if (chunk->count > sizeof(chunk->data) || gOnlineSaveData.size() + chunk->count > gOnlineSaveSize) {
                gOnlineLoadState = OnlineLoadState::AwaitingResume;
                U8_Event ready = {{EVENT_CLIENT_SAVEGAME_READY}, 0};
                if (!gIsReplayMode && !gIsServerModeSpectator)
                    PutEvent(ready);
                break;
            }
            gOnlineSaveData.insert(gOnlineSaveData.end(), chunk->data, chunk->data + chunk->count);
            gOnlineSaveProgress = std::chrono::steady_clock::now();
            break;
        }
        case EVENT_SERVER_SAVEGAME_END: {
            if (gOnlineLoadState != OnlineLoadState::Receiving)
                break;
            bool loaded = false;
            if (gOnlineSaveData.size() == gOnlineSaveSize && SaveDataHash(gOnlineSaveData) == gOnlineSaveHash) {
                SaveGameContext context{};
                context.mReading = true;
                context.mBuffer.mData = std::move(gOnlineSaveData);
                context.mBuffer.mDataBitSize = int(gOnlineSaveSize * 8);
                context.mBuffer.mWriteBitPos = context.mBuffer.mDataBitSize;
                // Apply before consuming any later gameplay event from this TCP batch.
                loaded = ApplyOnlineSave(app, &context);
            }
            gOnlineLoadState = OnlineLoadState::AwaitingResume;
            gOnlineSaveLoaded = loaded;
            gOnlineSaveProgress = std::chrono::steady_clock::now();
            U8_Event ready = {{EVENT_CLIENT_SAVEGAME_READY}, uint8_t(loaded)};
            if (!gIsReplayMode && !gIsServerModeSpectator)
                PutEvent(ready);
            LOG_INFO("[NETPLAY] received coop save: loaded={}", loaded);
            break;
        }
        case EVENT_SERVER_SAVEGAME_RESUME:
            if (gOnlineLoadState == OnlineLoadState::AwaitingResume) {
                if (gOnlineSaveLoaded)
                    ResetSaveGameTransfer();
                else
                    AbortOnlineSave(app, false);
            }
            break;
        case EVENT_SERVER_SAVEGAME_ABORT:
            AbortOnlineSave(app, false);
            break;
        default:
            break;
    }
    return true;
}

bool LawnSaveGame_Original(Board *theBoard, const pvzstl::string &theFilePath) {
    SaveGameContext aContext{};
    aContext.mFailed = false;
    aContext.mReading = false;

    SaveFileHeader aHeader{};
    aHeader.mMagicNumber = SAVE_FILE_MAGIC_NUMBER;
    aHeader.mBuildVersion = SAVE_FILE_VERSION;
    aHeader.mBuildDate = SAVE_FILE_DATE;

    aContext.SyncBytes(&aHeader, sizeof(aHeader));
    SyncBoard(&aContext, theBoard);
    theBoard->mApp->mNeedGoBackToMain = false; // 用于从暂停菜单返回主界面
    return gLawnApp->WriteBufferToFile(theFilePath, &aContext.mBuffer);
}

bool LawnSaveGame(Board *theBoard, const pvzstl::string &theFilePath) {
    if (theBoard->mApp->IsCoopMode() && IsRemoteClientOrViewer()) {
        // A received board must never replace the guest's own local coop save.
        theBoard->mApp->mNeedGoBackToMain = false;
        return true;
    }
    if (disableSaveUserdata) {
        theBoard->mApp->mNeedGoBackToMain = false; // 用于从暂停菜单返回主界面
        return true;
    }

    // 结盟模式存档，将SeedBank2的4个种子放到SeedBank1里面。因为原版存档逻辑难以改动，只好出此下策，凑合着存吧。
    if (theBoard->mApp->IsCoopMode()) {
        if (theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS) {
            int aNumSeeds = 6;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank1->mX = seedBank2->mX;
            for (int i = 0; i < aNumSeeds; ++i) {
                seedBank1->mSeedPackets[i].mSlotMachiningNextSeed = (SeedType)seedBank2->mSeedPackets[i].mY;
                seedBank1->mSeedPackets[i].mTimesUsed = seedBank2->mSeedPackets[i].mX;
                seedBank1->mSeedPackets[i].mImitaterType = seedBank2->mSeedPackets[i].mPacketType;
                seedBank1->mSeedPackets[i].mRefreshCounter = seedBank2->mSeedPackets[i].mOffsetY;
                seedBank1->mSeedPackets[i].mSlotMachineCountDown = seedBank2->mSeedPackets[i].mIndex;
            }
            bool result = LawnSaveGame_Original(theBoard, theFilePath);
            seedBank1->mX = 0;
            return result;
        } else {
            int theSeedNum = 4;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank1->mNumPackets = 2 * theSeedNum;
            seedBank1->mX = seedBank2->mX;
            for (int i = theSeedNum; i < 2 * theSeedNum; ++i) {
                seedBank1->mSeedPackets[i].mX = seedBank2->mSeedPackets[i - theSeedNum].mX;
                seedBank1->mSeedPackets[i].mY = seedBank2->mSeedPackets[i - theSeedNum].mY;
                seedBank1->mSeedPackets[i].mRefreshCounter = seedBank2->mSeedPackets[i - theSeedNum].mRefreshCounter;
                seedBank1->mSeedPackets[i].mRefreshTime = seedBank2->mSeedPackets[i - theSeedNum].mRefreshTime;
                seedBank1->mSeedPackets[i].mIndex = seedBank2->mSeedPackets[i - theSeedNum].mIndex;
                seedBank1->mSeedPackets[i].mOffsetY = seedBank2->mSeedPackets[i - theSeedNum].mOffsetY;
                seedBank1->mSeedPackets[i].mPacketType = seedBank2->mSeedPackets[i - theSeedNum].mPacketType;
                seedBank1->mSeedPackets[i].mImitaterType = seedBank2->mSeedPackets[i - theSeedNum].mImitaterType;
                seedBank1->mSeedPackets[i].mActive = seedBank2->mSeedPackets[i - theSeedNum].mActive;
                seedBank1->mSeedPackets[i].mRefreshing = seedBank2->mSeedPackets[i - theSeedNum].mRefreshing;
                seedBank1->mSeedPackets[i].mTimesUsed = seedBank2->mSeedPackets[i - theSeedNum].mTimesUsed;
                seedBank1->mSeedPackets[i].mSeedBank = seedBank1;
                seedBank1->mSeedPackets[i].mSelectedBy2P = seedBank2->mSeedPackets[i - theSeedNum].mSelectedBy2P;
                seedBank1->mSeedPackets[i].mSelected = seedBank2->mSeedPackets[i - theSeedNum].mSelected;
                seedBank1->mSeedPackets[i].mSelectedByBothPlayer = seedBank2->mSeedPackets[i - theSeedNum].mSelectedByBothPlayer;
            }
            bool result = LawnSaveGame_Original(theBoard, theFilePath);
            seedBank1->mNumPackets = theSeedNum;
            seedBank1->mX = 0;
            return result;
        }
    }
    // Zombie *zombie = NULL;
    // while (Board_IterateZombies(theBoard, &zombie)) {
    // if (zombie->mZombieType == ZombieType::Flag) {
    // LawnApp_RemoveReanimation(zombie->mApp, zombie->mBossFireBallReanimID);
    // zombie->mBossFireBallReanimID = 0;
    // }
    // }
    return LawnSaveGame_Original(theBoard, theFilePath);
}

bool LawnLoadGame_Original(Board *theBoard, SaveGameContext *theContext) {

    SaveFileHeader aHeader{};
    theContext->SyncBytes(&aHeader, sizeof(aHeader));

    // 检查存档魔数和版本范围。
    if (aHeader.mMagicNumber != SAVE_FILE_MAGIC_NUMBER || aHeader.mBuildVersion > SAVE_FILE_VERSION) {
        if (!netplay::IsApplyingOnlineSaveGame())
            gLawnApp->HandleCorruptedGameFile();
        return false;
    }
    // 魔数正确，但不是当前支持的版本。
    if (aHeader.mBuildVersion != SAVE_FILE_VERSION) {
        if (!netplay::IsApplyingOnlineSaveGame())
            gLawnApp->HandleOldGameFile();
        return false;
    }
    SyncBoard(theContext, theBoard);
    if (gLawnApp->IsAdventureMode()) {
        if (gLawnApp->mPlayerInfo->mLevel != theBoard->mLevel) {
            const int highestLevel = theBoard->mLevel > gLawnApp->mPlayerInfo->mLevel ? theBoard->mLevel : gLawnApp->mPlayerInfo->mLevel;
            gLawnApp->mPlayerInfo->mLevel = highestLevel;
        }
    }
    if (theContext->mFailed) {
        if (!netplay::IsApplyingOnlineSaveGame())
            gLawnApp->HandleCorruptedGameFile();
        return false;
    }

    FixBoardAfterLoad(theBoard);
    theBoard->mApp->mGameScene = GameScenes::SCENE_PLAYING;
    return true;
}

void FixBoardAfterLoad(Board *theBoard) {
    LawnApp *app = theBoard->mApp;

    theBoard->mTangleKelpTree->clear();
    theBoard->mFlowerPotTree->clear();
    theBoard->mPumpkinTree->clear();

    Plant *aPlant = nullptr;
    while (theBoard->mPlants.IterateNext(aPlant)) {
        aPlant->mApp = app;
        aPlant->mBoard = theBoard;

        switch (aPlant->mSeedType) {
            case SeedType::SEED_TANGLEKELP:
                theBoard->mTangleKelpTree->emplace(aPlant);
                break;

            case SeedType::SEED_FLOWERPOT:
                theBoard->mFlowerPotTree->emplace(aPlant);
                break;

            case SeedType::SEED_PUMPKINSHELL:
                theBoard->mPumpkinTree->emplace(aPlant);
                break;

            default:
                break;
        }
    }

    Zombie *aZombie = nullptr;
    while (theBoard->mZombies.IterateNext(aZombie)) {
        aZombie->mApp = app;
        aZombie->mBoard = theBoard;

        aZombie->StartZombieSound();
    }

    Projectile *aProjectile = nullptr;
    while (theBoard->mProjectiles.IterateNext(aProjectile)) {
        aProjectile->mApp = app;
        aProjectile->mBoard = theBoard;
    }

    Coin *aCoin = nullptr;
    while (theBoard->mCoins.IterateNext(aCoin)) {
        aCoin->mApp = app;
        aCoin->mBoard = theBoard;
    }

    LawnMower *aLawnMower = nullptr;
    while (theBoard->mLawnMowers.IterateNext(aLawnMower)) {
        aLawnMower->mApp = app;
        aLawnMower->mBoard = theBoard;
    }

    GridItem *aGridItem = nullptr;
    while (theBoard->mGridItems.IterateNext(aGridItem)) {
        aGridItem->mApp = app;
        aGridItem->mBoard = theBoard;
    }

    theBoard->mAdvice->mApp = app;
    for (int player = 0; player < 2; ++player) {
        theBoard->mCursorObject[player]->mApp = app;
        theBoard->mCursorObject[player]->mBoard = theBoard;
        theBoard->mCursorPreview[player]->mApp = app;
        theBoard->mCursorPreview[player]->mBoard = theBoard;
        SeedBank *aSeedBank = theBoard->mSeedBank[player];
        if (aSeedBank != nullptr) {
            aSeedBank->mApp = app;
            aSeedBank->mBoard = theBoard;
            for (auto &aPacket : aSeedBank->mSeedPackets) {
                aPacket.mApp = app;
                aPacket.mBoard = theBoard;
                aPacket.mSeedBank = aSeedBank;
            }
        }
    }
    theBoard->mChallenge->mApp = app;
    theBoard->mChallenge->mBoard = theBoard;
    theBoard->mGamepadControls[0]->mGamepadIndex = app->PlayerToGamepadIndex(theBoard->mGamepadControls[0]->mPlayerIndex);
    app->mMusic->mApp = app;
    app->mMusic->mMusicInterface = app->mMusicInterface;

    // 修复读档后的各种问题
    theBoard->FixReanimErrorAfterLoad();
}

bool LawnLoadGame(Board *theBoard, SaveGameContext *theContext) {
    // 结盟模式读档，将SeedBank2的4个种子从SeedBank1里面取出。因为原版读档逻辑难以改动，只好出此下策，凑合着读吧。
    if (theBoard->mApp->IsCoopMode()) {
        if (theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS) {
            bool result = LawnLoadGame_Original(theBoard, theContext);
            if (!result)
                return false;
            int theSeedNum = 6;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank2->mNumPackets = theSeedNum;
            seedBank1->mNumPackets = theSeedNum;
            seedBank2->mX = seedBank1->mX;
            seedBank1->mX = 0;
            for (int i = 0; i < theSeedNum; ++i) {
                seedBank2->mSeedPackets[i].mY = seedBank1->mSeedPackets[i].mSlotMachiningNextSeed;
                seedBank2->mSeedPackets[i].mX = seedBank1->mSeedPackets[i].mTimesUsed;
                seedBank2->mSeedPackets[i].mPacketType = seedBank1->mSeedPackets[i].mImitaterType;
                seedBank2->mSeedPackets[i].mOffsetY = seedBank1->mSeedPackets[i].mRefreshCounter;
                seedBank2->mSeedPackets[i].mIndex = seedBank1->mSeedPackets[i].mSlotMachineCountDown;

                seedBank1->mSeedPackets[i].mTimesUsed = 0;
                seedBank1->mSeedPackets[i].mImitaterType = SeedType::SEED_NONE;
                seedBank1->mSeedPackets[i].mRefreshCounter = 0;
                seedBank1->mSeedPackets[i].mSlotMachineCountDown = 0;
                seedBank1->mSeedPackets[i].mSlotMachiningNextSeed = SeedType::SEED_NONE;
            }
            return result;
        } else {
            bool result = LawnLoadGame_Original(theBoard, theContext);
            if (!result)
                return false;
            int theSeedNum = 4;
            SeedBank *seedBank1 = theBoard->mSeedBank[0];
            SeedBank *seedBank2 = theBoard->mSeedBank[1];
            seedBank2->mNumPackets = theSeedNum;
            seedBank1->mNumPackets = theSeedNum;
            seedBank2->mX = seedBank1->mX;
            seedBank1->mX = 0;
            for (int i = theSeedNum; i < 2 * theSeedNum; ++i) {
                seedBank2->mSeedPackets[i - theSeedNum].mX = seedBank1->mSeedPackets[i].mX;
                seedBank2->mSeedPackets[i - theSeedNum].mY = seedBank1->mSeedPackets[i].mY;
                seedBank2->mSeedPackets[i - theSeedNum].mRefreshCounter = seedBank1->mSeedPackets[i].mRefreshCounter;
                seedBank2->mSeedPackets[i - theSeedNum].mRefreshTime = seedBank1->mSeedPackets[i].mRefreshTime;
                seedBank2->mSeedPackets[i - theSeedNum].mIndex = seedBank1->mSeedPackets[i].mIndex;
                seedBank2->mSeedPackets[i - theSeedNum].mOffsetY = seedBank1->mSeedPackets[i].mOffsetY;
                seedBank2->mSeedPackets[i - theSeedNum].mPacketType = seedBank1->mSeedPackets[i].mPacketType;
                seedBank2->mSeedPackets[i - theSeedNum].mImitaterType = seedBank1->mSeedPackets[i].mImitaterType;
                seedBank2->mSeedPackets[i - theSeedNum].mActive = seedBank1->mSeedPackets[i].mActive;
                seedBank2->mSeedPackets[i - theSeedNum].mRefreshing = seedBank1->mSeedPackets[i].mRefreshing;
                seedBank2->mSeedPackets[i - theSeedNum].mTimesUsed = seedBank1->mSeedPackets[i].mTimesUsed;
                seedBank2->mSeedPackets[i - theSeedNum].mSeedBank = seedBank2;
                seedBank2->mSeedPackets[i - theSeedNum].mSelectedBy2P = seedBank1->mSeedPackets[i].mSelectedBy2P;
                seedBank2->mSeedPackets[i - theSeedNum].mSelected = seedBank1->mSeedPackets[i].mSelected;
                seedBank2->mSeedPackets[i - theSeedNum].mSelectedByBothPlayer = seedBank1->mSeedPackets[i].mSelectedByBothPlayer;
            }
            return result;
        }
    }


    return LawnLoadGame_Original(theBoard, theContext);
}


void SaveGameContext::SyncReanimationDef(ReanimatorDefinition *&theDefinition) {
    // 解决大头贴动画的读档问题
    if (mReading) {
        int aReanimType = 0;
        SyncInt(aReanimType);
        if (aReanimType == ReanimationType::REANIM_NONE) {
            theDefinition = nullptr;
        } else if (aReanimType >= 0 && aReanimType < ReanimationType::EXTENDED_NUM_REANIMS) {
            ReanimatorEnsureDefinitionLoaded(ReanimationType(aReanimType), true);
            theDefinition = &gReanimatorDefArray[aReanimType];
        } else {
            mFailed = true;
        }
    } else {
        int aReanimType = ReanimationType::REANIM_NONE;
        for (int i = 0; i < ReanimationType::EXTENDED_NUM_REANIMS; ++i) {
            ReanimatorDefinition *aDef = &gReanimatorDefArray[i];
            if (theDefinition == aDef) {
                aReanimType = i;
                break;
            }
        }
        SyncInt(aReanimType);
    }
}

void SyncReanimation(Board *theBoard, Reanimation *theReanimation, SaveGameContext &theContext) {
    theContext.SyncReanimationDef(theReanimation->mDefinition);

    if (theContext.mReading) {
        theReanimation->mReanimationHolder = theBoard->mApp->mEffectSystem->mReanimationHolder;
    }

    const int aTrackCount = theReanimation->mDefinition->mTrackCount;
    if (aTrackCount == 0) {
        return;
    }

    const std::size_t aTrackInstancesSize = static_cast<std::size_t>(aTrackCount) * sizeof(ReanimatorTrackInstance);

    const std::size_t aTransformsSize = static_cast<std::size_t>(aTrackCount) * sizeof(ReanimatorTransform);

    const std::size_t aAllocationSize = aTrackInstancesSize + aTransformsSize;

    if (theContext.mReading) {
        auto *aMemory = static_cast<unsigned char *>(FindGlobalAllocator(aAllocationSize)->Calloc(aAllocationSize));

        HOMURA_ASSERT(aMemory != nullptr);

        theReanimation->mTrackInstances = reinterpret_cast<ReanimatorTrackInstance *>(aMemory);

        theReanimation->mReanimatorTransforms = reinterpret_cast<ReanimatorTransform *>(aMemory + aTrackInstancesSize);

        theReanimation->unk2[0] = 7;

        theReanimation->SetAnimRate(theReanimation->mAnimRate);
    }

    // 原版存档只同步 TrackInstance，不同步后面的 ReanimatorTransform。
    theContext.SyncBytes(theReanimation->mTrackInstances, static_cast<int>(aTrackInstancesSize));

    for (int aTrackIndex = 0; aTrackIndex < aTrackCount; ++aTrackIndex) {
        ReanimatorTrackInstance &aTrackInstance = theReanimation->mTrackInstances[aTrackIndex];

        theContext.SyncImage(aTrackInstance.mImageOverride);

        if (theContext.mReading) {
            aTrackInstance.mBlendTransform.mText = "";

            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mFont == nullptr);
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mImage == nullptr);
        } else {
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mText[0] == '\0');
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mFont == nullptr);
            HOMURA_ASSERT(aTrackInstance.mBlendTransform.mImage == nullptr);
        }
    }
}
