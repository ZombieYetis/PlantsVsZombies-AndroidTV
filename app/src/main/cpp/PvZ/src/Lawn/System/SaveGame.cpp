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
#include "Homura/HookUtils.h"
#include "Homura/Logger.h"
#include "PvZ/Android/Native/BridgeApp.h"
#include "PvZ/Android/Native/NativeApp.h"
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
#include "PvZ/Lawn/System/SnapshotAnimationWire.h"
#include "PvZ/Lawn/System/SnapshotReanimation.h"
#include "PvZ/Lawn/System/SpectatorWidgetSync.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
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
#include "PvZ/TodLib/Effect/EffectSystem.h"
#include "PvZ/TodLib/Effect/Reanimator.h"
#include "PvZ/TodLib/Effect/TodParticle.h"

#include <dlfcn.h>
#include <sys/socket.h>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <memory>
#include <type_traits>
#include <utility>

namespace {
const char *gSpectatorSnapshotFailure = nullptr;
const char *gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_LOAD_FAILED]";
// Diagnostic state is external: never change native layouts or the save format.
struct SnapshotReadBlock {
    const char *operation = "none";
    const char *stage = "none";
    unsigned index = 0;
    int bitOffset = 0;
    int expected = 0;
    int actual = -1;
    int available = 0;
    uint32_t word = 0;
    bool hasWord = false;
    uintptr_t caller = 0;
    int reanimationType = -1;
    int trackCount = -1;
};

struct SnapshotReadDiagnostics {
    SaveGameContext *context;
    bool allowTrackRemap = false;
    const netplay::snapshot::AnimationLayouts *sourceLayouts = nullptr;
    netplay::snapshot::AnimationLayouts localLayouts;
    netplay::snapshot::CosmeticSnapshotState cosmetics;
    const char *stage = "scene";
    bool reported = false;
    unsigned blocks = 0;
    const SnapshotReadBlock *bytesBlock = nullptr;
    SnapshotReadBlock last;
    int reanimationType = -1;
    int trackCount = -1;
};
thread_local SnapshotReadDiagnostics *gSnapshotReadDiagnostics = nullptr;

struct SnapshotReadScope {
    SnapshotReadDiagnostics state;
    SnapshotReadDiagnostics *previous;
    explicit SnapshotReadScope(SaveGameContext &context, bool spectator, const netplay::snapshot::AnimationLayouts *layouts)
        : state{.context = &context, .allowTrackRemap = spectator, .sourceLayouts = layouts}
        , previous(gSnapshotReadDiagnostics) {
        gSnapshotReadDiagnostics = &state;
    }
    ~SnapshotReadScope() {
        gSnapshotReadDiagnostics = previous;
    }
};

struct SnapshotReadStage {
    SnapshotReadDiagnostics *diagnostics;
    const char *previous;
    int previousReanimationType;
    int previousTrackCount;
    SnapshotReadStage(SaveGameContext &context, const char *stage)
        : diagnostics(gSnapshotReadDiagnostics && gSnapshotReadDiagnostics->context == &context ? gSnapshotReadDiagnostics : nullptr)
        , previous(diagnostics ? diagnostics->stage : nullptr)
        , previousReanimationType(diagnostics ? diagnostics->reanimationType : -1)
        , previousTrackCount(diagnostics ? diagnostics->trackCount : -1) {
        if (diagnostics) {
            diagnostics->stage = stage;
            diagnostics->reanimationType = diagnostics->trackCount = -1;
        }
    }
    ~SnapshotReadStage() {
        if (diagnostics) {
            diagnostics->stage = previous;
            diagnostics->reanimationType = previousReanimationType;
            diagnostics->trackCount = previousTrackCount;
        }
    }
};

SnapshotReadBlock DescribeSnapshotRead(SnapshotReadDiagnostics &diagnostics, const char *operation, int expected, bool lengthPrefixed, void *caller) {
    const auto &buffer = diagnostics.context->mBuffer;
    SnapshotReadBlock block;
    block.operation = operation;
    block.stage = diagnostics.stage;
    block.index = ++diagnostics.blocks;
    block.bitOffset = buffer.mReadBitPos;
    block.expected = expected;
    block.caller = reinterpret_cast<uintptr_t>(caller) & ~uintptr_t{1};
    block.reanimationType = diagnostics.reanimationType;
    block.trackCount = diagnostics.trackCount;
    const auto validBits = std::min<int64_t>(buffer.mDataBitSize, static_cast<int64_t>(buffer.mData.size()) * 8);
    const auto remainingBits = std::max<int64_t>(0, validBits - std::max(0, block.bitOffset));
    block.available = static_cast<int>(remainingBits / 8);
    // Peek without moving the cursor or calling the native reader. Also handle
    // non-byte-aligned positions, so diagnostics cannot introduce an over-read.
    block.hasWord = block.bitOffset >= 0 && remainingBits >= 32;
    if (block.hasWord) {
        for (unsigned bit = 0; bit < 32; ++bit) {
            const auto pos = static_cast<size_t>(block.bitOffset) + bit;
            block.word |= uint32_t((buffer.mData[pos / 8] >> (pos % 8)) & 1) << bit;
        }
    }
    block.actual = lengthPrefixed ? (block.hasWord ? static_cast<int32_t>(block.word) : -1) : std::min(expected, block.available);
    if (lengthPrefixed)
        block.available = std::max(0, block.available - 4);
    return block;
}

void PersistSnapshotReadFailure(const std::string &report) {
    auto *bridge = Native::BridgeApp::getSingleton();
    if (!bridge || !bridge->mNativeApp)
        return;
    auto *env = bridge->getJNIEnv();
    auto activity = bridge->mNativeApp->getActivity();
    // Do not disturb an exception belonging to the caller.
    if (!env || !activity || env->ExceptionCheck())
        return;
    if (env->PushLocalFrame(3) < 0) {
        env->ExceptionClear();
        LOG_ERROR("[SNAPSHOT_LOCAL] cannot allocate JNI references");
        return;
    }
    jboolean saved = JNI_FALSE;
    do {
        auto activityClass = env->GetObjectClass(activity);
        if (!activityClass || env->ExceptionCheck())
            break;
        auto method = env->GetMethodID(activityClass, "saveSnapshotFailure", "(Ljava/lang/String;)Z");
        if (!method || env->ExceptionCheck())
            break;
        auto text = env->NewStringUTF(report.c_str());
        if (!text || env->ExceptionCheck())
            break;
        saved = env->CallBooleanMethod(activity, method, text);
    } while (false);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        saved = JNI_FALSE;
    }
    env->PopLocalFrame(nullptr);
    if (!saved)
        LOG_ERROR("[SNAPSHOT_LOCAL] could not persist first failure");
}

void ReportSnapshotReadFailure(SaveGameContext *context, const char *reason) {
    auto *diagnostics = gSnapshotReadDiagnostics;
    if (!diagnostics || diagnostics->context != context || diagnostics->reported)
        return;
    diagnostics->reported = true;
    const auto &block = diagnostics->last;
    if (std::strcmp(reason, "block_length_mismatch") == 0 && std::strcmp(block.stage, "reanimation") == 0)
        gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_RESOURCE_MISMATCH]";
    const auto detail = std::format("[SNAPSHOT] first_failure reason={} stage={} block={} op={} offset_bytes={} offset_bits={} expected_bytes={} actual_bytes={} available_bytes={} prefix_present={} "
                                    "wire_word=0x{:08X} read_after_bits={} total_bits={} reanim_type={} tracks={}",
                                    reason,
                                    block.stage,
                                    block.index,
                                    block.operation,
                                    block.bitOffset / 8,
                                    block.bitOffset,
                                    block.expected,
                                    block.actual,
                                    block.available,
                                    block.hasWord,
                                    block.word,
                                    context->mBuffer.mReadBitPos,
                                    context->mBuffer.mDataBitSize,
                                    block.reanimationType,
                                    block.trackCount);
    Dl_info module{};
    dladdr(reinterpret_cast<void *>(block.caller), &module);
    const auto caller = std::format("[SNAPSHOT] first_failure caller=0x{:X} module={} module_offset=0x{:X} symbol={}",
                                    block.caller,
                                    module.dli_fname ? module.dli_fname : "?",
                                    module.dli_fbase ? block.caller - reinterpret_cast<uintptr_t>(module.dli_fbase) : 0,
                                    module.dli_sname ? module.dli_sname : "?");
    // Persist independently of logcat filtering; only the first failure per load reaches here.
    PersistSnapshotReadFailure(std::format("[SNAPSHOT] base_version={} netplay_version={}\n{}\n{}", PVZ_VERSION, NETPLAY_VERSION, detail, caller));
    LOG_ERROR("{}", detail);
    LOG_ERROR("{}", caller);
}

// Preserve the native return register as well as all reader side effects.
int (*gOriginalSnapshotSyncInt)(SaveGameContext *, int &);
int (*gOriginalSnapshotSyncBytes)(SaveGameContext *, void *, int);

[[gnu::noinline]] int DiagnoseSnapshotSyncInt(SaveGameContext *context, int &value) {
    auto *diagnostics = gSnapshotReadDiagnostics;
    if (diagnostics && diagnostics->context == context && context->mReading && context->mFailed && !diagnostics->bytesBlock)
        ReportSnapshotReadFailure(context, "native_validation_after_previous_block");
    if (!diagnostics || diagnostics->context != context || !context->mReading || context->mFailed || diagnostics->reported || diagnostics->bytesBlock)
        return gOriginalSnapshotSyncInt(context, value);
    diagnostics->last = DescribeSnapshotRead(*diagnostics, "SyncInt", sizeof(value), false, __builtin_return_address(0));
    const int result = gOriginalSnapshotSyncInt(context, value);
    if (context->mFailed)
        ReportSnapshotReadFailure(context, "integer_read");
    return result;
}

[[gnu::noinline]] int DiagnoseSnapshotSyncBytes(SaveGameContext *context, void *destination, int size) {
    auto *diagnostics = gSnapshotReadDiagnostics;
    if (diagnostics && diagnostics->context == context && context->mReading && context->mFailed)
        ReportSnapshotReadFailure(context, "native_validation_after_previous_block");
    if (!diagnostics || diagnostics->context != context || !context->mReading || context->mFailed || diagnostics->reported)
        return gOriginalSnapshotSyncBytes(context, destination, size);
    diagnostics->last = DescribeSnapshotRead(*diagnostics, "SyncBytes", size, true, __builtin_return_address(0));
    // SyncBytes internally calls SyncInt for its length prefix. Report that as
    // part of this block rather than losing the payload's expected length.
    diagnostics->bytesBlock = &diagnostics->last;
    const int result = gOriginalSnapshotSyncBytes(context, destination, size);
    diagnostics->bytesBlock = nullptr;
    if (context->mFailed)
        ReportSnapshotReadFailure(context, !diagnostics->last.hasWord ? "length_prefix_truncated" : diagnostics->last.actual != size ? "block_length_mismatch" : "block_payload_truncated");
    return result;
}

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
std::vector<unsigned char> gSpectatorLayoutsOut;
std::size_t gSpectatorLayoutsOffset = 0;
uint32_t gSpectatorLayoutsHash = 0;
netplay::snapshot::LayoutTailReceiver gSpectatorLayoutReceiver;
netplay::snapshot::AnimationLayouts gSpectatorLayoutsIn;
thread_local netplay::snapshot::AnimationLayouts *gSnapshotLayoutCapture = nullptr;
thread_local bool *gSnapshotLayoutCaptureValid = nullptr;
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
    SnapshotReadStage diagnosticStage(context, chooser.mIsZombieChooser ? "zombie_chooser" : "plant_chooser");
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
    SnapshotReadStage diagnosticStage(context, "intro");
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
        SnapshotReadStage menuDiagnosticStage(context, "vs_setup");
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
    SnapshotReadStage diagnosticStage(context, "extras");
    // Native SyncBoard only includes player 0. Use registered blocks, never pointers/vtables.
    const auto syncObject = [&](SyncObject *object, const char *name) {
        SnapshotReadStage objectDiagnosticStage(context, name);
        for (auto &block : *object->mSyncBlocks) {
            context.SyncBytes(block.mAddress, static_cast<int>(block.mSize));
        }
    };
    syncObject(board->mSeedBank[1], "player1.seed_bank");
    syncObject(board->mGamepadControls[1], "player1.gamepad");
    syncObject(board->mCursorObject[1], "player1.cursor");
    syncObject(board->mCursorPreview[1], "player1.cursor_preview");
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

void FailSpectatorSnapshot(const char *reason) {
    LOG_ERROR("[NETPLAY] live spectator snapshot failed; closing viewer connection reason={} received_bytes={} expected_bytes={} expected_hash=0x{:08X} catchup_bytes={}",
              reason,
              gSpectatorSnapshotIn.size(),
              gSpectatorSnapshotSize,
              gSpectatorSnapshotHash,
              gSpectatorCatchup.size());
    // Defer widget/socket cleanup until the packet dispatcher has unwound.
    // The update loop consumes this before showing a dialog or drawing Board.
    if (!gSpectatorSnapshotFailure) {
        gSpectatorSnapshotFailure = std::strcmp(reason, "apply_runtime_snapshot") == 0 ? gSnapshotReadFailureMessage
            : std::strcmp(reason, "receive_timeout") == 0                              ? "[SPECTATE_SNAPSHOT_TIMEOUT]"
                                                                                       : "[SPECTATE_SNAPSHOT_TRANSFER_FAILED]";
    }
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
    SnapshotReadScope diagnostics(context, spectator, spectator && gSpectatorLayoutReceiver.present ? &gSpectatorLayoutsIn : nullptr);
    gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_LOAD_FAILED]";
    LOG_INFO("[SNAPSHOT] apply base_version={} netplay_version={} bytes={} mode={} spectator={} board_before={}",
             PVZ_VERSION,
             NETPLAY_VERSION,
             context.mBuffer.mData.size(),
             static_cast<int>(mode),
             spectator,
             static_cast<void *>(app->mBoard));
    SpectatorScene scene;
    scene.Sync(context);
    if (context.mFailed || (spectator && !scene.historyEpoch) || (scene.scene != SCENE_LEVEL_INTRO && scene.scene != SCENE_PLAYING)
        || (scene.scene == SCENE_PLAYING && (scene.setup || scene.plantChooser || scene.zombieChooser)) || (mode == GAMEMODE_MP_VS && (scene.plantChooser || scene.zombieChooser) && !scene.setup)) {
        ReportSnapshotReadFailure(&context, "scene_validation");
        LOG_ERROR("[SNAPSHOT] scene={} background={} epoch={} setup={} plant_chooser={} zombie_chooser={}",
                  static_cast<int>(scene.scene),
                  static_cast<int>(scene.background),
                  scene.historyEpoch,
                  scene.setup,
                  scene.plantChooser,
                  scene.zombieChooser);
        return false;
    }

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
    if (!LawnLoadGame_Original(app->mBoard, &context)) {
        LOG_ERROR("[SNAPSHOT] apply failed stage=board board={} plant_chooser={} zombie_chooser={} read_bits={} failed={}",
                  static_cast<void *>(app->mBoard),
                  static_cast<void *>(app->mSeedChooserScreen),
                  static_cast<void *>(app->mZombieChooserScreen),
                  context.mBuffer.mReadBitPos,
                  context.mFailed);
        return false;
    }
    app->mGameScene = scene.scene; // LawnLoadGame normally resumes straight into combat.
    SyncSpectatorExtras(context, app->mBoard);
    if (scene.scene == SCENE_LEVEL_INTRO)
        SyncSpectatorIntro(context, app);
    if (context.mFailed || context.mBuffer.mReadBitPos != context.mBuffer.mDataBitSize) {
        ReportSnapshotReadFailure(&context, context.mFailed ? "extras_or_intro" : "unconsumed_snapshot_bytes");
        LOG_ERROR("[SNAPSHOT] apply failed stage=end expected_bits={} consumed_bits={} board={}", context.mBuffer.mDataBitSize, context.mBuffer.mReadBitPos, static_cast<void *>(app->mBoard));
        return false;
    }
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

void netplay::RegisterSnapshotDiagnostics() {
    homura::HookFunc(SaveGameContext_SyncIntAddr, &DiagnoseSnapshotSyncInt, &gOriginalSnapshotSyncInt);
    homura::HookFunc(SaveGameContext_SyncBytesAddr, &DiagnoseSnapshotSyncBytes, &gOriginalSnapshotSyncBytes);
}

bool netplay::HasSpectatorSnapshotFailure() noexcept {
    return gSpectatorSnapshotFailure != nullptr;
}

bool netplay::RecoverSpectatorSnapshotFailure(LawnApp *app) {
    if (!gSpectatorSnapshotFailure)
        return false;
    const char *message = std::exchange(gSpectatorSnapshotFailure, nullptr);

    // Disconnect only this viewer. Never enqueue a gameplay/abort event.
    auto *oldLobby = NetplayLobbyWidget::GetInstance();
    if (oldLobby && gTcpServerSocket >= 0 && oldLobby->mServerSock == gTcpServerSocket)
        oldLobby->mServerSock = -1; // ClearSecondPlayer owns this borrowed fd.
    app->ClearSecondPlayer();
    if (oldLobby)
        oldLobby->ServerDisconnect("spectator snapshot failed");
    ResetSpectatorSnapshot();

    // Loading failed before FixBoardAfterLoad. Do not run gameplay repair or
    // resume the partially loaded board. Destruction must not follow animation
    // attachment IDs from the failed snapshot, and delayed chooser deletion
    // must not act on an animation in the next scene.
    for (auto *chooser : {app->mSeedChooserScreen, app->mZombieChooserScreen}) {
        if (chooser)
            chooser->mReanimSeedChooser = REANIMATIONID_NULL;
    }
    Reanimation *animation = nullptr;
    while (app->mEffectSystem->mReanimationHolder->mReanimations.IterateNext(animation))
        animation->mDead = true;
    app->KillBoard(); // removes Board/widgets before the modal dialog pumps frames
    app->mNeedLoadGame = false;
    app->mBoardResult = BOARDRESULT_NONE;
    app->KillChallengeScreen();
    app->ShowChallengeScreen(CHALLENGE_PAGE_VS);
    auto *screen = app->mChallengeScreen;
    auto *lobby = new NetplayLobbyWidget(app, false);
    screen->mNetplayLobbyWidget = lobby;
    screen->AddWidget(lobby);
    lobby->SetMode(UIMode::MODE3_SERVER);
    lobby->mServerStatusText = TodStringTranslate(message);
    app->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[SPECTATE_SNAPSHOT_FAILED_TITLE]", message, "[DIALOG_BUTTON_OK]", "", 3);
    return true;
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
    gSpectatorLayoutsOut.clear();
    gSpectatorLayoutsOffset = 0;
    gSpectatorLayoutReceiver = {};
    gSpectatorLayoutsIn.clear();
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
        FailSpectatorSnapshot("receive_timeout");
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
        netplay::snapshot::AnimationLayouts layouts;
        bool layoutsValid = true;
        {
            struct Capturing {
                Capturing(netplay::snapshot::AnimationLayouts &layouts, bool &valid) {
                    gCapturingSpectatorSnapshot = true;
                    gSnapshotLayoutCapture = &layouts;
                    gSnapshotLayoutCaptureValid = &valid;
                }
                ~Capturing() {
                    gCapturingSpectatorSnapshot = false;
                    gSnapshotLayoutCapture = nullptr;
                    gSnapshotLayoutCaptureValid = nullptr;
                }
            } capturing(layouts, layoutsValid);
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
        gSpectatorLayoutsOut.clear();
        gSpectatorLayoutsOffset = 0;
        if (!layoutsValid || !netplay::snapshot::EncodeLayouts(layouts, begin.data3.u32, gSpectatorLayoutsOut)
            || (gSpectatorLayoutsOut.size() + netplay::snapshot::kLayoutTailCapacity - 1) / netplay::snapshot::kLayoutTailCapacity > gSpectatorSnapshotOut.size()) {
            // Metadata is optional. Never prevent the existing 3215 transfer
            // just because a local definition cannot be described safely.
            gSpectatorLayoutsOut.clear();
            LOG_ERROR("[SNAPSHOT] animation_layout_capture_unavailable; sending legacy snapshot");
        }
        gSpectatorLayoutsHash = netplay::snapshot::LayoutHash(gSpectatorLayoutsOut);
        PutEvent(begin);
        LOG_INFO("[NETPLAY] sending live spectator snapshot: {} bytes", gSpectatorSnapshotOut.size());
    }
    if (gSpectatorSnapshotOut.empty())
        return;
    // Bound actual network bytes per frame, including optional layout tails.
    for (size_t sent = 0; gSpectatorSnapshotOffset < gSpectatorSnapshotOut.size() && sent + sizeof(U16U8x240_Event) <= 16 * 1024; sent += sizeof(U16U8x240_Event)) {
        U16U8x240_Event chunk{};
        chunk.type = EVENT_SERVER_SPECTATE_SNAPSHOT_CHUNK;
        chunk.count =
            netplay::snapshot::FillSnapshotChunk(chunk.data, std::span(gSpectatorSnapshotOut).subspan(gSpectatorSnapshotOffset), gSpectatorLayoutsOut, gSpectatorLayoutsHash, gSpectatorLayoutsOffset);
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
        gSpectatorLayoutsOut.clear();
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
            LOG_ERROR("[SNAPSHOT] draft_end expected_bytes={} actual_bytes={}", sizeof(U16UNI32UNI32_Event), event->size);
            FailSpectatorSnapshot("draft_end_size");
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
            LOG_ERROR("[SNAPSHOT] begin expected_bytes={} actual_bytes={}", sizeof(U16UNI32UNI32_Event), event->size);
            FailSpectatorSnapshot("begin_size");
            return true;
        }
        const auto *begin = static_cast<const U16UNI32UNI32_Event *>(event);
        if (begin->data1 != kSpectatorSnapshotVersion || begin->data2.u32 < sizeof(SaveFileHeader) || begin->data2.u32 > kMaxOnlineSaveSize) {
            LOG_ERROR("[SNAPSHOT] begin expected_version={} actual_version={} bytes={} min_bytes={} max_bytes={}",
                      kSpectatorSnapshotVersion,
                      begin->data1,
                      begin->data2.u32,
                      sizeof(SaveFileHeader),
                      kMaxOnlineSaveSize);
            FailSpectatorSnapshot("begin_version_or_length");
            return true;
        }
        gSpectatorSnapshotIn.clear();
        gSpectatorCatchup.clear();
        gSpectatorLayoutReceiver = {};
        gSpectatorLayoutsIn.clear();
        gSpectatorSnapshotSize = begin->data2.u32;
        gSpectatorSnapshotHash = begin->data3.u32;
        gReceivingSpectatorSnapshot = true;
        gSpectatorSnapshotProgress = std::chrono::steady_clock::now();
        LOG_INFO("[SNAPSHOT] receive begin version={} expected_bytes={} expected_hash=0x{:08X}", begin->data1, gSpectatorSnapshotSize, gSpectatorSnapshotHash);
        return true;
    }
    if (!gReceivingSpectatorSnapshot) {
        if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT && lobby)
            lobby->mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_WAIT_NEXT]");
        return snapshotEvent;
    }
    if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_CHUNK) {
        if (event->size != sizeof(U16U8x240_Event)) {
            LOG_ERROR("[SNAPSHOT] chunk expected_bytes={} actual_bytes={}", sizeof(U16U8x240_Event), event->size);
            FailSpectatorSnapshot("chunk_size");
            return true;
        }
        const auto *chunk = static_cast<const U16U8x240_Event *>(event);
        if (!chunk->count || chunk->count > sizeof(chunk->data) || gSpectatorSnapshotIn.size() + chunk->count > gSpectatorSnapshotSize) {
            LOG_ERROR("[SNAPSHOT] chunk offset_bytes={} payload_bytes={} max_payload_bytes={}", gSpectatorSnapshotIn.size(), chunk->count, sizeof(chunk->data));
            FailSpectatorSnapshot("chunk_count_or_overflow");
            return true;
        }
        if (!gSpectatorLayoutReceiver.Add(chunk->data, chunk->count)) {
            FailSpectatorSnapshot("animation_layout_fragment");
            return true;
        }
        gSpectatorSnapshotIn.insert(gSpectatorSnapshotIn.end(), chunk->data, chunk->data + chunk->count);
        gSpectatorSnapshotProgress = std::chrono::steady_clock::now();
    } else if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_END) {
        // Same checks and short-circuit order; distinguish transport rejection
        // from a failure after ApplyRuntimeSnapshot has started changing Board.
        if (event->size != sizeof(U16UNI32UNI32_Event)) {
            LOG_ERROR("[SNAPSHOT] end expected_bytes={} actual_bytes={}", sizeof(U16UNI32UNI32_Event), event->size);
            FailSpectatorSnapshot("end_size");
            return true;
        }
        if (gSpectatorSnapshotIn.size() != gSpectatorSnapshotSize) {
            FailSpectatorSnapshot("payload_length");
            return true;
        }
        const uint32_t actualHash = SaveDataHash(gSpectatorSnapshotIn);
        if (actualHash != gSpectatorSnapshotHash) {
            LOG_ERROR("[SNAPSHOT] hash expected=0x{:08X} actual=0x{:08X}", gSpectatorSnapshotHash, actualHash);
            FailSpectatorSnapshot("payload_hash");
            return true;
        }
        if (!gSpectatorLayoutReceiver.Complete()
            || (gSpectatorLayoutReceiver.present
                && !netplay::snapshot::DecodeLayouts(gSpectatorLayoutReceiver.bytes, gSpectatorSnapshotHash, ReanimationType::EXTENDED_NUM_REANIMS, gSpectatorLayoutsIn))) {
            FailSpectatorSnapshot("animation_layout_payload");
            return true;
        }
        if (!ApplyRuntimeSnapshot(app, gSpectatorSnapshotIn, GAMEMODE_MP_VS, true)) {
            FailSpectatorSnapshot("apply_runtime_snapshot");
            return true;
        }
        gReceivingSpectatorSnapshot = false;
        gSpectatorLayoutReceiver = {};
        gSpectatorLayoutsIn.clear();
        gSpectatorCatchupReady = std::move(gSpectatorCatchup);
        const auto *bytes = reinterpret_cast<const std::byte *>(event);
        gSpectatorCatchupReady.insert(gSpectatorCatchupReady.end(), bytes, bytes + event->size);
        gSpectatorDraftProgressPending = true;
    } else if (event->type == EVENT_SERVER_SPECTATE_SNAPSHOT_ABORT) {
        gReceivingSpectatorSnapshot = false;
        gSpectatorSnapshotIn.clear();
        gSpectatorLayoutReceiver = {};
        gSpectatorLayoutsIn.clear();
        gSpectatorCatchup.clear(); // Lobby can still align with the next round.
        if (lobby)
            lobby->mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_WAIT_NEXT]");
    } else {
        if (gSpectatorCatchup.size() + event->size > kMaxSpectatorCatchup) {
            LOG_ERROR("[SNAPSHOT] catchup offset_bytes={} event_bytes={} max_bytes={}", gSpectatorCatchup.size(), event->size, kMaxSpectatorCatchup);
            FailSpectatorSnapshot("catchup_overflow");
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
    SnapshotReadStage headerDiagnosticStage(*theContext, "save_header");
    SaveFileHeader aHeader{};
    theContext->SyncBytes(&aHeader, sizeof(aHeader));

    // 检查存档魔数和版本范围。
    if (aHeader.mMagicNumber != SAVE_FILE_MAGIC_NUMBER || aHeader.mBuildVersion > SAVE_FILE_VERSION) {
        ReportSnapshotReadFailure(theContext, "save_header_magic_or_version");
        if (gSnapshotReadDiagnostics && gSnapshotReadDiagnostics->context == theContext)
            LOG_ERROR("[SNAPSHOT] save_header expected_magic=0x{:08X} actual_magic=0x{:08X} expected_version={} actual_version={}",
                      SAVE_FILE_MAGIC_NUMBER,
                      aHeader.mMagicNumber,
                      SAVE_FILE_VERSION,
                      aHeader.mBuildVersion);
        if (!netplay::IsApplyingOnlineSaveGame())
            gLawnApp->HandleCorruptedGameFile();
        return false;
    }
    // 魔数正确，但不是当前支持的版本。
    if (aHeader.mBuildVersion != SAVE_FILE_VERSION) {
        ReportSnapshotReadFailure(theContext, "save_header_old_version");
        if (gSnapshotReadDiagnostics && gSnapshotReadDiagnostics->context == theContext)
            LOG_ERROR("[SNAPSHOT] save_header expected_version={} actual_version={}", SAVE_FILE_VERSION, aHeader.mBuildVersion);
        if (!netplay::IsApplyingOnlineSaveGame())
            gLawnApp->HandleOldGameFile();
        return false;
    }
    {
        SnapshotReadStage boardDiagnosticStage(*theContext, "native.SyncBoard");
        SyncBoard(theContext, theBoard);
        if (theContext->mFailed)
            ReportSnapshotReadFailure(theContext, "SyncBoard_validation");
    }
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

    if (gSnapshotReadDiagnostics && gSnapshotReadDiagnostics->context == theContext && gSnapshotReadDiagnostics->allowTrackRemap)
        netplay::snapshot::FinishSnapshotCosmetics(*theBoard, gSnapshotReadDiagnostics->cosmetics);
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
            ReportSnapshotReadFailure(this, "reanimation_definition_index");
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
            ReportSnapshotReadFailure(this, "particle_definition_index");
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

namespace {
const char *RestoreLocalReanimation(
    Board &board, Reanimation &reanimation, SaveGameContext &context, int sourceTracks, const netplay::snapshot::AnimationLayout &localLayout, std::span<const uint32_t> discardedAttachments) {
    using namespace netplay::snapshot;
    AnimationPlayback playback{reanimation.mFrameStart, reanimation.mFrameCount, reanimation.mFrameBasePose, reanimation.mAnimRate, reanimation.mAnimTime, reanimation.mLastFrameTime};
    const auto actions = LocalAnimationActions(board, reanimation);
    if (!RebuildLocalPlayback(localLayout, playback, actions))
        return "reanimation_local_playback_invalid";
    std::vector<ReanimatorTrackInstance> source(sourceTracks);
    if (!ReadDiscardedTracks(context, std::span(source)))
        return "reanimation_local_tracks_truncated";
    for (auto &track : source)
        ClearDiscardedAttachment(track, discardedAttachments);
    for (auto &track : std::span(reanimation.mTrackInstances, reanimation.mDefinition->mTrackCount)) {
        // Match native constructor defaults; no foreign image/blend pointers or
        // source track indices survive the local reconstruction.
        std::memset(static_cast<void *>(&track), 0, sizeof(track));
        track.mTrackColor = Sexy::Color(255, 255, 255, 255);
        track.mTruncateDisappearingFrames = true;
        track.mBlendTransform.mText = "";
    }
    if (!RestoreLocalAnimationAttachments(board, reanimation, source))
        return "reanimation_local_attachments_unresolved";
    RestoreLocalAnimationLayers(board, reanimation);
    LOG_INFO("[SNAPSHOT] rebuilt_local_reanimation type={} source_tracks={} local_tracks={} source_clip={}:{} local_clip={}:{}",
             static_cast<int>(reanimation.mReanimationType),
             sourceTracks,
             localLayout.tracks.size(),
             reanimation.mFrameStart,
             reanimation.mFrameCount,
             playback.start,
             playback.count);
    reanimation.mFrameStart = playback.start;
    reanimation.mFrameCount = playback.count;
    reanimation.mFrameBasePose = playback.basePose;
    reanimation.SetAnimRate(playback.rate);
    // Keep mAnimTime, mLastFrameTime, mLoopType/mLoopCount, IDs and transforms.
    // Restarting PlayReanim here would repeat completed actions in the viewer.
    return nullptr;
}

bool RestoreDifferentReanimationTracks(Reanimation &reanimation,
                                       SaveGameContext &context,
                                       const netplay::snapshot::AnimationLayout &sourceLayout,
                                       const netplay::snapshot::AnimationLayout &localLayout,
                                       std::span<const uint32_t> discardedAttachments) {
    using namespace netplay::snapshot;
    AnimationPlayback playback{reanimation.mFrameStart, reanimation.mFrameCount, reanimation.mFrameBasePose, reanimation.mAnimRate, reanimation.mAnimTime, reanimation.mLastFrameTime};
    if (!RemapPlayback(sourceLayout, localLayout, playback))
        return false;

    auto local = std::span(reanimation.mTrackInstances, reanimation.mDefinition->mTrackCount);
    for (auto &track : local) {
        // Allocation is zeroed. These are the nonzero native constructor defaults.
        track.mTrackColor = Sexy::Color(255, 255, 255, 255);
        track.mTruncateDisappearingFrames = true;
        track.mBlendTransform.mText = "";
    }
    std::vector<ReanimatorTrackInstance> source(sourceLayout.tracks.size());
    if (!ReadRemappedTracks(context, std::span(source), local, sourceLayout, localLayout, discardedAttachments))
        return false;

    LOG_ERROR("[SNAPSHOT] remapped_reanimation type={} source_tracks={} local_tracks={} source_clip={}:{} local_clip={}:{}",
              static_cast<int>(reanimation.mReanimationType),
              source.size(),
              local.size(),
              reanimation.mFrameStart,
              reanimation.mFrameCount,
              playback.start,
              playback.count);
    reanimation.mFrameStart = playback.start;
    reanimation.mFrameCount = playback.count;
    reanimation.mFrameBasePose = playback.basePose;
    reanimation.SetAnimRate(playback.rate);
    return true;
}
} // namespace

void SyncReanimation(Board *theBoard, Reanimation *theReanimation, SaveGameContext &theContext) {
    static_assert(sizeof(ReanimatorTrackInstance) == 100); // 3215 ARM save layout
    SnapshotReadStage diagnosticStage(theContext, "reanimation");
    if (diagnosticStage.diagnostics)
        diagnosticStage.diagnostics->reanimationType = static_cast<int>(theReanimation->mReanimationType);
    if (theContext.mReading) {
        // The preceding raw object block contains the host's pointer values.
        // Never leave those available to failure cleanup or a zero-track case.
        theReanimation->mDefinition = nullptr;
        theReanimation->mTrackInstances = nullptr;
        theReanimation->mReanimatorTransforms = nullptr;
        theReanimation->mReanimationHolder = theBoard->mApp->mEffectSystem->mReanimationHolder;
    }
    theContext.SyncReanimationDef(theReanimation->mDefinition);
    if (theContext.mFailed || !theReanimation->mDefinition)
        return;

    int definitionType = -1;
    for (int type = 0; type < ReanimationType::EXTENDED_NUM_REANIMS; ++type)
        if (theReanimation->mDefinition == &gReanimatorDefArray[type]) {
            definitionType = type;
            break;
        }
    if (!theContext.mReading && gSnapshotLayoutCapture && !gSnapshotLayoutCapture->contains(definitionType)) {
        netplay::snapshot::AnimationLayout layout;
        if (definitionType < 0 || !netplay::snapshot::DescribeAnimation(*theReanimation->mDefinition, layout))
            *gSnapshotLayoutCaptureValid = false;
        else
            gSnapshotLayoutCapture->emplace(definitionType, std::move(layout));
    }
    auto *diagnostics = diagnosticStage.diagnostics;
    bool discardCosmetic = false;
    auto localCosmetic = netplay::snapshot::LocalCosmeticAnimation::None;
    if (theContext.mReading && diagnostics && diagnostics->allowTrackRemap) {
        // Native SyncBoard has loaded all object/attachment IDs before entering
        // this loop. Prune only classified cosmetic references, without calling
        // gameplay code or following still-foreign object pointers.
        if (!diagnostics->cosmetics.prepared && !netplay::snapshot::PrepareSnapshotCosmetics(*theBoard, diagnostics->cosmetics)) {
            theContext.mFailed = true;
            ReportSnapshotReadFailure(&theContext, "reanimation_cosmetic_attachment_invalid");
            return;
        }
        const auto id = theBoard->mApp->mEffectSystem->mReanimationHolder->mReanimations.DataArrayGetID(theReanimation);
        discardCosmetic = diagnostics->cosmetics.DiscardsAnimation(id);
        localCosmetic = diagnostics->cosmetics.LocalAnimation(id);
    }
    const netplay::snapshot::AnimationLayout *sourceLayout = nullptr;
    if (theContext.mReading && diagnostics && diagnostics->allowTrackRemap && diagnostics->sourceLayouts) {
        const auto entry = diagnostics->sourceLayouts->find(definitionType);
        if (entry == diagnostics->sourceLayouts->end()) {
            theContext.mFailed = true;
            ReportSnapshotReadFailure(&theContext, "reanimation_layout_missing");
            return;
        }
        sourceLayout = &entry->second;
    }
    const int aTrackCount = theReanimation->mDefinition->mTrackCount;
    if (diagnosticStage.diagnostics)
        diagnosticStage.diagnostics->trackCount = aTrackCount;
    if (discardCosmetic || localCosmetic != netplay::snapshot::LocalCosmeticAnimation::None) {
        // Read the complete 3215 payload, but neither resolve foreign images
        // nor restore the host's cosmetic playback. Persistent visuals retain
        // their slots/parents and restart using the local animation definition.
        if (discardCosmetic) {
            // Leave an inert, serializable slot for the normal deletion pass.
            // A replay/checkpoint can be saved before that pass runs.
            theReanimation->mDead = true;
            theReanimation->mFrameStart = 0;
            theReanimation->mFrameCount = aTrackCount ? theReanimation->mDefinition->mTracks[0].mTransformCount : 0;
            theReanimation->mFrameBasePose = -1;
            theReanimation->mAnimTime = 0;
            theReanimation->mLastFrameTime = -1;
            theReanimation->SetAnimRate(0);
        }
        const auto originalBlock = DescribeSnapshotRead(*diagnostics, "SyncBytes", aTrackCount * sizeof(ReanimatorTrackInstance), true, __builtin_return_address(0));
        std::vector<ReanimatorTrackInstance> source;
        const bool sourceEmpty = sourceLayout ? sourceLayout->tracks.empty() : aTrackCount == 0;
        if (!sourceEmpty) {
            const auto &buffer = theContext.mBuffer;
            const auto block = netplay::snapshot::PeekTrackBlock(buffer.mData, buffer.mDataBitSize, buffer.mReadBitPos, sizeof(ReanimatorTrackInstance));
            if (!block.complete) {
                theContext.mFailed = true;
                diagnostics->last = originalBlock;
                ReportSnapshotReadFailure(&theContext, "reanimation_cosmetic_tracks_invalid");
                return;
            }
            source.resize(block.count);
            if (!netplay::snapshot::ReadDiscardedTracks(theContext, std::span(source))) {
                theContext.mFailed = true;
                ReportSnapshotReadFailure(&theContext, "reanimation_cosmetic_tracks_truncated");
                return;
            }
        }
        if (aTrackCount > 0) {
            const size_t allocationSize = size_t(aTrackCount) * (sizeof(ReanimatorTrackInstance) + sizeof(ReanimatorTransform));
            auto *memory = static_cast<unsigned char *>(FindGlobalAllocator(allocationSize)->Calloc(allocationSize));
            HOMURA_ASSERT(memory != nullptr);
            theReanimation->mTrackInstances = reinterpret_cast<ReanimatorTrackInstance *>(memory);
            theReanimation->mReanimatorTransforms = reinterpret_cast<ReanimatorTransform *>(memory + size_t(aTrackCount) * sizeof(ReanimatorTrackInstance));
            for (auto &track : std::span(theReanimation->mTrackInstances, aTrackCount)) {
                track.mTrackColor = Sexy::Color(255, 255, 255, 255);
                track.mTruncateDisappearingFrames = true;
                track.mBlendTransform.mText = "";
            }
        }
        if (!discardCosmetic) {
            for (auto &track : source)
                netplay::snapshot::ClearDiscardedAttachment(track, diagnostics->cosmetics.attachments);
            if (!netplay::snapshot::RestoreLocalCosmeticAnimation(*theBoard, *theReanimation, localCosmetic, source)) {
                theContext.mFailed = true;
                diagnostics->last = originalBlock;
                gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_RESOURCE_MISMATCH]";
                ReportSnapshotReadFailure(&theContext, "reanimation_local_cosmetic_invalid");
                return;
            }
            LOG_INFO("[SNAPSHOT] local_cosmetic_reanimation type={} source_tracks={} local_tracks={}", definitionType, source.size(), aTrackCount);
        }
        return;
    }
    if (aTrackCount == 0 || (sourceLayout && sourceLayout->tracks.empty())) {
        if (sourceLayout && sourceLayout->tracks.size() != static_cast<size_t>(aTrackCount)) {
            theContext.mFailed = true;
            gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_RESOURCE_MISMATCH]";
            ReportSnapshotReadFailure(&theContext, "reanimation_empty_layout_mismatch");
        }
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

    // Adapt only incoming spectator snapshots. Metadata lets us also detect
    // reordered tracks with the same count. Legacy hosts have no names: prefer
    // the packaged baseline, then reconstruct unknown layouts from local state.
    if (theContext.mReading && diagnostics && diagnostics->allowTrackRemap) {
        const auto &buffer = theContext.mBuffer;
        const auto block = netplay::snapshot::PeekTrackBlock(buffer.mData, buffer.mDataBitSize, buffer.mReadBitPos, sizeof(ReanimatorTrackInstance));
        const bool outsideLocalFrames = !netplay::snapshot::ValidClip(theReanimation->mFrameStart, theReanimation->mFrameCount, theReanimation->mDefinition->mTracks[0].mTransformCount);
        if (block.complete && (sourceLayout || block.bytes != static_cast<int>(aTrackInstancesSize) || outsideLocalFrames)) {
            if (!sourceLayout) {
                sourceLayout = netplay::snapshot::GetBundledAnimationLayout(definitionType);
                if (sourceLayout
                    && (sourceLayout->tracks.size() != static_cast<size_t>(block.count) || !netplay::snapshot::ValidClip(theReanimation->mFrameStart, theReanimation->mFrameCount, sourceLayout->frames)
                        || theReanimation->mFrameBasePose >= sourceLayout->frames))
                    sourceLayout = nullptr; // An old custom host cannot be identified from count alone.
            }
            if (sourceLayout) {
                const auto originalBlock = DescribeSnapshotRead(*diagnostics, "SyncBytes", static_cast<int>(aTrackInstancesSize), true, __builtin_return_address(0));
                auto [local, inserted] = diagnostics->localLayouts.try_emplace(definitionType);
                const bool described = !inserted || netplay::snapshot::DescribeAnimation(*theReanimation->mDefinition, local->second);
                if (described && *sourceLayout == local->second && sourceLayout->tracks.size() == static_cast<size_t>(block.count)) {
                    // Identical definitions use the original native read below.
                } else if (!described || sourceLayout->tracks.size() != static_cast<size_t>(block.count)
                           || !RestoreDifferentReanimationTracks(*theReanimation, theContext, *sourceLayout, local->second, diagnostics->cosmetics.attachments)) {
                    theContext.mFailed = true;
                    diagnostics->last = originalBlock;
                    gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_RESOURCE_MISMATCH]";
                    ReportSnapshotReadFailure(&theContext, "reanimation_layout_incompatible");
                    return;
                } else {
                    return;
                }
            } else {
                auto [local, inserted] = diagnostics->localLayouts.try_emplace(definitionType);
                const bool described = !inserted || netplay::snapshot::DescribeAnimation(*theReanimation->mDefinition, local->second);
                const auto originalBlock = DescribeSnapshotRead(*diagnostics, "SyncBytes", static_cast<int>(aTrackInstancesSize), true, __builtin_return_address(0));
                const char *failure =
                    described ? RestoreLocalReanimation(*theBoard, *theReanimation, theContext, block.count, local->second, diagnostics->cosmetics.attachments) : "reanimation_local_layout_invalid";
                if (failure) {
                    theContext.mFailed = true;
                    diagnostics->last = originalBlock;
                    gSnapshotReadFailureMessage = "[SPECTATE_SNAPSHOT_RESOURCE_MISMATCH]";
                    ReportSnapshotReadFailure(&theContext, failure);
                }
                return;
            }
        }
    }

    // 原版存档只同步 TrackInstance，不同步后面的 ReanimatorTransform。
    theContext.SyncBytes(theReanimation->mTrackInstances, static_cast<int>(aTrackInstancesSize));

    if (theContext.mFailed)
        return;

    for (int aTrackIndex = 0; aTrackIndex < aTrackCount; ++aTrackIndex) {
        ReanimatorTrackInstance &aTrackInstance = theReanimation->mTrackInstances[aTrackIndex];

        theContext.SyncImage(aTrackInstance.mImageOverride);

        if (theContext.mReading) {
            if (diagnostics && diagnostics->allowTrackRemap)
                netplay::snapshot::ClearDiscardedAttachment(aTrackInstance, diagnostics->cosmetics.attachments);
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
