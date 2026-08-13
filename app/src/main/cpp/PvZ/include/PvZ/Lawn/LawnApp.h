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

#ifndef PVZ_LAWN_LAWN_APP_H
#define PVZ_LAWN_LAWN_APP_H

#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/System/Mailbox.h"
#include "PvZ/Lawn/System/TypingCheck.h"
#include "PvZ/Lawn/Widget/AchievementsWidget.h"
#include "PvZ/SexyAppFramework/GamepadApp.h"
#include "PvZ/SexyAppFramework/IGameCenter.h"
#include "PvZ/SexyAppFramework/Level.h"
#include "PvZ/SexyAppFramework/Misc/ProfileMgr.h"
#include "PvZ/SexyAppFramework/SexyAppBase.h"
#include "PvZ/SexyAppFramework/Thread.h"
#include "PvZ/SexyAppFramework/Widget/ProfileEventListener.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodFoley.h"
#include "PvZ/TodLib/Effect/EffectSystem.h"
#include "PvZ/TodLib/Effect/TodParticle.h"

#include <cstdint>

class ZenGarden;
class Board;
class TitleScreen;
class MainMenu;
class SeedChooserScreen;
class CreditScreen;
class ChallengeScreen;
class AlmanacDialog;
class PoolEffect;
class ReanimatorCache;
class Music2;
class TodFoley;
class PottedPlant;
class StoreScreen;
class VSSetupMenu;
class VSResultsMenu;
class HelpBarWidget;
class HelpTextScreen;
class SaveGameContext;

struct ListNodeBase {
    ListNodeBase *next;
    ListNodeBase *prev;
};

struct LawnSession {
    void **vtable;
    bool mShutdownRequested; // +0x04
    bool mTaskDone;
    void *mCurrentTask; // +0x08
    Sexy::Thread mThread;
};

struct QueryCoinState {
    bool mPending;
    unsigned int mLastRequestTime;
    int mRequestCount;
    int mUserId;
};

struct QueryCoin {
    void **mVtable;
    QueryCoinState mState;
};

class LawnApp : public Sexy::GamepadApp, public Sexy::ProfileEventListener, public Sexy::IGameCenter::Listener {
public:
    struct LawnAppVTable {
        void (*completeDestructor)(LawnApp *self);                                                                                                                                    // 0x000
        void (*deletingDestructor)(LawnApp *self);                                                                                                                                    // 0x004
        void (*ButtonPress)(LawnApp *self, int id);                                                                                                                                   // 0x008
        void (*ButtonPressWithCount)(Sexy::ButtonListener *self, int id, int count);                                                                                                  // 0x00C
        void (*ButtonDepress)(LawnApp *self, int id);                                                                                                                                 // 0x010
        void (*ButtonDownTick)(Sexy::ButtonListener *self, int id);                                                                                                                   // 0x014
        void (*ButtonMouseEnter)(Sexy::ButtonListener *self, int id);                                                                                                                 // 0x018
        void (*ButtonMouseLeave)(Sexy::ButtonListener *self, int id);                                                                                                                 // 0x01C
        void (*ButtonMouseMove)(Sexy::ButtonListener *self, int id, int x, int y);                                                                                                    // 0x020
        void (*MakeWindow)(SexyAppBase *self);                                                                                                                                        // 0x024
        void (*InitCursors)(SexyAppBase *self);                                                                                                                                       // 0x028
        void (*EnforceCursor)(LawnApp *self);                                                                                                                                         // 0x02C
        void (*ClearKeysDown)(SexyAppBase *self);                                                                                                                                     // 0x030
        void (*UpdateFrames)(LawnApp *self);                                                                                                                                          // 0x034
        void (*ReInitImages)(SexyAppBase *self);                                                                                                                                      // 0x038
        void (*DeleteNativeImageData)(SexyAppBase *self);                                                                                                                             // 0x03C
        void (*DeleteExtraImageData)(SexyAppBase *self);                                                                                                                              // 0x040
        void (*Delete3DImageData)(SexyAppBase *self);                                                                                                                                 // 0x044
        void (*LoadingThreadCompleted)(LawnApp *self);                                                                                                                                // 0x048
        void (*Evict3DImageData)(SexyAppBase *self, unsigned int amount);                                                                                                             // 0x04C
        void (*StartSensor)(SexyAppBase *self, int sensorType);                                                                                                                       // 0x050
        void (*StopSensor)(SexyAppBase *self, int sensorType);                                                                                                                        // 0x054
        void (*PauseApp)(SexyAppBase *self);                                                                                                                                          // 0x058
        void (*ResumeApp)(SexyAppBase *self);                                                                                                                                         // 0x05C
        int *(*CreateMusicInterface)(SexyAppBase *self, void *context);                                                                                                               // 0x060
        void (*InitHook)(LawnApp *self);                                                                                                                                              // 0x064
        void (*ShutdownHook)(SexyAppBase *self);                                                                                                                                      // 0x068
        void (*PreTerminate)(SexyAppBase *self);                                                                                                                                      // 0x06C
        void (*LoadingThreadProc)(LawnApp *self);                                                                                                                                     // 0x070
        void (*WriteToRegistry)(LawnApp *self);                                                                                                                                       // 0x074
        void (*ReadFromRegistry)(LawnApp *self);                                                                                                                                      // 0x078
        Sexy::Dialog *(*NewDialog)(LawnApp *self, int dialogId, bool modal, const pvzstl::string &header, const pvzstl::string &lines, const pvzstl::string &footer, int buttonMode); // 0x07C
        void (*PreDisplayHook)(LawnApp *self);                                                                                                                                        // 0x080
        bool (*IsUIOrientationAllowed)(SexyAppBase *self, int orientation);                                                                                                           // 0x084
        void (*UIOrientationChanged)(SexyAppBase *self, int orientation);                                                                                                             // 0x088
        int (*GetUIOrientation)(SexyAppBase *self);                                                                                                                                   // 0x08C
        void (*OnFullVersionChange)(SexyAppBase *self);                                                                                                                               // 0x090
        void (*LowMemoryWarning)(SexyAppBase *self);                                                                                                                                  // 0x094
        void (*AppEnteredBackground)(SexyCommonApp *self);                                                                                                                            // 0x098
        void (*AppEnteredForeground)(SexyCommonApp *self);                                                                                                                            // 0x09C
        void (*BeginPopup)(SexyAppBase *self);                                                                                                                                        // 0x0A0
        void (*EndPopup)(SexyAppBase *self);                                                                                                                                          // 0x0A4
        int (*MsgBox)(SexyAppBase *self, const pvzstl::string &text, const pvzstl::string &title, int flags);                                                                         // 0x0A8
        int (*MsgBoxIntString)(SexyAppBase *self, const std::basic_string<int> &text, const std::basic_string<int> &title, int flags);                                                // 0x0AC
        void (*Popup)(SexyAppBase *self, const pvzstl::string &text);                                                                                                                 // 0x0B0
        void (*PopupIntString)(SexyAppBase *self, const std::basic_string<int> &text);                                                                                                // 0x0B4
        void (*LogScreenSaverError)(SexyAppBase *self, const pvzstl::string &error);                                                                                                  // 0x0B8
        void (*SafeDeleteWidget)(LawnApp *self, Sexy::Widget *widget);                                                                                                                // 0x0BC
        void (*URLOpenFailed)(LawnApp *self, const pvzstl::string &url);                                                                                                              // 0x0C0
        void (*URLOpenSucceeded)(LawnApp *self, const pvzstl::string &url);                                                                                                           // 0x0C4
        bool (*OpenURL)(LawnApp *self, const pvzstl::string &url, bool shutdownOnOpen);                                                                                               // 0x0C8
        void (*OpenRegisterPageWithParameters)(SexyAppBase *self, int parameters);                                                                                                    // 0x0CC
        void (*OpenRegisterPage)(SexyAppBase *self);                                                                                                                                  // 0x0D0
        pvzstl::string (*GetProductVersion)(SexyAppBase *self, const pvzstl::string &path);                                                                                           // 0x0D4
        void (*SEHOccured)(SexyAppBase *self);                                                                                                                                        // 0x0D8
        pvzstl::string (*GetGameSEHInfo)(SexyAppBase *self);                                                                                                                          // 0x0DC
        void (*GetSEHWebParams)(SexyAppBase *self, int *parameters);                                                                                                                  // 0x0E0
        void (*Shutdown)(LawnApp *self);                                                                                                                                              // 0x0E4
        void (*Quit)(SexyAppBase *self);                                                                                                                                              // 0x0E8
        pvzstl::string (*FormatHelpString)(SexyAppBase *self);                                                                                                                        // 0x0EC
        void (*AddParameterEntry)(SexyAppBase *self, int *entry);                                                                                                                     // 0x0F0
        void (*AddParameterEntries)(SexyAppBase *self, int *entries);                                                                                                                 // 0x0F4
        void (*DoParseCmdLine)(SexyAppBase *self);                                                                                                                                    // 0x0F8
        void (*ParseCmdLineVector)(SexyAppBase *self, const std::vector<pvzstl::string> &arguments);                                                                                  // 0x0FC
        void (*ParseCmdLineString)(SexyAppBase *self, const pvzstl::string &commandLine);                                                                                             // 0x100
        void (*HandleCmdLineParam)(LawnApp *self, const pvzstl::string &name, const pvzstl::string &value);                                                                           // 0x104
        void (*HandleNotifyGameMessage)(SexyAppBase *self, int type, int parameter);                                                                                                  // 0x108
        void (*HandleGameAlreadyRunning)(SexyAppBase *self);                                                                                                                          // 0x10C
        void (*Startup)(SexyAppBase *self);                                                                                                                                           // 0x110
        void (*Start)(LawnApp *self);                                                                                                                                                 // 0x114
        void (*Terminate)(SexyAppBase *self);                                                                                                                                         // 0x118
        void (*Init)(LawnApp *self);                                                                                                                                                  // 0x11C
        void (*Cleanup)(SexyCommonApp *self);                                                                                                                                         // 0x120
        void (*PreDDInterfaceInitHook)(SexyAppBase *self);                                                                                                                            // 0x124
        void (*PostDDInterfaceInitHook)(SexyAppBase *self);                                                                                                                           // 0x128
        bool (*ChangeDirHook)(LawnApp *self, const char *intendedPath);                                                                                                               // 0x12C
        void (*PlaySample)(SexyAppBase *self, int soundId);                                                                                                                           // 0x130
        void (*PlaySampleWithPan)(SexyAppBase *self, int soundId, int pan);                                                                                                           // 0x134
        void (*PlaySampleSingle)(SexyAppBase *self, int soundId);                                                                                                                     // 0x138
        void (*PlaySampleSingleWithPan)(SexyAppBase *self, int soundId, int pan);                                                                                                     // 0x13C
        double (*GetMasterVolume)(SexyAppBase *self);                                                                                                                                 // 0x140
        double (*GetMusicVolume)(SexyAppBase *self);                                                                                                                                  // 0x144
        double (*GetSfxVolume)(SexyAppBase *self);                                                                                                                                    // 0x148
        bool (*IsMuted)(SexyAppBase *self);                                                                                                                                           // 0x14C
        void (*SetMasterVolume)(SexyAppBase *self, double volume);                                                                                                                    // 0x150
        void (*SetMusicVolume)(SexyAppBase *self, double volume);                                                                                                                     // 0x154
        void (*SetSfxVolume)(SexyAppBase *self, double volume);                                                                                                                       // 0x158
        void (*Mute)(SexyAppBase *self, bool autoMute);                                                                                                                               // 0x15C
        void (*Unmute)(SexyAppBase *self, bool autoMute);                                                                                                                             // 0x160
        double (*GetLoadingThreadProgress)(SexyAppBase *self);                                                                                                                        // 0x164
        Sexy::Image *(*GetTexImage)(SexyAppBase *self, const pvzstl::string &fileName, bool commitBits);                                                                              // 0x168
        Sexy::Image *(*GetImage)(Sexy::GamepadApp *self, const pvzstl::string &fileName, bool commitBits);                                                                            // 0x16C
        Sexy::Image *(*GetImageWithAttrs)(Sexy::GamepadApp *self, const pvzstl::string &fileName, const pvzstl::string &variant, const pvzstl::string &attributes, bool commitBits);  // 0x170
        bool (*ReloadImage)(SexyAppBase *self, Sexy::Image *image);                                                                                                                   // 0x174
        int *(*GetSharedImage)(SexyAppBase *self, const pvzstl::string &fileName, const pvzstl::string &variant, bool *isNew, bool commitBits, bool allowTriReps);                    // 0x178
        int *(*GetSharedImageWithAttrs)(
            SexyAppBase *self, const pvzstl::string &fileName, const pvzstl::string &variant, const pvzstl::string &attributes, const pvzstl::string &alphaFile, bool *isNew);       // 0x17C
        Sexy::Image *(*GetImageForInput)(SexyAppBase *self, const pvzstl::string &fileName, int width, int height, const pvzstl::string &variant);                                   // 0x180
        void (*RemoveImageForInput)(SexyAppBase *self, const pvzstl::string &fileName);                                                                                              // 0x184
        void (*SwitchScreenMode)(SexyAppBase *self);                                                                                                                                 // 0x188
        void (*SwitchScreenModeWindowed)(SexyAppBase *self, bool windowed);                                                                                                          // 0x18C
        void (*SwitchScreenModeFull)(LawnApp *self, bool windowed, bool use3D, bool force);                                                                                          // 0x190
        void (*SetAlphaDisabled)(SexyAppBase *self, bool disabled);                                                                                                                  // 0x194
        Sexy::Dialog *(*DoDialog)(LawnApp *self, int dialogId, bool modal, const pvzstl::string &header, const pvzstl::string &lines, const pvzstl::string &footer, int buttonMode); // 0x198
        Sexy::Dialog *(*GetDialog)(SexyAppBase *self, int dialogId);                                                                                                                 // 0x19C
        void (*AddDialogWithId)(SexyAppBase *self, int dialogId, Sexy::Dialog *dialog);                                                                                              // 0x1A0
        void (*AddDialog)(SexyAppBase *self, Sexy::Dialog *dialog);                                                                                                                  // 0x1A4
        bool (*KillDialogFull)(SexyAppBase *self, int dialogId, bool removeWidget, bool deleteWidget);                                                                               // 0x1A8
        bool (*KillDialogById)(LawnApp *self, int dialogId);                                                                                                                         // 0x1AC
        bool (*KillDialogByPointer)(SexyAppBase *self, Sexy::Dialog *dialog);                                                                                                        // 0x1B0
        int (*GetDialogCount)(SexyAppBase *self);                                                                                                                                    // 0x1B4
        void (*ModalOpen)(LawnApp *self);                                                                                                                                            // 0x1B8
        void (*ModalClose)(LawnApp *self);                                                                                                                                           // 0x1BC
        void (*DialogButtonPress)(SexyAppBase *self, int dialogId, int buttonId);                                                                                                    // 0x1C0
        void (*DialogButtonDepress)(SexyAppBase *self, int dialogId, int buttonId);                                                                                                  // 0x1C4
        void (*GotFocus)(LawnApp *self);                                                                                                                                             // 0x1C8
        void (*LostFocus)(LawnApp *self);                                                                                                                                            // 0x1CC
        bool (*IsAltKeyUsed)(SexyAppBase *self, int keyCode);                                                                                                                        // 0x1D0
        bool (*KeyDown)(SexyAppBase *self, int keyCode);                                                                                                                             // 0x1D4
        bool (*DebugKeyDown)(SexyAppBase *self, int keyCode);                                                                                                                        // 0x1D8
        bool (*DebugKeyDownAsync)(SexyAppBase *self, int keyCode, bool controlDown, bool altDown);                                                                                   // 0x1DC
        void (*CloseRequestAsync)(LawnApp *self);                                                                                                                                    // 0x1E0
        void (*Done3dTesting)(SexyAppBase *self);                                                                                                                                    // 0x1E4
        pvzstl::string (*NotifyCrashHook)(SexyAppBase *self);                                                                                                                        // 0x1E8
        bool (*CheckSignature)(SexyAppBase *self, const Sexy::Buffer &buffer, const pvzstl::string &fileName);                                                                       // 0x1EC
        void (*PreDrawScreen)(LawnApp *self);                                                                                                                                        // 0x1F0
        bool (*DrawDirtyStuff)(LawnApp *self);                                                                                                                                       // 0x1F4
        void (*Redraw)(SexyAppBase *self, Sexy::TRect<int> *clipRect);                                                                                                               // 0x1F8
        void (*InitPropertiesHook)(SexyAppBase *self);                                                                                                                               // 0x1FC
        bool (*UpdateAppStep)(LawnApp *self, bool *updated);                                                                                                                         // 0x200
        bool (*UpdateApp)(LawnApp *self);                                                                                                                                            // 0x204
        void (*PopulateMessages)(SexyAppBase *self);                                                                                                                                 // 0x208
        bool (*ProcessMessages)(SexyAppBase *self, std::vector<Sexy::Event> &events);                                                                                                // 0x20C
        bool (*ProcessMessage)(LawnApp *self, Sexy::Event &event);                                                                                                                   // 0x210
        void (*CleanupHook)(SexyAppBase *self);                                                                                                                                      // 0x214
        void (*DrawOneFrame)(SexyAppBase *self);                                                                                                                                     // 0x218
        bool (*AppCanRestore)(SexyAppBase *self);                                                                                                                                    // 0x21C
        void (*InputStatusChanged)(SexyAppBase *self, int *status);                                                                                                                  // 0x220
        void (*InitPreLoadWidget)(SexyAppBase *self);                                                                                                                                // 0x224
        Sexy::Widget *(*CreatePreLoadWidget)(SexyAppBase *self);                                                                                                                     // 0x228
        void (*FinishPreLoadWidget)(SexyAppBase *self);                                                                                                                              // 0x22C
        void (*PostPreLoad)(SexyAppBase *self);                                                                                                                                      // 0x230
        void (*PreLoadResources)(SexyAppBase *self);                                                                                                                                 // 0x234
        void (*AuthFinished)(LawnApp *self, bool success);                                                                                                                           // 0x238
        void (*StartAuth)(LawnApp *self);                                                                                                                                            // 0x23C
        void (*GetMemoryInfo)(SexyAppBase *self, int &memoryInfo);                                                                                                                   // 0x240
        bool (*FrameNeedsSwapScreenImage)(SexyAppBase *self);                                                                                                                        // 0x244
        bool (*TakeScreenshot)(SexyAppBase *self, Sexy::MemoryImage &image);                                                                                                         // 0x248
        void (*DoAuthenticate)(SexyCommonApp *self);                                                                                                                                 // 0x24C
        void (*DoGameTrialDialog)(SexyCommonApp *self, int unk);                                                                                                                     // 0x250
        Sexy::Dialog *(*CreateGameTrialDialog)(SexyCommonApp *self);                                                                                                                 // 0x254
        Sexy::Dialog *(*CreateGotoBuyResultDialog)(SexyCommonApp *self, int result, pvzstl::string message);                                                                         // 0x258
        Sexy::Dialog *(*CreateWaitResultDialog)(SexyCommonApp *self);                                                                                                                // 0x25C
        Sexy::Dialog *(*CreateWaitMessageDialog)(SexyCommonApp *self);                                                                                                               // 0x260
        Sexy::Dialog *(*CreateNetworkFaultDialog)(SexyCommonApp *self);                                                                                                              // 0x264
#if PVZ_VERSION == 111
        int (*GamePaidStatus)(SexyCommonApp *self);
#endif
        void (*setOfferRes)(SexyCommonApp *self, bool success);                                                  // 0x268
        void (*setOfferFullOpenRes)(SexyCommonApp *self, bool success);                                          // 0x26C
        void (*setPayItemsRes)(LawnApp *self, bool success, const char *itemId, int value, const char *message); // 0x270
        void (*setQueryBalanceRes)(LawnApp *self, bool success, int balance);                                    // 0x274
        void (*adBarClick)(SexyCommonApp *self);                                                                 // 0x278
        void (*setLoginRes)(SexyCommonApp *self, bool success, const char *userName);                            // 0x27C
        void (*setFullVersion)(SexyCommonApp *self, bool fullVersion);                                           // 0x280
        void (*messageBoxRes)(SexyCommonApp *self, int result);                                                  // 0x284
        void (*setErrorCode)(LawnApp *self, pvzstl::string errorCode);                                           // 0x288
        void (*showRechargeBoard)(LawnApp *self, pvzstl::string message);                                        // 0x28C
        void (*DeviceAdded)(Sexy::GamepadApp *self, int *driver);                                                // 0x290
        void (*DeviceRemoved)(Sexy::GamepadApp *self, int *driver);                                              // 0x294
        void (*GamepadConnected)(LawnApp *self);                                                                 // 0x298
        void (*GamepadDisconnected)(LawnApp *self);                                                              // 0x29C
        void (*LoadingCompleted)(LawnApp *self);                                                                 // 0x2A0
        void (*PlaySampleWithLoopFlag)(LawnApp *self, int soundId, bool flag);                                   // 0x2A4
        void (*DrawPost)(LawnApp *self);                                                                         // 0x2A8
        void (*OnButtonDown)(LawnApp *self, int button, int playerIndex, unsigned int flags);                    // 0x2AC
        void (*ShowResourceError)(LawnApp *self, bool show);                                                     // 0x2B0
        int (*GetProfileVersion)(LawnApp *self);                                                                 // 0x2B4
        Sexy::PlayerInfo *(*CreatePlayerInfo)(LawnApp *self);                                                    // 0x2B8
        void (*NotifyProfileChanged)(LawnApp *self, Sexy::PlayerInfo *playerInfo);                               // 0x2BC
        void (*OnProfileSaveError)(LawnApp *self, Sexy::PlayerInfo *playerInfo);                                 // 0x2C0
        void (*onPay)(LawnApp *self, int result);                                                                // 0x2C4
    };

public:
    static Sexy::Rect &FULLSCREEN_RECT;

    Board *mBoard;                                 // 552
    TitleScreen *mTitleScreen;                     // 553
    MainMenu *mGameSelector;                       // 554
    int unk555[2];                                 // 555 ~ 556, Board::DrawOverlay
    HelpTextScreen *mHelpTextScreen;               // 557
    int unkUnk;                                    // 558
    VSSetupMenu *mVSSetupMenu;                     // 559
    VSResultsMenu *mVSResultsMenu;                 // 560
    SeedChooserScreen *mSeedChooserScreen;         // 561
    SeedChooserScreen *mZombieChooserScreen;       // 562
    int *mAwardScreen;                             // 563
    CreditScreen *mCreditScreen;                   // 564
    ChallengeScreen *mChallengeScreen;             // 565
    TodFoley *mSoundSystem;                        // 566
    ListNodeBase mControlButtonList;               // 567 ~ 568
    ListNodeBase mCreatedImageList;                // 569 ~ 570
    homura::Storage<pvzstl::string> mReferId;      // 571
    homura::Storage<pvzstl::string> mRegisterLink; // 572
    homura::Storage<pvzstl::string> mMod;          // 573
    bool mRegisterResourcesLoaded;                 // 2296
    bool mTodCheatKeys;                            // 2297
    bool mNewIs3DAccelerated;                      // 2298，在对齐间隙插入新成员
    GameMode mGameMode;                            // 575
    GameScenes mGameScene;                         // 576
    bool mLoadingZombiesThreadCompleted;           // 2308
    bool mFirstTimeGameSelector;                   // 2309
    int mGamesPlayed;                              // 578
    int mMaxExecutions;                            // 579
    int mMaxPlays;                                 // 580
    int mMaxTime;                                  // 581
    bool mEasyPlantingCheat;                       // 2328
    PoolEffect *mPoolEffect;                       // 583
    ZenGarden *mZenGarden;                         // 584
    EffectSystem *mEffectSystem;                   // 585
    ReanimatorCache *mReanimatorCache;             // 586
    LawnPlayerInfo *mPlayerInfo;                   // 587
    Sexy::DefaultPlayerInfo *mPlayer2Info;         // 588 , 游戏内没有初始化，但有一些判定
    int *mLastLevelStats;                          // 589
    bool mCloseRequest;                            // 2360
    int mAppCounter;                               // 591
    Music2 *mMusic;                                // 592
    int mCrazyDaveReanimID;                        // 593
    CrazyDaveState mCrazyDaveState;                // 594
    int mCrazyDaveBlinkCounter;                    // 595
    int mCrazyDaveBlinkReanimID;                   // 596
    int mCrazyDaveMessageIndex;                    // 597
    int *mCrazyDaveMessageText;                    // 598
    int mAppRandSeed;                              // 599;
    bool mUnknownDrawFlag;                         // 600
    Sexy::DefaultProfileMgr *mProfileMgr;          // 601
    bool mSkipProfileSaving;                       // 602 * 4
    homura::Storage<Sexy::Level> mLevel;           // 603 ~ 636
    Sexy::Image *mQRCodeImage;                     // 637
    Sexy::Image *mQRCodeImageBackground;           // 638
    homura::Storage<StringIntMap> mKeyValueData;   // 639 ~ 644
    int mInitialSunMoney;                          // 645     // 这个数据能给玩家加初始阳光
    bool mIsFullVersion;                           // 2584
    int mPendingBuyToolId;                         // 647
    int mPendingRechargeAmount;                    // 648
    TrialType mTrialType;                          // 649
    int mVsInitialPlantMode;                       // 650
    int unk9_2[3];                                 // 651 ~ 653
    BoardResult mBoardResult;                      // 654
    bool mKilledYetiAndRestarted;                  // 2620
    TypingCheck *mKonamiCheck;                     // 656
    TypingCheck *mMustacheCheck;                   // 657
    TypingCheck *mMoustacheCheck;                  // 658
    TypingCheck *mSuperMowerCheck;                 // 659
    TypingCheck *mSuperMowerCheck2;                // 660
    TypingCheck *mFutureCheck;                     // 661
    TypingCheck *mPinataCheck;                     // 662
    TypingCheck *mDanceCheck;                      // 663
    TypingCheck *mDaisyCheck;                      // 664
    TypingCheck *mSukhbirCheck;                    // 665
    bool mMustacheMode;                            // 2664
    bool mSuperMowerMode;                          // 2665
    bool mFutureMode;                              // 2666
    bool mPinataMode;                              // 2667
    bool mDanceMode;                               // 2668
    bool mDaisyMode;                               // 2669
    bool mSukhbirMode;                             // 2670
    int unk668;                                    // 668
    bool mMuteSoundsForCutscene;                   // 2676
    bool mNeedLoadGame;                            // 2677
    bool mNeedGoBackToMain;                        // 2678
    SaveGameOperation mSaveGameOperation;          // 670
    int mSecondPlayerGamepadIndex;                 // 671
    int unk672;                                    // 672
    int mCurrentTestDialogId;                      // 673
    int mCurrentTestDaveMessage;                   // 674
    Mailbox *mMailBox;                             // 675
    int unk676;                                    // 676
    bool mMailboxRefreshed;                        // 677
    ReanimationID mSavingDingusReanimationID;      // 678
    float mSavingDingusCurrentY;                   // 679
    float mSavingDingusAnimationStartY;            // 680
    float mSavingDingusAnimationEndY;              // 681
    float mSavingDingusAnimationTime;              // 682
    homura::Storage<pvzstl::string> mDingusText;   // 683
    int mSavingDingusShowCount;                    // 684
    bool mProfileSaveInProgress;                   // 685[0]
    bool mProfileOperationPending1;                // 685[1]
    bool mProfileOperationPending2;                // 685[2]
    int mP2JoinPromptTimer;                        // 686
    bool unk687;
    SaveGameContext *mSaveGame;         // 688
    SessionTaskType mSessionTaskType;   // 689
    bool mLoginToServer;                // 690
    LawnSession mSession;               // 691 ~ 695
    int *mLawnSessionTask;              // 696
    pvzstl::string mGameInfoStrings[5]; // 697 ~ 701
    HelpBarWidget *mHelpBarWidget;      // 702
    int unk14;                          // 703
    int *mLogComposer;                  // 704
    int *MLogManager;                   // 705
    int mLaunchTime;                    // 706
    DaveHelp *mDaveHelp;                // 707
    MaskHelpWidget *mMaskHelpWidget;    // 708
    QueryCoinState mQueryCoinState;     // 709 ~ 713
    bool mCanDoBuyMoneyDialog;          // 714

    Reanimation *ReanimationGet(ReanimationID theReanimationID) {
        return reinterpret_cast<Reanimation *(*)(LawnApp *, ReanimationID)>(LawnApp_ReanimationGetAddr)(this, theReanimationID);
    }
    Reanimation *ReanimationTryToGet(ReanimationID theReanimationID) {
        return reinterpret_cast<Reanimation *(*)(LawnApp *, ReanimationID)>(LawnApp_ReanimationTryToGetAddr)(this, theReanimationID);
    }
    // void ClearSecondPlayer() {
    // reinterpret_cast<void (*)(LawnApp *)>(LawnApp_ClearSecondPlayerAddr)(this);
    // }
    bool CanShowStore() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_CanShowStoreAddr)(this);
    }
    bool CanShowAlmanac() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_CanShowAlmanacAddr)(this);
    }
    void KillNewOptionsDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_KillNewOptionsDialogAddr)(this);
    }
    void KillMainMenu() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_KillMainMenuAddr)(this);
    }
    void PlayFoleyPitch(FoleyType theFoleyType, float thePitch) {
        reinterpret_cast<void (*)(LawnApp *, FoleyType, float)>(LawnApp_PlayFoleyPitchAddr)(this, theFoleyType, thePitch);
    }
    void DoCheatDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_DoCheatDialogAddr)(this);
    }
    void WriteCurrentUserConfig() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_WriteCurrentUserConfigAddr)(this);
    }
    StoreScreen *ShowStoreScreen() {
        return reinterpret_cast<StoreScreen *(*)(LawnApp *)>(LawnApp_ShowStoreScreenAddr)(this);
    }
    AlmanacDialog *DoAlmanacDialog(SeedType theSeedType = SeedType::SEED_NONE, ZombieType theZombieType = ZombieType::ZOMBIE_INVALID) {
        return reinterpret_cast<AlmanacDialog *(*)(LawnApp *, SeedType, ZombieType)>(LawnApp_DoAlmanacDialogAddr)(this, theSeedType, theZombieType);
    }
    void DoCheatCodeDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_DoCheatCodeDialogAddr)(this);
    }
    void DoUserDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_DoUserDialogAddr)(this);
    }
    void DoRenameUserDialog(const pvzstl::string &theName) {
        reinterpret_cast<void (*)(LawnApp *, const pvzstl::string &)>(LawnApp_DoRenameUserDialogAddr)(this, theName);
    }
    Sexy::Dialog *DoConfirmRestartDialog() {
        return reinterpret_cast<Sexy::Dialog *(*)(LawnApp *)>(LawnApp_DoConfirmRestartDialogAddr)(this);
    }
    bool IsFirstTimeAdventureMode() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_IsFirstTimeAdventureModeAddr)(this);
    }
    // 阻塞式函数，能创建并立即展示一个带按钮的对话框。按钮个数由最后一个参数决定。其返回值就是用户按下的按钮ID，一般情况下只可能为1000/1001
    int LawnMessageBox(Dialogs theDialogId, // 用于标识本对话框的ID，以便于用KillDialog(theDialogId)关闭此对话框。一般用不到，所以随便填个数字就可以
                       const char *theHeaderName,
                       const char *theLinesName,
                       const char *theButton1Name,
                       const char *theButton2Name,
                       int theButtonMode) // 取值为0-3。其中0就是无按钮；1和2会展示两个按钮，其ID分别为1000和1001；3只会展示一个按钮，其ID为1000
    {
        return reinterpret_cast<int (*)(LawnApp *, Dialogs, const char *, const char *, const char *, const char *, int)>(LawnApp_LawnMessageBoxAddr)(
            this, theDialogId, theHeaderName, theLinesName, theButton1Name, theButton2Name, theButtonMode);
    }
    TodParticleSystem *AddTodParticle(float theX, float theY, int theRenderOrder, ParticleEffect theEffect) {
        return reinterpret_cast<TodParticleSystem *(*)(LawnApp *, float, float, int, ParticleEffect)>(LawnApp_AddTodParticleAddr)(this, theX, theY, theRenderOrder, theEffect);
    }
    ParticleSystemID ParticleGetID(TodParticleSystem *theParticle) {
        return reinterpret_cast<ParticleSystemID (*)(LawnApp *, TodParticleSystem *)>(LawnApp_ParticleGetIDAddr)(this, theParticle);
    }
    Reanimation *AddReanimation(float theX, float theY, int theRenderOrder, ReanimationType theReanimationType) {
        return reinterpret_cast<Reanimation *(*)(LawnApp *, float, float, int, ReanimationType)>(LawnApp_AddReanimationAddr)(this, theX, theY, theRenderOrder, theReanimationType);
    }
    bool IsSurvivalMode() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_IsSurvivalModeAddr)(this);
    }
    bool HasFinishedAdventure() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_HasFinishedAdventureAddr)(this);
    }
    bool CanSpawnYetis() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_CanSpawnYetisAddr)(this);
    }
    void HideHelpBarWidget() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_HideHelpBarWidgetAddr)(this);
    }
    void ShowHelpTextScreen(int thePage) {
        reinterpret_cast<void (*)(LawnApp *, int)>(LawnApp_ShowHelpTextScreenAddr)(this, thePage);
    }
    void CrazyDaveStopTalking() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_CrazyDaveStopTalkingAddr)(this);
    }
    void DoRetryAchievementsDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_DoRetryAchievementsDialogAddr)(this);
    }
    bool EarnedGoldTrophy() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_EarnedGoldTrophyAddr)(this);
    }
    void RemoveReanimation(ReanimationID theReanimationID) {
        reinterpret_cast<void (*)(LawnApp *, ReanimationID)>(LawnApp_RemoveReanimationAddr)(this, theReanimationID);
    }
    ReanimationID ReanimationGetID(Reanimation *theReanimation) {
        return reinterpret_cast<ReanimationID (*)(LawnApp *, Reanimation *)>(LawnApp_ReanimationGetIDAddr)(this, theReanimation);
    }
    void KillAlmanacDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_KillAlmanacDialogAddr)(this);
    }
    int GetNumTrophies(ChallengePage thePage) {
        return reinterpret_cast<int (*)(LawnApp *, ChallengePage)>(LawnApp_GetNumTrophiesAddr)(this, thePage);
    }
    void ShowMainMenuScreen() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_ShowMainMenuScreenAddr)(this);
    }
    TodParticleSystem *ParticleTryToGet(ParticleSystemID theParticleID) {
        return reinterpret_cast<TodParticleSystem *(*)(LawnApp *, ParticleSystemID)>(LawnApp_ParticleTryToGetAddr)(this, theParticleID);
    }
    void KillHelpTextScreen() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_KillHelpTextScreenAddr)(this);
    }
    void NextTestDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_NextTestDialogAddr)(this);
    }
    void KillBoard() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_KillBoardAddr)(this);
    }
    void SetBoardResult(int result) {
        reinterpret_cast<void (*)(LawnApp *, int)>(LawnApp_SetBoardResultAddr)(this, result);
    }
    void ShowGameSelector() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_ShowGameSelectorAddr)(this);
    }
    void SetSecondPlayer(int thePlayerIndex) {
        reinterpret_cast<void (*)(LawnApp *, int)>(LawnApp_SetSecondPlayerAddr)(this, thePlayerIndex);
    }
    void PlayFoley(FoleyType theFoleyType) {
        reinterpret_cast<void (*)(LawnApp *, FoleyType)>(LawnApp_PlayFoleyAddr)(this, theFoleyType);
    }
    void PlaySample(int theSoundNum) {
        reinterpret_cast<void (*)(LawnApp *, int, bool unknown)>(LawnApp_PlaySampleAddr)(this, theSoundNum, true);
    }
    int PlayerToGamepadIndex(int thePlayerIndex) {
        return reinterpret_cast<int (*)(LawnApp *, int)>(LawnApp_PlayerToGamepadIndexAddr)(this, thePlayerIndex);
    }
    void SafeDeleteWidget(Sexy::Widget *widget) { // vTable + 4 * 47
        reinterpret_cast<void (*)(LawnApp *, Sexy::Widget *)>(LawnApp_SafeDeleteWidgetAddr)(this, widget);
    }
    void KillVSSetupScreen() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_KillVSSetupScreenAddr)(this);
    }
    void StartPlaying() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_StartPlayingAddr)(this);
    }
    bool IsTrialStageLocked() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_IsTrialStageLockedAddr)(this);
    }
    void HandleCorruptedGameFile() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_HandleCorruptedGameFileAddr)(this);
    }
    void HandleOldGameFile() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_HandleOldGameFileFileAddr)(this);
    }
    void DoPauseDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_DoPauseDialogAddr)(this);
    }
    bool CanPauseNow() {
        return reinterpret_cast<bool (*)(LawnApp *)>(LawnApp_CanPauseNowAddr)(this);
    }

    LawnApp() {
        _constructor();
    };
    ~LawnApp() = delete;

    void Init();
    bool IsNight();
    bool IsIceDemo() {
        return false;
    }
    void HardwareInit();
    void DoBackToMain();
    void MakeNewBoard();
    Sexy::Dialog *ConfirmQuit() {
        return reinterpret_cast<Sexy::Dialog *(*)(LawnApp *)>(LawnApp_ConfirmQuitAddr)(this);
    }
    void PostLeaveLevel() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_PostLeaveLevelAddr)(this);
    }
    void PostEnterLevel() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_PostEnterLevelAddr)(this);
    }
    void KillDialog(Dialogs theId) {
        reinterpret_cast<void (*)(LawnApp *, Dialogs)>(LawnApp_KillDialogAddr)(this, theId);
    }
    void CheckForGameEnd() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_CheckForGameEndAddr)(this);
    }
    void UpdateSavingDingus() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_UpdateSavingDingusAddr)(this);
    }
    void DoContinueDialog() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_DoContinueDialogAddr)(this);
    }
    const LawnAppVTable *GetVTable() const {
        return (LawnAppVTable *)Sexy::ButtonListener::vTable;
    }
    void DoSettingsDialog(bool theIsModal);
    bool CanShopLevel();
    void DoNewOptions(bool theFromGameSelector, unsigned int a3);
    int GetNumPreloadingTasks();
    void DoConfirmBackToMain(bool theIsSave);
    void BuyFullVersion() {
        reinterpret_cast<void (*)(LawnApp *)>(LawnApp_BuyFullVersionAddr)(this);
    }
    bool MapToButtonEvent(const Sexy::Event *theEvent, Sexy::GamepadButton &theButtonCode, int &thePlayerIndex, unsigned int &theButtonFlags) {
        return reinterpret_cast<bool (*)(LawnApp *, const Sexy::Event *, Sexy::GamepadButton &, int &, unsigned int &)>(LawnApp_MapToButtonEventAddr)(
            this, theEvent, theButtonCode, thePlayerIndex, theButtonFlags);
    }
    int TrophiesNeedForGoldSunflower();
    int GamepadToPlayerIndex(unsigned int thePlayerIndex) const;
    void ShowCreditScreen(bool theIsFromMainMenu);
    void OnSessionTaskFailed();
    void UpdateApp();
    void ShowAwardScreen(AwardType theAwardType);
    void KillAwardScreen();
    void LoadLevelConfiguration(int a2, int a3);
    void LoadingThreadProc();
    bool IsChallengeWithoutSeedBank();
    void TryHelpTextScreen(HelpTextPage thePage);
    void KillSeedChooserScreen();
    bool IsIZombieLevel() const;
    bool IsWallnutBowlingLevel() const;
    bool IsAdventureMode() const;
    bool IsPuzzleMode() const;
    static bool IsSurvivalNormal(GameMode theGameMode);
    static bool IsSurvivalHard(GameMode theGameMode);
    static bool IsSurvivalEndless(GameMode theGameMode);
    static bool IsEndlessScaryPotter(GameMode theGameMode);
    static bool IsEndlessIZombie(GameMode theGameMode);
    bool IsLittleTroubleLevel() const;
    bool IsScaryPotterLevel() const;
    bool IsSlotMachineLevel() const;
    bool IsArtChallenge() const;
    bool IsSquirrelLevel() const;
    bool IsWhackAZombieLevel() const;
    bool IsStormyNightLevel() const;
    bool IsVSMode() const;
    bool IsCoopMode() const;
    bool IsTwinSunbankMode() const;
    bool IsMiniBossLevel() const;
    bool IsFinalBossLevel() const;
    void LoadAddonImages();
    void LoadAddonSounds();
    Sexy::Image *GetImageByFileName(const char *theFileName);
    int GetSoundByFileName(const char *theFileName);
    static void Load(const char *theGroupName);
    void DoConvertImitaterImages();
    int GetSeedsAvailable(bool theIsZombieChooser);
    bool HasSeedType(SeedType theSeedType, bool theIsZombie);
    bool GrantAchievement(AchievementType theAchievementId);
    void SetFoleyVolume(FoleyType theFoleyType, double theVolume) const;
    void ShowLeaderboards();
    void KillLeaderboards();
    void ShowZombatarScreen();
    void KillZombatarScreen();
    void SetHouseReanim(Reanimation *theHouseAnim);
    void LoadZombatarResources();
    PottedPlant *GetPottedPlantByIndex(int thePottedPlantIndex) const;
    void ClearSecondPlayer();
    bool Is3DAccelerated() const;
    void Set3DAccelerated(bool isAccelerated);
    void HandleTcpClientMessage(const std::byte *buf, size_t bufSize);
    void HandleTcpServerMessage(const std::byte *buf, size_t bufSize);
    void UpdateFrames();
    void FinishLoadGame();
    void ShowSeedChooserScreen();
    void ShowZombieChooserScreen();
    void KillZombieChooserScreen();
    void ShowChallengeScreen(ChallengePage thePage);
    void KillChallengeScreen();
    void ShowVSSetupScreen();
    void PreNewGame(GameMode theGameMode, bool theLookForSavedGame);
    void NewGame();
    bool HasBeatenChallenge(GameMode theGameMode) const;
    void ShowVSResultsScreen();
    void KillVSResultsScreen();
    void LoadingCompleted();
    bool TryLoadGame();

protected:
    friend void InitHookFunction();

    void _constructor();
    void _destructor();
};


inline bool gSlowMo;
inline bool gFastMo;
inline int gSlowMoCounter;
inline bool gStep;
inline bool gStepReady;
/***************************************************************************************************************/
inline bool disableShop;
inline bool doCheatDialog;     // 菜单DoCheatDialog
inline bool doCheatCodeDialog; // 菜单DoCheatCodeDialog
inline int gNetDelayNow = 0;
inline bool gNetPingHasValidDelay = false;
inline bool gNetPingAwaitingPong = false;
inline int gNetPingSendCounter = 0;
inline uint32_t gNetPingNowTick = 0;
inline uint16_t gNetPingLatestSentTick = 0;
inline uint16_t gNetPingLastPongTick = 0;
inline bool gHostPeerPingBaseValid = false;
inline int32_t gHostPeerPingBaseOffset = 0;
inline bool gSpectatePeerPingValid = false;
inline uint16_t gSpectatePeerPingToken = 0;
inline uint32_t gSpectatePeerPingRecvTick = 0;

inline std::vector<std::byte> clientRecvBuffer;
inline std::vector<std::byte> serverRecvBuffer;

inline void (*old_LawnApp_ClearSecondPlayer)(LawnApp *lawnApp);

inline void (*old_LawnApp_DoBackToMain)(LawnApp *lawnApp);

inline void (*old_LawnApp_ShowAwardScreen)(LawnApp *lawnApp, AwardType a2);

inline void (*old_LawnApp_KillAwardScreen)(LawnApp *lawnApp);

inline int (*old_LawnApp_GamepadToPlayerIndex)(LawnApp *lawnApp, unsigned int a2);

inline void (*old_LawnApp_UpDateApp)(LawnApp *lawnApp);

inline bool (*old_LawnApp_CanShopLevel)(LawnApp *lawnApp);

inline void (*old_LawnApp_ShowCreditScreen)(LawnApp *lawnApp, bool isFromMainMenu);

inline void (*old_LawnApp_LoadLevelConfiguration)(LawnApp *lawnApp, int a2, int a3);

inline void (*old_LawnApp_LawnApp)(LawnApp *lawnApp);

inline void (*old_LawnApp__destructor)(LawnApp *lawnApp);

inline void (*old_LawnApp_Init)(LawnApp *lawnApp);

inline void (*old_LawnApp_LoadingThreadProc)(LawnApp *lawnApp);

inline bool (*old_LawnApp_IsChallengeWithoutSeedBank)(LawnApp *lawnApp);

inline int (*old_LawnApp_GetSeedsAvailable)(LawnApp *lawnApp, bool isZombieChooser);

inline void (*old_LawnApp_HardwareInit)(LawnApp *lawnApp);

inline int (*old_LawnApp_GetNumPreloadingTasks)(LawnApp *lawnApp);

inline bool (*old_LawnApp_IsNight)(LawnApp *lawnApp);

inline bool (*old_LawnApp_HasSeedType)(LawnApp *lawnApp, SeedType theSeedType, bool theIsZombie);

inline void (*old_LawnApp_DoNewOptions)(LawnApp *lawnApp, bool a2, unsigned int a3);

inline void (*old_LawnApp_UpdateFrames)(LawnApp *lawnApp);

inline void (*old_LawnApp_KillSeedChooserScreen)(LawnApp *lawnApp);

inline void (*old_LawnApp_PreNewGame)(LawnApp *, GameMode theGameMode, bool theLookForSavedGame);

#endif // PVZ_LAWN_LAWN_APP_H
