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

#include "PvZ/Lawn/Board/LawnMower.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/NetPlay.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodFoley.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

using namespace Sexy;

void LawnMower::Update() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        return;
    }
    if (mMowerState == LawnMowerState::MOWER_SQUISHED) {
        if (--mSquishedCounter <= 0) {
            Die();
        }
        return;
    }
    if (mMowerState == LawnMowerState::MOWER_ROLLING_IN) {
        ++mRollingInCounter;
        mPosX = TodAnimateCurveFloat(0, 100, mRollingInCounter, -160.0f, -21.0f, TodCurves::CURVE_EASE_IN_OUT);
        if (mRollingInCounter == 100) {
            mMowerState = LawnMowerState::MOWER_READY;
        }
        return;
    }
    if (mApp->mGameScene != GameScenes::SCENE_PLANTS_WON && mApp->mGameScene != GameScenes::SCENE_PLAYING && !mBoard->mCutScene->ShouldRunUpsellBoard()) {
        return;
    }

    const Rect aAttackRect(int(mPosX), int(mPosY), 50, 80);
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow != mRow || aZombie->mZombiePhase == ZombiePhase::PHASE_ZOMBIE_MOWERED || aZombie->IsTangleKelpTarget()
            || !aZombie->EffectedByDamage(127U)) {
            continue;
        }

        const int aOverlap = GetRectOverlap(aAttackRect, aZombie->GetZombieRect());
        const int aMinOverlap = aZombie->mZombieType == ZombieType::ZOMBIE_BALLOON ? 20 : 0;
        const bool aHeadOn = aZombie->mZombieType != ZombieType::ZOMBIE_BUNGEE && aZombie->mHasHead;
        if (aOverlap > aMinOverlap && (mMowerState != LawnMowerState::MOWER_READY || aHeadOn)) {
            MowZombie(aZombie);
        }
    }

    GridItem *aPile = nullptr;
    while (mBoard->IterateGridItems(aPile)) {
        if (aPile->IsSkeletonPile() && aPile->mGridY == mRow && aPile->mSummonIndex == 0 && GetRectOverlap(aAttackRect, aPile->GetItemRect()) > 0) {
            aPile->GridItemDie();
        }
    }

    if (mMowerState != LawnMowerState::MOWER_TRIGGERED && mMowerState != LawnMowerState::MOWER_SQUISHED) {
        return;
    }

    float aSpeed = mMowerType == LawnMowerType::LAWNMOWER_POOL ? 2.5f : 3.33f;
    if (mChompCounter > 0) {
        --mChompCounter;
        aSpeed = TodAnimateCurveFloat(50, 0, mChompCounter, aSpeed, 1.0f, TodCurves::CURVE_BOUNCE_SLOW_MIDDLE);
    }
    mPosX += aSpeed;
    mPosY = mBoard->GetPosYBasedOnRow(mPosX + 40.0f, mRow) + 23.0f;

    if (mMowerType == LawnMowerType::LAWNMOWER_POOL) {
        UpdatePool();
    }
    if (mMowerType == LawnMowerType::LAWNMOWER_LAWN && mBoard->mPlantRow[mRow] == PlantRowType::PLANTROW_POOL && mPosX > 50.0f) {
        Reanimation *aSplashReanim = mApp->AddReanimation(mPosX, mPosY + 25.0f, mRenderOrder + 1, ReanimationType::REANIM_SPLASH);
        aSplashReanim->OverrideScale(1.2f, 0.8f);
        mApp->AddTodParticle(mPosX + 50.0f, mPosY + 67.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PLANTING_POOL);
        mApp->PlaySample(Sexy::SOUND_ZOMBIE_ENTERING_WATER);
        mApp->mSoundSystem->StopFoley(FoleyType::FOLEY_LAWNMOWER);
        Die();
        return;
    }
    if (mPosX > 800.0f) {
        Die();
        return;
    }
    mApp->ReanimationGet(mReanimID)->Update();
}

void LawnMower::SquishMower() {
    if (IsRemoteClientOrViewer())
        return;

    if (mApp->mGameScene == SCENE_PLAYING && IsRemoteServer()) {
        U16_Event event = {{EventType::EVENT_SERVER_BOARD_LAWNMOWER_SQUISH}, uint16_t(mRow)};
        netplay::PutEvent(event);
    }

    SquishMower_Origin();
}

void LawnMower::SquishMower_Origin() {
    Reanimation *aMowerReanim = mApp->ReanimationGet(mReanimID);
    aMowerReanim->OverrideScale(0.85f, 0.22f);
    aMowerReanim->SetPosition(-11.0f, 65.0f);
    mMowerState = LawnMowerState::MOWER_SQUISHED;
    mSquishedCounter = 500;
    mApp->PlayFoley(FoleyType::FOLEY_SQUISH);
}

void LawnMower::StartMower() {
    if (IsRemoteClientOrViewer())
        return;

    if (mApp->mGameScene == SCENE_PLAYING) {
        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_BOARD_LAWNMOWER_START}, uint16_t(mRow)};
            netplay::PutEvent(event);
            // 小推车战损
            netplay::MetricsRecordMowerLoss();
        }
    }

    old_LawnMower_StartMower(this);
}
