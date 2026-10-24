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

#include "PvZ/Lawn/Widget/VSResultsMenu.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/NetPlay.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Widget/ImageWidget.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <unistd.h>

#include <ctime>

#include <chrono>

using namespace Sexy;

namespace {
constexpr int kVSResultRequestStateReplaySaved = -2;
constexpr int kVSResultRequestStateOpponentDisconnected = -3;
constexpr int kVSResultRequestStateSelfDisconnected = -4;

static bool IsOnlineResultsSessionActive() {
    return IsRemoteClient() || IsRemoteServer();
}

static void CloseResultsSocketsAndResetNetState() {
    if (gTcpServerSocket >= 0) {
        shutdown(gTcpServerSocket, SHUT_RDWR);
        close(gTcpServerSocket);
        gTcpServerSocket = -1;
    }
    if (IsRemoteServer()) {
        shutdown(gTcpClientSocket, SHUT_RDWR);
        close(gTcpClientSocket);
        gTcpClientSocket = -1;
    }
    if (gTcpListenSocket >= 0) {
        close(gTcpListenSocket);
        gTcpListenSocket = -1;
    }

    gTcpConnected = false;
    gTcpConnecting = false;
    gIsServerModeNetplay = false;
    gServerModeTransport = ServerModeTransport::NONE;
    gIsServerModeSpectator = false;
    gIsReplayMode = false;
    gReplayPauseByMenu = false;
    clientRecvBuffer.clear();
    serverRecvBuffer.clear();
    netplay::ClearSendBuffer();

    gNetPingSendCounter = 0;
    gNetDelayNow = 0;
    gNetPingHasValidDelay = false;
    gNetPingAwaitingPong = false;
    gNetPingNowTick = 0;
    gNetPingLatestSentTick = 0;
    gNetPingLastPongTick = 0;
}

static void DisablePlayAgainButton(VSResultsMenu *menu) {
    if (menu == nullptr) {
        return;
    }
    if (Sexy::Widget *playAgain = menu->FindWidget(VSResultsMenu::VSResultsMenu_Play_Again)) {
        playAgain->mDisabled = true;
        (*playAgain->mColors)[ButtonWidget::COLOR_LABEL] = gColorGray;
        (*playAgain->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = gColorGray;
    }
}

static void SetPlayAgainAsReplayManageButton(VSResultsMenu *menu) {
    if (menu == nullptr) {
        return;
    }
    if (Sexy::Widget *playAgain = menu->FindWidget(VSResultsMenu::VSResultsMenu_Play_Again)) {
        auto *button = reinterpret_cast<GameButton *>(playAgain);
        button->SetLabel("[REPLAY_MANAGE]");
        button->mDisabled = false;
        (*button->mColors)[ButtonWidget::COLOR_LABEL] = Color(25, 197, 45);
        (*button->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = Color(277, 225, 108);
    }
}
} // namespace

static std::string BuildDeckText(const SeedType *seeds, int count) {
    std::string s;
    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            s.push_back(',');
        }
        s += std::to_string(static_cast<int>(seeds[i]));
    }
    return s;
}

static const char *CampNameFromSide(int side) {
    if (side == 0) {
        return "Plant";
    }
    if (side == 1) {
        return "Zombie";
    }
    return "Unknown";
}


void VSResultsMenu::_constructor() {
    old_VSResultsMenu_Constructor(this);

    mIsReplaySession = gIsReplayMode;
    mIsOnlineSession = !mIsReplaySession && IsOnlineResultsSessionActive();
    gVSResultRequestState = -1;
    gNetDelayNow = 0; // 清除旧的延时数据

    mBackButton = MakeButton(VSResultsMenu::VSResultsMenu_Back, this, this, "[BACK_TO_MODE_SELECT]");
    mBackButton->mDrawStoneButton = false;
    mBackButton->mButtonImage = addonImages.VS_Button;
    mBackButton->mOverImage = addonImages.VS_Button_selected;
    mBackButton->mDownImage = addonImages.VS_Button_selected;
    mBackButton->SetFont(FONT_DWARVENTODCRAFT24);
    (*mBackButton->mColors)[ButtonWidget::COLOR_LABEL] = Color(25, 197, 45);
    (*mBackButton->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = Color(277, 225, 108);
    mBackButton->mLabelJustify = BUTTON_LABEL_WRAP_CENTER;
    mBackButton->Resize(-60, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void VSResultsMenu::_destructor() {
    delete mBackButton;

    old_VSResultsMenu_Destructor(this);
}

void VSResultsMenu::AddedToManager(Sexy::WidgetManager *theWidgetManager) {
    old_VSResultsMenu_AddedToManager(this, theWidgetManager);

    AddWidget(mBackButton);
    if (mIsReplaySession) {
        SetPlayAgainAsReplayManageButton(this);
    }
    if (Sexy::Widget *quitVsWidget = FindWidget(VSResultsMenu::VSResultsMenu_Quit_VS)) {
        auto *quitVsButton = static_cast<GameButton *>(quitVsWidget);
        quitVsButton->SetLabel(mIsOnlineSession && IsOnlineResultsSessionActive() ? "[DISCONNECT]" : "[QUIT_VS]");
    }
}

void VSResultsMenu::RemovedFromManager(Sexy::WidgetManager *theWidgetManager) {
    RemoveWidget(mBackButton);

    old_VSResultsMenu_RemovedFromManager(this, theWidgetManager);
}

void VSResultsMenu::ClearPlayerRecords() {
    for (auto &record : msPlayerRecords) {
        record[0] = -1;
        record[1] = 0;
        record[2] = 0;
        record[3] = 0;
        record[4] = -1;
    }
}

void VSResultsMenu::processClientEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_CLIENT_VSRESULT_BUTTON_DEPRESS: {
            auto *event1 = static_cast<const U8_Event *>(event);
            int anId = event1->data;
            gVSResultRequestState = anId;
        } break;
        default:
            break;
    }
}


void VSResultsMenu::processServerEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_SERVER_VSRESULT_BUTTON_DEPRESS: {
            auto *event1 = static_cast<const U8_Event *>(event);
            int anId = event1->data;
            mResultsButtonId = anId;
            OnExit();
        } break;
        default:
            break;
    }
}

void VSResultsMenu::InitFromBoard(Board *theBoard) {
    mBoardMainCounter = theBoard->mMainCounter;
    mBoardBackground = Challenge::msVSShuffleMode ? BackgroundType(-1) : theBoard->mBackground;

    int aSeedNum = theBoard->mSeedBank[0]->mNumPackets;
    for (int i = 1; i < aSeedNum; ++i) {
        mPlantSeeds[i - 1] = theBoard->mSeedBank[0]->mSeedPackets[i].mPacketType;
        mZombieSeeds[i - 1] = theBoard->mSeedBank[1]->mSeedPackets[i].mPacketType;
    }

    const BoardResult aBoardResult = gLawnApp->mBoardResult;
    const auto *controls0 = theBoard->mGamepadControls[0];
    const auto *controls1 = theBoard->mGamepadControls[1];

    mPlayerIndices[0] = controls0->mGamepadIndex;
    mPlayerIndices[1] = controls1->mGamepadIndex;
    mSides[0] = WinSide(controls0->mIsZombie);
    mSides[1] = WinSide(controls1->mIsZombie);

    const bool globalBpActive = VSSetupAddonWidget::msGlobalBpMode != VSSetupAddonWidget::GLOBALBP_CLOSED;
    int losingPlayerIndex = -1;
    if (globalBpActive && (aBoardResult == BoardResult::BOARDRESULT_VS_PLANT_WON || aBoardResult == BoardResult::BOARDRESULT_VS_ZOMBIE_WON)) {
        const WinSide winningCamp = (aBoardResult == BoardResult::BOARDRESULT_VS_PLANT_WON) ? WinSide::WIN_SIDE_PLANT : WinSide::WIN_SIDE_ZOMBIE;
        for (int slot = 0; slot < 2; ++slot) {
            if (mSides[slot] != winningCamp && mPlayerIndices[slot] >= 0 && mPlayerIndices[slot] <= 1) {
                losingPlayerIndex = mPlayerIndices[slot];
                break;
            }
        }
    }
    if (globalBpActive && losingPlayerIndex != -1) {
        VSSetupMenu::msNextSidePickPlayerIndex = losingPlayerIndex;
    }

    for (int slot = 0; slot < 2; ++slot) {
        int *playerRecord = GetPlayerRecord(mPlayerIndices[slot]);
        if (playerRecord == nullptr) {
            continue;
        }

        if (aBoardResult == BoardResult::BOARDRESULT_VS_PLANT_WON && mSides[slot] == WinSide::WIN_SIDE_PLANT) {
            playerRecord[0] = 0;
            ++playerRecord[1];
            ++playerRecord[3];
        } else if (aBoardResult == BoardResult::BOARDRESULT_VS_ZOMBIE_WON && mSides[slot] == WinSide::WIN_SIDE_ZOMBIE) {
            playerRecord[0] = 1;
            ++playerRecord[2];
            ++playerRecord[3];
        } else {
            playerRecord[0] = -1;
            playerRecord[3] = 0;
        }
    }

    int globalBpTarget = 0;
    if (VSSetupAddonWidget::msGlobalBpMode == VSSetupAddonWidget::GLOBALBP_BO3) {
        globalBpTarget = 2;
    } else if (VSSetupAddonWidget::msGlobalBpMode == VSSetupAddonWidget::GLOBALBP_BO5) {
        globalBpTarget = 3;
    }
    if (globalBpTarget > 0) {
        bool shouldClearGlobalBpSeeds = false;
        for (int slot = 0; slot < 2; ++slot) {
            int playerIndex = mPlayerIndices[slot];
            if (playerIndex < 0 || playerIndex > 1) {
                continue;
            }

            if ((aBoardResult == BoardResult::BOARDRESULT_VS_PLANT_WON && mSides[slot] == WinSide::WIN_SIDE_PLANT)
                || (aBoardResult == BoardResult::BOARDRESULT_VS_ZOMBIE_WON && mSides[slot] == WinSide::WIN_SIDE_ZOMBIE)) {
                ++VSSetupAddonWidget::msGlobalBpWins[playerIndex];
                if (VSSetupAddonWidget::msGlobalBpWins[playerIndex] >= globalBpTarget) {
                    shouldClearGlobalBpSeeds = true;
                    break;
                }
            }
        }
        if (shouldClearGlobalBpSeeds) {
            for (auto &row : VSSetupAddonWidget::msGlobalBpSeeds) {
                for (SeedType &seedType : row) {
                    seedType = SeedType::SEED_NONE;
                }
            }
            VSSetupAddonWidget::msGlobalBpWins[0] = 0;
            VSSetupAddonWidget::msGlobalBpWins[1] = 0;
            VSSetupMenu::msNextSidePickPlayerIndex = 0;
        }
    }

    Sexy::Widget *widget4 = FindWidget(VSResultsMenu::VSResultsMenu_Plant_Side);
    Sexy::Widget *widget5 = FindWidget(VSResultsMenu::VSResultsMenu_Plant_Side_Front);
    Sexy::Widget *widget6 = FindWidget(VSResultsMenu::VSResultsMenu_Zombie_Side);
    Sexy::Widget *widget7 = FindWidget(VSResultsMenu::VSResultsMenu_Zombie_Side_Front);
    auto setImageWidgetAlpha = [](Sexy::Widget *widget, int alpha) {
        if (widget == nullptr) {
            return;
        }
        reinterpret_cast<Sexy::ImageWidget *>(widget)->mAlpha = alpha;
    };
    if (aBoardResult == BoardResult::BOARDRESULT_VS_PLANT_WON) {
        setImageWidgetAlpha(widget4, 255);
        setImageWidgetAlpha(widget5, 255);
        setImageWidgetAlpha(widget6, 0);
        setImageWidgetAlpha(widget7, 0);
    } else {
        setImageWidgetAlpha(widget4, 0);
        setImageWidgetAlpha(widget5, 0);
        setImageWidgetAlpha(widget6, 255);
        setImageWidgetAlpha(widget7, 255);
    }

    Sexy::Widget *winnerWidget = nullptr;
    if (VSResultsMenu::msPlayerRecords[0][0] == -1) {
        if (mPlayerIndices[1] != -1) {
            winnerWidget = FindWidget(VSResultsMenu::VSResultsMenu_Info_Box_P2);
        }
    } else {
        winnerWidget = FindWidget(VSResultsMenu::VSResultsMenu_Info_Box_P1);
    }
    if (winnerWidget != nullptr) {
        mTrophyPosX = 94.0f + float(Sexy::IMAGE_MP_TROPHY_BASE->mWidth) / 2.0f;
        mTrophyPosY = float(-60 - winnerWidget->mY - Sexy::IMAGE_MP_TROPHY_BASE->mHeight - Sexy::IMAGE_MP_PLANT_TROPHY->mHeight);
        if (TodParticleSystem *sparkle = gLawnApp->AddTodParticle(mTrophyPosX, mTrophyPosY, 0, ParticleEffect::PARTICLE_TROPHY_SPARKLE)) {
            mSparkleParticleID = gLawnApp->ParticleGetID(sparkle);
        }
    }

    if (mIsOnlineSession && gLawnApp->mPlayerInfo->mVSResultsAutoSaveReplay) {
        SaveReplay();
    }
}

void VSResultsMenu::Update() {
    ++mVSResultsCounter;
    old_VSResultsMenu_Update(this);

    //    if (mBackButton != nullptr) {
    //        mBackButton->SetLabel(mIsOnlineSession && IsOnlineResultsSessionActive() ? "[DISCONNECT]" : "[BACK_TO_MODE_SELECT]");
    //    }
    //    if (gVSResultRequestState == kVSResultRequestStateOpponentDisconnected || gVSResultRequestState == kVSResultRequestStateSelfDisconnected) {
    //        DisablePlayAgainButton(this);
    //    } else if (Sexy::Widget *playAgain = FindWidget(VSResultsMenu_Play_Again)) {
    //        if (mIsOnlineSession) {
    //            playAgain->mDisabled = !IsOnlineResultsSessionActive();
    //        }
    //    }
}

void VSResultsMenu::HandleOpponentDisconnected() {
    gVSResultRequestState = kVSResultRequestStateOpponentDisconnected;
    if (mCheckboxController != nullptr) {
        mCheckboxController->HideCheckboxWidget();
    }
    DisablePlayAgainButton(this);
    if (Sexy::Widget *quitVsWidget = FindWidget(VSResultsMenu::VSResultsMenu_Quit_VS)) {
        auto *quitVsButton = reinterpret_cast<GameButton *>(quitVsWidget);
        quitVsButton->SetLabel("[QUIT_VS]");
    }
}

void VSResultsMenu::HideReplayButton(bool forceHide) {
    Sexy::Widget *saveBtn = FindWidget(VSResultsMenu::VSResultsMenu_Save_Replay);
    if (saveBtn == nullptr) {
        return;
    }
    const bool connected = (IsRemoteClient() || IsRemoteServer());
    if (forceHide || !connected || mIsReplaySession) {
        saveBtn->SetVisible(false);
        saveBtn->mDisabled = true;
    }
}

void VSResultsMenu::OnExit() const {
    if (mResultsButtonId == VSResultsMenu_Quit_VS) {
        gLawnApp->ShowMainMenuScreen();
        gLawnApp->KillVSResultsScreen();
    } else if (mResultsButtonId == VSResultsMenu_Play_Again) {
        gLawnApp->PreNewGame(GameMode::GAMEMODE_MP_VS, false);
        gLawnApp->KillVSResultsScreen();
    } else if (mResultsButtonId == VSResultsMenu_Back) {
        gLawnApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_VS);
        gLawnApp->KillVSResultsScreen();
    }
}

bool VSResultsMenu::SaveReplay() {
    if (mIsReplaySession) {
        LOG_INFO("[REPLAY] ignore save replay in replay session");
        return false;
    }

    ReplayMetaInfo meta;
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTm{};
    localtime_r(&now, &localTm);
    char timeText[32]{};
    std::strftime(timeText, sizeof(timeText), "%Y-%m-%d %H:%M", &localTm);
    meta.hostName = (gServerHostName[0] != '\0') ? gServerHostName : gLawnApp->mPlayerInfo->mName;
    meta.guestName = (gSecondPlayerName[0] != '\0') ? gSecondPlayerName : "Guest";
    int winnerSide = -1;
    int hostCampSide = -1;
    int guestCampSide = -1;
    for (int slot = 0; slot < 2; ++slot) {
        const int sideInSlot = mSides[slot];
        if (sideInSlot < 0 || sideInSlot > 1) {
            continue;
        }
        const int this76Side = mPlayerIndices[sideInSlot];
        if (int *playerRecord = GetPlayerRecord((unsigned int)this76Side)) {
            if (winnerSide == -1 && (playerRecord[0] == 0 || playerRecord[0] == 1)) {
                winnerSide = playerRecord[0];
            }
        }
        if (this76Side == 0) {
            hostCampSide = sideInSlot;
        } else if (this76Side == 1) {
            guestCampSide = sideInSlot;
        }
    }
    meta.hostCamp = CampNameFromSide(hostCampSide);
    meta.guestCamp = CampNameFromSide(guestCampSide);
    meta.winnerCamp = CampNameFromSide(winnerSide);
    if (winnerSide >= 0 && winnerSide == hostCampSide) {
        meta.winnerName = meta.hostName;
    } else if (winnerSide >= 0 && winnerSide == guestCampSide) {
        meta.winnerName = meta.guestName;
    } else {
        meta.winnerName = "Unknown";
    }
    meta.mapName = StrFormat("VSBG_%d", int(mBoardBackground));
    meta.vsBackground = int(mBoardBackground);
    meta.boardTicks = mBoardMainCounter;
    meta.plantDeck = BuildDeckText(mPlantSeeds, 6);
    meta.zombieDeck = BuildDeckText(mZombieSeeds, 6);
    meta.createdAt = timeText;
    meta.fileName = StrFormat("replay_%lld_%s_vs_%s.rpl", static_cast<long long>(now), meta.hostName.c_str(), meta.guestName.c_str());
    for (char &c : meta.fileName) {
        if (c == ' ' || c == '/' || c == '\\' || c == ':') {
            c = '_';
        }
    }

    const bool saved = replay::SaveCurrentMatchReplay(meta);
    LOG_INFO("[REPLAY] save requested, saved={}, file={}", saved, meta.fileName);
    if (saved) {
        HideReplayButton(true);
        gVSResultRequestState = kVSResultRequestStateReplaySaved;
    }
    return saved;
}

void VSResultsMenu::ButtonDepress(int theId) {
    if (mIsFading)
        return;

    if (theId == VSResultsMenu::VSResultsMenu_Play_Again && mIsReplaySession) {
        // 即使回放已自然结束，返回管理时也要清理会话状态。
        gLawnApp->ClearSecondPlayer();
        gChallengeScreenOpenReplayManage = true;
        gLawnApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_VS);
        gLawnApp->KillVSResultsScreen();
        return;
    }

    if (theId == VSResultsMenu::VSResultsMenu_Save_Replay) {
        SaveReplay();
        return;
    }

    if ((theId == VSResultsMenu::VSResultsMenu_Play_Again || theId == VSResultsMenu::VSResultsMenu_Back) && mIsOnlineSession && mVSResultsCounter < 300) { // 3 秒后才能点再来一局或返回模式选择
        return;
    }

    if (theId == VSResultsMenu::VSResultsMenu_Quit_VS) {
        if (mIsOnlineSession && IsOnlineResultsSessionActive()) {
            if (mCheckboxController != nullptr) {
                mCheckboxController->HideCheckboxWidget();
            }
            gVSResultRequestState = kVSResultRequestStateSelfDisconnected;
            DisablePlayAgainButton(this);
            CloseResultsSocketsAndResetNetState();
            if (Sexy::Widget *quitVsWidget = FindWidget(VSResultsMenu::VSResultsMenu_Quit_VS)) {
                auto *quitVsButton = reinterpret_cast<GameButton *>(quitVsWidget);
                quitVsButton->SetLabel("[QUIT_VS]");
            }
            if (Sexy::Widget *playAgain = FindWidget(VSResultsMenu::VSResultsMenu_Play_Again)) {
                playAgain->mDisabled = true;
            }
            return;
        }
        mResultsButtonId = theId;
        OnExit();
        return;
    }

    if (IsRemoteClient()) {
        // 客户端点击再来一局或返回模式选择
        U8_Event event = {{EventType::EVENT_CLIENT_VSRESULT_BUTTON_DEPRESS}, uint8_t(theId)};
        netplay::PutEvent(event);
        gVSResultRequestState = theId;
        return;
    }

    if (IsRemoteServer()) {
        U8_Event event = {{EventType::EVENT_SERVER_VSRESULT_BUTTON_DEPRESS}, uint8_t(theId)};
        netplay::PutEvent(event);
    }

    mResultsButtonId = theId;
    OnExit();
}

void VSResultsMenu::Draw(Graphics *g) {
    old_VSResultsMenu_Draw(this, g);

    // 观战者不绘制这些
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }

    if (mCheckboxController != nullptr) {
        mCheckboxController->DrawCheckboxLabel(g);
    }

    if (gVSResultRequestState == kVSResultRequestStateReplaySaved) {
        TodDrawString(g, "[REPLAY_SAVED]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
    } else if (gVSResultRequestState == kVSResultRequestStateOpponentDisconnected) {
        TodDrawString(g, "[VS_RESULT_OPPONENT_DISCONNECTED]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
    } else if (gVSResultRequestState == kVSResultRequestStateSelfDisconnected) {
        TodDrawString(g, "[VS_RESULT_SELF_DISCONNECTED]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
    } else {
        if (IsRemoteClient()) {
            switch (gVSResultRequestState) {
                case VSResultsMenu::VSResultsMenu_Play_Again:
                    TodDrawString(g, "[VS_RESULT_REMIND_HOST_PLAY_AGAIN]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
                    break;
                case VSResultsMenu::VSResultsMenu_Back:
                    TodDrawString(g, "[VS_RESULT_REMIND_HOST_BACK_TO_MODE_SELECT]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
                    break;
                default:
                    break;
            }
        }

        if (IsRemoteServer()) {
            switch (gVSResultRequestState) {
                case VSResultsMenu::VSResultsMenu_Play_Again:
                    TodDrawString(g, "[VS_RESULT_OPPONENT_REQUEST_PLAY_AGAIN]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
                    break;
                case VSResultsMenu::VSResultsMenu_Back:
                    TodDrawString(g, "[VS_RESULT_OPPONENT_REQUEST_BACK_TO_MODE_SELECT]", 400, -20, Sexy::FONT_HOUSEOFTERROR28, Color(0, 205, 0, 255), DrawStringJustification::DS_ALIGN_CENTER);
                    break;
                default:
                    break;
            }
        }
    }
}

void VSResultsMenu::ShowReplayButton() {
    if (mIsReplaySession) {
        return;
    }

    mSaveReplayButton = MakeButton(VSResultsMenu::VSResultsMenu_Save_Replay, this, this, "[SAVE_REPLAY]");
    mSaveReplayButton->mDrawStoneButton = false;
    mSaveReplayButton->mButtonImage = addonImages.VS_Button;
    mSaveReplayButton->mOverImage = addonImages.VS_Button_selected;
    mSaveReplayButton->mDownImage = addonImages.VS_Button_selected;

    // mSaveReplayButton->mTextOffsetX = -2;
    // mSaveReplayButton->mTextOffsetY = -4;
    // mSaveReplayButton->mTextDownOffsetX = 1;
    // mSaveReplayButton->mTextDownOffsetY = 1;
    mSaveReplayButton->SetFont(Sexy::FONT_DWARVENTODCRAFT24);
    (*mSaveReplayButton->mColors)[ButtonWidget::COLOR_LABEL] = Color(25, 197, 45);
    (*mSaveReplayButton->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = Color(277, 225, 108);
    mSaveReplayButton->mLabelJustify = BUTTON_LABEL_WRAP_CENTER;
    mSaveReplayButton->GameButton::Resize(660, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    AddWidget(mSaveReplayButton);
}

void VSResultsMenu::KillReplayButton() {
    if (mSaveReplayButton != nullptr) {
        RemoveWidget(mSaveReplayButton);
        gLawnApp->SafeDeleteWidget(mSaveReplayButton);
        mSaveReplayButton = nullptr;
    }
}

void VSResultsMenu::DrawInfoBox(Sexy::Graphics *a2, int a3) {
    Sexy::Widget *slotWidget = FindWidget(a3 + 2);
    if (slotWidget == nullptr) {
        return;
    }

    auto *infoWidget = reinterpret_cast<Sexy::ImageWidget *>(slotWidget);
    a2->PushState();
    a2->Translate(slotWidget->mX, slotWidget->mY);

    int sideInSlot = mSides[a3];
    if (sideInSlot < 0 || sideInSlot > 1) {
        a2->PopState();
        return;
    }
    int this76Side = mPlayerIndices[sideInSlot];
    int *playerRecord = GetPlayerRecord((unsigned int)this76Side);
    DefaultProfileMgr *profileMgr = gLawnApp->mProfileMgr;
    PlayerInfo *profileObj = gLawnApp->mPlayerInfo;
    if (!mIsReplaySession) {
        if (profileMgr != nullptr) {
            profileObj = profileMgr->GetProfile(profileObj->mName, this76Side);
        }
        if (playerRecord == nullptr || profileObj == nullptr) {
            a2->PopState();
            return;
        }
    } else if (playerRecord == nullptr) {
        a2->PopState();
        return;
    }
    bool isZombieSlot = (sideInSlot != 0);

    infoWidget->mImage = isZombieSlot ? Sexy::IMAGE_VS_INFO_BOX_ZOMBIES : Sexy::IMAGE_VS_INFO_BOX_PLANTS;
    a2->DrawImage(Sexy::IMAGE_NO_GAMERPIC, 31, 52);
    a2->DrawImage(isZombieSlot ? Sexy::IMAGE_VS_INFO_BOX_ZOMBIES_OVERLAY : Sexy::IMAGE_VS_INFO_BOX_PLANTS_OVERLAY, 0, 0);
    pvzstl::string playerFmt = TodStringTranslate("[PLAYER_FMT]");
    pvzstl::string playerLabel = StrFormat(playerFmt.c_str(), a3 + 1);
    const char *replayHostName = (gReplayHostName[0] != '\0') ? gReplayHostName : gServerHostName;
    const char *replayGuestName = (gReplayGuestName[0] != '\0') ? gReplayGuestName : gSecondPlayerName;
    if ((mIsReplaySession || mIsOnlineSession) && ((mIsReplaySession && replayGuestName[0] != '\0') || (!mIsReplaySession && gSecondPlayerName[0] != '\0'))) {
        const char *hostName =
            mIsReplaySession ? ((replayHostName[0] != '\0') ? replayHostName : gLawnApp->mPlayerInfo->mName) : ((gServerHostName[0] != '\0') ? gServerHostName : gLawnApp->mPlayerInfo->mName);
        const char *guestName = mIsReplaySession ? replayGuestName : gSecondPlayerName;
        playerLabel = (this76Side == 0) ? hostName : guestName;
    }

    a2->SetColor(Sexy::Color::White);
    a2->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
    a2->DrawString(playerLabel, 42, 44);
    if (!mIsReplaySession) {
        int winStreak = playerRecord[3];
        profileObj->mWinStreak = winStreak;
        if (winStreak > 1) {
            pvzstl::string streakFmt = TodStringTranslate("[WIN_STREAK_FMT]");
            a2->DrawString(StrFormat(streakFmt.c_str(), winStreak), 263, 78);
        }
    }
    float trophyX = mTrophyPosX;
    float trophyY = mTrophyPosY;
    if (mSmokeCounter > 49) {
        trophyY = (float)TodAnimateCurve(50, 60, mSmokeCounter, 82, 74, TodCurves::CURVE_EASE_IN_OUT);
    } else {
        trophyY = (float)TodAnimateCurve(0, 50, mSmokeCounter, -60 - slotWidget->mY - Sexy::IMAGE_MP_TROPHY_BASE->mHeight - Sexy::IMAGE_MP_PLANT_TROPHY->mHeight, 82, TodCurves::CURVE_LINEAR);
    }

    int winnerSide = playerRecord[0];
    if (winnerSide != -1) {
        if (TodParticleSystem *smoke = gLawnApp->ParticleTryToGet(mSmokeParticleID)) {
            smoke->Draw(a2);
        }

        Sexy::Image *topTrophy = (winnerSide != 0) ? Sexy::IMAGE_MP_ZOMBIE_TROPHY : Sexy::IMAGE_MP_PLANT_TROPHY;
        a2->DrawImage(Sexy::IMAGE_MP_TROPHY_BASE, (int)(trophyX - Sexy::IMAGE_MP_TROPHY_BASE->mWidth / 2.0f), (int)trophyY);
        a2->DrawImage(topTrophy, (int)(trophyX + 2.0f - topTrophy->mWidth / 2.0f), (int)(trophyY - 46.0f));

        if (TodParticleSystem *sparkle = gLawnApp->ParticleTryToGet(mSparkleParticleID)) {
            sparkle->SystemMove(trophyX, trophyY);
            sparkle->Draw(a2);
        }
    }
    if (!mIsReplaySession) {
        float plantTrophyX = (winnerSide == -1) ? 117.0f : 192.0f;
        int plantWins = playerRecord[1];
        if (plantWins > 0) {
            float step = 196.0f / (float)plantWins;
            if (step > 52.0f) {
                step = 52.0f;
            }
            for (int i = 0; i < plantWins; i++) {
                a2->DrawImage(Sexy::IMAGE_MP_PLANT_TROPHY, (int)plantTrophyX, 82, 40, 40);
                plantTrophyX += step;
            }
        }
        float zombieTrophyX = (winnerSide == -1) ? 117.0f : 192.0f;
        int zombieWins = playerRecord[2];
        if (zombieWins > 0) {
            float step = 196.0f / (float)zombieWins;
            if (step > 52.0f) {
                step = 52.0f;
            }
            for (int i = 0; i < zombieWins; i++) {
                a2->DrawImage(Sexy::IMAGE_MP_ZOMBIE_TROPHY, (int)zombieTrophyX, 124, 40, 40);
                zombieTrophyX += step;
            }
        }
    } else {
        if (winnerSide == 0) {
            a2->DrawImage(Sexy::IMAGE_MP_PLANT_TROPHY, 192, 82, 40, 40);
        } else if (winnerSide == 1) {
            a2->DrawImage(Sexy::IMAGE_MP_ZOMBIE_TROPHY, 192, 124, 40, 40);
        }
    }
    a2->PopState();
}
