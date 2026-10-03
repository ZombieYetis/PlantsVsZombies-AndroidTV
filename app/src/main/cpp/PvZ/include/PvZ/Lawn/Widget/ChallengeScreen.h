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

#ifndef PVZ_LAWN_WIDGET_CHALLENGE_SCREEN_H
#define PVZ_LAWN_WIDGET_CHALLENGE_SCREEN_H

#include "PvZ/Lawn/Board/ToolTipWidget.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/SexyAppFramework/Misc/Curve1DUtil.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"
#include "PvZ/SexyAppFramework/Widget/Dialog.h"
#include "PvZ/Symbols.h"

#include "HelpBarWidget.h"
#include "NetplayLobbyWidget.h"

inline constexpr int NUM_CHALLENGE_MODES_ORIGINAL(int(GameMode::NUM_GAME_MODES - 1));
inline constexpr int MAX_CHALLENGE_MODES = 200;
inline constexpr int NUM_CHALLENGE_MODES_EXTENDED = MAX_CHALLENGE_MODES - NUM_CHALLENGE_MODES_ORIGINAL;
extern const int NUM_CHALLENGE_MODES;
inline constexpr int GAMEMODE_MP_VS_DAY = 70;
inline constexpr int GAMEMODE_MP_VS_NIGHT = 71;
inline constexpr int GAMEMODE_MP_VS_POOL_DAY = 72;
inline constexpr int GAMEMODE_MP_VS_POOL_NIGHT = 73;
inline constexpr int GAMEMODE_MP_VS_ROOF = 74;
inline constexpr int GAMEMODE_MP_VS_SHUFFLE_MODE = 75;
inline constexpr int NUM_VS_MODES(GAMEMODE_MP_VS_SHUFFLE_MODE - GAMEMODE_MP_VS_DAY + 1);

class NewLawnButton;
class ChallengeScreen : public Sexy::Widget, public Sexy::ButtonListener {
public:
    enum {
        ChallengeScreen_Back = 100,
        ChallengeScreen_Mode = 200,
        ChallengeScreen_Page = ChallengeScreen_Mode + MAX_CHALLENGE_MODES,
    };

public:
    Sexy::ButtonWidget *mChallengeButtons[NUM_CHALLENGE_MODES_ORIGINAL]; // 65 ~ 158
    LawnApp *mApp;                                                       // 159
    ToolTipWidget *mToolTip;                                             // 160
    ChallengePage mPage;                                                 // 161
    bool mCheatEnableChallenges;                                         // 648
    UnlockingState mUnlockState;                                         // 163
    int mUnlockStateCounter;                                             // 164
    int mUnlockChallengeIndex;                                           // 165
    float mLockShakeX;                                                   // 166
    float mLockShakeY;                                                   // 167
    Sexy::Curve1DUtil mUtil;                                             // 168 ~ 183
    HelpBarWidget *mHelpBarWidget;                                       // 184
    int mScrollPosition;                                                 // 185
    int mScrollTargetPosition;                                           // 186
    float mScrollAnimationTime;                                          // 187
    int mPageChallengeIndex[NUM_CHALLENGE_MODES_ORIGINAL];               // 188 ~ 281
    float mPageChallengeAnimTime[NUM_CHALLENGE_MODES_ORIGINAL];          // 282 ~ 375
    int mPageChallengeCount;                                             // 376
    GameMode mSelectedGameMode;                                          // 377 其值固定比mSelectedMode小2
    int mSelectedChallengeIndex;                                         // 378
    int mSurvivalCount;                                                  // 379
    // 大小380个整数, 以下是新增成员!
    NewLawnButton *mBackButton = nullptr;
    NetplayLobbyWidget *mNetplayLobbyWidget = nullptr;
    Sexy::ButtonWidget *mChallengeButtonsExtended[NUM_CHALLENGE_MODES_EXTENDED]{};
    int mPageChallengeIndexExtended[NUM_CHALLENGE_MODES_EXTENDED]{};
    float mPageChallengeAnimTimeExtended[NUM_CHALLENGE_MODES_EXTENDED]{};

    Sexy::ButtonWidget *&GetChallengeButton(int theChallengeIndex) {
        return theChallengeIndex < NUM_CHALLENGE_MODES_ORIGINAL ? mChallengeButtons[theChallengeIndex] : mChallengeButtonsExtended[theChallengeIndex - NUM_CHALLENGE_MODES_ORIGINAL];
    }
    int &GetPageChallengeIndex(int thePageIndex) {
        return thePageIndex < NUM_CHALLENGE_MODES_ORIGINAL ? mPageChallengeIndex[thePageIndex] : mPageChallengeIndexExtended[thePageIndex - NUM_CHALLENGE_MODES_ORIGINAL];
    }
    float &GetPageChallengeAnimTime(int thePageIndex) {
        return thePageIndex < NUM_CHALLENGE_MODES_ORIGINAL ? mPageChallengeAnimTime[thePageIndex] : mPageChallengeAnimTimeExtended[thePageIndex - NUM_CHALLENGE_MODES_ORIGINAL];
    }

    ChallengeScreen(LawnApp *theApp, ChallengePage thePage) {
        _constructor(theApp, thePage);
    }

    ~ChallengeScreen() {
        _destructor();
    };

    void SetUnlockChallengeIndex(ChallengePage thePage, bool theIsIZombie = false);
    void SetScrollTarget(int theIndex) {
        reinterpret_cast<void (*)(ChallengeScreen *, int)>(ChallengeScreen_SetScrollTargetAddr)(this, theIndex);
    }
    int MoreTrophiesNeeded(int theChallengeIndex);
    int AccomplishmentsNeeded(int theChallengeIndex);
    bool IsScaryPotterLevel(GameMode theGameMode);
    bool IsIZombieLevel(GameMode theGameMode);
    void UpdateToolTip();
    void KeyChar(char theChar);
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag);

    void Draw(Sexy::Graphics *g);
    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void Update();
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void ButtonPress(int theButtonId);
    void ButtonDepress(int theId);
    void UpdateButtons();
    void DrawButton(Sexy::Graphics *g, int theChallengeIndex, int theChallengeMode);
    void MouseDown(int x, int y, int theClickCount);
    void MouseUp(int x, int y);
    void MouseDrag(int x, int y);
    void KeyDown(Sexy::KeyCode theKey);
    void KeyDown_Origin(Sexy::KeyCode theKey);
    void processClientEvent(const BaseEvent *event) const;
    void processServerEvent(const BaseEvent *event);

protected:
    friend void InitHookFunction();

    void _constructor(LawnApp *theApp, ChallengePage thePage);
    void _destructor();
};

class ChallengeDefinition {
public:
    GameMode mChallengeMode;    // 0
    int mChallengeIconIndex;    // 1
    ChallengePage mPage;        // 2
    int mRow;                   // 3 无用
    int mCol;                   // 4 无用
    const char *mChallengeName; // 5
};
extern ChallengeDefinition gChallengeDefs[];

ChallengeDefinition &GetChallengeDefinition(int theChallengeMode);
bool ChallengePageHasEntry(ChallengePage thePage, int theRow, int theCol);
int GetChallengeByRowColumn(ChallengePage thePage, int theRow, int theCol);
GameMode GetModeByRowColumn(ChallengePage thePage, int theRow, int theCol);
/***************************************************************************************************************/
inline int gChallengeScreenRequestState = 0;
inline bool gChallengeScreenOpenReplayManage = false;

#endif // PVZ_LAWN_WIDGET_CHALLENGE_SCREEN_H
