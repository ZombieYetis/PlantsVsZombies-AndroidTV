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

#ifndef PVZ_LAWN_BOARD_PROJECTILE_H
#define PVZ_LAWN_BOARD_PROJECTILE_H

#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/SexyAppFramework/Misc/Rect.h"
#include "PvZ/Symbols.h"

inline constexpr int MAX_PIERCE_HIT_COUNT = 3;

class Plant;
class Zombie;
class GridItem;

class ProjectileDefinition {
public:
    ProjectileType mProjectileType;
    int mImageRow;
    int mDamage;
};
extern ProjectileDefinition gProjectileDefinition[NUM_PROJECTILES];
extern ProjectileDefinition gExtendedProjectileDefinition[EXTENDED_NUM_PROJECTILES - NUM_PROJECTILES];

class Projectile : public GameObject {
public:
    int mFrame;                     // 13
    int mNumFrames;                 // 14
    int mAnimCounter;               // 15
    float mPosX;                    // 16
    float mPosY;                    // 17
    float mPosZ;                    // 18
    float mVelX;                    // 19
    float mVelY;                    // 20
    float mVelZ;                    // 21
    float mAccZ;                    // 22
    float mShadowY;                 // 23
    bool mDead;                     // 96
    int mAnimTicksPerFrame;         // 25
    ProjectileMotion mMotionType;   // 26
    ProjectileType mProjectileType; // 27
    int mProjectileAge;             // 28
    int mClickBackoffCounter;       // 29
    float mRotation;                // 30
    float mRotationSpeed;           // 31
    bool mOnHighGround;             // 128
    int mDamageRangeFlags;          // 33
    int mHitTorchwoodGridX;         // 34
    AttachmentID mAttachmentID;     // 35
    float mCobTargetX;              // 36
    int mCobTargetRow;              // 37
    ZombieID mTargetZombieID;       // 38
    int mLastPortalX;               // 39
    int mPierceHitCount = 0;
    ZombieID mHitZombieIDs[MAX_PIERCE_HIT_COUNT];
    GridItemID mHitGridItemIDs[MAX_PIERCE_HIT_COUNT];
    PlantID mRelatedPlantID = PlantID::PLANTID_NULL;
    bool mReturning = false;

    Projectile() {
        _constructor();
    }
    ~Projectile() {
        _destructor();
    }

    void Die() {
        reinterpret_cast<void (*)(Projectile *)>(Projectile_DieAddr)(this);
    }

    void DoSplashDamage(Zombie *theZombie, GridItem *theGridItem);
    void ProjectileInitialize(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType);
    void ConvertToFireball(int theGridX);
    void ConvertToZombieFireball();
    void ConvertToPea(int theGridX);
    void Update();
    void UpdateMotion();
    void UpdateNormalMotion();
    void UpdateLobMotion();
    void UpdateAckeeMotion();
    void UpdateBoomerang();
    void BoomerangReturn();
    void DoImpact(Zombie *theZombie);
    void DoImpactGridItem(GridItem *theGridItem);
    void SpawnSporeShroom(int theGridX, int theGridY);
    void CheckForCollision();
    Zombie *FindCollisionMindControlledTarget();
    GridItem *FindCollisionTargetGridItem();
    ProjectileDefinition &GetProjectileDef() const;
    Sexy::Rect GetProjectileRect();
    Plant *FindCollisionTargetPlant();
    bool CantHitHighGround() const;
    bool IsSplashDamage(Zombie *theZombie) const;
    void PlayImpactSound(Zombie *theZombie);
    unsigned int GetDamageFlags(Zombie *theZombie);
    bool PeaAboutToHitTorchwood();
    Zombie *FindCollisionTarget();
    bool IsZombieHitBySplash(Zombie *theZombie);
    bool IsGridItemHitBySplash(GridItem *theGridItem);
    void Draw(Sexy::Graphics *g);
    void DrawShadow(Sexy::Graphics *g);

protected:
    friend void InitHookFunction();

    void _constructor() {
        reinterpret_cast<void (*)(Projectile *)>(Projectile_ProjectileCtorAddr)(this);
    }
    void _destructor() {
        reinterpret_cast<void (*)(Projectile *)>(Projectile_ProjectileDtorAddr)(this);
    }
};

/***************************************************************************************************************/
// 随机子弹
inline bool randomBullet;
inline int bulletSpinnerChosenNum = -1;
inline bool isOnlyPeaUseable;
inline bool banCobCannon;
inline bool banStar;
inline bool isOnlyTouchFireWood;
inline bool ColdPeaCanPassFireWood;


// inline Plant *(*Projectile_FindCollisionTargetPlant)(Projectile *a1);
//
// inline Zombie *(*Projectile_FindCollisionTarget)(Projectile *a);
//
// inline GridItem *(*Projectile_FindCollisionTargetGridItem)(Projectile *a);


inline void (*old_Projectile_ConvertToPea)(Projectile *projectile, int aGridX);

inline void (*old_Projectile_Draw)(Projectile *, Sexy::Graphics *);

inline void (*old_Projectile_DrawShadow)(Projectile *, Sexy::Graphics *);

inline void (*old_Projectile_UpdateNormalMotion)(Projectile *);

#endif // PVZ_LAWN_BOARD_PROJECTILE_H
