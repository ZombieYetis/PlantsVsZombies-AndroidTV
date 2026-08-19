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

#include "PvZ/Lawn/Board/Plant.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/OpeningEncounter.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/ZenGarden.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/ReanimationLawn.h"
#include "PvZ/Lawn/VSActionSystem.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Misc.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Misc/SexyVector.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Reanimator.h"
#include "PvZ/TodLib/Effect/TodParticle.h"

#include <cmath>

#include <algorithm>

using namespace Sexy;

PlantDefinition gPlantDefs[SeedType::NUM_SEED_TYPES] = {
    {SeedType::SEED_PEASHOOTER, nullptr, ReanimationType::REANIM_PEASHOOTER, 0, 100, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "PEASHOOTER"},
    {SeedType::SEED_SUNFLOWER, nullptr, ReanimationType::REANIM_SUNFLOWER, 1, 50, 750, PlantSubClass::SUBCLASS_NORMAL, 2500, "SUNFLOWER"},
    {SeedType::SEED_CHERRYBOMB, nullptr, ReanimationType::REANIM_CHERRYBOMB, 3, 150, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "CHERRY_BOMB"},
    {SeedType::SEED_WALLNUT, nullptr, ReanimationType::REANIM_WALLNUT, 2, 50, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "WALL_NUT"},
    {SeedType::SEED_POTATOMINE, nullptr, ReanimationType::REANIM_POTATOMINE, 37, 25, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "POTATO_MINE"},
    {SeedType::SEED_SNOWPEA, nullptr, ReanimationType::REANIM_SNOWPEA, 4, 175, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "SNOW_PEA"},
    {SeedType::SEED_CHOMPER, nullptr, ReanimationType::REANIM_CHOMPER, 31, 150, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "CHOMPER"},
    {SeedType::SEED_REPEATER, nullptr, ReanimationType::REANIM_REPEATER, 5, 200, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "REPEATER"},
    {SeedType::SEED_PUFFSHROOM, nullptr, ReanimationType::REANIM_PUFFSHROOM, 6, 0, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "PUFF_SHROOM"},
    {SeedType::SEED_SUNSHROOM, nullptr, ReanimationType::REANIM_SUNSHROOM, 7, 25, 750, PlantSubClass::SUBCLASS_NORMAL, 2500, "SUN_SHROOM"},
    {SeedType::SEED_FUMESHROOM, nullptr, ReanimationType::REANIM_FUMESHROOM, 9, 75, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "FUME_SHROOM"},
    {SeedType::SEED_GRAVEBUSTER, nullptr, ReanimationType::REANIM_GRAVE_BUSTER, 40, 75, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "GRAVE_BUSTER"},
    {SeedType::SEED_HYPNOSHROOM, nullptr, ReanimationType::REANIM_HYPNOSHROOM, 10, 75, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "HYPNO_SHROOM"},
    {SeedType::SEED_SCAREDYSHROOM, nullptr, ReanimationType::REANIM_SCRAREYSHROOM, 33, 25, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "SCAREDY_SHROOM"},
    {SeedType::SEED_ICESHROOM, nullptr, ReanimationType::REANIM_ICESHROOM, 36, 75, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "ICE_SHROOM"},
    {SeedType::SEED_DOOMSHROOM, nullptr, ReanimationType::REANIM_DOOMSHROOM, 20, 125, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "DOOM_SHROOM"},
    {SeedType::SEED_LILYPAD, nullptr, ReanimationType::REANIM_LILYPAD, 19, 25, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "LILY_PAD"},
    {SeedType::SEED_SQUASH, nullptr, ReanimationType::REANIM_SQUASH, 21, 50, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "SQUASH"},
    {SeedType::SEED_THREEPEATER, nullptr, ReanimationType::REANIM_THREEPEATER, 12, 325, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "THREEPEATER"},
    {SeedType::SEED_TANGLEKELP, nullptr, ReanimationType::REANIM_TANGLEKELP, 17, 25, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "TANGLE_KELP"},
    {SeedType::SEED_JALAPENO, nullptr, ReanimationType::REANIM_JALAPENO, 11, 125, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "JALAPENO"},
    {SeedType::SEED_SPIKEWEED, nullptr, ReanimationType::REANIM_SPIKEWEED, 22, 100, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "SPIKEWEED"},
    {SeedType::SEED_TORCHWOOD, nullptr, ReanimationType::REANIM_TORCHWOOD, 29, 175, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "TORCHWOOD"},
    {SeedType::SEED_TALLNUT, nullptr, ReanimationType::REANIM_TALLNUT, 28, 125, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "TALL_NUT"},
    {SeedType::SEED_SEASHROOM, nullptr, ReanimationType::REANIM_SEASHROOM, 39, 0, 3000, PlantSubClass::SUBCLASS_SHOOTER, 150, "SEA_SHROOM"},
    {SeedType::SEED_PLANTERN, nullptr, ReanimationType::REANIM_PLANTERN, 38, 25, 3000, PlantSubClass::SUBCLASS_NORMAL, 2500, "PLANTERN"},
    {SeedType::SEED_CACTUS, nullptr, ReanimationType::REANIM_CACTUS, 15, 125, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "CACTUS"},
    {SeedType::SEED_BLOVER, nullptr, ReanimationType::REANIM_BLOVER, 18, 100, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "BLOVER"},
    {SeedType::SEED_SPLITPEA, nullptr, ReanimationType::REANIM_SPLITPEA, 32, 125, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "SPLIT_PEA"},
    {SeedType::SEED_STARFRUIT, nullptr, ReanimationType::REANIM_STARFRUIT, 30, 125, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "STARFRUIT"},
    {SeedType::SEED_PUMPKINSHELL, nullptr, ReanimationType::REANIM_PUMPKIN, 25, 125, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "PUMPKIN"},
    {SeedType::SEED_MAGNETSHROOM, nullptr, ReanimationType::REANIM_MAGNETSHROOM, 35, 100, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "MAGNET_SHROOM"},
    {SeedType::SEED_CABBAGEPULT, nullptr, ReanimationType::REANIM_CABBAGEPULT, 13, 100, 750, PlantSubClass::SUBCLASS_SHOOTER, 300, "CABBAGE_PULT"},
    {SeedType::SEED_FLOWERPOT, nullptr, ReanimationType::REANIM_FLOWER_POT, 33, 25, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "FLOWER_POT"},
    {SeedType::SEED_KERNELPULT, nullptr, ReanimationType::REANIM_KERNELPULT, 13, 100, 750, PlantSubClass::SUBCLASS_SHOOTER, 300, "KERNEL_PULT"},
    {SeedType::SEED_INSTANT_COFFEE, nullptr, ReanimationType::REANIM_COFFEEBEAN, 33, 75, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "COFFEE_BEAN"},
    {SeedType::SEED_GARLIC, nullptr, ReanimationType::REANIM_GARLIC, 8, 50, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "GARLIC"},
    {SeedType::SEED_UMBRELLA, nullptr, ReanimationType::REANIM_UMBRELLALEAF, 23, 100, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "UMBRELLA_LEAF"},
    {SeedType::SEED_MARIGOLD, nullptr, ReanimationType::REANIM_MARIGOLD, 24, 50, 3000, PlantSubClass::SUBCLASS_NORMAL, 2500, "MARIGOLD"},
    {SeedType::SEED_MELONPULT, nullptr, ReanimationType::REANIM_MELONPULT, 14, 300, 750, PlantSubClass::SUBCLASS_SHOOTER, 300, "MELON_PULT"},
    {SeedType::SEED_GATLINGPEA, nullptr, ReanimationType::REANIM_GATLINGPEA, 5, 250, 5000, PlantSubClass::SUBCLASS_SHOOTER, 150, "GATLING_PEA"},
    {SeedType::SEED_TWINSUNFLOWER, nullptr, ReanimationType::REANIM_TWIN_SUNFLOWER, 1, 150, 5000, PlantSubClass::SUBCLASS_NORMAL, 2500, "TWIN_SUNFLOWER"},
    {SeedType::SEED_GLOOMSHROOM, nullptr, ReanimationType::REANIM_GLOOMSHROOM, 27, 150, 5000, PlantSubClass::SUBCLASS_SHOOTER, 200, "GLOOM_SHROOM"},
    {SeedType::SEED_CATTAIL, nullptr, ReanimationType::REANIM_CATTAIL, 27, 225, 5000, PlantSubClass::SUBCLASS_SHOOTER, 150, "CATTAIL"},
    {SeedType::SEED_WINTERMELON, nullptr, ReanimationType::REANIM_WINTER_MELON, 27, 200, 5000, PlantSubClass::SUBCLASS_SHOOTER, 300, "WINTER_MELON"},
    {SeedType::SEED_GOLD_MAGNET, nullptr, ReanimationType::REANIM_GOLD_MAGNET, 27, 50, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "GOLD_MAGNET"},
    {SeedType::SEED_SPIKEROCK, nullptr, ReanimationType::REANIM_SPIKEROCK, 27, 125, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "SPIKEROCK"},
    {SeedType::SEED_COBCANNON, nullptr, ReanimationType::REANIM_COBCANNON, 16, 500, 5000, PlantSubClass::SUBCLASS_NORMAL, 600, "COB_CANNON"},
    {SeedType::SEED_IMITATER, nullptr, ReanimationType::REANIM_IMITATER, 33, 0, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "IMITATER"},
    {SeedType::NUM_SEEDS_IN_CHOOSER, nullptr, ReanimationType::REANIM_NONE, 0, 0, 0, PlantSubClass::SUBCLASS_NORMAL, 0, "NUM_SEEDS_IN_CHOOSER"},
    {SeedType::SEED_EXPLODE_O_NUT, nullptr, ReanimationType::REANIM_WALLNUT, 2, 0, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "EXPLODE_O_NUT"},
    {SeedType::SEED_GIANT_WALLNUT, nullptr, ReanimationType::REANIM_WALLNUT, 2, 0, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "GIANT_WALLNUT"},
    {SeedType::SEED_SPROUT, nullptr, ReanimationType::REANIM_ZENGARDEN_SPROUT, 33, 0, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "SPROUT"},
    {SeedType::SEED_LEFTPEATER, nullptr, ReanimationType::REANIM_REPEATER, 5, 200, 750, PlantSubClass::SUBCLASS_SHOOTER, 150, "REPEATER"}};

PlantDefinition gExtendedPlantDefs[]{
    {SeedType::SEED_ICEBERG_LETTUCE, nullptr, ReanimationType::REANIM_ICEBERG_LETTUCE, 0, 0, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "ICEBERG_LETTUCE"},
    {SeedType::SEED_BLOOMERANG, nullptr, ReanimationType::REANIM_BLOOMERANG, 0, 175, 750, PlantSubClass::SUBCLASS_SHOOTER, 300, "BLOOMERANG"},
    {SeedType::SEED_BONK_CHOY, nullptr, ReanimationType::REANIM_BONK_CHOY, 0, 150, 750, PlantSubClass::SUBCLASS_NORMAL, 0, "BONK_CHOY"},
    {SeedType::SEED_CELERY_STALKER, nullptr, ReanimationType::REANIM_CELERY_STALKER, 0, 50, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "CELERY_STALKER"},
    {SeedType::SEED_SPORESHROOM, nullptr, ReanimationType::REANIM_SPORE_SHROOM, 0, 125, 750, PlantSubClass::SUBCLASS_SHOOTER, 300, "SPORE_SHROOM"},
    {SeedType::SEED_SWEET_POTATO, nullptr, ReanimationType::REANIM_SWEET_POTATO, 0, 150, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "SWEET_POTATO"},
    {SeedType::SEED_CHILLY_PEPPER, nullptr, ReanimationType::REANIM_CHILLY_PEPPER, 0, 100, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "CHILLY_PEPPER"},
    {SeedType::SEED_IMP_PEAR, nullptr, ReanimationType::REANIM_IMP_PEAR, 0, 100, 3000, PlantSubClass::SUBCLASS_NORMAL, 0, "IMP_PEAR"},
};

void Plant::PlantInitialize(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int a6) {
    // 在初始化植物后更新一次动画，以解决开场前存在的植物只绘制阴影而不绘制植物本体的问题
    old_Plant_PlantInitialize(this, theGridX, theGridY, theSeedType, theImitaterType, a6);
    UpdateReanim();

    switch (theSeedType) {
        case SeedType::SEED_ICEBERG_LETTUCE:
            break;
        case SeedType::SEED_CELERY_STALKER:
            mPlantMaxHealth = 1200;
            break;
        case SeedType::SEED_BONK_CHOY:
            mTargetX = 1; // mTargetX 被菜问用作朝向标记；刚种下时默认朝右。
            break;
        case SeedType::SEED_SWEET_POTATO:
            mPlantMaxHealth = 4000;
            break;
        case SeedType::SEED_CHILLY_PEPPER: {
            Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
            if (IsInPlay() && aBodyReanim != nullptr) {
                mDoSpecialCountdown = 100;
                aBodyReanim->SetFramesForLayer("anim_explode");
                aBodyReanim->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD;
                aBodyReanim->SetAnimRate(18.0f);
            }
            break;
        }
        default:
            break;
    }

    if (mApp->IsVSMode()) {
        if ((theSeedType == SeedType::SEED_SUNFLOWER || theSeedType == SeedType::SEED_SUNSHROOM) && vsai::HasEnhancedAIProduction(mBoard, vsai::VSSide::Plants)) {
            mLaunchCounter = vsai::ScaleEnhancedAIProductionCooldown(mLaunchCounter);
        }

        //        if (mLaunchRate > 0) {
        //            if (MakesSun())
        //                mLaunchCounter = RandRangeInt(300, mLaunchRate / 2);
        //            else
        //                mLaunchCounter = RandRangeInt(0, mLaunchRate);
        //        } else
        //            mLaunchCounter = 0;

        if (gTcpClientSocket >= 0) {
            U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_LAUNCHCOUNTER}, uint16_t(mBoard->mPlants.DataArrayGetID(this)), uint16_t(mLaunchCounter)};
            netplay::PutEvent(event);
        }
    }

    // 在对战模式修改指定植物的血量
    //    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
    //        switch (theSeedType) {
    //            case SeedType::SEED_SUNFLOWER:
    //            case SeedType::SEED_PEASHOOTER:
    //                mPlantMaxHealth = 300;
    //                break;
    //            default:
    //                break;
    //        }
    //    }

    mPlantHealth = mPlantMaxHealth;
}

void Plant::SetSleeping(bool theIsAsleep) {
    if (mushroomsNoSleep && !IsOnlineServerModeActive() && !gIsReplayMode) {
        // 如果开启"蘑菇免唤醒"
        theIsAsleep = false;
    }

    old_Plant_SetSleeping(this, theIsAsleep);
}

int Plant::GetDamageRangeFlags(PlantWeapon thePlantWeapon) const {
    switch (mSeedType) {
        case SeedType::SEED_CACTUS:
            return thePlantWeapon == PlantWeapon::WEAPON_SECONDARY ? 1 : 2;
        case SeedType::SEED_CHERRYBOMB:
        case SeedType::SEED_JALAPENO:
        case SeedType::SEED_COBCANNON:
        case SeedType::SEED_DOOMSHROOM:
        case SeedType::SEED_CHILLY_PEPPER:
            return 127;
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_WINTERMELON:
        case SeedType::SEED_SPORESHROOM:
            return 13;
        case SeedType::SEED_POTATOMINE:
            return 77;
        case SeedType::SEED_SQUASH:
            return 13;
        case SeedType::SEED_PUFFSHROOM:
        case SeedType::SEED_SEASHROOM:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_GLOOMSHROOM:
        case SeedType::SEED_CHOMPER:
        case SeedType::SEED_ICEBERG_LETTUCE:
        case SeedType::SEED_BONK_CHOY:
        case SeedType::SEED_CELERY_STALKER:
            return 9; // DAMANGES_GROUND | DAMAGES_DOG
        case SeedType::SEED_CATTAIL:
            return 11;
        case SeedType::SEED_TANGLEKELP:
            return 5;
        case SeedType::SEED_GIANT_WALLNUT:
            return 17;
        default:
            return 1;
    }
}

void Plant::PlayBodyReanim(const char *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate) {
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);

    if (theBlendTime > 0) {
        aBodyReanim->StartBlend(theBlendTime);
    }
    if (theAnimRate > 0.0f) {
        aBodyReanim->SetAnimRate(theAnimRate);
    }

    aBodyReanim->mLoopType = theLoopType;
    aBodyReanim->mLoopCount = 0;
    aBodyReanim->SetFramesForLayer(theTrackName);
}

void Plant::SpikeweedAttack() {
    if (mState != PlantState::STATE_SPIKEWEED_ATTACKING) {
        PlayBodyReanim("anim_attack", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 18.0f);
        mApp->PlaySample(Sexy::SOUND_THROW);

        mState = PlantState::STATE_SPIKEWEED_ATTACKING;
        mStateCountdown = 100;
    }
}

void Plant::SpikeRockTakeDamage() {
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);

    SpikeweedAttack();

    mPlantHealth -= 50;
    if (mPlantHealth <= 300) {
        aBodyReanim->AssignRenderGroupToTrack("bigspike3", RENDER_GROUP_HIDDEN);
    }
    if (mPlantHealth <= 150) {
        aBodyReanim->AssignRenderGroupToTrack("bigspike2", RENDER_GROUP_HIDDEN);
    }
    if (mPlantHealth <= 0) {
        mApp->PlayFoley(FoleyType::FOLEY_SQUISH);
        Die();
    }
}

bool Plant::IsSpiky() const {
    return mSeedType == SeedType::SEED_SPIKEWEED || mSeedType == SeedType::SEED_SPIKEROCK;
}

bool Plant::IsLowProfile() const {
    switch (mSeedType) {
        case SeedType::SEED_PUFFSHROOM:
        case SeedType::SEED_SUNSHROOM:
        case SeedType::SEED_POTATOMINE:
        case SeedType::SEED_SPIKEWEED:
        case SeedType::SEED_SPIKEROCK:
        case SeedType::SEED_LILYPAD:
        case SeedType::SEED_ICEBERG_LETTUCE:
            return true;

        case SeedType::SEED_CELERY_STALKER:
            return mState == PlantState::STATE_CELERY_STALKER_LOW || mState == PlantState::STATE_CELERY_STALKER_LOWERING;

        default:
            return false;
    }
}

void Plant::UpdateReanimColor() {
    // 修复玩家选中但不拿起(gameState为1就是选中但不拿起，为7就是选中且拿起)某个紫卡植物时，相应的可升级绿卡植物也会闪烁的BUG。
    if (mBoard == nullptr) {
        old_Plant_UpdateReanimColor(this);
        return;
    }
    SeedType aSeedType = mSeedType;
    if (!Plant::IsUpgrade(mSeedType)) {
        old_Plant_UpdateReanimColor(this);
        return;
    }
    if (mSeedType == SeedType::SEED_EXPLODE_O_NUT) {
        old_Plant_UpdateReanimColor(this);
        return;
    }
    GamepadControls *gamePad = mBoard->mGamepadControls[0];
    if (gamePad->mGamepadState != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
        mSeedType = SeedType::SEED_PEASHOOTER;
        old_Plant_UpdateReanimColor(this);
        mSeedType = aSeedType;
        return;
    }

    old_Plant_UpdateReanimColor(this);
}

bool Plant::IsOnBoard() const {
    if (!mIsOnBoard)
        return false;

    return true;
}

bool Plant::IsOnHighGround() {
    return mBoard && mBoard->mGridSquareType[mPlantCol][mRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND;
}

bool Plant::IsInPlay() {
    return IsOnBoard() && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && mApp->mGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM;
}

void Plant::Animate() {
    if ((mSeedType == SeedType::SEED_CHERRYBOMB || mSeedType == SeedType::SEED_JALAPENO) && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
        mShakeOffsetX = RandRangeFloat(-1.0f, 1.0f);
        mShakeOffsetY = RandRangeFloat(-1.0f, 1.0f);
    } else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && mPottedPlantIndex != -1) {
        UpdateNeedsFood();
    }

    if (mRecentlyEatenCountdown > 0) {
        mRecentlyEatenCountdown--;
    }
    if (mEatenFlashCountdown > 0) {
        mEatenFlashCountdown--;
    }
    if (mBeghouledFlashCountdown > 0) {
        mBeghouledFlashCountdown--;
    }

    if (mSquished) {
        mFrame = 0;
        return;
    }

    if (mSeedType == SeedType::SEED_WALLNUT || mSeedType == SeedType::SEED_TALLNUT) {
        AnimateNuts();
    } else if (mSeedType == SeedType::SEED_GARLIC) {
        AnimateGarlic();
    } else if (mSeedType == SeedType::SEED_PUMPKINSHELL) {
        AnimatePumpkin();
    } else if (mSeedType == SeedType::SEED_CELERY_STALKER) {
        AnimateCeleryStalker();
    } else if (mSeedType == SeedType::SEED_SWEET_POTATO) {
        AnimateSweetPotato();
    }

    UpdateBlink();

    if (mAnimPing) {
        if (mAnimCounter < mFrameLength * mNumFrames - 1) {
            mAnimCounter++;
        } else {
            mAnimPing = false;
            mAnimCounter -= mFrameLength;
        }
    } else if (mAnimCounter > 0) {
        mAnimCounter--;
    } else {
        mAnimPing = true;
        mAnimCounter += mFrameLength;
    }

    mFrame = mAnimCounter / mFrameLength;
}

void Plant::AnimateCeleryStalker() {
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
    Image *aImageOverride = aBodyReanim->GetImageOverride("CeleryStalker_arm2_upper");
    if (mPlantHealth < mPlantMaxHealth / 3) {
        if (aImageOverride != addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER3) {
            aBodyReanim->SetImageOverride("CeleryStalker_arm2_upper", addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER3);
            aBodyReanim->SetImageOverride("CeleryStalker_arm2_lower", addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_LOWER3);
        }
    } else if (mPlantHealth < mPlantMaxHealth * 2 / 3) {
        if (aImageOverride != addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER2) {
            aBodyReanim->SetImageOverride("CeleryStalker_arm2_upper", addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER2);
            aBodyReanim->SetImageOverride("CeleryStalker_arm2_lower", addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_LOWER2);
        }
    } else {
        aBodyReanim->SetImageOverride("CeleryStalker_arm2_upper", nullptr);
        aBodyReanim->SetImageOverride("CeleryStalker_arm2_lower", nullptr);
    }
}

void Plant::AnimateSweetPotato() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    Image *aImageOverride = aBodyReanim->GetImageOverride("SweetPotato_body");
    if (mPlantHealth < mPlantMaxHealth / 3) {
        if (aImageOverride != addonImages.IMAGE_REANIM_SWEET_POTATO_BODY3) {
            aBodyReanim->SetImageOverride("SweetPotato_body", addonImages.IMAGE_REANIM_SWEET_POTATO_BODY3);
            aBodyReanim->SetImageOverride("SweetPotato_mouth", addonImages.IMAGE_REANIM_SWEET_POTATO_MOUTH3);
            aBodyReanim->SetImageOverride("SweetPotato_stem", IMAGE_BLANK);
        }
    } else if (mPlantHealth < mPlantMaxHealth * 2 / 3) {
        if (aImageOverride != addonImages.IMAGE_REANIM_SWEET_POTATO_BODY2) {
            aBodyReanim->SetImageOverride("SweetPotato_body", addonImages.IMAGE_REANIM_SWEET_POTATO_BODY2);
            aBodyReanim->SetImageOverride("SweetPotato_mouth", addonImages.IMAGE_REANIM_SWEET_POTATO_MOUTH2);
            aBodyReanim->SetImageOverride("SweetPotato_eye", addonImages.IMAGE_REANIM_SWEET_POTATO_EYE3);
        }
    } else {
        aBodyReanim->SetImageOverride("SweetPotato_body", nullptr);
        aBodyReanim->SetImageOverride("SweetPotato_mouth", nullptr);
    }
}

void Plant::Update() {
    // 用于修复植物受击闪光、生产发光、铲子下方植物发光，同时实现技能无冷却

    if (abilityFastCoolDown && !IsOnlineServerModeActive() && !gIsReplayMode && mSeedType != SeedType::SEED_SPIKEWEED
        && mSeedType != SeedType::SEED_SPIKEROCK) { // 修复地刺和地刺王开启技能无冷却后不攻击敌人
        if (mStateCountdown > 10) {
            mStateCountdown = 10;
        }
    }

    int mHighLightCounter = mEatenFlashCountdown;
    int cancelHighLightLimit = 999 - ((speedUpMode > 0 && !IsOnlineServerModeActive() && !gIsReplayMode) ? 10 : 0); // 铲子的发光计数是1000。这段代码用于在铲子移走之后的1ms内取消植物发光
    if (mHighLightCounter >= 900 && mHighLightCounter <= cancelHighLightLimit) {
        mBeghouledFlashCountdown = 0;
        mEatenFlashCountdown = 0;
    } else if (mHighLightCounter > 0) {
        mBeghouledFlashCountdown = mHighLightCounter > 25 ? 25 : mHighLightCounter;
    }

    GameScenes mGameScene = mApp->mGameScene;

    if ((!IsOnBoard() || mGameScene != GameScenes::SCENE_LEVEL_INTRO || !mApp->IsWallnutBowlingLevel()) && (!IsOnBoard() || mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN)
        && (!IsOnBoard() || !mBoard->mCutScene->ShouldRunUpsellBoard()) && IsOnBoard() && mGameScene != GameScenes::SCENE_PLAYING) {
        return;
    }

    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 如果开了高级暂停
        UpdateReanimColor();
        if (mHighLightCounter == 1000) {
            mBeghouledFlashCountdown = 0;
            mEatenFlashCountdown = 0;
        }
        return;
    }
    // 为了不影响改so，这里不是完全重写，而是执行旧函数
    // Plant_UpdateAbilities(plant);
    // Plant_Animate(plant);
    // if (plant->mPlantHealth < 0)
    // Plant_Die(plant);
    // Plant_UpdateReanim(plant);

    old_Plant_Update(this);
}

void Plant::UpdateAbilities() {
    if (!IsInPlay())
        return;

    if (mSeedType == SeedType::SEED_SPORESHROOM && mIsAsleep) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim && aBodyReanim->IsAnimPlaying("anim_grow") && aBodyReanim->mLoopCount > 0) {
            PlayBodyReanim("anim_sleep", ReanimLoopType::REANIM_LOOP, 20, aBodyReanim->mDefinition->mFPS);
            return;
        }
    }

    old_Plant_UpdateAbilities(this);

    if (mIsAsleep || mSquished || mOnBungeeState != PlantOnBungeeState::NOT_ON_BUNGEE) {
        return;
    }

    if (mSeedType == SeedType::SEED_ICEBERG_LETTUCE) {
        UpdateIcebergLettuce();
    } else if (mSeedType == SeedType::SEED_CELERY_STALKER) {
        UpdateCeleryStalker();
    } else if (mSeedType == SeedType::SEED_BLOOMERANG) {
        UpdateBloomerang();
    } else if (mSeedType == SeedType::SEED_BONK_CHOY) {
        UpdateBonkChoy();
    } else if (mSeedType == SeedType::SEED_SWEET_POTATO) {
        UpdateSweetPotato();
    }
}

bool Plant::HasActiveBoomerang() {
    if (mBoard == nullptr) {
        return false;
    }

    const PlantID aPlantID = mBoard->PlantGetID(this);
    Projectile *aProjectile = nullptr;
    while (mBoard->IterateProjectiles(aProjectile)) {
        if (!aProjectile->mDead && aProjectile->mProjectileType == ProjectileType::PROJECTILE_BOOMERANG && aProjectile->mRelatedPlantID == aPlantID) {
            return true;
        }
    }
    return false;
}

void Plant::UpdateBloomerang() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);

    if (mState == PlantState::STATE_UMBRELLA_TRIGGERED) {
        if (mStateCountdown == 0) {
            mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PROJECTILE, mRow + 1, 0);
            mState = PlantState::STATE_UMBRELLA_REFLECTING;
        }
    } else if (mState == PlantState::STATE_UMBRELLA_REFLECTING) {
        if (aBodyReanim && aBodyReanim->mLoopCount > 0) {
            PlayIdleAnim(0.0f);
            mState = PlantState::STATE_NOTREADY;
            mRenderOrder = CalcRenderOrder();
        }
    } else if (mState == PlantState::STATE_BLOOMERANG_CATCHING) {
        if (aBodyReanim && aBodyReanim->mLoopCount > 0) {
            PlayIdleAnim(0.0f);
            mState = PlantState::STATE_NOTREADY;
        }
    }
}

void Plant::UpdateSweetPotato() {
    if (mApp->IsVSMode() && (gTcpConnected || gIsReplayMode)) {
        return;
    }

    const int aFrontCol = mPlantCol + 1;
    if (aFrontCol >= MAX_GRID_SIZE_X) {
        return;
    }

    // 为每个僵尸确定唯一的甜薯归属，防止相邻行的多个甜薯反复抢夺：
    auto FindSweetPotatoOwner = [this](Zombie *theZombie) -> Plant * {
        Plant *anOwner = nullptr;
        Plant *aSweetPotato = nullptr;

        while (mBoard->IteratePlants(aSweetPotato)) {
            if (aSweetPotato->mSeedType != SeedType::SEED_SWEET_POTATO || aSweetPotato->mPlantCol != mPlantCol) {
                continue;
            }

            // 如果僵尸当前逻辑行已有同列甜薯，则由同行甜薯锁定。
            if (aSweetPotato->mRow == theZombie->mRow) {
                return aSweetPotato;
            }

            if (std::abs(aSweetPotato->mRow - theZombie->mRow) != 1) {
                continue;
            }

            // 如果同行没有甜薯，而上下相邻行同时存在候选，则固定由上方甜薯获得。
            if (anOwner == nullptr || aSweetPotato->mRow < anOwner->mRow) {
                anOwner = aSweetPotato;
            }
        }

        return anOwner;
    };

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (aZombie->mDead || !aZombie->IsOnBoard()) {
            continue;
        }

        if (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE || aZombie->mZombieType == ZombieType::ZOMBIE_CATAPULT || aZombie->mZombieType == ZombieType::ZOMBIE_BOSS
            || aZombie->mZombieType == ZombieType::ZOMBIE_DOGWALKER || aZombie->mZombieType == ZombieType::ZOMBIE_DOG || !aZombie->CanBeFrozen() || aZombie->ZombieNotWalking()) {
            continue;
        }

        if (std::abs(aZombie->mRow - mRow) != 1) {
            continue;
        }

        const Rect aZombieRect = aZombie->GetZombieRect();
        const int aZombieCenterX = aZombieRect.mX + aZombieRect.mWidth / 2;
        const int aZombieCenterY = aZombieRect.mY + aZombieRect.mHeight / 2;
        if (mBoard->PixelToGridX(aZombieCenterX, aZombieCenterY) != aFrontCol) {
            continue;
        }

        if (FindSweetPotatoOwner(aZombie) != this) {
            continue;
        }

        aZombie->StopEating();
        aZombie->StartWalkAnim(20);
        aZombie->SetRow(mRow);

        if (mApp->IsVSMode() && gTcpClientSocket >= 0) {
            U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_SET_ROW}, uint16_t(mBoard->mZombies.DataArrayGetID(aZombie)), uint16_t(mRow)};
            netplay::PutEvent(event);
        }
    }
}

void Plant::Squish() {
    if (NotOnGround())
        return;

    if (!mIsAsleep) {
        if (mSeedType == SeedType::SEED_CHERRYBOMB || mSeedType == SeedType::SEED_JALAPENO || mSeedType == SeedType::SEED_DOOMSHROOM || mSeedType == SeedType::SEED_ICESHROOM
            || mSeedType == SeedType::SEED_ICEBERG_LETTUCE || mSeedType == SeedType::SEED_CHILLY_PEPPER) {
            DoSpecial();
            return;
        } else if (mSeedType == SeedType::SEED_POTATOMINE && mState != PlantState::STATE_NOTREADY) {
            DoSpecial();
            return;
        }
    }

    if (mSeedType == SeedType::SEED_SQUASH && mState != PlantState::STATE_NOTREADY)
        return;

    mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, mRow, 8);
    mSquished = true;
    mDisappearCountdown = 500;
    mApp->PlayFoley(FoleyType::FOLEY_SQUISH);
    RemoveEffects();

    GridItem *aLadder = mBoard->GetLadderAt(mPlantCol, mRow);
    if (aLadder) {
        aLadder->GridItemDie();
    }

    if (mApp->IsIZombieLevel()) {
        mBoard->mChallenge->IZombiePlantDropRemainingSun(this);
    }
}


bool Plant::NotOnGround() const {
    if (mSeedType == SeedType::SEED_SQUASH) {
        if (mState == PlantState::STATE_SQUASH_RISING || mState == PlantState::STATE_SQUASH_FALLING || mState == PlantState::STATE_SQUASH_DONE_FALLING)
            return true;
    }

    return mSquished || mOnBungeeState == PlantOnBungeeState::RISING_WITH_BUNGEE || mDead;
}

void Plant::Draw(Sexy::Graphics *g) {
    // 根据玩家的“植物显血”功能是否开启，决定是否在游戏的原始old_Plant_Draw函数执行完后额外绘制血量文本。

    g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
    int theCelRow = 0;
    float aOffsetX = 0.0f;
    float aOffsetY = PlantDrawHeightOffset(mBoard, nullptr, mSeedType, mPlantCol, mRow);
    Rect aOldClipRect = g->mClipRect;
    bool aCeleryClipApplied = false;
    if (IsFlying(mSeedType) && mSquished) {
        aOffsetY += 30.0f;
    }
    int theCelCol = mFrame;
    Image *aImage = GetImage(mSeedType);
    if (mSquished) {
        if (mSeedType == SeedType::SEED_FLOWERPOT) {
            aOffsetY -= 15.0f;
        }
        if (mSeedType == SeedType::SEED_INSTANT_COFFEE) {
            aOffsetY -= 20.0f;
        }
        float ratioSquished = 0.5f;
        g->SetScale(1.0f, ratioSquished, 0.0f, 0.0f);
        g->SetColorizeImages(true);
        Color color = {255, 255, 255, (int)(255.0f * std::min(1.0f, mDisappearCountdown / 100.0f))};
        g->SetColor(color);
        Plant::DrawSeedType(g, mSeedType, mImitaterType, DrawVariation::VARIATION_NORMAL, aOffsetX, aOffsetY + 85.0f * (1 - ratioSquished));
        g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
        g->SetColorizeImages(false);
        return;
    }

    bool aDrawPumpkinBack = false;
    Plant *aPumpkin = nullptr;

    if (IsOnBoard()) {
        aPumpkin = mBoard->GetPumpkinAt(mPlantCol, mRow);
        if (aPumpkin != nullptr) {
            Plant *aPlantInPumpkin = mBoard->GetTopPlantAt(mPlantCol, mRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
            if (aPlantInPumpkin != nullptr && aPlantInPumpkin->mRenderOrder > aPumpkin->mRenderOrder) {
                aPlantInPumpkin = nullptr;
            }
            if (aPlantInPumpkin != nullptr && aPlantInPumpkin->mOnBungeeState == PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
                aPlantInPumpkin = nullptr;
            }
            if (aPlantInPumpkin == this) {
                aDrawPumpkinBack = true;
            }
            if (aPlantInPumpkin == nullptr && mSeedType == SeedType::SEED_PUMPKINSHELL) {
                aDrawPumpkinBack = true;
            }
        } else if (mSeedType == SeedType::SEED_PUMPKINSHELL) {
            aDrawPumpkinBack = true;
            aPumpkin = this;
        }
    } else if (mSeedType == SeedType::SEED_PUMPKINSHELL) {
        aDrawPumpkinBack = true;
        aPumpkin = this;
    }

    DrawShadow(g, aOffsetX, aOffsetY);

    if (IsFlying(mSeedType)) {
        int num3 = 0;
        if (IsOnBoard()) {
            num3 = mBoard->mMainCounter;
        } else {
            num3 = mApp->mAppCounter;
        }
        float num4 = (num3 + mRow * 97 + mPlantCol * 61) * 0.03f;
        float num5 = sin(num4) * 2.0f;
        aOffsetY += num5;
    }

    if (aDrawPumpkinBack) {
        Reanimation *reanimation = mApp->ReanimationGet(aPumpkin->mBodyReanimID);
        Graphics aPumpkinGraphics(*g);
        aPumpkinGraphics.mTransX += aPumpkin->mX - mX;
        aPumpkinGraphics.mTransY += aPumpkin->mY - mY;
        reanimation->DrawRenderGroup(&aPumpkinGraphics, 1);
    }

    aOffsetX += mShakeOffsetX;
    aOffsetY += mShakeOffsetY;
    if (IsInPlay() && mApp->IsIZombieLevel()) {
        mBoard->mChallenge->IZombieDrawPlant(g, this);
    } else if (mBodyReanimID != 0) {
        Reanimation *reanimation2 = mApp->ReanimationTryToGet(mBodyReanimID);
        if (reanimation2 != nullptr) {
            if (mSeedType == SeedType::SEED_CELERY_STALKER) {
                Rect aPlantRect = GetPlantRect();
                int aClipLeft = std::min(aOldClipRect.mX, int(aOffsetX) + (aPlantRect.mX - mX) - 100);
                int aClipRight = std::max(aOldClipRect.mX + aOldClipRect.mWidth, int(aOffsetX) + (aPlantRect.mX - mX) + aPlantRect.mWidth + 20);
                int aClipTop = std::min(aOldClipRect.mY, aOldClipRect.mY - 40);
                int aClipBottom = std::min(aOldClipRect.mY + aOldClipRect.mHeight, int(aOffsetY + 5) + (aPlantRect.mY - mY) + aPlantRect.mHeight);
                if (aClipRight > aClipLeft && aClipBottom > aClipTop) {
                    g->SetClipRect(aClipLeft, aClipTop, aClipRight - aClipLeft, aClipBottom - aClipTop);
                    aCeleryClipApplied = true;
                }
            }
            // if (plant->mGloveGrabbed)
            // {
            // SetColorizeImages(g,true);
            // Color color = {150, 255, 150, 255};
            // SetColor(g,&color);
            // }
            const bool aMirrorBonkChoyAttack = mSeedType == SeedType::SEED_BONK_CHOY && mTargetX < 0;
            if (aMirrorBonkChoyAttack) {
                const SexyTransform2D anOldOverlayMatrix = reanimation2->mOverlayMatrix;
                SexyTransform2D aMirrorMatrix;
                aMirrorMatrix.m00 = -1.0f;
                aMirrorMatrix.m02 = 80.0f;

                reanimation2->mOverlayMatrix = anOldOverlayMatrix * aMirrorMatrix;
                reanimation2->DrawRenderGroup(g, 0);
                reanimation2->mOverlayMatrix = anOldOverlayMatrix;
            } else {
                reanimation2->DrawRenderGroup(g, 0);
            }
            if (aCeleryClipApplied) {
                g->mClipRect = aOldClipRect;
            }
            // if (plant->mGloveGrabbed)
            // {
            // SetColorizeImages(g,false);
            // }
        }
    } else {
        SeedType seedType = SeedType::SEED_NONE;
        SeedType seedType2 = SeedType::SEED_NONE;
        if (mBoard != nullptr) {
            seedType = mBoard->GetSeedTypeInCursor(0);
            seedType2 = mBoard->GetSeedTypeInCursor(1);
        }
        if ((IsPartOfUpgradableTo(seedType) && mBoard->CanPlantAt(mPlantCol, mRow, seedType) == PlantingReason::PLANTING_OK)
            || (IsPartOfUpgradableTo(seedType2) && mBoard->CanPlantAt(mPlantCol, mRow, seedType2) == PlantingReason::PLANTING_OK)) {
            g->SetColorizeImages(true);
            Color color = GetFlashingColor(mBoard->mMainCounter, 90);
            g->SetColor(color);
        } else if ((seedType == SeedType::SEED_COBCANNON && mBoard->CanPlantAt(mPlantCol - 1, mRow, seedType) == PlantingReason::PLANTING_OK)
                   || (seedType2 == SeedType::SEED_COBCANNON && mBoard->CanPlantAt(mPlantCol - 1, mRow, seedType2) == PlantingReason::PLANTING_OK)) {
            g->SetColorizeImages(true);
            Color color = GetFlashingColor(mBoard->mMainCounter, 90);
            g->SetColor(color);
        } else if (mBoard != nullptr && mBoard->mTutorialState == TutorialState::TUTORIAL_SHOVEL_DIG) {
            g->SetColorizeImages(true);
            Color color = GetFlashingColor(mBoard->mMainCounter, 90);
            g->SetColor(color);
        }
        if (aImage != nullptr) {
            TodDrawImageCelF(g, aImage, aOffsetX, aOffsetY, theCelCol, theCelRow);
        }
        // if (mSeedType == a::Sprout)
        // {
        // if (plant->mGloveGrabbed)
        // {
        // SetColorizeImages(g,true);
        // Color color ={150, 255, 150, 255};
        // SetColor(g, &color);
        // }
        // TodDrawImageCelF(g, AtlasResources.IMAGE_CACHED_MARIGOLD, Constants.ZenGarden_Marigold_Sprout_Offset.X, Constants.ZenGarden_Marigold_Sprout_Offset.Y, 0, 0);
        // if (plant->mGloveGrabbed)
        // {
        // SetColorizeImages(g,false);
        // }
        // }
        g->SetColorizeImages(false);
        if (mHighlighted) {
            g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
            g->SetColorizeImages(true);
            Color color = {255, 255, 255, 196};
            g->SetColor(color);
            if (aImage != nullptr) {
                TodDrawImageCelF(g, aImage, aOffsetX, aOffsetY, theCelCol, theCelRow);
            }
            g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
            g->SetColorizeImages(false);
        } else if (mEatenFlashCountdown > 0) {
            g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
            g->SetColorizeImages(true);
            int theAlpha = std::clamp(mEatenFlashCountdown * 3, 0, 255);
            Color color = {255, 255, 255, theAlpha};
            g->SetColor(color);
            if (aImage != nullptr) {
                TodDrawImageCelF(g, aImage, aOffsetX, aOffsetY, theCelCol, theCelRow);
            }
            g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
            g->SetColorizeImages(false);
        }
    }
    if (mSeedType == SeedType::SEED_MAGNETSHROOM && !DrawMagnetItemsOnTop()) {
        DrawMagnetItems(g);
    }

    // 如果玩家开了 植物显血
    if (showPlantHealth
        || (showNutGarlicSpikeHealth
            && (mSeedType == SeedType::SEED_WALLNUT ||        //
                mSeedType == SeedType::SEED_TALLNUT ||        //
                mSeedType == SeedType::SEED_PUMPKINSHELL ||   //
                mSeedType == SeedType::SEED_GARLIC ||         //
                mSeedType == SeedType::SEED_SPIKEROCK ||      //
                mSeedType == SeedType::SEED_CELERY_STALKER || //
                mSeedType == SeedType::SEED_SWEET_POTATO))) {
        if (!IsOnlineServerModeActive()) {
            pvzstl::string str = StrFormat("%d/%d", mPlantHealth, mPlantMaxHealth);
            g->SetFont(Sexy::FONT_DWARVENTODCRAFT12);
            if (mSeedType == SeedType::SEED_PUMPKINSHELL) {
                g->SetColor(gColorYellow);
                g->DrawString(str, 0, 52);
            } else if (mSeedType == SeedType::SEED_FLOWERPOT) {
                g->SetColor(gColorBrown);
                g->DrawString(str, 0, 93);
            } else if (mSeedType == SeedType::SEED_LILYPAD) {
                g->SetColor(gColorGreen);
                g->DrawString(str, 0, 100);
            } else if (mSeedType == SeedType::SEED_COBCANNON) {
                g->SetColor(gColorWhite);
                g->DrawString(str, 40, 34);
            } else {
                g->SetColor(gColorWhite);
                g->DrawString(str, 0, 34);
            }
            g->SetFont(nullptr);
        }
    }
}

void Plant::DrawShadow(Sexy::Graphics *g, float theOffsetX, float theOffsetY) {
    if (mSeedType == SeedType::SEED_LILYPAD || mSeedType == SeedType::SEED_STARFRUIT || mSeedType == SeedType::SEED_TANGLEKELP || mSeedType == SeedType::SEED_SEASHROOM
        || mSeedType == SeedType::SEED_COBCANNON || mSeedType == SeedType::SEED_SPIKEWEED || mSeedType == SeedType::SEED_SPIKEROCK || mSeedType == SeedType::SEED_GRAVEBUSTER
        || mSeedType == SeedType::SEED_CATTAIL || mOnBungeeState == PlantOnBungeeState::RISING_WITH_BUNGEE)
        return;

    if (IsOnBoard() && mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && mApp->mZenGarden->mGardenType == GardenType::GARDEN_MAIN)
        return;

    int aShadowType = 0;
    float aShadowOffsetX = -3.0f;
    float aShadowOffsetY = 51.0f;
    float aScale = 1.0f;
    if (mBoard && mBoard->StageIsNight()) {
        aShadowType = 1;
    }

    if (mSeedType == SeedType::SEED_SQUASH) {
        if (mBoard) {
            aShadowOffsetY += mBoard->GridToPixelY(mPlantCol, mRow) - mY;
        }
        aShadowOffsetY += 5.0f;
    } else if (mSeedType == SeedType::SEED_PUFFSHROOM) {
        aScale = 0.5f;
        aShadowOffsetY = 42.0f;
    } else if (mSeedType == SeedType::SEED_SUNSHROOM) {
        aShadowOffsetY = 42.0f;
        if (mState == PlantState::STATE_SUNSHROOM_SMALL) {
            aScale = 0.5f;
        } else if (mState == PlantState::STATE_SUNSHROOM_GROWING) {
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            aScale = 0.5f + 0.5f * aBodyReanim->mAnimTime;
        }
    } else if (mSeedType == SeedType::SEED_UMBRELLA) {
        aScale = 0.5f;
        aShadowOffsetX = -7.0f;
        aShadowOffsetY = 52.0f;
    } else if (mSeedType == SeedType::SEED_FUMESHROOM || mSeedType == SeedType::SEED_GLOOMSHROOM) {
        aScale = 1.3f;
        aShadowOffsetY = 47.0f;
    } else if (mSeedType == SeedType::SEED_CABBAGEPULT || mSeedType == SeedType::SEED_MELONPULT || mSeedType == SeedType::SEED_WINTERMELON) {
        aShadowOffsetY = 47.0f;
    } else if (mSeedType == SeedType::SEED_KERNELPULT) {
        aShadowOffsetX = 0.0f;
        aShadowOffsetY = 47.0f;
    } else if (mSeedType == SeedType::SEED_SCAREDYSHROOM) {
        aShadowOffsetX = -9.0f;
        aShadowOffsetY = 55.0f;
    } else if (mSeedType == SeedType::SEED_CHOMPER) {
        aShadowOffsetX = -21.0f;
        aShadowOffsetY = 57.0f;
    } else if (mSeedType == SeedType::SEED_FLOWERPOT) {
        aShadowOffsetX = -4.0f;
        aShadowOffsetY = 46.0f;
    } else if (mSeedType == SeedType::SEED_TALLNUT) {
        aShadowOffsetY = 54.0f;
        aScale = 1.3f;
    } else if (mSeedType == SeedType::SEED_PUMPKINSHELL) {
        aShadowOffsetY = 46.0f;
        aScale = 1.4f;
    } else if (mSeedType == SeedType::SEED_CACTUS) {
        aShadowOffsetX = -8.0f;
        aShadowOffsetY = 50.0f;
    } else if (mSeedType == SeedType::SEED_PLANTERN) {
        aShadowOffsetY = 57.0f;
    } else if (mSeedType == SeedType::SEED_INSTANT_COFFEE) {
        aShadowOffsetY = 71.0f;
    } else if (mSeedType == SeedType::SEED_GIANT_WALLNUT) {
        aShadowOffsetX = -33.0f;
        aShadowOffsetY = 56.0f;
        aScale = 1.7f;
    } else if (mSeedType == SeedType::SEED_CELERY_STALKER) {
        aShadowOffsetX = -3.0f;
        aShadowOffsetY = 57.0f;
        aScale = 1.3f;
    }

    if (Plant::IsFlying(mSeedType)) {
        aShadowOffsetY += 10.0f;
        if (mBoard && (mBoard->GetTopPlantAt(mPlantCol, mRow, TOPPLANT_ONLY_NORMAL_POSITION) || mBoard->GetTopPlantAt(mPlantCol, mRow, TOPPLANT_ONLY_PUMPKIN)))
            return;
    }

    if (aShadowType == 0) {
        TodDrawImageCelCenterScaledF(g, IMAGE_PLANTSHADOW, theOffsetX + aShadowOffsetX, theOffsetY + aShadowOffsetY, 0, aScale, aScale);
    } else {
        TodDrawImageCelCenterScaledF(g, IMAGE_PLANTSHADOW2, theOffsetX + aShadowOffsetX, theOffsetY + aShadowOffsetY, 0, aScale, aScale);
    }
}

void Plant::DrawSeedType(Sexy::Graphics *g, SeedType theSeedType, SeedType theImitaterType, DrawVariation theDrawVariation, float thePosX, float thePosY) {
    // 用于绘制卡槽内的模仿者SeedPacket变白效果、模仿者变身后的植物被压扁的白色效果、模仿者变身前被压扁后绘制模仿者自己而非变身后的植物。
    float oldTransX = g->mTransX;
    float oldTransY = g->mTransY;

    float oldScaleX = g->mScaleX;
    float oldScaleY = g->mScaleY;
    float oldScaleOrigX = g->mScaleOrigX;
    float oldScaleOrigY = g->mScaleOrigY;

    Rect oldClipRect = g->mClipRect;

    Color oldColor = g->GetColor();
    bool oldColorizeImages = g->GetColorizeImages();
    SeedType theSeedType2 = theSeedType;

    if ((theSeedType == theImitaterType && theImitaterType != SeedType::SEED_NONE) ||         // seedPacket中的灰色模仿者卡片在冷却完成后
        (theImitaterType == SeedType::SEED_IMITATER && theSeedType != SeedType::SEED_NONE)) { // 模仿者变身之后的植物被压扁
        switch (theSeedType2) {
            case SeedType::SEED_POTATOMINE:
            case SeedType::SEED_HYPNOSHROOM:
            case SeedType::SEED_LILYPAD:
            case SeedType::SEED_SQUASH:
            case SeedType::SEED_GARLIC:
                theDrawVariation = DrawVariation::VARIATION_IMITATER_LESS;
                break;
            case SeedType::SEED_IMITATER:
                theDrawVariation = DrawVariation::VARIATION_NORMAL;
                break;
            default:
                theDrawVariation = DrawVariation::VARIATION_IMITATER;
                break;
        }
    }
    LawnApp *lawnApp = gLawnApp;
    float v24 = NAN, v25 = NAN;
    if (lawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BIG_TIME
        && (theSeedType2 == SeedType::SEED_SUNFLOWER || theSeedType2 == SeedType::SEED_WALLNUT || theSeedType2 == SeedType::SEED_MARIGOLD)) {
        v24 = -40.0f;
        v25 = -20.0f;
        g->mScaleX = g->mScaleX * 1.5f;
        g->mScaleY = g->mScaleY * 1.5f;
    } else {
        v24 = 0.0f;
        v25 = 0.0f;
    }
    if (theSeedType2 == SeedType::SEED_LEFTPEATER) {
        v25 = v25 + g->mScaleX * 80.0f;
        g->mScaleX = -g->mScaleX;
    }
    if (Challenge::IsZombieSeedType(theSeedType2)) {
        ZombieType theZombieType = Challenge::IZombieSeedTypeToZombieType(theSeedType2);
        if (theZombieType != ZombieType::ZOMBIE_INVALID) {
            if (theZombieType < ZombieType::NUM_CACHED_ZOMBIE_TYPES) {
                lawnApp->mReanimatorCache->DrawCachedZombie(g, thePosX, thePosY, theZombieType);
            } else {
                lawnApp->mReanimatorCache->DrawCachedExtendedZombie(g, thePosX, thePosY, theZombieType);
            }
        }
        //        return;
    } else {
        PlantDefinition aPlantDef = GetPlantDefinition(theSeedType2);
        if (theSeedType2 == SeedType::SEED_GIANT_WALLNUT) {
            g->mScaleX = g->mScaleX * 1.4f;
            g->mScaleY = g->mScaleY * 1.4f;
            TodDrawImageScaledF(g, Sexy::IMAGE_REANIM_WALLNUT_BODY, thePosX - 53.0f, thePosY - 56.0f, g->mScaleX, g->mScaleY);
        } else if (aPlantDef.mReanimationType == -1) {
            int v29 = 0;
            if (theSeedType2 == SeedType::SEED_KERNELPULT)
                v29 = 2;
            else
                v29 = theSeedType2 == SeedType::SEED_TWINSUNFLOWER;

            Image *aImage = GetImage(theSeedType2);
            int v31 = aImage->mNumCols;
            int v32 = 0;
            if (v31 > 2)
                v32 = 2;
            else
                v32 = v31 - 1;
            TodDrawImageCelScaledF(g, aImage, v25 + thePosX, v24 + thePosY, v32, v29, g->mScaleX, g->mScaleY);
        } else {
            if (theSeedType < NUM_SEED_TYPES) {
                lawnApp->mReanimatorCache->DrawCachedPlant(g, v25 + thePosX, v24 + thePosY, theSeedType2, theDrawVariation);
            } else if (theSeedType >= SEED_ICEBERG_LETTUCE) {
                lawnApp->mReanimatorCache->DrawCachedExtendedPlant(g, v25 + thePosX, v24 + thePosY, theSeedType2, theDrawVariation);
            }
        }
    }
    g->mClipRect = oldClipRect;

    g->mScaleX = oldScaleX;
    g->mScaleY = oldScaleY;
    g->mScaleOrigX = oldScaleOrigX;
    g->mScaleOrigY = oldScaleOrigY;

    g->mTransX = oldTransX;
    g->mTransY = oldTransY;

    g->SetColor(oldColor);
    g->SetColorizeImages(oldColorizeImages);
}

void Plant::KillAllPlantsNearDoom() {
    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == mRow && aPlant->mPlantCol == mPlantCol) {
            aPlant->Die();
        }
    }
}

void Plant::DoSpecial() {
    // 试图修复辣椒爆炸后反而在本行的末尾处产生冰道。失败。

    if (mApp->IsVSMode() && mApp->mGameScene == SCENE_PLAYING) {
        if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
            return;

        if (gTcpClientSocket >= 0) {
            if (mSeedType == SeedType::SEED_ICEBERG_LETTUCE) {
                Zombie *aZombie = FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY);
                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_ICEBERG_LETTUCE_DO_SPECIAL},
                                      uint16_t(mBoard->mPlants.DataArrayGetID(this)),
                                      aZombie == nullptr ? NETPLAY_ZOMBIE_ID_NULL : uint16_t(mBoard->mZombies.DataArrayGetID(aZombie))};
                netplay::PutEvent(event);
                IcebergLettuceDoSpecial(aZombie);
                return;
            }

            U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_DO_SPECIAL}, uint16_t(mBoard->mPlants.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
    }

    DoSpecial_Origin();
}

void Plant::IcebergLettuceDoSpecial(Zombie *theZombie) {
    if (theZombie != nullptr && theZombie->CanBeFrozen()) {
        theZombie->mIceTrapCounter = 1000;
        theZombie->StopZombieSound();
        if (theZombie->mZombieType == ZombieType::ZOMBIE_BALLOON) {
            theZombie->BalloonPropellerHatSpin(false);
        }
        if (theZombie->mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_SPIT) {
            mBoard->RemoveParticleByType(ParticleEffect::PARTICLE_ZOMBIE_BOSS_FIREBALL);
        }
        if (theZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && theZombie->mHasObject) {
            theZombie->ExplorerTorchConvert(false);
        }
        theZombie->UpdateAnimSpeed();
    }

    Die();
}

void Plant::DoSpecial_Origin() {
    int aPosX = mX + mWidth / 2;
    int aPosY = mY + mHeight / 2;
    int aDamageRangeFlags = GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);

    switch (mSeedType) {
        case SeedType::SEED_BLOVER: {
            if (mState != PlantState::STATE_DOINGSPECIAL) {
                mState = PlantState::STATE_DOINGSPECIAL;
                BlowAwayFliers(mX, mRow);
            }
            break;
        }
        case SeedType::SEED_CHERRYBOMB: {
            mApp->PlayFoley(FoleyType::FOLEY_CHERRYBOMB);
            mApp->PlayFoley(FoleyType::FOLEY_JUICY);

            if (mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, 115, 1, true, aDamageRangeFlags) >= 10)
                if (!mApp->IsLittleTroubleLevel())
                    mBoard->GrantAchievement(AchievementType::ACHIEVEMENT_EXPLODONATOR, true);

            mApp->AddTodParticle(aPosX, aPosY, (int)RenderLayer::RENDER_LAYER_TOP, ParticleEffect::PARTICLE_POWIE);
            mBoard->ShakeBoard(3, -4);

            Die();
            break;
        }
        case SeedType::SEED_DOOMSHROOM: {
            mApp->PlaySample(Sexy::SOUND_DOOMSHROOM);

            mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, 250, 3, true, aDamageRangeFlags);
            KillAllPlantsNearDoom();

            mApp->AddTodParticle(aPosX, aPosY, (int)RenderLayer::RENDER_LAYER_TOP, ParticleEffect::PARTICLE_DOOM);
            GridItem *crater = mBoard->AddACrater(mPlantCol, mRow);
            if (crater) {
                crater->mGridItemCounter = 18000;
            }
            mBoard->ShakeBoard(3, -4);

            Die();
            break;
        }
        case SeedType::SEED_JALAPENO: {
            mApp->PlayFoley(FoleyType::FOLEY_JALAPENO_IGNITE);
            mApp->PlayFoley(FoleyType::FOLEY_JUICY);

            mBoard->DoFwoosh(mRow);
            mBoard->ShakeBoard(3, -4);

            BurnRow(mRow);
            mBoard->mIceTimer[mRow] = 20;

            Die();
            break;
        }
        case SeedType::SEED_UMBRELLA:
        case SeedType::SEED_BLOOMERANG: {
            if (mState != PlantState::STATE_UMBRELLA_TRIGGERED && mState != PlantState::STATE_UMBRELLA_REFLECTING) {
                mState = PlantState::STATE_UMBRELLA_TRIGGERED;
                mStateCountdown = 5;

                PlayBodyReanim("anim_block", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 22.0f);
            }

            break;
        }
        case SeedType::SEED_ICESHROOM: {
            mApp->PlayFoley(FoleyType::FOLEY_FROZEN);
            IceZombies();
            mApp->AddTodParticle(aPosX, aPosY, (int)RenderLayer::RENDER_LAYER_TOP, ParticleEffect::PARTICLE_ICE_TRAP);

            Die();
            TriggerVibration(VibrationEffect::VIBRATION_ICE_TRAP);
            break;
        }
        case SeedType::SEED_POTATOMINE: {
            aPosX = mX + mWidth / 2 - 20;
            aPosY = mY + mHeight / 2;

            mApp->PlaySample(SOUND_POTATO_MINE);
            mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, 60, 0, false, aDamageRangeFlags);

            int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, mRow, 0);
            mApp->AddTodParticle(aPosX + 20.0f, aPosY, aRenderPosition, ParticleEffect::PARTICLE_POTATO_MINE);
            mBoard->ShakeBoard(3, -4);

            Die();
            break;
        }
        case SeedType::SEED_INSTANT_COFFEE: {
            Plant *aPlant = mBoard->GetTopPlantAt(mPlantCol, mRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
            if (aPlant && aPlant->mIsAsleep) {
                aPlant->mWakeUpCounter = 100;
            }

            mState = PlantState::STATE_DOINGSPECIAL;
            PlayBodyReanim("anim_crumble", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 22.0f);
            mApp->PlayFoley(FoleyType::FOLEY_COFFEE);

            break;
        }
        case SeedType::SEED_ICEBERG_LETTUCE: {
            IcebergLettuceDoSpecial(FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY));
            break;
        }
        case SeedType::SEED_CHILLY_PEPPER: {
            mApp->PlayFoley(FoleyType::FOLEY_FROZEN);
            mApp->PlayFoley(FoleyType::FOLEY_JUICY);

            mBoard->DoChillyFwoosh(mRow, mX, mY);
            mBoard->ShakeBoard(3, -4);

            Zombie *aZombie = nullptr;
            while (mBoard->IterateZombies(aZombie)) {
                if ((aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow == mRow) && aZombie->EffectedByDamage(aDamageRangeFlags)) {
                    aZombie->HitIceTrap();
                    if (aZombie->CanBeFrozen()) {
                        aZombie->TakeDamage(880, 1U);
                    }
                }
            }

            Die();
            break;
        }
        default:
            break;
    }
}

void Plant::CobCannonFire(int x, int y) {
    mState = PlantState::STATE_COBCANNON_FIRING;
    mShootingCounter = 206;
    PlayBodyReanim("anim_shooting", REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0f);
    mTargetY = y;
    mTargetX = x - 47;

    Reanimation *bodyReanim = mApp->ReanimationGet(mBodyReanimID);
    if (bodyReanim != nullptr) {
        ReanimatorTrackInstance *trackInstance = bodyReanim->GetTrackInstanceByName("CobCannon_cob");
        if (trackInstance != nullptr) {
            trackInstance->mTrackColor = Sexy::Color::White;
        }
    }
}

void Plant::Fire(Zombie *theTargetZombie, int theRow, PlantWeapon thePlantWeapon, GridItem *theTargetGridItem) {
    if (mApp->IsVSMode()) {
        if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
            return;

        if (gTcpClientSocket >= 0) {
            U16U16U16UNI32UNI32_Event event{};

            event.type = EventType::EVENT_SERVER_BOARD_PLANT_FIRE;
            event.data1 = uint16_t(mBoard->mPlants.DataArrayGetID(this));
            event.data2 = theTargetZombie == nullptr ? NETPLAY_ZOMBIE_ID_NULL : uint16_t(mBoard->mZombies.DataArrayGetID(theTargetZombie));
            event.data4.u16x2.u16_1 = uint16_t(theRow);
            event.data4.u16x2.u16_2 = uint16_t(thePlantWeapon);
            // 如果同时传入有效的 theTargetZombie 和 theTargetGridItem 会导致投手弹道计算错误
            if (theTargetZombie) { // 存在僵尸目标时传入空的场地物 ID
                event.data5.u16x2.u16_1 = NETPLAY_GRIDITEM_ID_NULL;
            } else {
                event.data5.u16x2.u16_1 = theTargetGridItem == nullptr ? NETPLAY_GRIDITEM_ID_NULL : uint16_t(mBoard->mGridItems.DataArrayGetID(theTargetGridItem));
            }
            netplay::PutEvent(event);
            //            SyncPingPongAnimationToClient();
            //            SyncAnimationToClient();
        }
    }

    Fire_Origin(theTargetZombie, theRow, thePlantWeapon, theTargetGridItem);
}

void Plant::DoRowAreaDamage(int theDamage, unsigned int theDamageFlags) {
    int aDamageRangeFlags = GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
    Rect aAttackRect = GetPlantAttackRect(PlantWeapon::WEAPON_PRIMARY);

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (IsSpiky() && aZombie->IsFlying()) {
            continue;
        }

        int aDiffY = (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS) ? 0 : (aZombie->mRow - mRow);
        if (mSeedType == SeedType::SEED_GLOOMSHROOM) {
            if (aDiffY < -1 || aDiffY > 1)
                continue;
        } else if (aDiffY) {
            continue;
        }

        if (aZombie->mOnHighGround == IsOnHighGround() && aZombie->EffectedByDamage(aDamageRangeFlags)) {
            Rect aZombieRect = aZombie->GetZombieRect();
            if (GetRectOverlap(aAttackRect, aZombieRect) > 0) {
                int aDamage = theDamage;
                if ((aZombie->mZombieType == ZombieType::ZOMBIE_ZAMBONI || aZombie->mZombieType == ZombieType::ZOMBIE_CATAPULT) && TestBit(theDamageFlags, DamageFlags::DAMAGE_SPIKE)) {
                    aDamage = 1800;

                    if (mSeedType == SeedType::SEED_SPIKEROCK) {
                        SpikeRockTakeDamage();
                    } else {
                        Die();
                    }
                }

                aZombie->TakeDamage(aDamage, theDamageFlags);
                mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
            }
        }
    }

    // 对战模式下，大喷菇会命中前方最近的墓碑/靶子。这里补充召唤墓碑同逻辑。
    if (mSeedType == SeedType::SEED_FUMESHROOM && mApp->IsVSMode()) {
        int aStartGridX = mBoard->PixelToGridX(mX, mY);
        for (int aDist = 1; aDist <= 3; ++aDist) {
            int aGridX = aStartGridX + aDist;
            GridItem *aGridItem = mBoard->GetGridItemAt(GridItemType::GRIDITEM_GRAVESTONE, aGridX, mRow);
            if (!aGridItem) {
                aGridItem = mBoard->GetGridItemAt(GridItemType::GRIDITEM_MP_BURIAL_MOUND, aGridX, mRow);
            }
            if (!aGridItem) {
                aGridItem = mBoard->GetGridItemAt(GridItemType::GRIDITEM_MP_TARGET_ZOMBIE, aGridX, mRow);
            }
            if (aGridItem) {
                aGridItem->TakeDamage(theDamage, theDamageFlags);
                return;
            }
        }
        return;
    }

    GridItem *aGridItem = nullptr;
    while (mBoard->IterateGridItems(aGridItem)) {
        int aDiffY = aGridItem->mGridY - mRow;
        if (mSeedType == SeedType::SEED_GLOOMSHROOM) {
            if (aDiffY < -1 || aDiffY > 1) {
                continue;
            }
        } else if (aDiffY) {
            continue;
        }

        Rect aGridItemRect = aGridItem->GetItemRect();
        if (GetRectOverlap(aAttackRect, aGridItemRect) <= 0) {
            continue;
        }

        aGridItem->TakeDamage(theDamage, theDamageFlags);
        mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
    }
}

void Plant::Fire_Origin(Zombie *theTargetZombie, int theRow, PlantWeapon thePlantWeapon, GridItem *theTargetGridItem) {
    if (mSeedType == SeedType::SEED_FUMESHROOM) {
        DoRowAreaDamage(20, 2U);
        mApp->PlayFoley(FoleyType::FOLEY_FUME);
        return;
    }
    if (mSeedType == SeedType::SEED_GLOOMSHROOM) {
        DoRowAreaDamage(20, 2U);
        return;
    }
    if (mSeedType == SeedType::SEED_STARFRUIT) {
        StarFruitFire();
        return;
    }

    ProjectileType aProjectileType;
    switch (mSeedType) {
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_GATLINGPEA:
        case SeedType::SEED_LEFTPEATER:
            aProjectileType = ProjectileType::PROJECTILE_PEA;
            break;
        case SeedType::SEED_SNOWPEA:
            aProjectileType = ProjectileType::PROJECTILE_SNOWPEA;
            break;
        case SeedType::SEED_PUFFSHROOM:
        case SeedType::SEED_SCAREDYSHROOM:
        case SeedType::SEED_SEASHROOM:
            aProjectileType = ProjectileType::PROJECTILE_PUFF;
            break;
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_CATTAIL:
            aProjectileType = ProjectileType::PROJECTILE_SPIKE;
            break;
        case SeedType::SEED_CABBAGEPULT:
            aProjectileType = ProjectileType::PROJECTILE_CABBAGE;
            break;
        case SeedType::SEED_KERNELPULT:
            aProjectileType = ProjectileType::PROJECTILE_KERNEL;
            break;
        case SeedType::SEED_MELONPULT:
            aProjectileType = ProjectileType::PROJECTILE_MELON;
            break;
        case SeedType::SEED_WINTERMELON:
            aProjectileType = ProjectileType::PROJECTILE_WINTERMELON;
            break;
        case SeedType::SEED_COBCANNON:
            aProjectileType = ProjectileType::PROJECTILE_COBBIG;
            break;
        case SeedType::SEED_SPORESHROOM:
            aProjectileType = ProjectileType::PROJECTILE_SPORE;
            break;
        case SeedType::SEED_BLOOMERANG:
            aProjectileType = ProjectileType::PROJECTILE_BOOMERANG;
            break;
        default:
            break;
    }
    if (mSeedType == SeedType::SEED_KERNELPULT && thePlantWeapon == PlantWeapon::WEAPON_SECONDARY) {
        aProjectileType = ProjectileType::PROJECTILE_BUTTER;
    }

    mApp->PlayFoley(FoleyType::FOLEY_THROW);
    if (mSeedType == SeedType::SEED_SNOWPEA || mSeedType == SeedType::SEED_WINTERMELON) {
        mApp->PlayFoley(FoleyType::FOLEY_SNOW_PEA_SPARKLES);
    } else if (mSeedType == SeedType::SEED_PUFFSHROOM || mSeedType == SeedType::SEED_SCAREDYSHROOM || mSeedType == SeedType::SEED_SEASHROOM) {
        mApp->PlayFoley(FoleyType::FOLEY_PUFF);
    }

    int aOriginX = 0, aOriginY = 0;
    if (mSeedType == SeedType::SEED_PUFFSHROOM) {
        aOriginX = mX + 40;
        aOriginY = mY + 40;
    } else if (mSeedType == SeedType::SEED_SEASHROOM) {
        aOriginX = mX + 45;
        aOriginY = mY + 63;
    } else if (mSeedType == SeedType::SEED_CABBAGEPULT || mSeedType == SeedType::SEED_SPORESHROOM) {
        aOriginX = mX + 5;
        aOriginY = mY - 12;
    } else if (mSeedType == SeedType::SEED_MELONPULT || mSeedType == SeedType::SEED_WINTERMELON) {
        aOriginX = mX + 25;
        aOriginY = mY - 46;
    } else if (mSeedType == SeedType::SEED_CATTAIL) {
        aOriginX = mX + 20;
        aOriginY = mY - 3;
    } else if (mSeedType == SeedType::SEED_KERNELPULT && thePlantWeapon == PlantWeapon::WEAPON_PRIMARY) {
        aOriginX = mX + 19;
        aOriginY = mY - 37;
    } else if (mSeedType == SeedType::SEED_KERNELPULT && thePlantWeapon == PlantWeapon::WEAPON_SECONDARY) {
        aOriginX = mX + 12;
        aOriginY = mY - 56;
    } else if (mSeedType == SeedType::SEED_PEASHOOTER || mSeedType == SeedType::SEED_SNOWPEA || mSeedType == SeedType::SEED_REPEATER) {
        int aOffsetX = 0, aOffsetY = 0;
        GetPeaHeadOffset(aOffsetX, aOffsetY);
        aOriginX = mX + aOffsetX + 24;
        aOriginY = mY + aOffsetY - 33;
    } else if (mSeedType == SeedType::SEED_LEFTPEATER) {
        int aOffsetX = 0, aOffsetY = 0;
        GetPeaHeadOffset(aOffsetX, aOffsetY);
        aOriginX = mX - aOffsetX + 27;
        aOriginY = mY + aOffsetY - 33;
    } else if (mSeedType == SeedType::SEED_GATLINGPEA) {
        int aOffsetX = 0, aOffsetY = 0;
        GetPeaHeadOffset(aOffsetX, aOffsetY);
        aOriginX = mX + aOffsetX + 34;
        aOriginY = mY + aOffsetY - 33;
    } else if (mSeedType == SeedType::SEED_SPLITPEA) {
        int aOffsetX = 0, aOffsetY = 0;
        GetPeaHeadOffset(aOffsetX, aOffsetY);
        aOriginY = mY + aOffsetY - 33;

        if (thePlantWeapon == PlantWeapon::WEAPON_SECONDARY) {
            aOriginX = mX + aOffsetX - 64;
        } else {
            aOriginX = mX + aOffsetX + 24;
        }
    } else if (mSeedType == SeedType::SEED_THREEPEATER) {
        aOriginX = mX + 45;
        aOriginY = mY + 10;
    } else if (mSeedType == SeedType::SEED_SCAREDYSHROOM) {
        aOriginX = mX + 29;
        aOriginY = mY + 21;
    } else if (mSeedType == SeedType::SEED_CACTUS) {
        if (thePlantWeapon == PlantWeapon::WEAPON_PRIMARY) {
            aOriginX = mX + 93;
            aOriginY = mY - 50;
        } else {
            aOriginX = mX + 70;
            aOriginY = mY + 23;
        }
    } else if (mSeedType == SeedType::SEED_COBCANNON) {
        aOriginX = mX - 44;
        aOriginY = mY - 184;
    } else if (mSeedType == SeedType::SEED_BLOOMERANG) {
        aOriginX = mX + 10;
        aOriginY = mY - 18;
    } else {
        aOriginX = mX + 10;
        aOriginY = mY + 5;
    }
    if (mBoard->GetFlowerPotAt(mPlantCol, mRow)) {
        aOriginY -= 5;
    }

    if (mSeedType == SeedType::SEED_SNOWPEA) {
        int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_LAWN_MOWER, mRow, 1);
        mApp->AddTodParticle(aOriginX + 8, aOriginY + 13, aRenderPosition, ParticleEffect::PARTICLE_SNOWPEA_PUFF);
    } else if (mSeedType == SeedType::SEED_PUFFSHROOM) {
        int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_LAWN_MOWER, mRow, 1);
        mApp->AddTodParticle(aOriginX + 18, aOriginY + 13, aRenderPosition, ParticleEffect::PARTICLE_PUFFSHROOM_MUZZLE);
    } else if (mSeedType == SeedType::SEED_SCAREDYSHROOM) {
        int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_LAWN_MOWER, mRow, 1);
        mApp->AddTodParticle(aOriginX + 27, aOriginY + 13, aRenderPosition, ParticleEffect::PARTICLE_PUFFSHROOM_MUZZLE);
    }

    Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder - 1, theRow, aProjectileType);
    aProjectile->mDamageRangeFlags = GetDamageRangeFlags(thePlantWeapon);

    if (mSeedType == SeedType::SEED_BLOOMERANG) {
        aProjectile->mRelatedPlantID = mBoard->PlantGetID(this);
        aProjectile->mVelX = 6.6f; // 直线子弹的两倍速

        // 发射时预先锁定本行最靠前的三个目标，并在最远锁定目标的 X + 80 处停留后折返。
        // mHitZombieIDs / mHitGridItemIDs 保存锁定名单；mHitTorchwoodGridX 保存去程命中位图；mCobTargetRow 保存回程命中位图。
        constexpr int BOOMERANG_MAX_TARGETS = 3;

        struct BoomerangLockedTarget {
            float mTargetX;
            ZombieID mZombieID;
            GridItemID mGridItemID;
        };

        BoomerangLockedTarget aLockedTargets[BOOMERANG_MAX_TARGETS];
        int aLockedTargetCount = 0;

        auto AddBoomerangTarget = [&](float theTargetX, ZombieID theZombieID, GridItemID theGridItemID) {
            if (theTargetX + 40.0f < static_cast<float>(aOriginX)) {
                return;
            }

            int anInsertIndex = aLockedTargetCount;
            if (aLockedTargetCount < BOOMERANG_MAX_TARGETS) {
                ++aLockedTargetCount;
            } else {
                if (theTargetX >= aLockedTargets[BOOMERANG_MAX_TARGETS - 1].mTargetX) {
                    return;
                }
                anInsertIndex = BOOMERANG_MAX_TARGETS - 1;
            }

            while (anInsertIndex > 0 && theTargetX < aLockedTargets[anInsertIndex - 1].mTargetX) {
                aLockedTargets[anInsertIndex] = aLockedTargets[anInsertIndex - 1];
                --anInsertIndex;
            }

            aLockedTargets[anInsertIndex] = {theTargetX, theZombieID, theGridItemID};
        };

        Zombie *aZombie = nullptr;
        while (mBoard->IterateZombies(aZombie)) {
            if (aZombie->mDead) {
                continue;
            }
            if (aZombie->mZombieType != ZombieType::ZOMBIE_BOSS && aZombie->mRow != theRow) {
                continue;
            }
            if (!aZombie->EffectedByDamage(static_cast<unsigned int>(aProjectile->mDamageRangeFlags))) {
                continue;
            }
            if (aZombie->mOnHighGround && !IsOnHighGround()) {
                continue;
            }

            const Rect aZombieRect = aZombie->GetZombieRect();
            if (aZombieRect.mX + aZombieRect.mWidth <= aOriginX) {
                continue;
            }

            AddBoomerangTarget(float(aZombie->mX), mBoard->ZombieGetID(aZombie), GridItemID::GRIDITEMID_NULL);
        }

        if (mApp->IsVSMode()) {
            bool aHasGravestoneInRow = false;
            GridItem *aGridItem = nullptr;
            while (mBoard->IterateGridItems(aGridItem)) {
                if (aGridItem->mGridY == theRow && (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND)) {
                    aHasGravestoneInRow = true;
                    break;
                }
            }

            // 墓碑只锁定本行最靠近回旋镖射手的一个。
            GridItem *aClosestGravestone = nullptr;
            float aClosestGravestoneX = 0.0f;

            aGridItem = nullptr;
            while (mBoard->IterateGridItems(aGridItem)) {
                if (aGridItem->mGridY != theRow) {
                    continue;
                }

                if (aHasGravestoneInRow && aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) {
                    continue;
                }

                const bool aIsGravestone = aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND;
                const bool aDamageableGridItem = aIsGravestone || (aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE && aGridItem->mVSTargetZombieHealth > 0);
                if (!aDamageableGridItem) {
                    continue;
                }

                const Rect aGridItemRect = aGridItem->GetItemRect();
                if (aGridItemRect.mX + aGridItemRect.mWidth <= aOriginX) {
                    continue;
                }

                if (aIsGravestone) {
                    const float aGridItemX = float(aGridItemRect.mX);
                    if (aClosestGravestone == nullptr || aGridItemX < aClosestGravestoneX) {
                        aClosestGravestone = aGridItem;
                        aClosestGravestoneX = aGridItemX;
                    }
                    continue;
                }

                AddBoomerangTarget(float(aGridItemRect.mX), ZombieID::ZOMBIEID_NULL, mBoard->GridItemGetID(aGridItem));
            }

            if (aClosestGravestone != nullptr) {
                AddBoomerangTarget(aClosestGravestoneX, ZombieID::ZOMBIEID_NULL, mBoard->GridItemGetID(aClosestGravestone));
            }
        }

        for (int i = 0; i < BOOMERANG_MAX_TARGETS; ++i) {
            aProjectile->mHitZombieIDs[i] = ZombieID::ZOMBIEID_NULL;
            aProjectile->mHitGridItemIDs[i] = GridItemID::GRIDITEMID_NULL;
        }

        for (int i = 0; i < aLockedTargetCount; ++i) {
            aProjectile->mHitZombieIDs[i] = aLockedTargets[i].mZombieID;
            aProjectile->mHitGridItemIDs[i] = aLockedTargets[i].mGridItemID;
        }

        aProjectile->mPierceHitCount = aLockedTargetCount;
        aProjectile->mHitTorchwoodGridX = 0;
        aProjectile->mCobTargetRow = 0;

        if (aLockedTargetCount > 0) {
            aProjectile->mCobTargetX = aLockedTargets[aLockedTargetCount - 1].mTargetX + 80.0f;
        } else {
            // 动画期间目标全部消失时，保留一个安全的出界折返点。
            aProjectile->mCobTargetX = 790.0f;
        }

        return;
    }

    if (mSeedType == SeedType::SEED_CABBAGEPULT || mSeedType == SeedType::SEED_KERNELPULT || mSeedType == SeedType::SEED_MELONPULT || mSeedType == SeedType::SEED_WINTERMELON
        || mSeedType == SeedType::SEED_SPORESHROOM) {
        float aRangeX = NAN, aRangeY = NAN;
        if (theTargetZombie) {
            Rect aZombieRect = theTargetZombie->GetZombieRect();
            aRangeX = theTargetZombie->ZombieTargetLeadX(50.0f) - aOriginX - 30.0f;
            aRangeY = aZombieRect.mY - aOriginY;

            if (theTargetZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING) {
                aRangeX -= 60.0f;
            }
            if (theTargetZombie->mZombieType == ZombieType::ZOMBIE_POGO && theTargetZombie->mHasObject) {
                aRangeX -= 60.0f;
            }
            if (theTargetZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL) {
                aRangeX -= 40.0f;
            }
            if (theTargetZombie->mZombieType == ZombieType::ZOMBIE_BOSS) {
                aRangeY = mBoard->GridToPixelY(8, mRow) - aOriginY;
            }
        } else if (theTargetGridItem) {
            aRangeX = mBoard->GridToPixelX(theTargetGridItem->mGridX, theTargetGridItem->mGridY) - aOriginX;
            // 为靶子僵尸添加半格距离，以匹配靶子僵尸的碰撞箱。水路除外，以修复西瓜投手无法命中靶子
            if (theTargetGridItem->mGridItemType == GRIDITEM_MP_TARGET_ZOMBIE && mBoard->mPlantRow[theRow] != PlantRowType::PLANTROW_POOL) {
                aRangeX += mBoard->GridCellWidth(theTargetGridItem->mGridX, theTargetGridItem->mGridY) / 2.0f;
            }
            aRangeY = float(mBoard->GridToPixelY(theTargetGridItem->mGridX, theTargetGridItem->mGridY) - aOriginY) * 0.0083333f - 7.0f;
        } else {
            aRangeX = 700.0f - aOriginX;
            aRangeY = 0.0f;
        }
        if (aRangeX < 40.0f) {
            aRangeX = 40.0f;
        }

        aProjectile->mMotionType = ProjectileMotion::MOTION_LOBBED;
        aProjectile->mVelX = aRangeX / 120.0f;
        aProjectile->mVelY = 0.0f;
        aProjectile->mVelZ = aRangeY / 120.0f - 7.0f;
        aProjectile->mAccZ = 0.115f;
    } else if (mSeedType == SeedType::SEED_THREEPEATER) {
        if (theRow < mRow) {
            aProjectile->mMotionType = ProjectileMotion::MOTION_THREEPEATER;
            aProjectile->mVelY = -3.0f;
            aProjectile->mShadowY += 80.0f;
        } else if (theRow > mRow) {
            aProjectile->mMotionType = ProjectileMotion::MOTION_THREEPEATER;
            aProjectile->mVelY = 3.0f;
            aProjectile->mShadowY -= 80.0f;
        }
    } else if (mSeedType == SeedType::SEED_PUFFSHROOM || mSeedType == SeedType::SEED_SEASHROOM) {
        aProjectile->mMotionType = ProjectileMotion::MOTION_PUFF;
    } else if (mSeedType == SeedType::SEED_SPLITPEA && thePlantWeapon == PlantWeapon::WEAPON_SECONDARY) {
        aProjectile->mMotionType = ProjectileMotion::MOTION_BACKWARDS;
    } else if (mSeedType == SeedType::SEED_LEFTPEATER) {
        aProjectile->mMotionType = ProjectileMotion::MOTION_BACKWARDS;
    } else if (mSeedType == SeedType::SEED_CATTAIL) {
        aProjectile->mVelX = 2.0f;
        aProjectile->mMotionType = ProjectileMotion::MOTION_HOMING;
        aProjectile->mTargetZombieID = mBoard->ZombieGetID(theTargetZombie);
    } else if (mSeedType == SeedType::SEED_COBCANNON) {
        aProjectile->mVelX = 0.001f;
        aProjectile->mDamageRangeFlags = GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);
        aProjectile->mMotionType = ProjectileMotion::MOTION_LOBBED;
        aProjectile->mVelY = 0.0f;
        aProjectile->mAccZ = 0.0f;
        aProjectile->mVelZ = -8.0f;
        aProjectile->mCobTargetX = mTargetX - 40;
        aProjectile->mCobTargetRow = mBoard->PixelToGridYKeepOnBoard(mTargetX, mTargetY);
    }
}

Zombie *Plant::FindTargetZombie(int theRow, PlantWeapon thePlantWeapon) {
    int aDamageRangeFlags = GetDamageRangeFlags(thePlantWeapon);
    Rect aAttackRect = GetPlantAttackRect(thePlantWeapon);
    int aHighestWeight = 0;
    Zombie *aBestZombie = nullptr;

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if ((mSeedType == SeedType::SEED_POTATOMINE || mSeedType == SeedType::SEED_CHOMPER || IsSpiky()) && aZombie->IsFlying()) {
            continue; // 专门判定土豆雷、大嘴花、地刺无法攻击气球僵尸，以解决对战时地刺打气球的BUG
        }

        int aRowDeviation = aZombie->mRow - theRow;
        if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS) {
            aRowDeviation = 0;
        }

        if (!aZombie->mHasHead || aZombie->IsTangleKelpTarget()) {
            if (mSeedType == SeedType::SEED_POTATOMINE || mSeedType == SeedType::SEED_CHOMPER || mSeedType == SeedType::SEED_TANGLEKELP || mSeedType == SeedType::SEED_ICEBERG_LETTUCE
                || mSeedType == SeedType::SEED_CELERY_STALKER || mSeedType == SeedType::SEED_BONK_CHOY) {
                continue;
            }
        }

        bool needPortalCheck = false;
        if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_PORTAL_COMBAT) {
            if (mSeedType == SeedType::SEED_PEASHOOTER || mSeedType == SeedType::SEED_CACTUS || mSeedType == SeedType::SEED_REPEATER) {
                needPortalCheck = true;
            }
        }

        if (mSeedType != SeedType::SEED_CATTAIL) {
            if (mSeedType == SeedType::SEED_GLOOMSHROOM) {
                if (aRowDeviation < -1 || aRowDeviation > 1) {
                    continue;
                }
            } else if (needPortalCheck) {
                if (!mBoard->mChallenge->CanTargetZombieWithPortals(this, aZombie)) {
                    continue;
                }
            } else if (aRowDeviation) {
                continue;
            }
        }

        if (mSeedType == SeedType::SEED_ICEBERG_LETTUCE) {
            if ((!aZombie->CanBeFrozen() && aZombie->mZombieType != ZombieType::ZOMBIE_ZAMBONI) || aZombie->IsImmobilizied() || (aZombie->mZombieType == ZombieType::ZOMBIE_POGO && aZombie->mHasObject)
                || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT
                || aZombie->mZombiePhase == ZombiePhase::PHASE_SQUASH_RISING || aZombie->mZombiePhase == ZombiePhase::PHASE_SQUASH_FALLING
                || aZombie->mZombiePhase == ZombiePhase::PHASE_SQUASH_DONE_FALLING || aZombie->IsFlying() || aZombie->mZombieHeight != ZombieHeight::HEIGHT_ZOMBIE_NORMAL) {
                continue;
            }
            if (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE && aZombie->mTargetCol != mPlantCol) {
                continue;
            }
            if (aZombie->IsChangingRow()) {
                continue;
            }
        }

        if (aZombie->EffectedByDamage(aDamageRangeFlags)) {
            int aExtraRange = 0;

            if (mSeedType == SeedType::SEED_CHOMPER) {
                if (aZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_WALKING) {
                    aAttackRect.mX += 20;
                    aAttackRect.mWidth -= 20;
                }

                if (aZombie->mZombiePhase == ZombiePhase::PHASE_POGO_BOUNCING || (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE && aZombie->mTargetCol == mPlantCol)) {
                    continue;
                }

                if (aZombie->mIsEating || mState == PlantState::STATE_CHOMPER_BITING) {
                    aExtraRange = 60;
                }
            }

            if (mSeedType == SeedType::SEED_POTATOMINE) {
                if ((aZombie->mZombieType == ZombieType::ZOMBIE_POGO && aZombie->mHasObject) || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT
                    || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT) {
                    continue;
                }

                if (aZombie->mZombieType == ZombieType::ZOMBIE_POLEVAULTER) {
                    aAttackRect.mX += 40;
                    aAttackRect.mWidth -= 40; // 原版经典土豆地雷 Bug 及“四撑杆引雷”的原理
                }

                if (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE && aZombie->mTargetCol != mPlantCol) {
                    continue;
                }

                if (aZombie->mIsEating) {
                    aExtraRange = 30;
                }
            }

            if (mSeedType == SeedType::SEED_CELERY_STALKER) {
                if (aZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_WALKING || aZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_STUNNED) {
                    aAttackRect.mX += 25;
                }
                if (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE && aZombie->mTargetCol != mPlantCol - 1) {
                    continue; // 只能攻击左侧一格的蹦极僵尸
                }
            }

            if ((mSeedType == SeedType::SEED_EXPLODE_O_NUT && aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT) || (mSeedType == SeedType::SEED_TANGLEKELP && !aZombie->mInPool)) {
                continue;
            }

            Rect aZombieRect = aZombie->GetZombieRect();
            if (!needPortalCheck && GetRectOverlap(aAttackRect, aZombieRect) < -aExtraRange) {
                continue;
            }

            ////////////////////

            int aWeight = -aZombieRect.mX;
            if (mSeedType == SeedType::SEED_BONK_CHOY) {
                const float aPlantCenterX = static_cast<float>(mX) + 40.0f;
                const float aZombieLeft = static_cast<float>(aZombieRect.mX);
                const float aZombieRight = static_cast<float>(aZombieRect.mX + aZombieRect.mWidth);
                const float aNearestTargetX = std::clamp(aPlantCenterX, aZombieLeft, aZombieRight);
                const float aDistance = std::abs(aNearestTargetX - aPlantCenterX);
                const bool aZombieIsBehind = aZombieLeft + static_cast<float>(aZombieRect.mWidth) * 0.5f < aPlantCenterX;

                aWeight = -static_cast<int>(aDistance * 100.0f);
                if (!aZombieIsBehind) {
                    ++aWeight; // 距离相同时优先右侧。
                }
            } else if (mSeedType == SeedType::SEED_CATTAIL) {
                aWeight = -Distance2D(mX + 40.0f, mY + 40.0f, aZombieRect.mX + aZombieRect.mWidth / 2.0f, aZombieRect.mY + aZombieRect.mHeight / 2.0f);
                if (aZombie->IsFlying()) {
                    aWeight += 10000; // 优先攻击飞行单位
                }
            }

            if (mSeedType == SeedType::SEED_ICEBERG_LETTUCE) {
                if (aZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER) {
                    aWeight += 10000; // 优先攻击探险家僵尸
                }
            }

            if (aBestZombie == nullptr || aWeight > aHighestWeight) {
                aHighestWeight = aWeight;
                aBestZombie = aZombie;
            }
        }
    }

    return aBestZombie;
}

GridItem *Plant::FindTargetGridItem(int theRow, PlantWeapon thePlantWeapon) {
    // 对战模式专用，植物索敌僵尸墓碑和靶子僵尸。
    // 原版函数BUG：植物还会索敌梯子和毁灭菇弹坑，故重写以修复BUG。

    if (mSeedType == SEED_SPLITPEA && thePlantWeapon == WEAPON_SECONDARY) {
        // 如果是裂荚射手朝后的头，就直接不开火
        return nullptr;
    }

    if (mSeedType == SeedType::SEED_BONK_CHOY) {
        static constexpr int kBonkChoyGridItemRange = 2;

        if (!mApp->IsVSMode()) {
            return nullptr;
        }

        GridItem *aBestGridItem = nullptr;
        int aBestDistance = kBonkChoyGridItemRange + 1;
        bool aBestIsBehind = true;

        GridItem *aGridItem = nullptr;
        while (mBoard->IterateGridItems(aGridItem)) {
            if (aGridItem->mDead || aGridItem->mGridY != theRow) {
                continue;
            }

            int aGridItemHealth = 0;
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) {
                aGridItemHealth = aGridItem->mVSTargetZombieHealth;
            } else if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND) {
                aGridItemHealth = aGridItem->mVSGraveStoneHealth;
            } else {
                continue;
            }

            if (aGridItemHealth <= 0) {
                continue;
            }

            const int aDistance = std::abs(aGridItem->mGridX - mPlantCol);
            if (aDistance < 1 || aDistance > kBonkChoyGridItemRange) {
                continue;
            }

            const bool aGridItemIsBehind = aGridItem->mGridX < mPlantCol;
            if (aBestGridItem == nullptr || aDistance < aBestDistance || (aDistance == aBestDistance && !aGridItemIsBehind && aBestIsBehind)) {
                aBestGridItem = aGridItem;
                aBestDistance = aDistance;
                aBestIsBehind = aGridItemIsBehind;
            }
        }

        return aBestGridItem;
    }

    int aLastGridX = 0;
    GridItem *aBestGridItem = nullptr;

    GridItem *aGridItem = nullptr;
    if (mApp->IsVSMode()) { // 如果是对战模式
        int aRow = mStartRow;
        while (mBoard->IterateGridItems(aGridItem)) { // 遍历场上的所有GridItem
            int aGridX = aGridItem->mGridX;
            int aGridY = aGridItem->mGridY;
            GridItemType aGridItemType = aGridItem->mGridItemType;

            if (aGridItemType != GridItemType::GRIDITEM_GRAVESTONE && aGridItemType != GridItemType::GRIDITEM_MP_TARGET_ZOMBIE && aGridItemType != GridItemType::GRIDITEM_MP_BURIAL_MOUND) {
                // 修复植物们攻击核坑和梯子
                continue;
            }

            if (mSeedType == SeedType::SEED_THREEPEATER ? abs(aGridY - aRow) > 1 : aGridY != aRow) {
                // 如果是三线射手，则索敌三行; 反之，索敌一行
                continue;
            }

            if (aBestGridItem == nullptr || aGridX < aLastGridX) {
                if (mSeedType == SeedType::SEED_FUMESHROOM && aGridX - mPlantCol > 3) {
                    // 如果是大喷菇，则索敌三格以内的靶子或墓碑
                    continue;
                }
                if (mSeedType == SeedType::SEED_PUFFSHROOM || mSeedType == SeedType::SEED_SEASHROOM) {
                    //                    if (VSSetupAddonWidget::msBalancePatchMode && aGridX - mPlantCol > 2) {
                    //                        continue;
                    //                    }
                    // 如果是小喷菇或水兵菇，则索敌三格以内的墓碑
                    if (aGridX - mPlantCol > 3) {
                        continue;
                    }
                    // 不主动攻击靶子
                    if (aGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) {
                        continue;
                    }
                }

                aBestGridItem = aGridItem;
                aLastGridX = aGridX;
            }
        }
    }

    return aBestGridItem;
}

void Plant::Die() {
    if (mApp->IsVSMode() && mApp->mGameScene == SCENE_PLAYING) {
        if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
            return;

        if (gTcpClientSocket >= 0) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_DIE}, uint16_t(mBoard->mPlants.DataArrayGetID(this))};
            netplay::PutEvent(event);
            // 向日葵战损
            if (mSeedType == SEED_SUNFLOWER) {
                netplay::MetricsRecordSunflowerLoss();
            }
        }
    }

    Die_Origin();
}

void Plant::Die_Origin() {
    if (IsOnBoard() && mSeedType == SeedType::SEED_TANGLEKELP) {
        Zombie *aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
        if (aZombie) {
            aZombie->DieWithLoot();
        }
    }

    if (IsOnBoard() && mApp->IsVSMode()) {
        bool isSunFlower = (mSeedType == SEED_SUNFLOWER) || (mSeedType == SEED_TWINSUNFLOWER);
        if (isSunFlower && (Challenge::gVSResourseDropMode == 2 || Challenge::gVSResourseDropMode == 3) && Challenge::gVSResourceDropCount > 0) {
            for (int sunDropCount = 0; sunDropCount < Challenge::gVSResourceDropCount; ++sunDropCount) {
                mBoard->AddCoin(mX, mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
            }
        }
    }

    mDead = true;
    RemoveEffects();

    if (!Plant::IsFlying(mSeedType) && IsOnBoard()) {
        GridItem *aLadder = mBoard->GetLadderAt(mPlantCol, mRow);
        if (aLadder) {
            aLadder->GridItemDie();
        }
    }

    if (IsOnBoard()) {
        Plant *aTopPlant = mBoard->GetTopPlantAt(mPlantCol, mRow, PlantPriority::TOPPLANT_BUNGEE_ORDER);
        Plant *aFlowerPot = mBoard->GetFlowerPotAt(mPlantCol, mRow);
        if (aFlowerPot && aTopPlant == aFlowerPot) {
            Reanimation *aPotReanim = mApp->ReanimationGet(aFlowerPot->mBodyReanimID);
            aPotReanim->SetAnimRate(RandRangeFloat(10.0f, 15.0f));
        }
    }

    if (mApp->mPlayerInfo) {
        mApp->mPlayerInfo->mGameStats.ChangeMiscStat(GameStats::PLANTS_KILLED, 1);
    }
}

PlantDefinition &GetPlantDefinition(SeedType theSeedType) {
    if (theSeedType >= SeedType::SEED_ICEBERG_LETTUCE && theSeedType < SeedType::NUM_SEEDS_IN_CHOOSER_EXTENDED) {
        return gExtendedPlantDefs[theSeedType - SEED_ICEBERG_LETTUCE];
    }
    return gPlantDefs[theSeedType];
}

static int GetVSCostDefault(SeedType theSeedType) {
    switch (theSeedType) {
        case SeedType::SEED_INSTANT_COFFEE:
        case SeedType::SEED_ZOMBIE_NORMAL:
        case SeedType::SEED_ZOMBIE_IMP:
        case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
            return 25;
        case SeedType::SEED_ZOMBIE_GRAVESTONE:
        case SeedType::SEED_ZOMBIE_TRASHCAN:
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
        case SeedType::SEED_ZOMBIE_YETI:
        case SeedType::SEED_ZOMBIE_PEA_HEAD:
        case SeedType::SEED_ZOMBIE_SQUASH_HEAD:
            return 50;
        case SeedType::SEED_SQUASH:
        case SeedType::SEED_GARLIC:
        case SeedType::SEED_CELERY_STALKER:
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
        case SeedType::SEED_ZOMBIE_BOBSLED:
        case SeedType::SEED_ZOMBIE_BALLOON:
        case SeedType::SEED_ZOMBIE_MOUND:
            return 75;
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_SPORESHROOM:
        case SeedType::SEED_ZOMBIE_POLEVAULTER:
        case SeedType::SEED_ZOMBIE_PAIL:
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
        case SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX:
        case SeedType::SEED_ZOMBIE_WALLNUT_HEAD:
        case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
        case SeedType::SEED_ZOMBIE_EXPLORER:
        case SeedType::SEED_ZOMBIE_DOGWALKER:
            return 100;
        case SeedType::SEED_TORCHWOOD:
        case SeedType::SEED_BLOOMERANG:
        case SeedType::SEED_BONK_CHOY:
        case SeedType::SEED_ZOMBIE_BUNGEE:
        case SeedType::SEED_ZOMBIE_SNORKEL:
        case SeedType::SEED_ZOMBIE_DOLPHIN_RIDER:
        case SeedType::SEED_ZOMBIE_JALAPENO_HEAD:
            return 125;
        case SeedType::SEED_SNOWPEA:
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_ZOMBIE_FOOTBALL:
        case SeedType::SEED_ZOMBIE_DANCER:
        case SeedType::SEED_ZOMBIE_DIGGER:
        case SeedType::SEED_ZOMBIE_LADDER:
        case SeedType::SEED_ZOMBIE_GATLINGPEA_HEAD:
        case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
        case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
        case SeedType::SEED_ZOMBIE_JACKSON:
        case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
        case SeedType::SEED_ZOMBIE_ZOMBLOB:
            return 150;
        case SeedType::SEED_STARFRUIT:
        case SeedType::SEED_ZOMBONI:
            return 175;
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_ZOMBIE_CATAPULT:
            return 200;
        case SeedType::SEED_ZOMBIE_POGO:
            return 225;
        case SeedType::SEED_ZOMBIE_GARGANTUAR:
            return 250;
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_ZOMBIE_FLAG:
        case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
            return 300;
        default:
            return GetPlantDefinition(theSeedType).mSeedCost;
    }
}

static int GetVSRefreshTimeDefault(SeedType theSeedType) {
    if (Challenge::IsMPSeedType(theSeedType)) {
        switch (theSeedType) {
            case SeedType::SEED_ZOMBIE_TRASHCAN:
            case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
            case SeedType::SEED_ZOMBIE_POLEVAULTER:
            case SeedType::SEED_ZOMBIE_PAIL:
            case SeedType::SEED_ZOMBIE_FLAG:
            case SeedType::SEED_ZOMBIE_FOOTBALL:
            case SeedType::SEED_ZOMBIE_DANCER:
            case SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX:
            case SeedType::SEED_ZOMBIE_DIGGER:
            case SeedType::SEED_ZOMBIE_BUNGEE:
            case SeedType::SEED_ZOMBIE_LADDER:
            case SeedType::SEED_ZOMBIE_BOBSLED:
            case SeedType::SEED_ZOMBIE_YETI:
            case SeedType::SEED_ZOMBIE_IMP:
            case SeedType::SEED_ZOMBIE_BALLOON:
            case SeedType::SEED_ZOMBIE_WALLNUT_HEAD:
            case SeedType::SEED_ZOMBIE_JALAPENO_HEAD:
            case SeedType::SEED_ZOMBIE_GATLINGPEA_HEAD:
            case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
            case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
            case SeedType::SEED_ZOMBIE_JACKSON:
            case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
            case SeedType::SEED_ZOMBIE_EXPLORER:
            case SeedType::SEED_ZOMBIE_ZOMBLOB:
            case SeedType::SEED_ZOMBIE_DOGWALKER:
                return 3000;
            case SeedType::SEED_ZOMBIE_NEWSPAPER:
            case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
            case SeedType::SEED_ZOMBIE_SQUASH_HEAD:
            case SeedType::SEED_ZOMBIE_MOUND:
            case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
            case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
                return 1500;
            case SeedType::SEED_ZOMBONI:
            case SeedType::SEED_ZOMBIE_POGO:
            case SeedType::SEED_ZOMBIE_CATAPULT:
            case SeedType::SEED_ZOMBIE_GARGANTUAR:
            case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
                return 6000;
            default:
                return 750;
        }
    }

    switch (theSeedType) {
        case SeedType::SEED_CHERRYBOMB:
        case SeedType::SEED_ICESHROOM:
        case SeedType::SEED_DOOMSHROOM:
        case SeedType::SEED_JALAPENO:
        case SeedType::SEED_CHILLY_PEPPER:
            return 6000;
        case SeedType::SEED_GRAVEBUSTER:
        case SeedType::SEED_SQUASH:
            return 3000;
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_STARFRUIT:
        case SeedType::SEED_MELONPULT:
            return 1500;
        default:
            return GetPlantDefinition(theSeedType).mRefreshTime;
    }
}

static int GetVSCostBalanced(SeedType theSeedType) {
    int aCost = GetVSCostDefault(theSeedType);
    switch (theSeedType) {
        case SeedType::SEED_SUNSHROOM:
            aCost = 0;
            break;
        case SeedType::SEED_ICESHROOM:       // 75 -> 25
        case SeedType::SEED_ZOMBIE_TRASHCAN: // 50 -> 25
        case SeedType::SEED_ZOMBIE_SNORKEL:  // 125 -> 25
            aCost = 25;
            break;
        case SeedType::SEED_TANGLEKELP:          // 25 -> 50
        case SeedType::SEED_BLOVER:              // 100 -> 50
        case SeedType::SEED_PUMPKINSHELL:        // 125 -> 50
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE: // 75 -> 50
        case SeedType::SEED_ZOMBIE_BOBSLED:      // 75 -> 50
            aCost = 50;
            break;
        case SeedType::SEED_PEASHOOTER:           // 100 -> 75
        case SeedType::SEED_KERNELPULT:           // 100 -> 75
        case SeedType::SEED_SPIKEWEED:            // 100 -> 75
        case SeedType::SEED_UMBRELLA:             // 100 -> 75
        case SeedType::SEED_ZOMBIE_POLEVAULTER:   // 100 -> 75
        case SeedType::SEED_ZOMBIE_DOLPHIN_RIDER: // 125 -> 75
        case SeedType::SEED_ZOMBIE_EXPLORER:      // 100 -> 75
            aCost = 75;
            break;
        case SeedType::SEED_SQUASH:   // 75 -> 100 削弱窝瓜!!!
        case SeedType::SEED_TALLNUT:  // 125 -> 100
        case SeedType::SEED_SPLITPEA: // 125 -> 100
            aCost = 100;
            break;
        case SeedType::SEED_SNOWPEA:                 // 150 -> 125
        case SeedType::SEED_ZOMBIE_DIGGER:           // 150 -> 125
        case SeedType::SEED_ZOMBIE_LADDER:           // 150 -> 125
        case SeedType::SEED_ZOMBIE_CATAPULT:         // 200 -> 125
        case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER: // 150 -> 125
        case SeedType::SEED_ZOMBIE_ZOMBLOB:          // 150 -> 125
            aCost = 125;
            break;
        case SeedType::SEED_ZOMBONI: // 175 -> 125
            aCost = 125;
            break;
        case SeedType::SEED_DOOMSHROOM:  // 125 -> 175
        case SeedType::SEED_CACTUS:      // 100 -> 175
        case SeedType::SEED_ZOMBIE_POGO: // 225 -> 175
        case SeedType::SEED_ZOMBIE_FLAG: // 300 -> 175
            aCost = 175;
            break;
        case SeedType::SEED_MELONPULT: // 300 -> 200
            aCost = 200;
            break;
        case SeedType::SEED_ZOMBIE_GARGANTUAR: // 250 -> 225
            aCost = 225;
            break;
        case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR: // 300 -> 275
            aCost = 275;
            break;
        default:
            break;
    }

    if (gLawnApp->mBoard && gLawnApp->mBoard->StageIsNight() && Plant::IsNocturnal(theSeedType)) {
        aCost += 25;
    }

    return aCost;
}

static int GetVSCostShuffle(SeedType theSeedType) {
    int aCost = GetVSCostDefault(theSeedType);
    switch (theSeedType) {
        case SeedType::SEED_ICESHROOM: // 75 -> 50
            return 50;
        case SeedType::SEED_SQUASH: // 75 -> 100
        case SeedType::SEED_GARLIC: // 75 -> 100
            return 100;
        case SeedType::SEED_CHILLY_PEPPER: // 100 -> 125
        case SeedType::SEED_ZOMBIE_PAIL:   // 100 -> 125
        case SeedType::SEED_ZOMBIE_DIGGER: // 150 -> 125
            return 125;
        case SeedType::SEED_JALAPENO:        // 125 -> 150
        case SeedType::SEED_TORCHWOOD:       // 125 -> 150
        case SeedType::SEED_BLOOMERANG:      // 125 -> 150
        case SeedType::SEED_ZOMBIE_BUNGEE:   // 125 -> 150
        case SeedType::SEED_ZOMBIE_CATAPULT: // 200 -> 150
            return 150;
        case SeedType::SEED_CHERRYBOMB:      // 150 -> 175
        case SeedType::SEED_REPEATER:        // 150 -> 175
        case SeedType::SEED_DOOMSHROOM:      // 125 -> 175
        case SeedType::SEED_ZOMBIE_FOOTBALL: // 150 -> 175
            return 175;
        case SeedType::SEED_STARFRUIT: // 150 -> 200
        case SeedType::SEED_CACTUS:    // 100 -> 200
            return 200;
        case SeedType::SEED_THREEPEATER: // 200 -> 225
        case SeedType::SEED_MELONPULT:   // 300 -> 225
            return 225;
        case SeedType::SEED_ZOMBIE_FLAG: // 300 -> 250
            return 250;
        default:
            break;
    }
    return aCost;
}

static int GetVSRefreshTimeBalanced(SeedType theSeedType) {
    int aRefreshTime = GetVSRefreshTimeDefault(theSeedType);
    switch (theSeedType) {
        case SeedType::SEED_SNOWPEA:                // 7.5 -> 15
        case SeedType::SEED_REPEATER:               // 7.5 -> 15
        case SeedType::SEED_PUFFSHROOM:             // 7.5 -> 15
        case SeedType::SEED_CACTUS:                 // 7.5 -> 15
        case SeedType::SEED_SPLITPEA:               // 7.5 -> 15
        case SeedType::SEED_ZOMBIE_NORMAL:          // 7.5 -> 15
        case SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX: // 30 -> 15
        case SeedType::SEED_ZOMBIE_SNORKEL:         // 7.5 -> 15
            return 1500;
        case SeedType::SEED_TORCHWOOD:            // 7.5 -> 30
        case SeedType::SEED_SPIKEWEED:            // 7.5 -> 30
        case SeedType::SEED_UMBRELLA:             // 7.5 -> 30
        case SeedType::SEED_ZOMBIE_DOLPHIN_RIDER: // 7.5 -> 30
            return 3000;
        case SeedType::SEED_PUMPKINSHELL: // 30 -> 60
            return 6000;
        default:
            return aRefreshTime;
    }
}

static int GetVSRefreshTimeShuffle(SeedType theSeedType) {
    int aRefreshTime = GetVSRefreshTimeDefault(theSeedType);
    switch (theSeedType) {
            //        case SeedType::SEED_SNOWPEA:  // 7.5 -> 15
            //        case SeedType::SEED_REPEATER: // 7.5 -> 15
            //        return 1500;
        default:
            break;
    }
    return aRefreshTime;
}

int Plant::GetCost(SeedType theSeedType, SeedType theImitaterType) {
    if (gLawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || gLawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
        if (theSeedType == SeedType::SEED_REPEATER) {
            return 1000;
        } else if (theSeedType == SeedType::SEED_FUMESHROOM) {
            return 500;
        } else if (theSeedType == SeedType::SEED_TALLNUT) {
            return 250;
        } else if (theSeedType == SeedType::SEED_BEGHOULED_BUTTON_SHUFFLE) {
            return 100;
        } else if (theSeedType == SeedType::SEED_BEGHOULED_BUTTON_CRATER) {
            return 200;
        }
    }
    if (gLawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON) {
        if (theSeedType == SeedType::SEED_REPEATER) {
            return 200;
        } else if (theSeedType == SeedType::SEED_THREEPEATER) {
            return 400;
        } else if (theSeedType == SeedType::SEED_CATTAIL) {
            return 200;
        } else if (theSeedType == SeedType::SEED_SNOWPEA) {
            return 100;
        } else if (theSeedType == SeedType::SEED_TORCHWOOD) {
            return 125;
        } else if (theSeedType == SeedType::SEED_DOOMSHROOM) {
            return 200;
        }
    }
    if (gLawnApp->IsVSMode()) {
        if (theSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE) {
            if (gOpeningEncounter && gOpeningEncounter->mType == EncounterType::ENCOUNTER_FREE_SHUFFLE) {
                return 0;
            }
            if (gFreeForFristShuffle[0]) {
                return 0;
            }
            return 25;
        } else if (theSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
            if (gOpeningEncounter && gOpeningEncounter->mType == EncounterType::ENCOUNTER_FREE_SHUFFLE) {
                return 0;
            }
            if (gFreeForFristShuffle[1]) {
                return 0;
            }
            return 25;
        }

        if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) {
            theSeedType = theImitaterType;
        }
        if (Challenge::msVSShuffleMode) {
            return GetVSCostShuffle(theSeedType);
        }
        return VSSetupAddonWidget::msBalancePatchMode ? GetVSCostBalanced(theSeedType) : GetVSCostDefault(theSeedType);
    } else {
        switch (theSeedType) {
            case SeedType::SEED_SLOT_MACHINE_SUN:
                return 0;
            case SeedType::SEED_SLOT_MACHINE_DIAMOND:
                return 0;
            case SeedType::SEED_ZOMBIQUARIUM_SNORKLE:
                return 100;
            case SeedType::SEED_ZOMBIQUARIUM_TROPHY:
                return 1000;
            case SeedType::SEED_ZOMBIE_NORMAL:
                return 50;
            case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
                return 75;
            case SeedType::SEED_ZOMBIE_POLEVAULTER:
                return 75;
            case SeedType::SEED_ZOMBIE_PAIL:
                return 125;
            case SeedType::SEED_ZOMBIE_LADDER:
                return 150;
            case SeedType::SEED_ZOMBIE_DIGGER:
                return 125;
            case SeedType::SEED_ZOMBIE_BUNGEE:
                return 125;
            case SeedType::SEED_ZOMBIE_FOOTBALL:
                return 175;
            case SeedType::SEED_ZOMBIE_BALLOON:
                return 150;
            case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
                return 100;
            case SeedType::SEED_ZOMBONI:
                return 175;
            case SeedType::SEED_ZOMBIE_POGO:
                return 200;
            case SeedType::SEED_ZOMBIE_DANCER:
                return 350;
            case SeedType::SEED_ZOMBIE_GARGANTUAR:
                return 300;
            case SeedType::SEED_ZOMBIE_IMP:
                return 50;
            default: {
                if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) {
                    const PlantDefinition &aPlantDef = GetPlantDefinition(theImitaterType);
                    return aPlantDef.mSeedCost;
                } else {
                    const PlantDefinition &aPlantDef = GetPlantDefinition(theSeedType);
                    return aPlantDef.mSeedCost;
                }
            }
        }
    }
}

int Plant::GetRefreshTime(SeedType theSeedType, SeedType theImitaterType) {
    if (seedPacketFastCoolDown && !IsOnlineServerModeActive() && !gIsReplayMode) {
        return 0;
    }

    if (gLawnApp->IsVSMode()) {
        if (theSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE || theSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
            if (gOpeningEncounter && gOpeningEncounter->mType == EncounterType::ENCOUNTER_QUICK_SHUFFLE) {
                return 0;
            }
            return 3000;
        }

        if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) {
            theSeedType = theImitaterType;
        }
        int aRefreshTime =
            Challenge::msVSShuffleMode ? GetVSRefreshTimeShuffle(theSeedType) : (VSSetupAddonWidget::msBalancePatchMode ? GetVSRefreshTimeBalanced(theSeedType) : GetVSRefreshTimeDefault(theSeedType));
        if (gLawnApp->mBoard->mChallenge->IsMPSuddenDeath() && Challenge::gVSSuddenDeathMode == 1) {
            // sd 不减冷却的卡片
            switch (theSeedType) {
                // 墓碑和向日葵，sd 用不到
                case SeedType::SEED_ZOMBIE_GRAVESTONE:
                case SeedType::SEED_SUNFLOWER:
                // 默认五个不减 cd 的
                case SeedType::SEED_TALLNUT:
                case SeedType::SEED_WALLNUT:
                case SeedType::SEED_PUMPKINSHELL:
                case SeedType::SEED_ZOMBIE_TRASHCAN:
                case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
                // 新增不减 cd
                case SeedType::SEED_SWEET_POTATO:
                    return aRefreshTime;
                // 平衡调整后 cd 减幅下降
                case SeedType::SEED_POTATOMINE:
                case SeedType::SEED_SQUASH:
                case SeedType::SEED_CHERRYBOMB:
                case SeedType::SEED_JALAPENO:
                case SeedType::SEED_DOOMSHROOM:
                case SeedType::SEED_ICESHROOM:
                case SeedType::SEED_ICEBERG_LETTUCE:
                case SeedType::SEED_CHILLY_PEPPER:
                    if (Challenge::msVSShuffleMode || VSSetupAddonWidget::msBalancePatchMode) {
                        return aRefreshTime / 2;
                    }
                    [[fallthrough]];
                default:
                    return aRefreshTime / 3;
            }
        }
        return aRefreshTime;
    }
    return old_Plant_GetRefreshTime(theSeedType, theImitaterType);
}

bool Plant::IsNocturnal(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_PUFFSHROOM || theSeedType == SeedType::SEED_SEASHROOM || theSeedType == SeedType::SEED_SUNSHROOM || theSeedType == SeedType::SEED_FUMESHROOM
        || theSeedType == SeedType::SEED_HYPNOSHROOM || theSeedType == SeedType::SEED_DOOMSHROOM || theSeedType == SeedType::SEED_ICESHROOM || theSeedType == SeedType::SEED_MAGNETSHROOM
        || theSeedType == SeedType::SEED_SCAREDYSHROOM || theSeedType == SeedType::SEED_GLOOMSHROOM || theSeedType == SeedType::SEED_SPORESHROOM;
}

bool Plant::IsAquatic(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_LILYPAD || theSeedType == SeedType::SEED_TANGLEKELP || theSeedType == SeedType::SEED_SEASHROOM || theSeedType == SeedType::SEED_CATTAIL;
}

bool Plant::IsFlying(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_INSTANT_COFFEE;
}

bool Plant::IsLobber(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_CABBAGEPULT || theSeedType == SeedType::SEED_KERNELPULT || theSeedType == SeedType::SEED_MELONPULT || theSeedType == SeedType::SEED_WINTERMELON
        || theSeedType == SeedType::SEED_SPORESHROOM;
}

bool Plant::IsUpgrade(SeedType theSeedType) {
    // 修复机枪射手在SeedBank光标移动到shop栏后变为绿卡。
    if (theSeedType == SeedType::SEED_GATLINGPEA) {
        LawnApp *lawnApp = gLawnApp;
        Board *board = lawnApp->mBoard;
        if (board == nullptr) {
            return old_Plant_IsUpgrade(theSeedType); // 等价于直接return true;但方便改版修改所以返回旧函数
        }
        if (lawnApp->mSeedChooserScreen != nullptr) {
            return true;
        }
        GamepadControls *gamePad = board->mGamepadControls[0];
        return !(gamePad->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR && gamePad->mIsInShopSeedBank);
    }
    return old_Plant_IsUpgrade(theSeedType);
}

bool Plant::IsDefender(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_WALLNUT || theSeedType == SeedType::SEED_TALLNUT || theSeedType == SeedType::SEED_PUMPKINSHELL || theSeedType == SeedType::SEED_SWEET_POTATO;
}

Rect Plant::GetPlantRect() {
    Rect aRect;
    if (mSeedType == SeedType::SEED_TALLNUT || mSeedType == SeedType::SEED_CELERY_STALKER) {
        aRect = Rect(mX + 10, mY, mWidth, mHeight);
    } else if (mSeedType == SeedType::SEED_PUMPKINSHELL) {
        aRect = Rect(mX, mY, mWidth - 20, mHeight);
    } else if (mSeedType == SeedType::SEED_COBCANNON) {
        aRect = Rect(mX, mY, 140, 80);
    } else {
        aRect = Rect(mX + 10, mY, mWidth - 20, mHeight);
    }

    return aRect;
}

Rect Plant::GetPlantAttackRect(PlantWeapon thePlantWeapon) {
    Rect aRect;
    if (mApp->IsWallnutBowlingLevel()) {
        aRect = Rect(mX, mY, mWidth - 20, mHeight);
    } else if (thePlantWeapon == PlantWeapon::WEAPON_SECONDARY && mSeedType == SeedType::SEED_SPLITPEA) {
        aRect = Rect(0, mY, mX + 16, mHeight);
    } else
        switch (mSeedType) {
            case SeedType::SEED_LEFTPEATER:
                aRect = Rect(0, mY, mX, mHeight);
                break;
            case SeedType::SEED_SQUASH:
                aRect = Rect(mX + 20, mY, mWidth - 35, mHeight);
                break;
            case SeedType::SEED_CHOMPER:
                aRect = Rect(mX + 80, mY, 40, mHeight);
                break;
            case SeedType::SEED_SPIKEWEED:
            case SeedType::SEED_SPIKEROCK:
                aRect = Rect(mX + 20, mY, mWidth - 50, mHeight);
                break;
            case SeedType::SEED_POTATOMINE:
                aRect = Rect(mX, mY, mWidth - 25, mHeight);
                break;
            case SeedType::SEED_TORCHWOOD:
                aRect = Rect(mX + 50, mY, 30, mHeight);
                break;
            case SeedType::SEED_PUFFSHROOM:
            case SeedType::SEED_SEASHROOM:
                aRect = Rect(mX + 60, mY, 230, mHeight);
                break;
            case SeedType::SEED_FUMESHROOM:
                aRect = Rect(mX + 60, mY, 340, mHeight);
                break;
            case SeedType::SEED_GLOOMSHROOM:
                aRect = Rect(mX - 80, mY - 80, 240, 240);
                break;
            case SeedType::SEED_TANGLEKELP:
                aRect = Rect(mX, mY, mWidth, mHeight);
                break;
            case SeedType::SEED_CATTAIL:
                aRect = Rect(-BOARD_WIDTH, -BOARD_HEIGHT, BOARD_WIDTH * 2, BOARD_HEIGHT * 2);
                break;
            case SeedType::SEED_ICEBERG_LETTUCE:
                aRect = Rect(mX, mY, mWidth, mHeight);
                break;
            case SeedType::SEED_CELERY_STALKER:
                aRect = Rect(mX - 80, mY, 70, mHeight);
                break;
            case SeedType::SEED_BONK_CHOY:
                aRect = Rect(mX - 95, mY, 270, mHeight);
                break;
            default:
                aRect = Rect(mX + 60, mY, BOARD_WIDTH, mHeight);
                break;
        }

    return aRect;
}

Image *Plant::GetImage(SeedType theSeedType) {
    Image **aImages = GetPlantDefinition(theSeedType).mPlantImage;
    return aImages ? aImages[0] : nullptr;
}

pvzstl::string Plant::GetNameString(SeedType theSeedType, SeedType theImitaterType) {
    const PlantDefinition &aPlantDef = GetPlantDefinition(theSeedType);
    pvzstl::string aName = StrFormat("[%s]", aPlantDef.mPlantName);
    pvzstl::string aTranslatedName = TodStringTranslate(aName.c_str());

    if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) {
        const PlantDefinition &aImitaterDef = GetPlantDefinition(theImitaterType);
        pvzstl::string aImitaterName = StrFormat("[%s]", aImitaterDef.mPlantName);
        pvzstl::string aTranslatedImitaterName = TodStringTranslate(aImitaterName.c_str());
        pvzstl::string aTranslatedTemplate = TodStringTranslate("[IMITATED_PLANT]");
        return TodReplaceString(aTranslatedTemplate, "{PLANT}", aTranslatedImitaterName);
    }

    return aTranslatedName;
}

pvzstl::string Plant::GetToolTip(SeedType theSeedType) {
    if (theSeedType == SeedType::SEED_CACTUS && VSSetupAddonWidget::msBalancePatchMode) {
        return TodStringTranslate("[CACTUS_TOOLTIP_VS]");
    }

    const PlantDefinition &aPlantDef = GetPlantDefinition(theSeedType);
    pvzstl::string aToolTip = StrFormat("[%s_TOOLTIP]", aPlantDef.mPlantName);
    return TodStringTranslate(aToolTip.c_str());
}

void Plant::SetImitaterFilterEffect() {
    FilterEffect aFilterEffect = GetFilterEffectTypeBySeedType(mSeedType);
    Reanimation *mBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (mBodyReanim != nullptr)
        mBodyReanim->mFilterEffect = aFilterEffect;
    Reanimation *mHeadReanim = mApp->ReanimationTryToGet(mHeadReanimID);
    if (mHeadReanim != nullptr)
        mHeadReanim->mFilterEffect = aFilterEffect;
    Reanimation *mHeadReanim2 = mApp->ReanimationTryToGet(mHeadReanimID2);
    if (mHeadReanim2 != nullptr)
        mHeadReanim2->mFilterEffect = aFilterEffect;
    Reanimation *mHeadReanim3 = mApp->ReanimationTryToGet(mHeadReanimID3);
    if (mHeadReanim3 != nullptr)
        mHeadReanim3->mFilterEffect = aFilterEffect;
}

bool Plant::DrawMagnetItemsOnTop() {
    if (mSeedType == SeedType::SEED_GOLD_MAGNET) {
        for (auto &mMagnetItem : mMagnetItems) {
            if (mMagnetItem.mItemType != MagnetItemType::MAGNET_ITEM_NONE) {
                return true;
            }
        }

        return false;
    }

    if (mSeedType == SeedType::SEED_MAGNETSHROOM) {
        for (auto &mMagnetItem : mMagnetItems) {
            MagnetItem *aMagnetItem = &mMagnetItem;
            if (aMagnetItem->mItemType != MagnetItemType::MAGNET_ITEM_NONE) {
                SexyVector2 aVectorToPlant(mX + aMagnetItem->mDestOffsetX - aMagnetItem->mPosX, mY + aMagnetItem->mDestOffsetY - aMagnetItem->mPosY);
                if (aVectorToPlant.Magnitude() > 20.0f) {
                    return true;
                }
            }
        }

        return false;
    }

    return false;
}

void Plant::BurnRow(int theRow) {
    int aDamageRangeFlags = GetDamageRangeFlags(PlantWeapon::WEAPON_PRIMARY);

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if ((aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow == theRow) && aZombie->EffectedByDamage(aDamageRangeFlags)) {
            aZombie->RemoveColdEffects();
            aZombie->ApplyBurn();
        }
        if (aZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && !aZombie->mHasObject && aZombie->mMindControlled) {
            aZombie->ExplorerTorchConvert(true);
        }
    }

    GridItem *aGridItem = nullptr;
    while (mBoard->IterateGridItems(aGridItem)) {
        if (aGridItem->mGridY == theRow && (aGridItem->mGridItemType == GridItemType::GRIDITEM_LADDER || aGridItem->mGridItemType == GridItemType::GRIDITEM_POLE)) {
            aGridItem->GridItemDie();
        }
    }

    Zombie *aBossZombie = mBoard->GetBossZombie();
    if (aBossZombie && aBossZombie->mFireballRow == theRow) {
        // 注：原版中将 Zombie::BossDestroyIceballInRow(int) 函数改为了 Zombie::BossDestroyIceball()，冰球是否位于目标行的判断则移动至此处进行
        aBossZombie->BossDestroyIceballInRow(theRow);
    }
}

void Plant::BlowAwayFliers(int theX, int theRow) {
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (!aZombie->IsDeadOrDying()) {
            //            Rect aZombieRect = aZombie->GetZombieRect(); // 原版遗留无用法代码
            if (aZombie->IsFlying() || aZombie->IsImpFlying() /* 新增粉丝小鬼被吹飞 */) {
                aZombie->mBlowingAway = true;
            }
        }
    }

    mApp->PlaySample(SOUND_BLOVER);
    mBoard->mFogBlownCountDown = 4000;
}

bool Plant::MakesSun() const {
    return mSeedType == SeedType::SEED_SUNFLOWER || mSeedType == SeedType::SEED_TWINSUNFLOWER || mSeedType == SeedType::SEED_SUNSHROOM;
}

void Plant::UpdateProductionPlant() {
    if (mApp->mGameMode == GAMEMODE_MP_VS && (gTcpConnected || gTcpClientSocket >= 0 || gIsServerModeSpectator || gIsReplayMode)) {
        if (!IsInPlay()) {
            return;
        }
        if (mApp->IsIZombieLevel() || mApp->mGameMode == GameMode::GAMEMODE_UPSELL || mApp->mGameMode == GameMode::GAMEMODE_INTRO) {
            return;
        }
        if (mBoard->HasLevelAwardDropped()) {
            return;
        }
        if (mSeedType == SeedType::SEED_MARIGOLD && mBoard->mCurrentWave == mBoard->mNumWaves) {
            if (mState != PlantState::STATE_MARIGOLD_ENDING) {
                mState = PlantState::STATE_MARIGOLD_ENDING;
                mStateCountdown = 6000;
            } else if (mStateCountdown <= 0) {
                return;
            }
        }
        if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && mBoard->mChallenge->mChallengeState != ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT) {
            return;
        }
        // mLaunchCounter -= 3;
        mLaunchCounter--;
        if (mLaunchCounter <= 100) {
            int num = TodAnimateCurve(100, 0, mLaunchCounter, 0, 100, TodCurves::CURVE_LINEAR);
            mEatenFlashCountdown = mEatenFlashCountdown > num ? mEatenFlashCountdown : num;
        }
        if (mLaunchCounter <= 0)
        // 生产
        {
            if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode) {
                return;
            }
            mLaunchCounter = RandRangeInt(mLaunchRate - 150, mLaunchRate);
            if ((mSeedType == SeedType::SEED_SUNFLOWER || mSeedType == SeedType::SEED_SUNSHROOM) && vsai::HasEnhancedAIProduction(mBoard, vsai::VSSide::Plants)) {
                mLaunchCounter = vsai::ScaleEnhancedAIProductionCooldown(mLaunchCounter);
            }
            if (gTcpClientSocket >= 0) {
                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_LAUNCHCOUNTER}, uint16_t(mBoard->mPlants.DataArrayGetID(this)), uint16_t(mLaunchCounter)};
                netplay::PutEvent(event);
            }
            mApp->PlayFoley(FoleyType::FOLEY_SPAWN_SUN);
            if (mSeedType == SeedType::SEED_SUNSHROOM) {
                if (mState == PlantState::STATE_SUNSHROOM_SMALL) {
                    mBoard->AddCoin(mX, mY, CoinType::COIN_SMALLSUN, CoinMotion::COIN_MOTION_FROM_PLANT);
                } else {
                    mBoard->AddCoin(mX, mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
                }
            } else if (mSeedType == SeedType::SEED_SUNFLOWER) {
                mBoard->AddCoin(mX, mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
            } else if (mSeedType == SeedType::SEED_TWINSUNFLOWER) {
                mBoard->AddCoin(mX, mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
                mBoard->AddCoin(mX, mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
            } else if (mSeedType == SeedType::SEED_MARIGOLD) {
                int num2 = Sexy::Rand(100);
                CoinType theCoinType = CoinType::COIN_SILVER;
                if (num2 < 10) {
                    theCoinType = CoinType::COIN_GOLD;
                }
                mBoard->AddCoin(mX, mY, theCoinType, CoinMotion::COIN_MOTION_COIN);
            }
            if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BIG_TIME) {
                if (mSeedType == SeedType::SEED_SUNFLOWER) {
                    mBoard->AddCoin(mX, mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
                    return;
                }
                if (mSeedType == SeedType::SEED_MARIGOLD) {
                    mBoard->AddCoin(mX, mY, CoinType::COIN_SILVER, CoinMotion::COIN_MOTION_COIN);
                }
            }
        }
        return;
    }

    const bool enhancedProducer = mApp->IsVSMode() && (mSeedType == SeedType::SEED_SUNFLOWER || mSeedType == SeedType::SEED_SUNSHROOM) && vsai::HasEnhancedAIProduction(mBoard, vsai::VSSide::Plants);
    if (!enhancedProducer) {
        old_Plant_UpdateProductionPlant(this);
        return;
    }

    // Local VS reaches the original producer path, not the custom network
    // path above. Scale the freshly reset counter after that path produces,
    // so every Sunflower/Sun-shroom cycle is 0.7x rather than only its first.
    const int counterBeforeUpdate = mLaunchCounter;
    old_Plant_UpdateProductionPlant(this);
    if (counterBeforeUpdate <= 1 && mLaunchCounter > 0) {
        mLaunchCounter = vsai::ScaleEnhancedAIProductionCooldown(mLaunchCounter);
    }
}

void Plant::UpdateShooting() {
    if (NotOnGround() || mShootingCounter == 0)
        return;

    mShootingCounter--;

    if (mSeedType == SeedType::SEED_FUMESHROOM && mShootingCounter == 15) {
        int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, mRow, 0);
        AddAttachedParticle(mX + 85, mY + 31, aRenderPosition, ParticleEffect::PARTICLE_FUMECLOUD);
    }

    if (mSeedType == SeedType::SEED_GLOOMSHROOM) {
        if (mShootingCounter == 136 || mShootingCounter == 108 || mShootingCounter == 80 || mShootingCounter == 52) {
            int aRenderPosition = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, mRow, 0);
            AddAttachedParticle(mX + 40, mY + 40, aRenderPosition, ParticleEffect::PARTICLE_GLOOMCLOUD);
        }
        if (mShootingCounter == 126 || mShootingCounter == 98 || mShootingCounter == 70 || mShootingCounter == 42) {
            Fire(nullptr, mRow, PlantWeapon::WEAPON_PRIMARY, nullptr);
        }
    } else if (mSeedType == SeedType::SEED_GATLINGPEA) {
        if (mShootingCounter == 18 || mShootingCounter == 35 || mShootingCounter == 51 || mShootingCounter == 68) {
            Fire(nullptr, mRow, PlantWeapon::WEAPON_PRIMARY, nullptr);
        }
    } else if (mSeedType == SeedType::SEED_CATTAIL) {
        if (mShootingCounter == 19) {
            Zombie *aZombie = FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY);
            GridItem *aGridItem = FindTargetGridItem(mRow, PlantWeapon::WEAPON_PRIMARY);
            if (aZombie) {
                Fire(aZombie, mRow, PlantWeapon::WEAPON_PRIMARY, aGridItem);
            }
        }
    } else if (mShootingCounter == 1) {
        if (mSeedType == SeedType::SEED_THREEPEATER) {
            int rowAbove = mRow - 1;
            int rowBelow = mRow + 1;
            Reanimation *aHeadReanim2 = mApp->ReanimationTryToGet(mHeadReanimID2);
            Reanimation *aHeadReanim3 = mApp->ReanimationTryToGet(mHeadReanimID3);
            Reanimation *aHeadReanim1 = mApp->ReanimationTryToGet(mHeadReanimID);

            if (aHeadReanim1->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                Fire(nullptr, rowBelow, PlantWeapon::WEAPON_PRIMARY, nullptr);
            }
            if (aHeadReanim2->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                Fire(nullptr, mRow, PlantWeapon::WEAPON_PRIMARY, nullptr);
            }
            if (aHeadReanim3->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                Fire(nullptr, rowAbove, PlantWeapon::WEAPON_PRIMARY, nullptr);
            }
        } else if (mSeedType == SeedType::SEED_SPLITPEA) {
            Reanimation *aHeadBackReanim = mApp->ReanimationTryToGet(mHeadReanimID2);
            Reanimation *aHeadFrontReanim = mApp->ReanimationTryToGet(mHeadReanimID);
            if (aHeadFrontReanim->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                Fire(nullptr, mRow, PlantWeapon::WEAPON_PRIMARY, nullptr);
            }
            if (aHeadBackReanim->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                Fire(nullptr, mRow, PlantWeapon::WEAPON_SECONDARY, nullptr);
            }
        } else if (mState == PlantState::STATE_CACTUS_LOW) {
            Fire(nullptr, mRow, PlantWeapon::WEAPON_SECONDARY, nullptr);
        } else if (mSeedType == SeedType::SEED_CABBAGEPULT || mSeedType == SeedType::SEED_KERNELPULT || mSeedType == SeedType::SEED_MELONPULT || mSeedType == SeedType::SEED_WINTERMELON
                   || mSeedType == SeedType::SEED_SPORESHROOM) {
            PlantWeapon aPlantWeapon = PlantWeapon::WEAPON_PRIMARY;
            if (mState == PlantState::STATE_KERNELPULT_BUTTER) {
                Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
                aBodyReanim->AssignRenderGroupToPrefix("Cornpult_butter", RENDER_GROUP_HIDDEN);
                aBodyReanim->AssignRenderGroupToPrefix("Cornpult_kernal", RENDER_GROUP_NORMAL);
                mState = PlantState::STATE_NOTREADY;
                aPlantWeapon = PlantWeapon::WEAPON_SECONDARY;
            }

            Zombie *aZombie = FindTargetZombie(mRow, aPlantWeapon);
            GridItem *aGridItem = FindTargetGridItem(mRow, aPlantWeapon);
            Fire(aZombie, mRow, aPlantWeapon, aGridItem);
        } else {
            Fire(nullptr, mRow, PlantWeapon::WEAPON_PRIMARY, nullptr);
        }

        return;
    }

    if (mShootingCounter != 0)
        return;

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mHeadReanimID);
    if (mSeedType == SeedType::SEED_THREEPEATER) {
        Reanimation *aHeadReanim2 = mApp->ReanimationTryToGet(mHeadReanimID2);
        Reanimation *aHeadReanim3 = mApp->ReanimationTryToGet(mHeadReanimID3);

        if (aHeadReanim2->mLoopCount > 0) {
            if (aHeadReanim->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                aHeadReanim->StartBlend(20);
                aHeadReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
                aHeadReanim->SetFramesForLayer("anim_head_idle1");
                aHeadReanim->SetAnimRate(aBodyReanim->mAnimRate);
                aHeadReanim->mAnimTime = aBodyReanim->mAnimTime;
            }

            aHeadReanim2->StartBlend(20);
            aHeadReanim2->mLoopType = ReanimLoopType::REANIM_LOOP;
            aHeadReanim2->SetFramesForLayer("anim_head_idle2");
            aHeadReanim2->SetAnimRate(aBodyReanim->mAnimRate);
            aHeadReanim2->mAnimTime = aBodyReanim->mAnimTime;

            if (aHeadReanim3->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD) {
                aHeadReanim3->StartBlend(20);
                aHeadReanim3->mLoopType = ReanimLoopType::REANIM_LOOP;
                aHeadReanim3->SetFramesForLayer("anim_head_idle3");
                aHeadReanim3->SetAnimRate(aBodyReanim->mAnimRate);
                aHeadReanim3->mAnimTime = aBodyReanim->mAnimTime;
            }

            return;
        }
    } else if (mSeedType == SeedType::SEED_SPLITPEA) {
        Reanimation *aHeadReanim2 = mApp->ReanimationGet(mHeadReanimID2);

        if (aHeadReanim->mLoopCount > 0) {
            aHeadReanim->StartBlend(20);
            aHeadReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
            aHeadReanim->SetFramesForLayer("anim_head_idle");
            aHeadReanim->SetAnimRate(aBodyReanim->mAnimRate);
            aHeadReanim->mAnimTime = aBodyReanim->mAnimTime;
        }

        if (aHeadReanim2->mLoopCount > 0) {
            aHeadReanim2->StartBlend(20);
            aHeadReanim2->mLoopType = ReanimLoopType::REANIM_LOOP;
            aHeadReanim2->SetFramesForLayer("anim_splitpea_idle");
            aHeadReanim2->SetAnimRate(aBodyReanim->mAnimRate);
            aHeadReanim2->mAnimTime = aBodyReanim->mAnimTime;
        }

        return;
    } else if (mState == PlantState::STATE_CACTUS_HIGH) {
        if (aBodyReanim->mLoopCount > 0) {
            PlayBodyReanim("anim_idlehigh", ReanimLoopType::REANIM_LOOP, 20, 0.0f);

            aBodyReanim->SetAnimRate(aBodyReanim->mDefinition->mFPS);
            if (mApp->IsIZombieLevel()) {
                aBodyReanim->SetAnimRate(0.0f);
            }

            return;
        }
    } else if (aHeadReanim) {
        if (aHeadReanim->mLoopCount > 0) {
            aHeadReanim->StartBlend(20);
            aHeadReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
            aHeadReanim->SetFramesForLayer("anim_head_idle");
            aHeadReanim->SetAnimRate(aBodyReanim->mAnimRate);
            aHeadReanim->mAnimTime = aBodyReanim->mAnimTime;
            return;
        }
    } else if (mSeedType == SeedType::SEED_COBCANNON) {
        if (aBodyReanim->mLoopCount > 0) {
            mState = PlantState::STATE_COBCANNON_ARMING;
            mStateCountdown = 3000;
            PlayBodyReanim("anim_unarmed_idle", ReanimLoopType::REANIM_LOOP, 20, aBodyReanim->mDefinition->mFPS);
            return;
        }
    } else if (aBodyReanim && aBodyReanim->mLoopCount > 0) {
        PlayIdleAnim(aBodyReanim->mDefinition->mFPS);
        return;
    }

    mShootingCounter = 1;
}

void Plant::UpdateShooter() {

    if (mApp->mGameMode == GAMEMODE_MP_VS && (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)) {
        return;
    }

    mLaunchCounter--;
    if (mSeedType == SeedType::SEED_BLOOMERANG
        && (HasActiveBoomerang() || mState == PlantState::STATE_BLOOMERANG_CATCHING || mState == PlantState::STATE_UMBRELLA_TRIGGERED || mState == PlantState::STATE_UMBRELLA_REFLECTING)) {
        return;
    }
    if (mLaunchCounter <= 0) {
        mLaunchCounter = mLaunchRate - Sexy::Rand(15);
        if (gTcpClientSocket >= 0) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_SHOOTER_LAUNCH}, uint16_t(mBoard->mPlants.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
        if (mSeedType == SeedType::SEED_THREEPEATER) {
            LaunchThreepeater();
        } else if (mSeedType == SeedType::SEED_STARFRUIT) {
            LaunchStarFruit();
        } else if (mSeedType == SeedType::SEED_SPLITPEA) {
            FindTargetAndFire(mRow, PlantWeapon::WEAPON_SECONDARY);
            Reanimation *aHeadReanim = mApp->ReanimationGet(mHeadReanimID);
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            aHeadReanim->StartBlend(20);
            aHeadReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
            aHeadReanim->SetFramesForLayer("anim_head_idle");
            aHeadReanim->SetAnimRate(aBodyReanim->mAnimRate);
            aHeadReanim->mAnimTime = aBodyReanim->mAnimTime;
        } else if (mSeedType == SeedType::SEED_CACTUS) {
            if (mState == PlantState::STATE_CACTUS_HIGH) {
                FindTargetAndFire(mRow, PlantWeapon::WEAPON_PRIMARY);
            } else if (mState == PlantState::STATE_CACTUS_LOW) {
                FindTargetAndFire(mRow, PlantWeapon::WEAPON_SECONDARY);
            }
        } else {
            FindTargetAndFire(mRow, PlantWeapon::WEAPON_PRIMARY);
        }
    }

    if (mLaunchCounter == 50 && mSeedType == SeedType::SEED_CATTAIL) {
        FindTargetAndFire(mRow, PlantWeapon::WEAPON_PRIMARY);
    }
    if (mLaunchCounter == 25) {
        if (mSeedType == SeedType::SEED_REPEATER || mSeedType == SeedType::SEED_LEFTPEATER) {
            FindTargetAndFire(mRow, PlantWeapon::WEAPON_PRIMARY);
        } else if (mSeedType == SeedType::SEED_SPLITPEA) {
            FindTargetAndFire(mRow, PlantWeapon::WEAPON_PRIMARY);
            FindTargetAndFire(mRow, PlantWeapon::WEAPON_SECONDARY);
        }
    }
}

void Plant::PlayIdleAnim(float theRate) {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim) {
        PlayBodyReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, theRate);
        if (mApp->IsIZombieLevel()) {
            aBodyReanim->SetAnimRate(0.0f);
        }
    }
}

void Plant::IceZombies() {
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        aZombie->HitIceTrap();
    }

    mBoard->mIceTrapCounter = 300;
    TodParticleSystem *aPoolSparklyParticle = mApp->ParticleTryToGet(mBoard->mPoolSparklyParticleID);
    if (aPoolSparklyParticle) {
        aPoolSparklyParticle->mDontUpdate = false;
    }

    Zombie *aBossZombie = nullptr;
    while (mBoard->IterateZombies(aBossZombie)) {
        if (aBossZombie->mZombieType == ZOMBIE_BOSS) {
            aBossZombie->BossDestroyFireball();
        }
    }
}

bool Plant::IsDisposable(SeedType theSeedType) {
    return theSeedType == SeedType::SEED_CHERRYBOMB || theSeedType == SeedType::SEED_JALAPENO || theSeedType == SeedType::SEED_HYPNOSHROOM || theSeedType == SeedType::SEED_ICESHROOM
        || theSeedType == SeedType::SEED_CHILLY_PEPPER;
}

bool Plant::IsInvulnerable() {
    if (mSeedType == SeedType::SEED_CHERRYBOMB || mSeedType == SeedType::SEED_ICESHROOM || mSeedType == SeedType::SEED_DOOMSHROOM || mSeedType == SeedType::SEED_JALAPENO
        || mSeedType == SeedType::SEED_BLOVER || mSeedType == SeedType::SEED_ICEBERG_LETTUCE || mSeedType == SeedType::SEED_CHILLY_PEPPER || mState == PlantState::STATE_SQUASH_LOOK
        || mState == PlantState::STATE_SQUASH_PRE_LAUNCH) {
        if (!mIsAsleep) {
            return true;
        }
    }
    return false;
}


ReanimationID Plant::GetPlantReanimationIDByIndex(int index) const {
    switch (index) {
        case 0:
            return mBodyReanimID;
        case 1:
            return mHeadReanimID;
        case 2:
            return mHeadReanimID2;
        case 3:
            return mHeadReanimID3;
        case 4:
            return mBlinkReanimID;
        case 5:
            return mLightReanimID;
        case 6:
            return mSleepingReanimID;
        default:
            return REANIMATIONID_NULL;
    }
}


void Plant::SyncPingPongAnimationToClient() {
    uint16_t id = mBoard->mPlants.DataArrayGetID(this);

    U16U16U16UNI32UNI32_Event event{};
    event.type = EventType::EVENT_SERVER_BOARD_PLANT_PINGPONG_ANIMATION;
    event.data1 = id;
    event.data2 = mFrameLength;
    event.data3 = mAnimPing;
    event.data4.u32 = mFrame;
    event.data5.u32 = mAnimCounter;

    netplay::PutEvent(event);
}

void Plant::SyncAnimationToClient() {

    uint16_t id = mBoard->mPlants.DataArrayGetID(this);

    for (int i = 0; i < 7; ++i) {

        Reanimation *theReanim = mApp->ReanimationTryToGet(GetPlantReanimationIDByIndex(i));
        if (theReanim == nullptr) {
            continue;
        }

        U16U16U16UNI32UNI32_Event event2{};
        event2.type = EventType::EVENT_SERVER_BOARD_PLANT_OTHER_ANIMATION;
        event2.data1 = id;
        event2.data2 = i;
        event2.data3 = theReanim->mLoopType;
        event2.data4.f32 = theReanim->mAnimTime;
        event2.data5.f32 = theReanim->mAnimRate;
        netplay::PutEvent(event2);
    }
}


bool Plant::FindTargetAndFire(int theRow, PlantWeapon thePlantWeapon) {
    // 此函数用于在mLaunchCounter到0之后播放投手的投掷动画、豌豆的发射动画
    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode) {
        return false;
    }

    bool result;
    if (mSeedType == SeedType::SEED_BLOOMERANG) {
        if (HasActiveBoomerang() || mState == PlantState::STATE_BLOOMERANG_CATCHING || mState == PlantState::STATE_UMBRELLA_TRIGGERED || mState == PlantState::STATE_UMBRELLA_REFLECTING) {
            return false;
        }

        Zombie *aZombie = FindTargetZombie(theRow, thePlantWeapon);
        GridItem *aGridItem = FindTargetGridItem(theRow, thePlantWeapon);
        if (aZombie == nullptr && aGridItem == nullptr) {
            return false;
        }

        mApp->PlayFoley(FoleyType::FOLEY_BLOOMERANG);
        PlayBodyReanim("anim_shooting", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
        mShootingCounter = 65;
        result = true;
    } else {
        result = old_Plant_FindTargetAndFire(this, theRow, thePlantWeapon);
    }

    if (result) {
        if (gTcpClientSocket >= 0) {

            if (mSeedType == SEED_KERNELPULT) {
                U8U8U16U16_Event event = {
                    {EventType::EVENT_SERVER_BOARD_PLANT_KERNELPLUT_FINDTARGETANDFIRE}, uint8_t(theRow), uint8_t(thePlantWeapon), uint16_t(mBoard->mPlants.DataArrayGetID(this)), uint16_t(mState)};
                netplay::PutEvent(event);
            } else {
                U8U8U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_FINDTARGETANDFIRE}, uint8_t(theRow), uint8_t(thePlantWeapon), uint16_t(mBoard->mPlants.DataArrayGetID(this))};
                netplay::PutEvent(event);
            }
        }
    }


    return result;
}

void Plant::UpdateGraveBuster() {
    if (mState == PlantState::STATE_GRAVEBUSTER_LANDING) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim && aBodyReanim->mLoopCount > 0) {
            PlayBodyReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 10, 12.0f);
            mStateCountdown = 400;
            mState = PlantState::STATE_GRAVEBUSTER_EATING;
            AddAttachedParticle(mX + 40, mY + 40, mRenderOrder + 4, ParticleEffect::PARTICLE_GRAVE_BUSTER);
        }
    } else if (mState == PlantState::STATE_GRAVEBUSTER_EATING && mStateCountdown == 0) {
        GridItem *aGraveStone = mBoard->GetGraveStoneAt(mPlantCol, mRow);
        if (aGraveStone) {
            aGraveStone->GridItemDie();
            mBoard->mGravesCleared++;
        } else {
            GridItem *aMound = mBoard->GetMoundAt(mPlantCol, mRow);
            if (aMound) {
                aMound->TakeDamage(420, 0U);
                if (aMound->mVSGraveStoneHealth <= 0) {
                    mBoard->mGravesCleared++;
                }
            }
        }

        mApp->AddTodParticle(mX + 40, mY + 80, mRenderOrder + 4, ParticleEffect::PARTICLE_GRAVE_BUSTER_DIE);
        Die();
        if (!mApp->IsVSMode()) {
            mBoard->DropLootPiece(mX + 40, mY, 12);
        }
    }
}

void Plant::UpdateChomper() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (mState == PlantState::STATE_READY) {
        if (FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY)) {
            PlayBodyReanim("anim_bite", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            mState = PlantState::STATE_CHOMPER_BITING;
            mStateCountdown = 70;
        }
    } else if (mState == PlantState::STATE_CHOMPER_BITING) {
        if (mStateCountdown == 0) {
            mApp->PlayFoley(FoleyType::FOLEY_BIGCHOMP);

            Zombie *aZombie = FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY);
            bool doBite = false;
            if (aZombie) {
                if (aZombie->mZombieType == ZombieType::ZOMBIE_GARGANTUAR || aZombie->mZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR || aZombie->mZombieType == ZombieType::ZOMBIE_BOSS
                    || aZombie->mZombieType == ZombieType::ZOMBIE_ZOMBLOB_SMALL || aZombie->mZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR) {
                    doBite = true;
                }
            }
            bool doMiss = false;
            if (aZombie == nullptr) {
                doMiss = true;
            } else if (!aZombie->IsImmobilizied()) {
                if (aZombie->IsBouncingPogo() || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT) {
                    doMiss = true;
                }
            }

            if (doBite) {
                mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
                aZombie->TakeDamage(40, 0U);
                mState = PlantState::STATE_CHOMPER_BITING_MISSED;
            } else if (doMiss) {
                mState = PlantState::STATE_CHOMPER_BITING_MISSED;
            } else {
                if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
                    return;

                if (gTcpClientSocket >= 0) {
                    U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_CHOMPER_BIT}, uint16_t(mBoard->mPlants.DataArrayGetID(this)), uint16_t(mBoard->mZombies.DataArrayGetID(aZombie))};
                    netplay::PutEvent(event);
                }

                aZombie->DieWithLoot();
                mState = PlantState::STATE_CHOMPER_BITING_GOT_ONE;
            }
        }
    } else if (mState == PlantState::STATE_CHOMPER_BITING_GOT_ONE) {
        if (aBodyReanim->mLoopCount > 0) {
            PlayBodyReanim("anim_chew", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
            if (mApp->IsIZombieLevel()) {
                aBodyReanim->SetAnimRate(0.0f);
            }

            mState = PlantState::STATE_CHOMPER_DIGESTING;
            mStateCountdown = 4000;
        }
    } else if (mState == PlantState::STATE_CHOMPER_DIGESTING) {
        if (mStateCountdown == 0) {
            PlayBodyReanim("anim_swallow", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0f);
            mState = PlantState::STATE_CHOMPER_SWALLOWING;
        }
    } else if ((mState == PlantState::STATE_CHOMPER_SWALLOWING || mState == PlantState::STATE_CHOMPER_BITING_MISSED) && aBodyReanim->mLoopCount > 0) {
        PlayIdleAnim(aBodyReanim->mDefinition->mFPS);
        mState = PlantState::STATE_READY;
    }
}

void Plant::UpdateMagnetShroom() {
    for (auto &mMagnetItem : mMagnetItems) {
        MagnetItem *aMagnetItem = &mMagnetItem;
        if (aMagnetItem->mItemType != MagnetItemType::MAGNET_ITEM_NONE) {
            SexyVector2 aVectorToPlant(mX + aMagnetItem->mDestOffsetX - aMagnetItem->mPosX, mY + aMagnetItem->mDestOffsetY - aMagnetItem->mPosY);
            if (aVectorToPlant.Magnitude() > 20.0f) {
                aMagnetItem->mPosX += aVectorToPlant.x * 0.05f;
                aMagnetItem->mPosY += aVectorToPlant.y * 0.05f;
            }
        }
    }

    if (mState == PlantState::STATE_MAGNETSHROOM_CHARGING) {
        if (mStateCountdown == 0) {
            mState = PlantState::STATE_READY;

            float aAnimRate = RandRangeFloat(10.0f, 15.0f);
            PlayBodyReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 30, aAnimRate);
            if (mApp->IsIZombieLevel()) {
                Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
                aBodyReanim->SetAnimRate(0.0f);
            }

            mMagnetItems[0].mItemType = MagnetItemType::MAGNET_ITEM_NONE;
        }
    } else if (mState == PlantState::STATE_MAGNETSHROOM_SUCKING) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            PlayBodyReanim("anim_nonactive_idle2", ReanimLoopType::REANIM_LOOP, 20, 2.0f);
            if (mApp->IsIZombieLevel()) {
                aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
                aBodyReanim->mAnimRate = 0.0f;
            }

            mState = PlantState::STATE_MAGNETSHROOM_CHARGING;
        }
    } else {
        float aClosestDistance = 0.0f;
        Zombie *aClosestZombie = nullptr;

        Zombie *aZombie = nullptr;
        while (mBoard->IterateZombies(aZombie)) {
            int aDiffY = aZombie->mRow - mRow;
            Rect aZombieRect = aZombie->GetZombieRect();

            if (aZombie->mMindControlled)
                continue;

            if (!aZombie->mHasHead)
                continue;

            if (aZombie->mZombieHeight != ZombieHeight::HEIGHT_ZOMBIE_NORMAL || aZombie->mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE)
                continue;

            if (aZombie->IsDeadOrDying())
                continue;

            if (aZombieRect.mX > BOARD_WIDTH || aDiffY > 2 || aDiffY < -2)
                continue;

            if (aZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING || aZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_STUNNED || aZombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_WALKING
                || aZombie->mZombieType == ZombieType::ZOMBIE_POGO) {
                if (!aZombie->mHasObject)
                    continue;
            } else if (!(aZombie->mHelmType == HelmType::HELMTYPE_PAIL || aZombie->mHelmType == HelmType::HELMTYPE_FOOTBALL || aZombie->mShieldType == ShieldType::SHIELDTYPE_DOOR
                         || aZombie->mShieldType == ShieldType::SHIELDTYPE_LADDER || aZombie->mShieldType == ShieldType::SHIELDTYPE_TRASHCAN
                         || aZombie->mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING))
                continue;

            int aRadius = aZombie->mIsEating ? 320 : 270;
            if (GetCircleRectOverlap(mX, mY + 20, aRadius, aZombieRect)) {
                float aDistance = Distance2D(mX, mY, aZombieRect.mX, aZombieRect.mY);
                aDistance += abs(aDiffY) * 80.0f;

                if (aClosestZombie == nullptr || aDistance < aClosestDistance) {
                    aClosestZombie = aZombie;
                    aClosestDistance = aDistance;
                }
            }
        }

        if (aClosestZombie) {
            if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
                return;

            if (gTcpClientSocket >= 0) {
                U16U16_Event event = {
                    {EventType::EVENT_SERVER_BOARD_PLANT_MAGNETSHROOM_ATTACK}, uint16_t(mBoard->mPlants.DataArrayGetID(this)), uint16_t(mBoard->mZombies.DataArrayGetID(aClosestZombie))};
                netplay::PutEvent(event);
            }

            MagnetShroomAttactItem(aClosestZombie);
            return;
        }

        ////////////////////

        float aClosestLadderDist = 0.0f;
        GridItem *aClosestLadder = nullptr;

        GridItem *aGridItem = nullptr;
        while (mBoard->IterateGridItems(aGridItem)) {
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_LADDER) {
                int aDiffX = abs(aGridItem->mGridX - mPlantCol);
                int aDiffY = abs(aGridItem->mGridY - mRow);
                int aSquareDistance = std::max(aDiffX, aDiffY);
                if (aSquareDistance <= 2) {
                    float aDistance = aSquareDistance + aDiffY * 0.05f;
                    if (aClosestLadder == nullptr || aDistance < aClosestLadderDist) {
                        aClosestLadder = aGridItem;
                        aClosestLadderDist = aDistance;
                    }
                }
            }
        }

        if (aClosestLadder) {
            if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
                return;

            if (gTcpClientSocket >= 0) {
                U16U16_Event event = {
                    {EventType::EVENT_SERVER_BOARD_PLANT_MAGNETSHROOM_ATTACK_LADDER}, uint16_t(mBoard->mPlants.DataArrayGetID(this)), uint16_t(mBoard->mGridItems.DataArrayGetID(aClosestLadder))};
                netplay::PutEvent(event);
            }

            mState = PlantState::STATE_MAGNETSHROOM_SUCKING;
            mStateCountdown = 1500;
            PlayBodyReanim("anim_shooting", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0f);
            mApp->PlayFoley(FoleyType::FOLEY_MAGNETSHROOM);

            aClosestLadder->GridItemDie();

            MagnetItem *aMagnetItem = GetFreeMagnetItem();
            aMagnetItem->mPosX = mBoard->GridToPixelX(aClosestLadder->mGridX, aClosestLadder->mGridY) + 40;
            aMagnetItem->mPosY = mBoard->GridToPixelY(aClosestLadder->mGridX, aClosestLadder->mGridY);
            aMagnetItem->mDestOffsetX = RandRangeFloat(-10.0f, 10.0f) + 10.0f;
            aMagnetItem->mDestOffsetY = RandRangeFloat(-10.0f, 10.0f);
            aMagnetItem->mItemType = MagnetItemType::MAGNET_ITEM_LADDER_PLACED;
        }
    }
}

void Plant::UpdateSquash() {
    // mApp->ReanimationTryToGet(mBodyReanimID); // disassembled code
    bool isRemoteClient = mApp->IsVSMode() && (gTcpConnected || gIsServerModeSpectator || gIsReplayMode);
    auto syncSquashState = [this]() {
        if (gTcpClientSocket < 0) {
            return;
        }
        U16U16I16I16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_PLANT_SQUASH_STATE;
        event.data1 = uint16_t(mBoard->mPlants.DataArrayGetID(this));
        event.data2 = uint16_t(mState);
        event.data3 = int16_t(mStateCountdown);
        event.data4 = int16_t(mTargetX);
        netplay::PutEvent(event);
    };

    if (mState == PlantState::STATE_NOTREADY) {
        if (isRemoteClient) {
            return;
        }
        Zombie *aZombie = FindSquashTarget();
        if (aZombie) {
            mTargetZombieID = mBoard->ZombieGetID(aZombie);
            mTargetX = aZombie->ZombieTargetLeadX(0.0f) - mWidth / 2;
            mState = PlantState::STATE_SQUASH_LOOK;
            mStateCountdown = 80;
            PlayBodyReanim(mTargetX < mX ? "anim_lookleft" : "anim_lookright", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
            mApp->PlayFoley(FoleyType::FOLEY_SQUASH_HMM);
            syncSquashState();
        }
    } else if (mState == PlantState::STATE_SQUASH_LOOK) {
        if (isRemoteClient) {
            return;
        }
        if (mStateCountdown <= 0) {
            PlayBodyReanim("anim_jumpup", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            mState = PlantState::STATE_SQUASH_PRE_LAUNCH;
            mStateCountdown = 45;
            syncSquashState();
        }
    } else if (mState == PlantState::STATE_SQUASH_PRE_LAUNCH) {
        if (mStateCountdown == 1) {
            TriggerVibration(VibrationEffect::VIBRATION_JUMP); // 这窝瓜有力气!!
        }
        if (isRemoteClient) {
            return;
        }
        if (mStateCountdown <= 0) {
            Zombie *aZombie = FindSquashTarget();
            if (aZombie) {
                mTargetX = aZombie->ZombieTargetLeadX(30.0f) - mWidth / 2;
            }

            mState = PlantState::STATE_SQUASH_RISING;
            mStateCountdown = 50;
            mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, mRow, 0);
            syncSquashState();
        }
    } else {
        int aTargetCol = mBoard->PixelToGridXKeepOnBoard(mTargetX, mY);
        int aDestY = mBoard->GridToPixelY(aTargetCol, mRow) + 8;

        if (mState == PlantState::STATE_SQUASH_RISING) {
            mX = TodAnimateCurve(50, 20, mStateCountdown, mBoard->GridToPixelX(mPlantCol, mStartRow), mTargetX, TodCurves::CURVE_EASE_IN_OUT);
            mY = TodAnimateCurve(50, 20, mStateCountdown, mBoard->GridToPixelY(mPlantCol, mStartRow), aDestY - 120, TodCurves::CURVE_EASE_IN_OUT);

            if (mStateCountdown == 0) {
                PlayBodyReanim("anim_jumpdown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 60.0f);
                mState = PlantState::STATE_SQUASH_FALLING;
                mStateCountdown = 10;
                syncSquashState();
            }
        } else if (mState == PlantState::STATE_SQUASH_FALLING) {
            mY = TodAnimateCurve(10, 0, mStateCountdown, aDestY - 120, aDestY, TodCurves::CURVE_LINEAR);

            if (mStateCountdown == 5) {
                DoSquashDamage();
            }

            if (mStateCountdown == 0) {
                if (mBoard->IsPoolSquare(aTargetCol, mRow)) {
                    mApp->AddReanimation(mX - 11, mY + 20, mRenderOrder + 1, ReanimationType::REANIM_SPLASH);
                    mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
                    mApp->PlaySample(SOUND_ZOMBIESPLASH);

                    Die();
                } else {
                    mState = PlantState::STATE_SQUASH_DONE_FALLING;
                    mStateCountdown = 100;

                    mBoard->ShakeBoard(1, 4);
                    mApp->PlayFoley(FoleyType::FOLEY_THUMP);
                    float aOffsetY = mBoard->StageHasRoof() ? 69.0f : 80.0f;
                    mApp->AddTodParticle(mX + 40, mY + aOffsetY, mRenderOrder + 4, ParticleEffect::PARTICLE_DUST_SQUASH);
                }
            }
        } else if (mState == PlantState::STATE_SQUASH_DONE_FALLING) {
            if (mStateCountdown == 0) {
                Die();
            }
        }
    }
}

void Plant::UpdateIcebergLettuce() {
    bool isRemoteClient = mApp->IsVSMode() && (gTcpConnected || gIsServerModeSpectator || gIsReplayMode);
    auto syncIcebergLettuceState = [this]() {
        if (gTcpClientSocket < 0) {
            return;
        }
        U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_PLANT_ICEBERG_LETTUCE_LAUNCH;
        event.data = uint16_t(mBoard->mPlants.DataArrayGetID(this));
        netplay::PutEvent(event);
    };

    if (mState == PlantState::STATE_NOTREADY) {
        if (isRemoteClient) {
            return;
        }
        if (FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY)) {
            mApp->PlayFoley(FoleyType::FOLEY_ICEBERG);
            PlayBodyReanim("anim_explode", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
            mState = PlantState::STATE_READY;
            mStateCountdown = 50;
            syncIcebergLettuceState();
        }
    } else if (mState == PlantState::STATE_READY) {
        if (mStateCountdown <= 0) {
            DoSpecial();
        }
    }
}

void Plant::UpdateCeleryStalker() {
    static constexpr int kCeleryAttackDamage = 20;
    static constexpr int kCeleryHitInterval = 50;
    static constexpr int kCeleryRetreatDelay = 300;

    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
    Zombie *aZombie = FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY);

    if (mState == PlantState::STATE_CELERY_STALKER_LOWERING) {
        if (aBodyReanim->mLoopCount > 0) {
            mState = PlantState::STATE_CELERY_STALKER_LOW;
            PlayBodyReanim("anim_idlelow", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
            if (mApp->IsIZombieLevel()) {
                aBodyReanim->mAnimRate = 0.0f;
            }
        }
        return;
    }

    if (mState == PlantState::STATE_CELERY_STALKER_LOW) {
        if (aZombie != nullptr) {
            mState = PlantState::STATE_CELERY_STALKER_RISING;
            PlayBodyReanim("anim_rise", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, aBodyReanim->mDefinition->mFPS);
            mApp->PlayFoley(FoleyType::FOLEY_CELERY_STALKER_RISE);
        }
        return;
    }

    if (mState == PlantState::STATE_CELERY_STALKER_RISING) {
        if (aBodyReanim->mLoopCount > 0) {
            if (aZombie != nullptr) {
                mState = PlantState::STATE_CELERY_STALKER_PUNCHING;
                mLaunchCounter = 0;
                mStateCountdown = 0;
                PlayBodyReanim("anim_punch", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
            } else {
                mState = PlantState::STATE_CELERY_STALKER_HIGH;
                mLaunchCounter = 0;
                mStateCountdown = 0;
                PlayBodyReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
                if (mApp->IsIZombieLevel()) {
                    aBodyReanim->mAnimRate = 0.0f;
                }
            }
        }
        return;
    }

    if (mState == PlantState::STATE_CELERY_STALKER_PUNCHING) {
        if (aZombie == nullptr) {
            mState = PlantState::STATE_CELERY_STALKER_STOPPING;
            PlayBodyReanim("anim_stop", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
            return;
        }

        if (aBodyReanim->mLoopCount > 0) {
            mState = PlantState::STATE_CELERY_STALKER_ATTACKING;
            mLaunchCounter = 0;
            PlayBodyReanim("anim_attack", ReanimLoopType::REANIM_LOOP, 0, 18.0f);
        }
        return;
    }

    if (mState == PlantState::STATE_CELERY_STALKER_ATTACKING) {
        if (aZombie == nullptr) {
            mState = PlantState::STATE_CELERY_STALKER_STOPPING;
            PlayBodyReanim("anim_stop", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
            return;
        }

        if (mLaunchCounter <= 0) {
            unsigned int aDamageFlags = 0U;
            SetBit(aDamageFlags, int(DamageFlags::DAMAGE_BYPASSES_SHIELD), true);
            aZombie->TakeDamage(kCeleryAttackDamage, aDamageFlags);
            mApp->PlayFoley(FoleyType::FOLEY_CELERY_STALKER_ATTACK);
            mLaunchCounter = kCeleryHitInterval;
        } else {
            mLaunchCounter--;
        }
        return;
    }

    if (mState == PlantState::STATE_CELERY_STALKER_STOPPING) {
        if (aBodyReanim->mLoopCount > 0) {
            mState = PlantState::STATE_CELERY_STALKER_HIGH;
            mLaunchCounter = 0;
            mStateCountdown = kCeleryRetreatDelay;
            PlayBodyReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
            if (mApp->IsIZombieLevel()) {
                aBodyReanim->mAnimRate = 0.0f;
            }
        }
        return;
    }

    if (mState == PlantState::STATE_CELERY_STALKER_HIGH) {
        if (aZombie == nullptr) {
            if (mStateCountdown == 1) {
                mState = PlantState::STATE_CELERY_STALKER_LOWERING;
                PlayBodyReanim("anim_lower", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, aBodyReanim->mDefinition->mFPS);
            } else if (mStateCountdown <= 0) {
                mStateCountdown = kCeleryRetreatDelay;
            }
            return;
        }

        mStateCountdown = 0;
        mState = PlantState::STATE_CELERY_STALKER_PUNCHING;
        mLaunchCounter = 0;
        PlayBodyReanim("anim_punch", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
        return;
    }

    if (aZombie != nullptr) {
        mState = PlantState::STATE_CELERY_STALKER_HIGH;
        mLaunchCounter = 0;
        mStateCountdown = 0;
        PlayBodyReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
        if (mApp->IsIZombieLevel()) {
            aBodyReanim->mAnimRate = 0.0f;
        }
    } else {
        mState = PlantState::STATE_CELERY_STALKER_LOWERING;
        PlayBodyReanim("anim_lower", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, aBodyReanim->mDefinition->mFPS);
    }
}

void Plant::UpdateBonkChoy() {
    static constexpr int kBonkChoyGridItemRange = 2;
    static constexpr int kBonkChoyAttackInterval = 40;
    static constexpr int kBonkChoyPunchDamage = 15;
    static constexpr int kBonkChoyUppercutDamage = 35;

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr) {
        return;
    }

    if (mLaunchCounter > 0) {
        --mLaunchCounter;
    }

    const bool anAttackAnimationPlaying = mState == PlantState::STATE_BONK_CHOY_PUNCHING || mState == PlantState::STATE_BONK_CHOY_UPPERCUTTING;
    if (anAttackAnimationPlaying) {
        if (aBodyReanim->mLoopCount > 0) {
            PlayIdleAnim(12.0f);
            mState = PlantState::STATE_NOTREADY;
        }
        return;
    }

    if (mLaunchCounter > 0) {
        return;
    }

    const float aPlantCenterX = static_cast<float>(mX) + 40.0f;
    Zombie *aBestZombie = FindTargetZombie(mRow, PlantWeapon::WEAPON_PRIMARY);

    GridItem *aBestGridItem = aBestZombie == nullptr ? FindTargetGridItem(mRow, PlantWeapon::WEAPON_PRIMARY) : nullptr;
    const int aBestGridItemDistance = aBestGridItem != nullptr ? std::abs(aBestGridItem->mGridX - mPlantCol) : kBonkChoyGridItemRange + 1;
    bool aBestTargetIsBehind = false;

    if (aBestZombie != nullptr) {
        const Rect aZombieRect = aBestZombie->GetZombieRect();
        const auto aZombieLeft = static_cast<float>(aZombieRect.mX);

        aBestTargetIsBehind = aZombieLeft + static_cast<float>(aZombieRect.mWidth) * 0.5f < aPlantCenterX;
    } else if (aBestGridItem != nullptr) {
        aBestTargetIsBehind = aBestGridItem->mGridX < mPlantCol;
    }

    if (aBestZombie == nullptr && aBestGridItem == nullptr) {
        return;
    }

    if (aBestGridItem != nullptr) {
        const int aGridItemHealth = aBestGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE ? aBestGridItem->mVSTargetZombieHealth : aBestGridItem->mVSGraveStoneHealth;
        const bool aUseUppercut = aBestGridItemDistance == 1 && aGridItemHealth > 0 && aGridItemHealth <= kBonkChoyUppercutDamage;

        const char *anAttackTrack = aUseUppercut ? "anim_rightuppercut" : "anim_rightpunch";
        const float aAnimRate = aUseUppercut ? 30.0f : 45.0f;
        FoleyType aFoleyType = aUseUppercut ? FoleyType::FOLEY_BONK_CHOY_UPPERCUT : FoleyType::FOLEY_BONK_CHOY_PUNCH;

        // 左侧攻击也播放右侧动画，并在 Draw() 中水平镜像。
        mTargetX = aBestTargetIsBehind ? -1 : 1;
        PlayBodyReanim(anAttackTrack, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, aAnimRate);

        mState = aUseUppercut ? PlantState::STATE_BONK_CHOY_UPPERCUTTING : PlantState::STATE_BONK_CHOY_PUNCHING;
        mLaunchCounter = kBonkChoyAttackInterval;

        aBestGridItem->TakeDamage(aUseUppercut ? aGridItemHealth : kBonkChoyPunchDamage, 0U);
        mApp->PlayFoley(aFoleyType);
        return;
    }

    unsigned int aDamageFlags = 0U;
    if (aBestTargetIsBehind) {
        SetBit(aDamageFlags, int(DamageFlags::DAMAGE_BYPASSES_SHIELD), true);
    }

    const bool aShieldBlocksBody = !aBestTargetIsBehind && aBestZombie->mShieldType != ShieldType::SHIELDTYPE_NONE && aBestZombie->mShieldHealth > 0;
    const bool aHelmBlocksBody = aBestZombie->mHelmType != HelmType::HELMTYPE_NONE && aBestZombie->mHelmHealth > 0;
    const bool aDamageReachesBody = !aShieldBlocksBody && !aHelmBlocksBody;

    bool aUseUppercut = false;
    bool anUppercutExecutesTarget = false;
    if (aDamageReachesBody) {
        if (aBestZombie->CanLoseBodyParts() && aBestZombie->mHasHead && aBestZombie->mBodyMaxHealth > 0) {
            const int aHeadDropThreshold = aBestZombie->mBodyMaxHealth / 3;
            const int aDamageNeededToDropHead = aBestZombie->mBodyHealth - aHeadDropThreshold + 1;
            if (aDamageNeededToDropHead > 0 && aDamageNeededToDropHead <= kBonkChoyUppercutDamage) {
                aUseUppercut = true;
                anUppercutExecutesTarget = true;
            }
        } else if (!aBestZombie->CanLoseBodyParts() && aBestZombie->mBodyHealth <= kBonkChoyUppercutDamage) {
            aUseUppercut = true;
        }
    }

    const char *anAttackTrack = aUseUppercut ? "anim_rightuppercut" : "anim_rightpunch";
    const float aAnimRate = aUseUppercut ? 30.0f : 45.0f;
    FoleyType aFoleyType = aUseUppercut ? FoleyType::FOLEY_BONK_CHOY_UPPERCUT : FoleyType::FOLEY_BONK_CHOY_PUNCH;

    mTargetX = aBestTargetIsBehind ? -1 : 1;
    PlayBodyReanim(anAttackTrack, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, aAnimRate);

    mState = aUseUppercut ? PlantState::STATE_BONK_CHOY_UPPERCUTTING : PlantState::STATE_BONK_CHOY_PUNCHING;
    mLaunchCounter = kBonkChoyAttackInterval;

    if (anUppercutExecutesTarget) {
        aBestZombie->TakeDamage(aBestZombie->mBodyHealth, aDamageFlags);
    } else {
        aBestZombie->TakeDamage(aUseUppercut ? kBonkChoyUppercutDamage : kBonkChoyPunchDamage, aDamageFlags);
    }
    mApp->PlayFoley(aFoleyType);
}

int Plant::CalcRenderOrder() {
    PLANT_ORDER anOrder = PLANT_ORDER::PLANT_ORDER_NORMAL;
    RenderLayer aLayer = RenderLayer::RENDER_LAYER_PLANT;

    SeedType aSeedType = mSeedType;
    if (mSeedType == SeedType::SEED_IMITATER && mImitaterType != SeedType::SEED_NONE)
        aSeedType = mImitaterType;

    if (mApp->IsWallnutBowlingLevel()) {
        aLayer = RenderLayer::RENDER_LAYER_PROJECTILE;
    } else if (aSeedType == SeedType::SEED_PUMPKINSHELL) {
        anOrder = PLANT_ORDER::PLANT_ORDER_PUMPKIN;
    } else if (IsFlying(aSeedType)) {
        anOrder = PLANT_ORDER::PLANT_ORDER_FLYER;
    } else if (aSeedType == SeedType::SEED_FLOWERPOT || (aSeedType == SeedType::SEED_LILYPAD && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN)) {
        anOrder = PLANT_ORDER::PLANT_ORDER_LILYPAD;
    }

    return Board::MakeRenderOrder(aLayer, mRow, anOrder * 5 - mX + 800);
}
