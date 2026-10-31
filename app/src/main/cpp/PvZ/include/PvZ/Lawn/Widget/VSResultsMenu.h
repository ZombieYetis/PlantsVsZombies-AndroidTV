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

#ifndef PVZ_LAWN_WIDGET_VS_RESULTS_MENU_H
#define PVZ_LAWN_WIDGET_VS_RESULTS_MENU_H

#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Common/LawnCommon.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Widget/Checkbox.h"
#include "PvZ/SexyAppFramework/Widget/CheckboxListener.h"
#include "PvZ/SexyAppFramework/Widget/MenuWidget.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstddef>

inline constexpr int BUTTON_LABEL_WRAP_CENTER = 2;
inline constexpr int BUTTON_Y = 472;
inline constexpr int BUTTON_WIDTH = 205;
inline constexpr int BUTTON_HEIGHT = 96;

class VSResultsMenu : public Sexy::MenuWidget {
public:
    enum {
        VSResultsMenu_Play_Again = 0,
        VSResultsMenu_Quit_VS = 1,
        VSResultsMenu_Info_Box_P1 = 2,
        VSResultsMenu_Info_Box_P2 = 3,
        VSResultsMenu_Plant_Side = 4,
        VSResultsMenu_Plant_Side_Front = 5,
        VSResultsMenu_Zombie_Side = 6,
        VSResultsMenu_Zombie_Side_Front = 7,
        VSResultsMenu_Win_Image = 8,
        VSResultsMenu_Back = 9,
        VSResultsMenu_Save_Replay = 1200,
    };

    enum WinSide {
        INVALID_WIN_SIDE = -1,
        WIN_SIDE_PLANT = 0,
        WIN_SIDE_ZOMBIE = 1,
    };

    // [记录槽] -> [胜负方, 植物胜场, 僵尸胜场, 连胜数, playerIndex2]
    static int (&msPlayerRecords)[2][5];

    int unk[3];                          // 70 ~ 72
    int mSparkleCounter;                 // 73
    int mSmokeCounter;                   // 74
    int mResultsButtonId;                // 75
    int mPlayerIndices[2];               // 76 ~ 77, P1 / P2 的 playerIndex2
    WinSide mSides[2];                   // 78 ~ 79
    float mTrophyPosX;                   // 80, 奖杯中心 X
    float mTrophyPosY;                   // 81, 奖杯中心 Y
    ParticleSystemID mSparkleParticleID; // 82
    ParticleSystemID mSmokeParticleID;   // 83
    int mUpdateCounter;                  // 84
    int mBoardMainCounter = 0;
    BackgroundType mBoardBackground = BackgroundType::BACKGROUND_1_DAY;
    SeedType mPlantSeeds[6] = {SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE};
    SeedType mZombieSeeds[6] = {SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE, SeedType::SEED_NONE};
    bool mIsReplaySession = false;
    bool mIsOnlineSession = false;
    int mVSResultsCounter = 0;
    class VSResultsCheckboxController *mCheckboxController = nullptr;
    GameButton *mBackButton = nullptr;
    GameButton *mSaveReplayButton = nullptr;

    int *GetPlayerRecord(unsigned int playerIndex) {
        return reinterpret_cast<int *(*)(VSResultsMenu *, unsigned int)>(VSResultsMenu_GetPlayerRecordAddr)(this, playerIndex);
    }

    VSResultsMenu() {
        _constructor();
    }
    ~VSResultsMenu() = delete;

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    static void ClearPlayerRecords();
    void Update();
    void OnExit() const;
    void ButtonDepress(int theId);
    void Draw(Sexy::Graphics *g);
    void DrawInfoBox(Sexy::Graphics *a2, int a3);
    void HideReplayButton(bool forceHide);
    void HandleOpponentDisconnected();
    void InitFromBoard(Board *theBoard);
    bool SaveReplay();
    void ShowReplayButton();
    void UpdateReplayButtonState();
    void KillReplayButton();

    void processClientEvent(const BaseEvent *event);
    void processServerEvent(const BaseEvent *event);

protected:
    friend void InitHookFunction();

    void _constructor();
    void _destructor();
};

class VSResultsCheckboxController final : public Sexy::CheckboxListener {
public:
    enum {
        VSResultsMenu_Send_Player_Name = 100,
        VSResultsMenu_Auto_Save_Replay = 101,
    };

    Sexy::Checkbox *mSendPlayerNameCheckbox;
    Sexy::Checkbox *mAutoSaveReplayCheckbox;

    VSResultsCheckboxController()
        : Sexy::CheckboxListener()
        , mSendPlayerNameCheckbox(nullptr)
        , mAutoSaveReplayCheckbox(nullptr)
        , mParentMenu(nullptr) {}

    void CheckboxChecked(int theId, bool checked) override {
        if (theId == VSResultsMenu_Send_Player_Name) {
            gLawnApp->mPlayerInfo->mVSResultsSendPlayerName = checked;
            gLawnApp->mPlayerInfo->SaveDetails();
        } else if (theId == VSResultsMenu_Auto_Save_Replay) {
            gLawnApp->mPlayerInfo->mVSResultsAutoSaveReplay = checked;
            gLawnApp->mPlayerInfo->SaveDetails();
            if (checked && mParentMenu != nullptr && mParentMenu->mSaveReplayButton != nullptr && mParentMenu->mSaveReplayButton->mVisible && !mParentMenu->mSaveReplayButton->mDisabled) {
                mParentMenu->SaveReplay();
            }
        }
    }

    void InitCheckboxWidget(VSResultsMenu *parentMenu) {
        mParentMenu = parentMenu;
        if (mParentMenu == nullptr || mParentMenu->mWidgetManager == nullptr || mParentMenu->mIsReplaySession) {
            return;
        }
        if (mSendPlayerNameCheckbox != nullptr || mAutoSaveReplayCheckbox != nullptr) {
            return;
        }
        mSendPlayerNameCheckbox = MakeNewCheckbox(VSResultsMenu_Send_Player_Name, this, mParentMenu, false);
        mSendPlayerNameCheckbox->Resize(-60, 580, 175, 50);
        mSendPlayerNameCheckbox->SetChecked(gLawnApp->mPlayerInfo->mVSResultsSendPlayerName, false);
        mParentMenu->AddWidget(mSendPlayerNameCheckbox);

        mAutoSaveReplayCheckbox = MakeNewCheckbox(VSResultsMenu_Auto_Save_Replay, this, mParentMenu, false);
        mAutoSaveReplayCheckbox->Resize(660, 580, 205, 50);
        mAutoSaveReplayCheckbox->SetChecked(gLawnApp->mPlayerInfo->mVSResultsAutoSaveReplay, false);
        mParentMenu->AddWidget(mAutoSaveReplayCheckbox);

        SetSendPlayerNameCheckboxVisible(gIsServerModeNetplay && gServerModeTransport == ServerModeTransport::RELAY && !gIsServerModeSpectator);
        SetAutoSaveReplayCheckboxVisible((IsRemoteClient() || IsRemoteServer()) && !gIsServerModeSpectator);
    }

    void DrawCheckboxLabel(Sexy::Graphics *g) const {
        if (g == nullptr) {
            return;
        }
        g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
        if (mSendPlayerNameCheckbox != nullptr && mSendPlayerNameCheckbox->mVisible) {
            const Sexy::Color color = (mParentMenu != nullptr && mParentMenu->mFocusedChildWidget == mSendPlayerNameCheckbox) ? Sexy::Color(0, 255, 0) : Sexy::Color(107, 110, 145);
            g->SetColor(color);
            g->DrawString(TodStringTranslate("[SEND_PLAYER_NAME]"), mSendPlayerNameCheckbox->mX + 40, mSendPlayerNameCheckbox->mY + 26);
        }
        if (mAutoSaveReplayCheckbox != nullptr && mAutoSaveReplayCheckbox->mVisible) {
            const Sexy::Color color = (mParentMenu != nullptr && mParentMenu->mFocusedChildWidget == mAutoSaveReplayCheckbox) ? Sexy::Color(0, 255, 0) : Sexy::Color(107, 110, 145);
            g->SetColor(color);
            g->DrawString(TodStringTranslate("[AUTO_SAVE_REPLAY]"), mAutoSaveReplayCheckbox->mX + 40, mAutoSaveReplayCheckbox->mY + 26);
        }
    }

    void HideCheckboxWidget() {
        if (mSendPlayerNameCheckbox != nullptr) {
            mSendPlayerNameCheckbox->SetVisible(false);
            mSendPlayerNameCheckbox->mDisabled = true;
        }
        if (mAutoSaveReplayCheckbox != nullptr) {
            mAutoSaveReplayCheckbox->SetVisible(false);
            mAutoSaveReplayCheckbox->mDisabled = true;
        }
    }

    void SetSendPlayerNameCheckboxVisible(bool visible) {
        if (mSendPlayerNameCheckbox == nullptr) {
            return;
        }
        mSendPlayerNameCheckbox->SetVisible(visible);
        mSendPlayerNameCheckbox->mDisabled = !visible;
    }

    void SetAutoSaveReplayCheckboxVisible(bool visible) {
        if (mAutoSaveReplayCheckbox == nullptr) {
            return;
        }
        mAutoSaveReplayCheckbox->SetVisible(visible);
        mAutoSaveReplayCheckbox->mDisabled = !visible;
    }

    void DestroyCheckboxWidget() {
        if (mSendPlayerNameCheckbox != nullptr) {
            if (mParentMenu != nullptr && mParentMenu->mWidgetManager != nullptr) {
                mParentMenu->RemoveWidget(mSendPlayerNameCheckbox);
            }
            gLawnApp->SafeDeleteWidget(mSendPlayerNameCheckbox);
            mSendPlayerNameCheckbox = nullptr;
        }
        if (mAutoSaveReplayCheckbox != nullptr) {
            if (mParentMenu != nullptr && mParentMenu->mWidgetManager != nullptr) {
                mParentMenu->RemoveWidget(mAutoSaveReplayCheckbox);
            }
            gLawnApp->SafeDeleteWidget(mAutoSaveReplayCheckbox);
            mAutoSaveReplayCheckbox = nullptr;
        }
        mParentMenu = nullptr;
    }

private:
    VSResultsMenu *mParentMenu;
};

inline int gVSResultRequestState = -1;

inline void (*old_VSResultsMenu_Update)(VSResultsMenu *a);

inline void (*old_VSResultsMenu_Draw)(VSResultsMenu *, Sexy::Graphics *);

inline void (*old_VSResultsMenu_DrawInfoBox)(VSResultsMenu *a, Sexy::Graphics *a2, int a3);

inline void (*old_VSResultsMenu_Constructor)(VSResultsMenu *);

inline void (*old_VSResultsMenu_AddedToManager)(VSResultsMenu *, Sexy::WidgetManager *);

inline void (*old_VSResultsMenu_RemovedFromManager)(VSResultsMenu *, Sexy::WidgetManager *);

inline void (*old_VSResultsMenu_Destructor)(VSResultsMenu *);

#endif // PVZ_LAWN_WIDGET_VS_RESULTS_MENU_H
