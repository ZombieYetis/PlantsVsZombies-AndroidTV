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

#ifndef PVZ_LAWN_BOARD_BOARD_H
#define PVZ_LAWN_BOARD_BOARD_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Widget/AchievementsWidget.h"
#include "PvZ/NetPlay.h"
#include "PvZ/STL/map.h"
#include "PvZ/STL/set.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/TodLib/Common/DataArray.h"
#include "PvZ/TodLib/Common/TodCommon.h"

#include "Coin.h"
#include "GridItem.h"
#include "LawnMower.h"
#include "Plant.h"
#include "Projectile.h"
#include "Zombie.h"

inline constexpr int MAX_GRID_SIZE_X = 9;
inline constexpr int MAX_GRID_SIZE_Y = 6;
inline constexpr int MAX_ZOMBIES_IN_WAVE = 50;
inline constexpr int MAX_ZOMBIE_WAVES = 100;
inline constexpr int MAX_GRAVE_STONES = MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y;
inline constexpr int MAX_POOL_GRID_SIZE = 10;
inline constexpr int MAX_RENDER_ITEMS = 2048;
inline constexpr int PROGRESS_METER_COUNTER = 150;

struct BaseEvent;

class Challenge;
class CursorObject;
class CursorPreview;
class CutScene;
class CustomMessageWidget;
class GameButton;
class GamepadControls;
class LawnApp;
class Reanimation;
class ReplayControlsWidget;
class SaveGameContext;
class SeedBank;
class SeedPacket;
class TodParticleSystem;
class ToolTipWidget;
class ShovelRedirectWidget;

class HitResult {
public:
    void *mObject;
    GameObjectType mObjectType;
};

class RenderItem {
public:
    RenderObjectType mRenderObjectType;
    int mZPos;
    union {
        GameObject *mGameObject;
        Plant *mPlant;
        Zombie *mZombie;
        Coin *mCoin;
        Projectile *mProjectile;
        CursorPreview *mCursorPreview;
        TodParticleSystem *mParticleSytem;
        Reanimation *mReanimation;
        GridItem *mGridItem;
        LawnMower *mMower;
        BossPart mBossPart;
        int mBoardGridY;
    };
};

struct ZombiePicker {
    int mZombieCount;
    int mZombiePoints;
    int mZombieTypeCount[ZombieType::EXTENDED_NUM_ZOMBIE_TYPES];
    int mAllWavesZombieTypeCount[ZombieType::EXTENDED_NUM_ZOMBIE_TYPES];
};

/*inline*/ void ZombiePickerInitForWave(ZombiePicker *theZombiePicker);
/*inline*/ void ZombiePickerInit(ZombiePicker *theZombiePicker);

struct PlantsOnLawn {
    Plant *mUnderPlant;
    Plant *mPumpkinPlant;
    Plant *mFlyingPlant;
    Plant *mNormalPlant;
};

struct BungeeDropGrid {
    TodWeightedGridArray mGridArray[MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y];
    int mGridArrayCount;
};

using PlantRbTree = pvzstl::set<Plant *>;
using StringIntMap = pvzstl::map<pvzstl::string, int>;
using StringSetLayout = pvzstl::set<pvzstl::string>;

enum TouchState {
    TOUCHSTATE_NONE = 0,
    TOUCHSTATE_SEED_BANK = 1,
    TOUCHSTATE_SHOVEL_RECT = 2,
    TOUCHSTATE_BUTTER_RECT = 3,
    TOUCHSTATE_BOARD = 4,
    TOUCHSTATE_BOARD_MOVED_FROM_SEED_BANK = 5,
    TOUCHSTATE_BOARD_MOVED_FROM_SHOVEL_RECT = 6,
    TOUCHSTATE_BOARD_MOVED_FROM_BUTTER_RECT = 7,
    TOUCHSTATE_VALID_COBCONON = 8,
    TOUCHSTATE_USEFUL_SEED_PACKET = 9,
    TOUCHSTATE_UNUSED = 10,
    TOUCHSTATE_HEAVY_WEAPON = 11,
    TOUCHSTATE_PICKING_SOMETHING = 12,
    TOUCHSTATE_ZEN_GARDEN_TOOLS = 13,
    TOUCHSTATE_BOARD_MOVED_FROM_ZEN_GARDEN_TOOLS = 14,
    TOUCHSTATE_VALID_COBCONON_SECOND = 15,
};

class Board : public Sexy::Widget, public Sexy::ButtonListener, public SyncObject {
    struct BoardVTable {
        void (*completeDestructor)(Board *self);                                                                                                                                            // 0x000
        void (*deletingDestructor)(Board *self);                                                                                                                                            // 0x004
        Sexy::Rect (*GetRect)(Sexy::WidgetContainer *self);                                                                                                                                 // 0x008
        bool (*Intersects)(Sexy::WidgetContainer *self, Sexy::WidgetContainer *other);                                                                                                      // 0x00C
        void (*SortWidgets)(Sexy::WidgetContainer *self);                                                                                                                                   // 0x010
        void (*SortWidgetsWithWidget)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                   // 0x014
        void (*AddWidget)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                               // 0x018
        void (*RemoveWidget)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                            // 0x01C
        bool (*HasWidget)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                               // 0x020
        Sexy::Widget *(*FindWidget)(Sexy::WidgetContainer *self, int index);                                                                                                                // 0x024
        void (*DisableWidget)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                           // 0x028
        void (*RemoveAllWidgets)(Sexy::WidgetContainer *self, bool deleteWidgets, bool recursive, Sexy::WidgetManager *manager);                                                            // 0x02C
        void (*SetFocusContainer)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                       // 0x030
        bool (*IsBelow)(Sexy::WidgetContainer *self, Sexy::Widget *first, Sexy::Widget *second);                                                                                            // 0x034
        void (*MarkAllDirty)(Sexy::WidgetContainer *self);                                                                                                                                  // 0x038
        void (*BringToFront)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                            // 0x03C
        void (*BringToBack)(Sexy::WidgetContainer *self, Sexy::Widget *widget);                                                                                                             // 0x040
        void (*PutBehind)(Sexy::WidgetContainer *self, Sexy::Widget *first, Sexy::Widget *second);                                                                                          // 0x044
        void (*PutInfront)(Sexy::WidgetContainer *self, Sexy::Widget *first, Sexy::Widget *second);                                                                                         // 0x048
        Sexy::Point (*GetAbsPos)(const Sexy::WidgetContainer *self);                                                                                                                        // 0x04C
        Sexy::Point (*GetAbsPosInManager)(const Sexy::WidgetContainer *self);                                                                                                               // 0x050
        Sexy::Point (*GetAbsPosInScreen)(const Sexy::WidgetContainer *self);                                                                                                                // 0x054
        Sexy::Point (*GetCenter)(const Sexy::WidgetContainer *self);                                                                                                                        // 0x058
        Sexy::Point (*GetAbsCenter)(const Sexy::WidgetContainer *self);                                                                                                                     // 0x05C
        Sexy::Point (*GetAbsCenterInScreen)(const Sexy::WidgetContainer *self);                                                                                                             // 0x060
        void (*MarkDirty)(Sexy::WidgetContainer *self);                                                                                                                                     // 0x064
        void (*MarkDirtyFull)(Sexy::WidgetContainer *self);                                                                                                                                 // 0x068
        void (*MarkDirtyFullWithContainer)(Sexy::WidgetContainer *self, Sexy::WidgetContainer *container);                                                                                  // 0x06C
        void (*MarkDirtyWithContainer)(Sexy::WidgetContainer *self, Sexy::WidgetContainer *container);                                                                                      // 0x070
        void (*AddedToManager)(Sexy::WidgetContainer *self, Sexy::WidgetManager *manager);                                                                                                  // 0x074
        void (*RemovedFromManager)(Board *self, Sexy::WidgetManager *manager);                                                                                                              // 0x078
        void (*Update)(Board *self);                                                                                                                                                        // 0x07C
        void (*UpdateAll)(Sexy::WidgetContainer *self, Sexy::ModalFlags *modalFlags);                                                                                                       // 0x080
        void (*UpdateF)(Sexy::Widget *self, float deltaTime);                                                                                                                               // 0x084
        void (*UpdateFAll)(Sexy::WidgetContainer *self, Sexy::ModalFlags *modalFlags, float deltaTime);                                                                                     // 0x088
        void (*DrawPre)(Sexy::Widget *self, Sexy::Graphics *graphics);                                                                                                                      // 0x08C
        void (*Draw)(Board *self, Sexy::Graphics *graphics);                                                                                                                                // 0x090
        void (*DrawOther)(Sexy::Widget *self, Sexy::Graphics *graphics);                                                                                                                    // 0x094
        void (*DrawAll)(Sexy::WidgetContainer *self, Sexy::ModalFlags *modalFlags, Sexy::Graphics *graphics);                                                                               // 0x098
        void (*SysColorChangedAll)(Sexy::WidgetContainer *self);                                                                                                                            // 0x09C
        void (*SysColorChanged)(Sexy::WidgetContainer *self);                                                                                                                               // 0x0A0
        void (*OrderInManagerChanged)(Sexy::Widget *self);                                                                                                                                  // 0x0A4
        void (*SetVisible)(Sexy::Widget *self, bool visible);                                                                                                                               // 0x0A8
        void (*SetColors3)(Sexy::Widget *self, int (*colors)[3], int count);                                                                                                                // 0x0AC
        void (*SetColors4)(Sexy::Widget *self, int (*colors)[4], int count);                                                                                                                // 0x0B0
        void (*SetColor)(Sexy::Widget *self, int index, const Sexy::Color &color);                                                                                                          // 0x0B4
        Sexy::Color (*GetColor)(Sexy::Widget *self, int index);                                                                                                                             // 0x0B8
        Sexy::Color (*GetColorWithDefault)(Sexy::Widget *self, int index, const Sexy::Color &defaultColor);                                                                                 // 0x0BC
        void (*SetDisabled)(Sexy::Widget *self, bool disabled);                                                                                                                             // 0x0C0
        void (*ShowFinger)(Sexy::Widget *self, bool show);                                                                                                                                  // 0x0C4
        void (*Resize)(Sexy::Widget *self, int x, int y, int width, int height);                                                                                                            // 0x0C8
        void (*ResizeRect)(Sexy::Widget *self, const Sexy::TRect<int> &rect);                                                                                                               // 0x0CC
        void (*Move)(Board *self, int x, int y);                                                                                                                                            // 0x0D0
        bool (*WantsFocus)(Sexy::Widget *self);                                                                                                                                             // 0x0D4
        void (*DrawOverlay)(Board *self, Sexy::Graphics *graphics);                                                                                                                         // 0x0D8
        void (*DrawOverlayWithPriority)(Sexy::Widget *self, Sexy::Graphics *graphics, int priority);                                                                                        // 0x0DC
        void (*SetDefaultFocus)(Sexy::Widget *self, Sexy::Widget *widget);                                                                                                                  // 0x0E0
        void (*SetFocus)(Sexy::Widget *self, Sexy::Widget *widget, bool force);                                                                                                             // 0x0E4
        void (*SetFocusLink)(Sexy::Widget *self, int direction, Sexy::Widget *widget);                                                                                                      // 0x0E8
        void (*SetFocusLinks)(Sexy::Widget *self, Sexy::Widget *up, Sexy::Widget *down, Sexy::Widget *left, Sexy::Widget *right);                                                           // 0x0EC
        void (*FocusDirectionHint)(Sexy::Widget *self, int direction);                                                                                                                      // 0x0F0
        void (*GotFocus)(Board *self);                                                                                                                                                      // 0x0F4
        void (*LostFocus)(Sexy::Widget *self);                                                                                                                                              // 0x0F8
        bool (*HasWindowFocus)(Sexy::Widget *self);                                                                                                                                         // 0x0FC
        void (*SetWindowFocus)(Sexy::Widget *self, bool focused);                                                                                                                           // 0x100
        void (*GotWindowFocus)(Sexy::Widget *self);                                                                                                                                         // 0x104
        void (*LostWindowFocus)(Sexy::Widget *self);                                                                                                                                        // 0x108
        void (*KeyChar)(Board *self, char character);                                                                                                                                       // 0x10C
        void (*KeyUnicode)(Sexy::Widget *self, int character);                                                                                                                              // 0x110
        void (*KeyDownEvent)(Sexy::Widget *self, const Sexy::Event &event);                                                                                                                 // 0x114
        void (*KeyUpEvent)(Sexy::Widget *self, const Sexy::Event &event);                                                                                                                   // 0x118
        void (*KeyDown)(Board *self, Sexy::KeyCode keyCode);                                                                                                                                // 0x11C
        void (*KeyUp)(Board *self, Sexy::KeyCode keyCode);                                                                                                                                  // 0x120
        void (*MouseEnter)(Sexy::Widget *self);                                                                                                                                             // 0x124
        void (*MouseLeave)(Sexy::Widget *self);                                                                                                                                             // 0x128
        void (*MouseMove)(Board *self, int x, int y);                                                                                                                                       // 0x12C
        void (*MouseDown3)(Board *self, int x, int y, int clickCount);                                                                                                                      // 0x130
        void (*MouseDown4)(Sexy::Widget *self, int x, int y, int button, int clickCount);                                                                                                   // 0x134
        void (*MouseUp2)(Sexy::Widget *self, int x, int y);                                                                                                                                 // 0x138
        void (*MouseUp3)(Board *self, int x, int y, int clickCount);                                                                                                                        // 0x13C
        void (*MouseUp4)(Sexy::Widget *self, int x, int y, int button, int clickCount);                                                                                                     // 0x140
        void (*MouseDrag)(Board *self, int x, int y);                                                                                                                                       // 0x144
        void (*MouseWheel)(Sexy::Widget *self, int delta);                                                                                                                                  // 0x148
        void (*MouseWheelWithPosition)(Sexy::Widget *self, int x, int y);                                                                                                                   // 0x14C
        void (*TouchEnter)(Sexy::Widget *self);                                                                                                                                             // 0x150
        void (*TouchLeave)(Sexy::Widget *self);                                                                                                                                             // 0x154
        void (*TouchDown3)(Sexy::Widget *self, int touchId, int x, int y);                                                                                                                  // 0x158
        void (*TouchMove3)(Sexy::Widget *self, int touchId, int x, int y);                                                                                                                  // 0x15C
        void (*TouchUp3)(Sexy::Widget *self, int touchId, int x, int y);                                                                                                                    // 0x160
        void (*TouchCancel3)(Sexy::Widget *self, int touchId, int x, int y);                                                                                                                // 0x164
        void (*TouchEvents)(Sexy::Widget *self, const std::vector<Sexy::Event> &events);                                                                                                    // 0x168
        void (*TouchDownEvents)(Sexy::Widget *self, const std::vector<Sexy::Event> &events);                                                                                                // 0x16C
        void (*TouchMoveEvents)(Sexy::Widget *self, const std::vector<Sexy::Event> &events);                                                                                                // 0x170
        void (*TouchUpEvents)(Sexy::Widget *self, const std::vector<Sexy::Event> &events);                                                                                                  // 0x174
        void (*TouchCancelEvents)(Sexy::Widget *self, const std::vector<Sexy::Event> &events);                                                                                              // 0x178
        void (*TouchBeganVector)(Sexy::Widget *self, std::vector<Sexy::Touch> &touches);                                                                                                    // 0x17C
        void (*TouchMovedVector)(Sexy::Widget *self, std::vector<Sexy::Touch> &touches);                                                                                                    // 0x180
        void (*TouchEndedVector)(Sexy::Widget *self, std::vector<Sexy::Touch> &touches);                                                                                                    // 0x184
        void (*TouchCanceledVector)(Sexy::Widget *self, std::vector<Sexy::Touch> &touches);                                                                                                 // 0x188
        void (*TouchBegan)(Sexy::Widget *self, Sexy::Touch *touch);                                                                                                                         // 0x18C
        void (*TouchMoved)(Sexy::Widget *self, Sexy::Touch *touch);                                                                                                                         // 0x190
        void (*TouchEnded)(Sexy::Widget *self, Sexy::Touch *touch);                                                                                                                         // 0x194
        void (*TouchCanceled)(Sexy::Widget *self, Sexy::Touch *touch);                                                                                                                      // 0x198
        void (*TouchesCanceled)(Sexy::Widget *self);                                                                                                                                        // 0x19C
        void (*AxisMoved)(Sexy::Widget *self, const Sexy::Event &event);                                                                                                                    // 0x1A0
        void (*UserEvent)(Sexy::Widget *self, const Sexy::Event &event);                                                                                                                    // 0x1A4
        bool (*IsPointVisible)(Sexy::Widget *self, int x, int y);                                                                                                                           // 0x1A8
        bool (*IsOnScreen)(Sexy::Widget *self);                                                                                                                                             // 0x1AC
        void (*WriteCenteredLine)(Sexy::Widget *self, Sexy::Graphics *graphics, int y, const std::string &text);                                                                            // 0x1B0
        void (*WriteCenteredLineEx)(Sexy::Widget *self, Sexy::Graphics *graphics, int y, const std::string &text, Sexy::Color color1, Sexy::Color color2, const Sexy::TPoint<int> &offset); // 0x1B4
        void (*WriteString)(Sexy::Widget *self, Sexy::Graphics *graphics, const std::string &text, int x, int y, int width, int justification, bool drawString, int offset, int length);    // 0x1B8
        void (*WriteWordWrapped)(Sexy::Widget *self, Sexy::Graphics *graphics, const Sexy::TRect<int> &rect, const std::string &text, int lineSpacing, int justification);                  // 0x1BC
        int (*GetWordWrappedHeight)(Sexy::Widget *self, Sexy::Graphics *graphics, int width, const std::string &text, int lineSpacing);                                                     // 0x1C0
        int (*GetNumDigits)(Sexy::Widget *self, int number);                                                                                                                                // 0x1C4
        void (*WriteNumberFromStrip)(Sexy::Widget *self, Sexy::Graphics *graphics, int number, int x, int y, Sexy::Image *image, int digitWidth);                                           // 0x1C8
        bool (*Contains)(Sexy::Widget *self, int x, int y);                                                                                                                                 // 0x1CC
        Sexy::Rect (*GetInsetRect)(Sexy::Widget *self);                                                                                                                                     // 0x1D0
        Sexy::Widget *(*FindFocusableWidget)(Sexy::Widget *self, int direction, Sexy::Widget *relativeWidget);                                                                              // 0x1D4
        Sexy::Widget *(*FindClosest)(Sexy::Widget *self, Sexy::Widget *widget);                                                                                                             // 0x1D8
        void (*OnArrowKeys)(Sexy::Widget *self, Sexy::KeyCode keyCode);                                                                                                                     // 0x1DC
        void (*OnKeyReturn)(Sexy::Widget *self);                                                                                                                                            // 0x1E0
        void (*OnKeyEscape)(Sexy::Widget *self);                                                                                                                                            // 0x1E4
        void (*ButtonMouseEnter)(Board *self, int id);                                                                                                                                      // 0x1E8
        void (*ButtonMouseLeave)(Board *self, int id);                                                                                                                                      // 0x1EC
        void (*ButtonPress)(Board *self, int id);                                                                                                                                           // 0x1F0
    };

public:
    LawnApp *mApp;                                                    // 69
    DataArray<Zombie> mZombies;                                       // 70 ~ 76
    DataArray<Plant> mPlants;                                         // 77 ~ 83
    DataArray<Projectile> mProjectiles;                               // 84 ~ 90
    DataArray<Coin> mCoins;                                           // 91 ~ 97
    DataArray<LawnMower> mLawnMowers;                                 // 98 ~ 104
    DataArray<GridItem> mGridItems;                                   // 105 ~ 111
    homura::Storage<PlantRbTree> mTangleKelpTree;                     // 112 ~ 117
    homura::Storage<PlantRbTree> mFlowerPotTree;                      // 118 ~ 123
    homura::Storage<PlantRbTree> mPumpkinTree;                        // 124 ~ 129
    CustomMessageWidget *mAdvice;                                     // 130
    SeedBank *mSeedBank[2];                                           // 131 ~ 132
    int mUnknown214;                                                  // 133
    homura::Storage<StringIntMap> mUnknownStringIntMap;               // 134 ~ 139
    GamepadControls *mGamepadControls[2];                             // 140 ~ 141
    CursorObject *mCursorObject[2];                                   // 142 ~ 143
    CursorPreview *mCursorPreview[2];                                 // 144 ~ 145
    ToolTipWidget *mToolTip;                                          // 146
    Sexy::Font *mDebugFont;                                           // 147
    CutScene *mCutScene;                                              // 148
    Challenge *mChallenge;                                            // 149
    bool unknownBool;                                                 // 600
    bool mPaused;                                                     // 601
    GridSquareType mGridSquareType[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y]; // 151 ~ 204
    int mGridCelLook[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y];               // 205 ~ 258
    int mGridCelOffset[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y][2];          // 259 ~ 366
    int mGridCelFog[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y + 1];            // 367 ~ 429
    bool mEnableGraveStones;                                          // 1720
    int mSpecialGraveStoneX;                                          // 431
    int mSpecialGraveStoneY;                                          // 432
    float mFogOffset;                                                 // 433
    int mOffsetMoved;                                                 // 434
    ReanimationID mCoverLayerAnimIDs[7];                              // 435 ~ 441
    int mFogBlownCountDown;                                           // 442
    PlantRowType mPlantRow[MAX_GRID_SIZE_Y];                          // 443 ~ 448
    int mWaveRowGotLawnMowered[MAX_GRID_SIZE_Y];                      // 449 ~ 454
    int mBonusLawnMowersRemaining;                                    // 455
    int mIceMinX[MAX_GRID_SIZE_Y];                                    // 456 ~ 461
    int mIceTimer[MAX_GRID_SIZE_Y];                                   // 462 ~ 467
    ParticleSystemID mIceParticleID[MAX_GRID_SIZE_Y];                 // 468 ~ 473
    TodSmoothArray mRowPickingArray[MAX_GRID_SIZE_Y];                 // 474 ~ 497
    ZombieType mZombiesInWave[MAX_ZOMBIE_WAVES][MAX_ZOMBIES_IN_WAVE]; // 498 ~ 5497
    bool mZombieAllowed[100];                                         // 5498 ~ 5522
    int mSunCountDown;                                                // 5523
    int mNumSunsFallen;                                               // 5524
    int mShakeCounter;                                                // 5525
    int mShakeAmountX;                                                // 5526
    int mShakeAmountY;                                                // 5527
    BackgroundType mBackground;                                       // 5528
    int mLevel;                                                       // 5529
    int mSodPosition;                                                 // 5530
    int mPrevMouseX;                                                  // 5531
    int mPrevMouseY;                                                  // 5532
    int mSunMoney1;                                                   // 5533
    int mSunMoney2;                                                   // 5534
    int mDeathMoney;                                                  // 5535
    int mNumWaves;                                                    // 5536
    int mMainCounter;                                                 // 5537
    int mEffectCounter;                                               // 5538
    int mDrawCount;                                                   // 5539
    int mRiseFromGraveCounter;                                        // 5540
    int mOutOfMoneyCounter;                                           // 5541
    int mCurrentWave;                                                 // 5542
    int mTotalSpawnedWaves;                                           // 5543
    TutorialState mTutorialState;                                     // 5544
    int *mTutorialParticleID;                                         // 5545
    int mTutorialTimer;                                               // 5546
    int mLastBungeeWave;                                              // 5547
    int mZombieHealthToNextWave;                                      // 5548
    int mZombieHealthWaveStart;                                       // 5549
    int mZombieCountDown;                                             // 5550
    int mZombieCountDownStart;                                        // 5551
    int mHugeWaveCountDown;                                           // 5552
    bool mHelpDisplayed[72];                                          // 5553 ~ 5570
    AdviceType mHelpIndex;                                            // 5571
    bool mFinalBossKilled;                                            // 22288
    bool mShowShovel;                                                 // 22289
    bool mShowButter;                                                 // 22290
    bool mShowHammer;                                                 // 22291
    int mCoinBankFadeCount;                                           // 5573
    DebugTextMode mDebugTextMode;                                     // 5574
    bool mLevelComplete;                                              // 22300
    bool mNewWallNutAndSunFlowerAndChomperOnly;                       // 在对齐间隙插入成员，22301
    char mNewPeaShooterCount;                                         // 在对齐间隙插入成员，22302
    int mBoardFadeOutCounter;                                         // 5576
    int mNextSurvivalStageCounter;                                    // 5577
    int mScoreNextMowerCounter;                                       // 5578
    bool mLevelAwardSpawned;                                          // 22316
    int mProgressMeterWidth;                                          // 5580
    int mFlagRaiseCounter;                                            // 5581
    int mIceTrapCounter;                                              // 5582
    int mBoardRandSeed;                                               // 5583
    ParticleSystemID mPoolSparklyParticleID;                          // 5584
    ReanimationID mFwooshID[MAX_GRID_SIZE_Y][12];                     // 5585 ~ 5656
    int mFwooshCountDown;                                             // 5657
    int mTimeStopCounter;                                             // 5658
    bool mDroppedFirstCoin;                                           // 22636
    int mFinalWaveSoundCounter;                                       // 5660
    int mCobCannonCursorDelayCounter;                                 // 5661
    int mCobCannonMouseX;                                             // 5662
    int mCobCannonMouseY;                                             // 5663
    bool mKilledYeti;                                                 // 22656
    bool mMustacheMode;                                               // 22657
    bool mSuperMowerMode;                                             // 22658
    bool mFutureMode;                                                 // 22659
    bool mPinataMode;                                                 // 22660
    bool mDanceMode;                                                  // 22661
    bool mDaisyMode;                                                  // 22662
    bool mSukhbirMode;                                                // 22663
    int mPrevBoardResult;                                             // 5666
    int mTriggeredLawnMowers;                                         // 5667
    int mPlayTimeActiveLevel;                                         // 5668
    int mPlayTimeInactiveLevel;                                       // 5669
    int mMaxSunPlants;                                                // 5670
    int mStartDrawTime;                                               // 5671
    int mIntervalDrawTime;                                            // 5672
    int mIntervalDrawCountStart;                                      // 5673
    float mMinFPS;                                                    // 5674
    int mPreloadTime;                                                 // 5675
    int mGameID;                                                      // 5676
    int mGravesCleared;                                               // 5677
    int mPlantsEaten;                                                 // 5678
    int mPlantsShoveled;                                              // 5679
    int mCoinsCollected;                                              // 5680
    int mDiamondsCollected;                                           // 5681
    int mPottedPlantsCollected;                                       // 5682
    int mChocolateCollected;                                          // 5683
    bool mPeaShooterUsed;                                             // 22736
    bool mCatapultPlantsUsed;                                         // 22737
    int mCollectedCoinStreak;                                         // 5685
    int mUnkIntSecondPlayer1;                                         // 5686
    bool mUnkBoolSecondPlayer;                                        // 22748
    int mUnkIntSecondPlayer2;                                         // 5688
    homura::Storage<pvzstl::string> mStringSecondPlayer;              // 5689
    bool mUnknown58E8;                                                // 0x58E8
    bool mUnknown58E9;                                                // 0x58E9
    std::uint8_t mPadding58EA[2];
    homura::Storage<StringSetLayout> mUnknownStringSet; // 0x58EC
    int mUnknown5904;                                   // 0x5904
    GameButton *mBoardMenuButton = nullptr;             // 新增成员
    GameButton *mBoardStoreButton = nullptr;            // 新增成员
    ShovelRedirectWidget *mShovelWidget = nullptr;
    ReplayControlsWidget *mReplayControlsWidget = nullptr;
    bool mJacksonDanceMode = false;

    Projectile *AddProjectile(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType);
    void SpawnTeleportEffect(float theX, float theY, int theRow);
    void TeleportZombie(Zombie *theZombie, float theDestX);
    bool TeleportPlant(Plant *thePlant, int theDestGridX, int theDestGridY);
    int PixelToGridX(int theX, int theY) {
        return reinterpret_cast<int (*)(Board *, int, int)>(Board_PixelToGridXAddr)(this, theX, theY);
    }
    int PixelToGridY(int theX, int theY) {
        return reinterpret_cast<int (*)(Board *, int, int)>(Board_PixelToGridYAddr)(this, theX, theY);
    }
    const BoardVTable *GetVTable() const {
        return (BoardVTable *)Widget::vTable;
    }
    void ResetFPSStats() {
        reinterpret_cast<void (*)(Board *)>(Board_ResetFPSStatsAddr)(this);
    }
    GridItem *GetCraterAt(int theGridX, int theGridY);
    GridItem *GetGraveStoneAt(int theGridX, int theGridY);
    GridItem *GetLadderAt(int theGridX, int theGridY);
    GridItem *GetScaryPotAt(int theGridX, int theGridY);
    GridItem *GetMoundAt(int theGridX, int theGridY);
    GridItem *GetGridItemAt(GridItemType theGridItemType, int theGridX, int theGridY);
    void Move(int theX, int theY) {
        reinterpret_cast<void (*)(Board *, int, int)>(Board_MoveAddr)(this, theX, theY);
    } // 整体移动整个草坪，包括种子栏和铲子按钮等等。
    void DoFwoosh(int theRow) {
        reinterpret_cast<void (*)(Board *, int)>(Board_DoFwooshAddr)(this, theRow);
    }
    void DoChillyFwoosh(int theRow, float theX, float theY);
    bool IteratePlants(Plant *&thePlant) {
        return reinterpret_cast<bool (*)(Board *, Plant *&)>(Board_IteratePlantsAddr)(this, thePlant);
    }
    bool IterateZombies(Zombie *&theZombie) {
        return reinterpret_cast<bool (*)(Board *, Zombie *&)>(Board_IterateZombiesAddr)(this, theZombie);
    }
    bool IterateProjectiles(Projectile *&theProjectile);
    bool IterateCoins(Coin *&theCoin) {
        return reinterpret_cast<bool (*)(Board *, Coin *&)>(Board_IterateCoinsAddr)(this, theCoin);
    }
    bool IterateLawnMowers(LawnMower *&theLawnMower) {
        return reinterpret_cast<bool (*)(Board *, LawnMower *&)>(Board_IterateLawnMowersAddr)(this, theLawnMower);
    }
    bool IterateParticles(TodParticleSystem *&theParticle) {
        return reinterpret_cast<bool (*)(Board *, TodParticleSystem *&)>(Board_IterateParticlesAddr)(this, theParticle);
    }
    bool IterateReanimations(Reanimation *&theReanimation) {
        return reinterpret_cast<bool (*)(Board *, Reanimation *&)>(Board_IterateReanimationsAddr)(this, theReanimation);
    }
    bool IterateGridItems(GridItem *&theGridItem) {
        return reinterpret_cast<bool (*)(Board *, GridItem *&)>(Board_IterateGridItemsAddr)(this, theGridItem);
    }
    Plant *GetTopPlantAt(int theGridX, int theGridY, PlantPriority thePriority) {
        return reinterpret_cast<Plant *(*)(Board *, int, int, PlantPriority)>(Board_GetTopPlantAtAddr)(this, theGridX, theGridY, thePriority);
    }
    bool ProgressMeterHasFlags() {
        return reinterpret_cast<bool (*)(Board *)>(Board_ProgressMeterHasFlagsAddr)(this);
    }
    bool IsSurvivalStageWithRepick() {
        return reinterpret_cast<bool (*)(Board *)>(Board_IsSurvivalStageWithRepickAddr)(this);
    }
    void PickUpTool(GameObjectType theObjectType, int thePlayerIndex) {
        reinterpret_cast<void (*)(Board *, GameObjectType, int)>(Board_PickUpToolAddr)(this, theObjectType, thePlayerIndex);
    }
    int GameAxisMove(int theButton, int thePlayerIndex, int theIsLongPress) {
        return reinterpret_cast<int (*)(Board *, int, int, int)>(Board_GameAxisMoveAddr)(this, theButton, thePlayerIndex, theIsLongPress);
    }
    int InitCoverLayer() {
        return reinterpret_cast<int (*)(Board *)>(Board_InitCoverLayerAddr)(this);
    }
    void LoadBackgroundImages() {
        reinterpret_cast<void (*)(Board *)>(Board_LoadBackgroundImagesAddr)(this);
    }
    bool HasConveyorBeltSeedBank(int thePlayerIndex) {
        return reinterpret_cast<bool (*)(Board *, int)>(Board_HasConveyorBeltSeedBankAddr)(this, thePlayerIndex);
    }
    Zombie *ZombieHitTest(int theMouseX, int theMouseY, int thePlayerIndex) {
        return reinterpret_cast<Zombie *(*)(Board *, int, int, int)>(Board_ZombieHitTestAddr)(this, theMouseX, theMouseY, thePlayerIndex);
    }
    void ClearAdviceImmediately() {
        reinterpret_cast<void (*)(Board *)>(Board_ClearAdviceImmediatelyAddr)(this);
    }
    void DisplayAdvice(const pvzstl::string &theAdvice, MessageStyle theMessageStyle, AdviceType theHelpIndex) {
        reinterpret_cast<void (*)(Board *, const pvzstl::string &, MessageStyle, AdviceType)>(Board_DisplayAdviceAddr)(this, theAdvice, theMessageStyle, theHelpIndex);
    }
    void DisplayAdviceAgain(const pvzstl::string &theAdvice, MessageStyle theMessageStyle, AdviceType theHelpIndex) {
        reinterpret_cast<void (*)(Board *, const pvzstl::string &, MessageStyle, AdviceType)>(Board_DisplayAdviceAgainAddr)(this, theAdvice, theMessageStyle, theHelpIndex);
    }
    Plant *NewPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex) {
        return reinterpret_cast<Plant *(*)(Board *, int, int, SeedType, SeedType, int)>(Board_NewPlantAddr)(this, theGridX, theGridY, theSeedType, theImitaterType, thePlayerIndex);
    }
    bool CanUseGameObject(GameObjectType theGameObject) {
        return reinterpret_cast<int (*)(Board *, GameObjectType)>(Board_CanUseGameObjectAddr)(this, theGameObject);
    }
    Plant *ToolHitTest(int theX, int theY) {
        return reinterpret_cast<Plant *(*)(Board *, int, int)>(Board_ToolHitTestAddr)(this, theX, theY);
    }
    void RefreshSeedPacketFromCursor(int thePlayerIndex) {
        reinterpret_cast<void (*)(Board *, int)>(Board_RefreshSeedPacketFromCursorAddr)(this, thePlayerIndex);
    }
    Plant *GetPlantsOnLawn(int theGridX, int theGridY, PlantsOnLawn *thePlantOnLawn) { // 检查加农炮用
        return reinterpret_cast<Plant *(*)(Board *, int, int, PlantsOnLawn *)>(Board_GetPlantsOnLawnAddr)(this, theGridX, theGridY, thePlantOnLawn);
    }
    void ClearCursor(int thePlayerIndex) {
        reinterpret_cast<void (*)(Board *, int)>(Board_ClearCursorAddr)(this, thePlayerIndex);
    }
    void MouseDownWithTool(int x, int y, int theClickCount, CursorType theCursorType, int thePlayerIndex) {
        reinterpret_cast<void (*)(Board *, int, int, int, CursorType, int)>(Board_MouseDownWithToolAddr)(this, x, y, theClickCount, theCursorType, thePlayerIndex);
    }
    void SetTutorialState(TutorialState theTutorialState) {
        reinterpret_cast<void (*)(Board *, TutorialState)>(Board_SetTutorialStateAddr)(this, theTutorialState);
    }
    Sexy::Rect GetButterButtonRect() {
        return reinterpret_cast<Sexy::Rect (*)(Board *)>(Board_GetButterButtonRectAddr)(this);
    }
    Zombie *ZombieTryToGet(ZombieID theZombieID) {
        return reinterpret_cast<Zombie *(*)(Board *, ZombieID)>(Board_ZombieTryToGetAddr)(this, theZombieID);
    }
    Zombie *ZombieGet(ZombieID theZombieID) {
        return mZombies.DataArrayGet((uint32_t)theZombieID);
    }
    void MouseDownWithPlant(int x, int y, int theClickCount, int thePlayerIndex);
    bool CanInteractWithBoardButtons() {
        return reinterpret_cast<bool (*)(Board *)>(Board_CanInteractWithBoardButtonsAddr)(this);
    }
    bool CanTakeSunMoney(int theAmount, int thePlayerIndex) {
        return reinterpret_cast<bool (*)(Board *, int, int)>(Board_CanTakeSunMoneyAddr)(this, theAmount, thePlayerIndex);
    }
    Sexy::Rect GetZenButtonRect(GameObjectType theObjectType) {
        return reinterpret_cast<Sexy::Rect (*)(Board *, GameObjectType)>(Board_GetZenButtonRectAddr)(this, theObjectType);
    }
    int PickRowForNewZombie(ZombieType theZombieType) {
        return reinterpret_cast<int (*)(Board *, ZombieType)>(Board_PickRowForNewZombieAddr)(this, theZombieType);
    }
    ZombieType GetIntroducedZombieType() {
        return reinterpret_cast<ZombieType (*)(Board *)>(Board_GetIntroducedZombieTypeAddr)(this);
    }
    ZombieType PickZombieType(int theZombiePoints, int theWaveIndex, ZombiePicker *theZombiePicker);
    bool HasLevelAwardDropped() {
        return reinterpret_cast<bool (*)(Board *)>(Board_HasLevelAwardDroppedAddr)(this);
    }
    bool CanDropLoot() {
        return reinterpret_cast<bool (*)(Board *)>(Board_CanDropLootAddr)(this);
    }
    void DropLootPiece(int thePosX, int thePosY, int theDropFactor) {
        reinterpret_cast<void (*)(Board *, int, int, int)>(Board_DropLootPieceAddr)(this, thePosX, thePosY, theDropFactor);
    }
    void ClearAdvice(AdviceType theHelpIndex) {
        reinterpret_cast<void (*)(Board *)>(Board_ClearAdviceAddr)(this);
    }
    void NextWaveComing() {
        reinterpret_cast<void (*)(Board *)>(Board_NextWaveComingAddr)(this);
    }
    int CountSunBeingCollected(int thePlayerIndex) {
        return reinterpret_cast<int (*)(Board *, int)>(Board_CountSunBeingCollectedAddr)(this, thePlayerIndex);
    }
    int CountSunFlowers() {
        return reinterpret_cast<int (*)(Board *)>(Board_CountSunFlowersAddr)(this);
    }
    int GridCellWidth(int theGridX, int theGridY) {
        return reinterpret_cast<int (*)(Board *, int, int)>(Board_GridCellWidthAddr)(this, theGridX, theGridY);
    }
    int GridCellHeight(int theGridX, int theGridY) {
        return reinterpret_cast<int (*)(Board *, int, int)>(Board_GridCellHeightAddr)(this, theGridX, theGridY);
    }
    SeedType GetSeedTypeInCursor(int thePlayerIndex) {
        return reinterpret_cast<SeedType (*)(Board *, int)>(Board_GetSeedTypeInCursorAddr)(this, thePlayerIndex);
    }
    void TryToSaveGame() {
        reinterpret_cast<void (*)(Board *)>(Board_TryToSaveGameAddr)(this);
    }
    bool CanTakeDeathMoney(int theAmount) {
        return reinterpret_cast<bool (*)(Board *, int)>(Board_CanTakeDeathMoneyAddr)(this, theAmount);
    }
    void RemoveAllMowers() {
        reinterpret_cast<void (*)(Board *)>(Board_RemoveAllMowersAddr)(this);
    }
    void ResetLawnMowers() {
        reinterpret_cast<void (*)(Board *)>(Board_ResetLawnMowersAddr)(this);
    }
    ZombieID ZombieGetID(Zombie *theZombie) {
        return reinterpret_cast<ZombieID (*)(Board *, Zombie *)>(Board_ZombieGetIDAddr)(this, theZombie);
    }
    GridItemID GridItemGetID(GridItem *theGridItem) {
        return GridItemID(mGridItems.DataArrayGetID(theGridItem));
    }
    PlantID PlantGetID(Plant *thePlant) {
        return PlantID(mPlants.DataArrayGetID(thePlant));
    }
    void SetDanceMode(bool theEnableDance) {
        reinterpret_cast<void (*)(Board *, bool)>(Board_SetDanceModeAddr)(this, theEnableDance);
    }
    void SetJacksonDanceMode(bool theEnableDance);
    bool ChooseSeedsOnCurrentLevel() {
        return reinterpret_cast<bool (*)(Board *)>(Board_ChooseSeedsOnCurrentLevelAddr)(this);
    }
    bool RowCanHaveZombies(int theRow) {
        return reinterpret_cast<bool (*)(Board *, int)>(Board_RowCanHaveZombiesAddr)(this, theRow);
    }
    void UpdateCoverLayer() {
        reinterpret_cast<void (*)(Board *)>(Board_UpdateCoverLayerAddr)(this);
    }
    void PlaceRake() {
        reinterpret_cast<void (*)(Board *)>(Board_PlaceRakeAddr)(this);
    }
    void SpawnZombiesFromSky() {
        reinterpret_cast<void (*)(Board *)>(Board_SpawnZombiesFromSkyAddr)(this);
    }
    void SpawnZombiesFromPool() {
        reinterpret_cast<void (*)(Board *)>(Board_SpawnZombiesFromPoolAddr)(this);
    }
    ZombieType PickGraveRisingZombieType(int theZombiePoints) {
        return reinterpret_cast<ZombieType (*)(Board *, int)>(Board_PickGraveRisingZombieTypeAddr)(this, theZombiePoints);
    }
    bool IsValidCobCannonSpot(int theGridX, int theGridY) {
        return reinterpret_cast<bool (*)(Board *, int, int)>(Board_IsValidCobCannonSpotAddr)(this, theGridX, theGridY);
    }
    int CountDeathBeingCollected();
    int GetMPTargetCount() {
        return reinterpret_cast<int (*)(Board *)>(Board_GetMPTargetCountAddr)(this);
    }
    void FreezeEffectsForCutscene(bool theFreeze) {
        reinterpret_cast<int (*)(Board *, bool)>(Board_FreezeEffectsForCutsceneAddr)(this, theFreeze);
    }
    Plant *FindUmbrellaPlant(int theGridX, int theGridY) {
        return reinterpret_cast<Plant *(*)(Board *, int, int)>(Board_FindUmbrellaPlantAddr)(this, theGridX, theGridY);
    }
    bool MouseHitTestPlant(int x, int y, HitResult *theHitResult) {
        return reinterpret_cast<bool (*)(Board *, int, int, HitResult *)>(Board_MouseHitTestPlantAddr)(this, x, y, theHitResult);
    }
    int GetLevelRandSeed() {
        return reinterpret_cast<int (*)(Board *)>(Board_GetLevelRandSeedAddr)((this));
    }
    bool StageHasFog() {
        return reinterpret_cast<bool (*)(Board *)>(Board_StageHasFogAddr)((this));
    }
    bool IsScaryPotterDaveTalking() {
        return reinterpret_cast<bool (*)(Board *)>(Board_IsScaryPotterDaveTalkingAddr)((this));
    }
    unsigned int SeedNotRecommendedForLevel(SeedType theSeedType) {
        return reinterpret_cast<unsigned int (*)(Board *, SeedType)>(Board_SeedNotRecommendedForLevelAddr)(this, theSeedType);
    }
    bool GetNumWavesPerSurvivalStage() {
        return reinterpret_cast<bool (*)(Board *)>(Board_GetNumWavesPerSurvivalStageAddr)((this));
    }

    Board(LawnApp *theApp) {
        _constructor(theApp);
    }
    ~Board() = delete;

    void InitLevel();
    void SetGrids();
    void StartLevel();
    void Update();
    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void UpdateButtons();
    int GetNumSeedsInBank(bool isZombieBank);
    void RemoveParticleByType(ParticleEffect theEffectType);
    void FadeOutLevel();
    Plant *AddPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex, bool theIsDoEffect);
    Plant *AddPlant_Origin(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex, bool theIsDoEffect);
    void AddSunMoney(int theAmount, int thePlayerIndex);
    void AddDeathMoney(int theAmount);
    bool IsIceAt(int theGridX, int theGridY);
    PlantingReason CanPlantAt(int theGridX, int theGridY, SeedType theSeedType);
    bool PlantingRequirementsMet(SeedType theSeedType);
    Plant *GetFlowerPotAt(int theGridX, int theGridY);
    Plant *GetPumpkinAt(int theGridX, int theGridY);
    void ZombiesWon(Zombie *theZombie);
    void UpdateSunSpawning();
    void UpdateZombieSpawning();
    void PickBackground();
    void DrawCoverLayer(Sexy::Graphics *g, int theRow);
    void UpdateGame();
    void UpdateGameObjects();
    bool IsFlagWave(int theWaveNumber);
    int GetGraveStonesCount();
    void SpawnZombiesFromGraves();
    void SpawnZombieWave();
    void DrawProgressMeter(Sexy::Graphics *g, int theX, int theY);
    int GetNumWavesPerFlag() const;
    bool IsLevelDataLoaded();
    bool NeedSaveGame();
    void PostLoadGame() {
        reinterpret_cast<void (*)(Board *)>(Board_PostLoadGameAddr)(this);
    }
    void UpdateFwoosh();
    void UpdateFog();
    void DrawFog(Sexy::Graphics *g);
    void UpdateIce();
    void DrawBackdrop(Sexy::Graphics *g);
    bool RowCanHaveZombieType(int theRow, ZombieType theZombieType);
    void DrawDebugText(Sexy::Graphics *g);
    void DrawDebugObjectRects(Sexy::Graphics *g);
    void DrawFadeOut(Sexy::Graphics *g);
    int GetCurrentPlantCost(SeedType theSeedType, SeedType theImitaterType);
    void Pause(bool thePause);
    void AddSecondPlayer(int a2);
    bool IsLastStandFinalStage() const;
    bool MouseHitTest(int x, int y, HitResult *theHitResult, bool thePlayerIndex);
    void DrawShovel(Sexy::Graphics *g);
    bool StageIsNight() const;
    bool StageHasPool() const;
    bool StageHasRoof() const;
    bool StageHas6Rows() const;
    Zombie *AddZombieInRow(ZombieType theZombieType, int theRow, int theFromWave, bool theIsRustle);
    Zombie *AddZombieInRow_Origin(ZombieType theZombieType, int theRow, int theFromWave, bool theIsRustle);
    Zombie *AddZombie(ZombieType theZombieType, int theFromWave, bool theIsRustle);
    Zombie *AddZombie_Origin(ZombieType theZombieType, int theFromWave, bool theIsRustle);
    void DoPlantingEffects(int theGridX, int theGridY, Plant *thePlant);
    void InitLawnMowers();
    void PickZombieWaves();
    void DrawUITop(Sexy::Graphics *g);
    // void GetShovelButtonRect(Rect *rect);
    void UpdateLevelEndSequence();
    void UpdateGridItems();
    void ShakeBoard(int theShakeAmountX, int theShakeAmountY);
    void DrawZenButtons(Sexy::Graphics *g);
    void DrawGameObjects(Sexy::Graphics *g);
    void SpeedUpUpdate();
    void DrawShovelButton(Sexy::Graphics *g, LawnApp *theApp);
    void ShovelDown();
    int PixelToGridXKeepOnBoard(int theX, int theY);
    int PixelToGridYKeepOnBoard(int theX, int theY);
    int GridToPixelX(int theGridX, int theGridY) const;
    int GridToPixelY(int theGridX, int theGridY);
    static int MakeRenderOrder(RenderLayer theRenderLayer, int theRow, int theLayerOffset);
    int GetLiveGargantuarCount();
    int GetLiveZombiesCount();
    Zombie *GetLiveZombieByType(ZombieType theZombieType);
    bool GetAliveJacksonZombie();
    void FixReanimErrorAfterLoad();
    void DoPlantingAchievementCheck(SeedType theSeedType);
    bool GrantAchievement(AchievementType theAchievementId, bool theIsShow);
    int CountPlantByType(SeedType theSeedType);
    [[nodiscard]] bool ZenGardenItemNumIsZero(CursorType theCursorType) const;
    int GetSeedBankExtraWidth();
    Sexy::Rect GetShovelButtonRect();
    [[nodiscard]] bool PlantUsesAcceleratedPricing(SeedType theSeedType) const;
    bool IsPlantInCursor();
    void RemoveAllPlants();
    void RemoveAllGridItems();
    void RemoveAllZombies();
    bool IsValidCobCannonSpotHelper(int theGridX, int theGridY);
    bool IsPoolSquare(int theGridX, int theGridY);
    void PutZombieInWave(ZombieType theZombieType, int theWaveNumber, ZombiePicker *theZombiePicker);
    int TotalZombiesHealthInWave(int theWaveIndex);
    int KillAllZombiesInRadius(int theRow, int theX, int theY, int theRadius, int theRowRange, bool theBurn, int theDamageRangeFlags) {
        return reinterpret_cast<int (*)(Board *, int, int, int, int, int, bool, int)>(Board_KillAllZombiesInRadiusAddr)(this, theRow, theX, theY, theRadius, theRowRange, theBurn, theDamageRangeFlags);
    }
    int KillAllZombiesInRadius_Custom(int theRow, int theX, int theY, int theRadius, int theRowRange, bool theBurn, int theDamageRangeFlags);
    void KillAllPlantsInRadius(int theX, int theY, int theRadius);
    void PlantsTakeDamageInGrid(int theGridX, int theGridY, int theDamage);
    void RemoveCutsceneZombies();
    int CountZombiesOnScreen();
    float GetPosYBasedOnRow(float thePosX, int theRow);
    Zombie *GetBossZombie();
    GamepadControls *GetGamepadControlsByPlayerIndex(int thePlayerIndex);
    void DrawHammerButton(Sexy::Graphics *g, LawnApp *theApp);
    void DrawButterButton(Sexy::Graphics *g, LawnApp *theApp);
    void Draw(Sexy::Graphics *g);
    int GetSeedPacketPositionX(int thePacketIndex, int theSeedBankIndex, bool isZombiePlayer);
    Coin *AddCoin(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion);
    bool TakeDeathMoney(int theAmount);
    bool TakeSunMoney(int theAmount, int thePlayer);
    void ShuffleButtonDown(SeedPacket *theSeedPacket);
    bool CanAddGraveStoneAt(int theGridX, int theGridY);
    void DrawLevel(Sexy::Graphics *g);
    bool CanAddBobSledMP();
    GridItem *AddMPTarget(int theGridX, int theGridY);
    void PlantsWon(GridItem *theGridItem);
    void PlantsWon_Origin(GridItem *theGridItem);
    GridItem *AddALadder(int theGridX, int theGridY);
    GridItem *AddALadder_Origin(int theGridX, int theGridY);
    GridItem *AddACrater(int theGridX, int theGridY);
    GridItem *AddACrater_Origin(int theGridX, int theGridY);
    GridItem *AddAGraveStone(int theGridX, int theGridY);
    GridItem *AddAMound(int theGridX, int theGridY, int theMoundLevel);
    GridItem *AddAPole(int theX, int theY, int theGridY);
    GridItem *AddAPole_Origin(int theX, int theY, int theGridY);
    [[nodiscard]] ZombieType PickGraveRisingZombieTypeMP(int theMoundLevel) const;
    static bool IsZombieTypeSpawnedOnly(ZombieType theZombieType);
    void ProcessDeleteQueue();
    static bool IsZombieTypePoolOnly(ZombieType theZombieType);
    Plant *FindBloomerangPlant(int theGridX, int theGridY);

    void MouseMove(int x, int y);
    void MouseDown(int x, int y, int theClickCount);
    void MouseDownSecond(int x, int y, int theClickCount);
    void MouseUp(int x, int y, int theClickCount);
    void MouseUpSecond(int x, int y, int theClickCount);
    void MouseDrag(int x, int y);
    void MouseDragSecond(int x, int y);
    void __MouseDown(int x, int y, int theClickCount);
    void __MouseDrag(int x, int y);
    void __MouseUp(int x, int y, int theClickCount);
    void ButtonDepress(int theId);
    bool KeyDown(Sexy::KeyCode theKey);
    bool KeyUp(Sexy::KeyCode theKey);
    void GameButtonUp(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags) {
        reinterpret_cast<void (*)(Board *, Sexy::GamepadButton, int, unsigned int)>(Board_GameButtonDownAddr)(this, theButton, thePlayerIndex, theFlags);
    }
    void processClientEvent(const BaseEvent *event);
    void processServerEvent(const BaseEvent *event);
    void PauseFromSecondPlayer(bool thePause);

protected:
    friend void InitHookFunction();

    void _constructor(LawnApp *theApp);
    void _destructor();
    void ClientMouseDownLocal(int x, int y, bool isInBank);
    void ClientMouseDragLocal(int x, int y);
    static void ClientMouseUpLocal(int x, int y);
};

int GetRectOverlap(const Sexy::Rect &rect1, const Sexy::Rect &rect2);
bool GetCircleRectOverlap(int theCircleX, int theCircleY, int theRadius, const Sexy::Rect &theRect);

const char *GetNameByAchievementId(AchievementType theAchievementId);
Sexy::Image *GetIconByAchievementId(AchievementType theAchievementId);
/***************************************************************************************************************/
inline int gCheatPlaceColumn;
inline int gCheatPlaceRow;
inline bool gCheatPlaceLadder;
inline bool gCheatPlaceGraveStone;
inline bool gCheatPlacePlant;
inline bool gCheatPlaceZombie;
inline SeedType gCheatPlacePlantType = SeedType::SEED_NONE;
inline ZombieType gCheatPlaceZombieType = ZombieType::ZOMBIE_INVALID;
inline bool gCheatIsPlaceImitaterPlant;
inline bool passNowLevel;
inline std::string customFormation;
inline int formationId = -1;
inline bool clearAllPlant;
inline bool clearAllZombies;
inline bool hypnoAllZombies;
inline bool freezeAllZombies;
inline bool clearAllGraves;
inline bool clearAllMowers;
inline bool recoverAllMowers;
inline bool startAllMowers;
inline bool banMower;
inline bool layChoseFormation;
inline bool layPastedFormation;
inline bool noFog;
inline bool gCheatZombiesToSpawn[ZombieType::EXTENDED_NUM_ZOMBIE_TYPES]; // 僵尸选中情况
inline int choiceSpawnMode;                                              // 刷怪模式
inline bool buttonSetSpawn;                                              // 设置出怪
inline int targetSeedBank;
inline int choiceSeedPacketIndex;
inline SeedType choiceSeedType = SeedType::SEED_NONE;
inline bool isImitaterSeed;
inline bool setSeedPacket;
inline Sexy::Rect gTouchVSShovelRect = {-120, 10, 70, 72};

inline bool hideCoverLayer;
inline bool infiniteSun; // 无限阳光
inline bool drawDebugText;
inline bool drawDebugRects;
inline bool FreePlantAt;
inline bool ZombieCanNotWon;
inline bool PumpkinWithLadder; // AddPlant
inline bool endlessLastStand;
inline BackgroundType gVSBackground; // 对战模式战场


// isLongPress的数值为：首次按下为0，后续按下为1
// playerIndex为0或者1，代表玩家1或者2
// buttonCode和GameButton通用
// 上 0
// 下 1
// 左 2
// 右 3

inline void (*old_Board_UpdateGame)(Board *board);

inline void (*old_Board_UpdateGameObjects)(Board *board);

inline void (*old_Board_DrawDebugText)(Board *board, Sexy::Graphics *graphics);

inline void (*old_Board_DrawDebugObjectRects)(Board *board, Sexy::Graphics *graphics);

inline int (*old_Board_GetCurrentPlantCost)(Board *board, SeedType a2, SeedType a3);

inline bool (*old_Board_PlantingRequirementsMet)(Board *board, SeedType theSeedType);

inline void (*old_BoardZombiesWon)(Board *board, Zombie *zombie);

inline Plant *(*old_Board_AddPlant)(Board *board, int x, int y, SeedType seedType, SeedType theImitaterType, int playerIndex, bool doPlantEffect);

inline bool (*old_Board_KeyDown)(Board *board, Sexy::KeyCode theKey);

inline bool (*old_Board_KeyUp)(Board *board, Sexy::KeyCode theKey);

inline void (*old_Board_UpdateZombieSpawning)(Board *board);

inline void (*old_Board_UpdateIce)(Board *board);

inline void (*old_Board_PickBackground)(Board *board);

inline bool (*old_Board_StageHasPool)(Board *board);

inline void (*old_Board_UpdateFwoosh)(Board *board);

inline void (*old_Board_DrawFog)(Board *board, Sexy::Graphics *g);

inline Zombie *(*old_Board_AddZombieInRow)(Board *board, ZombieType theZombieType, int theRow, int theFromWave, bool playAnim);

inline void (*old_Board_Update)(Board *board);

inline bool (*old_Board_IsFlagWave)(Board *board, int currentWave);

inline void (*old_Board_SpawnZombieWave)(Board *board);

inline void (*old_Board_DrawProgressMeter)(Board *board, Sexy::Graphics *graphics, int a3, int a4);

inline bool (*old_Board_IsLevelDataLoaded)(Board *board);

inline bool (*old_Board_NeedSaveGame)(Board *board);

inline void (*old_Board_DrawBackdrop)(Board *board, Sexy::Graphics *graphics);

inline void (*old_Board_Pause)(Board *board, bool a2);

inline void (*old_Board_InitLawnMowers)(Board *board);

inline void (*old_Board_PickZombieWaves)(Board *board);

inline ZombieType (*old_Board_PickZombieType)(Board *board, int theZombiePoints, int theWaveIndex, ZombiePicker *theZombiePicker);

inline void (*old_Board_AddedToManager)(Board *, Sexy::WidgetManager *);

inline void (*old_Board_RemovedFromManager)(Board *, Sexy::WidgetManager *);

inline void (*old_Board_InitLevel)(Board *board);

inline void (*old_Board_ButtonDepress)(Board *board, int id);

inline void (*old_Board_MouseUp)(Board *board, int a2, int a3, int a4);

inline void (*old_Board_MouseDrag)(Board *board, int x, int y);

inline void (*old_Board_MouseDown)(Board *board, int x, int y, int theClickCount);
inline void (*old_Board_MouseDownWithPlant)(Board *board, int x, int y, int theClickCount, int thePlayerIndex);

inline void (*old_Board_MouseMove)(Board *board, int x, int y);

inline bool (*old_Board_MouseHitTest)(Board *board, int x, int y, HitResult *hitResult, bool posScaled);

inline void (*old_Board_FadeOutLevel)(Board *board);

inline void (*old_Board_AddSunMoney)(Board *board, int theAmount, int playerIndex);

inline void (*old_Board_AddDeathMoney)(Board *board, int theAmount);

inline void (*old_Board_UpdateLevelEndSequence)(Board *board);

inline void (*old_Board_UpdateGridItems)(Board *board);

inline void (*old_Board_StartLevel)(Board *board);

inline void (*old_Board_DrawUITop)(Board *board, Sexy::Graphics *graphics);

inline void (*old_Board_ShakeBoard)(Board *board, int theShakeAmountX, int theShakeAmountY);

inline void (*old_Board_UpdateFog)(Board *board);

inline Sexy::Rect (*old_Board_GetShovelButtonRect)(Board *board);

inline void (*old_Board_DrawZenButtons)(Board *board, Sexy::Graphics *a2);

inline void (*old_Board_DrawGameObjects)(Board *, Sexy::Graphics *);

inline int (*old_Board_GetNumSeedsInBank)(Board *, bool);

inline void (*old_Board_Draw)(Board *, Sexy::Graphics *g);

inline Coin *(*old_Board_AddCoin)(Board *board, int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion);

inline GridItem *(*old_Board_AddAGraveStone)(Board *board, int gridX, int gridY);

inline bool (*old_Board_TakeSunMoney)(Board *board, int amount, int player);

inline bool (*old_Board_TakeDeathMoney)(Board *board, int amount);

inline void (*old_Board_DrawLevel)(Board *, Sexy::Graphics *);

inline void (*old_Board_PlantsWon)(Board *, GridItem *);

#endif // PVZ_LAWN_BOARD_BOARD_H
