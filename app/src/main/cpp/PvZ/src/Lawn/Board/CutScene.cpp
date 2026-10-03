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

#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/Board/Projectile.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/NetplayLobbyWidget.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

using namespace Sexy;

void CutScene::ShowShovel() {
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN) {
        // 不绘制锤子铲子黄油按钮
        mBoard->mShowShovel = false;
        mBoard->mShowButter = false;
        mBoard->mShowHammer = false;
        return;
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        // 对战直接启用铲子
        mBoard->mShowShovel = true;
        return;
    }

    old_CutScene_ShowShovel(this);
}


void CutScene::Update() {
    if (mPreUpdatingBoard || !mBoard->mZombieWavesReady) {
        return;
    }
    if (mApp->mGameMode == GameMode::GAMEMODE_ADVENTURE_TWO_PLAYER || mApp->IsCoopMode()) {
        if (mApp->mSecondPlayerGamepadIndex == -1 && !mApp->GetDialog(Dialogs::DIALOG_CONTINUE) && NetplayLobbyWidget::GetInstance() == nullptr) {
            mApp->SetSecondPlayer(1);
            return;
        }
    }

    old_CutScene_Update(this);
}


void CutScene::UpdateZombiesWonMP() {}

void CutScene::UpdatePlantsWon() {}


void CutScene::AddFlowerPots() {
    old_CutScene_AddFlowerPots(this);
}

bool CutScene::Is2x2Zombie(ZombieType theZombieType) {
    return Zombie::IsGargantuar(theZombieType);
}

void CutScene::PlaceStreetZombies() {
    if (mPlacedZombies || !mBoard->mZombieWavesReady) {
        return;
    }

    mPlacedZombies = true;
    if (mApp->IsFinalBossLevel() || mApp->IsScaryPotterLevel() || mApp->IsIZombieLevel() || mApp->IsWhackAZombieLevel() || mApp->IsWallnutBowlingLevel() || mApp->IsVSMode()) {
        return;
    }

    // 以下统计出怪列表中各种可预览的僵尸的数量
    //    int aZombieValueTotal = 0;
    int aTotalZombieCount = 0;
    int aZombieTypeCount[ZombieType::EXTENDED_NUM_ZOMBIE_TYPES] = {0};

    for (int aWave = 0; aWave < mBoard->mNumWaves; aWave++) {
        for (int aZombieIndex = 0; aZombieIndex < MAX_ZOMBIES_IN_WAVE; aZombieIndex++) {
            ZombieType aZombieType = mBoard->mZombiesInWave[aWave][aZombieIndex];
            if (aZombieType == ZombieType::ZOMBIE_INVALID) {
                break;
            }

            //            aZombieValueTotal += GetZombieDefinition(aZombieType).mZombieValue;

            if (aZombieType == ZombieType::ZOMBIE_FLAG) {
                continue;
            }
            if (aZombieType == ZombieType::ZOMBIE_YETI && !mApp->IsStormyNightLevel()) {
                continue;
            }
            if (aZombieType == ZombieType::ZOMBIE_BOBSLED && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_BOBSLED_BONANZA) {
                continue;
            }

            ++aZombieTypeCount[aZombieType];
            ++aTotalZombieCount;
            if (aZombieType == ZombieType::ZOMBIE_BUNGEE || aZombieType == ZombieType::ZOMBIE_BOBSLED) {
                aZombieTypeCount[aZombieType] = 1; // 蹦极僵尸和雪橇僵尸至多仅允许有 1 只预览僵尸
            }
        }
    }

    // 谁笑到最后关卡，除雪人僵尸外，所有允许出怪的僵尸类型至少计入 1 只僵尸
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
        for (int aZombieType = 0; aZombieType < ZombieType::NUM_ZOMBIE_TYPES; aZombieType++) {
            if (aZombieType != ZombieType::ZOMBIE_YETI && mBoard->mZombieAllowed[aZombieType]) {
                aZombieTypeCount[aZombieType] = std::max(aZombieTypeCount[aZombieType], 1);
            }
        }
    }
    if (mBoard->StageHasPool()) {
        aZombieTypeCount[ZombieType::ZOMBIE_DUCKY_TUBE] = 1; // 泳池关卡，必定出现鸭子僵尸预览
    }

    bool aZombieGrid[5][5] = {{false}};
    int aPreviewCapacity = 10;
    if (mApp->IsLittleTroubleLevel()) {
        aPreviewCapacity = 15;
    } else if ((mApp->IsStormyNightLevel() && mApp->IsAdventureMode()) || mApp->IsMiniBossLevel()) {
        aPreviewCapacity = 18;
    }

    // 优先放置较大体型的僵尸，然后再放置较小体型的僵尸
    for (ZombieType aZombieType = ZombieType::ZOMBIE_NORMAL; aZombieType < ZombieType::EXTENDED_NUM_ZOMBIE_TYPES; aZombieType = ZombieType(aZombieType + 1)) {
        if (aZombieTypeCount[(int)aZombieType] && (Is2x2Zombie(aZombieType) || aZombieType == ZombieType::ZOMBIE_ZAMBONI)) {
            FindAndPlaceZombie(aZombieType, aZombieGrid);
        }
    }
    for (ZombieType aZombieType = ZombieType::ZOMBIE_NORMAL; aZombieType < ZombieType::EXTENDED_NUM_ZOMBIE_TYPES; aZombieType = ZombieType(aZombieType + 1)) {
        if (aZombieTypeCount[aZombieType] && !Is2x2Zombie(aZombieType) && aZombieType != ZombieType::ZOMBIE_ZAMBONI) {
            int aZombieNumInWave = aZombieTypeCount[aZombieType];
            int aZombiePreviewNum = aZombieNumInWave * aPreviewCapacity / aTotalZombieCount;
            aZombiePreviewNum = ClampInt(aZombiePreviewNum, 1, aZombieNumInWave);
            for (int i = 0; i < aZombiePreviewNum; i++) {
                FindAndPlaceZombie(aZombieType, aZombieGrid);
            }
        }
    }
}

void CutScene::PlaceLawnItems() {
    if (mPlacedLawnItems) {
        return;
    }
    mPlacedLawnItems = true;

    if (!IsSurvivalRepick()) {
        mBoard->InitLawnMowers();
        AddFlowerPots();
        mBoard->PlaceRake();
    }

    if (mApp->IsVSMode()) {
        // 对战固定开局植物由两端初始化，正式开局再同步 ID；不走运行期 AddPlant 的客户端拦截。
        int aNumRows = mBoard->StageHas6Rows() ? 6 : 5;
        SeedType aSunPlantType = mBoard->StageIsNight() ? SeedType::SEED_SUNSHROOM : SeedType::SEED_SUNFLOWER;
        for (int aRow = 0; aRow < aNumRows; ++aRow) {
            mBoard->AddMPTarget(8, aRow);
            if ((aRow == 1 || aRow == aNumRows - 2) || mBoard->StageIsNight()) { // 黑夜种满一列
                mBoard->AddAGraveStone(8, aRow);
                Plant *aPlant = mBoard->AddPlant_Origin(0, aRow, aSunPlantType, SeedType::SEED_NONE, -1, true);
                if (aSunPlantType == SeedType::SEED_SUNSHROOM) {
                    aPlant->mStateCountdown = 0;
                }
            }
            if (mBoard->StageHasPool() && (aRow == 2 || aRow == 3)) {
                for (int aCol = 0; aCol < 4; ++aCol) {
                    mBoard->AddPlant_Origin(aCol, aRow, SeedType::SEED_LILYPAD, SeedType::SEED_NONE, -1, true);
                }
            }
        }
    }
}

void CutScene::LoadUpsellChallengeScreen() {
    ClearUpsellBoard();
    mUpsellChallengeScreen = new ChallengeScreen(mApp, ChallengePage::CHALLENGE_PAGE_CHALLENGE);
}

void CutScene::ClearUpsellBoard() {
    for (int i = 0; i < MAX_GRID_SIZE_Y; i++) {
        mBoard->mIceTimer[i] = 0;
        mBoard->mIceMinX[i] = BOARD_WIDTH;
    }

    mBoard->mZombies.DataArrayFreeAll();
    mBoard->mPlants.DataArrayFreeAll();
    mBoard->mCoins.DataArrayFreeAll();
    mBoard->mProjectiles.DataArrayFreeAll();
    mBoard->mGridItems.DataArrayFreeAll();
    mBoard->mLawnMowers.DataArrayFreeAll();

    mBoard->RemoveAllPlants();
    mBoard->RemoveAllZombies();
    mBoard->RemoveAllGridItems();
    mBoard->RemoveAllMowers();

    TodParticleSystem *aParticle = nullptr;
    while (mBoard->IterateParticles(aParticle)) {
        aParticle->ParticleSystemDie();
    }
    Reanimation *aReanim = nullptr;
    while (mBoard->IterateReanimations(aReanim)) {
        aReanim->ReanimationDie();
    }
    mBoard->mPoolSparklyParticleID = ParticleSystemID::PARTICLESYSTEMID_NULL;

    if (mUpsellChallengeScreen) {
        delete mUpsellChallengeScreen;
        mUpsellChallengeScreen = nullptr;
    }
}

namespace {
int TimeSeedChooserSlideOnEnd = 4250;
int TimePanLeftEnd = 6000;
} // namespace

void CutScene::EndSeedChooser() {
    if (mApp->mSeedChooserScreen) {
        mApp->mSeedChooserScreen->mMouseVisible = false;
    }

    if (mApp->mZombieChooserScreen) {
        mApp->mZombieChooserScreen->mMouseVisible = false;
    }
    mSeedChoosing = false;
    mCutsceneTime = TimeSeedChooserSlideOnEnd + mCrazyDaveTime + 10;
    if (IsNonScrollingCutscene()) {
        mCutsceneTime = mCrazyDaveTime + TimePanLeftEnd;
    }
    mApp->mWidgetManager->SetFocus(mBoard);
    if (mApp->mGameMode == GAMEMODE_MP_VS) {
        //        mBoard->mChallenge->mSuddenDeathStartTick =  Sexy::GetTickCount();
        mBoard->mEnableGraveStones = true;
    }
}
