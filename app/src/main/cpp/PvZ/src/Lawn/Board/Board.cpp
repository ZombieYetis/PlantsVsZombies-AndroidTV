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

#include "PvZ/Lawn/Board/Board.h"
#include "Homura/ContainerUtils.h"
#include "Homura/Logger.h"
#include "PvZ/Android/IntroVideo.h"
#include "PvZ/Android/Native/NativeApp.h"
#include "PvZ/Formation.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/MessageWidget.h"
#include "PvZ/Lawn/Board/OpeningEncounter.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/Projectile.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Board/ZenGarden.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/Common/LawnCommon.h"
#include "PvZ/Lawn/Common/Resources.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/System/ReanimationLawn.h"
#include "PvZ/Lawn/VSActionSystem.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/ReplayControlsWidget.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/ShovelRedirectWidget.h"
#include "PvZ/Lawn/Widget/VSResultsMenu.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Misc.h"
#include "PvZ/NetPlay.h"
#include "PvZ/PatchList.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/SexyAppFramework/GamepadApp.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"
#include "PvZ/TodLib/Effect/TodParticle.h"

#include <unistd.h>

#include <cmath>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <numbers>
#include <unordered_map>

using namespace Sexy;

namespace {
using IdMap = std::unordered_map<uint16_t, uint16_t>;
IdMap serverPlantIDMap;
IdMap serverZombieIDMap;
IdMap serverCoinIDMap;
IdMap serverGridItemIDMap;
constexpr uintptr_t kBoardButtonListenerVtableOffset = 0x1FC;
constexpr uintptr_t kBoardButtonListenerVTableOffset2 = 0x228;
constexpr uint8_t kNoSelectedSeedIndex = UINT8_MAX;

int DecodeSelectedSeedIndex(uint8_t encodedIndex, const SeedBank *seedBank) {
    if (encodedIndex == kNoSelectedSeedIndex) {
        return -1;
    }

    const int selectedIndex = encodedIndex;
    if (seedBank == nullptr || selectedIndex >= seedBank->mNumPackets || selectedIndex >= 10) {
        LOG_WARN("[NETPLAY] ignore invalid selected seed index={} packetCount={}", selectedIndex, seedBank ? seedBank->mNumPackets : -1);
        return -1;
    }
    return selectedIndex;
}

// 新增：远端暂停同步保护
bool gPauseSyncFromRemote = false;

} // namespace

void Board::_constructor(LawnApp *theApp) {
    Sexy::Widget::_constructor();
    Widget::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(vTableForBoardAddr) + 8);
    ButtonListener::vTable = reinterpret_cast<const Sexy::ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(Widget::vTable) + kBoardButtonListenerVtableOffset);
    SyncObject::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(Widget::vTable) + kBoardButtonListenerVTableOffset2);
    auto &syncBlocks = mSyncBlocks.Construct();

    mFlowerPotTree.Construct();
    mTangleKelpTree.Construct();
    mPumpkinTree.Construct();
    mUnknownStringIntMap.Construct();

    constexpr std::uintptr_t kBoardSyncStart = 0x259;
    constexpr std::uintptr_t kBoardSyncEnd = 0x58D2;
    constexpr std::uint32_t kBoardSyncSize = kBoardSyncEnd - kBoardSyncStart; // 0x5679
    auto *boardBytes = reinterpret_cast<std::uint8_t *>(this);
    void *syncStart = boardBytes + kBoardSyncStart;
    syncBlocks.push_back({syncStart, kBoardSyncSize});
    std::memset(syncStart, 0, kBoardSyncSize);

    mStringSecondPlayer.Construct();

    mApp = theApp;
    mApp->mBoard = this;
    unknownBool = false;

    mZombies.DataArrayInitialize(1024U, "zombies");
    mZombies.mNextKey = 0xDF6D;
    mPlants.DataArrayInitialize(1024U, "plants");
    mPlants.mNextKey = 0xDC61;
    mProjectiles.DataArrayInitialize(1024U, "projectiles");
    mProjectiles.mNextKey = 0xD26F;
    mCoins.DataArrayInitialize(1024U, "coins");
    mCoins.mNextKey = 0xDF69;
    mLawnMowers.DataArrayInitialize(32U, "lawnmowers");
    mLawnMowers.mNextKey = 0xD761;
    mGridItems.DataArrayInitialize(128U, "griditems");
    mGridItems.mNextKey = 0xD269;

    mApp->mEffectSystem->EffectSystemFreeAll();
    mBoardRandSeed = mApp->mAppRandSeed;
    if (mApp->IsSurvivalMode()) {
        mBoardRandSeed = Sexy::Rand();
    }
    mCoinBankFadeCount = 0;
    mLevel = 0;

    for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
        int gamepadIndex = -1;
        if (playerIndex == 0) {
            gamepadIndex = mApp->mPlayerInfo->GetId();
        } else if (mApp->mGameMode != GAMEMODE_CHALLENGE_ZEN_GARDEN && mApp->mGameMode != GAMEMODE_TREE_OF_WISDOM) {
            gamepadIndex = mApp->mSecondPlayerGamepadIndex;
        }

        if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
            mGamepadControls[playerIndex] = new ZenGardenControls(this, playerIndex, gamepadIndex);
        } else if (mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
            mGamepadControls[playerIndex] = new TreeOfWisdomControls(this, playerIndex, gamepadIndex);
        } else {
            mGamepadControls[playerIndex] = new GamepadControls_(this, playerIndex, gamepadIndex);
        }

        auto snap = mGamepadControls[playerIndex]->GetSnapToGridPos();
        mGamepadControls[playerIndex]->mCursorPositionX = snap.mX;
        mGamepadControls[playerIndex]->mCursorPositionY = snap.mY;
        mGamepadControls[playerIndex]->mGridCenterPositionX = snap.mX;
        mGamepadControls[playerIndex]->mGridCenterPositionY = snap.mY;

        mCursorObject[playerIndex] = new CursorObject();
        mCursorPreview[playerIndex] = new CursorPreview(playerIndex);
    }

    mSunMoney2 = 0;
    mSunMoney1 = 0;
    mDeathMoney = 0;
    mSeedBank[0] = new SeedBank(false);
    mSeedBank[1] = nullptr;
    if (mApp->mGameMode >= GameMode::GAMEMODE_MULTI_PLAYER && mApp->mGameMode <= GameMode::GAMEMODE_MP_VS) {
        mSeedBank[1] = new SeedBank(true);
    }
    if (mApp->IsCoopMode()) {
        mSeedBank[1] = new SeedBank(false);
    }

    mCutScene = new CutScene();
    mSpecialGraveStoneX = -1;
    mSpecialGraveStoneY = -1;
    for (int i = 0; i < MAX_GRID_SIZE_X; ++i) {
        for (int j = 0; j < MAX_GRID_SIZE_Y; ++j) {
            mGridSquareType[i][j] = GridSquareType::GRIDSQUARE_GRASS;
            mGridCelLook[i][j] = Sexy::Rand(20);
            mGridCelOffset[i][j][0] = Sexy::Rand(10) - 5;
            mGridCelOffset[i][j][1] = Sexy::Rand(10) - 5;
        }
        for (int j = 0; j < MAX_GRID_SIZE_Y + 1; ++j) {
            mGridCelFog[i][j] = 0;
        }
    }

    mSunCountDown = 0;
    mShakeCounter = 0;
    mShakeAmountX = 0;
    mShakeAmountY = 0;
    mPaused = false;
    mBoardFadeOutCounter = -1;
    mLevelAwardSpawned = false;
    mFlagRaiseCounter = 0;
    mIceTrapCounter = 0;
    mLevelComplete = false;
    mNextSurvivalStageCounter = 0;
    mScoreNextMowerCounter = 0;
    mProgressMeterWidth = 0;
    mPoolSparklyParticleID = ParticleSystemID::PARTICLESYSTEMID_NULL;
    mFogBlownCountDown = 0;
    mFogOffset = 0.0f;
    mOffsetMoved = 0;
    std::memset(mCoverLayerAnimIDs, 0, sizeof(mCoverLayerAnimIDs));
    mFwooshCountDown = 0;
    mTimeStopCounter = 0;
    mCobCannonCursorDelayCounter = 0;
    mCobCannonMouseX = 0;
    mCobCannonMouseY = 0;
    mDroppedFirstCoin = false;
    mBonusLawnMowersRemaining = 0;
    mEnableGraveStones = false;
    mHelpIndex = AdviceType::ADVICE_NONE;
    mEffectCounter = 0;
    mDrawCount = 0;
    mRiseFromGraveCounter = 0;
    mFinalWaveSoundCounter = 0;
    mTriggeredLawnMowers = 0;
    mPlayTimeActiveLevel = 0;
    mPlayTimeInactiveLevel = 0;
    mMaxSunPlants = 0;
    mStartDrawTime = 0;
    mIntervalDrawTime = 0;
    mIntervalDrawCountStart = 0;
    mPreloadTime = 0;
    mGameID = static_cast<int>(std::time(nullptr));
    mMinFPS = 1000.0f;
    mGravesCleared = 0;
    mPlantsEaten = 0;
    mPlantsShoveled = 0;
    mCoinsCollected = 0;
    mDiamondsCollected = 0;
    mPottedPlantsCollected = 0;
    mChocolateCollected = 0;
    std::memset(mFwooshID, 0, sizeof(mFwooshID));
    mPrevMouseX = -1;
    mPrevMouseY = -1;
    mFinalBossKilled = false;
    mMustacheMode = mApp->mMustacheMode;
    mSuperMowerMode = mApp->mSuperMowerMode;
    mFutureMode = mApp->mFutureMode;
    mPinataMode = mApp->mPinataMode;
    mDanceMode = mApp->mDanceMode;
    mDaisyMode = mApp->mDaisyMode;
    mSukhbirMode = mApp->mSukhbirMode;
    mShowShovel = false;
    mShowButter = false;
    mShowHammer = false;
    mToolTip = new ToolTipWidget();
    mDebugFont = Sexy::FONT_BRIANNETOD12;
    mAdvice = new CustomMessageWidget(mApp);
    mBackground = BackgroundType::BACKGROUND_1_DAY;
    mMainCounter = 0;
    mTutorialState = TutorialState::TUTORIAL_OFF;
    mTutorialParticleID = nullptr;
    mTutorialTimer = -1;
    mChallenge = new Challenge();
    mClip = false;
    mDebugTextMode = DebugTextMode::DEBUG_TEXT_NONE;
    mUnknown214 = 0;
    mUnknown58E8 = true;
    mUnknown58E9 = false;
    mUnknownStringSet.Construct();
    mUnknown5904 = 0;

    pvzstl::string str = (theApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || theApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) ? "[MAIN_MENU_BUTTON]" : "[MENU_BUTTON]";
    mBoardMenuButton = MakeButton(1000, this, this, str);
    mBoardMenuButton->Resize(705, -3, 120, 80);
    mBoardMenuButton->mBtnNoDraw = true;
    mBoardMenuButton->mDisabled = true;
    if (theApp->IsCoopMode() || theApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        mBoardMenuButton->Resize(880, -3, 120, 80);
    } else if (theApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || theApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
        mBoardMenuButton->Resize(650, 550, 170, 120);
    }

    if (theApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
        mBoardStoreButton = MakeButton(1001, this, this, "[START_ONSLAUGHT]");
        mBoardStoreButton->Resize(0, 0, 0, 0);
        mBoardStoreButton->mBtnNoDraw = true;
        mBoardStoreButton->mDisabled = true;
    } else {
        mBoardStoreButton = MakeButton(1001, this, this, "[SHOP_BUTTON]");
        if (theApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || theApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
            mBoardStoreButton->Resize(0, 550, 170, 120);
        } else {
            mBoardStoreButton->Resize(0, 0, 0, 0);
            mBoardStoreButton->mBtnNoDraw = true;
            mBoardStoreButton->mDisabled = true;
        }
    }
    if (theApp->IsVSMode()) {
        mShovelWidget = new ShovelRedirectWidget(this);
        mShovelWidget->Resize(gTouchVSShovelRect.mX, gTouchVSShovelRect.mY, gTouchVSShovelRect.mWidth, gTouchVSShovelRect.mHeight);
        serverPlantIDMap.clear();
        serverZombieIDMap.clear();
        serverCoinIDMap.clear();
        serverGridItemIDMap.clear();
    }
    mReplayControlsWidget = new ReplayControlsWidget(this);
}

void Board::_destructor() {
    Widget::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(vTableForBoardAddr) + 8);
    ButtonListener::vTable = reinterpret_cast<const Sexy::ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(Widget::vTable) + kBoardButtonListenerVtableOffset);
    SyncObject::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(Widget::vTable) + kBoardButtonListenerVTableOffset2);

    delete mReplayControlsWidget;
    delete mBoardStoreButton;
    delete mBoardMenuButton;
    delete mShovelWidget;

    delete mAdvice;
    delete mSeedBank[0];
    delete mSeedBank[1];

    mZombies.DataArrayDispose();
    mPlants.DataArrayDispose();
    mProjectiles.DataArrayDispose();
    mCoins.DataArrayDispose();
    mLawnMowers.DataArrayDispose();
    mGridItems.DataArrayDispose();

    delete mToolTip;
    delete mCutScene;
    delete mChallenge;

    for (int aPlayerIndex = 0; aPlayerIndex < 2; ++aPlayerIndex) {
        GamepadControls *aControls = mGamepadControls[aPlayerIndex];
        if (aControls != nullptr) {
            aControls->GetVTable()->deletingDestructor(aControls);
            //            homura::CallVirtualFunc<GamepadControls, 17, void>(aControls); // delete GamepadControls/ZenGardenControls/TreeOfWisdomControls
        }

        delete mCursorObject[aPlayerIndex];
        delete mCursorPreview[aPlayerIndex];
    }

    GetVTable()->RemoveAllWidgets(this, false, false, nullptr);
    // RemoveAllWidgets
    //    homura::CallVirtualFunc<Board, 11, void, bool, bool, WidgetManager *>(this, false, false, nullptr);

    std::memset(mCoverLayerAnimIDs, 0, sizeof(mCoverLayerAnimIDs));

    for (const auto &aResourceName : *mUnknownStringSet) {
        TodDeleteResources(aResourceName);
    }
    mUnknownStringSet.Destruct();
    mUnknownStringIntMap.Destruct();

    mStringSecondPlayer.Destruct();

    mPumpkinTree.Destruct();
    mFlowerPotTree.Destruct();
    mTangleKelpTree.Destruct();

    mSyncBlocks.Destruct();

    SyncObject::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(vTableForSyncObjectAddr) + 8);
    ButtonListener::vTable = reinterpret_cast<const Sexy::ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(vTableForSexyButtonListenerAddr) + 8);

    Sexy::Widget::_destructor();
}

void Board::AddedToManager(WidgetManager *theWidgetManager) {
    old_Board_AddedToManager(this, theWidgetManager);

    if (mShovelWidget != nullptr && mParent != nullptr) {
        mParent->AddWidget(mShovelWidget);
    }
    AddWidget(mBoardMenuButton);
    AddWidget(mBoardStoreButton);
    AddWidget(mReplayControlsWidget);
}

void Board::RemovedFromManager(WidgetManager *theWidgetManager) {
    if (mShovelWidget != nullptr) {
        if (mShovelWidget->mParent != nullptr) {
            mShovelWidget->mParent->RemoveWidget(mShovelWidget);
        }
    }

    RemoveWidget(mBoardStoreButton);
    RemoveWidget(mBoardMenuButton);
    RemoveWidget(mReplayControlsWidget);


    old_Board_RemovedFromManager(this, theWidgetManager);
}

Projectile *Board::AddProjectile(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theProjectileType) {
    Projectile *aProjectile = mProjectiles.DataArrayAlloc();
    aProjectile->ProjectileInitialize(theX, theY, theRenderOrder, theRow, theProjectileType);
    mPeaShooterUsed = true;
    return aProjectile;
}

void Board::SpawnTeleportEffect(float theX, float theY, int theRow) {
    constexpr float TELEPORT_REANIM_OFFSET_X = -10.0f;
    constexpr float TELEPORT_REANIM_OFFSET_Y = -60.0f;
    const int aRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, theRow, 1);
    Reanimation *aTeleport = mApp->AddReanimation(theX + TELEPORT_REANIM_OFFSET_X, theY + TELEPORT_REANIM_OFFSET_Y, aRenderOrder, ReanimationType::REANIM_TELEPORTATION);
    if (aTeleport != nullptr) {
        aTeleport->SetFramesForLayer("anim_teleportation");
        aTeleport->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_FULL_LAST_FRAME;
        aTeleport->mAnimRate = 24.0f;
        mApp->PlayFoley(FoleyType::FOLEY_TELEPORTATION);
    }
}

void Board::TeleportZombie(Zombie *theZombie, float theDestX) {
    if (theZombie == nullptr || theZombie->IsDeadOrDying()) {
        return;
    }

    Zombie *aPartner = nullptr;
    if (theZombie->mZombieType == ZombieType::ZOMBIE_DOGWALKER || theZombie->mZombieType == ZombieType::ZOMBIE_DOG) {
        aPartner = theZombie->GetDogPartner();
        if (aPartner != nullptr && aPartner->IsDeadOrDying()) {
            aPartner = nullptr;
        }
    }

    float aDeltaX = std::max(0.0f, theDestX) - theZombie->mPosX;
    if (aPartner != nullptr) {
        aDeltaX = std::max(aDeltaX, -aPartner->mPosX);
    }

    auto TeleportOneZombie = [this, aDeltaX](Zombie *aZombie) {
        SpawnTeleportEffect(aZombie->mPosX + 30.0f, aZombie->mPosY + 55.0f, aZombie->mRow);
        aZombie->mPosX += aDeltaX;
        aZombie->mX = int(aZombie->mPosX);
        aZombie->mJustGotShotCounter = 150;
        SpawnTeleportEffect(aZombie->mPosX + 30.0f, aZombie->mPosY + 55.0f, aZombie->mRow);
    };

    TeleportOneZombie(theZombie);
    if (aPartner != nullptr) {
        TeleportOneZombie(aPartner);
    }
}

bool Board::TeleportPlant(Plant *thePlant, int theDestGridX, int theDestGridY) {
    if (thePlant == nullptr || thePlant->NotOnGround() || thePlant->IsInvulnerable()) {
        return false;
    }

    const int aOriginGridX = thePlant->mPlantCol;
    const int aOriginGridY = thePlant->mRow;
    SpawnTeleportEffect(GridToPixelX(aOriginGridX, aOriginGridY), GridToPixelY(aOriginGridX, aOriginGridY) + 20.0f, aOriginGridY);

    const SeedType aSeedType = thePlant->mSeedType == SeedType::SEED_IMITATER ? thePlant->mImitaterType : thePlant->mSeedType;
    const int aDestGridY = aOriginGridY;
    int aDestGridX = -1;
    const int aSearchStartGridX = std::max(aOriginGridX + 2, theDestGridX);
    if (theDestGridY == aDestGridY) {
        for (int aGridX = aSearchStartGridX; aGridX < MAX_GRID_SIZE_X; ++aGridX) {
            if (CanPlantAt(aGridX, aDestGridY, aSeedType) == PlantingReason::PLANTING_OK) {
                aDestGridX = aGridX;
                break;
            }
        }
    }

    if (aDestGridX == -1) {
        thePlant->Die();
        return false;
    }

    thePlant->mPlantCol = aDestGridX;
    thePlant->mRow = aDestGridY;
    thePlant->mX = GridToPixelX(aDestGridX, aDestGridY);
    thePlant->mY = GridToPixelY(aDestGridX, aDestGridY);
    thePlant->mEatenFlashCountdown = 150;
    thePlant->UpdateReanim();
    SpawnTeleportEffect(GridToPixelX(aDestGridX, aDestGridY), GridToPixelY(aDestGridX, aDestGridY) + 20.0f, aDestGridY);
    return true;
}

bool Board::IterateProjectiles(Projectile *&theProjectile) {
    if (theProjectile == reinterpret_cast<Projectile *>(-1)) {
        return false;
    }

    while (mProjectiles.IterateNext(theProjectile)) {
        if (!theProjectile->mDead) {
            return true;
        }
    }

    theProjectile = reinterpret_cast<Projectile *>(-1);
    return false;
}

void Board::ProcessDeleteQueue() {
    for (Plant *aPlant = nullptr; mPlants.IterateNext(aPlant);) {
        if (aPlant->mDead) {
            mPlants.DataArrayFree(aPlant);
        }
    }

    for (Zombie *aZombie = nullptr; mZombies.IterateNext(aZombie);) {
        if (aZombie->mDead) {
            mZombies.DataArrayFree(aZombie);
        }
    }

    for (Projectile *aProjectile = nullptr; mProjectiles.IterateNext(aProjectile);) {
        if (aProjectile->mDead) {
            mProjectiles.DataArrayFree(aProjectile);
        }
    }

    for (Coin *aCoin = nullptr; mCoins.IterateNext(aCoin);) {
        if (aCoin->mDead) {
            mCoins.DataArrayFree(aCoin);
        }
    }

    for (LawnMower *aMower = nullptr; mLawnMowers.IterateNext(aMower);) {
        if (aMower->mDead) {
            mLawnMowers.DataArrayFree(aMower);
        }
    }

    for (GridItem *aGridItem = nullptr; mGridItems.IterateNext(aGridItem);) {
        if (aGridItem->mDead) {
            mGridItems.DataArrayFree(aGridItem);
        }
    }
}

void Board::InitLevel() {
    old_Board_InitLevel(this);
    mNewWallNutAndSunFlowerAndChomperOnly = !(mApp->IsScaryPotterLevel() || mApp->IsIZombieLevel() || mApp->IsWhackAZombieLevel() || HasConveyorBeltSeedBank(0) || mApp->IsChallengeWithoutSeedBank());
    mNewPeaShooterCount = 0;
}

void Board::SetGrids() {
    // 更换场地时需要，用于初始化每一个格子的类型。
    for (int i = 0; i < MAX_GRID_SIZE_X; i++) {
        for (int j = 0; j < MAX_GRID_SIZE_Y; j++) {
            if (mPlantRow[j] == PlantRowType::PLANTROW_DIRT) {
                mGridSquareType[i][j] = GridSquareType::GRIDSQUARE_DIRT;
            } else if (mPlantRow[j] == PlantRowType::PLANTROW_POOL && i >= 0 && i <= 8) {
                mGridSquareType[i][j] = GridSquareType::GRIDSQUARE_POOL;
            } else if (mPlantRow[j] == PlantRowType::PLANTROW_HIGH_GROUND && i >= 4 && i <= 8) {
                mGridSquareType[i][j] = GridSquareType::GRIDSQUARE_HIGH_GROUND;
            } else if (mPlantRow[j] == PlantRowType::PLANTROW_NORMAL) {
                mGridSquareType[i][j] = GridSquareType::GRIDSQUARE_GRASS;
            }
        }
    }
}

namespace {
const char *GetServerModeTransportSuffix() {
    if (!gIsServerModeNetplay) {
        return "";
    }
    return gServerModeTransport == ServerModeTransport::P2P ? " [P2P]" : gServerModeTransport == ServerModeTransport::RELAY ? " [Relay]" : "";
}
} // namespace

void Board::ShovelDown() {
    // 用于铲掉光标正下方的植物。

    if (mApp->IsVSMode() && mApp->mGameScene != SCENE_PLAYING) { // 对战正式开始对局后才能真正铲除植物
        return;
    }

    requestDrawShovelInCursor = false;
    if (gTcpClientSocket >= 0) {
        U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
        netplay::PutEvent(event);
    }
    bool isInShovelTutorial = (unsigned int)(mTutorialState - 15) <= 2;
    if (isInShovelTutorial) {
        // 如果正在铲子教学中(即冒险1-5的保龄球的开场前，戴夫要求你铲掉三个豌豆的这段时间),则发送铲除键来铲除。
        mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_QUICK_DIG, 1112);
        ClearCursor(0);
        RefreshSeedPacketFromCursor(0);
        return;
    }
    // 下方就是自己写的铲除逻辑喽。
    float aXPos = mGamepadControls[0]->mCursorPositionX;
    float aYPos = mGamepadControls[0]->mCursorPositionY;
    Plant *aPlantUnderShovel = ToolHitTest(aXPos, aYPos);
    if (aPlantUnderShovel != nullptr) {
        if (gTcpClientSocket >= 0) {
            BaseEvent event = {EventType::EVENT_SERVER_BOARD_GAMEPAD_USE_SHOVEL};
            netplay::PutEvent(event);
        }
        mApp->PlayFoley(FoleyType::FOLEY_USE_SHOVEL); // 播放铲除音效
        aPlantUnderShovel->Die();                     // 让被铲的植物趋势
        SeedType aSeedType = aPlantUnderShovel->mSeedType;
        int aRow = aPlantUnderShovel->mRow;
        if (aSeedType == SeedType::SEED_CATTAIL && GetTopPlantAt(aPlantUnderShovel->mPlantCol, aRow, PlantPriority::TOPPLANT_ONLY_PUMPKIN) != nullptr) {
            // 如果铲的是南瓜套内的猫尾草,则再在原地种植一个荷叶
            NewPlant(aPlantUnderShovel->mPlantCol, aRow, SeedType::SEED_LILYPAD, SeedType::SEED_NONE, -1);
        }
        if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
            if (mChallenge->mChallengeState == ChallengeState::STATECHALLENGE_NORMAL && mApp->mGameScene == GameScenes::SCENE_PLAYING) {
                int aCost = Plant::GetCost(aSeedType, aPlantUnderShovel->mImitaterType);
                int num = aCost / 25;
                if (aSeedType == SeedType::SEED_GARLIC || aSeedType == SeedType::SEED_WALLNUT || aSeedType == SeedType::SEED_TALLNUT || aSeedType == SeedType::SEED_PUMPKINSHELL) {
                    int mPlantHealth = aPlantUnderShovel->mPlantHealth;
                    int mPlantMaxHealth = aPlantUnderShovel->mPlantMaxHealth;
                    num = (mPlantHealth * 3 > mPlantMaxHealth * 2) ? num : 0;
                }
                for (int i = 0; i < num; i++) {
                    Coin *aCoin = AddCoin(aXPos, aYPos, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_FROM_PLANT);
                    aCoin->Collect(0);
                }
            }
        }
    }

    ClearCursor(0);
    RefreshSeedPacketFromCursor(0);
}

void Board::UpdateGame() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        UpdateGameObjects();
        return;
    }

    old_Board_UpdateGame(this);

    // 防止选卡界面浓雾遮挡僵尸
    if (mFogBlownCountDown > 0 && mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO) {
        float thePositionStart = 1065.0 - 4 * 80.0 + 100; // 1065f - LeftFogColumn() * 80f + Constants.BOARD_EXTRA_ROOM;
        mFogOffset = TodAnimateCurveFloat(200, 0, mFogBlownCountDown, thePositionStart, 0.0, TodCurves::CURVE_EASE_OUT);
    }
}

void Board::UpdateGameObjects() {
    // 修复过关后游戏卡住不动
    if (mBoardFadeOutCounter > 0) {
        // 如果已经过关，则手动刷新植物，僵尸，子弹
        Plant *aPlant = nullptr;
        while (IteratePlants(aPlant)) {
            aPlant->Update();
        }
        Zombie *aZombie = nullptr;
        while (IterateZombies(aZombie)) {
            aZombie->Update();
        }
        Projectile *aProjectile = nullptr;
        while (IterateProjectiles(aProjectile)) {
            aProjectile->Update();
        }
    }

    old_Board_UpdateGameObjects(this);
}

void Board::DrawDebugText(Sexy::Graphics *g) {
    // 出僵DEBUG功能
    if (drawDebugText && !IsOnlineServerModeActive() && !gIsReplayMode) {
        DebugTextMode tmp = mDebugTextMode;
        mDebugTextMode = DebugTextMode::DEBUG_TEXT_ZOMBIE_SPAWN;
        old_Board_DrawDebugText(this, g);
        mDebugTextMode = tmp;
        return;
    }

    old_Board_DrawDebugText(this, g);
}

void Board::DrawDebugObjectRects(Sexy::Graphics *g) {
    // 碰撞体积绘制
    if (drawDebugRects && !IsOnlineServerModeActive() && !gIsReplayMode) {
        DebugTextMode tmp = mDebugTextMode;
        mDebugTextMode = DebugTextMode::DEBUG_TEXT_COLLISION;
        old_Board_DrawDebugObjectRects(this, g);
        if (mApp->IsVSMode()) {
            GridItem *aGridItem = nullptr;
            while (IterateGridItems(aGridItem)) {
                switch (aGridItem->mGridItemType) {
                    case GRIDITEM_MP_TARGET_ZOMBIE:
                    case GRIDITEM_MP_BURIAL_MOUND:
                    case GRIDITEM_GRAVESTONE: {
                        g->SetColor(Color{0, 255, 0});
                        g->DrawRect(aGridItem->GetItemRect());
                    } break;
                    default:
                        break;
                }
            }
        }
        mDebugTextMode = tmp;
        return;
    }

    old_Board_DrawDebugObjectRects(this, g);
}

void Board::DrawFadeOut(Sexy::Graphics *g) {
    // 修复关卡完成后的白色遮罩无法遮住整个屏幕
    if (mBoardFadeOutCounter < 0) {
        return;
    }

    if (IsSurvivalStageWithRepick()) {
        return;
    }

    int theAlpha = TodAnimateCurve(200, 0, mBoardFadeOutCounter, 0, 255, TodCurves::CURVE_LINEAR);
    if (mLevel == 9 || mLevel == 19 || mLevel == 29 || mLevel == 39 || mLevel == 49) {
        Color theColor = {0, 0, 0, theAlpha};
        g->SetColor(theColor);
    } else {
        Color theColor = {255, 255, 255, theAlpha};
        g->SetColor(theColor);
    }

    g->SetColorizeImages(true);
    Rect fullScreenRect = {-240, -60, 1280, 720};
    // 修复BUG的核心原理，就是不要在此处PushState和PopState，而是直接FillRect。这将保留graphics的trans属性。
    g->FillRect(fullScreenRect);
}

int Board::GetCurrentPlantCost(SeedType theSeedType, SeedType theImitaterType) {
    // 无限阳光
    if (infiniteSun && !IsOnlineServerModeActive() && !gIsReplayMode)
        return 0;

    if (theSeedType == SeedType::SEED_ZOMBIE_MOUND) {
        GamepadControls *aGamepad = mGamepadControls[0]->mIsZombie ? mGamepadControls[0] : (mGamepadControls[1]->mIsZombie ? mGamepadControls[1] : nullptr);
        int aGridX = PixelToGridXKeepOnBoard(int(aGamepad->mCursorPositionX), int(aGamepad->mCursorPositionY));
        int aGridY = PixelToGridYKeepOnBoard(int(aGamepad->mCursorPositionX), int(aGamepad->mCursorPositionY));
        GridItem *aMound = GetMoundAt(aGridX, aGridY);
        if (aMound) {
            return aMound->GetMoundUpgradeCost();
        }
    }

    return old_Board_GetCurrentPlantCost(this, theSeedType, theImitaterType);
}

void Board::AddSunMoney(int theAmount, int thePlayerIndex) {
    // 无限阳光
    if (infiniteSun && !IsOnlineServerModeActive() && !gIsReplayMode) {
        if (thePlayerIndex == 0) {
            mSunMoney1 = 9990;
        } else {
            mSunMoney2 = 9990;
        }
    } else {
        old_Board_AddSunMoney(this, theAmount, thePlayerIndex);
    }
}

void Board::AddDeathMoney(int theAmount) {
    // 无限阳光
    if (infiniteSun && !IsOnlineServerModeActive() && !gIsReplayMode) {
        mDeathMoney = 9990;
    } else {
        old_Board_AddDeathMoney(this, theAmount);
    }
}

bool Board::IsIceAt(int theGridX, int theGridY) {
    if (mIceTimer[theGridY] == 0 || mIceMinX[theGridY] > 750)
        return false;

    return theGridX >= PixelToGridXKeepOnBoard(mIceMinX[theGridY] + 12, 0);
}

PlantingReason Board::CanPlantAt(int theGridX, int theGridY, SeedType theSeedType) {
    // 自由种植！
    if (FreePlantAt && !IsOnlineServerModeActive() && !gIsReplayMode) {
        return PlantingReason::PLANTING_OK;
    }

    // 目标位置不在场地内，则返回“不能种在那里”
    if (theGridX < 0 || theGridX >= MAX_GRID_SIZE_X || theGridY < 0 || theGridY >= MAX_GRID_SIZE_Y) {
        return PlantingReason::PLANTING_NOT_HERE;
    }

    // 从关卡玩法上，判断能否种植
    PlantingReason aReason = mChallenge->CanPlantAt(theGridX, theGridY, theSeedType);
    if (aReason != PlantingReason::PLANTING_OK || Challenge::IsZombieSeedType(theSeedType)) {
        return aReason;
    }

    PlantsOnLawn aPlantOnLawn{};
    GetPlantsOnLawn(theGridX, theGridY, &aPlantOnLawn);
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
        if (aPlantOnLawn.mUnderPlant || aPlantOnLawn.mPumpkinPlant || aPlantOnLawn.mFlyingPlant || aPlantOnLawn.mNormalPlant) {
            return PlantingReason::PLANTING_NOT_HERE;
        }
        if (mApp->mZenGarden->mGardenType == GARDEN_AQUARIUM && !Plant::IsAquatic(theSeedType)) {
            return PlantingReason::PLANTING_NOT_ON_WATER;
        }

        return PlantingReason::PLANTING_OK;
    }

    // 墓碑吞噬者只能种植在墓碑上
    bool aHasGrave = GetGraveStoneAt(theGridX, theGridY);
    bool aHasMound = GetMoundAt(theGridX, theGridY);
    if (theSeedType == SeedType::SEED_GRAVEBUSTER) {
        if (aPlantOnLawn.mNormalPlant) {
            return PlantingReason::PLANTING_NOT_HERE;
        }

        return (aHasGrave || aHasMound) ? PlantingReason::PLANTING_OK : PlantingReason::PLANTING_ONLY_ON_GRAVES;
    }
    if (theSeedType == SeedType::SEED_INSTANT_COFFEE) {
        if (aPlantOnLawn.mFlyingPlant) {
            return PlantingReason::PLANTING_NOT_HERE;
        }

        if (!aPlantOnLawn.mNormalPlant || !aPlantOnLawn.mNormalPlant->mIsAsleep || aPlantOnLawn.mNormalPlant->mWakeUpCounter > 0
            || aPlantOnLawn.mNormalPlant->mOnBungeeState == PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
            return PlantingReason::PLANTING_NEEDS_SLEEPING;
        }

        return PlantingReason::PLANTING_OK;
    }
    // 非墓碑吞噬者且非飞行植物，则不能种在墓碑上
    if (aHasGrave) {
        return Plant::IsFlying(theSeedType) ? PlantingReason::PLANTING_OK : PlantingReason::PLANTING_NOT_ON_GRAVE;
    }

    Plant *aUnderPlant = aPlantOnLawn.mUnderPlant;
    bool aHasLilypad = false, aHasFlowerPot = false;
    if (!aUnderPlant || aUnderPlant->mOnBungeeState == PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
        aHasLilypad = false;
        aHasFlowerPot = false;
    } else {
        aHasLilypad = aUnderPlant->mSeedType == SeedType::SEED_LILYPAD;
        aHasFlowerPot = aUnderPlant->mSeedType == SeedType::SEED_FLOWERPOT;
    }
    // 部分情况下的格子中不能种植植物
    if (GetCraterAt(theGridX, theGridY)) {
        return PlantingReason::PLANTING_NOT_ON_CRATER;
    }
    if (GetScaryPotAt(theGridX, theGridY) || IsIceAt(theGridX, theGridY)) {
        return PlantingReason::PLANTING_NOT_HERE;
    }
    GridSquareType aGridSquare = mGridSquareType[theGridX][theGridY];
    if (aGridSquare == GridSquareType::GRIDSQUARE_DIRT || aGridSquare == GridSquareType::GRIDSQUARE_NONE) {
        return PlantingReason::PLANTING_NOT_HERE;
    }
    // 水生植物只能种在水上
    Plant *aNormalPlant = aPlantOnLawn.mNormalPlant;
    if (theSeedType == SeedType::SEED_LILYPAD || theSeedType == SeedType::SEED_TANGLEKELP || theSeedType == SeedType::SEED_SEASHROOM) {
        if (!IsPoolSquare(theGridX, theGridY)) {
            return PlantingReason::PLANTING_ONLY_IN_POOL;
        }

        return (aNormalPlant || aUnderPlant) ? PlantingReason::PLANTING_NOT_HERE : PlantingReason::PLANTING_OK;
    }
    if (Plant::IsFlying(theSeedType)) {
        return aPlantOnLawn.mFlyingPlant ? PlantingReason::PLANTING_NOT_HERE : PlantingReason::PLANTING_OK;
    }
    // 地刺/地刺王只能种在坚固的地面
    if (theSeedType == SeedType::SEED_SPIKEWEED || theSeedType == SeedType::SEED_SPIKEROCK) {
        if (aGridSquare == GridSquareType::GRIDSQUARE_POOL || StageHasRoof() || aUnderPlant) {
            return PlantingReason::PLANTING_NEEDS_GROUND;
        }
    }
    // 非水生植物不能种在水面上（南瓜头可以种在香蒲上）
    Plant *aPumpkinPlant = aPlantOnLawn.mPumpkinPlant;
    if (aGridSquare == GridSquareType::GRIDSQUARE_POOL && !aHasLilypad && theSeedType != SeedType::SEED_CATTAIL) {
        if (!aNormalPlant || aNormalPlant->mSeedType != SeedType::SEED_CATTAIL || theSeedType != SeedType::SEED_PUMPKINSHELL) {
            return PlantingReason::PLANTING_NOT_ON_WATER;
        }
    }
    // 花盆的种植条件
    if (theSeedType == SeedType::SEED_FLOWERPOT) {
        return (aNormalPlant || aUnderPlant || aPumpkinPlant) ? PlantingReason::PLANTING_NOT_HERE : PlantingReason::PLANTING_OK;
    }
    // 屋顶种植需要花盆
    if (StageHasRoof() && !aHasFlowerPot) {
        return PlantingReason::PLANTING_NEEDS_POT;
    }
    // 南瓜头的种植条件
    bool aAidPurchased = mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_FIRSTAID] > 0;
    if (theSeedType == SeedType::SEED_PUMPKINSHELL) {
        // 不可种植在玉米加农炮上
        if (aNormalPlant && aNormalPlant->mSeedType == SeedType::SEED_COBCANNON) {
            return PlantingReason::PLANTING_NOT_HERE;
        }
        // 无南瓜头时，可以种植南瓜头
        if (!aPumpkinPlant) {
            return PlantingReason::PLANTING_OK;
        }
        // 南瓜头的坚果包扎术
        if (aAidPurchased && aPumpkinPlant->mPlantHealth < aPumpkinPlant->mPlantMaxHealth * 2 / 3 && aPumpkinPlant->mSeedType == SeedType::SEED_PUMPKINSHELL
            && aPumpkinPlant->mOnBungeeState != PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
            return PlantingReason::PLANTING_OK;
        }

        return PlantingReason::PLANTING_NOT_HERE;
    }
    // 土豆地雷/潜伏芹菜只能种在陆地上
    if (aHasLilypad && (theSeedType == SeedType::SEED_POTATOMINE || theSeedType == SeedType::SEED_CELERY_STALKER)) {
        return PlantingReason::PLANTING_ONLY_ON_GROUND;
    }

    if (aUnderPlant) {
        // 香蒲对底端植物的紫卡升级
        if (theSeedType == SeedType::SEED_CATTAIL) {
            if (aNormalPlant) {
                return PlantingReason::PLANTING_NOT_HERE;
            }
            if (aUnderPlant->IsUpgradableTo(theSeedType) && aUnderPlant->mOnBungeeState != PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
                return PlantingReason::PLANTING_OK;
            }
            if (Plant::IsUpgrade(theSeedType)) {
                return PlantingReason::PLANTING_NEEDS_UPGRADE;
            }
        } else {
            // 模仿中的模仿者不可作为花盆或睡莲
            if (aUnderPlant->mSeedType == SeedType::SEED_IMITATER) {
                return PlantingReason::PLANTING_NOT_HERE;
            }
        }
    }

    // 一般紫卡植物的更迭判断
    if (aNormalPlant) {
        // 紫卡植物的升级
        if (aNormalPlant->IsUpgradableTo(theSeedType) && aNormalPlant->mOnBungeeState != PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
            return PlantingReason::PLANTING_OK;
        }
        if (Plant::IsUpgrade(theSeedType)) {
            return PlantingReason::PLANTING_NEEDS_UPGRADE;
        }

        // 坚果包扎术
        if (Plant::IsDefender(theSeedType) && theSeedType != SeedType::SEED_PUMPKINSHELL && aAidPurchased) {
            int aDamageThreshold = aNormalPlant->mSeedType == SeedType::SEED_PEANUT ? aNormalPlant->mPlantMaxHealth * 3 / 4 : aNormalPlant->mPlantMaxHealth * 2 / 3;
            if (aNormalPlant->mPlantHealth < aDamageThreshold && aNormalPlant->mSeedType == theSeedType && aNormalPlant->mOnBungeeState != PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
                return PlantingReason::PLANTING_OK;
            }
        }

        return PlantingReason::PLANTING_NOT_HERE;
    }

    // 免费种植模式下紫卡的额外判断
    if (!mApp->mEasyPlantingCheat && Plant::IsUpgrade(theSeedType)) {
        return PlantingReason::PLANTING_NEEDS_UPGRADE;
    }
    if (theSeedType == SeedType::SEED_COBCANNON && !IsValidCobCannonSpot(theGridX, theGridY)) {
        return PlantingReason::PLANTING_NEEDS_UPGRADE;
    } else if (theSeedType == SeedType::SEED_CATTAIL && aGridSquare != GridSquareType::GRIDSQUARE_POOL) {
        return PlantingReason::PLANTING_NOT_HERE;
    }

    return PlantingReason::PLANTING_OK;
}


bool Board::PlantingRequirementsMet(SeedType theSeedType) {
    // 紫卡直接种植！
    if (FreePlantAt && !IsOnlineServerModeActive() && !gIsReplayMode) {
        return true;
    }
    return old_Board_PlantingRequirementsMet(this, theSeedType);
}

void Board::ZombiesWon(Zombie *theZombie) {
    if (theZombie == nullptr) { // 如果是IZ或者僵尸水族馆，第二个参数是NULL，此时就返回原函数。否则闪退
        old_BoardZombiesWon(this, theZombie);
        return;
    }
    if (ZombieCanNotWon && !IsOnlineServerModeActive() && !gIsReplayMode) {
        theZombie->ApplyBurn();
        theZombie->DieNoLoot();
        return;
    }
    old_BoardZombiesWon(this, theZombie);
}

int Board::CountPlantByType(SeedType theSeedType) {
    int aCount = 0;
    Plant *aPlant = nullptr;
    while (IteratePlants(aPlant)) {
        if (aPlant->mSeedType == theSeedType) {
            aCount++;
        }
    }
    return aCount;
}

Plant *Board::AddPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex, bool theIsDoEffect) {
    Plant *aPlant = AddPlant_Origin(theGridX, theGridY, theSeedType, theImitaterType, thePlayerIndex, theIsDoEffect);

    if (mApp->mGameMode == GAMEMODE_MP_VS && mApp->mGameScene == SCENE_PLAYING) {
        if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
            return nullptr;

        if (gTcpClientSocket >= 0) {
            U16U16U16UNI32UNI32_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_PLANT_ADD;
            event.data1 = uint16_t(theGridX);
            event.data2 = uint16_t(theGridY);
            event.data3 = uint16_t(aPlant->mLaunchCounter);
            event.data4.u16x2.u16_1 = uint16_t(theSeedType);
            event.data4.u16x2.u16_2 = uint16_t(theImitaterType);
            event.data5.u16x2.u16_1 = uint16_t(mPlants.DataArrayGetID(aPlant));
            event.data5.u16x2.u16_2 = theIsDoEffect;
            netplay::PutEvent(event);

            //            aPlant->SyncPingPongAnimationToClient();
            aPlant->SyncAnimationToClient();
            // 植物放置数量
            netplay::MetricsRecordPlantUsed(int(theSeedType));
        }
    }

    return aPlant;
}

Plant *Board::AddPlant_Origin(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex, bool theIsDoEffect) {
    Plant *aPlant = NewPlant(theGridX, theGridY, theSeedType, theImitaterType, thePlayerIndex);

    if (theIsDoEffect) {
        DoPlantingEffects(theGridX, theGridY, aPlant);
    }
    mChallenge->PlantAdded(aPlant);

    // 检查成就！
    DoPlantingAchievementCheck(theSeedType);
    int aSunPlantsCount = CountPlantByType(SeedType::SEED_SUNFLOWER) + CountPlantByType(SeedType::SEED_SUNSHROOM);
    if (aSunPlantsCount > mMaxSunPlants) {
        mMaxSunPlants = aSunPlantsCount;
    }
    if (theSeedType == SeedType::SEED_CABBAGEPULT || theSeedType == SeedType::SEED_KERNELPULT || theSeedType == SeedType::SEED_MELONPULT || theSeedType == SeedType::SEED_WINTERMELON) {
        mCatapultPlantsUsed = true;
    }
    if (theSeedType == SeedType::SEED_PUMPKINSHELL && PumpkinWithLadder && !IsOnlineServerModeActive() && !gIsReplayMode && GetLadderAt(theGridX, theGridY) == nullptr) {
        AddALadder(theGridX, theGridY);
    }

    return aPlant;
}

bool Board::ZenGardenItemNumIsZero(CursorType theCursorType) const {
    // 消耗性工具的数量是否为0个
    switch (theCursorType) {
        case CursorType::CURSOR_TYPE_FERTILIZER:
            return mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_FERTILIZER] <= 1000;
        case CursorType::CURSOR_TYPE_BUG_SPRAY:
            return mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_BUG_SPRAY] <= 1000;
        case CursorType::CURSOR_TYPE_CHOCOLATE:
            return mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_CHOCOLATE] <= 1000;
        case CursorType::CURSOR_TYPE_TREE_FOOD:
            return mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_TREE_FOOD] <= 1000;
        default:
            return false;
    }
}

void Board::DrawZenButtons(Sexy::Graphics *g) {
    old_Board_DrawZenButtons(this, g);
}

void Board::DrawGameObjects(Graphics *g) {
    old_Board_DrawGameObjects(this, g);

    if (gOpeningEncounter && mApp->mGameScene == SCENE_PLAYING) {
        if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUN_RAIN && gOpeningEncounter->mDoEffect) {
            mChallenge->DrawWeather(g);
        }
    }
}

bool Board::KeyUp(Sexy::KeyCode theKey) {
    // 联机对战屏蔽按键，仅允许返回键
    bool isOnlineMode = (gTcpConnected || gTcpClientSocket >= 0);

    if (isOnlineMode) {
        return theKey == KEYCODE_BACK;
    }
    // 原版函数开始

    const unsigned int anEventFlags = mEvent != nullptr ? mEvent->mFlags : 0;

    if (mEvent != nullptr) {
        if (mApp->HasGamepad() || (mApp->mGamePad1IsOn && mApp->mGamePad2IsOn)) {

            Sexy::GamepadButton aButtonCode;
            int aPlayerIndex;
            unsigned int aButtonFlags;
            if (mApp->MapToButtonEvent(mEvent, aButtonCode, aPlayerIndex, aButtonFlags)) {
                GameButtonUp(aButtonCode, aPlayerIndex, aButtonFlags);
                return true;
            }
            return true;
        }
    }

    // 304 305
    if (theKey == KEYCODE_GAMEPAD_A || theKey == KEYCODE_GAMEPAD_B) {
        return false;
    }
    GamepadControls *aControls = mGamepadControls[0];
    if (isOnlineMode) {
        if (gTcpConnected) { // Guest
            aControls = mGamepadControls[0]->mGamepadIndex == 1 ? mGamepadControls[0] : mGamepadControls[1];
        } else if (gTcpClientSocket >= 0) { // Host
            aControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
        }
    }
    if (aControls->GetVTable()->OnKeyUp(aControls, theKey, anEventFlags)) {
        return true;
    }

    if (theKey == KEYCODE_ESCAPE && (anEventFlags & 0x80u) == 0) {
        mApp->DoConfirmBackToMain(NeedSaveGame());
        return true;
    }
    return true;
}

bool Board::KeyDown(KeyCode theKey) {
    // 联机对战屏蔽按键，仅允许返回键
    bool isOnlineMode = (gTcpConnected || gTcpClientSocket >= 0);
    if (isOnlineMode) {
        return theKey == KEYCODE_BACK;
    }
    // 用于切换键盘模式，自动开关砸罐子老虎机种子雨关卡内的"自动拾取植物卡片"功能
    if (theKey >= 37 && theKey <= 40) {
        if (!gKeyboardMode) {
            patchlist::autoPickupSeedPacketDisable.Restore();
        }
        gKeyboardMode = true;
        requestDrawShovelInCursor = false;
    }

    // 原版函数开始

    /*
     * 手柄模式处理。
     */
    const unsigned int anEventFlags = mEvent != nullptr ? mEvent->mFlags : 0;
    bool skipInitialKeyProcessing = false;

    if (mEvent != nullptr) {

        if (mApp->HasGamepad() || (mApp->mGamePad1IsOn && mApp->mGamePad2IsOn)) {
            if (theKey == KEYCODE_ESCAPE) {
                mApp->DoConfirmBackToMain(NeedSaveGame());
                return true;
            }

            Sexy::GamepadButton aButtonCode;
            int aPlayerIndex;
            unsigned int aButtonFlags;
            if (mApp->MapToButtonEvent(mEvent, aButtonCode, aPlayerIndex, aButtonFlags)) {
                GameButtonDown(aButtonCode, aPlayerIndex, aButtonFlags);
                return true;
            }

            skipInitialKeyProcessing = true;
        }
    }

    if (!skipInitialKeyProcessing) {
        /*
         * 忽略键值 304 和 305。
         */
        if (theKey == KEYCODE_GAMEPAD_A || theKey == KEYCODE_GAMEPAD_B) {
            return false;
        }

        /*
         * Crazy Dave 帮助界面处理。
         */
        //        if (mApp->mCrazyDaveState != CRAZY_DAVE_OFF) {
        //
        //            const bool aDaveHelpResult = GetVTable()->HasWidget(this, mApp->mDaveHelp);
        //
        //            if (aDaveHelpResult) {
        //                if (mApp->mCrazyDaveState == CRAZY_DAVE_ENTERING || mApp->mCrazyDaveState == CRAZY_DAVE_LEAVING) {
        //                    return true;
        //                }
        //
        //                mApp->mDaveHelp->DealClick(theKey);
        //                return true;
        //            }
        //        }

        /*
         * 关卡开场动画优先处理按键。
         */
        if (mApp->mGameScene == SCENE_LEVEL_INTRO && mCutScene->OnKeyDown(theKey, anEventFlags)) {
            return true;
        }
        GamepadControls *aControls = mGamepadControls[0];
        if (isOnlineMode) {
            if (gTcpConnected) { // Guest
                aControls = mGamepadControls[0]->mGamepadIndex == 1 ? mGamepadControls[0] : mGamepadControls[1];
            } else if (gTcpClientSocket >= 0) { // Host
                aControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            }
        }


        if (aControls->GetVTable()->OnKeyDown(aControls, theKey, anEventFlags)) {
            return true;
        }

        if (theKey == KEYCODE_ESCAPE && (anEventFlags & 0x80u) == 0) {
            mApp->DoConfirmBackToMain(NeedSaveGame());
            return true;
        }

        if (theKey == KEYCODE_RETURN) {

            if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN) {
                if (mApp->mCrazyDaveMessageIndex != -1) {
                    if (!mApp->GetDialog(DIALOG_STORE) && !mApp->GetDialog(DIALOG_MAIL)) {
                        mApp->mZenGarden->AdvanceCrazyDaveDialog();
                        return true;
                    }
                }
            } else if (mApp->mCrazyDaveMessageIndex != -1 && (mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM || IsScaryPotterDaveTalking())) {
                mChallenge->AdvanceCrazyDaveDialog();
                return true;
            }
        }
    }

    /*
     * 通用按键处理。
     * 对应原来的 LABEL_5 和 LABEL_6。
     */

    if (mApp->mGameScene == SCENE_LEVEL_INTRO) {

        if (mApp->mGameMode != GAMEMODE_CHALLENGE_ZEN_GARDEN && mApp->mGameMode != GAMEMODE_TREE_OF_WISDOM) {
            mCutScene->KeyDown(theKey);
            return true;
        }
    }

    if (theKey == KEYCODE_RETURN || theKey == KEYCODE_SPACE) {

        if (IsScaryPotterDaveTalking() && mApp->mCrazyDaveMessageIndex != -1) {
            mChallenge->AdvanceCrazyDaveDialog();
            return true;
        }

        if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN || mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM) {
            mApp->mZenGarden->AdvanceCrazyDaveDialog();
            return true;
        }

        if (theKey == KEYCODE_SPACE) {

            /*
             * 即使当前不能暂停，原函数仍返回 true，
             * 表示空格键已经被消费。
             */
            if (!mApp->CanPauseNow())
                return true;

            mApp->PlaySample(Sexy::SOUND_PAUSE);
            mApp->DoPauseDialog();
            return true;
        }
    }

    if (theKey == KEYCODE_RIGHT) {
        gStepReady = true; // mApp->StepAdvance();
        return true;
    }

    return false;
}

void Board::GameButtonUp(GamepadButton theButton, int thePlayerIndex, unsigned int theFlags) {
    // 作弊菜单已由修改器实现，故移除实现呼出 DoCheatCodeDialog 的部分
    for (GamepadControls *controls : mGamepadControls) {
        if (controls != nullptr && controls->mPlayerIndex == thePlayerIndex) {
            controls->OnButtonUp(theButton, thePlayerIndex, theFlags);
        }
    }
}

Coin *Board::AddCoin(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion) {
    if (gTcpClientSocket >= 0) {
        U8U8U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_COIN_ADD}, uint8_t(theCoinType), uint8_t(theCoinMotion), uint16_t(theX), uint16_t(theY)};
        netplay::PutEvent(event);
    }

    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode) {
        return nullptr;
    }

    return old_Board_AddCoin(this, theX, theY, theCoinType, theCoinMotion);
}

void Board::UpdateSunSpawning() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 如果开了高级暂停
        return;
    }
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN) {
        return;
    }

    if (mApp->IsVSMode() && (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)) {
        return;
    }

    if (StageIsNight() || HasLevelAwardDropped() || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ICE
        || mApp->mGameMode == GameMode::GAMEMODE_UPSELL || mApp->mGameMode == GameMode::GAMEMODE_INTRO || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM
        || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND
        || mApp->IsIZombieLevel() || mApp->IsScaryPotterLevel() || mApp->IsSquirrelLevel() || HasConveyorBeltSeedBank(0) || mTutorialState == TutorialState::TUTORIAL_SLOT_MACHINE_PULL)
        return;

    if (mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER || mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PLANT_PEASHOOTER) {
        if (mPlants.mSize == 0) {
            return;
        }
    }

    mSunCountDown--;
    if (mSunCountDown != 0)
        return;

    mNumSunsFallen++;
    mSunCountDown = std::min(SUN_COUNTDOWN_MAX, SUN_COUNTDOWN + mNumSunsFallen * 10) + Rand(SUN_COUNTDOWN_RANGE);
    CoinType aSunType = mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SUNNY_DAY ? CoinType::COIN_LARGESUN : CoinType::COIN_SUN;
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON) {
        mSunCountDown = 2 * mSunCountDown / 3;
    }
    if (mApp->IsCoopMode() && (Sexy::Rand(2) == 0)) { // 结盟模式 50% 概率掉落双倍阳光
        aSunType = CoinType::COIN_COOP_DOUBLE_SUN;
    }
    if (mApp->IsVSMode()) {
        if (mChallenge->IsMPSuddenDeath()) {
            mSunCountDown /= 3;
        }

        if (mSeedBank[0]->mNumPackets == 7) {
            mSunCountDown *= 0.8f; // 多槽模式自然掉落的阳光产能提高至 1.25 倍
        }

        aSunType = CoinType::COIN_SUN;
        CoinType aBrainType = CoinType::COIN_VS_ZOMBIE_BRAIN;
        if (gOpeningEncounter) {
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUN_RAIN && gOpeningEncounter->mDoEffect) { // 刷牌模式阳光雨
                mSunCountDown /= 10;
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUNNY_DAY) { // 刷牌模式晴天
                aSunType = CoinType::COIN_LARGESUN;
                aBrainType = CoinType::COIN_LARGE_VS_ZOMBIE_BRAIN;
            }
        }

        int aSpawnCount = 1;
        if (mChallenge->IsMPSuddenDeath()) {
            aSpawnCount = (Challenge::gVSSuddenDeathMode == 1) ? 2 : 1;
        }

        for (int aSpawnIndex = 0; aSpawnIndex < aSpawnCount; ++aSpawnIndex) {
            AddCoin(RandRangeInt(100, 499), 60, aSunType, CoinMotion::COIN_MOTION_FROM_SKY);
            AddCoin(RandRangeInt(500, 799), 60, aBrainType, CoinMotion::COIN_MOTION_FROM_SKY);
        }
    } else {
        AddCoin(RandRangeInt(100, 649), 60, aSunType, CoinMotion::COIN_MOTION_FROM_SKY);
    }
}

void Board::UpdateZombieSpawning() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 如果开了高级暂停
        return;
    }

    // 在黄油爆米花关卡改变出怪倒计时。
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN) {
        if (mZombieCountDown >= 2500 && mZombieCountDown <= 3100) {
            mZombieCountDown = 750;
            mZombieCountDownStart = mZombieCountDown;
        }
    }
    // int *lawnApp = (int *) this[69];
    // GameMode::GameMode mGameMode = (GameMode::GameMode)*(lawnApp + LAWNAPP_GAMEMODE_OFFSET);
    // if(mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN){
    // int mFinalWaveSoundCounter = this[5660];
    // if (mFinalWaveSoundCounter > 0) {
    // mFinalWaveSoundCounter--;
    // this[5660] = mFinalWaveSoundCounter;
    // if (mFinalWaveSoundCounter == 0) {
    // mApp->PlaySample(Sexy_SOUND_FINALWAVE_Addr);
    // }
    // }
    // if (Board_HasLevelAwardDropped(this)) {
    // return;
    // }
    //
    // int mRiseFromGraveCounter = this[5540];
    // if (mRiseFromGraveCounter > 0) {
    // mRiseFromGraveCounter--;
    // this[5540] = mRiseFromGraveCounter;
    // if (mRiseFromGraveCounter == 0) {
    // Board_SpawnZombiesFromGraves(this);
    // }
    // }
    //
    // int mHugeWaveCountDown = this[5552];
    // if (mHugeWaveCountDown > 0) {
    // mHugeWaveCountDown--;
    // this[5552] = mHugeWaveCountDown;
    // if (mHugeWaveCountDown == 0) {
    // Board_ClearAdvice(this, 42);
    // Board_NextWaveComing(this);
    // this[5550] = 1; //  mZombieCountDown = 1;
    // }else if(mHugeWaveCountDown == 725){
    // mApp->PlaySample(Sexy_SOUND_FINALWAVE_Addr);
    // }
    // }
    //
    // int mZombieCountDown = this[5550];
    // mZombieCountDown--; //  mZombieCountDown--;
    // this[5550] = mZombieCountDown;
    //
    // int mZombieCountDownStart = this[5551];
    // int mCurrentWave = this[5542];
    // int mZombieHealthToNextWave = this[5548];
    // int num2 = mZombieCountDownStart - mZombieCountDown;
    // if (mZombieCountDown > 5 && num2 > 400) {
    // int num3 = Board_TotalZombiesHealthInWave(this, mCurrentWave - 1);
    // if (num3 <= mZombieHealthToNextWave && mZombieCountDown > 200) {
    // this[5550] = 200;//  mZombieCountDown = 200;
    // }
    // }
    //
    // if (mZombieCountDown == 5) {
    // if (IsFlagWave(this, mCurrentWave)) {
    // Board_ClearAdviceImmediately(this);
    // int holder[1];
    // StrFormat(holder,"[ADVICE_HUGE_WAVE]");
    // Board_DisplayAdviceAgain(this, holder, 15, 42);
    // StringDelete(holder);
    // mHugeWaveCountDown = 750;
    // return;
    // }
    // Board_NextWaveComing(this);
    // }
    //
    // if (mZombieCountDown != 0) {
    // return;
    // }
    // Board_SpawnZombieWave(this);
    // this[5549] = Board_TotalZombiesHealthInWave(this, mCurrentWave -1);
    // //mZombieHealthWaveStart = Board_TotalZombiesHealthInWave(this,mCurrentWave - 1);
    // if (IsFlagWave(this, mCurrentWave)) {
    // this[5548] = 0;//  mZombieHealthToNextWave = 0;
    // this[5550] = 0;//  mZombieCountDown = 4500;
    // } else {
    // this[5548] = (int) (RandRangeFloat(0.5f, 0.65f) * this[5549]);
    // this[5550] = 750;//   mZombieCountDown = 750;
    // }
    // this[5551] = mZombieCountDown;
    // return;
    // }
    old_Board_UpdateZombieSpawning(this);
}

void Board::UpdateIce() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 如果开了高级暂停
        return;
    }

    old_Board_UpdateIce(this);
}

void Board::DrawCoverLayer(Sexy::Graphics *g, int theRow) {
    if (mBackground < BackgroundType::BACKGROUND_1_DAY || hideCoverLayer) {
        // 如果背景非法，或玩家“隐藏草丛和电线杆”，则终止绘制函数
        return;
    }

    if (mBackground <= BackgroundType::BACKGROUND_4_FOG) {
        // 如果是前院(0 1)或者泳池(2 3)，则绘制草丛。整个草丛都是动画而非贴图，没有僵尸来的时候草丛会保持在动画第一帧。
        Reanimation *aReanim = mApp->ReanimationTryToGet(mCoverLayerAnimIDs[theRow]);
        if (aReanim != nullptr) {
            (aReanim)->Draw(g);
        }
    }
    if (theRow == 6) {
        // 绘制栏杆和电线杆
        switch (mBackground) {
            case BackgroundType::BACKGROUND_1_DAY: // 前院白天
                if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON) {
                    // 在重型武器关卡中不绘制栏杆。
                    return;
                }
                g->DrawImage(Sexy::IMAGE_BACKGROUND1_COVER, 684, 557);
                break;
            case BackgroundType::BACKGROUND_2_NIGHT: // 前院夜晚
                g->DrawImage(Sexy::IMAGE_BACKGROUND2_COVER, 684, 557);
                break;
            case BackgroundType::BACKGROUND_3_POOL: // 泳池白天
                g->DrawImage(Sexy::IMAGE_BACKGROUND3_COVER, 671, 613);
                break;
            case BackgroundType::BACKGROUND_4_FOG: // 泳池夜晚
                g->DrawImage(Sexy::IMAGE_BACKGROUND4_COVER, 672, 613);
                break;
            case BackgroundType::BACKGROUND_5_ROOF: // 屋顶白天
                g->DrawImage(Sexy::IMAGE_ROOF_TREE, mOffsetMoved + mOffsetMoved / 2 + 628, -60);
                // 对战模式电线杆会遮挡僵尸卡槽，严重影响游戏体验
                g->DrawImage(Sexy::IMAGE_ROOF_POLE, mOffsetMoved * 2 + 628 + (mApp->IsVSMode() ? 65 : 0), -60);
                break;
            case BackgroundType::BACKGROUND_6_BOSS:
                // 可在此处添加代码绘制月夜电线杆喔
                // if(LawnApp_IsFinalBossLevel(mApp))  return;

                g->DrawImage(addonImages.trees_night, mOffsetMoved + mOffsetMoved / 2 + 628, -60);
                g->DrawImage(addonImages.pole_night, mOffsetMoved * 2 + 628, -60);
                break;
            default:
                return;
        }
    }
}

void Board::PickBackground() {
    // 用于控制关卡的场地选取。可选择以下场地：前院白天/夜晚，泳池白天/夜晚，屋顶白天/夜晚
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN) {
        mBackground = BackgroundType::BACKGROUND_3_POOL;
        LoadBackgroundImages();
        mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[2] = PlantRowType::PLANTROW_POOL;
        mPlantRow[3] = PlantRowType::PLANTROW_POOL;
        mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[5] = PlantRowType::PLANTROW_NORMAL;
        InitCoverLayer();
        SetGrids();
    } else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_POOL_PARTY) {
        mBackground = BackgroundType::BACKGROUND_3_POOL;
        LoadBackgroundImages();
        mPlantRow[0] = PlantRowType::PLANTROW_POOL;
        mPlantRow[1] = PlantRowType::PLANTROW_POOL;
        mPlantRow[2] = PlantRowType::PLANTROW_POOL;
        mPlantRow[3] = PlantRowType::PLANTROW_POOL;
        mPlantRow[4] = PlantRowType::PLANTROW_POOL;
        mPlantRow[5] = PlantRowType::PLANTROW_NORMAL;
        InitCoverLayer();
        SetGrids();
    } else if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        mBackground = gVSBackground;
        LoadBackgroundImages();
        mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
        mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
        if (StageHas6Rows()) {
            mPlantRow[2] = PlantRowType::PLANTROW_POOL;
            mPlantRow[3] = PlantRowType::PLANTROW_POOL;
            mPlantRow[5] = PlantRowType::PLANTROW_NORMAL;
        }
        InitCoverLayer();
        SetGrids();
    } else {
        switch (VSBackGround) {
            case 1:
                mBackground = BackgroundType::BACKGROUND_1_DAY;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
                InitCoverLayer();
                SetGrids();
                break;
            case 2:
                mBackground = BackgroundType::BACKGROUND_2_NIGHT;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
                InitCoverLayer();
                SetGrids();
                break;
            case 3:
                mBackground = BackgroundType::BACKGROUND_3_POOL;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_POOL;
                mPlantRow[3] = PlantRowType::PLANTROW_POOL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_NORMAL;
                InitCoverLayer();
                SetGrids();
                break;
            case 4:
                mBackground = BackgroundType::BACKGROUND_4_FOG;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_POOL;
                mPlantRow[3] = PlantRowType::PLANTROW_POOL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_NORMAL;
                InitCoverLayer();
                SetGrids();
                break;
            case 5:
                mBackground = BackgroundType::BACKGROUND_5_ROOF;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
                InitCoverLayer();
                SetGrids();
                break;
            case 6:
                mBackground = BackgroundType::BACKGROUND_6_BOSS;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
                InitCoverLayer();
                SetGrids();
                break;
            case 7:
                mBackground = BackgroundType::BACKGROUND_GREENHOUSE;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
                InitCoverLayer();
                SetGrids();
                break;
            case 8:
                mBackground = BackgroundType::BACKGROUND_MUSHROOM_GARDEN;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[3] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_DIRT;
                InitCoverLayer();
                SetGrids();
                break;
            case 9:
                mBackground = BackgroundType::BACKGROUND_ZOMBIQUARIUM;
                LoadBackgroundImages();
                mPlantRow[0] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[1] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[2] = PlantRowType::PLANTROW_POOL;
                mPlantRow[3] = PlantRowType::PLANTROW_POOL;
                mPlantRow[4] = PlantRowType::PLANTROW_NORMAL;
                mPlantRow[5] = PlantRowType::PLANTROW_NORMAL;
                InitCoverLayer();
                SetGrids();
                break;
            default:
                old_Board_PickBackground(this);
        }
    }
}

bool Board::StageIsNight() const {
    // 关系到天上阳光掉落与否。
    return mBackground == BackgroundType::BACKGROUND_2_NIGHT || mBackground == BackgroundType::BACKGROUND_4_FOG || mBackground == BackgroundType::BACKGROUND_MUSHROOM_GARDEN
        || mBackground == BackgroundType::BACKGROUND_6_BOSS;
}

bool Board::StageHasPool() const {
    // 关系到泳池特有的僵尸，如救生圈僵尸、海豚僵尸、潜水僵尸在本关出现与否。此处我们添加水族馆场景。
    return (mBackground == BackgroundType::BACKGROUND_ZOMBIQUARIUM && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN)
        || mBackground == BackgroundType::BACKGROUND_3_POOL || mBackground == BackgroundType::BACKGROUND_4_FOG;
}

bool Board::StageHasRoof() const {
    return (mBackground == BackgroundType::BACKGROUND_5_ROOF || mBackground == BackgroundType::BACKGROUND_6_BOSS);
}

bool Board::StageHas6Rows() const {
    // 关系到第六路可否操控（比如种植植物）。
    return mBackground == BackgroundType::BACKGROUND_3_POOL || mBackground == BackgroundType::BACKGROUND_4_FOG;
}

void Board::UpdateFwoosh() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        return;
    }

    old_Board_UpdateFwoosh(this);
}

void Board::UpdateFog() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        return;
    }

    old_Board_UpdateFog(this);
}

void Board::DrawFog(Sexy::Graphics *g) {
    if (noFog && !IsOnlineServerModeActive() && !gIsReplayMode) {
        return;
    }

    if (mApp->IsVSMode()) {
        // 对战模式只在中间三列绘制
        Image *aImageFog = mApp->Is3DAccelerated() ? Sexy::IMAGE_FOG : Sexy::IMAGE_FOG_SOFTWARE;
        bool aVsCenterFogOnly = mApp->IsVSMode();
        for (int x = 0; x < MAX_GRID_SIZE_X; x++) {
            if (aVsCenterFogOnly && (x < 3 || x > 5)) {
                continue;
            }

            for (int y = 0; y < MAX_GRID_SIZE_Y + 1; y++) {
                int aSampleX = x;
                int aFadeAmount = mGridCelFog[aSampleX][y];
                if (aVsCenterFogOnly) {
                    int aCenterSampleX = MAX_GRID_SIZE_X / 2;
                    int aLeftSampleX = aCenterSampleX - 1;
                    int aRightSampleX = aCenterSampleX + 1;
                    aFadeAmount = mGridCelFog[aCenterSampleX][y];
                    if (aLeftSampleX >= 0 && mGridCelFog[aLeftSampleX][y] > aFadeAmount) {
                        aFadeAmount = mGridCelFog[aLeftSampleX][y];
                    }
                    if (aRightSampleX < MAX_GRID_SIZE_X && mGridCelFog[aRightSampleX][y] > aFadeAmount) {
                        aFadeAmount = mGridCelFog[aRightSampleX][y];
                    }
                    aSampleX = aCenterSampleX;
                }
                if (aFadeAmount == 0) {
                    continue;
                }

                int aCelLook = mGridCelLook[aSampleX][y % MAX_GRID_SIZE_Y];
                int aCelCol = aCelLook % 8;
                float aPosX = x * 80 + mFogOffset - 15;
                float aPosY = y * 85 + 20;
                auto aTime = static_cast<float>(mMainCounter * std::numbers::pi * 2.0);
                auto aPhaseX = static_cast<float>(6.0 * std::numbers::pi * x / MAX_GRID_SIZE_X);
                auto aPhaseY = static_cast<float>(6.0 * std::numbers::pi * y / (MAX_GRID_SIZE_Y + 1));
                float aMotion = 13 + 4 * sin(aTime / 900 + aPhaseY) + 8 * sin(aTime / 500 + aPhaseX);

                int aColorVariant = 255 - aCelLook * 1.5f - aMotion * 1.5f;
                int aLightnessVariant = 255 - aCelLook - aMotion;
                if (!mApp->Is3DAccelerated()) {
                    aPosX += 10;
                    aPosY += 3;
                    aCelCol = aCelLook % Sexy::IMAGE_FOG_SOFTWARE->mNumCols;
                    aColorVariant = 255;
                    aLightnessVariant = 255;
                }

                g->SetColorizeImages(true);
                g->SetColor(Color(aColorVariant, aColorVariant, aLightnessVariant, aFadeAmount));
                g->DrawImageCel(aImageFog, aPosX, aPosY, aCelCol, 0);
                if (x == MAX_GRID_SIZE_X - 1) {
                    g->DrawImageCel(aImageFog, aPosX + 80, aPosY, aCelCol, 0);
                }
                g->SetColorizeImages(false);
            }
        }
        return;
    }

    // TODO: 让冒险模式选卡阶段的迷雾后退一些

    old_Board_DrawFog(this, g);
}

Zombie *Board::AddZombieInRow(ZombieType theZombieType, int theRow, int theFromWave, bool theIsRustle) {

    //    if (theZombieType == ZOMBIE_BOBSLED && (gTcpConnected || gTcpClientSocket >= 0)) {
    //
    //        // 不允许客户端通过AddZombieInRow来添加雪橇小队僵尸
    //        if (gTcpConnected)
    //            return nullptr;
    //
    //
    //        bool theUnkBool = Sexy::Rand(5) == 0;
    //        Zombie *bobsledZombies[4];
    //        bobsledZombies[0] = mZombies.DataArrayAlloc();
    //        bobsledZombies[0]->ZombieInitialize(theRow, theZombieType, theUnkBool, nullptr, theFromWave, true);
    //
    //        for (int i = 1; i < 4; ++i) {
    //            bobsledZombies[i] = mZombies.DataArrayAlloc();
    //            bobsledZombies[i]->ZombieInitialize(theRow, theZombieType, theUnkBool, bobsledZombies[0], theFromWave, true);
    //        }
    //
    //
    //        if (gTcpClientSocket >= 0) {
    //
    //            U8x2U16x4UNI32x8_Event event{};
    //            event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_BOBSELD_ADD;
    //            event.data1[0] = uint8_t(theRow);
    //            event.data1[1] = int8_t(theFromWave);
    //
    //            for (int i = 0; i < 4; ++i) {
    //                event.data3[i] = mZombies.DataArrayGetID(bobsledZombies[i]);
    //                event.data3[i].f32 = bobsledZombies[i]->mVelX;
    //                event.data5[i].f32 = bobsledZombies[i]->mPosX;
    //            }
    //            netplay::PutEvent(event);
    //        }
    //        return bobsledZombies[0];
    //    }


    Zombie *aZombie = AddZombieInRow_Origin(theZombieType, theRow, theFromWave, theIsRustle);

    if (mApp->IsVSMode() && mApp->mGameScene == SCENE_PLAYING) {
        if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
            return nullptr;

        if (gTcpClientSocket >= 0) {
            if (theZombieType == ZombieType::ZOMBIE_BUNGEE) {
                if (theFromWave == 0) {
                    // theFromWave == 0代表是偷植物的蹦极
                    GamepadControls *aGamepad = mGamepadControls[0]->mIsZombie ? mGamepadControls[0] : mGamepadControls[1];
                    int aTargetCol = PixelToGridXKeepOnBoard(aGamepad->mCursorPositionX, aGamepad->mCursorPositionY);
                    U16UNI32UNI32_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_SET_STEAL_GRID;
                    event.data1 = uint16_t(mZombies.DataArrayGetID(aZombie));
                    event.data2.u8x4.u8_1 = uint8_t(aTargetCol);
                    event.data2.u8x4.u8_2 = uint8_t(theRow);
                    event.data3.f32 = aZombie->mAltitude;
                    netplay::PutEvent(event);
                } else {
                    // theFromWave == -5代表是放僵尸的蹦极，在屋顶模式专属。
                    // 此处我们直接不处理，由专门的EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_DROP_ZOMBIE事件处理
                }
            } else if (theZombieType == ZombieType::ZOMBIE_BOBSLED) {
                U8x2U16x4UNI32x8_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_BOBSELD_ADD;
                event.data1[0] = uint8_t(theRow);
                event.data1[1] = int8_t(theFromWave);
                event.data2[0] = mZombies.DataArrayGetID(aZombie);
                event.data3[0].f32 = aZombie->mVelX;
                event.data4[0].f32 = aZombie->mPosX;

                for (int i = 0; i < 3; ++i) {
                    Zombie *aFollowerZombie = mZombies.DataArrayGet(aZombie->mFollowerZombieID[i]);
                    event.data2[i + 1] = aZombie->mFollowerZombieID[i];
                    event.data3[i + 1].f32 = aFollowerZombie->mVelX;
                    event.data4[i + 1].f32 = aFollowerZombie->mPosX;
                }
                netplay::PutEvent(event);
            } else if (theZombieType == ZombieType::ZOMBIE_DOGWALKER) {
                Zombie *aZombieDog = mZombies.DataArrayGet(aZombie->mRelatedZombieID);
                if (aZombieDog != nullptr) {
                    U8x2U16x4UNI32x8_Event event{};
                    event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_DOGWALKER_ADD;
                    event.data1[0] = uint8_t(theRow);
                    event.data1[1] = int8_t(theFromWave);
                    event.data2[0] = uint16_t(mZombies.DataArrayGetID(aZombie));
                    event.data2[1] = uint16_t(mZombies.DataArrayGetID(aZombieDog));
                    event.data2[2] = uint16_t(theIsRustle);
                    event.data3[0].f32 = aZombie->mVelX;
                    event.data3[1].f32 = aZombieDog->mVelX;
                    event.data4[0].f32 = aZombie->mPosX;
                    event.data4[1].f32 = aZombieDog->mPosX;
                    netplay::PutEvent(event);
                }
            } else {
                U8x5U16UNI32x2_Event event{};
                event.type = EventType::EVENT_SERVER_BOARD_ZOMBIE_ADD;
                event.data1[0] = uint8_t(theZombieType);
                event.data1[1] = uint8_t(theRow);
                event.data1[2] = int8_t(theFromWave);
                event.data1[3] = uint8_t(theIsRustle);
                if (theZombieType == ZombieType::ZOMBIE_NORMAL && !aZombie->mInPool) {
                    event.data1[4] = aZombie->mBloated;
                }

                event.data2 = uint16_t(mZombies.DataArrayGetID(aZombie));
                event.data3[0].f32 = aZombie->mVelX;
                event.data3[1].f32 = aZombie->mPosX;
                netplay::PutEvent(event);
            }
            // 僵尸放置数量
            netplay::MetricsRecordZombieUsed(int(theZombieType));
        }
        return aZombie;
    }

    return aZombie;
}

Zombie *Board::AddZombieInRow_Origin(ZombieType theZombieType, int theRow, int theFromWave, bool theIsRustle) {
    // 修复蹦极僵尸出现时草丛也会摇晃
    if (theZombieType == ZombieType::ZOMBIE_BUNGEE) {
        theIsRustle = false;
    }
    Zombie *aZombie = old_Board_AddZombieInRow(this, theZombieType, theRow, theFromWave, theIsRustle);
    if (theZombieType == ZombieType::ZOMBIE_DOGWALKER) {
        Zombie *aZombieDog = AddZombieInRow_Origin(ZombieType::ZOMBIE_DOG, aZombie->mRow, aZombie->mFromWave, false);
        if (aZombieDog != nullptr) {
            aZombieDog->mPosX = aZombie->mPosX + (aZombie->IsWalkingBackwards() ? 80.0f : -80.0f);
            aZombieDog->mRelatedZombieID = ZombieGetID(aZombie);
            aZombieDog->mRenderOrder = aZombie->mRenderOrder + 1;
            aZombie->mRelatedZombieID = ZombieGetID(aZombieDog);
        }
    }
    return aZombie;
}

Zombie *Board::AddZombie(ZombieType theZombieType, int theFromWave, bool theIsRustle) {
    //    return AddZombie_Origin(theZombieType, theFromWave, theIsRustle);
    return AddZombieInRow(theZombieType, PickRowForNewZombie(theZombieType), theFromWave, theIsRustle);
}

Zombie *Board::AddZombie_Origin(ZombieType theZombieType, int theFromWave, bool theIsRustle) {
    return AddZombieInRow_Origin(theZombieType, PickRowForNewZombie(theZombieType), theFromWave, theIsRustle);
}

// void (*old_Board_UpdateCoverLayer)(Board *this);
//
// void Board_UpdateCoverLayer(Board *this) {
// if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
// return;
// }
// old_Board_UpdateCoverLayer(this);
// }

void Board::SpeedUpUpdate() {
    UpdateGridItems();
    UpdateFwoosh();
    UpdateGame();
    UpdateFog();
    // Board_UpdateCoverLayer(this);
    mChallenge->Update();
}


void Board::processClientEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_CLIENT_BOARD_TOUCH_DOWN: {
            auto *event1 = static_cast<const I16I16_Event *>(event);
            if (gIsServerModeSpectator || gIsReplayMode) {
                GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
                SeedBank *clientSeedBank = clientGamepadControls ? clientGamepadControls->GetSeedBank() : nullptr;
                bool inClientSeedBank = clientSeedBank != nullptr && clientSeedBank->ContainsPoint(event1->data1, event1->data2);
                ClientMouseDownLocal(event1->data1, event1->data2, inClientSeedBank);
                break;
            }
            MouseDownSecond(event1->data1, event1->data2, 0);
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            U8U8I16I16_Event eventReply = {{EventType::EVENT_BOARD_TOUCH_DOWN_REPLY},
                                           uint8_t(clientGamepadControls->mSelectedSeedIndex),
                                           uint8_t(clientGamepadControls->mGamepadState),
                                           int16_t(clientGamepadControls->mCursorPositionX),
                                           int16_t(clientGamepadControls->mCursorPositionY)};
            netplay::PutEvent(eventReply);
        } break;
        case EVENT_CLIENT_BOARD_TOUCH_DRAG: {
            auto *event1 = static_cast<const I16I16_Event *>(event);
            if (gIsServerModeSpectator || gIsReplayMode) {
                ClientMouseDragLocal(event1->data1, event1->data2);
                break;
            }
            MouseDragSecond(event1->data1, event1->data2);
            //            GamepadControls *clientGamepadControls = mGamepadControls[1]->mGamepadIndex == 1 ? mGamepadControls[1] : mGamepadControls[0];
            //            I16I16_Event eventReply = {{EventType::EVENT_BOARD_TOUCH_DRAG_REPLY}, int16_t(clientGamepadControls->mCursorPositionX), int16_t(clientGamepadControls->mCursorPositionY)};
            //            netplay::PutEvent(gTcpClientSocket, eventReply);
        } break;
        case EVENT_CLIENT_BOARD_TOUCH_UP: {
            auto *event1 = static_cast<const I16I16_Event *>(event);
            if (gIsServerModeSpectator || gIsReplayMode) {
                ClientMouseUpLocal(event1->data1, event1->data2);
                break;
            }
            MouseUpSecond(event1->data1, event1->data2, 0);
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            CursorObject *clientCursorObject = mGamepadControls[1]->mGamepadIndex == 1 ? mCursorObject[1] : mCursorObject[0];
            U8U8_Event eventReply = {{EventType::EVENT_BOARD_TOUCH_UP_REPLY}, uint8_t(clientGamepadControls->mGamepadState), uint8_t(clientCursorObject->mCursorType)};
            netplay::PutEvent(eventReply);
        } break;
        case EVENT_CLIENT_BOARD_PAUSE: {
            auto *event1 = static_cast<const U8_Event *>(event);
            if (mApp->mGameScene == SCENE_PLAYING) { // 在对局结束后，不接收对方暂停的信息
                PauseFromSecondPlayer(event1->data);
            }
        } break;
        case EVENT_CLIENT_BOARD_CONCEDE: {
            mApp->KillNewOptionsDialog();
            mApp->KillDialog(DIALOG_CONFIRM_IN_GAME_RESTART);
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            if (clientGamepadControls->mIsZombie) {
                mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_PLANT_WON);
                mApp->mGameScene = SCENE_PLANTS_WON;
            } else {
                mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_ZOMBIE_WON);
                mApp->mGameScene = SCENE_ZOMBIES_WON;
            }
            if (mApp->IsVSMode() && gTcpClientSocket >= 0) {
                const bool plantWin = (mApp->mGameScene == SCENE_PLANTS_WON);
                netplay::MetricsSetVsBackground(int(gVSBackground));
                netplay::MetricsSetShuffleMode(Challenge::msVSShuffleMode);
                netplay::MetricsSendSettlement(plantWin, mMainCounter);
            }

            mApp->ShowVSResultsScreen();
            mApp->mVSResultsMenu->InitFromBoard(this);
            mApp->KillBoard();
        } break;
        default:
            break;
    }
}


void Board::processServerEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_LOCAL_BOARD_ACTION:
            vsai::ExecuteReplayAction(this, *static_cast<const vsai::VSLocalActionReplayEvent *>(event));
            break;
        case EVENT_BOARD_TOUCH_DOWN_REPLY: {
            auto *event1 = static_cast<const U8U8I16I16_Event *>(event);
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            SeedBank *clientSeedBank = mGamepadControls[1]->mGamepadIndex == 1 ? mSeedBank[1] : mSeedBank[0];
            const int selectedSeedIndex = DecodeSelectedSeedIndex(event1->data1, clientSeedBank);
            if (clientGamepadControls->mSelectedSeedIndex != selectedSeedIndex) {
                clientGamepadControls->mSelectedSeedIndex = selectedSeedIndex;
                if (selectedSeedIndex >= 0) {
                    clientSeedBank->mSeedPackets[selectedSeedIndex].mLastSelectedTime = 0.0f; // 动画效果专用
                }
            }
            clientGamepadControls->mGamepadState = BaseGamepadControls::MovementState(event1->data2);
            //            clientGamepadControls->mCursorPositionX = event1->data5;
            //            clientGamepadControls->mCursorPositionY = event1->data5;
        } break;
        case EVENT_BOARD_TOUCH_DRAG_REPLY: {
            //  auto *event1 = static_cast<const U16U16_Event *>(event);
            // GamepadControls *clientGamepadControls = mGamepadControls[1]->mGamepadIndex == 1 ? mGamepadControls[1] : mGamepadControls[0];
            //            clientGamepadControls->mCursorPositionX = event1->data1;
            //            clientGamepadControls->mCursorPositionY = event1->data3;
        } break;
        case EVENT_BOARD_TOUCH_UP_REPLY: {
            auto *event1 = static_cast<const U8U8_Event *>(event);
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            CursorObject *clientCursorObject = mGamepadControls[1]->mGamepadIndex == 1 ? mCursorObject[1] : mCursorObject[0];
            clientGamepadControls->mGamepadState = BaseGamepadControls::MovementState(event1->data1);
            clientCursorObject->mCursorType = (CursorType)event1->data2;
        } break;
        case EVENT_SERVER_BOARD_TOUCH_DOWN: {
            auto *event1 = static_cast<const U8U8I16I16_Event *>(event);
            GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            SeedBank *serverSeedBank = mGamepadControls[0]->mGamepadIndex == 0 ? mSeedBank[0] : mSeedBank[1];
            const int selectedSeedIndex = DecodeSelectedSeedIndex(event1->data1, serverSeedBank);
            if (serverGamepadControls->mSelectedSeedIndex != selectedSeedIndex) {
                serverGamepadControls->mSelectedSeedIndex = selectedSeedIndex;
                if (selectedSeedIndex >= 0) {
                    serverSeedBank->mSeedPackets[selectedSeedIndex].mLastSelectedTime = 0.0f; // 动画效果专用
                }
            }
            serverGamepadControls->mGamepadState = BaseGamepadControls::MovementState(event1->data2);
            serverGamepadControls->mCursorPositionX = event1->data3;
            serverGamepadControls->mCursorPositionY = event1->data4;
        } break;
        case EVENT_SERVER_BOARD_TOUCH_DRAG: {
            auto *event1 = static_cast<const U16U16_Event *>(event);
            GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            serverGamepadControls->mCursorPositionX = event1->data1;
            serverGamepadControls->mCursorPositionY = event1->data2;
        } break;
        case EVENT_SERVER_BOARD_TOUCH_UP: {
            auto *event1 = static_cast<const U8U8_Event *>(event);
            GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            CursorObject *serverCursorObject = mGamepadControls[0]->mGamepadIndex == 0 ? mCursorObject[0] : mCursorObject[1];
            serverGamepadControls->mGamepadState = BaseGamepadControls::MovementState(event1->data1);
            serverCursorObject->mCursorType = (CursorType)event1->data2;
        } break;
        case EVENT_SERVER_BOARD_TOUCH_CLEAR_CURSOR: {
            GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            ClearCursor(mGamepadControls[0]->mGamepadIndex == 0 ? 0 : 1);
            serverGamepadControls->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
        } break;
        case EVENT_CLIENT_BOARD_TOUCH_CLEAR_CURSOR: {
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            ClearCursor(mGamepadControls[0]->mGamepadIndex == 0 ? 1 : 0);
            clientGamepadControls->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
        } break;
        case EVENT_CLIENT_BOARD_GAMEPAD_SET_STATE: {
            GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
            auto *event1 = static_cast<const U8_Event *>(event);
            clientGamepadControls->mGamepadState = BaseGamepadControls::MovementState(event1->data);
        } break;
        case EVENT_SERVER_BOARD_GAMEPAD_SET_STATE: {
            auto *event1 = static_cast<const U8_Event *>(event);
            GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            serverGamepadControls->mGamepadState = BaseGamepadControls::MovementState(event1->data);
        } break;
        case EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL: {
            auto *event1 = static_cast<const U8_Event *>(event);
            if (!requestDrawShovelInCursor && event1->data) {
                mApp->PlayFoley(FOLEY_SHOVEL);
            }
            requestDrawShovelInCursor = event1->data;
        } break;
        case EVENT_SERVER_BOARD_GAMEPAD_USE_SHOVEL: {
            mApp->PlayFoley(FOLEY_USE_SHOVEL);
        } break;
        case EVENT_SERVER_BOARD_PAUSE: {
            auto *event1 = static_cast<const U8_Event *>(event);
            PauseFromSecondPlayer(event1->data);
        } break;
        case EVENT_SERVER_BOARD_COIN_ADD: {
            auto *event1 = static_cast<const U8U8U16U16_Event *>(event);
            old_Board_AddCoin(this, event1->data3, event1->data4, CoinType(event1->data1), CoinMotion(event1->data2));
        } break;
        case EVENT_SERVER_BOARD_PLANT_LAUNCHCOUNTER: {
            auto *event1 = static_cast<const U16U16_Event *>(event);
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, event1->data1, clientPlantID)) {
                Plant *plant = mPlants.DataArrayGet(clientPlantID);
                plant->mLaunchCounter = event1->data2;
            }
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_DIE: {
            auto *eventGridItemDie = static_cast<const U16_Event *>(event);
            uint16_t serverGridItemID = eventGridItemDie->data;
            uint16_t clientGridItemID = 0;
            if (homura::FindInMap(serverGridItemIDMap, serverGridItemID, clientGridItemID)) {
                GridItem *aGridItem = mGridItems.DataArrayGet(clientGridItemID);
                old_GridItem_GridItemDie(aGridItem);
                serverGridItemIDMap.erase(serverGridItemID);
            }
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_TAKE_DAMAGE: {
            auto *eventGridItemDamage = static_cast<const U16U16U8_Event *>(event);
            uint16_t clientGridItemID = 0;
            if (homura::FindInMap(serverGridItemIDMap, eventGridItemDamage->data1, clientGridItemID)) {
                GridItem *aGridItem = mGridItems.DataArrayGet(clientGridItemID);
                aGridItem->TakeDamage_Origin(eventGridItemDamage->data2, eventGridItemDamage->data3);
            }
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_LAUNCHCOUNTER: {
            auto *event1 = static_cast<const U16U16_Event *>(event);
            uint16_t clientGridItemID = 0;
            if (homura::FindInMap(serverGridItemIDMap, event1->data1, clientGridItemID)) {
                GridItem *gridItem = mGridItems.DataArrayGet(clientGridItemID);
                gridItem->mLaunchCounter = event1->data2;
            }
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_SUMMONCOUNTER: {
            auto *eventSummonCounter = static_cast<const U16U16_Event *>(event);
            uint16_t clientGridItemID = 0;
            if (homura::FindInMap(serverGridItemIDMap, eventSummonCounter->data1, clientGridItemID)) {
                GridItem *gridItem = mGridItems.DataArrayGet(clientGridItemID);
                gridItem->mSummonCounter = eventSummonCounter->data2;
            }
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_ADDLADDER: {
            auto *event1 = static_cast<const U8U8U16_Event *>(event);
            GridItem *ladder = AddALadder_Origin(event1->data1, event1->data2);
            serverGridItemIDMap[event1->data3] = uint16_t(mGridItems.DataArrayGetID(ladder));
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_ADDCRATER: {
            auto *event1 = static_cast<const U8U8U16_Event *>(event);
            GridItem *crater = AddACrater_Origin(event1->data1, event1->data2);
            serverGridItemIDMap[event1->data3] = uint16_t(mGridItems.DataArrayGetID(crater));
            crater->mGridItemCounter = 18000;
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_ADDGRAVE: {
            auto *event1 = static_cast<const U8U8U16U16_Event *>(event);
            GridItem *gridItem = AddAGraveStone(event1->data1, event1->data2);
            gridItem->mLaunchCounter = event1->data4;
            //            gridItem->mVSGraveStoneHealth = 350;
            //            gridItem->mIsSpecialGrave = true;
            serverGridItemIDMap[event1->data3] = uint16_t(mGridItems.DataArrayGetID(gridItem));
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_ADDMOUND: {
            auto *eventAddMound = static_cast<const U8x3U16x3_Event *>(event);
            int gridX = eventAddMound->data1[0];
            int gridY = eventAddMound->data1[1];
            int moundLevel = eventAddMound->data1[2];
            GridItem *mound = AddAMound(gridX, gridY, moundLevel);
            serverGridItemIDMap[eventAddMound->data2[0]] = uint16_t(mGridItems.DataArrayGetID(mound));
            mound->mLaunchCounter = eventAddMound->data2[1];
            mound->mSummonCounter = eventAddMound->data2[2];
            mound->mIsSpecialGrave = true;
            mound->mVSGraveStoneHealth = 350 + 70 * (moundLevel + 1);
        } break;
        case EVENT_SERVER_BOARD_GRIDITEM_ADDPOLE: {
            auto *eventAddPole = static_cast<const U16x4_Event *>(event);
            GridItem *pole = AddAPole_Origin(eventAddPole->data[0], eventAddPole->data[1], eventAddPole->data[2]);
            serverGridItemIDMap[eventAddPole->data[3]] = uint16_t(mGridItems.DataArrayGetID(pole));
        } break;
        case EVENT_SERVER_BOARD_PLANT_PINGPONG_ANIMATION: {
            auto *event1 = static_cast<const U16U16U16UNI32UNI32_Event *>(event);
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, event1->data1, clientPlantID)) {
                Plant *plant = mPlants.DataArrayGet(clientPlantID);
                plant->mFrameLength = event1->data2;
                plant->mAnimPing = event1->data3;
                plant->mFrame = int(event1->data4.u32);
                plant->mAnimCounter = int(event1->data5.u32);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_OTHER_ANIMATION: {
            auto *event1 = static_cast<const U16U16U16UNI32UNI32_Event *>(event);
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, event1->data1, clientPlantID)) {
                Plant *plant = mPlants.DataArrayGet(clientPlantID);
                Reanimation *reanimation = mApp->ReanimationTryToGet(plant->GetPlantReanimationIDByIndex(event1->data2));
                if (reanimation != nullptr) {
                    reanimation->mLoopType = ReanimLoopType(event1->data3);
                    reanimation->mAnimTime = event1->data4.f32;
                    reanimation->SetAnimRate(event1->data5.f32);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_FIRE: {
            auto *eventPlantFire = static_cast<const U16U16U16UNI32UNI32_Event *>(event);
            uint16_t serverPlantID = eventPlantFire->data1;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                uint16_t aZombieID = eventPlantFire->data2;
                uint16_t aGridItemID = eventPlantFire->data5.u16x2.u16_1;
                uint16_t aRow = eventPlantFire->data4.u16x2.u16_1;
                auto aPlantWeapon = PlantWeapon(eventPlantFire->data4.u16x2.u16_2);
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);

                Zombie *aZombie = nullptr;
                if (aZombieID != NETPLAY_ZOMBIE_ID_NULL) {
                    uint16_t clientZombieID = 0;
                    if (!homura::FindInMap(serverZombieIDMap, aZombieID, clientZombieID)) {
                        break;
                    }
                    aZombie = mZombies.DataArrayGet(clientZombieID);
                }

                GridItem *aGridItem = nullptr;
                if (aGridItemID != NETPLAY_GRIDITEM_ID_NULL) {
                    uint16_t clientGridItemID = 0;
                    if (!homura::FindInMap(serverGridItemIDMap, aGridItemID, clientGridItemID)) {
                        break;
                    }
                    aGridItem = mGridItems.DataArrayGet(clientGridItemID);
                }

                aPlant->Fire_Origin(aZombie, aRow, aPlantWeapon, aGridItem);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_ADD: {
            auto *eventPlantAdd = static_cast<const U16U16U16UNI32UNI32_Event *>(event);
            int gridX = eventPlantAdd->data1;
            int gridY = eventPlantAdd->data2;
            auto seedType = SeedType(eventPlantAdd->data4.u16x2.u16_1);
            auto imitaterType = SeedType(eventPlantAdd->data4.u16x2.u16_2);
            bool isDoEffect = eventPlantAdd->data5.u16x2.u16_2;
            Plant *plant = AddPlant_Origin(gridX, gridY, seedType, imitaterType, 0, isDoEffect);
            plant->mLaunchCounter = int(eventPlantAdd->data3);
            serverPlantIDMap[eventPlantAdd->data5.u16x2.u16_1] = uint16_t(mPlants.DataArrayGetID(plant));
        } break;
        case EVENT_SERVER_BOARD_PLAY_SOUND: {
            auto *eventSound = static_cast<const U8_Event *>(event);
            switch (eventSound->data) {
                case 0:
                    mApp->PlaySample(SOUND_GULP);
                    break;
                case 1:
                    mApp->PlaySample(SOUND_BUZZER);
                    break;
                case 2:
                    mApp->PlaySample(SOUND_SEEDLIFT);
                    break;
                default:
                    break;
            }
        } break;
        case EVENT_SERVER_BOARD_PLAY_SOUND_SR: {
            if (!(gIsServerModeSpectator || gIsReplayMode)) {
                break;
            }
            auto *eventSound = static_cast<const U8U8_Event *>(event);
            if (eventSound->data1 == 0) {
                mApp->PlaySample(eventSound->data2);
            } else if (eventSound->data1 == 1) {
                mApp->PlayFoley(static_cast<FoleyType>(eventSound->data2));
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_DIE: {
            auto *eventPlantDie = static_cast<const U16_Event *>(event);
            uint16_t serverPlantID = eventPlantDie->data;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                aPlant->Die_Origin();
                serverPlantIDMap.erase(serverPlantID);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_DO_SPECIAL: {
            auto *eventPlantDoSpecial = static_cast<const U16_Event *>(event);
            uint16_t serverPlantID = eventPlantDoSpecial->data;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                aPlant->DoSpecial_Origin();
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_ICE_A_ZOMBIE: {
            auto *eventIceAZombie = static_cast<const U16U16_Event *>(event);
            uint16_t serverPlantID = eventIceAZombie->data1;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                Zombie *aZombie = nullptr;
                if (eventIceAZombie->data2 != NETPLAY_ZOMBIE_ID_NULL) {
                    uint16_t clientZombieID = 0;
                    if (homura::FindInMap(serverZombieIDMap, eventIceAZombie->data2, clientZombieID)) {
                        aZombie = mZombies.DataArrayGet(clientZombieID);
                    }
                }
                aPlant->IceAZombie(aZombie);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_FINDTARGETANDFIRE: {
            auto *event1 = static_cast<const U8U8U16_Event *>(event);
            uint16_t serverPlantID = event1->data3;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                if (aPlant->mSeedType == SeedType::SEED_PEANUT) {
                    aPlant->LaunchPeanut();
                } else {
                    old_Plant_FindTargetAndFire(aPlant, event1->data1, PlantWeapon(event1->data2));
                }
                if (aPlant->mSeedType == SEED_LEFTPEATER) {
                    Reanimation *aHeadReanim = mApp->ReanimationGet(aPlant->mHeadReanimID);
                    Reanimation *aBodyReanim = mApp->ReanimationGet(aPlant->mBodyReanimID);
                    aHeadReanim->StartBlend(20);
                    aHeadReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
                    aHeadReanim->SetFramesForLayer("anim_head_idle");
                    aHeadReanim->SetAnimRate(aBodyReanim->mAnimRate);
                    aHeadReanim->mAnimTime = aBodyReanim->mAnimTime;
                }
                if (aPlant->mSeedType == SeedType::SEED_BLOOMERANG) {
                    mApp->PlayFoley(FoleyType::FOLEY_BLOOMERANG);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_KERNELPLUT_FINDTARGETANDFIRE: {
            auto *event1 = static_cast<const U8U8U16U16_Event *>(event);
            uint16_t serverPlantID = event1->data3;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                old_Plant_FindTargetAndFire(aPlant, event1->data1, PlantWeapon(event1->data2));
                auto serverState = PlantState(event1->data4);
                if (aPlant->mState != serverState) {
                    aPlant->mState = serverState;
                    Reanimation *reanimation = mApp->ReanimationGet(aPlant->mBodyReanimID);
                    if (serverState == STATE_KERNELPULT_BUTTER) {
                        reanimation->AssignRenderGroupToPrefix("Cornpult_butter", RENDER_GROUP_NORMAL);
                        reanimation->AssignRenderGroupToPrefix("Cornpult_kernal", RENDER_GROUP_HIDDEN);
                    } else {
                        reanimation->AssignRenderGroupToPrefix("Cornpult_butter", RENDER_GROUP_HIDDEN);
                        reanimation->AssignRenderGroupToPrefix("Cornpult_kernal", RENDER_GROUP_NORMAL);
                    }
                }
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_SHOOTER_LAUNCH: {
            auto *event1 = static_cast<const U16_Event *>(event);
            uint16_t serverPlantID = event1->data;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                if (aPlant->mSeedType == SEED_THREEPEATER) {
                    aPlant->LaunchThreepeater();
                } else if (aPlant->mSeedType == SEED_STARFRUIT) {
                    aPlant->LaunchStarFruit();
                }
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_CHOMPER_BIT: {
            auto *eventChomperBit = static_cast<const U16U16_Event *>(event);
            uint16_t serverPlantID = eventChomperBit->data1;
            uint16_t serverZombieID = eventChomperBit->data2;
            uint16_t clientPlantID = 0;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID) && homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->DieWithLoot();
                aPlant->mState = PlantState::STATE_CHOMPER_BITING_GOT_ONE;
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_MAGNETSHROOM_ATTACK: {
            auto *eventMagnetshroomAttack = static_cast<const U16U16_Event *>(event);
            uint16_t serverPlantID = eventMagnetshroomAttack->data1;
            uint16_t serverZombieID = eventMagnetshroomAttack->data2;
            uint16_t clientPlantID = 0;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID) && homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aPlant->MagnetShroomAttactItem(aZombie);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_MAGNETSHROOM_ATTACK_LADDER: {
            auto *eventMagnetshroomAttackLadder = static_cast<const U16U16_Event *>(event);
            uint16_t serverPlantID = eventMagnetshroomAttackLadder->data1;
            uint16_t serverGridItemID = eventMagnetshroomAttackLadder->data2;
            uint16_t clientPlantID = 0;
            uint16_t clientGridItemID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID) && homura::FindInMap(serverGridItemIDMap, serverGridItemID, clientGridItemID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                GridItem *aGridItem = mGridItems.DataArrayGet(clientGridItemID);

                aPlant->mState = PlantState::STATE_MAGNETSHROOM_SUCKING;
                aPlant->mStateCountdown = 1500;
                aPlant->PlayBodyReanim("anim_shooting", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 12.0f);
                aPlant->mApp->PlayFoley(FoleyType::FOLEY_MAGNETSHROOM);

                aGridItem->GridItemDie();

                MagnetItem *aMagnetItem = aPlant->GetFreeMagnetItem();
                aMagnetItem->mPosX = aPlant->mBoard->GridToPixelX(aGridItem->mGridX, aGridItem->mGridY) + 40;
                aMagnetItem->mPosY = aPlant->mBoard->GridToPixelY(aGridItem->mGridX, aGridItem->mGridY);
                aMagnetItem->mDestOffsetX = RandRangeFloat(-10.0f, 10.0f) + 10.0f;
                aMagnetItem->mDestOffsetY = RandRangeFloat(-10.0f, 10.0f);
                aMagnetItem->mItemType = MagnetItemType::MAGNET_ITEM_LADDER_PLACED;
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_SQUASH_STATE: {
            auto *eventPlantSquashState = static_cast<const U16U16I16I16_Event *>(event);
            uint16_t serverPlantID = eventPlantSquashState->data1;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                PlantState oldState = aPlant->mState;
                auto serverState = PlantState(eventPlantSquashState->data2);
                aPlant->mState = serverState;
                aPlant->mStateCountdown = eventPlantSquashState->data3;
                aPlant->mTargetX = eventPlantSquashState->data4;

                if (serverState == PlantState::STATE_SQUASH_LOOK && oldState != PlantState::STATE_SQUASH_LOOK) {
                    aPlant->PlayBodyReanim(aPlant->mTargetX < aPlant->mX ? "anim_lookleft" : "anim_lookright", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
                } else if (serverState == PlantState::STATE_SQUASH_PRE_LAUNCH && oldState != PlantState::STATE_SQUASH_PRE_LAUNCH) {
                    aPlant->PlayBodyReanim("anim_jumpup", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
                } else if (serverState == PlantState::STATE_SQUASH_RISING && oldState != PlantState::STATE_SQUASH_RISING) {
                    aPlant->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, aPlant->mRow, 0);
                } else if (serverState == PlantState::STATE_SQUASH_FALLING && oldState != PlantState::STATE_SQUASH_FALLING) {
                    aPlant->PlayBodyReanim("anim_jumpdown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 60.0f);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_ICEBERG_LETTUCE_LAUNCH: {
            auto *eventPlantLaunch = static_cast<const U16_Event *>(event);
            uint16_t serverPlantID = eventPlantLaunch->data;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                mApp->PlayFoley(FoleyType::FOLEY_ICEBERG);
                aPlant->PlayBodyReanim("anim_explode", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_WIN: {
            //            auto *plantWinEvent = static_cast<const U16_Event *>(event);
            //            uint16_t serverGridItemID = plantWinEvent->data;
            //            uint16_t clientGridItemID;
            //            if (homura::FindInMap(serverGridItemIDMap, serverGridItemID, clientGridItemID)) {
            //                GridItem *aGridItem = mGridItems.DataArrayGet(clientGridItemID);
            //                PlantsWon_Origin(aGridItem);
            //            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_DIE: {
            auto *eventZombieDie = static_cast<const U16_Event *>(event);
            uint16_t serverZombieID = eventZombieDie->data;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                if (!aZombie->mDead) {
                    aZombie->DieNoLoot_Origin();
                } else {
                    LOG_WARN("mDead:{}", (int)aZombie->mZombieType);
                }
                serverZombieIDMap.erase(serverZombieID);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_MIND_CONTROLLED: {
            auto *eventZombieMindControlled = static_cast<const U16_Event *>(event);
            uint16_t serverZombieID = eventZombieMindControlled->data;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->StartMindControlled_Origin();
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_CONVERT_TO_IMP: {
            auto *eventZombieConvertToImp = static_cast<const U16_Event *>(event);
            uint16_t serverZombieID = eventZombieConvertToImp->data;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->ConvertToImp_Origin();
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_ADD: {
            auto *eventZombieAdd = static_cast<const U8x5U16UNI32x2_Event *>(event);
            auto aZombieType = ZombieType(eventZombieAdd->data1[0]);
            uint8_t aRow = eventZombieAdd->data1[1];
            auto aFromWave = int8_t(eventZombieAdd->data1[2]);
            uint8_t aIsRustle = eventZombieAdd->data1[3];
            if (aZombieType == ZombieType::ZOMBIE_BACKUP_DANCER) // 移除主机生成时向客机同步传递的舞伴
                return;
            Zombie *aZombie = AddZombieInRow_Origin(aZombieType, aRow, aFromWave, aIsRustle);
            serverZombieIDMap[eventZombieAdd->data2] = uint16_t(mZombies.DataArrayGetID(aZombie));
            float aVelX = eventZombieAdd->data3[0].f32;
            aZombie->ApplySyncedSpeed(aVelX, short(aZombie->mAnimTicksPerFrame));
            aZombie->mPosX = eventZombieAdd->data3[1].f32;
            if (aZombie->mZombieType == ZombieType::ZOMBIE_NORMAL && !aZombie->mInPool) {
                aZombie->mBloated = eventZombieAdd->data1[4];
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_BOBSELD_ADD: {
            auto *eventZombieBobseldAdd = static_cast<const U8x2U16x4UNI32x8_Event *>(event);
            uint8_t aRow = eventZombieBobseldAdd->data1[0];
            auto aFromWave = int8_t(eventZombieBobseldAdd->data1[1]);
            Zombie *aZombie = AddZombieInRow_Origin(ZOMBIE_BOBSLED, aRow, aFromWave, true);
            serverZombieIDMap[eventZombieBobseldAdd->data2[0]] = uint16_t(mZombies.DataArrayGetID(aZombie));
            aZombie->ApplySyncedSpeed(eventZombieBobseldAdd->data3[0].f32, short(aZombie->mAnimTicksPerFrame));
            aZombie->mPosX = eventZombieBobseldAdd->data4[0].f32;
            for (int i = 0; i < 3; ++i) {
                Zombie *aFollowerZombie = mZombies.DataArrayGet(aZombie->mFollowerZombieID[i]);
                serverZombieIDMap[eventZombieBobseldAdd->data2[i + 1]] = uint16_t(aZombie->mFollowerZombieID[i]);
                aFollowerZombie->ApplySyncedSpeed(eventZombieBobseldAdd->data3[i + 1].f32, short(aFollowerZombie->mAnimTicksPerFrame));
                aFollowerZombie->mPosX = eventZombieBobseldAdd->data4[i + 1].f32;
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_DOGWALKER_ADD: {
            auto *eventDogWalkerAdd = static_cast<const U8x2U16x4UNI32x8_Event *>(event);
            const int aRow = eventDogWalkerAdd->data1[0];
            const int aFromWave = int8_t(eventDogWalkerAdd->data1[1]);
            const bool aIsRustle = eventDogWalkerAdd->data2[2] != 0;

            Zombie *aZombie = AddZombieInRow_Origin(ZombieType::ZOMBIE_DOGWALKER, aRow, aFromWave, aIsRustle);
            if (aZombie == nullptr) {
                break;
            }

            Zombie *aZombieDog = mZombies.DataArrayGet(aZombie->mRelatedZombieID);
            if (aZombieDog == nullptr) {
                break;
            }

            serverZombieIDMap[eventDogWalkerAdd->data2[0]] = uint16_t(mZombies.DataArrayGetID(aZombie));
            serverZombieIDMap[eventDogWalkerAdd->data2[1]] = uint16_t(mZombies.DataArrayGetID(aZombieDog));

            aZombie->ApplySyncedSpeed(eventDogWalkerAdd->data3[0].f32, short(aZombie->mAnimTicksPerFrame));
            aZombieDog->ApplySyncedSpeed(eventDogWalkerAdd->data3[1].f32, short(aZombieDog->mAnimTicksPerFrame));

            aZombie->mPosX = eventDogWalkerAdd->data4[0].f32;
            aZombieDog->mPosX = eventDogWalkerAdd->data4[1].f32;
            aZombie->mX = int(aZombie->mPosX);
            aZombieDog->mX = int(aZombieDog->mPosX);
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_SET_STEAL_GRID: {
            auto *eventZombieBungeeAdd = static_cast<const U16UNI32UNI32_Event *>(event);

            int aTargetCol = eventZombieBungeeAdd->data2.u8x4.u8_1;
            int aRow = eventZombieBungeeAdd->data2.u8x4.u8_2;
            Zombie *aZombie = AddZombieInRow_Origin(ZOMBIE_BUNGEE, aRow, 0, false);
            serverZombieIDMap[eventZombieBungeeAdd->data1] = uint16_t(mZombies.DataArrayGetID(aZombie));

            aZombie->mAltitude = eventZombieBungeeAdd->data3.f32;
            aZombie->mTargetCol = aTargetCol;
            aZombie->SetRow(aRow);
            aZombie->mPosX = float(GridToPixelX(aTargetCol, aRow));
            aZombie->mPosY = aZombie->GetPosYBasedOnRow(aRow);
            aZombie->mRenderOrder = Board::MakeRenderOrder(RENDER_LAYER_GRAVE_STONE, aRow, 7);
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_LIFT_TARGET: {
            auto *eventBungeeLiftTarget = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = eventBungeeLiftTarget->data1;
            uint16_t serverPlantID = eventBungeeLiftTarget->data2;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                if (serverPlantID == NETPLAY_PLANT_ID_NULL) {
                    aZombie->mTargetPlantID = PlantID::PLANTID_NULL;
                } else {
                    uint16_t clientPlantID = 0;
                    if (!homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                        break;
                    }
                    aZombie->mTargetPlantID = PlantID(clientPlantID);
                }
                aZombie->BungeeLiftTarget();
                aZombie->mZombiePhase = ZombiePhase::PHASE_BUNGEE_RISING;
            }
        } break;

        case EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_DROP_ZOMBIE: {
            auto *eventBungeeDropZombie = static_cast<const U16UNI32UNI32_Event *>(event);
            int gridX = eventBungeeDropZombie->data2.u8x4.u8_1;
            int gridY = eventBungeeDropZombie->data2.u8x4.u8_2;
            uint16_t serverBungeeZombieID = eventBungeeDropZombie->data2.u16x2.u16_1;
            uint16_t serverDroppedZombieID = eventBungeeDropZombie->data2.u16x2.u16_2;
            uint16_t clientDroppedZombieID = 0;

            Zombie *aBungeeZombie = AddZombie_Origin(ZombieType::ZOMBIE_BUNGEE, Zombie::ZOMBIE_WAVE_VS, false);
            serverZombieIDMap[serverBungeeZombieID] = uint16_t(mZombies.DataArrayGetID(aBungeeZombie));

            if (homura::FindInMap(serverZombieIDMap, serverDroppedZombieID, clientDroppedZombieID)) {
                Zombie *droppedZombie = mZombies.DataArrayGet(clientDroppedZombieID);
                aBungeeZombie->BungeeDropZombie_Origin(droppedZombie, gridX, gridY);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_BUNGEE_HIT_UMBRELLA: {
            auto *eventBungeeHitUmbrella = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = eventBungeeHitUmbrella->data1;
            uint16_t serverPlantID = eventBungeeHitUmbrella->data2;
            uint16_t clientZombieID = 0;
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID) && homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                mApp->PlaySample(SOUND_BOING);
                mApp->PlayFoley(FoleyType::FOLEY_UMBRELLA);
                aPlant->DoSpecial();
                aZombie->mZombiePhase = ZombiePhase::PHASE_BUNGEE_RISING;
                aZombie->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_TOP, 0, 1);
                aZombie->mHitUmbrella = true;
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_ADD_BY_CHEAT: {
            auto *eventZombieAddByCheat = static_cast<const U16UNI32UNI32_Event *>(event);
            int theGridX = eventZombieAddByCheat->data2.u8x4.u8_1;
            int theGridY = eventZombieAddByCheat->data2.u8x4.u8_2;
            uint16_t serverZombieID = eventZombieAddByCheat->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                if (aZombie->mZombieType == ZOMBIE_BUNGEE) {
                    aZombie->mAltitude = eventZombieAddByCheat->data3.f32;
                    aZombie->mTargetCol = theGridX;
                    aZombie->SetRow(theGridY);
                    aZombie->mPosX = float(GridToPixelX(theGridX, theGridY));
                    aZombie->mPosY = aZombie->GetPosYBasedOnRow(theGridY);
                    aZombie->mRenderOrder = Board::MakeRenderOrder(RENDER_LAYER_GRAVE_STONE, theGridX, 7);
                } else {
                    aZombie->mPosX = float(GridToPixelX(theGridX, theGridY)) - 30.0f;
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_RIZE_FORM_GRAVE: {
            auto *event1 = static_cast<const U8U8U16_Event *>(event);
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, event1->data3, clientZombieID)) {
                Zombie *zombie = mZombies.DataArrayGet(clientZombieID);
                zombie->RiseFromGrave(event1->data1, event1->data2);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_SUMMON_BACKUP_DANCERS: {
            auto *eventSummonBackupDancers = static_cast<const U16x5UNI32x5_Event *>(event);
            uint16_t serverZombieID = eventSummonBackupDancers->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = eventSummonBackupDancers->data2.f32;
                aZombie->SummonBackupDancers_Origin();

                for (int i = 0; i < NUM_BACKUP_DANCERS; ++i) {
                    Zombie *backupZombie = ZombieTryToGet(aZombie->mFollowerZombieID[i]);
                    if (backupZombie) {
                        serverZombieIDMap[uint16_t(eventSummonBackupDancers->data3[i])] = uint16_t(aZombie->mFollowerZombieID[i]);
                        float aVelX = eventSummonBackupDancers->data4[i].f32;
                        backupZombie->ApplySyncedSpeed(aVelX, short(aZombie->mAnimTicksPerFrame));
                    }
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_RAISE_DEAD: {
            auto *eventRaiseDead = static_cast<const U16UNI32U8x16U16x15UNI32x15_Event *>(event);
            uint16_t serverZombieID = eventRaiseDead->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = eventRaiseDead->data2.f32;

                int count = std::min<int>(15, eventRaiseDead->data3[0]);
                for (int i = 0; i < count; ++i) {
                    int aRow = 0, aCol = 0;
                    if (i < 5) {
                        aRow = i;
                        aCol = aZombie->mMindControlled ? 4 : 6;
                    } else if (i < 10) {
                        aRow = i - 5;
                        aCol = aZombie->mMindControlled ? 3 : 7;
                    } else {
                        aRow = i - 10;
                        aCol = aZombie->mMindControlled ? 2 : 8;
                    }

                    ZombieID revivedClientID = aZombie->RaiseDeadZombie(ZombieType(eventRaiseDead->data3[i + 1]), aRow, aCol);
                    if (revivedClientID != ZombieID::ZOMBIEID_NULL) {
                        serverZombieIDMap[eventRaiseDead->data4[i]] = uint16_t(revivedClientID);
                        Zombie *revivedZombie = ZombieTryToGet(revivedClientID);
                        if (revivedZombie) {
                            revivedZombie->ApplySyncedSpeed(eventRaiseDead->data5[i].f32, short(revivedZombie->mAnimTicksPerFrame));
                        }
                    }
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_PICK_SPEED: {
            auto *eventPickSpeed = static_cast<const U16U16U16UNI32UNI32_Event *>(event);
            uint16_t serverZombieID = eventPickSpeed->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                float aVelX = eventPickSpeed->data4.f32;
                uint16_t anAnimTicks = eventPickSpeed->data2;
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->ApplySyncedSpeed(aVelX, short(anAnimTicks));
                float syncedPosX = eventPickSpeed->data5.f32;
                if (aZombie->mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER) {
                    LOG_DEBUG("{} {} {}", aZombie->mPosX, syncedPosX, (int)aZombie->mZombiePhase);
                }
                // During jump-into-pool transition, keep local X progression and don't force-resync X.
                aZombie->mPosX = syncedPosX;


                // 撑杆僵尸落地
                // polevaulter landing is authoritative from EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_POST_VAULT

                // dolphin rider phase transition is authoritative from EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_BOBSLED_PICK_SPEED: {
            auto *eventBobsledPickSpeed = static_cast<const U8x2U16x4UNI32x8_Event *>(event);
            uint16_t serverLeaderID = eventBobsledPickSpeed->data2[0];
            uint16_t clientLeaderID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverLeaderID, clientLeaderID)) {
                Zombie *leaderZombie = mZombies.DataArrayGet(clientLeaderID);
                if (leaderZombie) {
                    for (int i = 0; i < NUM_BOBSLED_FOLLOWERS; i++) {
                        Zombie *aZombie = ZombieGet(leaderZombie->mFollowerZombieID[i]);
                        if (aZombie == nullptr) {
                            continue;
                        }
                        aZombie->mRelatedZombieID = ZombieID::ZOMBIEID_NULL;
                        leaderZombie->mFollowerZombieID[i] = ZombieID::ZOMBIEID_NULL;
                        aZombie->ApplySyncedSpeed(eventBobsledPickSpeed->data3[i + 1].f32, short(aZombie->mAnimTicksPerFrame));
                        aZombie->mPosX = eventBobsledPickSpeed->data4[i + 1].f32;
                    }
                    leaderZombie->ApplySyncedSpeed(eventBobsledPickSpeed->data3[0].f32, short(leaderZombie->mAnimTicksPerFrame));
                    leaderZombie->mPosX = eventBobsledPickSpeed->data4[0].f32;
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_ICE_TRAP: {
            auto *eventIceTrap = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = eventIceTrap->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                uint16_t aIceTrapCounter = eventIceTrap->data2;
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->ApplySyncedIceTrap(aIceTrapCounter);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_IMP_THROWN: {
            auto *eventImpThrown = static_cast<const U16U16U16UNI32UNI32_Event *>(event);
            uint16_t serverGargantuarID = eventImpThrown->data1;
            uint16_t serverImpID = eventImpThrown->data2;
            uint16_t clientGargantuarID = 0;
            uint16_t clientImpID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverImpID, clientImpID) && homura::FindInMap(serverZombieIDMap, serverGargantuarID, clientGargantuarID)) {
                Zombie *aGargantuar = mZombies.DataArrayGet(clientGargantuarID);
                Zombie *aZombieImp = mZombies.DataArrayGet(clientImpID);
                float aOffsetDistance = eventImpThrown->data4.f32;
                if (aGargantuar && aZombieImp) {
                    if (aZombieImp->mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
                        aZombieImp->mTargetCol = int(eventImpThrown->data3);
                    }
                    aZombieImp->ZombieImpThrown(aGargantuar, aOffsetDistance);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_IMP_KICKED: {
            auto *eventImpKicked = static_cast<const U16UNI32UNI32_Event *>(event);
            uint16_t serverImpID = eventImpKicked->data1;
            uint16_t clientImpID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverImpID, clientImpID)) {
                Zombie *aZombieImp = mZombies.DataArrayGet(clientImpID);
                float aKickingDistance = eventImpKicked->data2.f32;
                if (aZombieImp) {
                    aZombieImp->mPosX = eventImpKicked->data3.f32;
                    aZombieImp->ZombieImpKicked(aKickingDistance);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_IMP_POP: {
            auto *eventImpPop = static_cast<const U16_Event *>(event);
            uint16_t serverImpID = eventImpPop->data;
            uint16_t clientImpID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverImpID, clientImpID)) {
                Zombie *aZombieImp = mZombies.DataArrayGet(clientImpID);
                if (aZombieImp) {
                    aZombieImp->mZombiePhase = ZombiePhase::PHASE_IMP_POPPING;
                    if (aZombieImp->mZombieType == ZombieType::ZOMBIE_GIGA_IMP) {
                        aZombieImp->PlayZombieReanim("anim_land", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
                    } else {
                        aZombieImp->PlayZombieReanim("anim_explode", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0f);
                    }
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_POST_VAULT: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosY = event1->data2.f32;
                aZombie->mY = int(aZombie->mPosY);
                aZombie->mZombiePhase = PHASE_POLEVAULTER_POST_VAULT;
                aZombie->mZombieAttackRect = Rect(50, 0, 20, 115);
                aZombie->mZombieHeight = HEIGHT_ZOMBIE_NORMAL;
                aZombie->StartWalkAnim(0);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_POLEVAULTER_IN_VAULT: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                if (aZombie->mZombiePhase == ZombiePhase::PHASE_RISING_FROM_GRAVE) {
                    aZombie->FinishZombieRiseFromGrave();
                }
                aZombie->mX = event1->data2.i16x2.i16_1;
                int aJumpDistance = event1->data2.i16x2.i16_2;
                aZombie->mZombiePhase = PHASE_POLEVAULTER_IN_VAULT;
                aZombie->PlayZombieReanim("anim_jump", REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
                Reanimation *aReanim = mApp->ReanimationGet(aZombie->mBodyReanimID);
                float aAnimDuration = aReanim->mFrameCount / aReanim->mAnimRate * 100.0f;
                aZombie->mVelX = aJumpDistance / aAnimDuration;
                aZombie->mHasObject = false;
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_SMASH: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = event1->data2.f32;
                aZombie->mZombiePhase = PHASE_GARGANTUAR_SMASHING;
                mApp->PlayFoley(FOLEY_LOW_GROAN);
                aZombie->PlayZombieReanim("anim_smash", REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_GARGANTUAR_START_THROW: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = event1->data2.f32;
                aZombie->mZombiePhase = PHASE_GARGANTUAR_THROWING;
                if (aZombie->mZombieType == ZombieType::ZOMBIE_GIGA_GARGANTUAR) {
                    aZombie->StopEating();
                    aZombie->mSummonCounter = 0;
                    aZombie->mVelX = 0.0f;
                    aZombie->mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_THROW_PREPARING;
                    aZombie->PlayZombieReanim("anim_throw_preparing", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);

                } else {
                    aZombie->mZombiePhase = ZombiePhase::PHASE_GARGANTUAR_THROWING;
                    aZombie->PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
                }
            }
        } break;
        case EventType::EVENT_SERVER_BOARD_ZOMBIE_GIGA_GARGANTUAR_START_LIGHTNING: {
            auto *event1 = static_cast<const U16UNI32UNI32_Event *>(event);
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, event1->data1, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                if (aZombie != nullptr) {
                    aZombie->StopEating();
                    aZombie->mPosX = event1->data2.f32;
                    aZombie->mX = int(aZombie->mPosX);
                    aZombie->mHasObject = false;
                    aZombie->mTargetPlantID = PlantID::PLANTID_NULL;
                    aZombie->mTargetCol = event1->data3.i32;
                    aZombie->mVelX = 0.0f;
                    aZombie->mZombiePhase = ZombiePhase::PHASE_GIGA_GARGANTUAR_LIGHTNING_PREPARING;
                    aZombie->mApp->PlayFoley(FoleyType::FOLEY_POWER_POLE_CHARGE);
                    aZombie->PlayZombieReanim("anim_lightning_attack_preparing", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_CATAPLUT_LAUNCHIING: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = event1->data2.f32;
                aZombie->mZombiePhase = PHASE_CATAPULT_LAUNCHING;
                aZombie->mPhaseCounter = 300;
                aZombie->PlayZombieReanim("anim_shoot", REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_CATAPLUT_FIRE: {
            auto *event1 = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            uint16_t serverPlantID = event1->data2;
            uint16_t clientPlantID = 0;
            if (serverPlantID != NETPLAY_PLANT_ID_NULL) {
                if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID) && homura::FindInMap(serverPlantIDMap, serverPlantID, clientPlantID)) {
                    Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                    Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                    aZombie->ZombieCatapultFire(aPlant);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_LADDER_START_PLACING: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = event1->data2.f32;
                aZombie->StopEating();
                aZombie->mZombiePhase = PHASE_LADDER_PLACING;
                aZombie->PlayZombieReanim("anim_placeladder", REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_LADDER_PLACED: {
            auto *event1 = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mApp->PlaySample(SOUND_LADDER_ZOMBIE);
                aZombie->mZombieHeight = HEIGHT_UP_LADDER;
                aZombie->mUseLadderCol = event1->data2;
                aZombie->DetachShield();
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_START_EATING: {
            auto *event1 = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mPosX = event1->data2.f32;
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_HUGE_WAVE: {
            mApp->PlaySample(Sexy::SOUND_HUGE_WAVE);
            DisplayAdviceAgain("[ADVICE_HUGE_WAVE]", MESSAGE_STYLE_HUGE_WAVE, ADVICE_HUGE_WAVE);
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_SET_ROW: {
            auto *event1 = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = event1->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->SetRow(event1->data2);
                aZombie->StartWalkAnim(20);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_SUN_BEAN_SUN: {
            auto *sunBeanEvent = static_cast<const U16U16_Event *>(event);
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, sunBeanEvent->data1, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->mSunBeanSun = short(sunBeanEvent->data2);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_TELEPORTATION_SHOOT: {
            auto *eventShoot = static_cast<const U16_Event *>(event);
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, eventShoot->data, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->StopEating();
                aZombie->mZombiePhase = ZombiePhase::PHASE_TELEPORTATION_SHOOTING;
                aZombie->PlayZombieReanim("anim_shoot", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 10, 24.0f);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_TELEPORT: {
            auto *eventTeleport = static_cast<const U16UNI32_Event *>(event);
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, eventTeleport->data1, clientZombieID)) {
                TeleportZombie(mZombies.DataArrayGet(clientZombieID), eventTeleport->data2.f32);
            }
        } break;
        case EVENT_SERVER_BOARD_PLANT_TELEPORT: {
            auto *eventTeleport = static_cast<const U16U16_Event *>(event);
            uint16_t clientPlantID = 0;
            if (homura::FindInMap(serverPlantIDMap, eventTeleport->data1, clientPlantID)) {
                Plant *aPlant = mPlants.DataArrayGet(clientPlantID);
                if (eventTeleport->data2 == UINT16_MAX) {
                    TeleportPlant(aPlant, int(UINT16_MAX), aPlant->mRow);
                    serverPlantIDMap.erase(eventTeleport->data1);
                } else {
                    TeleportPlant(aPlant, eventTeleport->data2, aPlant->mRow);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_PHASE_COUNTER: {
            auto *eventZombiePhaseCounter = static_cast<const U8U8U16U16_Event *>(event);
            uint8_t serverZombiePhase = eventZombiePhaseCounter->data1;
            uint8_t serverZombieSummonCounter = eventZombiePhaseCounter->data2;
            uint16_t serverZombieID = eventZombiePhaseCounter->data3;
            uint16_t clientZombieID = 0;
            uint16_t serverPhaseCounter = eventZombiePhaseCounter->data4;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                ZombiePhase oldZombiePhase = aZombie->mZombiePhase;
                aZombie->mZombiePhase = ZombiePhase(serverZombiePhase);
                aZombie->mPhaseCounter = serverPhaseCounter;
                if (aZombie->mZombieType == ZombieType::ZOMBIE_LADDER && aZombie->mZombiePhase == ZombiePhase::PHASE_LADDER_CARRYING) {
                    aZombie->StartWalkAnim(0);
                }
                if (aZombie->mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX && aZombie->mZombiePhase == ZombiePhase::PHASE_JACK_IN_THE_BOX_POPPING) {
                    aZombie->PlayZombieReanim("anim_pop", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 28.0f);
                }
                if (aZombie->mZombieType == ZombieType::ZOMBIE_GIGA_FOOTBALL) {
                    if (aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_TACKLING) {
                        aZombie->PlayZombieReanim("anim_tackle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_KICKING) {
                        aZombie->StopEating();
                        aZombie->PlayZombieReanim("anim_kick", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 20.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_WALKING || aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_CHARGING
                               || aZombie->mZombiePhase == ZombiePhase::PHASE_FOOTBALL_PRE_CHARGE) {
                        aZombie->StartWalkAnim(0);
                    }
                } else if (aZombie->mZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
                    aZombie->mSummonCounter = serverZombieSummonCounter;
                    aZombie->mPhaseCounter = serverPhaseCounter;
                    aZombie->mHasObject = aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PREPARE
                        || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_THROW;
                    if (aZombie->mHasObject) {
                        aZombie->mZombieAttackRect = Rect(-29, 0, 70, 115);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_POST_VAULT || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PICK
                               || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_TAKE || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_IN_VAULT) {
                        aZombie->mZombieAttackRect = Rect(50, 0, 20, 115);
                    }

                    if (aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_THROW) {
                        aZombie->PlayZombieReanim("anim_throw", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 36.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PREPARE) {
                        aZombie->PlayZombieReanim("anim_prepare", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 40.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PICK) {
                        aZombie->StopEating();
                        aZombie->PlayZombieReanim("anim_pick", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_TAKE) {
                        aZombie->PlayZombieReanim("anim_take", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 16.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_PRE_VAULT || aZombie->mZombiePhase == ZombiePhase::PHASE_POLEVAULTER_POST_VAULT) {
                        aZombie->StartWalkAnim(0);
                    }
                } else if (aZombie->mZombieType == ZombieType::ZOMBIE_JACKSON) {
                    if (aZombie->mZombiePhase == ZombiePhase::PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT) {
                        aZombie->PlayZombieReanim("anim_point", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
                    }
                } else if (aZombie->mZombieType == ZombieType::ZOMBIE_CATAPULT) {
                    int oldSummonCounter = aZombie->mSummonCounter;
                    aZombie->mSummonCounter = serverZombieSummonCounter;
                    if (aZombie->mZombiePhase == ZombiePhase::PHASE_ZOMBIE_NORMAL) {
                        aZombie->StartWalkAnim(20);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_CATAPULT_LAUNCHING) {
                        aZombie->PlayZombieReanim("anim_shoot", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_CATAPULT_RELOADING) {
                        aZombie->PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, 12.0f);
                    }
                    if (oldSummonCounter != aZombie->mSummonCounter) {
                        if (aZombie->mSummonCounter == 4) {
                            aZombie->ReanimShowTrack("Zombie_catapult_basketball", -1);
                        } else if (aZombie->mSummonCounter == 3) {
                            aZombie->ReanimShowTrack("Zombie_catapult_basketball2", -1);
                        } else if (aZombie->mSummonCounter == 2) {
                            aZombie->ReanimShowTrack("Zombie_catapult_basketball3", -1);
                        } else if (aZombie->mSummonCounter == 1) {
                            aZombie->ReanimShowTrack("Zombie_catapult_basketball4", -1);
                        }
                    }
                } else if (aZombie->mZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER) {
                    if (aZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_INTO_POOL) {
                        aZombie->PlayZombieReanim("anim_jumpinpool", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 16.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_RIDING) {
                        aZombie->mInPool = true;
                        aZombie->mPosX -= 70.0f;
                        aZombie->mZombieAttackRect = Rect(-29, 0, 70, 115);
                        aZombie->PlayZombieReanim("anim_ride", ReanimLoopType::REANIM_LOOP_FULL_LAST_FRAME, 0, 12.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_IN_JUMP) {
                        aZombie->mVelX = 0.5f;
                        aZombie->PlayZombieReanim("anim_dolphinjump", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 10.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_IN_POOL) {
                        aZombie->mZombieAttackRect = Rect(30, 0, 30, 115);
                        aZombie->mZombieRect = Rect(20, 0, 42, 115);
                        aZombie->mUnk95 = 0;
                        aZombie->StartWalkAnim(0);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING_WITHOUT_DOLPHIN) {
                        aZombie->mZombieHeight = ZombieHeight::HEIGHT_OUT_OF_POOL;
                        aZombie->mAltitude = -40.0f;
                        aZombie->PlayZombieReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_DOLPHIN_WALKING) {
                        aZombie->mZombieHeight = ZombieHeight::HEIGHT_OUT_OF_POOL;
                        aZombie->mAltitude = -40.0f;
                        aZombie->PlayZombieReanim("anim_walkdolphin", ReanimLoopType::REANIM_LOOP, 0, 0.0f);
                    }
                } else if (aZombie->mZombieType == ZombieType::ZOMBIE_SQUASH_HEAD) {
                    aZombie->mSquashHeadCol = serverZombieSummonCounter == 255 ? -1 : short(serverZombieSummonCounter);

                    if (aZombie->mZombiePhase == ZombiePhase::PHASE_SQUASH_RISING && oldZombiePhase != ZombiePhase::PHASE_SQUASH_RISING) {
                        aZombie->StopEating();
                        aZombie->PlayZombieReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 20, 12.0f);
                        aZombie->mHasHead = false;

                        Reanimation *aHeadReanim = aZombie->mApp->ReanimationTryToGet(aZombie->mSpecialHeadReanimID);
                        if (aHeadReanim) {
                            aHeadReanim->PlayReanim("anim_jumpup", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 20, 24.0f);
                            aHeadReanim->mRenderOrder = aZombie->mRenderOrder + 1;
                            aHeadReanim->SetPosition(aZombie->mPosX + 6.0f, aZombie->mPosY - 21.0f);
                            aHeadReanim->OverrideScale(0.75f, 0.75f);
                            aHeadReanim->mOverlayMatrix.m10 = 0.0f;
                        }
                        Reanimation *aBodyReanim = aZombie->mApp->ReanimationTryToGet(aZombie->mBodyReanimID);
                        if (aBodyReanim) {
                            ReanimatorTrackInstance *aTrackInstance = aBodyReanim->GetTrackInstanceByName("anim_head1");
                            if (aTrackInstance) {
                                AttachmentDetach(aTrackInstance->mAttachmentID);
                            }
                        }
                    } else if (aZombie->mZombiePhase == ZombiePhase::PHASE_SQUASH_FALLING && oldZombiePhase != ZombiePhase::PHASE_SQUASH_FALLING) {
                        Reanimation *aHeadReanim = aZombie->mApp->ReanimationTryToGet(aZombie->mSpecialHeadReanimID);
                        if (aHeadReanim) {
                            aHeadReanim->PlayReanim("anim_jumpdown", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 60.0f);
                        }
                    }
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_DO_SPECIAL: {
            auto *eventZombieDoSpecial = static_cast<const U16_Event *>(event);
            uint16_t serverZombieID = eventZombieDoSpecial->data;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->DoSpecial();
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_EXPLORER_BURN_PLANT: {
            auto *eventExplorerBurnPlant = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = eventExplorerBurnPlant->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                Plant *aPlant = nullptr;
                uint16_t clientPlantID = 0;
                if (homura::FindInMap(serverPlantIDMap, eventExplorerBurnPlant->data2, clientPlantID)) {
                    aPlant = mPlants.DataArrayGet(clientPlantID);
                }
                aZombie->ExplorerBurnPlant(aPlant);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_SQUISH_ALL_IN_SQUARE: {
            auto *eventZombieSquishAllInSquare = static_cast<const U16UNI32_Event *>(event);
            uint16_t serverZombieID = eventZombieSquishAllInSquare->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                for (Plant *aPlant = nullptr; IteratePlants(aPlant);) {
                    if (aPlant->mRow == eventZombieSquishAllInSquare->data2.u8x4.u8_2 && aPlant->mPlantCol == eventZombieSquishAllInSquare->data2.u8x4.u8_1) {
                        if (eventZombieSquishAllInSquare->data2.u8x4.u8_3 == ZombieAttackType::ATTACKTYPE_DRIVE_OVER && aPlant->IsSpiky()) {
                            continue;
                        }
                        if (aPlant->mSeedType != SeedType::SEED_SPIKEROCK) {
                            aPlant->Squish();
                        }
                    }
                }
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_TAKE_DAMAGE: {
            auto *eventZombieTakeDmg = static_cast<const U16U16U8_Event *>(event);
            int damage = eventZombieTakeDmg->data2;
            unsigned int damageFlags = eventZombieTakeDmg->data3;
            uint16_t serverZombieID = eventZombieTakeDmg->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->TakeDamage_Origin(damage, damageFlags);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_DROP_HEAD: {
            auto *eventZombieDropHead = static_cast<const U16U16_Event *>(event);
            uint16_t serverZombieID = eventZombieDropHead->data1;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->DropHead_Origin(eventZombieDropHead->data2);
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_MOW_DOWN: {
            auto *eventZombieMowDown = static_cast<const U16_Event *>(event);
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, eventZombieMowDown->data, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                aZombie->MowDown_Original();
            }
        } break;
        case EVENT_SERVER_BOARD_ZOMBIE_WIN: {
            auto *zombieWinEvent = static_cast<const U16_Event *>(event);
            uint16_t serverZombieID = zombieWinEvent->data;
            uint16_t clientZombieID = 0;
            if (homura::FindInMap(serverZombieIDMap, serverZombieID, clientZombieID)) {
                Zombie *aZombie = mZombies.DataArrayGet(clientZombieID);
                ZombiesWon(aZombie);
            }
        } break;
        case EVENT_SERVER_BOARD_LAWNMOWER_START: {
            auto *eventLawnMowerStart = static_cast<const U16_Event *>(event);
            uint16_t aRow = eventLawnMowerStart->data;
            LawnMower *aLawnMower = nullptr;
            while (IterateLawnMowers(aLawnMower)) {
                if (aLawnMower && aLawnMower->mRow == aRow) {
                    old_LawnMower_StartMower(aLawnMower);
                }
            }
        } break;
        case EVENT_SERVER_BOARD_TAKE_SUNMONEY: {
            auto *event1 = static_cast<const I16_Event *>(event);
            mSunMoney1 = event1->data;
        } break;
        case EVENT_SERVER_BOARD_TAKE_DEATHMONEY: {
            auto *event1 = static_cast<const I16_Event *>(event);
            mDeathMoney = event1->data;
        } break;
        case EVENT_SERVER_BOARD_SEEDPACKET_WASPLANTED: {
            auto *event1 = static_cast<const U8U8_Event *>(event);
            SeedBank *theSeedBank = event1->data2 ? mSeedBank[0] : mSeedBank[1];
            SeedPacket *seedPacket = &theSeedBank->mSeedPackets[event1->data1];
            seedPacket->Deactivate();
            seedPacket->WasPlanted(0);
        } break;
        case EVENT_SERVER_BOARD_SYNC_ID: {
            auto *eventSync = static_cast<const U16UNI32_Event *>(event);

            switch (eventSync->data2.u8x4.u8_1) {
                case 0: // Plant
                {
                    Plant *plant = nullptr;
                    while (IteratePlants(plant)) {
                        if (plant->mSeedType == eventSync->data2.u8x4.u8_2 && plant->mRow == eventSync->data2.u8x4.u8_3 && plant->mPlantCol == eventSync->data2.u8x4.u8_4) {
                            serverPlantIDMap[eventSync->data1] = uint16_t(mPlants.DataArrayGetID(plant));
                        }
                    }
                }

                break;
                case 1: // GridItem
                {
                    GridItem *gridItem = nullptr;
                    while (IterateGridItems(gridItem)) {
                        if (gridItem->mGridItemType == eventSync->data2.u8x4.u8_2 && gridItem->mGridX == eventSync->data2.u8x4.u8_3 && gridItem->mGridY == eventSync->data2.u8x4.u8_4) {
                            serverGridItemIDMap[eventSync->data1] = uint16_t(mGridItems.DataArrayGetID(gridItem));
                        }
                    }
                } break;
                default:
                    break;
            }

        } break;

        case EVENT_SERVER_BOARD_START_LEVEL: {
            // 与主机端同步置0
            mMainCounter = 0;
            serverPlantIDMap.clear();
            serverZombieIDMap.clear();
            serverCoinIDMap.clear();
            serverGridItemIDMap.clear();

        } break;
        case EVENT_SERVER_BOARD_CONCEDE: {
            mApp->mMusic->StopAllMusic();
            mApp->mSoundSystem->CancelPausedFoley();
            mApp->KillNewOptionsDialog();
            mApp->KillDialog(DIALOG_CONFIRM_IN_GAME_RESTART);
            GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
            if (serverGamepadControls->mIsZombie) {
                mApp->SetBoardResult(BoardResult::BOARDRESULT_VS_PLANT_WON);
                mApp->mGameScene = SCENE_PLANTS_WON;
            } else {
                mApp->SetBoardResult(BOARDRESULT_VS_ZOMBIE_WON);
                mApp->mGameScene = SCENE_ZOMBIES_WON;
            }

            mApp->ShowVSResultsScreen();
            mApp->mVSResultsMenu->InitFromBoard(this);
            mApp->KillBoard();
        } break;
        case EVENT_SERVER_BOARD_SHUFFLE_RANDOM_PICK: {
            auto *eventRandomPick = static_cast<const U16x6_Event *>(event);
            int isZombie = eventRandomPick->data[5];
            for (int i = 0; i < 5; ++i) {
                mSeedBank[isZombie]->mSeedPackets[i + 1].SetPacketType(SeedType(eventRandomPick->data[i]), SeedType::SEED_NONE);
            }
        } break;
        case EVENT_SERVER_BOARD_SHUFFLE_RANDOM_PICK_NEXT: {
            auto *eventRandomPickNext = static_cast<const U8U8U16U16_Event *>(event);
            int isZombie = int(eventRandomPickNext->data1);
            auto seedType = SeedType(eventRandomPickNext->data3);
            int seedIndex = eventRandomPickNext->data4;
            mSeedBank[isZombie]->mSeedPackets[seedIndex].SetPacketType(seedType, SeedType::SEED_NONE);
        } break;
        default:
            break;
    }
}

static void CheatPlacePlant(Board *theBoard, int theCol, int theRow, SeedType theSeedType, bool isImitaterPlant) {
    const int colsCount = (theSeedType == SeedType::SEED_COBCANNON) ? 8 : 9; // 玉米加农炮不种在九列
    const int width = (theSeedType == SeedType::SEED_COBCANNON) ? 2 : 1;     // 玉米加农炮宽度两列
    const int rowsCount = theBoard->StageHas6Rows() ? 6 : 5;
    const bool isIZMode = theBoard->mApp->IsIZombieLevel();

    auto PlacePlant = [theBoard, theSeedType, isImitaterPlant, isIZMode](int theCol, int theRow) {
        Plant *aPlant = theBoard->AddPlant(theCol, theRow, theSeedType, (isImitaterPlant ? SeedType::SEED_IMITATER : SeedType::SEED_NONE), 0, true);
        if (isImitaterPlant) {
            aPlant->SetImitaterFilterEffect();
        }
        if (isIZMode) {
            theBoard->mChallenge->IZombieSetupPlant(aPlant);
        }
    };

    // 全场
    if (theCol == 9 && theRow == 6) {
        for (int col = 0; col < colsCount; col += width) {
            for (int row = 0; row < rowsCount; row++) {
                PlacePlant(col, row);
            }
        }
    }
    // 单行
    else if (theCol == 9 && theRow < 6) {
        for (int col = 0; col < colsCount; col += width) {
            PlacePlant(col, theRow);
        }
    }
    // 单列
    else if (theCol < 9 && theRow == 6) {
        for (int row = 0; row < rowsCount; row++) {
            PlacePlant(theCol, row);
        }
    }
    // 单格
    else if (theCol < colsCount && theRow < rowsCount) {
        PlacePlant(theCol, theRow);
    }
}

static void CheatPlaceZombie(Board *theBoard, int theCol, int theRow, ZombieType theZombieType) {
    if (theZombieType == ZombieType::ZOMBIE_BOSS) {
        theBoard->AddZombieInRow(theZombieType, 0, 0, true);
        return;
    }

    const int colsCount = 9;
    const int rowsCount = theBoard->StageHas6Rows() ? 6 : 5;

    // 僵尸出生线
    if (theCol == 10 && theRow == 6) {
        for (int row = 0; row < rowsCount; ++row) {
            theBoard->AddZombieInRow(theZombieType, row, theBoard->mCurrentWave, true);
        }
    }
    // 僵尸出生点
    else if (theCol == 10 && theRow < 6) {
        theBoard->AddZombieInRow(theZombieType, theRow, theBoard->mCurrentWave, true);
    }
    // 全场
    else if (theCol == 9 && theRow == 6) {
        for (int col = 0; col < colsCount; ++col) {
            for (int row = 0; row < rowsCount; ++row) {
                theBoard->mChallenge->IZombiePlaceZombie(theZombieType, col, row);
            }
        }
    }
    // 单行
    else if (theCol == 9 && theRow < 6) {
        for (int col = 0; col < colsCount; ++col) {
            theBoard->mChallenge->IZombiePlaceZombie(theZombieType, col, theRow);
        }
    }
    // 单列
    else if (theCol < 9 && theRow == 6) {
        for (int row = 0; row < rowsCount; ++row) {
            theBoard->mChallenge->IZombiePlaceZombie(theZombieType, theCol, row);
        }
    }
    // 单格
    else if (theCol < colsCount && theRow < rowsCount) {
        theBoard->mChallenge->IZombiePlaceZombie(theZombieType, theCol, theRow);
    }
}

static void CheatPlaceGraveStone(Board *theBoard, int theCol, int theRow) {
    const int colsCount = 9;
    const int rowsCount = theBoard->StageHas6Rows() ? 6 : 5;

    // 全场
    if (theCol == 9 && theRow == 6) {
        for (GridItem *aGridItem = nullptr; theBoard->IterateGridItems(aGridItem);) {
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE) {
                aGridItem->GridItemDie();
            }
        }
        for (int col = 0; col < colsCount; ++col) {
            for (int row = 0; row < rowsCount; ++row) {
                theBoard->mChallenge->GraveDangerSpawnGraveAt(col, row);
            }
        }
    }
    // 单行
    else if (theCol == 9 && theRow < 6) {
        for (GridItem *aGridItem = nullptr; theBoard->IterateGridItems(aGridItem);) {
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE && aGridItem->mGridY == theRow) {
                aGridItem->GridItemDie();
            }
        }
        for (int col = 0; col < colsCount; ++col) {
            theBoard->mChallenge->GraveDangerSpawnGraveAt(col, theRow);
        }
    }
    // 单列
    else if (theCol < 9 && theRow == 6) {
        for (GridItem *aGridItem = nullptr; theBoard->IterateGridItems(aGridItem);) {
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE && aGridItem->mGridX == theCol) {
                aGridItem->GridItemDie();
            }
        }
        for (int row = 0; row < rowsCount; ++row) {
            theBoard->mChallenge->GraveDangerSpawnGraveAt(theCol, row);
        }
    }
    // 单格
    else if (theCol < 9 && theRow < 6) {
        if (theBoard->GetGraveStoneAt(theCol, theRow) == nullptr) {
            theBoard->mChallenge->GraveDangerSpawnGraveAt(theCol, theRow);
        }
    }
}

static void CheatSetZombieSpawn(Board *theBoard, const bool (&theZombiesToSpawn)[ZombieType::EXTENDED_NUM_ZOMBIE_TYPES]) {
    int typesCount = 0;                                   // 已选僵尸种类数
    int typesList[ZombieType::EXTENDED_NUM_ZOMBIE_TYPES]; // 已选僵尸种类列表

    // 将僵尸代号放入种类列表, 并更新已选种类数
    for (int type = 0; type < ZombieType::EXTENDED_NUM_ZOMBIE_TYPES; ++type) {
        if (theZombiesToSpawn[type] && type != ZombieType::ZOMBIE_BUNGEE) // 飞贼僵尸不应作为正常僵尸出现在出怪列表中
        {
            typesList[typesCount++] = type;
        }
    }

    // 设置出怪需要选择至少 1 种除飞贼以外的僵尸
    if (typesCount < 1) {
        return;
    }

    // 自然出怪
    if (choiceSpawnMode == 1) {
        // 清空出怪列表
        for (int wave = 0, numWaves = theBoard->mNumWaves; wave < numWaves; ++wave) {
            for (int index = 0; index < MAX_ZOMBIES_IN_WAVE; ++index) {
                theBoard->mZombiesInWave[wave][index] = ZombieType::ZOMBIE_INVALID;
            }
        }

        // 设置游戏中的僵尸允许类型
        for (int type = 0; type < ZombieType::EXTENDED_NUM_ZOMBIE_TYPES; ++type) {
            theBoard->mZombieAllowed[type] = theZombiesToSpawn[type];
        }
        theBoard->mZombieAllowed[ZombieType::ZOMBIE_NORMAL] = true; // 自然出怪下必须含有普通僵尸

        theBoard->PickZombieWaves(); // 由游戏生成出怪列表
    }
    // 极限出怪
    else if (choiceSpawnMode == 2) {
        int indexInLevel = 0;
        // 均匀填充出怪列表
        for (int wave = 0, numWaves = theBoard->mNumWaves; wave < numWaves; ++wave) {
            for (int indexInWave = 0; indexInWave < MAX_ZOMBIES_IN_WAVE; ++indexInWave) {
                // 使用僵尸的“关内序号”遍历设置出怪可能会比使用“波内序号”更加均匀
                theBoard->mZombiesInWave[wave][indexInWave] = ZombieType(typesList[indexInLevel % typesCount]);
                ++indexInLevel;
            }
            if (theBoard->IsFlagWave(wave)) {
                theBoard->mZombiesInWave[wave][0] = ZombieType::ZOMBIE_FLAG; // 生成旗帜僵尸
                if (theZombiesToSpawn[ZombieType::ZOMBIE_BUNGEE]) {
                    // 生成飞贼僵尸
                    for (int index : {1, 2, 3, 4})
                        theBoard->mZombiesInWave[wave][index] = ZombieType::ZOMBIE_BUNGEE;
                }
            }
        }
        // 不能只出雪人僵尸, 在第一波生成 1 只普通僵尸
        if (theZombiesToSpawn[ZombieType::ZOMBIE_YETI] && typesCount == 1) {
            theBoard->mZombiesInWave[0][0] = ZombieType::ZOMBIE_NORMAL;
        }
    }

    // 重新生成选卡预览僵尸
    if (theBoard->mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO) {
        theBoard->RemoveCutsceneZombies();
        theBoard->mCutScene->mPlacedZombies = false;
    }
}


void Board::Update() {
    isMainMenu = false;

    if (requestDrawButterInCursor) {
        Zombie *aZombieUnderButter = ZombieHitTest(mGamepadControls[1]->mCursorPositionX, mGamepadControls[1]->mCursorPositionY, 1);
        if (aZombieUnderButter != nullptr) {
            aZombieUnderButter->AddButter();
        }
    }

    if (requestDrawShovelInCursor) {
        Plant *plantUnderShovel = ToolHitTest(mGamepadControls[0]->mCursorPositionX, mGamepadControls[0]->mCursorPositionY);
        if (plantUnderShovel != nullptr) {
            // 让这个植物高亮
            plantUnderShovel->mEatenFlashCountdown = 1000; // 1000是为了不和其他闪光效果冲突
        }
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN && mApp->mGameScene == GameScenes::SCENE_PLAYING) {
        Zombie *aZombieUnderButter_1P = ZombieHitTest(mGamepadControls[0]->mCursorPositionX, mGamepadControls[0]->mCursorPositionY, 1);
        if (aZombieUnderButter_1P != nullptr) {
            aZombieUnderButter_1P->AddButter();
        }
        if (mGamepadControls[1]->mGamepadIndex != -1) {
            Zombie *aZombieUnderButter_2P = ZombieHitTest(mGamepadControls[1]->mCursorPositionX, mGamepadControls[1]->mCursorPositionY, 1);
            if (aZombieUnderButter_2P != nullptr) {
                aZombieUnderButter_2P->AddButter();
            }
        }
    }
    // GameButton_Update(mBoardMenuButton);
    if (isKeyboardTwoPlayerMode) {
        mGamepadControls[0]->mIsInShopSeedBank = false;
        mGamepadControls[1]->mIsInShopSeedBank = false;
        mGamepadControls[0]->mGamepadIndex = 0;
        mGamepadControls[1]->mGamepadIndex = 1;
        mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
        mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
    }

    if (!mPaused && mTimeStopCounter <= 0) {
        if (speedUpMode > 0 && !IsOnlineServerModeActive() && !gIsReplayMode) {
            switch (speedUpMode) {
                case 1:
                    if (speedUpCounter++ % 5 == 0) {
                        SpeedUpUpdate();
                    }
                    break;
                case 2:
                    if (speedUpCounter++ % 2 == 0) {
                        SpeedUpUpdate();
                    }
                    break;
                case 3:
                    SpeedUpUpdate();
                    break;
                case 4:
                    SpeedUpUpdate();
                    if (speedUpCounter++ % 2 == 0) {
                        SpeedUpUpdate();
                    }
                    break;
                case 5:
                    for (int i = 0; i < 2; ++i) {
                        SpeedUpUpdate();
                    }
                    break;
                case 6:
                    for (int i = 0; i < 4; ++i) {
                        SpeedUpUpdate();
                    }
                    break;
                case 7:
                    for (int i = 0; i < 9; ++i) {
                        SpeedUpUpdate();
                    }
                    break;
                default:
                    break;
            }
        }

        if (gIsReplayMode && replay::IsPlaybackActive() && !replay::IsPlaybackPaused()) {
            const int extraUpdates = std::max(0, static_cast<int>(replay::GetPlaybackSpeedMultiplier()) - 1);
            for (int i = 0; i < extraUpdates; ++i) {
                replay::AdvancePlaybackOneTick();
                if (mApp->mGameScene == GameScenes::SCENE_LEVEL_INTRO && mCutScene != nullptr) {
                    mCutScene->Update();
                } else {
                    SpeedUpUpdate();
                }
            }
        }

        // 为夜晚泳池场景补全泳池反射闪光特效
        // if ( this->mBackground == BackgroundType::BACKGROUND_4_FOG && this->mPoolSparklyParticleID == 0 && this->mDrawCount > 0 ){
        // TodParticleSystem * poolSparklyParticle = AddTodParticle(this->mApp, 450.0, 295.0, 220000, a::PARTICLE_POOL_SPARKLY);
        // this->mPoolSparklyParticleID = LawnApp_ParticleGetID(this->mApp, poolSparklyParticle);
        // }
    }

    if (mReplayControlsWidget != nullptr) {
        mReplayControlsWidget->SetVisible(gIsReplayMode && replay::IsPlaybackActive());
    }

    if (clearAllPlant) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            RemoveAllPlants();
        }
        clearAllPlant = false;
    }

    if (clearAllZombies) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            RemoveAllZombies();
        }
        clearAllZombies = false;
    }

    if (clearAllGraves) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            for (GridItem *aGridItem = nullptr; IterateGridItems(aGridItem);) {
                if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE) {
                    aGridItem->GridItemDie();
                }
            }
        }
        clearAllGraves = false;
    }

    if (clearAllMowers) {
        if (mApp->mGameScene == GameScenes::SCENE_PLAYING && !IsOnlineServerModeActive() && !gIsReplayMode) {
            RemoveAllMowers();
        }
        clearAllMowers = false;
    }

    if (recoverAllMowers) {
        if (mApp->mGameScene == GameScenes::SCENE_PLAYING && !IsOnlineServerModeActive() && !gIsReplayMode) {
            ResetLawnMowers();
        }
        recoverAllMowers = false;
    }

    // 魅惑所有僵尸
    if (hypnoAllZombies) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            for (Zombie *aZombie = nullptr; IterateZombies(aZombie);) {
                if (aZombie->mZombieType != ZombieType::ZOMBIE_BOSS) {
                    aZombie->mMindControlled = true;
                }
            }
        }
        hypnoAllZombies = false;
    }

    if (freezeAllZombies) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            for (Zombie *aZombie = nullptr; IterateZombies(aZombie); aZombie->HitIceTrap()) {}
        }
        freezeAllZombies = false;
    }

    if (startAllMowers) {
        if (mApp->mGameScene == GameScenes::SCENE_PLAYING && !IsOnlineServerModeActive() && !gIsReplayMode) {
            for (LawnMower *aLawnMower = nullptr; IterateLawnMowers(aLawnMower); aLawnMower->StartMower()) {}
        }
        startAllMowers = false;
    }

    // 修改卡槽
    if (setSeedPacket) {
        if (choiceSeedType != SeedType::SEED_NONE && !IsOnlineServerModeActive() && !gIsReplayMode) {
            if (SeedBank *aSeedBank = mSeedBank[targetSeedBank]) {
                SeedPacket &aSeedPacket = aSeedBank->mSeedPackets[choiceSeedPacketIndex];
                if (aSeedBank->mIsZombie) {
                    // IZ模式中用不了墓碑
                    if (Challenge::IsZombieSeedType(choiceSeedType) && (choiceSeedType != SeedType::SEED_ZOMBIE_GRAVESTONE || mApp->IsVSMode())) {
                        aSeedPacket.mPacketType = choiceSeedType;
                    }
                } else if (choiceSeedType < SeedType::NUM_SEED_TYPES) {
                    aSeedPacket.mPacketType = isImitaterSeed ? SeedType::SEED_IMITATER : choiceSeedType;
                    aSeedPacket.mImitaterType = isImitaterSeed ? choiceSeedType : SeedType::SEED_NONE;
                }
            }
        }
        setSeedPacket = false;
    }

    if (passNowLevel) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            mLevelComplete = true;
            mApp->mBoardResult = mApp->mGameMode == GameMode::GAMEMODE_MP_VS ? BoardResult::BOARDRESULT_VS_PLANT_WON : BoardResult::BOARDRESULT_WON;
        }
        passNowLevel = false;
    }

    // 布置选择阵型
    if (layChoseFormation) // 用按钮触发, 防止进入游戏时自动布阵
    {
        if (formationId >= 0 && !IsOnlineServerModeActive() && !gIsReplayMode) {
            formation::ApplyFormation(this, formation::GetBuiltinFormationStr(formationId));
        }
        layChoseFormation = false;
    }

    // 布置粘贴阵型
    if (layPastedFormation) {
        if (!customFormation.empty() && !IsOnlineServerModeActive() && !gIsReplayMode) {
            formation::ApplyFormation(this, customFormation);
        }
        layPastedFormation = false;
    }

    // 放置植物
    if (gCheatPlacePlant) {
        if (gCheatPlacePlantType != SeedType::SEED_NONE && !IsOnlineServerModeActive() && !gIsReplayMode) {
            CheatPlacePlant(this, gCheatPlaceColumn, gCheatPlaceRow, gCheatPlacePlantType, gCheatIsPlaceImitaterPlant);
        }
        gCheatPlacePlant = false;
    }

    // 放置僵尸
    if (gCheatPlaceZombie) {
        if (gCheatPlaceZombieType != ZombieType::ZOMBIE_INVALID && !IsOnlineServerModeActive() && !gIsReplayMode) {
            CheatPlaceZombie(this, gCheatPlaceColumn, gCheatPlaceRow, gCheatPlaceZombieType);
        }
        gCheatPlaceZombie = false;
    }

    // 放置墓碑
    if (gCheatPlaceGraveStone) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            CheatPlaceGraveStone(this, gCheatPlaceColumn, gCheatPlaceRow);
        }
        gCheatPlaceGraveStone = false;
    }

    // 放置梯子
    if (gCheatPlaceLadder) {
        // 防止选“所有行”或“所有列”的时候放置到空地
        if (gCheatPlaceColumn < 9 && gCheatPlaceRow < (StageHas6Rows() ? 6 : 5) && !IsOnlineServerModeActive() && !gIsReplayMode) {
            if (GetLadderAt(gCheatPlaceColumn, gCheatPlaceRow) == nullptr) {
                AddALadder(gCheatPlaceColumn, gCheatPlaceRow);
            }
        }
        gCheatPlaceLadder = false;
    }

    // 出怪设置
    if (buttonSetSpawn) {
        if (choiceSpawnMode > 0 && !IsOnlineServerModeActive() && !gIsReplayMode) {
            CheatSetZombieSpawn(this, gCheatZombiesToSpawn);
        }
        buttonSetSpawn = false;
    }

    UpdateButtons();
    old_Board_Update(this);
    vsai::Update(this);
}

int Board::GetNumWavesPerFlag() const {
    // 修改此函数，以做到在进度条上正常绘制旗帜波的旗帜。
    if (mApp->IsFirstTimeAdventureMode() && mNumWaves < 10) {
        return mNumWaves;
    }

    // 额外添加一段判断逻辑，判断关卡代码20且波数少于10的情况
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && mNumWaves < 10) {
        return mNumWaves;
    }

    return 10;
}

bool Board::IsFlagWave(int theWaveNumber) {
    // 修改此函数，以做到正常出旗帜波僵尸。
    if (!normalLevel) {
        return old_Board_IsFlagWave(this, theWaveNumber);
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS)
        return true;

    if (mApp->IsFirstTimeAdventureMode() && mLevel == 1)
        return false;

    int aWavesPerFlag = GetNumWavesPerFlag();
    return theWaveNumber % aWavesPerFlag == aWavesPerFlag - 1;
}

int Board::GetGraveStonesCount() {
    int aCount = 0;

    GridItem *aGridItem = nullptr;
    while (IterateGridItems(aGridItem)) {
        if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE) {
            aCount++;
        }
    }

    return aCount;
}

void Board::SpawnZombiesFromGraves() {
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_WAR_AND_PEAS || mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_WAR_AND_PEAS_2)
        return;

    if (StageHasRoof()) {
        SpawnZombiesFromSky();
        TriggerVibration(VibrationEffect::VIBRATION_BUNGEE_LANDING);
    } else if (StageHasPool()) {
        SpawnZombiesFromPool();
        TriggerVibration(VibrationEffect::VIBRATION_ZOMBIE_RISE_FROM_POOL);
        return;
    }

    int aZombiePoints = GetGraveStonesCount();
    GridItem *aGridItem = nullptr;
    while (IterateGridItems(aGridItem)) {
        if (aGridItem->mGridItemType != GridItemType::GRIDITEM_GRAVESTONE || aGridItem->mGridItemCounter < 100) {
            continue;
        }
        if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_GRAVE_DANGER && Rand(mNumWaves) > mCurrentWave) {
            continue;
        }

        ZombieType aZombieType = PickGraveRisingZombieType(aZombiePoints);
        Zombie *aZombie = AddZombie(aZombieType, mCurrentWave, false);
        if (aZombie == nullptr) {
            return;
        }

        aZombie->RiseFromGrave(aGridItem->mGridX, aGridItem->mGridY);
        aZombiePoints -= GetZombieDefinition(aZombieType).mZombieValue;
        if (aZombieType < 1) {
            aZombiePoints = 1;
        }
    }

    TriggerVibration(VibrationEffect::VIBRATION_ZOMBIE_RISE_FROM_GRAVE);
}

void Board::SpawnZombieWave() {
    // 在联机对战模式同步大波僵尸事件
    if (mApp->IsVSMode()) {
        if (gTcpClientSocket) {
            BaseEvent event = {EventType::EVENT_SERVER_BOARD_ZOMBIE_HUGE_WAVE};
            netplay::PutEvent(event);
        }
    }

    old_Board_SpawnZombieWave(this);
}

void Board::DrawProgressMeter(Sexy::Graphics *g, int theX, int theY) {
    if (gIsReplayMode) {
        // 先画上面的空进度条
        g->DrawImageCel(Sexy::IMAGE_FLAGMETER, theX, theY + 20, 0);
        int aCelWidth = Sexy::IMAGE_FLAGMETER->GetCelWidth();
        int aCelHeight = Sexy::IMAGE_FLAGMETER->GetCelHeight();
        constexpr int aMeterWidth = 143;
        int aTotalTicks = std::max(1, replay::GetPlaybackBoardTicks());
        int aCurrentTicks = std::clamp(mMainCounter, 0, aTotalTicks);
        int aClipWidth = aCurrentTicks * aMeterWidth / aTotalTicks;
        Rect aSrcRect(aCelWidth - aClipWidth - 7, aCelHeight, aClipWidth, aCelHeight);
        Rect aDstRect(aCelWidth - aClipWidth + theX - 7, theY + 20, aClipWidth, aCelHeight);
        g->DrawImage(Sexy::IMAGE_FLAGMETER, aDstRect, aSrcRect);

        const int aMeterRight = aCelWidth + theX - 7;
        const int aMeterTop = theY;
        const int aBoardStartTick = std::max(0, replay::GetPlaybackBoardStartTick());
        auto getMarkerX = [&](int theReplayTick) {
            int aBoardTick = std::clamp(theReplayTick - aBoardStartTick, 0, aTotalTicks);
            return aMeterRight - aBoardTick * aMeterWidth / aTotalTicks;
        };
        auto drawMarker = [&](Sexy::Image *theImage, int theReplayTick, int theWidth, int theHeight, int theOffsetX, int theOffsetY) {
            if (theImage == nullptr) {
                return;
            }
            int aMarkerX = getMarkerX(theReplayTick) - theWidth / 2 + theOffsetX;
            int aMarkerY = aMeterTop + theOffsetY;
            Rect aSrc(0, 0, theImage->mWidth, theImage->mHeight);
            Rect aDst(aMarkerX, aMarkerY, theWidth, theHeight);
            g->DrawImage(theImage, aDst, aSrc);
        };

        for (int aTick : replay::GetPlaybackGridItemDieTicks()) {
            drawMarker(addonImages.zombie_target, aTick, 24, 32, 0, -10);
            drawMarker(IMAGE_MP_TARGETS_X, aTick, 18, 24, 0, -5);
        }
        for (int aTick : replay::GetPlaybackLawnMowerStartTicks()) {
            drawMarker(mApp->mReanimatorCache->mLawnMowers[LAWNMOWER_LAWN], aTick, 40, 44, 0, 34);
            drawMarker(IMAGE_MP_TARGETS_X, aTick, 18, 20, 4, 39);
        }

        return;
    }

    if (normalLevel) {
        // 修改此函数，以做到在进度条上正常绘制旗帜波的旗帜。
        if (mApp->IsAdventureMode() && ProgressMeterHasFlags()) {
            mApp->mGameMode = GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON; // 修改关卡信息为非冒险模式
            old_Board_DrawProgressMeter(this, g, theX, theY);
            mApp->mGameMode = GameMode::GAMEMODE_ADVENTURE; // 再把关卡信息改回冒险模式
            return;
        }
        old_Board_DrawProgressMeter(this, g, theX, theY);
    } else {
        old_Board_DrawProgressMeter(this, g, theX, theY);
    }
}

bool Board::IsLevelDataLoaded() {
    // 确保在开启原版难度时，所有用到levels.xml的地方都不生效
    if (normalLevel)
        return false;

    return old_Board_IsLevelDataLoaded(this);
}

bool Board::NeedSaveGame() {
    // 可以让结盟关卡存档
    if (mApp->IsCoopMode()) {
        return mApp->mGameScene == SCENE_PLAYING;
    }
    return old_Board_NeedSaveGame(this);
}

void Board::DrawHammerButton(Sexy::Graphics *g, LawnApp *theApp) {
    if (!gKeyboardMode)
        return;
    float tmp = g->mTransY;
    Rect rect = GetButterButtonRect();
    g->DrawImage(Sexy::IMAGE_SHOVELBANK, rect.mX, rect.mY);
    g->DrawImage(Sexy::IMAGE_HAMMER_ICON, rect.mX - 7, rect.mY - 3);

    if (theApp->HasGamepad() || (theApp->mGamePad1IsOn && theApp->mGamePad2IsOn)) {
        g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS, rect.mX + 36, rect.mY + 40, 2);
    } else {
        g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS2, rect.mX + 36, rect.mY + 40, 2);
    }
    g->SetColorizeImages(false);
    g->mTransY = tmp;
}

void Board::DrawButterButton(Sexy::Graphics *g, LawnApp *theApp) {
    if (!theApp->IsCoopMode()) {
        if (!theApp->IsAdventureMode())
            return;
        if (theApp->mSecondPlayerGamepadIndex == -1)
            return;
    }
    float tmp = g->mTransY;
    Rect rect = GetButterButtonRect();
    g->DrawImage(Sexy::IMAGE_SHOVELBANK, rect.mX, rect.mY);
    if (mChallenge->mChallengeState == ChallengeState::STATECHALLENGE_SHOVEL_FLASHING) {
        Color color = GetFlashingColor(mMainCounter, 75);
        g->SetColorizeImages(true);
        g->SetColor(color);
    }
    // 实现拿着黄油的时候不在栏内绘制黄油
    if (!requestDrawButterInCursor) {
        g->DrawImage(Sexy::IMAGE_BUTTER_ICON, rect.mX - 7, rect.mY - 3);
    }
    if (gKeyboardMode) {
        g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS, rect.mX + 36, rect.mY + 40, 2);
    }
    g->SetColorizeImages(false);
    g->mTransY = tmp;
}

void Board::DrawShovelButton(Sexy::Graphics *g, LawnApp *theApp) {
    // 实现玩家拿着铲子时不在ShovelBank中绘制铲子、实现在对战模式中添加铲子按钮

    if (theApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        // LOGD("%d %d",rect[0],rect[1]);
        // return;  原版游戏在此处就return了，所以对战中不绘制铲子按钮。
        if (gKeyboardMode)
            return;


        g->DrawImage(Sexy::IMAGE_SHOVELBANK, gTouchVSShovelRect.mX, gTouchVSShovelRect.mY);
        if (!requestDrawShovelInCursor) {
            g->DrawImage(Sexy::IMAGE_SHOVEL, gTouchVSShovelRect.mX - 7, gTouchVSShovelRect.mY - 3);
        }
        return;
    }

    float tmp = g->mTransY;
    Rect rect = GetShovelButtonRect();
    g->DrawImage(Sexy::IMAGE_SHOVELBANK, rect.mX, rect.mY);

    if (mChallenge->mChallengeState == ChallengeState::STATECHALLENGE_SHOVEL_FLASHING) {
        Color color = GetFlashingColor(mMainCounter, 75);
        g->SetColorizeImages(true);
        g->SetColor(color);
    }

    // 实现拿着铲子的时候不在栏内绘制铲子
    if (!requestDrawShovelInCursor) {
        if (theApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
            Challenge *challenge = mChallenge;
            if (challenge->mChallengeState == ChallengeState::STATECHALLENGE_NORMAL && theApp->mGameScene == GameScenes::SCENE_PLAYING) {
                g->DrawImage(Sexy::IMAGE_ZEN_MONEYSIGN, rect.mX - 7, rect.mY - 3);
            } else {
                g->DrawImage(Sexy::IMAGE_SHOVEL, rect.mX - 7, rect.mY - 3);
            }
        } else {
            g->DrawImage(Sexy::IMAGE_SHOVEL, rect.mX - 7, rect.mY - 3);
        }
    }

    if (gKeyboardMode) {
        if (theApp->IsCoopMode()) {
            g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS, rect.mX + 40, rect.mY + 40, 1);
        } else {
            if (theApp->HasGamepad() || (theApp->mGamePad1IsOn && theApp->mGamePad2IsOn)) {
                g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS, rect.mX + 50, rect.mY + 40, 1);
            } else {
                g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS2, rect.mX + 50, rect.mY + 40, 1);
            }
        }
    }

    g->SetColorizeImages(false);
    g->mTransY = tmp;
}

void Board::DrawShovel(Sexy::Graphics *g) {
    // 实现拿着铲子、黄油的时候不在栏内绘制铲子、黄油，同时为对战模式添加铲子按钮
    GameMode mGameMode = mApp->mGameMode;
    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON)
        return;

    if (mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM || mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) { // 如果是花园或智慧树
        DrawZenButtons(g);
        return;
    }

    if (mShowHammer) { // 绘制锤子按钮
        DrawHammerButton(g, mApp);
    }

    if (mShowButter) { // 绘制黄油按钮
        DrawButterButton(g, mApp);
    }

    if (mShowShovel) { // 绘制铲子按钮
        DrawShovelButton(g, mApp);
    }
}

void Board::Draw(Sexy::Graphics *g) {
    old_Board_Draw(this, g);

    if (mApp->IsVSMode()) {
        Color aColor = Color(0, 205, 0, 255);

        if (gIsReplayMode) {

        } else if (gTcpConnected) {
            if (gNetDelayNow == 0) {
                pvzstl::string status = TodStringTranslate(gIsServerModeSpectator ? "[SPECTATE]" : "[VS_STATUS_IN_ROOM]");
                TodDrawString(g, GetServerModeTransportSuffix() + std::move(status), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            } else {
                pvzstl::string fmt = TodStringTranslate("[VS_STATUS_IN_ROOM_MS_FMT]");
                pvzstl::string delayText = gIsServerModeSpectator ? StrFormat("%s %dms", TodStringTranslate("[SPECTATE]").c_str(), gNetDelayNow * 10) : StrFormat(fmt.c_str(), gNetDelayNow * 10);
                TodDrawString(g, GetServerModeTransportSuffix() + std::move(delayText), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            }
        } else if (gTcpClientSocket >= 0) {
            if (gNetDelayNow == 0) {
                TodDrawString(g, GetServerModeTransportSuffix() + TodStringTranslate("[VS_STATUS_HOST]"), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            } else {
                pvzstl::string fmt = TodStringTranslate("[VS_STATUS_HOST_MS_FMT]");
                TodDrawString(g, GetServerModeTransportSuffix() + StrFormat(fmt.c_str(), gNetDelayNow * 10), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            }
        }
    }
}


void Board::PauseFromSecondPlayer(bool thePause) {
    if (mPaused == thePause)
        return;

    LOG_DEBUG("PauseFromSecondPlayer={}", thePause);

    gPauseSyncFromRemote = true;

    if (thePause) {
        mApp->PlaySample(Sexy::SOUND_PAUSE);

        // 防止重复开菜单
        if (mApp->GetDialog(Dialogs::DIALOG_NEWOPTIONS) == nullptr) {
            mApp->DoNewOptions(false, 0);
        }
    } else {
        // 防止重复关菜单
        if (mApp->GetDialog(Dialogs::DIALOG_NEWOPTIONS) != nullptr) {
            mApp->KillNewOptionsDialog();
        }
    }

    gPauseSyncFromRemote = false;

    // 兜底：如果 DoNewOptions / KillNewOptionsDialog 内部没有把暂停态改到目标值，
    // 再手动补一次 old_Board_Pause
    if (mPaused != thePause) {
        old_Board_Pause(this, thePause);
    }
}


void Board::Pause(bool thePause) {
    //    LOG_DEBUG("Pause={}, remoteSync={}", thePause, gPauseSyncFromRemote);

    if (gIsReplayMode && !gPauseSyncFromRemote) {
        replay::SetPlaybackPaused(thePause);
        if (!thePause) {
            gReplayPauseByMenu = false;
        }
        return;
    }

    // Spectator is read-only: block local pause/resume attempts.
    if (gIsServerModeSpectator && !gPauseSyncFromRemote) {
        //        LOG_DEBUG("Pause blocked for spectator");
        return;
    }

    if (mPaused == thePause)
        return;

    if (gIsReplayMode && !thePause) {
        gReplayPauseByMenu = false;
    }

    // 只有“本地主动触发”的暂停/恢复才发网络包
    if (!gPauseSyncFromRemote && mApp->mGameMode == GAMEMODE_MP_VS && !mApp->mVSSetupMenu) {

        if (gTcpConnected) {
            U8_Event event = {{EventType::EVENT_CLIENT_BOARD_PAUSE}, thePause};
            netplay::PutEvent(event);
        }

        if (gTcpClientSocket >= 0) {
            U8_Event event = {{EventType::EVENT_SERVER_BOARD_PAUSE}, thePause};
            netplay::PutEvent(event);
        }
    }

    old_Board_Pause(this, thePause);
}

void Board::AddSecondPlayer(int a2) {
    // 去除加入2P时的声音

    // (*(void (**)(int, int, int))(*(uint32_t *)this[69] + 680))(this[69], Sexy::SOUND_CHIME, 1);
    // ((void (*)(int *, const char *, int))loc_2F098C)(v2 + 25, "[P2_JOINED]", 11);
    mUnkIntSecondPlayer1 = 3;
    mUnkBoolSecondPlayer = false;
    mUnkIntSecondPlayer2 = 0;
}

bool Board::IsLastStandFinalStage() const {
    // 无尽坚不可摧
    if (endlessLastStand && !IsOnlineServerModeActive() && !gIsReplayMode)
        return false;

    return mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && mChallenge->mSurvivalStage == 4;
}

Plant *Board::GetFlowerPotAt(int theGridX, int theGridY) {
    // 修复 屋顶关卡加农炮无法种植在第三第四列的组合上
    Plant *aPlant = nullptr;
    while (IteratePlants(aPlant)) {
        if (aPlant->mSeedType == SeedType::SEED_FLOWERPOT && aPlant->mRow == theGridY && aPlant->mPlantCol == theGridX && !aPlant->NotOnGround()) {
            return aPlant;
        }
    }

    return nullptr;
}

Plant *Board::GetPumpkinAt(int theGridX, int theGridY) {
    Plant *aPlant = nullptr;
    while (IteratePlants(aPlant)) {
        if (aPlant->mSeedType == SeedType::SEED_PUMPKINSHELL && aPlant->mRow == theGridY && aPlant->mPlantCol == theGridX && !aPlant->NotOnGround()) {
            return aPlant;
        }
    }

    return nullptr;
}

void Board::DoPlantingEffects(int theGridX, int theGridY, Plant *thePlant) {
    int num = GridToPixelX(theGridX, theGridY) + 41;
    int num2 = GridToPixelY(theGridX, theGridY) + 74;
    SeedType mSeedType = thePlant->mSeedType;
    if (mSeedType == SeedType::SEED_LILYPAD) {
        num2 += 15;
    } else if (mSeedType == SeedType::SEED_FLOWERPOT) {
        num2 += 30;
    }

    if (mBackground == BackgroundType::BACKGROUND_GREENHOUSE) {
        mApp->PlayFoley(FoleyType::FOLEY_CERAMIC);
        return;
    }

    if (mBackground == BackgroundType::BACKGROUND_ZOMBIQUARIUM) {
        mApp->PlayFoley(FoleyType::FOLEY_PLANT_WATER);
        return;
    }

    if (Plant::IsFlying(mSeedType)) {
        mApp->PlayFoley(FoleyType::FOLEY_PLANT);
        return;
    }

    if (IsPoolSquare(theGridX, theGridY)) {
        mApp->PlayFoley(FoleyType::FOLEY_PLANT_WATER);
        mApp->AddTodParticle(num, num2, 400000, ParticleEffect::PARTICLE_PLANTING_POOL);
        return;
    }

    mApp->PlayFoley(FoleyType::FOLEY_PLANT);
    // switch (mSeedType) {
    // case a::SEED_SUNFLOWER:
    // mApp->PlaySample( Addon_Sounds.achievement);
    // break;
    // default:
    // PlayFoley(mApp, FoleyType::Plant);
    // break;
    // }
    mApp->AddTodParticle(num, num2, 400000, ParticleEffect::PARTICLE_PLANTING);
}


void Board::InitLawnMowers() {
    if (banMower && !IsOnlineServerModeActive() && !gIsReplayMode)
        return;

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN)
        return;

    // 在泳池对战时，固定使用泳池小推车
    if (mApp->mGameMode == GAMEMODE_MP_VS && StageHasPool()) {
        bool tmp = mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_POOL_CLEANER];
        mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_POOL_CLEANER] = true;
        old_Board_InitLawnMowers(this);
        mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_POOL_CLEANER] = tmp;
        return;
    }

    // 在屋顶对战时，固定使用屋顶小推车
    if (mApp->mGameMode == GAMEMODE_MP_VS && StageHasRoof()) {
        bool tmp = mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_ROOF_CLEANER];
        mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_ROOF_CLEANER] = true;
        old_Board_InitLawnMowers(this);
        mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_ROOF_CLEANER] = tmp;
        return;
    }

    old_Board_InitLawnMowers(this);
}

void ZombiePickerInitForWave(ZombiePicker *theZombiePicker) {
    memset(theZombiePicker, 0, sizeof(ZombiePicker));
}

void ZombiePickerInit(ZombiePicker *theZombiePicker) {
    ZombiePickerInitForWave(theZombiePicker);
    memset(theZombiePicker->mAllWavesZombieTypeCount, 0, sizeof(theZombiePicker->mAllWavesZombieTypeCount));
}

void Board::PutZombieInWave(ZombieType theZombieType, int theWaveNumber, ZombiePicker *theZombiePicker) {
    mZombiesInWave[theWaveNumber][theZombiePicker->mZombieCount++] = theZombieType;
    if (theZombiePicker->mZombieCount < MAX_ZOMBIES_IN_WAVE) {
        mZombiesInWave[theWaveNumber][theZombiePicker->mZombieCount] = ZombieType::ZOMBIE_INVALID;
    }
    theZombiePicker->mZombiePoints -= GetZombieDefinition(theZombieType).mZombieValue;
    theZombiePicker->mZombieTypeCount[theZombieType]++;
    theZombiePicker->mAllWavesZombieTypeCount[theZombieType]++;
}

void Board::PickZombieWaves() {
    // 有问题，在111和115里，冒险中锤僵尸的mNumWaves从8变6了
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN && !IsLevelDataLoaded()) {
        mNumWaves = 20;
        ZombiePicker zombiePicker{};
        ZombiePickerInit(&zombiePicker);
        // ZombieType introducedZombieType = Board_GetIntroducedZombieType(this);
        for (int i = 0; i < mNumWaves; i++) {
            ZombiePickerInitForWave(&zombiePicker);
            mZombiesInWave[i][0] = ZombieType::ZOMBIE_INVALID;
            bool isFlagWave = IsFlagWave(i);
            // bool isBeforeLastWave = i == mNumWaves - 1;
            int &aZombiePoints = zombiePicker.mZombiePoints;
            aZombiePoints = i * 4 / 5 + 1;
            if (isFlagWave) {
                int num2 = std::min(zombiePicker.mZombiePoints, 8);
                zombiePicker.mZombiePoints = (int)(zombiePicker.mZombiePoints * 2.5f);
                for (int k = 0; k < num2; k++) {
                    PutZombieInWave(ZombieType::ZOMBIE_NORMAL, i, &zombiePicker);
                }
                PutZombieInWave(ZombieType::ZOMBIE_FLAG, i, &zombiePicker);
            }
            if (i == mNumWaves - 1)
                PutZombieInWave(ZombieType::ZOMBIE_GARGANTUAR, i, &zombiePicker);
            while (aZombiePoints > 0 && zombiePicker.mZombieCount < MAX_ZOMBIES_IN_WAVE) {
                ZombieType aZombieType = PickZombieType(aZombiePoints, i, &zombiePicker);
                PutZombieInWave(aZombieType, i, &zombiePicker);
            }
        }
        return;
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_POOL_PARTY && !IsLevelDataLoaded()) {
        mNumWaves = 20;
        ZombiePicker zombiePicker{};
        ZombiePickerInit(&zombiePicker);
        // ZombieType introducedZombieType = Board_GetIntroducedZombieType(this);
        for (int i = 0; i < mNumWaves; i++) {
            ZombiePickerInitForWave(&zombiePicker);
            mZombiesInWave[i][0] = ZombieType::ZOMBIE_INVALID;
            bool isFlagWave = IsFlagWave(i);
            // bool isBeforeLastWave = i == mNumWaves - 1;
            int &aZombiePoints = zombiePicker.mZombiePoints;
            aZombiePoints = i * 4 / 5 + 1;
            if (isFlagWave) {
                int num2 = std::min(zombiePicker.mZombiePoints, 8);
                zombiePicker.mZombiePoints = (int)(zombiePicker.mZombiePoints * 2.5f);
                for (int k = 0; k < num2; k++) {
                    PutZombieInWave(ZombieType::ZOMBIE_NORMAL, i, &zombiePicker);
                }
                PutZombieInWave(ZombieType::ZOMBIE_FLAG, i, &zombiePicker);
            }
            while (aZombiePoints > 0 && zombiePicker.mZombieCount < MAX_ZOMBIES_IN_WAVE) {
                ZombieType aZombieType = PickZombieType(aZombiePoints, i, &zombiePicker);
                PutZombieInWave(aZombieType, i, &zombiePicker);
            }
        }
        return;
    }

    old_Board_PickZombieWaves(this);
}

ZombieType Board::PickZombieType(int theZombiePoints, int theWaveIndex, ZombiePicker *theZombiePicker) {
    // 没有启用扩展僵尸时，保持原版抽取逻辑不变。
    bool hasExtendedZombie = false;
    for (int type = ZombieType::NUM_ZOMBIE_TYPES; type < ZombieType::EXTENDED_NUM_ZOMBIE_TYPES; ++type) {
        if (mZombieAllowed[type]) {
            hasExtendedZombie = true;
            break;
        }
    }
    if (!hasExtendedZombie) {
        return old_Board_PickZombieType(this, theZombiePoints, theWaveIndex, theZombiePicker);
    }

    TodWeightedArray aZombieWeightArray[(int)ZombieType::EXTENDED_NUM_ZOMBIE_TYPES];
    int aCount = 0;
    for (int type = 0; type < ZombieType::EXTENDED_NUM_ZOMBIE_TYPES; ++type) {
        if (!mZombieAllowed[type]) {
            continue;
        }

        // 旗帜僵尸、飞贼僵尸仍然走原来的特殊生成路径。
        if (type == ZombieType::ZOMBIE_FLAG || type == ZombieType::ZOMBIE_BUNGEE) {
            continue;
        }

        ZombieDefinition &aZombieDef = GetZombieDefinition(ZombieType(type));
        if (aZombieDef.mPickWeight <= 0 || aZombieDef.mZombieValue > theZombiePoints) {
            continue;
        }

        aZombieWeightArray[aCount].mItem = type;
        aZombieWeightArray[aCount].mWeight = aZombieDef.mPickWeight;
        ++aCount;
    }

    if (aCount <= 0) {
        return old_Board_PickZombieType(this, theZombiePoints, theWaveIndex, theZombiePicker);
    }

    return ZombieType(TodPickFromWeightedArray(aZombieWeightArray, aCount));
}

int Board::GetLiveGargantuarCount() {
    int num = 0;
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (!aZombie->mDead && aZombie->mHasHead && !aZombie->IsDeadOrDying() && aZombie->IsOnBoard()
            && (aZombie->mZombieType == ZombieType::ZOMBIE_GARGANTUAR || aZombie->mZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR)) {
            num++;
        }
    }
    return num;
}

int Board::GetLiveZombiesCount() {
    int num = 0;
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (!aZombie->mDead && aZombie->mHasHead && !aZombie->IsDeadOrDying() && aZombie->IsOnBoard()) {
            num++;
        }
    }
    return num;
}

Zombie *Board::GetLiveZombieByType(ZombieType theZombieType) {
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (!aZombie->mDead && aZombie->mHasHead && !aZombie->IsDeadOrDying() && aZombie->IsOnBoard() && !aZombie->mMindControlled && aZombie->mZombieType == theZombieType) {
            return aZombie;
        }
    }

    return nullptr;
}

bool Board::GetAliveJacksonZombie() {
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (aZombie->mZombieType == ZombieType::ZOMBIE_JACKSON && !aZombie->mDead && aZombie->mHasHead && aZombie->IsOnBoard() && !aZombie->IsDeadOrDying()) {
            return true;
        }
    }
    return false;
}

void Board::UpdateLevelEndSequence() {
    // 修复无尽最后一波僵尸出现后高级暂停无法暂停下一关的到来
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode))
        return;

    old_Board_UpdateLevelEndSequence(this);
}

void Board::UpdateGridItems() {
    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode))
        return;

    old_Board_UpdateGridItems(this);
}

void Board::MouseMove(int x, int y) {
    // 无用。鼠标指针移动、但左键未按下时调用
    // LOGD("Move%d %d", x, y);
    old_Board_MouseMove(this, x, y);
    // positionAutoFix = false;
    // LawnApp *mApp = this->mApp;
    // GameMode::GameMode mGameMode = mApp->mGameMode;
    // GamepadControls* gamepadControls1 = this->mGamepadControls[0];
    // CursorObject* cursorObject = this->mCursorObject[0];
    // CursorType::CursorType mCursorType = cursorObject->mCursorType;
    // if (mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
    // gamepadControls1->mCursorPositionX = x - 40;
    // gamepadControls1->mCursorPositionY = y - 40;
    // } else if (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN && mApp->mPlayerInfo->mPurchases[a::STORE_ITEM_GOLD_WATERINGCAN] != 0) {
    // gamepadControls1->mCursorPositionX = x - 40;
    // gamepadControls1->mCursorPositionY = y - 40;
    // }else {
    // gamepadControls1->mCursorPositionX = x;
    // gamepadControls1->mCursorPositionY = y;
    // }
}

void Board::MouseDownWithPlant(int x, int y, int theClickCount, int thePlayerIndex) {
    // 右击鼠标：放下卡牌
    if (theClickCount < 0) {
        RefreshSeedPacketFromCursor(thePlayerIndex);
        mApp->PlayFoley(FoleyType::FOLEY_DROP);
        return;
    }

    // 我是僵尸模式中，交由 Challenge 处理
    if (mApp->IsIZombieLevel()) {
        mChallenge->IZombieMouseDownWithZombie(x, y, theClickCount, thePlayerIndex);
        return;
    }

    if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM || mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON) {
        return;
    }

    SeedType aPlantingSeedType = GetSeedTypeInCursor(thePlayerIndex);
    int aGridX = PixelToGridX(x, y);
    int aGridY = PixelToGridY(x, y);

    // 不在场地内的点击：放下卡牌
    if (aGridX < 0 || aGridX >= MAX_GRID_SIZE_X || aGridY < 0 || aGridY >= MAX_GRID_SIZE_Y) {
        RefreshSeedPacketFromCursor(thePlayerIndex);
        mApp->PlayFoley(FoleyType::FOLEY_DROP);
        return;
    }

    CursorObject *aCursor = mCursorObject[thePlayerIndex];
    GamepadControls *aGamepad = nullptr;
    SeedBank *aSeedBank = nullptr;
    if (mApp->IsVSMode()) {
        aGamepad = mGamepadControls[0]->mIsZombie ? mGamepadControls[1] : mGamepadControls[0];
        aSeedBank = mSeedBank[0]->mIsZombie ? mSeedBank[1] : mSeedBank[0];
    } else {
        aGamepad = mGamepadControls[thePlayerIndex];
        aSeedBank = aGamepad->GetSeedBank();
    }

    if (mApp->IsIZombieLevel() || (mApp->mGameMode == GameMode::GAMEMODE_MULTI_PLAYER && thePlayerIndex > 0)) {
        HitResult aHitResult{};
        mChallenge->MouseDown(x, y, theClickCount, &aHitResult, thePlayerIndex);
        return;
    }

    PlantingReason aReason = CanPlantAt(aGridX, aGridY, aPlantingSeedType);
    if (aReason != PlantingReason::PLANTING_OK) {
        // 根据不同的种植原因播放相应的提示字幕
        if (aReason == PlantingReason::PLANTING_ONLY_ON_GRAVES) {
            DisplayAdvice("[ADVICE_GRAVEBUSTERS_ON_GRAVES]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_GRAVEBUSTERS_ON_GRAVES);
        } else if (aPlantingSeedType == SeedType::SEED_LILYPAD && aReason == PlantingReason::PLANTING_ONLY_IN_POOL) {
            DisplayAdvice("[ADVICE_LILYPAD_ON_WATER]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_LILYPAD_ON_WATER);
        } else if (aPlantingSeedType == SeedType::SEED_TANGLEKELP && aReason == PlantingReason::PLANTING_ONLY_IN_POOL) {
            DisplayAdvice("[ADVICE_TANGLEKELP_ON_WATER]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_TANGLEKELP_ON_WATER);
        } else if (aPlantingSeedType == SeedType::SEED_SEASHROOM && aReason == PlantingReason::PLANTING_ONLY_IN_POOL) {
            DisplayAdvice("[ADVICE_SEASHROOM_ON_WATER]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_SEASHROOM_ON_WATER);
        } else if (aReason == PlantingReason::PLANTING_ONLY_ON_GROUND) {
            DisplayAdvice("[ADVICE_POTATO_MINE_ON_LILY]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_POTATOE_MINE_ON_LILY);
        } else if (aReason == PlantingReason::PLANTING_NOT_PASSED_LINE) {
            DisplayAdvice("[ADVICE_NOT_PASSED_LINE]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_NOT_PASSED_LINE);
        } else if (aReason == PlantingReason::PLANTING_NEEDS_UPGRADE) {
            switch (aPlantingSeedType) {
                case SeedType::SEED_GATLINGPEA:
                    DisplayAdvice("[ADVICE_ONLY_ON_REPEATERS]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_REPEATERS);
                    break;
                case SeedType::SEED_TWINSUNFLOWER:
                    DisplayAdvice("[ADVICE_ONLY_ON_SUNFLOWER]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_SUNFLOWER);
                    break;
                case SeedType::SEED_GLOOMSHROOM:
                    DisplayAdvice("[ADVICE_ONLY_ON_FUMESHROOM]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_FUMESHROOM);
                    break;
                case SeedType::SEED_CATTAIL:
                    DisplayAdvice("[ADVICE_ONLY_ON_LILYPAD]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_LILYPAD);
                    break;
                case SeedType::SEED_WINTERMELON:
                    DisplayAdvice("[ADVICE_ONLY_ON_MELONPULT]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_MELONPULT);
                    break;
                case SeedType::SEED_GOLD_MAGNET:
                    DisplayAdvice("[ADVICE_ONLY_ON_MAGNETSHROOM]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_MAGNETSHROOM);
                    break;
                case SeedType::SEED_SPIKEROCK:
                    DisplayAdvice("[ADVICE_ONLY_ON_SPIKEWEED]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_SPIKEWEED);
                    break;
                case SeedType::SEED_COBCANNON:
                    DisplayAdvice("[ADVICE_ONLY_ON_KERNELPULT]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_ONLY_ON_KERNELPULT);
                    break;
                default:
                    break;
            }
        } else if (aReason == PlantingReason::PLANTING_NOT_ON_ART) {
            DisplayAdvice("[ADVICE_WRONG_ART_TYPE]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_WRONG_ART_TYPE);
        } else if (aReason == PlantingReason::PLANTING_NEEDS_POT) {
            if (mApp->IsFirstTimeAdventureMode() && mLevel == 41) {
                DisplayAdvice("[ADVICE_PLANT_NEED_POT1]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_NEED_POT);
            } else {
                DisplayAdvice("[ADVICE_PLANT_NEED_POT2]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_NEED_POT);
            }
        } else if (aReason == PlantingReason::PLANTING_NOT_ON_GRAVE) {
            DisplayAdvice("[ADVICE_PLANT_NOT_ON_GRAVE]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_NOT_ON_GRAVE);
        } else if (aReason == PlantingReason::PLANTING_NOT_ON_CRATER) {
            if (IsPoolSquare(aGridX, aGridY)) {
                DisplayAdvice("[ADVICE_CANT_PLANT_THERE]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_CANT_PLANT_THERE);
            } else {
                DisplayAdvice("[ADVICE_PLANT_NOT_ON_CRATER]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_NOT_ON_CRATER);
            }
        } else if (aReason == PlantingReason::PLANTING_NOT_ON_WATER) {
            if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && mApp->mZenGarden->mGardenType == GARDEN_AQUARIUM) {
                DisplayAdvice("[ZEN_ONLY_AQUATIC_PLANTS]", MessageStyle::MESSAGE_STYLE_HINT_TALL_FAST, AdviceType::ADVICE_NONE);
            } else if (aPlantingSeedType == SeedType::SEED_POTATOMINE) {
                DisplayAdvice("[ADVICE_POTATO_MINE_ON_LILY]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_POTATOE_MINE_ON_LILY);
            } else {
                DisplayAdvice("[ADVICE_PLANT_NOT_ON_WATER]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANT_NOT_ON_WATER);
            }
        } else if (aReason == PlantingReason::PLANTING_NEEDS_GROUND) {
            DisplayAdvice("[ADVICE_PLANTING_NEEDS_GROUND]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANTING_NEEDS_GROUND);
        } else if (aReason == PlantingReason::PLANTING_NEEDS_SLEEPING) {
            DisplayAdvice("[ADVICE_PLANTING_NEED_SLEEPING]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_PLANTING_NEED_SLEEPING);
        }

        // 特定情况下，放下原有手持的植物
        if (aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE || mApp->IsWhackAZombieLevel()) {
            RefreshSeedPacketFromCursor(thePlayerIndex);
            mApp->PlayFoley(FoleyType::FOLEY_DROP);
        } else {
            RefreshSeedPacketFromCursor(thePlayerIndex);
            mApp->PlaySample(SOUND_BUZZER);
        }
        // 不可种植的情况至此结束，直接跳转至返回
        return;
    }

    /* 以下为植物类型可以种植的情况 */
    // 清除种植相关的提示字幕
    ClearAdvice(AdviceType::ADVICE_PLANTING_NEED_SLEEPING);
    ClearAdvice(AdviceType::ADVICE_CANT_PLANT_THERE);
    ClearAdvice(AdviceType::ADVICE_PLANTING_NEEDS_GROUND);
    ClearAdvice(AdviceType::ADVICE_PLANT_NOT_ON_WATER);
    ClearAdvice(AdviceType::ADVICE_PLANT_NOT_ON_CRATER);
    ClearAdvice(AdviceType::ADVICE_PLANT_NOT_ON_GRAVE);
    ClearAdvice(AdviceType::ADVICE_PLANT_NEED_POT);
    ClearAdvice(AdviceType::ADVICE_PLANT_WRONG_ART_TYPE);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_LILYPAD);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_MAGNETSHROOM);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_FUMESHROOM);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_KERNELPULT);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_SUNFLOWER);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_SPIKEWEED);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_MELONPULT);
    ClearAdvice(AdviceType::ADVICE_PLANT_ONLY_ON_REPEATERS);
    ClearAdvice(AdviceType::ADVICE_PLANT_NOT_PASSED_LINE);
    ClearAdvice(AdviceType::ADVICE_PLANT_GRAVEBUSTERS_ON_GRAVES);
    ClearAdvice(AdviceType::ADVICE_PLANT_LILYPAD_ON_WATER);
    ClearAdvice(AdviceType::ADVICE_PLANT_TANGLEKELP_ON_WATER);
    ClearAdvice(AdviceType::ADVICE_PLANT_SEASHROOM_ON_WATER);
    ClearAdvice(AdviceType::ADVICE_PLANT_POTATOE_MINE_ON_LILY);
    ClearAdvice(AdviceType::ADVICE_SURVIVE_FLAGS);

    // 无免费种植、非传送带关卡的卡槽植物，判断阳光是否充足：充足则扣除阳光，不足则退出
    if (!mApp->mEasyPlantingCheat && aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK && !HasConveyorBeltSeedBank(thePlayerIndex)) {
        if (!TakeSunMoney(GetCurrentPlantCost(aPlantingSeedType, SeedType::SEED_NONE), thePlayerIndex)) {
            return;
        }
    }

    // 升级种植或坚果包扎术等情况时，先将原植物销毁
    bool aIsAwake = false;
    int aWakeUpCounter = 0;
    PlantsOnLawn aPlantOnLawn{};
    GetPlantsOnLawn(aGridX, aGridY, &aPlantOnLawn);
    Plant *aNormalPlant = aPlantOnLawn.mNormalPlant;
    Plant *aPumpkinPlant = aPlantOnLawn.mPumpkinPlant;
    if (aNormalPlant != nullptr && aNormalPlant->IsUpgradableTo(aPlantingSeedType)) {
        if (aPlantingSeedType == SeedType::SEED_GLOOMSHROOM) {
            aIsAwake = !aNormalPlant->mIsAsleep;
            aWakeUpCounter = aNormalPlant->mWakeUpCounter;
        }
        aNormalPlant->Die();
    }
    if (Plant::IsDefender(aPlantingSeedType) && aPlantingSeedType != SeedType::SEED_PUMPKINSHELL && aNormalPlant != nullptr) {
        if (aNormalPlant->mSeedType == aPlantingSeedType) {
            aNormalPlant->Die();
        }
    }
    if (aPlantingSeedType == SeedType::SEED_PUMPKINSHELL && aPumpkinPlant != nullptr) {
        if (aPumpkinPlant->mSeedType == SeedType::SEED_PUMPKINSHELL) {
            aPumpkinPlant->Die();
        }
    }
    if (aPlantingSeedType == SeedType::SEED_COBCANNON) {
        Plant *aRightPlant = GetTopPlantAt(aGridX + 1, aGridY, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
        if (aRightPlant != nullptr) {
            aRightPlant->Die();
        }
    }
    if (aPlantingSeedType == SeedType::SEED_CATTAIL) {
        if (aPlantOnLawn.mUnderPlant != nullptr) {
            aPlantOnLawn.mUnderPlant->Die();
        }
        if (aNormalPlant != nullptr) {
            aNormalPlant->Die();
        }
    }

    if (aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE) {
        mApp->mZenGarden->MovePlant(mPlants.DataArrayGet(aCursor->mGlovePlantID), aGridX, aGridY);
    } else if (aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_WHEEL_BARROW) {
        mApp->mZenGarden->MouseDownWithFullWheelBarrow(x, y);
    } else if (aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
        AddPlant(aGridX, aGridY, aCursor->mType, aCursor->mImitaterType, thePlayerIndex, true);
        if (aCursor->mCoinID != COINID_NULL) {
            mCoins.DataArrayGet(aCursor->mCoinID)->Die();
            aCursor->mCoinID = COINID_NULL;
        }
    } else if (aCursor->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK) {
        Plant *aPlant = AddPlant(aGridX, aGridY, aCursor->mType, aCursor->mImitaterType, thePlayerIndex, true);
        if (aPlant != nullptr) {
            if (aIsAwake) {
                aPlant->SetSleeping(false);
            } else {
                aPlant->mWakeUpCounter = aWakeUpCounter;
            }
        }
        aSeedBank->mSeedPackets[aGamepad->mSelectedSeedIndex].Deactivate();
        aSeedBank->mSeedPackets[aGamepad->mSelectedSeedIndex].WasPlanted(thePlayerIndex);
    } else {
        return;
    }

    // 柱子关卡中，一列种植
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_COLUMN) {
        for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; ++aRow) {
            if (aRow == aGridY || CanPlantAt(aGridX, aRow, aPlantingSeedType) != PlantingReason::PLANTING_OK) {
                continue;
            }
            if (aPlantingSeedType == SeedType::SEED_WALLNUT || aPlantingSeedType == SeedType::SEED_TALLNUT) {
                aNormalPlant = GetTopPlantAt(aGridX, aRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
                if (aNormalPlant != nullptr && aNormalPlant->mSeedType == aPlantingSeedType) {
                    aNormalPlant->Die();
                }
            }
            if (aPlantingSeedType == SeedType::SEED_PUMPKINSHELL) {
                aNormalPlant = GetTopPlantAt(aGridX, aRow, PlantPriority::TOPPLANT_ONLY_PUMPKIN);
                if (aNormalPlant != nullptr && aNormalPlant->mSeedType == SeedType::SEED_PUMPKINSHELL) {
                    aNormalPlant->Die();
                }
            }
            AddPlant(aGridX, aRow, aCursor->mType, aCursor->mImitaterType, thePlayerIndex, true);
        }
    }

    // 设置教程状态相关
    if (thePlayerIndex == 0 && mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PLANT_PEASHOOTER) {
        SetTutorialState(int(mPlants.mSize) >= 2 ? TutorialState::TUTORIAL_LEVEL_1_COMPLETED : TutorialState::TUTORIAL_LEVEL_1_REFRESH_PEASHOOTER);
    } else if (thePlayerIndex == 0 && mTutorialState == TutorialState::TUTORIAL_LEVEL_2_PLANT_SUNFLOWER) {
        int aSunFlowersCount = CountSunFlowers();
        if (aPlantingSeedType == SeedType::SEED_SUNFLOWER && aSunFlowersCount == 2) {
            DisplayAdvice("[ADVICE_MORE_SUNFLOWERS]", MessageStyle::MESSAGE_STYLE_TUTORIAL_LEVEL2, AdviceType::ADVICE_NONE);
            if (!mSeedBank[0]->mSeedPackets[1].CanPickUp()) {
                SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER);
            } else {
                SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER);
            }
        } else if (aSunFlowersCount >= 3) {
            SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_COMPLETED);
        } else if (!mSeedBank[0]->mSeedPackets[1].CanPickUp()) {
            SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER);
        } else {
            SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER);
        }
    } else if (thePlayerIndex == 0 && mTutorialState == TutorialState::TUTORIAL_MORESUN_PLANT_SUNFLOWER) {
        if (CountSunFlowers() >= 3) {
            SetTutorialState(TutorialState::TUTORIAL_MORESUN_COMPLETED);
            DisplayAdvice("[ADVICE_PLANT_SUNFLOWER5]", MessageStyle::MESSAGE_STYLE_TUTORIAL_LATER, AdviceType::ADVICE_PLANT_SUNFLOWER5);
            mTutorialTimer = -1;
        } else if (!mSeedBank[0]->mSeedPackets[1].CanPickUp()) {
            SetTutorialState(TutorialState::TUTORIAL_MORESUN_REFRESH_SUNFLOWER);
        } else {
            SetTutorialState(TutorialState::TUTORIAL_MORESUN_PICK_UP_SUNFLOWER);
        }
    }

    // 保龄球关卡，播放保龄球滚动的音效
    if (mApp->IsWallnutBowlingLevel()) {
        mApp->PlaySample(SOUND_BOWLING);
    }

    // 重置鼠标
    ClearCursor(thePlayerIndex);
}

bool Board::MouseHitTest(int x, int y, HitResult *theHitResult, bool thePlayerIndex) {
    GameMode aGameMode = mApp->mGameMode;
    if (aGameMode == GameMode::GAMEMODE_MP_VS) {
        if (gTouchVSShovelRect.Contains(x, y)) {
            theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_SHOVEL;
            return true;
        }
    } else {
        if (mShowShovel && GetShovelButtonRect().Contains(x, y)) {
            theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_SHOVEL;
            return true;
        }
    }

    if (mApp->IsCoopMode()) {
        if (mShowButter && GetButterButtonRect().Contains(x, y)) {
            theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_BUTTER;
            return true;
        }
    }

    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || aGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
        Rect rect;
        for (int i = GameObjectType::OBJECT_TYPE_WATERING_CAN; i <= GameObjectType::OBJECT_TYPE_TREE_OF_WISDOM_GARDEN; i++) {
            if (CanUseGameObject((GameObjectType)i)) {
                rect = GetZenButtonRect((GameObjectType)i);
                if (rect.Contains(x, y)) {
                    theHitResult->mObjectType = (GameObjectType)i;
                    return true;
                }
            }
        }
    }

    if (old_Board_MouseHitTest(this, x, y, theHitResult, thePlayerIndex)) {
        if (theHitResult->mObjectType == GameObjectType::OBJECT_TYPE_TREE_OF_WISDOM_GARDEN) {
            theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_NONE;
            return false;
        }
        return true;
    }

    SeedBank *aSeedBank = mGamepadControls[0]->GetSeedBank();
    if (aSeedBank->ContainsPoint(x, y)) {
        if (aSeedBank->MouseHitTest(x, y, theHitResult)) {
            if (mCursorObject[0]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK) {
                RefreshSeedPacketFromCursor(0);
            }
            return true;
        }
        theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_SEED_BANK_BLANK;
        return false;
    }

    if (aGameMode == GameMode::GAMEMODE_MP_VS || mApp->IsCoopMode()) {
        SeedBank *aSeedBank2 = mGamepadControls[1]->GetSeedBank();
        if (aSeedBank2->ContainsPoint(x, y)) {
            if (aSeedBank2->MouseHitTest(x, y, theHitResult)) {
                if (mCursorObject[1]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK) {
                    RefreshSeedPacketFromCursor(1);
                }
                return true;
            }
            theHitResult->mObjectType = GameObjectType::OBJECT_TYPE_SEED_BANK_BLANK;
            return false;
        }
    }

    return false;
}


namespace {
constexpr int gTouchShovelRectWidth = 72;
constexpr int gTouchButterRectWidth = 72;
constexpr int gTouchTrigger = 40;

int gTouchLastX;
int gTouchLastY;
int gTouchDownX;
int gTouchDownY;
bool gSendKeyWhenTouchUp;
TouchState gTouchState = TouchState::TOUCHSTATE_NONE;
float gTouchHeavyWeaponX;
Rect gSlotMachineRect = {250, 0, 320, 100};

bool gClientMouseInBank = false;
bool gClientMouseInBoard = false;
} // namespace


void Board::ClientMouseDownLocal(int x, int y, bool isInBank) {
    gClientMouseInBank = isInBank;
    gClientMouseInBoard = !isInBank;
    if (gClientMouseInBoard) {
        GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
        clientGamepadControls->mCursorPositionX = x;
        clientGamepadControls->mCursorPositionY = y;
    }
}

void Board::ClientMouseDragLocal(int x, int y) {

    GamepadControls *clientGamepadControls = mGamepadControls[(mGamepadControls[1]->mGamepadIndex == 1) ? 1 : 0];
    SeedBank *seedBank = clientGamepadControls->GetSeedBank();
    bool isInBank = seedBank->ContainsPoint(x, y);

    if (gClientMouseInBank) {
        if (!isInBank) {
            gClientMouseInBank = false;
            gClientMouseInBoard = true;
        }
    }

    if (gClientMouseInBoard) {
        int seedBankHeight = seedBank->mY + seedBank->mHeight;
        if (y < seedBankHeight && clientGamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            gClientMouseInBoard = false;
            return;
        }
        clientGamepadControls->mCursorPositionX = x;
        clientGamepadControls->mCursorPositionY = y;
    }
}

void Board::ClientMouseUpLocal(int x, int y) {
    gClientMouseInBank = false;
    gClientMouseInBoard = false;
}


// 触控落下手指在此处理
void Board::MouseDown(int x, int y, int theClickCount) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }

    if (mApp->mGameMode != GAMEMODE_MP_VS || (!gTcpConnected && gTcpClientSocket == -1)) {
        __MouseDown(x, y, theClickCount);
        return;
    }

    bool inRangeOf1PSeedBank = (mGamepadControls[0]->mGamepadIndex == 1 && mSeedBank[1]->ContainsPoint(x, y))
        || (mGamepadControls[0]->mGamepadIndex == 0 && (mSeedBank[0]->ContainsPoint(x, y) || gTouchVSShovelRect.Contains(x, y)));
    bool inRangeOf2PSeedBank = (mGamepadControls[1]->mGamepadIndex == 1 && mSeedBank[1]->ContainsPoint(x, y))
        || (mGamepadControls[1]->mGamepadIndex == 0 && (mSeedBank[0]->ContainsPoint(x, y) || gTouchVSShovelRect.Contains(x, y)));


    // 如果是客户端
    if (gTcpConnected) {
        if (inRangeOf1PSeedBank)
            return;
        I16I16_Event event = {{EventType::EVENT_CLIENT_BOARD_TOUCH_DOWN}, int16_t(x), int16_t(y)};
        netplay::PutEvent(event);
        ClientMouseDownLocal(x, y, inRangeOf2PSeedBank);
        return;
    }

    // 如果是主机端
    if (gTcpClientSocket >= 0) {
        if (inRangeOf2PSeedBank)
            return;
        __MouseDown(x, y, theClickCount);
        GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
        U8U8I16I16_Event event = {{EventType::EVENT_SERVER_BOARD_TOUCH_DOWN},
                                  uint8_t(serverGamepadControls->mSelectedSeedIndex),
                                  uint8_t(serverGamepadControls->mGamepadState),
                                  int16_t(serverGamepadControls->mCursorPositionX),
                                  int16_t(serverGamepadControls->mCursorPositionY)};
        netplay::PutEvent(event);
    }
}
void Board::__MouseDown(int x, int y, int theClickCount) {

    old_Board_MouseDown(this, x, y, theClickCount);
    gTouchDownX = x;
    gTouchDownY = y;
    gTouchLastX = x;
    gTouchLastY = y;
    // xx = x;
    // yy = y;
    // LOGD("%d %d",x,y);
    if (gKeyboardMode) {
        patchlist::autoPickupSeedPacketDisable.Modify();
    }
    gKeyboardMode = false;
    SeedBank *aSeedBank = mGamepadControls[0]->GetSeedBank();
    int currentSeedBankIndex = mGamepadControls[0]->mSelectedSeedIndex;
    BaseGamepadControls::MovementState aGameState = mGamepadControls[0]->mGamepadState;
    bool isCobCannonSelected = mGamepadControls[0]->mIsCobCannonSelected;

    SeedBank *aSeedBank_2P = mGamepadControls[1]->GetSeedBank();
    int currentSeedBankIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
    BaseGamepadControls::MovementState aGameState_2P = mGamepadControls[1]->mGamepadState;
    bool isCobCannonSelected_2P = mGamepadControls[1]->mIsCobCannonSelected;
    HitResult hitResult;
    MouseHitTest(x, y, &hitResult, false);
    GameObjectType aObjectType = hitResult.mObjectType;
    GameMode aGameMode = mApp->mGameMode;
    bool isTwoSeedBankMode = (aGameMode == GameMode::GAMEMODE_MP_VS || mApp->IsCoopMode());
    GameScenes aGameScene = mApp->mGameScene;

    SeedChooserScreen *aSeedChooserScreen = mApp->mSeedChooserScreen;
    if (aGameScene == GameScenes::SCENE_LEVEL_INTRO && aSeedChooserScreen != nullptr && aSeedChooserScreen->mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN) {
        aSeedChooserScreen->GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 0, 0);
        return;
    }
    if (aGameScene == GameScenes::SCENE_LEVEL_INTRO) {
        mCutScene->MouseDown(x, y);
    }


    if (aObjectType == GameObjectType::OBJECT_TYPE_SEEDPACKET) {
        if (aGameScene == GameScenes::SCENE_LEVEL_INTRO)
            return;
        auto *aSeedPacket = (SeedPacket *)hitResult.mObject;
        const auto seedPacketPlayerIndex = static_cast<TouchPlayerIndex>(aSeedPacket->GetPlayerIndex());
        if (aGameMode == GameMode::GAMEMODE_MP_VS && gTcpClientSocket >= 0) {
            gPlayerIndex = mGamepadControls[0]->mGamepadIndex == 0 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER1 : TouchPlayerIndex::TOUCHPLAYER_PLAYER2;
            if (seedPacketPlayerIndex != gPlayerIndex)
                return;
        } else {
            gPlayerIndex = seedPacketPlayerIndex;
        }
        if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            requestDrawShovelInCursor = false; // 不再绘制铲子
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            if (isCobCannonSelected) { // 如果拿着加农炮，将其放下
                mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST
                || aGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM) {
                if (aSeedPacket->CanPickUp()) {
                    gSendKeyWhenTouchUp = true;
                } else {
                    mApp->PlaySample(Sexy::SOUND_BUZZER);
                    return;
                }
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) { // 拉老虎机用
                mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_X_BUTTON, 1112);
                return;
            }
            gTouchState = TouchState::TOUCHSTATE_SEED_BANK; // 记录本次触控的状态
            RefreshSeedPacketFromCursor(0);
            int newSeedPacketIndex = aSeedPacket->mIndex;
            mGamepadControls[0]->mSelectedSeedIndex = newSeedPacketIndex;
            aSeedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
            if (currentSeedBankIndex != newSeedPacketIndex || aGameState != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                mGamepadControls[0]->mIsInShopSeedBank = false;
                bool isClientGamepadControl = mGamepadControls[0]->mGamepadIndex == 1;
                if (gTcpClientSocket >= 0 && isClientGamepadControl) { // 让对方播放音效
                    U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 2};
                    netplay::PutEvent(event);
                } else {
                    mApp->PlaySample(SOUND_SEEDLIFT);
                    if (gTcpClientSocket >= 0) {
                        U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_SEEDLIFT)};
                        netplay::PutEvent(event);
                    }
                }
            } else if (currentSeedBankIndex == newSeedPacketIndex && aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                if (!isTwoSeedBankMode)
                    mGamepadControls[0]->mIsInShopSeedBank = true;
            }
        } else {
            requestDrawButterInCursor = false; // 不再绘制黄油
            if (isCobCannonSelected_2P) {      // 如果拿着加农炮，将其放下
                mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST
                || aGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM) {
                if (aSeedPacket->CanPickUp()) {
                    gSendKeyWhenTouchUp = true;
                } else {
                    mApp->PlaySample(Sexy::SOUND_BUZZER);
                    return;
                }
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) { // 拉老虎机用
                mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_X_BUTTON, 1112);
                return;
            }
            gTouchState = TouchState::TOUCHSTATE_SEED_BANK; // 记录本次触控的状态
            RefreshSeedPacketFromCursor(1);
            int newSeedPacketIndex_2P = aSeedPacket->mIndex;
            mGamepadControls[1]->mSelectedSeedIndex = newSeedPacketIndex_2P;
            aSeedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用

            if (currentSeedBankIndex_2P != newSeedPacketIndex_2P || aGameState_2P != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                mGamepadControls[1]->mIsInShopSeedBank = false;
                bool isClientGamepadControl = mGamepadControls[1]->mGamepadIndex == 1;
                if (gTcpClientSocket >= 0 && isClientGamepadControl) { // 让对方播放音效
                    U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 2};
                    netplay::PutEvent(event);
                } else {
                    mApp->PlaySample(SOUND_SEEDLIFT);
                    if (gTcpClientSocket >= 0) {
                        U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_SEEDLIFT)};
                        netplay::PutEvent(event);
                    }
                }
            } else if (currentSeedBankIndex_2P == newSeedPacketIndex_2P && aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                if (!isTwoSeedBankMode)
                    mGamepadControls[1]->mIsInShopSeedBank = true;
            }
        }

        ShuffleButtonDown(aSeedPacket);

        return;
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_SEED_BANK_BLANK) {
        return;
    }

    CursorType mCursorType = mCursorObject[0]->mCursorType;
    CursorType mCursorType_2P = mCursorObject[1]->mCursorType;

    if (aObjectType == GameObjectType::OBJECT_TYPE_SHOVEL) {
        gPlayerIndex = TouchPlayerIndex::TOUCHPLAYER_PLAYER1; // 玩家1
        gTouchState = TouchState::TOUCHSTATE_SHOVEL_RECT;
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            if (!isTwoSeedBankMode)
                mGamepadControls[0]->mIsInShopSeedBank = true;
            int newSeedPacketIndex = mGamepadControls[0]->mSelectedSeedIndex;
            aSeedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
        }
        if (mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            RefreshSeedPacketFromCursor(0);
            ClearCursor(0);
        }
        if (isCobCannonSelected) { // 如果拿着加农炮，将其放下
            mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
        }
        RefreshSeedPacketFromCursor(0);
        if (requestDrawShovelInCursor) {
            requestDrawShovelInCursor = false;
        } else {
            requestDrawShovelInCursor = true;
            mApp->PlayFoley(FoleyType::FOLEY_SHOVEL);
        }
        if (gTcpClientSocket) {
            U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
            netplay::PutEvent(event);
        }
        return;
    }
    if (aObjectType == GameObjectType::OBJECT_TYPE_BUTTER) {
        gPlayerIndex = TouchPlayerIndex::TOUCHPLAYER_PLAYER2; // 玩家2
        gTouchState = TouchState::TOUCHSTATE_BUTTER_RECT;
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            if (!isTwoSeedBankMode)
                mGamepadControls[1]->mIsInShopSeedBank = true;
            int newSeedPacketIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
            aSeedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用
        }
        if (mCursorType_2P == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            RefreshSeedPacketFromCursor(1);
            ClearCursor(1);
        }
        if (isCobCannonSelected_2P) { // 如果拿着加农炮，将其放下
            mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
        }
        RefreshSeedPacketFromCursor(1);
        if (requestDrawButterInCursor) {
            requestDrawButterInCursor = false;
        } else {
            requestDrawButterInCursor = true;
            mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
        }
        return;
    }

    if (aGameMode == GameMode::GAMEMODE_MP_VS) {
        if (gTcpClientSocket >= 0) {
            gPlayerIndex = mGamepadControls[0]->mGamepadIndex == 0 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER1 : TouchPlayerIndex::TOUCHPLAYER_PLAYER2;
        } else {
            gPlayerIndex = PixelToGridX(x, y) > 5 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER2 : TouchPlayerIndex::TOUCHPLAYER_PLAYER1;
        }
    } else if (aGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && aGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS) {
        gPlayerIndex = x > 400 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER2 : TouchPlayerIndex::TOUCHPLAYER_PLAYER1;
    } else {
        gPlayerIndex = TouchPlayerIndex::TOUCHPLAYER_PLAYER1;
    }

    if (gPlayerIndexSecond != TouchPlayerIndex::TOUCHPLAYER_NONE && gPlayerIndexSecond == gPlayerIndex) {
        gPlayerIndex = TouchPlayerIndex::TOUCHPLAYER_NONE;
        gTouchState = TouchState::TOUCHSTATE_NONE;
        return;
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_COIN) {
        Coin *coin = (Coin *)hitResult.mObject;
        if (coin->mType == CoinType::COIN_USABLE_SEED_PACKET) {
            gTouchState = TouchState::TOUCHSTATE_USEFUL_SEED_PACKET;
            requestDrawShovelInCursor = false;
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            // if (mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            // LOGD("5656565656");
            // GamepadControls_OnKeyDown(gamepadCon
            // trols1, 27, 1096);//放下手上的植物卡片
            // gSendKeyWhenTouchUp = false;
            // }
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            RefreshSeedPacketFromCursor(0);
            coin->Collect(0);
        }
    }

    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) { // 拉老虎机用
        if (gSlotMachineRect.Contains(x, y)) {
            mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_X_BUTTON, 1112);
            return;
        }
    }

    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON) { // 移动重型武器
        gTouchState = TouchState::TOUCHSTATE_HEAVY_WEAPON;
        gTouchHeavyWeaponX = mChallenge->mHeavyWeaponX;
        return;
    }

    if (mChallenge->MouseDown(x, y, 0, &hitResult, 0)) {
        if (mApp->IsScaryPotterLevel()) {
            requestDrawShovelInCursor = false;
        }
        if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
            mGamepadControls[0]->mCursorPositionX = x - 40;
            mGamepadControls[0]->mCursorPositionY = y - 40;
        } else {
            mGamepadControls[0]->mCursorPositionX = x;
            mGamepadControls[0]->mCursorPositionY = y;
        }
        if (!mApp->IsWhackAZombieLevel() || aGameState != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR)
            return; // 这一行代码的意义：在锤僵尸关卡，手持植物时，允许拖动种植。
    }
    if (aObjectType == GameObjectType::OBJECT_TYPE_WATERING_CAN || aObjectType == GameObjectType::OBJECT_TYPE_FERTILIZER || aObjectType == GameObjectType::OBJECT_TYPE_BUG_SPRAY
        || aObjectType == GameObjectType::OBJECT_TYPE_PHONOGRAPH || aObjectType == GameObjectType::OBJECT_TYPE_CHOCOLATE || aObjectType == GameObjectType::OBJECT_TYPE_GLOVE
        || aObjectType == GameObjectType::OBJECT_TYPE_MONEY_SIGN || aObjectType == GameObjectType::OBJECT_TYPE_WHEELBARROW || aObjectType == GameObjectType::OBJECT_TYPE_TREE_FOOD) {
        PickUpTool(aObjectType, 0);
        ((ZenGardenControls *)mGamepadControls[0])->mObjectType = aObjectType;
        gTouchState = TouchState::TOUCHSTATE_ZEN_GARDEN_TOOLS;
        return;
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_ZEN_GARDEN || aObjectType == GameObjectType::OBJECT_TYPE_MUSHROOM_GARDEN || aObjectType == GameObjectType::OBJECT_TYPE_QUARIUM_GARDEN
        || aObjectType == GameObjectType::OBJECT_TYPE_TREE_OF_WISDOM_GARDEN) {
        ((ZenGardenControls *)mGamepadControls[0])->mObjectType = aObjectType;
        MouseDownWithTool(x, y, 0, (CursorType)(aObjectType + 3), 0);
        return;
    }


    if (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN || mCursorType == CursorType::CURSOR_TYPE_FERTILIZER || mCursorType == CursorType::CURSOR_TYPE_BUG_SPRAY
        || mCursorType == CursorType::CURSOR_TYPE_PHONOGRAPH || mCursorType == CursorType::CURSOR_TYPE_CHOCOLATE || mCursorType == CursorType::CURSOR_TYPE_GLOVE
        || mCursorType == CursorType::CURSOR_TYPE_MONEY_SIGN || mCursorType == CursorType::CURSOR_TYPE_WHEEELBARROW || mCursorType == CursorType::CURSOR_TYPE_TREE_FOOD
        || mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE) {
        gSendKeyWhenTouchUp = true;
    }

    // *(uint32_t *) (mGamepadControls[0] + 152) = 0;//疑似用于设置该gamepadControls1属于玩家1。可能的取值：-1，0，1
    // 其中，1P恒为0，2P禁用时为-1，2P启用时为1。

    if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
        if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST
            || (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN && mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_GOLD_WATERINGCAN] != 0)) {
            mGamepadControls[0]->mCursorPositionX = x - 40;
            mGamepadControls[0]->mCursorPositionY = y - 40;
        } else {
            mGamepadControls[0]->mCursorPositionX = x;
            mGamepadControls[0]->mCursorPositionY = y;
        }
    } else if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
        mGamepadControls[1]->mCursorPositionX = x - 40;
        mGamepadControls[1]->mCursorPositionY = y - 40;
    } else {
        mGamepadControls[1]->mCursorPositionX = x;
        mGamepadControls[1]->mCursorPositionY = y;
    }

    if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || isCobCannonSelected || requestDrawShovelInCursor
            || (mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN && gTouchState != TouchState::TOUCHSTATE_USEFUL_SEED_PACKET)) {
            gTouchState = TouchState::TOUCHSTATE_PICKING_SOMETHING;
            gSendKeyWhenTouchUp = true;
        }
    } else {
        if (aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || isCobCannonSelected_2P || requestDrawButterInCursor
            || (mCursorType_2P == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN && gTouchState != TouchState::TOUCHSTATE_USEFUL_SEED_PACKET)) {
            gTouchState = TouchState::TOUCHSTATE_PICKING_SOMETHING;
            gSendKeyWhenTouchUp = true;
        }
    }


    if (aObjectType == GameObjectType::OBJECT_TYPE_PLANT) {
        if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1 && requestDrawShovelInCursor)
            return;
        auto *plant = (Plant *)hitResult.mObject;
        bool isValidCobCannon = plant->mSeedType == SeedType::SEED_COBCANNON && plant->mState == PlantState::STATE_COBCANNON_READY;
        if (isValidCobCannon) {
            if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
                if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                    mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                    gSendKeyWhenTouchUp = false;
                    if (!isTwoSeedBankMode)
                        mGamepadControls[0]->mIsInShopSeedBank = true;
                    int newSeedPacketIndex = mGamepadControls[0]->mSelectedSeedIndex;
                    aSeedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
                }
                if (mGamepadControls[1]->mIsCobCannonSelected && mGamepadControls[1]->mCobCannonPlantIndexInList == mPlants.DataArrayGetID(plant)) {
                    // 不能同时选同一个加农炮！
                    gTouchState = TouchState::TOUCHSTATE_NONE;
                    return;
                }
                mGamepadControls[0]->pickUpCobCannon(plant);
            } else {
                if (aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                    mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                    gSendKeyWhenTouchUp = false;
                    if (!isTwoSeedBankMode)
                        mGamepadControls[1]->mIsInShopSeedBank = true;
                    int newSeedPacketIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
                    aSeedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用
                }
                if (mGamepadControls[0]->mIsCobCannonSelected && mGamepadControls[0]->mCobCannonPlantIndexInList == mPlants.DataArrayGetID(plant)) {
                    // 不能同时选同一个加农炮！
                    gTouchState = TouchState::TOUCHSTATE_NONE;
                    return;
                }
                mGamepadControls[1]->pickUpCobCannon(plant);
            }
            gTouchState = TouchState::TOUCHSTATE_VALID_COBCONON;
            return;
        }
    }
    if (gTouchState == TouchState::TOUCHSTATE_NONE)
        gTouchState = TouchState::TOUCHSTATE_BOARD;
}

void Board::MouseDrag(int x, int y) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    // Drag函数仅仅负责移动光标即可

    if (mApp->mGameMode != GAMEMODE_MP_VS) {
        __MouseDrag(x, y);
        return;
    }
    if (gTcpConnected) {
        I16I16_Event event = {{EventType::EVENT_CLIENT_BOARD_TOUCH_DRAG}, int16_t(x), int16_t(y)};
        netplay::PutEvent(event);
        ClientMouseDragLocal(x, y);
        return;
    }
    __MouseDrag(x, y);

    if (gTcpClientSocket >= 0) {
        GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
        I16I16_Event event = {{EventType::EVENT_SERVER_BOARD_TOUCH_DRAG}, int16_t(serverGamepadControls->mCursorPositionX), int16_t(serverGamepadControls->mCursorPositionY)};
        netplay::PutEvent(event);
    }
}
void Board::__MouseDrag(int x, int y) {
    // Drag函数仅仅负责移动光标即可
    old_Board_MouseDrag(this, x, y);
    // xx = x;
    // yy = y;
    // LOGD("%d %d",x,y);
    if (gTouchState == TouchState::TOUCHSTATE_NONE)
        return;

    bool isCobCannonSelected = mGamepadControls[0]->mIsCobCannonSelected;
    SeedBank *seedBank = mGamepadControls[0]->GetSeedBank();
    BaseGamepadControls::MovementState mGameState_2P = mGamepadControls[1]->mGamepadState;
    bool isCobCannonSelected_2P = mGamepadControls[1]->mIsCobCannonSelected;
    BaseGamepadControls::MovementState mGameState = mGamepadControls[0]->mGamepadState;
    GameMode mGameMode = mApp->mGameMode;
    bool isTwoSeedBankMode = (mGameMode == GameMode::GAMEMODE_MP_VS || (mGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && mGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS));
    int seedBankHeight = mApp->IsChallengeWithoutSeedBank() ? 87 : seedBank->mY + seedBank->mHeight;

    if (gTouchState == TouchState::TOUCHSTATE_SEED_BANK && mApp->IsVSMode()) {
        if (mGamepadControls[0]->mSelectedSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE || mGamepadControls[1]->mSelectedSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE
            || mGamepadControls[0]->mSelectedSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE || mGamepadControls[1]->mSelectedSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE)
            return;
    }

    if (gTouchState == TouchState::TOUCHSTATE_SEED_BANK && gTouchLastY < seedBankHeight && y >= seedBankHeight) {
        gTouchState = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_SEED_BANK;
        if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
            mGamepadControls[0]->mIsInShopSeedBank = false;
            requestDrawShovelInCursor = false;
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            if (gTcpClientSocket >= 0 && mGamepadControls[0]->mGamepadIndex == 0) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_SET_STATE}, 7};
                netplay::PutEvent(event);
            }
        } else {
            mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
            mGamepadControls[1]->mIsInShopSeedBank = false;
            requestDrawButterInCursor = false;
            if (gTcpClientSocket >= 0 && mGamepadControls[1]->mGamepadIndex == 0) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_SET_STATE}, 7};
                netplay::PutEvent(event);
            }
        }
        gSendKeyWhenTouchUp = true;
    }

    if (gTouchState == TouchState::TOUCHSTATE_SHOVEL_RECT) {
        if (mGameMode == GameMode::GAMEMODE_MP_VS) {
            if (gTouchVSShovelRect.Contains(gTouchLastX, gTouchLastY) && !gTouchVSShovelRect.Contains(x, y)) {
                gTouchState = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_SHOVEL_RECT;
                if (!requestDrawShovelInCursor)
                    mApp->PlayFoley(FoleyType::FOLEY_SHOVEL);
                requestDrawShovelInCursor = true;
                if (gTcpClientSocket) {
                    U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                    netplay::PutEvent(event);
                }
                mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                gSendKeyWhenTouchUp = true;
            }
        } else if (gTouchLastY < gTouchShovelRectWidth && y >= gTouchShovelRectWidth) {
            gTouchState = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_SHOVEL_RECT;
            if (!requestDrawShovelInCursor)
                mApp->PlayFoley(FoleyType::FOLEY_SHOVEL);
            requestDrawShovelInCursor = true;
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            gSendKeyWhenTouchUp = true;
        }
    }

    if (gTouchState == TouchState::TOUCHSTATE_BUTTER_RECT && gTouchLastY < gTouchButterRectWidth && y >= gTouchButterRectWidth) {
        gTouchState = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_BUTTER_RECT;
        if (!requestDrawButterInCursor)
            mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
        requestDrawButterInCursor = true;
        mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
        gSendKeyWhenTouchUp = true;
    }

    if (gTouchState == TouchState::TOUCHSTATE_VALID_COBCONON || gTouchState == TouchState::TOUCHSTATE_USEFUL_SEED_PACKET) {
        if (!gSendKeyWhenTouchUp && (abs(x - gTouchDownX) > gTouchTrigger || abs(y - gTouchDownY) > gTouchTrigger)) {
            gSendKeyWhenTouchUp = true;
        }
    }

    if (gTouchState == TouchState::TOUCHSTATE_ZEN_GARDEN_TOOLS && gTouchLastY < gTouchButterRectWidth && y >= gTouchButterRectWidth) {
        gTouchState = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_ZEN_GARDEN_TOOLS;
        gSendKeyWhenTouchUp = true;
    }

    if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1 && isCobCannonSelected && gTouchLastY > seedBankHeight && y <= seedBankHeight) {
        mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096); // 退选加农炮
        gTouchState = TouchState::TOUCHSTATE_NONE;
        gSendKeyWhenTouchUp = false;
    }
    if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER2 && isCobCannonSelected_2P && gTouchLastY > seedBankHeight && y <= seedBankHeight) {
        mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096); // 退选加农炮
        gTouchState = TouchState::TOUCHSTATE_NONE;
        gSendKeyWhenTouchUp = false;
    }

    if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
        if (mGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR && gTouchLastY > seedBankHeight && y <= seedBankHeight) {
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL; // 退选植物
            if (!isTwoSeedBankMode)
                mGamepadControls[0]->mIsInShopSeedBank = true;
            int newSeedPacketIndex = mGamepadControls[0]->mSelectedSeedIndex;
            seedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
            gTouchState = TouchState::TOUCHSTATE_NONE;
            gSendKeyWhenTouchUp = false;

            if (mGamepadControls[0]->mGamepadIndex == 0 && gTcpClientSocket >= 0) {
                BaseEvent event = {EventType::EVENT_SERVER_BOARD_TOUCH_CLEAR_CURSOR};
                netplay::PutEvent(event);
            }
        }
    } else {
        if (mGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR && gTouchLastY > seedBankHeight && y <= seedBankHeight) {
            mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL; // 退选植物
            if (!isTwoSeedBankMode)
                mGamepadControls[1]->mIsInShopSeedBank = true;
            int newSeedPacketIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
            SeedBank *seedBank_2P = mGamepadControls[1]->GetSeedBank();
            seedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用
            gTouchState = TouchState::TOUCHSTATE_NONE;
            gSendKeyWhenTouchUp = false;

            if (mGamepadControls[1]->mGamepadIndex == 0 && gTcpClientSocket >= 0) {
                BaseEvent event = {EventType::EVENT_SERVER_BOARD_TOUCH_CLEAR_CURSOR};
                netplay::PutEvent(event);
            }
        }
    }


    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && gTouchState == TouchState::TOUCHSTATE_HEAVY_WEAPON) {
        mChallenge->mHeavyWeaponX = gTouchHeavyWeaponX + x - gTouchDownX; // 移动重型武器X坐标
        return;
    }

    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
        return;
    }

    gTouchLastX = x;
    gTouchLastY = y;

    if (gTouchState != TouchState::TOUCHSTATE_SEED_BANK && gTouchState != TouchState::TOUCHSTATE_ZEN_GARDEN_TOOLS) {
        if (x > 770)
            x = 770;
        if (x < 40)
            x = 40;
        if (y > 580)
            y = 580;
        if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            CursorType mCursorType = mCursorObject[0]->mCursorType;
            if (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN && mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_GOLD_WATERINGCAN] != 0) {
                mGamepadControls[0]->mCursorPositionX = x - 40;
                mGamepadControls[0]->mCursorPositionY = y - 40;
            } else {
                mGamepadControls[0]->mCursorPositionX = x;
                mGamepadControls[0]->mCursorPositionY = y;
            }
        } else {
            mGamepadControls[1]->mCursorPositionX = x;
            mGamepadControls[1]->mCursorPositionY = y;
        }
    }
}

void Board::MouseUp(int x, int y, int theClickCount) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (mApp->mGameMode != GAMEMODE_MP_VS) {
        __MouseUp(x, y, theClickCount);
        return;
    }
    if (gTcpConnected) {
        I16I16_Event event = {{EventType::EVENT_CLIENT_BOARD_TOUCH_UP}, int16_t(x), int16_t(y)};
        netplay::PutEvent(event);
        ClientMouseUpLocal(x, y);
        return;
    }
    __MouseUp(x, y, theClickCount);

    if (gTcpClientSocket >= 0) {
        GamepadControls *serverGamepadControls = mGamepadControls[0]->mGamepadIndex == 0 ? mGamepadControls[0] : mGamepadControls[1];
        CursorObject *serverCursorObject = mGamepadControls[0]->mGamepadIndex == 0 ? mCursorObject[0] : mCursorObject[1];
        U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_TOUCH_UP}, uint8_t(serverGamepadControls->mGamepadState), uint8_t(serverCursorObject->mCursorType)};
        netplay::PutEvent(event);
    }
}
void Board::__MouseUp(int x, int y, int theClickCount) {
    old_Board_MouseUp(this, x, y, theClickCount);
    if (gTouchState != TouchState::TOUCHSTATE_NONE && gSendKeyWhenTouchUp) {
        SeedBank *seedBank = mGamepadControls[0]->GetSeedBank();
        int numSeedsInBank = seedBank->GetNumSeedsOnConveyorBelt();
        BaseGamepadControls::MovementState mGameState = mGamepadControls[0]->mGamepadState;
        bool isCobCannonSelected = mGamepadControls[0]->mIsCobCannonSelected;

        SeedBank *seedBank_2P = mGamepadControls[1]->GetSeedBank();
        int numSeedsInBank_2P = seedBank_2P->GetNumSeedsOnConveyorBelt();
        BaseGamepadControls::MovementState mGameState_2P = mGamepadControls[1]->mGamepadState;
        bool isCobCannonSelected_2P = mGamepadControls[1]->mIsCobCannonSelected;

        GameMode aGameMode = mApp->mGameMode;
        CursorType mCursorType = mCursorObject[0]->mCursorType;
        CursorType mCursorType_2P = mCursorObject[1]->mCursorType;
        ChallengeState mChallengeState = mChallenge->mChallengeState;

        if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            if (requestDrawShovelInCursor) {
                ShovelDown();
            } else if (mGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || isCobCannonSelected || mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
                if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                } else if ((aGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && mChallenge->mChallengeState == ChallengeState::STATECHALLENGE_NORMAL
                            && mApp->mGameScene == GameScenes::SCENE_PLAYING)
                           || aGameMode == GameMode::GAMEMODE_MP_VS) {
                    mGamepadControls[0]->OnButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, mGamepadControls[0]->mPlayerIndex, 0);
                } else {
                    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON) { // 重型武器关卡需要设置为状态6才能种植猫尾草
                        mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_SELECT_SEED;
                    }
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                }
                BaseGamepadControls::MovementState mGameStateNew = mGamepadControls[0]->mGamepadState;
                int seedPacketIndexNew = mGamepadControls[0]->mSelectedSeedIndex;
                int numSeedsInBankNew = seedBank->GetNumSeedsOnConveyorBelt();
                mGamepadControls[0]->mIsInShopSeedBank = mGameStateNew != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                if (mGameState != mGameStateNew) {
                    if (!HasConveyorBeltSeedBank(0) || numSeedsInBank == numSeedsInBankNew) { // 修复传送带关卡种植之后SeedBank动画不正常
                        seedBank->mSeedPackets[seedPacketIndexNew].mLastSelectedTime = 0.0f;  // 动画效果专用
                    }
                }
            } else if (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN || mCursorType == CursorType::CURSOR_TYPE_FERTILIZER || mCursorType == CursorType::CURSOR_TYPE_BUG_SPRAY
                       || mCursorType == CursorType::CURSOR_TYPE_PHONOGRAPH || mCursorType == CursorType::CURSOR_TYPE_CHOCOLATE || mCursorType == CursorType::CURSOR_TYPE_GLOVE
                       || mCursorType == CursorType::CURSOR_TYPE_MONEY_SIGN || mCursorType == CursorType::CURSOR_TYPE_WHEEELBARROW || mCursorType == CursorType::CURSOR_TYPE_TREE_FOOD
                       || mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE) {
                if (!ZenGardenItemNumIsZero(mCursorType)) {
                    if (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN && mApp->mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_GOLD_WATERINGCAN] != 0) {
                        MouseDownWithTool(x - 40, y - 40, 0, mCursorType, 0);
                    } else {
                        MouseDownWithTool(x, y, 0, mCursorType, 0);
                    }
                }
            }
        } else {
            if (requestDrawButterInCursor) {
                requestDrawButterInCursor = false;
            } else if (mGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || isCobCannonSelected_2P || mCursorType_2P == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
                if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                } else if ((aGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && mChallengeState == ChallengeState::STATECHALLENGE_NORMAL && mApp->mGameScene == GameScenes::SCENE_PLAYING)
                           || aGameMode == GameMode::GAMEMODE_MP_VS) {
                    mGamepadControls[1]->OnButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, mGamepadControls[1]->mPlayerIndex, 0);
                } else {
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                }
                BaseGamepadControls::MovementState mGameStateNew_2P = mGamepadControls[1]->mGamepadState;
                int seedPacketIndexNew_2P = mGamepadControls[1]->mSelectedSeedIndex;
                int numSeedsInBankNew_2P = seedBank_2P->GetNumSeedsOnConveyorBelt();
                mGamepadControls[1]->mIsInShopSeedBank = mGameStateNew_2P != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                if (mGameState_2P != mGameStateNew_2P) {
                    if (!HasConveyorBeltSeedBank(1) || numSeedsInBank_2P == numSeedsInBankNew_2P) { // 修复传送带关卡种植之后SeedBank动画不正常
                        seedBank_2P->mSeedPackets[seedPacketIndexNew_2P].mLastSelectedTime = 0.0f;  // 动画效果专用
                    }
                }
                //                if (aGameMode == GameMode::GAMEMODE_MP_VS) {
                //                    mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                //                }
            }
        }
    }
    gPlayerIndex = TouchPlayerIndex::TOUCHPLAYER_NONE;
    gSendKeyWhenTouchUp = false;
    gTouchState = TouchState::TOUCHSTATE_NONE;
}


namespace {
int gTouchLastXSecond;
int gTouchLastYSecond;
int gTouchDownXSecond;
int gTouchDownYSecond;
bool gSendKeyWhenTouchUpSecond;
TouchState gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
} // namespace

void Board::MouseDownSecond(int x, int y, int theClickCount) {
    // 触控落下手指在此处理
    gTouchDownXSecond = x;
    gTouchDownYSecond = y;
    gTouchLastXSecond = x;
    gTouchLastYSecond = y;
    if (gKeyboardMode) {
        patchlist::autoPickupSeedPacketDisable.Modify();
    }
    gKeyboardMode = false;

    SeedBank *aSeedBank = mGamepadControls[0]->GetSeedBank();
    int currentSeedBankIndex = mGamepadControls[0]->mSelectedSeedIndex;
    BaseGamepadControls::MovementState aGameState = mGamepadControls[0]->mGamepadState;
    bool isCobCannonSelected = mGamepadControls[0]->mIsCobCannonSelected;

    SeedBank *seedBank_2P = mGamepadControls[1]->GetSeedBank();
    int currentSeedBankIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
    BaseGamepadControls::MovementState aGameState_2P = mGamepadControls[1]->mGamepadState;
    bool isCobCannonSelected_2P = mGamepadControls[1]->mIsCobCannonSelected;
    HitResult hitResult;
    MouseHitTest(x, y, &hitResult, false);
    GameObjectType aObjectType = hitResult.mObjectType;
    GameMode aGameMode = mApp->mGameMode;
    bool isTwoSeedBankMode = (aGameMode == GameMode::GAMEMODE_MP_VS || (aGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && aGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS));
    GameScenes aGameScene = mApp->mGameScene;

    SeedChooserScreen *aSeedChooserScreen = mApp->mSeedChooserScreen;
    if (aGameScene == GameScenes::SCENE_LEVEL_INTRO && aSeedChooserScreen != nullptr && aSeedChooserScreen->mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN) {
        aSeedChooserScreen->GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 0, 0);
        return;
    }
    if (aGameScene == GameScenes::SCENE_LEVEL_INTRO) {
        mCutScene->MouseDown(x, y);
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_SEEDPACKET) {
        if (aGameScene == GameScenes::SCENE_LEVEL_INTRO)
            return;
        auto *aSeedPacket = (SeedPacket *)hitResult.mObject;
        int newSeedPacketIndex = aSeedPacket->mIndex;
        const auto seedPacketPlayerIndex = static_cast<TouchPlayerIndex>(aSeedPacket->GetPlayerIndex());
        if (aGameMode == GameMode::GAMEMODE_MP_VS && gTcpClientSocket >= 0) {
            gPlayerIndexSecond = mGamepadControls[1]->mGamepadIndex == 0 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER1 : TouchPlayerIndex::TOUCHPLAYER_PLAYER2;
            if (seedPacketPlayerIndex != gPlayerIndexSecond)
                return;
        } else {
            gPlayerIndexSecond = seedPacketPlayerIndex;
        }

        if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            requestDrawShovelInCursor = false; // 不再绘制铲子
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            if (isCobCannonSelected) { // 如果拿着加农炮，将其放下
                mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST
                || aGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM) {
                if (aSeedPacket->CanPickUp()) {
                    gSendKeyWhenTouchUpSecond = true;
                } else {
                    mApp->PlaySample(Sexy::SOUND_BUZZER);
                    return;
                }
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) { // 拉老虎机用
                mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_X_BUTTON, 1112);
                return;
            }
            gTouchStateSecond = TouchState::TOUCHSTATE_SEED_BANK; // 记录本次触控的状态
            RefreshSeedPacketFromCursor(0);

            mGamepadControls[0]->mSelectedSeedIndex = newSeedPacketIndex;
            aSeedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用

            if (currentSeedBankIndex != newSeedPacketIndex || aGameState != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                mGamepadControls[0]->mIsInShopSeedBank = false;
                bool isClientGamepadControl = mGamepadControls[0]->mGamepadIndex == 1;
                if (gTcpClientSocket >= 0 && isClientGamepadControl) { // 让对方播放音效
                    U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 2};
                    netplay::PutEvent(event);
                } else {
                    mApp->PlaySample(SOUND_SEEDLIFT);
                    if (gTcpClientSocket >= 0) {
                        U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_SEEDLIFT)};
                        netplay::PutEvent(event);
                    }
                }
            } else if (currentSeedBankIndex == newSeedPacketIndex && aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                if (!isTwoSeedBankMode)
                    mGamepadControls[0]->mIsInShopSeedBank = true;
            }
        } else {
            requestDrawButterInCursor = false; // 不再绘制黄油
            if (isCobCannonSelected_2P) {      // 如果拿着加农炮，将其放下
                mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST
                || aGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM) {
                if (aSeedPacket->CanPickUp()) {
                    gSendKeyWhenTouchUpSecond = true;
                } else {
                    mApp->PlaySample(Sexy::SOUND_BUZZER);
                    return;
                }
            }
            if (aGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) { // 拉老虎机用
                mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_X_BUTTON, 1112);
                return;
            }
            gTouchStateSecond = TouchState::TOUCHSTATE_SEED_BANK; // 记录本次触控的状态
            RefreshSeedPacketFromCursor(1);
            int newSeedPacketIndex_2P = aSeedPacket->mIndex;
            mGamepadControls[1]->mSelectedSeedIndex = newSeedPacketIndex_2P;
            seedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用

            if (currentSeedBankIndex_2P != newSeedPacketIndex_2P || aGameState_2P != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                mGamepadControls[1]->mIsInShopSeedBank = false;

                bool isClientGamepadControl = mGamepadControls[1]->mGamepadIndex == 1;
                if (gTcpClientSocket >= 0 && isClientGamepadControl) { // 让对方播放音效
                    U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 2};
                    netplay::PutEvent(event);
                } else {
                    mApp->PlaySample(SOUND_SEEDLIFT);
                    if (gTcpClientSocket >= 0) {
                        U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_SEEDLIFT)};
                        netplay::PutEvent(event);
                    }
                }
            } else if (currentSeedBankIndex_2P == newSeedPacketIndex_2P && aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                if (!isTwoSeedBankMode)
                    mGamepadControls[1]->mIsInShopSeedBank = true;
            }
        }

        ShuffleButtonDown(aSeedPacket);

        return;
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_SEED_BANK_BLANK) {
        return;
    }

    CursorType mCursorType = mCursorObject[0]->mCursorType;
    CursorType mCursorType_2P = mCursorObject[1]->mCursorType;
    // if (mCursorType == CursorType::CURSOR_TYPE_WATERING_CAN || mCursorType == CursorType::CURSOR_TYPE_FERTILIZER ||
    // mCursorType == CursorType::CURSOR_TYPE_BUG_SPRAY || mCursorType == CursorType::OBJECT_TYPE_PHONOGRAPH ||
    // mCursorType == CursorType::OBJECT_TYPE_CHOCOLATE || mCursorType == CursorType::OBJECT_TYPE_GLOVE ||
    // mCursorType == CursorType::CURSOR_TYPE_MONEY_SIGN || mCursorType == CursorType::CURSOR_TYPE_WHEEELBARROW ||
    // mCursorType == CursorType::CURSOR_TYPE_TREE_FOOD) {
    // MouseDownWithTool(this, x, y, 0, mCursorType, false);
    // return;
    // }

    if (aObjectType == GameObjectType::OBJECT_TYPE_SHOVEL) {
        if (!useNewShovel) {
            mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_QUICK_DIG, 1112);
            return;
        }
        gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_PLAYER1; // 玩家1
        gTouchStateSecond = TouchState::TOUCHSTATE_SHOVEL_RECT;
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            if (!isTwoSeedBankMode)
                mGamepadControls[0]->mIsInShopSeedBank = true;
            int newSeedPacketIndex = mGamepadControls[0]->mSelectedSeedIndex;
            aSeedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
        }
        if (mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            RefreshSeedPacketFromCursor(0);
            ClearCursor(0);
        }
        if (isCobCannonSelected) { // 如果拿着加农炮，将其放下
            mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
        }
        RefreshSeedPacketFromCursor(0);
        if (requestDrawShovelInCursor) {
            requestDrawShovelInCursor = false;
        } else {
            requestDrawShovelInCursor = true;
            mApp->PlayFoley(FoleyType::FOLEY_SHOVEL);
        }
        if (gTcpClientSocket) {
            U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
            netplay::PutEvent(event);
        }
        return;
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_BUTTER) {
        gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_PLAYER2; // 玩家2
        gTouchStateSecond = TouchState::TOUCHSTATE_BUTTER_RECT;
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            if (!isTwoSeedBankMode)
                mGamepadControls[1]->mIsInShopSeedBank = true;
            int newSeedPacketIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
            seedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用
        }
        if (mCursorType_2P == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            RefreshSeedPacketFromCursor(1);
            ClearCursor(1);
        }
        if (isCobCannonSelected_2P) { // 如果拿着加农炮，将其放下
            mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
        }
        RefreshSeedPacketFromCursor(1);
        if (requestDrawButterInCursor) {
            requestDrawButterInCursor = false;
        } else {
            requestDrawButterInCursor = true;
            mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
        }
        return;
    }

    if (aGameMode == GameMode::GAMEMODE_MP_VS) {
        if (gTcpClientSocket >= 0) {
            gPlayerIndexSecond = mGamepadControls[1]->mGamepadIndex == 0 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER1 : TouchPlayerIndex::TOUCHPLAYER_PLAYER2;
        } else {
            gPlayerIndexSecond = PixelToGridX(x, y) > 5 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER2 : TouchPlayerIndex::TOUCHPLAYER_PLAYER1;
        }
    } else if (aGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && aGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS) {
        gPlayerIndexSecond = x > 400 ? TouchPlayerIndex::TOUCHPLAYER_PLAYER2 : TouchPlayerIndex::TOUCHPLAYER_PLAYER1;
    } else {
        gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_PLAYER1;
    }

    if (gPlayerIndex != TouchPlayerIndex::TOUCHPLAYER_NONE && gPlayerIndexSecond == gPlayerIndex) {
        if (aObjectType == GameObjectType::OBJECT_TYPE_PLANT) {
            auto *plant = (Plant *)hitResult.mObject;
            bool isValidCobCannon = plant->mSeedType == SeedType::SEED_COBCANNON && plant->mState == PlantState::STATE_COBCANNON_READY;
            if (!isValidCobCannon) {
                gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_NONE;
                gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
                return;
            }
        } else {
            gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_NONE;
            gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
            return;
        }
    }

    if (aObjectType == GameObjectType::OBJECT_TYPE_COIN) {
        Coin *coin = (Coin *)hitResult.mObject;

        if (coin->mType == CoinType::COIN_USABLE_SEED_PACKET) {
            gTouchStateSecond = TouchState::TOUCHSTATE_USEFUL_SEED_PACKET;
            requestDrawShovelInCursor = false;
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            // if (mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            // LOGD("5656565656");
            // GamepadControls_OnKeyDown(gamepadCon
            // trols1, 27, 1096);//放下手上的植物卡片
            // gSendKeyWhenTouchUp = false;
            // }
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            RefreshSeedPacketFromCursor(0);
            coin->Collect(0);
        }
    }

    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) { // 拉老虎机用
        if (gSlotMachineRect.Contains(x, y)) {
            mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_X_BUTTON, 1112);
            return;
        }
    }

    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON) { // 移动重型武器
        gTouchStateSecond = TouchState::TOUCHSTATE_HEAVY_WEAPON;
        gTouchHeavyWeaponX = mChallenge->mHeavyWeaponX;
        return;
    }

    if (mChallenge->MouseDown(x, y, 0, &hitResult, 0)) {
        if (mApp->IsScaryPotterLevel()) {
            requestDrawShovelInCursor = false;
        }
        if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
            mGamepadControls[0]->mCursorPositionX = x - 40;
            mGamepadControls[0]->mCursorPositionY = y - 40;
        } else {
            mGamepadControls[0]->mCursorPositionX = x;
            mGamepadControls[0]->mCursorPositionY = y;
        }
        if (!mApp->IsWhackAZombieLevel() || aGameState != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR)
            return; // 这一行代码的意义：在锤僵尸关卡，手持植物时，允许拖动种植。
    }


    if (aObjectType == GameObjectType::OBJECT_TYPE_WATERING_CAN || aObjectType == GameObjectType::OBJECT_TYPE_FERTILIZER || aObjectType == GameObjectType::OBJECT_TYPE_BUG_SPRAY
        || aObjectType == GameObjectType::OBJECT_TYPE_PHONOGRAPH || aObjectType == GameObjectType::OBJECT_TYPE_CHOCOLATE || aObjectType == GameObjectType::OBJECT_TYPE_GLOVE
        || aObjectType == GameObjectType::OBJECT_TYPE_MONEY_SIGN || aObjectType == GameObjectType::OBJECT_TYPE_WHEELBARROW || aObjectType == GameObjectType::OBJECT_TYPE_TREE_FOOD) {
        PickUpTool(aObjectType, 0);
        return;
    }

    float tmpX1 = NAN, tmpY1 = NAN;
    tmpX1 = mGamepadControls[0]->mCursorPositionX;
    tmpY1 = mGamepadControls[0]->mCursorPositionY;

    if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
        if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
            mGamepadControls[0]->mCursorPositionX = x - 40;
            mGamepadControls[0]->mCursorPositionY = y - 40;
        } else {
            mGamepadControls[0]->mCursorPositionX = x;
            mGamepadControls[0]->mCursorPositionY = y;
        }
    } else {
        if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
            mGamepadControls[1]->mCursorPositionX = x - 40;
            mGamepadControls[1]->mCursorPositionY = y - 40;
        } else {
            mGamepadControls[1]->mCursorPositionX = x;
            mGamepadControls[1]->mCursorPositionY = y;
        }
    }

    if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || isCobCannonSelected || requestDrawShovelInCursor
            || (mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN && gTouchStateSecond != TouchState::TOUCHSTATE_USEFUL_SEED_PACKET)) {
            gTouchStateSecond = TouchState::TOUCHSTATE_PICKING_SOMETHING;
            gSendKeyWhenTouchUpSecond = true;
        }
    } else {
        if (aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || isCobCannonSelected_2P || requestDrawButterInCursor
            || (mCursorType_2P == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN && gTouchStateSecond != TouchState::TOUCHSTATE_USEFUL_SEED_PACKET)) {
            gTouchStateSecond = TouchState::TOUCHSTATE_PICKING_SOMETHING;
            gSendKeyWhenTouchUpSecond = true;
        }
    }


    if (aObjectType == GameObjectType::OBJECT_TYPE_PLANT) {
        if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1 && requestDrawShovelInCursor)
            return;
        auto *plant = (Plant *)hitResult.mObject;
        bool isValidCobCannon = plant->mSeedType == SeedType::SEED_COBCANNON && plant->mState == PlantState::STATE_COBCANNON_READY;
        if (isValidCobCannon) {
            if (gPlayerIndex == TouchPlayerIndex::TOUCHPLAYER_PLAYER1 && mGamepadControls[1]->mGamepadIndex == -1) {
                mGamepadControls[1]->mCursorPositionX = x;
                mGamepadControls[1]->mCursorPositionY = y;
                mGamepadControls[0]->mCursorPositionX = tmpX1;
                mGamepadControls[0]->mCursorPositionY = tmpY1;
                if (mGamepadControls[0]->mIsCobCannonSelected && mGamepadControls[0]->mCobCannonPlantIndexInList == mPlants.DataArrayGetID(plant)) {
                    // 不能同时选同一个加农炮！
                    gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
                    return;
                }
                mGamepadControls[1]->mIsInShopSeedBank = true;
                mGamepadControls[1]->mGamepadIndex = 1;
                gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_PLAYER2;
                mGamepadControls[1]->pickUpCobCannon(plant);
                gTouchStateSecond = TouchState::TOUCHSTATE_VALID_COBCONON_SECOND;
                return;
            } else if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
                if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                    mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                    gSendKeyWhenTouchUpSecond = false;
                    if (!isTwoSeedBankMode)
                        mGamepadControls[0]->mIsInShopSeedBank = true;
                    int newSeedPacketIndex = mGamepadControls[0]->mSelectedSeedIndex;
                    aSeedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
                }
                mGamepadControls[0]->pickUpCobCannon(plant);
            } else {
                if (aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                    mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                    gSendKeyWhenTouchUpSecond = false;
                    if (!isTwoSeedBankMode)
                        mGamepadControls[1]->mIsInShopSeedBank = true;
                    int newSeedPacketIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
                    seedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用
                }
                mGamepadControls[1]->pickUpCobCannon(plant);
            }
            gTouchStateSecond = TouchState::TOUCHSTATE_VALID_COBCONON;
            return;
        }
    }

    if (gTouchStateSecond == TouchState::TOUCHSTATE_NONE)
        gTouchStateSecond = TouchState::TOUCHSTATE_BOARD;
}


void Board::MouseDragSecond(int x, int y) {
    // Drag函数仅仅负责移动光标即可
    if (gTouchStateSecond == TouchState::TOUCHSTATE_NONE)
        return;

    bool isCobCannonSelected = mGamepadControls[0]->mIsCobCannonSelected;
    bool isCobCannonSelected_2P = mGamepadControls[1]->mIsCobCannonSelected;
    SeedBank *aSeedBank = mGamepadControls[0]->GetSeedBank();
    BaseGamepadControls::MovementState aGameState = mGamepadControls[0]->mGamepadState;
    BaseGamepadControls::MovementState aGameState_2P = mGamepadControls[1]->mGamepadState;
    GameMode aGameMode = mApp->mGameMode;
    bool isTwoSeedBankMode = (aGameMode == GameMode::GAMEMODE_MP_VS || (aGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && aGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS));
    int seedBankHeight = mApp->IsChallengeWithoutSeedBank() ? 87 : aSeedBank->mY + aSeedBank->mHeight;
    if (gTouchStateSecond == TouchState::TOUCHSTATE_SEED_BANK && gTouchLastYSecond < seedBankHeight && y >= seedBankHeight) {
        gTouchStateSecond = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_SEED_BANK;
        if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
            mGamepadControls[0]->mIsInShopSeedBank = false;
            requestDrawShovelInCursor = false;
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            if (gTcpClientSocket >= 0 && mGamepadControls[0]->mGamepadIndex == 1) {
                U8_Event event = {{EventType::EVENT_CLIENT_BOARD_GAMEPAD_SET_STATE}, 7};
                netplay::PutEvent(event);
            }
        } else {
            mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
            mGamepadControls[1]->mIsInShopSeedBank = false;
            requestDrawButterInCursor = false;
            if (gTcpClientSocket >= 0 && mGamepadControls[1]->mGamepadIndex == 1) {
                U8_Event event = {{EventType::EVENT_CLIENT_BOARD_GAMEPAD_SET_STATE}, 7};
                netplay::PutEvent(event);
            }
        }
        gSendKeyWhenTouchUpSecond = true;
    }

    if (gTouchStateSecond == TouchState::TOUCHSTATE_SHOVEL_RECT) {
        if (aGameMode == GameMode::GAMEMODE_MP_VS) {
            if (gTouchVSShovelRect.Contains(gTouchLastXSecond, gTouchLastYSecond) && !gTouchVSShovelRect.Contains(x, y)) {
                gTouchStateSecond = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_SHOVEL_RECT;
                if (!requestDrawShovelInCursor)
                    mApp->PlayFoley(FoleyType::FOLEY_SHOVEL);
                requestDrawShovelInCursor = true;
                if (gTcpClientSocket) {
                    U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                    netplay::PutEvent(event);
                }
                mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                gSendKeyWhenTouchUpSecond = true;
            }
        } else if (gTouchLastYSecond < gTouchShovelRectWidth && y >= gTouchShovelRectWidth) {
            gTouchStateSecond = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_SHOVEL_RECT;
            if (!requestDrawShovelInCursor)
                mApp->PlayFoley(FoleyType::FOLEY_SHOVEL);
            requestDrawShovelInCursor = true;
            if (gTcpClientSocket) {
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_GAMEPAD_PICKUP_SHOVEL}, requestDrawShovelInCursor};
                netplay::PutEvent(event);
            }
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
            gSendKeyWhenTouchUpSecond = true;
        }
    }

    if (gTouchStateSecond == TouchState::TOUCHSTATE_BUTTER_RECT && gTouchLastYSecond < gTouchButterRectWidth && y >= gTouchButterRectWidth) {
        gTouchStateSecond = TouchState::TOUCHSTATE_BOARD_MOVED_FROM_BUTTER_RECT;
        if (!requestDrawButterInCursor)
            mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
        requestDrawButterInCursor = true;
        mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
        gSendKeyWhenTouchUpSecond = true;
    }

    if (gTouchStateSecond == TouchState::TOUCHSTATE_VALID_COBCONON || gTouchStateSecond == TouchState::TOUCHSTATE_VALID_COBCONON_SECOND
        || gTouchStateSecond == TouchState::TOUCHSTATE_USEFUL_SEED_PACKET) {
        if (!gSendKeyWhenTouchUpSecond && (abs(x - gTouchDownXSecond) > gTouchTrigger || abs(y - gTouchDownYSecond) > gTouchTrigger)) {
            gSendKeyWhenTouchUpSecond = true;
        }
    }


    if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1 && isCobCannonSelected && gTouchLastYSecond > seedBankHeight && y <= seedBankHeight) {
        mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096); // 退选加农炮
        gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
        gSendKeyWhenTouchUpSecond = false;
    }

    if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER2 && isCobCannonSelected_2P && gTouchLastYSecond > seedBankHeight && y <= seedBankHeight) {
        mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096); // 退选加农炮
        if (gTouchStateSecond == TouchState::TOUCHSTATE_VALID_COBCONON_SECOND) {
            mApp->ClearSecondPlayer();
            mGamepadControls[1]->mGamepadIndex = -1;
        }
        gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
        gSendKeyWhenTouchUpSecond = false;
    }

    if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
        if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR && gTouchLastYSecond > seedBankHeight && y <= seedBankHeight) {
            mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL; // 退选植物
            if (!isTwoSeedBankMode)
                mGamepadControls[0]->mIsInShopSeedBank = true;
            int newSeedPacketIndex = mGamepadControls[0]->mSelectedSeedIndex;
            SeedBank *seedBank = mGamepadControls[0]->GetSeedBank();
            seedBank->mSeedPackets[newSeedPacketIndex].mLastSelectedTime = 0.0f; // 动画效果专用
            gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
            gSendKeyWhenTouchUpSecond = false;

            if (gTcpClientSocket >= 0 && mGamepadControls[0]->mGamepadIndex == 1) {
                BaseEvent event = {EventType::EVENT_CLIENT_BOARD_TOUCH_CLEAR_CURSOR};
                netplay::PutEvent(event);
            }
        }
    } else {
        if (aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR && gTouchLastYSecond > seedBankHeight && y <= seedBankHeight) {
            mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL; // 退选植物
            if (!isTwoSeedBankMode)
                mGamepadControls[1]->mIsInShopSeedBank = true;
            int newSeedPacketIndex_2P = mGamepadControls[1]->mSelectedSeedIndex;
            SeedBank *seedBank_2P = mGamepadControls[1]->GetSeedBank();
            seedBank_2P->mSeedPackets[newSeedPacketIndex_2P].mLastSelectedTime = 0.0f; // 动画效果专用
            gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
            gSendKeyWhenTouchUpSecond = false;

            if (gTcpClientSocket >= 0 && mGamepadControls[1]->mGamepadIndex == 1) {
                BaseEvent event = {EventType::EVENT_CLIENT_BOARD_TOUCH_CLEAR_CURSOR};
                netplay::PutEvent(event);
            }
        }
    }


    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && gTouchStateSecond == TouchState::TOUCHSTATE_HEAVY_WEAPON) {
        mChallenge->mHeavyWeaponX = gTouchHeavyWeaponX + x - gTouchDownXSecond; // 移动重型武器X坐标
        return;
    }

    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
        return;
    }

    gTouchLastXSecond = x;
    gTouchLastYSecond = y;

    if (gTouchStateSecond != TouchState::TOUCHSTATE_SEED_BANK) {
        if (x > 770)
            x = 770;
        if (x < 40)
            x = 40;
        if (y > 580)
            y = 580;
        if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            mGamepadControls[0]->mCursorPositionX = x;
            mGamepadControls[0]->mCursorPositionY = y;
        } else {
            mGamepadControls[1]->mCursorPositionX = x;
            mGamepadControls[1]->mCursorPositionY = y;
        }
    }
}


void Board::MouseUpSecond(int x, int y, int theClickCount) {
    if (gTouchStateSecond != TouchState::TOUCHSTATE_NONE && gSendKeyWhenTouchUpSecond) {
        SeedBank *aSeedBank = mGamepadControls[0]->GetSeedBank();
        int aNumSeedsOnConveyor = aSeedBank->GetNumSeedsOnConveyorBelt();
        BaseGamepadControls::MovementState aGameState = mGamepadControls[0]->mGamepadState;
        bool aIsCobCannonSelected = mGamepadControls[0]->mIsCobCannonSelected;

        SeedBank *aSeedBank_2P = mGamepadControls[1]->GetSeedBank();
        int aNumSeedsOnConveyor_2P = aSeedBank_2P->GetNumSeedsOnConveyorBelt();
        BaseGamepadControls::MovementState aGameState_2P = mGamepadControls[1]->mGamepadState;
        bool aIsCobCannonSelected_2P = mGamepadControls[1]->mIsCobCannonSelected;

        GameMode aGameMode = mApp->mGameMode;
        CursorType aCursorType = mCursorObject[0]->mCursorType;
        CursorType aCursorType_2P = mCursorObject[1]->mCursorType;
        ChallengeState aChallengeState = mChallenge->mChallengeState;
        GameScenes aGameScene = mApp->mGameScene;

        if (gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            if (requestDrawShovelInCursor) {
                ShovelDown();
            } else if (aGameState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || aIsCobCannonSelected || aCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {

                if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                } else if ((aGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && aChallengeState == ChallengeState::STATECHALLENGE_NORMAL && aGameScene == GameScenes::SCENE_PLAYING)
                           || aGameMode == GameMode::GAMEMODE_MP_VS) {
                    mGamepadControls[0]->OnButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, mGamepadControls[0]->mPlayerIndex, 0);
                } else {
                    if (aGameMode == GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON) { // 重型武器关卡需要设置为状态6才能种植猫尾草
                        mGamepadControls[0]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_SELECT_SEED;
                    }
                    mGamepadControls[0]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                }
                BaseGamepadControls::MovementState mGameStateNew = mGamepadControls[0]->mGamepadState;
                int numSeedsInBankNew = aSeedBank->GetNumSeedsOnConveyorBelt();
                int seedPacketIndexNew = mGamepadControls[0]->mSelectedSeedIndex;
                mGamepadControls[0]->mIsInShopSeedBank = mGameStateNew != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                if (aGameState != mGameStateNew) {
                    if (!HasConveyorBeltSeedBank(0) || aNumSeedsOnConveyor == numSeedsInBankNew) { // 修复传送带关卡种植之后SeedBank动画不正常
                        aSeedBank->mSeedPackets[seedPacketIndexNew].mLastSelectedTime = 0.0f;      // 动画效果专用
                    }
                }
            }
        } else {
            if (requestDrawButterInCursor) {
                requestDrawButterInCursor = false;
            } else if (aGameState_2P == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR || aIsCobCannonSelected_2P || aCursorType_2P == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
                if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_ESCAPE, 1096);
                } else if ((aGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND && aChallengeState == ChallengeState::STATECHALLENGE_NORMAL && aGameScene == GameScenes::SCENE_PLAYING)
                           || aGameMode == GameMode::GAMEMODE_MP_VS) {
                    mGamepadControls[1]->OnButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, mGamepadControls[1]->mPlayerIndex, 0);
                } else {
                    mGamepadControls[1]->OnKeyDown(KeyCode::KEYCODE_RETURN, 1096);
                }
                BaseGamepadControls::MovementState mGameStateNew_2P = mGamepadControls[1]->mGamepadState;
                int numSeedsInBankNew_2P = aSeedBank_2P->GetNumSeedsOnConveyorBelt();
                int seedPacketIndexNew_2P = mGamepadControls[1]->mSelectedSeedIndex;
                mGamepadControls[1]->mIsInShopSeedBank = mGameStateNew_2P != BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
                if (aGameState_2P != mGameStateNew_2P) {
                    if (!HasConveyorBeltSeedBank(1) || aNumSeedsOnConveyor_2P == numSeedsInBankNew_2P) { // 修复传送带关卡种植之后SeedBank动画不正常
                        SeedBank *seedBank_2P = mGamepadControls[1]->GetSeedBank();
                        seedBank_2P->mSeedPackets[seedPacketIndexNew_2P].mLastSelectedTime = 0.0f; // 动画效果专用
                    }
                }
                //                if (aGameMode == GameMode::GAMEMODE_MP_VS) {
                //                    mGamepadControls[1]->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_NORMAL;
                //                }
                if (gTouchStateSecond == TouchState::TOUCHSTATE_VALID_COBCONON_SECOND) {
                    mApp->ClearSecondPlayer();
                    mGamepadControls[1]->mGamepadIndex = -1;
                }
            }
        }
    }
    gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_NONE;
    gSendKeyWhenTouchUpSecond = false;
    gTouchStateSecond = TouchState::TOUCHSTATE_NONE;
}


void Board::StartLevel() {
    if (mApp->IsVSMode()) {
        const bool isGlobalBpFirstRound =
            VSSetupAddonWidget::msGlobalBpMode != VSSetupAddonWidget::GLOBALBP_CLOSED && VSSetupAddonWidget::msGlobalBpWins[0] == 0 && VSSetupAddonWidget::msGlobalBpWins[1] == 0;

        if (isGlobalBpFirstRound) {
            VSResultsMenu::ClearPlayerRecords();
        }

        if (gTcpClientSocket >= 0) {

            // 重置计时器，以与客户端同步舞王的舞步节奏
            mMainCounter = 0;

            BaseEvent nineShortDataEvent = {EventType::EVENT_SERVER_BOARD_START_LEVEL};
            netplay::PutEvent(nineShortDataEvent);
            GridItem *gridItem = nullptr;
            while (IterateGridItems(gridItem)) {

                U16UNI32_Event eventSync{};
                eventSync.type = EventType::EVENT_SERVER_BOARD_SYNC_ID;
                eventSync.data1 = uint16_t(mGridItems.DataArrayGetID(gridItem));
                eventSync.data2.u8x4.u8_1 = 1; // 1 --> GridItem
                eventSync.data2.u8x4.u8_2 = uint8_t(gridItem->mGridItemType);
                eventSync.data2.u8x4.u8_3 = uint8_t(gridItem->mGridX);
                eventSync.data2.u8x4.u8_4 = uint8_t(gridItem->mGridY);
                netplay::PutEvent(eventSync);


                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_GRIDITEM_LAUNCHCOUNTER}, uint16_t(mGridItems.DataArrayGetID(gridItem)), uint16_t(gridItem->mLaunchCounter)};
                netplay::PutEvent(event);
            }

            Plant *plant = nullptr;
            while (IteratePlants(plant)) {

                U16UNI32_Event eventSync{};
                eventSync.type = EventType::EVENT_SERVER_BOARD_SYNC_ID;
                eventSync.data1 = uint16_t(mPlants.DataArrayGetID(plant));
                eventSync.data2.u8x4.u8_1 = 0; // 0 --> Plant
                eventSync.data2.u8x4.u8_2 = uint8_t(plant->mSeedType);
                eventSync.data2.u8x4.u8_3 = uint8_t(plant->mRow);
                eventSync.data2.u8x4.u8_4 = uint8_t(plant->mPlantCol);
                netplay::PutEvent(eventSync);


                U16U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_LAUNCHCOUNTER}, uint16_t(mPlants.DataArrayGetID(plant)), uint16_t(plant->mLaunchCounter)};
                netplay::PutEvent(event);
                //                plant->SyncPingPongAnimationToClient();
                plant->SyncAnimationToClient();
            }
        }
    }
    old_Board_StartLevel(this);
}

void Board::UpdateButtons() {
    SeedChooserScreen *aSeedChooser = mApp->mSeedChooserScreen;
    VSSetupMenu *aVSSetup = mApp->mVSSetupMenu;
    GamepadControls *aGamepad = (gGamePlayerIndex == 1) ? mGamepadControls[1] : mGamepadControls[0];
    if (gKeyDown) {
        aGamepad->OnKeyDown(KeyCode::KEYCODE_QUICK_DIG, 1112);
        aGamepad->mGamepadState = BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR;
        gKeyDown = false;
        gGamePlayerIndex = -1;
    }
    if (gButtonDown) {
        aGamepad->OnButtonDown(gButtonCode, gGamePlayerIndex, 0);
        gButtonDown = false;
        gButtonCode = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
        gGamePlayerIndex = -1;
    }
    if (gButtonDownP1) {
        mGamepadControls[0]->OnButtonDown(gButtonCodeP1, 0, 0);
        gButtonDownP1 = false;
        gButtonCodeP1 = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
    }
    if (gButtonDownP2) {
        mGamepadControls[1]->OnButtonDown(gButtonCodeP2, 0, 0);
        gButtonDownP2 = false;
        gButtonCodeP2 = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
    }
    if (gButtonDownSeedChooser) {
        aSeedChooser->GameButtonDown(gButtonCode, gGamePlayerIndex, 0);
        gButtonDownSeedChooser = false;
        gButtonCode = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
        gGamePlayerIndex = -1;
    }
    if (gButtonDownVSSetup) {
        if (!(aVSSetup->mState == VSSetupMenu::VS_SETUP_STATE_CUSTOM_BATTLE && gButtonCode == Sexy::GamepadButton::GAMEPAD_BUTTON_B)) { // 修复对战选卡阶段按下 B 键崩溃
            aVSSetup->GameButtonDown(gButtonCode, gGamePlayerIndex, 0);
        }
        gButtonDownVSSetup = false;
        gButtonCode = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
        gGamePlayerIndex = -1;
    }

    if (mBoardMenuButton != nullptr) {
        if (mApp->IsVSMode()) {
            mBoardMenuButton->mBtnNoDraw = false;
            mBoardMenuButton->mDisabled = false;
        } else {
            if (mApp->mGameScene == GameScenes::SCENE_PLAYING) {
                mBoardMenuButton->mBtnNoDraw = false;
                mBoardMenuButton->mDisabled = false;
            } else {
                mBoardMenuButton->mBtnNoDraw = true;
                mBoardMenuButton->mDisabled = true;
            }
        }

        if (mBoardFadeOutCounter > 0) {
            mBoardMenuButton->mBtnNoDraw = true;
            mBoardMenuButton->mDisabled = true;
        }
    }
}


void Board::ButtonDepress(int theId) {
    if (theId == 1000) {
        if (gIsServerModeSpectator && mBoardFadeOutCounter > 0) {
            return; // 修复观战在 FadeOut 界面返回会导致 NewOptionsDialog 卡死
        }

        LawnApp *lawnApp = gLawnApp;
        if (lawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN || lawnApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
            lawnApp->DoBackToMain();
            return;
        }
        if (gIsReplayMode) {
            gReplayPauseByMenu = true;
        }
        lawnApp->PlaySample(Sexy::SOUND_PAUSE);
        lawnApp->DoNewOptions(false, 0);
        return;
    } else if (theId == 1001) {
        LawnApp *lawnApp = gLawnApp;
        if (lawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
            Board *mBoard = lawnApp->mBoard;
            mBoard->mChallenge->mChallengeState = ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT;
            mBoard->mBoardStoreButton->mBtnNoDraw = true;
            mBoard->mBoardStoreButton->mDisabled = true;
            mBoard->mBoardStoreButton->Resize(0, 0, 0, 0);
            mBoard->mZombieCountDown = 9;
            mBoard->mZombieCountDownStart = mBoard->mZombieCountDown;
        } else if (lawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
            lawnApp->mZenGarden->OpenStore();
        } else if (lawnApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM) {
            lawnApp->mBoard->mChallenge->TreeOfWisdomOpenStore();
        }
        return;
    }
}

Image *GetIconByAchievementId(AchievementType theAchievementId) {
    switch (theAchievementId) {
        case AchievementType::ACHIEVEMENT_HOME_SECURITY:
            return addonImages.achievement_homeLawnsecurity;
        case AchievementType::ACHIEVEMENT_MORTICULTURALIST:
            return addonImages.achievement_morticulturalist;
        case AchievementType::ACHIEVEMENT_IMMORTAL:
            return addonImages.achievement_immortal;
        case AchievementType::ACHIEVEMENT_SOILPLANTS:
            return addonImages.achievement_soilplants;
        case AchievementType::ACHIEVEMENT_CLOSESHAVE:
            return addonImages.achievement_closeshave;
        case AchievementType::ACHIEVEMENT_CHOMP:
            return addonImages.achievement_chomp;
        case AchievementType::ACHIEVEMENT_VERSUS:
            return addonImages.achievement_versusz;
        case AchievementType::ACHIEVEMENT_GARG:
            return addonImages.achievement_garg;
        case AchievementType::ACHIEVEMENT_COOP:
            return addonImages.achievement_coop;
        case AchievementType::ACHIEVEMENT_SHOP:
            return addonImages.achievement_shop;
        case AchievementType::ACHIEVEMENT_EXPLODONATOR:
            return addonImages.achievement_explodonator;
        case AchievementType::ACHIEVEMENT_TREE:
            return addonImages.achievement_tree;
        default:
            return nullptr;
    }
}

const char *GetNameByAchievementId(AchievementType theAchievementId) {
    switch (theAchievementId) {
        case AchievementType::ACHIEVEMENT_HOME_SECURITY:
            return "ACHIEVEMENT_HOME_SECURITY";
        case AchievementType::ACHIEVEMENT_MORTICULTURALIST:
            return "ACHIEVEMENT_MORTICULTURALIST";
        case AchievementType::ACHIEVEMENT_IMMORTAL:
            return "ACHIEVEMENT_IMMORTAL";
        case AchievementType::ACHIEVEMENT_SOILPLANTS:
            return "ACHIEVEMENT_SOILPLANTS";
        case AchievementType::ACHIEVEMENT_CLOSESHAVE:
            return "ACHIEVEMENT_CLOSESHAVE";
        case AchievementType::ACHIEVEMENT_CHOMP:
            return "ACHIEVEMENT_CHOMP";
        case AchievementType::ACHIEVEMENT_VERSUS:
            return "ACHIEVEMENT_VERSUS";
        case AchievementType::ACHIEVEMENT_GARG:
            return "ACHIEVEMENT_GARG";
        case AchievementType::ACHIEVEMENT_COOP:
            return "ACHIEVEMENT_COOP";
        case AchievementType::ACHIEVEMENT_SHOP:
            return "ACHIEVEMENT_SHOP";
        case AchievementType::ACHIEVEMENT_EXPLODONATOR:
            return "ACHIEVEMENT_EXPLODONATOR";
        case AchievementType::ACHIEVEMENT_TREE:
            return "ACHIEVEMENT_TREE";
        default:
            return "";
    }
}

bool Board::GrantAchievement(AchievementType theAchievementId, bool theIsShow) {
    LawnApp *lawnApp = mApp;
    DefaultPlayerInfo *playerInfo = lawnApp->mPlayerInfo;
    if (!playerInfo->mAchievements[theAchievementId]) {
        mApp->PlaySample(addonSounds.achievement);
        ClearAdviceImmediately();
        const char *theAchievementName = GetNameByAchievementId(theAchievementId);
        pvzstl::string str = TodStringTranslate("[ACHIEVEMENT_GRANTED]");
        pvzstl::string str1 = StrFormat("[%s]", theAchievementName);
        pvzstl::string str2 = TodReplaceString(str, "{achievement}", str1);
        DisplayAdviceAgain("[ACHIEVEMENT_GRANTED]", MessageStyle::MESSAGE_STYLE_ACHIEVEMENT, AdviceType::ADVICE_NEED_ACHIVEMENT_EARNED);
        mAdvice->mIcon = GetIconByAchievementId(theAchievementId);
        playerInfo->mAchievements[theAchievementId] = true;
        return true;
    }
    return false;
}

void Board::FadeOutLevel() {
    old_Board_FadeOutLevel(this);

    if (mApp->IsSurvivalMode() && mChallenge->mSurvivalStage >= 19) {
        GrantAchievement(AchievementType::ACHIEVEMENT_IMMORTAL, true);
    }

    if (!mApp->IsSurvivalMode()) {
        int theNumLawnMowers = 0;
        for (auto &i : mPlantRow) {
            if (i != PlantRowType::PLANTROW_DIRT) {
                theNumLawnMowers++;
            }
        }
        if (mTriggeredLawnMowers == theNumLawnMowers) {
            GrantAchievement(AchievementType::ACHIEVEMENT_CLOSESHAVE, true);
        }
    }
    if (mLevel == 50) {
        GrantAchievement(AchievementType::ACHIEVEMENT_HOME_SECURITY, true);
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING) {
        GrantAchievement(AchievementType::ACHIEVEMENT_COOP, true);
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        if ((VSResultsMenu::msPlayerRecords[VSSide::VS_SIDE_PLANT][3] == 4 && mApp->mBoardResult == BoardResult::BOARDRESULT_VS_PLANT_WON)
            || (VSResultsMenu::msPlayerRecords[VSSide::VS_SIDE_ZOMBIE][3] == 4 && mApp->mBoardResult == BoardResult::BOARDRESULT_VS_ZOMBIE_WON)) {
            GrantAchievement(AchievementType::ACHIEVEMENT_VERSUS, true);
        }
    }

    if (mNewWallNutAndSunFlowerAndChomperOnly && !mApp->IsSurvivalMode() && !HasConveyorBeltSeedBank(0)) {
        int num = mSeedBank[0]->mNumPackets;
        for (int i = 0; i < num; ++i) {
            SeedType theType = mSeedBank[0]->mSeedPackets[i].mPacketType;
            if (theType == SeedType::SEED_CHOMPER || theType == SeedType::SEED_WALLNUT || theType == SeedType::SEED_SUNFLOWER) {
                GrantAchievement(AchievementType::ACHIEVEMENT_CHOMP, true);
                break;
            }
        }
    }
}

void Board::DoPlantingAchievementCheck(SeedType theSeedType) {
    if (theSeedType != SeedType::SEED_CHOMPER && theSeedType != SeedType::SEED_SUNFLOWER && theSeedType != SeedType::SEED_WALLNUT) {
        mNewWallNutAndSunFlowerAndChomperOnly = false;
    }
    if (theSeedType == SeedType::SEED_PEASHOOTER && !HasConveyorBeltSeedBank(0)) {
        mNewPeaShooterCount++;
        if (mNewPeaShooterCount >= 10) {
            GrantAchievement(AchievementType::ACHIEVEMENT_SOILPLANTS, true);
        }
    }
}

void Board::DrawUITop(Sexy::Graphics *g) {
    if (seedBankPin && !mApp->IsSlotMachineLevel()) {
        if (mApp->mGameScene != GameScenes::SCENE_PLANTS_WON && mApp->mGameScene != GameScenes::SCENE_ZOMBIES_WON) {
            // 第一路存在任意存活巨人时，置顶选中的种子栏
            bool hasGargantuarInTopRow = false;

            Zombie *aZombie = nullptr;
            while (IterateZombies(aZombie)) {
                if (aZombie->mDead || !aZombie->mHasHead || aZombie->IsDeadOrDying() || !aZombie->IsOnBoard() || aZombie->mRow != 0) {
                    continue;
                }

                switch (aZombie->mZombieType) {
                    case ZombieType::ZOMBIE_GARGANTUAR:
                    case ZombieType::ZOMBIE_REDEYE_GARGANTUAR:
                    case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
                        hasGargantuarInTopRow = true;
                        break;

                    default:
                        break;
                }

                if (hasGargantuarInTopRow) {
                    break;
                }
            }

            if (hasGargantuarInTopRow) {
                for (int i = 0; i < 2; ++i) {
                    SeedBank *aSeedBank = mGamepadControls[i]->GetSeedBank();
                    if (aSeedBank != nullptr && mGamepadControls[i]->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
                        if (aSeedBank->BeginDraw(g)) {
                            aSeedBank->SeedBank::Draw(g);
                            aSeedBank->EndDraw(g);
                        }
                    }
                }
            }
        }
    }

    old_Board_DrawUITop(this, g);
}

int Board::GetSeedBankExtraWidth() {
    // 去除对战7Packets时Banks的额外宽度
    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        return 0;
    }

    int aNumPackets = mSeedBank[0]->mNumPackets;
    return aNumPackets <= 6 ? 0 : aNumPackets == 7 ? 60 : aNumPackets == 8 ? 76 : aNumPackets == 9 ? 112 : 153;
}

Rect Board::GetShovelButtonRect() {
    // Rect aRect(GetSeedBankExtraWidth() + 456, 0, Sexy::IMAGE_SHOVELBANK->GetWidth(), Sexy::IMAGE_SEEDBANK->GetHeight());
    // if (mApp->IsSlotMachineLevel() || mApp->IsSquirrelLevel())
    // {
    // aRect.mX = 600;
    // }
    // return aRect;

    return old_Board_GetShovelButtonRect(this);
}

void Board::DrawBackdrop(Sexy::Graphics *g) {
    // 实现泳池动态效果、实现对战结盟分界线
    old_Board_DrawBackdrop(this, g);

    // if (mBackground == BackgroundType::BACKGROUND_3_POOL || mBackground == BackgroundType::BACKGROUND_4_FOG) {
    // PoolEffect_PoolEffectDraw(this->mApp->mPoolEffect, g, Board_StageIsNight(this));
    // }

    GameMode mGameMode = mApp->mGameMode;
    if (mGameMode == GameMode::GAMEMODE_MP_VS) {
        switch (mBackground) {
            case BackgroundType::BACKGROUND_1_DAY:
                g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 512, 73);
                break;
            case BackgroundType::BACKGROUND_2_NIGHT:
                g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 512, 73);
                break;
            case BackgroundType::BACKGROUND_3_POOL:
                g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 512, 73);
                break;
            case BackgroundType::BACKGROUND_4_FOG:
                g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 512, 73);
                break;
            case BackgroundType::BACKGROUND_5_ROOF:
                g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 512, 73);
                break;
            case BackgroundType::BACKGROUND_6_BOSS:
                g->DrawImage(Sexy::IMAGE_WALLNUT_BOWLINGSTRIPE, 512, 73);
                break;
            default:
                break;
        }
        return;
    }
    if (mGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && mGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS && mGameMode != GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING) {
        switch (mBackground) {
            case BackgroundType::BACKGROUND_1_DAY:
                g->DrawImage(addonImages.stripe_day_coop, 384, 69);
                break;
            case BackgroundType::BACKGROUND_2_NIGHT:
                g->DrawImage(addonImages.stripe_day_coop, 384, 69);
                break;
            case BackgroundType::BACKGROUND_3_POOL:
                g->DrawImage(addonImages.stripe_pool_coop, 348, 72);
                break;
            case BackgroundType::BACKGROUND_4_FOG:
                g->DrawImage(addonImages.stripe_pool_coop, 348, 72);
                break;
            case BackgroundType::BACKGROUND_5_ROOF:
                g->DrawImage(addonImages.stripe_roof_left, 365, 82);
                break;
            case BackgroundType::BACKGROUND_6_BOSS:
                g->DrawImage(addonImages.stripe_roof_left, 365, 82);
                break;
            default:
                break;
        }
    }
}

bool Board::RowCanHaveZombieType(int theRow, ZombieType theZombieType) {
    if (!RowCanHaveZombies(theRow)) {
        return false;
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_POOL_PARTY) {
        return Zombie::ZombieTypeCanGoInPool(theZombieType);
    }

    if (mApp->IsVSMode() && mPlantRow[theRow] == PlantRowType::PLANTROW_POOL
        && (theZombieType == ZombieType::ZOMBIE_NORMAL || theZombieType == ZombieType::ZOMBIE_TRAFFIC_CONE || theZombieType == ZombieType::ZOMBIE_PAIL || theZombieType == ZombieType::ZOMBIE_FLAG)) {
        return true; // 修复泳池对战放置旗帜僵尸在水路不出怪
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_RESODDED && mPlantRow[theRow] == PlantRowType::PLANTROW_DIRT && mCurrentWave < 5) {
        return false; // 无草皮之地关卡，无草皮的行在前 5 波不刷出僵尸
    }
    if (mPlantRow[theRow] == PlantRowType::PLANTROW_POOL && !Zombie::ZombieTypeCanGoInPool(theZombieType)) {
        return false; // 水路不会刷出不能进入泳池的僵尸
    }
    if (mPlantRow[theRow] == PlantRowType::PLANTROW_HIGH_GROUND && !Zombie::ZombieTypeCanGoOnHighGround(theZombieType)) {
        return false; // 高地不会刷出不能走上高地的僵尸
    }

    int aCurrentWave = mCurrentWave;
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
        aCurrentWave += mChallenge->mSurvivalStage * GetNumWavesPerSurvivalStage();
    }
    // 非水路不能刷出水路僵尸；前 5 小波，水面仅刷出潜水僵尸或海豚骑士僵尸
    if (mPlantRow[theRow] == PlantRowType::PLANTROW_POOL) {
        if (aCurrentWave < 5 && !IsZombieTypePoolOnly(theZombieType)) {
            return false;
        }
    } else if (IsZombieTypePoolOnly(theZombieType)) {
        return false;
    }
    // 雪橇僵尸小队仅能在有冰道的行刷出
    if (theZombieType == ZOMBIE_BOBSLED && !mIceTimer[theRow]) {
        return false;
    }
    // “自古一路无巨人”（生存模式除外）
    if (theRow == 0 && !LawnApp::IsSurvivalEndless(mApp->mGameMode)) {
        if (theZombieType == ZombieType::ZOMBIE_GARGANTUAR || theZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR) {
            return false;
        }
    }
    // 非舞王僵尸或当前为泳池关卡，则可以刷出该僵尸
    if (theZombieType != ZombieType::ZOMBIE_DANCER || StageHasPool()) {
        return true;
    }
    // 舞王僵尸在非泳池关卡中，为保证能召唤伴舞僵尸，仅在中间三行刷出
    return RowCanHaveZombies(theRow - 1) && RowCanHaveZombies(theRow + 1);
}

void Board::ShakeBoard(int theShakeAmountX, int theShakeAmountY) {
    old_Board_ShakeBoard(this, theShakeAmountX, theShakeAmountY);

    // 添加 手机振动效果
    switch (theShakeAmountY) {
        case 4: // PHASE_SQUASH_FALLING
        case 3: // GARGANTUAR_DEATH
        case 2: // BOSS_EXPLOSION, BOSS_RV_THROW
            TriggerVibration(VibrationEffect::VIBRATION_THUMP);
            break;
        case -2: // WALLNUT_BOWLING_IMPACT
            TriggerVibration(VibrationEffect::VIBRATION_BOWLING);
            break;
        case -4: // PLANT_BURN
        case -6: // JACK_IN_THE_BOX_POPPING
            TriggerVibration(VibrationEffect::VIBRATION_EXPLOSION);
            break;
        default:
            break;
    }
}

int Board::GetNumSeedsInBank(bool isZombieBank) {
    // 对战额外卡槽
    if (mApp->IsVSMode()) {
        VSSetupMenu *setupMenu = mApp->mVSSetupMenu;
        if (setupMenu) {
            bool isExtraPacket = setupMenu->mAddonWidget->mExtraPacketMode;
            return isExtraPacket ? 7 : 6;
        }
    }

    return old_Board_GetNumSeedsInBank(this, isZombieBank);
}

int Board::GetSeedPacketPositionX(int thePacketIndex, int theSeedBankIndex, bool isZombiePlayer) {
    if (mApp->IsSlotMachineLevel()) {
        return 59 * thePacketIndex + 247;
    }
    if (HasConveyorBeltSeedBank(0)) {
        if (mApp->IsCoopMode()) {
            return 50 * thePacketIndex + 10;
        }
        return 50 * thePacketIndex + 91;
    }

    const int aNumPackets = mSeedBank[theSeedBankIndex]->mNumPackets;
    if (aNumPackets <= 6) {
        return 59 * thePacketIndex + 85 + (isZombiePlayer ? -70 : 0);
    }
    if (aNumPackets == 7) {
        if (mApp->IsVSMode()) {
            return 51 * thePacketIndex + 79 + (isZombiePlayer ? -68 : 0); // 额外卡槽
        }
        return 59 * thePacketIndex + 85;
    }
    if (aNumPackets == 8) {
        return 54 * thePacketIndex + 81;
    }
    if (aNumPackets == 9) {
        return 52 * thePacketIndex + 80;
    }
    return 51 * thePacketIndex + 79;
}

void Board::RemoveParticleByType(ParticleEffect theEffectType) {
    TodParticleSystem *aParticle = nullptr;
    while (IterateParticles(aParticle)) {
        if (aParticle->mEffectType == theEffectType) {
            aParticle->ParticleSystemDie();
        }
    }
}

GridItem *Board::GetGridItemAt(GridItemType theGridItemType, int theGridX, int theGridY) {
    GridItem *aGridItem = nullptr;
    while (IterateGridItems(aGridItem)) {
        if (aGridItem->mGridX == theGridX && aGridItem->mGridY == theGridY && aGridItem->mGridItemType == theGridItemType) {
            return aGridItem;
        }
    }
    return nullptr;
}

GridItem *Board::GetCraterAt(int theGridX, int theGridY) {
    return GetGridItemAt(GridItemType::GRIDITEM_CRATER, theGridX, theGridY);
}

GridItem *Board::GetGraveStoneAt(int theGridX, int theGridY) {
    return GetGridItemAt(GridItemType::GRIDITEM_GRAVESTONE, theGridX, theGridY);
}

GridItem *Board::GetLadderAt(int theGridX, int theGridY) {
    return GetGridItemAt(GridItemType::GRIDITEM_LADDER, theGridX, theGridY);
}

GridItem *Board::GetScaryPotAt(int theGridX, int theGridY) {
    return GetGridItemAt(GridItemType::GRIDITEM_SCARY_POT, theGridX, theGridY);
}

GridItem *Board::GetMoundAt(int theGridX, int theGridY) {
    return GetGridItemAt(GridItemType::GRIDITEM_MP_BURIAL_MOUND, theGridX, theGridY);
}

int Board::PixelToGridXKeepOnBoard(int theX, int theY) {
    int aGridX = PixelToGridX(theX, theY);
    return std::max(aGridX, 0);
}

int Board::GridToPixelX(int theGridX, int theGridY) const {
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
        if (mBackground == BackgroundType::BACKGROUND_GREENHOUSE || mBackground == BackgroundType::BACKGROUND_MUSHROOM_GARDEN || mBackground == BackgroundType::BACKGROUND_ZOMBIQUARIUM) {
            return mApp->mZenGarden->GridToPixelX(theGridX, theGridY);
        }
    }

    return theGridX * 80 + LAWN_XMIN;
}

int Board::PixelToGridYKeepOnBoard(int theX, int theY) {
    int aGridY = PixelToGridY(std::max(theX, 80), theY);
    return std::max(aGridY, 0);
}

int Board::GridToPixelY(int theGridX, int theGridY) {
    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
        if (mBackground == BackgroundType::BACKGROUND_GREENHOUSE || mBackground == BackgroundType::BACKGROUND_MUSHROOM_GARDEN || mBackground == BackgroundType::BACKGROUND_ZOMBIQUARIUM) {
            return mApp->mZenGarden->GridToPixelY(theGridX, theGridY);
        }
    }

    int aY = 0;
    if (StageHasRoof()) {
        int aSlopeOffset = 0;
        if (theGridX < 5) {
            aSlopeOffset = (5 - theGridX) * 20;
        } else {
            aSlopeOffset = 0;
        }
        aY = theGridY * 85 + aSlopeOffset + LAWN_YMIN - 10;
    } else if (StageHasPool()) {
        aY = theGridY * 85 + LAWN_YMIN;
    } else {
        aY = theGridY * 100 + LAWN_YMIN;
    }

    if (theGridX != -1 && mGridSquareType[theGridX][theGridY] == GridSquareType::GRIDSQUARE_HIGH_GROUND) {
        aY -= HIGH_GROUND_HEIGHT;
    }

    return aY;
}

int GetRectOverlap(const Rect &rect1, const Rect &rect2) {
    int xmax = 0, rmin = 0, rmax = 0;

    if (rect1.mX < rect2.mX) {
        rmin = rect1.mX + rect1.mWidth;
        rmax = rect2.mX + rect2.mWidth;
        xmax = rect2.mX;
    } else {
        rmin = rect2.mX + rect2.mWidth;
        rmax = rect1.mX + rect1.mWidth;
        xmax = rect1.mX;
    }

    if (rmin > xmax && rmin > rmax) {
        rmin = rmax;
    }

    return rmin - xmax;
}

bool GetCircleRectOverlap(int theCircleX, int theCircleY, int theRadius, const Rect &theRect) {
    int dx = 0;        // 圆心与矩形较近一条纵边的横向距离
    int dy = 0;        // 圆心与矩形较近一条横边的纵向距离
    bool xOut = false; // 圆心横坐标是否不在矩形范围内
    bool yOut = false; // 圆心纵坐标是否不在矩形范围内

    if (theCircleX < theRect.mX) {
        xOut = true;
        dx = theRect.mX - theCircleX;
    } else if (theCircleX > theRect.mX + theRect.mWidth) {
        xOut = true;
        dx = theCircleX - theRect.mX - theRect.mWidth;
    }
    if (theCircleY < theRect.mY) {
        yOut = true;
        dy = theRect.mY - theCircleY;
    } else if (theCircleY > theRect.mY + theRect.mHeight) {
        yOut = true;
        dy = theCircleY - theRect.mY - theRect.mHeight;
    }

    if (!xOut && !yOut) // 如果圆心在矩形内
    {
        return true;
    } else if (xOut && yOut) {
        return dx * dx + dy * dy <= theRadius * theRadius;
    } else if (xOut) {
        return dx <= theRadius;
    } else {
        return dy <= theRadius;
    }
}

int Board::MakeRenderOrder(RenderLayer theRenderLayer, int theRow, int theLayerOffset) {
    return theRow * (int)RenderLayer::RENDER_LAYER_ROW_OFFSET + theRenderLayer + theLayerOffset;
}

void Board::FixReanimErrorAfterLoad() {
    // 修复读档后的各种问题
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        ZombieType zombieType = aZombie->mZombieType;
        Reanimation *mBodyReanim = mApp->ReanimationGet(aZombie->mBodyReanimID);
        if (mBodyReanim == nullptr)
            return;

        if (!aZombie->mHasArm) {
            aZombie->SetupLostArmReanim();
        }
        // 修复读档后豌豆、机枪、倭瓜僵尸头部变为普通僵尸
        if (zombieType == ZombieType::ZOMBIE_PEA_HEAD || zombieType == ZombieType::ZOMBIE_GATLING_HEAD || zombieType == ZombieType::ZOMBIE_SQUASH_HEAD) {
            mBodyReanim->SetImageOverride("anim_head1", IMAGE_BLANK);
        }

        // 修复读档后盾牌贴图变为满血盾牌贴图、垃圾桶变为铁门
        if (aZombie->mShieldType != ShieldType::SHIELDTYPE_NONE) {
            int aShieldDamageIndex = aZombie->GetShieldDamageIndex();
            switch (aZombie->mShieldType) {
                case ShieldType::SHIELDTYPE_DOOR:
                    switch (aShieldDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("anim_screendoor", Sexy::IMAGE_REANIM_ZOMBIE_SCREENDOOR2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("anim_screendoor", Sexy::IMAGE_REANIM_ZOMBIE_SCREENDOOR3);
                            break;
                        default:
                            break;
                    }
                    break;
                case ShieldType::SHIELDTYPE_NEWSPAPER:
                    switch (aShieldDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("Zombie_paper_paper", Sexy::IMAGE_REANIM_ZOMBIE_PAPER_PAPER2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("Zombie_paper_paper", Sexy::IMAGE_REANIM_ZOMBIE_PAPER_PAPER3);
                            break;
                        default:
                            break;
                    }
                    break;
                case ShieldType::SHIELDTYPE_LADDER:
                    switch (aShieldDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("Zombie_ladder_1", Sexy::IMAGE_REANIM_ZOMBIE_LADDER_1_DAMAGE1);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("Zombie_ladder_1", Sexy::IMAGE_REANIM_ZOMBIE_LADDER_1_DAMAGE2);
                            break;
                        default:
                            break;
                    }
                    break;
                case ShieldType::SHIELDTYPE_TRASHCAN:
                    switch (aShieldDamageIndex) {
                        case 0:
                            mBodyReanim->SetImageOverride("anim_screendoor", Sexy::IMAGE_REANIM_ZOMBIE_TRASHCAN1);
                            break;
                        case 1:
                            mBodyReanim->SetImageOverride("anim_screendoor", Sexy::IMAGE_REANIM_ZOMBIE_TRASHCAN2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("anim_screendoor", Sexy::IMAGE_REANIM_ZOMBIE_TRASHCAN3);
                            break;
                        default:
                            break;
                    }
                    break;
                case ShieldType::SHIELDTYPE_SUNDAY_EDITION:
                    switch (aShieldDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("Zombie_paper_paper", addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("Zombie_paper_paper", addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER3);
                            break;
                        default:
                            break;
                    }
                    break;
                case ShieldType::SHIELDTYPE_NONE:
                    std::unreachable();
            }
        }

        // 修复读档后头盔贴图变为满血头盔贴图
        if (aZombie->mHelmType != HelmType::HELMTYPE_NONE) {
            int helmDamageIndex = aZombie->GetHelmDamageIndex();
            switch (aZombie->mHelmType) {
                case HelmType::HELMTYPE_TRAFFIC_CONE:
                    switch (helmDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("anim_cone", Sexy::IMAGE_REANIM_ZOMBIE_CONE2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("anim_cone", Sexy::IMAGE_REANIM_ZOMBIE_CONE3);
                            break;
                        default:
                            break;
                    }
                    break;
                case HelmType::HELMTYPE_PAIL:
                    switch (helmDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("anim_bucket", Sexy::IMAGE_REANIM_ZOMBIE_BUCKET2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("anim_bucket", Sexy::IMAGE_REANIM_ZOMBIE_BUCKET3);
                            break;
                        default:
                            break;
                    }
                    break;
                case HelmType::HELMTYPE_FOOTBALL:
                    switch (helmDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("zombie_football_helmet", Sexy::IMAGE_REANIM_ZOMBIE_FOOTBALL_HELMET2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("zombie_football_helmet", Sexy::IMAGE_REANIM_ZOMBIE_FOOTBALL_HELMET3);
                            break;
                        default:
                            break;
                    }
                    break;
                case HelmType::HELMTYPE_DIGGER:
                    switch (helmDamageIndex) {
                        case 1:
                            mBodyReanim->SetImageOverride("Zombie_digger_hardhat", Sexy::IMAGE_REANIM_ZOMBIE_DIGGER_HARDHAT2);
                            break;
                        case 2:
                            mBodyReanim->SetImageOverride("Zombie_digger_hardhat", Sexy::IMAGE_REANIM_ZOMBIE_DIGGER_HARDHAT3);
                            break;
                        default:
                            break;
                    }
                    break;
                case HelmType::HELMTYPE_WALLNUT: {
                    Reanimation *mSpecialHeadReanim = mApp->ReanimationGet(aZombie->mSpecialHeadReanimID);
                    switch (helmDamageIndex) {
                        case 1:
                            mSpecialHeadReanim->SetImageOverride("anim_face", Sexy::IMAGE_REANIM_WALLNUT_CRACKED1);
                            break;
                        case 2:
                            mSpecialHeadReanim->SetImageOverride("anim_face", Sexy::IMAGE_REANIM_WALLNUT_CRACKED2);
                            break;
                        default:
                            break;
                    }
                } break;
                case HelmType::HELMTYPE_TALLNUT: {
                    Reanimation *mSpecialHeadReanim = mApp->ReanimationGet(aZombie->mSpecialHeadReanimID);
                    switch (helmDamageIndex) {
                        case 1:
                            mSpecialHeadReanim->SetImageOverride("anim_face", Sexy::IMAGE_REANIM_TALLNUT_CRACKED1);
                            break;
                        case 2:
                            mSpecialHeadReanim->SetImageOverride("anim_face", Sexy::IMAGE_REANIM_TALLNUT_CRACKED2);
                            break;
                        default:
                            break;
                    }
                } break;
                default:
                    break;
            }
        }

        // 修复读档后巨人僵尸创可贴消失、红眼巨人变白眼巨人
        if (zombieType == ZombieType::ZOMBIE_GARGANTUAR || zombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR) {
            int bodyDamageIndex = aZombie->GetBodyDamageIndex();
            switch (bodyDamageIndex) {
                case 0:
                    if (zombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR)
                        mBodyReanim->SetImageOverride("anim_head1", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD_REDEYE);
                    break;
                case 1:
                    if (zombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR)
                        mBodyReanim->SetImageOverride("anim_head1", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD_REDEYE);
                    mBodyReanim->SetImageOverride("Zombie_gargantua_body1", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_BODY1_2);
                    mBodyReanim->SetImageOverride("Zombie_gargantuar_outerarm_lower", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_OUTERARM_LOWER2);
                    break;
                case 2:
                    mBodyReanim->SetImageOverride("Zombie_gargantua_body1", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_BODY1_3);
                    mBodyReanim->SetImageOverride("Zombie_gargantuar_outerleg_foot", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_FOOT2);
                    mBodyReanim->SetImageOverride("Zombie_gargantuar_outerarm_lower", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_OUTERARM_LOWER2);
                    mBodyReanim->SetImageOverride("anim_head1",
                                                  zombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR ? Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD2_REDEYE : Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD2);
                    break;
                default:
                    break;
            }
        }

        // 修复读档后僵尸博士机甲变全新机甲
        if (zombieType == ZombieType::ZOMBIE_BOSS) {
            int bodyDamageIndex = aZombie->GetBodyDamageIndex();
            switch (bodyDamageIndex) {
                case 1:
                    mBodyReanim->SetImageOverride("Boss_head", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_HEAD_DAMAGE1);
                    mBodyReanim->SetImageOverride("Boss_jaw", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_JAW_DAMAGE1);
                    mBodyReanim->SetImageOverride("Boss_outerarm_hand", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_HAND_DAMAGE1);
                    mBodyReanim->SetImageOverride("Boss_outerarm_thumb2", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_HAND_DAMAGE2);
                    mBodyReanim->SetImageOverride("Boss_innerleg_foot", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_FOOT_DAMAGE1);
                    break;
                case 2:
                    mBodyReanim->SetImageOverride("Boss_head", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_HEAD_DAMAGE2);
                    mBodyReanim->SetImageOverride("Boss_jaw", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_JAW_DAMAGE2);
                    mBodyReanim->SetImageOverride("Boss_outerarm_hand", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_HAND_DAMAGE2);
                    mBodyReanim->SetImageOverride("Boss_outerarm_thumb2", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_OUTERARM_THUMB_DAMAGE2);
                    mBodyReanim->SetImageOverride("Boss_outerleg_foot", Sexy::IMAGE_REANIM_ZOMBIE_BOSS_FOOT_DAMAGE2);
                    break;
                default:
                    break;
            }
        }

        // 修复读档后冰车变全新冰车
        if (zombieType == ZombieType::ZOMBIE_ZAMBONI) {
            int bodyDamageIndex = aZombie->GetBodyDamageIndex();
            switch (bodyDamageIndex) {
                case 1:
                    mBodyReanim->SetImageOverride("Zombie_zamboni_1", Sexy::IMAGE_REANIM_ZOMBIE_ZAMBONI_1_DAMAGE1);
                    mBodyReanim->SetImageOverride("Zombie_zamboni_2", Sexy::IMAGE_REANIM_ZOMBIE_ZAMBONI_2_DAMAGE1);
                    break;
                case 2:
                    mBodyReanim->SetImageOverride("Zombie_zamboni_1", Sexy::IMAGE_REANIM_ZOMBIE_ZAMBONI_1_DAMAGE2);
                    mBodyReanim->SetImageOverride("Zombie_zamboni_2", Sexy::IMAGE_REANIM_ZOMBIE_ZAMBONI_2_DAMAGE2);
                    break;
                default:
                    break;
            }
        }

        // 修复读档后投篮车变全新投篮车
        if (zombieType == ZombieType::ZOMBIE_CATAPULT) {
            int bodyDamageIndex = aZombie->GetBodyDamageIndex();
            switch (bodyDamageIndex) {
                case 1:
                case 2:
                    mBodyReanim->SetImageOverride("Zombie_catapult_siding", Sexy::IMAGE_REANIM_ZOMBIE_CATAPULT_SIDING_DAMAGE);
                    break;
                default:
                    break;
            }
        }
    }

    // 修复读档后雏菊、糖果变色、泳池闪光消失
    TodParticleSystem *aParticle = nullptr;
    while (IterateParticles(aParticle)) {
        if (aParticle->mEffectType == ParticleEffect::PARTICLE_ZOMBIE_DAISIES || aParticle->mEffectType == ParticleEffect::PARTICLE_ZOMBIE_PINATA) {
            // 设置颜色
            aParticle->OverrideColor(nullptr, gColorWhite);
        } else if (aParticle->mEffectType == ParticleEffect::PARTICLE_POOL_SPARKLY) {
            // 直接删除泳池闪光特效
            aParticle->ParticleSystemDie();
            mPoolSparklyParticleID = PARTICLESYSTEMID_NULL;
        }
    }

    if (mBackground == BackgroundType::BACKGROUND_3_POOL) {
        // 添加泳池闪光特效
        TodParticleSystem *poolSparklyParticle = mApp->AddTodParticle(450.0, 295.0, 220000, ParticleEffect::PARTICLE_POOL_SPARKLY);
        mPoolSparklyParticleID = mApp->ParticleGetID(poolSparklyParticle);
    }
}

bool Board::PlantUsesAcceleratedPricing(SeedType theSeedType) const {
    return Plant::IsUpgrade(theSeedType) && LawnApp::IsSurvivalEndless(mApp->mGameMode);
}

bool Board::IsPlantInCursor() {
    return mCursorObject[0]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_BANK || mCursorObject[0]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN
        || mCursorObject[0]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_GLOVE || mCursorObject[0]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_DUPLICATOR
        || mCursorObject[0]->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_WHEEL_BARROW;
}

void Board::RemoveAllPlants() {
    for (Plant *aPlant = nullptr; IteratePlants(aPlant); aPlant->Die())
        ;
}

void Board::RemoveAllZombies() {
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (!aZombie->IsDeadOrDying())
            aZombie->DieNoLoot();
    }
}

void Board::RemoveAllGridItems() {
    for (GridItem *aItem = nullptr; IterateGridItems(aItem); aItem->GridItemDie())
        ;
}

bool Board::IsValidCobCannonSpotHelper(int theGridX, int theGridY) {
    PlantsOnLawn aPlantOnLawn{};
    GetPlantsOnLawn(theGridX, theGridY, &aPlantOnLawn);
    if (aPlantOnLawn.mPumpkinPlant)
        return false;

    if (aPlantOnLawn.mNormalPlant && aPlantOnLawn.mNormalPlant->mSeedType == SeedType::SEED_KERNELPULT)
        return true;

    return mApp->mEasyPlantingCheat && CanPlantAt(theGridX, theGridY, SeedType::SEED_KERNELPULT) == PlantingReason::PLANTING_OK;
}

bool Board::IsPoolSquare(int theGridX, int theGridY) {
    if (theGridX >= 0 && theGridY >= 0) {
        return mGridSquareType[theGridX][theGridY] == GridSquareType::GRIDSQUARE_POOL;
    }
    return false;
}

int Board::TotalZombiesHealthInWave(int theWaveIndex) {
    int aTotalHealth = 0;
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (aZombie->mFromWave == theWaveIndex && !aZombie->mMindControlled && !aZombie->IsDeadOrDying() && aZombie->mZombieType != ZombieType::ZOMBIE_BUNGEE
            && aZombie->mRelatedZombieID == ZombieID::ZOMBIEID_NULL) {
            aTotalHealth += aZombie->mBodyHealth + aZombie->mHelmHealth + aZombie->mShieldHealth / 5 + aZombie->mFlyingHealth;
        }
    }
    return aTotalHealth;
}

int Board::KillAllZombiesInRadius_Custom(int theRow, int theX, int theY, int theRadius, int theRowRange, bool theBurn, int theDamageRangeFlags) {
    Zombie *aZombie = nullptr;
    int aKilledZombies = 0;
    while (IterateZombies(aZombie)) {
        if (aZombie->EffectedByDamage(theDamageRangeFlags)) {
            Rect aZombieRect = aZombie->GetZombieRect();
            int aRowDist = aZombie->mRow - theRow;
            if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS) {
                aRowDist = 0;
            }

            if (aRowDist <= theRowRange && aRowDist >= -theRowRange && GetCircleRectOverlap(theX, theY, theRadius, aZombieRect)) {
                if (theBurn) {
                    aZombie->ApplyBurn();
                } else {
                    aZombie->TakeDamage(1800, 18U);
                }

                aKilledZombies++;
            }
        }
    }

    int aGridX = PixelToGridXKeepOnBoard(theX, theY);
    int aGridY = PixelToGridYKeepOnBoard(theX, theY);
    GridItem *aGridItem = nullptr;
    while (IterateGridItems(aGridItem)) {
        if (aGridItem->mGridItemType == GridItemType::GRIDITEM_LADDER || (theDamageRangeFlags == 127 && aGridItem->mGridItemType == GridItemType::GRIDITEM_POLE)) {
            if (GridInRange(aGridItem->mGridX, aGridItem->mGridY, aGridX, aGridY, theRowRange, theRowRange)) {
                aGridItem->GridItemDie();
            }
        }
    }

    return aKilledZombies;
}

void Board::KillAllPlantsInRadius(int theX, int theY, int theRadius) {
    Plant *aPlant = nullptr;
    while (IteratePlants(aPlant)) {
        if (GetCircleRectOverlap(theX, theY, theRadius, aPlant->GetPlantRect())) {
            mPlantsEaten++;
            aPlant->Die();
        }
    }
}

void Board::PlantsTakeDamageInGrid(int theGridX, int theGridY, int theDamage) {
    Plant *aPlant = GetTopPlantAt(theGridX, theGridY, PlantPriority::TOPPLANT_EATING_ORDER);
    if (aPlant != nullptr && !aPlant->IsInvulnerable()) {
        aPlant->mPlantHealth -= theDamage;
        aPlant->mEatenFlashCountdown = std::max(aPlant->mEatenFlashCountdown, 50);
    }
}

void Board::RemoveCutsceneZombies() {
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (aZombie->mFromWave == Zombie::ZOMBIE_WAVE_CUTSCENE) {
            aZombie->DieNoLoot();
        }
    }
}

int Board::CountZombiesOnScreen() {
    int aCount = 0;
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (aZombie->mHasHead && !aZombie->IsDeadOrDying() && !aZombie->mMindControlled && aZombie->IsOnBoard()) {
            aCount++;
        }
    }
    return aCount;
}

float Board::GetPosYBasedOnRow(float thePosX, int theRow) {
    if (StageHasRoof()) {
        float aSlopeOffset = 0.0f;
        if (thePosX < 440.0f) {
            aSlopeOffset = (440.0f - thePosX) * 0.25f;
        }

        return GridToPixelY(8, theRow) + aSlopeOffset;
    }

    return GridToPixelY(0, theRow);
}

Zombie *Board::GetBossZombie() {
    Zombie *aZombie = nullptr;
    while (IterateZombies(aZombie)) {
        if (aZombie->mZombieType == ZombieType::ZOMBIE_BOSS) {
            return aZombie;
        }
    }
    return nullptr;
}

GamepadControls *Board::GetGamepadControlsByPlayerIndex(int thePlayerIndex) {
    GamepadControls *aGamepad = mGamepadControls[0];
    if (aGamepad->mPlayerIndex != thePlayerIndex) {
        aGamepad = mGamepadControls[1];
        if (aGamepad->mPlayerIndex != thePlayerIndex)
            return nullptr;
    }
    return aGamepad;
}

GridItem *Board::AddACrater_Origin(int theGridX, int theGridY) {
    GridItem *aCrater = mGridItems.DataArrayAlloc();
    aCrater->mGridItemType = GridItemType::GRIDITEM_CRATER;
    aCrater->mRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_GROUND, theGridY, 1);
    aCrater->mGridX = theGridX;
    aCrater->mGridY = theGridY;
    return aCrater;
}

GridItem *Board::AddACrater(int theGridX, int theGridY) {
    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode) {
        return nullptr;
    }

    GridItem *aCrater = AddACrater_Origin(theGridX, theGridY);

    if (gTcpClientSocket >= 0 && mApp->mGameScene == SCENE_PLAYING) {
        U8U8U16_Event event = {{EventType::EVENT_SERVER_BOARD_GRIDITEM_ADDCRATER}, uint8_t(theGridX), uint8_t(theGridY), uint16_t(mGridItems.DataArrayGetID(aCrater))};
        netplay::PutEvent(event);
    }
    return aCrater;
}


GridItem *Board::AddALadder_Origin(int theGridX, int theGridY) {
    GridItem *aLadder = mGridItems.DataArrayAlloc();
    aLadder->mGridItemType = GridItemType::GRIDITEM_LADDER;
    aLadder->mRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_PLANT, theGridY, 800);
    aLadder->mGridX = theGridX;
    aLadder->mGridY = theGridY;
    return aLadder;
}

GridItem *Board::AddALadder(int theGridX, int theGridY) {
    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode) {
        return nullptr;
    }

    GridItem *aLadder = AddALadder_Origin(theGridX, theGridY);

    if (gTcpClientSocket >= 0 && mApp->mGameScene == SCENE_PLAYING) {
        U8U8U16_Event event = {{EventType::EVENT_SERVER_BOARD_GRIDITEM_ADDLADDER}, uint8_t(theGridX), uint8_t(theGridY), uint16_t(mGridItems.DataArrayGetID(aLadder))};
        netplay::PutEvent(event);
    }
    return aLadder;
}

GridItem *Board::AddAGraveStone(int theGridX, int theGridY) {
    GridItem *aGraveStone = mGridItems.DataArrayAlloc();
    aGraveStone->mGridItemType = GridItemType::GRIDITEM_GRAVESTONE;
    aGraveStone->mGridItemCounter = -Rand(50);
    if (mGridSquareType[theGridX][theGridY] == GridSquareType::GRIDSQUARE_POOL) { // 泳池墓碑更快浮现
        aGraveStone->mGridItemCounter /= 3;
    }
    aGraveStone->mRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, theGridY, 3);
    aGraveStone->mGridX = theGridX;
    aGraveStone->mGridY = theGridY;
    if (vsai::HasEnhancedAIProduction(this, vsai::VSSide::Zombies)) {
        aGraveStone->mLaunchCounter = vsai::ScaleEnhancedAIProductionCooldown(aGraveStone->mLaunchCounter);
    }

    if (mApp->IsVSMode()) {
        aGraveStone->unkBool = true;
        int aX = GridToPixelX(theGridX, theGridY);
        int aY = GridToPixelY(theGridX, theGridY);
        int aRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, theGridY, 0);
        Reanimation *aReanim = mApp->AddReanimation((float)aX, (float)aY, aRenderOrder, ReanimationType::REANIM_VS_GRAVESTONE);
        aReanim->mIsAttachment = true;
        aReanim->SetTruncateDisappearingFrames(nullptr, false);
        aReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0);
        aReanim->IgnoreClipRectForPrefix("chunk", true);
        aReanim->IgnoreClipRectForPrefix("Layer", true);
        aReanim->IgnoreClipRectForPrefix("bit", true);
        aReanim->AssignRenderGroupToPrefix("chunk", true);
        aReanim->AssignRenderGroupToPrefix("Layer", true);
        aReanim->AssignRenderGroupToPrefix("bit", true);
        aReanim->AssignRenderGroupToTrack("Stone dirt", RENDER_GROUP_HIDDEN);
        aGraveStone->mGridItemReanimID = mApp->ReanimationGetID(aReanim);
        aGraveStone->AddGraveStoneParticles();
    }

    if (gTcpClientSocket >= 0 && mApp->mGameScene == SCENE_PLAYING) {
        U8U8U16U16_Event event = {
            {EventType::EVENT_SERVER_BOARD_GRIDITEM_ADDGRAVE}, uint8_t(theGridX), uint8_t(theGridY), uint16_t(mGridItems.DataArrayGetID(aGraveStone)), uint16_t(aGraveStone->mLaunchCounter)};
        netplay::PutEvent(event);
    }

    return aGraveStone;
}

GridItem *Board::AddAMound(int theGridX, int theGridY, int theMoundLevel) {
    GridItem *aMound = mGridItems.DataArrayAlloc();
    aMound->mMoundLevel = theMoundLevel;
    aMound->mGridX = theGridX;
    aMound->mGridY = theGridY;
    aMound->mGridItemType = GridItemType::GRIDITEM_MP_BURIAL_MOUND;
    aMound->mGridItemCounter = -Rand(50);
    aMound->mLaunchCounter = RandRangeInt(aMound->mLaunchRate - 150, aMound->mLaunchRate);
    if (vsai::HasEnhancedAIProduction(this, vsai::VSSide::Zombies)) {
        aMound->mLaunchCounter = vsai::ScaleEnhancedAIProductionCooldown(aMound->mLaunchCounter);
    }
    aMound->mSummonCounter = RandRangeInt(1000, 1500);
    aMound->mRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, theGridY, 3);

    if (mApp->IsVSMode()) {
        aMound->unkBool = true;
        int aX = GridToPixelX(theGridX, theGridY);
        int aY = GridToPixelY(theGridX, theGridY);
        int aRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, theGridY, 0);
        Reanimation *aReanim = mApp->AddReanimation((float)aX, (float)aY, aRenderOrder, ReanimationType::REANIM_VS_MOUNDS);
        if (aReanim) {
            aReanim->mIsAttachment = true;
            aReanim->SetTruncateDisappearingFrames(nullptr, false);
            char animIdleName[16];
            int moundLevel = std::clamp(aMound->mMoundLevel, 0, 4);
            if (moundLevel == 0) {
                std::snprintf(animIdleName, sizeof(animIdleName), "anim_idle");
            } else {
                std::snprintf(animIdleName, sizeof(animIdleName), "anim_idle%d", moundLevel + 1);
            }
            aReanim->PlayReanim(animIdleName, ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
            aReanim->IgnoreClipRectForPrefix("chunk", true);
            aReanim->IgnoreClipRectForPrefix("Layer", true);
            aReanim->IgnoreClipRectForPrefix("bit", true);
            aReanim->AssignRenderGroupToPrefix("chunk", true);
            aReanim->AssignRenderGroupToPrefix("Layer", true);
            aReanim->AssignRenderGroupToPrefix("bit", true);
            aReanim->AssignRenderGroupToTrack("Stone dirt", RENDER_GROUP_HIDDEN);
            aMound->mGridItemReanimID = mApp->ReanimationGetID(aReanim);
            aMound->AddGraveStoneParticles();
        }
    }

    if (gTcpClientSocket >= 0 && mApp->mGameScene == SCENE_PLAYING) {
        U8x3U16x3_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_GRIDITEM_ADDMOUND;
        event.data1[0] = uint8_t(theGridX);
        event.data1[1] = uint8_t(theGridY);
        event.data1[2] = uint8_t(aMound->mMoundLevel);
        event.data2[0] = uint16_t(mGridItems.DataArrayGetID(aMound));
        event.data2[1] = uint16_t(aMound->mLaunchCounter);
        event.data2[2] = uint16_t(aMound->mSummonCounter);
        netplay::PutEvent(event);
    }

    return aMound;
}

GridItem *Board::AddAPole(int theX, int theY, int theGridY) {
    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode) {
        return nullptr;
    }

    GridItem *aPole = AddAPole_Origin(theX, theY, theGridY);

    if (gTcpClientSocket >= 0 && mApp->mGameScene == SCENE_PLAYING) {
        U16x4_Event event = {{EventType::EVENT_SERVER_BOARD_GRIDITEM_ADDPOLE}, {uint16_t(theX), uint16_t(theY), uint16_t(theGridY), uint16_t(mGridItems.DataArrayGetID(aPole))}};
        netplay::PutEvent(event);
    }
    return aPole;
}

GridItem *Board::AddAPole_Origin(int theX, int theY, int theGridY) {
    GridItem *aPole = mGridItems.DataArrayAlloc();
    aPole->mGridItemType = GridItemType::GRIDITEM_POLE;
    int aRenderOrder = aPole->mRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_PLANT, theY, 800);
    aPole->mPosX = float(theX + 10);
    aPole->mPosY = float(theY - 18);
    aPole->mGridX = PixelToGridX(theX, theY);
    aPole->mGridY = theGridY;
    Reanimation *aReanim = mApp->AddReanimation(float(theX + 10), float(theY - 18), aRenderOrder, ReanimationType::REANIM_GIGA_POLEVAULTER);
    if (aReanim) {
        aReanim->PlayReanim("anim_pole_wobble", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0f);
        aPole->mGridItemReanimID = mApp->ReanimationGetID(aReanim);
    }
    return aPole;
}

bool Board::TakeSunMoney(int theAmount, int thePlayer) {
    //    LOG_DEBUG("{} {}", theAmount, thePlayer);
    bool result = old_Board_TakeSunMoney(this, theAmount, thePlayer);
    if (gTcpClientSocket >= 0) {
        I16_Event event = {{EventType::EVENT_SERVER_BOARD_TAKE_SUNMONEY}, int16_t(mSunMoney1)};
        netplay::PutEvent(event);
    }
    return result;
}

bool Board::TakeDeathMoney(int theAmount) {
    // 重写以添加计算CountDeathBeingCollected(this)

    bool result = false;
    if (theAmount > mDeathMoney + CountDeathBeingCollected()) {
        result = false;
        mApp->PlaySample(Sexy::SOUND_BUZZER);
        mOutOfMoneyCounter = 70;
    } else {
        result = true;
        mDeathMoney -= theAmount;
    }

    if (gTcpClientSocket >= 0) {
        I16_Event event = {{EventType::EVENT_SERVER_BOARD_TAKE_DEATHMONEY}, int16_t(mDeathMoney)};
        netplay::PutEvent(event);
    }
    return result;
}

int Board::CountDeathBeingCollected() {
    int aCount = 0;
    Coin *aCoin = nullptr;
    while (IterateCoins(aCoin)) {
        if (aCoin->mIsBeingCollected && aCoin->IsDeath()) {
            aCount += aCoin->GetSunValue();
        }
    }
    return aCount;
}

void Board::ShuffleButtonDown(SeedPacket *theSeedPacket) {
    if (!Challenge::msVSShuffleMode)
        return;

    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
        return;

    SeedType aPacketType = theSeedPacket->mPacketType;
    int aPacketCost = GetCurrentPlantCost(theSeedPacket->mPacketType, SeedType::SEED_NONE);
    if (aPacketType == SEED_BEGHOULED_BUTTON_SHUFFLE) {
        if (!CanTakeSunMoney(aPacketCost, 0) || !theSeedPacket->CanPickUp() || HasLevelAwardDropped())
            return;

        std::vector<SeedType> aPlantSeeds, aZombieSeeds;
        PickShuffleSeeds(mApp, aPlantSeeds, aZombieSeeds, false);
        if (!aPlantSeeds.empty()) {
            for (int aPacketIndex = 1; aPacketIndex <= aPlantSeeds.size(); ++aPacketIndex) {
                SeedType aSeedType = aPlantSeeds[aPacketIndex - 1];
                mSeedBank[0]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
            }
        }
        TakeSunMoney(aPacketCost, 0);
        theSeedPacket->Deactivate();
        theSeedPacket->WasPlanted(0);

        if (gTcpClientSocket >= 0) {
            U16x6_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_SHUFFLE_RANDOM_PICK;
            for (int i = 0; i < 5; ++i) {
                event.data[i] = aPlantSeeds[i];
            }
            event.data[5] = 0;
            netplay::PutEvent(event);
        }
    }
    if (aPacketType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
        if (!CanTakeDeathMoney(aPacketCost) || !theSeedPacket->CanPickUp() || HasLevelAwardDropped())
            return;

        std::vector<SeedType> aPlantSeeds, aZombieSeeds;
        PickShuffleSeeds(mApp, aPlantSeeds, aZombieSeeds, true);
        if (!aZombieSeeds.empty()) {
            for (int aPacketIndex = 1; aPacketIndex <= aZombieSeeds.size(); ++aPacketIndex) {
                SeedType aSeedType = aZombieSeeds[aPacketIndex - 1];
                mSeedBank[1]->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
            }
        }
        TakeDeathMoney(aPacketCost);
        theSeedPacket->Deactivate();
        theSeedPacket->WasPlanted(1);

        if (gTcpClientSocket >= 0) {
            U16x6_Event event{};
            event.type = EventType::EVENT_SERVER_BOARD_SHUFFLE_RANDOM_PICK;
            for (int i = 0; i < 5; ++i) {
                event.data[i] = aZombieSeeds[i];
            }
            event.data[5] = 1;
            netplay::PutEvent(event);
        }
    }
}

bool Board::CanAddGraveStoneAt(int theGridX, int theGridY) {
    if (mGridSquareType[theGridX][theGridY] != GridSquareType::GRIDSQUARE_GRASS && mGridSquareType[theGridX][theGridY] != GridSquareType::GRIDSQUARE_HIGH_GROUND) {
        if (!(mApp->IsVSMode() && mGridSquareType[theGridX][theGridY] == GridSquareType::GRIDSQUARE_POOL)) // 允许对战水路种植墓碑
            return false;
    }

    GridItem *aGridItem = nullptr;
    while (IterateGridItems(aGridItem)) {
        if (aGridItem->mGridX == theGridX && aGridItem->mGridY == theGridY) {
            if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE || aGridItem->mGridItemType == GridItemType::GRIDITEM_CRATER || aGridItem->mGridItemType == GridItemType::GRIDITEM_LADDER
                || aGridItem->mGridItemType == GridItemType::GRIDITEM_MP_BURIAL_MOUND /* 禁止墓碑和召唤墓碑叠种 */)
                return false;
        }
    }
    return true;
}

void Board::DrawLevel(Graphics *g) {
    // 禁止对战模式绘制关卡名称
    if (mApp->IsVSMode()) {
        if (gOpeningEncounter) {
            pvzstl::string aLevelStr;
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUN_RAIN) {
                aLevelStr = "[SUN_RAIN]";
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_SUNNY_DAY) {
                aLevelStr = "[SUNNY_DAY]";
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_FREE_SHUFFLE) {
                aLevelStr = "[FREE_SHUFFLE]";
            }
            if (gOpeningEncounter->mType == EncounterType::ENCOUNTER_QUICK_SHUFFLE) {
                aLevelStr = "[QUICK_SHUFFLE]";
            }
            int aPosX = 593;
            int aPosY = 595;
            TodDrawString(g, aLevelStr, aPosX, aPosY, addonFonts.JN_BOBO_HEI24, Color::Black, DrawStringJustification::DS_ALIGN_RIGHT);
            TodDrawString(g, aLevelStr, aPosX + 2, aPosY - 2, addonFonts.JN_BOBO_HEI24, Color(224, 187, 98), DrawStringJustification::DS_ALIGN_RIGHT);
        }
        return;
    }

    old_Board_DrawLevel(this, g);
}


bool Board::CanAddBobSledMP() {
    // 客户端不允许私自召唤雪橇小队
    if (gTcpConnected || gIsServerModeSpectator || gIsReplayMode)
        return false;

    // 遍历 6 条车道
    for (int lane = 0; lane < 6; lane++) {
        // 检查当前车道的冰面状态：
        // 1. mIceTimer[lane] > 0 : 说明该行当前有冰面存在（计时器未归零）
        // 2. mIceMinX[lane] < 700 : 说明冰面从左侧延伸到了坐标 700 以内（冰面够长）

        if (mIceTimer[lane] > 0 && mIceMinX[lane] < 700) {
            // 只要找到任意一条符合条件的冰面，就可以放置雪橇车
            return true;
        }
    }

    // 如果所有车道都不满足条件，返回 false
    return false;
}

GridItem *Board::AddMPTarget(int theGridX, int theGridY) {

    // 修复六路靶子不绘制，原版固定绘制5行，现在绘制aNumRows
    int RenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_LAWN, theGridY, 1);
    if (theGridY == 0) {
        RenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_UI_BOTTOM, 0, 0);
    }
    GridItem *aGridItem = mGridItems.DataArrayAlloc();
    aGridItem->mGridItemType = GRIDITEM_MP_TARGET_ZOMBIE;
    aGridItem->mGridY = theGridY;
    aGridItem->mRenderOrder = RenderOrder;
    aGridItem->mGridX = theGridX;
    int aPixelX = GridToPixelX(theGridX, theGridY);
    int aPixelY = GridToPixelY(theGridX, theGridY);
    int aNumRows = StageHas6Rows() ? 6 : 5;
    Reanimation *reanimation = mApp->AddReanimation((20.0f / (aNumRows - theGridY)) + 26.0f + aPixelX, aPixelY - 54.0f, RenderOrder, REANIM_ZOMBIE_TARGET);
    reanimation->SetAnimRate(0.0);
    reanimation->mLoopType = REANIM_LOOP;
    reanimation->mIsAttachment = true;
    aGridItem->mGridItemReanimID = mApp->ReanimationGetID(reanimation);
    return aGridItem;
}

void Board::PlantsWon(GridItem *theGridItem) {

    // 此处不需要同步，因为上级GridItemDie已经同步了，此处同同步反而会导致动画ID为null导致的闪退BUG

    //    if (mApp->IsVSMode() && gTcpConnected)
    //        return;
    //
    //    if (gTcpClientSocket >= 0) {
    //        U16_Event event = {{EventType::EVENT_SERVER_BOARD_PLANT_WIN},uint16_t(mGridItems.DataArrayGetID(theGridItem))};
    //        netplay::PutEvent(event);
    //    }

    if (mApp->IsVSMode() && gTcpClientSocket >= 0) {
        // 所选对战场地
        netplay::MetricsSetVsBackground(int(gVSBackground));
        // 对战游戏模式
        netplay::MetricsSetShuffleMode(Challenge::msVSShuffleMode);
        // 植物胜利的对局时间
        netplay::MetricsSendSettlement(true, mMainCounter);
    }
    PlantsWon_Origin(theGridItem);
}

void Board::PlantsWon_Origin(GridItem *theGridItem) {
    old_Board_PlantsWon(this, theGridItem);
}

ZombieType Board::PickGraveRisingZombieTypeMP(int theMoundLevel) const {
    TodWeightedArray aZombieWeightArray[(int)ZombieType::NUM_ZOMBIE_TYPES];
    int aCount = 0;
    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS_DEBUG) {
        if (theMoundLevel > 3) {
            aZombieWeightArray[0].mItem = ZombieType::ZOMBIE_GARGANTUAR;
            aZombieWeightArray[0].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_GARGANTUAR).mPickWeight;
            aZombieWeightArray[1].mItem = ZombieType::ZOMBIE_CATAPULT;
            aZombieWeightArray[1].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_CATAPULT).mPickWeight;
            aZombieWeightArray[2].mItem = ZombieType::ZOMBIE_ZAMBONI;
            aZombieWeightArray[2].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_ZAMBONI).mPickWeight;
            aCount = 3;
        } else {
            switch (theMoundLevel) {
                case 3:
                    aZombieWeightArray[0].mItem = ZombieType::ZOMBIE_DOOR;
                    aZombieWeightArray[0].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_DOOR).mPickWeight;
                    aZombieWeightArray[1].mItem = ZombieType::ZOMBIE_JACK_IN_THE_BOX;
                    aZombieWeightArray[1].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_JACK_IN_THE_BOX).mPickWeight;
                    aCount = 2;
                    break;

                case 2:
                    aZombieWeightArray[0].mItem = ZombieType::ZOMBIE_POLEVAULTER;
                    aZombieWeightArray[0].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_POLEVAULTER).mPickWeight;
                    aZombieWeightArray[1].mItem = ZombieType::ZOMBIE_FOOTBALL;
                    aZombieWeightArray[1].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_FOOTBALL).mPickWeight;
                    aCount = 2;
                    break;

                case 1:
                    aZombieWeightArray[0].mItem = ZombieType::ZOMBIE_NEWSPAPER;
                    aZombieWeightArray[0].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_NEWSPAPER).mPickWeight;
                    aZombieWeightArray[1].mItem = ZombieType::ZOMBIE_TRAFFIC_CONE;
                    aZombieWeightArray[1].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_TRAFFIC_CONE).mPickWeight;
                    aCount = 2;
                    break;

                default:
                    aZombieWeightArray[0].mItem = ZombieType::ZOMBIE_NORMAL;
                    aZombieWeightArray[0].mWeight = GetZombieDefinition(ZombieType::ZOMBIE_NORMAL).mPickWeight;
                    aCount = 1;
                    break;
            }
        }

        return ZombieType(TodPickFromWeightedArray(aZombieWeightArray, aCount));
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        switch (theMoundLevel) {
            case 0:
                return ZombieType::ZOMBIE_NORMAL;
            case 1:
                return ZombieType::ZOMBIE_TRAFFIC_CONE;
            case 2:
                return ZombieType::ZOMBIE_PAIL;
            case 3:
                return ZombieType::ZOMBIE_FOOTBALL;
            case 4:
                return ZombieType::ZOMBIE_GARGANTUAR;
            default:
                break;
        }
    }

    return ZombieType(TodPickFromWeightedArray(aZombieWeightArray, aCount));
}

bool Board::IsZombieTypeSpawnedOnly(ZombieType theZombieType) {
    return (theZombieType == ZombieType::ZOMBIE_BACKUP_DANCER || theZombieType == ZombieType::ZOMBIE_BOBSLED || theZombieType == ZombieType::ZOMBIE_IMP);
}

bool Board::IsZombieTypePoolOnly(ZombieType theZombieType) {
    return (theZombieType == ZombieType::ZOMBIE_SNORKEL || theZombieType == ZombieType::ZOMBIE_DOLPHIN_RIDER);
}

Plant *Board::FindBloomerangPlant(int theGridX, int theGridY) {
    Plant *aPlant = nullptr;
    while (IteratePlants(aPlant)) {
        if (aPlant->mSeedType == SeedType::SEED_BLOOMERANG && !aPlant->NotOnGround() && GridInRange(theGridX, theGridY, aPlant->mPlantCol, aPlant->mRow, 1, 1)) {
            return aPlant;
        }
    }
    return nullptr;
}
void Board::DoChillyFwoosh(int theRow, float theX, float theY) {
    const int aRenderOrder = MakeRenderOrder(RenderLayer::RENDER_LAYER_PARTICLE, theRow, 1);

    Sexy::Image *const kIceImages[] = {addonImages.IMAGE_REANIM_ICE1, addonImages.IMAGE_REANIM_ICE2, addonImages.IMAGE_REANIM_ICE3, addonImages.IMAGE_REANIM_ICE4};

    for (int i = 0; i < 12; ++i) {
        Reanimation *anOldFwoosh = mApp->ReanimationTryToGet(mFwooshID[theRow][i]);
        if (anOldFwoosh != nullptr) {
            anOldFwoosh->ReanimationDie();
        }

        const float aPosX = 750.0f * static_cast<float>(i) / 11.0f + 10.0f;
        const float aPosY = GetPosYBasedOnRow(aPosX + 10.0f, theRow) - 10.0f;

        Reanimation *anIce = mApp->AddReanimation(aPosX, aPosY, aRenderOrder, ReanimationType::REANIM_ICE);
        anIce->SetImageOverride("ice", kIceImages[Rand(4)]);
        anIce->SetFramesForLayer("anim_start");
        anIce->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD;
        anIce->mAnimRate *= RandRangeFloat(0.7f, 1.3f);

        const float aScale = RandRangeFloat(0.8f, 1.0f);
        const float aFlip = Rand(2) ? 1.0f : -1.0f;
        anIce->OverrideScale(aScale * aFlip, 1.0f);

        mFwooshID[theRow][i] = mApp->ReanimationGetID(anIce);
    }

    Reanimation *aChiloosh = mApp->AddReanimation(theX + 10.0f, theY - 40.0f, aRenderOrder + 1, ReanimationType::REANIM_CHILOOSH);
    aChiloosh->SetFramesForLayer("anim_chiloosh");
    aChiloosh->mLoopType = ReanimLoopType::REANIM_PLAY_ONCE_FULL_LAST_FRAME;
    mFwooshCountDown = 100;
}
