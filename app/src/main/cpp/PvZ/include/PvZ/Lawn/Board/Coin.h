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

#ifndef PVZ_LAWN_BOARD_COIN_H
#define PVZ_LAWN_BOARD_COIN_H

#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/System/PlayerInfo.h"
#include "PvZ/Symbols.h"

class Coin : public GameObject {

    struct CoinVTable {
        // 0x00 -> Coin::~Coin()
        void (*completeDestructor)(Coin *self);
        // 0x04 -> Coin::~Coin() + operator delete
        void (*deletingDestructor)(Coin *self);
        // 0x08 -> GameObject::BeginDraw(Graphics*)
        bool (*BeginDraw)(GameObject *self, Sexy::Graphics *graphics);
        // 0x0C -> GameObject::EndDraw(Graphics*)
        void (*EndDraw)(GameObject *self, Sexy::Graphics *graphics);
        // 0x10 -> GameObject::MakeParentGraphicsFrame(Graphics*)
        void (*MakeParentGraphicsFrame)(GameObject *self, Sexy::Graphics *graphics);
    };

public:
    float mPosX;                   // 13
    float mPosY;                   // 14
    float mPrevPosX;               // 15
    float mPrevPosY;               // 16
    float mVelX;                   // 17
    float mVelY;                   // 18
    float mScale;                  // 19
    bool mDead;                    // 80
    int mFadeCount;                // 21
    float mCollectX;               // 22
    float mCollectY;               // 23
    int mGroundY;                  // 24
    int mCoinAge;                  // 25
    bool mIsBeingCollected;        // 104
    int mCollectedByPlayerIndex;   // 27
    int mDisappearCounter;         // 28
    CoinType mType;                // 29
    CoinMotion mCoinMotion;        // 30
    AttachmentID mAttachmentID[3]; // 31 ~ 33
    float mCollectionDistance;     // 34
    SeedType mUsableSeedType;      // 35
    PottedPlant mPottedPlantSpec;  // 36
    bool mNeedsBouncyArrow;        // 164
    bool mHasBouncyArrow;          // 165
    bool mHitGround;               // 166
    int mTimesDropped;             // 42
    int mPlayerIndex;              // 43
    float unk2;                    // 44
    bool unk3;                     // 180
    int mCustomSunValue;           // 46
    bool mAutoCollect;             // 188
    int mAutoCollectAge;           // 48
    bool unk7;                     // 196
    bool unk8;                     // 197
    // 大小50个整数

    void MouseDown(int x, int y, int theClickCount) {
        reinterpret_cast<void (*)(Coin *, int, int, int)>(Coin_MouseDownAddr)(this, x, y, theClickCount);
    }
    void Collect(int thePlayerIndex);
    void Die() {
        reinterpret_cast<void (*)(Coin *)>(Coin_DieAddr)(this);
    }
    bool IsLevelAward() {
        return reinterpret_cast<bool (*)(Coin *)>(Coin_IsLevelAwardAddr)(this);
    }
    bool IsPresentWithAdvice() {
        return reinterpret_cast<bool (*)(Coin *)>(Coin_IsPresentWithAdviceAddr)(this);
    }
    bool IsMoney() {
        return reinterpret_cast<bool (*)(Coin *)>(Coin_IsMoneyAddr)(this);
    }
    void PlayGroundSound() {
        reinterpret_cast<void (*)(Coin *)>(Coin_PlayGroundSoundAddr)(this);
    }
    int GetDisappearTime() {
        return reinterpret_cast<int (*)(Coin *)>(Coin_GetDisappearTimeAddr)(this);
    }
    void StartFade() {
        reinterpret_cast<void (*)(Coin *)>(Coin_StartFadeAddr)(this);
    }
    static int GetCoinValue(CoinType coinType) {
        return reinterpret_cast<int (*)(CoinType)>(Coin_GetCoinValueAddr)(coinType);
    }
    SeedType GetFinalSeedPacketType() {
        return reinterpret_cast<SeedType (*)(Coin *)>(Coin_GetFinalSeedPacketTypeAddr)(this);
    }
    const CoinVTable *GetVTable() {
        return (CoinVTable *)vTable;
    }

    void CoinInitialize(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion);
    void GamepadCursorOver(int thePlayerIndex);
    void Update();
    void PlayCollectSound();
    void FanOutCoins(CoinType theCoinType, int theNumCoins);
    void ScoreCoin();
    void UpdateCollected();
    void UpdateFallForAward();
    void UpdateFall();
    bool MouseHitTest(int theX, int theY, int **theHitResult, int thePlayerIndex);
    bool IsSun() const;
    bool IsDeath() const;
    void Draw(Sexy::Graphics *g);
    Sexy::Color GetColor();
    int GetSunValue();
    float GetSunScale();
};

/***************************************************************************************************************/
inline bool enableManualCollect;
inline bool BanDropCoin;


inline void (*old_Coin_CoinInitialize)(Coin *, int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion);

inline void (*old_Coin_Collect)(Coin *coin, int thePlayerIndex);

inline void (*old_Coin_GamepadCursorOver)(Coin *coin, int a2);

inline void (*old_Coin_Update)(Coin *coin);

inline void (*old_Coin_FanOutCoins)(Coin *coin, CoinType theCoinType, int theNumCoins);

inline void (*old_Coin_UpdateFall)(Coin *coin);

inline bool (*old_Coin_MouseHitTest)(Coin *coin, int a2, int a3, int **hitResult, int a5);

#endif // PVZ_LAWN_BOARD_COIN_H
