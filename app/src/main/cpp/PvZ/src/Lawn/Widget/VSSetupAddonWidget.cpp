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

#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "Homura/Logger.h"
#include "Homura/MemberUtils.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Widget/Checkbox.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

using namespace Sexy;

VSSetupAddonWidget::VSSetupAddonWidget(VSSetupMenu *theVSSetupMenu) {
    mButtonListener = theVSSetupMenu;

    if (!msGlobalBpSeedsInitialized) {
        for (auto &row : msGlobalBpSeeds) {
            for (SeedType &seedType : row) {
                seedType = SeedType::SEED_NONE;
            }
        }
        msGlobalBpSeedsInitialized = true;
    }

    mBackButton = MakeNewButton(VSSetupAddonWidget::VSSetupAddonWidget_Back,
                                mButtonListener,
                                theVSSetupMenu,
                                "[BACK_TO_MODE_SELECT]",
                                nullptr,
                                Sexy::IMAGE_SEEDCHOOSER_BUTTON_DISABLED,
                                Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW,
                                Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW);
    mBackButton->mTextOffsetX = -2;
    mBackButton->mTextOffsetY = -4;
    mBackButton->mTextDownOffsetX = 1;
    mBackButton->mTextDownOffsetY = 1;
    mBackButton->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
    (*mBackButton->mColors)[ButtonWidget::COLOR_LABEL] = Color(0, 205, 0);
    mBackButton->Resize(800, 540, 160, 50);
    mBoard->AddWidget(mBackButton);

    mGlobalBpButton = MakeButton(VSSetupAddonWidget::VSSetupAddonWidget_GlobalBP, mButtonListener, theVSSetupMenu, "[VS_UI_GLOBAL_BP_CLOSED]");
    mGlobalBpButton->Resize(VS_ADDON_BUTTON_X, 360, 175, 50);
    mBoard->AddWidget(mGlobalBpButton);

    mExtraPacketMode = mApp->mPlayerInfo->mVSExtraPacketMode;
    mExtendedSeedsMode = mApp->mPlayerInfo->mVSExtendedSeedsMode;
    mBanMode = mApp->mPlayerInfo->mVSBanMode;
    mBalancePatchMode = mApp->mPlayerInfo->mVSBalancePatchMode;
    mPlantAIMode = mApp->mPlayerInfo->mVSPlantAIMode;
    mZombieAIMode = mApp->mPlayerInfo->mVSZombieAIMode;
    mAIEnhancementMode = mApp->mPlayerInfo->mVSAIEnhancementMode;
    mAIDraftDisabledMode = mApp->mPlayerInfo->mVSAIDraftDisabledMode;
    mAITemplateDeckDisabledMode = mApp->mPlayerInfo->mVSAITemplateDeckDisabledMode;
    msBalancePatchMode = mBalancePatchMode;
    msExtraPacketMode = mExtraPacketMode;
    msExtendedSeedsMode = mExtendedSeedsMode;
    msPlantAIMode = mPlantAIMode;
    msZombieAIMode = mZombieAIMode;
    msAIEnhancementMode = mAIEnhancementMode;
    msAIDraftDisabledMode = mAIDraftDisabledMode;
    msAITemplateDeckDisabledMode = mAITemplateDeckDisabledMode;

    mExtraPacketCheckbox = MakeNewCheckbox(VSSetupAddonWidget_ExtraPacket, this, theVSSetupMenu, mExtraPacketMode);
    mExtendedSeedsCheckbox = MakeNewCheckbox(VSSetupAddonWidget_ExtendedSeeds, this, theVSSetupMenu, mExtendedSeedsMode);
    mBanModeCheckbox = MakeNewCheckbox(VSSetupAddonWidget_BanMode, this, theVSSetupMenu, mBanMode);
    mBalancePatchCheckbox = MakeNewCheckbox(VSSetupAddonWidget_BalancePatch, this, theVSSetupMenu, mBalancePatchMode);
    mAISettingsButton = MakeButton(VSSetupAddonWidget_AISettings, mButtonListener, theVSSetupMenu, "[VS_UI_AI_SETTINGS]");
    mAISettingsButton->mDrawStoneButton = true;

    mBoard->AddWidget(mExtraPacketCheckbox);
    mBoard->AddWidget(mExtendedSeedsCheckbox);
    mBoard->AddWidget(mBanModeCheckbox);
    mBoard->AddWidget(mBalancePatchCheckbox);
    mBoard->AddWidget(mAISettingsButton);

    mExtraPacketCheckbox->Resize(VS_ADDON_BUTTON_X, VS_BUTTON_EXTRA_PACKET_Y, 175, 50);
    mExtendedSeedsCheckbox->Resize(VS_ADDON_BUTTON_X, VS_BUTTON_EXTENDED_SEEDS_Y, 175, 50);
    mBanModeCheckbox->Resize(VS_ADDON_BUTTON_X, VS_BUTTON_BAN_MODE_Y, 175, 50);
    mBalancePatchCheckbox->Resize(VS_ADDON_BUTTON_X, VS_BUTTON_BALANCE_PATCH_Y, 175, 50);
    mAISettingsButton->Resize(VS_ADDON_BUTTON_X, VS_BUTTON_AI_SETTINGS_Y, 175, 50);

    UpdateGlobalBpButtonState();

    if (Challenge::msVSShuffleMode) {
        SetDisable(mExtraPacketCheckbox);
        SetDisable(mExtendedSeedsCheckbox);
        SetDisable(mBanModeCheckbox);
        SetDisable(mBalancePatchCheckbox);
        SetDisable(mAISettingsButton);
        SetDisable(mGlobalBpButton);
        mBanMode = false;
    }
}

VSSetupAddonWidget::~VSSetupAddonWidget() {
    CloseAISettings();
    if (mBoard) {
        if (mBackButton) {
            mBoard->RemoveWidget(mBackButton);
        }
        if (mExtraPacketCheckbox) {
            mBoard->RemoveWidget(mExtraPacketCheckbox);
        }
        if (mExtendedSeedsCheckbox) {
            mBoard->RemoveWidget(mExtendedSeedsCheckbox);
        }
        if (mBanModeCheckbox) {
            mBoard->RemoveWidget(mBanModeCheckbox);
        }
        if (mBalancePatchCheckbox) {
            mBoard->RemoveWidget(mBalancePatchCheckbox);
        }
        if (mAISettingsButton) {
            mBoard->RemoveWidget(mAISettingsButton);
        }
        if (mGlobalBpButton) {
            mBoard->RemoveWidget(mGlobalBpButton);
        }
    }

    delete mBackButton;
    delete mExtraPacketCheckbox;
    delete mExtendedSeedsCheckbox;
    delete mBanModeCheckbox;
    delete mBalancePatchCheckbox;
    delete mAISettingsButton;
    delete mGlobalBpButton;
}

void VSSetupAddonWidget::ResetGlobalBpState() {
    VSSetupMenu::msNextSidePickPlayerIndex = 0;
    msGlobalBpMode = GLOBALBP_CLOSED;
    msGlobalBpWins[0] = 0;
    msGlobalBpWins[1] = 0;
    for (auto &row : msGlobalBpSeeds) {
        for (SeedType &seedType : row) {
            seedType = SEED_NONE;
        }
    }
    msGlobalBpSeedsInitialized = true;
}

void VSSetupAddonWidget::SetDisable(Sexy::Widget *theWidget) {
    theWidget->mDisabled = true;
    theWidget->SetVisible(false);
    if (theWidget == mBanModeCheckbox) {
        UpdateGlobalBpButtonState();
    }
}

void VSSetupAddonWidget::UpdateGlobalBpButtonState() const {
    if (mGlobalBpButton == nullptr) {
        return;
    }

    const bool enabled = !mBanModeCheckbox->mDisabled && mBanMode && mExtendedSeedsMode;
    mGlobalBpButton->SetVisible(enabled);
    mGlobalBpButton->mDisabled = !enabled;
    switch (msGlobalBpMode) {
        case GLOBALBP_BO3:
            mGlobalBpButton->SetLabel("[VS_UI_GLOBAL_BP_BO3]");
            break;
        case GLOBALBP_BO5:
            mGlobalBpButton->SetLabel("[VS_UI_GLOBAL_BP_BO5]");
            break;
        case GLOBALBP_CLOSED:
        default:
            mGlobalBpButton->SetLabel("[VS_UI_GLOBAL_BP_CLOSED]");
            break;
    }
}

void VSSetupAddonWidget::ButtonDepress(this VSSetupAddonWidget &self, int theId) {
    if (theId == VSSetupAddonWidget_Back) {
        self.mApp->mVSSetupMenu->CloseVSSetup(true);
        self.mApp->KillBoard();
        self.mApp->ShowChallengeScreen(ChallengePage::CHALLENGE_PAGE_VS);
        return;
    }
    if (theId == VSSetupAddonWidget_GlobalBP) {
        self.mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
        switch (msGlobalBpMode) {
            case GLOBALBP_CLOSED:
                msGlobalBpMode = GLOBALBP_BO3;
                break;
            case GLOBALBP_BO3:
                msGlobalBpMode = GLOBALBP_BO5;
                break;
            case GLOBALBP_BO5:
            default:
                msGlobalBpMode = GLOBALBP_CLOSED;
                break;
        }
        self.UpdateGlobalBpButtonState();
        return;
    }
    if (theId == VSSetupAddonWidget_AISettings) {
        self.OpenAISettings();
        return;
    }
    if (theId == VSSetupAddonWidget_AISettingsClose) {
        self.CloseAISettings();
    }
}

void VSSetupAddonWidget::CheckboxChecked(int theId, bool checked) {
    if (theId < VSSetupAddonWidget_ExtraPacket || theId > VSSetupAddonWidget_AITemplateDeckDisabled) {
        return;
    }
    // AI settings are intentionally local-only. They configure the local
    // builtin agents and must not change an online opponent's chooser.
    if (IsLocalAIOption(theId)) {
        mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
        SetAddonMode(theId, checked, true);
        return;
    }
    // guest 不能直接改选项，只能发起请求，随后回滚到当前状态
    if (gTcpConnected) {
        U8_Event event = {{EventType::EVENT_CLIENT_VSSETUP_ADDON_CHECKBOX_CHECKED}, uint8_t(theId)};
        netplay::PutEvent(event);
        gVSSetupRequestState = theId;
        SetAddonMode(theId, GetAddonMode(theId), false);
        return;
    }

    mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
    SetAddonMode(theId, checked, true);

    if (gVSSetupRequestState == theId) {
        gVSSetupRequestState = 0;
    }

    if (gTcpClientSocket >= 0 && !IsLocalAIOption(theId)) {
        U8U8_Event event = {{EventType::EVENT_SERVER_VSSETUP_ADDON_CHECKBOX_CHECKED}, uint8_t(theId), uint8_t(checked)};
        netplay::PutEvent(event);
    }
}

bool VSSetupAddonWidget::GetAddonMode(int theId) const {
    switch (theId) {
        case VSSetupAddonWidget_ExtraPacket:
            return mExtraPacketMode;
        case VSSetupAddonWidget_ExtendedSeeds:
            return mExtendedSeedsMode;
        case VSSetupAddonWidget_BanMode:
            return mBanMode;
        case VSSetupAddonWidget_BalancePatch:
            return mBalancePatchMode;
        case VSSetupAddonWidget_PlantAI:
            return mPlantAIMode;
        case VSSetupAddonWidget_ZombieAI:
            return mZombieAIMode;
        case VSSetupAddonWidget_AIEnhancement:
            return mAIEnhancementMode;
        case VSSetupAddonWidget_AIDraftDisabled:
            return mAIDraftDisabledMode;
        case VSSetupAddonWidget_AITemplateDeckDisabled:
            return mAITemplateDeckDisabledMode;
        default:
            return false;
    }
}

void VSSetupAddonWidget::SetAddonMode(int theId, bool checked, bool saveDetails) {
    LOG_DEBUG("{} {}", theId, checked);
    switch (theId) {
        case VSSetupAddonWidget_ExtraPacket:
            mExtraPacketMode = checked;
            mExtraPacketCheckbox->SetChecked(mExtraPacketMode, false);
            msExtraPacketMode = mExtraPacketMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSExtraPacketMode = mExtraPacketMode;
            }
            break;
        case VSSetupAddonWidget_ExtendedSeeds:
            mExtendedSeedsMode = checked;
            mExtendedSeedsCheckbox->SetChecked(mExtendedSeedsMode, false);
            msExtendedSeedsMode = mExtendedSeedsMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSExtendedSeedsMode = mExtendedSeedsMode;
            }
            if (!mExtendedSeedsMode) {
                msGlobalBpMode = GlobalBpMode::GLOBALBP_CLOSED;
            }
            UpdateGlobalBpButtonState();
            break;
        case VSSetupAddonWidget_BanMode:
            mBanMode = checked;
            mBanModeCheckbox->SetChecked(mBanMode, false);
            if (saveDetails) {
                mApp->mPlayerInfo->mVSBanMode = mBanMode;
            }
            if (!mBanMode) {
                msGlobalBpMode = GlobalBpMode::GLOBALBP_CLOSED;
            }
            UpdateGlobalBpButtonState();
            break;
        case VSSetupAddonWidget_BalancePatch:
            mBalancePatchMode = checked;
            mBalancePatchCheckbox->SetChecked(mBalancePatchMode, false);
            msBalancePatchMode = mBalancePatchMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSBalancePatchMode = mBalancePatchMode;
            }
            break;
        case VSSetupAddonWidget_PlantAI:
            mPlantAIMode = checked;
            msPlantAIMode = mPlantAIMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSPlantAIMode = mPlantAIMode;
            }
            break;
        case VSSetupAddonWidget_ZombieAI:
            mZombieAIMode = checked;
            msZombieAIMode = mZombieAIMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSZombieAIMode = mZombieAIMode;
            }
            break;
        case VSSetupAddonWidget_AIEnhancement:
            mAIEnhancementMode = checked;
            msAIEnhancementMode = mAIEnhancementMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSAIEnhancementMode = mAIEnhancementMode;
            }
            break;
        case VSSetupAddonWidget_AIDraftDisabled:
            mAIDraftDisabledMode = checked;
            msAIDraftDisabledMode = mAIDraftDisabledMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSAIDraftDisabledMode = mAIDraftDisabledMode;
            }
            break;
        case VSSetupAddonWidget_AITemplateDeckDisabled:
            mAITemplateDeckDisabledMode = checked;
            msAITemplateDeckDisabledMode = mAITemplateDeckDisabledMode;
            if (saveDetails) {
                mApp->mPlayerInfo->mVSAITemplateDeckDisabledMode = mAITemplateDeckDisabledMode;
            }
            break;
        default:
            break;
    }

    if (saveDetails) {
        mApp->mPlayerInfo->SaveDetails();
    }
    if (mAISettingsWidget != nullptr) {
        mAISettingsWidget->SyncState();
    }
}

void VSSetupAddonWidget::Draw(Graphics *g) const {
    if (!mDrawString)
        return;

    g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
    if (mExtraPacketCheckbox->mVisible) {
        g->SetColor(mExtraPacketMode ? Color(255, 255, 153) : Color(0, 205, 0, 255));
        g->DrawString(TodStringTranslate("[VS_UI_EXTRA_SLOTS]"), VS_ADDON_BUTTON_X + 40, VS_BUTTON_EXTRA_PACKET_Y + 25);
    }
    if (mExtendedSeedsCheckbox->mVisible) {
        g->SetColor(mExtendedSeedsMode ? Color(255, 255, 153) : Color(0, 205, 0, 255));
        g->DrawString(TodStringTranslate("[VS_UI_EXTRA_SEEDS]"), VS_ADDON_BUTTON_X + 40, VS_BUTTON_EXTENDED_SEEDS_Y + 25);
    }
    if (mBanModeCheckbox->mVisible) {
        g->SetColor(mBanMode ? Color(255, 255, 153) : Color(0, 205, 0, 255));
        g->DrawString(TodStringTranslate("[VS_UI_BAN_MODE]"), VS_ADDON_BUTTON_X + 40, VS_BUTTON_BAN_MODE_Y + 25);
        if (mBanMode) {
            g->SetColor(Color(205, 0, 0, 255));
            g->DrawString(TodStringTranslate("[VS_UI_BAN_PHASE_BIG]"), 200, 45);
        }
    }
    if (mBalancePatchCheckbox->mVisible) {
        g->SetColor(mBalancePatchMode ? Color(255, 255, 153) : Color(0, 205, 0, 255));
        g->DrawString(TodStringTranslate("[VS_UI_BALANCE_PATCH]"), VS_ADDON_BUTTON_X + 40, VS_BUTTON_BALANCE_PATCH_Y + 25);
    }
}

void VSSetupAddonWidget::OpenAISettings() {
    if (mAISettingsWidget != nullptr || mApp == nullptr || mAISettingsButton->mDisabled) {
        return;
    }
    mAISettingsWidget = new AISettingsWidget(this);
    mAISettingsWidget->SetDisabled(Challenge::msVSShuffleMode);
    mApp->mWidgetManager->AddWidget(mAISettingsWidget);
    mApp->mWidgetManager->BringToFront(mAISettingsWidget);
    mApp->mWidgetManager->SetFocus(mAISettingsWidget);
}

void VSSetupAddonWidget::CloseAISettings() {
    if (mAISettingsWidget == nullptr) {
        return;
    }
    AISettingsWidget *const settingsWidget = mAISettingsWidget;
    mAISettingsWidget = nullptr;
    if (mApp != nullptr) {
        mApp->mWidgetManager->RemoveWidget(settingsWidget);
        mApp->SafeDeleteWidget(settingsWidget);
    } else {
        delete settingsWidget;
    }
    if (mApp != nullptr && mAISettingsButton != nullptr) {
        mApp->mWidgetManager->SetFocus(mAISettingsButton);
    }
}

void PickMPRandomSeeds(LawnApp *theApp, std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> &theZombieSeeds, bool theIsZombie, bool theAllowCoffee) {
    std::vector<SeedType> &aSeeds = theIsZombie ? theZombieSeeds : thePlantSeeds;

    int alreadyPicked = 0;
    if (!theIsZombie && theAllowCoffee && Sexy::Rand(5) == 1) {
        aSeeds.push_back(SEED_INSTANT_COFFEE);
        alreadyPicked = 1;
    }

    const int poolGroupOffset = 3 * alreadyPicked;

    for (int num_possible = alreadyPicked; num_possible < theApp->mBoard->GetNumSeedsInBank(true) - 1; ++num_possible) {
        int pool = 0;
        if (num_possible == 2 || num_possible == 3)
            pool = 1;
        else if (num_possible == 4)
            pool = 2;

        const int poolBase = theIsZombie ? 6 + pool : poolGroupOffset + pool;

        int numSeedsInPool = 0;
        for (int i = 0; i < MAX_SEEDS_IN_POOL; ++i) {
            const SeedType aSeedType = VSSetupMenu::msShufflePools[poolBase][i];
            if (aSeedType == SeedType::SEED_NONE)
                break;

            ++numSeedsInPool;
        }

        SeedType aSeedType = SeedType::SEED_NONE;
        for (;;) {
            do {
                const int seedIndex = Sexy::Rand(numSeedsInPool);
                aSeedType = VSSetupMenu::msShufflePools[poolBase][seedIndex];
            } while (std::ranges::contains(aSeeds, aSeedType));

            if (theApp->HasSeedType(aSeedType, theIsZombie))
                break;
        }

        aSeeds.push_back(aSeedType);
    }

    if (theIsZombie) {
        //        if (NeedSeedZombieScreenDoor(theApp)) {
        //            auto it = std::ranges::find(aSeeds, SeedType::SEED_ZOMBIE_NEWSPAPER);
        //            if (it != aSeeds.end()) {
        //                *it = SeedType::SEED_ZOMBIE_SCREEN_DOOR;
        //            }
        //        }
        //        if (NeedSeedZombieYeti(theApp)) {
        //            auto it = std::ranges::find(aSeeds, SeedType::SEED_ZOMBIE_TRASHCAN);
        //            if (it != aSeeds.end()) {
        //                *it = SeedType::SEED_ZOMBIE_YETI;
        //            }
        //        }
    } else {
        if (NeedSeedTallnut(theApp)) {
            auto it = std::ranges::find(aSeeds, SeedType::SEED_WALLNUT);
            if (it != aSeeds.end()) {
                *it = SeedType::SEED_TALLNUT;
            }
        }
        if (NeedSeedUmbrella(theApp)) {
            auto it = std::ranges::find(aSeeds, SeedType::SEED_SPIKEWEED);
            if (it != aSeeds.end()) {
                *it = SeedType::SEED_UMBRELLA;
            }
        }
        if (NeedSeedMagnetshroom(theApp)) {
            auto it = std::ranges::find(aSeeds, SeedType::SEED_ICESHROOM);
            if (it != aSeeds.end()) {
                *it = SeedType::SEED_MAGNETSHROOM;
            }
        }
        //        if (NeedSeedSplitPea(theApp)) {
        //            auto it = std::ranges::find(aSeeds, SeedType::SEED_PEASHOOTER);
        //            if (it != aSeeds.end()) {
        //                *it = SeedType::SEED_SPLITPEA;
        //            }
        //        }
    }
}

void PickShuffleSeeds(LawnApp *theApp, std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> &theZombieSeeds, bool theIsZombie) {
    PickMPRandomSeeds(theApp, thePlantSeeds, theZombieSeeds, theIsZombie);

    if (theIsZombie) {
        if (NeedSeedZombieImp(theApp)) {
            auto it = std::ranges::find(theZombieSeeds, SeedType::SEED_ZOMBIE_NORMAL);
            if (it != theZombieSeeds.end()) {
                *it = SeedType::SEED_ZOMBIE_IMP;
            }
        }
        if (NeedSeedZombieSuperFanImp(theApp, theZombieSeeds, true)) {
            if (!theZombieSeeds.empty()) {
                theZombieSeeds[0] = SeedType::SEED_ZOMBIE_SUPER_FAN_IMP;
            }
        }
    } else {
        if (NeedSeedInstantCoffee(theApp, thePlantSeeds, true)) {
            if (!thePlantSeeds.empty()) {
                thePlantSeeds[0] = SeedType::SEED_INSTANT_COFFEE;
            }
        }
        if (NeedSeedTorchwood(theApp, thePlantSeeds, true)) {
            auto it = std::ranges::find(thePlantSeeds, SeedType::SEED_SPIKEWEED);
            if (it != thePlantSeeds.end()) {
                *it = SeedType::SEED_TORCHWOOD;
            }
        }
    }
}

SeedType PickNextRandomSeed(LawnApp *theApp, std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> &theZombieSeeds, bool theIsZombie, int theSeedIndex) {
    PickMPRandomSeeds(theApp, thePlantSeeds, theZombieSeeds, theIsZombie, false);
    SeedType aSeedType = theIsZombie ? theZombieSeeds[theSeedIndex - 1] : thePlantSeeds[theSeedIndex - 1];

    if (theIsZombie) {
        if (NeedSeedZombieSuperFanImp(theApp) && theSeedIndex == 1) {
            aSeedType = SeedType::SEED_ZOMBIE_SUPER_FAN_IMP;
        }
    } else {
        if (NeedSeedInstantCoffee(theApp) && theSeedIndex == 1) {
            aSeedType = SeedType::SEED_INSTANT_COFFEE;
        }
        if (NeedSeedTorchwood(theApp) && aSeedType == SeedType::SEED_SPIKEWEED) {
            aSeedType = SeedType::SEED_TORCHWOOD;
        }
    }

    return aSeedType;
}

bool NeedSeedInstantCoffee(LawnApp *theApp, const std::vector<SeedType> &thePlantSeeds, bool theIsShuffle) {
    if (theIsShuffle) {
        // 刷出的新卡组中有蘑菇
        for (SeedType aSeedType : thePlantSeeds) {
            if (Plant::IsNocturnal(aSeedType)) {
                return true;
            }
        }
    } else {
        // 种子栏存在可用的蘑菇
        for (int i = 1; i < 6; ++i) {
            SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[0]->mSeedPackets[i];
            if (Plant::IsNocturnal(aSeedPacket.mPacketType) && aSeedPacket.mActive) {
                return true;
            }
        }
    }

    // 场上存在未唤醒且头顶没有咖啡豆的植物
    Plant *aPlant = nullptr;
    while (theApp->mBoard->IteratePlants(aPlant)) {
        if (aPlant->mIsAsleep && !theApp->mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_ONLY_FLYING)) {
            return true;
        }
    }
    return false;
}

bool NeedSeedTallnut(LawnApp *theApp) {
    // 僵尸种子栏存在可用的蹦蹦僵尸
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[1]->mSeedPackets[i];
        if (aSeedPacket.mPacketType == SeedType::SEED_ZOMBIE_POGO && aSeedPacket.mActive) {
            return true;
        }
    }
    // 场上存在正在弹跳的蹦蹦僵尸
    Zombie *aZombie = nullptr;
    while (theApp->mBoard->IterateZombies(aZombie)) {
        if (aZombie->mMindControlled) {
            continue;
        }
        if (aZombie->IsBouncingPogo()) {
            return true;
        }
    }
    return false;
}

bool NeedSeedUmbrella(LawnApp *theApp) {
    // 僵尸种子栏存在可用的蹦极僵尸或投篮僵尸
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[1]->mSeedPackets[i];
        if ((aSeedPacket.mPacketType == SeedType::SEED_ZOMBIE_BUNGEE || aSeedPacket.mPacketType == SeedType::SEED_ZOMBIE_CATAPULT) && aSeedPacket.mActive) {
            return true;
        }
    }
    // 场上剩余篮球数大于15的投篮僵尸
    Zombie *aZombie = nullptr;
    while (theApp->mBoard->IterateZombies(aZombie)) {
        if (aZombie->IsDeadOrDying()) {
            continue;
        }
        if (aZombie->mZombieType == ZombieType::ZOMBIE_CATAPULT && aZombie->mSummonCounter > 15) {
            return true;
        }
    }
    return false;
}

bool IsIronItemZombieType(ZombieType theZombieType) {
    return theZombieType == ZombieType::ZOMBIE_PAIL || theZombieType == ZombieType::ZOMBIE_DOOR || theZombieType == ZombieType::ZOMBIE_FOOTBALL || theZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX
        || theZombieType == ZombieType::ZOMBIE_DIGGER || theZombieType == ZombieType::ZOMBIE_POGO || theZombieType == ZombieType::ZOMBIE_LADDER || theZombieType == ZombieType::ZOMBIE_TRASHCAN;
}

bool NeedSeedMagnetshroom(LawnApp *theApp) {
    // 僵尸种子栏存在2个以上可用的铁具类僵尸
    int aNumIronItemZombies = 0;
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[1]->mSeedPackets[i];
        ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(aSeedPacket.mPacketType);
        if (IsIronItemZombieType(aZombieType) && aSeedPacket.mActive) {
            ++aNumIronItemZombies;
        }
    }
    if (aNumIronItemZombies >= 2) {
        return true;
    }
    // 场上存在3个以上的铁具类僵尸
    int aCount = 0;
    Zombie *aZombie = nullptr;
    while (theApp->mBoard->IterateZombies(aZombie)) {
        if (aZombie->mMindControlled) {
            continue;
        }
        if (IsIronItemZombieType(aZombie->mZombieType)) {
            ++aCount;
        }
    }
    if (aCount >= 3) {
        return true;
    }
    return false;
}

bool NeedSeedSplitPea(LawnApp *theApp) {
    // 僵尸种子栏存在可用的矿工僵尸
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[1]->mSeedPackets[i];
        if (aSeedPacket.mPacketType == SeedType::SEED_ZOMBIE_DIGGER && aSeedPacket.mActive) {
            return true;
        }
    }
    // 场上存在存活且未被魅惑的矿工僵尸
    Zombie *aZombie = nullptr;
    while (theApp->mBoard->IterateZombies(aZombie)) {
        if (aZombie->IsDeadOrDying() || aZombie->mMindControlled) {
            continue;
        }
        if (aZombie->mZombieType == ZombieType::ZOMBIE_DIGGER) {
            return true;
        }
    }
    return false;
}

bool IsPeaSeedType(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_PEASHOOTER || theSeedType == SeedType::SEED_REPEATER || theSeedType == SeedType::SEED_THREEPEATER || theSeedType == SeedType::SEED_SPLITPEA
        || theSeedType == SeedType::SEED_GATLINGPEA;
}

bool IsPultSeedType(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_CABBAGEPULT || theSeedType == SeedType::SEED_KERNELPULT || theSeedType == SeedType::SEED_MELONPULT || theSeedType == SeedType::SEED_WINTERMELON;
}

int CountPeasOnScreen(LawnApp *theApp) {
    int aCount = 0;
    Plant *aPlant = nullptr;
    while (theApp->mBoard->IteratePlants(aPlant)) {
        if (IsPeaSeedType(aPlant->mSeedType)) {
            ++aCount;
        }
    }
    return aCount;
}

int CountPultsOnScreen(LawnApp *theApp) {
    int aCount = 0;
    Plant *aPlant = nullptr;
    while (theApp->mBoard->IteratePlants(aPlant)) {
        if (IsPultSeedType(aPlant->mSeedType)) {
            ++aCount;
        }
    }
    return aCount;
}

bool NeedSeedTorchwood(LawnApp *theApp, const std::vector<SeedType> &thePlantSeeds, bool theIsShuffle) {
    if (theIsShuffle) {
        // 刷出的新卡组中有两个以上的豌豆类植物
        int numPeasInBank = 0;
        for (SeedType aSeedType : thePlantSeeds) {
            if (IsPeaSeedType(aSeedType)) {
                ++numPeasInBank;
            }
            if (numPeasInBank >= 2) {
                return true;
            }
        }
    } else {
        // 种子栏存在2株以上可用的豌豆类种子替换地刺为火炬树桩
        int aNumPeasInBank = 0;
        for (int i = 1; i < 6; ++i) {
            SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[0]->mSeedPackets[i];
            if (IsPeaSeedType(aSeedPacket.mPacketType) && aSeedPacket.mActive) {
                ++aNumPeasInBank;
            }
            if (aNumPeasInBank >= 2) {
                return true;
            }
        }
    }

    // 场上存在3株以上的豌豆类植物
    if (CountPeasOnScreen(theApp) >= 3) {
        return true;
    }
    return false;
}

bool NeedSeedZombieImp(LawnApp *theApp) {
    // 种子栏存在可用的土豆雷
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[0]->mSeedPackets[i];
        if (aSeedPacket.mPacketType == SeedType::SEED_POTATOMINE && aSeedPacket.mActive) {
            return true;
        }
    }
    // 场上存在土豆雷
    Plant *aPlant = nullptr;
    while (theApp->mBoard->IteratePlants(aPlant)) {
        if (aPlant->mSeedType == SeedType::SEED_POTATOMINE) {
            return true;
        }
    }
    return false;
}

bool NeedSeedZombieScreenDoor(LawnApp *theApp) {
    bool hasPultOrFume = false;
    bool hasStrongPea = false;
    // 种子栏存在可用的寒冰射手或双发射手且无投手或大喷菇
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[0]->mSeedPackets[i];
        if ((IsPultSeedType(aSeedPacket.mPacketType) || aSeedPacket.mPacketType == SeedType::SEED_FUMESHROOM) && aSeedPacket.mActive) {
            hasPultOrFume = true;
        }
        if ((aSeedPacket.mPacketType == SeedType::SEED_SNOWPEA || aSeedPacket.mPacketType == SeedType::SEED_REPEATER) && aSeedPacket.mActive) {
            hasStrongPea = true;
        }
    }
    // 场上存在寒冰射手或双发射手且无投手或大喷菇
    Plant *aPlant = nullptr;
    while (theApp->mBoard->IteratePlants(aPlant)) {
        if (IsPultSeedType(aPlant->mSeedType) || aPlant->mSeedType == SeedType::SEED_FUMESHROOM) {
            hasPultOrFume = true;
        }
        if (aPlant->mSeedType == SeedType::SEED_SNOWPEA || aPlant->mSeedType == SeedType::SEED_REPEATER) {
            hasStrongPea = true;
        }
    }
    return !hasPultOrFume && hasStrongPea;
}

bool NeedSeedZombieYeti(LawnApp *theApp) {
    // 场上存在3株以上的投手类植物
    if (CountPultsOnScreen(theApp) >= 3) {
        return true;
    }
    // 种子栏存在可用的投手或大喷菇且无射手
    bool hasPea = false;
    bool hasPult = false;
    for (int i = 1; i < 6; ++i) {
        SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[0]->mSeedPackets[i];
        if ((IsPeaSeedType(aSeedPacket.mPacketType) || aSeedPacket.mPacketType == SeedType::SEED_SNOWPEA) && aSeedPacket.mActive) {
            hasPea = true;
        }
        if ((IsPultSeedType(aSeedPacket.mPacketType) || aSeedPacket.mPacketType == SeedType::SEED_FUMESHROOM) && aSeedPacket.mActive) {
            hasPult = true;
        }
    }
    return !hasPea && hasPult;
}

bool NeedSeedZombieSuperFanImp(LawnApp *theApp, const std::vector<SeedType> &theZombieSeeds, bool theIsShuffle) {
    if (theIsShuffle) {
        // 刷出的新卡组中有全明星僵尸
        for (SeedType aSeedType : theZombieSeeds) {
            if (aSeedType == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL) {
                return true;
            }
        }
    } else {
        // 种子栏存在可用的全明星僵尸
        for (int i = 1; i < 6; ++i) {
            SeedPacket &aSeedPacket = theApp->mBoard->mSeedBank[1]->mSeedPackets[i];
            if (aSeedPacket.mPacketType == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL && aSeedPacket.mActive) {
                return true;
            }
        }
    }
    return false;
}
