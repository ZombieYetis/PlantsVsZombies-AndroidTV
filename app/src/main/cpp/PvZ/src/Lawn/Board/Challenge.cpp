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

#include "PvZ/Lawn/Board/Challenge.h"
#include "Homura/Logger.h"
#include "PvZ/Android/IntroVideo.h"
#include "PvZ/Android/Native/BridgeApp.h"
#include "PvZ/Android/Native/NativeApp.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/OpeningEncounter.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Misc.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <cstddef>

using namespace Sexy;

SeedType gArtChallengeWallnut[MAX_GRID_SIZE_Y][MAX_GRID_SIZE_X] = {
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_WALLNUT, SEED_WALLNUT, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_WALLNUT, SEED_WALLNUT, SEED_WALLNUT, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
};

SeedType gArtChallengeSunFlower[MAX_GRID_SIZE_Y][MAX_GRID_SIZE_X] = {
    {SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_STARFRUIT, SEED_WALLNUT, SEED_WALLNUT, SEED_WALLNUT, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_UMBRELLA, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_UMBRELLA, SEED_UMBRELLA, SEED_UMBRELLA, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
};

SeedType gArtChallengeStarFruit[MAX_GRID_SIZE_Y][MAX_GRID_SIZE_X] = {
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_STARFRUIT, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_NONE, SEED_NONE, SEED_STARFRUIT, SEED_NONE, SEED_NONE},
    {SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE, SEED_NONE},
};

void Challenge::_constructor() {
    if (requestJumpSurvivalStage) {
        // 如果玩家按了无尽跳关
        if (mSurvivalStage > 0 || mApp->mGameScene == GameScenes::SCENE_PLAYING) {
            // 需要玩家至少已完成选种子，才能跳关。否则有BUG
            if (!IsOnlineServerModeActive() && !gIsReplayMode) {
                mSurvivalStage = targetWavesToJump;
            }
        }
        requestJumpSurvivalStage = false;
    }

    old_Challenge_Challenge(this);

    if (mApp->IsVSMode()) {
        ReanimatorEnsureDefinitionLoaded(REANIM_APPLE_CLOCK, true);
        Reanimation *aClockReanim = mApp->AddReanimation(335, 580, 0, REANIM_APPLE_CLOCK);
        aClockReanim->mAnimRate = 0;
        aClockReanim->mIsAttachment = true;
        mReanimChallenge = mApp->ReanimationGetID(aClockReanim);
        aClockReanim->OverrideScale(0.4f, 0.4f);

        mSuddenDeathStartTick = -1;
        mPauseStartTick = -1;
        Zombie::msDeadFollowers.clear();
        mBoard->mJacksonDanceMode = false;

        if (Challenge::msVSShuffleMode) {
            if (!gOpeningEncounter) {
                gOpeningEncounter = new OpeningEncounter();
            }
        }
    }
}

void Challenge::_destructor() {
    delete gOpeningEncounter;
    gOpeningEncounter = nullptr;

    old_Challenge_Delete(this);

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && heavyWeaponAccel) {
        Native::BridgeApp *bridgeApp = Native::BridgeApp::getSingleton();
        JNIEnv *env = bridgeApp->getJNIEnv();
        jobject activity = bridgeApp->mNativeApp->getActivity();
        jclass cls = env->GetObjectClass(activity);
        jmethodID methodID = env->GetMethodID(cls, "stopOrientationListener", "()V");
        env->CallVoidMethod(activity, methodID);
        env->DeleteLocalRef(cls);
    }
}

bool Challenge::IsMPSuddenDeath() const {
    if (gLawnApp->mGameMode != GameMode::GAMEMODE_MP_VS || mBoard->mMainCounter < 0) {
        return false;
    }
    return mBoard->mMainCounter > MP_SUDDEN_DEATH_START_COUNTER;
}

int Challenge::GetSuddenDeathCount() const {
    if (!IsMPSuddenDeath()) {
        return -1;
    }
    return mBoard->mMainCounter / MP_SUDDEN_DEATH_TICKS_PER_SECOND - MP_SUDDEN_DEATH_SECONDS;
}

void Challenge::Update() {
    if (requestJumpSurvivalStage) {
        // 如果玩家按了无尽跳关
        if (mSurvivalStage > 0 || mApp->mGameScene == GameScenes::SCENE_PLAYING) {
            // 需要玩家至少已完成选种子，才能跳关。否则有BUG
            if (!IsOnlineServerModeActive() && !gIsReplayMode) {
                mSurvivalStage = targetWavesToJump;
            }
        }
        requestJumpSurvivalStage = false;
    }

    GameMode aGameMode = mApp->mGameMode;
    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || aGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
        TutorialState mTutorialState = mBoard->mTutorialState;
        if (mTutorialState == TutorialState::TUTORIAL_ZEN_GARDEN_PICKUP_WATER || mTutorialState == TutorialState::TUTORIAL_ZEN_GARDEN_WATER_PLANT
            || mTutorialState == TutorialState::TUTORIAL_ZEN_GARDEN_KEEP_WATERING) {
            mBoard->mBoardMenuButton->mBtnNoDraw = true;
            mBoard->mBoardStoreButton->mBtnNoDraw = true;
            mBoard->mBoardStoreButton->mDisabled = true;
            mBoard->mBoardMenuButton->mDisabled = true;
        } else if (mApp->mCrazyDaveState != CrazyDaveState::CRAZY_DAVE_OFF) {
            mBoard->mBoardStoreButton->mBtnNoDraw = true;
            mBoard->mBoardMenuButton->mBtnNoDraw = true;
            mBoard->mBoardStoreButton->mDisabled = true;
            mBoard->mBoardMenuButton->mDisabled = true;
        } else if (mTutorialState == TutorialState::TUTORIAL_ZEN_GARDEN_VISIT_STORE) {
            mBoard->mBoardStoreButton->mBtnNoDraw = false;
            mBoard->mBoardMenuButton->mBtnNoDraw = true;
            mBoard->mBoardStoreButton->mDisabled = false;
            mBoard->mBoardMenuButton->mDisabled = true;
        } else {
            mBoard->mBoardStoreButton->mBtnNoDraw = false;
            mBoard->mBoardMenuButton->mBtnNoDraw = false;
            mBoard->mBoardStoreButton->mDisabled = false;
            mBoard->mBoardMenuButton->mDisabled = false;
        }
    }

    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        return;
    }

    if (mApp->IsStormyNightLevel()) {
        UpdateStormyNight();
    }

    if (mBoard->mPaused) {
        if (mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
            mChallengeGridX = -1;
            mChallengeGridY = -1;
        }
        return;
    }

    //    if (aGameMode == GAMEMODE_MP_VS && mSuddenDeathStartTick >= 0 && mApp->mGameScene == GameScenes::SCENE_PLAYING) {
    //        ++mSuddenDeathStartTick;
    //    }

    if (mApp->mGameMode == GAMEMODE_CHALLENGE_RAINING_SEEDS || mApp->IsStormyNightLevel()) {
        UpdateRain();
    }

    if (mApp->mGameScene == SCENE_PLAYING) {
        if (mApp->mGameMode == GAMEMODE_MULTI_PLAYER) {
            UpdateMPZombieBank();
        }
    } else if (mApp->mGameMode != GAMEMODE_TREE_OF_WISDOM) {
        return;
    }

    if (mBoard->HasConveyorBeltSeedBank(0)) {
        UpdateConveyorBelt(0);
        if (mApp->IsCoopMode()) {
            UpdateConveyorBelt(1);
        }
    }

    if (mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED || mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
        UpdateBeghouled();
    }

    if (mApp->IsScaryPotterLevel()) {
        ScaryPotterUpdate();
    }

    if (mApp->IsScaryPotterLevel() || mApp->IsWhackAZombieLevel()) {
        SeedBank *aSeedBank = mBoard->mSeedBank[0];
        if (aSeedBank != nullptr && aSeedBank->mY < 0) {
            int aSeedBankHeight = IMAGE_SEEDBANK != nullptr ? IMAGE_SEEDBANK->mHeight : 0;
            if (mBoard->CountSunBeingCollected(0) + mBoard->mSunMoney1 > 0 || aSeedBank->mY > -aSeedBankHeight) {
                aSeedBank->mY += 2;
                if (aSeedBank->mY > 0) {
                    aSeedBank->mY = 0;
                }
            }
        }
    }

    if (mApp->IsWhackAZombieLevel()) {
        WhackAZombieUpdate();
    }
    if (mApp->IsIZombieLevel()) {
        IZombieUpdate();
    }
    if (mApp->IsSlotMachineLevel()) {
        UpdateSlotMachine();
    }

    if (aGameMode == GAMEMODE_CHALLENGE_SPEED) {
        mBoard->UpdateGame();
    }
    if (aGameMode == GAMEMODE_CHALLENGE_RAINING_SEEDS) {
        UpdateRainingSeeds();
    }
    if (aGameMode == GAMEMODE_CHALLENGE_PORTAL_COMBAT) {
        UpdatePortalCombat();
    }

    if (mApp->IsSquirrelLevel()) {
        SquirrelUpdate();
    }

    if (aGameMode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM) {
        ZombiquariumUpdate();
    }
    if (aGameMode == GAMEMODE_TREE_OF_WISDOM) {
        TreeOfWisdomUpdate();
    }

    if (aGameMode == GAMEMODE_CHALLENGE_ICE) {
        if (mBoard->mMainCounter == 3000) {
            mApp->PlayFoley(FOLEY_FLOOP);
            mApp->PlaySample(Sexy::SOUND_LOSEMUSIC);
        }
    } else if (aGameMode == GAMEMODE_MP_VS) {
        UpdateMPGraveStones();

        bool aHasBobsledWithSled = false;
        Zombie *aZombie = nullptr;
        while (mBoard->IterateZombies(aZombie)) {
            if (aZombie->IsBobsledTeamWithSled()) {
                aHasBobsledWithSled = true;
                break;
            }
        }

        if (!aHasBobsledWithSled && mBoard->CanAddBobSledMP()) {
            int aBobSledStep = (IsMPSuddenDeath() && Challenge::gVSSuddenDeathMode == 1) ? 3 : 1; // 死斗时加速雪橇小队的生成
            mBobSledMPCounter -= aBobSledStep;
            if (mBobSledMPCounter <= 0) {
                mBobSledMPCounter = 6000;
                mBoard->AddZombie(ZOMBIE_BOBSLED, Zombie::ZOMBIE_WAVE_VS, true);
            }
        }

        bool aIsSuddenDeath = IsMPSuddenDeath();
        if (!mIsMPSuddenDeathNow && aIsSuddenDeath && !mBoard->mLevelAwardSpawned) {
            mIsMPSuddenDeathNow = true;
            mBoard->DisplayAdvice(TodStringTranslate("[SUDDEN_DEATH]"), MESSAGE_STYLE_BIG_MIDDLE_FAST, ADVICE_NONE);
        }

        if (aIsSuddenDeath && Challenge::gVSSuddenDeathMode == 2) {
            int aSuddenDeathCount = GetSuddenDeathCount();
            if (aSuddenDeathCount > 0 && aSuddenDeathCount % 15 == 0 && aSuddenDeathCount > 15 * mSuddenDeathBoomCount) {
                ++mSuddenDeathBoomCount;
                int aGridX = Sexy::Rand(MAX_GRID_SIZE_X);
                int aRowCount = mBoard->StageHas6Rows() ? MAX_GRID_SIZE_Y : 5;
                int aGridY = Sexy::Rand(aRowCount);
                int aCenterX = mBoard->GridToPixelX(aGridX, aGridY) + mBoard->GridCellWidth(aGridX, aGridY) / 2;
                int aCenterY = mBoard->GridToPixelY(aGridX, aGridY) + mBoard->GridCellHeight(aGridX, aGridY) / 2;

                mBoard->KillAllZombiesInRadius(aGridY, aCenterX, aCenterY, 1, 1, true, 127);
                mBoard->KillAllPlantsInRadius(aCenterX, aCenterY, 1);
                int aRenderOrder = Board::MakeRenderOrder(RENDER_LAYER_PARTICLE, aGridY, 0);
                mApp->AddTodParticle((float)aCenterX + 20.0f, (float)aCenterY, aRenderOrder, PARTICLE_BLASTMARK);
                mBoard->ShakeBoard(3, -4);

                GridItem *aGraveStone = mBoard->GetGraveStoneAt(aGridX, aGridY);
                if (aGraveStone != nullptr && aGraveStone->mGridItemType == GRIDITEM_GRAVESTONE) {
                    aGraveStone->GridItemDie();
                }

                GridItem *aCrater = mBoard->AddACrater(aGridX, aGridY);
                if (aCrater != nullptr) {
                    aCrater->mGridItemCounter = 18000;
                }
                LOG_DEBUG("BOOOOOOM: ({},{})", aGridX, aGridY);
            }
        } else if (aIsSuddenDeath && Challenge::gVSSuddenDeathMode == 0) {
            int aSuddenDeathCount = GetSuddenDeathCount();
            if (aSuddenDeathCount > 0 && aSuddenDeathCount % 60 == 0) {
                int aDisableRound = aSuddenDeathCount / 60 - 1;
                if (aDisableRound >= 0 && aDisableRound < 3 && Sexy::GetTickCount() % 10 == 0) {
                    for (int aPlayerIndex = 0; aPlayerIndex < 2; ++aPlayerIndex) {
                        SeedBank *aSeedBank = mBoard->mSeedBank[aPlayerIndex];
                        if (aSeedBank == nullptr) {
                            continue;
                        }

                        SeedType aDisabledSeed = SEED_NONE;
                        for (int aTryCount = 0; aTryCount < 64 && aDisabledSeed == SEED_NONE; ++aTryCount) {
                            SeedType aSeedType = aSeedBank->mSeedPackets[Sexy::Rand(10)].mPacketType;
                            if (aSeedType == SEED_NONE || ISMPSeedSuddenDeathDisabled(aPlayerIndex, aSeedType) || IsMPResourceProducer(aSeedType)) {
                                continue;
                            }
                            aDisabledSeed = aSeedType;
                        }

                        if (aDisabledSeed != SEED_NONE) {
                            if (aPlayerIndex == 0) {
                                mSuddenDeathDisableSeeds1[aDisableRound] = aDisabledSeed;
                            } else {
                                mSuddenDeathDisableSeeds2[aDisableRound] = aDisabledSeed;
                            }
                            LOG_DEBUG("Disabling Seed Round({}) Chooser({}) Seed({})", aDisableRound, aPlayerIndex, static_cast<int>(aDisabledSeed));
                        }
                    }
                }
            }
        }
    }

    if (aGameMode == GAMEMODE_CHALLENGE_LAST_STAND) {
        LastStandUpdate();
    }
    if (aGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON) {
        HeavyWeaponUpdate();
    }

    if (mApp->IsVSMode()) {
        Reanimation *aClockReanim = mApp->ReanimationGet(mReanimChallenge);
        if (mBoard->mMainCounter == 1) {
            aClockReanim->PlayReanim("anim_timer", REANIM_PLAY_ONCE_AND_HOLD, 0, 1.2f);
        }
        if (mBoard->mMainCounter == MP_SUDDEN_DEATH_START_COUNTER) {
            aClockReanim->PlayReanim("anim_ring", REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
            TriggerVibration(VibrationEffect::VIBRATION_ZOMBIE_RISE_FROM_GRAVE);
        }

        UpdateVSAddPlants();
        if (gOpeningEncounter) {
            gOpeningEncounter->Update();
        }
    }

    Reanimation *aChallengeReanim = mApp->ReanimationTryToGet(mReanimChallenge);
    if (aChallengeReanim != nullptr && aChallengeReanim->mIsAttachment) {
        aChallengeReanim->Update();
    }
}

void Challenge::UpdateVSAddPlants() const {
    if (IsRemoteClient()) {
        return;
    }
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }

    if (mBoard->StageHasPool() || mBoard->StageHasRoof()) {
        gVSAddUnderPlantsCounter--;
        if (gVSAddUnderPlantsCounter <= 0) {
            gVSAddUnderPlantsCounter = 3000;
            int aNumRows = mBoard->StageHas6Rows() ? 6 : 5;
            SeedType aUnderPlantType = mBoard->StageHasPool() ? SEED_LILYPAD : mBoard->StageHasRoof() ? SEED_FLOWERPOT : SEED_NONE;
            // 在每一行最左列的空格上生成垫子植物
            for (int aRow = 0; aRow < aNumRows; ++aRow) {
                int aCol = GetUnderPlantCol(aRow);
                if (mBoard->CanPlantAt(aCol, aRow, aUnderPlantType) == PLANTING_OK && CanPlantAt(aCol, aRow, aUnderPlantType) == PLANTING_OK) {
                    mBoard->AddPlant(aCol, aRow, aUnderPlantType, SeedType::SEED_NONE, -1, true);
                }
            }
        }
    }
}

int Challenge::GetUnderPlantCol(int theRow) const {
    bool aHasBasePlant = false;
    int aTargetCol = -1;

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        SeedType aUnderPlantType = mBoard->StageHasPool() ? SEED_LILYPAD : mBoard->StageHasRoof() ? SEED_FLOWERPOT : SEED_NONE;
        if (aPlant->mSeedType == aUnderPlantType && aPlant->mRow == theRow && !aPlant->NotOnGround()) {
            // 检查第一列是否有植物
            if (aPlant->mPlantCol == 0) {
                aHasBasePlant = true;
            }
            // 检查右边相邻位置的是否有植物
            Plant *aPlantOnRight = mBoard->GetTopPlantAt(aPlant->mPlantCol + 1, aPlant->mRow, PlantPriority::TOPPLANT_ONLY_UNDER_PLANT);
            if (aPlantOnRight == nullptr) {
                // 右边无植物, 比较列号寻找最左边(列号最小)的植物, 取其右一列
                if (aTargetCol == -1 || aPlant->mPlantCol < aTargetCol) {
                    aTargetCol = mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_ONLY_UNDER_PLANT)->mPlantCol + 1;
                }
            }
        }
    }
    // 若第一个无植物则返回0, 否则返回目标列
    return aHasBasePlant ? aTargetCol : 0;
}

void Challenge::HeavyWeaponFire(float a2, float a3) {
    // 设定重型武器子弹的发射角度
    if (a2 == 0 && a3 == 1) {
        a2 = angle1;
        a3 = angle2;
    }
    old_Challenge_HeavyWeaponFire(this, a2, a3);
}

void Challenge::HeavyWeaponReanimUpdate() const {
    Reanimation *heavyWeaponReanim = mApp->ReanimationTryToGet(mReanimHeavyWeaponID2);
    if (heavyWeaponReanim == nullptr)
        return;

    SexyTransform2D sexyTransform2D{};
    sexyTransform2D.Translate(-129.55, -71.45);
    sexyTransform2D.RotateRad(mHeavyWeaponAngle);
    sexyTransform2D.Translate(mHeavyWeaponX, mHeavyWeaponY);
    sexyTransform2D.Translate(129.55, 71.45);
    sexyTransform2D.Translate(0.0, -20.0);
    heavyWeaponReanim->mOverlayMatrix = sexyTransform2D;
}

void Challenge::HeavyWeaponUpdate() {
    // 设定重型武器动画的发射角度
    old_Challenge_HeavyWeaponUpdate(this);

    if (angle1 != 0) {
        mHeavyWeaponAngle = acosf(angle1) - 1.5708f;
        HeavyWeaponReanimUpdate();
    }
}

void Challenge::IZombieDrawPlant(Sexy::Graphics *g, Plant *thePlant) const {
    // 参照PC内测版源代码，在IZ模式绘制植物的函数开始前额外绘制纸板效果。

    Reanimation *mBodyReanim = mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
    if (mBodyReanim != nullptr) {
        IZombieSetPlantFilterEffect(thePlant, FilterEffect::FILTEREFFECT_WHITE);
        float aOffsetX = g->mTransX;
        float aOffsetY = g->mTransY;
        Color theColor;
        g->SetColorizeImages(true);

        g->mTransX = aOffsetX + 4.0f;
        g->mTransY = aOffsetY + 4.0f;
        theColor.mRed = 122;
        theColor.mGreen = 86;
        theColor.mBlue = 58;
        theColor.mAlpha = 255;
        g->SetColor(theColor);
        mBodyReanim->DrawRenderGroup(g, 0);

        g->mTransX = aOffsetX + 2.0f;
        g->mTransY = aOffsetY + 2.0f;
        theColor.mRed = 171;
        theColor.mGreen = 135;
        theColor.mBlue = 107;
        theColor.mAlpha = 255;
        g->SetColor(theColor);
        mBodyReanim->DrawRenderGroup(g, 0);

        g->mTransX = aOffsetX - 2.0f;
        g->mTransY = aOffsetY - 2.0f;
        theColor.mRed = 171;
        theColor.mGreen = 135;
        theColor.mBlue = 107;
        theColor.mAlpha = 255;
        g->SetColor(theColor);
        mBodyReanim->DrawRenderGroup(g, 0);

        g->mTransX = aOffsetX;
        g->mTransY = aOffsetY;
        theColor.mRed = 255;
        theColor.mGreen = 201;
        theColor.mBlue = 160;
        theColor.mAlpha = 255;
        g->SetColor(theColor);
        IZombieSetPlantFilterEffect(thePlant, FilterEffect::FILTEREFFECT_NONE);
        mBodyReanim->DrawRenderGroup(g, 0);

        IZombieSetPlantFilterEffect(thePlant, FilterEffect::FILTEREFFECT_NONE);
        g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
        g->SetColorizeImages(false);
    }
}

bool Challenge::IZombieEatBrain(Zombie *theZombie) {
    // 修复IZ脑子血量太高
    GridItem *aBrain = IZombieGetBrainTarget(theZombie);
    if (aBrain == nullptr)
        return false;

    theZombie->StartEating();
    // int mHealth = aBrain->mGridItemCounter - 1;
    int mHealth = aBrain->mGridItemCounter - 2; // 一次吃掉脑子的两滴血
    aBrain->mGridItemCounter = mHealth;
    if (mHealth <= 0) {
        mApp->PlaySample(SOUND_GULP);
        aBrain->GridItemDie();
        IZombieScoreBrain(aBrain);
    }
    return true;
}

void Challenge::DrawArtChallenge(Sexy::Graphics *g) const {
    // 绘制坚果的两只大眼睛
    g->SetColorizeImages(true);
    Color theColor = {255, 255, 255, 100};
    g->SetColor(theColor);

    for (int theGridY = 0; theGridY < 6; theGridY++) {
        for (int theGridX = 0; theGridX < 9; theGridX++) {
            SeedType ArtChallengeSeed = GetArtChallengeSeed(theGridX, theGridY);
            if (ArtChallengeSeed != SeedType::SEED_NONE && mBoard->GetTopPlantAt(theGridX, theGridY, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION) == nullptr) {
                int x = mBoard->GridToPixelX(theGridX, theGridY);
                int y = mBoard->GridToPixelY(theGridX, theGridY);
                Plant::DrawSeedType(g, ArtChallengeSeed, SeedType::SEED_NONE, DrawVariation::VARIATION_NORMAL, x, y);
            }
        }
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT) {
        Color theNewColor = {255, 255, 255, 255};
        g->SetColor(theNewColor);
        int x1 = mBoard->GridToPixelX(4, 1);
        int y1 = mBoard->GridToPixelY(4, 1);
        g->DrawImage(addonImages.googlyeye, x1, y1);
        int x2 = mBoard->GridToPixelX(6, 1);
        int y2 = mBoard->GridToPixelY(6, 1);
        g->DrawImage(addonImages.googlyeye, x2, y2);
    }

    g->SetColorizeImages(false);
}

PlantingReason Challenge::CanPlantAt(int theGridX, int theGridY, SeedType theSeedType) const {
    if (mApp->IsWallnutBowlingLevel()) {
        return theGridX > 2 ? PLANTING_NOT_PASSED_LINE : PLANTING_OK;
    } else if (mApp->IsIZombieLevel()) {
        int aLimit = 6;
        if (mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_1 || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_2 || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_3
            || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_4 || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_5) {
            aLimit = 4;
        }
        if (mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_6 || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_7 || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_8
            || mApp->mGameMode == GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS) {
            aLimit = 5;
        }
        // 修复IZ多个蹦极可放置在同一格子内
        if (theSeedType == SeedType::SEED_ZOMBIE_BUNGEE) {
            Zombie *aZombie = nullptr;
            while (mBoard->IterateZombies(aZombie)) {
                if (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE) {
                    int aGridX = mBoard->PixelToGridX(aZombie->mX, aZombie->mY);
                    if (aGridX == theGridX && aZombie->mRow == theGridY) {
                        return PlantingReason::PLANTING_NOT_HERE;
                    }
                }
            }
            return theGridX < aLimit ? PLANTING_OK : PLANTING_NOT_HERE;
        } else if (IsZombieSeedType(theSeedType)) {
            return theGridX >= aLimit ? PLANTING_OK : PLANTING_NOT_HERE;
        }
    } else if (mApp->IsArtChallenge()) {
        SeedType anArtSeed = GetArtChallengeSeed(theGridX, theGridY);
        if (anArtSeed != SeedType::SEED_NONE && anArtSeed != theSeedType && theSeedType != SeedType::SEED_LILYPAD && theSeedType != SeedType::SEED_PUMPKINSHELL) {
            return PLANTING_NOT_ON_ART;
        }
        if (mApp->mGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT) {
            if ((theGridX == 4 || theGridX == 6) && theGridY == 1) {
                return PLANTING_NOT_HERE;
            }
        }
    } else if (mApp->IsFinalBossLevel() && theGridX >= 8) {
        return PLANTING_NOT_HERE;
    } else if (mApp->IsVSMode()) {
        if (IsMPSeedType(theSeedType)) {
            if (theSeedType == SeedType::SEED_ZOMBIE_MOUND && mBoard->mPlantRow[theGridY] == PlantRowType::PLANTROW_POOL) {
                return PlantingReason::PLANTING_ONLY_ON_GROUND; // 召唤墓碑禁止放置在水路
            }
            ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(theSeedType);
            if (Challenge::IsMPZombieTypeCanGoInPool(aZombieType)) {
                if (Board::IsZombieTypePoolOnly(aZombieType) && mBoard->mPlantRow[theGridY] != PlantRowType::PLANTROW_POOL) {
                    return PLANTING_ONLY_IN_POOL; // 潜水僵尸和海豚骑士僵尸禁止放置在非水路
                }
            } else {
                if (theSeedType != SeedType::SEED_ZOMBIE_GRAVESTONE && mBoard->mPlantRow[theGridY] == PlantRowType::PLANTROW_POOL) {
                    return PLANTING_ONLY_ON_GROUND; // 部分僵尸类型禁止放置在水路
                }
            }

            if ((theSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE || theSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE)) {
                if (isKeyboardTwoPlayerMode)
                    return PlantingReason::PLANTING_OK;
                return PlantingReason::PLANTING_NOT_HERE;
            }

            return (theGridX > 5 || theSeedType == SeedType::SEED_ZOMBIE_BUNGEE) ? PLANTING_OK : PLANTING_NOT_PASSED_LINE_VS;
        }
        if (theSeedType == SeedType::SEED_GRAVEBUSTER) {
            if (mBoard->GetGridItemAt(GridItemType::GRIDITEM_GRAVESTONE, theGridX, theGridY) == nullptr
                && mBoard->GetGridItemAt(GridItemType::GRIDITEM_MP_BURIAL_MOUND, theGridX, theGridY) == nullptr) {
                return PlantingReason::PLANTING_ONLY_ON_GRAVES;
            }
        } else {
            return theGridX <= 5 ? PLANTING_OK : PLANTING_NOT_PASSED_LINE_VS;
        }
    }
    return PlantingReason::PLANTING_OK;
}

void Challenge::InitLevel() {
    old_Challenge_InitLevel(this);

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN) {
        mBoard->NewPlant(0, 0, SeedType::SEED_COBCANNON, SeedType::SEED_NONE, -1);
        mBoard->NewPlant(0, 1, SeedType::SEED_COBCANNON, SeedType::SEED_NONE, -1);
        mBoard->NewPlant(0, 4, SeedType::SEED_COBCANNON, SeedType::SEED_NONE, -1);
        mBoard->NewPlant(0, 5, SeedType::SEED_COBCANNON, SeedType::SEED_NONE, -1);
    }

    // 为结盟僵王的2P传送带补充开局的4个固定植物
    if (mApp->mGameMode == GAMEMODE_TWO_PLAYER_COOP_BOSS) {
        mBoard->mSeedBank[1]->AddSeed(SeedType::SEED_CABBAGEPULT, false);
        mBoard->mSeedBank[1]->AddSeed(SeedType::SEED_JALAPENO, false);
        mBoard->mSeedBank[1]->AddSeed(SeedType::SEED_CABBAGEPULT, false);
        mBoard->mSeedBank[1]->AddSeed(SeedType::SEED_ICESHROOM, false);
        mConveyorBeltCounter2 = 1000;
    }

    if (mApp->IsVSMode()) {
        if (mBoard->StageHasPool() || mBoard->StageHasRoof()) {
            gVSAddUnderPlantsCounter = 2000;
        }
    } else {
        Challenge::msVSShuffleMode = false;
    }
}

void Challenge::StartLevel() {
    old_Challenge_StartLevel(this);

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && heavyWeaponAccel) {
        Native::BridgeApp *bridgeApp = Native::BridgeApp::getSingleton();
        JNIEnv *env = bridgeApp->getJNIEnv();
        jobject activity = bridgeApp->mNativeApp->getActivity();
        jclass cls = env->GetObjectClass(activity);
        jmethodID methodID = env->GetMethodID(cls, "startOrientationListener", "()V");
        env->CallVoidMethod(activity, methodID);
        env->DeleteLocalRef(cls);
    }

    if (mApp->IsVSMode()) {
        if (gOpeningEncounter) {
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUN_RAIN) {
                mBoard->DisplayAdvice("[ADVICE_SUN_RAIN_COMING]", MESSAGE_STYLE_HINT_FAST, ADVICE_NONE);
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUNNY_DAY) {
                mBoard->DisplayAdvice("[ADVICE_SUNNY_DAY]", MESSAGE_STYLE_HINT_FAST, ADVICE_NONE);
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_FREE_SHUFFLE) {
                mBoard->DisplayAdvice("[ADVICE_FREE_SHUFFLE]", MESSAGE_STYLE_HINT_FAST, ADVICE_NONE);
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_QUICK_SHUFFLE) {
                mBoard->DisplayAdvice("[ADVICE_QUICK_SHUFFLE]", MESSAGE_STYLE_HINT_FAST, ADVICE_NONE);
            }
        }
    }
}

void Challenge::InitZombieWaves() {
    old_Challenge_InitZombieWaves(this);

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN) {
        ZombieType zombieList[] = {
            ZombieType::ZOMBIE_NORMAL,
            ZombieType::ZOMBIE_TRAFFIC_CONE,
            ZombieType::ZOMBIE_PAIL,
            ZombieType::ZOMBIE_DOOR,
            ZombieType::ZOMBIE_FOOTBALL,
            ZombieType::ZOMBIE_NEWSPAPER,
            ZombieType::ZOMBIE_JACK_IN_THE_BOX,
            ZombieType::ZOMBIE_POLEVAULTER,
            ZombieType::ZOMBIE_DOLPHIN_RIDER,
            ZombieType::ZOMBIE_LADDER,
            ZombieType::ZOMBIE_GARGANTUAR,
        };
        InitZombieWavesFromList(zombieList, std::size(zombieList));
    }
}

void Challenge::TreeOfWisdomFertilize() {
    old_Challenge_TreeOfWisdomFertilize(this);

    // 检查智慧树成就
    PlayerInfo *playerInfo = mApp->mPlayerInfo;
    if (playerInfo->mChallengeRecords[GameMode::GAMEMODE_TREE_OF_WISDOM - 2] >= 99) {
        mBoard->GrantAchievement(AchievementType::ACHIEVEMENT_TREE, true);
    }
}

void Challenge::LastStandUpdate() {
    if (mBoard->mNextSurvivalStageCounter == 0 && mChallengeState == ChallengeState::STATECHALLENGE_NORMAL && mBoard->mBoardStoreButton->mBtnNoDraw) {
        mBoard->mBoardStoreButton->mBtnNoDraw = false;
        mBoard->mBoardStoreButton->mDisabled = false;
        pvzstl::string str = mSurvivalStage == 0 ? "[START_ONSLAUGHT]" : "[CONTINUE_ONSLAUGHT]";
        mBoard->mBoardStoreButton->SetLabel(str);
        mBoard->mBoardStoreButton->Resize(325, 555, 170, 120);
    }

    if (mChallengeState == ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT && mApp->mGameScene == GameScenes::SCENE_PLAYING) {
        mChallengeStateCounter++;
    }

    if (mChallengeState == ChallengeState::STATECHALLENGE_NORMAL && !mBoard->mBoardStoreButton->mBtnNoDraw) {
        mBoard->mBoardStoreButton->Resize(325, 555, 170, 120);
        mBoard->mBoardStoreButton->mBtnNoDraw = false;
        mBoard->mBoardStoreButton->mDisabled = false;
    }
}

int Challenge::IsMPSeedType(SeedType theSeedType) {
    return theSeedType >= SEED_ZOMBIE_GRAVESTONE && theSeedType < NUM_ZOMBIE_SEED_TYPES;
}

int Challenge::IsZombieSeedType(SeedType theSeedType) {
    return theSeedType >= SEED_ZOMBIE_GRAVESTONE && theSeedType < NUM_ZOMBIE_SEED_TYPES;
}

void Challenge::IZombieSetPlantFilterEffect(Plant *thePlant, FilterEffect theFilterEffect) const {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
    Reanimation *aHeadReanim = mApp->ReanimationTryToGet(thePlant->mHeadReanimID);
    Reanimation *aHeadReanim2 = mApp->ReanimationTryToGet(thePlant->mHeadReanimID2);
    Reanimation *aHeadReanim3 = mApp->ReanimationTryToGet(thePlant->mHeadReanimID3);
    if (aBodyReanim)
        aBodyReanim->mFilterEffect = theFilterEffect;
    if (aHeadReanim)
        aHeadReanim->mFilterEffect = theFilterEffect;
    if (aHeadReanim2)
        aHeadReanim2->mFilterEffect = theFilterEffect;
    if (aHeadReanim3)
        aHeadReanim3->mFilterEffect = theFilterEffect;
}

ZombieType Challenge::IZombieSeedTypeToZombieType(SeedType theSeedType) {
    // 此处可修改VS和IZ中的僵尸类型
    switch (theSeedType) {
        case SEED_ZOMBIE_NORMAL:
            return ZOMBIE_NORMAL;
        case SEED_ZOMBIE_TRASHCAN:
            return ZOMBIE_TRASHCAN;
        case SEED_ZOMBIE_TRAFFIC_CONE:
            return ZOMBIE_TRAFFIC_CONE;
        case SEED_ZOMBIE_POLEVAULTER:
            return ZOMBIE_POLEVAULTER;
        case SEED_ZOMBIE_PAIL:
            return ZOMBIE_PAIL;
        case SEED_ZOMBIE_FLAG:
            return ZOMBIE_FLAG;
        case SEED_ZOMBIE_NEWSPAPER:
            return ZOMBIE_NEWSPAPER;
        case SEED_ZOMBIE_SCREEN_DOOR:
            return ZOMBIE_DOOR;
        case SEED_ZOMBIE_FOOTBALL:
            return ZOMBIE_FOOTBALL;
        case SEED_ZOMBIE_DANCER:
            return ZOMBIE_DANCER;
        case SEED_ZOMBONI:
            return ZOMBIE_ZAMBONI;
        case SEED_ZOMBIE_JACK_IN_THE_BOX:
            return ZOMBIE_JACK_IN_THE_BOX;
        case SEED_ZOMBIE_DIGGER:
            return ZOMBIE_DIGGER;
        case SEED_ZOMBIE_POGO:
            return ZOMBIE_POGO;
        case SEED_ZOMBIE_BUNGEE:
            return ZOMBIE_BUNGEE;
        case SEED_ZOMBIE_LADDER:
            return ZOMBIE_LADDER;
        case SEED_ZOMBIE_CATAPULT:
            return ZOMBIE_CATAPULT;
        case SEED_ZOMBIE_GARGANTUAR:
            return ZOMBIE_GARGANTUAR;
        case SEED_ZOMBIE_YETI:
            return ZOMBIE_YETI;
        case SEED_ZOMBIE_BOBSLED:
            return ZOMBIE_BOBSLED;
        case SEED_ZOMBIE_SNORKEL:
            return ZOMBIE_SNORKEL;
        case SEED_ZOMBIE_DOLPHIN_RIDER:
            return ZOMBIE_DOLPHIN_RIDER;
        case SEED_ZOMBIE_IMP:
            return ZOMBIE_IMP;
        case SEED_ZOMBIE_BALLOON:
            return ZOMBIE_BALLOON;
        case SEED_ZOMBIE_PEA_HEAD:
            return ZOMBIE_PEA_HEAD; // 豌豆射手僵尸
        case SEED_ZOMBIE_WALLNUT_HEAD:
            return ZOMBIE_WALLNUT_HEAD; // 坚果僵尸
        case SEED_ZOMBIE_JALAPENO_HEAD:
            return ZOMBIE_JALAPENO_HEAD; // 火爆辣椒僵尸
        case SEED_ZOMBIE_GATLINGPEA_HEAD:
            return ZOMBIE_GATLING_HEAD; // 机枪射手僵尸
        case SEED_ZOMBIE_SQUASH_HEAD:
            return ZOMBIE_SQUASH_HEAD; // 窝瓜僵尸
        case SEED_ZOMBIE_TALLNUT_HEAD:
            return ZOMBIE_TALLNUT_HEAD; // 高坚果僵尸
        case SEED_ZOMBIE_GIGA_FOOTBALL:
            return ZOMBIE_GIGA_FOOTBALL;
        case SEED_ZOMBIE_SUPER_FAN_IMP:
            return ZOMBIE_SUPER_FAN_IMP;
        case SEED_ZOMBIE_JACKSON:
            return ZOMBIE_JACKSON;
        case SEED_ZOMBIE_GIGA_POLEVAULTER:
            return ZOMBIE_GIGA_POLEVAULTER;
        case SEED_ZOMBIE_SUNDAY_EDITION:
            return ZOMBIE_SUNDAY_EDITION;
        case SEED_ZOMBIE_EXPLORER:
            return ZOMBIE_EXPLORER;
        case SEED_ZOMBIE_ZOMBLOB:
            return ZOMBIE_ZOMBLOB;
        case SEED_ZOMBIE_GIGA_GARGANTUAR:
            return ZOMBIE_GIGA_GARGANTUAR;
        case SEED_ZOMBIE_DOGWALKER:
            return ZOMBIE_DOGWALKER;
        case SEED_ZOMBIE_TELEPORTATION:
            return ZOMBIE_TELEPORTATION;
        case SEED_ZOMBIE_SUPER_NOVA_GARGANTUAR:
            return ZOMBIE_SUPER_NOVA_GARGANTUAR;
        case SEED_ZOMBIE_CROSSING_GUARD:
            return ZOMBIE_CROSSING_GUARD;
        case SEED_ZOMBIE_SCIENTIST:
            return ZOMBIE_SCIENTIST;
        default:
            return ZOMBIE_INVALID;
    }
}

void Challenge::IZombiePlaceZombie(ZombieType theZombieType, int theGridX, int theGridY) const {
    Zombie *aZombie = mBoard->AddZombieInRow(theZombieType, theGridY, 0, false);
    if (theZombieType == ZOMBIE_BUNGEE) {
        aZombie->mTargetCol = theGridX;
        aZombie->SetRow(theGridY);
        aZombie->mPosX = mBoard->GridToPixelX(theGridX, theGridY);
        aZombie->mPosY = aZombie->GetPosYBasedOnRow(theGridY);
        aZombie->mRenderOrder = Board::MakeRenderOrder(RENDER_LAYER_GRAVE_STONE, theGridY, 7);
    } else {
        aZombie->mPosX = mBoard->GridToPixelX(theGridX, theGridY) - 30.0f;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (IsRemoteServer()) {
        U16UNI32UNI32_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_ADD_BY_CHEAT;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(aZombie));
        event.data2.u8x4.u8_1 = uint8_t(theGridX);
        event.data2.u8x4.u8_2 = uint8_t(theGridY);
        //            event.data3.u16x2.u16_2 = uint16_t(theZombieType);
        if (theZombieType == ZOMBIE_BUNGEE) {
            event.data3.f32 = aZombie->mAltitude;
        }
        netplay::PutEvent(event);
    }
}

void Challenge::DrawHeavyWeapon(Sexy::Graphics *g) {
    // 修复僵尸进家后重型武器关卡长草露馅
    g->DrawImage(Sexy::IMAGE_HEAVY_WEAPON_OVERLAY, -73, 559);
}

bool Challenge::UpdateZombieSpawning() {
    if (stopSpawning && !IsOnlineServerModeActive() && !gIsReplayMode) {
        return true;
    }

    return old_Challenge_UpdateZombieSpawning(this);
}

void Challenge::HeavyWeaponPacketClicked(SeedPacket *theSeedPacket) {
    // 修复疯狂点击毁灭菇导致GridItem数量超出上限而闪退
    if (theSeedPacket->mPacketType == SeedType::SEED_DOOMSHROOM) {
        GridItem *aGridItem = nullptr;
        while (mBoard->IterateGridItems(aGridItem)) {
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_CRATER) {
                aGridItem->GridItemDie();
            }
        }
    }

    old_Challenge_HeavyWeaponPacketClicked(this, theSeedPacket);
}

void Challenge::ScaryPotterOpenPot(GridItem *theScaryPot) {
    old_Challenge_ScaryPotterOpenPot(this, theScaryPot);
}

void Challenge::UpdateConveyorBelt(int thePlayerIndex) {
    old_Challenge_UpdateConveyorBelt(this, thePlayerIndex);
}

GridItem *Challenge::IZombieGetBrainTarget(Zombie *theZombie) {
    return old_Challenge_IZombieGetBrainTarget(this, theZombie);
    // if (theZombie->mZombieType == ZOMBIE_BUNGEE || theZombie->IsWalkingBackwards())
    // return nullptr;
    //
    // Rect aZombieRect = theZombie->GetZombieAttackRect();
    // if (theZombie->mZombiePhase == PHASE_POLEVAULTER_PRE_VAULT)
    // {
    // aZombieRect = Rect(50 + theZombie->mX, 0, 20, 115);
    // }
    // if (theZombie->mZombieType == ZOMBIE_BALLOON)
    // {
    // aZombieRect.mX += 25;
    // }
    //
    // if (aZombieRect.mX > 20)
    // return nullptr;
    //
    // GridItem* aBrain = mBoard->GetGridItemAt(GRIDITEM_IZOMBIE_BRAIN, 0, theZombie->mRow);
    // return (aBrain && aBrain->mGridItemState != GRIDITEM_STATE_BRAIN_SQUISHED) ? aBrain : nullptr;
}

void Challenge::IZombieSquishBrain(GridItem *theBrain) {
    old_Challenge_IZombieSquishBrain(this, theBrain);
}

int Challenge::ScaryPotterCountSunInPot(GridItem *theScaryPot) {
    return theScaryPot->mSunCount;
}

SeedType Challenge::GetArtChallengeSeed(int theGridX, int theGridY) const {
    if (theGridY < 6) {

        GameMode aGameMode = mApp->mGameMode;
        if (aGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT)
            return gArtChallengeWallnut[theGridY][theGridX];
        if (aGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER)
            return gArtChallengeSunFlower[theGridY][theGridX];
        if (aGameMode == GAMEMODE_CHALLENGE_SEEING_STARS)
            return gArtChallengeStarFruit[theGridY][theGridX];
    }
    return SEED_NONE;
}

void Challenge::InitZombieWavesFromList(const ZombieType *theZombieList, int theListLength) const {
    for (int i = 0; i < theListLength; i++) {
        mBoard->mZombieAllowed[(int)theZombieList[i]] = true;
    }
}

void Challenge::IZombieSetupPlant(Plant *thePlant) const {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(thePlant->mBodyReanimID);
    Reanimation *aHeadReanim = mApp->ReanimationTryToGet(thePlant->mHeadReanimID);
    Reanimation *aHeadReanim2 = mApp->ReanimationTryToGet(thePlant->mHeadReanimID2);
    Reanimation *aHeadReanim3 = mApp->ReanimationTryToGet(thePlant->mHeadReanimID3);
    if (aBodyReanim)
        aBodyReanim->mAnimRate = 0;
    if (aHeadReanim)
        aHeadReanim->mAnimRate = 0;
    if (aHeadReanim2)
        aHeadReanim2->mAnimRate = 0;
    if (aHeadReanim3)
        aHeadReanim3->mAnimRate = 0;

    if (thePlant->mSeedType == SEED_POTATOMINE) {
        thePlant->PlayBodyReanim("anim_armed", REANIM_LOOP, 0, 0);
        thePlant->mState = STATE_POTATO_ARMED;
    }

    thePlant->mBlinkCountdown = 0;
    thePlant->UpdateReanim();
}

void Challenge::MouseDownWhackAZombie(int theX, int theY, int thePlayerIndex) const {
    CursorObject *aCursorObject = (thePlayerIndex == 4) ? mBoard->mCursorObject[1] : mBoard->mCursorObject[0];
    mApp->ReanimationTryToGet(aCursorObject->mReanimCursorID)->mAnimTime = 0.2f;
    mApp->PlayFoley(FoleyType::FOLEY_SWING);

    Zombie *aZombie = nullptr;
    Zombie *aTopZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (!aZombie->IsDeadOrDying()) {
            Rect aZombieRect = aZombie->GetZombieRect();
            if (GetCircleRectOverlap(theX, theY - 20, 45, aZombieRect)) {
                if (aTopZombie == nullptr || aZombie->mRenderOrder >= aTopZombie->mRenderOrder) {
                    aTopZombie = aZombie;
                }
            }
        }
    }

    if (aTopZombie) {
        if (aTopZombie->mHelmType != HelmType::HELMTYPE_NONE) {
            if (aTopZombie->mHelmType == HelmType::HELMTYPE_PAIL) {
                mApp->PlayFoley(FoleyType::FOLEY_SHIELD_HIT);
            } else if (aTopZombie->mHelmType == HelmType::HELMTYPE_TRAFFIC_CONE) {
                mApp->PlayFoley(FoleyType::FOLEY_PLASTIC_HIT);
            }

            aTopZombie->TakeHelmDamage(900, 0U);
        } else {
            mApp->PlayFoley(FoleyType::FOLEY_BONK);
            mApp->AddTodParticle(theX - 3, theY + 9, RenderLayer::RENDER_LAYER_ABOVE_UI, ParticleEffect::PARTICLE_POW);
            aTopZombie->DieWithLoot();
            mBoard->ClearCursor(thePlayerIndex);
        }

        TriggerVibration(VibrationEffect::VIBRATION_WHACK_HIT);
    } else {
        TriggerVibration(VibrationEffect::VIBRATION_WHACK_MISS);
    }
}

bool Challenge::IsMPResourceProducer(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_SUNFLOWER || theSeedType == SeedType::SEED_SUNSHROOM || theSeedType == SeedType::SEED_TWINSUNFLOWER || theSeedType == SeedType::SEED_ZOMBIE_GRAVESTONE
        || theSeedType == SeedType::SEED_SUN_BEAN;
}

bool Challenge::IsMPZombieTypeAddInRow(ZombieType theZombieType) {
    return theZombieType == ZombieType::ZOMBIE_DANCER || theZombieType == ZombieType::ZOMBIE_ZAMBONI || theZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX
        || theZombieType == ZombieType::ZOMBIE_DIGGER || theZombieType == ZombieType::ZOMBIE_CATAPULT || theZombieType == ZombieType::ZOMBIE_GARGANTUAR
        || theZombieType == ZombieType::ZOMBIE_DUCKY_TUBE || theZombieType == ZombieType::ZOMBIE_SNORKEL || theZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER
        || theZombieType == ZombieType::ZOMBIE_BALLOON || theZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR || theZombieType == ZombieType::ZOMBIE_BOBSLED
        || theZombieType == ZombieType::ZOMBIE_JACKSON || theZombieType == ZombieType::ZOMBIE_EXPLORER || theZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR
        || theZombieType == ZombieType::ZOMBIE_DOGWALKER || theZombieType == ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR;
}

bool Challenge::IsMPZombieTypeCanGoInPool(ZombieType theZombieType) {
    if (theZombieType == ZombieType::ZOMBIE_SNORKEL || theZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER || theZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX
        || theZombieType == ZombieType::ZOMBIE_BALLOON)
        return true;

    if (IsMPZombieTypeAddInRow(theZombieType))
        return false;

    return theZombieType == ZombieType::ZOMBIE_NORMAL || theZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE || theZombieType == ZombieType::ZOMBIE_PAIL || theZombieType == ZombieType::ZOMBIE_FLAG
        || theZombieType == ZombieType::ZOMBIE_FOOTBALL || theZombieType == ZombieType::ZOMBIE_BUNGEE || theZombieType == ZombieType::ZOMBIE_IMP || theZombieType == ZombieType::ZOMBIE_PEA_HEAD
        || theZombieType == ZombieType::ZOMBIE_GATLING_HEAD || theZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD;
}

void Challenge::DrawWeather(Sexy::Graphics *g) {
    if (mApp->IsStormyNightLevel() || mApp->mGameMode == GAMEMODE_CHALLENGE_RAINING_SEEDS)
        DrawRain(g);

    if (mApp->IsStormyNightLevel())
        DrawStormNight(g);

    if (gOpeningEncounter) {
        if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUN_RAIN && gOpeningEncounter->mDoEffect)
            DrawRain(g);
    }
}

void Challenge::UpdateMPGraveStones() {
    // 空函数替换，原逻辑移动至 GridItem::UpdateBurialMound
}

bool Challenge::ISMPSeedSuddenDeathDisabled(int thePlayerIndex, SeedType theSeedType) {
    SeedType *aDisableList = (thePlayerIndex == 1) ? mSuddenDeathDisableSeeds2 : mSuddenDeathDisableSeeds1;
    return aDisableList[0] == theSeedType || aDisableList[1] == theSeedType || aDisableList[2] == theSeedType;
}

void Challenge::DrawBackdrop(Sexy::Graphics *g) {
    old_Challenge_DrawBackdrop(this, g);

    if (mApp->IsVSMode()) {
        DrawVSSuddenDeathClock(g);
    }
}

void Challenge::DrawVSSuddenDeathClock(Graphics *g) {
    if (mApp->mGameScene != SCENE_PLAYING) {
        return;
    }

    Graphics gBoardParent = Graphics(*g);
    gBoardParent.mTransX -= mBoard->mX;
    gBoardParent.mTransY -= mBoard->mY;
    mApp->ReanimationGet(mReanimChallenge)->Draw(&gBoardParent);

    if (mBoard->mMainCounter >= 0) {
        int aRemainSeconds = std::max(0, MP_SUDDEN_DEATH_SECONDS - mBoard->mMainCounter / MP_SUDDEN_DEATH_TICKS_PER_SECOND);
        int aMinutes = aRemainSeconds / 60;
        int aSeconds = aRemainSeconds % 60;
        Color aSuddenDeathColor = aRemainSeconds <= 10 ? Color(255, 0, 0) : Color::White;
        TodDrawString(&gBoardParent, StrFormat("%d:%02d", aMinutes, aSeconds), 400, 620, Sexy::FONT_DWARVENTODCRAFT18, aSuddenDeathColor, DS_ALIGN_CENTER);
    }
}
