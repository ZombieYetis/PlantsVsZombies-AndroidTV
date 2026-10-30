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

#include "PvZ/Lawn/Widget/ReplayManageWidget.h"
#include "Homura/Logger.h"
#include "Homura/MemberUtils.h"
#include "PvZ/Android/Native/BridgeApp.h"
#include "PvZ/Android/Native/NativeApp.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/NetplayLobbyWidget.h"
#include "PvZ/NetPlay.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/ScrollWidget.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstring>

#include <filesystem>
#include <mutex>
#include <sstream>
#include <vector>

using namespace Sexy;

namespace {
constexpr int kReplayRowHeight = 140;
bool gReplayListDirty = false;
bool gReplayImportSucceeded = false;

bool IsReplayPlayable(const ReplayMetaInfo &item) {
    return item.netplayVersion == 0 || item.netplayVersion == NETPLAY_VERSION;
}

std::vector<SeedType> ParseDeck(const std::string &text) {
    std::vector<SeedType> out;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            continue;
        }
        int v = std::atoi(token.c_str());
        out.push_back(static_cast<SeedType>(v));
    }
    return out;
}
} // namespace


class ReplayListContentWidget : public Widget {
public:
    ReplayManageWidget *mOwner;
    std::vector<ReplayMetaInfo> mReplays;
    int mTotalItems;

    explicit ReplayListContentWidget(ReplayManageWidget *owner) {
        Widget::_constructor();

        static void *sReplayListContentWidgetVTable[122];
        static std::once_flag vtableInitFlag;
        std::call_once(vtableInitFlag, [this] {
            std::memcpy(sReplayListContentWidgetVTable, vTable, sizeof(sReplayListContentWidgetVTable));
            sReplayListContentWidgetVTable[0] = (void *)homura::ExtractMemFuncPtr(&ReplayListContentWidget::_destructor);
            sReplayListContentWidgetVTable[1] = (void *)homura::ExtractMemFuncPtr(&ReplayListContentWidget::_destructor2);
            sReplayListContentWidgetVTable[36] = (void *)homura::ExtractMemFuncPtr(&ReplayListContentWidget::Draw);
            sReplayListContentWidgetVTable[78] = (void *)homura::ExtractMemFuncPtr(&ReplayListContentWidget::MouseDown);
        });
        vTable = sReplayListContentWidgetVTable;

        mOwner = owner;
        RefreshReplays();
    }

    ~ReplayListContentWidget() {
        // 不调用自身的 _destructor, 否则会重复析构子对象
        Widget::_destructor();
    }

    void RefreshReplays() {
        mReplays = replay::ListReplayFiles();
        mTotalItems = static_cast<int>(mReplays.size());
        if (mOwner != nullptr) {
            const int selected = mOwner->mSelectedReplayIndex;
            if (selected >= mTotalItems) {
                mOwner->mSelectedReplayIndex = mTotalItems > 0 ? (mTotalItems - 1) : -1;
            }
            Resize(0, 0, mOwner->mScrollWidget->mWidth, mTotalItems * kReplayRowHeight);
        }
        LOG_INFO("[REPLAY] list loaded count={}", mTotalItems);
    }

    void Draw(Graphics *g) {
        int y = 0;
        int index = 0;
        for (const auto &item : mReplays) {
            const int rowTop = y;
            const int rowMidY = rowTop + kReplayRowHeight / 2;
            const int centerX = mWidth / 2;
            const int sideMargin = 200;
            const int leftNameX = sideMargin;
            const int leftDeckX = sideMargin;
            [[maybe_unused]] const int rightDeckX = mWidth - sideMargin - (6 * 50);
            const int rightDeckRightmostX = mWidth - sideMargin - 50;
            const int rightNameX = mWidth - sideMargin;
            const int secs = item.boardTicks / 100;
            const int mm = secs / 60;
            const int ss = secs % 60;
            const char *plantPlayer = "Unknown";
            const char *zombiePlayer = "Unknown";
            if (item.hostCamp == "Plant") {
                plantPlayer = item.hostName.c_str();
                zombiePlayer = item.guestName.c_str();
            } else if (item.hostCamp == "Zombie") {
                plantPlayer = item.guestName.c_str();
                zombiePlayer = item.hostName.c_str();
            }
            const bool versionMismatch = item.netplayVersion != NETPLAY_VERSION;
            const bool plantWin = item.winnerCamp == "Plant";
            const bool zombieWin = item.winnerCamp == "Zombie";

            if (mOwner != nullptr && index == mOwner->mSelectedReplayIndex) {
                g->SetColor(Color(255, 238, 120, 80));
                g->FillRect(Rect(sideMargin - 30, rowTop + 4, mWidth - sideMargin * 2 + 60, kReplayRowHeight - 8));
                g->SetColor(Color(255, 220, 90, 180));
                g->DrawRect(Rect(sideMargin - 30, rowTop + 4, mWidth - sideMargin * 2 + 60, kReplayRowHeight - 8));
            }

            TodDrawString(g, plantPlayer, leftNameX, rowTop + 26, FONT_DWARVENTODCRAFT18, Color(170, 255, 170), DrawStringJustification::DS_ALIGN_LEFT);
            TodDrawString(g, zombiePlayer, rightNameX, rowTop + 26, FONT_DWARVENTODCRAFT18, Color(255, 170, 170), DrawStringJustification::DS_ALIGN_RIGHT);

            const pvzstl::string timeText = StrFormat(TodStringTranslate("[REPLAY_TIME_FMT]").c_str(), item.createdAt.c_str(), mm, ss);

            pvzstl::string bgText;
            switch (item.vsBackground) {
                case 0:
                    bgText = TodStringTranslate("[MP_VS_DAY]");
                    break;
                case 1:
                    bgText = TodStringTranslate("[MP_VS_NIGHT]");
                    break;
                case 2:
                    bgText = TodStringTranslate("[MP_VS_POOL_DAY]");
                    break;
                case 3:
                    bgText = TodStringTranslate("[MP_VS_POOL_NIGHT]");
                    break;
                case 4:
                    bgText = TodStringTranslate("[MP_VS_ROOF]");
                    break;
                case -1:
                    bgText = TodStringTranslate("[MP_VS_SHUFFLE_MODE]");
                    break;
                case -2:
                    bgText = TodStringTranslate("[MP_VS_CLASSIC_MODE]");
                    break;
                default:
                    bgText = "Unknown";
                    break;
            };
            TodDrawString(g, timeText, centerX, rowTop + 35, FONT_DWARVENTODCRAFT18, Color(255, 244, 130), DrawStringJustification::DS_ALIGN_CENTER);
            TodDrawString(g, bgText, centerX, rowTop + 62, FONT_HOUSEOFTERROR16, Color(220, 220, 220), DrawStringJustification::DS_ALIGN_CENTER);
            if (plantWin) {
                g->DrawImage(IMAGE_MP_PLANT_TROPHY, centerX - 30, rowMidY - 10, 60, 60);
            } else if (zombieWin) {
                g->DrawImage(IMAGE_MP_ZOMBIE_TROPHY, centerX - 30, rowMidY - 10, 60, 60);
            }
            if (versionMismatch) {
                TodDrawString(g, "[REPLAY_ERROR_VERSION]", centerX, rowTop + 124, FONT_HOUSEOFTERROR16, Color(255, 120, 120), DrawStringJustification::DS_ALIGN_CENTER);
            }

            const auto plantDeck = ParseDeck(item.plantDeck);
            const auto zombieDeck = ParseDeck(item.zombieDeck);
            int seedX = leftDeckX;
            for (SeedType st : plantDeck) {
                if (st != SEED_NONE) {
                    DrawSeedPacket(g, (float)seedX, (float)(rowTop + 48), st, SeedType::SEED_NONE, 0.0f, 255, false, false, false, true);
                    seedX += 50;
                }
            }
            seedX = rightDeckRightmostX;
            for (auto it = zombieDeck.rbegin(); it != zombieDeck.rend(); ++it) {
                SeedType st = *it;
                if (st != SEED_NONE) {
                    DrawSeedPacket(g, (float)seedX, (float)(rowTop + 48), st, SeedType::SEED_NONE, 0.0f, 255, false, false, true, true);
                    seedX -= 50;
                }
            }
            y += kReplayRowHeight;
            ++index;
        }
    }

    void MouseDown(int x, int y, int theClickCount) {
        const int index = y / kReplayRowHeight;
        if (index < 0 || index >= static_cast<int>(mReplays.size()) || mOwner == nullptr) {
            LOG_WARN("[REPLAY] invalid click y={}, idx={}, count={}", y, index, (int)mReplays.size());
            return;
        }
        LOG_INFO("[REPLAY] click list index={} file={}", index, mReplays[index].fileName);
        mOwner->SelectReplayIndex(index);
    }

protected:
    void _destructor() {
        mReplays.~vector();
        Widget::_destructor();
    }

    void _destructor2() {
        delete this;
    }
};

ReplayManageWidget::ReplayManageWidget(LawnApp *app, ButtonListener *buttonListener) {
    Widget::_constructor();

    static void *sReplayManageWidgetVTable[122];
    static std::once_flag vtableInitFlag;
    std::call_once(vtableInitFlag, [this] {
        std::memcpy(sReplayManageWidgetVTable, vTable, sizeof(sReplayManageWidgetVTable));
        sReplayManageWidgetVTable[0] = (void *)homura::ExtractMemFuncPtr(&ReplayManageWidget::_destructor);
        sReplayManageWidgetVTable[1] = (void *)homura::ExtractMemFuncPtr(&ReplayManageWidget::_destructor2);
        sReplayManageWidgetVTable[29] = (void *)homura::ExtractMemFuncPtr(&ReplayManageWidget::AddedToManager);
        sReplayManageWidgetVTable[30] = (void *)homura::ExtractMemFuncPtr(&ReplayManageWidget::RemovedFromManager);
        sReplayManageWidgetVTable[36] = (void *)homura::ExtractMemFuncPtr(&ReplayManageWidget::Draw);
    });
    vTable = sReplayManageWidgetVTable;

    mApp = app;
    mButtonListener = buttonListener;
    // ReplayManageWidget is a child of NetplayLobbyWidget, so its position is
    // relative to the lobby rather than the screen's fullscreen offset.
    Resize(0, 0, LawnApp::FULLSCREEN_RECT.mWidth, LawnApp::FULLSCREEN_RECT.mHeight);
    mClip = true;

    mScrollWidget = new ScrollWidget();
    mScrollWidget->Resize(0, 150, mWidth, mHeight - 150);
    mScrollWidget->SetScrollMode(ScrollWidget::ScrollMode::SCROLL_VERTICAL);
    mScrollWidget->EnableBounce(false);
    mScrollContent = new ReplayListContentWidget(this);
    mScrollContent->Resize(0, 0, mScrollWidget->mWidth, mScrollContent->mTotalItems * kReplayRowHeight);

    mScrollWidget->ScrollToMin(false);
    mPlayButton = MakeButton(ReplayManageWidget_Play, buttonListener, this, "[REPLAY_PLAY]");
    mPlayButton->Resize(120, 564, 170, 50);
    mDeleteButton = MakeButton(ReplayManageWidget_Delete, buttonListener, this, "[REPLAY_DELETE]");
    mDeleteButton->Resize(340, 564, 170, 50);
    mImportButton = MakeButton(ReplayManageWidget_Import, buttonListener, this, "[REPLAY_IMPORT]");
    mImportButton->Resize(560, 564, 170, 50);
    mExportButton = MakeButton(ReplayManageWidget_Export, buttonListener, this, "[REPLAY_EXPORT]");
    mExportButton->Resize(780, 564, 170, 50);
    mCloseButton = MakeButton(ReplayManageWidget_Close, buttonListener, this, "[CLOSE]");
    mCloseButton->Resize(1000, 564, 170, 50);

    mZombieBackground = Rand(2);

    mSelectedReplayIndex = mScrollContent->mTotalItems > 0 ? 0 : -1;
    mNeedRefreshList = false;
    TodLoadResources("DelayLoad_Almanac");
}

ReplayManageWidget::~ReplayManageWidget() {
    _destructor();
}

void ReplayManageWidget::_destructor() {
    delete mPlayButton;
    delete mDeleteButton;
    delete mExportButton;
    delete mImportButton;
    delete mCloseButton;
    delete mScrollContent;
    delete mScrollWidget;

    Widget::_destructor();
}

void ReplayManageWidget::_destructor2() {
    delete this;
}

void ReplayManageWidget::AddedToManager(Sexy::WidgetManager *theWidgetManager) {
    WidgetContainer::AddedToManager(theWidgetManager);

    AddWidget(mScrollWidget);
    mScrollWidget->AddWidget(mScrollContent);
    AddWidget(mCloseButton);
    AddWidget(mImportButton);
    AddWidget(mExportButton);
    AddWidget(mDeleteButton);
    AddWidget(mPlayButton);
}

void ReplayManageWidget::RemovedFromManager(Sexy::WidgetManager *theWidgetManager) {
    WidgetContainer::RemovedFromManager(theWidgetManager);

    mScrollWidget->RemoveWidget(mScrollContent);
    RemoveWidget(mScrollWidget);
    RemoveWidget(mCloseButton);
    RemoveWidget(mImportButton);
    RemoveWidget(mExportButton);
    RemoveWidget(mDeleteButton);
    RemoveWidget(mPlayButton);
}

void ReplayManageWidget::Draw(Graphics *g) {
    if (mNeedRefreshList || gReplayListDirty) {
        RefreshReplayList();
        if (gReplayImportSucceeded) {
            // 导入成功后回到顶部，选中最近加入的回放。
            mSelectedReplayIndex = mScrollContent->mTotalItems > 0 ? 0 : -1;
            mScrollWidget->ScrollToMin(false);
            gReplayImportSucceeded = false;
        }
        gReplayListDirty = false;
    }
    const bool hasReplay = mScrollContent->mTotalItems > 0;
    const bool hasPlayableSelection = hasReplay && mSelectedReplayIndex >= 0 && mSelectedReplayIndex < mScrollContent->mTotalItems && IsReplayPlayable(mScrollContent->mReplays[mSelectedReplayIndex]);
    mPlayButton->mDisabled = !hasPlayableSelection;
    mExportButton->mDisabled = !hasReplay;
    mDeleteButton->mDisabled = !hasReplay;
    g->DrawImage(mZombieBackground ? IMAGE_ALMANAC_ZOMBIEBACK : IMAGE_ALMANAC_PLANTBACK, 0, 0);
    TodDrawString(g, "[REPLAY_MANAGE]", 640, 110, FONT_DWARVENTODCRAFT24, Color(255, 248, 195), DrawStringJustification::DS_ALIGN_CENTER);
    TodDrawString(
        g, StrFormat(TodStringTranslate("[REPLAY_TOTAL_NUM]").c_str(), mScrollContent->mTotalItems), 1110, 110, FONT_DWARVENTODCRAFT18, Color(255, 248, 195), DrawStringJustification::DS_ALIGN_RIGHT);
    if (!hasReplay) {
        TodDrawString(g, "[NO_REPLAY_FILES]", 640, 330, FONT_HOUSEOFTERROR20, Color(255, 230, 170), DrawStringJustification::DS_ALIGN_CENTER);
    }
}

void ReplayManageWidget::SelectReplayIndex(int index) {
    if (index < 0 || index >= mScrollContent->mTotalItems) {
        return;
    }
    mSelectedReplayIndex = index;
}

void ReplayManageWidget::RefreshReplayList() {
    mScrollContent->RefreshReplays();
    if (mSelectedReplayIndex < 0 && mScrollContent->mTotalItems > 0) {
        mSelectedReplayIndex = 0;
    }
    mNeedRefreshList = false;
}

void ReplayManageWidget::RequestImportReplay() {
    std::error_code ec;
    std::filesystem::create_directories("replays", ec);
    const std::string targetDir = std::filesystem::absolute("replays", ec).string();

    Native::BridgeApp *bridgeApp = Native::BridgeApp::getSingleton();
    JNIEnv *env = bridgeApp->getJNIEnv();
    jobject activity = bridgeApp->mNativeApp->getActivity();
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls, "showReplayImportPicker", "(Ljava/lang/String;)V");
    if (mid == nullptr) {
        env->DeleteLocalRef(cls);
        return;
    }
    jstring jTargetDir = env->NewStringUTF(targetDir.c_str());
    env->CallVoidMethod(activity, mid, jTargetDir);
    env->DeleteLocalRef(jTargetDir);
    env->DeleteLocalRef(cls);
}

void ReplayManageWidget::RequestExportReplay() const {
    if (mSelectedReplayIndex < 0 || mSelectedReplayIndex >= mScrollContent->mTotalItems) {
        return;
    }
    const auto &item = mScrollContent->mReplays[mSelectedReplayIndex];

    Native::BridgeApp *bridgeApp = Native::BridgeApp::getSingleton();
    JNIEnv *env = bridgeApp->getJNIEnv();
    jobject activity = bridgeApp->mNativeApp->getActivity();
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls, "showReplayExportPicker", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (mid == nullptr) {
        env->DeleteLocalRef(cls);
        return;
    }
    jstring jSourcePath = env->NewStringUTF(item.filePath.c_str());
    jstring jName = env->NewStringUTF(item.fileName.c_str());
    env->CallVoidMethod(activity, mid, jSourcePath, jName);
    env->DeleteLocalRef(jSourcePath);
    env->DeleteLocalRef(jName);
    env->DeleteLocalRef(cls);
}

void ReplayManageWidget::DeleteSelectedReplay() {
    if (mSelectedReplayIndex < 0 || mSelectedReplayIndex >= mScrollContent->mTotalItems) {
        return;
    }
    if (mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[DIALOG_WARNING]", "[CONFIRM_DELETE_REPLAY]", "[DIALOG_BUTTON_OK]", "[DIALOG_BUTTON_CANCEL]", 1) != 1000) {
        return;
    }
    const auto path = mScrollContent->mReplays[mSelectedReplayIndex].filePath;
    std::error_code ec;
    const bool removed = std::filesystem::remove(path, ec);
    LOG_INFO("[REPLAY] delete selected path={} removed={} err={}", path, removed, ec.value());
    RefreshReplayList();
}

void ReplayManageWidget::PlaySelectedReplay() {
    if (mSelectedReplayIndex < 0 || mSelectedReplayIndex >= mScrollContent->mTotalItems) {
        return;
    }
    StartReplayByIndex(mSelectedReplayIndex);
}

void ReplayManageWidget::StartReplayByIndex(int index) const {
    if (index < 0 || index >= mScrollContent->mTotalItems) {
        LOG_ERROR("[REPLAY] start failed invalid index={} size={}", index, mScrollContent->mTotalItems);
        return;
    }
    const ReplayMetaInfo &item = mScrollContent->mReplays[index];
    if (!IsReplayPlayable(item)) {
        LOG_WARN("[REPLAY] start blocked version mismatch file={} version={} local={}", item.filePath, item.netplayVersion, NETPLAY_VERSION);
        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[REPLAY]", "[REPLAY_ERROR_VERSION_TEXT]", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }
    if (!replay::BeginPlaybackFromFile(item.filePath)) {
        LOG_ERROR("[REPLAY] begin playback failed file={}", item.filePath);
        return;
    }
    gIsServerModeNetplay = true;
    gServerModeTransport = ServerModeTransport::RELAY;
    gIsServerModeSpectator = false;
    gIsReplayMode = true;
    LOG_INFO("[REPLAY] start playback success file={}", item.filePath);

    if (auto *dialog = NetplayLobbyWidget::GetInstance()) {
        LOG_INFO("[REPLAY] closing replay manager via NetplayLobbyWidget");
        dialog->CloseReplayManageWidget();
        dialog->RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
    }

    Challenge::msVSShuffleMode = false;
    Challenge::msVSClassicMode = false;
    switch (item.vsBackground) {
        case 0:
            gVSBackground = BackgroundType::BACKGROUND_1_DAY;
            break;
        case 1:
            gVSBackground = BackgroundType::BACKGROUND_2_NIGHT;
            break;
        case 2:
            gVSBackground = BackgroundType::BACKGROUND_3_POOL;
            break;
        case 3:
            gVSBackground = BackgroundType::BACKGROUND_4_FOG;
            break;
        case 4:
            gVSBackground = BackgroundType::BACKGROUND_5_ROOF;
            break;
        case -1:
            gVSBackground = BackgroundType::BACKGROUND_1_DAY;
            Challenge::msVSShuffleMode = true;
            break;
        case -2:
            gVSBackground = BackgroundType::BACKGROUND_1_DAY;
            Challenge::msVSClassicMode = true;
            break;
        default:
            break;
    }

    mApp->KillChallengeScreen();
    mApp->PreNewGame(GAMEMODE_MP_VS, false);
}

namespace replayui {
void OnReplayImportFinished(bool success, const char *message) {
    LOG_INFO("[REPLAY] import finished success={} msg={}", success, message ? message : "");
    if (success) {
        gReplayImportSucceeded = true;
    }
    gReplayListDirty = true;
}

void OnReplayExportFinished(bool success, const char *message) {
    LOG_INFO("[REPLAY] export finished success={} msg={}", success, message ? message : "");
}
} // namespace replayui
