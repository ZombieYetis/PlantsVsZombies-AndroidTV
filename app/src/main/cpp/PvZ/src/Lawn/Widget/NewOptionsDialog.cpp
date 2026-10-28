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

#include "PvZ/Lawn/Widget/NewOptionsDialog.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/ConfirmBackToMainDialog.h"
#include "PvZ/Lawn/Widget/VSResultsMenu.h"
#include "PvZ/NetPlay.h"

using namespace Sexy;

void NewOptionsDialog::AddedToManager(WidgetManager *theWidgetManager) {
    old_NewOptionsDialog_AddedToManager(this, theWidgetManager);
}

void NewOptionsDialog::RemovedFromManager(WidgetManager *theWidgetManager) {
    old_NewOptionsDialog_RemovedFromManager(this, theWidgetManager);
}

void NewOptionsDialog::ButtonDepress(int theId) {
    if (theId == NewOptionsDialog::NewOptionsDialog_MainMenu && (IsRemoteClient() || IsRemoteServer())) {
        // 对战时返回主菜单，加一层退出确认
        mApp->DoConfirmBackToMain(false);
        auto aConfirmDialog = reinterpret_cast<ConfirmBackToMainDialog *>(mApp->GetDialog(DIALOG_CONFIRM_BACK_TO_MAIN));
        *aConfirmDialog->mDialogLines = TodStringTranslate("[QUIT_WHEN_ONLINE_WARNING]");
        aConfirmDialog->mRestartButton->mDisabled = true;
        return;
    }
    if (theId == 5 && gIsReplayMode) {
        LawnApp *aApp = mApp;
        aApp->KillNewOptionsDialog();
        aApp->KillBoard();
        // 在大厅初始化控件前完整清理回放状态。
        aApp->ClearSecondPlayer();
        gChallengeScreenOpenReplayManage = true;
        aApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_VS);
        return;
    }
    if (theId == NewOptionsDialog_Restart && mApp->IsCoopMode() && IsOnlineModeActive()) {
        LawnApp *aApp = mApp;
        if (gIsServerModeSpectator || gIsReplayMode) {
            aApp->PlaySample(Sexy::SOUND_BUZZER);
            return;
        }
        aApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        Sexy::Dialog::ButtonDepress(theId);
        Board *aBoard = aApp->mBoard;
        if (aBoard == nullptr) {
            return;
        }
        Sexy::Dialog *restartDialog = aApp->DoConfirmRestartDialog();
        const int result = restartDialog->WaitForResult(false);
        if (aApp->GetDialog(DIALOG_CONFIRM_IN_GAME_RESTART) == restartDialog) {
            aApp->KillDialog(DIALOG_CONFIRM_IN_GAME_RESTART);
        }
        if (result != 1000 || aApp->mBoard != aBoard || !aApp->IsCoopMode()) {
            return;
        }
        if (IsRemoteClient()) {
            BaseEvent event = {EventType::EVENT_CLIENT_BOARD_RETRY};
            netplay::PutEvent(event);
        } else if (IsRemoteServer()) {
            aApp->RequestCoopRestart();
        }
        return;
    }
    if (theId == NewOptionsDialog_Restart && mApp->IsVSMode() && IsOnlineModeActive()) {
        if (gIsServerModeSpectator) {
            mApp->PlaySample(Sexy::SOUND_BUZZER);
            return;
        }


        mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);

        Sexy::Dialog::ButtonDepress(theId);

        if (mApp->mBoard) {


            Sexy::Dialog *restartDialog = mApp->DoConfirmRestartDialog();

            if (restartDialog->WaitForResult(false) == 1000) {
                mApp->mMusic->StopAllMusic();
                mApp->mSoundSystem->CancelPausedFoley();
                mApp->KillNewOptionsDialog();

                if (IsRemoteClient()) {
                    // 客户端点击投降
                    BaseEvent event = {EventType::EVENT_CLIENT_BOARD_CONCEDE};
                    netplay::PutEvent(event);
                    GamepadControls *clientGamepadControls = mApp->mBoard->mGamepadControls[1]->mGamepadIndex == 1 ? mApp->mBoard->mGamepadControls[1] : mApp->mBoard->mGamepadControls[0];
                    if (!clientGamepadControls->mIsZombie) {
                        mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_ZOMBIE_WON);
                        mApp->mGameScene = SCENE_ZOMBIES_WON;
                    } else {
                        mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_PLANT_WON);
                        mApp->mGameScene = SCENE_PLANTS_WON;
                    }
                }

                if (IsRemoteServer()) {
                    // 主机端点击投降
                    BaseEvent event = {EventType::EVENT_SERVER_BOARD_CONCEDE};
                    netplay::PutEvent(event);
                    GamepadControls *serverGamepadControls = mApp->mBoard->mGamepadControls[0]->mGamepadIndex == 0 ? mApp->mBoard->mGamepadControls[0] : mApp->mBoard->mGamepadControls[1];
                    if (!serverGamepadControls->mIsZombie) {
                        mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_ZOMBIE_WON);
                        mApp->mGameScene = SCENE_ZOMBIES_WON;
                    } else {
                        mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_PLANT_WON);
                        mApp->mGameScene = SCENE_PLANTS_WON;
                    }
                    const bool plantWin = (mApp->mGameScene == SCENE_PLANTS_WON);
                    netplay::MetricsSetVsBackground(int(gVSBackground));
                    netplay::MetricsSetVsMode(Challenge::msVSShuffleMode, Challenge::msVSClassicMode);
                    netplay::MetricsSendSettlement(plantWin, mApp->mBoard->mMainCounter);
                }

                mApp->ShowVSResultsScreen();
                mApp->mVSResultsMenu->InitFromBoard(mApp->mBoard);
                mApp->KillBoard();
            }
        }
        return;
    }

    old_NewOptionsDialog_ButtonDepress(this, theId);
}
