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

#ifndef PVZ_LAWN_WIDGET_SEED_CHOOSER_SCREEN_H
#define PVZ_LAWN_WIDGET_SEED_CHOOSER_SCREEN_H

#include "PvZ/Lawn/Board/ToolTipWidget.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"
#include "PvZ/Symbols.h"

#include "VSSetupMenu.h"

#include <cstdint>

struct TodWeightedArray;

class Board;
class GameButton;
class ImitaterDialog;
class LawnApp;
class SeedBank;

namespace Sexy {
class MTRand;
}

enum SeedChooserTouchState {
    VIEW_LAWN_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_SEED_CHOOSER,
    SEEDCHOOSER_TOUCHSTATE_STORE_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_START_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_ALMANAC_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_NONE,
};

class ChosenSeed {
public:
    int mX;                     // 0
    int mY;                     // 1
    int mTimeStartMotion;       // 2
    int mTimeEndMotion;         // 3
    int mStartX;                // 4
    int mStartY;                // 5
    int mEndX;                  // 6
    int mEndY;                  // 7
    int mChosenPlayerIndex;     // 8
    SeedType mSeedType;         // 9
    ChosenSeedState mSeedState; // 10
    int mSeedIndexInBank;       // 11
    bool mRefreshing;           // 12
    int mRefreshCounter;        // 13
    SeedType mImitaterType;     // 14
    bool mCrazyDavePicked;      // 60
};

class BannedSeed {
public:
    int mX = 0;
    int mY = 0;
    SeedType mSeedType = SEED_NONE;
    BannedSeedState mSeedState = SEED_NOT_BANNED;
};

using ButtonVector = std::vector<GameButton *>;

class SeedChooserScreen : public Sexy::Widget, public Sexy::ButtonListener {
private:
    enum {
        SeedChooserScreen_Start = 100,
        SeedChooserScreen_Random = 101,
        SeedChooserScreen_ViewLawn = 102,
        SeedChooserScreen_Almanac = 103,
        SeedChooserScreen_Menu = 104,
        SeedChooserScreen_Store = 105,
        SeedChooserScreen_Imitater = 106,
        SeedChooserScreen_Page
    };

public:
    static constexpr uint8_t kCursorMoveOnlyEventFlag = 0x01;
    static constexpr uint8_t kCursorPageOneEventFlag = 0x02;

    enum SeedDir {
        SEED_DIR_UP,
        SEED_DIR_DOWN,
        SEED_DIR_LEFT,
        SEED_DIR_RIGHT,
    };

    bool mShowHelpText;                      // 65
    GameButton *mImitaterButton;             // 66
    ChosenSeed mChosenSeeds[NUM_SEED_TYPES]; // 67 ~ 930
    LawnApp *mApp;                           // 931
    Board *mBoard;                           // 932
    int mSeedChooserAge;                     // 933
    int mSeedsInFlight;                      // 934
    int mSeedsInBank;                        // 935
    int mSeedsIn1PBank;                      // 936
    int mSeedsIn2PBank;                      // 937
    ToolTipWidget *mToolTip1;                // 938
    ToolTipWidget *mToolTip2;                // 939
    int mToolTipSeed1;                       // 940
    int mToolTipSeed2;                       // 941
    int mCursorPositionX1;                   // 942
    int mCursorPositionX2;                   // 943
    int mCursorPositionY1;                   // 944
    int mCursorPositionY2;                   // 945
    SeedChooserState mChooseState;           // 946
    int mViewLawnTime;                       // 947
    bool mOpeningDialog;                     // 3792
    int mPlayerIndex;                        // 949
    int mSeedIndex1;                         // 950
    int mSeedIndex2;                         // 951
    float mCursorBobPhase;                   // 952
    bool mIsZombieChooser;                   // 3812
    SeedBank *mSeedBank1;                    // 954
    SeedBank *mSeedBank2;                    // 955
    int mDimCounter;                         // 956
    ImitaterDialog *mImitaterDialog;         // 957
    GameButton *mViewLawnButton;             // 958
    GameButton *mStoreButton;                // 959
    GameButton *mStartButton;                // 960
    GameButton *mAlmanacButton;              // 961
    homura::Storage<ButtonVector> mButtons;  // 962 ~ 964
    int mButtonSlotState;                    // 965
    NewLawnButton *mPageButton = nullptr;
    int mPageIndex = 0;
    int mNumBanPackets = 2;
    int mSeedsInBanned = 0;
    BannedSeed mBannedSeed[NUM_SEEDS_IN_CHOOSER_EXTENDED]{};
    bool mBanningPhase = false;
    bool mShowExtendedSeeds = false;
    bool mHas7Packets = false;
    bool mGlobalBpBansApplied = false;
    ChosenSeed mChosenSeedsExtended[NUM_SEED_TYPES_EXTENDED]{};
    GameButton *mMainMenuButton = nullptr;

    SeedChooserScreen(bool theIsZombieChooser) {
        _constructor(theIsZombieChooser);
    }

    ~SeedChooserScreen() = delete;

    bool Has7Rows() {
        return reinterpret_cast<bool (*)(SeedChooserScreen *)>(SeedChooserScreen_Has7RowsAddr)(this);
    }
    bool CancelLawnView() {
        return reinterpret_cast<bool (*)(SeedChooserScreen *)>(SeedChooserScreen_CancelLawnViewAddr)(this);
    }
    bool SeedNotAllowedDuringTrial(SeedType theSeedType) {
        return reinterpret_cast<bool (*)(SeedChooserScreen *, SeedType)>(SeedChooserScreen_SeedNotAllowedDuringTrialAddr)(this, theSeedType);
    }
    bool CanPickNow() {
        return reinterpret_cast<bool (*)(SeedChooserScreen *)>(SeedChooserScreen_CanPickNowAddr)(this);
    }
    void RemoveToolTip(int thePlayerIndex) {
        reinterpret_cast<void (*)(SeedChooserScreen *, int)>(SeedChooserScreen_RemoveToolTipAddr)(this, thePlayerIndex);
    }
    bool ShouldDisplayCursor(int thePlayerIndex) {
        return reinterpret_cast<bool (*)(SeedChooserScreen *, int)>(SeedChooserScreen_ShouldDisplayCursorAddr)(this, thePlayerIndex);
    }
    int PickFromWeightedArrayUsingSpecialRandSeed(TodWeightedArray *theArray, int theCount, Sexy::MTRand &theLevelRNG) {
        return reinterpret_cast<int (*)(SeedChooserScreen *, TodWeightedArray *, int, Sexy::MTRand &)>(SeedChooserScreen_PickFromWeightedArrayUsingSpecialRandSeedAddr)(
            this, theArray, theCount, theLevelRNG);
    }
    void UpdateViewLawn() {
        reinterpret_cast<void (*)(SeedChooserScreen *)>(SeedChooserScreen_UpdateViewLawnAddr)(this);
    }
    ChosenSeed &GetChosenSeed(int theIndex) {
        if (theIndex < NUM_SEED_TYPES) {
            return mChosenSeeds[theIndex];
        }

        return mChosenSeedsExtended[theIndex - NUM_SEED_TYPES];
    }
    int GetChosenSeedIndex(const ChosenSeed &theChosenSeed) const {
        const ChosenSeed *aChosenSeed = &theChosenSeed;

        if (aChosenSeed >= mChosenSeeds && aChosenSeed < mChosenSeeds + NUM_SEED_TYPES) {
            return int(aChosenSeed - mChosenSeeds);
        }

        if (aChosenSeed >= mChosenSeedsExtended && aChosenSeed < mChosenSeedsExtended + NUM_SEED_TYPES_EXTENDED) {
            return NUM_SEED_TYPES + int(aChosenSeed - mChosenSeedsExtended);
        }

        return -1;
    }

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void EnableStartButton(int theIsEnabled);
    void RebuildHelpbar();
    bool PickedPlantType(SeedType theSeedType);
    SeedType GetZombieSeedType(int theSeedIndex);
    SeedType GetPlantSeedType(int theSeedIndex) const;
    int GetSeedStorageCount() const;
    int GetCurrentPageSeedCount() const;
    int GetPageSeedStorageIndex(int theSeedIndex) const;
    int GetSeedPacketIndex(int theSeedIndex) const;
    void OnPlayerPickedSeed(int thePlayerIndex);
    void ClickedSeedInChooser(ChosenSeed &theChosenSeed, int thePlayerIndex);
    void ClickedSeedInChooser_Orgin(ChosenSeed &theChosenSeed, int thePlayerIndex);
    void CrazyDavePickSeeds();
    void OnStartButton();
    void Update();
    void UpdateBuiltinAIPick();
    void UpdateImitaterButton();
    void UpdateCursor();
    void UpdateAfterPurchase();
    void PickRandomSeeds();
    void CloseSeedChooser();
    void LandFlyingSeed(ChosenSeed &theChosenSeed);
    bool SeedNotAllowedToPick(SeedType theSeedType);
    unsigned int SeedNotRecommendedToPick(SeedType theSeedType);
    SeedType FindSeedInBank(int theIndexInBank, int thePlayerIndex);
    void ClickedSeedInBank(ChosenSeed &theChosenSeed, int thePlayerIndex);
    bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theEventFlag);
    /*virtual*/ void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag);
    void
    DrawPacket(Sexy::Graphics *g, int x, int y, SeedType theSeedType, SeedType theImitaterType, float thePercentDark, int theGrayness, Sexy::Color *theColor, bool theDrawCost, bool theUseCurrentCost);
    void GetSeedPositionInBank(int theIndex, int &x, int &y, int thePlayerIndex);
    void GetSeedPositionInChooser(int theIndex, int &x, int &y);
    int NumColumns() const;
    void ShowToolTip(int thePlayerIndex);
    int GetNextSeedInDir(int theNumSeed, SeedDir theMoveDirection);
    void Draw(Sexy::Graphics *g);
    void DrawBanIcon(Sexy::Graphics *g);
    void SetPageIndex(int thePageIndex);
    SeedType SeedHitTest(int x, int y);
    SeedType SeedHitTest_Origin(int x, int y);
    void VSAutoPickResourceGen();
    int ResolveGlobalBpPlayerIndex() const;
    void ApplyGlobalBpBans();
    bool HasPacket(SeedType theSeedType, bool theIsZombieChooser);

    void MouseMove(int x, int y);
    void MouseDown(int x, int y, int theClickCount);
    void MouseUp(int x, int y);
    void MouseDrag(int x, int y);
    bool KeyDown(Sexy::KeyCode theKey);
    bool KeyUp(Sexy::KeyCode theKey);
    void ButtonPress(int theId);
    void ButtonDepress(int theId);
    void ButtonDepress_Origin(int theId);

protected:
    friend void InitHookFunction();

    void _constructor(bool theIsZombieChooser);
    void _destructor();
};


inline SeedChooserTouchState gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_NONE;

inline bool daveNoPickSeeds;

/***************************************************************************************************************/

inline void (*old_SeedChooserScreen_AddedToManager)(SeedChooserScreen *, Sexy::WidgetManager *);

inline void (*old_SeedChooserScreen_RemovedFromManager)(SeedChooserScreen *, Sexy::WidgetManager *);

inline bool (*old_SeedChooserScreen_KeyDown)(SeedChooserScreen *seedChooserScreen, Sexy::KeyCode theKey);

inline bool (*old_SeedChooserScreen_KeyUp)(SeedChooserScreen *seedChooserScreen, Sexy::KeyCode theKey);

inline void (*old_SeedChooserScreen_RebuildHelpbar)(SeedChooserScreen *seedChooserScreen);

inline void (*old_SeedChooserScreen__destructor)(SeedChooserScreen *);

inline void (*old_SeedChooserScreen_EnableStartButton)(SeedChooserScreen *seedChooserScreen, int isEnabled);

inline void (*old_SeedChooserScreen_OnStartButton)(SeedChooserScreen *seedChooserScreen);

inline bool (*old_SeedChooserScreen_SeedNotAllowedToPick)(SeedChooserScreen *seedChooserScreen, SeedType theSeedType);

inline void (*old_SeedChooserScreen_MouseMove)(SeedChooserScreen *seedChooserScreen, int x, int y);

inline void (*old_SeedChooserScreen_MouseDown)(SeedChooserScreen *a, int x, int y, int theClickCount);

inline void (*old_SeedChooserScreen_MouseDrag)(SeedChooserScreen *seedChooserScreen, int x, int y);

inline void (*old_SeedChooserScreen_MouseUp)(SeedChooserScreen *seedChooserScreen, int x, int y);

inline void (*old_SeedChooserScreen_Draw)(SeedChooserScreen *, Sexy::Graphics *);

inline bool (*old_SeedChooserScreen_OnKeyDown)(SeedChooserScreen *, Sexy::KeyCode, unsigned int);

#endif // PVZ_LAWN_WIDGET_SEED_CHOOSER_SCREEN_H
