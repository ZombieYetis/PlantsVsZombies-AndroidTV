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

#include "PvZ/Lawn/Widget/ReplayControlsWidget.h"
#include "Homura/MemberUtils.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Common/LawnCommon.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Misc.h"
#include "PvZ/NetPlay.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstring>

#include <algorithm>
#include <mutex>

using namespace Sexy;

namespace {
constexpr int kReplayButtonY = 5;
constexpr int kReplayButtonH = 28;
constexpr int kReplayButtonW = 108;
constexpr int kReplayTicksPerSecond = 100;

void ApplyReplayBackground(int vsBackground) {
    Challenge::msVSShuffleMode = false;
    Challenge::msVSClassicMode = false;
    switch (vsBackground) {
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
}

pvzstl::string FormatReplayTime(int ticks) {
    const int totalSeconds = std::max(0, ticks) / kReplayTicksPerSecond;
    return StrFormat("%d:%02d", totalSeconds / 60, totalSeconds % 60);
}

void FinishReplayIntro(Board *board) {
    if (board == nullptr || board->mApp == nullptr || board->mApp->mGameScene != GameScenes::SCENE_LEVEL_INTRO) {
        return;
    }
}

void FastForwardReplayBoard(Board *board, int targetTick, bool cancelIntroOnIntro = false) {
    if (board == nullptr) {
        return;
    }
    targetTick = std::clamp(targetTick, 0, replay::GetPlaybackDurationTicks());
    while (replay::IsPlaybackActive() && replay::GetPlaybackTick() < targetTick) {
        if (gLawnApp != nullptr && gLawnApp->mBoard != nullptr) {
            board = gLawnApp->mBoard;
        }
        replay::AdvancePlaybackOneTick();
        if (board->mApp != nullptr && board->mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO && board->mCutScene != nullptr) {
            if (cancelIntroOnIntro) {
                FinishReplayIntro(board);
                cancelIntroOnIntro = false;
            } else {
                board->mCutScene->Update();
            }
        } else {
            board->SpeedUpUpdate();
        }
    }
}

bool SeekReplayBoard(Board *board, int targetTick, bool allowRestart, bool cancelIntroOnIntro = false) {
    if (board == nullptr || !replay::IsPlaybackActive()) {
        return false;
    }

    targetTick = std::clamp(targetTick, 0, replay::GetPlaybackDurationTicks());
    if (targetTick >= replay::GetPlaybackTick()) {
        const bool wasPaused = replay::IsPlaybackPaused();
        replay::SetPlaybackPaused(false);
        FastForwardReplayBoard(board, targetTick, cancelIntroOnIntro);
        replay::SetPlaybackPaused(wasPaused);
        return false;
    }

    if (!allowRestart) {
        return false;
    }

    const std::string &path = replay::GetPlaybackFilePath();
    const int vsBackground = replay::GetPlaybackVsBackground();
    const bool wasPaused = replay::IsPlaybackPaused();
    const int speedLevel = replay::GetPlaybackSpeedLevel();
    if (path.empty() || gLawnApp == nullptr || !replay::BeginPlaybackFromFile(path)) {
        return false;
    }

    ApplyReplayBackground(vsBackground);
    gIsServerModeNetplay = true;
    gServerModeTransport = ServerModeTransport::RELAY;
    gIsServerModeSpectator = false;
    gIsReplayMode = true;
    gLawnApp->PreNewGame(GAMEMODE_MP_VS, false);
    replay::SetPlaybackSpeedLevel(speedLevel);
    replay::SetPlaybackPaused(false);
    FastForwardReplayBoard(gLawnApp->mBoard, targetTick, cancelIntroOnIntro);
    replay::SetPlaybackPaused(wasPaused);
    return true;
}
} // namespace

ReplayControlsWidget::ReplayControlsWidget(Board *board) {
    Widget::_constructor();

    static void *sReplayControlsWidgetVTable[122];
    static std::once_flag vtableInitFlag;
    std::call_once(vtableInitFlag, [this] {
        std::memcpy(sReplayControlsWidgetVTable, vTable, sizeof(sReplayControlsWidgetVTable));
        sReplayControlsWidgetVTable[0] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::_destructor);
        sReplayControlsWidgetVTable[1] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::_destructor2);
        sReplayControlsWidgetVTable[36] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::Draw);
        sReplayControlsWidgetVTable[76] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::MouseDown);
        sReplayControlsWidgetVTable[78] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::MouseDown);
        sReplayControlsWidgetVTable[79] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::MouseUp);
        sReplayControlsWidgetVTable[81] = (void *)homura::ExtractMemFuncPtr(&ReplayControlsWidget::MouseDrag);
    });
    vTable = sReplayControlsWidgetVTable;

    Resize(kX, kInitialY, kWidth, kHeight);
    mClip = false;
}

ReplayControlsWidget::~ReplayControlsWidget() {
    Widget::_destructor();
}

void ReplayControlsWidget::Draw(Graphics *g) {
    if (!gIsReplayMode || !replay::IsPlaybackActive()) {
        return;
    }

    TodDrawImageScaledF(g, IMAGE_CONVEYORBELT_BACKDROP, 0, 0, (float)mWidth / (float)IMAGE_CONVEYORBELT_BACKDROP->mWidth, (float)mHeight / (float)IMAGE_CONVEYORBELT_BACKDROP->mHeight);
    //    g->SetColor(Color(20, 20, 20, 170));
    //    g->FillRect(Rect(0, 0, mWidth, mHeight));
    //    g->SetColor(Color(255, 238, 170, 210));
    //    g->DrawRect(Rect(0, 0, mWidth, mHeight));

    TodDrawString(g, "[REPLAY]", 56, 26, FONT_DWARVENTODCRAFT18, Color(0, 205, 0, 255), DS_ALIGN_CENTER);
    DrawButton(g, PauseButtonRect(), replay::IsPlaybackPaused() ? "[REPLAY_PLAY]" : "[REPLAY_PAUSE]");
    DrawButton(g, SpeedButtonRect(), StrFormat("%s:%gx", TodStringTranslate("[REPLAY_SPEED]").c_str(), static_cast<double>(replay::GetPlaybackSpeedMultiplier())).c_str());
    DrawButton(g, ForwardButtonRect(), "+10s");
    DrawButton(g, RestartButtonRect(), "[REPLAY_RESTART]");
    if (ShowSkipSetupButton()) {
        DrawButton(g, SkipSetupButtonRect(), "[REPLAY_SKIP_CHOOSER]");
    }

    Board *board = GetCurrentBoard();
    if (board != nullptr && board->mApp != nullptr && board->mApp->mGameScene == GameScenes::SCENE_PLAYING) {
        const pvzstl::string timeText = FormatReplayTime(board->mMainCounter) + " / " + FormatReplayTime(replay::GetPlaybackBoardTicks());
        TodDrawString(g, timeText.c_str(), 718, 28, FONT_HOUSEOFTERROR16, Color(255, 255, 255, 255), DS_ALIGN_CENTER);
    }
}

void ReplayControlsWidget::MouseDown(int x, int y, int theClickCount) {
    if (!gIsReplayMode || !replay::IsPlaybackActive()) {
        return;
    }
    if (Contains(PauseButtonRect(), x, y)) {
        replay::SetPlaybackPaused(!replay::IsPlaybackPaused());
        return;
    }
    if (Contains(SpeedButtonRect(), x, y)) {
        replay::CyclePlaybackSpeed();
        return;
    }
    if (Contains(ForwardButtonRect(), x, y)) {
        const int startLevelTick = GetStartLevelTick();
        const int maxBoardForwardableTick = replay::GetPlaybackBoardTicks() - GetCurrentBoard()->mMainCounter;
        if (maxBoardForwardableTick < 1000) {
            return;
        }
        const int aTargetTick = replay::GetPlaybackTick() + 1000;
        const bool crossesStartLevel = startLevelTick > replay::GetPlaybackTick() && startLevelTick <= aTargetTick;
        if (crossesStartLevel) {
            SeekReplayBoard(GetCurrentBoard(), startLevelTick, false, true);
        } else {
            SeekReplayBoard(GetCurrentBoard(), aTargetTick, false);
        }
        return;
    }
    if (Contains(RestartButtonRect(), x, y)) {
        SeekReplayBoard(GetCurrentBoard(), 0, true);
        return;
    }
    if (ShowSkipSetupButton() && Contains(SkipSetupButtonRect(), x, y)) {
        const int startLevelTick = GetStartLevelTick();
        if (startLevelTick >= 0) {
            replay::SetPlaybackPaused(false);
            SeekReplayBoard(GetCurrentBoard(), startLevelTick, false, true);
        }
    }
}

void ReplayControlsWidget::MouseDrag(int x, int y) {}

void ReplayControlsWidget::MouseUp(int x, int y, int theClickCount) {}

bool ReplayControlsWidget::Contains(const Rect &rect, int x, int y) {
    return x >= rect.mX && x < rect.mX + rect.mWidth && y >= rect.mY && y < rect.mY + rect.mHeight;
}

void ReplayControlsWidget::DrawButton(Graphics *g, const Rect &rect, const char *label) {
    g->SetColor(Color(20, 20, 20, 170));
    g->FillRect(rect);
    g->SetColor(Color(255, 238, 170, 210));
    g->DrawRect(rect);
    TodDrawString(g, label, rect.mX + rect.mWidth / 2, rect.mY + 24, FONT_HOUSEOFTERROR16, Color(255, 255, 255), DS_ALIGN_CENTER);
}

Board *ReplayControlsWidget::GetCurrentBoard() {
    return gLawnApp->mBoard;
}

int ReplayControlsWidget::GetStartLevelTick() {
    if (mStartLevelTick == -2) {
        mStartLevelTick = replay::FindPlaybackEventTick(EVENT_SERVER_BOARD_START_LEVEL) - 312;
    }
    return mStartLevelTick;
}

bool ReplayControlsWidget::ShowSkipSetupButton() {
    Board *board = GetCurrentBoard();
    return board != nullptr && board->mApp != nullptr && board->mApp->mVSSetupMenu != nullptr;
}

Rect ReplayControlsWidget::PauseButtonRect() {
    return {112, kReplayButtonY, kReplayButtonW, kReplayButtonH};
}

Rect ReplayControlsWidget::SpeedButtonRect() {
    return {228, kReplayButtonY, kReplayButtonW, kReplayButtonH};
}

Rect ReplayControlsWidget::ForwardButtonRect() {
    return {344, kReplayButtonY, kReplayButtonW, kReplayButtonH};
}

Rect ReplayControlsWidget::RestartButtonRect() {
    return {460, kReplayButtonY, kReplayButtonW, kReplayButtonH};
}

Rect ReplayControlsWidget::SkipSetupButtonRect() {
    return {576, kReplayButtonY, kReplayButtonW, kReplayButtonH};
}

void ReplayControlsWidget::_destructor() {
    Widget::_destructor();
}

void ReplayControlsWidget::_destructor2() {
    delete this;
}
