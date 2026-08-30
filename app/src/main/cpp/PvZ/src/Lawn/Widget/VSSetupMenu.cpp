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

#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/OpeningEncounter.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/WaitForSecondPlayerDialog.h"
#include "PvZ/SexyAppFramework/Widget/Checkbox.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <unistd.h>

#include <cassert>

using namespace Sexy;

namespace {
static int GetPrioritySidePickSlot(const VSSetupMenu *menu) {
    if (menu == nullptr || VSSetupAddonWidget::msGlobalBpMode == VSSetupAddonWidget::GLOBALBP_CLOSED) {
        return -1;
    }
    // 第一轮 P2 优先选边，客户端承受延迟故对其进行补偿
    if (VSSetupAddonWidget::msGlobalBpWins[0] == 0 && VSSetupAddonWidget::msGlobalBpWins[1] == 0) {
        return 1;
    }
    if (VSSetupMenu::msNextSidePickPlayerIndex < 0 || VSSetupMenu::msNextSidePickPlayerIndex > 1) {
        return -1;
    }
    // 败方选边
    return VSSetupMenu::msNextSidePickPlayerIndex;
}

static bool HasPrioritySidePickPending(const VSSetupMenu *menu) {
    const int prioritySlot = GetPrioritySidePickSlot(menu);
    if (prioritySlot < 0) {
        return false;
    }
    return menu->mState == VSSetupMenu::VS_SETUP_STATE_SIDES && menu->mSides[prioritySlot] == VS_SIDE_NONE;
}

static bool CanControlSideSlot(const VSSetupMenu *menu, int slot) {
    if (menu == nullptr || slot < 0 || slot > 1) {
        return false;
    }
    const int prioritySlot = GetPrioritySidePickSlot(menu);
    if (prioritySlot < 0 || !HasPrioritySidePickPending(menu)) {
        return true;
    }
    return slot == prioritySlot;
}

static VSSide ResolveRequestedSide(const VSSetupMenu *menu, int slot, VSSide requestedSide) {
    if (menu == nullptr || slot < 0 || slot > 1) {
        return VS_SIDE_NONE;
    }
    if (requestedSide < VS_SIDE_NONE || requestedSide > VS_SIDE_ZOMBIE) {
        return VS_SIDE_NONE;
    }
    if (requestedSide == VS_SIDE_NONE) {
        return VS_SIDE_NONE;
    }

    const int otherSlot = 1 - slot;
    if (requestedSide != menu->mSides[otherSlot]) {
        return requestedSide;
    }

    const VSSide currentSide = menu->mSides[slot];
    if (currentSide != VS_SIDE_NONE && currentSide != menu->mSides[otherSlot]) {
        return currentSide;
    }
    return VS_SIDE_NONE;
}

} // namespace

void VSSetupMenu::_constructor() {
    old_VSSetupMenu_Constructor(this);
    msNextFirstPick = VSSide::VS_SIDE_ZOMBIE;

    // 拓展卡槽,禁选模式 etc.
    mAddonWidget = new VSSetupAddonWidget(this);

    is1PControllerMoving = false;
    is2PControllerMoving = false;
    touchingOnWhichController = 0;
    drawTipArrowAlphaCounter = 0;
    gVSSetupRequestState = 0;
}

void VSSetupMenu::_destructor() {
    delete mAddonWidget;

    old_VSSetupMenu_Destructor(this);
}

void VSSetupMenu::AddedToManager(Sexy::WidgetManager *theWidgetManager) {
    old_VSSetupMenu_AddedToManager(this, theWidgetManager);

    // 缩小Widget，使得触控可传递给VSSetupMenu自身
    for (int i = BACKGROUND_FRAME; i <= CONTROLLER_1; ++i) {
        Sexy::Widget *aWidget = FindWidget(i);
        if (aWidget) {
            aWidget->Resize(aWidget->mX, aWidget->mY, 0, 0);
        }
    }
    // 在完成选边前禁用按钮
    for (int i = QUICK_BUTTON; i <= RANDOM_BUTTON; ++i) {
        Sexy::ButtonWidget *aButton = ((Sexy::ButtonWidget *)FindWidget(i));
        aButton->SetDisabled(true);
        (*aButton->mColors)[ButtonWidget::COLOR_LABEL] = gColorGray;
        (*aButton->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = gColorGray;
    }
}

static int GetControllerSideAnchorX(VSSide theSide) {
    if (theSide == VS_SIDE_PLANT) {
        return 240;
    }
    if (theSide == VS_SIDE_ZOMBIE) {
        return 410;
    }
    return 325;
}

void VSSetupMenu::Draw(Graphics *g) {
    // 在这里绘制会被 DrawOverlay 遮挡，去 DrawOverlay 绘制即可
    old_VSSetupMenu_Draw(this, g);
}

void VSSetupMenu::DrawOverlay(Graphics *g) {
    old_VSSetupMenu_DrawOverlay(this, g);

    if (mState == VSSetupState::VS_SETUP_STATE_SIDES) {
        TodDrawString(g, "[VS_PICK_SIDES]", 350, 110, Sexy::FONT_DWARVENTODCRAFT18, Color::White, DrawStringJustification::DS_ALIGN_LEFT);
    } else if (mState == VSSetupState::VS_SETUP_STATE_SELECT_BATTLE) {
        TodDrawString(g, "[VS_PICK_BATTLES]", 350, 110, Sexy::FONT_DWARVENTODCRAFT18, Color::White, DrawStringJustification::DS_ALIGN_LEFT);
    }

    if (!(gIsServerModeSpectator || gIsReplayMode) && drawTipArrowAlphaCounter > 200) {
        int aAlpha = TodAnimateCurve(0, 100, drawTipArrowAlphaCounter % 100, 50, 255, TodCurves::CURVE_BOUNCE);
        g->SetColorizeImages(true);
        g->SetColor(Color(255, 255, 255, aAlpha));

        if (!gTcpConnected && mSides[0] == VSSide::VS_SIDE_NONE && CanControlSideSlot(this, 0)) {
            Sexy::Widget *theController1Widget = FindWidget(CONTROLLER_0);
            g->DrawImage(Sexy::IMAGE_ZEN_NEXTGARDEN, theController1Widget->mX + 160, theController1Widget->mY + 40);
            g->DrawImageMirror(Sexy::IMAGE_ZEN_NEXTGARDEN, theController1Widget->mX - 50, theController1Widget->mY + 40, true);
        }


        if (gTcpClientSocket < 0 && mSides[1] == VSSide::VS_SIDE_NONE && CanControlSideSlot(this, 1)) {
            Sexy::Widget *theController2Widget = FindWidget(CONTROLLER_1);
            g->DrawImage(Sexy::IMAGE_ZEN_NEXTGARDEN, theController2Widget->mX + 160, theController2Widget->mY + 40);
            g->DrawImageMirror(Sexy::IMAGE_ZEN_NEXTGARDEN, theController2Widget->mX - 50, theController2Widget->mY + 40, true);
        }

        g->SetColorizeImages(false);
    }

    if (!(gIsServerModeSpectator || gIsReplayMode) && gVSSetupRequestState != 0 && mState != VSSetupState::VS_SETUP_STATE_CUSTOM_BATTLE) {

        // ======================
        // 我是 guest：已提醒房主...
        // (gTcpConnected == true 代表我作为 client 连接到 host)
        // ======================
        if (gTcpConnected) {
            switch (gVSSetupRequestState) {
                case VSSetupMenu_Quick_Play: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate("[VS_OPT_QUICK_GAME]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupMenu_Custom_Battle: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate("[VS_OPT_CUSTOM_ARENA]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupMenu_Random_Battle: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate("[VS_OPT_RANDOM_ARENA]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_ExtraPacket: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mExtraPacketMode) ? "[VS_OPT_ENABLE_EXTRA_SLOTS]" : "[VS_OPT_DISABLE_EXTRA_SLOTS]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_ExtendedSeeds: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mExtendedSeedsMode) ? "[VS_OPT_ENABLE_EXTRA_SEEDS]" : "[VS_OPT_DISABLE_EXTRA_SEEDS]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_BanMode: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mBanMode) ? "[VS_OPT_ENABLE_BAN_MODE]" : "[VS_OPT_DISABLE_BAN_MODE]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_BalancePatch: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mBalancePatchMode) ? "[VS_OPT_ENABLE_BALANCE_PATCH]" : "[VS_OPT_DISABLE_BALANCE_PATCH]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_PlantAI: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mPlantAIMode) ? "[VS_OPT_ENABLE_PLANT_AI]" : "[VS_OPT_DISABLE_PLANT_AI]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_ZombieAI: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mZombieAIMode) ? "[VS_OPT_ENABLE_ZOMBIE_AI]" : "[VS_OPT_DISABLE_ZOMBIE_AI]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_AIEnhancement: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mAIEnhancementMode) ? "[VS_OPT_ENABLE_AI_ENHANCEMENT]" : "[VS_OPT_DISABLE_AI_ENHANCEMENT]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_Back: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    pvzstl::string opt = TodStringTranslate("[BACK_TO_MODE_SELECT]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_GlobalBP: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_REMIND_HOST_FMT]");
                    const char *label = "";
                    switch (VSSetupAddonWidget::msGlobalBpMode) {
                        case VSSetupAddonWidget::GLOBALBP_CLOSED:
                            label = "[VS_OPT_ENABLE_GLOBAL_BP_BO3]";
                            break;
                        case VSSetupAddonWidget::GLOBALBP_BO3:
                            label = "[VS_OPT_ENABLE_GLOBAL_BP_BO5]";
                            break;
                        case VSSetupAddonWidget::GLOBALBP_BO5:
                        default:
                            label = "[VS_OPT_DISABLE_GLOBAL_BP]";
                            break;
                    }
                    pvzstl::string opt = TodStringTranslate(label);
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                default:
                    break;
            }
        }

        // ======================
        // 我是 host：对方想玩/想要...
        // (gTcpClientSocket >= 0 表示我作为 host 收到了 client 连接)
        // ======================
        if (gTcpClientSocket >= 0) {
            switch (gVSSetupRequestState) {
                case VSSetupMenu_Quick_Play: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_PLAY_FMT]");
                    pvzstl::string opt = TodStringTranslate("[VS_OPT_QUICK_GAME]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupMenu_Custom_Battle: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_PLAY_FMT]");
                    pvzstl::string opt = TodStringTranslate("[VS_OPT_CUSTOM_ARENA]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupMenu_Random_Battle: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_PLAY_FMT]");
                    pvzstl::string opt = TodStringTranslate("[VS_OPT_RANDOM_ARENA]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_ExtraPacket: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mExtraPacketMode) ? "[VS_OPT_ENABLE_EXTRA_SLOTS]" : "[VS_OPT_DISABLE_EXTRA_SLOTS]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_ExtendedSeeds: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mExtendedSeedsMode) ? "[VS_OPT_ENABLE_EXTRA_SEEDS]" : "[VS_OPT_DISABLE_EXTRA_SEEDS]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_BanMode: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mBanMode) ? "[VS_OPT_ENABLE_BAN_MODE]" : "[VS_OPT_DISABLE_BAN_MODE]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_BalancePatch: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mBalancePatchMode) ? "[VS_OPT_ENABLE_BALANCE_PATCH]" : "[VS_OPT_DISABLE_BALANCE_PATCH]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_PlantAI: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mPlantAIMode) ? "[VS_OPT_ENABLE_PLANT_AI]" : "[VS_OPT_DISABLE_PLANT_AI]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_ZombieAI: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mZombieAIMode) ? "[VS_OPT_ENABLE_ZOMBIE_AI]" : "[VS_OPT_DISABLE_ZOMBIE_AI]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_AIEnhancement: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate((!mAddonWidget->mAIEnhancementMode) ? "[VS_OPT_ENABLE_AI_ENHANCEMENT]" : "[VS_OPT_DISABLE_AI_ENHANCEMENT]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_Back: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    pvzstl::string opt = TodStringTranslate("[BACK_TO_MODE_SELECT]");
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                case VSSetupAddonWidget::VSSetupAddonWidget_GlobalBP: {
                    pvzstl::string fmt = TodStringTranslate("[VS_TIP_OPPONENT_WANTS_GET_FMT]");
                    const char *label = "";
                    switch (VSSetupAddonWidget::msGlobalBpMode) {
                        case VSSetupAddonWidget::GLOBALBP_CLOSED:
                            label = "[VS_OPT_ENABLE_GLOBAL_BP_BO3]";
                            break;
                        case VSSetupAddonWidget::GLOBALBP_BO3:
                            label = "[VS_OPT_ENABLE_GLOBAL_BP_BO5]";
                            break;
                        case VSSetupAddonWidget::GLOBALBP_BO5:
                        default:
                            label = "[VS_OPT_DISABLE_GLOBAL_BP]";
                            break;
                    }
                    pvzstl::string opt = TodStringTranslate(label);
                    TodDrawString(g, StrFormat(fmt.c_str(), opt.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153), DrawStringJustification::DS_ALIGN_LEFT);
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (mAddonWidget) {
        mAddonWidget->Draw(g);
    }
}

void VSSetupMenu::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag) {
    int state = mState;

    if (state != VS_SETUP_STATE_CUSTOM_BATTLE) {
        switch (theButton) {
            case Sexy::GAMEPAD_BUTTON_LEFT:
            case Sexy::GAMEPAD_BUTTON_DPAD_LEFT: {
                if (state != VS_SETUP_STATE_SIDES) {
                    return;
                }

                bool side = mControllerIndex[0] != thePlayerIndex;
                if (mControllerIndex[side] != thePlayerIndex || mSideLocked[side] || !CanControlSideSlot(this, int(side))) {
                    return;
                }

                if (mSides[side] == VS_SIDE_NONE) {
                    mSides[side] = VS_SIDE_PLANT;
                } else if (mSides[side] == VS_SIDE_ZOMBIE) {
                    mSides[side] = VS_SIDE_NONE;
                }
                return;
            }

            case Sexy::GAMEPAD_BUTTON_RIGHT:
            case Sexy::GAMEPAD_BUTTON_DPAD_RIGHT: {
                if (state != VS_SETUP_STATE_SIDES) {
                    return;
                }

                bool side = mControllerIndex[0] != thePlayerIndex;
                if (mControllerIndex[side] != thePlayerIndex || mSideLocked[side] || !CanControlSideSlot(this, int(side))) {
                    return;
                }

                if (mSides[side] == VS_SIDE_NONE) {
                    mSides[side] = VS_SIDE_ZOMBIE;
                } else if (mSides[side] == VS_SIDE_PLANT) {
                    mSides[side] = VS_SIDE_NONE;
                }
                return;
            }

            case Sexy::GAMEPAD_BUTTON_A: {
                if (state != VS_SETUP_STATE_SIDES) {
                    return;
                }

                bool side = mControllerIndex[0] != thePlayerIndex;
                if (mControllerIndex[side] != thePlayerIndex || !CanControlSideSlot(this, int(side))) {
                    return;
                }

                VSSide selectedSide = mSides[side];
                if (selectedSide == VS_SIDE_NONE) {
                    return;
                }

                if (selectedSide == mSides[!side] && mSideLocked[!side]) {
                    mApp->PlaySample(Sexy::SOUND_BUZZER);
                    return;
                }

                mSideLocked[side] = true;
                if (mSideLocked[0] && mSideLocked[1]) {
                    GoToState(VS_SETUP_STATE_SELECT_BATTLE);
                }
                return;
            }

            case Sexy::GAMEPAD_BUTTON_B:
                break;

            default:
                return;
        }
    }

    if (theButton == Sexy::GAMEPAD_BUTTON_B) {
        if (mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CONFIRM_VS_CHOOSE_SEED_BACK_HEADER]", "[CONFIRM_VS_CHOOSE_SEED_BACK_BODY]", "[DIALOG_BUTTON_OK]", "[DIALOG_BUTTON_CANCEL]", 1) != 1000) {
            return;
        }

        switch (state) {
            case VS_SETUP_STATE_CONTROLLERS:
                CloseVSSetup(true);
                mApp->KillBoard();
                mApp->ShowGameSelector();
                return;

            case VS_SETUP_STATE_SIDES: {
                bool side = mControllerIndex[0] != thePlayerIndex;
                if (mControllerIndex[side] != thePlayerIndex) {
                    return;
                }

                if (mSideLocked[side]) {
                    mSideLocked[side] = false;
                } else {
                    CloseVSSetup(true);
                    mApp->KillBoard();
                    mApp->ShowGameSelector();
                }
                return;
            }

            case VS_SETUP_STATE_SELECT_BATTLE:
                GoToState(VS_SETUP_STATE_SIDES);
                return;

            case VS_SETUP_STATE_CUSTOM_BATTLE: {
                bool side = mControllerIndex[0] != thePlayerIndex;
                if (mControllerIndex[side] != thePlayerIndex) {
                    return;
                }

                if (mSides[side] != VS_SIDE_NONE) {
                    mSideLocked[side] = true;
                }

                mApp->KillSeedChooserScreen();
                mApp->KillZombieChooserScreen();
                GoToState(VS_SETUP_STATE_SELECT_BATTLE);
                return;
            }

            default:
                return;
        }
    }

    int player = mApp->GamepadToPlayerIndex(thePlayerIndex);
    VSSide side = mSides[player];
    if (side == VS_SIDE_ZOMBIE) {
        mApp->mZombieChooserScreen->GameButtonDown(theButton, thePlayerIndex, theModifierFlag);
    } else if (side == VS_SIDE_PLANT) {
        mApp->mSeedChooserScreen->GameButtonDown(theButton, thePlayerIndex, theModifierFlag);
    }
}

void VSSetupMenu::MouseDown(int x, int y, int theCount) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (mState == VS_SETUP_STATE_SIDES) {
        Sexy::Widget *theController1Widget = FindWidget(CONTROLLER_0);
        Sexy::Widget *theController2Widget = FindWidget(CONTROLLER_1);
        if (x > theController1Widget->mX && x < theController1Widget->mX + 170 && y > theController1Widget->mY && y < theController1Widget->mY + 122) {
            if (gTcpConnected || !CanControlSideSlot(this, 0)) {
                return;
            }
            is1PControllerMoving = true;
            drawTipArrowAlphaCounter = 0;
            touchingOnWhichController = 1;
        } else if (x > theController2Widget->mX && x < theController2Widget->mX + 170 && y > theController2Widget->mY && y < theController2Widget->mY + 122) {
            if (gTcpClientSocket >= 0 || !CanControlSideSlot(this, 1)) {
                return;
            }
            is2PControllerMoving = true;
            drawTipArrowAlphaCounter = 0;
            touchingOnWhichController = 2;
        }
        touchDownX = x;
    }
}

void VSSetupMenu::MouseDrag(int x, int y) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (touchingOnWhichController == 1) {
        if (gTcpConnected || gIsReplayMode)
            return;
        Sexy::Widget *theController1Widget = FindWidget(CONTROLLER_0);
        theController1Widget->Move(theController1Widget->mX + x - touchDownX, theController1Widget->mY);
        if (gTcpClientSocket >= 0) {
            U16_Event event = {{EventType::EVENT_SERVER_VSSETUPMENU_MOVE_CONTROLLER}, uint16_t(theController1Widget->mX)};
            netplay::PutEvent(event);
        }
    } else if (touchingOnWhichController == 2) {
        if (gTcpClientSocket >= 0)
            return;
        Sexy::Widget *theController2Widget = FindWidget(CONTROLLER_1);
        theController2Widget->Move(theController2Widget->mX + x - touchDownX, theController2Widget->mY);
        if (gTcpServerSocket >= 0) {
            U16_Event event = {{EventType::EVENT_CLIENT_VSSETUPMENU_MOVE_CONTROLLER}, uint16_t(theController2Widget->mX)};
            netplay::PutEvent(event);
        }
    }
    touchDownX = x;
}

void VSSetupMenu::MouseUp(int x, int y, int theCount) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }

    bool handledControllerMouseUp = false;
    if (touchingOnWhichController == 1) {
        if (gTcpConnected) {
            touchingOnWhichController = 0;
            return;
        }
        handledControllerMouseUp = true;
        Sexy::Widget *aControllerWidgetP1 = FindWidget(CONTROLLER_0);
        VSSide aSideP1 = aControllerWidgetP1->mX > 400 ? VS_SIDE_ZOMBIE : aControllerWidgetP1->mX > 250 ? VS_SIDE_NONE : VS_SIDE_PLANT;
        VSSide resolvedSideP1 = ResolveRequestedSide(this, 0, aSideP1);
        mSides[0] = resolvedSideP1;
        mSideLocked[0] = (mSides[0] != VS_SIDE_NONE);
        aControllerWidgetP1->Move(GetControllerSideAnchorX(mSides[0]), aControllerWidgetP1->mY);
        if (gTcpClientSocket >= 0) {
            U8U8_Event event = {{EventType::EVENT_SERVER_VSSETUPMENU_SET_SIDE}, 0, mSides[0] == -1 ? uint8_t(2) : uint8_t(mSides[0])};
            netplay::PutEvent(event);
        }
        is1PControllerMoving = false;
    } else if (touchingOnWhichController == 2) {
        if (gTcpClientSocket >= 0) {
            touchingOnWhichController = 0;
            return;
        }
        handledControllerMouseUp = true;
        Sexy::Widget *aControllerWidgetP2 = FindWidget(CONTROLLER_1);
        VSSide aSideP2 = aControllerWidgetP2->mX > 400 ? VS_SIDE_ZOMBIE : aControllerWidgetP2->mX > 250 ? VS_SIDE_NONE : VS_SIDE_PLANT;

        VSSide resolvedSideP2 = ResolveRequestedSide(this, 1, aSideP2);
        if (resolvedSideP2 == mSides[1]) {
            mSideLocked[1] = (mSides[1] != VS_SIDE_NONE);
        }
        if (gTcpServerSocket >= 0) {
            U8_Event event = {{EventType::EVENT_CLIENT_VSSETUPMENU_REQUEST_SIDE}, resolvedSideP2 == VS_SIDE_NONE ? uint8_t(2) : uint8_t(resolvedSideP2)};
            netplay::PutEvent(event);
        } else {
            mSides[1] = resolvedSideP2;
            mSideLocked[1] = (mSides[1] != VS_SIDE_NONE);
            aControllerWidgetP2->Move(GetControllerSideAnchorX(mSides[1]), aControllerWidgetP2->mY);
            is2PControllerMoving = false;
        }
    }
    touchingOnWhichController = 0;
    if (handledControllerMouseUp && mState == VS_SETUP_STATE_SIDES && mSides[0] != VS_SIDE_NONE && mSides[1] != VS_SIDE_NONE && mSides[0] != mSides[1]) {
        mSideLocked[0] = true;
        mSideLocked[1] = true;
        GoToState(VS_SETUP_STATE_SELECT_BATTLE);
    }
}

void VSSetupMenu::Update() {
    drawTipArrowAlphaCounter++;

    if (is1PControllerMoving || is2PControllerMoving) {
        Sexy::Widget *theController1Widget = FindWidget(CONTROLLER_0);
        Sexy::Widget *theController2Widget = FindWidget(CONTROLLER_1);
        int Controller1X = theController1Widget->mX;
        int Controller2X = theController2Widget->mX;
        old_VSSetupMenu_Update(this);
        if (is1PControllerMoving)
            theController1Widget->Move(Controller1X, theController1Widget->mY);
        if (is2PControllerMoving)
            theController2Widget->Move(Controller2X, theController2Widget->mY);
    } else {
        old_VSSetupMenu_Update(this);
    }

    if (mState == VS_SETUP_STATE_CONTROLLERS) {
        return;
    }
    if (mState == VS_SETUP_STATE_SIDES && !gTcpConnected && gTcpClientSocket == -1 && !isKeyboardTwoPlayerMode) {
        // 本地游戏
        // 自动分配阵营
        //        mSides[0] = 0;
        //        mSides[1] = 1;
        //        GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 0, 0);
        //        GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 1, 0);
        return;
    }
}

void VSSetupMenu::PickRandomZombies(std::vector<SeedType> &theZombieSeeds) const {
    assert(theZombieSeeds.empty());

    // 原本选 5 个, 扩展为 (卡槽数 - 1) 个
    const int numSeedsInBank = mApp->mBoard->GetNumSeedsInBank(true) - 1;
    theZombieSeeds.reserve(numSeedsInBank);

    for (int seedIdx = 0; seedIdx < numSeedsInBank; ++seedIdx) {
        // 0 1 -> 0
        // 2 3 -> 1
        // 4 5 -> 2
        int poolIdx = (seedIdx <= 5) ? seedIdx / 2 : 0;
        poolIdx += 6; // 僵尸卡池从池 6 开始

        // 统计该池有效元素个数
        int validCount = 0;
        while (validCount < 8 && msRandomPools[poolIdx][validCount] != SEED_NONE) {
            ++validCount;
        }

        SeedType aSeedType = SEED_NONE;
        do {
            do {
                aSeedType = msRandomPools[poolIdx][Sexy::Rand(validCount)];
            } while (std::ranges::contains(theZombieSeeds, aSeedType)); // 重复则重选
        } while (!mApp->HasSeedType(aSeedType, true)); // 未获得则重选
        theZombieSeeds.push_back(aSeedType);
    }
}

void VSSetupMenu::PickRandomPlants(std::vector<SeedType> &thePlantSeeds, const std::vector<SeedType> &theZombieSeeds) const {
    assert(thePlantSeeds.empty() && (theZombieSeeds.size() == mApp->mBoard->GetNumSeedsInBank(true) - 1));

    // 原本选 5 个, 扩展为 (卡槽数 - 1) 个
    const int numSeedsInBank = mApp->mBoard->GetNumSeedsInBank(false) - 1;
    thePlantSeeds.reserve(numSeedsInBank);

    int seedIdx = 0;
    int poolOffset = 0;

    // 是否为含蘑菇卡组
    if (mApp->mPlayerInfo->mLevel > 20 || /* 二周目 */ mApp->HasFinishedAdventure()) {
        bool isStageNight = mApp->mBoard->StageIsNight();
        bool flag = Sexy::Rand(5) == 1;
        if (!isStageNight && /* 1/5 */ flag) {
            thePlantSeeds.push_back(SEED_INSTANT_COFFEE);
            ++seedIdx;
            poolOffset = 3;
        } else if (isStageNight && /* 4/5 */ !flag) {
            poolOffset = 3;
        }
    }

    for (; seedIdx < numSeedsInBank; ++seedIdx) {
        // 0 1 -> 0
        // 2 3 -> 1
        // 4 5 -> 2
        int poolIdx = (seedIdx <= 5) ? seedIdx / 2 : 0;
        poolIdx += poolOffset; // 含蘑菇卡池从池 3 开始

        // 统计该池有效元素个数
        int validCount = 0;
        while (validCount < 8 && msRandomPools[poolIdx][validCount] != SEED_NONE) {
            ++validCount;
        }

        SeedType aSeedType = SEED_NONE;
        do {
            do {
                aSeedType = msRandomPools[poolIdx][Sexy::Rand(validCount)];
            } while (std::ranges::contains(thePlantSeeds, aSeedType)); // 重复则重选
        } while (!mApp->HasSeedType(aSeedType, false)); // 未获得则重选
        thePlantSeeds.push_back(aSeedType);
    }

    // 原代码疑点: 前面已检查过 HasSeedType()
    if (std::ranges::contains(theZombieSeeds, SEED_ZOMBIE_POGO) /* && mApp->HasSeedType(SEED_WALLNUT, false) */) {
        auto it = std::ranges::find(thePlantSeeds, SEED_WALLNUT);
        if (it != thePlantSeeds.end()) {
            *it = SEED_TALLNUT;
        }
    }
}

void VSSetupMenu::processClientEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_CLIENT_VSSETUPMENU_BUTTON_DEPRESS: {
            auto *eventBtnDepress = static_cast<const U8_Event *>(event);
            gVSSetupRequestState = eventBtnDepress->data;
        } break;
        case EVENT_CLIENT_VSSETUP_ADDON_CHECKBOX_CHECKED: {
            auto *eventCheckbox = static_cast<const U8_Event *>(event);
            if (VSSetupAddonWidget::IsLocalAIOption(eventCheckbox->data)) {
                break;
            }
            gVSSetupRequestState = eventCheckbox->data;
        } break;
        case EVENT_CLIENT_VSSETUP_SEND_NAME_STATE: {
            auto *eventState = static_cast<const U8_Event *>(event);
            gMetricsHostSendNameAllowed = eventState->data;
        } break;
        case EVENT_CLIENT_SEEDCHOOSER_BUTTON_DEPRESS: {
            auto *eventBtnDepress = static_cast<const U8U8_Event *>(event);
            SeedChooserScreen *seedChooser = eventBtnDepress->data2 != 0 ? mApp->mZombieChooserScreen : mApp->mSeedChooserScreen;
            if (seedChooser != nullptr) {
                seedChooser->ButtonDepress_Origin(eventBtnDepress->data1);
            }
        } break;
        case EVENT_CLIENT_SEEDCHOOSER_SELECT_SEED:
        case EVENT_CLIENT_SEEDCHOOSER_BAN_SEED: {
            auto *event1 = static_cast<const U8x3_Event *>(event);
            auto seedType = SeedType(event1->data[0]);
            bool isZombieChooser = event1->data[1] != 0;
            const uint8_t cursorFlags = event1->data[2];
            bool moveOnly = (cursorFlags & SeedChooserScreen::kCursorMoveOnlyEventFlag) != 0;
            int syncedPageIndex = (cursorFlags & SeedChooserScreen::kCursorPageOneEventFlag) != 0 ? 1 : 0;
            SeedChooserScreen *seedChooser = (isZombieChooser ? mApp->mZombieChooserScreen : mApp->mSeedChooserScreen);
            if (seedChooser == nullptr) {
                break;
            }
            if (isZombieChooser) {
                if (seedType < SeedType::SEED_ZOMBIE_GRAVESTONE || seedType >= SeedType::NUM_ZOMBIE_SEEDS_IN_CHOOSER) {
                    break;
                }
            } else {
                if (seedType < SeedType::SEED_PEASHOOTER || seedChooser->GetSeedPacketIndex(seedType) < 0) {
                    break;
                }
            }
            int seedIndex = seedChooser->GetSeedPacketIndex(seedType);
            if (seedIndex < 0 || seedIndex >= seedChooser->GetSeedStorageCount()) {
                break;
            }
            if (isZombieChooser) {
                if ((syncedPageIndex == 0 && seedIndex >= 25) || (syncedPageIndex == 1 && seedIndex < 25)) {
                    break;
                }
            } else {
                if ((syncedPageIndex == 0 && seedIndex >= NUM_SEEDS_IN_CHOOSER) || (syncedPageIndex == 1 && seedIndex < NUM_SEEDS_IN_CHOOSER)) {
                    break;
                }
            }

            seedChooser->SetPageIndex(syncedPageIndex);

            int cursorSeedIndex = seedIndex;
            if (syncedPageIndex == 1) {
                cursorSeedIndex -= isZombieChooser ? 25 : NUM_SEEDS_IN_CHOOSER;
            }

            if (cursorSeedIndex < 0 || cursorSeedIndex >= seedChooser->GetCurrentPageSeedCount()) {
                break;
            }

            VSSide targetSide = isZombieChooser ? VSSide::VS_SIDE_ZOMBIE : VSSide::VS_SIDE_PLANT;
            if (mSides[0] != targetSide && mSides[1] != targetSide) {
                break;
            }
            int ownerPlayerIndex = seedChooser->mPlayerIndex;
            int *cursorX = (ownerPlayerIndex == 0) ? &seedChooser->mCursorPositionX1 : &seedChooser->mCursorPositionX2;
            int *cursorY = (ownerPlayerIndex == 0) ? &seedChooser->mCursorPositionY1 : &seedChooser->mCursorPositionY2;
            seedChooser->GetSeedPositionInChooser(cursorSeedIndex, *cursorX, *cursorY);
            if (ownerPlayerIndex == 0)
                seedChooser->mSeedIndex1 = cursorSeedIndex;
            else
                seedChooser->mSeedIndex2 = cursorSeedIndex;

            if (gTcpClientSocket >= 0) {
                EventType syncType = event->type == EVENT_CLIENT_SEEDCHOOSER_BAN_SEED ? EventType::EVENT_SERVER_SEEDCHOOSER_BAN_SEED : EventType::EVENT_SERVER_SEEDCHOOSER_SELECT_SEED;
                U8x3_Event syncEvent = {{syncType}, {event1->data[0], event1->data[1], event1->data[2]}};
                netplay::PutEvent(syncEvent);
            }

            if (moveOnly) {
                break;
            }

            if (seedChooser->SeedNotAllowedToPick(seedType) || seedChooser->SeedNotAllowedDuringTrial(seedType)) {
                break;
            }

            ChosenSeed &chosenSeed = seedChooser->GetChosenSeed(seedIndex);
            if (chosenSeed.mSeedState != ChosenSeedState::SEED_IN_CHOOSER) {
                break;
            }
            chosenSeed.mSeedType = seedType;
            seedChooser->ClickedSeedInChooser_Orgin(chosenSeed, ownerPlayerIndex);
        } break;
            //        case EVENT_SERVER_VSSETUPMENU_PICKBACKGROUND: {
            //             auto *event1 = static_cast<const U8_Event *>(event);
            //            int tmp = VSBackGround;
            //            VSBackGround = event1->data;
            //            gTcpConnected = false;
            //            PickBackgroundImmediately();
            //            gTcpConnected = true;
            //            VSBackGround = tmp;
            //        } break;
        case EVENT_CLIENT_VSSETUPMENU_MOVE_CONTROLLER: {
            auto *event1 = static_cast<const U16_Event *>(event);
            Sexy::Widget *theController2Widget = FindWidget(CONTROLLER_1);
            if (theController2Widget != nullptr) {
                theController2Widget->Move(event1->data, theController2Widget->mY);
                is2PControllerMoving = true;
            }
        } break;
        case EVENT_CLIENT_VSSETUPMENU_REQUEST_SIDE: {
            auto *event1 = static_cast<const U8_Event *>(event);
            VSSide requestedSide = event1->data == 2 ? VS_SIDE_NONE : VSSide(event1->data);
            if (requestedSide < VS_SIDE_NONE || requestedSide > VS_SIDE_ZOMBIE) {
                break;
            }
            if (mState != VS_SETUP_STATE_SIDES) {
                break;
            }
            if (!CanControlSideSlot(this, 1)) {
                break;
            }

            VSSide resolvedSide = ResolveRequestedSide(this, 1, requestedSide);

            Sexy::Widget *controllerWidget = FindWidget(CONTROLLER_1);
            mSides[1] = resolvedSide;
            mSideLocked[1] = (mSides[1] != VS_SIDE_NONE);
            if (controllerWidget != nullptr) {
                controllerWidget->Move(GetControllerSideAnchorX(mSides[1]), controllerWidget->mY);
            }
            is2PControllerMoving = false;
            if (gTcpClientSocket >= 0) {
                uint8_t sideData = (resolvedSide == VS_SIDE_NONE) ? 2 : uint8_t(resolvedSide);
                U8U8_Event syncEvent = {{EventType::EVENT_SERVER_VSSETUPMENU_SET_SIDE}, 1, sideData};
                netplay::PutEvent(syncEvent);
            }
            if (mSides[0] != VS_SIDE_NONE && mSides[1] != VS_SIDE_NONE && mSides[0] != mSides[1]) {
                mSideLocked[0] = true;
                mSideLocked[1] = true;
                GoToState(VS_SETUP_STATE_SELECT_BATTLE);
            }
        } break;
        default:
            break;
    }
}

void VSSetupMenu::processServerEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_SERVER_VSSETUPMENU_BUTTON_DEPRESS: {
            auto *eventBtnDepress = static_cast<const U8_Event *>(event);
            int theId = eventBtnDepress->data;
            LOG_DEBUG("theId={}", theId);
            if (gVSSetupRequestState == theId) {
                gVSSetupRequestState = 0;
            }
            if (theId == VSSetupMenu_Random_Battle && mState == VS_SETUP_STATE_SELECT_BATTLE) { // 随机战场
                break;
            }
            ButtonDepress_Origin(theId);
        } break;
        case EVENT_SERVER_SEEDCHOOSER_BUTTON_DEPRESS: {
            auto *eventBtnDepress = static_cast<const U8U8_Event *>(event);
            SeedChooserScreen *seedChooser = eventBtnDepress->data2 != 0 ? mApp->mZombieChooserScreen : mApp->mSeedChooserScreen;
            if (seedChooser != nullptr) {
                seedChooser->ButtonDepress_Origin(eventBtnDepress->data1);
            }
        } break;
        case EVENT_VSSETUPMENU_ENTER_STATE: {
            [[maybe_unused]] int aState = static_cast<const U8_Event *>(event)->data;
            LOG_DEBUG("theState={}", aState);
            // GoToState(aState);
        } break;
        case EVENT_SERVER_SEEDCHOOSER_SELECT_SEED:
        case EVENT_SERVER_SEEDCHOOSER_BAN_SEED: {
            auto *event1 = static_cast<const U8x3_Event *>(event);
            auto seedType = SeedType(event1->data[0]);
            bool isZombieChooser = event1->data[1] != 0;
            const uint8_t cursorFlags = event1->data[2];
            bool moveOnly = (cursorFlags & SeedChooserScreen::kCursorMoveOnlyEventFlag) != 0;
            int syncedPageIndex = (cursorFlags & SeedChooserScreen::kCursorPageOneEventFlag) != 0 ? 1 : 0;
            SeedChooserScreen *seedChooser = (isZombieChooser ? mApp->mZombieChooserScreen : mApp->mSeedChooserScreen);
            if (seedChooser == nullptr) {
                break;
            }
            if (isZombieChooser) {
                if (seedType < SeedType::SEED_ZOMBIE_GRAVESTONE || seedType >= SeedType::NUM_ZOMBIE_SEEDS_IN_CHOOSER) {
                    break;
                }
            } else {
                if (seedType < SeedType::SEED_PEASHOOTER || seedChooser->GetSeedPacketIndex(seedType) < 0) {
                    break;
                }
            }
            int seedIndex = seedChooser->GetSeedPacketIndex(seedType);
            if (seedIndex < 0 || seedIndex >= seedChooser->GetSeedStorageCount()) {
                break;
            }
            if (isZombieChooser) {
                if ((syncedPageIndex == 0 && seedIndex >= 25) || (syncedPageIndex == 1 && seedIndex < 25)) {
                    break;
                }
            } else {
                if ((syncedPageIndex == 0 && seedIndex >= NUM_SEEDS_IN_CHOOSER) || (syncedPageIndex == 1 && seedIndex < NUM_SEEDS_IN_CHOOSER)) {
                    break;
                }
            }

            seedChooser->SetPageIndex(syncedPageIndex);

            int cursorSeedIndex = seedIndex;
            if (syncedPageIndex == 1) {
                cursorSeedIndex -= isZombieChooser ? 25 : NUM_SEEDS_IN_CHOOSER;
            }

            if (cursorSeedIndex < 0 || cursorSeedIndex >= seedChooser->GetCurrentPageSeedCount()) {
                break;
            }

            VSSide targetSide = isZombieChooser ? VSSide::VS_SIDE_ZOMBIE : VSSide::VS_SIDE_PLANT;
            if (mSides[0] != targetSide && mSides[1] != targetSide) {
                break;
            }
            int ownerPlayerIndex = seedChooser->mPlayerIndex;
            int *cursorX = (ownerPlayerIndex == 0) ? &seedChooser->mCursorPositionX1 : &seedChooser->mCursorPositionX2;
            int *cursorY = (ownerPlayerIndex == 0) ? &seedChooser->mCursorPositionY1 : &seedChooser->mCursorPositionY2;
            seedChooser->GetSeedPositionInChooser(cursorSeedIndex, *cursorX, *cursorY);
            if (ownerPlayerIndex == 0)
                seedChooser->mSeedIndex1 = cursorSeedIndex;
            else
                seedChooser->mSeedIndex2 = cursorSeedIndex;

            if (moveOnly) {
                break;
            }

            if (seedChooser->SeedNotAllowedToPick(seedType) || seedChooser->SeedNotAllowedDuringTrial(seedType)) {
                break;
            }

            ChosenSeed &chosenSeed = seedChooser->GetChosenSeed(seedIndex);
            if (chosenSeed.mSeedState != ChosenSeedState::SEED_IN_CHOOSER) {
                break;
            }
            chosenSeed.mSeedType = seedType;
            seedChooser->ClickedSeedInChooser_Orgin(chosenSeed, ownerPlayerIndex);
        } break;
        case EVENT_VSSETUPMENU_RANDOM_PICK: {
            auto *eventRandPick = static_cast<const U16x12_Event *>(event);
            ButtonDepress_Origin(VSSetupMenu::VSSetupMenu_Random_Battle);

            for (int i = 0; i < mApp->mBoard->GetNumSeedsInBank(false); ++i) {
                mApp->mBoard->mSeedBank[0]->mSeedPackets[i + 1].SetPacketType(SeedType(eventRandPick->data[i]), SeedType::SEED_NONE);
                mApp->mBoard->mSeedBank[1]->mSeedPackets[i + 1].SetPacketType(SeedType(eventRandPick->data[i + 6]), SeedType::SEED_NONE);
            }
            if (Challenge::msVSShuffleMode) {
                mApp->mBoard->mSeedBank[0]->mSeedPackets[6].SetPacketType(SEED_BEGHOULED_BUTTON_SHUFFLE, SeedType::SEED_NONE);
                mApp->mBoard->mSeedBank[1]->mSeedPackets[6].SetPacketType(SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE, SeedType::SEED_NONE);
            }
        } break;
        case EVENT_SERVER_VSSETUPMENU_MOVE_CONTROLLER: {
            auto *event1 = static_cast<const U16_Event *>(event);
            Sexy::Widget *theController1Widget = FindWidget(CONTROLLER_0);
            if (theController1Widget != nullptr) {
                theController1Widget->Move(event1->data, theController1Widget->mY);
                is1PControllerMoving = true;
            }
        } break;
        case EVENT_SERVER_VSSETUPMENU_SET_SIDE: {
            auto *event1 = static_cast<const U8U8_Event *>(event);
            int sideSlot = event1->data1;
            VSSide aSide = event1->data2 == 2 ? VS_SIDE_NONE : VSSide(event1->data2);
            if (sideSlot < 0 || sideSlot > 1) {
                break;
            }
            if (aSide < VS_SIDE_NONE || aSide > VS_SIDE_ZOMBIE) {
                break;
            }
            mSides[sideSlot] = aSide;
            mSideLocked[sideSlot] = (mSides[sideSlot] != VS_SIDE_NONE);
            Sexy::Widget *controllerWidget = FindWidget(sideSlot == 0 ? 7 : 8);
            if (controllerWidget != nullptr) {
                controllerWidget->Move(GetControllerSideAnchorX(mSides[sideSlot]), controllerWidget->mY);
            }
            if (sideSlot == 0) {
                is1PControllerMoving = false;
            } else {
                is2PControllerMoving = false;
            }
            if (mSides[0] != VS_SIDE_NONE && mSides[1] != VS_SIDE_NONE && mSides[0] != mSides[1]) {
                mSideLocked[0] = true;
                mSideLocked[1] = true;
                GoToState(VS_SETUP_STATE_SELECT_BATTLE);
            }
        } break;
        case EVENT_SERVER_VSSETUP_ADDON_BUTTON_INIT: {
            auto *eventButtonInit = static_cast<const B1x8_Event *>(event);
            mAddonWidget->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_ExtraPacket, eventButtonInit->data1, false);
            mAddonWidget->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_ExtendedSeeds, eventButtonInit->data2, false);
            mAddonWidget->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_BanMode, eventButtonInit->data3, false);
            mAddonWidget->SetAddonMode(VSSetupAddonWidget::VSSetupAddonWidget_BalancePatch, eventButtonInit->data4, false);
            U8_Event eventState = {{EventType::EVENT_CLIENT_VSSETUP_SEND_NAME_STATE}, mApp->mPlayerInfo->mVSResultsSendPlayerName};
            netplay::PutEvent(eventState);
        } break;
        case EVENT_SERVER_VSSETUP_GLOBALBP_SYNC: {
            auto *eventGlobalBp = static_cast<const VSSetupGlobalBpSyncEvent *>(event);
            VSSetupAddonWidget::msGlobalBpMode = VSSetupAddonWidget::GlobalBpMode(eventGlobalBp->mode);
            for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
                for (int seedIndex = 0; seedIndex < VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer; ++seedIndex) {
                    VSSetupAddonWidget::msGlobalBpSeeds[playerIndex][seedIndex] = SEED_NONE;
                }
                const int rawCount = int(eventGlobalBp->count[playerIndex]);
                const int count = rawCount > VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer ? VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer : rawCount;
                for (int seedIndex = 0; seedIndex < count; ++seedIndex) {
                    VSSetupAddonWidget::msGlobalBpSeeds[playerIndex][seedIndex] = SeedType(eventGlobalBp->seeds[playerIndex][seedIndex]);
                }
            }
            mAddonWidget->UpdateGlobalBpButtonState();
        } break;
        case EVENT_SERVER_VSSETUP_ADDON_CHECKBOX_CHECKED: {
            auto *eventCheckbox = static_cast<const U8U8_Event *>(event);
            int id = eventCheckbox->data1;
            bool checked = eventCheckbox->data2 != 0;
            if (VSSetupAddonWidget::IsLocalAIOption(id)) {
                break;
            }
            mAddonWidget->SetAddonMode(id, checked, false);
            if (gVSSetupRequestState == id) {
                gVSSetupRequestState = 0;
            }
        } break;
        case EVENT_SERVER_ENCOUNTER_PICK: {
            auto *eventEncounterPick = static_cast<const U16_Event *>(event);
            if (gOpeningEncounter) {
                gOpeningEncounter->mType = EncounterType(eventEncounterPick->data);
                gOpeningEncounter->OpeningEncounterInitialize(gOpeningEncounter->mType);
            }
        } break;
        default:
            break;
    }
}


void VSSetupMenu::KeyDown(Sexy::KeyCode theKey) {
    // 修复在对战的阵营选取界面无法按返回键退出的BUG。
    if (theKey == Sexy::KeyCode::KEYCODE_ESCAPE) {
        switch (mState) {
            case VS_SETUP_STATE_CONTROLLERS:
                break;
            case VS_SETUP_STATE_SIDES:
            case VS_SETUP_STATE_SELECT_BATTLE:
                mApp->DoBackToMain();
                return;
            case VS_SETUP_STATE_CUSTOM_BATTLE: // 自定义战场
                mApp->DoNewOptions(false, 0);
                return;
        }
    }

    old_VSSetupMenu_KeyDown(this, theKey);
}

void VSSetupMenu::OnStateEnter(VSSetupState theState) {

    if (gTcpClientSocket >= 0) {
        U8_Event event = {{EventType::EVENT_VSSETUPMENU_ENTER_STATE}, uint8_t(theState)};
        netplay::PutEvent(event);
    }

    if (theState == VSSetupState::VS_SETUP_STATE_SIDES) {
        drawTipArrowAlphaCounter = 0;

        if (gTcpClientSocket >= 0 && !Challenge::msVSShuffleMode) {
            B1x8_Event event = {
                {EventType::EVENT_SERVER_VSSETUP_ADDON_BUTTON_INIT},
                mAddonWidget->mExtraPacketMode,
                mAddonWidget->mExtendedSeedsMode,
                mAddonWidget->mBanMode,
                mAddonWidget->mBalancePatchMode,
                // Keep the B1x8 payload layout compatible. Builtin AI
                // preferences are local-only and intentionally stay zero.
            };
            netplay::PutEvent(event);

            VSSetupGlobalBpSyncEvent globalBpEvent = {{EventType::EVENT_SERVER_VSSETUP_GLOBALBP_SYNC}, int8_t(VSSetupAddonWidget::msGlobalBpMode)};
            for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
                int syncedSeedCount = 0;
                for (int seedIndex = 0; seedIndex < VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer; ++seedIndex) {
                    const SeedType seedType = VSSetupAddonWidget::msGlobalBpSeeds[playerIndex][seedIndex];
                    if (seedType == SEED_NONE) {
                        continue;
                    }
                    if (syncedSeedCount >= VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer) {
                        break;
                    }
                    globalBpEvent.seeds[playerIndex][syncedSeedCount++] = uint8_t(seedType);
                }
                globalBpEvent.count[playerIndex] = uint8_t(syncedSeedCount);
            }
            netplay::PutEvent(globalBpEvent);
        }
    }
    if (theState == VSSetupState::VS_SETUP_STATE_CONTROLLERS) {

        // 此事件仅针对中途加入的观战者，告知观战者本局对战的模式。Guest无需处理此事件。
        if (gTcpClientSocket >= 0) {
            U8U8_Event event = {{EventType::EVENT_SERVER_VSSETUPMENU_SYNC_VS_MODE}, uint8_t(Challenge::msVSShuffleMode), uint8_t(mApp->mBoard->mBackground)};
            netplay::PutEvent(event);
        }

        // 跳过 VSSetupState 的 WaitForSecondPlayerDialog
        mApp->SetSecondPlayer(1);
        SetSecondPlayerIndex(mApp->mSecondPlayerGamepadIndex);
        GoToState(VSSetupState::VS_SETUP_STATE_SIDES);


        return;

        //        mControllerIndex[1] = -1;
        //        auto *aWaitDialog = new WaitForSecondPlayerDialog(mApp);
        //        mApp->AddDialog(aWaitDialog);
        //
        //        int aButtonId = aWaitDialog->WaitForResult(true);
        //        if (aButtonId == VSSetupMenu::VSSetupMenu_Enter) {
        //            SetSecondPlayerIndex(mApp->mSecondPlayerGamepadIndex);
        //            GoToState(VSSetupState::VS_SETUP_STATE_SIDES);
        //        } else if (aButtonId == VSSetupMenu::VSSetupMenu_Back) {
        //            CloseVSSetup(true);
        //            mApp->KillBoard();
        //            //            mApp->ShowGameSelector();
        //            mApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_VS); // 返回主菜单改为返回战场选择
        //        }
        //        return;
    } else if (theState == VSSetupState::VS_SETUP_STATE_SELECT_BATTLE) {
        // 选边完成，启用按钮
        for (int i = QUICK_BUTTON; i <= RANDOM_BUTTON; ++i) {
            Sexy::ButtonWidget *aButton = ((Sexy::ButtonWidget *)FindWidget(i));
            aButton->SetDisabled(false);
            (*aButton->mColors)[ButtonWidget::COLOR_LABEL] = Color(25, 197, 45);
            (*aButton->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = Color(277, 225, 108);
        }
        gGamepad1ToPlayerIndex = mSides[0];

        if (Challenge::msVSShuffleMode) {
            if (gOpeningEncounter && Rand(10) == 0 && !(gTcpConnected || gIsServerModeSpectator || gIsReplayMode)) {
                gOpeningEncounter->mType = EncounterType(Rand(NUM_ENCOUNTER));
                gOpeningEncounter->OpeningEncounterInitialize(gOpeningEncounter->mType);
                if (gTcpClientSocket >= 0) {
                    U16_Event event = {{EventType::EVENT_SERVER_ENCOUNTER_PICK}, uint16_t(gOpeningEncounter->mType)};
                    netplay::PutEvent(event);
                }
            }
            ButtonDepress(VSSetupMenu_Random_Battle);
        }
    }

    old_VSSetupMenu_OnStateEnter(this, theState);

    // 禁选模式僵尸先手禁用（即从植物方开始）
    if (mState == VS_SETUP_STATE_CUSTOM_BATTLE && mAddonWidget->mBanMode) {
        mSeedPickTurn = VSSide::VS_SIDE_PLANT;
    }
}

void VSSetupMenu::ButtonPress(int theId) {
    old_VSSetupMenu_ButtonPress(this, theId);
}

void VSSetupMenu::ButtonDepress(int theId) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        LOG_INFO("[VSSETUP] ignore local ButtonDepress in read-only mode id={}", theId);
        return;
    }
    if (gVSSetupRequestState == theId) {
        gVSSetupRequestState = 0;
    }

    // These controls own a local overlay and must not be sent through the
    // VS setup event stream. Builtin AI is local-only by design.
    if (mAddonWidget != nullptr && (theId == VSSetupAddonWidget::VSSetupAddonWidget_AISettings || theId == VSSetupAddonWidget::VSSetupAddonWidget_AISettingsClose)) {
        mAddonWidget->ButtonDepress(theId);
        return;
    }

    if (gTcpConnected) {
        U8_Event event = {{EventType::EVENT_CLIENT_VSSETUPMENU_BUTTON_DEPRESS}, uint8_t(theId)};
        netplay::PutEvent(event);
        gVSSetupRequestState = theId;
        return;
    }

    if (gTcpClientSocket >= 0) {
        U8_Event event = {{EventType::EVENT_SERVER_VSSETUPMENU_BUTTON_DEPRESS}, uint8_t(theId)};
        netplay::PutEvent(event);
    }

    ButtonDepress_Origin(theId);
}

void VSSetupMenu::ButtonDepress_Origin(int theId) {
    // 拓展功能开启情况
    if (mAddonWidget) {
        netplay::MetricsSetAddonFlags(mAddonWidget->mExtraPacketMode, mAddonWidget->mExtendedSeedsMode, mAddonWidget->mBanMode, mAddonWidget->mBalancePatchMode);
    }

    int aNumPackets = mApp->mBoard->GetNumSeedsInBank(false);

    SeedBank *aPlantBank = mApp->mBoard->mSeedBank[0];
    SeedBank *aZombieBank = mApp->mBoard->mSeedBank[1];

    aPlantBank->mNumPackets = aZombieBank->mNumPackets = aNumPackets;

    SeedType aSunPlantType = mApp->mBoard->StageIsNight() ? SeedType::SEED_SUNSHROOM : SeedType::SEED_SUNFLOWER;

    if (mState == VSSetupState::VS_SETUP_STATE_SELECT_BATTLE) {
        switch (theId) {
            case VSSetupMenu_Quick_Play: {
                // 所选战场
                netplay::MetricsSetBattleType(VSSetupMenu_Quick_Play);
                for (int aPlayerIndex = 0; aPlayerIndex < 2; ++aPlayerIndex) {
                    for (int aPacketIndex = 0; aPacketIndex < 6; ++aPacketIndex) {
                        SeedType aSeedType = msQuickPlayDecks[aPlayerIndex][aPacketIndex];
                        mApp->mBoard->mSeedBank[aPlayerIndex]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                    }
                }

                mApp->mBoard->mSeedBank[0]->mSeedPackets[0].SetPacketType(aSunPlantType, SeedType::SEED_NONE);

                mSetupMode = VSSetupMode::VS_SETUP_MODE_QUICK_PLAY;
                CloseVSSetup(false);

                if (mAddonWidget->mExtraPacketMode) { // 额外卡槽
                    aPlantBank->mSeedPackets[6].SetPacketType(SeedType::SEED_TORCHWOOD, SeedType::SEED_NONE);
                    aZombieBank->mSeedPackets[6].SetPacketType(SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_NONE);
                }
            } break;

            case VSSetupMenu_Custom_Battle: {
                netplay::MetricsSetBattleType(VSSetupMenu_Custom_Battle);
                mApp->ShowSeedChooserScreen();
                mApp->ShowZombieChooserScreen();

                for (int aPlayerIndex = 0; aPlayerIndex < 2; ++aPlayerIndex) {
                    if (mSides[aPlayerIndex] == VSSide::VS_SIDE_ZOMBIE) {
                        mApp->mZombieChooserScreen->mPlayerIndex = mControllerIndex[aPlayerIndex];
                    } else if (mSides[aPlayerIndex] == VSSide::VS_SIDE_PLANT) {
                        mApp->mSeedChooserScreen->mPlayerIndex = mControllerIndex[aPlayerIndex];
                    }
                }

                mSetupMode = VSSetupMode::VS_SETUP_MODE_CUSTOM_BATTLE;
                GoToState(VSSetupState::VS_SETUP_STATE_CUSTOM_BATTLE);

                if (mState == VS_SETUP_STATE_CUSTOM_BATTLE) {
                    mAddonWidget->SetDisable(mAddonWidget->mExtraPacketCheckbox);
                    mAddonWidget->SetDisable(mAddonWidget->mExtendedSeedsCheckbox);
                    mAddonWidget->SetDisable(mAddonWidget->mBanModeCheckbox);
                    mAddonWidget->SetDisable(mAddonWidget->mBalancePatchCheckbox);
                    mAddonWidget->SetDisable(mAddonWidget->mAISettingsButton);
                    mAddonWidget->SetDisable(mAddonWidget->mBackButton);
                    mAddonWidget->mDrawString = false;
                    //                    PickBackgroundImmediately();
                }
            } break;

            case VSSetupMenu_Random_Battle: {
                netplay::MetricsSetBattleType(VSSetupMenu_Random_Battle);
                std::vector<SeedType> aZombieSeeds;
                PickRandomZombies(aZombieSeeds);

                mApp->mBoard->mSeedBank[1]->mSeedPackets[0].SetPacketType(SeedType::SEED_ZOMBIE_GRAVESTONE, SeedType::SEED_NONE);

                if (!aZombieSeeds.empty()) {
                    for (int aPacketIndex = 1; aPacketIndex <= aZombieSeeds.size(); ++aPacketIndex) {
                        SeedType aSeedType = aZombieSeeds[aPacketIndex - 1];
                        mApp->mBoard->mSeedBank[1]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                    }
                }

                std::vector<SeedType> aPlantSeeds;
                PickRandomPlants(aPlantSeeds, aZombieSeeds);

                mApp->mBoard->mSeedBank[0]->mSeedPackets[0].SetPacketType(aSunPlantType, SeedType::SEED_NONE);

                if (!aPlantSeeds.empty()) {
                    for (int aPacketIndex = 1; aPacketIndex <= aPlantSeeds.size(); ++aPacketIndex) {
                        SeedType aSeedType = aPlantSeeds[aPacketIndex - 1];
                        mApp->mBoard->mSeedBank[0]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                    }
                }

                if (Challenge::msVSShuffleMode) {
                    gFreeForFristShuffle[0] = gFreeForFristShuffle[1] = true;
                    aPlantBank->mNumPackets = aZombieBank->mNumPackets = 7;
                    aZombieSeeds.clear();
                    aPlantSeeds.clear();
                    PickShuffleSeeds(mApp, aPlantSeeds, aZombieSeeds, true);
                    if (!aZombieSeeds.empty()) {
                        for (int aPacketIndex = 1; aPacketIndex <= aZombieSeeds.size(); ++aPacketIndex) {
                            SeedType aSeedType = aZombieSeeds[aPacketIndex - 1];
                            mApp->mBoard->mSeedBank[1]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                        }
                    }
                    PickShuffleSeeds(mApp, aPlantSeeds, aZombieSeeds, false);
                    if (!aPlantSeeds.empty()) {
                        for (int aPacketIndex = 1; aPacketIndex <= aPlantSeeds.size(); ++aPacketIndex) {
                            SeedType aSeedType = aPlantSeeds[aPacketIndex - 1];
                            mApp->mBoard->mSeedBank[0]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                        }
                    }
                    aPlantBank->mSeedPackets[6].SetPacketType(SEED_BEGHOULED_BUTTON_SHUFFLE, SeedType::SEED_NONE);
                    aZombieBank->mSeedPackets[6].SetPacketType(SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE, SeedType::SEED_NONE);
                }

                mSetupMode = VSSetupMode::VS_SETUP_MODE_RANDOM_BATTLE;
                CloseVSSetup(false);

                if (gTcpClientSocket >= 0) {
                    U16x12_Event event{};
                    event.type = EventType::EVENT_VSSETUPMENU_RANDOM_PICK;
                    std::ranges::copy(aPlantSeeds, event.data);
                    std::ranges::copy(aZombieSeeds, event.data + 6);
                    netplay::PutEvent(event);
                }
            } break;

            default:
                break;
        }
    }

    // 修复“额外卡槽”开启后卡槽位置不正确
    for (int i = 0; i < SEEDBANK_MAX; i++) {
        SeedPacket *aPlantPacket = &aPlantBank->mSeedPackets[i];
        SeedPacket *aZombiePacket = &aZombieBank->mSeedPackets[i];
        aPlantPacket->mIndex = i;
        aPlantPacket->mX = mApp->mBoard->GetSeedPacketPositionX(i, 0, false);
        aZombiePacket->mX = mApp->mBoard->GetSeedPacketPositionX(i, 1, true);
    }

    switch (theId) {
        case VSSetupAddonWidget::VSSetupAddonWidget_Back: // 返回模式选择
        case VSSetupAddonWidget::VSSetupAddonWidget_GlobalBP:
        case VSSetupAddonWidget::VSSetupAddonWidget_AISettings:
        case VSSetupAddonWidget::VSSetupAddonWidget_AISettingsClose:
            mAddonWidget->ButtonDepress(theId);
            break;
        default:
            break;
    }
}

// void VSSetupMenu::PickBackgroundImmediately() {
//     // 如果修改器里开启了更换场地
//     if (VSBackGround != 0 && VSBackGround != mApp->mBoard->mBackground + 1) {
//
//         if (gTcpConnected) {
//             // 客户端
//             return;
//         }
//
//         for (int i = 0; i < 6; ++i) {
//             mApp->RemoveReanimation(mApp->mBoard->mCoverLayerAnimIDs[i]);
//         }
//         mApp->mBoard->PickBackground(); // 立即更换
//         mApp->mBoard->RemoveAllMowers();
//         mApp->mBoard->RemoveAllPlants();
//         mApp->mBoard->RemoveAllGridItems();
//         mApp->mBoard->mCutScene->mPlacedLawnItems = false;
//         mApp->mBoard->mCutScene->PlaceLawnItems();
//
//
//         if (gTcpClientSocket >= 0) {
//             U8_Event event = {{EventType::EVENT_SERVER_VSSETUPMENU_PICKBACKGROUND}, uint8_t(VSBackGround)};
//             netplay::PutEvent(event);
//         }
//     }
// }


void VSSetupMenu::CloseVSSetup(bool theShowGameSelector) {
    //    PickBackgroundImmediately();

    old_VSSetupMenu_CloseVSSetup(this, theShowGameSelector);
}
