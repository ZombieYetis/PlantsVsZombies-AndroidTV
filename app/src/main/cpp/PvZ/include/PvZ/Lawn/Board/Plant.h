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

#ifndef PVZ_LAWN_BOARD_PLANT_H
#define PVZ_LAWN_BOARD_PLANT_H

#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Misc/Rect.h"
#include "PvZ/Symbols.h"

inline constexpr int MAX_MAGNET_ITEMS = 5;

enum PlantSubClass {
    SUBCLASS_NORMAL = 0,
    SUBCLASS_SHOOTER = 1,
};

enum PlantWeapon {
    WEAPON_PRIMARY,
    WEAPON_SECONDARY,
};

enum PlantOnBungeeState {
    NOT_ON_BUNGEE,
    GETTING_GRABBED_BY_BUNGEE,
    RISING_WITH_BUNGEE,
};

enum PlantState // Prefix: STATE
{
    STATE_NOTREADY,
    STATE_READY,
    STATE_DOINGSPECIAL,
    STATE_SQUASH_LOOK,
    STATE_SQUASH_PRE_LAUNCH,
    STATE_SQUASH_RISING,
    STATE_SQUASH_FALLING,
    STATE_SQUASH_DONE_FALLING,
    STATE_GRAVEBUSTER_LANDING,
    STATE_GRAVEBUSTER_EATING,
    STATE_CHOMPER_BITING,
    STATE_CHOMPER_BITING_GOT_ONE,
    STATE_CHOMPER_BITING_MISSED,
    STATE_CHOMPER_DIGESTING,
    STATE_CHOMPER_SWALLOWING,
    STATE_POTATO_RISING,
    STATE_POTATO_ARMED,
    STATE_POTATO_MASHED,
    STATE_SPIKEWEED_ATTACKING,
    STATE_SPIKEWEED_ATTACKING_2,
    STATE_SCAREDYSHROOM_LOWERING,
    STATE_SCAREDYSHROOM_SCARED,
    STATE_SCAREDYSHROOM_RAISING,
    STATE_SUNSHROOM_SMALL,
    STATE_SUNSHROOM_GROWING,
    STATE_SUNSHROOM_BIG,
    STATE_MAGNETSHROOM_SUCKING,
    STATE_MAGNETSHROOM_CHARGING,
    STATE_BOWLING_UP,
    STATE_BOWLING_DOWN,
    STATE_CACTUS_LOW,
    STATE_CACTUS_RISING,
    STATE_CACTUS_HIGH,
    STATE_CACTUS_LOWERING,
    STATE_TANGLEKELP_GRABBING,
    STATE_COBCANNON_ARMING,
    STATE_COBCANNON_LOADING,
    STATE_COBCANNON_READY = 37,
    STATE_COBCANNON_FIRING,
    STATE_KERNELPULT_BUTTER,
    STATE_UMBRELLA_TRIGGERED,
    STATE_UMBRELLA_REFLECTING,
    STATE_IMITATER_MORPHING,
    STATE_ZEN_GARDEN_WATERED,
    STATE_ZEN_GARDEN_NEEDY,
    STATE_ZEN_GARDEN_HAPPY,
    STATE_MARIGOLD_ENDING,
    STATE_FLOWERPOT_INVULNERABLE,
    STATE_LILYPAD_INVULNERABLE,
    STATE_CELERY_STALKER_LOW,
    STATE_CELERY_STALKER_RISING,
    STATE_CELERY_STALKER_HIGH,
    STATE_CELERY_STALKER_LOWERING,
    STATE_CELERY_STALKER_ATTACKING,
    STATE_CELERY_STALKER_PUNCHING,
    STATE_CELERY_STALKER_STOPPING,
    STATE_BLOOMERANG_CATCHING,
    STATE_BONK_CHOY_PUNCHING,
    STATE_BONK_CHOY_UPPERCUTTING,
    STATE_ENDURIAN_STARTING,
    STATE_ENDURIAN_ATTACKING,
    STATE_ENDURIAN_STOPPING
};

enum PLANT_LAYER {
    PLANT_LAYER_BELOW = -1,
    PLANT_LAYER_MAIN,
    PLANT_LAYER_REANIM,
    PLANT_LAYER_REANIM_HEAD,
    PLANT_LAYER_REANIM_BLINK,
    PLANT_LAYER_ON_TOP,
    NUM_PLANT_LAYERS,
};

enum PLANT_ORDER {
    PLANT_ORDER_LILYPAD,
    PLANT_ORDER_NORMAL,
    PLANT_ORDER_PUMPKIN,
    PLANT_ORDER_FLYER,
    PLANT_ORDER_CHERRYBOMB,
};

enum MagnetItemType {
    MAGNET_ITEM_NONE,
    MAGNET_ITEM_PAIL_1,
    MAGNET_ITEM_PAIL_2,
    MAGNET_ITEM_PAIL_3,
    MAGNET_ITEM_FOOTBALL_HELMET_1,
    MAGNET_ITEM_FOOTBALL_HELMET_2,
    MAGNET_ITEM_FOOTBALL_HELMET_3,
    MAGNET_ITEM_DOOR_1,
    MAGNET_ITEM_DOOR_2,
    MAGNET_ITEM_DOOR_3,
    // MAGNET_ITEM_PROPELLER,
    MAGNET_ITEM_POGO_1,
    MAGNET_ITEM_POGO_2,
    MAGNET_ITEM_POGO_3,
    MAGNET_ITEM_JACK_IN_THE_BOX,
    MAGNET_ITEM_LADDER_1,
    MAGNET_ITEM_LADDER_2,
    MAGNET_ITEM_LADDER_3,
    MAGNET_ITEM_LADDER_PLACED,
    MAGNET_ITEM_SILVER_COIN,
    MAGNET_ITEM_GOLD_COIN,
    MAGNET_ITEM_DIAMOND,
    MAGNET_ITEM_PICK_AXE
};

class MagnetItem {
public:
    float mPosX;              //+0x0
    float mPosY;              //+0x4
    float mDestOffsetX;       //+0x8
    float mDestOffsetY;       //+0xC
    MagnetItemType mItemType; //+0x10
};

class Zombie;
class GridItem;
class TodParticleSystem;

class Plant : public GameObject {
public:
    SeedType mSeedType;                        // 13
    int mPlantCol;                             // 14
    int mAnimCounter;                          // 15
    int mFrame;                                // 16
    int mFrameLength;                          // 17
    int mNumFrames;                            // 18
    PlantState mState;                         // 19
    int mPlantHealth;                          // 20
    int mPlantMaxHealth;                       // 21
    int mSubclass;                             // 22
    int mDisappearCountdown;                   // 23
    int mDoSpecialCountdown;                   // 24
    int mStateCountdown;                       // 25
    int mLaunchCounter;                        // 26
    int mLaunchRate;                           // 27
    Sexy::Rect mPlantRect;                     // 28 ~ 31
    Sexy::Rect mPlantAttackRect;               // 32 ~ 35
    int mTargetX;                              // 36
    int mTargetY;                              // 37
    int mStartRow;                             // 38
    int *mParticleID;                          // 39
    int mShootingCounter;                      // 40
    ReanimationID mBodyReanimID;               // 41
    ReanimationID mHeadReanimID;               // 42
    ReanimationID mHeadReanimID2;              // 43
    ReanimationID mHeadReanimID3;              // 44
    ReanimationID mBlinkReanimID;              // 45
    ReanimationID mLightReanimID;              // 46
    ReanimationID mSleepingReanimID;           // 47
    int mBlinkCountdown;                       // 48
    int mRecentlyEatenCountdown;               // 49
    int mEatenFlashCountdown;                  // 50
    int mBeghouledFlashCountdown;              // 51
    float mShakeOffsetX;                       // 52
    float mShakeOffsetY;                       // 53
    MagnetItem mMagnetItems[MAX_MAGNET_ITEMS]; // 54 ~ 78
    ZombieID mTargetZombieID;                  // 79
    int mWakeUpCounter;                        // 80
    PlantOnBungeeState mOnBungeeState;         // 81
    SeedType mImitaterType;                    // 82
    int mPottedPlantIndex;                     // 83
    bool mAnimPing;                            // 336
    bool mDead;                                // 337
    bool mSquished;                            // 338
    bool mIsAsleep;                            // 339
    bool mIsOnBoard;                           // 340
    bool mHighlighted;                         // 341
    bool mInFlowerPot;                         // 342
    bool mGloveGrabbed;                        // 343
    int unk;                                   // 86
    // 大小87个整数

    void UpdateReanim() {
        reinterpret_cast<void (*)(Plant *)>(Plant_UpdateReanimAddr)(this);
    };
    bool IsPartOfUpgradableTo(SeedType theUpgradedType) {
        return reinterpret_cast<bool (*)(Plant *, SeedType)>(Plant_IsPartOfUpgradableToAddr)(this, theUpgradedType);
    }
    void DrawMagnetItems(Sexy::Graphics *g) {
        reinterpret_cast<void (*)(Plant *, Sexy::Graphics *)>(Plant_DrawMagnetItemsAddr)(this, g);
    }
    void RemoveEffects() {
        reinterpret_cast<void (*)(Plant *)>(Plant_RemoveEffectsAddr)(this);
    }
    void DoRowAreaDamage(int theDamage, unsigned int theDamageFlags);
    void StarFruitFire() {
        reinterpret_cast<void (*)(Plant *)>(Plant_StarFruitFireAddr)(this);
    }
    void GetPeaHeadOffset(int &theOffsetX, int &theOffsetY) {
        reinterpret_cast<void (*)(Plant *, int &, int &)>(Plant_GetPeaHeadOffsetAddr)(this, theOffsetX, theOffsetY);
    }
    TodParticleSystem *AddAttachedParticle(int thePosX, int thePosY, int theRenderPosition, ParticleEffect theEffect) {
        return reinterpret_cast<TodParticleSystem *(*)(Plant *, int, int, int, ParticleEffect)>(Plant_AddAttachedParticleAddr)(this, thePosX, thePosY, theRenderPosition, theEffect);
    }
    void LaunchThreepeater() {
        reinterpret_cast<void (*)(Plant *)>(Plant_LaunchThreepeaterAddr)(this);
    }
    void LaunchStarFruit() {
        reinterpret_cast<void (*)(Plant *)>(Plant_LaunchStarFruitAddr)(this);
    }
    bool IsUpgradableTo(SeedType theUpgradedType) {
        return reinterpret_cast<bool (*)(Plant *, SeedType)>(Plant_IsUpgradableToAddr)(this, theUpgradedType);
    }
    void MagnetShroomAttactItem(Zombie *theZombie) {
        reinterpret_cast<void (*)(Plant *, Zombie *)>(Plant_MagnetShroomAttactItemAddr)(this, theZombie);
    }
    MagnetItem *GetFreeMagnetItem() {
        return reinterpret_cast<MagnetItem *(*)(Plant *)>(Plant_GetFreeMagnetItemAddr)(this);
    }
    void DoSquashDamage() {
        return reinterpret_cast<void (*)(Plant *)>(Plant_DoSquashDamageAddr)(this);
    }
    Zombie *FindSquashTarget() {
        return reinterpret_cast<Zombie *(*)(Plant *)>(Plant_FindSquashTargetAddr)(this);
    }
    void UpdateNeedsFood() {
        reinterpret_cast<void (*)(Plant *)>(Plant_UpdateNeedsFoodAddr)(this);
    }
    void AnimateNuts() {
        reinterpret_cast<void (*)(Plant *)>(Plant_AnimateNutsAddr)(this);
    }
    void AnimateGarlic() {
        reinterpret_cast<void (*)(Plant *)>(Plant_AnimateGarlicAddr)(this);
    }
    void AnimatePumpkin() {
        reinterpret_cast<void (*)(Plant *)>(Plant_AnimatePumpkinAddr)(this);
    }
    void UpdateBlink() {
        reinterpret_cast<void (*)(Plant *)>(Plant_UpdateBlinkAddr)(this);
    }

    void PlantInitialize(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int a6);
    void Update();
    void UpdateAbilities();
    void UpdateBowling();
    void ImitaterMorph();
    void Squish();
    void Draw(Sexy::Graphics *g);
    void DrawShadow(Sexy::Graphics *g, float theOffsetX, float theOffsetY);
    void KillAllPlantsNearDoom();
    void DoSpecial();
    void DoSpecial_Origin();
    void IceAZombie(Zombie *theZombie);
    Zombie *FindTargetZombie(int theRow, PlantWeapon thePlantWeapon);
    GridItem *FindTargetGridItem(int theRow, PlantWeapon thePlantWeapon);
    void Die();
    void Die_Origin();
    static Sexy::Image *GetImage(SeedType theSeedType);
    static pvzstl::string GetNameString(SeedType theSeedType, SeedType theImitaterType);
    static pvzstl::string GetToolTip(SeedType theSeedType);
    static int GetCost(SeedType theSeedType, SeedType theImitaterType);
    static int GetRefreshTime(SeedType theSeedType, SeedType theImitaterType);
    static bool IsNocturnal(SeedType theSeedType);
    static bool IsAquatic(SeedType theSeedType);
    static bool IsFlying(SeedType theSeedType);
    static bool IsLobber(SeedType theSeedType);
    static bool IsUpgrade(SeedType theSeedType);
    static bool IsDefender(SeedType theSeedType);
    int GetDamageRangeFlags(PlantWeapon thePlantWeapon) const;
    Sexy::Rect GetPlantRect();
    Sexy::Rect GetPlantAttackRect(PlantWeapon thePlantWeapon);
    bool NotOnGround() const;
    static void DrawSeedType(Sexy::Graphics *g, SeedType theSeedType, SeedType theImitaterType, DrawVariation theDrawVariation, float thePosX, float thePosY);
    void SetSleeping(bool theIsAsleep);
    void UpdateReanimColor();
    bool IsOnBoard() const;
    bool IsOnHighGround();
    bool IsInPlay();
    void Animate();
    void AnimateCeleryStalker();
    void AnimateSweetPotato();
    void AnimatePeanut();
    void AnimateEndurian();
    void PlayBodyReanim(const char *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);
    void SpikeweedAttack();
    void SpikeRockTakeDamage();
    bool IsSpiky() const;
    bool IsCeleryStalkerLow() const;
    bool IsLowProfile() const;
    bool DrawMagnetItemsOnTop();
    void SetImitaterFilterEffect();
    void BurnRow(int theRow);
    void BlowAwayFliers(int theX, int theRow);
    bool MakesSun() const;
    void UpdateProductionPlant();
    void UpdateShooting();
    void UpdateShooter();
    void LaunchPeanut();
    void CobCannonFire(int theTargetX, int theTargetY);
    void CobCannonFire_Origin(int theTargetX, int theTargetY);
    void UpdateCobCannon();
    void SyncCobCannonState();
    void ApplyCobCannonState(PlantState theState, int theCountdown, int theShootingCounter, float theAnimTime);
    void Fire(Zombie *theTargetZombie, int theRow, PlantWeapon thePlantWeapon, GridItem *theTargetGridItem);
    void Fire_Origin(Zombie *theTargetZombie, int theRow, PlantWeapon thePlantWeapon, GridItem *theTargetGridItem);
    void PlayIdleAnim(float theRate);
    void IceZombies();
    static bool IsDisposable(SeedType theSeedType);
    bool IsInvulnerable();
    bool FindTargetAndFire(int theRow, PlantWeapon thePlantWeapon);
    ReanimationID GetPlantReanimationIDByIndex(int index) const;
    void SyncAnimationToClient();
    void SyncPingPongAnimationToClient();
    void UpdateChomper();
    void UpdateGraveBuster();
    void UpdateMagnetShroom();
    void UpdateSquash();
    void UpdateIcebergLettuce();
    void UpdateCeleryStalker();
    void UpdateBonkChoy();
    void UpdateSweetPotato();
    void UpdateEndurian();
    bool HasActiveBoomerang();
    void UpdateBloomerang();
    int CalcRenderOrder();
};

inline float PlantDrawHeightOffset(Board *theBoard, Plant *thePlant, SeedType theSeedType, int theCol, int theRow) {
    return reinterpret_cast<float (*)(Board *, Plant *, SeedType, int, int)>(PlantDrawHeightOffsetAddr)(theBoard, thePlant, theSeedType, theCol, theRow);
}
inline float PlantFlowerPotHeightOffset(SeedType theSeedType, float theFlowerPotScale) {
    return reinterpret_cast<float (*)(SeedType, float)>(PlantFlowerPotHeightOffsetAddr)(theSeedType, theFlowerPotScale);
}

class PlantDefinition {
public:
    SeedType mSeedType;               //+0x0
    Sexy::Image **mPlantImage;        //+0x4
    ReanimationType mReanimationType; //+0x8
    int mPacketIndex;                 //+0xC
    int mSeedCost;                    //+0x10
    int mRefreshTime;                 //+0x14
    PlantSubClass mSubClass;          //+0x18
    int mLaunchRate;                  //+0x1C
    const char *mPlantName;           //+0x20
};
extern PlantDefinition gPlantDefs[SeedType::NUM_SEED_TYPES];
extern PlantDefinition gExtendedPlantDefs[];

PlantDefinition &GetPlantDefinition(SeedType theSeedType);
/***************************************************************************************************************/

inline bool abilityFastCoolDown;
inline bool mushroomsNoSleep;
inline bool showPlantHealth;
inline bool showNutGarlicSpikeHealth;


inline void (*old_Plant_Draw)(Plant *plant, Sexy::Graphics *graphics);

inline int (*old_Plant_GetRefreshTime)(SeedType theSeedType, SeedType theImitaterType);

inline int (*old_Plant_GetCost)(SeedType theSeedType, SeedType theImitaterType);

inline void (*old_Plant_Update)(Plant *plant);

inline void (*old_Plant_UpdateAbilities)(Plant *);

inline void (*old_Plant_SetSleeping)(Plant *a, bool a2);

inline void (*old_Plant_UpdateReanimColor)(Plant *a);

inline void (*old_Plant_PlantInitialize)(Plant *plant, int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int a6);

inline bool (*old_Plant_IsUpgrade)(SeedType theSeedType);

inline void (*old_Plant_PlayBodyReanim)(Plant *, const char *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);

inline void (*old_Plant_UpdateProductionPlant)(Plant *);

inline void (*old_Plant_UpdateShooter)(Plant *);

inline void (*old_Plant_DoRowAreaDamage)(Plant *, int, unsigned int);

inline bool (*old_Plant_FindTargetAndFire)(Plant *, int, PlantWeapon);

inline int (*old_Plant_CobCannonFire)(Plant *plant, int x, int y);
int Plant_CobCannonFire(Plant *plant, int x, int y);

#endif // PVZ_LAWN_BOARD_PLANT_H
