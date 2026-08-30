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

#ifndef PVZ_LAWN_SYSTEM_PLAYER_INFO_H
#define PVZ_LAWN_SYSTEM_PLAYER_INFO_H

#include "PvZ/Lawn/Widget/AchievementsWidget.h"
#include "PvZ/Lawn/Widget/LeaderboardsWidget.h"

#include "../Common/ConstEnums.h"

class DataSync {
    [[maybe_unused]] int unk[19];
}; // 大小未知，至少19个int

class PottedPlant {
public:
    enum FacingDirection // Prefix: FACING
    {
        FACING_RIGHT,
        FACING_LEFT
    };

    SeedType mSeedType : 7;
    FacingDirection mFacing : 1;

    GardenType mWhichZenGarden : 2;
    int mY : 2;
    DrawVariation mDrawVariation : 4;

    int mX : 3;
    PottedPlantNeed mPlantNeed : 3;
    PottedPlantAge mPlantAge : 2;

    int mTimesFed : 3;
    int mFeedingsPerGrow : 3;
    int : 2;

    int mLastWateredTime;       // 1
    int mLastNeedFulfilledTime; // 2
    int mLastFertilizedTime;    // 3
    int mLastChocolateTime;     // 4
};

class Mode3RecentServerStorage {
public:
    char mRecentServerAddr[3][22];
};

namespace Sexy {
class PlayerInfo {
public:
    struct PlayerInfoVTable {
        void (*completeDestructor)(PlayerInfo *self);  // 0x00
        void (*deletingDestructor)(PlayerInfo *self);  // 0x04
        void (*UnknownPureVirtual0)(PlayerInfo *self); // 0x08
        void (*UnknownPureVirtual1)(PlayerInfo *self); // 0x0C
        void (*UnknownPureVirtual2)(PlayerInfo *self); // 0x10
        void (*UnknownPureVirtual3)(PlayerInfo *self); // 0x14
        void (*UnknownPureVirtual4)(PlayerInfo *self); // 0x18
        void (*UnknownPureVirtual5)(PlayerInfo *self); // 0x1C
        void (*UnknownPureVirtual6)(PlayerInfo *self); // 0x20
        void (*UnknownPureVirtual7)(PlayerInfo *self); // 0x24
        void (*UnknownPureVirtual8)(PlayerInfo *self); // 0x28
        bool (*IsSaving)(PlayerInfo *self);            // 0x2C
        bool (*IsLoaded)(PlayerInfo *self);            // 0x30
        bool (*IsLoading)(PlayerInfo *self);           // 0x34
    };

public:
    void **vTable;                                              // 0
    int unk1;                                                   // 1
    int *mProFileMgr;                                           // 2
    int unk2;                                                   // 3
    char *mName;                                                // 4
    int mUseSeq;                                                // 5
    int mId;                                                    // 6
    int mProfileId;                                             // 7
    int mFlags;                                                 // 8
    int mLevel;                                                 // 9
    int mCoins;                                                 // 10
    int mChallengeRecords[100];                                 // 11 ~ 110 ， 但末尾6个完全不会用到，可以成为我的自己存数据的空间。
    int mPurchases[36];                                         // 111 ~ 146 ，本应该是mPurchases[80]，111 ~ 190，但仅用到了前36个。
    bool mAchievements[AchievementType::NUM_ACHIEVEMENT_TYPES]; // 147 ~ 149, 从mPurchases[80]分出来的
    unsigned short mVSRoomPort;                                 // 150 ~
    bool mVSExtraPacketMode;
    bool mVSExtendedSeedsMode;
    bool mVSBanMode;
    bool mVSBalancePatchMode;
    bool mVSResultsSendPlayerName;
    bool mVSResultsAutoSaveReplay;
    bool mHostAllowSpectate;
    Mode3RecentServerStorage serverStorage;
    bool mRenamed;
    bool mVSPlantAIMode;
    bool mVSZombieAIMode;
    bool mVSAIEnhancementMode;
    bool mVSAIDraftDisabledMode;
    bool mVSAITemplateDeckDisabledMode;
    bool mUnused675[65];
    bool mHapticFeedbackEnabled;
    bool mZombatarEnabled;
    unsigned char mZombatarHat;
    unsigned char mZombatarHatColor;
    unsigned char mZombatarHair;
    unsigned char mZombatarHairColor;
    unsigned char mZombatarFacialHair;
    unsigned char mZombatarFacialHairColor;
    unsigned char mZombatarAccessory;
    unsigned char mZombatarAccessoryColor;
    unsigned char mZombatarEyeWear;
    unsigned char mZombatarEyeWearColor;
    unsigned char mZombatarTidBit;
    unsigned char mZombatarTidBitColor;
    bool mIs3DAcceleratedClosed;   // ~ 189, 从mPurchases[80]分出来的
    int mUsedCoins;                // 190, 从mPurchases[80]分出来的
    int unkMem4[2];                // 191 ~ 192
    int mLastStinkyChocolateTime;  // 193
    int mStinkyPosX;               // 194
    int mStinkyPosY;               // 195
    int mNumPottedPlants;          // 196
    int unk4;                      // 197
    PottedPlant mPottedPlants[50]; // 198 ~ 447
    double mMusicVolume;           // 448 ~ 449
    double mSoundVolume;           // 450 ~ 451
    int unkMems6[5];               // 452 ~ 456
    bool mHelpTextSeen[6];         // 1828 ~ 1833
    int mWinStreak;                // 459
    bool unkBool1;                 // 1840
    bool unkBool2;                 // 1841
    bool mPassedShopSeedTutorial;  // 1842
    bool mMailMessageRead[32];     // 1843 ~ 1874 ，紧密存放，可以存放32x8个bool
    bool mMailMessageSeen[32];     // 1875 ~ 1906 ，紧密存放，可以存放32x8个bool
    GameStats mGameStats;          // 477 ~ 514
    int unk6;                      // 515
    // 大小516个整数

    const PlayerInfoVTable *GetVTable() const {
        return (PlayerInfoVTable *)vTable;
    }
};

class DefaultPlayerInfo : public PlayerInfo {
public:
    struct DefaultPlayerInfoVTable {
        void (*completeDestructor)(DefaultPlayerInfo *self);                       // 0x00
        void (*deletingDestructor)(DefaultPlayerInfo *self);                       // 0x04
        int (*GetId)(DefaultPlayerInfo *self);                                     // 0x08
        int (*GetProfileId)(DefaultPlayerInfo *self);                              // 0x0C
        pvzstl::string (*GetName)(DefaultPlayerInfo *self);                        // 0x10
        void (*Reset)(DefaultPlayerInfo *self);                                    // 0x14
        void (*SyncSummary)(DefaultPlayerInfo *self, DataSync &sync);              // 0x18
        void (*SyncDetails)(DefaultPlayerInfo *self, DataSync &sync, int version); // 0x1C
        void (*DeleteUserFiles)(DefaultPlayerInfo *self);                          // 0x20
        void (*LoadDetails)(DefaultPlayerInfo *self);                              // 0x24
        void (*SaveDetails)(DefaultPlayerInfo *self);                              // 0x28
        bool (*IsSaving)(PlayerInfo *self);                                        // 0x2C
        bool (*IsLoaded)(PlayerInfo *self);                                        // 0x30
        bool (*IsLoading)(PlayerInfo *self);                                       // 0x34
    };

public:
    void SaveDetails() {
        reinterpret_cast<void (*)(DefaultPlayerInfo *)>(Sexy_DefaultPlayerInfo_SaveDetailsAddr)(this);
    }
    int GetId() { // vTable + 2
        return reinterpret_cast<int (*)(DefaultPlayerInfo *)>(Sexy_DefaultPlayerInfo_GetIdAddr)(this);
    }
    const DefaultPlayerInfoVTable *GetVTable() const {
        return (DefaultPlayerInfoVTable *)vTable;
    }
};
} // namespace Sexy

class LawnPlayerInfo : public Sexy::DefaultPlayerInfo {
public:
    struct LawnPlayerInfoVTable {
        void (*completeDestructor)(LawnPlayerInfo *self);                       // 0x00
        void (*deletingDestructor)(LawnPlayerInfo *self);                       // 0x04
        int (*GetId)(Sexy::DefaultPlayerInfo *self);                            // 0x08
        int (*GetProfileId)(Sexy::DefaultPlayerInfo *self);                     // 0x0C
        pvzstl::string (*GetName)(Sexy::DefaultPlayerInfo *self);               // 0x10
        void (*Reset)(LawnPlayerInfo *self);                                    // 0x14
        void (*SyncSummary)(Sexy::DefaultPlayerInfo *self, DataSync &sync);     // 0x18
        void (*SyncDetails)(LawnPlayerInfo *self, DataSync &sync, int version); // 0x1C
        void (*DeleteUserFiles)(LawnPlayerInfo *self);                          // 0x20
        void (*LoadDetails)(Sexy::DefaultPlayerInfo *self);                     // 0x24
        void (*SaveDetails)(Sexy::DefaultPlayerInfo *self);                     // 0x28
        bool (*IsSaving)(Sexy::PlayerInfo *self);                               // 0x2C
        bool (*IsLoaded)(Sexy::PlayerInfo *self);                               // 0x30
        bool (*IsLoading)(Sexy::PlayerInfo *self);                              // 0x34
    };

public:
    int GetLevel() {
        return reinterpret_cast<int (*)(LawnPlayerInfo *)>(LawnPlayerInfo_GetLevelAddr)(this);
    }
    int GetFlag(int theFlag) {
        return reinterpret_cast<int (*)(LawnPlayerInfo *, int)>(LawnPlayerInfo_GetFlagAddr)(this, theFlag);
    }
    void SetFlag(int theFlag, bool theHasFlag) {
        reinterpret_cast<void (*)(LawnPlayerInfo *, int, bool)>(LawnPlayerInfo_SetFlagAddr)(this, theFlag, theHasFlag);
    }
    const LawnPlayerInfoVTable *GetVTable() const {
        return (LawnPlayerInfoVTable *)vTable;
    }
    void AddCoins(int theAmount);
};

#endif // PVZ_LAWN_SYSTEM_PLAYER_INFO_H
