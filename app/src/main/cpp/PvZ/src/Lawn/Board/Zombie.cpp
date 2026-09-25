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

#include "PvZ/Lawn/Board/Zombie.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/Projectile.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Misc.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"
#include "PvZ/TodLib/Effect/TodParticle.h"

#include <cmath>
#include <cstring>

#include <numbers>

using namespace Sexy;

std::vector<ZombieType> Zombie::msDeadFollowers;

ZombieDefinition gZombieDefs[NUM_ZOMBIE_TYPES] = {
    {ZOMBIE_NORMAL, REANIM_ZOMBIE, 1, 1, 1, 4000, "ZOMBIE"},
    {ZOMBIE_FLAG, REANIM_ZOMBIE, 1, 1, 1, 0, "FLAG_ZOMBIE"},
    {ZOMBIE_TRAFFIC_CONE, REANIM_ZOMBIE, 2, 3, 1, 4000, "CONEHEAD_ZOMBIE"},
    {ZOMBIE_POLEVAULTER, REANIM_POLEVAULTER, 2, 6, 5, 2000, "POLE_VAULTING_ZOMBIE"},
    {ZOMBIE_PAIL, REANIM_ZOMBIE, 4, 8, 1, 3000, "BUCKETHEAD_ZOMBIE"},
    {ZOMBIE_NEWSPAPER, REANIM_ZOMBIE_NEWSPAPER, 2, 11, 1, 1000, "NEWSPAPER_ZOMBIE"},
    {ZOMBIE_DOOR, REANIM_ZOMBIE, 4, 13, 5, 3500, "SCREEN_DOOR_ZOMBIE"},
    {ZOMBIE_FOOTBALL, REANIM_ZOMBIE_FOOTBALL, 7, 16, 5, 2000, "FOOTBALL_ZOMBIE"},
    {ZOMBIE_DANCER, REANIM_DANCER, 5, 18, 5, 1000, "DANCING_ZOMBIE"},
    {ZOMBIE_BACKUP_DANCER, REANIM_BACKUP_DANCER, 1, 18, 1, 0, "BACKUP_DANCER"},
    {ZOMBIE_DUCKY_TUBE, REANIM_ZOMBIE, 1, 21, 5, 0, "DUCKY_TUBE_ZOMBIE"},
    {ZOMBIE_SNORKEL, REANIM_SNORKEL, 3, 23, 10, 2000, "SNORKEL_ZOMBIE"},
    {ZOMBIE_ZAMBONI, REANIM_ZOMBIE_ZAMBONI, 7, 26, 10, 2000, "ZOMBONI"},
    {ZOMBIE_BOBSLED, REANIM_BOBSLED, 3, 26, 10, 2000, "ZOMBIE_BOBSLED_TEAM"},
    {ZOMBIE_DOLPHIN_RIDER, REANIM_ZOMBIE_DOLPHINRIDER, 3, 28, 10, 1500, "DOLPHIN_RIDER_ZOMBIE"},
    {ZOMBIE_JACK_IN_THE_BOX, REANIM_JACKINTHEBOX, 3, 31, 10, 1000, "JACK_IN_THE_BOX_ZOMBIE"},
    {ZOMBIE_BALLOON, REANIM_BALLOON, 2, 33, 10, 2000, "BALLOON_ZOMBIE"},
    {ZOMBIE_DIGGER, REANIM_DIGGER, 4, 36, 10, 1000, "DIGGER_ZOMBIE"},
    {ZOMBIE_POGO, REANIM_POGO, 4, 38, 10, 1000, "POGO_ZOMBIE"},
    {ZOMBIE_YETI, REANIM_YETI, 4, 40, 1, 1, "ZOMBIE_YETI"},
    {ZOMBIE_BUNGEE, REANIM_BUNGEE, 3, 41, 10, 1000, "BUNGEE_ZOMBIE"},
    {ZOMBIE_LADDER, REANIM_LADDER, 4, 43, 10, 1000, "LADDER_ZOMBIE"},
    {ZOMBIE_CATAPULT, REANIM_CATAPULT, 5, 46, 10, 1500, "CATAPULT_ZOMBIE"},
    {ZOMBIE_GARGANTUAR, REANIM_GARGANTUAR, 10, 48, 15, 1500, "GARGANTUAR"},
    {ZOMBIE_IMP, REANIM_IMP, 10, 48, 1, 0, "IMP"},
    {ZOMBIE_BOSS, REANIM_BOSS, 10, 50, 1, 0, "BOSS"},
    {ZOMBIE_TRASHCAN, REANIM_ZOMBIE, 1, 99, 1, 4000, "TRASHCAN_ZOMBIE"},
    {ZOMBIE_PEA_HEAD, REANIM_ZOMBIE, 1, 99, 1, 4000, "PEA_HEAD_ZOMBIE"},
    {ZOMBIE_WALLNUT_HEAD, REANIM_ZOMBIE, 4, 99, 1, 3000, "WALLNUT_HEAD_ZOMBIE"},
    {ZOMBIE_JALAPENO_HEAD, REANIM_ZOMBIE, 3, 99, 10, 1000, "JALAPENO_HEAD_ZOMBIE"},
    {ZOMBIE_GATLING_HEAD, REANIM_ZOMBIE, 3, 99, 10, 2000, "GATLING_HEAD_ZOMBIE"},
    {ZOMBIE_SQUASH_HEAD, REANIM_ZOMBIE, 3, 99, 10, 2000, "SQUASH_HEAD_ZOMBIE"},
    {ZOMBIE_TALLNUT_HEAD, REANIM_ZOMBIE, 4, 99, 10, 2000, "TALLNUT_HEAD_ZOMBIE"},
    {ZOMBIE_REDEYE_GARGANTUAR, REANIM_GARGANTUAR, 10, 48, 15, 6000, "REDEYED_GARGANTUAR"},
};

ZombieDefinition gExtendedZombieDefs[] = {
    {ZOMBIE_GIGA_FOOTBALL, REANIM_GIGA_FOOTBALL, 7, 16, 5, 2000, "GIGA_FOOTBALL_ZOMBIE"},
    {ZOMBIE_SUPER_FAN_IMP, REANIM_SUPER_FAN_IMP, 1, 16, 1, 4000, "SUPER_FAN_IMP"},
    {ZOMBIE_JACKSON, REANIM_JACKSON, 5, 18, 5, 1000, "JACKSON_ZOMBIE"},
    {ZOMBIE_BACKUP_JACKSON, REANIM_BACKUP_JACKSON, 1, 18, 1, 0, "BACKUP_JACKSON"},
    {ZOMBIE_GIGA_POLEVAULTER, REANIM_GIGA_POLEVAULTER, 2, 6, 5, 2000, "GIGA_POLE_VAULTING_ZOMBIE"},
    {ZOMBIE_SUNDAY_EDITION, REANIM_SUNDAY_EDITION, 2, 11, 1, 1000, "SUNDAY_EDITION_ZOMBIE"},
    {ZOMBIE_EXPLORER, REANIM_EXPLORER, 2, 6, 5, 2000, "EXPLORER_ZOMBIE"},
    {ZOMBIE_ZOMBLOB, REANIM_ZOMBLOB, 5, 18, 5, 1000, "ZOMBLOB"},
    {ZOMBIE_ZOMBLOB_MIDDLE, REANIM_ZOMBLOB_MIDDLE, 1, 18, 1, 0, "ZOMBLOB"},
    {ZOMBIE_ZOMBLOB_SMALL, REANIM_ZOMBLOB_SMALL, 1, 18, 1, 0, "ZOMBLOB"},
    {ZOMBIE_GIGA_GARGANTUAR, REANIM_GIGA_GARGANTUAR, 10, 48, 15, 6000, "GIGA_GARGANTUAR"},
    {ZOMBIE_GIGA_IMP, REANIM_GIGA_IMP, 10, 48, 1, 0, "GIGA_IMP"},
    {ZOMBIE_DOGWALKER, REANIM_DOGWALKER, 2, 18, 5, 1000, "DOGWALKER_ZOMBIE"},
    {ZOMBIE_DOG, REANIM_DOG, 1, 18, 1, 0, "ZOMBIE_DOG"},
    {ZOMBIE_TELEPORTATION, REANIM_ZOMBIE_TELEPORTATION, 2, 18, 5, 1000, "TELEPORTATION_ZOMBIE"},
    {ZOMBIE_SUPER_NOVA_GARGANTUAR, REANIM_SUPER_NOVA_GARGANTUAR, 10, 48, 15, 1500, "SUPER_NOVA_GARGANTUAR"},
    {ZOMBIE_CROSSING_GUARD, REANIM_ZOMBIE_CROSSING_GUARD, 4, 36, 10, 1000, "CROSSING_GUARD_ZOMBIE"},
    {ZOMBIE_SCIENTIST, REANIM_ZOMBIE_SCIENTIST, 2, 33, 10, 2000, "SCIENTIST_ZOMBIE"},
};

ZombieDefinition &GetZombieDefinition(ZombieType theZombieType) {
    if (theZombieType == ZOMBIE_TRASHCAN) {
        return gZombieTrashBinDef;
    }

    if (theZombieType >= NUM_CACHED_ZOMBIE_TYPES) {
        return gExtendedZombieDefs[theZombieType - NUM_CACHED_ZOMBIE_TYPES];
    }

    return gZombieDefs[theZombieType];
}

void Zombie::ZombieInitialize(int theRow, ZombieType theType, bool theVariant, Zombie *theParentZombie, int theFromWave, bool isVisible) {
    old_Zombie_ZombieInitialize(this, theRow, theType, theVariant, theParentZombie, theFromWave, isVisible);

    mSquashHeadCol = -1;
    mIsRevived = false;
    mCanRevived = true;
    mButtered = false;
    mPoisoned = false;
    mPoisonedCounter = 0;
    mSunBeanSun = 0;
    mSunBeanDamageRemainder = 0;

    if (IsZombatarZombie(theType) && theFromWave != -3) {
        SetZombatarReanim();
    }

    // 为其余位于水路的僵尸添加鸭子救生圈
    if (mBoard && mBoard->mPlantRow[mRow] == PlantRowType::PLANTROW_POOL) {
        const bool shouldAttachDuckyTube = GetZombieDefinition(theType).mReanimationType != REANIM_ZOMBIE && mZombieType != ZombieType::ZOMBIE_SNORKEL
            && mZombieType != ZombieType::ZOMBIE_DOLPHIN_RIDER && mZombieType != ZombieType::ZOMBIE_BALLOON && mZombieType != ZombieType::ZOMBIE_BUNGEE;
        if (shouldAttachDuckyTube) {
            int offsetX = 20;
            int offsetY = -8;
            float scale = 1.0f;
            if (theType == ZombieType::ZOMBIE_FOOTBALL) {
                offsetX = 30;
                offsetY = -45;
                scale = 1.2f;
            } else if (theType == ZombieType::ZOMBIE_IMP) {
                offsetX = 35;
                offsetY = 22;
                scale = 0.8f;
            } else if (theType == ZombieType::ZOMBIE_GARGANTUAR) {
                offsetX = 0;
                offsetY = -50;
                scale = 1.5f;
            }
            Reanimation *reanim = AddAttachedReanim(offsetX, offsetY, ReanimationType::REANIM_ZOMBIE);
            SetupReanimLayers(reanim, theType);
            reanim->OverrideScale(scale, scale);
            reanim->PlayReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
            reanim->AssignRenderGroupToPrefix("zombie_duckytube", RENDER_GROUP_NORMAL);
            ReanimIgnoreClipRect("Zombie_duckytube", true);
            ReanimatorTrackInstance *aTrackInstance = reanim->GetTrackInstanceByName("Zombie_whitewater");
            aTrackInstance->mIgnoreExtraAdditiveColor = true;
            aTrackInstance->mIgnoreColorOverride = true;
            aTrackInstance->mIgnoreClipRect = true;
            ReanimatorTrackInstance *aTrackInstance2 = reanim->GetTrackInstanceByName("Zombie_whitewater2");
            aTrackInstance2->mIgnoreExtraAdditiveColor = true;
            aTrackInstance2->mIgnoreColorOverride = true;
            aTrackInstance2->mIgnoreClipRect = true;
            // 隐藏非鸭子救生圈轨道
            reanim->AssignRenderGroupToPrefix("anim_head", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("anim_hair", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("anim_tongue", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("anim_head1", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_neck", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_tie", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_body", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_outerarm_upper", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_outerarm_lower", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_outerarm_hand", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("anim_innerarm1", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("anim_innerarm2", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("anim_innerarm3", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_outerleg_lower", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_outerleg_foot", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_outerleg_upper", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_innerleg_lower", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_innerleg_foot", RENDER_GROUP_HIDDEN);
            reanim->AssignRenderGroupToPrefix("Zombie_innerleg_upper", RENDER_GROUP_HIDDEN);
        }
    }

    switch (theType) {
        // 默认值
        // mZombieRect = Rect(36, 0, 42, 115);
        // mZombieAttackRect = Rect(50, 0, 20, 115);
        case ZombieType::ZOMBIE_BALLOON:
            if (mApp->IsVSMode() && IsOnBoard()) {
                mAltitude = 0.0f;
                mFlyingHealth = 370;
                PickRandomSpeed();
            }
            break;

        case ZombieType::ZOMBIE_IMP:
            // if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
            // mBodyHealth = 70;
            // }
            break;

        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
            mZombieRect = Rect(50, 0, 57, 115);
            ReanimShowPrefix("anim_hair", RENDER_GROUP_HIDDEN);
            mHelmType = HelmType::HELMTYPE_GIGA_FOOTBALL;
            mHelmHealth = 1100;
            mAnimTicksPerFrame = 6;
            mVariant = false;
            if (IsOnBoard()) {
                mZombiePhase = ZombiePhase::PHASE_FOOTBALL_PRE_CHARGE;
                StartWalkAnim(0);
            }
            break;

        case ZombieType::ZOMBIE_SUPER_FAN_IMP:
        case ZombieType::ZOMBIE_GIGA_IMP:
            if (!IsOnBoard()) {
                PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
            } else {
                mZombiePhase = ZombiePhase::PHASE_IMP_PRE_RUN;
                mTargetCol = RandRangeInt(0, 2);            // 自爆的位置
                mTargetRow = mBoard->GridToPixelX(8, mRow); // 被伞弹回的落点
            }
            break;

        case ZombieType::ZOMBIE_JACKSON:
            if (!IsOnBoard()) {
                PlayZombieReanim("anim_moonwalk", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
            } else {
                mZombiePhase = ZombiePhase::PHASE_DANCER_DANCING_IN;
                mVelX = 0.5f;
                mPhaseCounter = 300 + Rand(12);
                mSummonCounter = mPhaseCounter + 1500;
                PlayZombieReanim("anim_moonwalk", ReanimLoopType::REANIM_LOOP, 0, 24.0f);
            }
            mBodyHealth = 500;
            mVariant = false;
            break;

        case ZombieType::ZOMBIE_BACKUP_JACKSON:
            if (!IsOnBoard()) {
                PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
            }
            mZombiePhase = ZombiePhase::PHASE_DANCER_DANCING_LEFT;
            mVariant = false;
            break;

        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
            mBodyHealth = 500;
            mAnimTicksPerFrame = 6;
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT;
            mHasObject = true;
            mVariant = false;
            mPosX = float(WIDE_BOARD_WIDTH + 70 + Rand(10));
            mSummonCounter = 1;
            ReanimShowPrefix("anim_pole1", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("anim_pole2", RENDER_GROUP_HIDDEN);
            if (IsOnBoard()) {
                PlayZombieReanim("anim_run", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
                PickRandomSpeed();
            }
            if (mApp->IsWallnutBowlingLevel()) {
                mZombieAttackRect = Rect(-229, 0, 270, 115);
            } else {
                mZombieAttackRect = Rect(-29, 0, 70, 115);
            }
            break;

        case ZombieType::ZOMBIE_SUNDAY_EDITION:
            mZombieAttackRect = Rect(20, 0, 50, 115);
            mZombiePhase = ZombiePhase::PHASE_NEWSPAPER_READING;
            mShieldType = ShieldType::SHIELDTYPE_SUNDAY_EDITION;
            mShieldHealth = 370;
            mBodyHealth = 500;
            mVariant = false;
            AttachShield();
            break;

        case ZombieType::ZOMBIE_EXPLORER:
            mZombieAttackRect = Rect(-10, 0, 50, 115);
            mHasObject = true;
            mBodyHealth = 500;
            mVariant = false;
            break;

        case ZombieType::ZOMBIE_ZOMBLOB:
            mZombieAttackRect = Rect(20, 0, 50, 115);
            mBodyHealth = 190;
            mVariant = false;
            break;

        case ZombieType::ZOMBIE_ZOMBLOB_MIDDLE:
            mZombieAttackRect = Rect(20, 0, 50, 115);
            mBodyHealth = 140;
            mVariant = false;
            break;

        case ZombieType::ZOMBIE_ZOMBLOB_SMALL:
            mZombieAttackRect = Rect(50, 0, 20, 115);
            mBodyHealth = 85;
            mVariant = false;
            break;

        case ZombieType::ZOMBIE_DOGWALKER:
            mBodyHealth = 500;
            mVariant = false;
            mZombieAttackRect = Rect(20, 0, 50, 115);
            ReanimShowTrack("Zombie_dogwalker_rope2_2", RENDER_GROUP_HIDDEN);
            break;

        case ZombieType::ZOMBIE_DOG:
            mBodyHealth = 330;
            mVariant = false;
            mZombieRect = Rect(20, 0, 50, 115);
            mZombieAttackRect = Rect(15, 0, 40, 115);
            mTargetRow = theRow; // 固定保存出生行，索敌范围始终是出生行及上下相邻行
            mZombiePhase = ZombiePhase::PHASE_DOG_WALKING;
            PickRandomSpeed();
            break;

        case ZombieType::ZOMBIE_TELEPORTATION:
            mBodyHealth = 500;
            mVariant = false;
            mZombieAttackRect = Rect(20, 0, 50, 115);
            mPhaseCounter = RandRangeInt(1000, 1500);
            break;

        case ZombieType::ZOMBIE_CROSSING_GUARD:
            mBodyHealth = 270;
            mHelmType = HelmType::HELMTYPE_CROSSING_GUARD;
            mHelmHealth = 100;
            mVariant = false;
            mZombieAttackRect = Rect(20, 0, 50, 115);
            mPhaseCounter = 500;
            break;

        case ZombieType::ZOMBIE_SCIENTIST:
            mBodyHealth = 500;
            mVariant = false;
            mZombieAttackRect = Rect(-110, -80, 160, 300);
            break;

        case ZombieType::ZOMBIE_GIGA_GARGANTUAR: {
            mWidth = 180;
            mHeight = 180;
            mBodyHealth = 3000;
            mAnimFrames = 24;
            mAnimTicksPerFrame = 8;
            mPosX = float(WIDE_BOARD_WIDTH + 45 + Rand(10));
            mZombieRect = Rect(-17, -38, 125, 154);
            mZombieAttackRect = Rect(-30, -38, 89, 154);
            mVariant = false;
            RenderLayer aRenderLayer = RenderLayer::RENDER_LAYER_ZOMBIE;
            int aRenderOffset = 8;
            mRenderOrder = Board::MakeRenderOrder(aRenderLayer, mRow, aRenderOffset);
            mHasObject = true;
            mSummonCounter = 0;
            mPhaseCounter = 1500;

            Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
            if (aBodyReanim != nullptr) {
                aBodyReanim->AssignRenderGroupToPrefix("lightning_attack", RENDER_GROUP_GIGA_LIGHTNING);
                aBodyReanim->AssignRenderGroupToPrefix("lightning_splash", RENDER_GROUP_GIGA_LIGHTNING);
            }
            break;
        }

        case ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR:
            mWidth = 180;
            mHeight = 180;
            mBodyHealth = 3000;
            mAnimFrames = 24;
            mAnimTicksPerFrame = 8;
            mPosX = float(WIDE_BOARD_WIDTH + 45 + Rand(10));
            mZombieRect = Rect(-17, -38, 125, 154);
            mZombieAttackRect = Rect(-30, -38, 89, 154);
            mVariant = false;
            mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_ZOMBIE, mRow, 8);
            mHasObject = false;
            mTargetPlantID = PlantID::PLANTID_NULL;
            mTargetCol = int(SeedType::SEED_NONE);
            break;

        default:
            break;
    }

    mBodyMaxHealth = mBodyHealth;
    mHelmMaxHealth = mHelmHealth;
    mShieldMaxHealth = mShieldHealth;
    mFlyingMaxHealth = mFlyingHealth;

    if (zombieSetScale != 0 && mZombieType != ZombieType::ZOMBIE_BOSS && !IsOnlineServerModeActive() && !gIsReplayMode) {
        mScaleZombie = 0.2f * zombieSetScale;
        UpdateAnimSpeed();
        float aRatio = mScaleZombie * mScaleZombie;
        mBodyHealth *= aRatio;
        mHelmHealth *= aRatio;
        mShieldHealth *= aRatio;
        mFlyingHealth *= aRatio;
    }
}

void Zombie::CheckIfPreyCaught() {
    if (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) {
        Zombie *aPartner = GetDogPartner();
        if (aPartner != nullptr) {
            const bool aChangingRow = IsChangingRow() || aPartner->IsChangingRow();
            if (aChangingRow) {
                if (mIsEating) {
                    StopEating();
                }
                return;
            }
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_BUNGEE || IsGargantuar() || mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombieType == ZombieType::ZOMBIE_CATAPULT || mZombieType == ZombieType::ZOMBIE_BOSS
        || IsBouncingPogo() || IsBobsledTeamWithSled() || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT
        || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_THROW || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PICK || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_TAKE
        || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PREPARE || mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MADDENING || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_STUNNED
        || mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_BLOCKED
        || mZombiePhase == ZombiePhase::PHASE_IMP_LANDING || mZombiePhase == ZombiePhase::PHASE_DANCER_RISING || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS
        || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD
        || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_WITHOUT_DOLPHIN || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL
        || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP || mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL
        || mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING || mZombiePhase == ZombiePhase::PHASE_LADDER_PLACING || mZombiePhase == ZombiePhase::PHASE_FOOTBALL_CHARGING
        || mZombiePhase == ZombiePhase::PHASE_FOOTBALL_TACKLING || mZombiePhase == ZombiePhase::PHASE_FOOTBALL_KICKING || mZombiePhase == ZombiePhase::PHASE_IMP_POPPING
        || mZombiePhase == ZombiePhase::PHASE_DOGWALKER_ROPE_BREAK || mZombiePhase == ZombiePhase::PHASE_TELEPORTATION_SHOOTING || mZombiePhase == ZombiePhase::PHASE_CROSSING_GUARD_THROWING
        || mZombieType == ZombieType::ZOMBIE_SCIENTIST || mZombieHeight == ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED || mZombieHeight == ZombieHeight::HEIGHT_UP_LADDER
        || mZombieHeight == ZombieHeight::HEIGHT_IN_TO_POOL || mZombieHeight == ZombieHeight::HEIGHT_OUT_OF_POOL || IsTangleKelpTarget() || mZombieHeight == ZombieHeight::HEIGHT_FALLING || !mHasHead
        || IsFlying()) {
        return;
    }

    int aTicksBetweenEats = TICKS_BETWEEN_EATS;
    if (mChilledCounter > 0) {
        aTicksBetweenEats *= 2;
    }
    if (mZombieAge % aTicksBetweenEats != 0) {
        return;
    }

    Zombie *aZombie = FindZombieTarget();
    if (aZombie) {
        EatZombie(aZombie);
        return;
    }

    if (!mMindControlled) {
        Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
        if (aPlant) {
            EatPlant(aPlant);
            return;
        }
    }

    if (mApp->IsIZombieLevel() && mBoard->mChallenge->IZombieEatBrain(this)) {
        return;
    }

    if (mIsEating) {
        StopEating();
    }
}

bool Zombie::IsOnBoard() const {
    if (mFromWave == Zombie::ZOMBIE_WAVE_CUTSCENE || mFromWave == Zombie::ZOMBIE_WAVE_UI) {
        return false;
    }

    return true;
}

void Zombie::Update() {
    if (zombieBloated && !IsOnlineServerModeActive() && !gIsReplayMode) {
        // 如果开启了“普僵必噎死”
        mBloated = mZombieType == ZombieType::ZOMBIE_NORMAL && !mInPool;
    }

    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 如果开了高级暂停
        return;
    }

    if (mZombieType == ZombieType::ZOMBIE_FLAG && mBossFireBallReanimID != 0) {
        Reanimation *reanimation = mApp->ReanimationTryToGet(mBossFireBallReanimID);
        if (reanimation != nullptr)
            reanimation->Update();
    }

    mUnk95 = 0;
    ++mZombieAge;
    bool doUpdate = false;
    if (mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO && mZombieType == ZombieType::ZOMBIE_BOSS) {
        doUpdate = true;
    } else if (IsOnBoard() && mBoard->mCutScene->ShouldRunUpsellBoard()) {
        doUpdate = true;
    } else if (mApp->mGameScene == GameScenes::SCENE_PLAYING || !IsOnBoard() || mFromWave == Zombie::ZOMBIE_WAVE_WINNER) {
        doUpdate = true;
    }

    if (doUpdate) {
        if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED) {
            UpdateBurn();
        } else if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED) {
            UpdateMowered();
        } else if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING) {
            UpdateDeath();
            UpdateZombieWalking();
        } else {
            if (mPhaseCounter > 0 && !IsImmobilizied() && /*修复蹦极空投中的僵尸会更新阶段*/ mZombieHeight != ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED) {
                --mPhaseCounter;
            }

            if (mApp->mGameScene == SCENE_ZOMBIES_WON) {
                UpdateZombieChimney();
                UpdateZombieWalkingIntoHouse();
            } else if (IsOnBoard()) {
                UpdatePlaying();
            }

            if (mZombieType == ZOMBIE_BUNGEE) {
                UpdateZombieBungee();
            }
            if (mZombieType == ZOMBIE_POGO) {
                UpdateZombiePogo();
            }

            Animate();
        }

        --mJustGotShotCounter;
        if (mShieldJustGotShotCounter > 0) {
            --mShieldJustGotShotCounter;
        }
        if (mShieldRecoilCounter > 0) {
            --mShieldRecoilCounter;
        }
        if (mZombieFade > 0) {
            --mZombieFade;
            if (mZombieFade == 0) {
                DieNoLoot();
            }
        }
        if (mPoisonedCounter > 0) {
            --mPoisonedCounter;
        }

        mX = int(mPosX);
        mY = int(mPosY);

        AttachmentUpdateAndMove(mAttachmentID, mPosX, mPosY);
        UpdateReanim();
    }
}

void Zombie::UpdateActions() {
    old_Zombie_UpdateActions(this);

    // UpdateZombieType类函数在此处添加
    if (mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
        UpdateGigaFootball();
    }
    if (mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP) {
        UpdateSuperFanImp();
    }
    if (mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
        UpdateZombieBackupDancer();
    }
    if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
        UpdateZombieJackson();
    }
    if (mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
        UpdateGigaPolevaulter();
    }
    if (mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        UpdateSundayEdition();
    }
    if (mZombieType == ZombieType::ZOMBIE_EXPLORER) {
        UpdateZombieExplorer();
    }
    if (mZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR) {
        UpdateGigaGargantuar();
    }
    if (mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
        UpdateGigaImp();
    }
    if (mZombieType == ZombieType::ZOMBIE_DOGWALKER) {
        UpdateDogWalker();
    }
    if (mZombieType == ZombieType::ZOMBIE_DOG) {
        UpdateZombieDog();
    }
    if (mZombieType == ZombieType::ZOMBIE_TELEPORTATION) {
        UpdateZombieTeleportation();
    }
    if (mZombieType == ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR) {
        UpdateSuperNovaGargantuar();
    }
    if (mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD) {
        UpdateZombieCrossingGuard();
    }
    if (mZombieType == ZombieType::ZOMBIE_SCIENTIST) {
        UpdateZombieScientist();
    }
}

void Zombie::UpdateZombieTeleportation() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr || IsDeadOrDying()) {
        return;
    }
    if (mZombiePhase == ZombiePhase::PHASE_TELEPORTATION_PRE_SHOOT) {
        mPhaseCounter = 500;
        mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
    }

    if (!mHasHead) {
        if (mZombiePhase == ZombiePhase::PHASE_TELEPORTATION_SHOOTING) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(10);
        }
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_TELEPORTATION_SHOOTING) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.5f)) {
            const float aOriginOffsetX = mMindControlled ? 95.0f : 5.0f;
            constexpr float aOriginOffsetY = 30.0f;
            Projectile *aProjectile = mBoard->AddProjectile(int(mPosX + aOriginOffsetX), int(mPosY + aOriginOffsetY - mAltitude), mRenderOrder + 1, mRow, ProjectileType::PROJECTILE_TELEPORTATION);
            aProjectile->mMotionType = mMindControlled ? ProjectileMotion::MOTION_STRAIGHT : ProjectileMotion::MOTION_BACKWARDS;
            aProjectile->mTargetZombieID = mBoard->ZombieGetID(this);
            aProjectile->mReturning = mMindControlled;
        }

        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            mPhaseCounter = IsRemoteClientOrViewer() ? 1500 : RandRangeInt(1000, 1500);
            StartWalkAnim(10);
        }
        return;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (mPhaseCounter <= 0 && !IsImmobilizied() && FindTeleportationTarget()) {
        StopEating();
        mZombiePhase = ZombiePhase::PHASE_TELEPORTATION_SHOOTING;
        PlayZombieReanim("anim_shoot", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
        if (IsRemoteServer()) {
            U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_TELEPORTATION_SHOOT;
            event.data = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            netplay::PutEvent(event);
        }
    }
}

bool Zombie::FindTeleportationTarget() {
    const float aProjectileOriginX = mPosX + 5.0f;
    const auto IsAhead = [this, aProjectileOriginX](float theLeft, float theRight) {
        return mMindControlled ? theRight > aProjectileOriginX && theLeft < WIDE_BOARD_WIDTH : theLeft < aProjectileOriginX && theRight > 0.0f;
    };

    if (!mMindControlled) {
        Plant *aPlant = nullptr;
        while (mBoard->IteratePlants(aPlant)) {
            if (aPlant->NotOnGround() || aPlant->IsInvulnerable() || aPlant->mRow != mRow || aPlant->IsLowProfile() || aPlant->mSeedType == SeedType::SEED_INSTANT_COFFEE) {
                continue;
            }

            const Rect aPlantRect = aPlant->GetPlantRect();
            if (IsAhead(float(aPlantRect.mX), float(aPlantRect.mX + aPlantRect.mWidth))) {
                return true;
            }
        }
    }

    const ZombieID aSelfID = mBoard->ZombieGetID(this);
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (!aZombie->IsValidTeleportationTarget() || aZombie->mRow != mRow || mBoard->ZombieGetID(aZombie) == aSelfID || (mMindControlled && aZombie->mMindControlled)) {
            continue;
        }

        const Rect aZombieRect = aZombie->GetZombieRect();
        if (IsAhead(float(aZombieRect.mX), float(aZombieRect.mX + aZombieRect.mWidth))) {
            return true;
        }
    }

    return false;
}

bool Zombie::IsValidCrossingGuardTarget(Zombie *theTarget, bool theCheckRange) {
    if (theTarget == nullptr || theTarget == this || theTarget->IsDeadOrDying() || !theTarget->mHasHead || theTarget->mMindControlled != mMindControlled
        || theTarget->mHelmType != HelmType::HELMTYPE_NONE || theTarget->mHelmHealth > 0) {
        return false;
    }

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(theTarget->mBodyReanimID);
    if (aBodyReanim == nullptr || !aBodyReanim->TrackExists("anim_cone")) {
        return false;
    }

    if (theCheckRange) {
        constexpr float CROSSING_GUARD_RANGE = 250.0f;
        const float aX = mPosX + float(mWidth / 2);
        const float aY = mPosY + float(mHeight / 2);
        const float aTargetX = theTarget->mPosX + float(theTarget->mWidth / 2);
        const float aTargetY = theTarget->mPosY + float(theTarget->mHeight / 2);
        if (Distance2D(aX, aY, aTargetX, aTargetY) > CROSSING_GUARD_RANGE) {
            return false;
        }
    }

    return true;
}

bool Zombie::IsTrafficConeTargetReserved(Zombie *theTarget) {
    if (theTarget == nullptr || theTarget->mRelatedZombieID != ZombieID::ZOMBIEID_NULL) {
        return true;
    }

    const ZombieID aTargetID = mBoard->ZombieGetID(theTarget);
    Projectile *aProjectile = nullptr;
    while (mBoard->IterateProjectiles(aProjectile)) {
        if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_TRAFFIC_CONE && aProjectile->mTargetZombieID == aTargetID) {
            return true;
        }
    }

    return false;
}

bool Zombie::BindRealatedZombie(Zombie *theZombie) {
    if (theZombie == nullptr || theZombie == this || mRelatedZombieID != ZombieID::ZOMBIEID_NULL || theZombie->mRelatedZombieID != ZombieID::ZOMBIEID_NULL) {
        return false;
    }

    mRelatedZombieID = mBoard->ZombieGetID(theZombie);
    theZombie->mRelatedZombieID = mBoard->ZombieGetID(this);
    return true;
}

void Zombie::UnbindRealatedZombie() {
    const ZombieID aRelatedZombieID = mRelatedZombieID;
    mRelatedZombieID = ZombieID::ZOMBIEID_NULL;

    Zombie *aRelatedZombie = mBoard->ZombieTryToGet(aRelatedZombieID);
    if (aRelatedZombie != nullptr && aRelatedZombie->mRelatedZombieID == mBoard->ZombieGetID(this)) {
        aRelatedZombie->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
    }
}

Zombie *Zombie::FindCrossingGuardTarget() {
    Zombie *aBestZombie = nullptr;
    bool aBestIsSameRow = false;
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (!IsValidCrossingGuardTarget(aZombie, true) || IsTrafficConeTargetReserved(aZombie)) {
            continue;
        }

        const bool aIsSameRow = aZombie->mRow == mRow;
        if (aBestZombie == nullptr || (aIsSameRow && !aBestIsSameRow) || (aIsSameRow == aBestIsSameRow && aZombie->mPosX < aBestZombie->mPosX)) {
            aBestZombie = aZombie;
            aBestIsSameRow = aIsSameRow;
        }
    }

    return aBestZombie;
}

void Zombie::ApplyTrafficCone() {
    if (IsDeadOrDying() || !mHasHead || mHelmType != HelmType::HELMTYPE_NONE || mHelmHealth > 0) {
        return;
    }

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr || !aBodyReanim->TrackExists("anim_cone")) {
        return;
    }

    mApp->PlayFoley(FoleyType::FOLEY_PLASTIC_HIT);

    aBodyReanim->SetImageOverride("anim_cone", IMAGE_REANIM_ZOMBIE_CONE1);
    ReanimShowPrefix("anim_cone", RENDER_GROUP_NORMAL);
    ReanimShowPrefix("anim_hair", RENDER_GROUP_HIDDEN);
    mHelmType = HelmType::HELMTYPE_TRAFFIC_CONE;
    mHelmMaxHealth = mHelmHealth = 370;
}

void Zombie::LaunchTrafficCone(Zombie *theTarget) {
    if (theTarget == nullptr) {
        return;
    }

    auto aOriginX = int(mPosX);
    auto aOriginY = int(mPosY - 10.0f);
    Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder + 1, mRow, ProjectileType::PROJECTILE_TRAFFIC_CONE);
    aProjectile->mMotionType = ProjectileMotion::MOTION_LOBBED;
    aProjectile->mTargetZombieID = mBoard->ZombieGetID(theTarget);
    aProjectile->mCobTargetRow = theTarget->mRow;
    aProjectile->mLastPortalX = mMindControlled ? 1 : 0;
    mApp->PlayFoley(FoleyType::FOLEY_THROW);
}

void Zombie::UpdateZombieCrossingGuard() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr || IsDeadOrDying()) {
        return;
    }

    if (!mHasHead) {
        if (mZombiePhase == ZombiePhase::PHASE_CROSSING_GUARD_THROWING) {
            UnbindRealatedZombie();
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(10);
        }
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_CROSSING_GUARD_THROWING) {
        if (!IsRemoteClientOrViewer() && aBodyReanim->ShouldTriggerTimedEvent(0.74f)) {
            Zombie *aZombie = mBoard->ZombieTryToGet(mRelatedZombieID);
            const bool aHasValidBinding = IsValidCrossingGuardTarget(aZombie, false) && aZombie->mRelatedZombieID == mBoard->ZombieGetID(this);
            if (!aHasValidBinding) {
                UnbindRealatedZombie();
                aZombie = FindCrossingGuardTarget();
                if (aZombie != nullptr && !BindRealatedZombie(aZombie)) {
                    aZombie = nullptr;
                }
            }

            if (aZombie != nullptr) {
                LaunchTrafficCone(aZombie);
                if (IsRemoteServer()) {
                    U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CROSSING_GUARD_FIRE;
                    event.data1 = uint16_t(mBoard->ZombieGetID(this));
                    event.data2 = uint16_t(mBoard->ZombieGetID(aZombie));
                    netplay::PutEvent(event);
                }
                UnbindRealatedZombie();
            } else {
                mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
                mPhaseCounter = 1000;
                StartWalkAnim(10);
                if (IsRemoteServer()) {
                    U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CROSSING_GUARD_FIRE;
                    event.data1 = uint16_t(mBoard->ZombieGetID(this));
                    event.data2 = NETPLAY_ZOMBIE_ID_NULL;
                    netplay::PutEvent(event);
                }
                return;
            }
        }

        if (aBodyReanim->mLoopCount > 0) {
            if (!IsRemoteClientOrViewer()) {
                const bool aThrowWasCancelled = mRelatedZombieID != ZombieID::ZOMBIEID_NULL;
                UnbindRealatedZombie();
                if (aThrowWasCancelled && IsRemoteServer()) {
                    U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CROSSING_GUARD_FIRE;
                    event.data1 = uint16_t(mBoard->ZombieGetID(this));
                    event.data2 = NETPLAY_ZOMBIE_ID_NULL;
                    netplay::PutEvent(event);
                }
            }
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            mPhaseCounter = 1000;
            StartWalkAnim(10);
        }
        return;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (mPhaseCounter <= 0 && !IsImmobilizied()) {
        if (Zombie *aZombie = FindCrossingGuardTarget(); aZombie != nullptr && BindRealatedZombie(aZombie)) {
            StopEating();
            mZombiePhase = ZombiePhase::PHASE_CROSSING_GUARD_THROWING;
            PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 16.0f);
            if (IsRemoteServer()) {
                U16_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CROSSING_GUARD_THROW;
                event.data = uint16_t(mBoard->ZombieGetID(this));
                netplay::PutEvent(event);
            }
        }
    }
}

bool Zombie::IsInScientistTargetRange(const Rect &theTargetRect, int theTargetRow, int theRangeInset) {
    if (std::abs(theTargetRow - mRow) > 1) {
        return false;
    }

    Rect aTargetRange = GetZombieAttackRect();
    aTargetRange.mWidth -= theRangeInset;
    if (!IsWalkingBackwards()) {
        aTargetRange.mX += theRangeInset;
    }
    return aTargetRange.Intersects(theTargetRect);
}

bool Zombie::HasScientistTriggerTarget() {
    constexpr int SCIENTIST_TARGET_RANGE_INSET = 20;
    constexpr int SCIENTIST_FRIEND_RANGE_INSET = 40;

    if (!mMindControlled) {
        Plant *aPlant = nullptr;
        while (mBoard->IteratePlants(aPlant)) {
            const Rect aPlantRect = aPlant->GetPlantRect();
            if (IsInScientistTargetRange(aPlantRect, aPlant->mRow, SCIENTIST_TARGET_RANGE_INSET) && CanTargetPlant(aPlant, ZombieAttackType::ATTACKTYPE_CHEW)) {
                return true;
            }
        }
    }

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (aZombie == this || aZombie->IsDeadOrDying() || !aZombie->mHasHead || !aZombie->IsOnBoard()) {
            continue;
        }

        const Rect aZombieRect = aZombie->GetZombieRect();
        const bool aIsFriendly = aZombie->mMindControlled == mMindControlled;
        const int aRangeInset = aIsFriendly ? SCIENTIST_FRIEND_RANGE_INSET : SCIENTIST_TARGET_RANGE_INSET;
        if (!IsInScientistTargetRange(aZombieRect, aZombie->mRow, aRangeInset)) {
            continue;
        }

        if (!aIsFriendly) {
            return true;
        }

        const bool aHelmNeedsHealing = aZombie->mHelmType != HelmType::HELMTYPE_NONE && aZombie->mHelmMaxHealth > 0 && aZombie->mHelmHealth * 3 <= aZombie->mHelmMaxHealth * 2;
        const bool aShieldNeedsHealing = aZombie->mShieldType != ShieldType::SHIELDTYPE_NONE && aZombie->mShieldMaxHealth > 0 && aZombie->mShieldHealth * 3 <= aZombie->mShieldMaxHealth * 2;
        const bool aBodyNeedsHealing = aZombie->mBodyMaxHealth > 0 && aZombie->mBodyHealth * 3 <= aZombie->mBodyMaxHealth * 2;
        if (aHelmNeedsHealing || aShieldNeedsHealing || aBodyNeedsHealing) {
            return true;
        }
    }

    return false;
}

bool Zombie::ApplyScientistHealing() {
    constexpr int SCIENTIST_HEAL_PER_PULSE = 20;
    if (IsDeadOrDying()) {
        return false;
    }

    bool aHealed = false;
    if (mHelmType != HelmType::HELMTYPE_NONE && mHelmHealth < mHelmMaxHealth) {
        mHelmHealth = std::min(mHelmMaxHealth, mHelmHealth + SCIENTIST_HEAL_PER_PULSE);
        aHealed = true;
    } else {
        if (mShieldType != ShieldType::SHIELDTYPE_NONE && mShieldHealth < mShieldMaxHealth) {
            mShieldHealth = std::min(mShieldMaxHealth, mShieldHealth + SCIENTIST_HEAL_PER_PULSE);
            aHealed = true;
        }
        if (mBodyHealth < mBodyMaxHealth) {
            mBodyHealth = std::min(mBodyMaxHealth, mBodyHealth + SCIENTIST_HEAL_PER_PULSE);
            aHealed = true;
        }
    }

    if (aHealed) {
        Reanimation *aHealReanim = mApp->AddReanimation(mPosX + 60.0f, mPosY, mRenderOrder + 1, ReanimationType::REANIM_HEAL_PARTICLES);
        if (aHealReanim != nullptr) {
            aHealReanim->PlayReanim("anim_heal", ReanimLoopType::REANIM_PLAY_ONCE, 0, 24.0f);
        }
    }
    return aHealed;
}

void Zombie::ApplyScientistSpray() {
    constexpr int SCIENTIST_DAMAGE_PER_PULSE = 150;
    constexpr int SCIENTIST_TARGET_RANGE_INSET = 20;

    if (!mMindControlled) {
        Plant *aPlant = nullptr;
        while (mBoard->IteratePlants(aPlant)) {
            const Rect aPlantRect = aPlant->GetPlantRect();
            if (!IsInScientistTargetRange(aPlantRect, aPlant->mRow, SCIENTIST_TARGET_RANGE_INSET) || !CanTargetPlant(aPlant, ZombieAttackType::ATTACKTYPE_CHEW) || aPlant->IsInvulnerable()) {
                continue;
            }

            aPlant->mPlantHealth -= SCIENTIST_DAMAGE_PER_PULSE;
            aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 50);
            if (aPlant->mPlantHealth <= 0) {
                aPlant->Die();
            }
        }
    }

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (aZombie == this || aZombie->IsDeadOrDying() || !aZombie->IsOnBoard()) {
            continue;
        }

        const Rect aZombieRect = aZombie->GetZombieRect();
        const bool aIsFriendly = aZombie->mMindControlled == mMindControlled;
        const int aRangeInset = aIsFriendly ? 0 : SCIENTIST_TARGET_RANGE_INSET;
        if (!IsInScientistTargetRange(aZombieRect, aZombie->mRow, aRangeInset)) {
            continue;
        }

        if (!aIsFriendly) {
            aZombie->TakeDamage(SCIENTIST_DAMAGE_PER_PULSE, 0U);
            continue;
        }

        if (IsRemoteClientOrViewer()) {
            continue;
        }

        const bool aHealed = aZombie->ApplyScientistHealing();
        if (aHealed && IsRemoteServer()) {
            U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_SCIENTIST_HEAL;
            event.data = uint16_t(mBoard->mZombies.DataArrayGetID(aZombie));
            netplay::PutEvent(event);
        }
    }
}

void Zombie::SetScientistPhase(ZombiePhase thePhase) {
    StopEating();
    mZombiePhase = thePhase;

    if (thePhase == ZombiePhase::PHASE_SCIENTIST_SHOOTING) {
        mPhaseCounter = 200;
        PlayZombieReanim("anim_shooting", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
    } else if (thePhase == ZombiePhase::PHASE_SCIENTIST_WAITING) {
        PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 10, 24.0f);
    } else if (thePhase == ZombiePhase::PHASE_ZOMBIE_NORMAL) {
        StartWalkAnim(10);
    }

    if (IsRemoteServer()) {
        U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_SCIENTIST_STATE;
        event.data1 = uint16_t(mBoard->ZombieGetID(this));
        event.data2 = uint16_t(thePhase);
        netplay::PutEvent(event);
    }
}

void Zombie::UpdateZombieScientist() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr || IsDeadOrDying()) {
        return;
    }

    if (!mHasHead) {
        if (!IsRemoteClientOrViewer() && (mZombiePhase == ZombiePhase::PHASE_SCIENTIST_WAITING || mZombiePhase == ZombiePhase::PHASE_SCIENTIST_SHOOTING)) {
            SetScientistPhase(ZombiePhase::PHASE_ZOMBIE_NORMAL);
        }
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_SCIENTIST_SHOOTING) {
        if (!IsImmobilizied() && aBodyReanim->ShouldTriggerTimedEvent(0.44f)) {
            Reanimation *aMistReanim = mApp->AddReanimation(mPosX + 20.0f, mPosY + 80.0f, mRenderOrder + 1, ReanimationType::REANIM_HEAL_MIST);
            if (aMistReanim != nullptr) {
                aMistReanim->PlayReanim("anim_mist", ReanimLoopType::REANIM_PLAY_ONCE, 0, 24.0f);
            }
            ApplyScientistSpray();
            mApp->PlayFoley(FoleyType::FOLEY_BALLOONINFLATE);
        }
        if (IsRemoteClientOrViewer()) {
            return;
        }
        if (aBodyReanim->mLoopCount > 0) {
            SetScientistPhase(HasScientistTriggerTarget() ? ZombiePhase::PHASE_SCIENTIST_WAITING : ZombiePhase::PHASE_ZOMBIE_NORMAL);
        }
        return;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    const bool aHasTarget = HasScientistTriggerTarget();
    if (mZombiePhase == ZombiePhase::PHASE_SCIENTIST_WAITING) {
        if (!aHasTarget) {
            SetScientistPhase(ZombiePhase::PHASE_ZOMBIE_NORMAL);
        } else if (!IsImmobilizied() && mPhaseCounter <= 0) {
            SetScientistPhase(ZombiePhase::PHASE_SCIENTIST_SHOOTING);
        }
        return;
    }

    if (!IsImmobilizied() && aHasTarget) {
        SetScientistPhase(mPhaseCounter <= 0 ? ZombiePhase::PHASE_SCIENTIST_SHOOTING : ZombiePhase::PHASE_SCIENTIST_WAITING);
    }
}

bool Zombie::IsValidTeleportationTarget() {
    return mHasHead && !IsDeadOrDying() && mZombieType != ZombieType::ZOMBIE_BUNGEE && mZombieType != ZombieType::ZOMBIE_BOSS && mZombieType != ZombieType::ZOMBIE_DOG && !IsBobsledTeamWithSled()
        && mZombiePhase != ZombiePhase::PHASE_DIGGER_TUNNELING && mZombiePhase != ZombiePhase::PHASE_POLEVAULTER_IN_VAULT && mZombiePhase != ZombiePhase::PHASE_GARGANTUAR_THROWING
        && mZombiePhase != ZombiePhase::PHASE_IMP_GETTING_THROWN && mZombiePhase != ZombiePhase::PHASE_IMP_GETTING_BLOCKED;
}

void Zombie::UpdatePlaying() {
    mGroanCounter--;
    int aZombiesCount = mBoard->mZombies.mSize;
    if (mGroanCounter == 0 && Rand(aZombiesCount) == 0 && mHasHead && mZombieType != ZombieType::ZOMBIE_BOSS && !mBoard->HasLevelAwardDropped()) {
        float aPitch = 0.0f;
        if (mApp->IsLittleTroubleLevel()) {
            aPitch = RandRangeFloat(40.0f, 50.0f);
        }

        if (IsGargantuar()) {
            mApp->PlayFoley(FoleyType::FOLEY_LOW_GROAN);
        } else if (mVariant) {
            mApp->PlayFoleyPitch(FoleyType::FOLEY_BRAINS, aPitch);
        } else if (mApp->mSukhbirMode) {
            mApp->PlayFoleyPitch(FoleyType::FOLEY_SUKHBIR, aPitch);
        } else {
            mApp->PlayFoleyPitch(FoleyType::FOLEY_GROAN, aPitch);
        }

        mGroanCounter = Rand(1000) + 500;
    }

    if (mIceTrapCounter > 0) {
        mIceTrapCounter--;
        if (mIceTrapCounter == 0) {
            RemoveIceTrap();
            AddAttachedParticle(75, 106, ParticleEffect::PARTICLE_ICE_TRAP_RELEASE);
        }
    }
    if (mChilledCounter > 0) {
        mChilledCounter--;
        if (mChilledCounter == 0) {
            UpdateAnimSpeed();
        }
    }
    if (mButteredCounter > 0) {
        mButteredCounter--;
        if (mButteredCounter == 0) {
            RemoveButter();
        }
    }

    if (mPoisoned) {
        StopEating();

        if (mPoisonedCounter <= 0) {
            mPoisonedCounter = 0;

            if (mHasHead) {
                DropHead(0U);
            }

            StopZombieSound();
            PlayDeathAnim(0U);
        }

        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE) {
        UpdateZombieRiseFromGrave();
        return;
    }

    if ((mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) && mRelatedZombieID != ZombieID::ZOMBIEID_NULL) {
        CheckDogPartnerDeath();
    }

    if (mZombieType == ZombieType::ZOMBIE_EXPLORER) {
        UpdateExplorerProjectiles();
    }

    if (IsImmobilizied()) {
        if (mZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR) {
            InterruptLightning();
        } else if (mZombieType == ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR) {
            InterruptSuperNovaDestroy();
        }
    }

    if (!IsImmobilizied()) {
        UpdateActions();
        UpdateZombiePosition();
        CheckIfPreyCaught();
        CheckForPool();
        CheckForHighGround();
        CheckForBoardEdge();
    }

    if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        UpdateBoss();
    }

    if (!IsDeadOrDying() && mFromWave != Zombie::ZOMBIE_WAVE_WINNER) {
        bool isDying = !mHasHead;
        if (mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombieType == ZombieType::ZOMBIE_CATAPULT) {
            if (mBodyHealth < 200) {
                isDying = true;
            }
        }

        if (isDying) {
            int aDamage = 1;
            if (mZombieType == ZombieType::ZOMBIE_YETI) {
                aDamage = 10;
            }
            if (mBodyMaxHealth >= 500) {
                aDamage = 3;
            }

            if (Rand(5) == 0) {
                TakeDamage(aDamage, 9U);
            }
        }
    }
}

Zombie *Zombie::GetDogPartner() const {
    if (mBoard == nullptr || mRelatedZombieID == ZombieID::ZOMBIEID_NULL) {
        return nullptr;
    }

    Zombie *aPartner = mBoard->ZombieTryToGet(mRelatedZombieID);
    if (aPartner == nullptr) {
        return nullptr;
    }

    const bool aValidPair = (mZombieType == ZombieType::ZOMBIE_DOGWALKER && aPartner->mZombieType == ZombieType::ZOMBIE_DOG)
        || (mZombieType == ZombieType::ZOMBIE_DOG && aPartner->mZombieType == ZombieType::ZOMBIE_DOGWALKER);
    return aValidPair ? aPartner : nullptr;
}

void Zombie::HandleDogPartnerLost() {
    Zombie *aPartner = GetDogPartner();
    if (aPartner != nullptr) {
        aPartner->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
    }
    mRelatedZombieID = ZombieID::ZOMBIEID_NULL;

    if (IsDeadOrDying()) {
        return;
    }

    StopEating();
    if (mZombieType == ZombieType::ZOMBIE_DOGWALKER) {
        if (mZombiePhase == ZombiePhase::PHASE_DOGWALKER_ROPE_BREAK) {
            return;
        }
        mZombiePhase = ZombiePhase::PHASE_DOGWALKER_ROPE_BREAK;
        ReanimShowTrack("Zombie_dogwalker_rope2_2", RENDER_GROUP_NORMAL);
        PlayZombieReanim("anim_ropebreak", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 18.0f);
        return;
    }

    if (mZombieType == ZombieType::ZOMBIE_DOG) {
        mZombiePhase = ZombiePhase::PHASE_DOG_RUNNING;
        StartWalkAnim(20);
    }
}

void Zombie::CheckDogPartnerDeath() {
    if (mRelatedZombieID == ZombieID::ZOMBIEID_NULL) {
        return;
    }

    Zombie *aPartner = GetDogPartner();
    if (aPartner == nullptr || aPartner->IsDeadOrDying() || !aPartner->mHasHead) {
        HandleDogPartnerLost();
        return;
    }

    if (aPartner->mMindControlled != mMindControlled) {
        Zombie *aWalker = mZombieType == ZombieType::ZOMBIE_DOGWALKER ? this : aPartner;
        Zombie *aDog = mZombieType == ZombieType::ZOMBIE_DOG ? this : aPartner;

        aWalker->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
        aDog->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;

        aWalker->HandleDogPartnerLost();
        aDog->HandleDogPartnerLost();
    }
}

void Zombie::UpdateDogWalker() {
    if (mZombiePhase == ZombiePhase::PHASE_DOGWALKER_ROPE_BREAK) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim != nullptr && aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(0);
            BreakRope();
        }
        return;
    }

    Zombie *aDog = GetDogPartner();
    if (aDog == nullptr || aDog->IsDeadOrDying()) {
        return;
    }

    if (mRow != aDog->mRow) {
        SetRow(aDog->mRow);
    }
}

Plant *Zombie::FindDogTarget() {
    const int aRowCount = mBoard->StageHas6Rows() ? 6 : 5;
    const int aHomeRow = ClampInt(mTargetRow, 0, aRowCount - 1);

    const Rect aDogRect = GetZombieRect();
    const int aDogCenterX = aDogRect.mX + aDogRect.mWidth / 2;
    const int aDogCenterY = aDogRect.mY + aDogRect.mHeight / 2;
    const int aDogCol = mBoard->PixelToGridX(aDogCenterX, aDogCenterY);
    const int aFrontCol = aDogCol + (IsWalkingBackwards() ? 1 : -1);

    Plant *aBestPlant = nullptr;
    float aBestDistance = 1.0e30f;
    bool aBestIsSweetPotato = false;

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->NotOnGround()) {
            continue;
        }
        if (aPlant->mRow < aHomeRow - 1 || aPlant->mRow > aHomeRow + 1) {
            continue;
        }
        if (aPlant->mRow < mRow - 1 || aPlant->mRow > mRow + 1) {
            continue;
        }
        if (aPlant->mPlantCol != aDogCol && aPlant->mPlantCol != aFrontCol) {
            continue;
        }
        if (!mBoard->RowCanHaveZombieType(aPlant->mRow, ZombieType::ZOMBIE_DOG)) {
            continue;
        }
        if (!CanTargetPlant(aPlant, ZombieAttackType::ATTACKTYPE_CHEW)) {
            continue;
        }

        const float aPlantCenterX = aPlant->mX + aPlant->mWidth * 0.5f;
        const float aForwardDistance = IsWalkingBackwards() ? (aPlantCenterX - aDogCenterX) : (aDogCenterX - aPlantCenterX);
        if (aForwardDistance < -20.0f) {
            continue;
        }

        const float aDistance = std::max(0.0f, aForwardDistance);
        const bool aIsSweetPotato = aPlant->mSeedType == SeedType::SEED_SWEET_POTATO && mBoard->GetLadderAt(aPlant->mPlantCol, aPlant->mRow) == nullptr;
        if (aBestPlant == nullptr || (aIsSweetPotato && !aBestIsSweetPotato)
            || (aIsSweetPotato == aBestIsSweetPotato && (aDistance < aBestDistance || (std::fabs(aDistance - aBestDistance) < 0.01f && aPlant->mRow == mRow && aBestPlant->mRow != mRow)))) {
            aBestPlant = aPlant;
            aBestDistance = aDistance;
            aBestIsSweetPotato = aIsSweetPotato;
        }
    }

    return aBestPlant;
}

void Zombie::SetDogPairRow(int theRow) {
    const int aRowCount = mBoard->StageHas6Rows() ? 6 : 5;
    if (theRow < 0 || theRow >= aRowCount || theRow == mRow) {
        return;
    }

    StopEating();
    Zombie *aPartner = GetDogPartner();
    if (aPartner != nullptr && !aPartner->IsDeadOrDying()) {
        aPartner->StopEating();
    }

    // Zombie::SetRow() 已负责同步仍存活的狗/主人，避免双方重复调用
    // SetRow() 导致同一帧内重复刷新行坐标。
    SetRow(theRow);

    if (IsRemoteServer()) {
        U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_SET_ROW;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2 = uint16_t(theRow);
        netplay::PutEvent(event);
    }
}

void Zombie::UpdateZombieDog() {
    Zombie *aWalker = GetDogPartner();
    if (aWalker == nullptr) {
        return;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (mIsEating || aWalker->mIsEating) {
        mPhaseCounter = 300;
        return;
    }

    if (aWalker->IsImmobilizied() || (aWalker->mYuckyFace && aWalker->mYuckyFaceCounter <= 169)) {
        return;
    }

    const bool aChangingRow = IsChangingRow() || aWalker->IsChangingRow();
    if (aChangingRow) {
        // 由追猎目标引发的换行完成前保持完整的 300 帧回程等待。
        if (mPhaseCounter > 0) {
            mPhaseCounter = 300;
        }
        return;
    }

    const int aRowCount = mBoard->StageHas6Rows() ? 6 : 5;
    const int aHomeRow = ClampInt(mTargetRow, 0, aRowCount - 1);
    Plant *aTarget = FindDogTarget();
    if (aTarget != nullptr) {
        // 只要追猎范围内仍有目标，就持续刷新回程倒计时。
        mPhaseCounter = 300;
        if (aTarget->mRow != mRow) {
            SetDogPairRow(aTarget->mRow);
        }
        return;
    }

    if (mRow == aHomeRow) {
        mPhaseCounter = -1;
        return;
    }

    // 大蒜等外部机制将组合移离出生行时，首次稳定到达新行后才启动 300 帧等待，不会在没有追猎目标时立即折返。
    if (mPhaseCounter < 0) {
        mPhaseCounter = 300;
        return;
    }

    if (mPhaseCounter == 0) {
        SetDogPairRow(aHomeRow);
        mPhaseCounter = -1;
    }
}


void Zombie::LandFlyer(unsigned int theDamageFlags) {
    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY) && mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING) {
        mApp->PlaySample(SOUND_BALLOON_POP);
        mZombiePhase = ZombiePhase::PHASE_BALLOON_POPPING;
        PlayZombieReanim("anim_pop", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
    }

    if (mBoard->mPlantRow[mRow] == PlantRowType::PLANTROW_POOL) {
        DieWithLoot();
    } else {
        mZombieHeight = ZombieHeight::HEIGHT_FALLING;
    }
}

void Zombie::UpdateZombieFlyer() {
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY && mPosX < 720.0f) {
        mAltitude -= 0.1f;
        if (mAltitude < -35.0f) {
            LandFlyer(0U);
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_BALLOON_POPPING) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_BALLOON_WALKING;
            StartWalkAnim(0);
        }
    }

    if (mApp->IsIZombieLevel() && mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING && mBoard->mChallenge->IZombieGetBrainTarget(this)) {
        LandFlyer(0U);
    }

    if (mApp->IsVSMode()) {
        float aMaxAltitude = 50.0f;
        if (mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING) {
            Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
            if (aPlant) {
                if (aPlant->mSeedType == SeedType::SEED_TALLNUT) {
                    LandFlyer(0U);
                    return;
                }
                if (mAltitude < aMaxAltitude) {
                    if (mAltitude == 0) {
                        mApp->PlayFoley(FoleyType::FOLEY_BALLOONINFLATE);
                    }
                    mAltitude++;
                    mVelX = 0.0f;
                    UpdateAnimSpeed();
                } else {
                    mPhaseCounter = 50;
                    PickRandomSpeed();
                }
            } else {
                if (mPhaseCounter <= 0 && mAltitude > 0) {
                    mAltitude--;
                    if (mAltitude <= 0) {
                        mAltitude = 0;
                        PickRandomSpeed();
                    }
                }
            }
        }
    }
}

void Zombie::UpdateYeti() {
    if (mMindControlled || !mHasHead || IsDeadOrDying())
        return;

    if (mApp->IsVSMode() || IsOnlineModeActive()) { // 联机时使用可同步的雪人状态逻辑
        if (mZombiePhase == PHASE_YETI_PRE_RUN) {
            mPhaseCounter = RandRangeInt(1500, 2000);
            mHasObject = true;
            mZombiePhase = PHASE_ZOMBIE_NORMAL;
        } else if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_NORMAL) {
            if (mPhaseCounter == 500 && IsRemoteServer()) {
                U8U8U16U16_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
                event.data1 = uint8_t(mZombiePhase);
                event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data4 = uint16_t(mPhaseCounter);
                netplay::PutEvent(event);
            }
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_NORMAL && mPhaseCounter == 0) {
        mZombiePhase = ZombiePhase::PHASE_YETI_RUNNING;
        mHasObject = false;
        PickRandomSpeed();
    }
}

void Zombie::UpdateZombieImp() {
    if (mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN) {
        mVelZ -= THOWN_ZOMBIE_GRAVITY;
        mAltitude += mVelZ;
        mPosX -= mVelX;

        float aDiffY = GetPosYBasedOnRow(mRow) - mPosY;
        mPosY += aDiffY;
        mAltitude += aDiffY;
        if (mAltitude <= 0.0f) {
            mAltitude = 0.0f;
            mZombiePhase = ZombiePhase::PHASE_IMP_LANDING;
            PlayZombieReanim("anim_land", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_LANDING) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(0);
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_IMP_PRE_RUN) {
        mZombiePhase = PHASE_IMP_RUNNING;
        PickRandomSpeed();
    }
}

void Zombie::UpdateSuperFanImp() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    if (Zombie *aZombie = FindZombieGigaFootball()) {
        mRelatedZombieID = mBoard->ZombieGetID(aZombie);
    }

    if (Zombie *aZombie = mBoard->ZombieTryToGet(mRelatedZombieID)) {
        if (!IsRemoteClientOrViewer()) {
            bool isKicked = false;
            Plant *aPlant = aZombie->FindCatapultTarget();

            if (aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_CHARGING) {
                if (mApp->IsVSMode() && aPlant != nullptr) {
                    mTargetCol = aPlant->mPlantCol;
                }
                isKicked = true;
            } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_WALKING) {
                if (mApp->IsVSMode() && aPlant != nullptr) {
                    mTargetCol = aPlant->mPlantCol;
                }
                aZombie->StopEating();
                aZombie->mZombiePhase = PHASE_FOOTBALL_KICKING;
                aZombie->PlayZombieReanim("anim_kick", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 20.0f);

                mApp->PlaySample(addonSounds.whistle);

                if (IsRemoteServer()) {
                    U8U8U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
                    event.data1 = uint8_t(aZombie->mZombiePhase);
                    event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(aZombie));
                    event.data4 = uint16_t(aZombie->mPhaseCounter);
                    netplay::PutEvent(event);
                }
            } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_KICKING) {
                Reanimation *aBodyReanim = mApp->ReanimationTryToGet(aZombie->mBodyReanimID);
                if (aBodyReanim && aBodyReanim->ShouldTriggerTimedEvent(0.35f)) {
                    mApp->PlayFoley(FoleyType::FOLEY_THUMP);
                    isKicked = true;
                }
            }

            if (isKicked) {
                mApp->PlayFoley(FoleyType::FOLEY_SWING);

                mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_THROWN;
                int aTargetX = 0;
                if (mApp->IsVSMode()) {
                    aTargetX = mBoard->GridToPixelX(mTargetCol, mRow) - 25;
                } else {
                    aTargetX = mBoard->GridToPixelX(RandRangeInt(0, 2), mRow) - 25;
                }
                float aKickingDistance = mPosX - aTargetX;
                if (mBoard->StageHasRoof()) {
                    aKickingDistance -= 60.0f;
                }
                if (aKickingDistance < 20.0f) {
                    aKickingDistance = 20.0f;
                }
                int aFlightFrames = std::lround(aKickingDistance * mScaleZombie / 0.55f);
                if (aFlightFrames < 48) {
                    aFlightFrames = 48;
                } else if (aFlightFrames > 192) {
                    aFlightFrames = 192;
                }
                mVelX = aKickingDistance / float(aFlightFrames);
                auto aFlightFramesF = float(aFlightFrames);
                float aStartAltitude = mAltitude;
                mVelZ = (-aStartAltitude + KICKED_ZOMBIE_GRAVITY * aFlightFramesF * (aFlightFramesF + 1.0f) * 0.5f) / aFlightFramesF;
                PlayZombieReanim("anim_thrown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
                UpdateReanim();
                mApp->PlayFoley(FoleyType::FOLEY_IMP);
                mRelatedZombieID = ZombieID::ZOMBIEID_NULL;

                if (IsRemoteServer()) {
                    U16UNI32UNI32_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_IMP_KICKED;
                    event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                    event.data2.f32 = aKickingDistance;
                    event.data3.f32 = mPosX;
                    netplay::PutEvent(event);
                }
            }
        }
    }

    bool doPop = false;
    if (mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN) {
        mVelZ -= KICKED_ZOMBIE_GRAVITY;
        mAltitude += mVelZ;
        mPosX -= mMindControlled ? -mVelX : mVelX;

        float aDiffY = GetPosYBasedOnRow(mRow) - mPosY;
        mPosY += aDiffY;
        mAltitude += aDiffY;

        if (mAltitude > 20.0f)
            return;

        int aPosX = mX + mWidth / 2;
        int aPosY = mY + mHeight / 2;
        int aGridX = mBoard->PixelToGridXKeepOnBoard(aPosX, aPosY);
        Plant *aPlant = mBoard->FindUmbrellaPlant(aGridX, mRow);
        if (aPlant && !mMindControlled) {
            mApp->PlaySample(SOUND_BOING);
            mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);

            aPlant->DoSpecial();

            mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_BLOCKED;
            mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
            mHitUmbrella = true;

            float aBackDistance = std::max(0.0f, float(mTargetRow) - mPosX);
            float aBackFrames = std::max(20.0f, std::min(56.0f, aBackDistance / 2.2f));
            mVelX = (aBackFrames > 0.0f) ? (aBackDistance / aBackFrames) : 0.0f;
            float aFramesF = std::max(1.0f, aBackFrames);
            mVelZ = (THOWN_ZOMBIE_GRAVITY * aFramesF * (aFramesF + 1.0f) * 0.5f - mAltitude) / aFramesF;

            return;
        }

        if (mAltitude <= 0.0f) {
            mAltitude = 0.0f;
            mZombiePhase = ZombiePhase::PHASE_IMP_LANDING;
            PlayZombieReanim("anim_land", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_BLOCKED) {
        mVelZ -= THOWN_ZOMBIE_GRAVITY;
        mAltitude += mVelZ;
        if (mPosX < mTargetRow) {
            mPosX += fabsf(mVelX);
            if (mPosX > mTargetRow) {
                mPosX = float(mTargetRow);
            }
        } else {
            mPosX = float(mTargetRow);
        }

        float aDiffY = GetPosYBasedOnRow(mRow) - mPosY;
        mPosY += aDiffY;
        mAltitude += aDiffY;

        if (mPosX >= mTargetRow && mAltitude <= 0.0f) {
            mAltitude = 0.0f;
            mZombiePhase = ZombiePhase::PHASE_IMP_LANDING;
            PlayZombieReanim("anim_land", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            return;
        }

        if (mAltitude < 0.0f && mPosX < mTargetRow) {
            mAltitude = 0.0f;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_LANDING) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim && aBodyReanim->mLoopCount > 0) {
            mHitUmbrella = false;
            doPop = true;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_PRE_RUN) {
        mZombiePhase = PHASE_IMP_RUNNING;
        PickRandomSpeed();
        if (mApp->IsVSMode()) {
            mPhaseCounter = RandRangeInt(1100, 1650);
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_RUNNING) {
        if (mApp->IsVSMode()) {
            if (mIsEating) {
                //                Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
                //                if (aPlant != nullptr && Plant::IsDefender(aPlant->mSeedType)) {
                //                    ++mPhaseCounter;
                //                }
                if (mPhaseCounter <= 0) {
                    doPop = true;
                }
            }
        } else {
            int aTargetX = mBoard->GridToPixelX(mTargetCol, mRow) + RandRangeInt(-20, 60);
            if (mX + mWidth / 2 <= aTargetX) {
                doPop = true;
            }
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_POPPING) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim && aBodyReanim->ShouldTriggerTimedEvent(0.20f)) {
            DoSpecial();
            DieNoLoot();
        }
    }

    if (doPop) {
        if (IsRemoteClientOrViewer()) {
            return;
        }

        mZombiePhase = ZombiePhase::PHASE_IMP_POPPING;
        PlayZombieReanim("anim_explode", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);

        if (IsRemoteServer()) {
            U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_IMP_POP;
            event.data = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            netplay::PutEvent(event);
        }
    }
}

void Zombie::UpdateGigaFootball() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    auto syncFootballPhaseCounter = [this]() {
        if (IsRemoteServer()) {
            U8U8U16U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
            event.data1 = uint8_t(mZombiePhase);
            event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data4 = uint16_t(mPhaseCounter);
            netplay::PutEvent(event);
        }
    };

    if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_PRE_CHARGE) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_FOOTBALL_CHARGING;
            StartWalkAnim(0);
            mApp->PlaySample(addonSounds.whistle);
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_CHARGING) {
        //        if (mButteredCounter > 0 || mIceTrapCounter > 0) {
        //            mZombiePhase = ZombiePhase::PHASE_FOOTBALL_WALKING;
        //            mPhaseCounter = RandRangeInt(1000, 1500) / 2;
        //        }

        if (IsRemoteClientOrViewer())
            return;

        bool doTackle = false;
        if (FindZombieTarget()) {
            doTackle = true;
        } else if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
            if (mBoard->GetLadderAt(aPlant->mPlantCol, aPlant->mRow)) {
                float aPlantX = mBoard->GridToPixelX(aPlant->mPlantCol, aPlant->mRow) + 40;
                if (aPlantX > mPosX && mZombieHeight == ZombieHeight::HEIGHT_ZOMBIE_NORMAL && mUseLadderCol != aPlant->mPlantCol) {
                    mZombieHeight = ZombieHeight::HEIGHT_UP_LADDER;
                    mUseLadderCol = aPlant->mPlantCol;
                }
                return;
            }
            doTackle = true;
        }

        if (doTackle) {
            mZombiePhase = ZombiePhase::PHASE_FOOTBALL_TACKLING;
            PlayZombieReanim("anim_tackle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            syncFootballPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_TACKLING) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.15f)) {
            int aMindCtrlIndex = mMindControlled ? -1 : 1;
            mApp->PlayFoley(FoleyType::FOLEY_ALLSTAR_TACKLE);
            mBoard->ShakeBoard(-4 * aMindCtrlIndex, 2);
            mPosX += 20.0f * aMindCtrlIndex;

            if (Zombie *aZombie = FindZombieTarget()) {
                aZombie->TakeDamage(1500, 0U);
            }

            if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
                SeedType aSeedType = aPlant->mSeedType;
                if (aPlant->IsInvulnerable()) {
                    return;
                }
                if (aSeedType == SEED_TALLNUT || aSeedType == SEED_TALLNUT) {
                    aPlant->mPlantHealth -= aPlant->mPlantHealth + 1; // 造成伤害可触发坚果损伤的粒子特效
                } else {
                    aPlant->Die();
                }
            }
        }

        if (aBodyReanim->mLoopCount > 0) {
            if (IsRemoteClientOrViewer())
                return;

            mZombiePhase = ZombiePhase::PHASE_FOOTBALL_WALKING;
            mPhaseCounter = RandRangeInt(1000, 1500);
            StartWalkAnim(0);
            syncFootballPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_WALKING) {
        if (IsRemoteClientOrViewer())
            return;

        if (mIsEating)
            mPhaseCounter++;

        if (mPhaseCounter <= 0) {
            mZombiePhase = ZombiePhase::PHASE_FOOTBALL_PRE_CHARGE;
            StartWalkAnim(0);
            syncFootballPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_KICKING) {
        if (IsRemoteClientOrViewer())
            return;

        mPhaseCounter++;

        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            StopEating();
            mZombiePhase = ZombiePhase::PHASE_FOOTBALL_WALKING;
            StartWalkAnim(0);
            syncFootballPhaseCounter();
        }
    }
}

void Zombie::UpdateZombieBackupDancer() {
    if (mIsEating)
        return;

    if (mZombiePhase == ZombiePhase::PHASE_DANCER_RISING) {
        mAltitude = TodAnimateCurve(150, 0, mPhaseCounter, ZOMBIE_BACKUP_DANCER_RISE_HEIGHT, 0, TodCurves::CURVE_LINEAR);

        if (mPhaseCounter != 0)
            return;

        if (IsOnHighGround()) {
            mAltitude = HIGH_GROUND_HEIGHT;
        }
    }

    ZombiePhase aDancerPhase = GetDancerPhase();
    if (aDancerPhase != mZombiePhase) {
        switch (aDancerPhase) {
            case ZombiePhase::PHASE_DANCER_DANCING_LEFT: {
                mZombiePhase = aDancerPhase;
                PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                break;
            }
            case ZombiePhase::PHASE_DANCER_WALK_TO_RAISE:
                mZombiePhase = aDancerPhase;
                PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                mApp->ReanimationTryToGet(mBodyReanimID)->mAnimTime = 0.6f;
                break;
            case ZombiePhase::PHASE_DANCER_RAISE_RIGHT_1:
            case ZombiePhase::PHASE_DANCER_RAISE_LEFT_2:
                mZombiePhase = aDancerPhase;
                PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                break;
            default:
                break;
        }
    }
}

void Zombie::UpdateZombieJackson() {
    if (mSummonCounter > 0) {
        mSummonCounter--;
    }

    if (mIsEating)
        return;

    if (mSummonCounter == 0 && !mMindControlled) {
        if (!msDeadFollowers.empty() && GetDancerFrame() == 12 && mHasHead && mPosX < 700.0f) {
            if (!IsRemoteClientOrViewer()) {
                mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT;
                PlayZombieReanim("anim_point", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            }
            if (IsRemoteServer()) {
                U8U8U16U16_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
                event.data1 = uint8_t(mZombiePhase);
                event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data4 = uint16_t(mPhaseCounter);
                netplay::PutEvent(event);
            }
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN) {
        if (mHasHead && mPhaseCounter == 0) {
            mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS;
            PlayZombieReanim("anim_point", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            PickRandomSpeed();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            SummonBackupDancers();
            mBoard->SetDanceMode(true);
            mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD;
            mPhaseCounter = 200;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            mApp->PlayFoley(FoleyType::FOLEY_THRILLER);
            mBoard->DisplayAdviceAgain("[THRILLER_NIGHT]", MessageStyle::MESSAGE_STYLE_HUGE_WAVE, AdviceType::ADVICE_HUGE_WAVE);

            RaiseDeadZombies();
            mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD;
            mPhaseCounter = 200;
            mSummonCounter = -1;
        }
    } else {
        if (mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD) {
            if (mPhaseCounter != 0)
                return;

            PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 20, 18.0f);
            mZombiePhase = ZombiePhase::PHASE_DANCER_DANCING_LEFT;
            UpdateAnimSpeed();

            for (auto &i : mFollowerZombieID) {
                Zombie *aDancer = mBoard->ZombieTryToGet(i);

                if (aDancer && !aDancer->IsDeadOrDying() && !aDancer->IsImmobilizied() && !aDancer->mIsEating) {
                    aDancer->PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 20, 18.0f);
                    aDancer->mZombiePhase = ZombiePhase::PHASE_DANCER_DANCING_LEFT;
                    UpdateAnimSpeed();
                }
            }
        }

        ZombiePhase aDancerPhase = GetDancerPhase();
        if (aDancerPhase != mZombiePhase) {
            switch (aDancerPhase) {
                case ZombiePhase::PHASE_DANCER_DANCING_LEFT: {
                    mZombiePhase = aDancerPhase;
                    PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                    break;
                }
                case ZombiePhase::PHASE_DANCER_WALK_TO_RAISE:
                    mZombiePhase = aDancerPhase;
                    PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                    mApp->ReanimationTryToGet(mBodyReanimID)->mAnimTime = 0.6f;
                    break;

                case ZombiePhase::PHASE_DANCER_RAISE_RIGHT_1:
                case ZombiePhase::PHASE_DANCER_RAISE_LEFT_2:
                    mZombiePhase = aDancerPhase;
                    PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                    break;
                default:
                    break;
            }
        }
    }
}

bool Zombie::CanRevived() const {
    if (mIsRevived) {
        return false;
    }

    return mZombieType != ZombieType::ZOMBIE_DANCER && mZombieType != ZombieType::ZOMBIE_SNORKEL && mZombieType != ZombieType::ZOMBIE_ZAMBONI && mZombieType != ZombieType::ZOMBIE_BOBSLED
        && mZombieType != ZombieType::ZOMBIE_DOLPHIN_RIDER && mZombieType != ZombieType::ZOMBIE_BALLOON && mZombieType != ZombieType::ZOMBIE_DIGGER && mZombieType != ZombieType::ZOMBIE_POGO
        && mZombieType != ZombieType::ZOMBIE_BUNGEE && mZombieType != ZombieType::ZOMBIE_CATAPULT && !IsGargantuar() && mZombieType != ZombieType::ZOMBIE_BOSS
        && mZombieType != ZombieType::ZOMBIE_JACKSON && mZombieType != ZombieType::ZOMBIE_DOGWALKER && mZombieType != ZombieType::ZOMBIE_DOG && !IsZomblob(mZombieType);
}

ZombieID Zombie::RaiseDeadZombie(ZombieType theZombieType, int theRow, int theCol) {
    if (!mBoard->RowCanHaveZombieType(theRow, theZombieType))
        return ZombieID::ZOMBIEID_NULL;

    Zombie *aZombie = mBoard->AddZombie_Origin(theZombieType, mFromWave, false);
    if (aZombie == nullptr)
        return ZombieID::ZOMBIEID_NULL;

    aZombie->mIsRevived = true;
    aZombie->RiseFromGrave(theCol, theRow);
    aZombie->mBodyHealth -= aZombie->mBodyMaxHealth / 3;
    if (aZombie->mHelmHealth > 0) {
        aZombie->TakeHelmDamage(aZombie->mHelmMaxHealth * 2 / 3 + 2, 0U);
    }
    if (aZombie->mShieldHealth > 0) {
        aZombie->TakeShieldDamage(aZombie->mShieldMaxHealth * 2 / 3 + 2, 0U);
    }
    aZombie->mHasArm = false;
    aZombie->SetupLostArmReanim();
    aZombie->mMindControlled = mMindControlled;
    return mBoard->ZombieGetID(aZombie);
}

void Zombie::RaiseDeadZombies() {
    if (IsRemoteClientOrViewer())
        return;

    std::vector<ZombieType> deadFollowers = msDeadFollowers;
    std::vector<uint16_t> revivedServerIDs;
    std::vector<float> revivedVelXs;

    for (int i = 0; i < msDeadFollowers.size(); i++) {
        if (!msDeadFollowers.empty()) {
            int aRow = 0, aCol = 0;
            if (i < 5) {
                aRow = i;
                aCol = mMindControlled ? 4 : 6;
            } else if (i < 10) {
                aRow = i - 5;
                aCol = mMindControlled ? 3 : 7;
            } else {
                aRow = i - 10;
                aCol = mMindControlled ? 2 : 8;
            }

            ZombieID revivedID = RaiseDeadZombie(msDeadFollowers[i], aRow, aCol);
            Zombie *revivedZombie = mBoard->ZombieTryToGet(revivedID);
            revivedServerIDs.push_back(uint16_t(revivedID));
            revivedVelXs.push_back(revivedZombie ? revivedZombie->mVelX : 0.0f);
        }
    }
    msDeadFollowers.clear();

    if (IsRemoteServer()) {
        U16UNI32U8x16U16x15UNI32x15_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_RAISE_DEAD;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2.f32 = mPosX;
        event.data3[0] = uint8_t(std::min<size_t>(15, std::min(deadFollowers.size(), revivedServerIDs.size())));
        for (int i = 0; i < event.data3[0]; ++i) {
            event.data3[i + 1] = uint8_t(deadFollowers[i]);
            event.data4[i] = revivedServerIDs[i];
            event.data5[i].f32 = revivedVelXs[i];
        }
        netplay::PutEvent(event);
    }
}

void Zombie::JacksonDie() {
    if (!IsOnBoard())
        return;

    if (!mBoard->GetAliveJacksonZombie()) {
        mBoard->SetDanceMode(false);
        msDeadFollowers.clear();
    }
    mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_DANCER);
    mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_THRILLER);
}

void Zombie::SquishAllInSquare(int theX, int theY, ZombieAttackType theAttackType) {
    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (IsRemoteServer()) {
        U16UNI32_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_SQUISH_ALL_IN_SQUARE;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2.u8x4.u8_1 = uint8_t(theX);
        event.data2.u8x4.u8_2 = uint8_t(theY);
        event.data2.u8x4.u8_3 = uint8_t(theAttackType);
        netplay::PutEvent(event);
    }

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == theY && aPlant->mPlantCol == theX) {
            if (theAttackType == ZombieAttackType::ATTACKTYPE_DRIVE_OVER && aPlant->IsSpiky()) {
                continue;
            }

            if (aPlant->mSeedType != SeedType::SEED_SPIKEROCK) {
                mBoard->mPlantsEaten++;
                aPlant->Squish();
            }
        }
    }
}

void Zombie::UpdateZombieJackInTheBox() {
    if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_PRE_RUN) {
        int aDistance = 450 + Rand(300);
        if (Rand(20) == 0) // 早爆的概率
        {
            aDistance /= 3;
        }
        mPhaseCounter = (int)(aDistance / mVelX) * ZOMBIE_LIMP_SPEED_FACTOR;
        mZombiePhase = ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING;
    }

    if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_RUNNING) {
        if (mHasHead) {
            bool doPop = false;
            if (!IsRemoteClientOrViewer()) {
                if (mApp->IsVSMode()) {
                    if (VSSetupAddonWidget::msBalancePatchMode) {
                        if (mIsEating) {
                            Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
                            if (aPlant != nullptr && !Plant::IsDefender(aPlant->mSeedType)) {
                                ++mPhaseCounter;
                            }
                        }
                        if (mPhaseCounter <= 0) {
                            if (mMindControlled) {
                                doPop = true;
                            } else {
                                int aPosX = mX + mWidth / 2;
                                int aPosY = mY + mHeight / 2;
                                Plant *aPlant = nullptr;
                                while (mBoard->IteratePlants(aPlant)) {
                                    if (aPlant->NotOnGround()) {
                                        continue;
                                    }
                                    if (GetCircleRectOverlap(aPosX, aPosY, JackInTheBoxPlantRadius, aPlant->GetPlantRect())) {
                                        doPop = true;
                                    }
                                }
                            }
                        }
                    } else {
                        doPop = mIsEating;
                    }
                } else {
                    doPop = (mPhaseCounter <= 0);
                }
            }
            if (doPop) {
                mPhaseCounter = 110;
                mZombiePhase = ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING;

                StopZombieSound();
                mApp->PlaySample(SOUND_BOING);
                PlayZombieReanim("anim_pop", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 28.0f);

                U8U8U16U16_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
                event.data1 = uint8_t(mZombiePhase);
                event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data4 = uint16_t(mPhaseCounter);
                netplay::PutEvent(event);
            }
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING) {
        if (mPhaseCounter == 80) {
            mApp->PlayFoley(FoleyType::FOLEY_JACK_SURPRISE);
        }

        if (mPhaseCounter <= 0) {
            if (IsRemoteClientOrViewer())
                return;

            DoSpecial();
        }
    }
}


void Zombie::UpdateZombiePolevaulter() {
    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT && mHasHead && mZombieHeight == ZombieHeight::HEIGHT_ZOMBIE_NORMAL) {
        Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT);
        if (aPlant) {
            if (mBoard->GetLadderAt(aPlant->mPlantCol, aPlant->mRow)) {
                float aPlantX = mBoard->GridToPixelX(aPlant->mPlantCol, aPlant->mRow) + 40;
                if (aPlantX > mPosX && mZombieHeight == ZombieHeight::HEIGHT_ZOMBIE_NORMAL && mUseLadderCol != aPlant->mPlantCol) {
                    mZombieHeight = ZombieHeight::HEIGHT_UP_LADDER;
                    mUseLadderCol = aPlant->mPlantCol;
                }
                return;
            }

            if (IsRemoteClientOrViewer()) {
                return;
            }

            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_IN_VAULT;
            PlayZombieReanim("anim_jump", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);

            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            float aAnimDuration = aBodyReanim->mFrameCount / aBodyReanim->mAnimRate * 100.0f;
            int aJumpDistance = mX - aPlant->mX - 80;
            if (mApp->IsWallnutBowlingLevel()) {
                aJumpDistance = 0;
            }
            mVelX = aJumpDistance / aAnimDuration;
            mHasObject = false;

            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_IN_VAULT;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.i16x2.i16_1 = int16_t(mX);
                event.data2.i16x2.i16_2 = int16_t(aJumpDistance);
                netplay::PutEvent(event);
            }
        }

        if (mApp->IsIZombieLevel() && mBoard->mChallenge->IZombieGetBrainTarget(this)) {
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
            StartWalkAnim(0);
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);

        bool aJumpEnds = false;
        if (!IsRemoteClientOrViewer()) {
            if (aBodyReanim->mAnimTime > 0.6f && aBodyReanim->mAnimTime <= 0.7f) {
                Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT);
                if (aPlant && aPlant->mSeedType == SeedType::SEED_TALLNUT) {
                    mApp->PlayFoley(FoleyType::FOLEY_BONK);
                    aJumpEnds = true;
                    mApp->AddTodParticle(aPlant->mX + 60, aPlant->mY - 20, mRenderOrder + 1, ParticleEffect::PARTICLE_TALL_NUT_BLOCK);

                    mZombieHeight = ZombieHeight::HEIGHT_FALLING;
                    mPosX = aPlant->mX;
                    mPosY -= 30.0f;
                }
            }

            if (aBodyReanim->mLoopCount > 0) {
                aJumpEnds = true;
                mPosX -= 150.0f;
            }
        }
        if (aBodyReanim->ShouldTriggerTimedEvent(0.2f)) {
            mApp->PlayFoley(FoleyType::FOLEY_GRASSSTEP);
        }
        if (aBodyReanim->ShouldTriggerTimedEvent(0.4f)) {
            mApp->PlayFoley(FoleyType::FOLEY_POLEVAULT);
        }

        if (aJumpEnds) {
            mX = (int)mPosX;
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
            mZombieAttackRect = Rect(50, 0, 20, 115);

            StartWalkAnim(0);

            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_POST_VAULT;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.f32 = mPosY;
                netplay::PutEvent(event);
            }
        } else {
            float aOldPosX = mPosX;
            mPosX -= 150.0f * aBodyReanim->mAnimTime;
            mPosY = GetPosYBasedOnRow(mRow);
            mPosX = aOldPosX;
        }
    }
}

void Zombie::UpdateZombieDolphinRider() {
    if (IsTangleKelpTarget()) {
        return;
    }

    auto syncDolphinPhaseCounter = [this]() {
        if (IsRemoteServer()) {
            U8U8U16U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
            event.data1 = uint8_t(mZombiePhase);
            event.data2 = uint8_t(mSummonCounter);
            event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data4 = uint16_t(mPhaseCounter);
            netplay::PutEvent(event);
        }
    };

    bool aWalkingBackwards = IsWalkingBackwards();
    switch (mZombiePhase) {
        case ZombiePhase::PHASE_DOLPHIN_WALKING:
            if (!aWalkingBackwards && mX >= 701 && mX <= 720) {
                mZombiePhase = ZombiePhase::PHASE_DOLPHIN_INTO_POOL;
                PlayZombieReanim("anim_jumpinpool", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
                if (!IsRemoteClientOrViewer()) {
                    syncDolphinPhaseCounter();
                }
            }
            break;

        case ZombiePhase::PHASE_DOLPHIN_INTO_POOL: {
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            if (aBodyReanim->ShouldTriggerTimedEvent(0.56f)) {
                Reanimation *aSplash = mApp->AddReanimation(mX - 83.0f, mY + 73.0f, mRenderOrder + 1, ReanimationType::REANIM_SPLASH);
                aSplash->OverrideScale(1.2f, 0.8f);
                mApp->AddTodParticle(mX - 46.0f, mY + 115.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PLANTING_POOL);
                mApp->PlayFoley(FoleyType::FOLEY_ZOMBIE_ENTERING_WATER);
            }

            if (!IsRemoteClientOrViewer() && aBodyReanim->mLoopCount > 0) {
                mZombiePhase = ZombiePhase::PHASE_DOLPHIN_RIDING;
                mInPool = true;
                mZombieAttackRect = Rect(-29, 0, 70, 115);
                mPosX -= 70.0f;
                PlayZombieReanim("anim_ride", ReanimLoopType::REANIM_LOOP_FULL_LAST_FRAME, 0, 12.0f);
                syncDolphinPhaseCounter();
            }
            break;
        }

        case ZombiePhase::PHASE_DOLPHIN_RIDING:
            if (mX <= 10) {
                if (IsRemoteClientOrViewer()) {
                    return;
                }
                mZombieHeight = ZombieHeight::HEIGHT_OUT_OF_POOL;
                mZombiePhase = ZombiePhase::PHASE_DOLPHIN_WALKING;
                mAltitude = -40.0f;
                PoolSplash(false);
                PlayZombieReanim("anim_walkdolphin", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
                syncDolphinPhaseCounter();
                PickRandomSpeed();
                return;
            }

            if (mHasHead && !IsTangleKelpTarget() && FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT)) {
                if (IsRemoteClientOrViewer()) {
                    return;
                }
                mApp->PlayFoley(FoleyType::FOLEY_DOLPHIN_BEFORE_JUMPING);
                mApp->PlayFoley(FoleyType::FOLEY_PLANT_WATER);
                mVelX = 0.5f;
                mPhaseCounter = 120;
                mZombiePhase = ZombiePhase::PHASE_DOLPHIN_IN_JUMP;
                PlayZombieReanim("anim_dolphinjump", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 10.0f);
                syncDolphinPhaseCounter();
            }
            break;

        case ZombiePhase::PHASE_DOLPHIN_IN_JUMP: {
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            mAltitude = TodAnimateCurveFloat(120, 0, mPhaseCounter, 0.0f, 10.0f, TodCurves::CURVE_LINEAR);

            if (!IsRemoteClientOrViewer() && aBodyReanim->ShouldTriggerTimedEvent(0.3f)) {
                Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT);
                if (aPlant && aPlant->mSeedType == SeedType::SEED_TALLNUT) {
                    mApp->PlayFoley(FoleyType::FOLEY_BONK);
                    mApp->AddTodParticle(aPlant->mX + 60.0f, aPlant->mY - 20.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_TALL_NUT_BLOCK);
                    mZombieHeight = ZombieHeight::HEIGHT_FALLING;
                    mAltitude = 30.0f;
                    mPosX = aPlant->mX + 25.0f;

                    mZombiePhase = ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL;
                    mZombieAttackRect = Rect(30, 0, 30, 115);
                    mZombieRect = Rect(20, 0, 42, 115);
                    mUnk95 = 0;
                    StartWalkAnim(0);
                    syncDolphinPhaseCounter();
                    return;
                }
            }

            if (aBodyReanim->ShouldTriggerTimedEvent(0.49f)) {
                Reanimation *aSplash = mApp->AddReanimation(mX - 63.0f, mY + 73.0f, mRenderOrder + 1, ReanimationType::REANIM_SPLASH);
                aSplash->OverrideScale(1.2f, 0.8f);
                mApp->AddTodParticle(mX - 26.0f, mY + 115.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PLANTING_POOL);
                mApp->PlayFoley(FoleyType::FOLEY_ZOMBIE_ENTERING_WATER);
                mVelX = 0.0f;
                return;
            }

            if (!IsRemoteClientOrViewer() && aBodyReanim->mLoopCount > 0) {
                mAltitude = 0.0f;
                mPosX -= 94.0f;
                mZombiePhase = ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL;
                mZombieAttackRect = Rect(30, 0, 30, 115);
                mZombieRect = Rect(20, 0, 42, 115);
                mUnk95 = 0;
                StartWalkAnim(0);
                syncDolphinPhaseCounter();
            }
            break;
        }

        case ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL:
            if ((mX > 10 && (mX <= 680 || !aWalkingBackwards)) || (mX <= 10 && aWalkingBackwards)) {
                return;
            }
            if (IsRemoteClientOrViewer()) {
                return;
            }

            mZombieHeight = ZombieHeight::HEIGHT_OUT_OF_POOL;
            mZombiePhase = ZombiePhase::PHASE_DOLPHIN_WALKING_WITHOUT_DOLPHIN;
            mAltitude = -40.0f;
            PoolSplash(false);
            PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
            PickRandomSpeed();
            syncDolphinPhaseCounter();
            return;

        default:
            return;
    }
}

GridItem *Zombie::FindPoleTarget() {
    if (mMindControlled) {
        return nullptr;
    }

    Rect aAttackRect = mZombieAttackRect;

    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_POST_VAULT) {
        aAttackRect = Rect(40, 0, 30, 115);
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PICK) {
        aAttackRect = Rect(30, 0, 40, 115);
    }

    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);

    aAttackRect.Offset(mX, mY + aDrawPos.mBodyY);

    if (aDrawPos.mClipHeight > CLIP_HEIGHT_LIMIT) {
        aAttackRect.mHeight -= aDrawPos.mClipHeight;
    }

    auto &aRelatedPoleID = reinterpret_cast<GridItemID &>(mRelatedZombieID);
    const ZombieID aZombieID = mBoard->ZombieGetID(this);

    GridItem *aBestPole = nullptr;
    int aBestDistance = BOARD_WIDTH;
    bool foundRelatedPole = false;

    GridItem *aPole = nullptr;
    while (mBoard->IterateGridItems(aPole)) {
        Rect aPoleRect = aPole->GetItemRect();
        if (aPole->mGridItemType != GridItemType::GRIDITEM_POLE || aPole->mGridY != mRow || GetRectOverlap(aAttackRect, aPoleRect) < 0) {
            continue;
        }

        GridItemID aPoleID = mBoard->GridItemGetID(aPole);
        const auto aRelatedZombieID = ZombieID(aPole->mZombieType);
        // 当杆子已被某一僵尸锁定时
        if (aRelatedZombieID != ZombieID::ZOMBIEID_NULL) {
            // 这根杆子不是自己已经找到的那根
            if (aPoleID != aRelatedPoleID) {
                continue;
            }
            foundRelatedPole = true;
            // 这跟杆子早已物有所主
            if (aRelatedZombieID != aZombieID) {
                return nullptr;
            }
            return aPole;
        }
        // 已经锁定了一根杆子
        if (aRelatedPoleID != GridItemID::GRIDITEMID_NULL) {
            continue;
        }

        int aDistance = std::abs(int(aPole->mPosX) - mX);

        if (aDistance < aBestDistance) {
            aBestDistance = aDistance;
            aBestPole = aPole;
        }
    }

    if (aRelatedPoleID != GridItemID::GRIDITEMID_NULL && !foundRelatedPole) {
        aRelatedPoleID = GridItemID::GRIDITEMID_NULL;
    }

    return aBestPole;
}

Plant *Zombie::FindGigaPolevaulterTarget() {
    Rect aAttackRect = Rect(-29, 0, BOARD_WIDTH, 115);

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == mRow && mX >= aPlant->mX + 100 && !aPlant->NotOnGround()) {
            Rect aPlantRect = aPlant->GetPlantRect();
            if (GetRectOverlap(aAttackRect, aPlantRect) >= 0 && CanTargetPlant(aPlant, ZombieAttackType::ATTACKTYPE_POLE)) {
                return aPlant;
            }
        }
    }

    return nullptr;
}

void Zombie::UpdateGigaPolevaulter() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    auto findNearestVaultPlant = [this]() -> Plant * {
        if (mMindControlled)
            return nullptr;

        Rect aAttackRect = GetZombieAttackRect();
        Plant *aBestPlant = nullptr;
        int aBestPlantLeft = -1;
        Plant *aBestTallnut = nullptr;
        int aBestTallnutLeft = -1;

        Plant *aPlant = nullptr;
        while (mBoard->IteratePlants(aPlant)) {
            if (aPlant->mRow != mRow || !CanTargetPlant(aPlant, ZombieAttackType::ATTACKTYPE_VAULT)) {
                continue;
            }

            Rect aPlantRect = aPlant->GetPlantRect();
            if (GetRectOverlap(aAttackRect, aPlantRect) < 20) {
                continue;
            }

            if (aPlant->mSeedType == SeedType::SEED_TALLNUT) {
                // 修复跳跃过程中因mPosX后移导致撞上身后的高坚果
                if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT && aPlant->mX > mX) {
                    continue;
                }
                if (aBestTallnut == nullptr || aPlant->mX > aBestTallnutLeft) {
                    aBestTallnut = aPlant;
                    aBestTallnutLeft = aPlant->mX;
                }
                continue;
            }

            int aPlantLeft = aPlant->mX;
            if (aBestPlant == nullptr || aPlantLeft > aBestPlantLeft) {
                aBestPlant = aPlant;
                aBestPlantLeft = aPlantLeft;
            }
        }

        if (aBestTallnut != nullptr) {
            return aBestTallnut;
        }

        return aBestPlant;
    };

    auto syncGigaPolevaulterPhaseCounter = [this]() {
        if (IsRemoteServer()) {
            U8U8U16U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
            event.data1 = uint8_t(mZombiePhase);
            event.data2 = uint8_t(mSummonCounter);
            event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data4 = uint16_t(mPhaseCounter);
            netplay::PutEvent(event);
        }
    };

    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT && mZombieHeight == ZombieHeight::HEIGHT_ZOMBIE_NORMAL) {
        if (IsRemoteClientOrViewer()) {
            return;
        }

        GridItem *aPole = FindPoleTarget();
        if (aPole && mSummonCounter < 3) {
            PlayZombieReanim("anim_prepare", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 40.0f);
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PREPARE;
            reinterpret_cast<GridItemID &>(mRelatedZombieID) = mBoard->GridItemGetID(aPole);
            reinterpret_cast<ZombieID &>(aPole->mZombieType) = mBoard->ZombieGetID(this);
            syncGigaPolevaulterPhaseCounter();
            return;
        } else if (!mHasArm && FindGigaPolevaulterTarget()) {
            PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 36.0f);
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_THROW;
            syncGigaPolevaulterPhaseCounter();
            return;
        }

        Plant *aPlant = findNearestVaultPlant();
        if (aPlant) {
            if (mBoard->GetLadderAt(aPlant->mPlantCol, aPlant->mRow)) {
                float aPlantX = mBoard->GridToPixelX(aPlant->mPlantCol, aPlant->mRow) + 40;
                if (aPlantX > mPosX && mZombieHeight == ZombieHeight::HEIGHT_ZOMBIE_NORMAL && mUseLadderCol != aPlant->mPlantCol) {
                    mZombieHeight = ZombieHeight::HEIGHT_UP_LADDER;
                    mUseLadderCol = aPlant->mPlantCol;
                }
                return;
            }

            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_IN_VAULT;
            PlayZombieReanim("anim_jump", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);

            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            float aAnimDuration = aBodyReanim->mFrameCount / aBodyReanim->mAnimRate * 100.0f;
            int aJumpDistance = mX - aPlant->mX - 80;
            if (mApp->IsWallnutBowlingLevel()) {
                aJumpDistance = 0;
            }
            mVelX = aJumpDistance / aAnimDuration;
            mHasObject = false;

            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_IN_VAULT;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.i16x2.i16_1 = int16_t(mX);
                event.data2.i16x2.i16_2 = int16_t(aJumpDistance);
                netplay::PutEvent(event);
            }
        }

        if (mApp->IsIZombieLevel() && mBoard->mChallenge->IZombieGetBrainTarget(this)) {
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
            StartWalkAnim(0);
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);

        bool aJumpEnds = false;
        if (!IsRemoteClientOrViewer()) {
            if (aBodyReanim->mAnimTime > 0.6f && aBodyReanim->mAnimTime <= 0.7f) {
                Plant *aPlant = findNearestVaultPlant();
                if (aPlant && aPlant->mSeedType == SeedType::SEED_TALLNUT) {
                    mApp->PlayFoley(FoleyType::FOLEY_BONK);
                    aJumpEnds = true;
                    mApp->AddTodParticle(aPlant->mX + 60, aPlant->mY - 20, mRenderOrder + 1, ParticleEffect::PARTICLE_TALL_NUT_BLOCK);

                    mZombieHeight = ZombieHeight::HEIGHT_FALLING;
                    mPosX = aPlant->mX;
                    mPosY -= 30.0f;
                }
            }

            if (aBodyReanim->mLoopCount > 0) {
                aJumpEnds = true;
                mPosX -= 150.0f;
            }
        }
        if (aBodyReanim->ShouldTriggerTimedEvent(0.2f)) {
            mApp->PlayFoley(FoleyType::FOLEY_GRASSSTEP);
        }
        if (aBodyReanim->ShouldTriggerTimedEvent(0.4f)) {
            mApp->PlayFoley(FoleyType::FOLEY_POLEVAULT);
        }
        if (aBodyReanim->ShouldTriggerTimedEvent(0.5f)) {
            mBoard->AddAPole(mX, mY, mRow);
        }

        if (aJumpEnds) {
            mX = (int)mPosX;
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
            mZombieAttackRect = Rect(50, 0, 20, 115);

            StartWalkAnim(0);

            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_POST_VAULT;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.f32 = mPosY;
                netplay::PutEvent(event);
            }
        } else {
            float aOldPosX = mPosX;
            mPosX -= 150.0f * aBodyReanim->mAnimTime;
            mPosY = GetPosYBasedOnRow(mRow);
            mPosX = aOldPosX;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_THROW) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.54f)) {
            mApp->PlayFoley(FoleyType::FOLEY_SWING);
            int aOriginX = int(mPosX - 68.0f);
            int aOriginY = int(mPosY - 44.0f);
            Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder, mRow, ProjectileType::PROJECTILE_ZOMBIE_POLE);
            aProjectile->mMotionType = ProjectileMotion::MOTION_BACKWARDS;
            mHasObject = false;
        }
        if (aBodyReanim->mLoopCount > 0) {
            if (IsRemoteClientOrViewer()) {
                return;
            }
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
            mZombieAttackRect = Rect(50, 0, 20, 115);
            StartWalkAnim(0);
            syncGigaPolevaulterPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_POST_VAULT) {
        if (IsRemoteClientOrViewer()) {
            return;
        }
        if (GridItem *aPole = FindPoleTarget()) {
            StopEating();
            PlayZombieReanim("anim_pick", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PICK;
            reinterpret_cast<GridItemID &>(mRelatedZombieID) = GridItemID(mBoard->mGridItems.DataArrayGetID(aPole));
            reinterpret_cast<ZombieID &>(aPole->mZombieType) = mBoard->ZombieGetID(this);
            syncGigaPolevaulterPhaseCounter();
        } else if (mSummonCounter > 0) {
            StopEating();
            PlayZombieReanim("anim_take", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 16.0f);
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_TAKE;
            syncGigaPolevaulterPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PREPARE) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.8f)) {
            ++mSummonCounter;
            if (mSummonCounter == 1) {
                ReanimShowPrefix("anim_pole3", RENDER_GROUP_NORMAL);
            } else if (mSummonCounter == 2) {
                ReanimShowPrefix("anim_pole2", RENDER_GROUP_NORMAL);
            } else if (mSummonCounter == 3) {
                ReanimShowPrefix("anim_pole1", RENDER_GROUP_NORMAL);
            }
        }
        if (aBodyReanim->mLoopCount > 0) {
            if (IsRemoteClientOrViewer()) {
                return;
            }
            if (FindPoleTarget()) {
                PlayZombieReanim("anim_pick", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
                mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PICK;
            } else {
                mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
                mZombieAttackRect = Rect(50, 0, 20, 115);
                StartWalkAnim(0);
            }
            syncGigaPolevaulterPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PICK) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.2f)) {
            if (GridItem *aPole = FindPoleTarget()) {
                aPole->GridItemDie();
                mHasObject = true;
            }
            reinterpret_cast<GridItemID &>(mRelatedZombieID) = GridItemID::GRIDITEMID_NULL;
        }
        if (aBodyReanim->mLoopCount > 0) {
            if (IsRemoteClientOrViewer()) {
                return;
            }
            if (mHasObject) {
                mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT;
                mZombieAttackRect = Rect(-29, 0, 70, 115);
                StartWalkAnim(0);
            } else {
                mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
                mZombieAttackRect = Rect(50, 0, 20, 115);
                StartWalkAnim(0);
            }
            reinterpret_cast<GridItemID &>(mRelatedZombieID) = GridItemID::GRIDITEMID_NULL;
            syncGigaPolevaulterPhaseCounter();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_TAKE) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.5f)) {
            --mSummonCounter;
            if (mSummonCounter == 0) {
                ReanimShowPrefix("anim_pole3", RENDER_GROUP_HIDDEN);
            } else if (mSummonCounter == 1) {
                ReanimShowPrefix("anim_pole2", RENDER_GROUP_HIDDEN);
            } else if (mSummonCounter == 2) {
                ReanimShowPrefix("anim_pole1", RENDER_GROUP_HIDDEN);
            }
            mHasObject = true;
        }
        if (aBodyReanim->mLoopCount > 0) {
            if (IsRemoteClientOrViewer()) {
                return;
            }
            mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT;
            mZombieAttackRect = Rect(-29, 0, 70, 115);
            StartWalkAnim(0);
            syncGigaPolevaulterPhaseCounter();
        }
    }
}

void Zombie::UpdateSundayEdition() {
    if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MADDENING) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_NEWSPAPER_MAD;
            if (mBoard->CountZombiesOnScreen() <= 10 && mHasHead) {
                mApp->PlayFoley(FoleyType::FOLEY_NEWSPAPER_RARRGH);
            }

            StartWalkAnim(20);
            aBodyReanim->SetImageOverride("anim_head1", IMAGE_REANIM_ZOMBIE_PAPER_MADHEAD);
        }
    }
}

void Zombie::UpdateZombieExplorer() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
        if (mHasObject) {
            if (aPlant->IsInvulnerable()) {
                return;
            }
            if (IsRemoteClientOrViewer()) {
                return;
            }

            if (IsRemoteServer()) {
                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_EXPLORER_BURN_PLANT}, uint16_t(mBoard->mZombies.DataArrayGetID(this)), uint16_t(mBoard->mPlants.DataArrayGetID(aPlant))};
                netplay::PutEvent(event);
            }

            ExplorerBurnPlant(aPlant);
            return;
        } else {
            if (aPlant->mSeedType == SeedType::SEED_TORCHWOOD) {
                ExplorerTorchConvert(true);
            }
        }
    }

    if (Plant *aPlant = FindFriendPlantTarget()) {
        if (!mHasObject && aPlant->mSeedType == SeedType::SEED_TORCHWOOD) {
            ExplorerTorchConvert(true);
        }
    }

    if (mHasObject) {
        if (Zombie *aZombie = FindZombieTarget()) {
            aZombie->TakeDamage(5, 0U);
            if (aZombie->mBodyHealth < 5) {
                mApp->PlayFoley(FoleyType::FOLEY_EXPLORER_IGNITE);
                aZombie->ApplyBurn();
            }
        }
    }

    if (!mHasObject) {
        if (Zombie *aZombie = FindFriendZombieTarget()) {
            if (aZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && aZombie->mHasObject) {
                ExplorerTorchConvert(true);
            }
        }
    }
}

void Zombie::UpdateExplorerProjectiles() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    Rect aAttackRect = GetZombieAttackRect();
    Projectile *aProjectile = nullptr;
    while (mBoard->IterateProjectiles(aProjectile)) {
        Rect aProjectileRect = aProjectile->GetProjectileRect();
        if (aProjectile->mRow == mRow && GetRectOverlap(aAttackRect, aProjectileRect) >= 10) {
            if (mMindControlled) {
                int aGridX = mBoard->PixelToGridX(mX, mY);
                if (mHasObject) {
                    if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_PEA) {
                        aProjectile->ConvertToFireball(aGridX);
                    } else if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_SNOWPEA) {
                        aProjectile->ConvertToPea(aGridX);
                    }
                } else {
                    if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
                        ExplorerTorchConvert(true);
                    }
                }
            } else {
                if (mHasObject) {
                    if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA) {
                        aProjectile->ConvertToZombieFireball();
                    }
                } else {
                    if (aProjectile->mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL) {
                        ExplorerTorchConvert(true);
                    }
                }
            }
        }
    }
}

void Zombie::ExplorerBurnPlant(Plant *thePlant) {
    if (thePlant == nullptr) {
        return;
    }

    mApp->PlayFoley(FoleyType::FOLEY_EXPLORER_IGNITE);
    int aRenderOrder = mBoard->MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, mRow, 1);
    Reanimation *aOriReanim = mApp->ReanimationTryToGet(mBoard->mFwooshID[mRow][thePlant->mRow]);
    if (aOriReanim) {
        aOriReanim->ReanimationDie();
    }

    float aPosX = mBoard->GridToPixelX(thePlant->mPlantCol, thePlant->mRow) + 40.0f;
    float aPosY = mBoard->GridToPixelY(thePlant->mPlantCol, thePlant->mRow);
    Reanimation *aFwoosh = mApp->AddReanimation(aPosX, aPosY, aRenderOrder, ReanimationType::REANIM_JALAPENO_FIRE);
    aFwoosh->SetFramesForLayer("anim_flame");
    aFwoosh->mLoopType = ReanimLoopType::REANIM_LOOP_FULL_LAST_FRAME;
    aFwoosh->mAnimRate *= RandRangeFloat(0.7f, 1.3f);

    float aScale = RandRangeFloat(0.9f, 1.1f);
    float aFlip = Rand(2) ? 1.0f : -1.0f;
    aFwoosh->OverrideScale(aScale * aFlip, 1);

    mBoard->mFwooshID[mRow][thePlant->mRow] = mApp->ReanimationGetID(aFwoosh);
    mBoard->mFwooshCountDown = 100;
    thePlant->Die();
}

void Zombie::ExplorerTorchConvert(bool theBurn) {
    if (theBurn) {
        if (IsMovingAtChilledSpeed()) {
            return;
        }
        mApp->PlayFoley(FoleyType::FOLEY_EXPLORER_IGNITE);
        mHasObject = true;
        mZombieAttackRect = Rect(-10, 0, 50, 115);
        if (Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID)) {
            aBodyReanim->AssignRenderGroupToPrefix("Fire", RENDER_GROUP_NORMAL);
        }
    } else {
        mHasObject = false;
        mZombieAttackRect = Rect(20, 0, 50, 115);
        if (Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID)) {
            aBodyReanim->AssignRenderGroupToPrefix("Fire", RENDER_GROUP_HIDDEN);
        }
    }
}

void Zombie::UpdateGigaGargantuar() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    static constexpr int GIGA_THROW_COUNT = 3;
    static constexpr int GIGA_THROW_COOLDOWN = 2000;

    static constexpr int GIGA_LIGHTNING_DURATION = 400;
    static constexpr int GIGA_LIGHTNING_MAIN_DAMAGE = 6;
    static constexpr int GIGA_LIGHTNING_SPLASH_DAMAGE = 2;
    static constexpr int GIGA_LIGHTNING_HIT_EFFECT_INTERVAL = 50;
    static constexpr int GIGA_LIGHTNING_HIT_EFFECT_DURATION = 40;

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr) {
        return;
    }

    auto spawnLightningHitEffectAt = [this](int theX, int theY, int theRow) {
        const int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, theRow, 1);
        Reanimation *aLightning = mApp->AddReanimation(theX, theY, aRenderOrder, ReanimationType::REANIM_LIGHTNING_HIT);
        if (aLightning != nullptr) {
            mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_HIFI);
            aLightning->SetFramesForLayer("anim_hit");
            aLightning->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_FULL_LAST_FRAME;
            aLightning->mAnimRate = aLightning->mFrameCount * 100.0f / float(GIGA_LIGHTNING_HIT_EFFECT_DURATION);
        }
    };

    auto spawnLightningPlantHitEffect = [&spawnLightningHitEffectAt](Plant *thePlant) {
        if (thePlant == nullptr || thePlant->NotOnGround()) {
            return;
        }

        spawnLightningHitEffectAt(thePlant->mX, thePlant->mY, thePlant->mRow);
    };

    auto spawnLightningZombieHitEffect = [&spawnLightningHitEffectAt](Zombie *theZombie) {
        if (theZombie == nullptr || theZombie->IsDeadOrDying()) {
            return;
        }

        spawnLightningHitEffectAt(theZombie->mX, theZombie->mY, theZombie->mRow);
    };

    auto damagePlant = [this, &spawnLightningPlantHitEffect](Plant *thePlant, int theDamage, bool theSpawnHitEffect) {
        if (thePlant == nullptr || thePlant->mDead || thePlant->NotOnGround()) {
            return;
        }

        if (thePlant->IsInvulnerable()) {
            return;
        }

        thePlant->mPlantHealth -= theDamage;
        thePlant->mEatenFlashCountdown = std::max(thePlant->mEatenFlashCountdown, 25);

        if (theSpawnHitEffect) {
            spawnLightningPlantHitEffect(thePlant);
        }

        if (thePlant->mPlantHealth > 0) {
            return;
        }

        ++mBoard->mPlantsEaten;
        thePlant->Die();

        if (mBoard->mChallenge != nullptr) {
            mBoard->mChallenge->ZombieAtePlant(this, thePlant);
        }
    };

    struct LightningMainTarget {
        Plant *mPlant = nullptr;
        Zombie *mZombie = nullptr;
        int mCenterX = 0;
        int mCenterY = 0;

        bool IsValid() const {
            return mPlant != nullptr || mZombie != nullptr;
        }
    };

    auto findLightningMainTarget = [this]() -> LightningMainTarget {
        LightningMainTarget aBestTarget{};

        const int aLightningOriginX = static_cast<int>(mPosX);
        int aBestDistance = INT_MAX;

        if (!mMindControlled) {
            Plant *aPlant = nullptr;
            while (mBoard->IteratePlants(aPlant)) {
                if (aPlant->NotOnGround() || aPlant->mRow != mRow || aPlant->IsLowProfile()) {
                    continue;
                }

                Rect aPlantRect = aPlant->GetPlantRect();
                const int aCenterX = aPlantRect.mX + aPlantRect.mWidth / 2;
                const int aCenterY = aPlantRect.mY + aPlantRect.mHeight / 2;
                if (aCenterX >= aLightningOriginX) {
                    continue;
                }

                const int aDistance = aLightningOriginX - aCenterX;
                if (aDistance < aBestDistance) {
                    aBestDistance = aDistance;
                    aBestTarget.mPlant = aPlant;
                    aBestTarget.mZombie = nullptr;
                    aBestTarget.mCenterX = aCenterX;
                    aBestTarget.mCenterY = aCenterY;
                }
            }
        }

        Zombie *aZombie = nullptr;
        while (mBoard->IterateZombies(aZombie)) {
            if (aZombie == this || !aZombie->IsOnBoard() || aZombie->IsDeadOrDying() || aZombie->mRow != mRow) {
                continue;
            }

            if (aZombie->mMindControlled == mMindControlled) {
                continue;
            }

            Rect aZombieRect = aZombie->GetZombieRect();
            const int aCenterX = aZombieRect.mX + aZombieRect.mWidth / 2;
            const int aCenterY = aZombieRect.mY + aZombieRect.mHeight / 2;

            int aDistance;

            if (mMindControlled) {
                if (aCenterX <= aLightningOriginX) {
                    continue;
                }

                aDistance = aCenterX - aLightningOriginX;
            } else {
                if (aCenterX >= aLightningOriginX) {
                    continue;
                }

                aDistance = aLightningOriginX - aCenterX;
            }

            if (aDistance < aBestDistance) {
                aBestDistance = aDistance;
                aBestTarget.mPlant = nullptr;
                aBestTarget.mZombie = aZombie;
                aBestTarget.mCenterX = aCenterX;
                aBestTarget.mCenterY = aCenterY;
            }
        }

        return aBestTarget;
    };

    auto refreshLightningMainTarget = [this, &findLightningMainTarget]() -> LightningMainTarget {
        LightningMainTarget aTarget = findLightningMainTarget();

        if (aTarget.mPlant != nullptr) {
            mTargetPlantID = PlantID(mBoard->mPlants.DataArrayGetID(aTarget.mPlant));
            mTargetCol = aTarget.mPlant->mPlantCol;
        } else if (aTarget.mZombie != nullptr) {
            mTargetPlantID = PlantID::PLANTID_NULL;

            Rect aRect = aTarget.mZombie->GetZombieRect();
            const int aCenterX = aRect.mX + aRect.mWidth / 2;
            const int aCenterY = aRect.mY + aRect.mHeight / 2;

            mTargetCol = mBoard->PixelToGridX(aCenterX, aCenterY);
        } else {
            mTargetPlantID = PlantID::PLANTID_NULL;
            mTargetCol = -1;
        }

        return aTarget;
    };

    auto throwGigaImp = [this]() {
        Zombie *aGigaImp = mBoard->AddZombie(ZombieType::ZOMBIE_GIGA_IMP, mFromWave, false);
        if (aGigaImp == nullptr) {
            return;
        }

        if (IsRemoteServer()) {
            U16U16U16UNI32UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_IMP_THROWN;
            event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data2 = uint16_t(mBoard->mZombies.DataArrayGetID(aGigaImp));
            event.data3 = uint16_t(aGigaImp->mTargetCol);
            event.data4.f32 = 0.0f;
            netplay::PutEvent(event);
        }

        aGigaImp->ZombieImpThrown(this, 0.0f);
    };

    auto hasGigaImpBombardTarget = [this]() -> bool {
        if (mMindControlled) {
            return false;
        }

        Plant *aPlant = nullptr;
        while (mBoard->IteratePlants(aPlant)) {
            if (aPlant->NotOnGround() || aPlant->mRow != mRow) {
                continue;
            }

            if (aPlant->mPlantCol < 0 || aPlant->mPlantCol > 2) {
                continue;
            }

            return true;
        }

        return false;
    };

    if (mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_SMASHING) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.64f)) {
            if (Zombie *aZombie = FindZombieTarget()) {
                aZombie->TakeDamage(1500, 0U);
            }

            if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
                if (aPlant->mSeedType == SeedType::SEED_SPIKEROCK) {
                    TakeDamage(20, 32U);
                    aPlant->SpikeRockTakeDamage();

                    if (aPlant->mPlantHealth <= 0) {
                        SquishAllInSquare(aPlant->mPlantCol, aPlant->mRow, ZombieAttackType::ATTACKTYPE_CHEW);
                    }
                } else {
                    SquishAllInSquare(aPlant->mPlantCol, aPlant->mRow, ZombieAttackType::ATTACKTYPE_CHEW);
                }
            }

            if (mApp->IsScaryPotterLevel()) {
                int aGridX = mBoard->PixelToGridX(mPosX, mPosY);
                if (GridItem *aScaryPot = mBoard->GetScaryPotAt(aGridX, mRow)) {
                    mBoard->mChallenge->ScaryPotterOpenPot(aScaryPot);
                }
            }

            if (mApp->IsIZombieLevel()) {
                if (GridItem *aBrain = mBoard->mChallenge->IZombieGetBrainTarget(this)) {
                    mBoard->mChallenge->IZombieSquishBrain(aBrain);
                }
            }

            mApp->PlayFoley(FoleyType::FOLEY_THUMP);
            mBoard->ShakeBoard(0, 3);
        }

        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(20);
        }

        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_PREPARING) {
        if (aBodyReanim->mLoopCount > 0) {
            mSummonCounter = 0;
            mZombiePhase = ZombiePhase::PHASE_GARGANTUAR_THROWING;
            PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
        }
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_THROWING) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.8f) && mSummonCounter < GIGA_THROW_COUNT) {
            ++mSummonCounter;
            mApp->PlayFoley(FoleyType::FOLEY_SWING);

            if (!IsRemoteClientOrViewer()) {
                throwGigaImp();
            }
        }

        if (aBodyReanim->mLoopCount > 0) {
            if (mSummonCounter < GIGA_THROW_COUNT) {
                PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            } else {
                mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_END;
                PlayZombieReanim("anim_throw_end", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            }
        }

        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_END) {
        if (aBodyReanim->mLoopCount > 0) {
            mSummonCounter = 0;
            mPhaseCounter = GIGA_THROW_COOLDOWN;
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;

            PickRandomSpeed();
            StartWalkAnim(20);
        }

        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_PREPARING) {
        if (aBodyReanim->mLoopCount > 0) {
            mApp->PlayFoley(FoleyType(RandRangeInt(FOLEY_GIGA_LAUGH, FOLEY_GIGA_LAUGH3)));
            mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_CORE);
            mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_WIDTH);
            mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_ATTACK;
            mPhaseCounter = GIGA_LIGHTNING_DURATION;
            PlayZombieReanim("anim_lightning_attack", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
        }

        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_ATTACK) {
        const int aLightningElapsedTicks = GIGA_LIGHTNING_DURATION - std::clamp(mPhaseCounter, 0, GIGA_LIGHTNING_DURATION);
        const bool aSpawnHitEffect = aLightningElapsedTicks == 1 || (aLightningElapsedTicks > 0 && aLightningElapsedTicks % GIGA_LIGHTNING_HIT_EFFECT_INTERVAL == 0);

        LightningMainTarget aMainTarget = refreshLightningMainTarget();
        const int aLightningOriginX = mX;

        auto isInsideLightningRange = [this, &aMainTarget, aLightningOriginX](int theCenterX) -> bool {
            if (mMindControlled) {
                if (theCenterX <= aLightningOriginX) {
                    return false;
                }

                if (aMainTarget.IsValid() && theCenterX > aMainTarget.mCenterX) {
                    return false;
                }
            } else {
                if (theCenterX >= aLightningOriginX) {
                    return false;
                }

                if (aMainTarget.IsValid() && theCenterX < aMainTarget.mCenterX) {
                    return false;
                }
            }

            return true;
        };

        struct GigaLightningHit {
            PlantID mPlantID;
            int mDamage;
        };
        std::vector<GigaLightningHit> aHits;

        struct GigaLightningZombieHit {
            ZombieID mZombieID;
            int mDamage;
        };
        std::vector<GigaLightningZombieHit> aZombieHits;

        if (!mMindControlled) {
            Plant *aPlant = nullptr;
            while (mBoard->IteratePlants(aPlant)) {
                if (aPlant->NotOnGround()) {
                    continue;
                }

                if (std::abs(aPlant->mRow - mRow) > 1) {
                    continue;
                }

                Rect aPlantRect = aPlant->GetPlantRect();
                const int aCenterX = aPlantRect.mX + aPlantRect.mWidth / 2;

                const bool aInsideLightningBeam = std::abs(aPlant->mRow - mRow) <= 1 && isInsideLightningRange(aCenterX);

                // 如果主目标是僵尸，则以该僵尸为中心产生范围溅射
                const bool aInsideZombieTargetSplash = aMainTarget.mZombie != nullptr && GetCircleRectOverlap(aMainTarget.mCenterX, aMainTarget.mCenterY, JackInTheBoxZombieRadius, aPlantRect);

                if (!aInsideLightningBeam && !aInsideZombieTargetSplash) {
                    continue;
                }

                auto aPlantID = PlantID(mBoard->mPlants.DataArrayGetID(aPlant));
                int aDamage = GIGA_LIGHTNING_SPLASH_DAMAGE;

                if (aMainTarget.mPlant == aPlant) {
                    aDamage = GIGA_LIGHTNING_MAIN_DAMAGE;
                }

                aHits.push_back({aPlantID, aDamage});
            }

            for (const GigaLightningHit &aHit : aHits) {
                Plant *aHitPlant = mBoard->mPlants.DataArrayTryToGet(aHit.mPlantID);

                if (aHitPlant == nullptr || aHitPlant->NotOnGround()) {
                    continue;
                }

                damagePlant(aHitPlant, aHit.mDamage, aSpawnHitEffect);
            }
        }

        Zombie *aZombie = nullptr;
        while (mBoard->IterateZombies(aZombie)) {
            if (aZombie == this || aZombie->mDead || !aZombie->IsOnBoard() || aZombie->IsDeadOrDying()) {
                continue;
            }

            if (aZombie->mMindControlled == mMindControlled) {
                continue;
            }

            Rect aRect = aZombie->GetZombieRect();
            const int aCenterX = aRect.mX + aRect.mWidth / 2;

            const bool aInsideLightningBeam = std::abs(aZombie->mRow - mRow) <= 1 && isInsideLightningRange(aCenterX);

            const bool aInsideTargetSplash =
                aMainTarget.IsValid() && std::abs(aZombie->mRow - mRow) <= 1 && GetCircleRectOverlap(aMainTarget.mCenterX, aMainTarget.mCenterY, JackInTheBoxZombieRadius, aRect);

            if (!aInsideLightningBeam && !aInsideTargetSplash) {
                continue;
            }

            int aDamage = GIGA_LIGHTNING_SPLASH_DAMAGE;

            if (aMainTarget.mZombie == aZombie) {
                aDamage = GIGA_LIGHTNING_MAIN_DAMAGE;
            }

            aZombieHits.push_back({mBoard->ZombieGetID(aZombie), aDamage});
        }

        for (const GigaLightningZombieHit &aHit : aZombieHits) {
            Zombie *aHitZombie = mBoard->mZombies.DataArrayTryToGet(aHit.mZombieID);
            if (aHitZombie == nullptr || aHitZombie == this || aHitZombie->mDead || aHitZombie->IsDeadOrDying()) {
                continue;
            }

            if (aHitZombie->mMindControlled == mMindControlled) {
                continue;
            }

            if (aSpawnHitEffect) {
                spawnLightningZombieHitEffect(aHitZombie);
            }

            aHitZombie->TakeDamage(aHit.mDamage, 0U);
        }

        refreshLightningMainTarget();

        if (mPhaseCounter <= 0) {
            mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_TAIL);
            mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_END;
            PlayZombieReanim("anim_lightning_attack_end", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 16.0f);
        }

        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_END) {
        if (aBodyReanim->mLoopCount > 0) {
            mTargetPlantID = PlantID::PLANTID_NULL;
            mTargetCol = -1;
            mPhaseCounter = GIGA_THROW_COOLDOWN;
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;

            PickRandomSpeed();
            StartWalkAnim(20);
        }

        return;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    const float aLaunchingDistance = mPosX - 400.0f;

    if (mHasObject && (mBodyHealth < mBodyMaxHealth / 2 || aLaunchingDistance <= 0.0f)) {
        StopEating();

        mHasObject = false;

        LightningMainTarget aMainTarget = findLightningMainTarget();
        if (aMainTarget.mPlant != nullptr) {
            mTargetPlantID = PlantID(mBoard->mPlants.DataArrayGetID(aMainTarget.mPlant));
            mTargetCol = aMainTarget.mPlant->mPlantCol;
        } else if (aMainTarget.mZombie != nullptr) {
            mTargetPlantID = PlantID::PLANTID_NULL;

            Rect aRect = aMainTarget.mZombie->GetZombieRect();
            const int aCenterX = aRect.mX + aRect.mWidth / 2;
            const int aCenterY = aRect.mY + aRect.mHeight / 2;

            mTargetCol = mBoard->PixelToGridX(aCenterX, aCenterY);
        } else {
            mTargetPlantID = PlantID::PLANTID_NULL;
            mTargetCol = -1;
        }

        if (IsRemoteServer()) {
            U16UNI32UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_GIGA_GARGANTUAR_START_LIGHTNING;
            event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data2.f32 = mPosX;
            event.data3.i32 = mTargetCol;
            netplay::PutEvent(event);
        }

        mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_PREPARING;
        mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_CHARGE);

        PlayZombieReanim("anim_lightning_attack_preparing", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0f);
        return;
    }

    bool doSmash = false;

    if (FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW) || FindZombieTarget()) {
        doSmash = true;
    }

    if (mApp->IsScaryPotterLevel()) {
        int aGridX = mBoard->PixelToGridX(mPosX, mPosY);
        if (mBoard->GetScaryPotAt(aGridX, mRow)) {
            doSmash = true;
        }
    } else if (mApp->IsIZombieLevel()) {
        if (mBoard->mChallenge->IZombieGetBrainTarget(this)) {
            doSmash = true;
        }
    }

    if (doSmash) {
        if (IsRemoteServer()) {
            U16UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_SMASH;
            event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data2.f32 = mPosX;
            netplay::PutEvent(event);
        }

        mZombiePhase = ZombiePhase::PHASE_GARGANTUAR_SMASHING;
        mApp->PlayFoley(FoleyType::FOLEY_LOW_GROAN);
        PlayZombieReanim("anim_smash", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
        return;
    }

    if (mPhaseCounter <= 0 && aLaunchingDistance > 0.0f && hasGigaImpBombardTarget()) {
        StopEating();

        if (IsRemoteServer()) {
            U16UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_THROW;
            event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data2.f32 = mPosX;
            netplay::PutEvent(event);
        }

        mSummonCounter = 0;
        mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_PREPARING;
        PlayZombieReanim("anim_throw_preparing", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
        return;
    }
}

void Zombie::InterruptLightning() {
    if (mZombiePhase != ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_PREPARING && mZombiePhase != ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_ATTACK) {
        return;
    }
    mPhaseCounter = 0;
    mTargetPlantID = PlantID::PLANTID_NULL;
    mTargetCol = -1;
    mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_END;
    mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_TAIL);
    PlayZombieReanim("anim_lightning_attack_end", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 16.0f);
    UpdateAnimSpeed();
}

void Zombie::InterruptSuperNovaDestroy() {
    if (mTargetCol == int(SeedType::SEED_NONE)) {
        return;
    }

    mTargetPlantID = PlantID::PLANTID_NULL;
    mTargetCol = int(SeedType::SEED_NONE);
    mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;

    StartWalkAnim(20);
    UpdateAnimSpeed();
}

void Zombie::UpdateGigaImp() {
    if (!mHasHead || IsDeadOrDying()) {
        return;
    }

    bool doPop = false;

    if (mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN) {
        mVelZ -= THOWN_ZOMBIE_GRAVITY;
        mAltitude += mVelZ;
        mPosX -= mMindControlled ? -mVelX : mVelX;

        float aDiffY = GetPosYBasedOnRow(mRow) - mPosY;
        mPosY += aDiffY;
        mAltitude += aDiffY;

        if (mAltitude <= 20.0f && !mMindControlled) {
            int aPosX = mX + mWidth / 2;
            int aPosY = mY + mHeight / 2;
            int aGridX = mBoard->PixelToGridXKeepOnBoard(aPosX, aPosY);

            Plant *aUmbrella = mBoard->FindUmbrellaPlant(aGridX, mRow);
            if (aUmbrella != nullptr) {
                mApp->PlaySample(SOUND_BOING);
                mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);

                aUmbrella->DoSpecial();

                mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_BLOCKED;
                mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
                mHitUmbrella = true;

                float aBackDistance = std::max(0.0f, float(mTargetRow) - mPosX);
                float aBackFrames = std::max(20.0f, std::min(56.0f, aBackDistance / 2.2f));
                mVelX = aBackFrames > 0.0f ? aBackDistance / aBackFrames : 0.0f;
                float aFramesF = std::max(1.0f, aBackFrames);
                mVelZ = (THOWN_ZOMBIE_GRAVITY * aFramesF * (aFramesF + 1.0f) * 0.5f - mAltitude) / aFramesF;

                return;
            }
        }

        if (mAltitude <= 0.0f) {
            mAltitude = 0.0f;
            doPop = true;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_BLOCKED) {
        mVelZ -= THOWN_ZOMBIE_GRAVITY;
        mAltitude += mVelZ;

        if (mPosX < mTargetRow) {
            mPosX += fabsf(mVelX);
            if (mPosX > mTargetRow) {
                mPosX = float(mTargetRow);
            }
        } else {
            mPosX = float(mTargetRow);
        }

        float aDiffY = GetPosYBasedOnRow(mRow) - mPosY;
        mPosY += aDiffY;
        mAltitude += aDiffY;

        if (mPosX >= mTargetRow && mAltitude <= 0.0f) {
            mAltitude = 0.0f;
            doPop = true;
        }

        if (mAltitude < 0.0f && mPosX < mTargetRow) {
            mAltitude = 0.0f;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_IMP_POPPING) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim != nullptr) {
            if (aBodyReanim->ShouldTriggerTimedEvent(0.8f)) {
                DoSpecial();
            } else if (aBodyReanim->mLoopCount > 0) {
                DieNoLoot();
            }
        }
        return;
    }

    if (doPop) {
        if (IsRemoteClientOrViewer()) {
            return;
        }

        mZombiePhase = ZombiePhase::PHASE_IMP_POPPING;
        PlayZombieReanim("anim_land", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);

        if (IsRemoteServer()) {
            U16_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_IMP_POP;
            event.data = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            netplay::PutEvent(event);
        }
        return;
    }
}

void Zombie::UpdateSuperNovaGargantuar() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr) {
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_SUPER_NOVA_GARGANTUAR_DESTROY) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.3f)) {
            const auto aTargetSeedType = SeedType(mTargetCol);
            Plant *aMatchingPlant = nullptr;
            while (mBoard->IteratePlants(aMatchingPlant)) {
                if (aMatchingPlant->NotOnGround() || aMatchingPlant->mSeedType != aTargetSeedType) {
                    continue;
                }

                const int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, aMatchingPlant->mRow, 1);
                const auto aEffectX = float(aMatchingPlant->mX);
                const auto aEffectY = float(aMatchingPlant->mY);
                if (Reanimation *aSuperNova = mApp->AddReanimation(aEffectX, aEffectY, aRenderOrder, ReanimationType::REANIM_SUPER_NOVA)) {
                    aSuperNova->PlayReanim("anim_done", ReanimLoopType::REANIM_PLAY_ONCE_FULL_LAST_FRAME, 0, 12.0f);
                }
                aMatchingPlant->Die();
            }
            mTargetCol = int(SeedType::SEED_NONE);
        }

        if (aBodyReanim->mLoopCount > 0) {
            mTargetCol = int(SeedType::SEED_NONE);
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(20);
        }
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_SMASHING) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.64f)) {
            if (Zombie *aZombie = FindZombieTarget()) {
                aZombie->TakeDamage(1500, 0U);
            }

            if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
                if (aPlant->mSeedType == SeedType::SEED_SPIKEROCK) {
                    TakeDamage(20, 32U);
                    aPlant->SpikeRockTakeDamage();
                    if (aPlant->mPlantHealth <= 0) {
                        SquishAllInSquare(aPlant->mPlantCol, aPlant->mRow, ZombieAttackType::ATTACKTYPE_CHEW);
                    }
                } else {
                    SquishAllInSquare(aPlant->mPlantCol, aPlant->mRow, ZombieAttackType::ATTACKTYPE_CHEW);
                }
            }

            if (mApp->IsScaryPotterLevel()) {
                const int aGridX = mBoard->PixelToGridX(int(mPosX), int(mPosY));
                if (GridItem *aScaryPot = mBoard->GetScaryPotAt(aGridX, mRow)) {
                    mBoard->mChallenge->ScaryPotterOpenPot(aScaryPot);
                }
            }

            if (mApp->IsIZombieLevel()) {
                if (GridItem *aBrain = mBoard->mChallenge->IZombieGetBrainTarget(this)) {
                    mBoard->mChallenge->IZombieSquishBrain(aBrain);
                }
            }

            mApp->PlayFoley(FoleyType::FOLEY_THUMP);
            mBoard->ShakeBoard(0, 3);
        }

        if (aBodyReanim->mLoopCount > 0) {
            Plant *aTargetPlant = mBoard->mPlants.DataArrayTryToGet(mTargetPlantID);
            const bool aTargetDied = mTargetPlantID != PlantID::PLANTID_NULL && (aTargetPlant == nullptr || aTargetPlant->NotOnGround());
            bool hasMatchingPlant = false;
            if (aTargetDied) {
                Plant *aMatchingPlant = nullptr;
                while (mBoard->IteratePlants(aMatchingPlant)) {
                    if (aMatchingPlant != aTargetPlant && !aMatchingPlant->NotOnGround() && aMatchingPlant->mSeedType == SeedType(mTargetCol)) {
                        hasMatchingPlant = true;
                        break;
                    }
                }
            }

            mTargetPlantID = PlantID::PLANTID_NULL;

            if (aTargetDied && hasMatchingPlant) {
                mZombiePhase = ZombiePhase::PHASE_SUPER_NOVA_GARGANTUAR_DESTROY;
                PlayZombieReanim("anim_skill", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
            } else {
                mTargetCol = int(SeedType::SEED_NONE);
                mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
                StartWalkAnim(20);
            }
        }

        return;
    }

    if (IsImmobilizied() || !mHasHead || IsRemoteClientOrViewer()) {
        return;
    }

    Plant *aTargetPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
    bool doSmash = aTargetPlant != nullptr || FindZombieTarget() != nullptr;
    if (mApp->IsScaryPotterLevel()) {
        const int aGridX = mBoard->PixelToGridX(int(mPosX), int(mPosY));
        doSmash = doSmash || mBoard->GetScaryPotAt(aGridX, mRow) != nullptr;
    } else if (mApp->IsIZombieLevel()) {
        doSmash = doSmash || mBoard->mChallenge->IZombieGetBrainTarget(this) != nullptr;
    }

    if (!doSmash) {
        return;
    }

    if (IsRemoteServer()) {
        U16UNI32_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_SMASH;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2.f32 = mPosX;
        netplay::PutEvent(event);
    }

    if (aTargetPlant != nullptr) {
        mTargetPlantID = PlantID(mBoard->mPlants.DataArrayGetID(aTargetPlant));
        mTargetCol = int(aTargetPlant->mSeedType);
    } else {
        mTargetPlantID = PlantID::PLANTID_NULL;
        mTargetCol = int(SeedType::SEED_NONE);
    }
    mZombiePhase = ZombiePhase::PHASE_GARGANTUAR_SMASHING;
    mApp->PlayFoley(FoleyType::FOLEY_LOW_GROAN);
    PlayZombieReanim("anim_smash", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
}

void Zombie::UpdateZombieGargantuar() {
    // 修复魅惑巨人不索敌敌方僵尸的BUG
    if (mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_SMASHING) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.64f)) {
            Zombie *aZombie = FindZombieTarget();
            if (mMindControlled && aZombie) {
                aZombie->TakeDamage(1500, 0U);
            }

            Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
            if (aPlant) {
                if (aPlant->mSeedType == SeedType::SEED_SPIKEROCK) {
                    TakeDamage(20, 32U);
                    aPlant->SpikeRockTakeDamage();
                    if (aPlant->mPlantHealth <= 0) {
                        SquishAllInSquare(aPlant->mPlantCol, aPlant->mRow, ZombieAttackType::ATTACKTYPE_CHEW);
                    }
                } else {
                    SquishAllInSquare(aPlant->mPlantCol, aPlant->mRow, ZombieAttackType::ATTACKTYPE_CHEW);
                }
            }

            if (mApp->IsScaryPotterLevel()) {
                int aGridX = mBoard->PixelToGridX(mPosX, mPosY);
                GridItem *aScaryPot = mBoard->GetScaryPotAt(aGridX, mRow);
                if (aScaryPot) {
                    mBoard->mChallenge->ScaryPotterOpenPot(aScaryPot);
                }
            }

            if (mApp->IsIZombieLevel()) {
                GridItem *aBrain = mBoard->mChallenge->IZombieGetBrainTarget(this);
                if (aBrain) {
                    mBoard->mChallenge->IZombieSquishBrain(aBrain);
                }
            }

            mApp->PlayFoley(FoleyType::FOLEY_THUMP);
            mBoard->ShakeBoard(0, 3);
        }

        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(20);
        }

        return;
    }

    float aThrowingDistance = mPosX - 360.0f;
    if (mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_THROWING) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.74f)) {
            mHasObject = false;
            ReanimShowPrefix("Zombie_imp", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_gargantuar_whiterope", RENDER_GROUP_HIDDEN);
            mApp->PlayFoley(FoleyType::FOLEY_SWING);

            if (IsRemoteClientOrViewer())
                return;

            Zombie *aZombieImp = mBoard->AddZombie(ZombieType::ZOMBIE_IMP, mFromWave, false);
            if (aZombieImp == nullptr)
                return;

            float aMinThrowDistance = 40.0f;
            float aOffsetDistance = RandRangeFloat(0.0f, 100.0f);
            if (mBoard->StageHasRoof()) {
                aThrowingDistance -= 180.0f;
                aMinThrowDistance = -140.0f;
            }
            if (aThrowingDistance < aMinThrowDistance) {
                aThrowingDistance = aMinThrowDistance;
            } else if (aThrowingDistance > 140.0f) {
                aThrowingDistance -= aOffsetDistance;
            }

            if (IsRemoteServer()) {
                U16U16U16UNI32UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_IMP_THROWN;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2 = uint16_t(mBoard->mZombies.DataArrayGetID(aZombieImp));
                event.data4.f32 = aOffsetDistance;
                netplay::PutEvent(event);
            }

            aZombieImp->mPosX = mPosX - 133.0f;
            aZombieImp->mPosY = GetPosYBasedOnRow(mRow);
            aZombieImp->SetRow(mRow);
            aZombieImp->mVariant = false;
            aZombieImp->mAltitude = 88.0f;
            aZombieImp->mRenderOrder = mRenderOrder + 1;
            aZombieImp->mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_THROWN;
            // 修复魅惑巨人不投掷魅惑小鬼的BUG
            aZombieImp->mScaleZombie = mScaleZombie;
            aZombieImp->mBodyHealth *= mScaleZombie * mScaleZombie;
            aZombieImp->mBodyMaxHealth *= mScaleZombie * mScaleZombie;

            if (mMindControlled) {
                aZombieImp->mPosX = mPosX + mWidth;
                aZombieImp->StartMindControlled();
                aZombieImp->mVelX = -3.0f;
            } else {
                aZombieImp->mVelX = 3.0f;
            }
            aZombieImp->mChilledCounter = mChilledCounter;
            aZombieImp->mVelZ = 0.5f * (aThrowingDistance / aZombieImp->mVelX) * THOWN_ZOMBIE_GRAVITY;
            aZombieImp->PlayZombieReanim("anim_thrown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
            aZombieImp->UpdateReanim();
            mApp->PlayFoley(FoleyType::FOLEY_IMP);
        }

        if (aBodyReanim->mLoopCount > 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            StartWalkAnim(20);
        }

        return;
    }

    if (IsImmobilizied() || !mHasHead)
        return;

    // 客机不判断是否扔小鬼、是否砸地
    if (IsRemoteClientOrViewer())
        return;

    if (mHasObject && mBodyHealth < mBodyMaxHealth / 2 && aThrowingDistance > 40.0f) {
        if (IsRemoteServer()) {
            U16UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_THROW;
            event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data2.f32 = mPosX;
            netplay::PutEvent(event);
        }
        mZombiePhase = ZombiePhase::PHASE_GARGANTUAR_THROWING;
        PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
        return;
    }

    bool doSmash = false;
    if (FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW) || (mMindControlled && FindZombieTarget())) {
        doSmash = true;
    }
    if (mApp->IsScaryPotterLevel()) {
        int aGridX = mBoard->PixelToGridX(mPosX, mPosY);
        if (mBoard->GetScaryPotAt(aGridX, mRow)) {
            doSmash = true;
        }
    } else if (mApp->IsIZombieLevel()) {
        if (mBoard->mChallenge->IZombieGetBrainTarget(this)) {
            doSmash = true;
        }
    }

    if (doSmash) {
        if (IsRemoteServer()) {
            U16UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_SMASH;
            event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
            event.data2.f32 = mPosX;
            netplay::PutEvent(event);
        }

        mZombiePhase = ZombiePhase::PHASE_GARGANTUAR_SMASHING;
        mApp->PlayFoley(FoleyType::FOLEY_LOW_GROAN);
        PlayZombieReanim("anim_smash", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
    }
}

void Zombie::ZombieImpThrown(Zombie *theThrowerZombie, float theOffsetDistance) {
    if (mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
        static constexpr float GIGA_IMP_FLY_SPEED = 3.0f;
        static constexpr float GIGA_IMP_LAUNCH_OFFSET_X = -45.0f;
        static constexpr float GIGA_IMP_LAUNCH_ALTITUDE = 90.0f;

        SetRow(theThrowerZombie->mRow);
        mPosX = theThrowerZombie->mPosX + GIGA_IMP_LAUNCH_OFFSET_X * theThrowerZombie->mScaleZombie;
        mPosY = GetPosYBasedOnRow(mRow);
        mAltitude = GIGA_IMP_LAUNCH_ALTITUDE * theThrowerZombie->mScaleZombie;
        mX = int(mPosX);
        mY = int(mPosY);
        mVariant = false;
        mHitUmbrella = false;
        mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;
        mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_THROWN;
        mRenderOrder = theThrowerZombie->mRenderOrder + 1;
        mScaleZombie = theThrowerZombie->mScaleZombie;
        mBodyHealth *= mScaleZombie * mScaleZombie;
        mBodyMaxHealth *= mScaleZombie * mScaleZombie;
        mChilledCounter = theThrowerZombie->mChilledCounter;

        const auto aTargetX = float(mBoard->GridToPixelX(mTargetCol, mRow));
        const float aThrowingDistance = std::max(0.0f, mPosX - aTargetX);
        mVelX = GIGA_IMP_FLY_SPEED;

        const int aFlightFrames = std::max(1, int(std::ceil(aThrowingDistance / GIGA_IMP_FLY_SPEED)));
        const auto aFrames = float(aFlightFrames);
        mVelZ = (THOWN_ZOMBIE_GRAVITY * aFrames * (aFrames + 1.0f) * 0.5f - mAltitude) / aFrames;

        PlayZombieReanim("anim_thrown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
        UpdateReanim();
        mApp->PlayFoley(FoleyType::FOLEY_IMP);
        return;
    }

    float aThrowingDistance = theThrowerZombie->mPosX - 360.0f;

    float aMinThrowDistance = 40.0f;
    if (mBoard->StageHasRoof()) {
        aThrowingDistance -= 180.0f;
        aMinThrowDistance = -140.0f;
    }
    if (aThrowingDistance < aMinThrowDistance) {
        aThrowingDistance = aMinThrowDistance;
    } else if (aThrowingDistance > 140.0f) {
        aThrowingDistance -= theOffsetDistance;
    }

    mPosX = theThrowerZombie->mPosX - 133.0f;
    mPosY = GetPosYBasedOnRow(theThrowerZombie->mRow);
    SetRow(theThrowerZombie->mRow);
    mVariant = false;
    mAltitude = 88.0f;
    mRenderOrder = theThrowerZombie->mRenderOrder + 1;
    mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_THROWN;
    mScaleZombie = theThrowerZombie->mScaleZombie;
    mBodyHealth *= mScaleZombie * mScaleZombie;
    mBodyMaxHealth *= mScaleZombie * mScaleZombie;

    if (theThrowerZombie->mMindControlled) {
        mPosX = theThrowerZombie->mPosX + theThrowerZombie->mWidth;
        StartMindControlled();
        mVelX = -3.0f;
    } else {
        mVelX = 3.0f;
    }
    mChilledCounter = theThrowerZombie->mChilledCounter;
    mVelZ = 0.5f * (aThrowingDistance / mVelX) * THOWN_ZOMBIE_GRAVITY;
    PlayZombieReanim("anim_thrown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
    UpdateReanim();
    mApp->PlayFoley(FoleyType::FOLEY_IMP);
}

void Zombie::ZombieImpKicked(float theKickingDistance) {
    mTargetRow = mX; // 起始位置
    mApp->PlayFoley(FoleyType::FOLEY_SWING);

    mZombiePhase = ZombiePhase::PHASE_IMP_GETTING_THROWN;
    float aKickingDistance = theKickingDistance;
    if (aKickingDistance < 20.0f) {
        aKickingDistance = 20.0f;
    }
    int aFlightFrames = std::lround(aKickingDistance * mScaleZombie / 0.55f);
    if (aFlightFrames < 48) {
        aFlightFrames = 48;
    } else if (aFlightFrames > 192) {
        aFlightFrames = 192;
    }
    mVelX = aKickingDistance / float(aFlightFrames);
    auto aFlightFramesF = float(aFlightFrames);
    float aStartAltitude = mAltitude;
    mVelZ = (-aStartAltitude + KICKED_ZOMBIE_GRAVITY * aFlightFramesF * (aFlightFramesF + 1.0f) * 0.5f) / aFlightFramesF;
    PlayZombieReanim("anim_thrown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 18.0f);
    UpdateReanim();
    mApp->PlayFoley(FoleyType::FOLEY_IMP);
    mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
}

void Zombie::UpdateZombiePeaHead() {
    // 用于修复豌豆僵尸被魅惑后依然向左发射会伤害植物的子弹的BUG、啃食时不发射子弹的BUG
    // 游戏原版逻辑是判断是否hasHead 且 是否isEating。这里去除对吃植物的判断

    if (!mHasHead)
        return;

    if (mPhaseCounter == 35) {
        Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
        aHeadReanim->PlayReanim("anim_shooting", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 35.0f);
    } else if (mPhaseCounter <= 0) {
        Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
        aHeadReanim->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 15.0f);
        mApp->PlayFoley(FoleyType::FOLEY_THROW);

        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        int index = aBodyReanim->FindTrackIndexById(ReanimTrackId_anim_head1);
        ReanimatorTransform aTransForm = ReanimatorTransform();
        aBodyReanim->GetCurrentTransform(index, &aTransForm);

        float aOriginX = mPosX + aTransForm.mTransX - 9.0f;
        float aOriginY = mPosY + aTransForm.mTransY + 6.0f - mAltitude;

        if (mMindControlled) // 魅惑修复
        {
            aOriginX += 90.0f * mScaleZombie;
            Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder, mRow, ProjectileType::PROJECTILE_PEA);
            aProjectile->mDamageRangeFlags = 1;
        } else {
            Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder, mRow, ProjectileType::PROJECTILE_ZOMBIE_PEA);
            aProjectile->mMotionType = ProjectileMotion::MOTION_BACKWARDS;
        }

        mPhaseCounter = 150;
    }
}

void Zombie::UpdateZombieGatlingHead() {
    // 用于修复加特林僵尸被魅惑后依然向左发射会伤害植物的子弹的BUG、啃食时不发射子弹的BUG
    // 游戏原版逻辑是判断是否hasHead 且 是否isEating。这里去除对吃植物的判断

    if (!mHasHead)
        return;

    if (mPhaseCounter == 100) {
        Reanimation *mSpecialHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
        mSpecialHeadReanim->PlayReanim("anim_shooting", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 35.0f);
    } else if (mPhaseCounter == 18 || mPhaseCounter == 35 || mPhaseCounter == 51 || mPhaseCounter == 68) {
        mApp->PlayFoley(FoleyType::FOLEY_THROW);

        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        int aTrackIndex = aBodyReanim->FindTrackIndexById(ReanimTrackId_anim_head1);
        ReanimatorTransform aTransForm = ReanimatorTransform();
        aBodyReanim->GetCurrentTransform(aTrackIndex, &aTransForm);

        float aOriginX = mPosX + aTransForm.mTransX - 9.0f;
        float aOriginY = mPosY + aTransForm.mTransY + 6.0f - mAltitude;

        if (mMindControlled) // 魅惑修复
        {
            aOriginX += 90.0f * mScaleZombie;
            Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder, mRow, ProjectileType::PROJECTILE_PEA);
            aProjectile->mDamageRangeFlags = 1;
        } else {
            Projectile *aProjectile = mBoard->AddProjectile(aOriginX, aOriginY, mRenderOrder, mRow, ProjectileType::PROJECTILE_ZOMBIE_PEA);
            aProjectile->mMotionType = ProjectileMotion::MOTION_BACKWARDS;
        }
    } else if (mPhaseCounter <= 0) {
        Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
        aHeadReanim->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 15.0f);
        mPhaseCounter = 150;
    }
}

void Zombie::BossDestroyIceballInRow(int theRow) {
    if (theRow != mFireballRow)
        return;

    Reanimation *aFireBallReanim = mApp->ReanimationTryToGet(mBossFireBallReanimID);
    if (aFireBallReanim && !mIsFireBall) {
        mApp->AddTodParticle(mPosX + 80.0f, mAnimCounter + 80.0f, 400000, ParticleEffect::PARTICLE_ICEBALL_DEATH);

        aFireBallReanim->ReanimationDie();
        mBossFireBallReanimID = ReanimationID::REANIMATIONID_NULL;
        mBoard->RemoveParticleByType(ParticleEffect::PARTICLE_ICEBALL_TRAIL);
    }
}

void Zombie::BossDestroyFireball() {
    Reanimation *aFireBallReanim = mApp->ReanimationTryToGet(mBossFireBallReanimID);
    if (aFireBallReanim && mIsFireBall) {
        float aPosX = aFireBallReanim->mOverlayMatrix.m02 + 80.0f;
        float aPosY = aFireBallReanim->mOverlayMatrix.m12 + 40.0f;
        for (int i = 0; i < 6; i++) {
            float aAngle = 2 * std::numbers::pi * i / 6 + std::numbers::pi / 2;
            Reanimation *aReanim = mApp->AddReanimation(aPosX + 60.0f * sin(aAngle), aPosY + 60.0f * cos(aAngle), 400000, ReanimationType::REANIM_JALAPENO_FIRE);
            aReanim->mAnimTime = 0.2f;
            aReanim->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_FULL_LAST_FRAME;
            aReanim->mAnimRate = RandRangeFloat(20.0f, 25.0f);
        }

        aFireBallReanim->ReanimationDie();
        mBossFireBallReanimID = ReanimationID::REANIMATIONID_NULL;
        mBoard->RemoveParticleByType(ParticleEffect::PARTICLE_FIREBALL_TRAIL);
    }
}

void Zombie::BurnRow(int theRow) {
    // 辣椒僵尸爆炸函数
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow == theRow) {
            if (aZombie->mMindControlled != mMindControlled) {
                if (aZombie->mMindControlled) {
                    aZombie->TakeDamage(1800, 18U);
                } else {
                    if (aZombie->EffectedByDamage(127)) {
                        aZombie->RemoveColdEffects();
                        aZombie->ApplyBurn();
                    }
                }
            } else if (aZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && !aZombie->mHasObject) {
                aZombie->ExplorerTorchConvert(true);
            }
        }
    }

    if (mMindControlled) {
        GridItem *aGridItem = nullptr;
        while (mBoard->IterateGridItems(aGridItem)) {
            if (aGridItem->mGridY == theRow && (aGridItem->mGridItemType == GridItemType::GRIDITEM_LADDER || aGridItem->mGridItemType == GridItemType::GRIDITEM_POLE)) {
                aGridItem->GridItemDie();
            }
        }

        Zombie *aBossZombie = mBoard->GetBossZombie();
        if (aBossZombie && aBossZombie->mFireballRow == theRow) {
            aBossZombie->BossDestroyIceballInRow(theRow);
        }

        mBoard->mIceTimer[theRow] = 20;
    }
}

bool Zombie::FindJalapenoHeadTarget() {
    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == mRow && mX >= aPlant->mX + 40 && !aPlant->NotOnGround() && !aPlant->IsSpiky() && !aPlant->IsCeleryStalkerLow()) {
            return true;
        }
    }
    return false;
}

void Zombie::UpdateZombieJalapenoHead() {
    if (!mHasHead || IsRemoteClientOrViewer()) {
        return;
    }

    if (mApp->IsVSMode() || IsOnlineModeActive()) {
        // 对战模式在倒计时结束或碰到目标时爆炸
        if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_NORMAL) {
            float aDistance = 275.0f + Rand(175.0f);
            mPhaseCounter = int(aDistance / mVelX) * ZOMBIE_LIMP_SPEED_FACTOR;
            mZombiePhase = PHASE_JALAPENO_PRE_BURN;
        } else if (mZombiePhase == ZombiePhase::PHASE_JALAPENO_PRE_BURN) {
            bool doBurn = false;
            if (mMindControlled) {
                if (mIsEating && FindZombieTarget()) {
                    doBurn = true;
                }
            } else {
                doBurn = mPhaseCounter == 0 && FindJalapenoHeadTarget();

                if (mIsEating) {
                    if (FindZombieTarget()) {
                        doBurn = true;
                    } else if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
                        const bool aTargetIgnored =
                            (aPlant->IsInvulnerable() || aPlant->mSeedType == SeedType::SEED_HYPNOSHROOM || aPlant->mSeedType == SeedType::SEED_GARLIC || aPlant->mSeedType == SeedType::SEED_SUN_BEAN)
                            && !aPlant->mIsAsleep;

                        if (!aTargetIgnored) {
                            doBurn = true;
                        }
                    }
                }
            }
            if (doBurn) {
                mPhaseCounter = 100;
                mZombiePhase = ZombiePhase::PHASE_JALAPENO_BURNNING;
                if (IsRemoteServer()) {
                    U8U8U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
                    event.data1 = uint8_t(mZombiePhase);
                    event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                    event.data4 = uint16_t(mPhaseCounter);
                    netplay::PutEvent(event);
                }

                Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
                if (aHeadReanim) {
                    aHeadReanim->SetFramesForLayer("anim_explode");
                    aHeadReanim->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD;
                }

                mApp->PlayFoley(FoleyType::FOLEY_REVERSE_EXPLOSION);
            }
        } else if (mZombiePhase == ZombiePhase::PHASE_JALAPENO_BURNNING) {
            if (mPhaseCounter == 0) {
                DoSpecial();
            }
        }
    } else {
        if (mPhaseCounter == 0) {
            DoSpecial();
        }
    }
}

void Zombie::UpdateZombieSquashHead() {
    bool justEnteredSquashRising = false;
    auto syncSquashHeadPhase = [this]() {
        if (!IsRemoteServer()) {
            return;
        }
        U8U8U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
        event.data1 = uint8_t(mZombiePhase);
        event.data2 = mSquashHeadCol == -1 ? uint8_t(255) : uint8_t(mSquashHeadCol);
        event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data4 = uint16_t(mPhaseCounter);
        netplay::PutEvent(event);
    };

    if (!IsRemoteClientOrViewer() && mHasHead && mIsEating && mZombiePhase == ZombiePhase::PHASE_SQUASH_PRE_LAUNCH) {
        StopEating();
        PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, 12.0f);
        mHasHead = false;

        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->PlayReanim("anim_jumpup", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            aHeadReanim->mRenderOrder = mRenderOrder + 1;
            aHeadReanim->SetPosition(mPosX + 6.0f, mPosY - 21.0f);

            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            ReanimatorTrackInstance *aTrackInstance = aBodyReanim->GetTrackInstanceByName("anim_head1");
            AttachmentDetach(aTrackInstance->mAttachmentID);
            aHeadReanim->OverrideScale(0.75f, 0.75f);
            aHeadReanim->mOverlayMatrix.m10 = 0.0f;
        }

        mZombiePhase = ZombiePhase::PHASE_SQUASH_RISING;
        mPhaseCounter = 95;
        justEnteredSquashRising = true;
    }

    if (mZombiePhase == ZombiePhase::PHASE_SQUASH_RISING) {
        int aDestX = mBoard->GridToPixelX(mBoard->PixelToGridXKeepOnBoard(mX, mY), mRow);

        if (!IsRemoteClientOrViewer() && mMindControlled) {
            Zombie *aZombie = FindZombieTarget();
            if (aZombie) {
                aDestX = aZombie->ZombieTargetLeadX(0.0f);
            } else {
                aDestX += 90.0f * mScaleZombie;
            }
        }

        if (mApp->IsVSMode()) {
            if (mSquashHeadCol == -1) { // 空压修复
                if (!IsRemoteClientOrViewer()) {
                    if (Zombie *aZombie = FindZombieTarget()) {
                        aDestX = aZombie->ZombieTargetLeadX(0.0f) - mWidth / 2;
                    } else if (Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW)) {
                        mSquashHeadCol = aPlant->mPlantCol;
                        aDestX = mBoard->GridToPixelX(mSquashHeadCol, mRow);
                    }
                }
            } else {
                aDestX = mBoard->GridToPixelX(mSquashHeadCol, mRow);
            }
        }
        if (justEnteredSquashRising) {
            syncSquashHeadPhase();
        }

        int aPosX = TodAnimateCurve(50, 20, mPhaseCounter, 0, aDestX - mPosX, TodCurves::CURVE_EASE_IN_OUT);
        int aPosY = TodAnimateCurve(50, 20, mPhaseCounter, 0, -20, TodCurves::CURVE_EASE_IN_OUT);

        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->SetPosition(mPosX + aPosX + 6.0f, mPosY + aPosY - 21.0f);
        }

        if (mPhaseCounter == 0) {
            if (aHeadReanim) {
                aHeadReanim->PlayReanim("anim_jumpdown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 60.0f);
            }
            mZombiePhase = ZombiePhase::PHASE_SQUASH_FALLING;
            mPhaseCounter = 10;
            syncSquashHeadPhase();
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_SQUASH_FALLING) {
        int aPosY = TodAnimateCurve(10, 0, mPhaseCounter, -20, 74, TodCurves::CURVE_LINEAR);
        int aDestX = mBoard->GridToPixelX(mBoard->PixelToGridXKeepOnBoard(mX, mY), mRow);

        if (!IsRemoteClientOrViewer() && mMindControlled) {
            Zombie *aZombie = FindZombieTarget();
            if (aZombie) {
                aDestX = aZombie->ZombieTargetLeadX(0.0f);
            } else {
                aDestX += 90.0f * mScaleZombie;
            }
        }

        if (mApp->IsVSMode()) {
            if (!IsRemoteClientOrViewer()) {
                if (Zombie *aZombie = FindZombieTarget()) {
                    aDestX = aZombie->ZombieTargetLeadX(0.0f) - mWidth / 2;
                } else if (mSquashHeadCol != -1) {
                    aDestX = mBoard->GridToPixelX(mSquashHeadCol, mRow);
                }
            } else if (mSquashHeadCol != -1) {
                aDestX = mBoard->GridToPixelX(mSquashHeadCol, mRow);
            }
        }

        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->SetPosition(mPosX + 6.0f + aDestX - mPosX, mPosY - 21.0f + aPosY);
        }

        float aSquashX = mX;
        if (mApp->IsVSMode()) {
            aSquashX = mPosX + 6.0f + aDestX - mPosX;
        }

        if (mPhaseCounter == 2 && !IsRemoteClientOrViewer()) {
            if (mMindControlled) // 魅惑修复
            {
                Rect aAttackRect(aDestX - 73, mPosY + 4, 65, 90); // 具体数值未实测，待定

                Zombie *aZombie = nullptr;
                while (mBoard->IterateZombies(aZombie)) {
                    if ((aZombie->mRow == mRow || aZombie->mZombieType == ZombieType::ZOMBIE_BOSS) && aZombie->EffectedByDamage(13U)) {
                        Rect aZombieRect = aZombie->GetZombieRect();
                        if (GetRectOverlap(aAttackRect, aZombieRect) > (aZombie->mZombieType == ZombieType::ZOMBIE_FOOTBALL ? -20 : 0)) {
                            aZombie->TakeDamage(1800, 18U);
                        }
                    }
                }
            } else {
                if (mApp->IsVSMode()) {
                    if (Zombie *aZombie = FindZombieTarget()) {
                        aZombie->TakeDamage(1800, 18U);
                    }
                }
                SquishAllInSquare(mBoard->PixelToGridXKeepOnBoard(aSquashX, mY), mRow, ZombieAttackType::ATTACKTYPE_CHEW);
            }
        }

        if (mPhaseCounter == 0) {
            mZombiePhase = ZombiePhase::PHASE_SQUASH_DONE_FALLING;
            mPhaseCounter = 100;

            mBoard->ShakeBoard(1, 4);
            mApp->PlayFoley(FoleyType::FOLEY_THUMP);
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_SQUASH_DONE_FALLING && mPhaseCounter == 0) {
        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->ReanimationDie();
        }
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;

        TakeDamage(1800, 9U);
    }
}

void Zombie::UpdateZombieDancer() {
    if (mIsEating)
        return; // 不更新动作

    if (mSummonCounter > 0) {
        mSummonCounter--;
        if (mSummonCounter == 0) {
            if (GetDancerFrame() == 12 && mHasHead && mPosX < 700.0f) {
                mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT;
                PlayZombieReanim("anim_point", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            } else {
                mSummonCounter = 1;
            }
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN) {
        if (mHasHead && mPhaseCounter == 0) {
            mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS;
            PlayZombieReanim("anim_point", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            PickRandomSpeed();
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim->mLoopCount > 0) {
            if (mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS && mBoard->CountZombiesOnScreen() <= 15) {
                mApp->PlayFoley(FoleyType::FOLEY_DANCER);
            }

            SummonBackupDancers();
            mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD;
            mPhaseCounter = 200;
        }
    } else {
        if (mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD) {
            if (mPhaseCounter != 0)
                return;

            mZombiePhase = ZombiePhase::PHASE_DANCER_DANCING_LEFT;
            PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
        }

        ZombiePhase aDancerPhase = GetDancerPhase();
        if (aDancerPhase != mZombiePhase) {
            switch (aDancerPhase) {
                case ZombiePhase::PHASE_DANCER_DANCING_LEFT:
                    mZombiePhase = aDancerPhase;
                    PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 10, 0.0f);
                    break;

                case ZombiePhase::PHASE_DANCER_WALK_TO_RAISE:
                    mZombiePhase = aDancerPhase;
                    PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                    mApp->ReanimationGet(mBodyReanimID)->mAnimTime = 0.6f;
                    break;

                case ZombiePhase::PHASE_DANCER_RAISE_LEFT_1:
                case ZombiePhase::PHASE_DANCER_RAISE_RIGHT_1:
                case ZombiePhase::PHASE_DANCER_RAISE_LEFT_2:
                case ZombiePhase::PHASE_DANCER_RAISE_RIGHT_2:
                    mZombiePhase = aDancerPhase;
                    PlayZombieReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 10, 18.0f);
                    break;
                default:
                    break;
            }
        }

        if (mHasHead && mSummonCounter == 0 && NeedsMoreBackupDancers()) {
            mSummonCounter = 100;
        }
    }
}

void Zombie::UpdateZombieRiseFromGrave() {
    if (mInPool) {
        mAltitude = TodAnimateCurve(50, 0, mPhaseCounter, -150, -40, TodCurves::CURVE_LINEAR) * mScaleZombie;
    } else {
        mAltitude = TodAnimateCurve(50, 0, mPhaseCounter, -200, 0, TodCurves::CURVE_LINEAR);
    }

    if (mPhaseCounter == 0) {
        switch (mZombieType) {
            case ZOMBIE_POLEVAULTER:
            case ZOMBIE_GIGA_POLEVAULTER:
                mZombiePhase = PHASE_POLEVAULTER_PRE_VAULT;
                break;
            case ZOMBIE_NEWSPAPER:
                mZombiePhase = PHASE_NEWSPAPER_READING;
                break;
            case ZOMBIE_DANCER:
                mZombiePhase = PHASE_DANCER_DANCING_IN;
                break;
            case ZOMBIE_POGO:
                mZombiePhase = PHASE_POGO_BOUNCING;
                break;
            case ZOMBIE_LADDER:
                mZombiePhase = PHASE_LADDER_CARRYING;
                break;
            // 对战模式修复
            case ZOMBIE_JACK_IN_THE_BOX:
                mZombiePhase = PHASE_JACK_IN_THE_BOX_PRE_RUN;
                break;
            case ZOMBIE_DIGGER:
                mZombiePhase = PHASE_DIGGER_WALKING_WITHOUT_AXE;
                break;
            case ZOMBIE_YETI:
                mZombiePhase = PHASE_YETI_PRE_RUN;
                break;
            case ZOMBIE_SQUASH_HEAD: // 修复窝瓜僵尸不起跳
                mZombiePhase = PHASE_SQUASH_PRE_LAUNCH;
                break;
            case ZOMBIE_IMP:
            case ZOMBIE_SUPER_FAN_IMP:
            case ZOMBIE_GIGA_IMP:
                mZombiePhase = PHASE_IMP_PRE_RUN;
                break;
            case ZOMBIE_GIGA_FOOTBALL:
                mZombiePhase = PHASE_FOOTBALL_PRE_CHARGE;
                break;
            case ZOMBIE_DOG:
                mZombiePhase = PHASE_DOG_RUNNING;
                break;
            case ZOMBIE_TELEPORTATION:
                mZombiePhase = PHASE_TELEPORTATION_PRE_SHOOT;
                break;
            default:
                mZombiePhase = PHASE_ZOMBIE_NORMAL;
                break;
        }

        FinishZombieRiseFromGrave();
    }
}

void Zombie::FinishZombieRiseFromGrave() {
    mAltitude = mInPool ? -40.0f * mScaleZombie : 0.0f;

    if (IsOnHighGround()) {
        mAltitude = HIGH_GROUND_HEIGHT;
    }

    if (mInPool) {
        ReanimIgnoreClipRect("Zombie_duckytube", true);
        ReanimIgnoreClipRect("Zombie_whitewater", true);
        ReanimIgnoreClipRect("Zombie_outerarm_hand", true);
        ReanimIgnoreClipRect("Zombie_innerarm3", true);
    }
}

void Zombie::UpdateDamageStates(unsigned int theDamageFlags) {
    // 史莱姆从不走普通的断臂流程。未黄油化时按阶段分裂；
    // 黄油化后则允许掉头，但仍不允许在受伤过程中断手。
    if (IsZomblob(mZombieType)) {
        if (mHasHead && mBodyHealth <= 0) {
            if (mButtered) {
                if (mZombieType != ZombieType::ZOMBIE_ZOMBLOB_SMALL) {
                    DropHead(theDamageFlags);
                }
                DropLoot();
                StopZombieSound();
                if (mBoard->HasLevelAwardDropped()) {
                    PlayDeathAnim(theDamageFlags);
                }
            } else {
                if (mZombieType == ZombieType::ZOMBIE_ZOMBLOB_SMALL) {
                    if (mBoard->HasLevelAwardDropped()) {
                        PlayDeathAnim(theDamageFlags);
                    }
                } else {
                    ZomblobSplit();
                }
            }
        }
        return;
    }

    if (!CanLoseBodyParts())
        return;

    if (mHasArm && mBodyHealth < 2 * mBodyMaxHealth / 3 && mBodyHealth > 0) {
        DropArm(theDamageFlags);
    }

    if (mHasHead && mBodyHealth < mBodyMaxHealth / 3) {
        DropHead(theDamageFlags);
        DropLoot();
        StopZombieSound();

        if (mBoard->HasLevelAwardDropped()) {
            PlayDeathAnim(theDamageFlags);
        }

        if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL) {
            DieNoLoot();
        }

        if (mApp->IsVSMode() && mBoard->GetAliveJacksonZombie() && CanRevived()) {
            mCanRevived = false;
            if (msDeadFollowers.size() >= 15) {
                msDeadFollowers.erase(msDeadFollowers.begin());
            }
            msDeadFollowers.push_back(mZombieType);
        }
    }
}

void Zombie::PlayDeathAnim(unsigned int theDamageFlags) {
    if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED) {
        return;
    }

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr || !aBodyReanim->TrackExists("anim_death")) {
        DieNoLoot();
        return;
    }
    if (mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER && mZombiePhase != ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL) {
        DieNoLoot();
        return;
    }
    if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL || mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING) {
        DieNoLoot();
        return;
    }

    if (mIceTrapCounter > 0) {
        AddAttachedParticle(75, 106, ParticleEffect::PARTICLE_ICE_TRAP_RELEASE);
        mIceTrapCounter = 0;
    }
    if (mButteredCounter > 0) {
        mButteredCounter = 0;
    }
    if (mYuckyFace) {
        ShowYuckyFace(false);
        mYuckyFace = false;
        mYuckyFaceCounter = 0;
    }

    if (TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
        if (mZombieType != ZombieType::ZOMBIE_BOSS && !IsGargantuar()) {
            DieNoLoot();
            return;
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_POGO) {
        mAltitude = 0.0f;
    }

    AttachmentReanimTypeDie(mAttachmentID, ReanimationType::REANIM_ZOMBIE_SURPRISE);
    StopEating();

    if (mShieldType != ShieldType::SHIELDTYPE_NONE) {
        DropShield(1U);
    }
    if (mZombieType == ZombieType::ZOMBIE_SQUASH_HEAD && !mHasHead) {
        Reanimation *aReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aReanim) {
            aReanim->ReanimationDie();
        }
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
    }

    mVelX = 0.0f;
    mZombiePhase = ZombiePhase::PHASE_ZOMBIE_DYING;
    if (mZombieHeight == ZombieHeight::HEIGHT_ZOMBIQUARIUM) {
        PlayZombieReanim("anim_aquarium_death", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 14.0f);
        return;
    }
    if (mZombieHeight == ZombieHeight::HEIGHT_UP_LADDER) {
        mZombieHeight = ZombieHeight::HEIGHT_FALLING;
    }

    float aDeathAnimRate = NAN;
    if (mZombieType == ZombieType::ZOMBIE_FOOTBALL || mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
        aDeathAnimRate = 24.0f;
    } else if (IsGargantuar()) {
        aDeathAnimRate = 14.0f;
        mApp->PlayFoley(FoleyType::FOLEY_GARGANTUDEATH);
    } else if (mZombieType == ZombieType::ZOMBIE_SNORKEL) {
        aDeathAnimRate = 14.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_DIGGER) {
        aDeathAnimRate = 18.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_YETI) {
        aDeathAnimRate = 14.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        aDeathAnimRate = 18.0f;

        BossDie();
        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->PlayReanim("anim_death", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, aDeathAnimRate);
        }
    } else {
        aDeathAnimRate = RandRangeFloat(24.0f, 30.0f);
    }

    const char *aDeathTrackName = "anim_death";
    int aDeathAnimHit = Rand(100);
    bool aCanDoSuperLongDeath = mApp->HasFinishedAdventure() || mBoard->mLevel > 5;
    if (mInPool && aBodyReanim->TrackExists("anim_waterdeath")) {
        aDeathTrackName = "anim_waterdeath";
        ReanimIgnoreClipRect("Zombie_duckytube", false);
    } else if (mWalnutDeath && aBodyReanim->TrackExists("anim_superlongdeath")) {
        aDeathAnimRate = 14.0f;
        aDeathTrackName = "anim_walnutdeath";
    } else if (aDeathAnimHit == 99 && aBodyReanim->TrackExists("anim_superlongdeath") && aCanDoSuperLongDeath && mChilledCounter == 0 && mBoard->CountZombiesOnScreen() <= 5) {
        aDeathAnimRate = 14.0f;
        aDeathTrackName = "anim_superlongdeath";
    } else if (aDeathAnimHit > 50 && aBodyReanim->TrackExists("anim_death2")) {
        aDeathTrackName = "anim_death2";
    }

    PlayZombieReanim(aDeathTrackName, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, aDeathAnimRate);
    ReanimShowPrefix("anim_tongue", RENDER_GROUP_HIDDEN);
}

void Zombie::UpdateDeath() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr) {
        DieNoLoot();
        return;
    }

    if (mZombieHeight == ZombieHeight::HEIGHT_FALLING) {
        UpdateZombieFalling();
    }
    if (IsGargantuar()) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.89f)) {
            mBoard->ShakeBoard(0, 3);
        } else if (aBodyReanim->ShouldTriggerTimedEvent(0.98f)) {
            mBoard->ShakeBoard(0, 1);
        }
    }

    if (!mInPool) {
        float aFallTime;
        switch (mZombieType) {
            case ZombieType::ZOMBIE_SNORKEL:
            case ZombieType::ZOMBIE_ZAMBONI:
            case ZombieType::ZOMBIE_DOLPHIN_RIDER:
            case ZombieType::ZOMBIE_BUNGEE:
            case ZombieType::ZOMBIE_CATAPULT:
            case ZombieType::ZOMBIE_IMP:
            case ZombieType::ZOMBIE_BOSS:
            case ZombieType::ZOMBIE_SUPER_FAN_IMP:
            case ZombieType::ZOMBIE_GIGA_IMP:
            case ZombieType::ZOMBIE_DOG:
                aFallTime = -1.0f;
                break;

            case ZombieType::ZOMBIE_NORMAL:
            case ZombieType::ZOMBIE_FLAG:
            case ZombieType::ZOMBIE_TRAFFIC_CONE:
            case ZombieType::ZOMBIE_PAIL:
            case ZombieType::ZOMBIE_DOOR:
            case ZombieType::ZOMBIE_PEA_HEAD:
            case ZombieType::ZOMBIE_WALLNUT_HEAD:
            case ZombieType::ZOMBIE_TALLNUT_HEAD:
            case ZombieType::ZOMBIE_JALAPENO_HEAD:
            case ZombieType::ZOMBIE_GATLING_HEAD:
            case ZombieType::ZOMBIE_SQUASH_HEAD:
            case ZombieType::ZOMBIE_DUCKY_TUBE:
            case ZombieType::ZOMBIE_EXPLORER:
            case ZombieType::ZOMBIE_DOGWALKER:
                if (aBodyReanim->IsAnimPlaying("anim_superlongdeath")) {
                    aFallTime = 0.788f;
                } else if (aBodyReanim->IsAnimPlaying("anim_death2")) {
                    aFallTime = 0.71f;
                } else {
                    aFallTime = 0.77f;
                }
                break;

            case ZombieType::ZOMBIE_POLEVAULTER:
            case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
                aFallTime = 0.68f;
                break;

            case ZombieType::ZOMBIE_FOOTBALL:
            case ZombieType::ZOMBIE_GIGA_FOOTBALL:
                aFallTime = 0.52f;
                break;

            case ZombieType::ZOMBIE_NEWSPAPER:
            case ZombieType::ZOMBIE_SUNDAY_EDITION:
                aFallTime = 0.63f;
                break;

            case ZombieType::ZOMBIE_DANCER:
            case ZombieType::ZOMBIE_BACKUP_DANCER:
            case ZombieType::ZOMBIE_JACKSON:
            case ZombieType::ZOMBIE_BACKUP_JACKSON:
                aFallTime = 0.83f;
                break;

            case ZombieType::ZOMBIE_BOBSLED:
                aFallTime = 0.81f;
                break;

            case ZombieType::ZOMBIE_JACK_IN_THE_BOX:
                aFallTime = 0.64f;
                break;

            case ZombieType::ZOMBIE_BALLOON:
                aFallTime = 0.68f;
                break;

            case ZombieType::ZOMBIE_DIGGER:
            case ZombieType::ZOMBIE_CROSSING_GUARD:
                aFallTime = 0.85f;
                break;

            case ZombieType::ZOMBIE_POGO:
                aFallTime = 0.84f;
                break;

            case ZombieType::ZOMBIE_YETI:
                aFallTime = 0.68f;
                break;

            case ZombieType::ZOMBIE_LADDER:
                aFallTime = 0.62f;
                break;

            case ZombieType::ZOMBIE_GARGANTUAR:
            case ZombieType::ZOMBIE_REDEYE_GARGANTUAR:
            case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
            case ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR:
                aFallTime = 0.86f;
                break;

            default:
                aFallTime = -1.0f;
                break;
        }

        if (aFallTime > 0 && aBodyReanim->ShouldTriggerTimedEvent(aFallTime)) {
            mApp->PlayFoley(FoleyType::FOLEY_ZOMBIE_FALLING);
            if (IsGargantuar()) {
                mApp->PlayFoley(FoleyType::FOLEY_THUMP);
            }

            if (mBoard->mDaisyMode) {
                DoDaisies();
            }
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        if (aBodyReanim->ShouldTriggerTimedEvent(0.1f) || aBodyReanim->ShouldTriggerTimedEvent(0.12f) || aBodyReanim->ShouldTriggerTimedEvent(0.15f) || aBodyReanim->ShouldTriggerTimedEvent(0.19f)
            || aBodyReanim->ShouldTriggerTimedEvent(0.2f) || aBodyReanim->ShouldTriggerTimedEvent(0.26f) || aBodyReanim->ShouldTriggerTimedEvent(0.3f) || aBodyReanim->ShouldTriggerTimedEvent(0.4f)
            || aBodyReanim->ShouldTriggerTimedEvent(0.42f) || aBodyReanim->ShouldTriggerTimedEvent(0.5f) || aBodyReanim->ShouldTriggerTimedEvent(0.58f) || aBodyReanim->ShouldTriggerTimedEvent(0.61f)
            || aBodyReanim->ShouldTriggerTimedEvent(0.71f)) {
            float aExplosionPosX = RandRangeFloat(600.0f, 750.0f);
            float aExplosionPosY = RandRangeFloat(50.0f, 300.0f);
            mApp->AddTodParticle(aExplosionPosX, aExplosionPosY, (int)RenderLayer::RENDER_LAYER_TOP, ParticleEffect::PARTICLE_BOSS_EXPLOSION);
            mApp->PlayFoley(FoleyType::FOLEY_BOSS_EXPLOSION_SMALL);
        }

        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aBodyReanim->ShouldTriggerTimedEvent(0.93f)) {
            mBoard->ShakeBoard(1, 2);
            mApp->PlayFoley(FoleyType::FOLEY_BOSS_EXPLOSION_SMALL);
            mApp->PlayFoley(FoleyType::FOLEY_THUMP);
        }

        if (aBodyReanim->ShouldTriggerTimedEvent(0.99f)) {
            aHeadReanim->PlayReanim("anim_flag", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 30.0f);
        }

        if (aHeadReanim->IsAnimPlaying("anim_flag") && aHeadReanim->mLoopCount > 0) {
            aHeadReanim->PlayReanim("anim_flag_loop", ReanimLoopType::REANIM_LOOP, 20, 17.0f);
        }

        if (aBodyReanim->mLoopCount > 0) {
            DropLoot();
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_ZAMBONI && mPhaseCounter > 0) {
        mPhaseCounter--;
        if (mPhaseCounter == 0) {
            aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
            if (aBodyReanim->IsTrackShowing("anim_wheelie2")) {
                mApp->AddTodParticle(mPosX + 80.0f, mPosY + 60.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_ZAMBONI_EXPLOSION2);
            } else {
                mApp->AddTodParticle(mPosX + 80.0f, mPosY + 60.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_ZAMBONI_EXPLOSION);
            }

            DieWithLoot();
            mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);
        }
    } else if (mZombieType == ZombieType::ZOMBIE_CATAPULT) {
        mPhaseCounter--;
        if (mPhaseCounter == 0) {
            mApp->AddTodParticle(mPosX + 80.0f, mPosY + 60.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_CATAPULT_EXPLOSION);
            DieWithLoot();
            mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);
        }
    } else if (mZombieFade == -1 && aBodyReanim->mLoopCount > 0 && mZombieType != ZombieType::ZOMBIE_BOSS) {
        mZombieFade = mInPool ? 10 : 100;
    }
}

void Zombie::Draw(Sexy::Graphics *g) {
    // 根据玩家的“僵尸显血”功能是否开启，决定是否在游戏的原始old_Zombie_Draw函数执行完后额外绘制血量文本。
    old_Zombie_Draw(this, g);
    int drawHeightOffset = mZombieType == ZombieType::ZOMBIE_DOG ? 60 : 0;
    if (showZombieBodyHealth || (showGargantuarHealth && IsGargantuar())) { // 如果玩家开了"僵尸显血"
        if (!IsOnlineServerModeActive()) {
            g->SetColor(gColorWhite);
            g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
            if (mZombieType == ZombieType::ZOMBIE_BOSS) {
                // 如果是僵王,将血量绘制到僵王头顶。从而修复图鉴中僵王血量绘制位置不正确。
                // 此处仅在图鉴中生效,实战中僵王绘制不走Zombie_Draw()，走Zombie_DrawBossPart()
                g->mTransX = 780.0f;
                g->mTransY = 240.0f;
            }
            g->DrawString(StrFormat("%d/%d", mBodyHealth, mBodyMaxHealth), 0, drawHeightOffset);
            g->SetFont(nullptr);
            drawHeightOffset += 20;
        }
    }
    if (showHelmAndShieldHealth && !IsOnlineServerModeActive()) {
        if (mHelmHealth > 0) { // 如果有头盔，绘制头盔血量
            g->SetColor(gColorYellow);
            g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
            g->DrawString(StrFormat("%d/%d", mHelmHealth, mHelmMaxHealth), 0, drawHeightOffset);
            g->SetFont(nullptr);
            drawHeightOffset += 20;
        }
        if (mShieldHealth > 0) { // 如果有盾牌，绘制盾牌血量
            g->SetColor(gColorBlue);
            g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
            g->DrawString(StrFormat("%d/%d", mShieldHealth, mShieldMaxHealth), 0, drawHeightOffset);
            g->SetFont(nullptr);
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_ATTACK) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim != nullptr) {
            Graphics aLightningGraphics(*g);
            if (mTargetCol >= 0) {
                const int aTargetX = mBoard->GridToPixelX(mTargetCol, mRow) + 40;
                const int aClipLeft = aTargetX - mX;
                aLightningGraphics.ClipRect(aClipLeft, -int(aLightningGraphics.mTransY), 800, 600);
            }

            aBodyReanim->DrawRenderGroup(&aLightningGraphics, RENDER_GROUP_GIGA_LIGHTNING);
        }
    }
}

void Zombie::DrawShadow(Graphics *g) {
    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);
    if (mApp->mGameScene == GameScenes::SCENE_ZOMBIES_WON && !SetupDrawZombieWon(g))
        return;

    int aShadowType = 0;
    float aShadowOffsetX = aDrawPos.mImageOffsetX;
    float aShadowOffsetY = aDrawPos.mImageOffsetY + aDrawPos.mBodyY;
    float aScale = mScaleZombie;
    aShadowOffsetX += mScaleZombie * 20.0f - 20.0f;
    if (IsOnBoard() && mBoard->StageIsNight()) {
        aShadowType = 1;
    }

    if (mZombieType == ZombieType::ZOMBIE_FOOTBALL || mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
        if (IsWalkingBackwards()) {
            aShadowOffsetX -= 11.0f * mScaleZombie;
        } else {
            aShadowOffsetX += 20.0f + 21.0f * mScaleZombie;
        }
        aShadowOffsetY += 16.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_NEWSPAPER || mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 5.0f;
        } else {
            aShadowOffsetX += 29.0f;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
        if (IsWalkingBackwards()) {
            aShadowOffsetX += -5.0f;
        } else {
            aShadowOffsetX += 36.0f;
        }
        aShadowOffsetY += 11.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_BOBSLED) {
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 13.0f;
        } else {
            aShadowOffsetX += 20.0f;
        }
        aShadowOffsetY += 13.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_IMP) {
        aScale *= 0.6f;
        aShadowOffsetY += 7.0f;
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 13.0f;
        } else {
            aShadowOffsetX += 25.0f;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_DIGGER || mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD) {
        aShadowOffsetY += 5.0f;
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 14.0f;
        } else {
            aShadowOffsetX += 17.0f;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_SNORKEL) {
        aShadowOffsetY += 5.0f;
        if (IsWalkingBackwards()) {
            aShadowOffsetX -= 2.0f;
        } else {
            aShadowOffsetX += 35.0f;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER) {
        aShadowOffsetY += 11.0f;
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 15.0f;
        } else {
            aShadowOffsetX += 19.0f;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_YETI) {
        aShadowOffsetY += 20.0f;
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 20.0f;
        } else {
            aShadowOffsetX += 3.0f;
        }
    } else if (IsGargantuar()) {
        aScale *= 1.5f;
        aShadowOffsetX += 27.0f;
        aShadowOffsetY += 7.0f;
    } else if (mApp->ReanimationTryToGet(mBodyReanimID) != nullptr) {
        if (IsWalkingBackwards()) {
            aShadowOffsetX += 11.0f;
        } else {
            aShadowOffsetX += 23.0f;
        }
    } else {
        if (IsWalkingBackwards()) {
            aShadowOffsetX -= 2.0f;
        } else {
            aShadowOffsetX += 35.0f;
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_NEWSPAPER || mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        aShadowOffsetY += 4.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
        aShadowOffsetY += 13.0f;
    } else if (mZombieType == ZombieType::ZOMBIE_BUNGEE) {
        aShadowOffsetX -= 12.0f;
        aScale = TodAnimateCurveFloat(BUNGEE_ZOMBIE_HEIGHT - 1000, 100, mAltitude, 0.1f, 1.5f, TodCurves::CURVE_LINEAR);
    }

    if (mZombieHeight == ZombieHeight::HEIGHT_UP_LADDER || mZombieHeight == ZombieHeight::HEIGHT_FALLING || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN
        || mZombieType == ZombieType::ZOMBIE_BUNGEE || IsBouncingPogo() || IsFlying()) {
        aShadowOffsetY += mAltitude;
        if (mOnHighGround) {
            aShadowOffsetY -= HIGH_GROUND_HEIGHT;
        }
    }

    if (mInPool) {
        TodDrawImageCenterScaledF(g, IMAGE_WHITEWATER_SHADOW, aShadowOffsetX, aShadowOffsetY + 67.0f, aScale, aScale);
    } else {
        if (aShadowType == 0) {
            TodDrawImageCelCenterScaledF(g, IMAGE_ZOMBIESHADOW, aShadowOffsetX, aShadowOffsetY + 92.0f, 0, aScale, aScale);
        } else {
            TodDrawImageCelCenterScaledF(g, IMAGE_ZOMBIESHADOW2, aShadowOffsetX, aShadowOffsetY + 92.0f, mInPool, aScale, aScale);
        }
    }

    g->ClearClipRect();
}

void Zombie::DrawBossPart(Sexy::Graphics *g, int theBossPart) {
    // 根据玩家的“僵尸显血”功能是否开启，决定是否在游戏的原始old_Zombie_DrawBossPart函数执行完后额外绘制血量文本。
    old_Zombie_DrawBossPart(this, g, theBossPart);
    if (theBossPart == 3) {
        // 每次绘制Boss都会调用四次本函数，且theBossPart从0到3依次增加，代表绘制Boss的不同Part。
        // 我们只在theBossPart==3时(绘制最后一个部分时)绘制一次血量，免去每次都绘制。
        if (showZombieBodyHealth && !IsOnlineServerModeActive()) { // 如果玩家开了"僵尸显血"
            pvzstl::string str = StrFormat("%d/%d", mBodyHealth, mBodyMaxHealth);
            g->SetColor(gColorWhite);
            g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
            float tmpTransX = g->mTransX;
            float tmpTransY = g->mTransY;
            g->mTransX = 800.0f;
            g->mTransY = 200.0f;
            g->DrawString(str, 0, 0);
            g->mTransX = tmpTransX;
            g->mTransY = tmpTransY;
            g->SetFont(nullptr);
        }
    }
}

int Zombie::GetDancerFrame() {
    if (mFromWave == -3 || IsImmobilizied())
        return 0;

    // 女仆秘籍
    if (maidCheats > 0 && !IsOnlineServerModeActive() && !gIsReplayMode) {
        switch (maidCheats) {
            case 1:
                return 11; // 保持前进 (DancerDancingLeft)
            case 2:
                return 18; // 跳舞 (DancerRaiseLeft1)
            case 3:
                return 12; // 召唤舞伴 (DancerWalkToRaise)
            default:
                break;
        }
    }

    int aFrameLength = 20;
    int aFramesCount = 23;
    if (mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN) {
        aFramesCount = 11;
        aFrameLength = 10;
    }
    // 修复女仆秘籍问题、修复舞王和舞者的跳舞时间不吃高级暂停也不吃倍速
    if (mBoard) {
        // 关键就是用 mBoard->mMainCounter 代替 mApp->mAppCounter 做计时
        return (mBoard->mMainCounter % (aFrameLength * aFramesCount)) / aFrameLength;
    } else {
        return (mApp->mAppCounter % (aFrameLength * aFramesCount)) / aFrameLength;
    }
}

bool Zombie::IsGargantuar() const {
    return IsGargantuar(mZombieType);
}

bool Zombie::IsGargantuar(ZombieType theZombieType) {
    return theZombieType == ZombieType::ZOMBIE_GARGANTUAR || theZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR || theZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR
        || theZombieType == ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR;
}

bool Zombie::IsZombotany(ZombieType theZombieType) {
    return theZombieType == ZombieType::ZOMBIE_PEA_HEAD || theZombieType == ZombieType::ZOMBIE_WALLNUT_HEAD || theZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD
        || theZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD || theZombieType == ZombieType::ZOMBIE_GATLING_HEAD || theZombieType == ZombieType::ZOMBIE_SQUASH_HEAD;
}

bool Zombie::IsZomblob(ZombieType theZombieType) {
    return theZombieType == ZombieType::ZOMBIE_ZOMBLOB || theZombieType == ZombieType::ZOMBIE_ZOMBLOB_MIDDLE || theZombieType == ZombieType::ZOMBIE_ZOMBLOB_SMALL;
}

bool Zombie::ZombieTypeCanGoInPool(ZombieType theZombieType) {
    // 修复泳池对战的僵尸走水路时不索敌植物
    if ((gLawnApp)->IsVSMode()) {
        if (gVSBackground == BackgroundType::BACKGROUND_3_POOL || gVSBackground == BackgroundType::BACKGROUND_4_FOG)
            return theZombieType != ZombieType::ZOMBIE_BUNGEE; // 蹦极不能落水
    }

    return theZombieType == ZombieType::ZOMBIE_NORMAL        //
        || theZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE  //
        || theZombieType == ZombieType::ZOMBIE_PAIL          //
        || theZombieType == ZombieType::ZOMBIE_FLAG          //
        || theZombieType == ZombieType::ZOMBIE_BALLOON       //
        || theZombieType == ZombieType::ZOMBIE_SNORKEL       //
        || theZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER //
        || theZombieType == ZombieType::ZOMBIE_PEA_HEAD      //
        || theZombieType == ZombieType::ZOMBIE_WALLNUT_HEAD  //
        || theZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD //
        || theZombieType == ZombieType::ZOMBIE_GATLING_HEAD  //
        || theZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD;
}

Rect Zombie::GetZombieRect() {
    Rect aZombieRect = mZombieRect;
    if (IsWalkingBackwards()) {
        aZombieRect.mX = mWidth - aZombieRect.mX - aZombieRect.mWidth;
    }

    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);
    aZombieRect.Offset(mX, mY + aDrawPos.mBodyY);
    if (aDrawPos.mClipHeight > CLIP_HEIGHT_LIMIT) {
        aZombieRect.mHeight -= aDrawPos.mClipHeight;
    }

    return aZombieRect;
}

void Zombie::RiseFromGrave(int theCol, int theRow) {
    mPosX = mBoard->GridToPixelX(theCol, mRow) - 25;
    mPosY = GetPosYBasedOnRow(theRow);
    SetRow(theRow);
    mX = int(mPosX);
    mY = int(mPosY);
    mAltitude = CLIP_HEIGHT_OFF;
    mZombiePhase = ZombiePhase::PHASE_RISING_FROM_GRAVE;
    mPhaseCounter = 150;

    if (mBoard->StageHasPool() && mBoard->mPlantRow[theRow] == PlantRowType::PLANTROW_POOL /*修复陆路僵尸触发水路特效*/) {
        mAltitude = -150.0f;
        mInPool = true;
        mPhaseCounter = 50;
        mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;

        StartWalkAnim(0);
        ReanimIgnoreClipRect("Zombie_duckytube", false);
        ReanimIgnoreClipRect("Zombie_whitewater", false);
        ReanimIgnoreClipRect("Zombie_outerarm_hand", false);
        ReanimIgnoreClipRect("Zombie_innerarm3", false);

        if (GetZombieDefinition(mZombieType).mReanimationType == ReanimationType::REANIM_ZOMBIE) { // 修复泳池放置非领带类僵尸闪退
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            TodParticleSystem *aParticle = mApp->AddTodParticle(0.0f, 0.0f, 0, ParticleEffect::PARTICLE_ZOMBIE_SEAWEED);
            OverrideParticleScale(aParticle);

            if (mZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE && aParticle) {
                aBodyReanim->AttachParticleToTrack("anim_cone", aParticle, 37.0f, 20.0f);
            } else if (mZombieType == ZombieType::ZOMBIE_PAIL && aParticle) {
                aBodyReanim->AttachParticleToTrack("anim_bucket", aParticle, 37.0f, 20.0f);
            } else if (aParticle) {
                aBodyReanim->AttachParticleToTrack("anim_head1", aParticle, 30.0f, 20.0f);
            }

            TodParticleSystem *aParticle2 = mApp->AddTodParticle(0.0f, 0.0f, 0, ParticleEffect::PARTICLE_ZOMBIE_SEAWEED);
            if (aParticle2) {
                OverrideParticleScale(aParticle2);
                aBodyReanim->AttachParticleToTrack("Zombie_outerarm_upper", aParticle2, 5.0f, 5.0f);
            }

            TodParticleSystem *aParticle3 = mApp->AddTodParticle(0.0f, 0.0f, 0, ParticleEffect::PARTICLE_ZOMBIE_SEAWEED);
            if (aParticle3) {
                OverrideParticleScale(aParticle3);
                aBodyReanim->AttachParticleToTrack("Zombie_duckytube", aParticle3, 77.0f, 20.0f);
            }
        }

        PoolSplash(false);
    } else {
        int aParticleX = mPosX + 60;
        int aParticleY = mPosY + 110;
        if (IsOnHighGround()) {
            aParticleY -= HIGH_GROUND_HEIGHT;
        }

        int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, theRow, 0);
        if (mApp->IsWhackAZombieLevel()) {
            mApp->PlayFoley(FoleyType::FOLEY_DIRT_RISE);
            mApp->AddTodParticle(aParticleX, aParticleY, aRenderOrder, ParticleEffect::PARTICLE_WHACK_A_ZOMBIE_RISE);
        } else {
            mApp->PlayFoley(FoleyType::FOLEY_GRAVESTONE_RUMBLE);
            mApp->AddTodParticle(aParticleX, aParticleY, aRenderOrder, ParticleEffect::PARTICLE_ZOMBIE_RISE);
        }
    }

    if (IsRemoteServer()) {
        U8U8U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_RIZE_FORM_GRAVE}, uint8_t(theCol), uint8_t(theRow), uint16_t(mBoard->mZombies.DataArrayGetID(this))};
        netplay::PutEvent(event);
    }
}

void Zombie::CheckForBoardEdge() {
    // 修复僵尸正常进家、支持调整僵尸进家线

    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (IsWalkingBackwards() && mPosX > 850.0f) {
        // 雪人成功逃跑属于离场，不结算阳光豆储存的阳光
        if (mZombieType == ZombieType::ZOMBIE_YETI && mZombiePhase == ZombiePhase::PHASE_YETI_RUNNING) {
            mSunBeanSun = 0;
            mSunBeanDamageRemainder = 0;
        }
        DieNoLoot();
        return;
    }
    int boardEdge = 0;
    if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER || IsGargantuar()) {
        // 如果是撑杆、巨人、红眼巨人
        boardEdge = -100;
    } else if (mZombieType == ZombieType::ZOMBIE_FOOTBALL || mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombieType == ZombieType::ZOMBIE_CATAPULT
               || mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
        // 如果是橄榄球、冰车、篮球
        boardEdge = -125;
    } else if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER || mZombieType == ZombieType::ZOMBIE_SNORKEL) {
        // 如果是舞王、伴舞、潜水
        boardEdge = -80;
    } else {
        // 如果是除上述僵尸外的僵尸
        boardEdge = -50;
    }
    if (boardEdgeAdjust > 0 && !IsOnlineServerModeActive() && !gIsReplayMode) {
        boardEdge -= boardEdgeAdjust; // 支持任意调整进家线
    }
    if (mX <= boardEdge && mHasHead) {
        if (mApp->IsIZombieLevel()) {
            DieNoLoot();
        } else {
            if (IsRemoteServer()) {
                U16_Event zombieWinEvent = {{EVENT_SERVER_BOARD_ZOMBIE_WIN}, uint16_t(mBoard->mZombies.DataArrayGetID(this))};
                netplay::PutEvent(zombieWinEvent);
                if (mApp->IsVSMode()) {
                    // 所选对战场地
                    netplay::MetricsSetVsBackground(int(gVSBackground));
                    // 对战游戏模式
                    netplay::MetricsSetShuffleMode(Challenge::msVSShuffleMode);
                    // 僵尸胜利的对局时间
                    netplay::MetricsSendSettlement(false, mBoard->mMainCounter);
                }
            }

            mBoard->ZombiesWon(this);
        }
    }
    if (mX <= boardEdge + 70 && !mHasHead) {
        TakeDamage(1800, 9u);
    }
}

void Zombie::SetupDoorArms(Reanimation *aReanim, bool theShow) {
    int aArmGroup = RENDER_GROUP_NORMAL;
    int aDoorGroup = RENDER_GROUP_HIDDEN;
    int aHandGroup = RENDER_GROUP_HIDDEN;
    if (theShow) {
        aArmGroup = RENDER_GROUP_HIDDEN;
        aDoorGroup = RENDER_GROUP_NORMAL;
        aHandGroup = 3;
    }

    aReanim->AssignRenderGroupToPrefix("Zombie_outerarm_hand", aArmGroup);
    aReanim->AssignRenderGroupToPrefix("Zombie_outerarm_lower", aArmGroup);
    aReanim->AssignRenderGroupToPrefix("Zombie_outerarm_upper", aArmGroup);
    aReanim->AssignRenderGroupToPrefix("anim_innerarm", aArmGroup);
    aReanim->AssignRenderGroupToPrefix("Zombie_innerarm_screendoor", aDoorGroup);
    aReanim->AssignRenderGroupToPrefix("Zombie_innerarm_screendoor_hand", aHandGroup);
    aReanim->AssignRenderGroupToPrefix("Zombie_outerarm_screendoor", aHandGroup);
}

void Zombie::SetupReanimLayers(Reanimation *aReanim, ZombieType theZombieType) {
    aReanim->AssignRenderGroupToPrefix("anim_cone", RENDER_GROUP_HIDDEN);
    aReanim->AssignRenderGroupToPrefix("anim_bucket", RENDER_GROUP_HIDDEN);
    aReanim->AssignRenderGroupToPrefix("anim_screendoor", RENDER_GROUP_HIDDEN);
    aReanim->AssignRenderGroupToPrefix("Zombie_flaghand", RENDER_GROUP_HIDDEN);
    aReanim->AssignRenderGroupToPrefix("Zombie_duckytube", RENDER_GROUP_HIDDEN);
    aReanim->AssignRenderGroupToPrefix("anim_tongue", RENDER_GROUP_HIDDEN);
    aReanim->AssignRenderGroupToPrefix("Zombie_mustache", RENDER_GROUP_HIDDEN);
    SetupDoorArms(aReanim, false);

    if (theZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE) {
        aReanim->AssignRenderGroupToPrefix("anim_cone", RENDER_GROUP_NORMAL);
        aReanim->AssignRenderGroupToPrefix("anim_hair", RENDER_GROUP_HIDDEN);
    } else if (theZombieType == ZombieType::ZOMBIE_PAIL) {
        aReanim->AssignRenderGroupToPrefix("anim_bucket", RENDER_GROUP_NORMAL);
        aReanim->AssignRenderGroupToPrefix("anim_hair", RENDER_GROUP_HIDDEN);
    } else if (theZombieType == ZombieType::ZOMBIE_DOOR) {
        SetupDoorArms(aReanim, true);
    } else if (theZombieType == ZombieType::ZOMBIE_NEWSPAPER || theZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        aReanim->AssignRenderGroupToPrefix("Zombie_paper_paper", RENDER_GROUP_HIDDEN);
    } else if (theZombieType == ZombieType::ZOMBIE_FLAG) {
        aReanim->AssignRenderGroupToPrefix("anim_innerarm", RENDER_GROUP_HIDDEN);
        aReanim->AssignRenderGroupToTrack("Zombie_flaghand", RENDER_GROUP_NORMAL);
        aReanim->AssignRenderGroupToTrack("Zombie_innerarm_screendoor", RENDER_GROUP_NORMAL);
    } else if (theZombieType == ZombieType::ZOMBIE_DUCKY_TUBE) {
        aReanim->AssignRenderGroupToPrefix("Zombie_duckytube", RENDER_GROUP_NORMAL);
    }
}

void Zombie::ShowDoorArms(bool theShow) {
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
    if (aBodyReanim) {
        SetupDoorArms(aBodyReanim, theShow);
        if (!mHasArm) {
            ReanimShowPrefix("Zombie_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_outerarm_hand", RENDER_GROUP_HIDDEN);
        }
    }
}

void Zombie::StartEating() {
    // 为确保视觉的流畅性允许客户端擅自触发啃咬，仅权威同步僵尸的位置以优化高移速僵尸“借过”植物的问题
    if (mApp->mGameScene == SCENE_PLAYING) {
        //        if (IsRemoteClientOrViewer()) {
        //            return;
        //        }

        if (mIsEating) {
            return;
        }

        if (IsRemoteServer()) {
            const auto syncEatingPosition = [this](Zombie *theZombie) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_START_EATING;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(theZombie));
                event.data2.f32 = theZombie->mPosX;
                netplay::PutEvent(event);
            };
            syncEatingPosition(this);

            if (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) {
                Zombie *aPartner = GetDogPartner();
                if (aPartner != nullptr && aPartner->mHasHead && !aPartner->IsDeadOrDying() && aPartner->mMindControlled == mMindControlled) {
                    syncEatingPosition(aPartner);
                }
            }
        }
    }

    StartEating_Origin();
}

void Zombie::StartEating_Origin() {
    if (mIsEating)
        return;

    mIsEating = true;

    if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING)
        return;

    if (mZombiePhase == ZombiePhase::PHASE_LADDER_CARRYING) {
        PlayZombieReanim("anim_laddereat", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD) {
        PlayZombieReanim("anim_eat_nopaper", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
    } else {
        if (mZombieType != ZombieType::ZOMBIE_SNORKEL) {
            PlayZombieReanim("anim_eat", ReanimLoopType::REANIM_LOOP, 20, 0.0f);
        }

        if (mShieldType == ShieldType::SHIELDTYPE_DOOR || mShieldType == ShieldType::SHIELDTYPE_TRASHCAN) {
            ShowDoorArms(false);
        }
    }
}

void Zombie::StopEating() {
    if (!mIsEating)
        return;

    mIsEating = false;
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);

    if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING)
        return;

    if (aBodyReanim && mZombieType != ZombieType::ZOMBIE_SNORKEL) {
        StartWalkAnim(20);
    }

    if (mShieldType == ShieldType::SHIELDTYPE_DOOR || mShieldType == ShieldType::SHIELDTYPE_TRASHCAN) {
        ShowDoorArms(true);
    }

    UpdateAnimSpeed();
}

void Zombie::EatPlant(Plant *thePlant) {
    if (mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN) {
        mPhaseCounter = 1;
        return;
    }

    if (mZombieType == ZombieType::ZOMBIE_JACKSON && mSummonCounter == 0) {
        if (!msDeadFollowers.empty() && mHasHead && mPosX < 700.0f) {
            StopEating();
            if (!IsRemoteClientOrViewer()) {
                mZombiePhase = ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT;
                PlayZombieReanim("anim_point", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            }
            if (IsRemoteServer()) {
                U8U8U16U16_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
                event.data1 = uint8_t(mZombiePhase);
                event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data4 = uint16_t(mPhaseCounter);
                netplay::PutEvent(event);
            }
            return;
        }
    }

    if (mYuckyFace) {
        return;
    }

    // 修复正向出土的矿工不上梯子
    if (mBoard->GetLadderAt(thePlant->mPlantCol, thePlant->mRow) && (mZombieType != ZombieType::ZOMBIE_DIGGER || !IsWalkingBackwards())) {
        StopEating();

        if (mZombieHeight == ZombieHeight::HEIGHT_ZOMBIE_NORMAL && mUseLadderCol != thePlant->mPlantCol) {
            mZombieHeight = ZombieHeight::HEIGHT_UP_LADDER;
            mUseLadderCol = thePlant->mPlantCol;
        }

        if (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) {
            Zombie *aPartner = GetDogPartner();
            if (aPartner != nullptr) {
                HandleDogPartnerLost();
                aPartner->HandleDogPartnerLost();
                return;
            }
        }

        return;
    }

    StartEating();

    if (mZombieType == ZombieType::ZOMBIE_SQUASH_HEAD && mZombiePhase == ZombiePhase::PHASE_SQUASH_PRE_LAUNCH && mSquashHeadCol == -1) {
        mSquashHeadCol = thePlant->mPlantCol; // 修复窝瓜僵尸索敌冰冻生菜时被冻住，解冻后头部飘移
    }

    if (thePlant->mSeedType == SeedType::SEED_JALAPENO || thePlant->mSeedType == SeedType::SEED_CHERRYBOMB || thePlant->mSeedType == SeedType::SEED_DOOMSHROOM
        || thePlant->mSeedType == SeedType::SEED_ICESHROOM || thePlant->mSeedType == SeedType::SEED_HYPNOSHROOM || thePlant->mState == PlantState::STATE_FLOWERPOT_INVULNERABLE
        || thePlant->mState == PlantState::STATE_LILYPAD_INVULNERABLE || thePlant->mState == PlantState::STATE_SQUASH_LOOK || thePlant->mState == PlantState::STATE_SQUASH_PRE_LAUNCH
        || thePlant->mSeedType == SeedType::SEED_ICEBERG_LETTUCE || thePlant->mSeedType == SeedType::SEED_CHILLY_PEPPER) {
        if (!thePlant->mIsAsleep) {
            return;
        }
    }
    if (thePlant->mSeedType == SeedType::SEED_POTATOMINE && thePlant->mState != PlantState::STATE_NOTREADY) {
        return;
    }

    bool triggered = false;
    if (thePlant->mSeedType == SeedType::SEED_BLOVER) {
        triggered = true;
    }
    if (thePlant->mSeedType == SeedType::SEED_ICESHROOM && !thePlant->mIsAsleep) {
        triggered = true;
    }
    if (triggered) {
        thePlant->DoSpecial();
        return;
    }

    if (mChilledCounter > 0 && mZombieAge % 2 == 1)
        return;

    if (mApp->IsIZombieLevel() && thePlant->mSeedType == SeedType::SEED_SUNFLOWER) // IZ模式下啃咬向日葵
    {
        int aStageBeforeChew = thePlant->mPlantHealth / 40;
        int aStageAfterChew = (thePlant->mPlantHealth - DAMAGE_PER_EAT) / 40;
        if (aStageAfterChew < aStageBeforeChew || thePlant->mPlantHealth - DAMAGE_PER_EAT <= 0) // 若本次啃食令植物血量下降了至少 1 个阶段
        {
            mBoard->AddCoin(thePlant->mX, thePlant->mY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
        }
    }

    thePlant->mPlantHealth -= DAMAGE_PER_EAT;
    thePlant->mRecentlyEatenCountdown = 50;
    auto absorbedEating = [this, &thePlant]() {
        if (!Plant::IsDefender(thePlant->mSeedType) || mJustGotShotCounter >= -500) {
            return false;
        }
        if (mApp->IsIZombieLevel()) {
            return true;
        }
        if (mApp->IsVSMode() && VSSetupAddonWidget::msBalancePatchMode) {
            return mZombieType != ZombieType::ZOMBIE_NORMAL && mZombieType != ZombieType::ZOMBIE_BOBSLED && mZombieType != ZombieType::ZOMBIE_PEA_HEAD && mZombieType != ZombieType::ZOMBIE_DOG;
        }
        return false;
    };
    if (absorbedEating()) {
        thePlant->mPlantHealth -= DAMAGE_PER_EAT;
    }
    if (mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD) {
            thePlant->mPlantHealth -= DAMAGE_PER_EAT * 3; // 4 倍啃咬伤害
        } else {
            thePlant->mPlantHealth -= DAMAGE_PER_EAT; // 2 倍啃咬伤害
        }
    }
    if (mZombieType == ZombieType::ZOMBIE_DOG) {
        thePlant->mPlantHealth -= DAMAGE_PER_EAT; // 僵尸狗固定为 2 倍啃咬伤害
    }

    if (thePlant->mPlantHealth <= 0) {
        if (!IsRemoteClientOrViewer()) {
            mApp->PlaySample(SOUND_GULP);
        }
        if (IsRemoteServer()) {
            U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 0};
            netplay::PutEvent(event);
        }

        if (thePlant->mSeedType == SeedType::SEED_IMP_PEAR) {
            mApp->PlayFoley(FoleyType::FOLEY_FLOOP);

            ConvertToImp();
            mApp->AddTodParticle(mPosX + 60.0f, mPosY + 40.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_MIND_CONTROL);
            TrySpawnLevelAward();

            mVelX = 0.17f;
            mAnimTicksPerFrame = 18;
            UpdateAnimSpeed();
        }

        mBoard->mPlantsEaten++;
        thePlant->Die();
        mBoard->mChallenge->ZombieAtePlant(this, thePlant);

        if (mBoard->mLevel >= 2 && mBoard->mLevel <= 4 && mApp->IsFirstTimeAdventureMode()) {
            if (thePlant->mPlantCol > 4 && mBoard->mPlants.mSize < 15 && thePlant->mSeedType == SeedType::SEED_PEASHOOTER) {
                mBoard->DisplayAdvice("[ADVICE_PEASHOOTER_DIED]", MessageStyle::MESSAGE_STYLE_HINT_TALL_FAST, AdviceType::ADVICE_PEASHOOTER_DIED);
            }
        }
    }
}

void Zombie::EatZombie(Zombie *theZombie) {
    theZombie->TakeDamage(DAMAGE_PER_EAT, 9U);
    if (mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD) {
            theZombie->TakeDamage(DAMAGE_PER_EAT * 3, 9U);
        } else {
            theZombie->TakeDamage(DAMAGE_PER_EAT, 9U);
        }
    }
    if (mZombieType == ZombieType::ZOMBIE_DOG) {
        theZombie->TakeDamage(DAMAGE_PER_EAT, 9U);
    }
    StartEating();
    if (theZombie->mBodyHealth <= 0) {
        mApp->PlaySample(SOUND_GULP);
    }
}

void Zombie::DetachShield() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim) {
        if (mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION) {
            ReanimShowPrefix("Zombie_paper_hands", RENDER_GROUP_NORMAL);
        }
    }

    old_Zombie_DetachShield(this);

    // 修复扶梯僵尸搭梯后断臂重生的 Bug
    if (mShieldType == ShieldType::SHIELDTYPE_LADDER && !mHasArm) {
        ReanimShowPrefix("Zombie_outerarm", RENDER_GROUP_HIDDEN);
    }
}

void Zombie::BossSpawnAttack() {
    // 修复泳池僵王为六路放僵尸时闪退
    RemoveColdEffects();
    mZombiePhase = ZombiePhase::PHASE_BOSS_SPAWNING;
    if (mBossMode == 0) {
        mSummonCounter = RandRangeInt(450, 550);
    } else if (mBossMode == 1) {
        mSummonCounter = RandRangeInt(350, 450);
    } else if (mBossMode == 2) {
        mSummonCounter = RandRangeInt(150, 250);
    }
    mTargetRow = mBoard->PickRowForNewZombie(ZombieType::ZOMBIE_NORMAL);
    switch (mTargetRow) {
        case 0:
            PlayZombieReanim("anim_spawn_1", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0);
            break;
        case 1:
            PlayZombieReanim("anim_spawn_2", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0);
            break;
        case 2:
            PlayZombieReanim("anim_spawn_3", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0);
            break;
        case 3:
            PlayZombieReanim("anim_spawn_4", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0);
            break;
        default:
            PlayZombieReanim("anim_spawn_5", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0);
            break;
    }

    mApp->PlayFoley(FoleyType::FOLEY_HYDRAULIC_SHORT);
}

bool Zombie::IsBouncingPogo() const {
    return mZombiePhase >= ZombiePhase::PHASE_POGO_BOUNCING && mZombiePhase <= ZombiePhase::PHASE_POGO_FORWARD_BOUNCE_7;
}

void Zombie::UpdateZombiePogo() {
    if (IsDeadOrDying() || IsImmobilizied() || !IsBouncingPogo() || mZombieHeight == ZombieHeight::HEIGHT_IN_TO_CHIMNEY
        /*|| mZombieHeight == ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED*/ /* 被蹦极空投时不更新 */)
        return;

    float aHeight = 40.0f;
    if (mZombiePhase >= ZombiePhase::PHASE_POGO_HIGH_BOUNCE_1 && mZombiePhase <= ZombiePhase::PHASE_POGO_HIGH_BOUNCE_6) {
        aHeight = 50.0f + 20.0f * (mZombiePhase - ZombiePhase::PHASE_POGO_HIGH_BOUNCE_1);
    } else if (mZombiePhase == ZombiePhase::PHASE_POGO_FORWARD_BOUNCE_2) {
        aHeight = 90.0f;
    } else if (mZombiePhase == ZombiePhase::PHASE_POGO_FORWARD_BOUNCE_7) {
        aHeight = 170.0f;
    }
    mAltitude = TodAnimateCurveFloat(POGO_BOUNCE_TIME, 0, mPhaseCounter, 9.0f, aHeight + 9.0f, TodCurves::CURVE_BOUNCE_SLOW_MIDDLE);
    mFrame = ClampInt(3 - mAltitude / 3, 0, 3);

    if (mPhaseCounter == 7) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        aBodyReanim->mAnimTime = 0.0f;
        aBodyReanim->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD;
    }
    if (IsOnBoard() && mPhaseCounter == 5) {
        mApp->PlayFoley(FoleyType::FOLEY_POGO_ZOMBIE);
    }

    if (mZombieHeight == ZombieHeight::HEIGHT_UP_TO_HIGH_GROUND) {
        mAltitude += HIGH_GROUND_HEIGHT;
        mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;
    } else if (mZombieHeight == ZombieHeight::HEIGHT_DOWN_OFF_HIGH_GROUND) {
        mOnHighGround = false;
        mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;
    } else if (mOnHighGround) {
        mAltitude += HIGH_GROUND_HEIGHT;
    }

    if (mZombiePhase == ZombiePhase::PHASE_POGO_FORWARD_BOUNCE_2 && mPhaseCounter == 70) {
        Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT);
        if (aPlant && aPlant->mSeedType == SeedType::SEED_TALLNUT) {
            mApp->PlayFoley(FoleyType::FOLEY_BONK);
            mApp->AddTodParticle(aPlant->mX + 60, aPlant->mY - 20, mRenderOrder + 1, ParticleEffect::PARTICLE_TALL_NUT_BLOCK);

            mShieldType = ShieldType::SHIELDTYPE_NONE;
            PogoBreak(0U);
            return;
        }
    }

    if (mPhaseCounter != 0)
        return;

    Plant *aPlant = nullptr;
    if (IsOnBoard()) {
        aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_VAULT);
    }
    if (aPlant == nullptr) {
        mZombiePhase = ZombiePhase::PHASE_POGO_BOUNCING;

        PickRandomSpeed();
        mPhaseCounter = POGO_BOUNCE_TIME;
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_POGO_HIGH_BOUNCE_1) {
        mZombiePhase = ZombiePhase::PHASE_POGO_FORWARD_BOUNCE_2;
        mVelX = (mX - aPlant->mX + 60) / (float)POGO_BOUNCE_TIME; // 速度 = 跳跃距离 / 跳跃时间
        mPhaseCounter = POGO_BOUNCE_TIME;
    } else {
        mZombiePhase = ZombiePhase::PHASE_POGO_HIGH_BOUNCE_1;
        mVelX = 0.0f;
        mPhaseCounter = POGO_BOUNCE_TIME;
    }
}

bool Zombie::IsFlying() const {
    return mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING || mZombiePhase == ZombiePhase::PHASE_BALLOON_POPPING;
}

bool Zombie::IsImpFlying() const {
    if (mZombieType != ZombieType::ZOMBIE_SUPER_FAN_IMP && mZombieType != ZombieType::ZOMBIE_GIGA_IMP) {
        return false;
    }
    return mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_BLOCKED;
}

int Zombie::GetBobsledPosition() {
    return old_Zombie_GetBobsledPosition(this);
}

void Zombie::BobsledCrash() {
    mAltitude = 0.0f;
    mZombieRect = Rect(36, 0, 42, 115);
    mZombiePhase = ZombiePhase::PHASE_BOBSLED_CRASHING;
    mPhaseCounter = BOBSLED_CRASH_TIME;
    StartWalkAnim(0);

    Reanimation *aLeaderReanim = mApp->ReanimationGet(mBodyReanimID);
    for (int i = 0; i < NUM_BOBSLED_FOLLOWERS; i++) {
        Zombie *aFollowerZombie = mBoard->ZombieGet(mFollowerZombieID[i]);
        if (aFollowerZombie == nullptr) {
            continue;
        }

        aFollowerZombie->mZombiePhase = ZombiePhase::PHASE_BOBSLED_CRASHING;
        aFollowerZombie->mPhaseCounter = BOBSLED_CRASH_TIME;
        aFollowerZombie->mPosY = GetPosYBasedOnRow(mRow);
        aFollowerZombie->mAltitude = 0.0f;
        aFollowerZombie->StartWalkAnim(0);

        Reanimation *aFollowerReanim = mApp->ReanimationGet(aFollowerZombie->mBodyReanimID);
        if (aFollowerReanim) {
            aFollowerZombie->mVelX = mVelX;
            aFollowerReanim->mAnimTime = RandRangeFloat(0.0f, 1.0f);
            if (aLeaderReanim) {
                aFollowerReanim->mAnimRate = aLeaderReanim->mAnimRate;
            }
        }
    }
}

bool Zombie::IsBobsledTeamWithSled() {
    return GetBobsledPosition() != -1;
}

void Zombie::UpdateZombieBobsled() {
    if (mZombiePhase == ZombiePhase::PHASE_BOBSLED_CRASHING) {
        if (mPhaseCounter == 0) {
            mZombiePhase = ZombiePhase::PHASE_ZOMBIE_NORMAL;
            if (GetBobsledPosition() == 0) {
                if (IsRemoteClientOrViewer())
                    return;

                if (IsRemoteServer()) {
                    U8x2U16x4UNI32x8_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_BOBSLED_PICK_SPEED;
                    event.data2[0] = mBoard->ZombieGetID(this);
                    event.data3[0].f32 = mVelX;
                    event.data4[0].f32 = mPosX;
                    for (int i = 0; i < NUM_BOBSLED_FOLLOWERS; i++) {
                        event.data2[i + 1] = mFollowerZombieID[i + 1];
                        Zombie *aZombie = mBoard->ZombieTryToGet(mFollowerZombieID[i]);
                        if (aZombie) {
                            event.data3[i + 1].f32 = aZombie->mVelX;
                            event.data4[i + 1].f32 = aZombie->mPosX;
                        }
                    }
                    netplay::PutEvent(event);
                }

                for (int i = 0; i < NUM_BOBSLED_FOLLOWERS; i++) {
                    Zombie *aZombie = mBoard->ZombieGet(mFollowerZombieID[i]);
                    if (aZombie == nullptr) {
                        continue;
                    }
                    aZombie->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
                    mFollowerZombieID[i] = ZombieID::ZOMBIEID_NULL;
                    aZombie->PickRandomSpeed();
                }
                PickRandomSpeed();
            }
        }
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_BOBSLED_SLIDING) {
        if (mPhaseCounter == 0) {
            mZombiePhase = ZombiePhase::PHASE_BOBSLED_BOARDING;
            PlayZombieReanim("anim_jump", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 20.0f);
        }
    } else {
        if (mZombiePhase != ZombiePhase::PHASE_BOBSLED_BOARDING) {
            return;
        }

        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        int aCounter = int(aBodyReanim->mAnimTime * 50.0f);
        int aPosition = GetBobsledPosition();
        if (aPosition == 1 || aPosition == 3) {
            mAltitude = TodAnimateCurveFloat(0, 50, aCounter, 8.0f, 18.0f, TodCurves::CURVE_LINEAR);
        } else {
            mAltitude = TodAnimateCurveFloat(0, 50, aCounter, -9.0f, 18.0f, TodCurves::CURVE_LINEAR);
        }
    }

    if (mBoard->mIceTimer[mRow] < 500) {
        mBoard->mIceTimer[mRow] = 500;
    }
    if (mPosX + 10.0f < mBoard->mIceMinX[mRow] && GetBobsledPosition() == 0) {
        TakeDamage(6, 8U);
    }
}

bool Zombie::IsDeadOrDying() const {
    return mDead || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED;
}

bool Zombie::CanBeChilled() {
    if (mZombieType == ZombieType::ZOMBIE_ZAMBONI || IsBobsledTeamWithSled())
        return false;

    if (IsDeadOrDying())
        return false;

    if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE || mZombiePhase == ZombiePhase::PHASE_DANCER_RISING)
        return false;

    if (mMindControlled)
        return false;

    return mZombieType != ZombieType::ZOMBIE_BOSS || mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_IDLE_BEFORE_SPIT || mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_IDLE_AFTER_SPIT
        || mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_SPIT;
}

bool Zombie::CanBeFrozen() {
    if (!CanBeChilled())
        return false;

    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP
        || mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL || IsFlying() || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN || mZombiePhase == ZombiePhase::PHASE_IMP_LANDING
        || mZombiePhase == ZombiePhase::PHASE_BOBSLED_CRASHING || mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING || mZombiePhase == ZombiePhase::PHASE_SQUASH_RISING
        || mZombiePhase == ZombiePhase::PHASE_SQUASH_FALLING || mZombiePhase == ZombiePhase::PHASE_SQUASH_DONE_FALLING || IsBouncingPogo() || mZombiePhase == ZombiePhase::PHASE_IMP_POPPING)
        return false;

    return mZombieType != ZombieType::ZOMBIE_BUNGEE || mZombiePhase == ZombiePhase::PHASE_BUNGEE_AT_BOTTOM;
}

bool Zombie::EffectedByDamage(unsigned int theDamageRangeFlags) {
    if (!TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_DYING) && IsDeadOrDying()) {
        return false;
    }

    if (TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_ONLY_MINDCONTROLLED)) {
        if (!mMindControlled) {
            return false;
        }
    } else if (mMindControlled) {
        return false;
    }

    if (mZombieType == ZombieType::ZOMBIE_BUNGEE && mZombiePhase != ZombiePhase::PHASE_BUNGEE_AT_BOTTOM && mZombiePhase != ZombiePhase::PHASE_BUNGEE_GRABBING) {
        return false; // 蹦极僵尸只有在停留时才会受到攻击
    }

    if (mZombieHeight == ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED) {
        return false; // 被空投的过程中不会受到攻击
    }

    if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_ENTER && aBodyReanim->mAnimTime < 0.5f) {
            return false;
        }
        if (mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_LEAVE && aBodyReanim->mAnimTime > 0.5f) {
            return false;
        }

        if (mZombiePhase != ZombiePhase::PHASE_BOSS_HEAD_IDLE_BEFORE_SPIT && mZombiePhase != ZombiePhase::PHASE_BOSS_HEAD_IDLE_AFTER_SPIT && mZombiePhase != ZombiePhase::PHASE_BOSS_HEAD_SPIT) {
            return false; // 僵王博士只有在低头状态下才会受到攻击
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_BOBSLED && GetBobsledPosition() > 0) {
        return false; // 存在雪橇时，只有领头僵尸会受到攻击
    }

    // 僵尸狗在非奔跑状态时只能被低矮植物或近战攻击命中
    if (mZombieType == ZombieType::ZOMBIE_DOG && mZombiePhase != ZombiePhase::PHASE_DOG_RUNNING) {
        return TestBit(theDamageRangeFlags, int(DamageRangeFlags::DAMAGES_DOG));
    }

    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL
        || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP || mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL || mZombiePhase == ZombiePhase::PHASE_BALLOON_POPPING
        || mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE || mZombiePhase == ZombiePhase::PHASE_BOBSLED_CRASHING || mZombiePhase == ZombiePhase::PHASE_DANCER_RISING) {
        return TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_OFF_GROUND);
    }

    if (mZombieType != ZombieType::ZOMBIE_BOBSLED && GetZombieRect().mX > WIDE_BOARD_WIDTH) {
        return false; // 除雪橇僵尸小队外，场外的僵尸不会受到攻击
    }

    bool submerged = mZombieType == ZombieType::ZOMBIE_SNORKEL && mInPool && !mIsEating;
    if (TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_SUBMERGED) && submerged) {
        return true;
    }

    bool underground = mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING;
    if (TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_UNDERGROUND) && underground) {
        return true;
    }

    if (TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_FLYING) && IsFlying()) {
        return true;
    }

    if (mApp->IsVSMode()) {
        if (IsFlying()) {
            return mAltitude < FLYER_ALTITUDE; // 对战气球低空飞行时会受到攻击
        }
    }

    return TestBit(theDamageRangeFlags, (int)DamageRangeFlags::DAMAGES_GROUND) && !IsFlying() && !submerged && !underground;
}

void Zombie::AddButter() {
    if (CanBeFrozen() && mZombieType != ZombieType::ZOMBIE_BOSS) {
        // Ban冰车 跳跳 僵王 飞翔的气球 跳跃的撑杆 即将跳水的潜水 等等
        if (mButteredCounter <= 100) {
            if (mButteredCounter == 0) {
                mApp->PlayFoley(FoleyType::FOLEY_BUTTER);
            }
            ApplyButter();
        }
    }
}

void Zombie::MowDown() {
    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (mApp->mGameScene == SCENE_PLAYING) {
        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_MOW_DOWN}, uint16_t(mBoard->mZombies.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
    }

    MowDown_Original();
}

void Zombie::MowDown_Original() {
    if (mDead || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED || mZombieType == ZombieType::ZOMBIE_BOSS)
        return;

    if (mZombieType == ZombieType::ZOMBIE_CATAPULT) {
        mApp->AddTodParticle(mPosX + 80.0f, mPosY + 60.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_CATAPULT_EXPLOSION);
        mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);
        DieWithLoot();
        return;
    }

    if (mZombieType == ZombieType::ZOMBIE_ZAMBONI) {
        mApp->AddTodParticle(mPosX + 80.0f, mPosY + 60.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_ZAMBONI_EXPLOSION);
        mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);
        DieWithLoot();
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE
        || mZombiePhase == ZombiePhase::PHASE_DANCER_RISING || mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED || IsGargantuar()
        || mZombieType == ZombieType::ZOMBIE_BUNGEE || mZombieType == ZombieType::ZOMBIE_DIGGER || mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD || mZombieType == ZombieType::ZOMBIE_IMP
        || mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP || mZombieType == ZombieType::ZOMBIE_GIGA_IMP || mZombieType == ZombieType::ZOMBIE_YETI || mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER
        || IsBobsledTeamWithSled() || IsFlying() || mInPool) {
        Reanimation *aPuffReanim = mApp->AddReanimation(mPosX - 73.0f, mPosY - 56.0f, mRenderOrder + 2, ReanimationType::REANIM_PUFF);
        aPuffReanim->SetFramesForLayer("anim_puff");
        mApp->AddTodParticle(mPosX + 110.0f, mPosY + 0.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_MOWER_CLOUD);

        if (mBoard->mPlantRow[mRow] != PlantRowType::PLANTROW_POOL) {
            DropHead(0U);
            DropArm(0U);
            DropHelm(0U);
            DropShield(0U);
        }

        DieWithLoot();
        return;
    }

    if (mIceTrapCounter > 0) {
        RemoveIceTrap();
    }
    if (mButteredCounter > 0) {
        mButteredCounter = 0;
    }

    DropShield(0U);
    DropHelm(0U);
    if (mZombieType == ZombieType::ZOMBIE_FLAG) {
        DropFlag();
    } else if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
        DropPole();
    } else if (mZombieType == ZombieType::ZOMBIE_NEWSPAPER || mZombieType == ZombieType::ZOMBIE_BALLOON || mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
        DropHead(0U);
    } else if (mZombieType == ZombieType::ZOMBIE_POGO) {
        DropHead(0U);
        mAltitude = 0.0f;
    } else if (mZombieType == ZOMBIE_DOGWALKER) {
        BreakRope();
    }

    Reanimation *aMoweredReanim = mApp->AddReanimation(0.0f, 0.0f, mRenderOrder, ReanimationType::REANIM_LAWN_MOWERED_ZOMBIE);
    aMoweredReanim->mAnimRate = 8.0f;
    aMoweredReanim->mIsAttachment = false;
    aMoweredReanim->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD;
    mMoweredReanimID = mApp->ReanimationGetID(aMoweredReanim);
    mZombiePhase = ZombiePhase::PHASE_ZOMBIE_MOWERED;
    DropLoot();
}

bool Zombie::IsWalkingBackwards() const {
    if (mMindControlled)
        return true;

    if (mZombieHeight == ZombieHeight::HEIGHT_ZOMBIQUARIUM) {
        if (mVelZ < 1.5707964f || mVelZ > 4.712389f) {
            return true;
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_DIGGER) {
        if (mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING || mZombiePhase == ZombiePhase::PHASE_DIGGER_STUNNED || mZombiePhase == ZombiePhase::PHASE_DIGGER_WALKING) {
            return true;
        } else if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED) {
            return mHasObject;
        }

        return false;
    }

    return mZombieType == ZombieType::ZOMBIE_YETI && !mHasObject;
}

void Zombie::SetZombatarReanim() {
    DefaultPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
    if (!aPlayerInfo->mZombatarEnabled)
        return;
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
    ReanimatorTrackInstance *aHeadTrackInstance = aBodyReanim->GetTrackInstanceByName("anim_head1");
    aHeadTrackInstance->mImageOverride = IMAGE_BLANK;
    Reanimation *aZombatarHeadReanim = mApp->AddReanimation(0, 0, 0, ReanimationType::REANIM_ZOMBATAR_HEAD);
    aZombatarHeadReanim->SetZombatarHats(aPlayerInfo->mZombatarHat, aPlayerInfo->mZombatarHatColor);
    aZombatarHeadReanim->SetZombatarHair(aPlayerInfo->mZombatarHair, aPlayerInfo->mZombatarHairColor);
    aZombatarHeadReanim->SetZombatarFHair(aPlayerInfo->mZombatarFacialHair, aPlayerInfo->mZombatarFacialHairColor);
    aZombatarHeadReanim->SetZombatarAccessories(aPlayerInfo->mZombatarAccessory, aPlayerInfo->mZombatarAccessoryColor);
    aZombatarHeadReanim->SetZombatarEyeWear(aPlayerInfo->mZombatarEyeWear, aPlayerInfo->mZombatarEyeWearColor);
    aZombatarHeadReanim->SetZombatarTidBits(aPlayerInfo->mZombatarTidBit, aPlayerInfo->mZombatarTidBitColor);
    aZombatarHeadReanim->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0);
    aZombatarHeadReanim->AssignRenderGroupToTrack("anim_hair", -1);
    mBossFireBallReanimID = mApp->ReanimationGetID(aZombatarHeadReanim);
    AttachEffect *attachEffect = AttachReanim(aHeadTrackInstance->mAttachmentID, aZombatarHeadReanim, 0.0f, 0.0f);
    TodScaleRotateTransformMatrix((SexyMatrix3 &)attachEffect->mOffset, -20.0, -1.0, 0.2, 1.0, 1.0);
    ReanimShowPrefix("anim_hair", -1);
    ReanimShowPrefix("anim_head2", -1);
}

bool Zombie::IsZombatarZombie(ZombieType theType) {
    // return type == ZombieType::ZOMBIE_FLAG || type == ZombieType::ZOMBIE_NORMAL || type == ZombieType::ZOMBIE_TRAFFIC_CONE || type == ZombieType::ZOMBIE_DOOR || type == ZombieType::ZOMBIE_TRASHCAN
    // || type == ZombieType::ZOMBIE_PAIL || type
    // == ZombieType::ZOMBIE_DUCKY_TUBE;
    return theType == ZombieType::ZOMBIE_FLAG;
}

int Zombie::GetSunBeanDamageCapacity(unsigned int theDamageFlags) {
    int aDamageCapacity = std::max(0, mFlyingHealth);

    if (mShieldType != ShieldType::SHIELDTYPE_NONE && !TestBit(theDamageFlags, int(DamageFlags::DAMAGE_BYPASSES_SHIELD)) && !TestBit(theDamageFlags, int(DamageFlags::DAMAGE_HITS_SHIELD_AND_BODY))) {
        aDamageCapacity += std::max(0, mShieldHealth);
    }

    if (mHelmType != HelmType::HELMTYPE_NONE) {
        aDamageCapacity += std::max(0, mHelmHealth);
    }

    int aBodyDamageCapacity = std::max(0, mBodyHealth);
    if (mHasHead && CanLoseBodyParts()) {
        aBodyDamageCapacity = std::max(0, mBodyHealth - mBodyMaxHealth / 3 + 1);
    }
    return aDamageCapacity + aBodyDamageCapacity;
}

void Zombie::SpawnSunBeanSun(int theSunValue) {
    if (mBoard == nullptr || theSunValue <= 0) {
        return;
    }

    const int aCoinX = mX + mWidth / 2;
    const int aCoinY = mY + mHeight / 2;
    while (theSunValue >= 50) {
        mBoard->AddCoin(aCoinX, aCoinY, CoinType::COIN_LARGESUN, CoinMotion::COIN_MOTION_FROM_PLANT);
        theSunValue -= 50;
    }
    while (theSunValue >= 25) {
        mBoard->AddCoin(aCoinX, aCoinY, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
        theSunValue -= 25;
    }
    while (theSunValue >= 15) {
        mBoard->AddCoin(aCoinX, aCoinY, CoinType::COIN_SMALLSUN, CoinMotion::COIN_MOTION_FROM_PLANT);
        theSunValue -= 15;
    }
    while (theSunValue >= 5) {
        mBoard->AddCoin(aCoinX, aCoinY, CoinType::COIN_MINISUN, CoinMotion::COIN_MOTION_FROM_PLANT);
        theSunValue -= 5;
    }
}

void Zombie::SettleSunBeanSun() {
    const int aDamageCapacity = GetSunBeanDamageCapacity(0U) + mSunBeanDamageRemainder;
    const int aSunValue = std::min(int(mSunBeanSun), aDamageCapacity / 20 * 5);
    mSunBeanSun = 0;
    mSunBeanDamageRemainder = 0;
    SpawnSunBeanSun(aSunValue);
}

void Zombie::DieWithLoot() {
    DieNoLoot();
    DropLoot();
}

void Zombie::DieNoLoot() {
    if (mDead) {
        LOG_WARN("mDead:{}", (int)mZombieType);
        return;
    }

    if (mApp->mGameScene == SCENE_PLAYING) {
        if (IsRemoteClientOrViewer()) {
            return;
        }
        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_DIE}, uint16_t(mBoard->mZombies.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_GARGANTUAR && mBoard != nullptr && mApp->mGameScene == GameScenes::SCENE_PLAYING) {
        mBoard->GrantAchievement(AchievementType::ACHIEVEMENT_GARG, true);
    }

    if (IsZombatarZombie(mZombieType)) {
        // 大头贴
        mApp->RemoveReanimation(mBossFireBallReanimID);
    }

    DieNoLoot_Origin();

    if (mApp->IsVSMode()) {
        if (mBoard && mBoard->GetAliveJacksonZombie() && CanRevived() && mCanRevived) {
            mCanRevived = false;
            if (msDeadFollowers.size() >= 15) {
                msDeadFollowers.erase(msDeadFollowers.begin());
            }
            msDeadFollowers.push_back(mZombieType);
        }

        if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
            JacksonDie();
        }
    }
}

void Zombie::DieNoLoot_Origin() {
    SettleSunBeanSun();

    if (mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD) {
        UnbindRealatedZombie();
    } else {
        Zombie *aRelatedZombie = mBoard->ZombieTryToGet(mRelatedZombieID);
        if (aRelatedZombie != nullptr && aRelatedZombie->mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD && aRelatedZombie->mRelatedZombieID == mBoard->ZombieGetID(this)) {
            aRelatedZombie->UnbindRealatedZombie();
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) {
        Zombie *aPartner = GetDogPartner();
        if (aPartner != nullptr && !aPartner->IsDeadOrDying()) {
            aPartner->HandleDogPartnerLost();
        }
        mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
    }

    old_Zombie_DieNoLoot(this);
}

void Zombie::StopZombieSound() {
    if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER) {
        bool aHasAliveDancer = false;

        if (mBoard) {
            Zombie *aZombie = nullptr;
            while (mBoard->IterateZombies(aZombie)) {
                if (aZombie->mHasHead && !aZombie->IsDeadOrDying() && aZombie->IsOnBoard()
                    && (aZombie->mZombieType == ZombieType::ZOMBIE_DANCER || aZombie->mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER)) {
                    aHasAliveDancer = true;
                    break;
                }
            }
        }

        if (!aHasAliveDancer) {
            mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_DANCER);
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
        bool aStopSound = false;

        if (mBoard) {
            Zombie *aZombie = nullptr;
            while (mBoard->IterateZombies(aZombie)) {
                if (aZombie->mHasHead && !aZombie->IsDeadOrDying() && aZombie->IsOnBoard() && aZombie->mZombieType == ZombieType::ZOMBIE_JACKSON) {
                    aStopSound = true;
                    break;
                }
            }
        }

        if (aStopSound) {
            mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_DANCER);
            mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_THRILLER);
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR) {
        mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_GIGA_LAUGH);
        mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_GIGA_LAUGH2);
        mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_GIGA_LAUGH3);
    }

    if (mPlayingSong) {
        mPlayingSong = false;

        if (mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX) {
            mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_JACKINTHEBOX);
        } else if (mZombieType == ZombieType::ZOMBIE_DIGGER) {
            mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_DIGGER);
        }
    }
}

void Zombie::DrawBungeeCord(Sexy::Graphics *g, int theOffsetX, int theOffsetY) {
    // 修复在Boss关的蹦极绳子不绑在Boss手上
    int aCordCelHeight = (Sexy::IMAGE_BUNGEECORD)->GetCelHeight() * mScaleZombie;
    float aPosX = 0.0f;
    float aPosY = 0.0f;
    GetTrackPosition("Zombie_bungi_body", aPosX, aPosY);
    bool aSetClip = false;
    if (IsOnBoard() && mApp->IsFinalBossLevel()) {
        Zombie *aBossZombie = mBoard->GetBossZombie();
        int aClipAmount = 55;
        if (aBossZombie->mZombiePhase == ZombiePhase::PHASE_BOSS_BUNGEES_LEAVE) {
            Reanimation *reanimation = mApp->ReanimationGet(aBossZombie->mBodyReanimID);
            aClipAmount = TodAnimateCurveFloatTime(0.0f, 0.2f, reanimation->mAnimTime, 55.0f, 0.0f, TodCurves::CURVE_LINEAR);
        }
        if (mTargetCol >= aBossZombie->mTargetCol) { // ">" ------ > ">="，修复第一根手指蹦极不绑在手上
            if (mTargetCol > aBossZombie->mTargetCol) {
                aClipAmount += 60; // 55 ---- > 115，修复第2、3根手指蹦极不绑在手上
            }
            g->SetClipRect(-g->mTransX, aClipAmount - g->mTransY, 800, 600);
            aSetClip = true;
        }
    }

    for (float y = aPosY - aCordCelHeight; y > -60 - aCordCelHeight; y -= aCordCelHeight) {
        float thePosX = theOffsetX + 61.0f - 4.0f / mScaleZombie;
        float thePosY = y - mPosY;
        TodDrawImageScaledF(g, Sexy::IMAGE_BUNGEECORD, thePosX, thePosY, mScaleZombie, mScaleZombie);
    }
    if (aSetClip) {
        g->ClearClipRect();
    }
}

void Zombie::GetDrawPos(ZombieDrawPosition &theDrawPos) {
    theDrawPos.mImageOffsetX = mPosX - mX;
    theDrawPos.mImageOffsetY = mPosY - mY;

    if (mIsEating) {
        theDrawPos.mHeadX = 47;
        theDrawPos.mHeadY = 4;
    } else {
        switch (mFrame) {
            case 0:
                theDrawPos.mHeadX = 50;
                theDrawPos.mHeadY = 2;
                break;
            case 1:
                theDrawPos.mHeadX = 49;
                theDrawPos.mHeadY = 1;
                break;
            case 2:
                theDrawPos.mHeadX = 49;
                theDrawPos.mHeadY = 2;
                break;
            case 3:
                theDrawPos.mHeadX = 48;
                theDrawPos.mHeadY = 4;
                break;
            case 4:
                theDrawPos.mHeadX = 48;
                theDrawPos.mHeadY = 5;
                break;
            case 5:
                theDrawPos.mHeadX = 48;
                theDrawPos.mHeadY = 4;
                break;
            case 6:
                theDrawPos.mHeadX = 48;
                theDrawPos.mHeadY = 2;
                break;
            case 7:
                theDrawPos.mHeadX = 49;
                theDrawPos.mHeadY = 1;
                break;
            case 8:
                theDrawPos.mHeadX = 49;
                theDrawPos.mHeadY = 2;
                break;
            case 9:
                theDrawPos.mHeadX = 50;
                theDrawPos.mHeadY = 4;
                break;
            case 10:
                theDrawPos.mHeadX = 50;
                theDrawPos.mHeadY = 5;
                break;
            default:
                theDrawPos.mHeadX = 50;
                theDrawPos.mHeadY = 4;
                break;
        }
    }

    theDrawPos.mArmY = theDrawPos.mHeadY / 2;

    switch (mZombieType) {
        case ZombieType::ZOMBIE_FOOTBALL:
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
            theDrawPos.mImageOffsetY -= 16.0f;
            break;
        case ZombieType::ZOMBIE_YETI:
            theDrawPos.mImageOffsetY -= 20.0f;
            break;
        case ZombieType::ZOMBIE_CATAPULT:
            theDrawPos.mImageOffsetX -= 25.0f;
            theDrawPos.mImageOffsetY -= 18.0f;
            break;
        case ZombieType::ZOMBIE_POGO:
            theDrawPos.mImageOffsetY += 16.0f;
            break;
        case ZombieType::ZOMBIE_BALLOON:
            theDrawPos.mImageOffsetY += 17.0f;
            break;
        case ZombieType::ZOMBIE_POLEVAULTER:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
            theDrawPos.mImageOffsetX -= 6.0f;
            theDrawPos.mImageOffsetY -= 11.0f;
            break;
        case ZombieType::ZOMBIE_ZAMBONI:
            theDrawPos.mImageOffsetX += 68.0f;
            theDrawPos.mImageOffsetY -= 23.0f;
            break;
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_REDEYE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
        case ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR:
            theDrawPos.mImageOffsetY -= 8.0f;
            break;
        case ZombieType::ZOMBIE_BOBSLED:
            theDrawPos.mImageOffsetY -= 12.0f;
            break;
        default:
            break;
    }

    if (mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE) {
        theDrawPos.mBodyY = -mAltitude;

        if (mInPool) {
            theDrawPos.mClipHeight = theDrawPos.mBodyY;
        } else {
            float aHeightLimit = std::min(mPhaseCounter, 40);
            theDrawPos.mClipHeight = theDrawPos.mBodyY + aHeightLimit;
        }

        if (IsOnHighGround()) {
            theDrawPos.mBodyY -= HIGH_GROUND_HEIGHT;
        }

        return;
    }

    if (mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER) {
        theDrawPos.mBodyY = -mAltitude;
        theDrawPos.mClipHeight = CLIP_HEIGHT_OFF;

        if (mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL) {
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);

            if (aBodyReanim->mAnimTime >= 0.56f && aBodyReanim->mAnimTime <= 0.65f) // 跳上海豚的起跳过程
            {
                theDrawPos.mClipHeight = 0.0f;
            } else if (aBodyReanim->mAnimTime >= 0.75f) // 跳上海豚的下落过程
            {
                theDrawPos.mClipHeight = -mAltitude - 10.0f;
            }
        } else if (mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING) {
            theDrawPos.mImageOffsetX += 70.0f; // 额外 70 像素的横坐标偏移用于弥补跳上海豚后的 mPosX -= 70.0f

            if (mZombieHeight == ZombieHeight::HEIGHT_DRAGGED_UNDER) {
                theDrawPos.mClipHeight = -mAltitude - 15.0f;
            } else {
                theDrawPos.mClipHeight = -mAltitude - 10.0f;
            }
        } else if (mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP) {
            theDrawPos.mImageOffsetX += 70.0f + mAltitude;

            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            if (aBodyReanim->mAnimTime <= 0.06f) // 起跳出水之前
            {
                theDrawPos.mClipHeight = -mAltitude - 10.0f;
            } else if (aBodyReanim->mAnimTime >= 0.5f && aBodyReanim->mAnimTime <= 0.76f) // 起跳过程中（脱离水面后至重新入水前）
            {
                theDrawPos.mClipHeight = -13.0f;
            }
        } else if (mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING) {
            theDrawPos.mImageOffsetY += 50.0f; // 额外 50 像素的横坐标偏移用于弥补跳跃过程中前进的距离

            if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING) {
                theDrawPos.mClipHeight = -mAltitude + 44.0f;
            } else if (mZombieHeight == ZombieHeight::HEIGHT_DRAGGED_UNDER) {
                theDrawPos.mClipHeight = -mAltitude + 36.0f;
            }
        } else if (mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING && mZombieHeight == ZombieHeight::HEIGHT_OUT_OF_POOL) {
            theDrawPos.mClipHeight = -mAltitude;
        } else if (mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_WITHOUT_DOLPHIN && mZombieHeight == ZombieHeight::HEIGHT_OUT_OF_POOL) {
            theDrawPos.mClipHeight = -mAltitude;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_SNORKEL) {
        theDrawPos.mBodyY = -mAltitude;
        theDrawPos.mClipHeight = CLIP_HEIGHT_OFF;

        if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL) {
            Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
            if (aBodyReanim->mAnimTime >= 0.8f) // 入水后
            {
                theDrawPos.mClipHeight = -10.0f;
            }
        } else if (mInPool) {
            theDrawPos.mClipHeight = -mAltitude - 5.0f;
            theDrawPos.mClipHeight += 20.0f - 20.0f * mScaleZombie;
        }
    } else if (mInPool) {
        theDrawPos.mBodyY = -mAltitude;
        theDrawPos.mClipHeight = -mAltitude - 7.0f;
        theDrawPos.mClipHeight += 10.0f - 10.0f * mScaleZombie;

        if (mIsEating) {
            theDrawPos.mClipHeight += 7.0f;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_DANCER_RISING) {
        theDrawPos.mBodyY = -mAltitude;
        theDrawPos.mClipHeight = -mAltitude;

        if (IsOnHighGround()) {
            theDrawPos.mBodyY -= HIGH_GROUND_HEIGHT;
        }
    } else if (mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE) {
        theDrawPos.mBodyY = -mAltitude;

        if (mPhaseCounter > 20) {
            theDrawPos.mClipHeight = -mAltitude;
        } else {
            theDrawPos.mClipHeight = CLIP_HEIGHT_OFF;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_BUNGEE) {
        theDrawPos.mBodyY = -mAltitude;
        theDrawPos.mImageOffsetX -= 18.0f;

        if (IsOnHighGround()) {
            theDrawPos.mBodyY -= HIGH_GROUND_HEIGHT;
        }

        theDrawPos.mClipHeight = CLIP_HEIGHT_OFF;
    } else {
        theDrawPos.mBodyY = -mAltitude;
        theDrawPos.mClipHeight = CLIP_HEIGHT_OFF;
    }
}

void Zombie::DrawIceTrap(Graphics *g, const ZombieDrawPosition &theDrawPos, bool theFront) {
    if (mInPool || mZombieType == ZombieType::ZOMBIE_BOSS)
        return;

    float aOffsetX = 46.0f;
    float aOffsetY = theDrawPos.mBodyY + 92.0f;
    float aScale = 1.0f;
    switch (mZombieType) {
        case ZombieType::ZOMBIE_POGO:
            aOffsetX -= 10.0f;
            aOffsetY += 20.0f;
            break;
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_REDEYE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
        case ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR:
            aOffsetX -= 20.0f;
            aOffsetY -= 7.0f;
            aScale = 1.6f;
            break;
        case ZombieType::ZOMBIE_BUNGEE:
            aOffsetX -= 45.0f;
            aOffsetY -= 23.0f;
            aScale = 1.2f;
            break;
        case ZombieType::ZOMBIE_DIGGER:
        case ZombieType::ZOMBIE_CROSSING_GUARD:
            aOffsetX -= 27.0f;
            break;
        case ZombieType::ZOMBIE_CATAPULT:
            aOffsetX += 32.0f;
            break;
        case ZombieType::ZOMBIE_BALLOON:
            aOffsetX -= 9.0f;
            aOffsetY += 27.0f;
            break;
        default:
            break;
    }

    TodDrawImageScaledF(g, theFront ? IMAGE_ICETRAP : IMAGE_ICETRAP2, aOffsetX, aOffsetY, aScale, aScale);
}

void Zombie::DrawButter(Graphics *g, const ZombieDrawPosition &theDrawPos) {
    float aOffsetX = mPosX + theDrawPos.mImageOffsetX + theDrawPos.mHeadX + 11.0f;
    float aOffsetY = mPosY + theDrawPos.mImageOffsetY + theDrawPos.mHeadY + theDrawPos.mBodyY + 21.0f;
    float aScale = 1.0f;
    if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MADDENING) {
        GetTrackPosition("anim_head_look", aOffsetX, aOffsetY);
    } else if (mZombieType == ZombieType::ZOMBIE_CATAPULT) {
        GetTrackPosition("Zombie_catapult_driver_head", aOffsetX, aOffsetY);
    } else if (mZombieType == ZombieType::ZOMBIE_DOG) {
        GetTrackPosition("Zombie_dog_outer_head1", aOffsetX, aOffsetY);
    } else if (mBodyReanimID != ReanimationID::REANIMATIONID_NULL) {
        GetTrackPosition("anim_head1", aOffsetX, aOffsetY);
    }
    aOffsetX -= mPosX + 29.0f;
    aOffsetY -= mPosY + 36.0f;

    switch (mZombieType) {
        case ZombieType::ZOMBIE_POGO:
            aOffsetY -= 5.0f;
            break;
        case ZombieType::ZOMBIE_GARGANTUAR:
        case ZombieType::ZOMBIE_REDEYE_GARGANTUAR:
        case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
        case ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR:
            aOffsetX -= 5.0f;
            aOffsetY -= 15.0f;
            aScale = 1.2f;
            break;
        case ZombieType::ZOMBIE_BUNGEE:
            aScale = 1.2f;
            break;
        case ZombieType::ZOMBIE_SQUASH_HEAD:
            aOffsetX += 6.0f;
            aOffsetY -= 9.0f;
            break;
        case ZombieType::ZOMBIE_WALLNUT_HEAD:
            aOffsetX -= 6.0f;
            aOffsetY -= 1.0f;
            break;
        case ZombieType::ZOMBIE_TALLNUT_HEAD:
            aOffsetX -= 24.0f;
            aOffsetY -= 39.0f;
            break;
        case ZombieType::ZOMBIE_EXPLORER:
            aOffsetY -= 15.0f;
            break;
        case ZombieType::ZOMBIE_DOG:
            aOffsetX -= 4.0f;
            aOffsetY += 5.0f;
            aScale = 0.6f;
            break;
        default:
            break;
    }

    TodDrawImageScaledF(g, IMAGE_REANIM_CORNPULT_BUTTER_SPLAT, aOffsetX, aOffsetY, aScale, aScale);
}

bool Zombie::IsOnHighGround() {
    return IsOnBoard() && mBoard->mGridSquareType[mBoard->PixelToGridXKeepOnBoard(mX + 75, mY)][mRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND;
}

bool Zombie::IsTangleKelpTarget() {
    // 修复水草拉僵尸有概率失效

    if (!mBoard->StageHasPool()) {
        return false;
    }
    if (mZombieHeight == ZombieHeight::HEIGHT_DRAGGED_UNDER) {
        return true;
    }
    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (!aPlant->mDead && aPlant->mSeedType == SeedType::SEED_TANGLEKELP && aPlant->mTargetZombieID == mBoard->mZombies.DataArrayGetID(this)) {
            return true;
        }
    }
    return false;
}

void Zombie::DrawReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, int theBaseRenderGroup) {
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
    if (aBodyReanim == nullptr) {
        return;
    }

    if (theDrawPos.mClipHeight > CLIP_HEIGHT_LIMIT) {
        float aDrawHeight = 120.0f - theDrawPos.mClipHeight + 71.0f;
        g->SetClipRect(theDrawPos.mImageOffsetX - 200.0f, theDrawPos.mImageOffsetY + theDrawPos.mBodyY - 78.0f, 520, aDrawHeight);
    }

    int aFadeAlpha = 255;
    if (mZombieFade >= 0) {
        aFadeAlpha = ClampInt(255 * mZombieFade / 10, 0, 255);
    }

    Color aColorOverride(255, 255, 255, aFadeAlpha);
    Color aExtraAdditiveColor = Color::Black;
    bool aEnableExtraAdditiveDraw = false;
    bool aColorChanged = false;
    if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED) {
        aColorOverride = Color(0, 0, 0, aFadeAlpha);
        aExtraAdditiveColor = Color::Black;
        aEnableExtraAdditiveDraw = false;
    } else if (mZombieType == ZombieType::ZOMBIE_BOSS && mZombiePhase != ZombiePhase::PHASE_ZOMBIE_DYING && mBodyHealth < mBodyMaxHealth / BOSS_FLASH_HEALTH_FRACTION) {
        int aGrayness = TodAnimateCurve(0, 39, mBoard->mMainCounter % 40, 155, 255, TodCurves::CURVE_BOUNCE);
        if (mChilledCounter > 0 || mIceTrapCounter > 0) {
            int aColdColor = TodAnimateCurve(0, 39, mBoard->mMainCounter % 40, 65, 75, TodCurves::CURVE_BOUNCE);
            aColorOverride = Color(aColdColor, aColdColor, aGrayness, aFadeAlpha);
        } else {
            aColorOverride = Color(aGrayness, aGrayness, aGrayness, aFadeAlpha);
        }

        aExtraAdditiveColor = Color::Black;
        aEnableExtraAdditiveDraw = false;
    } else if (mMindControlled) {
        aColorOverride = ZOMBIE_MINDCONTROLLED_COLOR;
        aColorOverride.mAlpha = aFadeAlpha;
        aExtraAdditiveColor = aColorOverride;
        aEnableExtraAdditiveDraw = true;
    } else if (mChilledCounter > 0 || mIceTrapCounter > 0) {
        aColorOverride = Color(75, 75, 255, aFadeAlpha);
        aExtraAdditiveColor = aColorOverride;
        aEnableExtraAdditiveDraw = true;
    } else if (mSunBeanSun > 0) {
        aColorOverride = Color(255, 255, 75, aFadeAlpha);
        aExtraAdditiveColor = aColorOverride;
        aEnableExtraAdditiveDraw = true;
    } else if (mZombieHeight == ZombieHeight::HEIGHT_ZOMBIQUARIUM && mBodyHealth < 100) {
        aColorOverride = Color(100, 150, 25, aFadeAlpha);
        aExtraAdditiveColor = aColorOverride;
        aEnableExtraAdditiveDraw = true;
    } else if (mPoisoned) {
        int aProgress = ClampInt(ZOMBIE_POISONING_TIME - mPoisonedCounter, 0, ZOMBIE_POISONING_TIME);

        int aRedBlue = TodAnimateCurve(0, ZOMBIE_POISONING_TIME, aProgress, 255, 70, TodCurves::CURVE_LINEAR);
        int aGreen = TodAnimateCurve(0, ZOMBIE_POISONING_TIME, aProgress, 255, 230, TodCurves::CURVE_LINEAR);
        int aPoisonGlow = TodAnimateCurve(0, ZOMBIE_POISONING_TIME, aProgress, 0, 120, TodCurves::CURVE_LINEAR);

        aColorOverride = Color(aRedBlue, aGreen, aRedBlue, aFadeAlpha);
        aExtraAdditiveColor = Color(0, aPoisonGlow, 0, 255);
        aEnableExtraAdditiveDraw = true;
    } else if (mIsRevived) {
        aColorOverride = ZOMBIE_REVIVED_COLOR;
        aColorOverride.mAlpha = aFadeAlpha;
        aExtraAdditiveColor = aColorOverride;
        aEnableExtraAdditiveDraw = true;
    }

    if (mJustGotShotCounter > 0 && !IsBobsledTeamWithSled()) {
        int aGrayness = mJustGotShotCounter * 10;
        Color aHighlightColor(aGrayness, aGrayness, aGrayness, 255);
        aExtraAdditiveColor = ColorAdd(aHighlightColor, aExtraAdditiveColor);
        aEnableExtraAdditiveDraw = true;
    }

    if (aColorOverride != aBodyReanim->mColorOverride || aExtraAdditiveColor != aBodyReanim->mExtraAdditiveColor || aEnableExtraAdditiveDraw != aBodyReanim->mEnableExtraAdditiveDraw) {
        aColorChanged = true;
        aBodyReanim->mColorOverride = aColorOverride;
        aBodyReanim->mExtraAdditiveColor = aExtraAdditiveColor;
        aBodyReanim->mEnableExtraAdditiveDraw = aEnableExtraAdditiveDraw;
    }

    if (mZombieType == ZombieType::ZOMBIE_BOBSLED) {
        DrawBobsledReanim(g, theDrawPos, true);
        aBodyReanim->DrawRenderGroup(g, theBaseRenderGroup);
        DrawBobsledReanim(g, theDrawPos, false);
    } else if (mZombieType == ZombieType::ZOMBIE_BUNGEE) {
        DrawBungeeReanim(g, theDrawPos);
    } else if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_JACKSON) {
        DrawDancerReanim(g, theDrawPos);
    } else {
        aBodyReanim->DrawRenderGroup(g, theBaseRenderGroup);
    }

    if (mShieldType != ShieldType::SHIELDTYPE_NONE) {
        if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED) {
            aColorChanged = true;
            aBodyReanim->mColorOverride = Color(0, 0, 0, aFadeAlpha);
            aBodyReanim->mExtraAdditiveColor = Color::Black;
            aBodyReanim->mEnableExtraAdditiveDraw = false;
        } else {
            if (mShieldJustGotShotCounter > 0) {
                int aGrayness = mShieldJustGotShotCounter * 10;
                aBodyReanim->mColorOverride = Color(255, 255, 255, aFadeAlpha);
                aBodyReanim->mExtraAdditiveColor = Color(aGrayness, aGrayness, aGrayness, 255);
                aBodyReanim->mEnableExtraAdditiveDraw = true;
                aColorChanged = true;
            } else {
                if (!aColorChanged) {
                    aColorChanged = aBodyReanim->mEnableExtraAdditiveDraw || aBodyReanim->mColorOverride.mRed != 255;
                }
                aBodyReanim->mColorOverride = Color(255, 255, 255, aFadeAlpha);
                aBodyReanim->mExtraAdditiveColor = Color::Black;
                aBodyReanim->mEnableExtraAdditiveDraw = false;
            }
        }

        float aShieldHitOffset = 0.0f;
        if (mShieldRecoilCounter > 0) {
            aShieldHitOffset = TodAnimateCurveFloat(12, 0, mShieldRecoilCounter, 3.0f, 0.0f, TodCurves::CURVE_LINEAR);
        }

        g->mTransX += aShieldHitOffset;
        aBodyReanim->DrawRenderGroup(g, RENDER_GROUP_SHIELD);
        g->mTransX -= aShieldHitOffset;

        if (mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER || mShieldType == ShieldType::SHIELDTYPE_DOOR || mShieldType == ShieldType::SHIELDTYPE_LADDER
            || mShieldType == ShieldType::SHIELDTYPE_TRASHCAN || mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION) {
            aBodyReanim->mColorOverride = aColorOverride;
            aBodyReanim->mExtraAdditiveColor = aExtraAdditiveColor;
            aBodyReanim->mEnableExtraAdditiveDraw = aEnableExtraAdditiveDraw;
            aBodyReanim->DrawRenderGroup(g, RENDER_GROUP_OVER_SHIELD);
        }
    }

    if (aColorChanged) {
        aBodyReanim->PropogateColorToAttachments();
    }

    g->ClearClipRect();

    // 大头贴专门Draw一下
    if (IsZombatarZombie(mZombieType)) {
        Reanimation *aZombatarReanim = mApp->ReanimationTryToGet(mBossFireBallReanimID);
        if (aZombatarReanim) {
            aZombatarReanim->Draw(g);
        }
    }
}

bool Zombie::CanLoseBodyParts() {
    return mZombieType != ZombieType::ZOMBIE_ZAMBONI && mZombieType != ZombieType::ZOMBIE_BUNGEE && mZombieType != ZombieType::ZOMBIE_CATAPULT && !IsGargantuar()
        && mZombieType != ZombieType::ZOMBIE_BOSS && mZombieHeight != ZombieHeight::HEIGHT_ZOMBIQUARIUM && !IsFlying() && !IsBobsledTeamWithSled() && !IsZomblob(mZombieType)
        && mZombieType != ZombieType::ZOMBIE_DOG;
}

void Zombie::SetupReanimForLostHead() {
    ReanimShowPrefix("anim_head", RENDER_GROUP_HIDDEN);
    ReanimShowPrefix("anim_hair", RENDER_GROUP_HIDDEN);
    ReanimShowPrefix("anim_tongue", RENDER_GROUP_HIDDEN);

    if (IsZomblob(mZombieType)) {
        // Zomblob 的轨道名来自其独立动画资源，不使用普通僵尸的 anim_head 前缀。
        ReanimShowPrefix("zombie_zomblob_head", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("zombie_zomblob_eye", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("zombie_zomblob_jaw", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("zombie_hair", RENDER_GROUP_HIDDEN);
    }
}

void Zombie::DropHead(unsigned int theDamageFlags) {
    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (IsRemoteServer()) {
        U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_DROP_HEAD;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2 = uint16_t(theDamageFlags);
        netplay::PutEvent(event);
    }

    DropHead_Origin(theDamageFlags);
}

void Zombie::DropHead_Origin(unsigned int theDamageFlags) {
    if (mZombieType >= ZombieType::NUM_CACHED_ZOMBIE_TYPES) {
        const bool aCanDropButteredZomblobHead = IsZomblob(mZombieType) && mButtered;
        if ((!CanLoseBodyParts() && !aCanDropButteredZomblobHead) || !mHasHead)
            return;

        Zombie *aRelatedZombie = mBoard->ZombieTryToGet(mRelatedZombieID);
        const bool aCrossingGuardBinding = mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD
            || (aRelatedZombie != nullptr && aRelatedZombie->mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD && aRelatedZombie->mRelatedZombieID == mBoard->ZombieGetID(this));
        if (aCrossingGuardBinding) {
            UnbindRealatedZombie();
        }

        if (mButteredCounter > 0) {
            mButteredCounter = 0;
            UpdateAnimSpeed();
        }

        if (mSunBeanSun > 0) {
            mSunBeanSun = 0;
            mSunBeanDamageRemainder = 0;
        }

        mHasHead = false;
        SetupReanimForLostHead();
        if (TestBit(theDamageFlags, DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
            return;
        }

        if (Zombie::IsZombotany(mZombieType) && mSpecialHeadReanimID != ReanimationID::REANIMATIONID_NULL) {
            Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
            if (aHeadReanim) {
                aHeadReanim->ReanimationDie();
            }
            mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
            return;
        }

        int aRenderOrder = mRenderOrder + 1;
        ZombieDrawPosition aDrawPos{};
        GetDrawPos(aDrawPos);
        float aPosX = mPosX + aDrawPos.mImageOffsetX + aDrawPos.mHeadX + 11.0f;
        float aPosY = mPosY + aDrawPos.mImageOffsetY + aDrawPos.mHeadY + aDrawPos.mBodyY + 21.0f;
        if (mBodyReanimID != ReanimationID::REANIMATIONID_NULL) {
            Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
            if (aBodyReanim) {
                if (IsZomblob(mZombieType) && aBodyReanim->TrackExists("zombie_zomblob_head")) {
                    GetTrackPosition("zombie_zomblob_head", aPosX, aPosY);
                } else if (aBodyReanim->TrackExists("anim_head1")) {
                    GetTrackPosition("anim_head1", aPosX, aPosY);
                }
            }
        }

        ParticleEffect aEffect = ParticleEffect::PARTICLE_ZOMBIE_HEAD;
        if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED) {
            aEffect = ParticleEffect::PARTICLE_MOWERED_ZOMBIE_HEAD;
        } else if (mInPool) {
            aEffect = ParticleEffect::PARTICLE_ZOMBIE_HEAD_POOL;
        }
        if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
            aRenderOrder = mRenderOrder - 1;
        }
        if (mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
            aEffect = ParticleEffect::PARTICLE_ZOMBIE_NEWSPAPER_HEAD;
        }

        TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, aRenderOrder, aEffect);
        OverrideParticleColor(aParticle);
        OverrideParticleScale(aParticle);
        if (aParticle) {
            if (IsZomblob(mZombieType) && mButtered) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBLOBHEAD_BUTTERED);
            } else if (mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
                aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEFOOTBALLHEAD);
            } else if (mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_SUPERFAN_ZOMBIEIMPHEAD);
                ReanimShowPrefix("Zombie_Ghost_Fans5", RENDER_GROUP_HIDDEN);
                ReanimShowPrefix("Zombie_Ghost_Fans6", RENDER_GROUP_HIDDEN);
            } else if (mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
                aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEIMPHEAD);
                ReanimShowTrack("anim_glasses", RENDER_GROUP_HIDDEN);
            } else if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_ZOMBIEJACKSONHEAD);
            } else if (mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
                ReanimShowPrefix("anim_earing", RENDER_GROUP_HIDDEN);

                aParticle->OverrideImage(nullptr, addonImages.IMAGE_ZOMBIEBACKUPDANCERHEAD);
            } else if (mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
                ReanimShowPrefix("anim_glasses", RENDER_GROUP_HIDDEN);
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_GIGA_ZOMBIEPOLEVAULTERHEAD);
                DropPole();
            } else if (mZombieType == ZombieType::ZOMBIE_EXPLORER) {
                ReanimShowPrefix("zombie_explorer_hat", RENDER_GROUP_HIDDEN);
                ReanimShowPrefix("zombie_explorer_beard", RENDER_GROUP_HIDDEN);
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_EXPLORER_HEAD);
                ExplorerTorchConvert(false);
            } else if (mZombieType == ZombieType::ZOMBIE_DOGWALKER) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_DOGWALKER_HEAD);
                BreakRope();
            } else if (mZombieType == ZombieType::ZOMBIE_TELEPORTATION) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_ZOMBIE_TELEPORTATION_HEAD);
            } else if (mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD) {
                ReanimShowPrefix("Zombie_crossing_guard_hair", RENDER_GROUP_HIDDEN);
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_ZOMBIE_CROSSING_GUARD_HEAD);
            } else if (mZombieType == ZombieType::ZOMBIE_SCIENTIST) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_ZOMBIE_SCIENTIST_HEAD);
            }
        }
        return;
    }

    if (!CanLoseBodyParts() || !mHasHead)
        return;

    if (mButteredCounter > 0) {
        mButteredCounter = 0;
        UpdateAnimSpeed();
    }

    mHasHead = false;
    mSunBeanSun = 0;
    mSunBeanDamageRemainder = 0;
    SetupReanimForLostHead();
    if (TestBit(theDamageFlags, DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
        return;
    }

    if (Zombie::IsZombotany(mZombieType)) {
        Reanimation *aReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aReanim) {
            aReanim->ReanimationDie();
        }
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
        return;
    }

    int aRenderOrder = mRenderOrder + 1;
    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);
    float aPosX = mPosX + aDrawPos.mImageOffsetX + aDrawPos.mHeadX + 11.0f;
    float aPosY = mPosY + aDrawPos.mImageOffsetY + aDrawPos.mHeadY + aDrawPos.mBodyY + 21.0f;
    if (mBodyReanimID != ReanimationID::REANIMATIONID_NULL) {
        GetTrackPosition("anim_head1", aPosX, aPosY);
    }

    ParticleEffect aEffect = ParticleEffect::PARTICLE_ZOMBIE_HEAD;
    if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED) {
        aEffect = ParticleEffect::PARTICLE_MOWERED_ZOMBIE_HEAD;
    } else if (mInPool) {
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_HEAD_POOL;
    }
    if (mZombieType == ZombieType::ZOMBIE_DANCER) {
        aRenderOrder = mRenderOrder - 1;
    }
    if (mZombieType == ZombieType::ZOMBIE_NEWSPAPER) {
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_NEWSPAPER_HEAD;
    } else if (mZombieType == ZombieType::ZOMBIE_POGO) {
        PogoBreak(theDamageFlags);
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_POGO_HEAD;
    } else if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
        ReanimShowPrefix("anim_hat", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("hat", RENDER_GROUP_HIDDEN);
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_BALLOON_HEAD;
    } else if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER) {
        DropPole();
    } else if (mZombieType == ZombieType::ZOMBIE_FLAG) {
        DropFlag();
    }

    TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, aRenderOrder, aEffect);
    OverrideParticleColor(aParticle);
    OverrideParticleScale(aParticle);
    if (aParticle) {
        if (mZombieType == ZombieType::ZOMBIE_DANCER) {
            ReanimShowPrefix("Zombie_disco_chops", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_disco_glasses", RENDER_GROUP_HIDDEN);
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEDANCERHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER) {
            ReanimShowPrefix("Zombie_disco_chops", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_backup_stash", RENDER_GROUP_HIDDEN);
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEBACKUPDANCERHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_BOBSLED) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEBOBSLEDHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_LADDER) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIELADDERHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_IMP) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEIMPHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_FOOTBALL) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEFOOTBALLHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEPOLEVAULTERHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_SNORKEL) {
            aParticle->OverrideImage(nullptr, IMAGE_REANIM_ZOMBIE_SNORKLE_HEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_DIGGER) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEDIGGERHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEDOLPHINRIDERHEAD);
        } else if (mZombieType == ZombieType::ZOMBIE_YETI) {
            aParticle->OverrideImage(nullptr, IMAGE_ZOMBIEYETIHEAD);
        }
    }

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim && mBoard->mMustacheMode && aBodyReanim->TrackExists("Zombie_mustache")) {
        ReanimShowPrefix("Zombie_mustache", RENDER_GROUP_HIDDEN);

        TodParticleSystem *aMustacheParticle = mApp->AddTodParticle(aPosX, aPosY, aRenderOrder, ParticleEffect::PARTICLE_ZOMBIE_MUSTACHE);
        OverrideParticleColor(aMustacheParticle);
        OverrideParticleScale(aMustacheParticle);

        Sexy::Image *aMustacheImage = aBodyReanim->GetImageOverride("Zombie_mustache");
        if (aMustacheParticle && aMustacheImage) {
            aMustacheParticle->OverrideImage(nullptr, aMustacheImage);
        }
    }

    if (aBodyReanim && mBoard->mFutureMode) {
        Sexy::Image *aHeadImage = aBodyReanim->GetImageOverride("anim_head1");
        int aFrame = -1;
        if (aHeadImage == IMAGE_REANIM_ZOMBIE_HEAD_SUNGLASSES1) {
            aFrame = 0;
        } else if (aHeadImage == IMAGE_REANIM_ZOMBIE_HEAD_SUNGLASSES2) {
            aFrame = 1;
        } else if (aHeadImage == IMAGE_REANIM_ZOMBIE_HEAD_SUNGLASSES3) {
            aFrame = 2;
        } else if (aHeadImage == IMAGE_REANIM_ZOMBIE_HEAD_SUNGLASSES4) {
            aFrame = 3;
        }

        if (aFrame != -1) {
            TodParticleSystem *aSunglassParticle = mApp->AddTodParticle(aPosX, aPosY, aRenderOrder, ParticleEffect::PARTICLE_ZOMBIE_SUNGLASS);
            OverrideParticleColor(aSunglassParticle);
            OverrideParticleScale(aSunglassParticle);
            if (aSunglassParticle) {
                aSunglassParticle->OverrideFrame(nullptr, aFrame);
            }
        }
    }

    if (mBoard->mPinataMode && mZombiePhase != ZombiePhase::PHASE_ZOMBIE_MOWERED) {
        TodParticleSystem *aPinataParticle = mApp->AddTodParticle(aPosX, aPosY, aRenderOrder, ParticleEffect::PARTICLE_ZOMBIE_PINATA);
        OverrideParticleScale(aPinataParticle);
    }

    mApp->PlayFoley(FoleyType::FOLEY_LIMBS_POP);

    // 大头贴僵尸掉头时掉饰品(掉帽子和眼镜)
    if (IsZombatarZombie(mZombieType)) {
        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mBossFireBallReanimID);
        if (aHeadReanim != nullptr) {
            int index[2] = {aHeadReanim->GetZombatarHatTrackIndex(), aHeadReanim->GetZombatarEyeWearTrackIndex()};
            for (int i : index) {
                if (i == -1)
                    continue;
                ReanimatorTrackInstance *aTrackInstance = aHeadReanim->mTrackInstances + i;
                ReanimatorTrack *aTrack = aHeadReanim->mDefinition->mTracks + i;
                SexyTransform2D aTransform2D;
                aHeadReanim->GetTrackMatrix(i, aTransform2D);
                float aParticleX = mPosX + aTransform2D.m[0][2];
                float aParticleY = mPosY + aTransform2D.m[1][2];
                TodParticleSystem *aHeadParticle = mApp->AddTodParticle(aParticleX, aParticleY, mRenderOrder + 1, ParticleEffect::PARTICLE_ZOMBIE_HEAD);
                aHeadParticle->OverrideColor(nullptr, aTrackInstance->mTrackColor);
                aHeadParticle->OverrideImage(nullptr, aTrack->mTransforms[0].mImage);
            }
            mApp->RemoveReanimation(mBossFireBallReanimID);
            mBossFireBallReanimID = ReanimationID::REANIMATIONID_NULL;
        }
    }
}

void Zombie::BreakRope() {
    if (mZombieType == ZombieType::ZOMBIE_DOGWALKER) {
        ReanimShowTrack("Zombie_dogwalker_rope2", RENDER_GROUP_HIDDEN);
    }
}

void Zombie::DropPole() {
    if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
        ReanimShowPrefix("Zombie_polevaulter_innerarm", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("Zombie_polevaulter_innerhand", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("Zombie_polevaulter_pole", RENDER_GROUP_HIDDEN);
    }
}

void Zombie::DropFlag() {
    reinterpret_cast<void (*)(Zombie *)>(Zombie_DropFlagAddr)(this);
}

void Zombie::DropHelm(unsigned int theDamageFlags) {
    if (mHelmType == HelmType::HELMTYPE_NONE)
        return;

    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);
    float aPosX = mPosX + aDrawPos.mImageOffsetX + aDrawPos.mHeadX + 14.0f;
    float aPosY = mPosY + aDrawPos.mImageOffsetY + aDrawPos.mHeadY + aDrawPos.mBodyY + 18.0f;
    ParticleEffect aEffect = ParticleEffect::PARTICLE_NONE;
    if (mHelmType == HelmType::HELMTYPE_TRAFFIC_CONE) {
        GetTrackPosition("anim_cone", aPosX, aPosY);
        ReanimShowPrefix("anim_cone", RENDER_GROUP_HIDDEN);
        if (!IsZombotany(mZombieType)) {
            ReanimShowPrefix("anim_hair", RENDER_GROUP_NORMAL);
        }
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_TRAFFIC_CONE;
    } else if (mHelmType == HelmType::HELMTYPE_PAIL) {
        GetTrackPosition("anim_bucket", aPosX, aPosY);
        ReanimShowPrefix("anim_bucket", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("anim_hair", RENDER_GROUP_NORMAL);
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_PAIL;
    } else if (mHelmType == HelmType::HELMTYPE_FOOTBALL) {
        GetTrackPosition("zombie_football_helmet", aPosX, aPosY);
        ReanimShowPrefix("zombie_football_helmet", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("anim_hair", RENDER_GROUP_NORMAL);
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_HELMET;
    } else if (mHelmType == HelmType::HELMTYPE_DIGGER) {
        GetTrackPosition("Zombie_digger_hardhat", aPosX, aPosY);
        ReanimShowTrack("Zombie_digger_hardhat", RENDER_GROUP_HIDDEN);
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_HEADLIGHT;
    } else if (mHelmType == HelmType::HELMTYPE_BOBSLED && !TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
        BobsledCrash();
    } else if (mHelmType == HelmType::HELMTYPE_GIGA_FOOTBALL) {
        GetTrackPosition("zombie_football_helmet", aPosX, aPosY);
        ReanimShowPrefix("zombie_football_helmet", RENDER_GROUP_HIDDEN);
        ReanimShowPrefix("anim_hair", RENDER_GROUP_NORMAL);
        //        aEffect = ParticleEffect::PARTICLE_ZOMBIE_GIGA_HELMET;
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_PAIL;
    } else if (mHelmType == HelmType::HELMTYPE_CROSSING_GUARD) {
        GetTrackPosition("Zombie_crossing_guard_hardhat", aPosX, aPosY);
        ReanimShowPrefix("Zombie_crossing_guard_hardhat", RENDER_GROUP_HIDDEN);
        //        aEffect = ParticleEffect::PARTICLE_ZOMBIE_HELMET;
        aEffect = ParticleEffect::PARTICLE_ZOMBIE_HEADLIGHT;
    }

    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY) && aEffect != ParticleEffect::PARTICLE_NONE) {
        TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, mRenderOrder + 1, aEffect);
        OverrideParticleScale(aParticle);
        if (aParticle != nullptr) {
            if (mHelmType == HelmType::HELMTYPE_GIGA_FOOTBALL) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET3);
            } else if (mHelmType == HelmType::HELMTYPE_CROSSING_GUARD) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_CROSSING_GUARD_HAT);
            }
        }
    }

    mHelmType = HelmType::HELMTYPE_NONE;
}

void Zombie::DropShield(unsigned int theDamageFlags) {
    if (mShieldType == ShieldType::SHIELDTYPE_NONE)
        return;

    if (mShieldType == ShieldType::SHIELDTYPE_DOOR) {
        DetachShield();
        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
            float aPosX, aPosY;
            GetTrackPosition("anim_screendoor", aPosX, aPosY);
            TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, mRenderOrder + 1, ParticleEffect::PARTICLE_ZOMBIE_DOOR);
            OverrideParticleScale(aParticle);
        }
    } else if (mShieldType == ShieldType::SHIELDTYPE_TRASHCAN) {
        DetachShield();
        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
            float aPosX, aPosY;
            GetTrackPosition("anim_screendoor", aPosX, aPosY);
            TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, mRenderOrder + 1, ParticleEffect::PARTICLE_ZOMBIE_TRASH_CAN);
            OverrideParticleScale(aParticle);
        }
    } else if (mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER || mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION) {
        StopEating();
        if (mYuckyFace) {
            ShowYuckyFace(false);
            mYuckyFace = false;
            mYuckyFaceCounter = 0;
        }

        mZombiePhase = ZombiePhase::PHASE_NEWSPAPER_MADDENING;
        PlayZombieReanim("anim_gasp", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 8.0f);
        DetachShield();

        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
            float aPosX, aPosY;
            GetTrackPosition("Zombie_paper_paper", aPosX, aPosY);
            TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, mRenderOrder + 1, ParticleEffect::PARTICLE_ZOMBIE_NEWSPAPER);
            OverrideParticleScale(aParticle);
            if (aParticle && mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
                aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER3);
            }
        }

        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY) && !TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_BYPASSES_SHIELD)) {
            mApp->PlayFoley(FoleyType::FOLEY_NEWSPAPER_RIP);
            AddAttachedReanim(-11, 0, ReanimationType::REANIM_ZOMBIE_SURPRISE);
        }
    } else if (mShieldType == ShieldType::SHIELDTYPE_LADDER) {
        DetachShield();
        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
            float aPosX = mPosX + 31.0f;
            float aPosY = mPosY + 80.0f;
            TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, mRenderOrder + 1, ParticleEffect::PARTICLE_ZOMBIE_LADDER);
            OverrideParticleScale(aParticle);
        }
    }

    mShieldType = ShieldType::SHIELDTYPE_NONE;
}

void Zombie::SetupReanimForLostArm(unsigned int theDamageFlags) {
    switch (mZombieType) {
        case ZombieType::ZOMBIE_GIGA_FOOTBALL:
            ReanimShowPrefix("Zombie_football_leftarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_football_leftarm_hand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_SUPER_FAN_IMP:
            ReanimShowTrack("Zombie_Ghost_Fans2", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_outerarm_lower", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_GIGA_IMP:
            ReanimShowTrack("Zombie_giga_outerarm_lower", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_JACKSON:
            ReanimShowTrack("Zombie_disco_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_disco_outerhand_point", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_disco_outerhand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_BACKUP_JACKSON:
            ReanimShowTrack("Zombie_disco_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_disco_outerhand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
            ReanimShowTrack("Zombie_polevaulter_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_SUNDAY_EDITION:
            ReanimShowTrack("Zombie_paper_hands", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_paper_leftarm_lower", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_EXPLORER:
            ReanimShowTrack("Zombie_explorer_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_explorer_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_DOGWALKER:
            ReanimShowTrack("Zombie_dogwalker_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_dogwalker_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_TELEPORTATION:
            ReanimShowPrefix("zombie_teleportation_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("zombie_teleportation_outerarm_hand", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("zombie_teleportation_telephone", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_CROSSING_GUARD:
            ReanimShowPrefix("Zombie_crossing_guard_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_crossing_guard_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_SCIENTIST:
            ReanimShowPrefix("Zombie_scientist_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_scientist_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        default:
            ReanimShowPrefix("Zombie_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
    }

    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);
    float aPosX = mPosX + aDrawPos.mImageOffsetX + 45.0f;
    float aPosY = mPosY + aDrawPos.mImageOffsetY + aDrawPos.mBodyY + 78.0f;
    if (IsWalkingBackwards()) {
        aPosX += 36.0f;
    }

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim) {
        switch (mZombieType) {
            case ZombieType::ZOMBIE_GIGA_FOOTBALL:
                GetTrackPosition("Zombie_football_leftarm_hand", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_football_leftarm_upper", IMAGE_REANIM_ZOMBIE_FOOTBALL_LEFTARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_SUPER_FAN_IMP:
                GetTrackPosition("Zombie_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_imp_outerarm_upper", IMAGE_REANIM_ZOMBIE_IMP_ARM1_BONE);
                break;
            case ZombieType::ZOMBIE_GIGA_IMP:
                GetTrackPosition("Zombie_giga_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_giga_imp_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_IMP_ARM1_BONE);
                break;
            case ZombieType::ZOMBIE_JACKSON:
                GetTrackPosition("Zombie_disco_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_disco_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_BACKUP_JACKSON:
                GetTrackPosition("Zombie_disco_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_disco_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_BACKUP_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
                GetTrackPosition("Zombie_polevaulter_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_polevaulter_outerarm_upper", IMAGE_REANIM_ZOMBIE_POLEVAULTER_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_SUNDAY_EDITION:
                GetTrackPosition("Zombie_paper_leftarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_paper_leftarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_LEFTARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_EXPLORER:
                GetTrackPosition("Zombie_explorer_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_explorer_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_EXPLORER_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_DOGWALKER:
                GetTrackPosition("Zombie_dogwalker_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_dogwalker_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_DOGWALKER_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_TELEPORTATION:
                GetTrackPosition("Zombie_teleportation_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_teleportation_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_TELEPORTATION_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_CROSSING_GUARD:
                GetTrackPosition("Zombie_crossing_guard_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_crossing_guard_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_CROSSING_GUARD_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_SCIENTIST:
                GetTrackPosition("Zombie_scientist_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_scientist_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_SCIENTIST_OUTERARM_UPPER2);
                break;
            default:
                GetTrackPosition("Zombie_outerarm_lower", aPosX, aPosY);
                aBodyReanim->SetImageOverride("Zombie_outerarm_upper", IMAGE_REANIM_ZOMBIE_OUTERARM_UPPER2);
                break;
        }
    }

    if (!mInPool && !TestBit(theDamageFlags, DamageFlags::DAMAGE_DOESNT_LEAVE_BODY)) {
        ParticleEffect aEffect = ParticleEffect::PARTICLE_ZOMBIE_ARM;
        if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED) {
            aEffect = ParticleEffect::PARTICLE_MOWERED_ZOMBIE_ARM;
        }

        TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, mRenderOrder + 1, aEffect);
        OverrideParticleColor(aParticle);
        OverrideParticleScale(aParticle);

        if (aParticle) {
            switch (mZombieType) {
                case ZombieType::ZOMBIE_GIGA_FOOTBALL:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_LEFTARM_HAND);
                    break;
                case ZombieType::ZOMBIE_SUPER_FAN_IMP:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_SUPER_FAN_IMP_OUTARM_GLOVE);
                    aParticle->OverrideScale(nullptr, 0.45f);
                    break;
                case ZombieType::ZOMBIE_GIGA_IMP:
                    aParticle->OverrideImage(nullptr, IMAGE_REANIM_ZOMBIE_IMP_ARM2);
                    break;
                case ZombieType::ZOMBIE_JACKSON:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_HAND);
                    break;
                case ZombieType::ZOMBIE_BACKUP_JACKSON:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_DANCER_INNERARM_HAND);
                    break;
                case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
                case ZombieType::ZOMBIE_EXPLORER:
                case ZombieType::ZOMBIE_DOGWALKER:
                    aParticle->OverrideImage(nullptr, IMAGE_REANIM_ZOMBIE_OUTERARM_HAND);
                    break;
                case ZombieType::ZOMBIE_TELEPORTATION:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_TELEPORTATION_TELEPHONE);
                    break;
                case ZombieType::ZOMBIE_CROSSING_GUARD:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_ZOMBIE_CROSSING_GUARD_ARM);
                    break;
                case ZombieType::ZOMBIE_SCIENTIST:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_SCIENTIST_HAND);
                    break;
                case ZombieType::ZOMBIE_SUNDAY_EDITION:
                    aParticle->OverrideImage(nullptr, addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_LEFTARM_LOWER);
                    break;
                default:
                    break;
            }
        }
    }
}

void Zombie::DropArm(unsigned int theDamageFlags) {
    if (mZombieType >= ZombieType::NUM_CACHED_ZOMBIE_TYPES) {
        if (!CanLoseBodyParts()) {
            return;
        }
        if (mShieldType == ShieldType::SHIELDTYPE_DOOR || mShieldType == ShieldType::SHIELDTYPE_TRASHCAN || mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER || mZombieType == ZombieType::ZOMBIE_BUNGEE
            || mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION) {
            return;
        }
        if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL
            || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP || mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_READING) {
            return;
        }
        if (!mHasArm) {
            return;
        }

        mHasArm = false;
        SetupReanimForLostArm(theDamageFlags);
        mApp->PlayFoley(FoleyType::FOLEY_LIMBS_POP);
        return;
    }
    old_Zombie_DropArm(this, theDamageFlags);
}

int Zombie::GetHelmDamageIndex() const {
    if (mHelmHealth < mHelmMaxHealth / 3) {
        return 2;
    }

    if (mHelmHealth < mHelmMaxHealth * 2 / 3) {
        return 1;
    }

    return 0;
}

int Zombie::GetBodyDamageIndex() const {
    if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        if (mBodyHealth < mBodyMaxHealth / 2) {
            return 2;
        }

        if (mBodyHealth < mBodyMaxHealth * 4 / 5) {
            return 1;
        }

        return 0;
    } else {
        if (mBodyHealth < mBodyMaxHealth / 3) {
            return 2;
        }

        if (mBodyHealth < mBodyMaxHealth * 2 / 3) {
            return 1;
        }

        return 0;
    }
}

int Zombie::GetShieldDamageIndex() const {
    if (mShieldHealth < mShieldMaxHealth / 3) {
        return 2;
    }

    if (mShieldHealth < mShieldMaxHealth * 2 / 3) {
        return 1;
    }

    return 0;
}

bool Zombie::IsFireResistant() const {
    return mZombieType == ZombieType::ZOMBIE_CATAPULT || mZombieType == ZombieType::ZOMBIE_ZAMBONI || mShieldType == ShieldType::SHIELDTYPE_DOOR || mShieldType == ShieldType::SHIELDTYPE_LADDER
        || mShieldType == ShieldType::SHIELDTYPE_TRASHCAN;
}

Rect Zombie::GetZombieAttackRect() {
    Rect aAttackRect = mZombieAttackRect;
    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP) {
        aAttackRect = Rect(-40, 0, 100, 115);
    }

    if (IsWalkingBackwards()) {
        aAttackRect.mX = mWidth - aAttackRect.mX - aAttackRect.mWidth;
    }

    ZombieDrawPosition aDrawPos{};
    GetDrawPos(aDrawPos);
    aAttackRect.Offset(mX, mY + aDrawPos.mBodyY);
    if (aDrawPos.mClipHeight > CLIP_HEIGHT_LIMIT) {
        aAttackRect.mHeight -= aDrawPos.mClipHeight;
    }

    return aAttackRect;
}

Plant *Zombie::FindPlantTarget(ZombieAttackType theAttackType) {
    if (mMindControlled)
        return nullptr;

    Rect aAttackRect = GetZombieAttackRect();

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == mRow) {
            Rect aPlantRect = aPlant->GetPlantRect();
            if (GetRectOverlap(aAttackRect, aPlantRect) >= 20 && CanTargetPlant(aPlant, theAttackType)) {
                return aPlant;
            }
        }
    }

    return nullptr;
}

Plant *Zombie::FindFriendPlantTarget() {
    if (!mMindControlled)
        return nullptr;

    Rect aAttackRect = GetZombieAttackRect();

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == mRow) {
            Rect aPlantRect = aPlant->GetPlantRect();
            if (GetRectOverlap(aAttackRect, aPlantRect) >= 20) {
                return aPlant;
            }
        }
    }

    return nullptr;
}

bool Zombie::CanTargetPlant(Plant *thePlant, ZombieAttackType theAttackType) {
    if (mApp->IsWallnutBowlingLevel() && theAttackType != ZombieAttackType::ATTACKTYPE_VAULT)
        return false;

    if (thePlant->NotOnGround() || thePlant->mSeedType == SeedType::SEED_TANGLEKELP)
        return false;

    if (mApp->IsVSMode() && IsFlying() && mBoard->IsPoolSquare(thePlant->mPlantCol, thePlant->mRow) && thePlant->mSeedType != SeedType::SEED_LILYPAD) {
        return true;
    }

    if (!mInPool && mBoard->IsPoolSquare(thePlant->mPlantCol, thePlant->mRow))
        return false;

    if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING) {
        return (thePlant->mSeedType == SeedType::SEED_POTATOMINE && thePlant->mState == PlantState::STATE_NOTREADY) || thePlant->mState == PlantState::STATE_CELERY_STALKER_LOW;
    }

    if (mZombieType == ZombieType::ZOMBIE_EXPLORER && mHasObject) {
        if (thePlant->mSeedType == SeedType::SEED_POTATOMINE || thePlant->mSeedType == SeedType::SEED_ICEBERG_LETTUCE) {
            return false;
        }
    }

    if (thePlant->mSeedType == SeedType::SEED_CELERY_STALKER) {
        return IsGargantuar() || mZombieType == ZombieType::ZOMBIE_DOG || theAttackType == ZombieAttackType::ATTACKTYPE_DRIVE_OVER
            || (theAttackType != ZombieAttackType::ATTACKTYPE_LADDER && !thePlant->IsCeleryStalkerLow());
    }

    if (thePlant->IsSpiky()) {
        return IsGargantuar() || mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombieType == ZombieType::ZOMBIE_DOG || mBoard->IsPoolSquare(thePlant->mPlantCol, thePlant->mRow)
            || mBoard->GetFlowerPotAt(thePlant->mPlantCol, thePlant->mRow); // 扶梯僵尸给花盆上的地刺/地刺王搭梯的原理
    }

    if (theAttackType == ZombieAttackType::ATTACKTYPE_DRIVE_OVER) {
        if (thePlant->mSeedType == SeedType::SEED_CHERRYBOMB || thePlant->mSeedType == SeedType::SEED_JALAPENO || thePlant->mSeedType == SeedType::SEED_BLOVER
            || thePlant->mSeedType == SeedType::SEED_SQUASH || thePlant->mSeedType == SeedType::SEED_ICEBERG_LETTUCE || thePlant->mSeedType == SeedType::SEED_CHILLY_PEPPER) {
            return false;
        }
        if (thePlant->mSeedType == SeedType::SEED_DOOMSHROOM || thePlant->mSeedType == SeedType::SEED_ICESHROOM) {
            return thePlant->mIsAsleep;
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_LADDER_CARRYING || mZombiePhase == ZombiePhase::PHASE_LADDER_PLACING) {
        bool aPlaceLadder = false;
        if (thePlant->mSeedType == SeedType::SEED_WALLNUT || thePlant->mSeedType == SeedType::SEED_TALLNUT || thePlant->mSeedType == SeedType::SEED_PUMPKINSHELL
            || thePlant->mSeedType == SeedType::SEED_SWEET_POTATO || thePlant->mSeedType == SeedType::SEED_ENDURIAN) {
            aPlaceLadder = true;
        }

        if (mBoard->GetLadderAt(thePlant->mPlantCol, thePlant->mRow)) {
            aPlaceLadder = false;
        }

        if ((theAttackType == ZombieAttackType::ATTACKTYPE_CHEW && aPlaceLadder) || (theAttackType == ZombieAttackType::ATTACKTYPE_LADDER && !aPlaceLadder)) {
            return false;
        }
    }

    if (theAttackType == ZombieAttackType::ATTACKTYPE_CHEW) {
        Plant *aTopPlant = mBoard->GetTopPlantAt(thePlant->mPlantCol, thePlant->mRow, PlantPriority::TOPPLANT_EATING_ORDER);
        if (aTopPlant != thePlant && aTopPlant && CanTargetPlant(aTopPlant, theAttackType)) {
            return false;
        }
    }

    if (theAttackType == ZombieAttackType::ATTACKTYPE_VAULT) {
        Plant *aTopPlant = mBoard->GetTopPlantAt(thePlant->mPlantCol, thePlant->mRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
        if (aTopPlant != thePlant && aTopPlant && CanTargetPlant(aTopPlant, theAttackType)) {
            return false;
        }
    }

    if (theAttackType == ZombieAttackType::ATTACKTYPE_POLE) {
        if (thePlant->IsLowProfile()) {
            return false;
        }
        Plant *aTopPlant = mBoard->GetTopPlantAt(thePlant->mPlantCol, thePlant->mRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
        if (aTopPlant != thePlant && aTopPlant && CanTargetPlant(aTopPlant, theAttackType)) {
            return false;
        }
    }

    return true;
}

Zombie *Zombie::FindZombieTarget() {
    if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING)
        return nullptr;

    Rect aAttackRect = GetZombieAttackRect();

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (mMindControlled != aZombie->mMindControlled && aZombie->mZombiePhase != ZombiePhase::PHASE_DIGGER_TUNNELING && aZombie->mZombiePhase != ZombiePhase::PHASE_BUNGEE_DIVING
            && aZombie->mZombiePhase != ZombiePhase::PHASE_BUNGEE_DIVING_SCREAMING && aZombie->mZombiePhase != ZombiePhase::PHASE_BUNGEE_RISING
            && aZombie->mZombieHeight != ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED && !aZombie->IsDeadOrDying() && aZombie->mRow == mRow) {
            // 对战气球低空飞行时可被敌方僵尸索敌
            if (aZombie->IsFlying() && (!mApp->IsVSMode() || aZombie->mAltitude >= FLYER_ALTITUDE)) {
                continue;
            }

            if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS && aZombie->mZombiePhase != ZombiePhase::PHASE_BOSS_IDLE) {
                continue;
            }

            Rect aZombieRect = aZombie->GetZombieRect();
            int aOverlap = GetRectOverlap(aAttackRect, aZombieRect);
            if (aOverlap >= 20 || (aOverlap > 0 && aZombie->mIsEating)) {
                return aZombie;
            }
        }
    }

    return nullptr;
}

Zombie *Zombie::FindFriendZombieTarget() {
    if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING)
        return nullptr;

    Rect aAttackRect = GetZombieAttackRect();

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (mMindControlled == aZombie->mMindControlled && aZombie->mZombiePhase != ZombiePhase::PHASE_DIGGER_TUNNELING && aZombie->mZombiePhase != ZombiePhase::PHASE_BUNGEE_DIVING
            && aZombie->mZombiePhase != ZombiePhase::PHASE_BUNGEE_DIVING_SCREAMING && aZombie->mZombiePhase != ZombiePhase::PHASE_BUNGEE_RISING
            && aZombie->mZombieHeight != ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED && !aZombie->IsDeadOrDying() && aZombie->mRow == mRow) {
            if (aZombie->IsFlying() && (!mApp->IsVSMode() || aZombie->mAltitude >= FLYER_ALTITUDE)) {
                continue;
            }

            if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS && aZombie->mZombiePhase != ZombiePhase::PHASE_BOSS_IDLE) {
                continue;
            }

            Rect aZombieRect = aZombie->GetZombieRect();
            int aOverlap = GetRectOverlap(aAttackRect, aZombieRect);
            if (aOverlap >= 20 || (aOverlap > 0 && aZombie->mIsEating)) {
                return aZombie;
            }
        }
    }

    return nullptr;
}

Zombie *Zombie::FindZombieGigaFootball() {
    Rect aZombieImpRect = GetZombieRect();

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (mMindControlled == aZombie->mMindControlled && mZombiePhase != ZombiePhase::PHASE_IMP_GETTING_THROWN && mZombiePhase != ZombiePhase::PHASE_IMP_POPPING
            && mZombiePhase != ZombiePhase::PHASE_IMP_GETTING_BLOCKED && mZombiePhase != ZombiePhase::PHASE_RISING_FROM_GRAVE && aZombie->mZombiePhase != ZombiePhase::PHASE_RISING_FROM_GRAVE
            && mZombieHeight != ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED && aZombie->mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL && !aZombie->IsDeadOrDying() && !aZombie->IsImmobilizied()
            && aZombie->mRow == mRow) {
            Rect aZombieFootballRect = aZombie->GetZombieAttackRect();
            int aOverlap = GetRectOverlap(aZombieImpRect, aZombieFootballRect);
            if (aOverlap > -20) {
                return aZombie;
            }
        }
    }

    return nullptr;
}

void Zombie::TakeDamage(int theDamage, unsigned int theDamageFlags) {
    if (IsRemoteClientOrViewer())
        return;

    if (IsRemoteServer()) {
        U16U16U8_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_TAKE_DAMAGE;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2 = uint16_t(theDamage);
        event.data3 = uint8_t(theDamageFlags);
        netplay::PutEvent(event);
    }

    TakeDamage_Origin(theDamage, theDamageFlags);
}

void Zombie::TakeDamage_Origin(int theDamage, unsigned int theDamageFlags) {
    if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        if (!TestBit(theDamageFlags, int(DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH))) {
            TriggerVibration(VibrationEffect::VIBRATION_BOSS_HIT);
        }
    }

    if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING || IsDeadOrDying() || mZombiePhase == ZombiePhase::PHASE_IMP_POPPING)
        return;

    if (mSunBeanSun > 0 && theDamage > 0) {
        const int aDamageBeforeHeadDrop = std::min(theDamage, GetSunBeanDamageCapacity(theDamageFlags));
        mSunBeanDamageRemainder += aDamageBeforeHeadDrop;
        int aSunValue = std::min(int(mSunBeanSun), mSunBeanDamageRemainder / 20 * 5);
        mSunBeanDamageRemainder -= aSunValue * 4;
        mSunBeanSun -= aSunValue;
        if (mSunBeanSun == 0) {
            mSunBeanDamageRemainder = 0;
        }

        SpawnSunBeanSun(aSunValue);
    }

    int aDamageRemaining = theDamage;

    if (IsFlying()) {
        aDamageRemaining = TakeFlyingDamage(aDamageRemaining, theDamageFlags);
    }
    if (aDamageRemaining > 0 && mShieldType != ShieldType::SHIELDTYPE_NONE && !TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_BYPASSES_SHIELD)) {
        aDamageRemaining = TakeShieldDamage(aDamageRemaining, theDamageFlags);
        if (TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_HITS_SHIELD_AND_BODY)) {
            aDamageRemaining = theDamage;
        }
    }
    if (aDamageRemaining > 0 && mHelmType != HelmType::HELMTYPE_NONE) {
        aDamageRemaining = TakeHelmDamage(aDamageRemaining, theDamageFlags);
    }
    if (aDamageRemaining > 0) {
        TakeBodyDamage(aDamageRemaining, theDamageFlags);
    }
}

void Zombie::TakeBodyDamage(int theDamage, unsigned int theDamageFlags) {
    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH)) {
        mJustGotShotCounter = 25;
    }

    if (TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_FREEZE)) {
        ApplyChill(false);
    }

    int aBodyHealthOrigin = mBodyHealth;
    int aDamageIndexBeforeDamage = GetBodyDamageIndex();
    mBodyHealth -= theDamage;
    int aDamageIndexAfterDamage = GetBodyDamageIndex();
    if (mZombieType == ZombieType::ZOMBIE_ZAMBONI) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH)) {
            mApp->PlayFoley(FoleyType::FOLEY_SHIELD_HIT);
        }

        if (TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_SPIKE)) {
            aBodyReanim->SetImageOverride("Zombie_zamboni_1", IMAGE_REANIM_ZOMBIE_ZAMBONI_1_DAMAGE2);
            aBodyReanim->SetImageOverride("Zombie_zamboni_2", IMAGE_REANIM_ZOMBIE_ZAMBONI_2_DAMAGE2);
            ZamboniDeath(theDamageFlags);
        } else if (mBodyHealth <= 0) {
            ZamboniDeath(theDamageFlags);
        } else if (aDamageIndexBeforeDamage != aDamageIndexAfterDamage) {
            if (aDamageIndexAfterDamage == 1) {
                aBodyReanim->SetImageOverride("Zombie_zamboni_1", IMAGE_REANIM_ZOMBIE_ZAMBONI_1_DAMAGE1);
                aBodyReanim->SetImageOverride("Zombie_zamboni_2", IMAGE_REANIM_ZOMBIE_ZAMBONI_2_DAMAGE1);
            } else if (aDamageIndexAfterDamage == 2) {
                aBodyReanim->SetImageOverride("Zombie_zamboni_1", IMAGE_REANIM_ZOMBIE_ZAMBONI_1_DAMAGE2);
                aBodyReanim->SetImageOverride("Zombie_zamboni_2", IMAGE_REANIM_ZOMBIE_ZAMBONI_2_DAMAGE2);
                AddAttachedParticle(27, 72, ParticleEffect::PARTICLE_ZAMBONI_SMOKE);
            }
        }
    } else if (mZombieType == ZombieType::ZOMBIE_CATAPULT) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_SPIKE) || mBodyHealth <= 0) {
            aBodyReanim->SetImageOverride("Zombie_catapult_siding", IMAGE_REANIM_ZOMBIE_CATAPULT_SIDING_DAMAGE);
            CatapultDeath(theDamageFlags);
        } else if (aDamageIndexBeforeDamage != aDamageIndexAfterDamage) {
            if (aDamageIndexAfterDamage == 1) {
                aBodyReanim->SetImageOverride("Zombie_catapult_siding", IMAGE_REANIM_ZOMBIE_CATAPULT_SIDING_DAMAGE);
            } else if (aDamageIndexAfterDamage == 2) {
                AddAttachedParticle(47, 77, ParticleEffect::PARTICLE_ZAMBONI_SMOKE);
            }
        }
    } else if (mZombieType == ZombieType::ZOMBIE_GARGANTUAR || mZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aDamageIndexBeforeDamage != aDamageIndexAfterDamage) {
            if (aDamageIndexAfterDamage == 1) {
                aBodyReanim->SetImageOverride("Zombie_gargantua_body1", IMAGE_REANIM_ZOMBIE_GARGANTUAR_BODY1_2);
                aBodyReanim->SetImageOverride("Zombie_gargantuar_outerarm_lower", IMAGE_REANIM_ZOMBIE_GARGANTUAR_OUTERARM_LOWER2);
            } else if (aDamageIndexAfterDamage == 2) {
                aBodyReanim->SetImageOverride("Zombie_gargantua_body1", IMAGE_REANIM_ZOMBIE_GARGANTUAR_BODY1_3);
                aBodyReanim->SetImageOverride("Zombie_gargantuar_outerleg_foot", IMAGE_REANIM_ZOMBIE_GARGANTUAR_FOOT2);
                aBodyReanim->SetImageOverride("Zombie_gargantuar_outerarm_lower", IMAGE_REANIM_ZOMBIE_GARGANTUAR_OUTERARM_LOWER2);
                if (mZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR) {
                    aBodyReanim->SetImageOverride("anim_head1", IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD2_REDEYE);
                } else {
                    aBodyReanim->SetImageOverride("anim_head1", IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD2);
                }
            }
        }
    } else if (mZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR) {
        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aDamageIndexBeforeDamage != aDamageIndexAfterDamage) {
            if (aDamageIndexAfterDamage == 1) {
                aBodyReanim->SetImageOverride("Zombie_giga_gargantua_body1", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_BODY1_2);
                aBodyReanim->SetImageOverride("Zombie_giga_gargantuar_outerarm_lower", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_OUTERARM_LOWER2);
            } else if (aDamageIndexAfterDamage == 2) {
                aBodyReanim->SetImageOverride("Zombie_giga_gargantua_body1", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_BODY1_3);
                aBodyReanim->SetImageOverride("Zombie_giga_gargantuar_outerleg_foot", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_FOOT2);
                aBodyReanim->SetImageOverride("Zombie_giga_gargantuar_outerarm_lower", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_OUTERARM_LOWER2);
                aBodyReanim->SetImageOverride("anim_head1", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_HEAD2);
            }
        }
    } else if (mZombieType == ZombieType::ZOMBIE_BOSS) {
        if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH)) {
            mApp->PlayFoley(FoleyType::FOLEY_SHIELD_HIT);
        }

        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aDamageIndexBeforeDamage != aDamageIndexAfterDamage) {
            if (aDamageIndexAfterDamage == 1) {
                aBodyReanim->SetImageOverride("Boss_head", IMAGE_REANIM_ZOMBIE_BOSS_HEAD_DAMAGE1);
                aBodyReanim->SetImageOverride("Boss_jaw", IMAGE_REANIM_ZOMBIE_BOSS_JAW_DAMAGE1);
                aBodyReanim->SetImageOverride("Boss_outerarm_hand", IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_HAND_DAMAGE1);
                aBodyReanim->SetImageOverride("Boss_outerarm_thumb2", IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_THUMB_DAMAGE1);
                aBodyReanim->SetImageOverride("Boss_innerleg_foot", IMAGE_REANIM_ZOMBIE_BOSS_FOOT_DAMAGE1);
            } else if (aDamageIndexAfterDamage == 2) {
                aBodyReanim->SetImageOverride("Boss_head", IMAGE_REANIM_ZOMBIE_BOSS_HEAD_DAMAGE2);
                aBodyReanim->SetImageOverride("Boss_jaw", IMAGE_REANIM_ZOMBIE_BOSS_JAW_DAMAGE2);
                aBodyReanim->SetImageOverride("Boss_outerarm_hand", IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_HAND_DAMAGE2);
                aBodyReanim->SetImageOverride("Boss_outerarm_thumb2", IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_THUMB_DAMAGE2);
                aBodyReanim->SetImageOverride("Boss_outerleg_foot", IMAGE_REANIM_ZOMBIE_BOSS_FOOT_DAMAGE2);
                ApplyBossSmokeParticles(true);
            }
        }

        if (aBodyHealthOrigin >= mBodyMaxHealth / BOSS_FLASH_HEALTH_FRACTION && mBodyHealth < mBodyMaxHealth / BOSS_FLASH_HEALTH_FRACTION) {
            mApp->AddTodParticle(770.0f, 260.0f, Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 0), ParticleEffect::PARTICLE_BOSS_EXPLOSION);
            mApp->PlayFoley(FoleyType::FOLEY_BOSS_EXPLOSION_SMALL);
            ApplyBossSmokeParticles(true);
        }

        if (mBodyHealth <= 0) {
            mBodyHealth = 1;
        }
    } else {
        UpdateDamageStates(theDamageFlags);
    }

    if (mBodyHealth <= 0) {
        mBodyHealth = 0;
        PlayDeathAnim(theDamageFlags);
        DropLoot();
    }
}

int Zombie::TakeHelmDamage(int theDamage, unsigned int theDamageFlags) {
    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH)) {
        mJustGotShotCounter = 25;
    }

    int aDamageIndexBeforeDamage = GetHelmDamageIndex();
    int aDamageActual = std::min(mHelmHealth, theDamage);
    int aDamageRemaining = theDamage - aDamageActual;
    mHelmHealth -= aDamageActual;
    if (TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_FREEZE)) {
        ApplyChill(false);
    }
    if (mHelmHealth == 0) {
        DropHelm(theDamageFlags);
        return aDamageRemaining;
    }

    int aDamageIndexAfterDamage = GetHelmDamageIndex();
    if (aDamageIndexBeforeDamage != aDamageIndexAfterDamage) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (mHelmType == HelmType::HELMTYPE_TRAFFIC_CONE && aDamageIndexAfterDamage == 1 && aBodyReanim) {
            aBodyReanim->SetImageOverride("anim_cone", IMAGE_REANIM_ZOMBIE_CONE2);
        } else if (mHelmType == HelmType::HELMTYPE_TRAFFIC_CONE && aDamageIndexAfterDamage == 2 && aBodyReanim) {
            aBodyReanim->SetImageOverride("anim_cone", IMAGE_REANIM_ZOMBIE_CONE3);
        } else if (mHelmType == HelmType::HELMTYPE_PAIL && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("anim_bucket", IMAGE_REANIM_ZOMBIE_BUCKET2);
        } else if (mHelmType == HelmType::HELMTYPE_PAIL && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("anim_bucket", IMAGE_REANIM_ZOMBIE_BUCKET3);
        } else if (mHelmType == HelmType::HELMTYPE_DIGGER && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("Zombie_digger_hardhat", IMAGE_REANIM_ZOMBIE_DIGGER_HARDHAT2);
        } else if (mHelmType == HelmType::HELMTYPE_DIGGER && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("Zombie_digger_hardhat", IMAGE_REANIM_ZOMBIE_DIGGER_HARDHAT3);
        } else if (mHelmType == HelmType::HELMTYPE_FOOTBALL && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("zombie_football_helmet", IMAGE_REANIM_ZOMBIE_FOOTBALL_HELMET2);
        } else if (mHelmType == HelmType::HELMTYPE_FOOTBALL && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("zombie_football_helmet", IMAGE_REANIM_ZOMBIE_FOOTBALL_HELMET3);
        } else if (mHelmType == HelmType::HELMTYPE_WALLNUT && aDamageIndexAfterDamage == 1) {
            Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
            aHeadReanim->SetImageOverride("anim_face", IMAGE_REANIM_WALLNUT_CRACKED1);
        } else if (mHelmType == HelmType::HELMTYPE_WALLNUT && aDamageIndexAfterDamage == 2) {
            Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
            aHeadReanim->SetImageOverride("anim_face", IMAGE_REANIM_WALLNUT_CRACKED2);
        } else if (mHelmType == HelmType::HELMTYPE_TALLNUT && aDamageIndexAfterDamage == 1) {
            Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
            aHeadReanim->SetImageOverride("anim_idle", IMAGE_REANIM_TALLNUT_CRACKED1);
        } else if (mHelmType == HelmType::HELMTYPE_TALLNUT && aDamageIndexAfterDamage == 2) {
            Reanimation *aHeadReanim = mApp->ReanimationGet(mSpecialHeadReanimID);
            aHeadReanim->SetImageOverride("anim_idle", IMAGE_REANIM_TALLNUT_CRACKED2);
        } else if (mHelmType == HelmType::HELMTYPE_GIGA_FOOTBALL && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("zombie_football_helmet", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET2);
        } else if (mHelmType == HelmType::HELMTYPE_GIGA_FOOTBALL && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("zombie_football_helmet", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET3);
        }
    }
    return aDamageRemaining;
}

int Zombie::TakeFlyingDamage(int theDamage, unsigned int theDamageFlags) {
    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH)) {
        mJustGotShotCounter = 25;
    }

    // 对战气球可被减速
    if (mApp->IsVSMode() && TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_FREEZE)) {
        ApplyChill(false);
    }

    int aDamageActual = std::min(mFlyingHealth, theDamage);
    int aDamageRemaining = theDamage - aDamageActual;
    mFlyingHealth -= aDamageActual;
    if (mFlyingHealth == 0) {
        LandFlyer(theDamageFlags);
    }

    return aDamageRemaining;
}

int Zombie::TakeShieldDamage(int theDamage, unsigned int theDamageFlags) {
    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH)) {
        mShieldJustGotShotCounter = 25;
        if (mJustGotShotCounter < 0) {
            mJustGotShotCounter = 0;
        }
    }

    if (!TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_DOESNT_CAUSE_FLASH) && !TestBit(theDamageFlags, (int)DamageFlags::DAMAGE_HITS_SHIELD_AND_BODY)) {
        mShieldRecoilCounter = 12;
        if (mShieldType == ShieldType::SHIELDTYPE_DOOR || mShieldType == ShieldType::SHIELDTYPE_LADDER || mShieldType == ShieldType::SHIELDTYPE_TRASHCAN) {
            mApp->PlayFoley(FoleyType::FOLEY_SHIELD_HIT);
        }
    }

    int aDamageIndexBeforeDamage = GetShieldDamageIndex();
    int aDamageActual = std::min(mShieldHealth, theDamage);
    int aDamageRemaining = theDamage - aDamageActual;
    mShieldHealth -= aDamageActual;
    if (mShieldHealth == 0) {
        DropShield(theDamageFlags);
        return aDamageRemaining;
    }

    int aDamageIndexAfterDamage = GetShieldDamageIndex();
    if (aDamageIndexAfterDamage != aDamageIndexBeforeDamage) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (mShieldType == ShieldType::SHIELDTYPE_DOOR && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("anim_screendoor", IMAGE_REANIM_ZOMBIE_SCREENDOOR2);
        } else if (mShieldType == ShieldType::SHIELDTYPE_DOOR && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("anim_screendoor", IMAGE_REANIM_ZOMBIE_SCREENDOOR3);
        } else if (mShieldType == ShieldType::SHIELDTYPE_TRASHCAN && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("anim_screendoor", IMAGE_REANIM_ZOMBIE_TRASHCAN2);
        } else if (mShieldType == ShieldType::SHIELDTYPE_TRASHCAN && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("anim_screendoor", IMAGE_REANIM_ZOMBIE_TRASHCAN3);
        } else if (mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("Zombie_paper_paper", IMAGE_REANIM_ZOMBIE_PAPER_PAPER2);
        } else if (mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("Zombie_paper_paper", IMAGE_REANIM_ZOMBIE_PAPER_PAPER3);
        } else if (mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("Zombie_paper_paper", addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER2);
        } else if (mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("Zombie_paper_paper", addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER3);
        } else if (mShieldType == ShieldType::SHIELDTYPE_LADDER && aDamageIndexAfterDamage == 1) {
            aBodyReanim->SetImageOverride("Zombie_ladder_1", IMAGE_REANIM_ZOMBIE_LADDER_1_DAMAGE1);
        } else if (mShieldType == ShieldType::SHIELDTYPE_LADDER && aDamageIndexAfterDamage == 2) {
            aBodyReanim->SetImageOverride("Zombie_ladder_1", IMAGE_REANIM_ZOMBIE_LADDER_1_DAMAGE2);
        }
    }

    return aDamageRemaining;
}

void Zombie::AttachShield() {
    const char *aTrackName = nullptr;
    Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
    if (mShieldType == ShieldType::SHIELDTYPE_DOOR) {
        ShowDoorArms(true);
        ReanimShowPrefix("Zombie_outerarm_screendoor", RENDER_GROUP_OVER_SHIELD);
        aTrackName = "anim_screendoor";
    } else if (mShieldType == ShieldType::SHIELDTYPE_TRASHCAN) {
        ShowDoorArms(true);
        ReanimShowPrefix("Zombie_outerarm_screendoor", RENDER_GROUP_OVER_SHIELD);
        aTrackName = "anim_screendoor";
        aBodyReanim->SetImageOverride("anim_screendoor", IMAGE_REANIM_ZOMBIE_TRASHCAN1);
    } else if (mShieldType == ShieldType::SHIELDTYPE_NEWSPAPER || mShieldType == ShieldType::SHIELDTYPE_SUNDAY_EDITION) {
        ReanimShowPrefix("Zombie_paper_hands", RENDER_GROUP_OVER_SHIELD);
        aTrackName = "Zombie_paper_paper";
    } else if (mShieldType == ShieldType::SHIELDTYPE_LADDER) {
        ReanimShowPrefix("Zombie_outerarm", RENDER_GROUP_OVER_SHIELD);
        aTrackName = "Zombie_ladder_1";
    }

    aBodyReanim->AssignRenderGroupToTrack(aTrackName, RENDER_GROUP_SHIELD);
}

void Zombie::PlayZombieReanim(const char *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate) {
    old_Zombie_PlayZombieReanim(this, theTrackName, theLoopType, theBlendTime, theAnimRate);
}

void Zombie::StartWalkAnim(int theBlendTime) {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr)
        return;

    PickRandomSpeed();
    if (mZombieType == ZombieType::ZOMBIE_DOG && mZombiePhase == ZombiePhase::PHASE_DOG_RUNNING) {
        PlayZombieReanim("anim_run", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_LADDER_CARRYING) {
        PlayZombieReanim("anim_ladderwalk", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD) {
        PlayZombieReanim("anim_walk_nopaper", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if (mInPool && mZombieHeight != ZombieHeight::HEIGHT_IN_TO_POOL && mZombieHeight != ZombieHeight::HEIGHT_OUT_OF_POOL && aBodyReanim->TrackExists("anim_swim")) {
        PlayZombieReanim("anim_swim", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if ((mZombieType == ZombieType::ZOMBIE_NORMAL || mZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE || mZombieType == ZombieType::ZOMBIE_PAIL) && mBoard->mDanceMode) {
        PlayZombieReanim("anim_dance", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT) { // 修复撑杆僵尸被蹦极空投落地后动画异常
        PlayZombieReanim("anim_run", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_POGO_BOUNCING) { // 修复蹦蹦僵尸被蹦极空投落地后动画异常
        PlayZombieReanim("anim_pogo", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 40.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_CHARGING) {
        PlayZombieReanim("anim_charge", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_WALKING) {
        PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_PRE_CHARGE) {
        PlayZombieReanim("anim_prepare", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
    } else {
        int aWalkAnimVariant = Rand(2);
        if (mZombieType == ZombieType::ZOMBIE_PEA_HEAD) {
            aWalkAnimVariant = 0;
        }
        if (mZombieType == ZombieType::ZOMBIE_FLAG) {
            aWalkAnimVariant = 0;
        }

        if (aWalkAnimVariant == 0 && aBodyReanim->TrackExists("anim_walk2")) {
            PlayZombieReanim("anim_walk2", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
        } else if (aBodyReanim->TrackExists("anim_walk")) {
            PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
        }
    }

    // 为 Attach 的鸭子救生圈播放下水动画
    Reanimation *reanim = FindReanimAttachment(mAttachmentID);
    if (reanim && mInPool && mZombieHeight != ZombieHeight::HEIGHT_IN_TO_POOL && mZombieHeight != ZombieHeight::HEIGHT_OUT_OF_POOL && reanim->TrackExists("anim_swim")) {
        reanim->PlayReanim("anim_swim", ReanimLoopType::REANIM_LOOP, theBlendTime, 0.0f);
    }
}

void Zombie::ReanimShowPrefix(const char *theTrackPrefix, int theRenderGroup) {
    old_Zombie_ReanimShowPrefix(this, theTrackPrefix, theRenderGroup);
}

void Zombie::ReanimShowTrack(const char *theTrackName, int theRenderGroup) {
    old_Zombie_ReanimShowTrack(this, theTrackName, theRenderGroup);
}

float Zombie::GetPosYBasedOnRow(int theRow) {
    return old_Zombie_GetPosYBasedOnRow(this, theRow);
}

void Zombie::SetRow(int theRow) {
    old_Zombie_SetRow(this, theRow);

    if (mZombieType != ZombieType::ZOMBIE_DOGWALKER && mZombieType != ZombieType::ZOMBIE_DOG) {
        return;
    }

    Zombie *aPartner = GetDogPartner();
    if (aPartner != nullptr && !aPartner->IsDeadOrDying() && aPartner->mRow != theRow) {
        aPartner->SetRow(theRow);
    }
}

void Zombie::StartMindControlled() {
    if (IsRemoteClientOrViewer())
        return;

    if (mApp->mGameScene == SCENE_PLAYING) {
        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_MIND_CONTROLLED}, uint16_t(mBoard->mZombies.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
    }

    StartMindControlled_Origin();
}

void Zombie::StartMindControlled_Origin() {
    SettleSunBeanSun();

    mApp->PlaySample(SOUND_MINDCONTROLLED);
    mMindControlled = true;
    mLastPortalX = -1;
    mCanRevived = false;

    if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_JACKSON) {
        for (int i = 0; i < NUM_BACKUP_DANCERS; i++) {
            mFollowerZombieID[i] = ZombieID::ZOMBIEID_NULL;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
        Zombie *aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
        if (aLeader) {
            ZombieID aId = mBoard->ZombieGetID(this);
            for (int i = 0; i < NUM_BACKUP_DANCERS; i++) {
                if (aLeader->mFollowerZombieID[i] == aId) {
                    aLeader->mFollowerZombieID[i] = ZombieID::ZOMBIEID_NULL;
                    break;
                }
            }
        }

        mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
    } else if (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) {
        Zombie *aPartner = GetDogPartner();
        if (aPartner != nullptr) {
            Zombie *aWalker = mZombieType == ZombieType::ZOMBIE_DOGWALKER ? this : aPartner;
            Zombie *aDog = mZombieType == ZombieType::ZOMBIE_DOG ? this : aPartner;
            aWalker->HandleDogPartnerLost();
            aDog->HandleDogPartnerLost();
        } else {
            mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
        }
    } else {
        Zombie *aZombie = mBoard->ZombieTryToGet(mRelatedZombieID);
        if (aZombie) {
            aZombie->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
            mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_JACKSON && !mBoard->GetLiveZombieByType(ZombieType::ZOMBIE_JACKSON)) {
        mBoard->SetDanceMode(false);
        msDeadFollowers.clear();
    }
}

void Zombie::ConvertToImp() {
    if (IsRemoteClientOrViewer())
        return;

    if (mApp->mGameScene == SCENE_PLAYING) {
        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_CONVERT_TO_IMP}, uint16_t(mBoard->mZombies.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
    }

    ConvertToImp_Origin();
}

void Zombie::ConvertToImp_Origin() {
    if (mZombieType == ZombieType::ZOMBIE_IMP || mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP || mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
        mPoisoned = true;
        mPoisonedCounter = ZOMBIE_POISONING_TIME;
        StopEating();
        PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0f);
    } else {
        mApp->PlayFoley(FoleyType::FOLEY_IMP);

        Zombie *aZombieImp = mBoard->AddZombieInRow(ZombieType::ZOMBIE_IMP, mRow, mFromWave, false);
        if (aZombieImp != nullptr) {
            aZombieImp->mPosX = mPosX;
            aZombieImp->mPosY = mPosY;
            aZombieImp->mZombiePhase = ZombiePhase::PHASE_IMP_PRE_RUN;
        }
        if (mHelmType != HelmType::HELMTYPE_NONE) {
            DropHelm(0U);
        }
        if (mShieldType != ShieldType::SHIELDTYPE_NONE) {
            DropShield(0U);
        }
        DieNoLoot();
    }
}

void Zombie::UpdateReanim() {
    old_Zombie_UpdateReanim(this);

    if (mZombieType >= ZombieType::NUM_CACHED_ZOMBIE_TYPES) {
        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim == nullptr || aBodyReanim->mDead)
            return;

        //        UpdateReanimColor();

        ZombieDrawPosition aDrawPos{};
        GetDrawPos(aDrawPos);
        float anOffsetX = aDrawPos.mImageOffsetX + 15.0f;
        float anOffsetY = aDrawPos.mImageOffsetY + aDrawPos.mBodyY - 28.0f + 20.0f;

        bool anOpposite = false;
        if (IsWalkingBackwards()) {
            anOpposite = true;
        }
        if (mZombieType == ZombieType::ZOMBIE_JACKSON || mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
            anOpposite = false;

            if (mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN || mZombiePhase == ZombiePhase::PHASE_DANCER_RAISE_RIGHT_1 || mZombiePhase == ZombiePhase::PHASE_DANCER_RAISE_RIGHT_2) {
                if (!mIsEating) {
                    anOpposite = true;
                }
            }

            if (mMindControlled) {
                anOpposite = !anOpposite;
            }
        }
        if (anOpposite) {
            anOffsetX += 90.0f * mScaleZombie;
        }

        aBodyReanim->mOverlayMatrix.m10 = 0.0f;
        aBodyReanim->mOverlayMatrix.m20 = 0.0f;
        aBodyReanim->mOverlayMatrix.m11 = 0.0f;
        aBodyReanim->mOverlayMatrix.m21 = 0.0f;
        float scaleMultiplier = 1.0f;
        float totalScale = mScaleZombie * scaleMultiplier;
        aBodyReanim->OverrideScale(totalScale, totalScale);
        aBodyReanim->SetPosition(anOffsetX + 30.0f - totalScale * 30.0f, anOffsetY + 120.0f - totalScale * 120.0f);
        if (anOpposite) {
            aBodyReanim->mOverlayMatrix.m00 = -totalScale;
        }

        Reanimation *aMoweredReanim = mApp->ReanimationTryToGet(mMoweredReanimID);
        if (aMoweredReanim) {
            aMoweredReanim->Update();

            SexyTransform2D aOverlayMatrix;
            aMoweredReanim->GetAttachmentOverlayMatrix(0, aOverlayMatrix);
            aOverlayMatrix.m00 *= aBodyReanim->mOverlayMatrix.m00;
            aOverlayMatrix.m10 *= aBodyReanim->mOverlayMatrix.m00;
            aOverlayMatrix.m01 *= aBodyReanim->mOverlayMatrix.m11;
            aOverlayMatrix.m11 *= aBodyReanim->mOverlayMatrix.m11;
            aOverlayMatrix.m02 *= aBodyReanim->mOverlayMatrix.m00;
            aOverlayMatrix.m12 *= aBodyReanim->mOverlayMatrix.m11;
            aOverlayMatrix.m02 += aBodyReanim->mOverlayMatrix.m11;
            aOverlayMatrix.m12 += aBodyReanim->mOverlayMatrix.m12;
            aBodyReanim->mOverlayMatrix = aOverlayMatrix;
        }
    }
}

bool Zombie::IsImmobilizied() const {
    return mIceTrapCounter > 0 || mButteredCounter > 0;
}

void Zombie::SetupLostArmReanim() {
    switch (mZombieType) {
        case ZombieType::ZOMBIE_FOOTBALL:
            ReanimShowPrefix("Zombie_football_leftarm_lower", -1);
            ReanimShowPrefix("Zombie_football_leftarm_hand", -1);
            break;
        case ZombieType::ZOMBIE_NEWSPAPER:
            ReanimShowTrack("Zombie_paper_hands", -1);
            ReanimShowTrack("Zombie_paper_leftarm_lower", -1);
            break;
        case ZombieType::ZOMBIE_POLEVAULTER:
        case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
            ReanimShowTrack("Zombie_polevaulter_outerarm_lower", -1);
            ReanimShowTrack("Zombie_outerarm_hand", -1);
            break;
        case ZombieType::ZOMBIE_DANCER:
        case ZombieType::ZOMBIE_JACKSON:
            ReanimShowPrefix("Zombie_disco_outerarm_lower", -1);
            ReanimShowPrefix("Zombie_disco_outerhand_point", -1);
            ReanimShowPrefix("Zombie_disco_outerhand", -1);
            ReanimShowPrefix("Zombie_disco_outerarm_upper", -1);
            break;
        case ZombieType::ZOMBIE_BACKUP_DANCER:
        case ZombieType::ZOMBIE_BACKUP_JACKSON:
            ReanimShowPrefix("Zombie_disco_outerarm_lower", -1);
            ReanimShowPrefix("Zombie_disco_outerhand", -1);
            break;
        case ZombieType::ZOMBIE_SUPER_FAN_IMP:
            ReanimShowTrack("Zombie_Ghost_Fans2", RENDER_GROUP_HIDDEN);
            ReanimShowTrack("Zombie_outerarm_lower", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_GIGA_IMP:
            ReanimShowTrack("Zombie_giga_outerarm_lower", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_TELEPORTATION:
            ReanimShowPrefix("Zombie_teleportation_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_teleportation_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        case ZombieType::ZOMBIE_CROSSING_GUARD:
            ReanimShowPrefix("Zombie_crossing_guard_outerarm_lower", RENDER_GROUP_HIDDEN);
            ReanimShowPrefix("Zombie_crossing_guard_outerarm_hand", RENDER_GROUP_HIDDEN);
            break;
        default:
            ReanimShowPrefix("Zombie_outerarm_lower", -1);
            ReanimShowPrefix("Zombie_outerarm_hand", -1);
            break;
    }
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim != nullptr) {
        switch (mZombieType) {
            case ZombieType::ZOMBIE_FOOTBALL:
                aBodyReanim->SetImageOverride("zombie_football_leftarm_hand", Sexy::IMAGE_REANIM_ZOMBIE_FOOTBALL_LEFTARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_NEWSPAPER:
                aBodyReanim->SetImageOverride("Zombie_paper_leftarm_lower", Sexy::IMAGE_REANIM_ZOMBIE_PAPER_LEFTARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_POLEVAULTER:
            case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
                aBodyReanim->SetImageOverride("Zombie_polevaulter_outerarm_lower", Sexy::IMAGE_REANIM_ZOMBIE_POLEVAULTER_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_BALLOON:
                aBodyReanim->SetImageOverride("zombie_outerarm_lower", Sexy::IMAGE_REANIM_ZOMBIE_BALLOON_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_IMP:
                aBodyReanim->SetImageOverride("Zombie_imp_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_IMP_ARM1_BONE);
                break;
            case ZombieType::ZOMBIE_SUPER_FAN_IMP:
                aBodyReanim->SetImageOverride("Zombie_imp_outerarm_upper", IMAGE_REANIM_ZOMBIE_IMP_ARM1_BONE);
                break;
            case ZombieType::ZOMBIE_GIGA_IMP:
                aBodyReanim->SetImageOverride("Zombie_giga_imp_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_GIGA_IMP_ARM1_BONE);
                break;
            case ZombieType::ZOMBIE_DIGGER:
                aBodyReanim->SetImageOverride("Zombie_digger_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_IMP_ARM1_BONE);
                break;
            case ZombieType::ZOMBIE_BOBSLED:
                aBodyReanim->SetImageOverride("Zombie_dolphinrider_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_BOBSLED_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_JACK_IN_THE_BOX:
                aBodyReanim->SetImageOverride("Zombie_jackbox_outerarm_lower", Sexy::IMAGE_REANIM_ZOMBIE_JACKBOX_OUTERARM_LOWER2);
                break;
            case ZombieType::ZOMBIE_SNORKEL:
                aBodyReanim->SetImageOverride("Zombie_snorkle_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_SNORKLE_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_DOLPHIN_RIDER:
                aBodyReanim->SetImageOverride("Zombie_dolphinrider_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_DOLPHINRIDER_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_POGO:
                aBodyReanim->SetImageOverride("Zombie_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_POGO_OUTERARM_UPPER2);
                aBodyReanim->SetImageOverride("Zombie_pogo_stickhands", Sexy::IMAGE_REANIM_ZOMBIE_POGO_STICKHANDS2);
                aBodyReanim->SetImageOverride("Zombie_pogo_stick", Sexy::IMAGE_REANIM_ZOMBIE_POGO_STICKDAMAGE2);
                aBodyReanim->SetImageOverride("Zombie_pogo_stick2", Sexy::IMAGE_REANIM_ZOMBIE_POGO_STICK2DAMAGE2);
                break;
            case ZombieType::ZOMBIE_FLAG: {
                aBodyReanim->SetImageOverride("Zombie_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_OUTERARM_UPPER2);
                Reanimation *reanimation2 = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
                if (reanimation2 != nullptr) {
                    reanimation2->SetImageOverride("Zombie_flag", Sexy::IMAGE_REANIM_ZOMBIE_FLAG3);
                }
                break;
            }
            case ZombieType::ZOMBIE_DANCER:
                aBodyReanim->SetImageOverride("Zombie_disco_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_DISCO_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_BACKUP_DANCER:
                aBodyReanim->SetImageOverride("Zombie_disco_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_BACKUP_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_JACKSON:
                aBodyReanim->SetImageOverride("Zombie_disco_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_BACKUP_JACKSON:
                aBodyReanim->SetImageOverride("Zombie_disco_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_BACKUP_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_LADDER:
                aBodyReanim->SetImageOverride("Zombie_ladder_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_LADDER_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_YETI:
                aBodyReanim->SetImageOverride("Zombie_yeti_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_YETI_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_TELEPORTATION:
                aBodyReanim->SetImageOverride("Zombie_teleportation_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_TELEPORTATION_OUTERARM_UPPER2);
                break;
            case ZombieType::ZOMBIE_CROSSING_GUARD:
                aBodyReanim->SetImageOverride("Zombie_crossing_guard_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_CROSSING_GUARD_OUTERARM_UPPER2);
                break;
            default:
                aBodyReanim->SetImageOverride("Zombie_outerarm_upper", Sexy::IMAGE_REANIM_ZOMBIE_OUTERARM_UPPER2);
                break;
        }
    }
}

void Zombie::BungeeDropZombie(Zombie *theDroppedZombie, int theGridX, int theGridY) {
    if (IsRemoteClientOrViewer())
        return;

    BungeeDropZombie_Origin(theDroppedZombie, theGridX, theGridY);

    if (IsRemoteServer()) {
        U16UNI32UNI32_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_DROP_ZOMBIE;
        event.data2.u16x2.u16_1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2.u16x2.u16_2 = uint16_t(mBoard->mZombies.DataArrayGetID(theDroppedZombie));
        event.data2.u8x4.u8_1 = uint8_t(theGridX);
        event.data2.u8x4.u8_2 = uint8_t(theGridY);
        netplay::PutEvent(event);
    }
}

void Zombie::BungeeDropZombie_Origin(Zombie *theDroppedZombie, int theGridX, int theGridY) {
    mTargetCol = theGridX;
    SetRow(theGridY);
    mPosX = mBoard->GridToPixelX(mTargetCol, mRow);
    mPosY = GetPosYBasedOnRow(mRow);
    PlayZombieReanim("anim_raise", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 36.0f);
    mRelatedZombieID = mBoard->ZombieGetID(theDroppedZombie);

    theDroppedZombie->mPosX = mPosX - 15.0f;
    theDroppedZombie->SetRow(theGridY);
    theDroppedZombie->mPosY = GetPosYBasedOnRow(theGridY);
    theDroppedZombie->mZombieHeight = ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED;
    theDroppedZombie->PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
    theDroppedZombie->mRenderOrder = mRenderOrder + 1;
    // 修复蹦蹦僵尸被蹦极空投时动画不正确
    if (theDroppedZombie->mZombieType == ZombieType::ZOMBIE_POGO) {
        theDroppedZombie->PlayZombieReanim("anim_pogo", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 40.0f);
    }
}

void Zombie::PickRandomSpeed() {
    if (IsRemoteClientOrViewer())
        return;

    if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL || (IsFlying() && mApp->IsVSMode())) {
        mVelX = 0.3f;
    } else if (mZombiePhase == ZombiePhase::PHASE_DIGGER_WALKING) { // 矿工行走
        if (mApp->IsIZombieLevel()) {
            mVelX = 0.23f; // IZ模式
        } else {
            mVelX = 0.12f; // 一般模式
        }
    } else if (mZombieType == ZombieType::ZOMBIE_IMP && mApp->IsIZombieLevel()) { // IZ小鬼
        mVelX = 0.9f;
    } else if (mZombiePhase == ZombiePhase::PHASE_YETI_RUNNING) {
        mVelX = 0.8f;
    } else if (mZombieType == ZombieType::ZOMBIE_YETI) {
        mVelX = 0.4f;
    } else if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER || mZombieType == ZombieType::ZOMBIE_POGO || mZombieType == ZombieType::ZOMBIE_FLAG
               || mZombiePhase == ZombiePhase::PHASE_IMP_RUNNING || mZombieType == ZombieType::ZOMBIE_JACKSON || mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON
               || mZombieType == ZombieType::ZOMBIE_EXPLORER || mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_SCIENTIST
               || mZombiePhase == ZombiePhase::PHASE_DOG_WALKING) {
        mVelX = 0.45f;
    } else if (mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT || mZombieType == ZombieType::ZOMBIE_FOOTBALL
               || mZombieType == ZombieType::ZOMBIE_SNORKEL || mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX) {
        mVelX = RandRangeFloat(0.66f, 0.68f);
    } else if (mZombiePhase == ZombiePhase::PHASE_LADDER_CARRYING || mZombieType == ZombieType::ZOMBIE_SQUASH_HEAD) {
        mVelX = RandRangeFloat(0.79f, 0.81f);
    } else if (mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MAD || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_WITHOUT_DOLPHIN
               || mZombiePhase == ZombiePhase::PHASE_DOG_RUNNING) {
        mVelX = RandRangeFloat(0.89f, 0.91f);
    } else if (mZombiePhase == ZombiePhase::PHASE_FOOTBALL_CHARGING) {
        mVelX = 1.5f;
    } else if (mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD) {
        mVelX = 0.12f;
    } else {
        mVelX = RandRangeFloat(0.23f, 0.37f); // 普僵
        if (mVelX < 0.3f) {
            mAnimTicksPerFrame = 12;
        } else {
            mAnimTicksPerFrame = 15;
        }
    }

    UpdateAnimSpeed();

    if (IsRemoteServer()) {
        U16U16U16UNI32UNI32_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PICK_SPEED;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2 = uint16_t(mAnimTicksPerFrame);
        event.data4.f32 = mVelX;
        event.data5.f32 = mPosX;
        netplay::PutEvent(event);
    }
}

void Zombie::ApplySyncedSpeed(float theVelX, short theAnimTicks) {
    mVelX = theVelX;
    mAnimTicksPerFrame = theAnimTicks;

    UpdateAnimSpeed();
}

float Zombie::ZombieTargetLeadX(float theTime) {
    float aSpeed = mVelX;
    if (mChilledCounter > 0) {
        aSpeed *= CHILLED_SPEED_FACTOR;
    }
    if (IsWalkingBackwards()) {
        aSpeed = -aSpeed;
    }
    bool aMovementBlocked = ZombieNotWalking();

    if (!aMovementBlocked && (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG)) {
        Zombie *aWalker = mZombieType == ZombieType::ZOMBIE_DOGWALKER ? this : GetDogPartner();
        Zombie *aDog = mZombieType == ZombieType::ZOMBIE_DOG ? this : GetDogPartner();

        if (aWalker != nullptr && aDog != nullptr && !aWalker->IsDeadOrDying() && !aDog->IsDeadOrDying() && aWalker->mMindControlled == aDog->mMindControlled) {
            aMovementBlocked = aWalker->ZombieNotWalking() || aDog->ZombieNotWalking();
        }
    }

    if (aMovementBlocked) {
        aSpeed = 0.0f;
    }

    Rect aZombieRect = GetZombieRect();
    float aCurrentPosX = aZombieRect.mX + aZombieRect.mWidth / 2.0f;
    float aDisplacementX = aSpeed * theTime;
    return aCurrentPosX - aDisplacementX;
}

void Zombie::ApplyBurn() {
    if (mDead || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_BURNED)
        return;

    if (mBodyHealth >= 1800 || mZombieType == ZombieType::ZOMBIE_BOSS) {
        TakeDamage(1800, 18U);
        return;
    }

    if (IsZomblob(mZombieType)) {
        if (mButtered) {
            DieNoLoot();
        } else {
            TakeDamage(1800, 18U);
        }
        return;
    }

    // 立即结算阳光豆的阳光
    SettleSunBeanSun();

    if (mZombieType == ZombieType::ZOMBIE_SQUASH_HEAD && !mHasHead) {
        Reanimation *aReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aReanim) {
            aReanim->ReanimationDie();
        }
        mSpecialHeadReanimID = ReanimationID::REANIMATIONID_NULL;
    }

    if (mIceTrapCounter > 0) {
        RemoveIceTrap();
    }
    if (mButteredCounter > 0) {
        mButteredCounter = 0;
    }

    AttachmentDetachCrossFadeParticleType(mAttachmentID, ParticleEffect::PARTICLE_ZAMBONI_SMOKE, nullptr);
    BungeeDropPlant();

    // 被灰烬炸死的僵尸禁止复活
    if (CanRevived()) {
        mCanRevived = false;
    }

    if (mZombiePhase == ZombiePhase::PHASE_ZOMBIE_DYING || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN
        || mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE || mZombiePhase == ZombiePhase::PHASE_DANCER_RISING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL
        || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING || mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED || mInPool) {
        DieWithLoot();
    } else if (mZombieType == ZOMBIE_BUNGEE || mZombieType == ZOMBIE_YETI || mZombieType == ZOMBIE_CROSSING_GUARD || mZombieType == ZOMBIE_DOG || Zombie::IsZombotany(mZombieType)
               || IsBobsledTeamWithSled() || IsFlying() || !mHasHead) {
        SetAnimRate(0.0f);
        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->mAnimRate = 0.0f;
        }

        mZombiePhase = ZombiePhase::PHASE_ZOMBIE_BURNED;
        mPhaseCounter = 300;
        mJustGotShotCounter = 0;
        DropLoot();

        if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
            BalloonPropellerHatSpin(false);
        }
    } else {
        ReanimationType aReanimType = ReanimationType::REANIM_ZOMBIE_CHARRED;
        float aCharredPosX = mPosX + 22.0f;
        float aCharredPosY = mPosY - 10.0f;
        if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
            aCharredPosY += 31.0f;
        }
        if (mZombieType == ZombieType::ZOMBIE_IMP || mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP || mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
            aCharredPosX -= 6.0f;
            aReanimType = ReanimationType::REANIM_ZOMBIE_CHARRED_IMP;
        }
        if (mZombieType == ZombieType::ZOMBIE_DIGGER) {
            if (IsWalkingBackwards()) {
                aCharredPosX += 14.0f;
            }
            aReanimType = ReanimationType::REANIM_ZOMBIE_CHARRED_DIGGER;
        }
        if (mZombieType == ZombieType::ZOMBIE_ZAMBONI) {
            aReanimType = ReanimationType::REANIM_ZOMBIE_CHARRED_ZAMBONI;
            aCharredPosX += 61.0f;
            aCharredPosY -= 16.0f;
        }
        if (mZombieType == ZombieType::ZOMBIE_CATAPULT) {
            aReanimType = ReanimationType::REANIM_ZOMBIE_CHARRED_CATAPULT;
            aCharredPosX -= 36.0f;
            aCharredPosY -= 20.0f;
        }
        if (IsGargantuar()) {
            aReanimType = ReanimationType::REANIM_ZOMBIE_CHARRED_GARGANTUAR;
            aCharredPosX -= 15.0f;
            aCharredPosY -= 10.0f;
        }

        Reanimation *aCharredReanim = mApp->AddReanimation(aCharredPosX, aCharredPosY, mRenderOrder, aReanimType);
        aCharredReanim->mAnimRate *= RandRangeFloat(0.9f, 1.1f);
        if (mZombiePhase == ZombiePhase::PHASE_DIGGER_WALKING_WITHOUT_AXE) {
            aCharredReanim->SetFramesForLayer("anim_crumble_noaxe");
        } else if (mZombieType == ZombieType::ZOMBIE_DIGGER) {
            aCharredReanim->SetFramesForLayer("anim_crumble");
        } else if (IsGargantuar() && !mHasObject) {
            aCharredReanim->SetImageOverride("impblink", IMAGE_BLANK);
            aCharredReanim->SetImageOverride("imphead", IMAGE_BLANK);
        }

        if (mScaleZombie != 1.0f) {
            aCharredReanim->mOverlayMatrix.m00 = mScaleZombie;
            aCharredReanim->mOverlayMatrix.m11 = mScaleZombie;
            aCharredReanim->mOverlayMatrix.m02 += 20.0f - mScaleZombie * 20.0f;
            aCharredReanim->mOverlayMatrix.m12 += 120.0f - mScaleZombie * 120.0f;
            aCharredReanim->OverrideScale(mScaleZombie, mScaleZombie);
        }

        if (IsWalkingBackwards()) {
            aCharredReanim->OverrideScale(-mScaleZombie, mScaleZombie);
            aCharredReanim->mOverlayMatrix.m02 += 60.0f * mScaleZombie;
        }

        DieWithLoot();
    }

    if (mZombieType == ZombieType::ZOMBIE_BOBSLED) {
        BobsledBurn();
    }
}

void Zombie::ApplyButter() {
    if (!mHasHead || !CanBeFrozen())
        return;

    if (mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombieType == ZombieType::ZOMBIE_BOSS || IsTangleKelpTarget() || IsBobsledTeamWithSled() || IsFlying() || IsZomblob(mZombieType))
        return;

    mButteredCounter = 400;
    if (mZombieType != ZombieType::ZOMBIE_DOGWALKER && mZombieType != ZombieType::ZOMBIE_DOG) {
        Zombie *aZombie = mBoard->ZombieTryToGet(mRelatedZombieID);
        const bool aCrossingGuardBinding = aZombie != nullptr && aZombie->mRelatedZombieID == mBoard->ZombieGetID(this)
            && (mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD || aZombie->mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD);
        if (aZombie && !aCrossingGuardBinding) {
            aZombie->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
            mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_POGO) {
        mAltitude = 0.0f;
        if (mOnHighGround) {
            mAltitude += HIGH_GROUND_HEIGHT;
        }
    } else if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
        BalloonPropellerHatSpin(false);
    } else if (Zombie::IsZombotany(mZombieType)) {
        Reanimation *aHeadReanim = mApp->ReanimationTryToGet(mSpecialHeadReanimID);
        if (aHeadReanim) {
            aHeadReanim->mAnimRate = 0.0f;
        }
    }

    UpdateAnimSpeed();
    StopZombieSound();
}

void Zombie::ApplyChill(bool theIsIceTrap) {
    if (!CanBeChilled())
        return;

    if (mChilledCounter == 0) {
        mApp->PlayFoley(FoleyType::FOLEY_FROZEN);
    }

    int aChillTime = 1000;
    if (theIsIceTrap) {
        aChillTime = 2000;
    }
    mChilledCounter = std::max(aChillTime, mChilledCounter);

    UpdateAnimSpeed();

    if (mZombieType == ZombieType::ZOMBIE_EXPLORER && mHasObject) {
        ExplorerTorchConvert(false);
    }
}

void Zombie::HitIceTrap() {
    if (IsRemoteClientOrViewer())
        return;

    bool cold = false;
    if (mChilledCounter > 0 || mIceTrapCounter != 0) {
        cold = true;
    }

    ApplyChill(true);
    uint16_t aIceTrapCounter = 0;
    if (CanBeFrozen()) {
        if (mInPool) {
            aIceTrapCounter = 300;
        } else if (cold) {
            aIceTrapCounter = RandRangeInt(300, 400);
        } else {
            aIceTrapCounter = RandRangeInt(400, 600);
        }

        mIceTrapCounter = aIceTrapCounter;

        StopZombieSound();
        if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
            BalloonPropellerHatSpin(false);
        }
        if (mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_SPIT) {
            mBoard->RemoveParticleByType(ParticleEffect::PARTICLE_ZOMBIE_BOSS_FIREBALL);
        }

        TakeDamage(20, 1U);
        UpdateAnimSpeed();
    }

    if (IsRemoteServer()) {
        U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_ICE_TRAP;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2 = aIceTrapCounter;
        netplay::PutEvent(event);
    }
}

void Zombie::ApplySyncedIceTrap(int theIceTrapCounter) {
    ApplyChill(true);
    if (!CanBeFrozen())
        return;

    mIceTrapCounter = theIceTrapCounter;

    StopZombieSound();
    if (mZombieType == ZombieType::ZOMBIE_BALLOON) {
        BalloonPropellerHatSpin(false);
    }
    if (mZombiePhase == ZombiePhase::PHASE_BOSS_HEAD_SPIT) {
        mBoard->RemoveParticleByType(ParticleEffect::PARTICLE_ZOMBIE_BOSS_FIREBALL);
    }

    TakeDamage(20, 1U);
    UpdateAnimSpeed();
}

bool Zombie::ZombieNotWalking() {
    if (mIsEating || IsImmobilizied()) {
        return true;
    }

    if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING || mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MADDENING || mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_THROWING
        || mZombiePhase == ZombiePhase::PHASE_GARGANTUAR_SMASHING || mZombiePhase == ZombiePhase::PHASE_CATAPULT_LAUNCHING || mZombiePhase == ZombiePhase::PHASE_CATAPULT_RELOADING
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_STUNNED || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT
        || mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD || mZombiePhase == ZombiePhase::PHASE_DANCER_RISING || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_THROWN
        || mZombiePhase == ZombiePhase::PHASE_IMP_GETTING_BLOCKED || mZombiePhase == ZombiePhase::PHASE_IMP_LANDING || mZombiePhase == ZombiePhase::PHASE_LADDER_PLACING
        || mZombieHeight == ZombieHeight::HEIGHT_IN_TO_CHIMNEY || mZombieHeight == ZombieHeight::HEIGHT_GETTING_BUNGEE_DROPPED || mZombieHeight == ZombieHeight::HEIGHT_ZOMBIQUARIUM
        || mZombieType == ZombieType::ZOMBIE_BUNGEE || mZombieType == ZombieType::ZOMBIE_BOSS || mZombiePhase == ZombiePhase::PHASE_DANCER_RAISE_LEFT_1
        || mZombiePhase == ZombiePhase::PHASE_DANCER_WALK_TO_RAISE || mZombiePhase == ZombiePhase::PHASE_DANCER_RAISE_RIGHT_1 || mZombiePhase == ZombiePhase::PHASE_DANCER_RAISE_LEFT_2
        || mZombiePhase == ZombiePhase::PHASE_DANCER_RAISE_RIGHT_2 || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PICK || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PREPARE
        || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_TAKE || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_THROW || mZombiePhase == ZombiePhase::PHASE_FOOTBALL_TACKLING
        || mZombiePhase == ZombiePhase::PHASE_FOOTBALL_KICKING || mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_PREPARING || mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_END
        || mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_PREPARING || mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_ATTACK
        || mZombiePhase == ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_END || mZombiePhase == ZombiePhase::PHASE_DOGWALKER_ROPE_BREAK || mZombiePhase == ZombiePhase::PHASE_TELEPORTATION_SHOOTING
        || mZombiePhase == ZombiePhase::PHASE_SUPER_NOVA_GARGANTUAR_DESTROY || mZombiePhase == ZombiePhase::PHASE_CROSSING_GUARD_THROWING || mZombiePhase == ZombiePhase::PHASE_SCIENTIST_WAITING
        || mZombiePhase == ZombiePhase::PHASE_SCIENTIST_SHOOTING) {
        return true;
    }

    if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER) {
        Zombie *aLeader = nullptr;
        if (mZombieType == ZombieType::ZOMBIE_DANCER) {
            aLeader = this;
        } else {
            aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
        }

        if (aLeader) {
            if (aLeader->IsImmobilizied() || aLeader->mIsEating) {
                return true;
            }

            for (auto &i : aLeader->mFollowerZombieID) {
                Zombie *aDancer = mBoard->ZombieTryToGet(i);
                if (aDancer && (aDancer->IsImmobilizied() || aDancer->mIsEating)) {
                    return true;
                }
            }
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_JACKSON || mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
        Zombie *aLeader = nullptr;
        if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
            aLeader = this;
        } else {
            aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
        }

        if (aLeader) {
            if (aLeader->IsImmobilizied() || aLeader->mIsEating) {
                return true;
            }

            for (auto &i : aLeader->mFollowerZombieID) {
                Zombie *aDancer = mBoard->ZombieTryToGet(i);
                if (aDancer && (aDancer->IsImmobilizied() || aDancer->mIsEating)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Zombie::IsMovingAtChilledSpeed() {
    if (mChilledCounter > 0 || mIceTrapCounter > 0)
        return true;

    if (mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER) {
        Zombie *aLeader = nullptr;
        if (mZombieType == ZombieType::ZOMBIE_DANCER) {
            aLeader = this;
        } else {
            aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
        }

        if (aLeader && !aLeader->IsDeadOrDying()) {
            if (aLeader->mChilledCounter > 0 || aLeader->mIceTrapCounter > 0) {
                return true;
            }

            for (auto &i : aLeader->mFollowerZombieID) {
                Zombie *aDancer = mBoard->ZombieTryToGet(i);
                if (aDancer && !aDancer->IsDeadOrDying() && (aDancer->mChilledCounter > 0 || aDancer->mIceTrapCounter > 0)) {
                    return true;
                }
            }
        }
    }

    if (mZombieType == ZombieType::ZOMBIE_JACKSON || mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
        Zombie *aLeader = nullptr;
        if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
            aLeader = this;
        } else {
            aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
        }

        if (aLeader && !aLeader->IsDeadOrDying()) {
            if (aLeader->mChilledCounter > 0 || aLeader->mIceTrapCounter > 0) {
                return true;
            }

            for (auto &i : aLeader->mFollowerZombieID) {
                Zombie *aDancer = mBoard->ZombieTryToGet(i);
                if (aDancer && !aDancer->IsDeadOrDying() && (aDancer->mChilledCounter > 0 || aDancer->mIceTrapCounter > 0)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void Zombie::UpdateAnimSpeed() {
    if (!IsOnBoard())
        return;

    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr)
        return;

    if (IsImmobilizied() || (mYuckyFace && mYuckyFaceCounter <= 169)) {
        ApplyAnimRate(0.0f);
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_UP_TO_EAT || mZombiePhase == ZombiePhase::PHASE_SNORKEL_DOWN_FROM_EAT || IsDeadOrDying()) {
        ApplyAnimRate(mOriginalAnimRate);
        return;
    }

    if (mIsEating) {
        if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_BALLOON || mZombieType == ZombieType::ZOMBIE_IMP || mZombieType == ZombieType::ZOMBIE_DIGGER
            || mZombieType == ZombieType::ZOMBIE_CROSSING_GUARD || mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX || mZombieType == ZombieType::ZOMBIE_SNORKEL
            || mZombieType == ZombieType::ZOMBIE_YETI || mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP || mZombieType == ZombieType::ZOMBIE_GIGA_IMP
            || mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER || IsZomblob(mZombieType)) {
            ApplyAnimRate(20.0f);
        } else {
            ApplyAnimRate(36.0f);
        }
        return;
    }

    if (ZombieNotWalking() || IsBobsledTeamWithSled() || mZombieType == ZombieType::ZOMBIE_CATAPULT || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING
        || mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL) {
        ApplyAnimRate(mOriginalAnimRate);
        return;
    }

    if (aBodyReanim->TrackExists("_ground")) {
        ReanimatorTrack *aTrack = &aBodyReanim->mDefinition->mTracks[aBodyReanim->FindTrackIndex("_ground")];
        float aDistance = aTrack->mTransforms[aBodyReanim->mFrameStart + aBodyReanim->mFrameCount - 1].mTransX - aTrack->mTransforms[aBodyReanim->mFrameStart].mTransX;
        if (aDistance >= 1e-6f) {
            float aOneOverSpeed = aBodyReanim->mFrameCount / aDistance;
            float aAnimRate = mVelX * aOneOverSpeed * 47.0f / mScaleZombie;
            ApplyAnimRate(aAnimRate);
        }
    }
}

void Zombie::UpdateZombieWalking() {
    if (ZombieNotWalking())
        return;

    // 绳子未断时由遛狗僵尸统一决定组合的水平位移。
    // 狗保留自己的动画，但不再独立消费 _ground 位移，避免减速/恢复
    // 多次切换后两条动画曲线的微小差异不断累积成位置偏差。
    if (mZombieType == ZombieType::ZOMBIE_DOG) {
        Zombie *aWalker = GetDogPartner();
        if (aWalker != nullptr && !aWalker->IsDeadOrDying() && aWalker->mMindControlled == mMindControlled) {
            return;
        }
    }

    const float aOldPosX = mPosX;
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim) {
        float aSpeed = NAN;
        if (IsBouncingPogo() || mZombiePhase == ZombiePhase::PHASE_BALLOON_FLYING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING || mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL
            || mZombieType == ZombieType::ZOMBIE_CATAPULT) {
            aSpeed = mVelX;
            if (IsMovingAtChilledSpeed()) {
                aSpeed *= CHILLED_SPEED_FACTOR;
            }
        } else if (mZombieType == ZombieType::ZOMBIE_ZAMBONI || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING || mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP || IsBobsledTeamWithSled()
                   || mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_SNORKEL_INTO_POOL) {
            aSpeed = mVelX;
        } else if (aBodyReanim->TrackExists("_ground")) {
            aSpeed = aBodyReanim->GetTrackVelocity("_ground") * mScaleZombie;
        } else {
            aSpeed = mVelX;
            if (IsMovingAtChilledSpeed()) {
                aSpeed *= CHILLED_SPEED_FACTOR;
            }
        }

        // 对战模式垃圾桶僵尸的移速为正常的0.2倍
        if (mApp->IsVSMode() && mZombieType == ZombieType::ZOMBIE_TRASHCAN) {
            aSpeed *= 0.2f;
        }

        if (mZombieType == ZombieType::ZOMBIE_JACKSON || mZombieType == ZombieType::ZOMBIE_BACKUP_JACKSON) {
            Zombie *aLeader = nullptr;
            if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
                aLeader = this;
            } else {
                aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
            }

            if (aLeader && !aLeader->IsDeadOrDying()) {
                if (aLeader->IsImmobilizied() || aLeader->mIsEating || aLeader->mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS
                    || aLeader->mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT || aLeader->mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_HOLD) {
                    aSpeed = 0;
                }

                for (auto &i : aLeader->mFollowerZombieID) {
                    Zombie *aDancer = mBoard->ZombieTryToGet(i);
                    if (aDancer && aDancer != this && !aDancer->IsDeadOrDying()) {
                        if (aDancer->ZombieNotWalking())
                            aSpeed = 0;
                    }
                }
            }
        }

        if (mZombieType == ZombieType::ZOMBIE_DOGWALKER || mZombieType == ZombieType::ZOMBIE_DOG) {
            Zombie *aLeader = nullptr;
            if (mZombieType == ZombieType::ZOMBIE_DOGWALKER) {
                aLeader = this;
            } else {
                aLeader = mBoard->ZombieTryToGet(mRelatedZombieID);
            }

            if (aLeader && !aLeader->IsDeadOrDying()) {
                if (aLeader->IsImmobilizied() || aLeader->mIsEating) {
                    aSpeed = 0;
                }

                if (aLeader->mRelatedZombieID != ZombieID::ZOMBIEID_NULL) {
                    Zombie *aDog = mBoard->ZombieTryToGet(aLeader->mRelatedZombieID);
                    if (aDog && aDog != this && !aDog->IsDeadOrDying()) {
                        if (aDog->ZombieNotWalking())
                            aSpeed = 0;
                    }
                }
            }

            Zombie *aPartner = GetDogPartner();
            if (aPartner != nullptr) {
                const bool aChangingRow = IsChangingRow() || aPartner->IsChangingRow();
                const bool aGarlicChangingRow = (mYuckyFace && mYuckyFaceCounter >= 170) || (aPartner->mYuckyFace && aPartner->mYuckyFaceCounter >= 170);
                if (aChangingRow && !aGarlicChangingRow) {
                    aSpeed = 0;
                }
            }
        }

        if (IsWalkingBackwards()) {
            mPosX += aSpeed;
        } else if (mZombiePhase == ZombiePhase::PHASE_DANCER_DANCING_IN) {
            mPosX += aSpeed / mScaleZombie; // 修复舞王缩小后登场位置靠后
        } else {
            mPosX -= aSpeed;
        }

        // 奔跑扬尘的粒子效果在泳池替换为水花
        ParticleEffect aParticleEffect = mInPool ? ParticleEffect::PARTICLE_PLANTING_POOL : ParticleEffect::PARTICLE_DUST_FOOT;
        if (mZombieType == ZombieType::ZOMBIE_FOOTBALL && mFromWave != Zombie::ZOMBIE_WAVE_WINNER) {
            if (aBodyReanim->ShouldTriggerTimedEvent(0.03f)) {
                mApp->AddTodParticle(mX + 81, mY + 106, mRenderOrder - 1, aParticleEffect);
            }
            if (aBodyReanim->ShouldTriggerTimedEvent(0.61f)) {
                mApp->AddTodParticle(mX + 87, mY + 110, mRenderOrder - 1, aParticleEffect);
            }
        }
        if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT) {
            if (aBodyReanim->ShouldTriggerTimedEvent(0.16f)) {
                mApp->AddTodParticle(mX + 81, mY + 106, mRenderOrder - 1, aParticleEffect);
            }
            if (aBodyReanim->ShouldTriggerTimedEvent(0.67f)) {
                mApp->AddTodParticle(mX + 87, mY + 110, mRenderOrder - 1, aParticleEffect);
            }
        }
    } else {
        bool doWalk = false;
        if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING || mZombieType == ZombieType::ZOMBIE_DANCER
            || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER || mZombieType == ZombieType::ZOMBIE_BOBSLED || mZombieType == ZombieType::ZOMBIE_POGO || mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER
            || mZombieType == ZombieType::ZOMBIE_BALLOON) {
            doWalk = true;
        } else if (mZombieType == ZombieType::ZOMBIE_SNORKEL && mInPool) {
            doWalk = true;
        } else if (mFrame >= 0 && mFrame <= 2) {
            doWalk = true;
        } else if (mFrame >= 6 && mFrame <= 8) {
            doWalk = true;
        }

        if (doWalk) {
            float aSpeed = mVelX;
            if (IsMovingAtChilledSpeed()) {
                aSpeed *= CHILLED_SPEED_FACTOR;
            }

            if (IsWalkingBackwards()) {
                mPosX += aSpeed;
            } else {
                mPosX -= aSpeed;
            }
        }
    }

    // 主人本帧实际走了多少，狗就走多少。这里同步的是最终世界位移，
    // 而不是 mVelX 或动画速率，因此 _ground 曲线、冰冻倍率和动画相位
    // 都只由主人结算一次，不会产生累计误差。
    if (mZombieType == ZombieType::ZOMBIE_DOGWALKER) {
        Zombie *aDog = GetDogPartner();
        if (aDog != nullptr && !aDog->IsDeadOrDying() && aDog->mMindControlled == mMindControlled) {
            const float aDeltaX = mPosX - aOldPosX;
            aDog->mPosX += aDeltaX;
            aDog->mX = int(aDog->mPosX);
        }
    }
}

ZombiePhase Zombie::GetDancerPhase() {
    int aFrame = GetDancerFrame();

    return aFrame <= 11 ? ZombiePhase::PHASE_DANCER_DANCING_LEFT
        : aFrame <= 12  ? ZombiePhase::PHASE_DANCER_WALK_TO_RAISE
        : aFrame <= 15  ? ZombiePhase::PHASE_DANCER_RAISE_RIGHT_1
        : aFrame <= 18  ? ZombiePhase::PHASE_DANCER_RAISE_LEFT_1
        : aFrame <= 21  ? ZombiePhase::PHASE_DANCER_RAISE_RIGHT_2
                        : ZombiePhase::PHASE_DANCER_RAISE_LEFT_2;
}

ZombieID Zombie::SummonBackupDancer(int theRow, int thePosX) {
    ZombieType aZombieType = ZombieType::ZOMBIE_BACKUP_DANCER;
    if (mZombieType == ZombieType::ZOMBIE_JACKSON) {
        aZombieType = ZombieType::ZOMBIE_BACKUP_JACKSON;
    }

    if (!mBoard->RowCanHaveZombieType(theRow, aZombieType))
        return ZombieID::ZOMBIEID_NULL;

    if (thePosX < -80)
        return ZombieID::ZOMBIEID_NULL;

    Zombie *aZombie = mBoard->AddZombie_Origin(aZombieType, mFromWave, false);
    if (aZombie == nullptr)
        return ZombieID::ZOMBIEID_NULL;

    aZombie->mPosX = thePosX;
    aZombie->mPosY = GetPosYBasedOnRow(theRow);
    aZombie->SetRow(theRow);
    aZombie->mX = (int)aZombie->mPosX;
    aZombie->mY = (int)aZombie->mPosY;

    aZombie->mAltitude = ZOMBIE_BACKUP_DANCER_RISE_HEIGHT;
    aZombie->mZombiePhase = ZombiePhase::PHASE_DANCER_RISING;
    aZombie->mPhaseCounter = 150;
    aZombie->mRelatedZombieID = mBoard->ZombieGetID(this);

    aZombie->SetAnimRate(0.0f);
    aZombie->mMindControlled = mMindControlled;

    int aParticleX = (int)aZombie->mPosX + 60;
    int aParticleY = (int)aZombie->mPosY + 110;
    if (aZombie->IsOnHighGround()) {
        aParticleY -= HIGH_GROUND_HEIGHT;
    }
    int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, theRow, 0);
    mApp->AddTodParticle(aParticleX, aParticleY, aRenderOrder, ParticleEffect::PARTICLE_DANCER_RISE);
    mApp->PlayFoley(FoleyType::FOLEY_GRAVESTONE_RUMBLE);

    return mBoard->ZombieGetID(aZombie);
}

void Zombie::SummonBackupDancers() {
    if (IsRemoteClientOrViewer())
        return;

    SummonBackupDancers_Origin();

    if (IsRemoteServer()) {
        U16x5UNI32x5_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_SUMMON_BACKUP_DANCERS;
        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data2.f32 = mPosX;
        for (int i = 0; i < NUM_BACKUP_DANCERS; i++) {
            event.data3[i] = uint16_t(mFollowerZombieID[i]);
            Zombie *aZombie = mBoard->ZombieTryToGet(mFollowerZombieID[i]);
            if (aZombie) {
                event.data4[i].f32 = aZombie->mVelX;
            }
        }
        netplay::PutEvent(event);
    }
}

void Zombie::SummonBackupDancers_Origin() {
    if (!mHasHead)
        return;

    for (int i = 0; i < NUM_BACKUP_DANCERS; i++) {
        if (mBoard->ZombieTryToGet(mFollowerZombieID[i]) != nullptr)
            continue;

        int aRow = mRow;
        int aPosX = int(mPosX);
        switch (i) {
            case 0:
                --aRow;
                break;
            case 1:
                ++aRow;
                break;
            case 2:
                aPosX -= 100;
                break;
            case 3:
                aPosX += 100;
                break;
            default:
                break;
        }
        mFollowerZombieID[i] = SummonBackupDancer(aRow, aPosX);
    }
}

bool Zombie::NeedsMoreBackupDancers() {
    for (int i = 0; i < NUM_BACKUP_DANCERS; i++) {
        Zombie *aZombie = mBoard->ZombieTryToGet(mFollowerZombieID[i]);
        if (aZombie == nullptr) {
            if (i == 0 && !mBoard->RowCanHaveZombieType(mRow - 1, ZombieType::ZOMBIE_BACKUP_DANCER)) {
                continue;
            }

            if (i == 1 && !mBoard->RowCanHaveZombieType(mRow + 1, ZombieType::ZOMBIE_BACKUP_DANCER)) {
                continue;
            }

            if (i == 2 && mX < 100) {
                continue;
            }

            if (i == 3 && mMindControlled && mX > 700) {
                continue;
            }

            return true;
        }
    }

    return false;
}

void Zombie::DropLoot() {
    old_Zombie_DropLoot(this);
}

bool Zombie::HasYuckyFaceImage() {
    if (mBoard->mFutureMode)
        return false;

    return mZombieType == ZombieType::ZOMBIE_NORMAL || mZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE || mZombieType == ZombieType::ZOMBIE_PAIL || mZombieType == ZombieType::ZOMBIE_FLAG
        || mZombieType == ZombieType::ZOMBIE_DOOR || mZombieType == ZombieType::ZOMBIE_DUCKY_TUBE || mZombieType == ZombieType::ZOMBIE_DANCER || mZombieType == ZombieType::ZOMBIE_BACKUP_DANCER
        || mZombieType == ZombieType::ZOMBIE_NEWSPAPER || mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION;
}

void Zombie::UpdateYuckyFace() {
    if (mApp->IsVSMode() || IsRemoteClientOrViewer() || IsRemoteServer()) {
        mYuckyFaceCounter++;
        // 20 < counter < 170 且还没有 yucky face 图时：停止吃并直接跳到 170
        if (mYuckyFaceCounter > 20 && mYuckyFaceCounter < 170 && !HasYuckyFaceImage()) {
            StopEating();
            mYuckyFaceCounter = 170;
            int zc = mBoard->CountZombiesOnScreen();
            if ((zc <= 5 && mHasHead) || (zc <= 10 && mHasHead && Sexy::Rand(2) == 0)) {
                mApp->PlayFoley(FOLEY_YUCK);
            }
        }
        // counter > 270：结束 yucky
        if (mYuckyFaceCounter > 270) {
            ShowYuckyFace(false);
            mYuckyFace = false;
            mYuckyFaceCounter = 0;
            return;
        }
        // counter == 70：显示 yucky face 并可能播放音效
        if (mYuckyFaceCounter == 70) {
            StopEating();
            ShowYuckyFace(true);
            int zc = mBoard->CountZombiesOnScreen();
            if ((zc <= 5 && mHasHead) || (zc <= 10 && mHasHead && Sexy::Rand(2) == 0)) {
                mApp->PlayFoley(FOLEY_YUCK);
            }
        }
        // counter == 170：开始走路动画 + 尝试换行
        if (mYuckyFaceCounter == 170) {
            StartWalkAnim(20);
            bool isThisRowPool = (mBoard->mPlantRow[mRow] == PLANTROW_POOL); // 2
            // canGoDown: row-1；canGoUp: row+1
            bool canGoDown = true;
            bool canGoUp = true;
            // down
            if (!mBoard->RowCanHaveZombies(mRow - 1) ||                               //
                ((mBoard->mPlantRow[mRow - 1] == PLANTROW_POOL) && !isThisRowPool) || //
                ((mBoard->mPlantRow[mRow - 1] != PLANTROW_POOL) && isThisRowPool)) {
                canGoDown = false;
            }
            // up
            if (!mBoard->RowCanHaveZombies(mRow + 1) ||                               //
                ((mBoard->mPlantRow[mRow + 1] == PLANTROW_POOL) && !isThisRowPool) || //
                ((mBoard->mPlantRow[mRow + 1] != PLANTROW_POOL) && isThisRowPool)) {
                canGoUp = false;
            }
            // 客机不允许随机换行
            if (IsRemoteClientOrViewer()) {
                return;
            }
            if (canGoDown && !canGoUp) {
                SetRow(mRow - 1);
            }
            if (!canGoDown && canGoUp) {
                SetRow(mRow + 1);
            }
            if (canGoDown && canGoUp) {
                if (Sexy::Rand(2) == 0) {
                    SetRow(mRow + 1);
                } else {
                    SetRow(mRow - 1);
                }
            }

            if (IsRemoteServer()) {
                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_SET_ROW}, uint16_t(mBoard->mZombies.DataArrayGetID(this)), uint16_t(mRow)};
                netplay::PutEvent(event);
            }
        }
        return;
    }
    return old_Zombie_UpdateYuckyFace(this);
}

void Zombie::AnimateChewSound() {
    if (mZombiePhase == ZombiePhase::PHASE_SNORKEL_UP_TO_EAT) {
        return;
    }

    Plant *aPlant = FindPlantTarget(ZombieAttackType::ATTACKTYPE_CHEW);
    if (aPlant == nullptr) {
        mApp->PlayFoley(mMindControlled ? FoleyType::FOLEY_CHOMP_SOFT : FoleyType::FOLEY_CHOMP);
        return;
    }

    if (aPlant->mSeedType == SeedType::SEED_HYPNOSHROOM && !aPlant->mIsAsleep) {
        mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
        aPlant->Die();

        StartMindControlled();
        mApp->AddTodParticle(mPosX + 60.0f, mPosY + 40.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_MIND_CONTROL);
        TrySpawnLevelAward();

        mVelX = 0.17f;
        mAnimTicksPerFrame = 18;
        UpdateAnimSpeed();
        return;
    }

    if (aPlant->mSeedType == SeedType::SEED_GARLIC) {
        if (!mYuckyFace) {
            mYuckyFace = true;
            mYuckyFaceCounter = 0;
            UpdateAnimSpeed();
            mApp->PlayFoley(FoleyType::FOLEY_CHOMP);
        }
        return;
    }

    if (aPlant->mSeedType == SeedType::SEED_SUN_BEAN) {
        if (IsRemoteClientOrViewer()) {
            return;
        }

        mApp->PlaySample(SOUND_GULP);
        aPlant->Die();

        mSunBeanSun += 200;
        if (IsRemoteServer()) {
            U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_SUN_BEAN_SUN}, uint16_t(mBoard->mZombies.DataArrayGetID(this)), uint16_t(mSunBeanSun)};
            netplay::PutEvent(event);
        }
        return;
    }

    const bool aSoftPlant = aPlant->mSeedType == SeedType::SEED_WALLNUT || aPlant->mSeedType == SeedType::SEED_TALLNUT || aPlant->mSeedType == SeedType::SEED_PUMPKINSHELL;
    if (!aSoftPlant) {
        mApp->PlayFoley(FoleyType::FOLEY_CHOMP);
        return;
    }

    // 普僵坚果过敏
    if (!mBloated) {
        mApp->PlayFoley(FoleyType::FOLEY_CHOMP_SOFT);
    } else if (!mWalnutDeath) {
        mWalnutDeath = true;
        mApp->PlayFoley(FoleyType::FOLEY_CHOMP);
        PlayDeathAnim(0U);
    }
}

void Zombie::Animate() {
    mPrevFrame = mFrame;
    if (mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING || mZombiePhase == ZombiePhase::PHASE_NEWSPAPER_MADDENING || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISING
        || mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_RISE_WITHOUT_AXE || mZombiePhase == ZombiePhase::PHASE_DIGGER_STUNNED
        || IsImmobilizied()) {
        return;
    }

    mAnimCounter++;
    if (mYuckyFace) {
        UpdateYuckyFace();
    }

    if (mIsEating && mHasHead) {
        int aFrameLength = 6;
        if (mChilledCounter > 0) {
            aFrameLength = 12;
        }
        if (mAnimCounter >= mAnimFrames * aFrameLength) {
            mAnimCounter = aFrameLength;
        }
        mFrame = mAnimCounter / aFrameLength;

        Reanimation *aBodyReanim = mApp->ReanimationGet(mBodyReanimID);
        if (aBodyReanim) {
            float aLeftHandTime = 0.14f;
            float aRightHandTime = 0.68f;
            if (mZombieType == ZombieType::ZOMBIE_POLEVAULTER || mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
                aLeftHandTime = 0.38f;
                aRightHandTime = 0.8f;
            } else if (mZombieType == ZombieType::ZOMBIE_NEWSPAPER || mZombieType == ZombieType::ZOMBIE_LADDER || mZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION
                       || mZombieType == ZombieType::ZOMBIE_TELEPORTATION) {
                aLeftHandTime = aRightHandTime = 0.42f;
            } else if (mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX) {
                aLeftHandTime = aRightHandTime = 0.53f;
            } else if (mZombieType == ZombieType::ZOMBIE_BOBSLED) {
                aLeftHandTime = 0.33f;
                aRightHandTime = 0.83f;
            } else if (mZombieType == ZombieType::ZOMBIE_IMP || mZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP || mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
                aLeftHandTime = 0.33f;
                aRightHandTime = 0.79f;
            } else if (IsZomblob(mZombieType)) {
                aLeftHandTime = aRightHandTime = 0.38f;
            }

            if (aBodyReanim->ShouldTriggerTimedEvent(aLeftHandTime) || aBodyReanim->ShouldTriggerTimedEvent(aRightHandTime)) {
                AnimateChewSound();
                AnimateChewEffect();
            }
        } else {
            if (mAnimCounter == 4 * aFrameLength) {
                AnimateChewSound();
            }
            if (mAnimCounter == 7 * aFrameLength && !mMindControlled) {
                AnimateChewEffect();
            }
        }
    } else {
        if (mAnimCounter >= mAnimFrames * mAnimTicksPerFrame) {
            mAnimCounter = 0;
        }
        mFrame = mAnimCounter / mAnimTicksPerFrame;
    }
}

void Zombie::UpdateZombiePool() {
    switch (mZombieHeight) {
        case ZombieHeight::HEIGHT_OUT_OF_POOL: {
            mAltitude++;
            if (mZombieType == ZombieType::ZOMBIE_SNORKEL) {
                mAltitude++;
            }

            if (mAltitude >= 0.0f) {
                mAltitude = 0.0f;
                mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;
                mInPool = false;
            }
        } break;
        case ZombieHeight::HEIGHT_IN_TO_POOL: {
            mAltitude--;
            int aDepth = -40 * mScaleZombie;
            if (mZombieType == ZombieType::ZOMBIE_FOOTBALL) {
                aDepth = -50 * mScaleZombie;
            } else if (mZombieType == ZombieType::ZOMBIE_IMP) {
                aDepth = -30 * mScaleZombie;
            }
            if (mAltitude <= aDepth) {
                mAltitude = aDepth;
                mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;
                StartWalkAnim(0);
            }
        } break;
        case ZombieHeight::HEIGHT_DRAGGED_UNDER:
            mAltitude--;
            break;
        default:
            break;
    }
}

void Zombie::DoSpecial() {
    if (mApp->mGameScene == SCENE_PLAYING) {
        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_DO_SPECIAL}, uint16_t(mBoard->mZombies.DataArrayGetID(this))};
            netplay::PutEvent(event);
        }
    }

    if (mZombieType == ZOMBIE_JACK_IN_THE_BOX || mZombieType == ZOMBIE_JALAPENO_HEAD || mZombieType == ZOMBIE_SUPER_FAN_IMP || mZombieType == ZOMBIE_GIGA_IMP) {
        // 自爆不结算阳光豆储存的阳光
        mSunBeanSun = 0;
        mSunBeanDamageRemainder = 0;
    }

    switch (mZombieType) {
        case ZombieType::ZOMBIE_JACK_IN_THE_BOX: {
            mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);

            int aPosX = mX + mWidth / 2;
            int aPosY = mY + mHeight / 2;
            if (mMindControlled) {
                mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, JackInTheBoxZombieRadius, 1, true, 127);
            } else {
                mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, JackInTheBoxZombieRadius, 1, true, 255);
                mBoard->KillAllPlantsInRadius(aPosX, aPosY, JackInTheBoxPlantRadius);
            }

            mApp->AddTodParticle(aPosX, aPosY, Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 0), ParticleEffect::PARTICLE_JACKEXPLODE);
            mBoard->ShakeBoard(4, -6);
            DieNoLoot();

            if (mApp->IsScaryPotterLevel()) {
                mBoard->mChallenge->ScaryPotterJackExplode(aPosX, aPosY);
            }
            break;
        }

        case ZombieType::ZOMBIE_JALAPENO_HEAD: {
            mApp->PlayFoley(FoleyType::FOLEY_JALAPENO_IGNITE);
            mApp->PlayFoley(FoleyType::FOLEY_JUICY);
            mBoard->DoFwoosh(mRow);
            mBoard->ShakeBoard(3, -4);

            if (mMindControlled) { // 修复辣椒僵尸被魅惑后爆炸依然伤害植物的BUG
                BurnRow(mRow);
            } else {
                Plant *aPlant = nullptr;
                while (mBoard->IteratePlants(aPlant)) {
                    if (aPlant->mRow == mRow && !aPlant->NotOnGround()) {
                        mBoard->mPlantsEaten++;
                        aPlant->Die();
                    }
                }

                if (mApp->IsVSMode()) {
                    BurnRow(mRow);
                }
            }

            DieNoLoot();
            break;
        }

        case ZombieType::ZOMBIE_SUPER_FAN_IMP:
        case ZombieType::ZOMBIE_GIGA_IMP: {
            mApp->PlayFoley(FoleyType::FOLEY_EXPLOSION);
            mBoard->ShakeBoard(4, -6);

            int aPosX = mX + mWidth / 2;
            int aPosY = mY + mHeight / 2;
            int aGridX = mBoard->PixelToGridXKeepOnBoard(aPosX, aPosY);
            int aGridY = mBoard->PixelToGridYKeepOnBoard(aPosX, aPosY);
            if (mMindControlled) {
                mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, SuperFanImpZombieRadius, 1, true, 127);
            } else {
                int aDamage = SUPER_FAN_IMP_POP_DAMAGE;
                if (mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
                    aDamage = GIGA_IMP_POP_DAMAGE;
                    TodParticleSystem *aParticle = mApp->AddTodParticle(aPosX, aPosY, Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 0), ParticleEffect::PARTICLE_JACKEXPLODE);
                    if (aParticle != nullptr) {
                        aParticle->OverrideScale(nullptr, 0.5f);
                    }
                }
                mBoard->KillAllZombiesInRadius_Custom(mRow, aPosX, aPosY, SuperFanImpZombieRadius, 1, true, 255);
                mBoard->PlantsTakeDamageInGrid(aGridX, aGridY, aDamage);
            }
            break;
        }

        default:
            break;
    }
}

void Zombie::BungeeLanding() {
    if (mZombiePhase == ZombiePhase::PHASE_BUNGEE_DIVING && mAltitude < 1500.0f && !mApp->IsFinalBossLevel()) {
        mApp->PlayFoley(FoleyType::FOLEY_BUNGEE_SCREAM);
        mZombiePhase = ZombiePhase::PHASE_BUNGEE_DIVING_SCREAMING;
    }

    if (mAltitude > 40.0f)
        return;

    Plant *aPlant = mBoard->FindUmbrellaPlant(mTargetCol, mRow);
    if (aPlant) {
        //        if (mApp->IsVSMode()) {
        //            if (gTcpConnected) {
        //                // Client waits for host-authoritative umbrella result; freeze descent to avoid falling out of map during high latency.
        //                if (mAltitude < 0.0f) {
        //                    mAltitude = 0.0f;
        //                }
        //                return;
        //            }
        //
        //            if (IsRemoteServer()) {
        //                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_HIT_UMBRELLA}, uint16_t(mBoard->mZombies.DataArrayGetID(this)),
        //                uint16_t(mBoard->mPlants.DataArrayGetID(aPlant))}; netplay::PutEvent(event);
        //            }
        //        }

        mApp->PlaySample(SOUND_BOING);
        mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);

        aPlant->DoSpecial();

        mZombiePhase = ZombiePhase::PHASE_BUNGEE_RISING;
        mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
        mHitUmbrella = true;

        return;
    }

    //    伪C代码，无用法故注释
    //    mBoard->GetTopPlantAt(mTargetCol, mRow, PlantPriority::TOPPLANT_BUNGEE_ORDER);

    if (mAltitude > 0.0f)
        return;

    mAltitude = 0.0f;
    Zombie *aZombie = mBoard->ZombieTryToGet(mRelatedZombieID);
    if (aZombie) // 存在关联的僵尸时，释放空投的僵尸
    {
        aZombie->mZombieHeight = ZombieHeight::HEIGHT_ZOMBIE_NORMAL;
        aZombie->StartWalkAnim(0);

        mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
        mZombiePhase = ZombiePhase::PHASE_BUNGEE_RISING;
        PlayZombieReanim("anim_raise", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 36.0f);
    } else // 不存在关联的僵尸时，开始偷取植物
    {
        mZombiePhase = ZombiePhase::PHASE_BUNGEE_AT_BOTTOM;
        mPhaseCounter = 300;
        PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 5, 24.0f);
        mApp->ReanimationGet(mBodyReanimID)->mAnimTime = 0.5f;
    }
}

void Zombie::UpdateZombieBungee() {
    if (IsDeadOrDying() || IsImmobilizied()) {
        return;
    }

    if (mZombiePhase == ZombiePhase::PHASE_BUNGEE_DIVING || mZombiePhase == ZombiePhase::PHASE_BUNGEE_DIVING_SCREAMING) {
        float oldAltitude = mAltitude;
        mAltitude -= 8.0f;
        if (mAltitude <= 2596.0f && oldAltitude > 2596.0f && mRelatedZombieID == ZombieID::ZOMBIEID_NULL) {
            mApp->PlayFoley(FoleyType::FOLEY_GRASSSTEP);
        }
        BungeeLanding();
    } else {
        switch (mZombiePhase) {
            case ZombiePhase::PHASE_BUNGEE_AT_BOTTOM:
                if (mPhaseCounter <= 0) {
                    BungeeStealTarget();
                    mZombiePhase = ZombiePhase::PHASE_BUNGEE_GRABBING;
                }
                break;

            case ZombiePhase::PHASE_BUNGEE_GRABBING:
                if (mApp->ReanimationGet(mBodyReanimID)->mLoopCount > 0) {
                    if (IsRemoteClientOrViewer()) {
                        return;
                    }

                    BungeeLiftTarget();
                    mZombiePhase = ZombiePhase::PHASE_BUNGEE_RISING;

                    if (IsRemoteServer()) {
                        U16U16_Event event{};
                        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_LIFT_TARGET;
                        event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                        event.data2 = mTargetPlantID == PLANTID_NULL ? NETPLAY_PLANT_ID_NULL : uint16_t(mTargetPlantID);
                        netplay::PutEvent(event);
                    }
                }
                break;

            case ZombiePhase::PHASE_BUNGEE_HIT_OUCHY:
                if (mPhaseCounter <= 0) {
                    DieWithLoot();
                }
                break;

            case ZombiePhase::PHASE_BUNGEE_RISING:
                mAltitude += 8.0f;
                if (mAltitude >= 600.0f) {
                    DieNoLoot();
                }
                break;

            case ZombiePhase::PHASE_BUNGEE_CUTSCENE:
                if (mPhaseCounter <= 0) {
                    mPhaseCounter = 200;
                }
                mAltitude = static_cast<float>(TodAnimateCurve(200, 0, mPhaseCounter, 40, 0, TodCurves::CURVE_SIN_WAVE));
                break;

            default:
                break;
        }
    }

    mX = int(mPosX);
    mY = int(mPosY);
}

Plant *Zombie::FindCatapultTarget() {
    Plant *aTarget = nullptr;

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow == mRow && mX >= aPlant->mX + 100 && !aPlant->NotOnGround() && !aPlant->IsSpiky() && !aPlant->IsCeleryStalkerLow()) {
            if (aTarget == nullptr || aPlant->mPlantCol < aTarget->mPlantCol) {
                aTarget = mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_CATAPULT_ORDER);
            }
        }
    }

    return aTarget;
}

void Zombie::UpdateZombieCatapult() {
    auto syncCatapultPhase = [this](ZombiePhase phase, int phaseCounter, int summonCounter) {
        if (!IsRemoteServer()) {
            return;
        }
        U8U8U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
        event.data1 = uint8_t(phase);
        event.data2 = uint8_t(summonCounter);
        event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data4 = uint16_t(phaseCounter);
        netplay::PutEvent(event);
    };

    if (mZombiePhase == PHASE_CATAPULT_LAUNCHING) {
        if (IsRemoteClientOrViewer()) {
            return;
        }

        Reanimation *reanimation = mApp->ReanimationGet(mBodyReanimID);
        if (reanimation->ShouldTriggerTimedEvent(0.545f)) {
            Plant *thePlant = FindCatapultTarget();
            if (IsRemoteServer()) {
                U16U16_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CATAPLUT_FIRE;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2 = thePlant == nullptr ? NETPLAY_PLANT_ID_NULL : uint16_t(mBoard->mPlants.DataArrayGetID(thePlant));
                netplay::PutEvent(event);
            }
            ZombieCatapultFire(thePlant);
        }
        if (reanimation->mLoopCount > 0) {
            mSummonCounter--;
            if (mSummonCounter == 4) {
                ReanimShowTrack("Zombie_catapult_basketball", -1);
            } else if (mSummonCounter == 3) {
                ReanimShowTrack("Zombie_catapult_basketball2", -1);
            } else if (mSummonCounter == 2) {
                ReanimShowTrack("Zombie_catapult_basketball3", -1);
            } else if (mSummonCounter == 1) {
                ReanimShowTrack("Zombie_catapult_basketball4", -1);
            }
            if (mSummonCounter == 0) {
                PlayZombieReanim("anim_walk", REANIM_LOOP, 20, 6.0f);
                mZombiePhase = PHASE_ZOMBIE_NORMAL;
                syncCatapultPhase(PHASE_ZOMBIE_NORMAL, mPhaseCounter, mSummonCounter);
                return;
            }
            PlayZombieReanim("anim_idle", REANIM_LOOP, 20, 12.0f);
            mZombiePhase = PHASE_CATAPULT_RELOADING;
            syncCatapultPhase(PHASE_CATAPULT_RELOADING, mPhaseCounter, mSummonCounter);
            return;
        }
        return;
    }

    if (IsRemoteClientOrViewer()) {
        return;
    }

    if (mZombiePhase == PHASE_ZOMBIE_NORMAL) {
        if (mPosX <= 650 && FindCatapultTarget() != nullptr && mSummonCounter > 0) {
            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CATAPLUT_LAUNCHIING;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.f32 = mPosX;
                netplay::PutEvent(event);
            }
            mZombiePhase = PHASE_CATAPULT_LAUNCHING;
            mPhaseCounter = 300;
            PlayZombieReanim("anim_shoot", REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            syncCatapultPhase(PHASE_CATAPULT_LAUNCHING, mPhaseCounter, mSummonCounter);
            return;
        }
    } else if (mZombiePhase == PHASE_CATAPULT_RELOADING && mPhaseCounter == 0) {
        Plant *plant = FindCatapultTarget();
        if (plant != nullptr) {
            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_CATAPLUT_LAUNCHIING;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.f32 = mPosX;
                netplay::PutEvent(event);
            }
            mZombiePhase = PHASE_CATAPULT_LAUNCHING;
            mPhaseCounter = 300;
            PlayZombieReanim("anim_shoot", REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            syncCatapultPhase(PHASE_CATAPULT_LAUNCHING, mPhaseCounter, mSummonCounter);
            return;
        }
        PlayZombieReanim("anim_walk", REANIM_LOOP, 20, 6.0f);
        mZombiePhase = PHASE_ZOMBIE_NORMAL;
        syncCatapultPhase(PHASE_ZOMBIE_NORMAL, mPhaseCounter, mSummonCounter);
    }
}

void Zombie::UpdateLadder() {
    if (mMindControlled || !mHasHead || IsDeadOrDying()) {
        return;
    }
    auto syncLadderPhase = [this](ZombiePhase phase, int phaseCounter, int summonCounter) {
        if (!IsRemoteServer()) {
            return;
        }

        U8U8U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER;
        event.data1 = uint8_t(phase);
        event.data2 = uint8_t(summonCounter);
        event.data3 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
        event.data4 = uint16_t(phaseCounter);
        netplay::PutEvent(event);
    };

    if (mZombiePhase == PHASE_LADDER_CARRYING && mZombieHeight == HEIGHT_ZOMBIE_NORMAL) {
        Plant *plant = FindPlantTarget(ATTACKTYPE_LADDER);
        if (plant != nullptr) {
            if (IsRemoteClientOrViewer()) {
                return;
            }

            if (IsRemoteServer()) {
                U16UNI32_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_LADDER_START_PLACING;
                event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                event.data2.f32 = mPosX;
                netplay::PutEvent(event);
            }

            StopEating();
            mZombiePhase = PHASE_LADDER_PLACING;
            PlayZombieReanim("anim_placeladder", REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
            return;
        }
    } else if (mZombiePhase == PHASE_LADDER_PLACING) {
        Reanimation *reanimation = mApp->ReanimationTryToGet(mBodyReanimID);
        if (reanimation != nullptr && reanimation->mLoopCount > 0) {
            if (IsRemoteClientOrViewer() && mShieldType == SHIELDTYPE_LADDER) {
                return;
            }

            Plant *plant2 = FindPlantTarget(ATTACKTYPE_LADDER);
            if (plant2 != nullptr) {
                if (IsRemoteServer()) {
                    U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_LADDER_PLACED;
                    event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(this));
                    event.data2 = plant2->mPlantCol;
                    netplay::PutEvent(event);
                }

                mBoard->AddALadder(plant2->mPlantCol, plant2->mRow);
                mApp->PlaySample(SOUND_LADDER_ZOMBIE);
                mZombieHeight = HEIGHT_UP_LADDER;
                mUseLadderCol = plant2->mPlantCol;
                DetachShield();
                return;
            }
            mZombiePhase = PHASE_LADDER_CARRYING;
            StartWalkAnim(0);
            syncLadderPhase(PHASE_LADDER_CARRYING, mPhaseCounter, mSummonCounter);
        }
    }
}

void Zombie::ZomblobSplit() {
    if (mButtered) {
        return;
    }

    ZombieType aNextZomblobType = mZombieType;
    bool aShouldSplit = false;
    if (mZombieType == ZombieType::ZOMBIE_ZOMBLOB) {
        aNextZomblobType = ZombieType::ZOMBIE_ZOMBLOB_MIDDLE;
        aShouldSplit = true;
    } else if (mZombieType == ZombieType::ZOMBIE_ZOMBLOB_MIDDLE) {
        aNextZomblobType = ZombieType::ZOMBIE_ZOMBLOB_SMALL;
        aShouldSplit = true;
    }

    // 最小阶段不再分裂
    if (!aShouldSplit) {
        StopZombieSound();
        DieWithLoot();
        return;
    }

    if (mBoard) {
        const int aLastRow = mBoard->StageHas6Rows() ? 5 : 4;
        const int aFirstTargetRow = std::max(0, mRow - 1);
        const int aSecondTargetRow = std::min(aLastRow, mRow + 1);
        const int aGridX = mBoard->PixelToGridXKeepOnBoard(int(mPosX + float(mWidth) * 0.5f), int(mPosY + float(mHeight) * 0.5f));

        constexpr float aFlightFrames = 90.0f;
        constexpr float aGravity = 0.1f;
        constexpr float aHorizontalOffset = 18.0f;
        constexpr float aHorizontalSpeed = aHorizontalOffset / aFlightFrames;
        const auto aSourceGroundY = float(mBoard->GridToPixelY(aGridX, mRow) + 67);

        auto LaunchZomblob = [&](int theTargetRow, float theVelX, float theRotationSpeed) {
            const auto aTargetGroundY = float(mBoard->GridToPixelY(aGridX, theTargetRow) + 67);
            const float aOriginX = mPosX + float(mWidth) * 0.5f - 20.0f;
            const float aOriginY = aSourceGroundY - 40.0f;

            Projectile *aProjectile = mBoard->AddProjectile(int(aOriginX), int(aOriginY), mRenderOrder + 1, mRow, ProjectileType::PROJECTILE_ZOMBLOB);
            if (!aProjectile)
                return;

            aProjectile->mMotionType = ProjectileMotion::MOTION_LOBBED;
            aProjectile->mVelX = theVelX;
            aProjectile->mVelY = (aTargetGroundY - aSourceGroundY) / aFlightFrames;
            aProjectile->mVelZ = -aGravity * aFlightFrames * 0.5f;
            aProjectile->mAccZ = aGravity;
            aProjectile->mShadowY = aSourceGroundY;
            aProjectile->mRotationSpeed = theRotationSpeed;

            // 复用投掷物中本类型不使用的字段，避免修改 Projectile 对象布局。
            aProjectile->mCobTargetRow = theTargetRow;
            aProjectile->mCobTargetX = aOriginX;
            aProjectile->mHitTorchwoodGridX = int(aNextZomblobType);
            aProjectile->mDamageRangeFlags = mFromWave;
            aProjectile->mLastPortalX = mMindControlled ? 1 : 0;
        };

        LaunchZomblob(aFirstTargetRow, -aHorizontalSpeed, -0.05f);
        LaunchZomblob(aSecondTargetRow, aHorizontalSpeed, 0.05f);

        StopZombieSound();
        mApp->PlayFoley(FoleyType::FOLEY_ZOMBLOB);
        DieNoLoot();
    }
}

void Zombie::SetupButteredZomblobReanim() {
    Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
    if (aBodyReanim == nullptr) {
        return;
    }

    auto SetOverrideIfPresent = [aBodyReanim](const char *theTrackName, Sexy::Image *theImage) {
        if (theImage && aBodyReanim->TrackExists(theTrackName)) {
            aBodyReanim->SetImageOverride(theTrackName, theImage);
        }
    };

    switch (mZombieType) {
        case ZombieType::ZOMBIE_ZOMBLOB:
            SetOverrideIfPresent("Zombie_death", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_DYING_BUTTERED);
            SetOverrideIfPresent("anim_head1", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_HEAD_BUTTERED);
            SetOverrideIfPresent("anim_head2", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_JAW_BUTTERED);
            SetOverrideIfPresent("Zombie_body", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_BUTTERED);
            SetOverrideIfPresent("Zombie_outerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_UPPER_BUTTERED);
            SetOverrideIfPresent("Zombie_outerarm_lower", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_LOWER_BUTTERED);
            SetOverrideIfPresent("Zombie_outerarm_hand", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_HAND_BUTTERED);
            SetOverrideIfPresent("Zombie_outerarm_hand2", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_HAND2_BUTTERED);
            SetOverrideIfPresent("Zombie_outerleg_upper", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_UPPER_BUTTERED);
            SetOverrideIfPresent("Zombie_outerleg_lower", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_LOWER_BUTTERED);
            SetOverrideIfPresent("Zombie_outerleg_foot", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_FOOT_BUTTERED);
            SetOverrideIfPresent("Zombie_innerleg_upper", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_UPPER_BUTTERED);
            SetOverrideIfPresent("Zombie_innerleg_lower", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_LOWER_BUTTERED);
            SetOverrideIfPresent("Zombie_innerleg_foot", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_FOOT_BUTTERED);
            SetOverrideIfPresent("anim_innerarm1", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_UPPER_BUTTERED);
            SetOverrideIfPresent("anim_innerarm2", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER_BUTTERED);
            SetOverrideIfPresent("anim_innerarm3", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND_BUTTERED);
            break;

        case ZombieType::ZOMBIE_ZOMBLOB_MIDDLE:
            SetOverrideIfPresent("Zombie_death", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_DYING_BUTTERED);
            SetOverrideIfPresent("Zombie_body", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY2_BUTTERED);
            SetOverrideIfPresent("anim_head1", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_HEAD2_BUTTERED);
            SetOverrideIfPresent("anim_head2", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_JAW_BUTTERED);
            SetOverrideIfPresent("zombie_innerarm_hand", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND2_BUTTERED);
            SetOverrideIfPresent("zombie_innerarm_upper", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_UPPER2_BUTTERED);
            SetOverrideIfPresent("zombie_innerarm_lower", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER2_BUTTERED);
            break;

        case ZombieType::ZOMBIE_ZOMBLOB_SMALL:
            SetOverrideIfPresent("Zombie_death", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_DYING_BUTTERED);
            SetOverrideIfPresent("Zombie_body", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY3_BUTTERED);
            SetOverrideIfPresent("zombie_innerarm_hand", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND3_BUTTERED);
            SetOverrideIfPresent("zombie_innerarm_lower", addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER3_BUTTERED);
            break;

        default:
            break;
    }
}

void Zombie::WalkIntoHouse() {
    AttachmentDetachCrossFadeParticleType(mAttachmentID, ParticleEffect::PARTICLE_ZAMBONI_SMOKE, nullptr);
    mFromWave = Zombie::ZOMBIE_WAVE_WINNER;
    ReanimReenableClipping();

    if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT) {
        mZombiePhase = ZombiePhase::PHASE_POLEVAULTER_POST_VAULT;
        StartWalkAnim(0);
    }

    if (mBoard->mBackground == BackgroundType::BACKGROUND_1_DAY || mBoard->mBackground == BackgroundType::BACKGROUND_2_NIGHT || mBoard->mBackground == BackgroundType::BACKGROUND_3_POOL
        || mBoard->mBackground == BackgroundType::BACKGROUND_4_FOG) {
        mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_ZOMBIE, 2, 100);

        // 目标二进制中这个判断实际已不可达，严格还原则保留。
        if (mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT) {
            mPosX += 35.0f;
        }

        if (mBoard->StageHasPool()) {
            if (mZombieType == ZombieType::ZOMBIE_FOOTBALL || mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
                mPosX -= 10.0f;
            } else {
                mPosX -= 80.0f;
            }
        }
    } else if (mBoard->mBackground == BackgroundType::BACKGROUND_5_ROOF || mBoard->mBackground == BackgroundType::BACKGROUND_6_BOSS) {
        mPosX = -180.0f;
        mPosY = 250.0f;
        mZombieHeight = ZombieHeight::HEIGHT_IN_TO_CHIMNEY;
        mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, 0, 2);

        if (IsGargantuar()) {
            mPosY += 5.0f;
        } else if (mZombieType == ZombieType::ZOMBIE_FOOTBALL || mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
            mPosX -= 14.0f;
        } else if (mZombieType == ZombieType::ZOMBIE_ZAMBONI) {
            mPosX -= 28.0f;
        }

        Reanimation *aBodyReanim = mApp->ReanimationTryToGet(mBodyReanimID);
        if (aBodyReanim && aBodyReanim->TrackExists("anim_idle") && mZombieType != ZombieType::ZOMBIE_POLEVAULTER && mZombieType != ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
            PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
        }
    }
}

bool Zombie::IsChangingRow() {
    const float aTargetY = GetPosYBasedOnRow(mRow);
    return std::fabs(mPosY - aTargetY) > 1.0f;
}
