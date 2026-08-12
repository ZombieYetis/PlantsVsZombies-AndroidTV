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

#include "PvZ/Lawn/Widget/MainMenu.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/MailScreen.h"
#include "PvZ/PatchList.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Graphics/MemoryImage.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <cstddef>
#include <cstring>

#include <algorithm>

using namespace Sexy;

namespace {
bool isPatched;
int gMainMenuAchievementCounter;
int gMainMenuAchievementsWidgetY;
int gMainMenuAchievementsKeyboardScrollWidgetY;
int gMainMenuAchievementKeyboardScrollCounter;
bool gMainMenuAchievementKeyboardScrollDirection;

AchievementsWidget *gMainMenuAchievementsWidget;
GameButton *gMainMenuAchievementsBack;
int gFoleyVolumeCounter;

} // namespace

void MainMenu::_constructor(LawnApp *theApp) {
    old_MainMenu_MainMenu(this, theApp);

    mVSModeLocked = false; // 直接解锁对战模式
}

FoleyType MainMenu::GetFoleyTypeByScene(int theScene) {
    switch (theScene) {
        case 0:
            return FoleyType::FOLEY_MENU_LEFT;
        case 1:
            return FoleyType::FOLEY_MENU_CENTRE;
        case 2:
            return FoleyType::FOLEY_MENU_RIGHT;
        default:
            return FoleyType::FOLEY_MENU_CENTRE;
    }
}

void MainMenu::Update() {
    // 成就界面处理
    isMainMenu = true;
    requestDrawShovelInCursor = false;
    requestDrawButterInCursor = false;

    if (!isPatched) {
        patchlist::autoPickupSeedPacketDisable.Modify();
        isPatched = true;
    }

    // 首次启动游戏时指导玩家重命名
    if (!mApp->mPlayerInfo->mRenamed) {
        if (mApp->mPlayerInfo->mName && std::strncmp(mApp->mPlayerInfo->mName, "Player", 6) == 0) {
            mApp->DoUserDialog();
            mApp->DoRenameUserDialog(mApp->mPlayerInfo->mName);
        }
        mApp->mPlayerInfo->mRenamed = true;
        mApp->mPlayerInfo->SaveDetails();
    }

    // 白噪音播放和淡入淡出
    if (mIsFading) {
        float num = mFadeCounterFloat + 0.005f;
        mFadeCounterFloat = fmin(num, 1.0f);
    } else {

        float theSoundVolume = mApp->mPlayerInfo == nullptr ? 1.0f : mApp->mPlayerInfo->mSoundVolume;

        if (InTransition()) {
            gFoleyVolumeCounter++;
            FoleyType aType = GetFoleyTypeByScene(mMenuScene);
            FoleyType aNextType = GetFoleyTypeByScene(mTargetMenuScene);
            if (!mApp->mSoundSystem->IsFoleyPlaying(aNextType)) {
                mApp->PlayFoley(aNextType);
                mApp->SetFoleyVolume(aNextType, 0);
            }
            float theVolume = TodAnimateCurveFloat(0, 93, gFoleyVolumeCounter, theSoundVolume, 0, TodCurves::CURVE_BOUNCE_SLOW_MIDDLE);
            if (gFoleyVolumeCounter >= 46) {
                mApp->SetFoleyVolume(aNextType, theVolume);
                if (mApp->mSoundSystem->IsFoleyPlaying(aType)) {
                    mApp->mSoundSystem->StopFoley(aType);
                }
            } else {
                mApp->SetFoleyVolume(aType, theVolume);
            }
        } else {
            gFoleyVolumeCounter = 0;
            FoleyType aType = GetFoleyTypeByScene(mMenuScene);
            if (gAchievementState == NOT_SHOWING) {
                if (!mApp->mSoundSystem->IsFoleyPlaying(aType) && mExitCounter == 0) {
                    // mApp->PlayFoley(aType);
                    mApp->SetFoleyVolume(aType, 0);
                }
                if (mEnterReanimationCounter > 0) {
                    float theVolume = TodAnimateCurveFloat(110, 0, mEnterReanimationCounter, 0, theSoundVolume, TodCurves::CURVE_LINEAR);
                    mApp->SetFoleyVolume(aType, theVolume);
                }
            }
            if (gAchievementState == SLIDING_IN) {
                float theVolume = TodAnimateCurveFloat(100, 0, gMainMenuAchievementCounter, theSoundVolume, 0, TodCurves::CURVE_LINEAR);
                mApp->SetFoleyVolume(aType, theVolume);
            }
            if (gAchievementState == SLIDING_OUT && gMainMenuAchievementCounter <= 100) {
                float theVolume = TodAnimateCurveFloat(100, 0, gMainMenuAchievementCounter, 0, theSoundVolume, TodCurves::CURVE_LINEAR);
                mApp->SetFoleyVolume(aType, theVolume);
            }
        }
    }


    if (gMainMenuAchievementKeyboardScrollCounter != 0) {
        gMainMenuAchievementKeyboardScrollCounter--;
        if (gMainMenuAchievementsWidget != nullptr) {
            int theY = TodAnimateCurve(KEYBOARD_SCROLL_TIME, 0, gMainMenuAchievementKeyboardScrollCounter, 0, 192, TodCurves::CURVE_LINEAR);
            int theNewY = gMainMenuAchievementsKeyboardScrollWidgetY - (gMainMenuAchievementKeyboardScrollDirection ? theY : -theY);
            if (theNewY > MAIN_MENU_HEIGHT)
                theNewY = MAIN_MENU_HEIGHT;
            if (theNewY < 720 + MAIN_MENU_HEIGHT - (ACHIEVEMENT_HOLE_LENGTH + 1) * addonImages.hole->mHeight)
                theNewY = 720 + MAIN_MENU_HEIGHT - (ACHIEVEMENT_HOLE_LENGTH + 1) * addonImages.hole->mHeight;
            gMainMenuAchievementsWidget->Move(gMainMenuAchievementsWidget->mX, theNewY);
        }
    }

    if (gAchievementState == SLIDING_IN) {
        gMainMenuAchievementCounter--;
        if (gMainMenuAchievementsWidget != nullptr) {
            int theY = TodAnimateCurve(100, 0, gMainMenuAchievementCounter, 660, -60, TodCurves::CURVE_EASE_IN_OUT);
            Move(mX, -720 + theY);
        }
        if (gMainMenuAchievementCounter == 0) {
            gAchievementState = SHOWING;
            gMainMenuAchievementsBack = MakeButton(ACHIEVEMENTS_BACK_BUTTON, this, this, "[CLOSE]");
            gMainMenuAchievementsBack->Resize(1000, 564 + 720, 170, 50);
            AddWidget(gMainMenuAchievementsBack);
        }
    }

    if (gAchievementState == SLIDING_OUT) {
        gMainMenuAchievementCounter--;
        if (gMainMenuAchievementsWidget != nullptr) {
            if (gMainMenuAchievementCounter <= 100) {
                int theY = TodAnimateCurve(100, 0, gMainMenuAchievementCounter, -780, -60, TodCurves::CURVE_EASE_IN_OUT);
                Move(mX, theY);
            } else {
                int theAchievementsY = TodAnimateCurve(150, 100, gMainMenuAchievementCounter, gMainMenuAchievementsWidgetY, MAIN_MENU_HEIGHT, TodCurves::CURVE_EASE_IN_OUT);
                gMainMenuAchievementsWidget->Move(gMainMenuAchievementsWidget->mX, theAchievementsY);
            }
        }
        if (gMainMenuAchievementCounter == 0) {
            gAchievementState = NOT_SHOWING;
            RemoveWidget(gMainMenuAchievementsWidget);
            mApp->SafeDeleteWidget(gMainMenuAchievementsWidget);
            gMainMenuAchievementsWidget = nullptr;
            if (gMainMenuAchievementsBack != nullptr) {
                RemoveWidget(gMainMenuAchievementsBack);
                mApp->SafeDeleteWidget(gMainMenuAchievementsBack);
                gMainMenuAchievementsBack = nullptr;
            }
            Sexy::Widget *achievementsButton = FindWidget(ACHIEVEMENTS_BUTTON);
            mFocusedChildWidget = achievementsButton;
            if (!mIsFading)
                mApp->PlayFoley(FoleyType::FOLEY_MENU_CENTRE);
        }
    }
    if (gAchievementState == SHOWING) {
        return;
    }

    old_MainMenu_Update(this);
}

void MainMenu::ButtonPress(int theSelectedButton) {
    // 按下按钮的声音
    if (gLawnApp->mGameSelector->InTransition())
        return;

    switch (theSelectedButton) {
        case HOUSE_BUTTON:
        case ACHIEVEMENTS_BUTTON:
        case HELP_AND_OPTIONS_BUTTON:
        case UNLOCK_BUTTON:
        case RETURN_TO_ARCADE_BUTTON:
        case MORE_BUTTON:
        case BACK_POT_BUTTON:
        case STORE_BUTTON:
        case ZEN_BUTTON:
        case ALMANAC_BUTTON:
        case MAIL_BUTTON:
            gLawnApp->PlayFoley(FoleyType::FOLEY_CERAMIC);
            break;
        default:
            gLawnApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
            break;
    }
}

void MainMenu::ButtonDepress(int theSelectedButton) {
    // 为1.1.5解锁触控或确认键进入“更多游戏模式”
    if (InTransition())
        return;
    if (mIsFading)
        return;
    if (mEnterReanimationCounter > 0)
        return;
    if (gAchievementState == SLIDING_IN || gAchievementState == SLIDING_OUT)
        return; // 在进入、退出成就时不允许玩家操作
    if (theSelectedButton == MORE_WAYS_BUTTON) {
        // 如果当前选中的按钮为"更多游戏方式"
        SetScene(MENUSCENE_MORE_WAYS);
        return;
    }

    // 为1.1.1添加触控或确认进入对战结盟模式，并检测是否解锁对战结盟
    LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
    switch (theSelectedButton) {
        case ADVENTURE_BUTTON:
        case START_ADVENTURE_BUTTON:
            StartAdventureMode();
            if (aPlayerInfo->GetFlag(4096) && aPlayerInfo->mLevel == 35) {
                mPressedButtonId = STORE_BUTTON;
                unkBool3 = true;
                Exit();
            } else {
                mPressedButtonId = ADVENTURE_BUTTON;
                mApp->mGameMode = GameMode::GAMEMODE_ADVENTURE;
                Exit();
            }
            return;
        case VS_BUTTON: // 如果按下了对战按钮
            if (mVSModeLocked) {
                // 如果没解锁结盟（冒险2-1解锁）
                mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[MODE_LOCKED]", "[VS_LOCKED_MESSAGE]", "[DIALOG_BUTTON_OK]", "", 3);
                return;
            }
            mPressedButtonId = theSelectedButton;
            Exit();
            return;
        case VS_COOP_BUTTON: // 如果按下了结盟按钮
            if (mCoopModeLocked) {
                // 如果没解锁结盟（冒险2-1解锁）
                mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[MODE_LOCKED]", "[COOP_LOCKED_MESSAGE]", "[DIALOG_BUTTON_OK]", "", 3);
                return;
            }
            mPressedButtonId = theSelectedButton;
            Exit();
            return;
        case ACHIEVEMENTS_BUTTON:
        case ACHIEVEMENTS_BACK_BUTTON:
            if (gMainMenuAchievementsWidget == nullptr) {
                gAchievementState = SLIDING_IN;
                gMainMenuAchievementCounter = 100;
                gMainMenuAchievementsWidget = new AchievementsWidget{mApp};
                gMainMenuAchievementsWidget->mIsScrolling = false;
                gMainMenuAchievementsWidget->Resize(0, MAIN_MENU_HEIGHT, 1280, addonImages.hole->mHeight * (ACHIEVEMENT_HOLE_LENGTH + 1));
                gMainMenuAchievementsWidget->mWidgetId = ACHIEVEMENTS_BUTTON;
                AddWidget(gMainMenuAchievementsWidget);
            } else {
                gAchievementState = SLIDING_OUT;
                gMainMenuAchievementCounter = gMainMenuAchievementsWidget->mY == MAIN_MENU_HEIGHT ? 100 : 150;
                gMainMenuAchievementsWidgetY = gMainMenuAchievementsWidget->mY;
            }
            return;
        case HOUSE_BUTTON:
            mPressedButtonId = theSelectedButton;
            Exit();
            return;
        case UNLOCK_BUTTON:
            mPressedButtonId = theSelectedButton;
            Exit();
            return;
        default:
            old_MainMenu_ButtonDepress(this, theSelectedButton);
            return;
    }
}

void MainMenu::KeyDown(Sexy::KeyCode theKey) {
    // 为1.1.5解锁左方向键进入“更多游戏模式”
    if (InTransition())
        return;
    if (mIsFading)
        return;
    if (mEnterReanimationCounter > 0)
        return;

    if (gMainMenuAchievementsWidget != nullptr) {
        if (gAchievementState != SHOWING)
            return;
        if (theKey == Sexy::KEYCODE_ESCAPE || theKey == Sexy::KEYCODE_GAMEPAD_B) {
            MainMenu::ButtonDepress(ACHIEVEMENTS_BUTTON);
        } else if (theKey == Sexy::KEYCODE_UP || theKey == Sexy::KEYCODE_DOWN) {
            if (gMainMenuAchievementKeyboardScrollCounter != 0) {
                return;
                // int theNewY = gMainMenuAchievementsKeyboardScrollWidgetY -(gMainMenuAchievementKeyboardScrollDirection ? 192 : -192);
                // if (theNewY > MAIN_MENU_HEIGHT) theNewY = MAIN_MENU_HEIGHT;
                // if (theNewY < 720 +MAIN_MENU_HEIGHT - (ACHIEVEMENT_HOLE_LENGTH + 1) * addonImages.hole->mHeight) theNewY =  720 +MAIN_MENU_HEIGHT - (ACHIEVEMENT_HOLE_LENGTH + 1) *
                // addonImages.hole->mHeight; Sexy_Widget_Move(gMainMenuAchievementsWidget, gMainMenuAchievementsWidget->mX, theNewY);
            }
            gMainMenuAchievementKeyboardScrollCounter = KEYBOARD_SCROLL_TIME;
            gMainMenuAchievementsKeyboardScrollWidgetY = gMainMenuAchievementsWidget->mY;
            gMainMenuAchievementKeyboardScrollDirection = theKey == Sexy::KEYCODE_DOWN;
        }
        return;
    }

    auto mSelectedButtonId = MainMenuButtonId(mFocusedChildWidget->mWidgetId);
    if ((mSelectedButtonId == ADVENTURE_BUTTON || mSelectedButtonId == MORE_WAYS_BUTTON || mSelectedButtonId == START_ADVENTURE_BUTTON) && theKey == Sexy::KEYCODE_LEFT) {
        // 如果当前选中的按钮为"冒险模式"或者为"更多游戏方式"，同时玩家又按下了左方向键
        SetScene(MENUSCENE_MORE_WAYS);
        return;
    }

    old_MainMenu_KeyDown(this, theKey);
}

void MainMenu::UpdateHouseReanim() const {
    Reanimation *aMainMenuReanim = mApp->ReanimationTryToGet(mMainMenuReanimID);
    if (aMainMenuReanim == nullptr)
        return;
    aMainMenuReanim->SetImageDefinition("leaderboards", addonImages.leaderboards);
    if (!showHouse)
        return;
    aMainMenuReanim->SetImageOrigin("Hood1", addonImages.hood1_house);
    aMainMenuReanim->SetImageOrigin("Hood2", addonImages.hood2_house);
    aMainMenuReanim->SetImageOrigin("Hood3", addonImages.hood3_house);
    aMainMenuReanim->SetImageOrigin("Hood4", addonImages.hood4_house);
    aMainMenuReanim->SetImageOrigin("ground color copy", addonImages.house_hill_house);
    Reanimation *aHouseReanim = mApp->ReanimationTryToGet(mHouseReanimID);
    if (aHouseReanim == nullptr)
        return;
    mApp->SetHouseReanim(aHouseReanim);
}

void MainMenu::SyncProfile(bool a2) {
    // LOGD("MainMenu_SyncProfile");
    old_MainMenu_SyncProfile(this, a2);
    mApp->mNewIs3DAccelerated = mApp->mPlayerInfo == nullptr || !mApp->mPlayerInfo->mIs3DAcceleratedClosed;

    // 去除道具教学关卡
    if (mApp->mPlayerInfo != nullptr) {
        mApp->mPlayerInfo->mPassedShopSeedTutorial = true; // 标记玩家已经通过1-1的道具教学关卡
    }
}


constexpr int mZombatarButtonX = 2800;
constexpr int mZombatarButtonY = -20;

void MainMenu::EnableButtons() {
    Sexy::Widget *achievementsButton = FindWidget(ACHIEVEMENTS_BUTTON);
    ((GameButton *)achievementsButton)->SetDisabled(false);
    Sexy::Widget *leaderboardsButton = FindWidget(HOUSE_BUTTON);
    leaderboardsButton->SetVisible(true);
    Sexy::Widget *helpButton = FindWidget(HELP_AND_OPTIONS_BUTTON);
    Sexy::Widget *backButton = FindWidget(RETURN_TO_ARCADE_BUTTON);
    helpButton->mFocusLinks[3] = backButton;
    backButton->mFocusLinks[2] = helpButton;
    Sexy::Widget *zombatarButton = FindWidget(UNLOCK_BUTTON);
    ((GameButton *)zombatarButton)->SetDisabled(false);
    ((GameButton *)zombatarButton)->mButtonImage = addonImages.SelectorScreen_WoodSign3;
    ((GameButton *)zombatarButton)->mDownImage = addonImages.SelectorScreen_WoodSign3_press;
    ((GameButton *)zombatarButton)->mOverImage = addonImages.SelectorScreen_WoodSign3_press;
    zombatarButton->mFocusLinks[0] = FindWidget(BACK_POT_BUTTON);
    zombatarButton->mFocusLinks[1] = zombatarButton->mFocusLinks[0];
    zombatarButton->mFocusLinks[2] = zombatarButton->mFocusLinks[0];
    zombatarButton->mFocusLinks[3] = zombatarButton->mFocusLinks[0];
    zombatarButton->Resize(addonImages.SelectorScreen_WoodSign3->mWidth / 2, 0, addonImages.SelectorScreen_WoodSign3->mWidth, addonImages.SelectorScreen_WoodSign3->mHeight);
    Reanimation *mainMenuReanim = mApp->ReanimationTryToGet(mMainMenuReanimID);
    if (mainMenuReanim == nullptr)
        return;
    mainMenuReanim->HideTrack("unlock stem", true);

    int index[3] = {mainMenuReanim->FindTrackIndex("unlock"), mainMenuReanim->FindTrackIndex("unlock pressed"), mainMenuReanim->FindTrackIndex("unlock selected")};

    for (int i : index) {
        ReanimatorTrack *reanimatorTrack = mainMenuReanim->mDefinition->mTracks + i;
        int mTransformCount = reanimatorTrack->mTransformCount;
        for (int j = 0; j < mTransformCount; ++j) {
            reanimatorTrack->mTransforms[j].mTransX = mZombatarButtonX;
            reanimatorTrack->mTransforms[j].mTransY = mZombatarButtonY;
        }
    }

    //    static const char *names[3] = {"survival button", "survival selected", "survival pressed"};
    //    for (int i = 0; i < 3; ++i) {
    //        ReanimatorTrack *reanimatorTrack = mainMenuReanim->mDefinition->mTracks + mainMenuReanim->FindTrackIndex(names[i]);
    //        int mTransformCount = reanimatorTrack->mTransformCount;
    //        for (int j = 0; j < mTransformCount; ++j) {
    //            LOG_DEBUG("{}: ({}, {})", names[i], reanimatorTrack->mTransforms[j].mTransX, reanimatorTrack->mTransforms[j].mTransY);
    //        }
    //    }

    // if (mainMenu->mPressedButtonId == UNLOCK_BUTTON) {
    // LOGD("123123213");
    // Reanimation *mainMenuReanim = ReanimationTryToGet(mainMenu->mApp, mainMenu->mMainMenuReanimID);
    // if (mainMenuReanim != nullptr) {
    // int index = Reanimation_FindTrackIndex(mainMenuReanim, "unlock");
    // ReanimatorTrack *reanimatorTrack = mainMenuReanim->mDefinition->mTracks + index;
    // int mTransformCount = reanimatorTrack->mTransformCount;
    // int theX = mZombatarButtonX + addonImages.SelectorScreen_WoodSign3->mWidth;
    // for (int j = 0; j < mTransformCount; ++j) {
    // reanimatorTrack->mTransforms[j].mTransX = theX;
    // }
    // }
    // }
}

void MainMenu::Enter() {
    old_MainMenu_Enter(this);
    UpdateHouseReanim();
    // 解除成就按钮的禁用状态
    EnableButtons();
}

bool MainMenu::UpdateExit() {
    return old_MainMenu_UpdateExit(this);
}

void MainMenu::Exit() {
    old_MainMenu_Exit(this);
    // 解除成就按钮的禁用状态
    EnableButtons();
}

void MainMenu::OnExit() {
    mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_MENU_LEFT);
    mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_MENU_CENTRE);
    mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_MENU_RIGHT);

    if (mPressedButtonId == HOUSE_BUTTON) {
        mApp->KillMainMenu();
        mApp->ShowLeaderboards();
    }

    if (mPressedButtonId == UNLOCK_BUTTON) {
        mApp->KillMainMenu();
        mApp->ShowZombatarScreen();
    }

    if (mPressedButtonId == VS_BUTTON) {
        mApp->KillMainMenu();
        mApp->mVsInitialPlantMode = 0;
        mApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_VS); // 为对战模式新增选择战场界面

        if (mRetainWidgetsOnExit) {
            mIsFading = false;
            mRetainWidgetsOnExit = false;
            Enter();
        }
        return;
    }

    old_MainMenu_OnExit(this);
}

void MainMenu::OnScene(int theScene) {
    old_MainMenu_OnScene(this, theScene);
}

void MainMenu::SyncButtons() {
    // 解除成就按钮的禁用状态,同时刷新房子
    old_MainMenu_SyncButtons(this);
    UpdateHouseReanim();
    EnableButtons();
}

namespace {
int theOffsetX = 1792;
int theOffsetY = 220;
int theOffsetX1 = 237;
int theOffsetY1 = 60;
} // namespace

void MainMenu::UpdateCameraPosition() {
    old_MainMenu_UpdateCameraPosition(this);
    if (showHouse) {
        Reanimation *houseAnim = mApp->ReanimationTryToGet(mHouseReanimID);
        if (houseAnim != nullptr) {
            houseAnim->SetPosition(mCameraPositionX + theOffsetX, mCameraPositionY + theOffsetY);
        }
    }
}

void MainMenu::AddedToManager(WidgetManager *theWidgetManager) {
    old_MainMenu_AddedToManager(this, theWidgetManager);
    if (!showHouse)
        return;
    Reanimation *reanimation = mApp->AddReanimation(0, 0, 0, ReanimationType::REANIM_LEADERBOARDS_HOUSE);
    // Reanimation *reanimation = LawnApp_AddReanimation(mainMenu->mApp, mainMenu->mCameraPositionX + theOffsetX,mainMenu->mCameraPositionY + theOffsetY, 0,
    // ReanimationType::REANIM_LEADERBOARDS_HOUSE);
    reanimation->mCustomFilterEffectColor = {142, 146, 232, 92};
    reanimation->mFilterEffect = FilterEffect::FILTEREFFECT_CUSTOM;


    mApp->SetHouseReanim(reanimation);
    mHouseReanimID = mApp->ReanimationGetID(reanimation);
}

void MainMenu::RemovedFromManager(WidgetManager *theWidgetManager) {
    if (gMainMenuAchievementsWidget != nullptr) {
        RemoveWidget(gMainMenuAchievementsWidget);
    }
    if (gMainMenuAchievementsBack != nullptr) {
        RemoveWidget(gMainMenuAchievementsBack);
    }

    old_MainMenu_RemovedFromManager(this, theWidgetManager);
}

void MainMenu::_destructor2() {
    if (gMainMenuAchievementsWidget != nullptr) {
        delete gMainMenuAchievementsWidget;
        gMainMenuAchievementsWidget = nullptr;
    }
    if (gMainMenuAchievementsBack != nullptr) {
        delete gMainMenuAchievementsBack;
        gMainMenuAchievementsBack = nullptr;
    }

    old_MainMenu_Delete2(this);
}

void MainMenu::Draw(Sexy::Graphics *g) {
    // 实现绘制房子
    if (!showHouse) {
        old_MainMenu_Draw(this, g);
        return;
    }

    if (mWidgetManager == nullptr)
        return;
    if (mApp->GetVTable()->GetDialog(mApp, DIALOG_STORE) || mApp->GetVTable()->GetDialog(mApp, DIALOG_ALMANAC) || mApp->GetVTable()->GetDialog(mApp, DIALOG_MAIL))
        return;

    Reanimation *mainMenuReanim = mApp->ReanimationTryToGet(mMainMenuReanimID);
    if (mainMenuReanim == nullptr)
        return;
    Reanimation *skyReanim = mApp->ReanimationTryToGet(mSkyReanimID);
    Reanimation *sky2Reanim = mApp->ReanimationTryToGet(mSky2ReanimID);

    if (skyReanim != nullptr && sky2Reanim != nullptr) {
        skyReanim->DrawRenderGroup(g, 0);
        SexyTransform2D tmp = sky2Reanim->mOverlayMatrix;
        sky2Reanim->mOverlayMatrix.Scale(1.0, 0.4);
        sky2Reanim->mOverlayMatrix.m[0][1] = -0.4;
        sky2Reanim->DrawRenderGroup(g, 2);
        sky2Reanim->mOverlayMatrix = tmp;
    }
    mainMenuReanim->DrawRenderGroup(g, 1);
    Reanimation *houseAnim = mApp->ReanimationTryToGet(mHouseReanimID);
    if (houseAnim != nullptr) {

        int TrackIndex = mainMenuReanim->FindTrackIndex("House");
        ReanimatorTransform v52{};
        SexyTransform2D v48{};
        mainMenuReanim->GetCurrentTransform(TrackIndex, &v52);
        Reanimation::MatrixFromTransform(v52, (SexyMatrix3 &)v48);
        v48.Translate(mCameraPositionX + theOffsetX1, mCameraPositionY + theOffsetY1);
        houseAnim->mOverlayMatrix = v48;
        houseAnim->DrawRenderGroup(g, 0);
    }
    mainMenuReanim->DrawRenderGroup(g, 0);
    if (mMenuScene == 2) {
        Reanimation *butterFlyReanim = mApp->ReanimationTryToGet(mButterflyReanimID);
        if (butterFlyReanim != nullptr) {
            butterFlyReanim->Draw(g);
        }
    }
    Reanimation *crowReanim = mApp->ReanimationTryToGet(mCrowReanimID);
    if (crowReanim != nullptr && (!unkBool5 || mExitCounter <= 65)) {
        crowReanim->Draw(g);
    }
    MenuWidget::Draw(g);
    DeferOverlay(0);
    if (!InTransition()) {
        DrawSpeechBubble(g);
    }
    SexyTransform2D aSexyTransform2D;
    ReanimatorTransform v43;
    int mailAlertTrackIndex = mainMenuReanim->FindTrackIndex("mail alert");
    if (mailAlertTrackIndex > 0 && mApp->mMailBox->GetNumUnseenMessages() > 0) {
        LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
        if (aPlayerInfo->mLevel > 0 || aPlayerInfo->GetFlag(1)) {
            v43 = ReanimatorTransform();
            mainMenuReanim->GetCurrentTransform(mailAlertTrackIndex, &v43);
            Sexy::Image *mailAlertImage = v43.mImage;
            Reanimation::MatrixFromTransform(v43, aSexyTransform2D);
            aSexyTransform2D.Translate(mCameraPositionX, mCameraPositionY);
            int v14 = mailAlertImage->mWidth;
            int v15 = v14 + 3;
            int v16 = v14 < 0;
            int v17 = v14 & ~(v14 >> 31);
            if (v16)
                v17 = v15;
            aSexyTransform2D.Translate(v17 >> 2, 0.0);
            int v18 = mUnseenMailCounter;
            if (v18 > 99)
                v18 = 0;
            mUnseenMailCounter = v18;
            TodAnimateCurveFloat(0, 100, v18, 0.75, 0.8, TodCurves::CURVE_SIN_WAVE);
            Sexy::Rect v38 = {0, 0, mailAlertImage->mWidth, mailAlertImage->mHeight};
            g->DrawImageMatrix(mailAlertImage, aSexyTransform2D, v38, 0.0, 0.0, true);
        }
    }
    int moreTrackIndex = mainMenuReanim->FindTrackIndex("more");
    v43 = ReanimatorTransform();
    mainMenuReanim->GetCurrentTransform(moreTrackIndex, &v43);
    std::construct_at(&aSexyTransform2D);
    Reanimation::MatrixFromTransform(v43, aSexyTransform2D);
    aSexyTransform2D.Translate(mCameraPositionX, mCameraPositionY);
    aSexyTransform2D.Translate(120.0, 200.0);

    Sexy::Rect v37 = {0, 0, m2DMarkImage->mWidth, m2DMarkImage->mHeight};
    g->DrawImageMatrix(m2DMarkImage, aSexyTransform2D, v37, 0.0, 0.0, true);
    Sexy::Rect v38 = {15, 15, 90, 90};
    aSexyTransform2D.Translate(-4.0, -16.0);
    g->DrawImageMatrix(mApp->mQRCodeImage, aSexyTransform2D, v38, 0.0, 0.0, true);
}

void MainMenu::DrawOverlay(Sexy::Graphics *g) {
    // 在成就界面存在时，冒险关卡数绘制位置也上移
    if (gMainMenuAchievementsWidget != nullptr) {
        float tmp = mCameraPositionY;
        mCameraPositionY += float(mY + 60);
        old_MainMenu_DrawOverlay(this, g);
        mCameraPositionY = tmp;
        return;
    }
    old_MainMenu_DrawOverlay(this, g);
}

void MainMenu::DrawFade(Sexy::Graphics *g) {
    // 修复主界面的退出动画在高帧率设备上的加速。原理是将计时器的更新从Draw移动至Update
    float num = mFadeCounterFloat;
    // if (mainMenu->mFadeCounterFloat < 0.992) {
    // mainMenu->mFadeCounterFloat -= 0.008;
    // }
    old_MainMenu_DrawFade(this, g);
    mFadeCounterFloat = num;
}
