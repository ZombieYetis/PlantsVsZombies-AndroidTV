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

#ifndef PVZ_LAWN_BOARD_GRID_ITEM_H
#define PVZ_LAWN_BOARD_GRID_ITEM_H

#include "PvZ/Lawn/Board/GameObject.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/DataArray.h"

class Board;
class Reanimation;

class MotionTrailFrame {
public:
    float mPosX;
    float mPosY;
    float mAnimTime;
};

class GridItem : public SyncObject {
public:
    LawnApp *mApp;                           // 4
    Board *mBoard;                           // 5
    GridItemType mGridItemType;              // 6
    GridItemState mGridItemState;            // 7
    int mGridX;                              // 8
    int mGridY;                              // 9
    int mGridItemCounter;                    // 10
    int mRenderOrder;                        // 11
    bool mDead;                              // 48
    float mPosX;                             // 13
    float mPosY;                             // 14
    float mGoalX;                            // 15
    float mGoalY;                            // 16
    ReanimationID mGridItemReanimID;         // 17
    ParticleSystemID mGridItemParticleID;    // 18
    ZombieType mZombieType;                  // 19
    SeedType mSeedType;                      // 20
    ScaryPotType mScaryPotType;              // 21
    bool mHighlighted;                       // 88
    int mTransparentCounter;                 // 23
    int mSunCount;                           // 24
    MotionTrailFrame mMotionTrailFrames[12]; // 25 ~ 60
    int mMotionTrailCount;                   // 61
    bool mIsVSGraveStone;                    // 62 * 4
    bool mIsSpecialGrave;                    // 62 * 4 + 1
    int mMoundLevel;                         // 63
    int mSummonIndex;                        // 64
    int mSummonCounter;                      // 65
    int mLaunchCounter;                      // 66
    int mLaunchRate;                         // 67
    int mGraveJustGotShotCounter;            // 68
    int mVSGraveStoneHealth;                 // 69
    int mVSTargetZombieHealth;               // 70
    int mVSTargetZombieDieCounter;           // 71
    int unkMems2[3];                         // 72 ~ 74
    // 大小75个整数

    void DrawLadder(Sexy::Graphics *g) {
        reinterpret_cast<void (*)(GridItem *, Sexy::Graphics *)>(GridItem_DrawLadderAddr)(this, g);
    }
    void DrawIZombieBrain(Sexy::Graphics *g) {
        reinterpret_cast<void (*)(GridItem *, Sexy::Graphics *)>(GridItem_DrawIZombieBrainAddr)(this, g);
    }
    void UpdatePortal() {
        reinterpret_cast<void (*)(GridItem *)>(GridItem_UpdatePortalAddr)(this);
    }
    void UpdateRake() {
        reinterpret_cast<void (*)(GridItem *)>(GridItem_UpdateRakeAddr)(this);
    }
    void UpdateBrain() {
        reinterpret_cast<void (*)(GridItem *)>(GridItem_UpdateBrainAddr)(this);
    }

    GridItem() {
        _constructor();
    }
    ~GridItem() {
        _deconstructor();
    };

    void GridItemDie();
    void DrawGridItem(Sexy::Graphics *g);
    void DrawScaryPot(Sexy::Graphics *g);
    void Update();
    void UpdateMPGraveStone();
    void UpdateScaryPot();
    void UpdateBurialMound();
    void UpdatePole();
    int GetMoundUpgradeCost() const;
    void DrawStinky(Sexy::Graphics *g);
    void DrawSquirrel(Sexy::Graphics *g) const;
    void DrawCrater(Sexy::Graphics *g) const;
    void DrawGraveStone(Sexy::Graphics *g) const;
    void DrawBurialMound(Sexy::Graphics *g) const;
    void AddGraveStoneParticles() const;
    void DrawMPTarget(Sexy::Graphics *g);
    void TakeDamage(int theDamage, unsigned int theDamageFlags);
    void TakeDamage_Origin(int theDamage, unsigned int theDamageFlags);
    Sexy::Rect GetItemRect() const;
    Sexy::Rect GetDebugDrawRect() const;

protected:
    friend void InitHookFunction();

    void _constructor();
    void _deconstructor() {
        reinterpret_cast<void (*)(GridItem *)>(GridItem_GridItemDieAddr)(this);
    };
};

/***************************************************************************************************************/
inline bool transparentVase;


inline void (*old_GridItem_GridItem)(GridItem *);

inline void (*old_GridItem_GridItemDie)(GridItem *);

inline void (*old_GridItem_Update)(GridItem *a1);

inline void (*old_GridItem_DrawGridItem)(GridItem *, Sexy::Graphics *);

inline void (*old_GridItem_UpdateScaryPot)(GridItem *scaryPot);

inline void (*old_GridItem_DrawStinky)(GridItem *mStinky, Sexy::Graphics *graphics);

inline void (*old_GridItem_DrawMPTarget)(GridItem *, Sexy::Graphics *graphics);

inline void (*old_GridItem_TakeDamage)(GridItem *, int theDamage, unsigned int theDamageFlags);

#endif // PVZ_LAWN_BOARD_GRID_ITEM_H
