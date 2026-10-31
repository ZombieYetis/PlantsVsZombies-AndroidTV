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
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/MessageWidget.h"
#include "PvZ/Lawn/Board/OpeningEncounter.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/System/SpectatorWidgetSync.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/NetplayLobbyWidget.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/VSResultsMenu.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/NetPlay.h"
#include "PvZ/RelayReconnect.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/SexyAppFramework/Widget/Checkbox.h"
#include "PvZ/SexyAppFramework/Widget/ImageWidget.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Reanimator.h"
#include "PvZ/TodLib/Effect/TodParticle.h"

#include <sys/socket.h>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <memory>
#include <type_traits>
#include <utility>

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

    // 原版 PostLoadGame 最后会调用 ClearSecondPlayer；此作用域内的标记
    // 用于保留联机连接，同时保留原版的其他读档修正逻辑。
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

namespace {
// Separate from the co-op handshake: capturing/sending never pauses the players.
bool gCapturingSpectatorSnapshot = false;
bool gApplyingSpectatorSnapshot = false;
bool gSpectatorSnapshotRequested = false;
bool gReceivingSpectatorSnapshot = false;
bool gSpectatorDraftProgressPending = false;
std::vector<unsigned char> gSpectatorSnapshotOut, gSpectatorSnapshotIn;
std::vector<std::byte> gSpectatorCatchup, gSpectatorCatchupReady;
std::size_t gSpectatorSnapshotOffset = 0;
uint32_t gSpectatorSnapshotSize = 0, gSpectatorSnapshotHash = 0;
uint32_t gSpectatorHistoryEpoch = 0;
auto gSpectatorSnapshotProgress = std::chrono::steady_clock::now();
constexpr std::size_t kMaxSpectatorCatchup = 16 * 1024 * 1024;
constexpr uint16_t kSpectatorSnapshotVersion = 4;

// UI objects contain local pointers, listeners and widget-manager links. Only
// serialize explicit value fields; construct the actual widgets on the viewer.
template <typename... T>
void SyncSpectatorValues(SaveGameContext &context, T &...values) {
    static_assert((std::is_trivially_copyable_v<T> && ...));
    static_assert((!std::is_pointer_v<std::remove_all_extents_t<T>> && ...));
    (context.SyncBytes(&values, sizeof(values)), ...);
}

struct SpectatorScene {
    GameScenes scene = SCENE_PLAYING;
    BackgroundType background = BACKGROUND_1_DAY;
    bool setup = false, plantChooser = false, zombieChooser = false;
    bool shuffle = false, classic = false, extraPackets = false, ban = false, timed = false;
    uint32_t historyEpoch = 0;

    void Sync(SaveGameContext &context) {
        SyncSpectatorValues(context, scene, background, setup, plantChooser, zombieChooser, shuffle, classic, extraPackets, ban, timed, historyEpoch);
    }
};

// Local identities only: restart a transfer if its board/UI is replaced, or
// drafting finishes before delivery. A new BEGIN discards the obsolete backlog.
Board *gSpectatorSourceBoard = nullptr;
VSSetupMenu *gSpectatorSourceSetup = nullptr;
SeedChooserScreen *gSpectatorSourcePlant = nullptr, *gSpectatorSourceZombie = nullptr;
GameScenes gSpectatorSourceScene = SCENE_PLAYING;

SpectatorScene GetSpectatorScene(LawnApp *app) {
    SpectatorScene scene;
    scene.scene = app->mGameScene;
    scene.historyEpoch = replay::GetRecordingEpoch();
    scene.background = app->mBoard->mBackground;
    scene.setup = app->mVSSetupMenu != nullptr;
    scene.plantChooser = app->mSeedChooserScreen != nullptr;
    scene.zombieChooser = app->mZombieChooserScreen != nullptr;
    scene.shuffle = Challenge::msVSShuffleMode;
    scene.classic = Challenge::msVSClassicMode;
    scene.timed = VSSetupAddonWidget::msTimedDraftMode;
    if (scene.setup) {
        scene.extraPackets = app->mVSSetupMenu->mAddonWidget->mExtraPacketMode;
        scene.ban = app->mVSSetupMenu->mAddonWidget->mBanMode;
    }
    return scene;
}

void SyncSpectatorWidget(SaveGameContext &context, Sexy::Widget *widget) {
    netplay::snapshot::SyncWidget(context, widget);
}

void SyncSpectatorChooser(SaveGameContext &context, SeedChooserScreen &chooser) {
    SyncSpectatorWidget(context, &chooser);
    SyncSpectatorValues(context,
                        chooser.mShowHelpText,
                        chooser.mChosenSeeds,
                        chooser.mSeedChooserAge,
                        chooser.mSeedsInFlight,
                        chooser.mSeedsInBank,
                        chooser.mSeedsIn1PBank,
                        chooser.mSeedsIn2PBank,
                        chooser.mCursorPositionX1,
                        chooser.mCursorPositionX2,
                        chooser.mCursorPositionY1,
                        chooser.mCursorPositionY2,
                        chooser.mChooseState,
                        chooser.mViewLawnTime,
                        chooser.mPlayerIndex,
                        chooser.mSeedIndex1,
                        chooser.mSeedIndex2,
                        chooser.mCursorBobPhase,
                        chooser.mDimCounter,
                        chooser.mPageIndex,
                        chooser.mNumBanPackets,
                        chooser.mSeedsInBanned,
                        chooser.mBannedSeed,
                        chooser.mBanningPhase,
                        chooser.mGlobalBpBansApplied,
                        chooser.mChosenSeedsExtended,
                        chooser.mTimedDraftTicksRemaining,
                        chooser.mTimedDraftWasActive,
                        chooser.mReanimSeedChooser);
    SyncSpectatorWidget(context, chooser.mStartButton);
    if (context.mReading && !context.mFailed) {
        // Keep all app/board/bank/button pointers from the local constructor.
        chooser.mToolTipSeed1 = chooser.mToolTipSeed2 = -1;
        chooser.UpdateImitaterButton();
        chooser.RebuildHelpbar();
        chooser.MarkDirty();
    }
}

void SyncSpectatorIntro(SaveGameContext &context, LawnApp *app) {
    auto &cutscene = *app->mBoard->mCutScene;
    SyncSpectatorWidget(context, app->mBoard);
    SyncSpectatorValues(context,
                        cutscene.mCutsceneTime,
                        cutscene.mSodTime,
                        cutscene.mGraveStoneTime,
                        cutscene.mReadySetPlantTime,
                        cutscene.mFogTime,
                        cutscene.mBossTime,
                        cutscene.mCrazyDaveTime,
                        cutscene.mLawnMowerTime,
                        cutscene.mCrazyDaveDialogStart,
                        cutscene.mSeedChoosing,
                        cutscene.mZombiesWonReanimID,
                        cutscene.mPreloaded,
                        cutscene.mPlacedZombies,
                        cutscene.mPlacedLawnItems,
                        cutscene.mCrazyDaveCountDown,
                        cutscene.mCrazyDaveLastTalkIndex,
                        cutscene.mUpsellHideBoard,
                        cutscene.mPreUpdatingBoard,
                        cutscene.mPlantsWonReanimID);
    if (auto *menu = app->mVSSetupMenu) {
        SyncSpectatorWidget(context, menu);
        SyncSpectatorValues(context,
                            menu->mInt70,
                            menu->mInt71,
                            menu->mInt72,
                            menu->mState,
                            menu->mControllerIndex,
                            menu->mSides,
                            menu->mSideLocked,
                            menu->mSeedPickTurn,
                            menu->mChooserAnimateUpdateCnt,
                            menu->mSetupMode);
        SyncSpectatorValues(context, is1PControllerMoving, is2PControllerMoving);
        for (int id = VSSetupMenu::BACKGROUND_FRAME; id <= VSSetupMenu::RANDOM_BUTTON; ++id) {
            auto *widget = menu->FindWidget(id);
            // VSSetupSides.txt omits PLANT_SIDE and ZOMBIE_SIDE. Do not
            // dereference these null results or skip their serialized fields.
            SyncSpectatorWidget(context, widget);
            if (id <= VSSetupMenu::CONTROLLER_1) {
                netplay::snapshot::SyncOptionalValue(context, widget ? &static_cast<Sexy::ImageWidget *>(widget)->mAlpha : nullptr);
            } else {
                for (int color : {Sexy::ButtonWidget::COLOR_LABEL, Sexy::ButtonWidget::COLOR_LABEL_HILITE}) {
                    auto *value = widget && widget->mColors->size() > static_cast<std::size_t>(color) ? &(*widget->mColors)[color] : nullptr;
                    netplay::snapshot::SyncOptionalValue(context, value);
                }
            }
        }
        auto *addon = menu->mAddonWidget;
        if (context.mReading) {
            addon->UpdateGlobalBpButtonState();
        }
        SyncSpectatorValues(context, addon->mDrawString);
        SyncSpectatorWidget(context, addon->mBackButton);
        SyncSpectatorWidget(context, addon->mGlobalBpButton);
        SyncSpectatorWidget(context, addon->mExtraPacketCheckbox);
        SyncSpectatorWidget(context, addon->mBanModeCheckbox);
        SyncSpectatorWidget(context, addon->mTimedDraftCheckbox);
        // AI settings are local-only and are never usable by a spectator.
        if (context.mReading) {
            addon->SetDisable(addon->mAISettingsButton);
        }
    }
    if (app->mSeedChooserScreen)
        SyncSpectatorChooser(context, *app->mSeedChooserScreen);
    if (app->mZombieChooserScreen)
        SyncSpectatorChooser(context, *app->mZombieChooserScreen);
}

void SyncSpectatorExtras(SaveGameContext &context, Board *board) {
    // Native SyncBoard only includes player 0. Use registered blocks, never pointers/vtables.
    const auto syncObject = [&](SyncObject *object) {
        for (auto &block : *object->mSyncBlocks) {
            context.SyncBytes(block.mAddress, static_cast<int>(block.mSize));
        }
    };
    syncObject(board->mSeedBank[1]);
    syncObject(board->mGamepadControls[1]);
    syncObject(board->mCursorObject[1]);
    syncObject(board->mCursorPreview[1]);
    context.SyncBytes(&Challenge::msVSShuffleMode, sizeof(Challenge::msVSShuffleMode));
    context.SyncBytes(&Challenge::msVSClassicMode, sizeof(Challenge::msVSClassicMode));
    SyncSpectatorValues(context, VSSetupMenu::msNextFirstPick, VSSetupMenu::msNextSidePickPlayerIndex, gGamepad1ToPlayerIndex);
    SyncSpectatorValues(context, Challenge::gVSResourseDropMode, Challenge::gVSResourceDropCount, Challenge::gVSWinMode, Challenge::gVSSuddenDeathMode);
    context.SyncBytes(gFreeForFristShuffle, sizeof(gFreeForFristShuffle));
    context.SyncBytes(&VSSetupAddonWidget::msGlobalBpMode, sizeof(VSSetupAddonWidget::msGlobalBpMode));
    context.SyncBytes(&VSSetupAddonWidget::msGlobalBpSeedsInitialized, sizeof(VSSetupAddonWidget::msGlobalBpSeedsInitialized));
    context.SyncBytes(VSSetupAddonWidget::msGlobalBpWins, sizeof(VSSetupAddonWidget::msGlobalBpWins));
    context.SyncBytes(VSSetupAddonWidget::msGlobalBpSeeds, sizeof(VSSetupAddonWidget::msGlobalBpSeeds));
    context.SyncBytes(VSResultsMenu::msPlayerRecords, sizeof(VSResultsMenu::msPlayerRecords));
    context.SyncBytes(&board->mJacksonDanceMode, sizeof(board->mJacksonDanceMode));
    bool hasEncounter = gOpeningEncounter != nullptr;
    context.SyncBytes(&hasEncounter, sizeof(hasEncounter));
    if (context.mReading && !context.mFailed) {
        delete gOpeningEncounter;
        gOpeningEncounter = hasEncounter ? new OpeningEncounter() : nullptr;
    }
    if (hasEncounter && !context.mFailed) {
        context.SyncBytes(&gOpeningEncounter->mType, sizeof(gOpeningEncounter->mType));
        context.SyncInt(gOpeningEncounter->mCounter);
        context.SyncBytes(&gOpeningEncounter->mDoEffect, sizeof(gOpeningEncounter->mDoEffect));
    }
    char hostName[sizeof(gServerHostName)]{};
    if (!context.mReading) {
        const char *localName = board->mApp->mPlayerInfo ? board->mApp->mPlayerInfo->mName : nullptr;
        const char *name = gServerHostName[0] != '\0' ? gServerHostName : localName;
        if (name)
            std::strncpy(hostName, name, sizeof(hostName) - 1);
    }
    context.SyncBytes(hostName, sizeof(hostName));
    if (context.mReading && !context.mFailed && hostName[0] != '\0') {
        // JOIN_RESULT already supplied a host name. An empty cache from an
        // older host must not overwrite it (or fall back to the viewer's name).
        hostName[sizeof(hostName) - 1] = '\0';
        std::memcpy(gServerHostName, hostName, sizeof(gServerHostName));
    }
    context.SyncBytes(gSecondPlayerName, sizeof(gSecondPlayerName));
}

void FailSpectatorSnapshot() {
    LOG_ERROR("[NETPLAY] live spectator snapshot failed; closing viewer connection");
    netplay::ResetSpectatorSnapshot();
    // Only disconnect this viewer; never send an abort to the playing host/guest.
    if (gIsServerModeSpectator && gTcpServerSocket >= 0) {
        shutdown(gTcpServerSocket, SHUT_RDWR);
    }
}

bool ApplyRuntimeSnapshot(LawnApp *app, std::vector<unsigned char> &data, GameMode mode, bool spectator) {
    struct Applying {
        Applying() {
            gApplyingOnlineSaveGame = gApplyingSpectatorSnapshot = true;
        }
        ~Applying() {
            gApplyingOnlineSaveGame = gApplyingSpectatorSnapshot = false;
        }
    } applying;
    SaveGameContext context{};
    context.mReading = true;
    context.mBuffer.mData = std::move(data);
    context.mBuffer.mDataBitSize = static_cast<int>(context.mBuffer.mData.size() * 8);
    context.mBuffer.mWriteBitPos = context.mBuffer.mDataBitSize;
    SpectatorScene scene;
    scene.Sync(context);
    if (context.mFailed || (spectator && !scene.historyEpoch) || (scene.scene != SCENE_LEVEL_INTRO && scene.scene != SCENE_PLAYING)
        || (scene.scene == SCENE_PLAYING && (scene.setup || scene.plantChooser || scene.zombieChooser)) || (mode == GAMEMODE_MP_VS && (scene.plantChooser || scene.zombieChooser) && !scene.setup))
        return false;

    app->mGameMode = mode;
    app->KillSeedChooserScreen();
    app->KillZombieChooserScreen();
    app->KillVSSetupScreen();
    app->PostEnterLevel();
    app->MakeNewBoard();
    app->SetSecondPlayer(1);
    app->mGameScene = scene.scene;
    app->mBoard->mBackground = gVSBackground = scene.background;
    Challenge::msVSShuffleMode = scene.shuffle;
    Challenge::msVSClassicMode = scene.classic;
    // Create the local widgets before restoring the effect arrays. The chooser
    // constructor skips its local clock; restore the host's clock ID below.
    if (scene.setup) {
        app->ShowVSSetupScreen();
        auto *addon = app->mVSSetupMenu->mAddonWidget;
        addon->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_ExtraPacket, scene.extraPackets, false);
        addon->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_BanMode, scene.ban, false);
        addon->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_TimedDraft, scene.timed, false);
    }
    VSSetupAddonWidget::msTimedDraftMode = scene.timed;
    if (scene.plantChooser)
        app->ShowSeedChooserScreen();
    if (scene.zombieChooser)
        app->ShowZombieChooserScreen();
    auto *music = app->mMusic;
    // SyncBoard replaces the music fields, but not the audio player's state.
    // Stop the local lobby tune before its ID is overwritten by the snapshot.
    music->GetVTable()->StopAllMusic(music);
    if (!LawnLoadGame_Original(app->mBoard, &context))
        return false;
    app->mGameScene = scene.scene; // LawnLoadGame normally resumes straight into combat.
    SyncSpectatorExtras(context, app->mBoard);
    if (scene.scene == SCENE_LEVEL_INTRO)
        SyncSpectatorIntro(context, app);
    if (context.mFailed || context.mBuffer.mReadBitPos != context.mBuffer.mDataBitSize)
        return false;
    gServerHostName[sizeof(gServerHostName) - 1] = '\0';
    gSecondPlayerName[sizeof(gSecondPlayerName) - 1] = '\0';
    auto *board = app->mBoard;
    const bool paused = board->mPaused;
    const MusicTune restoredMusicTune = music->mCurMusicTune;
    // A restored tune ID does not mean that tune is playing locally. Clear it
    // through the active TV/Xbox implementation before restarting playback.
    music->GetVTable()->StopAllMusic(music);
    if (scene.scene == SCENE_PLAYING) {
        board->PostLoadGame();
    } else {
        // PostLoadGame enables graves and starts battle music. During the
        // intro these must still be driven by the restored cutscene timeline.
        board->LoadBackgroundImages();
        board->ResetFPSStats();
    }
    // PostLoadGame chooses a default battle tune; the snapshot also covers
    // special-mode music and intro transitions, including intentional silence.
    if (restoredMusicTune == MUSIC_TUNE_NONE)
        music->GetVTable()->StopAllMusic(music);
    else
        music->GetVTable()->MakeSureMusicIsPlaying(music, restoredMusicTune);
    board->mPaused = paused;
    // LawnLoadGame already repaired native objects. Repair only the extra player-1
    // blocks here, avoiding a second animation/sound restoration pass.
    auto *bank = board->mSeedBank[1];
    bank->mApp = app;
    bank->mBoard = board;
    for (auto &packet : bank->mSeedPackets) {
        packet.mApp = app;
        packet.mBoard = board;
        packet.mSeedBank = bank;
    }
    board->mCursorObject[1]->mApp = app;
    board->mCursorObject[1]->mBoard = board;
    board->mCursorPreview[1]->mApp = app;
    board->mCursorPreview[1]->mBoard = board;
    for (int i = 0; i < 2; ++i) {
        board->mGamepadControls[i]->mApp = app;
        board->mGamepadControls[i]->mBoard = board;
        board->mGamepadControls[i]->mGamepadIndex = app->PlayerToGamepadIndex(board->mGamepadControls[i]->mPlayerIndex);
    }
    board->MapLoadedNetplayIds();
    gVSBackground = board->mBackground;
    app->mBoardResult = BoardResult::BOARDRESULT_NONE;
    app->mFirstTimeGameSelector = false;
    app->mNeedLoadGame = false;
    if (auto *lobby = NetplayLobbyWidget::GetInstance()) {
        lobby->mServerJoinedRoomGaming = false;
        lobby->mServerSpectateReservationActive = false;
        lobby->RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
    }
    app->KillChallengeScreen();
    if (spectator)
        replay::BindLiveHistory(scene.historyEpoch);
    LOG_INFO("[NETPLAY] live spectator snapshot loaded; catchup={} bytes", gSpectatorCatchup.size());
    return true;
}
} // namespace

namespace netplay {
bool CanCaptureReconnectSnapshot(LawnApp *app) {
    if (!app->mBoard || IsSynchronizingSaveGame() || app->mVSResultsMenu)
        return false;
    if (app->mGameMode != GAMEMODE_MP_VS && !app->IsCoopMode())
        return false;
    if (app->mGameScene == SCENE_PLAYING)
        return true;
    return app->mGameScene == SCENE_LEVEL_INTRO && app->mBoard->mZombieWavesReady && !app->mBoard->mCutScene->mPreUpdatingBoard && (!app->mVSSetupMenu || int(app->mVSSetupMenu->mState) >= 0);
}
bool CaptureReconnectSnapshot(LawnApp *app, std::vector<unsigned char> &data) {
    if (!CanCaptureReconnectSnapshot(app))
        return false;
    SaveGameContext context{};
    auto scene = GetSpectatorScene(app);
    scene.Sync(context);
    SaveFileHeader header{SAVE_FILE_MAGIC_NUMBER, SAVE_FILE_VERSION, SAVE_FILE_DATE};
    context.SyncBytes(&header, sizeof(header));
    struct Capturing {
        Capturing() {
            gCapturingSpectatorSnapshot = true;
        }
        ~Capturing() {
            gCapturingSpectatorSnapshot = false;
        }
    } capturing;
    SyncBoard(&context, app->mBoard);
    SyncSpectatorExtras(context, app->mBoard);
    if (scene.scene == SCENE_LEVEL_INTRO)
        SyncSpectatorIntro(context, app);
    if (context.mFailed || context.mBuffer.mData.size() > reconnect::MaxSnapshotSize)
        return false;
    data = std::move(context.mBuffer.mData);
    return true;
}
bool ApplyReconnectSnapshot(LawnApp *app, std::vector<unsigned char> &data, GameMode mode) {
    if (mode != GAMEMODE_MP_VS && !(mode >= GAMEMODE_TWO_PLAYER_COOP_DAY && mode <= GAMEMODE_TWO_PLAYER_COOP_ENDLESS) && mode != GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD)
        return false;
    return ApplyRuntimeSnapshot(app, data, mode, false);
}
} // namespace netplay

bool netplay::IsSerializingSpectatorSnapshot() noexcept {
    return gCapturingSpectatorSnapshot || gApplyingSpectatorSnapshot;
}
bool netplay::IsSendingSpectatorSnapshot() noexcept {
    return gSpectatorSnapshotRequested || !gSpectatorSnapshotOut.empty();
}

void netplay::ResetSpectatorSnapshot() noexcept {
    gSpectatorSnapshotRequested = false;
    gReceivingSpectatorSnapshot = false;
    gSpectatorDraftProgressPending = false;
    gSpectatorSourceBoard = nullptr;
    gSpectatorSourceSetup = nullptr;
    gSpectatorSourcePlant = gSpectatorSourceZombie = nullptr;
    gSpectatorSnapshotOut.clear();
    gSpectatorSnapshotIn.clear();
    gSpectatorCatchup.clear();
    gSpectatorCatchupReady.clear();
    gSpectatorSnapshotOffset = 0;
}

std::vector<std::byte> netplay::TakeSpectatorCatchupEvents() {
    return std::exchange(gSpectatorCatchupReady, {});
}

void netplay::UpdateSpectatorSnapshot(LawnApp *app) {
    if (reconnect::IsFrozen()) {
        // A room may wait indefinitely for its guest. This is not a stalled
        // spectator transfer; retain both the snapshot and its catchup backlog.
        if (gReceivingSpectatorSnapshot)
            gSpectatorSnapshotProgress = std::chrono::steady_clock::now();
        return;
    }
    if (gReceivingSpectatorSnapshot && std::chrono::steady_clock::now() - gSpectatorSnapshotProgress > std::chrono::seconds(60)) {
        FailSpectatorSnapshot();
        return;
    }
    if (!IsRemoteServer())
        return;
    // Client input can enqueue authoritative events earlier in this frame.
    // Flush them before the snapshot boundary instead of indefinitely starving
    // the transfer while either player continuously moves their cursor.
    if ((gSpectatorSnapshotRequested || !gSpectatorSnapshotOut.empty()) && HasPendingSendData())
        FlushSendBuffer(gTcpClientSocket);
    if (HasPendingSendData())
        return;
    if (!gSpectatorSnapshotOut.empty()
        && (app->mBoard != gSpectatorSourceBoard || app->mGameScene != gSpectatorSourceScene || app->mVSSetupMenu != gSpectatorSourceSetup || app->mSeedChooserScreen != gSpectatorSourcePlant
            || app->mZombieChooserScreen != gSpectatorSourceZombie)) {
        gSpectatorSnapshotOut.clear();
        gSpectatorSnapshotRequested = true;
    }
    if (gSpectatorSnapshotOut.empty() && gSpectatorSnapshotRequested) {
        if (app->mGameScene == SCENE_LEVEL_INTRO && app->mBoard
            && (!app->mBoard->mZombieWavesReady || app->mBoard->mCutScene->mPreUpdatingBoard || (app->mVSSetupMenu && int(app->mVSSetupMenu->mState) < 0))) {
            // Retry after level construction/preloading completes.
            return;
        }
        gSpectatorSnapshotRequested = false;
        if (app->mGameMode != GameMode::GAMEMODE_MP_VS || (app->mGameScene != SCENE_PLAYING && app->mGameScene != SCENE_LEVEL_INTRO) || !app->mBoard || app->mVSResultsMenu
            || IsSynchronizingSaveGame()) {
            BaseEvent abort{EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT};
            PutEvent(abort);
            return;
        }
        SaveGameContext context{};
        auto scene = GetSpectatorScene(app);
        gSpectatorHistoryEpoch = scene.historyEpoch;
        scene.Sync(context);
        SaveFileHeader header{SAVE_FILE_MAGIC_NUMBER, SAVE_FILE_VERSION, SAVE_FILE_DATE};
        context.SyncBytes(&header, sizeof(header));
        {
            struct Capturing {
                Capturing() {
                    gCapturingSpectatorSnapshot = true;
                }
                ~Capturing() {
                    gCapturingSpectatorSnapshot = false;
                }
            } capturing;
            SyncBoard(&context, app->mBoard);
            SyncSpectatorExtras(context, app->mBoard);
            if (scene.scene == SCENE_LEVEL_INTRO)
                SyncSpectatorIntro(context, app);
        }
        if (context.mFailed || context.mBuffer.mData.size() > kMaxOnlineSaveSize) {
            BaseEvent abort{EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT};
            PutEvent(abort);
            return;
        }
        gSpectatorSnapshotOut = std::move(context.mBuffer.mData);
        gSpectatorSnapshotOffset = 0;
        gSpectatorSourceBoard = app->mBoard;
        gSpectatorSourceSetup = app->mVSSetupMenu;
        gSpectatorSourcePlant = app->mSeedChooserScreen;
        gSpectatorSourceZombie = app->mZombieChooserScreen;
        gSpectatorSourceScene = app->mGameScene;
        U16UNI32UNI32_Event begin{};
        begin.type = EVENT_SERVER_SPECTATE_SNAPSHOT_BEGIN;
        begin.data1 = kSpectatorSnapshotVersion;
        begin.data2.u32 = static_cast<uint32_t>(gSpectatorSnapshotOut.size());
        begin.data3.u32 = SaveDataHash(gSpectatorSnapshotOut);
        PutEvent(begin);
        LOG_INFO("[NETPLAY] sending live spectator snapshot: {} bytes", gSpectatorSnapshotOut.size());
    }
    if (gSpectatorSnapshotOut.empty())
        return;
    const auto end = std::min(gSpectatorSnapshotOffset + 16 * 1024, gSpectatorSnapshotOut.size());
    while (gSpectatorSnapshotOffset < end) {
        U16U8x240_Event chunk{};
        chunk.type = EVENT_SERVER_SPECTATE_SNAPSHOT_CHUNK;
        chunk.count = static_cast<uint16_t>(std::min(sizeof(chunk.data), gSpectatorSnapshotOut.size() - gSpectatorSnapshotOffset));
        std::memcpy(chunk.data, gSpectatorSnapshotOut.data() + gSpectatorSnapshotOffset, chunk.count);
        PutEvent(chunk);
        gSpectatorSnapshotOffset += chunk.count;
    }
    if (gSpectatorSnapshotOffset == gSpectatorSnapshotOut.size()) {
        // Timers may have advanced while sending chunks. This final marker is
        // applied AFTER the viewer replays any intervening picks/bans.
        U16UNI32UNI32_Event done{};
        done.type = EVENT_SERVER_SPECTATE_SNAPSHOT_END;
        if (auto *chooser = app->mSeedChooserScreen) {
            done.data1 |= uint16_t(chooser->mTimedDraftWasActive);
            done.data2.i32 = chooser->mTimedDraftTicksRemaining;
        }
        if (auto *chooser = app->mZombieChooserScreen) {
            done.data1 |= uint16_t(chooser->mTimedDraftWasActive) << 1;
            done.data3.i32 = chooser->mTimedDraftTicksRemaining;
        }
        PutEvent(done);
        gSpectatorSnapshotOut.clear();
        replay::RequestHistoryTransfer(gSpectatorHistoryEpoch);
    }
}

bool netplay::HandleSpectatorSnapshotEvent(LawnApp *app, const BaseEvent *event, bool fromHost) {
    if (event->type == EVENT_RELAY_SPECTATE_SNAPSHOT_REQUEST) {
        if (!fromHost && event->size == sizeof(BaseEvent) && IsRemoteServer() && gIsServerModeNetplay && gServerModeTransport == ServerModeTransport::RELAY)
            gSpectatorSnapshotRequested = true;
        return true;
    }
    const bool snapshotEvent = event->type >= EVENT_SERVER_SPECTATE_SNAPSHOT_BEGIN && event->type <= EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT;
    if (!fromHost || !gIsServerModeSpectator || gIsReplayMode)
        return snapshotEvent;
    if (gSpectatorDraftProgressPending && event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_END) {
        gSpectatorDraftProgressPending = false;
        if (event->size != sizeof(U16UNI32UNI32_Event)) {
            FailSpectatorSnapshot();
            return true;
        }
        const auto *done = static_cast<const U16UNI32UNI32_Event *>(event);
        if (auto *chooser = app->mSeedChooserScreen) {
            chooser->mTimedDraftTicksRemaining = done->data2.i32;
            chooser->mTimedDraftWasActive = (done->data1 & 1) != 0;
        }
        if (auto *chooser = app->mZombieChooserScreen) {
            chooser->mTimedDraftTicksRemaining = done->data3.i32;
            chooser->mTimedDraftWasActive = (done->data1 & 2) != 0;
        }
        return true;
    }
    auto *lobby = NetplayLobbyWidget::GetInstance();
    if (!gReceivingSpectatorSnapshot && (!lobby || !lobby->ServerIsWaitingReservedSpectate()))
        return snapshotEvent;
    if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_BEGIN) {
        if (event->size != sizeof(U16UNI32UNI32_Event)) {
            FailSpectatorSnapshot();
            return true;
        }
        const auto *begin = static_cast<const U16UNI32UNI32_Event *>(event);
        if (begin->data1 != kSpectatorSnapshotVersion || begin->data2.u32 < sizeof(SaveFileHeader) || begin->data2.u32 > kMaxOnlineSaveSize) {
            FailSpectatorSnapshot();
            return true;
        }
        gSpectatorSnapshotIn.clear();
        gSpectatorCatchup.clear();
        gSpectatorSnapshotSize = begin->data2.u32;
        gSpectatorSnapshotHash = begin->data3.u32;
        gReceivingSpectatorSnapshot = true;
        gSpectatorSnapshotProgress = std::chrono::steady_clock::now();
        return true;
    }
    if (!gReceivingSpectatorSnapshot) {
        if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT && lobby)
            lobby->mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_WAIT_NEXT]");
        return snapshotEvent;
    }
    if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_CHUNK) {
        if (event->size != sizeof(U16U8x240_Event)) {
            FailSpectatorSnapshot();
            return true;
        }
        const auto *chunk = static_cast<const U16U8x240_Event *>(event);
        if (!chunk->count || chunk->count > sizeof(chunk->data) || gSpectatorSnapshotIn.size() + chunk->count > gSpectatorSnapshotSize) {
            FailSpectatorSnapshot();
            return true;
        }
        gSpectatorSnapshotIn.insert(gSpectatorSnapshotIn.end(), chunk->data, chunk->data + chunk->count);
        gSpectatorSnapshotProgress = std::chrono::steady_clock::now();
    } else if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_END) {
        if (event->size != sizeof(U16UNI32UNI32_Event) || gSpectatorSnapshotIn.size() != gSpectatorSnapshotSize || SaveDataHash(gSpectatorSnapshotIn) != gSpectatorSnapshotHash
            || !ApplyRuntimeSnapshot(app, gSpectatorSnapshotIn, GAMEMODE_MP_VS, true)) {
            FailSpectatorSnapshot();
            return true;
        }
        gReceivingSpectatorSnapshot = false;
        gSpectatorCatchupReady = std::move(gSpectatorCatchup);
        const auto *bytes = reinterpret_cast<const std::byte *>(event);
        gSpectatorCatchupReady.insert(gSpectatorCatchupReady.end(), bytes, bytes + event->size);
        gSpectatorDraftProgressPending = true;
    } else if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT) {
        gReceivingSpectatorSnapshot = false;
        gSpectatorSnapshotIn.clear();
        gSpectatorCatchup.clear(); // Lobby can still align with the next round.
        if (lobby)
            lobby->mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_WAIT_NEXT]");
    } else {
        if (gSpectatorCatchup.size() + event->size > kMaxSpectatorCatchup) {
            FailSpectatorSnapshot();
            return true;
        }
        const auto *bytes = reinterpret_cast<const std::byte *>(event);
        gSpectatorCatchup.insert(gSpectatorCatchup.end(), bytes, bytes + event->size);
    }
    return true;
}

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
        // 客户端不读取自己的本地存档，等待主机决定是否读档。
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
    app->mNeedLoadGame = false; // 此握手流程取代本地继续游戏的对话框。
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
            // 关闭停滞的连接，防止迟到的存档分块进入新Board。
            app->ClearSecondPlayer();
            AbortOnlineSave(app, false);
        }
        return;
    }
    if (gOnlineLoadState != OnlineLoadState::Sending || HasPendingSendData()) {
        return;
    }
    // 限制排队数据量，避免存档传输塞满中转服务器的待发送队列。
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
                // 在处理本批 TCP 数据中后续的游戏事件之前，先应用收到的存档。
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
        // 接收的 Board 不能覆盖客户端自己的本地结盟存档。
        theBoard->mApp->mNeedGoBackToMain = false;
        return true;
    }
    if (disableSaveUserdata) {
        theBoard->mApp->mNeedGoBackToMain = false; // 用于从暂停菜单返回主界面
        return true;
    }

    // 结盟模式存档，将SeedBank2的4个种子放到SeedBank1里面。因为原版存档逻辑难以改动，只好出此下策，凑合着存吧。
    if (theBoard->mApp->IsCoopMode()) {
        if (theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS
            || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD) {
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
        if (theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS
            || theBoard->mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD) {
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

void SaveGameContext::SyncParticleDef(TodParticleDefinition *&theDefinition) {
    // 原版把粒子定义数量写死为 108；扩展后按实际加载数量同步，沿用原存档的索引格式。
    int aParticleIndex = -1;
    if (mReading) {
        SyncInt(aParticleIndex);
        theDefinition = nullptr;
        if (mFailed || aParticleIndex == -1) {
            return;
        }
        if (gParticleDefArray != nullptr && aParticleIndex >= 0 && aParticleIndex < gParticleDefCount) {
            theDefinition = &gParticleDefArray[aParticleIndex];
        } else {
            mFailed = true;
        }
    } else {
        if (theDefinition != nullptr && gParticleDefArray != nullptr) {
            for (int i = 0; i < gParticleDefCount; ++i) {
                if (theDefinition == &gParticleDefArray[i]) {
                    aParticleIndex = i;
                    break;
                }
            }
        }
        SyncInt(aParticleIndex);
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
