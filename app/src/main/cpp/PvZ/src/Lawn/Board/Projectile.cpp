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

#include "PvZ/Lawn/Board/Projectile.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/Misc.h"
#include "PvZ/NetPlay.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <algorithm>
#include <limits>
#include <numbers>

using namespace Sexy;

namespace {
constexpr int SPIKE_PIERCE_DAMAGE[MAX_PIERCE_HIT_COUNT] = {30, 15, 10};
constexpr int TRAFFIC_CONE_FLIGHT_TICKS = 60;
constexpr int ACKEE_MAX_HITS = 4;
constexpr int ACKEE_BOUNCE_GRID_RANGE = 3;
constexpr float ACKEE_BOUNCE_VELOCITY_Z = -6.0f;
constexpr float ACKEE_BOUNCE_ACCELERATION_Z = 0.115f;
constexpr int ACKEE_DAMAGE[ACKEE_MAX_HITS] = {60, 50, 40, 30};

int GetProjectileImpactDamage(const Projectile *theProjectile) {
    int aDamage = theProjectile->GetProjectileDef().mDamage;
    if (theProjectile->mProjectileType == ProjectileType::PROJECTILE_RED_STINGER && theProjectile->mMotionType == ProjectileMotion::MOTION_STRAIGHT) {
        aDamage -= 10; // 红针花一形态的直射子弹减少 10 点伤害
    }
    return aDamage;
}

ProjectileType ResolveInitialProjectileType(ProjectileType theProjectileType) {
    if (isOnlyTouchFireWood || IsOnlineServerModeActive() || gIsReplayMode) {
        return theProjectileType;
    }

    // 这些子弹承载独立技能，不参与出生时的子弹替换。
    switch (theProjectileType) {
        case ProjectileType::PROJECTILE_COBBIG:
        case ProjectileType::PROJECTILE_ZOMBIE_PEA:
        case ProjectileType::PROJECTILE_ZOMBLOB:
        case ProjectileType::PROJECTILE_BOOMERANG:
        case ProjectileType::PROJECTILE_TELEPORTATION:
        case ProjectileType::PROJECTILE_TRAFFIC_CONE:
            return theProjectileType;
        default:
            break;
    }

    if ((theProjectileType == ProjectileType::PROJECTILE_STAR && banStar) || (isOnlyPeaUseable && theProjectileType != ProjectileType::PROJECTILE_PEA)) {
        return theProjectileType;
    }

    // 保持原优先级：随机子弹覆盖手动选择，但过滤条件始终判断原始类型。
    if (randomBullet) {
        const int aMaxType = ProjectileType::PROJECTILE_ZOMBIE_PEA - 1 - banCobCannon;
        int aType = RandRangeInt(ProjectileType::PROJECTILE_SNOWPEA, aMaxType);
        if (banCobCannon && aType == ProjectileType::PROJECTILE_COBBIG) {
            ++aType;
        }
        return static_cast<ProjectileType>(aType);
    }
    return bulletSpinnerChosenNum == -1 ? theProjectileType : static_cast<ProjectileType>(bulletSpinnerChosenNum);
}

bool IsPiercingSpike(const Projectile *theProjectile) {
    return theProjectile->mApp->IsVSMode() && (VSSetupAddonWidget::msBalancePatchMode || Challenge::msVSShuffleMode) && theProjectile->mProjectileType == ProjectileType::PROJECTILE_SPIKE;
}

int FindHitZombieSlot(const Projectile *theProjectile, Zombie *theZombie) {
    if (theZombie == nullptr) {
        return -1;
    }

    const ZombieID aZombieID = theProjectile->mBoard->ZombieGetID(theZombie);
    for (int i = 0; i < theProjectile->mPierceHitCount; ++i) {
        if (theProjectile->mHitZombieIDs[i] == aZombieID) {
            return i;
        }
    }
    return -1;
}

bool HasHitZombie(const Projectile *theProjectile, Zombie *theZombie) {
    return FindHitZombieSlot(theProjectile, theZombie) >= 0;
}

int FindHitGridItemSlot(const Projectile *theProjectile, GridItem *theGridItem) {
    if (theGridItem == nullptr) {
        return -1;
    }

    const GridItemID aGridItemID = theProjectile->mBoard->GridItemGetID(theGridItem);
    for (int i = 0; i < theProjectile->mPierceHitCount; ++i) {
        if (theProjectile->mHitGridItemIDs[i] == aGridItemID) {
            return i;
        }
    }
    return -1;
}

bool HasHitGridItem(const Projectile *theProjectile, GridItem *theGridItem) {
    return FindHitGridItemSlot(theProjectile, theGridItem) >= 0;
}

void ResetHitHistory(Projectile *theProjectile) {
    theProjectile->mPierceHitCount = 0;
    for (ZombieID &aZombieID : theProjectile->mHitZombieIDs) {
        aZombieID = ZombieID::ZOMBIEID_NULL;
    }
    for (GridItemID &aGridItemID : theProjectile->mHitGridItemIDs) {
        aGridItemID = GridItemID::GRIDITEMID_NULL;
    }
}

bool IsAckeeGridTarget(const GridItem *theGridItem) {
    return theGridItem != nullptr && theGridItem->IsPlantAttackTarget();
}

bool IsAckeeGraveTarget(const GridItem *theGridItem) {
    return theGridItem != nullptr && theGridItem->IsPlantAttackTarget()
        && (theGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || theGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND || theGridItem->IsSkeletonPile());
}

struct AckeeTarget {
    Zombie *mZombie = nullptr;
    GridItem *mGridItem = nullptr;
    float mDistanceSquared = std::numeric_limits<float>::max();
};

AckeeTarget FindAckeeBounceTarget(Projectile *theProjectile, float theSourceTargetX, int theSourceGridX, int theSourceRow) {
    AckeeTarget aBestTarget;

    Zombie *aZombie = nullptr;
    while (theProjectile->mBoard->IterateZombies(aZombie)) {
        if (aZombie->mRow != theSourceRow || aZombie->IsDeadOrDying() || !aZombie->EffectedByDamage(theProjectile->mDamageRangeFlags) || HasHitZombie(theProjectile, aZombie)) {
            continue;
        }

        const float aDeltaX = aZombie->mPosX - theSourceTargetX;
        const int aTargetGridX = theProjectile->mBoard->PixelToGridXKeepOnBoard(int(aZombie->mPosX), int(aZombie->mPosY));
        if (aDeltaX <= 0.0f || aTargetGridX < theSourceGridX || aTargetGridX > theSourceGridX + ACKEE_BOUNCE_GRID_RANGE) {
            continue;
        }

        const float aDistanceSquared = aDeltaX * aDeltaX;
        if (aDistanceSquared < aBestTarget.mDistanceSquared) {
            aBestTarget = {aZombie, nullptr, aDistanceSquared};
        }
    }

    if (theProjectile->mCobTargetRow == 0) {
        GridItem *aGridItem = nullptr;
        while (theProjectile->mBoard->IterateGridItems(aGridItem)) {
            if (aGridItem->mGridY != theSourceRow || !IsAckeeGridTarget(aGridItem) || HasHitGridItem(theProjectile, aGridItem)) {
                continue;
            }

            const float aTargetX = float(theProjectile->mBoard->GridToPixelX(aGridItem->mGridX, aGridItem->mGridY));
            if (aGridItem->mGridX < theSourceGridX || aGridItem->mGridX > theSourceGridX + ACKEE_BOUNCE_GRID_RANGE) {
                continue;
            }

            const float aDeltaX = std::max(0.0f, aTargetX - theSourceTargetX);
            const float aDistanceSquared = aDeltaX * aDeltaX;
            const bool aPreferGraveInSameCell = aDistanceSquared == aBestTarget.mDistanceSquared && IsAckeeGraveTarget(aGridItem) && aBestTarget.mGridItem != nullptr
                && aBestTarget.mGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE;
            if (aDistanceSquared < aBestTarget.mDistanceSquared || aPreferGraveInSameCell) {
                aBestTarget = {nullptr, aGridItem, aDistanceSquared};
            }
        }
    }

    return aBestTarget;
}

void StartAckeeBounce(Projectile *theProjectile, const AckeeTarget &theTarget) {
    theProjectile->mReturning = true;
    theProjectile->mMotionType = ProjectileMotion::MOTION_LOBBED;
    theProjectile->mTargetZombieID = theTarget.mZombie ? theProjectile->mBoard->ZombieGetID(theTarget.mZombie) : ZombieID::ZOMBIEID_NULL;
    // DataArray IDs use the upper 16 bits as a key.  Board initializes the
    // grid-item key above INT_MAX, so a valid ID is negative when stored in
    // this legacy int field.  Reserve zero as the null value and preserve the
    // ID's bit pattern instead of using its sign as validity.
    theProjectile->mLastPortalX = theTarget.mGridItem ? static_cast<int>(static_cast<uint32_t>(theProjectile->mBoard->GridItemGetID(theTarget.mGridItem))) : 0;

    Rect aTargetRect = theTarget.mZombie ? theTarget.mZombie->GetZombieRect() : theTarget.mGridItem->GetItemRect();
    const float aTargetX = float(aTargetRect.mX) + float(aTargetRect.mWidth - theProjectile->mWidth) * 0.5f;
    const float aTargetY = float(aTargetRect.mY) + float(aTargetRect.mHeight) * 0.35f;
    const float aFlightTime = -2.0f * ACKEE_BOUNCE_VELOCITY_Z / ACKEE_BOUNCE_ACCELERATION_Z;

    theProjectile->mPosZ = 0.0f;
    theProjectile->mVelX = (aTargetX - theProjectile->mPosX) / aFlightTime;
    theProjectile->mVelY = (aTargetY - theProjectile->mPosY) / aFlightTime;
    theProjectile->mVelZ = ACKEE_BOUNCE_VELOCITY_Z;
    theProjectile->mAccZ = ACKEE_BOUNCE_ACCELERATION_Z;
}

void HandleAckeeImpact(Projectile *theProjectile, Zombie *theZombie, GridItem *theGridItem) {
    if ((theZombie == nullptr && theGridItem == nullptr) || (theZombie && HasHitZombie(theProjectile, theZombie)) || (theGridItem && HasHitGridItem(theProjectile, theGridItem))) {
        theProjectile->Die();
        return;
    }

    const int aHitIndex = theProjectile->mPierceHitCount;
    if (aHitIndex >= ACKEE_MAX_HITS) {
        theProjectile->Die();
        return;
    }

    float aSourceTargetX;
    int aSourceGridX;
    int aSourceRow;
    if (theZombie) {
        aSourceTargetX = theZombie->mPosX;
        aSourceGridX = theProjectile->mBoard->PixelToGridXKeepOnBoard(int(theZombie->mPosX), int(theZombie->mPosY));
        aSourceRow = theZombie->mRow;
    } else {
        aSourceTargetX = float(theProjectile->mBoard->GridToPixelX(theGridItem->mGridX, theGridItem->mGridY));
        aSourceGridX = theGridItem->mGridX;
        aSourceRow = theGridItem->mGridY;
    }

    theProjectile->PlayImpactSound(theZombie);
    const int aDamage = ACKEE_DAMAGE[aHitIndex];
    if (theZombie) {
        theZombie->TakeDamage(aDamage, theProjectile->GetDamageFlags(theZombie));
    } else {
        const bool aHitGrave = theGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || theGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND;
        theGridItem->TakeDamage(aDamage, 0U);
        if (aHitGrave) {
            // VS chains can damage one grave, then continue through zombies only.
            theProjectile->mCobTargetRow = 1;
        }
    }
    theProjectile->mApp->AddTodParticle(theProjectile->mPosX + 12.0f, theProjectile->mPosY + theProjectile->mPosZ + 12.0f, theProjectile->mRenderOrder + 1, ParticleEffect::PARTICLE_CABBAGE_SPLAT);

    if (aHitIndex + 1 >= ACKEE_MAX_HITS) {
        theProjectile->Die();
        return;
    }

    if (theZombie) {
        theProjectile->mHitZombieIDs[aHitIndex] = theProjectile->mBoard->ZombieGetID(theZombie);
    } else {
        theProjectile->mHitGridItemIDs[aHitIndex] = theProjectile->mBoard->GridItemGetID(theGridItem);
    }
    ++theProjectile->mPierceHitCount;

    const AckeeTarget aNextTarget = FindAckeeBounceTarget(theProjectile, aSourceTargetX, aSourceGridX, aSourceRow);
    if (aNextTarget.mZombie == nullptr && aNextTarget.mGridItem == nullptr) {
        theProjectile->Die();
        return;
    }
    StartAckeeBounce(theProjectile, aNextTarget);
}

} // namespace

ProjectileDefinition gProjectileDefinition[] = {
    {ProjectileType::PROJECTILE_PEA, 0, 20},
    {ProjectileType::PROJECTILE_SNOWPEA, 0, 20},
    {ProjectileType::PROJECTILE_CABBAGE, 0, 40},
    {ProjectileType::PROJECTILE_MELON, 0, 80},
    {ProjectileType::PROJECTILE_PUFF, 0, 20},
    {ProjectileType::PROJECTILE_WINTERMELON, 0, 80},
    {ProjectileType::PROJECTILE_FIREBALL, 0, 40},
    {ProjectileType::PROJECTILE_STAR, 0, 20},
    {ProjectileType::PROJECTILE_SPIKE, 0, 20},
    {ProjectileType::PROJECTILE_BASKETBALL, 0, 75},
    {ProjectileType::PROJECTILE_KERNEL, 0, 20},
    {ProjectileType::PROJECTILE_COBBIG, 0, 300},
    {ProjectileType::PROJECTILE_BUTTER, 0, 40},
    {ProjectileType::PROJECTILE_ZOMBIE_PEA, 0, 20},
};

ProjectileDefinition gExtendedProjectileDefinition[] = {
    {ProjectileType::PROJECTILE_ZOMBIE_POLE, 0, 2400},
    {ProjectileType::PROJECTILE_ZOMBIE_FIREBALL, 0, 40},
    {ProjectileType::PROJECTILE_ZOMBLOB, 0, 0},
    {ProjectileType::PROJECTILE_SPORE, 0, 50},
    {ProjectileType::PROJECTILE_BOOMERANG, 0, 20},
    {ProjectileType::PROJECTILE_TELEPORTATION, 0, 0},
    {ProjectileType::PROJECTILE_TRAFFIC_CONE, 0, 0},
    {ProjectileType::PROJECTILE_ACKEE, 0, 60},
    {ProjectileType::PROJECTILE_RED_STINGER, 0, 30},
};

void Projectile::ProjectileInitialize(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType) {
    mProjectileType = ResolveInitialProjectileType(theProjectileType);
    mPosX = static_cast<float>(theX);
    mPosY = static_cast<float>(theY);
    mPosZ = 0.0f;
    mVelX = 0.0f;
    mVelY = 0.0f;
    mVelZ = 0.0f;
    mAccZ = 0.0f;
    mRow = theRow;
    mRenderOrder = theRenderOrder;
    mWidth = 40;
    mHeight = 40;
    mFrame = 0;
    mNumFrames = 1;
    mAnimCounter = 0;
    mAnimTicksPerFrame = 0;
    mMotionType = ProjectileMotion::MOTION_STRAIGHT;
    mProjectileAge = 0;
    mClickBackoffCounter = 0;
    mRotation = 0.0f;
    mRotationSpeed = 0.0f;
    mDead = false;
    mDamageRangeFlags = 0;
    mHitTorchwoodGridX = -1;
    mLastPortalX = -1;
    mAttachmentID = AttachmentID::ATTACHMENTID_NULL;
    mCobTargetX = 0.0f;
    mCobTargetRow = 0;
    mTargetZombieID = ZombieID::ZOMBIEID_NULL;
    mRelatedPlantID = PlantID::PLANTID_NULL;
    mReturning = false;
    ResetHitHistory(this);

    const int aGridX = mBoard->PixelToGridXKeepOnBoard(theX, theY);
    mShadowY = static_cast<float>(mBoard->GridToPixelY(aGridX, theRow)) + 67.0f;
    mOnHighGround = mBoard->mGridSquareType[aGridX][theRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND;
    if (mBoard->StageHasRoof() && mPosX < 480.0f) {
        mShadowY -= 12.0f;
    }

    switch (mProjectileType) {
        case ProjectileType::PROJECTILE_CABBAGE:
        case ProjectileType::PROJECTILE_BUTTER:
            mRotation = -7 * std::numbers::pi / 25;
            mRotationSpeed = RandRangeFloat(-0.08f, -0.02f);
            break;
        case ProjectileType::PROJECTILE_MELON:
        case ProjectileType::PROJECTILE_WINTERMELON:
            mRotation = -2 * std::numbers::pi / 5;
            mRotationSpeed = RandRangeFloat(-0.08f, -0.02f);
            break;
        case ProjectileType::PROJECTILE_KERNEL:
            mRotationSpeed = RandRangeFloat(-0.2f, -0.08f);
            break;
        case ProjectileType::PROJECTILE_SNOWPEA: {
            TodParticleSystem *aParticle = mApp->AddTodParticle(mPosX + 8.0f, mPosY + 13.0f, RenderLayer::RENDER_LAYER_TOP, ParticleEffect::PARTICLE_SNOWPEA_TRAIL);
            AttachParticle(mAttachmentID, aParticle, 8.0f, 13.0f);
            break;
        }
        case ProjectileType::PROJECTILE_PUFF: {
            TodParticleSystem *aParticle = mApp->AddTodParticle(mPosX + 13.0f, mPosY + 13.0f, RenderLayer::RENDER_LAYER_TOP, ParticleEffect::PARTICLE_PUFFSHROOM_TRAIL);
            AttachParticle(mAttachmentID, aParticle, 13.0f, 13.0f);
            break;
        }
        case ProjectileType::PROJECTILE_FIREBALL:
            ConvertToFireball(0);
            break;
        case ProjectileType::PROJECTILE_COBBIG: {
            Image *aImage = IMAGE_REANIM_COBCANNON_COB;
            mWidth = aImage->GetVTable()->GetWidth(aImage);
            mHeight = aImage->GetVTable()->GetHeight(aImage);
            mRotation = std::numbers::pi_v<float> / 2.0f;
            break;
        }
        case ProjectileType::PROJECTILE_BASKETBALL:
            mRotation = Sexy::Rand(2.0f * std::numbers::pi_v<float>);
            mRotationSpeed = RandRangeFloat(0.05f, 0.1f);
            break;
        case ProjectileType::PROJECTILE_STAR:
            mShadowY += 15.0f;
            mRotationSpeed = RandRangeFloat(0.05f, 0.1f);
            if (Sexy::Rand(2) == 0) {
                mRotationSpeed = -mRotationSpeed;
            }
            break;
        case ProjectileType::PROJECTILE_ZOMBLOB:
            mMotionType = ProjectileMotion::MOTION_LOBBED;
            break;
        case ProjectileType::PROJECTILE_SPORE:
            mRotation = -7 * std::numbers::pi / 25;
            mRotationSpeed = RandRangeFloat(-0.08f, -0.02f);
            break;
        case ProjectileType::PROJECTILE_BOOMERANG:
            mRotationSpeed = 0.2f;
            break;
        case ProjectileType::PROJECTILE_TRAFFIC_CONE:
            mRotation = RandRangeFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
            mRotationSpeed = RandRangeFloat(0.05f, 0.1f);
            break;
        case ProjectileType::PROJECTILE_ACKEE:
            mLastPortalX = 0;
            break;
        default:
            break;
    }

    mX = static_cast<int>(mPosX);
    mY = static_cast<int>(mPosY);
}

Plant *Projectile::FindCollisionTargetPlant() {
    Rect aProjectileRect = GetProjectileRect();

    // 魅惑传送僵尸的子弹只作用于敌方僵尸，不影响植物。
    if (mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION && mReturning) {
        return nullptr;
    }

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mRow != mRow)
            continue;

        if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_POLE || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL
            || mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION) {
            if (aPlant->IsLowProfile()) // 僵尸子弹不能击中低矮植物
                continue;
            if (mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION && (aPlant->mSeedType == SeedType::SEED_INSTANT_COFFEE || aPlant->IsInvulnerable()))
                continue;
        }

        Rect aPlantRect = aPlant->GetPlantRect();
        if (GetRectOverlap(aProjectileRect, aPlantRect) > 8) {
            if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_POLE || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL
                || mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION) {
                Plant *aTopPlant = mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_EATING_ORDER);
                if (mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION && aTopPlant != nullptr
                    && (aTopPlant->NotOnGround() || aTopPlant->IsLowProfile() || aTopPlant->mSeedType == SeedType::SEED_INSTANT_COFFEE)) {
                    return aPlant;
                }
                return aTopPlant;
            } else {
                return mBoard->GetTopPlantAt(aPlant->mPlantCol, aPlant->mRow, PlantPriority::TOPPLANT_CATAPULT_ORDER);
            }
        }
    }

    return nullptr;
}

bool Projectile::PeaAboutToHitTorchwood() {
    if (mMotionType != ProjectileMotion::MOTION_STRAIGHT)
        return false;

    if (mProjectileType != ProjectileType::PROJECTILE_PEA && mProjectileType != ProjectileType::PROJECTILE_SNOWPEA)
        return false;

    Plant *aPlant = nullptr;
    while (mBoard->IteratePlants(aPlant)) {
        if (aPlant->mSeedType == SeedType::SEED_TORCHWOOD && aPlant->mRow == mRow && !aPlant->NotOnGround() && mHitTorchwoodGridX != aPlant->mPlantCol) {
            Rect aPlantAttackRect = aPlant->GetPlantAttackRect(PlantWeapon::WEAPON_PRIMARY);
            Rect aProjectileRect = GetProjectileRect();
            aProjectileRect.mX += 40;

            if (GetRectOverlap(aPlantAttackRect, aProjectileRect) > 10) {
                return true;
            }
        }
    }

    return false;
}

Zombie *Projectile::FindCollisionTarget() {
    if (PeaAboutToHitTorchwood()) // “卡火炬”的原理，这段代码在两版内测版中均不存在
        return nullptr;

    Rect aProjectileRect = GetProjectileRect();
    Zombie *aBestZombie = nullptr;
    int aMinX = 0;

    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if ((aZombie->mZombieType == ZombieType::ZOMBIE_BOSS || aZombie->mRow == mRow) && aZombie->EffectedByDamage((unsigned int)mDamageRangeFlags)) {
            if (aZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL && mPosZ >= 45.0f) {
                continue;
            }

            if (mProjectileType == ProjectileType::PROJECTILE_STAR && mProjectileAge < 25 && mVelX >= 0.0f && aZombie->mZombieType == ZombieType::ZOMBIE_DIGGER) {
                continue;
            }

            // 修复对地发射的尖刺能命中气球僵尸
            if (mProjectileType == ProjectileType::PROJECTILE_SPIKE && mDamageRangeFlags == DamageRangeFlags::DAMAGES_FLYING && aZombie->IsFlying()) {
                continue;
            }

            Rect aZombieRect = aZombie->GetZombieRect();
            if (GetRectOverlap(aProjectileRect, aZombieRect) > 0) {
                if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
                    const int aTargetSlot = FindHitZombieSlot(this, aZombie);
                    if (aTargetSlot < 0) {
                        continue;
                    }

                    const int aHitMask = mReturning ? mCobTargetRow : mHitTorchwoodGridX;
                    if ((aHitMask & (1 << aTargetSlot)) != 0) {
                        continue;
                    }
                } else if (IsPiercingSpike(this) && HasHitZombie(this, aZombie)) {
                    continue;
                }

                const bool aPreferThisZombie = aBestZombie == nullptr || (!mReturning && aZombie->mX < aMinX) || (mReturning && aZombie->mX > aMinX);
                if (aPreferThisZombie) {
                    aBestZombie = aZombie;
                    aMinX = aZombie->mX;
                }
            }
        }
    }

    return aBestZombie;
}

Rect Projectile::GetProjectileRect() {
    if (mProjectileType == ProjectileType::PROJECTILE_PEA || mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA) {
        return {mX - 15, mY, mWidth + 15, mHeight};
    } else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG) {
        return {mX + mWidth / 2 - 115, mY + mHeight / 2 - 115, 230, 230};
    } else if (mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON) {
        return {mX + 20, mY, 60, mHeight};
    } else if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL) {
        return {mX, mY, mWidth - 10, mHeight};
    } else if (mProjectileType == ProjectileType::PROJECTILE_SPIKE) {
        return {mX - 25, mY, mWidth + 25, mHeight};
    } else if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_POLE) {
        return {mX + 60, mY, mWidth, mHeight};
    } else {
        return {mX, mY, mWidth, mHeight};
    }
}

void Projectile::ConvertToFireball(int theGridX) {
    if (isOnlyTouchFireWood && !IsOnlineServerModeActive() && !gIsReplayMode) {
        if (bulletSpinnerChosenNum != -1) {
            mProjectileType = (ProjectileType)bulletSpinnerChosenNum;
            return;
        }
        if (randomBullet) {
            mProjectileType = ProjectileType(RandRangeInt(PROJECTILE_SNOWPEA, PROJECTILE_ZOMBIE_PEA - 1));
            return;
        }
    }

    if (mHitTorchwoodGridX == theGridX)
        return;

    mProjectileType = ProjectileType::PROJECTILE_FIREBALL;
    mHitTorchwoodGridX = theGridX;
    mApp->PlayFoley(FoleyType::FOLEY_FIREPEA);

    float aOffsetX = -25.0f;
    float aOffsetY = -25.0f;
    Reanimation *aFirePeaReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_FIRE_PEA);
    if (mMotionType == ProjectileMotion::MOTION_BACKWARDS) {
        aFirePeaReanim->OverrideScale(-1.0f, 1.0f);
        aOffsetX += 80.0f;
    }

    aFirePeaReanim->SetPosition(mPosX + aOffsetX, mPosY + aOffsetY);
    aFirePeaReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
    aFirePeaReanim->mAnimRate = RandRangeFloat(50.0f, 80.0f);
    AttachReanim(mAttachmentID, aFirePeaReanim, aOffsetX, aOffsetY);
}

void Projectile::ConvertToZombieFireball() {
    mProjectileType = ProjectileType::PROJECTILE_ZOMBIE_FIREBALL;
    mApp->PlayFoley(FoleyType::FOLEY_FIREPEA);

    float aOffsetX = -25.0f;
    float aOffsetY = -25.0f;
    Reanimation *aFirePeaReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_FIRE_PEA);
    if (mMotionType == ProjectileMotion::MOTION_BACKWARDS) {
        aFirePeaReanim->OverrideScale(-1.0f, 1.0f);
        aOffsetX += 80.0f;
    }

    aFirePeaReanim->SetPosition(mPosX + aOffsetX, mPosY + aOffsetY);
    aFirePeaReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
    aFirePeaReanim->mAnimRate = RandRangeFloat(50.0f, 80.0f);
    AttachReanim(mAttachmentID, aFirePeaReanim, aOffsetX, aOffsetY);
}

void Projectile::ConvertToPea(int theGridX) {
    if (ColdPeaCanPassFireWood && !IsOnlineServerModeActive() && !gIsReplayMode) {
        if (mHitTorchwoodGridX != theGridX) {
            AttachmentDie(mAttachmentID);
            mHitTorchwoodGridX = theGridX;
            mProjectileType = ProjectileType::PROJECTILE_SNOWPEA;
            mApp->PlayFoley(FoleyType::FOLEY_THROW);
        }
        return;
    }

    old_Projectile_ConvertToPea(this, theGridX);
}

void Projectile::Update() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 如果开了高级暂停
        return;
    }

    mProjectileAge++;
    if (mApp->mGameScene != GameScenes::SCENE_PLAYING && !mBoard->mCutScene->ShouldRunUpsellBoard()) {
        return;
    }

    int aTime = 20;
    if (mProjectileType == ProjectileType::PROJECTILE_PEA || mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || mProjectileType == ProjectileType::PROJECTILE_CABBAGE
        || mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON || mProjectileType == ProjectileType::PROJECTILE_KERNEL
        || mProjectileType == ProjectileType::PROJECTILE_BUTTER || mProjectileType == ProjectileType::PROJECTILE_COBBIG || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA
        || mProjectileType == ProjectileType::PROJECTILE_SPIKE) {
        aTime = 0;
    }
    if (mProjectileAge > aTime) {
        mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PROJECTILE, mRow, 0);
    }

    if (mClickBackoffCounter > 0) {
        mClickBackoffCounter--;
    }
    mRotation += mRotationSpeed;

    UpdateMotion();
    if (mProjectileType != ProjectileType::PROJECTILE_FIREBALL || mRotation == 0.0) {
        AttachmentUpdateAndMove(mAttachmentID, mPosX, mPosZ + mPosY);
    } else {
        SexyTransform2D aTransform;
        aTransform.RotateRad(mRotation);
        aTransform.Translate(mPosX, mPosZ + mPosY);
        AttachmentUpdateAndSetMatrix(mAttachmentID, aTransform);
    }
}

void Projectile::UpdateMotion() {
    if (mAnimTicksPerFrame > 0) {
        mAnimCounter = (mAnimCounter + 1) % (mNumFrames * mAnimTicksPerFrame);
        mFrame = mAnimCounter / mAnimTicksPerFrame;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_ACKEE && mReturning) {
        UpdateAckeeMotion();
        mX = int(mPosX);
        mY = int(mPosY + mPosZ);
        return;
    }

    int aOldRow = mRow;
    float aOldY = mBoard->GetPosYBasedOnRow(mPosX, mRow);
    if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
        UpdateBoomerang();
    } else if (mMotionType == ProjectileMotion::MOTION_LOBBED) {
        UpdateLobMotion();
    } else {
        UpdateNormalMotion();
    }

    float aSlopeHeightChange = mBoard->GetPosYBasedOnRow(mPosX, aOldRow) - aOldY;

    if (mProjectileType == ProjectileType::PROJECTILE_COBBIG) {
        aSlopeHeightChange = 0.0f;
    }

    if (mMotionType == ProjectileMotion::MOTION_FLOAT_OVER) {
        mPosY += aSlopeHeightChange;
    }
    if (mMotionType == ProjectileMotion::MOTION_LOBBED) {
        mPosY += aSlopeHeightChange;
        mPosZ -= aSlopeHeightChange;
    }
    mShadowY += aSlopeHeightChange;
    mX = int(mPosX);
    mY = int(mPosY + mPosZ);
}

void Projectile::BoomerangReturn() {
    if (mReturning) {
        return;
    }

    mReturning = true;         // 回旋镖折返
    mClickBackoffCounter = 25; // 悬停时间
    mVelX = 0.0f;
}

void Projectile::UpdateBoomerang() {
    // 折返点暂停
    if (mClickBackoffCounter > 0) {
        return;
    }

    if (mReturning && mVelX == 0.0f) {
        mVelX = -6.6f;
    }

    mPosX += mVelX;
    mPosY += mVelY;
    mShadowY += mVelY;

    const bool aReachedTurnPoint = !mReturning && mPosX >= mCobTargetX;
    if (aReachedTurnPoint) {
        mPosX = mCobTargetX;
    }

    Zombie *aZombie = FindCollisionTarget();
    if (aZombie != nullptr) {
        if (!(aZombie->mOnHighGround && CantHitHighGround())) {
            DoImpact(aZombie);
        }
    } else {
        // 检测骨堆，以及对战模式的墓碑和靶子。
        GridItem *aGridItem = FindCollisionTargetGridItem();
        if (aGridItem != nullptr) {
            DoImpactGridItem(aGridItem);
        }
    }

    if (mDead) {
        return;
    }

    if (aReachedTurnPoint) {
        BoomerangReturn();
        return;
    }

    if (!mReturning) {
        return;
    }

    // 返回发射植物
    Plant *aRelatedPlant = mBoard->mPlants.DataArrayTryToGet(mRelatedPlantID);
    if (aRelatedPlant != nullptr && !aRelatedPlant->mDead && aRelatedPlant->mRow == mRow) {
        const auto aCatchX = static_cast<float>(aRelatedPlant->mX + 48);
        if (mPosX <= aCatchX) {
            Reanimation *aBodyReanim = mApp->ReanimationTryToGet(aRelatedPlant->mBodyReanimID);
            const float anAnimRate = aBodyReanim && aBodyReanim->mDefinition ? aBodyReanim->mDefinition->mFPS : 24.0f;

            aRelatedPlant->mState = PlantState::STATE_BLOOMERANG_CATCHING;
            aRelatedPlant->PlayBodyReanim("anim_catch", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, anAnimRate);

            Die();
        }
    } else if (mPosX + float(mWidth) < 0.0f) {
        Die();
    }
}

void Projectile::UpdateNormalMotion() {
    old_Projectile_UpdateNormalMotion(this);
}

void Projectile::UpdateAckeeMotion() {
    Zombie *aZombieTarget = mBoard->ZombieTryToGet(mTargetZombieID);
    GridItem *aGridItemTarget = mLastPortalX != 0 ? mBoard->mGridItems.DataArrayTryToGet(static_cast<uint32_t>(mLastPortalX)) : nullptr;

    Rect aTargetRect;
    if (aZombieTarget != nullptr && !aZombieTarget->IsDeadOrDying() && aZombieTarget->EffectedByDamage(mDamageRangeFlags)) {
        aTargetRect = aZombieTarget->GetZombieRect();
    } else if (IsAckeeGridTarget(aGridItemTarget)) {
        aTargetRect = aGridItemTarget->GetItemRect();
    } else {
        Die();
        return;
    }

    mVelZ += mAccZ;
    mPosX += mVelX;
    mPosY += mVelY;
    mPosZ += mVelZ;
    mShadowY += mVelY;
    mRow = mBoard->PixelToGridYKeepOnBoard(int(mPosX), int(mPosY));

    if (mVelZ <= 0.0f || mPosZ < 0.0f) {
        return;
    }

    mPosX = float(aTargetRect.mX) + float(aTargetRect.mWidth - mWidth) * 0.5f;
    mPosY = float(aTargetRect.mY) + float(aTargetRect.mHeight) * 0.35f;
    mPosZ = 0.0f;
    mReturning = false;
    if (aZombieTarget != nullptr) {
        DoImpact(aZombieTarget);
    } else {
        DoImpactGridItem(aGridItemTarget);
    }
}


void Projectile::UpdateLobMotion() {
    if (mProjectileType == ProjectileType::PROJECTILE_TRAFFIC_CONE) {
        Zombie *aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
        if (aZombie == nullptr || aZombie->IsDeadOrDying() || !aZombie->mHasHead || aZombie->mMindControlled != (mLastPortalX != 0) || aZombie->mHelmType != HelmType::HELMTYPE_NONE
            || aZombie->mHelmHealth > 0) {
            Die();
            return;
        }

        float aTargetX = aZombie->mPosX + 40.0f;
        float aTargetY = aZombie->mPosY + 10.0f;
        aZombie->GetTrackPosition("anim_cone", aTargetX, aTargetY);
        aTargetX -= 20.0f;
        aTargetY -= 30.0f;

        const int aTicksRemaining = std::max(1, TRAFFIC_CONE_FLIGHT_TICKS - mProjectileAge + 1);
        mPosX += (aTargetX - mPosX) / float(aTicksRemaining);
        mPosY += (aTargetY - mPosY) / float(aTicksRemaining);
        const float aProgress = std::min(1.0f, float(mProjectileAge) / float(TRAFFIC_CONE_FLIGHT_TICKS));
        mPosZ = -120.0f * 4.0f * aProgress * (1.0f - aProgress);
        mShadowY = aZombie->mPosY + 90.0f;
        if (aProgress >= 0.5f) {
            mRow = aZombie->mRow;
        }

        if (mProjectileAge >= TRAFFIC_CONE_FLIGHT_TICKS) {
            if (!IsRemoteClientOrViewer()) {
                aZombie->ApplyTrafficCone();
                if (IsRemoteServer()) {
                    U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_APPLY_CONE;
                    event.data = uint16_t(mBoard->ZombieGetID(aZombie));
                    netplay::PutEvent(event);
                }
            }
            Die();
        }
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_ZOMBLOB) {
        float aAccZ = mAccZ;
        if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY) {
            aAccZ *= 2.0f;
        }

        mPosZ += mVelZ + 0.5f * aAccZ;
        mVelZ += aAccZ;
        mPosX += mVelX;
        mPosY += mVelY;
        mShadowY += mVelY;
        mRow = mBoard->PixelToGridYKeepOnBoard(int(mPosX), int(mShadowY));

        // 上升阶段不落地；下降并回到地面高度后生成下一阶段僵尸。
        if (mProjectileAge <= 1 || mVelZ <= 0.0f || mPosZ < 0.0f) {
            return;
        }

        const int aLastRow = mBoard->StageHas6Rows() ? 5 : 4;
        const int aTargetRow = std::max(0, std::min(aLastRow, mCobTargetRow));
        const int aTargetGridX = mBoard->PixelToGridXKeepOnBoard(int(mCobTargetX + 20.0f), int(mShadowY));
        const auto aTargetGroundY = float(mBoard->GridToPixelY(aTargetGridX, aTargetRow) + 67);

        mRow = aTargetRow;
        mPosX = mCobTargetX;
        mPosY = aTargetGroundY - 40.0f;
        mShadowY = aTargetGroundY;
        mPosZ = 0.0f;

        const auto aZombieType = ZombieType(mHitTorchwoodGridX);
        const bool aIsAuthoritativeZomblobSpawn = !IsRemoteClientOrViewer();
        if (aIsAuthoritativeZomblobSpawn && (aZombieType == ZombieType::ZOMBIE_ZOMBLOB_MIDDLE || aZombieType == ZombieType::ZOMBIE_ZOMBLOB_SMALL)) {
            Zombie *aZombie = mBoard->AddZombieInRow(aZombieType, aTargetRow, mDamageRangeFlags, false);
            if (aZombie) {
                aZombie->mPosX = mPosX + 20.0f - float(aZombie->mWidth) * 0.5f;
                aZombie->mPosY = aZombie->GetPosYBasedOnRow(aTargetRow);
                aZombie->mX = int(aZombie->mPosX);
                aZombie->mY = int(aZombie->mPosY);
                aZombie->mVariant = false;

                if (IsRemoteServer()) {
                    U16U16U16UNI32UNI32_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_PICK_SPEED;
                    event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(aZombie));
                    event.data2 = uint16_t(aZombie->mAnimTicksPerFrame);
                    event.data4.f32 = aZombie->mVelX;
                    event.data5.f32 = aZombie->mPosX;
                    netplay::PutEvent(event);
                }

                if (mLastPortalX != 0) {
                    aZombie->StartMindControlled();
                }

                if (mBoard->mPlantRow[mRow] == PlantRowType::PLANTROW_POOL) {
                    mApp->AddReanimation(aZombie->mPosX + 23.0f, aZombie->mPosY + 78.0f, aZombie->mRenderOrder + 1, ReanimationType::REANIM_SPLASH)->OverrideScale(0.8f, 0.8f);
                    mApp->AddTodParticle(aZombie->mPosX + 23.0f + 37.0f, aZombie->mPosY + 78.0f + 42.0f, aZombie->mRenderOrder + 1, ParticleEffect::PARTICLE_PLANTING_POOL);
                    mApp->PlayFoley(FoleyType::FOLEY_ZOMBIESPLASH);
                    aZombie->DieNoLoot();
                }
            }
        }

        Die();
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_COBBIG && mPosZ < -700.0f) {
        mVelZ = 8.0f;
        mRow = mCobTargetRow;
        mPosX = mCobTargetX;
        int aCobTargetCol = mBoard->PixelToGridXKeepOnBoard(int(mCobTargetX), 0);
        mPosY = float(mBoard->GridToPixelY(aCobTargetCol, mCobTargetRow));
        mShadowY = mPosY + 67.0f;
        mRotation = -1.5708f;
    }

    mVelZ += mAccZ;
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY) {
        mVelZ += mAccZ;
    }
    mPosX += mVelX;
    mPosY += mVelY;
    mPosZ += mVelZ;

    const bool aIsRising = mVelZ < 0.0f;
    if (aIsRising && (mProjectileType == ProjectileType::PROJECTILE_BASKETBALL || mProjectileType == ProjectileType::PROJECTILE_COBBIG)) {
        return;
    }

    if (mProjectileAge > 20) {
        if (aIsRising) {
            return;
        }

        float aMinCollisionZ = 0.0f;
        if (mProjectileType == ProjectileType::PROJECTILE_BUTTER) {
            aMinCollisionZ = -32.0f;
        } else if (mProjectileType == ProjectileType::PROJECTILE_BASKETBALL) {
            aMinCollisionZ = 60.0f;
        } else if (mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON) {
            aMinCollisionZ = -35.0f;
        } else if (mProjectileType == ProjectileType::PROJECTILE_CABBAGE || mProjectileType == ProjectileType::PROJECTILE_KERNEL || mProjectileType == ProjectileType::PROJECTILE_SPORE
                   || mProjectileType == ProjectileType::PROJECTILE_ACKEE || mProjectileType == ProjectileType::PROJECTILE_RED_STINGER) {
            aMinCollisionZ = -30.0f;
        } else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG) {
            aMinCollisionZ = -60.0f;
        }

        if (mBoard->mPlantRow[mRow] == PlantRowType::PLANTROW_POOL) {
            aMinCollisionZ += 40.0f;
        }

        if (mPosZ <= aMinCollisionZ) {
            return;
        }
    }

    Plant *aPlantTarget = nullptr;
    Zombie *aZombieTarget = nullptr;
    GridItem *aGridTarget = nullptr;

    if (mProjectileType == ProjectileType::PROJECTILE_BASKETBALL || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA) {
        aPlantTarget = FindCollisionTargetPlant();
    } else {
        aZombieTarget = FindCollisionTarget();
        aGridTarget = FindCollisionTargetGridItem();
    }

    const float aGroundZ = mProjectileType == ProjectileType::PROJECTILE_COBBIG ? -40.0f : 80.0f;
    const bool aHitGround = mPosZ > aGroundZ;
    if (!aZombieTarget && !aPlantTarget && !aGridTarget && !aHitGround) {
        return;
    }

    if (aPlantTarget) {
        Plant *aUmbrella = mBoard->FindUmbrellaPlant(aPlantTarget->mPlantCol, aPlantTarget->mRow);
        if (aUmbrella) {
            if (aUmbrella->mState == PlantState::STATE_UMBRELLA_REFLECTING) {
                mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
                int aRenderPos = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
                mApp->AddTodParticle(mPosX + 20.0f, mPosY + 20.0f, aRenderPos, ParticleEffect::PARTICLE_UMBRELLA_REFLECT);
                Die();
            } else if (aUmbrella->mState != PlantState::STATE_UMBRELLA_TRIGGERED) {
                mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);
                aUmbrella->DoSpecial();
            }
            return;
        }
        Plant *aBloomerang = mBoard->FindBloomerangPlant(aPlantTarget->mPlantCol, aPlantTarget->mRow);
        if (aBloomerang) {
            if (aBloomerang->mState == PlantState::STATE_UMBRELLA_REFLECTING) {
                mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
                int aRenderPos = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
                mApp->AddTodParticle(mPosX + 20.0f, mPosY + 20.0f, aRenderPos, ParticleEffect::PARTICLE_UMBRELLA_REFLECT);
                Die();
            } else if (aBloomerang->mState != PlantState::STATE_UMBRELLA_TRIGGERED) {
                mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);
                aBloomerang->DoSpecial();
            }
            return;
        }

        aPlantTarget->mPlantHealth -= GetProjectileDef().mDamage;
        aPlantTarget->mEatenFlashCountdown = std::max(aPlantTarget->mEatenFlashCountdown, 25);
        mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
        Die();
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_COBBIG) {
        mBoard->KillAllZombiesInRadius_Custom(mRow, int(mPosX + 80.0f), int(mPosY + 40.0f), 115, 1, true, mDamageRangeFlags);
        DoImpact(nullptr);
        return;
    }

    if (aZombieTarget) {
        DoImpact(aZombieTarget);
    } else if (aGridTarget) {
        DoImpactGridItem(aGridTarget);
    } else {
        DoImpact(nullptr);
    }
}

void Projectile::DoSplashDamage(Zombie *theZombie, GridItem *theGridItem) {
    const ProjectileDefinition &aProjectileDef = GetProjectileDef();

    int aTargetsGetSplashed = 0;
    Zombie *aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (aZombie != theZombie && IsZombieHitBySplash(aZombie)) {
            ++aTargetsGetSplashed;
        }
    }

    GridItem *aGridItem = nullptr;
    while (mBoard->IterateGridItems(aGridItem)) {
        if (aGridItem != theGridItem && IsGridItemHitBySplash(aGridItem)) {
            ++aTargetsGetSplashed;
        }
    }

    int aOriginalDamage = aProjectileDef.mDamage;
    int aSplashDamage = aProjectileDef.mDamage / 3;
    int aSplashDamageBudget = (mProjectileType == ProjectileType::PROJECTILE_FIREBALL) ? aOriginalDamage : (aOriginalDamage * 7);
    if (aTargetsGetSplashed > 0 && aSplashDamage > 0) {
        int aTotalSplashDamage = aTargetsGetSplashed * aSplashDamage;
        if (aSplashDamageBudget < aTotalSplashDamage) {
            int aScaledSplashDamage = aOriginalDamage * aSplashDamageBudget / (3 * aTargetsGetSplashed * aSplashDamage);
            aSplashDamage = aScaledSplashDamage < 1 ? 1 : aScaledSplashDamage;
        }
    }

    aZombie = nullptr;
    while (mBoard->IterateZombies(aZombie)) {
        if (IsZombieHitBySplash(aZombie)) {
            unsigned int aDamageFlags = GetDamageFlags(aZombie);
            int aDamage = (aZombie == theZombie) ? aOriginalDamage : aSplashDamage;
            aZombie->TakeDamage(aDamage, aDamageFlags);
        }
    }

    aGridItem = nullptr;
    while (mBoard->IterateGridItems(aGridItem)) {
        if (IsGridItemHitBySplash(aGridItem) && aGridItem == theGridItem) {
            aGridItem->TakeDamage(aOriginalDamage, 0U);
        }
    }
}


void Projectile::PlayImpactSound(Zombie *theZombie) {
    bool aPlayHelmSound = true;
    bool aPlaySplatSound = true;
    if (mProjectileType == ProjectileType::PROJECTILE_KERNEL) {
        mApp->PlayFoley(FoleyType::FOLEY_KERNEL_SPLAT);
        aPlayHelmSound = false;
        aPlaySplatSound = false;
    } else if (mProjectileType == ProjectileType::PROJECTILE_BUTTER) {
        mApp->PlayFoley(FoleyType::FOLEY_BUTTER);
        aPlaySplatSound = false;
    } else if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL && IsSplashDamage(theZombie)) {
        mApp->PlayFoley(FoleyType::FOLEY_IGNITE);
        aPlayHelmSound = false;
        aPlaySplatSound = false;
    } else if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL) {
        mApp->PlayFoley(FoleyType::FOLEY_IGNITE);
        aPlayHelmSound = false;
        aPlaySplatSound = false;
    } else if (mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON) {
        mApp->PlayFoley(FoleyType::FOLEY_MELONIMPACT);
        aPlaySplatSound = false;
    }

    if (aPlayHelmSound && theZombie) {
        if (theZombie->mHelmType == HELMTYPE_PAIL) {
            mApp->PlayFoley(FoleyType::FOLEY_SHIELD_HIT);
            aPlaySplatSound = false;
        } else if (theZombie->mHelmType == HELMTYPE_TRAFFIC_CONE || theZombie->mHelmType == HELMTYPE_DIGGER || theZombie->mHelmType == HELMTYPE_FOOTBALL
                   || theZombie->mHelmType == HELMTYPE_GIGA_FOOTBALL || theZombie->mHelmType == HELMTYPE_CROSSING_GUARD || theZombie->mHelmType == HELMTYPE_GRAVEDIGGER) {
            mApp->PlayFoley(FoleyType::FOLEY_PLASTIC_HIT);
        }
    }

    if (aPlaySplatSound) {
        mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
    }
}

void Projectile::DoImpact(Zombie *theZombie) {
    bool aIsPiercingSpike = theZombie != nullptr && IsPiercingSpike(this) && !theZombie->IsFlying();
    if (aIsPiercingSpike && HasHitZombie(this, theZombie)) {
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
        if (theZombie == nullptr) {
            return;
        }

        const int aTargetSlot = FindHitZombieSlot(this, theZombie);
        if (aTargetSlot < 0) {
            return;
        }

        int &aHitMask = mReturning ? mCobTargetRow : mHitTorchwoodGridX;
        const int aTargetBit = 1 << aTargetSlot;
        if ((aHitMask & aTargetBit) != 0) {
            return;
        }

        PlayImpactSound(theZombie);
        theZombie->TakeDamage(GetProjectileDef().mDamage, GetDamageFlags(theZombie));

        // 同一目标在去程和回程各只命中一次。
        aHitMask |= aTargetBit;
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_ACKEE) {
        HandleAckeeImpact(this, theZombie, nullptr);
        return;
    }

    const bool aIsSporeImpact = mProjectileType == ProjectileType::PROJECTILE_SPORE && theZombie != nullptr;

    bool aWasAliveBeforeImpact = false;
    bool aCouldLoseBodyPartsBeforeImpact = false;
    bool aHadHeadBeforeImpact = false;
    bool aWasAboveHeadDropThreshold = false;
    int aHeadDropThreshold = 0;
    int aSporeGridX = -1;
    int aSporeGridY = -1;

    if (aIsSporeImpact) {
        aWasAliveBeforeImpact = !theZombie->IsDeadOrDying();
        aCouldLoseBodyPartsBeforeImpact = theZombie->CanLoseBodyParts();
        aHadHeadBeforeImpact = theZombie->mHasHead;

        if (aCouldLoseBodyPartsBeforeImpact) {
            // Zombie::UpdateDamageStates() 仅在 mBodyHealth < mBodyMaxHealth / 3 时掉头。
            // 命中前已经低于阈值但尚未来得及更新掉头状态的僵尸，不归功于本发孢子弹。
            aHeadDropThreshold = theZombie->mBodyMaxHealth / 3;
            aWasAboveHeadDropThreshold = theZombie->mBodyHealth >= aHeadDropThreshold;
        }

        aSporeGridX = mBoard->PixelToGridXKeepOnBoard(theZombie->mX + theZombie->mWidth / 2, theZombie->mY);
        aSporeGridY = theZombie->mRow;
    }

    PlayImpactSound(theZombie);

    if (IsSplashDamage(theZombie)) {
        if (theZombie && mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
            theZombie->RemoveColdEffects();

            if (theZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && !theZombie->mHasObject) {
                theZombie->ExplorerTorchConvert(true);
            }
        }

        DoSplashDamage(theZombie, nullptr);
    } else if (theZombie) {
        unsigned int aDamageFlags = GetDamageFlags(theZombie);
        if (mApp->IsVSMode() && mProjectileType == ProjectileType::PROJECTILE_SPIKE) {
            if (theZombie->IsFlying()) {
                theZombie->TakeDamage(theZombie->mFlyingHealth, aDamageFlags);
                Die();
                return;
            }

            if (VSSetupAddonWidget::msBalancePatchMode || Challenge::msVSShuffleMode) {
                int aHitIndex = mPierceHitCount;
                int aDamage = SPIKE_PIERCE_DAMAGE[aHitIndex];
                if (theZombie->mShieldType == ShieldType::SHIELDTYPE_DOOR || theZombie->mShieldType == ShieldType::SHIELDTYPE_LADDER || theZombie->mShieldType == ShieldType::SHIELDTYPE_TRASHCAN
                    || theZombie->mZombieType == ZombieType::ZOMBIE_ZAMBONI) {
                    aDamage = 0;
                    for (int i = aHitIndex; i < MAX_PIERCE_HIT_COUNT; ++i) {
                        aDamage += SPIKE_PIERCE_DAMAGE[i];
                    }
                    mPierceHitCount = MAX_PIERCE_HIT_COUNT;
                } else {
                    ++mPierceHitCount;
                }

                theZombie->TakeDamage(aDamage, aDamageFlags);
                mHitZombieIDs[aHitIndex] = mBoard->ZombieGetID(theZombie);
                if (mPierceHitCount >= MAX_PIERCE_HIT_COUNT) {
                    Die();
                }
            } else {
                theZombie->TakeDamage(GetProjectileDef().mDamage, aDamageFlags);
                Die();
            }
            return;
        } else {
            if (Zombie::IsZomblob(theZombie->mZombieType) && mProjectileType == ProjectileType::PROJECTILE_BUTTER) {
                if (!theZombie->mButtered) {
                    theZombie->mButtered = true;
                    theZombie->SetupButteredZomblobReanim();
                }
            }

            theZombie->TakeDamage(GetProjectileImpactDamage(this), aDamageFlags);
        }
    }

    bool aShouldSpawnSporeShroom = false;
    if (aIsSporeImpact && aWasAliveBeforeImpact) {
        if (aCouldLoseBodyPartsBeforeImpact) {
            // 要求同时满足：命中前有头、命中前尚未进入掉头区间、
            // 命中后生命低于阈值且头已经掉落。这样不会在临界状态补刀时重复生成。
            aShouldSpawnSporeShroom = aHadHeadBeforeImpact && aWasAboveHeadDropThreshold && theZombie->mBodyHealth < aHeadDropThreshold && !theZombie->mHasHead;
        } else {
            // 巨人、冰车、投石车、Boss 等不会走普通掉头逻辑，改为死亡时生成。
            aShouldSpawnSporeShroom = theZombie->IsDeadOrDying();
        }
    }

    if (aShouldSpawnSporeShroom && mBoard->CanPlantAt(aSporeGridX, aSporeGridY, SeedType::SEED_SPORESHROOM) == PlantingReason::PLANTING_OK) {
        Plant *aPlant = mBoard->AddPlant(aSporeGridX, aSporeGridY, SeedType::SEED_SPORESHROOM, SeedType::SEED_NONE, -1, false);
        if (aPlant != nullptr) {
            aPlant->PlayBodyReanim("anim_grow", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
        }
    }

    float aLastPosX = mPosX - mVelX;
    float aLastPosY = mPosY + mPosZ - mVelY - mVelZ;
    ParticleEffect aEffect = ParticleEffect::PARTICLE_NONE;
    float aSplatPosX = mPosX + 12.0f;
    float aSplatPosY = mPosY + 12.0f;
    if (mProjectileType == ProjectileType::PROJECTILE_MELON) {
        mApp->AddTodParticle(aLastPosX + 30.0f, aLastPosY + 30.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_MELONSPLASH);
    } else if (mProjectileType == ProjectileType::PROJECTILE_WINTERMELON) {
        mApp->AddTodParticle(aLastPosX + 30.0f, aLastPosY + 30.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_WINTERMELON);
    } else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG) {
        int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_GROUND, mCobTargetRow, 2);
        mApp->AddTodParticle(mPosX + 80.0f, mPosY + 40.0f, aRenderOrder, ParticleEffect::PARTICLE_BLASTMARK);
        mApp->AddTodParticle(mPosX + 80.0f, mPosY + 40.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_POPCORNSPLASH);
        mApp->PlaySample(Sexy::SOUND_DOOMSHROOM);
        mBoard->ShakeBoard(3, -4);
    } else if (mProjectileType == ProjectileType::PROJECTILE_PEA || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA) {
        aSplatPosX -= 15.0f;
        aEffect = ParticleEffect::PARTICLE_PEA_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_SNOWPEA) {
        aSplatPosX -= 15.0f;
        aEffect = ParticleEffect::PARTICLE_SNOWPEA_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
        if (IsSplashDamage(theZombie)) {
            Reanimation *aFireReanim = mApp->AddReanimation(mPosX + 38.0f, mPosY - 20.0f, mRenderOrder + 1, ReanimationType::REANIM_JALAPENO_FIRE);
            aFireReanim->mAnimTime = 0.25f;
            aFireReanim->mAnimRate = 24.0f;
            aFireReanim->OverrideScale(0.7f, 0.4f);
        }
    } else if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL) {
        if (theZombie) {
            Reanimation *aFireReanim = mApp->AddReanimation(mPosX + 38.0f, mPosY - 20.0f, mRenderOrder + 1, ReanimationType::REANIM_JALAPENO_FIRE);
            aFireReanim->mAnimTime = 0.25f;
            aFireReanim->mAnimRate = 24.0f;
            aFireReanim->OverrideScale(0.7f, 0.4f);
            if (theZombie->mZombieType == ZombieType::ZOMBIE_EXPLORER && !theZombie->mHasObject) {
                theZombie->ExplorerTorchConvert(true);
            }
        }
    } else if (mProjectileType == ProjectileType::PROJECTILE_STAR) {
        aEffect = ParticleEffect::PARTICLE_STAR_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_PUFF || mProjectileType == ProjectileType::PROJECTILE_SPORE) {
        aSplatPosX -= 20.0f;
        aEffect = ParticleEffect::PARTICLE_PUFF_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_CABBAGE) {
        aSplatPosX = aLastPosX - 38.0f;
        aSplatPosY = aLastPosY + 23.0f;
        aEffect = ParticleEffect::PARTICLE_CABBAGE_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_BUTTER) {
        aSplatPosX = aLastPosX - 20.0f;
        aSplatPosY = aLastPosY + 63.0f;
        aEffect = ParticleEffect::PARTICLE_BUTTER_SPLAT;

        if (theZombie) {
            theZombie->ApplyButter();
        }
    }

    if (aEffect != ParticleEffect::PARTICLE_NONE) {
        if (theZombie) {
            float aPosX = aSplatPosX + 52.0f - theZombie->mX;
            float aPosY = aSplatPosY - theZombie->mY;
            if (theZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL || theZombie->mZombiePhase == ZombiePhase::PHASE_SNORKEL_WALKING_IN_POOL) {
                aPosY += 60.0f;
            }
            if (mMotionType == ProjectileMotion::MOTION_BACKWARDS) {
                aPosX -= 80.0f;
            } else if (mPosX > theZombie->mX + 40 && mMotionType != ProjectileMotion::MOTION_LOBBED) {
                aPosX -= 60.0f;
            }
            if (aPosY > 100.0f)
                aPosY = 100.0f;
            if (aPosY < 20.0f)
                aPosY = 20.0f;
            theZombie->AddAttachedParticle(aPosX, aPosY, aEffect);
        } else {
            mApp->AddTodParticle(aSplatPosX, aSplatPosY, mRenderOrder + 1, aEffect);
        }
    }


    if (!projectilePierce || IsOnlineServerModeActive()) {
        Die();
    } else // 负责 直线子弹帧伤
    {
        // 如果玩家开启了“子弹帧伤”,且子弹是抛物线轨迹
        if (mMotionType == ProjectileMotion::MOTION_LOBBED && theZombie == nullptr) {
            Die();
        }
    }
}

void Projectile::DoImpactGridItem(GridItem *theGridItem) {
    // 骨堆开始重生后，已经在途的子弹也不能再造成伤害。
    if (theGridItem != nullptr && theGridItem->IsSkeletonPile() && theGridItem->mGridItemState != GridItemState::GRIDITEM_STATE_SKELETON_WAITING) {
        return;
    }
    bool aIsPiercingSpike = theGridItem != nullptr && IsPiercingSpike(this);
    if (aIsPiercingSpike && HasHitGridItem(this, theGridItem)) {
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
        if (theGridItem == nullptr) {
            return;
        }

        const int aTargetSlot = FindHitGridItemSlot(this, theGridItem);
        if (aTargetSlot < 0) {
            return;
        }

        int &aHitMask = mReturning ? mCobTargetRow : mHitTorchwoodGridX;
        const int aTargetBit = 1 << aTargetSlot;
        if ((aHitMask & aTargetBit) != 0) {
            return;
        }

        PlayImpactSound(nullptr);
        theGridItem->TakeDamage(GetProjectileDef().mDamage, 0U);

        // 同一目标在去程和回程各只命中一次。
        aHitMask |= aTargetBit;
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_ACKEE) {
        HandleAckeeImpact(this, nullptr, theGridItem);
        return;
    }

    PlayImpactSound(nullptr);

    if (IsSplashDamage(nullptr)) {
        DoSplashDamage(nullptr, theGridItem);
    } else if (theGridItem) {
        if (aIsPiercingSpike) {
            int aHitIndex = mPierceHitCount;
            ++mPierceHitCount;
            theGridItem->TakeDamage(SPIKE_PIERCE_DAMAGE[aHitIndex], 0U);
            mHitGridItemIDs[aHitIndex] = mBoard->GridItemGetID(theGridItem);
            if (mPierceHitCount >= MAX_PIERCE_HIT_COUNT) {
                Die();
            }
            return;
        }
        theGridItem->TakeDamage(GetProjectileImpactDamage(this), 0U);
    }

    float aLastPosX = mPosX - mVelX;
    float aLastPosY = mPosY + mPosZ - mVelY - mVelZ;
    ParticleEffect aEffect = ParticleEffect::PARTICLE_NONE;
    float aSplatPosX = mPosX + 12.0f;
    float aSplatPosY = mPosY + 12.0f;
    if (mProjectileType == ProjectileType::PROJECTILE_MELON) {
        mApp->AddTodParticle(aLastPosX + 30.0f, aLastPosY + 30.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_MELONSPLASH);
        Die();
        return;
    } else if (mProjectileType == ProjectileType::PROJECTILE_WINTERMELON) {
        mApp->AddTodParticle(aLastPosX + 30.0f, aLastPosY + 30.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_WINTERMELON);
        Die();
        return;
    } else if (mProjectileType == ProjectileType::PROJECTILE_COBBIG) {
        int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_GROUND, mCobTargetRow, 2);
        mApp->AddTodParticle(mPosX + 80.0f, mPosY + 40.0f, aRenderOrder, ParticleEffect::PARTICLE_BLASTMARK);
        mApp->AddTodParticle(mPosX + 80.0f, mPosY + 40.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_POPCORNSPLASH);
        mApp->PlaySample(Sexy::SOUND_DOOMSHROOM);
        mBoard->ShakeBoard(3, -4);
        Die();
        return;
    } else if (mProjectileType == ProjectileType::PROJECTILE_PEA) {
        aSplatPosX -= 15.0f;
        aEffect = ParticleEffect::PARTICLE_PEA_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_SNOWPEA) {
        aSplatPosX -= 15.0f;
        aEffect = ParticleEffect::PARTICLE_SNOWPEA_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
        if (IsSplashDamage(nullptr)) {
            Reanimation *aFireReanim = mApp->AddReanimation(mPosX + 38.0f, mPosY - 20.0f, mRenderOrder + 1, ReanimationType::REANIM_JALAPENO_FIRE);
            aFireReanim->mAnimTime = 0.25f;
            aFireReanim->mAnimRate = 24.0f;
            aFireReanim->OverrideScale(0.7f, 0.4f);
        }
        Die();
        return;
    } else if (mProjectileType == ProjectileType::PROJECTILE_STAR) {
        aEffect = ParticleEffect::PARTICLE_STAR_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_PUFF) {
        aSplatPosX -= 20.0f;
        aEffect = ParticleEffect::PARTICLE_PUFF_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_CABBAGE || mProjectileType == ProjectileType::PROJECTILE_SPORE) {
        aSplatPosX = aLastPosX - 38.0f;
        aSplatPosY = aLastPosY + 23.0f;
        aEffect = ParticleEffect::PARTICLE_CABBAGE_SPLAT;
    } else if (mProjectileType == ProjectileType::PROJECTILE_BUTTER) {
        aSplatPosX = aLastPosX - 20.0f;
        aSplatPosY = aLastPosY + 63.0f;
        aEffect = ParticleEffect::PARTICLE_BUTTER_SPLAT;
    } else {
        Die();
        return;
    }

    mApp->AddTodParticle(aSplatPosX, aSplatPosY, mRenderOrder + 1, aEffect);
    Die();
}

Zombie *Projectile::FindCollisionMindControlledTarget() {
    // 豌豆僵尸的子弹专用的寻敌函数，寻找被魅惑的僵尸。
    Zombie *aZombie = nullptr;
    Zombie *aBestZombie = nullptr;
    int aMinX = 0;

    Rect aProjectileRect = GetProjectileRect();
    while (mBoard->IterateZombies(aZombie)) {
        if (!aZombie->mDead && aZombie->mRow == mRow && aZombie->mMindControlled) {
            Rect aZombieRect = aZombie->GetZombieRect();
            if (GetRectOverlap(aProjectileRect, aZombieRect) >= 0 && (aBestZombie == nullptr || aZombie->mX > aMinX)) {
                aBestZombie = aZombie;
                aMinX = aZombie->mX;
            }
        }
    }

    return aBestZombie;
}

GridItem *Projectile::FindCollisionTargetGridItem() {
    GridItem *aBestGridItem = nullptr;
    GridItem *aGridItem = nullptr;
    bool aHasGravestoneInRow = false;

    Rect aProjectileRect = GetProjectileRect();
    while (mBoard->IterateGridItems(aGridItem)) {
        if (!aGridItem->IsPlantAttackTarget()) {
            continue;
        }
        // 修复对空发射的尖刺会被墓碑阻挡
        if (mProjectileType == ProjectileType::PROJECTILE_SPIKE && mDamageRangeFlags == DamageRangeFlags::DAMAGES_SUBMERGED) {
            continue;
        }

        if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND || aGridItem->IsSkeletonPile()) {
            if (mRow != aGridItem->mGridY) {
                continue;
            }
            if (IsPiercingSpike(this) || mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
                // 本行有墓碑时，穿透尖刺和回旋镖都不索敌靶子僵尸。
                aHasGravestoneInRow = true;
            }
            if (GetRectOverlap(aProjectileRect, aGridItem->GetItemRect()) > 12) {
                if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
                    const int aTargetSlot = FindHitGridItemSlot(this, aGridItem);
                    if (aTargetSlot < 0) {
                        continue;
                    }

                    const int aHitMask = mReturning ? mCobTargetRow : mHitTorchwoodGridX;
                    if ((aHitMask & (1 << aTargetSlot)) != 0) {
                        continue;
                    }

                    if (!aBestGridItem || (!mReturning && aGridItem->mGridX < aBestGridItem->mGridX) || (mReturning && aGridItem->mGridX > aBestGridItem->mGridX)) {
                        aBestGridItem = aGridItem;
                    }
                    continue;
                }

                if (IsPiercingSpike(this) && HasHitGridItem(this, aGridItem)) {
                    continue;
                }
                if (!aBestGridItem || aBestGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) {
                    aBestGridItem = aGridItem;
                } else if (aGridItem->mGridX < aBestGridItem->mGridX) {
                    aBestGridItem = aGridItem;
                }
            }
            continue;
        } else if (aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) {
            bool findTarget = mProjectileType == ProjectileType::PROJECTILE_BOOMERANG || !aBestGridItem || aBestGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE;
            if (findTarget && aGridItem->mVSTargetZombieHealth > 0) {
                if (mRow == aGridItem->mGridY) {
                    if (GetRectOverlap(aProjectileRect, aGridItem->GetItemRect()) > 12) {
                        if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
                            const int aTargetSlot = FindHitGridItemSlot(this, aGridItem);
                            if (aTargetSlot < 0) {
                                continue;
                            }

                            const int aHitMask = mReturning ? mCobTargetRow : mHitTorchwoodGridX;
                            if ((aHitMask & (1 << aTargetSlot)) != 0) {
                                continue;
                            }

                            if (!aBestGridItem || (!mReturning && aGridItem->mGridX < aBestGridItem->mGridX) || (mReturning && aGridItem->mGridX > aBestGridItem->mGridX)) {
                                aBestGridItem = aGridItem;
                            }
                            continue;
                        }
                        aBestGridItem = aGridItem;
                    }
                }
            }
            continue;
        }
    }

    if (aHasGravestoneInRow && aBestGridItem != nullptr && aBestGridItem->mGridItemType == GridItemType::GRIDITEM_MP_TARGET_ZOMBIE) {
        return nullptr;
    }

    return aBestGridItem;
}

void Projectile::CheckForCollision() {
    if (mProjectileType == ProjectileType::PROJECTILE_TRAFFIC_CONE) {
        return;
    }

    // 史莱姆弹只在 UpdateLobMotion 的落地点生效，不参与普通碰撞。
    if (mProjectileType == ProjectileType::PROJECTILE_ZOMBLOB) {
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION) {
        const Rect aProjectileRect = GetProjectileRect();
        Plant *aPlant = FindCollisionTargetPlant();
        Zombie *aZombie = nullptr;
        Zombie *aBestZombie = nullptr;
        while (mBoard->IterateZombies(aZombie)) {
            if (!aZombie->IsValidTeleportationTarget() || aZombie->mRow != mRow || mBoard->ZombieGetID(aZombie) == mTargetZombieID || (mReturning && aZombie->mMindControlled)) {
                continue;
            }
            const bool aPreferThisZombie = aBestZombie == nullptr || (mReturning ? aZombie->mX < aBestZombie->mX : aZombie->mX > aBestZombie->mX);
            if (GetRectOverlap(aProjectileRect, aZombie->GetZombieRect()) > 8 && aPreferThisZombie) {
                aBestZombie = aZombie;
            }
        }

        if (aPlant == nullptr && aBestZombie == nullptr) {
            if (mPosX > 800.0f || mPosX + mWidth < 0.0f) {
                Die();
            }
            return;
        }

        const bool aHitPlant = aPlant != nullptr && (aBestZombie == nullptr || aPlant->mX >= aBestZombie->mX);
        Reanimation *aBulletFlash = mApp->AddReanimation(mPosX, mPosY + 17.0f, mRenderOrder + 1, ReanimationType::REANIM_TELEPORTATION_BULLET_FLASH);
        if (aBulletFlash != nullptr) {
            aBulletFlash->SetFramesForLayer("anim_flame");
            aBulletFlash->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_FULL_LAST_FRAME;
            aBulletFlash->mAnimRate = 24.0f;
        }
        if (!IsRemoteClientOrViewer()) {
            if (aHitPlant) {
                const int aSearchStartGridX = aPlant->mPlantCol + 2;
                const int aDestGridY = aPlant->mRow;
                const uint16_t aPlantID = uint16_t(mBoard->mPlants.DataArrayGetID(aPlant));
                const bool aDidTeleport = mBoard->TeleportPlant(aPlant, aSearchStartGridX, aDestGridY);
                if (IsRemoteServer()) {
                    U16U16_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_PLANT_TELEPORT;
                    event.data1 = aPlantID;
                    event.data2 = aDidTeleport ? uint16_t(aPlant->mPlantCol) : UINT16_MAX;
                    netplay::PutEvent(event);
                }
            } else {
                const float aDestX = mReturning ? aBestZombie->mPosX + 160.0f : std::max(0.0f, aBestZombie->mPosX - 160.0f);
                mBoard->TeleportZombie(aBestZombie, aDestX);
                if (IsRemoteServer()) {
                    U16UNI32_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_TELEPORT;
                    event.data1 = uint16_t(mBoard->mZombies.DataArrayGetID(aBestZombie));
                    event.data2.f32 = aDestX;
                    netplay::PutEvent(event);
                }
            }
        }
        Die();
        return;
    }

    // 修复豌豆僵尸的子弹无法击中魅惑僵尸、修复随机子弹飞出屏幕不自动消失导致闪退。
    if (mMotionType == ProjectileMotion::MOTION_PUFF && mProjectileAge >= 75) {
        Die();
        return;
    }

    if (mPosX > 800.0f || mPosX + mWidth < 0.0f) {
        Die();
        return;
    }

    if (mMotionType == ProjectileMotion::MOTION_HOMING) {
        Zombie *aZombie = mBoard->ZombieTryToGet(mTargetZombieID);
        if (aZombie && aZombie->EffectedByDamage(mDamageRangeFlags)) {
            Sexy::Rect aProjectileRect = GetProjectileRect();
            Sexy::Rect aZombieRect = aZombie->GetZombieRect();
            int rectOverlap = GetRectOverlap(aProjectileRect, aZombieRect);
            if (rectOverlap >= 0 && mPosY > aZombieRect.mY && mPosY < aZombieRect.mY + aZombieRect.mHeight) {
                DoImpact(aZombie);
            }
        }
        return;
    }

    // if (mProjectileType == ProjectileType::PROJECTILE_STAR && (mPosY > 600.0f || mPosY < 40.0f)) {
    // Die(projectile);
    // return;
    // }

    if (mMotionType == ProjectileMotion::MOTION_STAR && (mPosY > 600.0f || mPosY < 40.0f)) {
        // 将判断条件从mProjectileType改为mMotionType，从而修复随机杨桃子弹在Y方向出界后不消失导致的闪退
        Die();
        return;
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && (mPosY > 600.0f || mPosY < 40.0f)) {
        // 添加一段逻辑，让重型武器中所有Y方向出界的子弹都会消失。无论子弹种类。
        Die();
        return;
    }


    if ((mProjectileType == ProjectileType::PROJECTILE_PEA || mProjectileType == ProjectileType::PROJECTILE_STAR || mProjectileType == ProjectileType::PROJECTILE_RED_STINGER)
        && mShadowY - mPosY > 90.0f) {
        return;
    }
    if (mMotionType == ProjectileMotion::MOTION_FLOAT_OVER) {
        return;
    }

    if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_PEA || mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_POLE) {
        Plant *aPlant = FindCollisionTargetPlant();
        if (aPlant) {
            const ProjectileDefinition &aProjectileDef = GetProjectileDef();
            aPlant->mPlantHealth -= aProjectileDef.mDamage;
            aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);

            mApp->PlayFoley(FoleyType::FOLEY_SPLAT);
            mApp->AddTodParticle(mPosX - 3.0f, mPosY + 17.0f, mRenderOrder + 1, ParticleEffect::PARTICLE_PEA_SPLAT);
            Die();
            return;
        }
        Zombie *aZombie = FindCollisionMindControlledTarget();
        if (aZombie) {
            if (aZombie->mOnHighGround && CantHitHighGround()) {
                return;
            }
            DoImpact(aZombie);
        }
        return;
    } else if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL) {
        Plant *aPlant = FindCollisionTargetPlant();
        if (aPlant) {
            const ProjectileDefinition &aProjectileDef = GetProjectileDef();
            aPlant->mPlantHealth -= aProjectileDef.mDamage;
            aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 25);

            mApp->PlayFoley(FoleyType::FOLEY_IGNITE);
            Reanimation *aFireReanim = mApp->AddReanimation(mPosX, mPosY, mRenderOrder + 1, ReanimationType::REANIM_JALAPENO_FIRE);
            aFireReanim->mAnimTime = 0.25f;
            aFireReanim->mAnimRate = 24.0f;
            aFireReanim->OverrideScale(0.7f, 0.4f);
            Die();
            return;
        }
        Zombie *aZombie = FindCollisionMindControlledTarget();
        if (aZombie) {
            if (aZombie->mOnHighGround && CantHitHighGround()) {
                return;
            }
            DoImpact(aZombie);
        }
        return;
    }

    // if ((mDamageRangeFlags & 1) == 0) { //TV的原版代码中存在这个，但是我这么写会导致仙人掌打不到气球。因此注释
    // return;
    // }

    Zombie *aZombie = FindCollisionTarget();
    if (aZombie) {
        if (aZombie->mOnHighGround && CantHitHighGround()) {
            return;
        }
        DoImpact(aZombie);
    } else {
        GridItem *aGridItem = FindCollisionTargetGridItem();
        if (aGridItem) {
            DoImpactGridItem(aGridItem);
        }
    }
}

bool Projectile::CantHitHighGround() const {
    if (mMotionType == ProjectileMotion::MOTION_BACKWARDS || mMotionType == ProjectileMotion::MOTION_HOMING)
        return false;

    return (mProjectileType == ProjectileType::PROJECTILE_PEA || mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || mProjectileType == ProjectileType::PROJECTILE_STAR
            || mProjectileType == ProjectileType::PROJECTILE_PUFF || mProjectileType == ProjectileType::PROJECTILE_FIREBALL || mProjectileType == ProjectileType::PROJECTILE_BOOMERANG
            || mProjectileType == ProjectileType::PROJECTILE_RED_STINGER)
        && !mOnHighGround;
}

bool Projectile::IsZombieHitBySplash(Zombie *theZombie) {
    Rect aProjectileRect = GetProjectileRect();
    if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
        aProjectileRect.mWidth = 100;
    }

    int aRowDeviation = theZombie->mRow - mRow;
    Rect aZombieRect = theZombie->GetZombieRect();
    if (theZombie->IsFireResistant() && mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
        return false;
    }

    if (theZombie->mZombieType == ZombieType::ZOMBIE_BOSS) {
        aRowDeviation = 0;
    }
    if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
        if (aRowDeviation != 0) {
            return false;
        }
    } else if (aRowDeviation > 1 || aRowDeviation < -1) {
        return false;
    }

    return theZombie->EffectedByDamage((unsigned int)mDamageRangeFlags) && GetRectOverlap(aProjectileRect, aZombieRect) >= 0;
}

bool Projectile::IsGridItemHitBySplash(GridItem *theGridItem) {
    if (theGridItem == nullptr || !theGridItem->IsPlantAttackTarget()) {
        return false;
    }

    Rect aProjectileRect = GetProjectileRect();
    int aRowDeviation = theGridItem->mGridY - mRow;
    if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL) {
        aProjectileRect.mWidth = 100;
        if (aRowDeviation != 0) {
            return false;
        }
    } else if (aRowDeviation > 1 || aRowDeviation < -1) {
        return false;
    }

    Rect aGridItemRect = theGridItem->GetItemRect();
    return GetRectOverlap(aProjectileRect, aGridItemRect) >= 0;
}

bool Projectile::IsSplashDamage(Zombie *theZombie) const {
    if (mProjectileType == ProjectileType::PROJECTILE_FIREBALL && theZombie && theZombie->IsFireResistant()) {
        return false;
    }

    return mProjectileType == ProjectileType::PROJECTILE_MELON || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON || mProjectileType == ProjectileType::PROJECTILE_FIREBALL;
}

unsigned int Projectile::GetDamageFlags(Zombie *theZombie) {
    unsigned int aDamageFlags = 0U;

    if (IsSplashDamage(theZombie)) {
        SetBit(aDamageFlags, (int)DamageFlags::DAMAGE_HITS_SHIELD_AND_BODY, true);
    } else if (mMotionType == ProjectileMotion::MOTION_LOBBED || mMotionType == ProjectileMotion::MOTION_BACKWARDS) {
        SetBit(aDamageFlags, (int)DamageFlags::DAMAGE_BYPASSES_SHIELD, true);
    } else if (mMotionType == ProjectileMotion::MOTION_STAR && mVelX < 0.0f) {
        SetBit(aDamageFlags, (int)DamageFlags::DAMAGE_BYPASSES_SHIELD, true);
    } else if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG && mReturning) {
        SetBit(aDamageFlags, (int)DamageFlags::DAMAGE_BYPASSES_SHIELD, true);
    }

    if (mProjectileType == ProjectileType::PROJECTILE_SNOWPEA || mProjectileType == ProjectileType::PROJECTILE_WINTERMELON) {
        SetBit(aDamageFlags, (int)DamageFlags::DAMAGE_FREEZE, true);
    }

    return aDamageFlags;
}

ProjectileDefinition &Projectile::GetProjectileDef() const {
    if (mProjectileType >= NUM_PROJECTILES) {
        return gExtendedProjectileDefinition[mProjectileType - NUM_PROJECTILES];
    }
    return gProjectileDefinition[(int)mProjectileType];
}

void Projectile::Draw(Graphics *g) {
    if (mProjectileType < NUM_PROJECTILES) {
        old_Projectile_Draw(this, g);
        return;
    }

    Graphics gProj(*g);
    // gProj.SetColorizeImages(true);
    // gProj.SetColor(mOverrideColor);

    const ProjectileDefinition &aProjectileDef = GetProjectileDef();

    Image *aImage = nullptr;
    float aScaleX = 1.0f;
    float aScaleY = 1.0f;
    if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_POLE) {
        aImage = addonImages.IMAGE_PROJECTILEPOLE;
    } else if (mProjectileType == ProjectileType::PROJECTILE_ZOMBIE_FIREBALL) {
        aImage = nullptr;
    } else if (mProjectileType == ProjectileType::PROJECTILE_ZOMBLOB) {
        aImage = addonImages.IMAGE_PROJECTILEZOMBLOB;
    } else if (mProjectileType == ProjectileType::PROJECTILE_SPORE) {
        aImage = addonImages.IMAGE_PROJECTILESPORE;
    } else if (mProjectileType == ProjectileType::PROJECTILE_BOOMERANG) {
        aScaleX = 0.8f;
        aScaleY = 0.5f;
        aImage = addonImages.IMAGE_PROJECTILEBOOMERANG;
    } else if (mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION) {
        aImage = addonImages.IMAGE_PROJECTILETELEPORTATION;
    } else if (mProjectileType == ProjectileType::PROJECTILE_TRAFFIC_CONE) {
        aScaleX = 0.8f;
        aScaleY = 0.8f;
        aImage = IMAGE_REANIM_ZOMBIE_CONE1;
    } else if (mProjectileType == ProjectileType::PROJECTILE_ACKEE) {
        aImage = addonImages.IMAGE_PROJECTILEACKEE;
    } else if (mProjectileType == ProjectileType::PROJECTILE_RED_STINGER) {
        aImage = addonImages.IMAGE_PROJECTILEREDSTINGER;
    }

    bool aMirror = false;
    if (mMotionType == ProjectileMotion::MOTION_BEE_BACKWARDS || (mProjectileType == ProjectileType::PROJECTILE_TELEPORTATION && mReturning)) {
        aMirror = true;
    }

    if (aImage) {
        int aCelWidth = aImage->GetCelWidth();
        int aCelHeight = aImage->GetCelHeight();
        Rect aSrcRect(aCelWidth * mFrame, aCelHeight * aProjectileDef.mImageRow, aCelWidth, aCelHeight);
        if (FloatApproxEqual(mRotation, 0.0f) && FloatApproxEqual(aScaleX, 1.0f) && FloatApproxEqual(aScaleY, 1.0f)) {
            Rect aDestRect(0, 0, aCelWidth, aCelHeight);
            gProj.DrawImageMirror(aImage, aDestRect, aSrcRect, aMirror);
        } else {
            float aOffsetX = mPosX + aCelWidth * 0.5f;
            float aOffsetY = mPosZ + mPosY + aCelHeight * 0.5f;
            float aWideScreenOffsetX = 240;
            float aWideScreenOffsetY = 80;
            if (aMirror) {
                aScaleX *= -1;
            }
            SexyTransform2D aTransform;
            TodScaleRotateTransformMatrix(aTransform, aOffsetX + mBoard->mX + aWideScreenOffsetX, aOffsetY + mBoard->mY + aWideScreenOffsetY, mRotation, aScaleX, aScaleY);
            TodBltMatrix(&gProj, aImage, aTransform, gProj.mClipRect, Color::White, gProj.mDrawMode, aSrcRect);
        }
    }

    if (mAttachmentID != AttachmentID::ATTACHMENTID_NULL) {
        Graphics theParticleGraphics(gProj);
        MakeParentGraphicsFrame(&theParticleGraphics);
        AttachmentDraw(mAttachmentID, &theParticleGraphics, false);
    }
}

void Projectile::DrawShadow(Graphics *g) {
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON)
        return;

    int aCelCol = 0;
    float aScale = 1.0f;
    float aStretch = 1.0f;
    float aOffsetX = mPosX - mX;
    float aOffsetY = mPosY - mY;

    int aGridX = mBoard->PixelToGridXKeepOnBoard(mX, mY);
    bool isHighGround = false;
    if (mBoard->mGridSquareType[aGridX][mRow] == GridSquareType::GRIDSQUARE_HIGH_GROUND) {
        isHighGround = true;
    }
    if (mOnHighGround && !isHighGround) {
        aOffsetY += HIGH_GROUND_HEIGHT;
    } else if (!mOnHighGround && isHighGround) {
        aOffsetY -= HIGH_GROUND_HEIGHT;
    }

    if (mBoard->StageIsNight()) {
        aCelCol = 1;
    }

    switch (mProjectileType) {
        case ProjectileType::PROJECTILE_PEA:
        case ProjectileType::PROJECTILE_ZOMBIE_PEA:
            aOffsetX += 3.0f;
            break;

        case ProjectileType::PROJECTILE_SNOWPEA:
            aOffsetX += -1.0f;
            aScale = 1.3f;
            break;

        case ProjectileType::PROJECTILE_STAR:
            aOffsetX += 7.0f;
            break;

        case ProjectileType::PROJECTILE_CABBAGE:
        case ProjectileType::PROJECTILE_KERNEL:
        case ProjectileType::PROJECTILE_BUTTER:
        case ProjectileType::PROJECTILE_MELON:
        case ProjectileType::PROJECTILE_WINTERMELON:
        case ProjectileType::PROJECTILE_SPORE:
        case ProjectileType::PROJECTILE_ACKEE:
            aOffsetX += 3.0f;
            aOffsetY += 10.0f;
            aScale = 1.6f;
            break;

        case ProjectileType::PROJECTILE_PUFF:
            return;

        case ProjectileType::PROJECTILE_COBBIG:
            aScale = 1.0f;
            aStretch = 3.0f;
            aOffsetX += 57.0f;
            break;

        case ProjectileType::PROJECTILE_FIREBALL:
        case ProjectileType::PROJECTILE_ZOMBIE_FIREBALL:
            aScale = 1.4f;
            break;

        case ProjectileType::PROJECTILE_BOOMERANG:
            aOffsetX += 17.0f;
            break;

        default:
            break;
    }

    if (mMotionType == ProjectileMotion::MOTION_LOBBED) {
        float aHeight = ClampFloat(-mPosZ, 0.0f, 200.0f);
        aScale *= 200.0f / (aHeight + 200.0f);
    }

    TodDrawImageCelScaledF(g, IMAGE_PEA_SHADOWS, aOffsetX, (mShadowY - mPosY + aOffsetY), aCelCol, 0, aScale * aStretch, aScale);
}
