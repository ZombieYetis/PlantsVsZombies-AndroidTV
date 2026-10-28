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

#ifndef PVZ_LAWN_BOARD_CHALLENGE_H
#define PVZ_LAWN_BOARD_CHALLENGE_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Common/SyncObject.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Effect/FilterEffect.h"

inline constexpr int BEGHOULED_MAX_GRIDSIZEX = 8;
inline constexpr int BEGHOULED_MAX_GRIDSIZEY = 5;
inline constexpr int ART_CHALLEGE_SIZE_X = 8;
inline constexpr int MAX_PICK_GRID_SIZE = 50;

inline constexpr int BEGHOULED_WINNING_SCORE = 75;
inline constexpr int SLOT_MACHINE_WINNING_SCORE = 2000;
inline constexpr int ZOMBIQUARIUM_WINNING_SCORE = 1000;
inline constexpr int I_ZOMBIE_WINNING_SCORE = 5;
inline constexpr int MAX_PORTALS = 4;
inline constexpr int MAX_SQUIRRELS = 7;
inline constexpr int MAX_SCARY_POTS = 54;
inline constexpr int STORM_FLASH_TIME = 150;

inline constexpr int MP_SUDDEN_DEATH_SECONDS = 300;
inline constexpr int MP_SUDDEN_DEATH_TICKS_PER_SECOND = 100;
inline constexpr int MP_SUDDEN_DEATH_START_COUNTER = MP_SUDDEN_DEATH_SECONDS * MP_SUDDEN_DEATH_TICKS_PER_SECOND;

class LawnApp;
class Board;
class Plant;
class Zombie;
class GridItem;
class SeedPacket;
class HitResult;
class HelpBarWidget;

enum BeghouledUpgrade {
    BEGHOULED_UPGRADE_REPEATER,
    BEGHOULED_UPGRADE_FUMESHROOM,
    BEGHOULED_UPGRADE_TALLNUT,
    NUM_BEGHOULED_UPGRADES = 3,
};

struct BeghouledBoardState {
    SeedType mSeedType[9][6];
};

class Challenge : public SyncObject {
public:
    static inline bool msVSShuffleMode = false; // 对战刷牌模式
    static inline bool msVSClassicMode = false; // 对战经典模式
    static int &gVSResourseDropMode;            // 常为0
    static int &gVSResourceDropCount;           // 常为4
    static int &gVSWinMode;
    static int &gVSSuddenDeathMode;

    LawnApp *mApp;                                          // 4
    Board *mBoard;                                          // 5
    HelpBarWidget *mHelpBar;                                // 6
    bool mBeghouledMouseCapture;                            // 28
    int mBeghouledMouseDownX;                               // 8
    int mBeghouledMouseDownY;                               // 9
    bool mBeghouledEated[9][6];                             // 40 ~  93
    bool mBeghouledPurcasedUpgrade[NUM_BEGHOULED_UPGRADES]; // 94 ~ 96
    int mBeghouledMatchesThisMove;                          // 25
    ChallengeState mChallengeState;                         // 26
    int mChallengeStateCounter;                             // 27
    int mConveyorBeltCounter1;                              // 28
    int mConveyorBeltCounter2;                              // 29
    int mChallengeScore;                                    // 30
    bool mShowBowlingLine;                                  // 124
    SeedType mLastConveyorSeedType;                         // 32
    int mSurvivalStage;                                     // 33
    int mSlotMachineRollCount;                              // 34
    ReanimationID mReanimChallenge;                         // 35
    int mReanimCloud[6];                                    // 36 ~ 41
    int mCloudCounter[6];                                   // 42 ~ 47
    int mChallengeGridX;                                    // 48
    int mChallengeGridY;                                    // 49
    int mScaryPotterPots;                                   // 50
    int mRainCounter;                                       // 51
    int mTreeOfWisdomTalkIndex;                             // 52
    int mBeghouledMouseDown;                                // 53
    int mBeghouledMouseCaptureCountdown;                    // 54
    int mMPZombieConveyorTimer;                             // 55
    int mBobSledMPCounter;                                  // 56
    int mSuddenDeathStartTick;                              // 57, 废弃
    int mPauseStartTick;                                    // 58, 废弃
    bool mIsMPSuddenDeathNow;                               // 59 * 4
    SeedType mSuddenDeathDisableSeeds1[3];                  // 60 ~ 62
    SeedType mSuddenDeathDisableSeeds2[3];                  // 63 ~ 65
    int mSuddenDeathBoomCount;                              // 66
    float mHeavyWeaponX;                                    // 67
    float mHeavyWeaponY;                                    // 68
    float mHeavyWeaponAngle;                                // 69
    int unkHeavyWeaponWithInitValue1000;                    // 70
    int mHeavyWeaponCatTailAttackCounter;                   // 71
    int mHeavyWeaponCatTailLevel;                           // 72
    int mHeavyWeaponPeaLevel;                               // 73
    int mHeavyWeaponSnowPeaCounter;                         // 74
    int mHeavyWeaponTorchWoodCounter;                       // 75
    int unkHeavyWeaponWithInitValue10;                      // 76
    ReanimationID mReanimHeavyWeaponID1;                    // 77
    ReanimationID mReanimHeavyWeaponID2;                    // 78
    ReanimationID mReanimHeavyWeaponID3;                    // 79
                                                            // 大小80个整数
    Challenge() {
        _constructor();
    };
    ~Challenge() {
        _destructor();
    };

    bool MouseDown(int x, int y, int theClickCount, HitResult *theHitResult, int thePlayerIndex) {
        return reinterpret_cast<bool (*)(Challenge *, int, int, int, HitResult *, int)>(Challenge_MouseDownAddr)(this, x, y, theClickCount, theHitResult, thePlayerIndex);
    }
    void TreeOfWisdomOpenStore() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_TreeOfWisdomOpenStoreAddr)(this);
    }
    void IZombieScoreBrain(GridItem *theBrain) {
        reinterpret_cast<void (*)(Challenge *, GridItem *)>(Challenge_IZombieScoreBrainAddr)(this, theBrain);
    }
    void GraveDangerSpawnGraveAt(int theGridX, int theGridY) {
        reinterpret_cast<void (*)(Challenge *, int, int)>(Challenge_GraveDangerSpawnGraveAtAddr)(this, theGridX, theGridY);
    }
    void PlantAdded(Plant *thePlant) {
        reinterpret_cast<void (*)(Challenge *, Plant *)>(Challenge_PlantAddedAddr)(this, thePlant);
    }
    void DrawRain(Sexy::Graphics *g) {
        reinterpret_cast<void (*)(Challenge *, Sexy::Graphics *)>(Challenge_DrawRainAddr)(this, g);
    }
    void DrawStormNight(Sexy::Graphics *g) {
        reinterpret_cast<void (*)(Challenge *, Sexy::Graphics *)>(Challenge_DrawStormNightAddr)(this, g);
    }
    void UpdateRain() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdateRainAddr)(this);
    }
    void UpdateStormyNight() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdateStormyNightAddr)(this);
    }
    void UpdateMPZombieBank() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdateMPZombieBankAddr)(this);
    }
    void UpdateBeghouled() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdateBeghouledAddr)(this);
    }
    void ScaryPotterUpdate() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_ScaryPotterUpdateAddr)(this);
    }
    int CanTargetZombieWithPortals(Plant *thePlant, Zombie *theZombie) {
        return reinterpret_cast<int (*)(Challenge *, Plant *, Zombie *)>(Challenge_CanTargetZombieWithPortalsAddr)(this, thePlant, theZombie);
    }
    void ScaryPotterJackExplode(int thePosX, int thePosY) {
        reinterpret_cast<void (*)(Challenge *, int, int)>(Challenge_ScaryPotterJackExplodeAddr)(this, thePosX, thePosY);
    }
    void ZombieAtePlant(Zombie *theZombie, Plant *thePlant) {
        reinterpret_cast<void (*)(Challenge *, Zombie *, Plant *)>(Challenge_ZombieAtePlantAddr)(this, theZombie, thePlant);
    }
    void WhackAZombieUpdate() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_WhackAZombieUpdateAddr)(this);
    }
    void IZombieUpdate() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_IZombieUpdateAddr)(this);
    }
    void UpdateSlotMachine() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdateSlotMachineAddr)(this);
    }
    void UpdateRainingSeeds() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdateRainingSeedsAddr)(this);
    }
    void UpdatePortalCombat() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_UpdatePortalCombatAddr)(this);
    }
    void SquirrelUpdate() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_SquirrelUpdateAddr)(this);
    }
    void ZombiquariumUpdate() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_ZombiquariumUpdateAddr)(this);
    }
    void TreeOfWisdomUpdate() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_TreeOfWisdomUpdateAddr)(this);
    }
    void IZombieMouseDownWithZombie(int x, int y, int theClickCount, int thePlayerIndex) {
        reinterpret_cast<void (*)(Challenge *, int, int, int, int)>(Challenge_IZombieMouseDownWithZombieAddr)(this, x, y, theClickCount, thePlayerIndex);
    }
    void IZombiePlantDropRemainingSun(Plant *thePlant) {
        reinterpret_cast<void (*)(Challenge *, Plant *)>(Challenge_IZombiePlantDropRemainingSunAddr)(this, thePlant);
    }
    void AdvanceCrazyDaveDialog() {
        reinterpret_cast<void (*)(Challenge *)>(Challenge_AdvanceCrazyDaveDialogAddr)(this);
    }


    void Update();
    void UpdateVSAddPlants() const;
    int GetSuddenDeathCount() const;
    bool IsMPSuddenDeath() const;
    int GetUnderPlantCol(int theRow) const;
    void HeavyWeaponFire(float a2, float a3);
    void IZombieDrawPlant(Sexy::Graphics *g, Plant *thePlant) const;
    void HeavyWeaponUpdate();
    bool IZombieEatBrain(Zombie *theZombie);
    void DrawArtChallenge(Sexy::Graphics *g) const;
    PlantingReason CanPlantAt(int theGridX, int theGridY, SeedType theSeedType) const;
    void InitLevel();
    void InitZombieWaves();
    void TreeOfWisdomFertilize();
    void LastStandUpdate();
    void DrawHeavyWeapon(Sexy::Graphics *g);
    bool UpdateZombieSpawning();
    void HeavyWeaponPacketClicked(SeedPacket *theSeedPacket);
    static int IsMPSeedType(SeedType theSeedType);
    static int IsZombieSeedType(SeedType theSeedType);
    void IZombieSetPlantFilterEffect(Plant *thePlant, FilterEffect theFilterEffect) const;
    static ZombieType IZombieSeedTypeToZombieType(SeedType theSeedType);
    void IZombiePlaceZombie(ZombieType theZombieType, int theGridX, int theGridY) const;
    void StartLevel();
    void ScaryPotterOpenPot(GridItem *theScaryPot);
    GridItem *IZombieGetBrainTarget(Zombie *theZombie);
    void IZombieSquishBrain(GridItem *theBrain);
    static int ScaryPotterCountSunInPot(GridItem *theScaryPot);
    SeedType GetArtChallengeSeed(int theGridX, int theGridY) const;
    void InitZombieWavesFromList(const ZombieType *theZombieList, int theListLength) const;
    void IZombieSetupPlant(Plant *thePlant) const;
    void HeavyWeaponReanimUpdate() const;
    void MouseDownWhackAZombie(int theX, int theY, int thePlayerIndex) const;
    static bool IsMPResourceProducer(SeedType theSeedType);
    static bool IsMPZombieTypeAddInRow(ZombieType theZombieType);
    static bool IsMPZombieTypeCanGoInPool(ZombieType theZombieType);
    void DrawWeather(Sexy::Graphics *g);
    void UpdateConveyorBelt(int thePlayerIndex);
    void UpdateMPGraveStones();
    bool ISMPSeedSuddenDeathDisabled(int thePlayerIndex, SeedType theSeedType);
    void DrawBackdrop(Sexy::Graphics *g);
    void DrawVSSuddenDeathClock(Sexy::Graphics *g);

protected:
    friend void InitHookFunction();

    void _constructor();
    void _destructor();
};

/***************************************************************************************************************/
inline int targetWavesToJump = 1;
inline bool requestJumpSurvivalStage;
inline bool stopSpawning;            // 暂停刷怪
inline int gVSAddUnderPlantsCounter; // 对战自动种植睡莲、花盆的间隔
inline bool gFreeForFristShuffle[2]; // 刷牌模式首次刷新免费


inline void (*old_Challenge_Update)(Challenge *a);

inline void (*old_Challenge_Challenge)(Challenge *a);

inline void (*old_Challenge_HeavyWeaponFire)(Challenge *challenge, float a2, float a3);

inline void (*old_Challenge_HeavyWeaponUpdate)(Challenge *challenge);

inline void (*old_Challenge_InitLevel)(Challenge *challenge);

inline void (*old_Challenge_InitZombieWaves)(Challenge *a);

inline void (*old_Challenge_TreeOfWisdomFertilize)(Challenge *);

inline ZombieType (*old_Challenge_IZombieSeedTypeToZombieType)(SeedType type);

inline bool (*old_Challenge_UpdateZombieSpawning)(Challenge *);

inline void (*old_Challenge_HeavyWeaponPacketClicked)(Challenge *challenge, SeedPacket *seedPacket);

inline void (*old_Challenge_StartLevel)(Challenge *challenge);

inline void (*old_Challenge_Delete)(Challenge *challenge);

inline void (*old_Challenge_ScaryPotterOpenPot)(Challenge *, GridItem *theScaryPot);

inline GridItem *(*old_Challenge_IZombieGetBrainTarget)(Challenge *, Zombie *theZombie);

inline void (*old_Challenge_IZombieSquishBrain)(Challenge *, GridItem *theBrain);

inline void (*old_Challenge_UpdateConveyorBelt)(Challenge *, int);

inline void (*old_Challenge_DrawBackdrop)(Challenge *, Sexy::Graphics *);

#endif // PVZ_LAWN_BOARD_CHALLENGE_H
