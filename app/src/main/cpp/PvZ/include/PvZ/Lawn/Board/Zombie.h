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

#ifndef PVZ_LAWN_BOARD_ZOMBIE_H
#define PVZ_LAWN_BOARD_ZOMBIE_H

#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Common/GameConstants.h"
#include "PvZ/SexyAppFramework/Graphics/Color.h"
#include "PvZ/SexyAppFramework/Misc/Rect.h"
#include "PvZ/Symbols.h"

inline constexpr int MAX_ZOMBIE_FOLLOWERS = 4;
inline constexpr int NUM_BOBSLED_FOLLOWERS = 3;
inline constexpr int NUM_BACKUP_DANCERS = 4;
inline constexpr int NUM_BOSS_BUNGEES = 3;

inline constexpr int MAX_DEAD_FOLLOWERS = 15;
inline constexpr int ProductorZombieLaunchRate = 2500;

inline constexpr int ZOMBIE_START_RANDOM_OFFSET = 40;
inline constexpr int BUNGEE_ZOMBIE_HEIGHT = 3000;
inline constexpr int RENDER_GROUP_SHIELD = 1;
inline constexpr int RENDER_GROUP_ARMS = 2;
inline constexpr int RENDER_GROUP_OVER_SHIELD = 3;
inline constexpr int RENDER_GROUP_BOSS_BACK_LEG = 4;
inline constexpr int RENDER_GROUP_BOSS_FRONT_LEG = 5;
inline constexpr int RENDER_GROUP_BOSS_BACK_ARM = 6;
inline constexpr int RENDER_GROUP_BOSS_FIREBALL_ADDITIVE = 7;
inline constexpr int RENDER_GROUP_BOSS_FIREBALL_TOP = 8;
inline constexpr int RENDER_GROUP_GIGA_LIGHTNING = 9;
inline constexpr int ZOMBIE_LIMP_SPEED_FACTOR = 2;
inline constexpr int POGO_BOUNCE_TIME = 80;
inline constexpr int DOLPHIN_JUMP_TIME = 120;
inline constexpr int JackInTheBoxZombieRadius = 115;
inline constexpr int JackInTheBoxPlantRadius = 90;
inline constexpr int SuperFanImpZombieRadius = 35;
inline constexpr int SUPER_FAN_IMP_POP_DAMAGE = 301;
inline constexpr int GIGA_IMP_POP_DAMAGE = 151;
inline constexpr int BOBSLED_CRASH_TIME = 150;
inline constexpr int ZOMBIE_BACKUP_DANCER_RISE_HEIGHT = -200;
inline constexpr int BOSS_FLASH_HEALTH_FRACTION = 10;
inline constexpr int TICKS_BETWEEN_EATS = 8;
inline constexpr int DAMAGE_PER_EAT = TICKS_BETWEEN_EATS;
inline constexpr float THOWN_ZOMBIE_GRAVITY = 0.05f;
inline constexpr float KICKED_ZOMBIE_GRAVITY = 0.05f * 0.8f;
inline constexpr float CHILLED_SPEED_FACTOR = 0.4f;
inline constexpr float CLIP_HEIGHT_LIMIT = -100.0f;
inline constexpr float CLIP_HEIGHT_OFF = -200.0f;
inline constexpr float FLYER_ALTITUDE = 25.0f;
inline constexpr Sexy::Color ZOMBIE_MINDCONTROLLED_COLOR = Sexy::Color(128, 0, 192, 255);
inline constexpr Sexy::Color ZOMBIE_REVIVED_COLOR = Sexy::Color(135, 206, 250, 180);
inline constexpr int ZOMBIE_POISONING_TIME = 300;

enum ZombieAttackType {
    ATTACKTYPE_CHEW,
    ATTACKTYPE_DRIVE_OVER,
    ATTACKTYPE_VAULT,
    ATTACKTYPE_LADDER,
    ATTACKTYPE_POLE,
};

enum ZombieParts {
    PART_BODY,
    PART_HEAD,
    PART_HEAD_EATING,
    PART_TONGUE,
    PART_ARM,
    PART_HAIR,
    PART_HEAD_YUCKY,
    PART_ARM_PICKAXE,
    PART_ARM_POLEVAULT,
    PART_ARM_LEASH,
    PART_ARM_FLAG,
    PART_POGO,
    PART_DIGGER
};

class ZombieDrawPosition {
public:
    int mHeadX;
    int mHeadY;
    int mArmY;
    float mBodyY;
    float mImageOffsetX;
    float mImageOffsetY;
    float mClipHeight;
};

class Coin;
class GridItem;
class Plant;
class Reanimation;
class TodParticleSystem;
class Zombie : public GameObject {
public:
    enum {
        ZOMBIE_WAVE_DEBUG = -1,
        ZOMBIE_WAVE_CUTSCENE = -2,
        ZOMBIE_WAVE_UI = -3,
        ZOMBIE_WAVE_WINNER = -4,
        ZOMBIE_WAVE_VS = -5,
    };

    static std::vector<ZombieType> msDeadFollowers;

    ZombieType mZombieType;                           // 13
    ZombiePhase mZombiePhase;                         // 14
    float mPosX;                                      // 15
    float mPosY;                                      // 16
    float mVelX;                                      // 17
    int mAnimCounter;                                 // 18
    int mGroanCounter;                                // 19
    int mAnimTicksPerFrame;                           // 20
    int mAnimFrames;                                  // 21
    int mFrame;                                       // 22
    int mPrevFrame;                                   // 23
    bool mVariant;                                    // 96
    bool mIsEating;                                   // 97
    short mSquashHeadCol;                             // 新增成员，用于窝瓜僵尸修复
    int mJustGotShotCounter;                          // 25
    int mShieldJustGotShotCounter;                    // 26
    int mShieldRecoilCounter;                         // 27
    int mZombieAge;                                   // 28
    ZombieHeight mZombieHeight;                       // 29
    int mPhaseCounter;                                // 30
    int mFromWave;                                    // 31
    bool mDroppedLoot;                                // 128
    bool mIsRevived;                                  // 新增成员，用于复生僵尸着色
    bool mCanRevived;                                 // 新增成员，用于计入复生池
    bool mButtered;                                   // 新增成员，用于史莱姆黄油化
    int mZombieFade;                                  // 33
    bool mFlatTires;                                  // 136
    bool mPoisoned;                                   // 新增成员，判定僵尸是否中毒
    short mPoisonedCounter;                           // 新增成员，中毒死亡倒计时
    int mUseLadderCol;                                // 35
    int mTargetCol;                                   // 36
    float mAltitude;                                  // 37
    bool mHitUmbrella;                                // 152
    short mSunBeanSun;                                // 阳光豆尚可掉落的阳光总量
    Sexy::Rect mZombieRect;                           // 39 ~ 42
    Sexy::Rect mZombieAttackRect;                     // 43 ~ 46
    int mChilledCounter;                              // 47
    int mButteredCounter;                             // 48
    int mIceTrapCounter;                              // 49
    bool mMindControlled;                             // 200
    bool mBlowingAway;                                // 201
    bool mHasHead;                                    // 202
    bool mHasArm;                                     // 203
    bool mHasObject;                                  // 204
    bool mInPool;                                     // 205
    bool mOnHighGround;                               // 206
    bool mYuckyFace;                                  // 207
    int mYuckyFaceCounter;                            // 52
    HelmType mHelmType;                               // 53
    int mBodyHealth;                                  // 54
    int mBodyMaxHealth;                               // 55
    int mHelmHealth;                                  // 56
    int mHelmMaxHealth;                               // 57
    ShieldType mShieldType;                           // 58
    int mShieldHealth;                                // 59
    int mShieldMaxHealth;                             // 60
    int mFlyingHealth;                                // 61
    int mFlyingMaxHealth;                             // 62
    bool mDead;                                       // 252
    short mSunBeanDamageRemainder;                    // 尚未兑换成阳光的伤害
    ZombieID mRelatedZombieID;                        // 64
    ZombieID mFollowerZombieID[MAX_ZOMBIE_FOLLOWERS]; // 65 ~ 68
    bool mPlayingSong;                                // 276
    int mParticleOffsetX;                             // 70
    int mParticleOffsetY;                             // 71
    AttachmentID mAttachmentID;                       // 72
    int mSummonCounter;                               // 73
    ReanimationID mBodyReanimID;                      // 74
    float mScaleZombie;                               // 75
    float mVelZ;                                      // 76
    float mOriginalAnimRate;                          // 77
    PlantID mTargetPlantID;                           // 78
    int mBossMode;                                    // 79
    int mTargetRow;                                   // 80
    int mBossBungeeCounter;                           // 81
    int mBossStompCounter;                            // 82
    int mBossHeadCounter;                             // 83
    ReanimationID mBossFireBallReanimID;              // 84
    ReanimationID mSpecialHeadReanimID;               // 85
    int mFireballRow;                                 // 86
    bool mIsFireBall;                                 // 348
    ReanimationID mMoweredReanimID;                   // 88
    int mLastPortalX;                                 // 89
    bool mBloated;                                    // 360
    int mUnk91;                                       // 91
    bool mWalnutDeath;                                // 92
    int mUnk93;                                       // 93
    int mUnk94;                                       // 94
    int mUnk95;                                       // 95
    int mUnk96;                                       // 96
    // 大小97个整数

    void RemoveColdEffects() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_RemoveColdEffectsAddr)(this);
    }
    TodParticleSystem *AddAttachedParticle(int thePosX, int thePosY, ParticleEffect theEffect) {
        return reinterpret_cast<TodParticleSystem *(*)(Zombie *, int, int, ParticleEffect)>(Zombie_AddAttachedParticleAddr)(this, thePosX, thePosY, theEffect);
    }
    static void SetupShieldReanims(ZombieType theZombieType, Reanimation *aReanim) {
        reinterpret_cast<void (*)(ZombieType, Reanimation *)>(Zombie_SetupShieldReanimsAddr)(theZombieType, aReanim);
    }
    void GetTrackPosition(const char *theTrackName, float &thePosX, float &thePosY) {
        reinterpret_cast<void (*)(Zombie *, const char *, float &, float &)>(Zombie_GetTrackPositionAddr)(this, theTrackName, thePosX, thePosY);
    }
    void ApplyAnimRate(float theAnimRate) {
        reinterpret_cast<void (*)(Zombie *, float)>(Zombie_ApplyAnimRateAddr)(this, theAnimRate);
    }
    void UpdateAnimSpeed();
    void UpdateZombiePosition() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateZombiePositionAddr)(this);
    };
    void LoadReanim(ReanimationType theReanimationType) {
        reinterpret_cast<void (*)(Zombie *, ReanimationType)>(Zombie_LoadReanimAddr)(this, theReanimationType);
    }
    void LoadPlainZombieReanim() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_LoadPlainZombieReanimAddr)(this);
    }
    void ReanimIgnoreClipRect(const char *theTrackName, bool theIgnoreClipRect) {
        reinterpret_cast<void (*)(Zombie *, const char *, bool)>(Zombie_ReanimIgnoreClipRectAddr)(this, theTrackName, theIgnoreClipRect);
    }
    void SetAnimRate(float theAnimRate) {
        reinterpret_cast<void (*)(Zombie *, float)>(Zombie_SetAnimRateAddr)(this, theAnimRate);
    }
    void DrawDancerReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos) {
        reinterpret_cast<void (*)(Zombie *, Sexy::Graphics *, ZombieDrawPosition &)>(Zombie_DrawDancerReanimAddr)(this, g, theDrawPos);
    }
    void DrawBobsledReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, bool theBeforeZombie) {
        reinterpret_cast<void (*)(Zombie *, Sexy::Graphics *, ZombieDrawPosition &, bool)>(Zombie_DrawBobsledReanimAddr)(this, g, theDrawPos, theBeforeZombie);
    }
    void DrawBungeeReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos) {
        reinterpret_cast<void (*)(Zombie *, Sexy::Graphics *, ZombieDrawPosition &)>(Zombie_DrawBungeeReanimAddr)(this, g, theDrawPos);
    }
    void RemoveIceTrap() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_RemoveIceTrapAddr)(this);
    }
    void BungeeDropPlant() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_BungeeDropPlantAddr)(this);
    }
    void BalloonPropellerHatSpin(bool theSpinning) {
        reinterpret_cast<void (*)(Zombie *, bool)>(Zombie_BalloonPropellerHatSpinAddr)(this, theSpinning);
    }
    void BobsledBurn() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_BobsledBurnAddr)(this);
    }
    void ShowYuckyFace(bool theShow) {
        reinterpret_cast<void (*)(Zombie *, bool)>(Zombie_ShowYuckyFaceAddr)(this, theShow);
    }
    void OverrideParticleColor(TodParticleSystem *theParticle) {
        reinterpret_cast<void (*)(Zombie *, TodParticleSystem *)>(Zombie_OverrideParticleColorAddr)(this, theParticle);
    }
    void OverrideParticleScale(TodParticleSystem *theParticle) {
        reinterpret_cast<void (*)(Zombie *, TodParticleSystem *)>(Zombie_OverrideParticleScaleAddr)(this, theParticle);
    }
    void PoolSplash(bool theInToPoolSound) {
        reinterpret_cast<void (*)(Zombie *, bool)>(Zombie_PoolSplashAddr)(this, theInToPoolSound);
    }
    Reanimation *AddAttachedReanim(int thePosX, int thePosY, ReanimationType theReanimType) {
        return reinterpret_cast<Reanimation *(*)(Zombie *, int, int, ReanimationType)>(Zombie_AddAttachedReanimAddr)(this, thePosX, thePosY, theReanimType);
    }
    void SetupWaterTrack(const char *theTrackName) {
        reinterpret_cast<void (*)(Zombie *, const char *)>(Zombie_SetupWaterTrackAddr)(this, theTrackName);
    }
    void RemoveButter() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_RemoveButterAddr)(this);
    }
    void CheckForPool() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_CheckForPoolAddr)(this);
    }
    void CheckForHighGround() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_CheckForHighGroundAddr)(this);
    }
    void UpdateBoss() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateBossAddr)(this);
    }
    void PogoBreak(unsigned int theDamageFlags) {
        reinterpret_cast<void (*)(Zombie *, unsigned int)>(Zombie_PogoBreakAddr)(this, theDamageFlags);
    }
    void UpdateBurn() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateBurnAddr)(this);
    }
    void UpdateMowered() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateMoweredAddr)(this);
    }
    void UpdateZombieChimney() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateZombieChimneyAddr)(this);
    }
    void UpdateZombieWalkingIntoHouse() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateZombieWalkingIntoHouseAddr)(this);
    }
    void BungeeStealTarget() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_BungeeStealTargetAddr)(this);
    }
    void BungeeLiftTarget() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_BungeeLiftTargetAddr)(this);
    }
    void AnimateChewEffect() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_AnimateChewEffectAddr)(this);
    }
    void ZombieCatapultFire(Plant *plant) {
        reinterpret_cast<void (*)(Zombie *, Plant *)>(Zombie_ZombieCatapultFireAddr)(this, plant);
    }
    bool SetupDrawZombieWon(Sexy::Graphics *g) {
        return reinterpret_cast<bool (*)(Zombie *, Sexy::Graphics *)>(Zombie_SetupDrawZombieWonAddr)(this, g);
    }
    void BossDie() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_BossDieAddr)(this);
    }
    void TrySpawnLevelAward() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_TrySpawnLevelAwardAddr)(this);
    }
    void StartZombieSound() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_StartZombieSoundAddr)(this);
    }
    static bool ZombieTypeCanGoOnHighGround(ZombieType theZombieType) {
        return reinterpret_cast<bool (*)(ZombieType)>(ZombieTypeCanGoOnHighGroundAddr)(theZombieType);
    }
    void ZamboniDeath(unsigned int theDamageFlags) {
        reinterpret_cast<void (*)(Zombie *, unsigned int)>(Zombie_ZamboniDeathAddr)(this, theDamageFlags);
    }
    void CatapultDeath(unsigned int theDamageFlags) {
        reinterpret_cast<void (*)(Zombie *, unsigned int)>(Zombie_CatapultDeathAddr)(this, theDamageFlags);
    }
    void ApplyBossSmokeParticles(bool theEnable) {
        reinterpret_cast<void (*)(Zombie *, bool)>(Zombie_ApplyBossSmokeParticlesAddr)(this, theEnable);
    }
    void UpdateZombieFalling() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_UpdateZombieFallingAddr)(this);
    }
    void DoDaisies() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_DoDaisiesAddr)(this);
    }
    void ReanimReenableClipping() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie_ReanimReenableClippingAddr)(this);
    }

    Zombie() {
        _constructor();
    }
    ~Zombie() {
        _deconstructor();
    };

    void ZombieInitialize(int theRow, ZombieType theType, bool theVariant, Zombie *theParentZombie, int theFromWave, bool theIsVisible);
    void CheckIfPreyCaught();
    void Draw(Sexy::Graphics *g);
    void DrawShadow(Sexy::Graphics *g);
    int GetSunBeanDamageCapacity(unsigned int theDamageFlags);
    void SpawnSunBeanSun(int theSunValue);
    void SettleSunBeanSun();
    void DieWithLoot();
    void DieNoLoot();
    void DieNoLoot_Origin();
    void StopZombieSound();
    void Update();
    void UpdateActions();
    void UpdatePlaying();
    void LandFlyer(unsigned int theDamageFlags);
    void UpdateZombieFlyer();
    void UpdateYeti();
    void UpdateZombieImp();
    void UpdateSuperFanImp();
    void UpdateGigaFootball();
    void UpdateZombieBackupDancer();
    void UpdateZombieJackson();
    bool CanRevived() const;
    ZombieID RaiseDeadZombie(ZombieType theZombieType, int theRow, int theCol);
    void RaiseDeadZombies();
    void JacksonDie();
    void UpdateZombieJackInTheBox();
    void UpdateZombiePolevaulter();
    GridItem *FindPoleTarget();
    Plant *FindGigaPolevaulterTarget();
    void UpdateGigaPolevaulter();
    void UpdateSundayEdition();
    void UpdateZombieExplorer();
    void UpdateExplorerProjectiles();
    void ExplorerTorchConvert(bool theBurn);
    void UpdateGigaGargantuar();
    void InterruptLightning();
    void InterruptSuperNovaDestroy();
    void UpdateGigaImp();
    Zombie *GetDogPartner() const;
    void CheckDogPartnerDeath();
    void HandleDogPartnerLost();
    void UpdateDogWalker();
    void UpdateZombieDog();
    void UpdateZombieTeleportation();
    void UpdateSuperNovaGargantuar();
    void UpdateZombieCrossingGuard();
    void UpdateZombieScientist();
    bool IsInScientistTargetRange(const Sexy::Rect &theTargetRect, int theTargetRow, int theRangeInset);
    bool HasScientistTriggerTarget();
    void ApplyScientistSpray();
    bool ApplyScientistHealing();
    void SetScientistPhase(ZombiePhase thePhase);
    bool FindTeleportationTarget();
    Zombie *FindCrossingGuardTarget();
    bool IsValidCrossingGuardTarget(Zombie *theTarget, bool theCheckRange);
    bool IsTrafficConeTargetReserved(Zombie *theTarget);
    bool BindRealatedZombie(Zombie *theZombie);
    void UnbindRealatedZombie();
    void LaunchTrafficCone(Zombie *theTarget);
    bool IsValidTeleportationTarget();
    void ApplyTrafficCone();
    Plant *FindDogTarget();
    void SetDogPairRow(int theRow);
    void UpdateZombieGargantuar();
    void ZombieImpThrown(Zombie *theThrowerZombie, float theOffsetDistance);
    void ZombieImpKicked(float theKickingDistance);
    void UpdateZombiePeaHead();
    void UpdateZombieGatlingHead();
    void BurnRow(int theRow);
    bool FindJalapenoHeadTarget();
    void UpdateZombieJalapenoHead();
    void UpdateZombieSquashHead();
    void UpdateZombieDancer();
    void UpdateZombieRiseFromGrave();
    void FinishZombieRiseFromGrave();
    void UpdateDamageStates(unsigned int theDamageFlags);
    void BossDestroyIceballInRow(int theRow);
    void BossDestroyFireball();
    int GetDancerFrame();
    void RiseFromGrave(int theCol, int theRow);
    void EatPlant(Plant *thePlant);
    void EatZombie(Zombie *theZombie);
    void DetachShield();
    void CheckForBoardEdge();
    void DrawBossPart(Sexy::Graphics *g, int theBossPart);
    bool IsGargantuar() const;
    static bool IsGargantuar(ZombieType theZombieType);
    static bool IsZombotany(ZombieType theZombieType);
    static bool IsZomblob(ZombieType theZombieType);
    static bool ZombieTypeCanGoInPool(ZombieType theZombieType);
    void BossSpawnAttack();
    void DrawBungeeCord(Sexy::Graphics *graphics, int theOffsetX, int theOffsetY);
    bool IsTangleKelpTarget();
    bool HasYuckyFaceImage();
    void DrawReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, int theBaseRenderGroup);
    bool CanLoseBodyParts();
    void SetupReanimForLostHead();
    void DropHead(unsigned int theDamageFlags);
    void DropHead_Origin(unsigned int theDamageFlags);
    void BreakRope();
    void DropPole();
    void DropFlag();
    void DropHelm(unsigned int theDamageFlags);
    void DropShield(unsigned int theDamageFlags);
    void SetupReanimForLostArm(unsigned int theDamageFlags);
    void DropArm(unsigned int theDamageFlags);
    Sexy::Rect GetZombieAttackRect();
    Plant *FindPlantTarget(ZombieAttackType theAttackType);
    Plant *FindFriendPlantTarget();
    bool CanTargetPlant(Plant *thePlant, ZombieAttackType theAttackType);
    Zombie *FindZombieTarget();
    Zombie *FindFriendZombieTarget();
    Zombie *FindZombieGigaFootball();
    void TakeDamage(int theDamage, unsigned int theDamageFlags);
    void TakeDamage_Origin(int theDamage, unsigned int theDamageFlags);
    void TakeBodyDamage(int theDamage, unsigned int theDamageFlags);
    int TakeHelmDamage(int theDamage, unsigned int theDamageFlags);
    int TakeFlyingDamage(int theDamage, unsigned int theDamageFlags);
    int TakeShieldDamage(int theDamage, unsigned int theDamageFlags);
    void AttachShield();
    void PlayZombieReanim(const char *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);
    void EnableDanceMode(bool theEnableDance) {
        reinterpret_cast<void (*)(Zombie *, bool)>(Zombie_EnableDanceModeAddr)(this, theEnableDance);
    }
    void StartWalkAnim(int theBlendTime);
    void ReanimShowPrefix(const char *theTrackPrefix, int theRenderGroup);
    void ReanimShowTrack(const char *theTrackName, int theRenderGroup);
    float GetPosYBasedOnRow(int theRow);
    void SetRow(int theRow);
    void StartMindControlled();
    void StartMindControlled_Origin();
    void ConvertToImp();
    void ConvertToImp_Origin();
    void UpdateReanim();
    void UpdateReanimColor();
    void SetupLostArmReanim();
    void SetZombatarReanim();
    bool IsImmobilizied() const;
    bool IsBouncingPogo() const;
    void UpdateZombiePogo();
    bool IsFlying() const;
    bool IsImpFlying() const;
    void AnimateChewSound();
    void Animate();
    int GetBobsledPosition();
    void BobsledCrash();
    bool IsBobsledTeamWithSled();
    bool IsDeadOrDying() const;
    bool CanBeChilled();
    bool CanBeFrozen();
    void AddButter();
    void MowDown();
    void MowDown_Original();
    static bool IsZombatarZombie(ZombieType theType);
    void SquishAllInSquare(int theX, int theY, ZombieAttackType theAttackType);
    bool IsWalkingBackwards() const;
    static void SetupDoorArms(Reanimation *aReanim, bool theShow);
    static void SetupReanimLayers(Reanimation *aReanim, ZombieType theZombieType);
    void ShowDoorArms(bool theShow);
    void StartEating();
    void StartEating_Origin();
    void StopEating();
    Sexy::Rect GetZombieRect();
    void GetDrawPos(ZombieDrawPosition &theDrawPos);
    void DrawIceTrap(Sexy::Graphics *g, const ZombieDrawPosition &theDrawPos, bool theFront);
    void DrawButter(Sexy::Graphics *g, const ZombieDrawPosition &theDrawPos);
    bool IsOnHighGround();
    void DropLoot();
    bool IsOnBoard() const;
    bool EffectedByDamage(unsigned int theDamageRangeFlags);
    int GetHelmDamageIndex() const;
    int GetBodyDamageIndex() const;
    int GetShieldDamageIndex() const;
    bool IsFireResistant() const;
    void BungeeDropZombie(Zombie *theDroppedZombie, int theGridX, int theGridY);
    void BungeeDropZombie_Origin(Zombie *theDroppedZombie, int theGridX, int theGridY);
    void PickRandomSpeed();
    void ApplySyncedSpeed(float theVelX, short theAnimTicks);
    float ZombieTargetLeadX(float theTime);
    void ApplyBurn();
    void ApplyButter();
    void ApplyChill(bool theIsIceTrap);
    void HitIceTrap();
    void ApplySyncedIceTrap(int theIceTrapCounter);
    bool ZombieNotWalking();
    bool IsMovingAtChilledSpeed();
    void UpdateZombieWalking();
    void UpdateZombieDolphinRider();
    ZombiePhase GetDancerPhase();
    ZombieID SummonBackupDancer(int theRow, int thePosX);
    void SummonBackupDancers();
    void SummonBackupDancers_Origin();
    bool NeedsMoreBackupDancers();
    void UpdateYuckyFace();
    void UpdateZombieBobsled();
    void UpdateZombiePool();
    void DoSpecial();
    void ExplorerBurnPlant(Plant *thePlant);
    void UpdateZombieBungee();
    Plant *FindCatapultTarget();
    void UpdateZombieCatapult();
    void BungeeLanding();
    void UpdateLadder();
    void PlayDeathAnim(unsigned int theDamageFlags);
    void UpdateDeath();
    void ZomblobSplit();
    void SetupButteredZomblobReanim();
    void WalkIntoHouse();
    bool IsChangingRow();

protected:
    void _constructor() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie__constructorAddr)(this);
    };
    void _deconstructor() {
        reinterpret_cast<void (*)(Zombie *)>(Zombie__deconstructorAddr)(this);
    }
};

class ZombieDefinition {
public:
    ZombieType mZombieType;
    ReanimationType mReanimationType;
    int mZombieValue;
    int mStartingLevel;
    int mFirstAllowedWave;
    int mPickWeight;
    const char *mZombieName;
};
extern ZombieDefinition gZombieDefs[NUM_ZOMBIE_TYPES];
inline ZombieDefinition gZombieTrashBinDef = {ZombieType::ZOMBIE_TRASHCAN, ReanimationType::REANIM_ZOMBIE, 1, 99, 1, 4000, "TRASHCAN_ZOMBIE"};
extern ZombieDefinition gExtendedZombieDefs[];

ZombieDefinition &GetZombieDefinition(ZombieType theZombieType);
/***************************************************************************************************************/

inline bool zombieBloated;
inline bool showZombieBodyHealth;
inline bool showGargantuarHealth;
inline bool showHelmAndShieldHealth;
inline int maidCheats; // 女仆秘籍
inline int boardEdgeAdjust;
inline int zombieSetScale;


inline void (*old_Zombie_UpdateActions)(Zombie *);

inline void (*old_Zombie_Draw)(Zombie *zombie, Sexy::Graphics *graphics);
inline void (*old_Zombie_DrawBossPart)(Zombie *a1, Sexy::Graphics *graphics, int theBossPart);

inline void (*old_Zombie_RiseFromGrave)(Zombie *zombie, int gridX, int gridY);

inline void (*old_Zombie_DetachShield)(Zombie *zombie);

inline void (*old_Zombie_ZombieInitialize)(Zombie *zombie, int theRow, ZombieType theType, bool theVariant, Zombie *theParentZombie, int theFromWave, bool isVisible);

inline void (*old_Zombie_DieNoLoot)(Zombie *);

inline void (*old_Zombie_DrawReanim)(Zombie *zombie, Sexy::Graphics *graphics, ZombieDrawPosition &zombieDrawPosition, int theBaseRenderGroup);

inline void (*old_Zombie_DropHead)(Zombie *zombie, unsigned int a2);

inline void (*old_Zombie_DropArm)(Zombie *, unsigned int);

inline int (*old_Zombie_TakeHelmDamage)(Zombie *, int theDamage, unsigned int theDamageFlags);

inline void (*old_Zombie_PlayZombieReanim)(Zombie *, const char *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);

inline void (*old_Zombie_ReanimShowPrefix)(Zombie *, const char *, int);

inline void (*old_Zombie_ReanimShowTrack)(Zombie *, const char *, int);

inline float (*old_Zombie_GetPosYBasedOnRow)(Zombie *, int theRow);

inline void (*old_Zombie_SetRow)(Zombie *, int theRow);

inline void (*old_Zombie_UpdateReanim)(Zombie *);

inline int (*old_Zombie_GetBobsledPosition)(Zombie *);

inline void (*old_Zombie_SquishAllInSquare)(Zombie *, int theX, int theY, ZombieAttackType theAttackType);

inline void (*old_Zombie_DropLoot)(Zombie *);

inline void (*old_Zombie_ApplyBurn)(Zombie *);

inline void (*old_Zombie_UpdateYuckyFace)(Zombie *);

#endif // PVZ_LAWN_BOARD_ZOMBIE_H
