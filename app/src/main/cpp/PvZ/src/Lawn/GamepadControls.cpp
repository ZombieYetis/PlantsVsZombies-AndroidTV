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

#include "PvZ/Lawn/GamepadControls.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/ZenGarden.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/SaveGame.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Misc/Gamepad.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <cmath>
#include <cstdint>

using namespace Sexy;

namespace {
bool WidgetManagerIsEscDown(Sexy::WidgetManager *widgetManager) {
    // Mirror original std::map<int,bool> key lookup for VK_ESCAPE (27).
    int *manager = reinterpret_cast<int *>(widgetManager);
    int *node = reinterpret_cast<int *>(manager[68]);
    int *candidate = manager + 67;
    while (node != nullptr) {
        int key = node[4];
        if (key <= 26) {
            node = reinterpret_cast<int *>(node[3]);
        } else {
            candidate = node;
        }
        if (key > 26) {
            node = reinterpret_cast<int *>(node[2]);
        }
    }
    if (candidate == manager + 67 || candidate[4] > 27) {
        return false;
    }
    return *(reinterpret_cast<unsigned char *>(candidate) + 20) != 0;
}

bool GamepadButtonDown(LawnApp *app, int playerIndex, int button) {
    if (playerIndex < 0 || playerIndex >= 2) {
        return false;
    }

    Sexy::Gamepad *gamepad = app->mGamepads[playerIndex];
    if (gamepad == nullptr) {
        return false;
    }

    return gamepad->IsButtonDown(button);
}

bool HasValidSelectedSeed(const GamepadControls *gamepadControls, const SeedBank *seedBank) {
    const int selectedIndex = gamepadControls->mSelectedSeedIndex;
    return seedBank != nullptr && selectedIndex >= 0 && selectedIndex < seedBank->mNumPackets && selectedIndex < 10;
}

void GamepadControls_UpdateOriginal(GamepadControls *gamepadControls, float dt) {
    if (gamepadControls->mGamepadIndex == -1) {
        return;
    }

    LawnApp *app = gamepadControls->mApp;
    if (app->HasGamepad() || (app->mGamePad1IsOn && app->mGamePad2IsOn)) {
        if (gamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_DIG_INDICATOR_WAIT) {
            gamepadControls->GotoState(BaseGamepadControls::MOVEMENT_STATE_NORMAL);
            gamepadControls->mIsShowingDigIndicator = false;
        }
        if (!GamepadButtonDown(app, gamepadControls->mGamepadIndex, Sexy::GamepadButton::GAMEPAD_BUTTON_B) && gamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_DIG_HOLD) {
            gamepadControls->GotoState(BaseGamepadControls::MOVEMENT_STATE_NORMAL);
        }
        if (!GamepadButtonDown(app, gamepadControls->mGamepadIndex, Sexy::GamepadButton::GAMEPAD_BUTTON_X) && gamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_BUTTER_HELD) {
            gamepadControls->GotoState(BaseGamepadControls::MOVEMENT_STATE_BUTTER_RELEASED);
        }
    } else {
        bool escDown = WidgetManagerIsEscDown(app->mWidgetManager);
        if (escDown && gamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_DIG_INDICATOR_WAIT
            && gamepadControls->mGamepadFrameCounter - gamepadControls->mDigIndicatorStartCounter > 20) {
            gamepadControls->GotoState(BaseGamepadControls::MOVEMENT_STATE_DIG_HOLD);
            gamepadControls->mIsShowingDigIndicator = true;
        }
        if (!escDown && gamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_DIG_HOLD) {
            gamepadControls->GotoState(BaseGamepadControls::MOVEMENT_STATE_NORMAL);
        }
    }

    gamepadControls->mDrawCursorFrame = !gamepadControls->mIsCobCannonSelected;

    Reanimation *cobCannonReanim = app->ReanimationTryToGet(gamepadControls->mCobCannonReanimID);
    if (cobCannonReanim == nullptr && gamepadControls->mGamepadIndex != -1) {
        cobCannonReanim = app->AddReanimation(0.0f, 0.0f, 0, static_cast<ReanimationType>(158));
        if (cobCannonReanim != nullptr) {
            cobCannonReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 12.0f);
            gamepadControls->mCobCannonReanimID = app->ReanimationGetID(cobCannonReanim);
            cobCannonReanim->mIsAttachment = true;
        }
    }
    if (cobCannonReanim != nullptr && gamepadControls->mGamepadIndex != -1) {
        Reanimation *cobCannonReanimById = app->ReanimationTryToGet(gamepadControls->mCobCannonReanimID);
        if (cobCannonReanimById != nullptr) {
            if (GamepadButtonDown(gamepadControls->mBoard->mApp, gamepadControls->mGamepadIndex, Sexy::GamepadButton::GAMEPAD_BUTTON_A)) {
                cobCannonReanimById->PlayReanim("anim_depressed", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
            } else if (!cobCannonReanimById->IsAnimPlaying("anim_bounce")) {
                cobCannonReanimById->PlayReanim("anim_bounce", ReanimLoopType::REANIM_LOOP, 0, 12.0f);
            }
            cobCannonReanimById->Update();
        }
    }

    Reanimation *previewReanim1 = app->ReanimationTryToGet(gamepadControls->mPreviewReanimID1);
    if (previewReanim1 != nullptr) {
        int gridY = gamepadControls->mBoard->PixelToGridYKeepOnBoard(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY);
        previewReanim1->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_PLANT, gridY, 100);
    }

    gamepadControls->BaseGamepadControls::Update(dt);

    if (gamepadControls->mIsCobCannonSelected) {
        gamepadControls->mBoard->mCobCannonMouseX = gamepadControls->mCursorPositionX;
        gamepadControls->mBoard->mCobCannonMouseY = gamepadControls->mCursorPositionY;
    }

    int gridX = gamepadControls->mBoard->PixelToGridXKeepOnBoard(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY);
    int gridY = gamepadControls->mBoard->PixelToGridYKeepOnBoard(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY);
    SeedBank *seedBank = gamepadControls->GetSeedBank();
    SeedPacket *selectedSeedPacket = &seedBank->mSeedPackets[gamepadControls->mSelectedSeedIndex];
    SeedType selectedSeedTypeForPlant = selectedSeedPacket->mPacketType;
    if (selectedSeedPacket->mPacketType == SeedType::SEED_IMITATER) {
        gamepadControls->mCanPickUp = (gamepadControls->mBoard->CanPlantAt(gridX, gridY, selectedSeedPacket->mImitaterType) == PlantingReason::PLANTING_OK) && selectedSeedPacket->CanPickUp();
        selectedSeedTypeForPlant = selectedSeedPacket->mImitaterType;
    } else {
        gamepadControls->mCanPickUp = (gamepadControls->mBoard->CanPlantAt(gridX, gridY, selectedSeedPacket->mPacketType) == PlantingReason::PLANTING_OK) && selectedSeedPacket->CanPickUp();
        selectedSeedTypeForPlant = selectedSeedPacket->mPacketType;
    }

    if (gamepadControls->mIsZombie) {
        gamepadControls->mCanPickUp = (gamepadControls->mBoard->CanPlantAt(gridX, gridY, selectedSeedPacket->mPacketType) == PlantingReason::PLANTING_OK) && selectedSeedPacket->CanPickUp();
    }

    if (gamepadControls->mIsInShopSeedBank) {
        selectedSeedTypeForPlant = seedBank->mShopSeedPackets[gamepadControls->mSelectedShopSeedIndex].mSeedType;
        gamepadControls->mCanPickUp = gamepadControls->mBoard->CanPlantAt(gridX, gridY, selectedSeedTypeForPlant) == PlantingReason::PLANTING_OK;
    }

    if ((gamepadControls->mGamepadFrameCounter & 25) == 0) {
        HitResult hitResult = {};
        gamepadControls->mBoard->MouseHitTest(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY, &hitResult, gamepadControls->mGamepadIndex);
        if (hitResult.mObjectType == GameObjectType::OBJECT_TYPE_COIN) {
            Coin *coin = reinterpret_cast<Coin *>(hitResult.mObject);
            GameMode gameMode = gamepadControls->mApp->mGameMode;
            if (gameMode == GameMode::GAMEMODE_MP_VS_DEBUG || gameMode == GameMode::GAMEMODE_MP_VS) {
                if (gamepadControls->mIsZombie) {
                    if (coin->IsDeath() || coin->mType == CoinType::COIN_VS_ZOMBIE_TROPHY) {
                        coin->MouseDown(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY, 1);
                    }
                } else if (coin->IsSun() || coin->mType == CoinType::COIN_VS_PLANT_TROPHY) {
                    coin->MouseDown(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY, 1);
                }
            } else if (gameMode != GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && gamepadControls->mGamepadState != BaseGamepadControls::MOVEMENT_STATE_DIG_HOLD
                       && coin->mType == CoinType::COIN_USABLE_SEED_PACKET) {
                gamepadControls->mBoard->RefreshSeedPacketFromCursor(gamepadControls->mPlayerIndex);
                coin->GamepadCursorOver(gamepadControls->mPlayerIndex);
            }
        }

        gamepadControls->mSelectedUpgradableType = 0;
        HitResult plantHitResult = {};
        if (gamepadControls->mBoard->MouseHitTestPlant(gamepadControls->mCursorPositionX, gamepadControls->mCursorPositionY, &plantHitResult)
            && plantHitResult.mObjectType == GameObjectType::OBJECT_TYPE_PLANT && plantHitResult.mObject != nullptr) {
            Plant *hitPlant = reinterpret_cast<Plant *>(plantHitResult.mObject);
            SeedType hitPlantSeedType = hitPlant->mSeedType;
            if (selectedSeedTypeForPlant == SeedType::SEED_FLOWERPOT) {
                if (hitPlantSeedType == SeedType::SEED_PUMPKINSHELL) {
                    Plant *topPlant = gamepadControls->mBoard->GetTopPlantAt(hitPlant->mPlantCol, hitPlant->mRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
                    if (topPlant != nullptr) {
                        hitPlant = topPlant;
                    }
                }
            } else {
                bool checkTopPlant = false;
                if (hitPlantSeedType == SeedType::SEED_FLOWERPOT) {
                    checkTopPlant = true;
                } else if (selectedSeedTypeForPlant != SeedType::SEED_PUMPKINSHELL) {
                    if (hitPlantSeedType == SeedType::SEED_PUMPKINSHELL) {
                        checkTopPlant = true;
                    } else if (selectedSeedTypeForPlant != SeedType::SEED_LILYPAD && hitPlantSeedType == SeedType::SEED_LILYPAD) {
                        checkTopPlant = true;
                    }
                }
                if (checkTopPlant) {
                    Plant *topPlant = gamepadControls->mBoard->GetTopPlantAt(hitPlant->mPlantCol, hitPlant->mRow, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
                    if (selectedSeedTypeForPlant != SeedType::SEED_LILYPAD && topPlant != nullptr) {
                        hitPlant = topPlant;
                    }
                }
            }

            if (hitPlant->IsUpgradableTo(gamepadControls->mSelectedSeedType)) {
                gamepadControls->mSelectedUpgradableType = 1;
            } else if (hitPlant->IsPartOfUpgradableTo(gamepadControls->mSelectedSeedType)) {
                gamepadControls->mSelectedUpgradableType = 2;
            } else if (selectedSeedTypeForPlant != SeedType::SEED_PUMPKINSHELL
                       || gamepadControls->mBoard->GetTopPlantAt(hitPlant->mPlantCol, hitPlant->mRow, PlantPriority::TOPPLANT_ONLY_PUMPKIN) != nullptr
                       || gamepadControls->mBoard->CanPlantAt(gridX, gridY, SeedType::SEED_PUMPKINSHELL) != PlantingReason::PLANTING_OK) {
                if (hitPlant->mSeedType != SeedType::SEED_LILYPAD || selectedSeedPacket->mPacketType == SeedType::SEED_LILYPAD) {
                    gamepadControls->mSelectedUpgradableType = 4;
                }
            } else {
                gamepadControls->mSelectedUpgradableType = 3;
            }
        }
    }

    if (gamepadControls->mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && gamepadControls->mGamepadState != BaseGamepadControls::MOVEMENT_STATE_DIG_HOLD) {
        Coin *coin = nullptr;
        while (gamepadControls->mBoard->IterateCoins(coin)) {
            if (gamepadControls->mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
                if (gamepadControls->mIsZombie) {
                    if (!coin->IsDeath() && coin->mType != CoinType::COIN_VS_ZOMBIE_TROPHY) {
                        continue;
                    }
                } else if (!coin->IsSun() && coin->mType != CoinType::COIN_VS_PLANT_TROPHY) {
                    continue;
                }
            }
            if (coin == nullptr || coin->mIsBeingCollected || coin->mCoinMotion == CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR || coin->mType == CoinType::COIN_USABLE_SEED_PACKET) {
                continue;
            }
            float dx = gamepadControls->mCursorPositionX - coin->mPosX;
            float dy = gamepadControls->mCursorPositionY - coin->mPosY;
            if (dx * dx + dy * dy >= 40000.0f) {
                continue;
            }
            coin->GamepadCursorOver(gamepadControls->mPlayerIndex);
        }
    }

    CursorPreview *cursorPreview = gamepadControls->mBoard->mCursorPreview[gamepadControls->mPlayerIndex];
    CursorObject *cursorObject = gamepadControls->mBoard->mCursorObject[gamepadControls->mPlayerIndex];
    if (cursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
        cursorPreview->mVisible = true;
        gamepadControls->mCanPickUp = true;
    } else if (cursorObject->mCursorType == CursorType::CURSOR_TYPE_HAMMER) {
        cursorPreview->mVisible = true;
        cursorObject->mX = static_cast<int>(gamepadControls->mCursorPositionX - 20.0f);
        cursorObject->mY = static_cast<int>(gamepadControls->mCursorPositionY - 32.0f);
    } else {
        cursorPreview->mVisible = true;
        cursorObject->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
        if (gamepadControls->mIsInShopSeedBank) {
            cursorObject->mSelectedIndex = gamepadControls->mSelectedShopSeedIndex;
            if (gamepadControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_NORMAL || app->IsChallengeWithoutSeedBank()) {
                cursorObject->mType = SeedType::SEED_NONE;
                gamepadControls->mSelectedSeedType = SeedType::SEED_NONE;
            } else {
                int shopSeedIndex = gamepadControls->mSelectedShopSeedIndex;
                if (shopSeedIndex == 4 || shopSeedIndex == 5) {
                    SeedType shopSeedType = seedBank->mShopSeedPackets[shopSeedIndex].mSeedType;
                    cursorObject->mType = shopSeedType;
                    gamepadControls->mSelectedSeedType = shopSeedType;
                } else {
                    cursorObject->mType = SeedType::SEED_NONE;
                    gamepadControls->mSelectedSeedType = SeedType::SEED_NONE;
                }
            }
        } else {
            cursorObject->mSelectedIndex = gamepadControls->mSelectedSeedIndex;
            if (!app->IsChallengeWithoutSeedBank()) {
                cursorObject->mType = selectedSeedPacket->mPacketType;
                gamepadControls->mSelectedSeedType = selectedSeedPacket->mPacketType;
                if (selectedSeedPacket->mPacketType == SeedType::SEED_IMITATER) {
                    cursorObject->mImitaterType = selectedSeedPacket->mImitaterType;
                    gamepadControls->mSelectedSeedType = selectedSeedPacket->mImitaterType;
                }
            } else {
                cursorObject->mType = SeedType::SEED_NONE;
                gamepadControls->mSelectedSeedType = SeedType::SEED_NONE;
            }
        }
    }

    if (gamepadControls->mBoard->HasConveyorBeltSeedBank(0) && selectedSeedPacket->mOffsetY >= 466) {
        gamepadControls->mCanPickUp = false;
    }

    float rise = gamepadControls->mCursorLabelLiftOffset + dt * 80.0f;
    if (rise > 30.0f) {
        rise = 30.0f;
    } else if (rise < 0.0f) {
        rise = 0.0f;
    }
    gamepadControls->mCursorLabelLiftOffset = rise;

    float yJitter = NAN;
    if (GamepadButtonDown(gamepadControls->mApp, gamepadControls->mGamepadIndex, Sexy::GamepadButton::GAMEPAD_BUTTON_A) || gamepadControls->mButterZombieID != ZOMBIEID_NULL) {
        yJitter = 3.0f;
    } else {
        gamepadControls->mCursorJitterAnimPhase += 0.016f;
        yJitter = sinf(gamepadControls->mCursorJitterAnimPhase * 3.0f);
    }

    gamepadControls->mCursorPositionYJitter = yJitter;
    cursorPreview->mVisible = true;
    gamepadControls->UpdateStates(dt);

    float velocityLeftX = gamepadControls->mGamepadVelocityLeftX;
    float velocityLeftY = gamepadControls->mGamepadVelocityLeftY;
    float newCursorY = gamepadControls->mCursorPositionY + dt * velocityLeftY;
    float newCursorX = gamepadControls->mCursorPositionX + dt * velocityLeftX;
    float newVelocityLeftX = velocityLeftX + gamepadControls->mGamepadAccLeftX * 3.0f;
    float newVelocityLeftY = velocityLeftY + gamepadControls->mGamepadAccLeftY * 3.0f;
    if (newCursorX > 770.0f) {
        newCursorX = 770.0f;
    } else if (newCursorX < 40.0f) {
        newCursorX = 40.0f;
    }
    if (newCursorY > 580.0f) {
        newCursorY = 580.0f;
    } else if (newCursorY < 80.0f) {
        newCursorY = 80.0f;
    }
    gamepadControls->mCursorPositionX = newCursorX;
    gamepadControls->mCursorPositionY = newCursorY;
    gamepadControls->mGamepadAccLeftX = newVelocityLeftX * 0.25f;
    gamepadControls->mGamepadAccLeftY = newVelocityLeftY * 0.25f;

    if (gamepadControls->mBoard->mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST) {
        int twistMaxX = gamepadControls->mBoard->GridToPixelX(6, 3) + 40;
        if (gamepadControls->mCursorPositionX > 40.0f && gamepadControls->mCursorPositionX > static_cast<float>(twistMaxX)) {
            gamepadControls->mCursorPositionX = static_cast<float>(twistMaxX);
        }
        int twistMaxY = gamepadControls->mBoard->GridToPixelY(6, 3) + 50;
        if (gamepadControls->mCursorPositionY > 80.0f && gamepadControls->mCursorPositionY > static_cast<float>(twistMaxY)) {
            gamepadControls->mCursorPositionY = static_cast<float>(twistMaxY);
        }
    }

    float canPickupAnim = gamepadControls->mCanPickUp ? (gamepadControls->mCanPickUpFade + dt) : (gamepadControls->mCanPickUpFade - dt * 2.0f);
    if (canPickupAnim > 1.0f) {
        canPickupAnim = 1.0f;
    } else if (canPickupAnim < 0.0f) {
        canPickupAnim = 0.0f;
    }
    gamepadControls->mCanPickUpFade = canPickupAnim;

    if (previewReanim1 != nullptr) {
        int pixelX = gamepadControls->mBoard->GridToPixelX(gridX, gridY);
        int pixelY = gamepadControls->mBoard->GridToPixelY(gridX, gridY);
        previewReanim1->SetPosition(static_cast<float>(pixelX - 30), static_cast<float>(pixelY - 45));
    }

    GameMode gameMode = gamepadControls->mApp->mGameMode;
    if (gameMode != GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST && gameMode != GameMode::GAMEMODE_CHALLENGE_BEGHOULED) {
        gamepadControls->UpdatePreviewReanim();
    }

    if (gamepadControls->mIsCobCannonSelected) {
        uint32_t cobPlantId = static_cast<uint32_t>(gamepadControls->mCobCannonPlantIndexInList);
        if (cobPlantId == 0 || gamepadControls->mBoard->mPlants.DataArrayTryToGet(cobPlantId) == nullptr) {
            gamepadControls->mIsCobCannonSelected = false;
            gamepadControls->mBoard->ClearCursor(gamepadControls->mPlayerIndex);
            gamepadControls->mBoard->mCobCannonCursorDelayCounter = 0;
            gamepadControls->mCobCannonAnimCounter = 0;
            gamepadControls->mCobCannonPlantIndexInList = 0;
        }
    }
}
} // namespace

// GamepadControls::GamepadControls(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2) {
// _constructor(theBoard, thePlayerIndex1, thePlayerIndex2);
//}

void GamepadControls::_constructor(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2) {
    old_GamepadControls_GamepadControls(this, theBoard, thePlayerIndex1, thePlayerIndex2);

    if (isKeyboardTwoPlayerMode)
        return;

    GameMode aGameMode = mApp->mGameMode;
    bool isTwoSeedBankMode = (aGameMode == GameMode::GAMEMODE_MP_VS || (aGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && aGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS));
    if (!gKeyboardMode && !isTwoSeedBankMode && aGameMode != GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE) {
        mIsInShopSeedBank = true; // 是否在Shop栏。
    }
}

void GamepadControls::pickUpCobCannon(Plant *cobCannon) {
    Plant *otherSelectedCob = nullptr;
    GamepadControls *otherGamepadControls = (mPlayerIndex != 0) ? mBoard->mGamepadControls[0] : mBoard->mGamepadControls[1];

    if (otherGamepadControls != nullptr && otherGamepadControls->mGamepadIndex != -1 && otherGamepadControls->mIsCobCannonSelected) {
        const uint32_t otherCobPlantId = static_cast<uint32_t>(otherGamepadControls->mCobCannonPlantIndexInList);
        if (otherCobPlantId != 0) {
            otherSelectedCob = mBoard->mPlants.DataArrayTryToGet(otherCobPlantId);
        }
    }

    if (cobCannon != otherSelectedCob && cobCannon->mState == PlantState::STATE_COBCANNON_READY && mGamepadState != MOVEMENT_STATE_DIG_HOLD) {
        if (!mIsCobCannonSelected) {
            mBoard->ClearCursor(mPlayerIndex);
            mBoard->mCobCannonCursorDelayCounter = 30;
            mBoard->mCobCannonMouseX = static_cast<int>(mCursorPositionX);
            mBoard->mCobCannonMouseY = static_cast<int>(mCursorPositionY);
            mCobCannonPlantIndexInList = static_cast<int>(mBoard->mPlants.DataArrayGetID(cobCannon));
            mCobCannonAnimCounter = 0;
            mIsCobCannonSelected = true;
        }
    }
}

void GamepadControls::Draw(Sexy::Graphics *g) {
    // 实现在光标内绘制铲子和黄油手套(黄油手套其实就是花园的手套),并在锤僵尸关卡绘制种植预览


    if (mGamepadIndex != -1) {
        LawnApp *anApp = mApp;
        bool is2P = mPlayerIndex == 1;
        CursorObject *aCursorObject = is2P ? mBoard->mCursorObject[1] : mBoard->mCursorObject[0];


        if (!anApp->IsCoopMode()) {
            requestDrawButterInCursor = false;
        }
        if (!mBoard->mShowShovel) {
            requestDrawShovelInCursor = false;
        }

        if (mBoard->IsButterInCursor(mPlayerIndex)) {
            g->DrawImage(addonImages.butter_glove, mCursorPositionX, mCursorPositionY);
        }

        if (mBoard->IsShovelInCursor(mPlayerIndex)) {
            if (anApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
                if (!mIsZombie) {
                    aCursorObject->mCursorType = CursorType::CURSOR_TYPE_SHOVEL;
                    aCursorObject->mX = mCursorPositionX - 20;
                    aCursorObject->mY = mCursorPositionY - 20;
                    if (aCursorObject->BeginDraw(g)) {
                        aCursorObject->Draw(g);
                        aCursorObject->EndDraw(g);
                    }
                }
            } else if (mBoard->UsesOnlineCoopTools() || !is2P) {
                if (anApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND) {
                    if (mBoard->mChallenge->mChallengeState == ChallengeState::STATECHALLENGE_NORMAL && anApp->mGameScene == GameScenes::SCENE_PLAYING) {
                        aCursorObject->mCursorType = CursorType::CURSOR_TYPE_MONEY_SIGN;
                        aCursorObject->mX = mCursorPositionX;
                        aCursorObject->mY = mCursorPositionY;
                    } else {
                        aCursorObject->mCursorType = CursorType::CURSOR_TYPE_SHOVEL;
                        aCursorObject->mX = mCursorPositionX - 20;
                        aCursorObject->mY = mCursorPositionY - 20;
                    }
                } else {
                    aCursorObject->mCursorType = CursorType::CURSOR_TYPE_SHOVEL;
                    aCursorObject->mX = mCursorPositionX - 20;
                    aCursorObject->mY = mCursorPositionY - 20;
                }
                if (aCursorObject->BeginDraw(g)) {
                    aCursorObject->Draw(g);
                    aCursorObject->EndDraw(g);
                }
            }
        }


        if (anApp->IsWhackAZombieLevel()) {
            int theGridX = mBoard->PixelToGridXKeepOnBoard(mCursorPositionX, mCursorPositionY);
            int theGridY = mBoard->PixelToGridYKeepOnBoard(mCursorPositionX, mCursorPositionY);
            int thePixelX = mBoard->GridToPixelX(theGridX, theGridY);
            int thePixelY = mBoard->GridToPixelY(theGridX, theGridY);
            g->mTransX += thePixelX;
            g->mTransY += thePixelY;
            DrawPreview(g);
            g->mTransX -= thePixelX;
            g->mTransY -= thePixelY;
        }
    }
    if (mIsCobCannonSelected && useNewCobCannon && !gKeyboardMode) {
        Sexy::Image *cobcannon_1 = Sexy::IMAGE_COBCANNON_TARGET_1;
        Sexy::IMAGE_COBCANNON_TARGET_1 = addonImages.custom_cobcannon;
        old_GamepadControls_Draw(this, g);
        Sexy::IMAGE_COBCANNON_TARGET_1 = cobcannon_1;
        return;
    }
    // 联机光标上绘制双方玩家昵称
    if (IsRemoteClient() || IsRemoteServer() || gIsReplayMode) {
        const char *hostName = gIsReplayMode ? ((gReplayHostName[0] != '\0') ? gReplayHostName : gServerHostName) : ((gServerHostName[0] != '\0') ? gServerHostName : mBoard->mApp->mPlayerInfo->mName);
        const char *guestName = gIsReplayMode ? ((gReplayGuestName[0] != '\0') ? gReplayGuestName : gSecondPlayerName) : ((gSecondPlayerName[0] != '\0') ? gSecondPlayerName : "Guest");
        if (mPlayerIndex == 0 && guestName[0] != '\0') {
            Image *tmp1 = Sexy::IMAGE_CURSOR_P1_TEXT;
            Sexy::IMAGE_CURSOR_P1_TEXT = IMAGE_BLANK;
            old_GamepadControls_Draw(this, g);
            const bool isHostSide = (mGamepadIndex == 0);
            TodDrawString(
                g, isHostSide ? hostName : guestName, mCursorPositionX - 5, mCursorPositionY - 60, Sexy::FONT_DWARVENTODCRAFT18, Color(255, 242, 14, 255), DrawStringJustification::DS_ALIGN_CENTER);
            Sexy::IMAGE_CURSOR_P1_TEXT = tmp1;
            return;
        }
        if (mPlayerIndex == 1 && guestName[0] != '\0') {
            Image *tmp = Sexy::IMAGE_CURSOR_P2_TEXT;
            Sexy::IMAGE_CURSOR_P2_TEXT = IMAGE_BLANK;
            old_GamepadControls_Draw(this, g);
            const bool isHostSide = (mGamepadIndex == 0);
            TodDrawString(
                g, isHostSide ? hostName : guestName, mCursorPositionX - 5, mCursorPositionY - 60, Sexy::FONT_DWARVENTODCRAFT18, Color(68, 207, 255, 255), DrawStringJustification::DS_ALIGN_CENTER);
            Sexy::IMAGE_CURSOR_P2_TEXT = tmp;
            return;
        }
    }
    old_GamepadControls_Draw(this, g);
}

void GamepadControls::Update(float a2) {
    LawnApp *anApp = mApp;
    int aGridX = mBoard->PixelToGridXKeepOnBoard(mCursorPositionX, mCursorPositionY);
    int aGridY = mBoard->PixelToGridYKeepOnBoard(mCursorPositionX, mCursorPositionY);
    int aGridCenterPosX = mBoard->GridToPixelX(aGridX, aGridY) + mBoard->GridCellWidth(aGridX, aGridY) / 2;
    int aGridCenterPosY = mBoard->GridToPixelY(aGridX, aGridY) + mBoard->GridCellHeight(aGridX, aGridY) / 2;
    // 键盘双人模式 平滑移动光标
    const bool readOnlyReplayOrSpectate = (gIsReplayMode || gIsServerModeSpectator);
    if (isKeyboardTwoPlayerMode && !readOnlyReplayOrSpectate) {
        int aGamepadIndex = mApp->PlayerToGamepadIndex(mPlayerIndex);
        if (aGamepadIndex == 0) {
            mGamepadVelocityLeftX = gGamepadP1VelX;
            mGamepadVelocityLeftY = gGamepadP1VelY;
            if (gGamepadP1VelX == 0) {
                mCursorPositionX += (aGridCenterPosX - mCursorPositionX) / 10;
            }
            if (gGamepadP1VelY == 0) {
                mCursorPositionY += (aGridCenterPosY - mCursorPositionY) / 10;
            }
        } else if (aGamepadIndex == 1) {
            mGamepadVelocityLeftX = gGamepadP2VelX;
            mGamepadVelocityLeftY = gGamepadP2VelY;
            if (gGamepadP2VelX == 0) {
                mCursorPositionX += (aGridCenterPosX - mCursorPositionX) / 10;
            }
            if (gGamepadP2VelY == 0) {
                mCursorPositionY += (aGridCenterPosY - mCursorPositionY) / 10;
            }
        }
    }

    if (!readOnlyReplayOrSpectate && positionAutoFix && !anApp->IsWhackAZombieLevel() && anApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM && gTcpServerSocket == -1) {
        if (this == mBoard->mGamepadControls[0] && gPlayerIndex != TouchPlayerIndex::TOUCHPLAYER_PLAYER1 && gPlayerIndexSecond != TouchPlayerIndex::TOUCHPLAYER_PLAYER1) {
            mCursorPositionX += (aGridCenterPosX - mCursorPositionX) / 10;
            mCursorPositionY += (aGridCenterPosY - mCursorPositionY) / 10;
        }
        if (this == mBoard->mGamepadControls[1] && gPlayerIndex != TouchPlayerIndex::TOUCHPLAYER_PLAYER2 && gPlayerIndexSecond != TouchPlayerIndex::TOUCHPLAYER_PLAYER2) {
            mCursorPositionX += (aGridCenterPosX - mCursorPositionX) / 10;
            mCursorPositionY += (aGridCenterPosY - mCursorPositionY) / 10;
        }
    }


    GamepadControls_UpdateOriginal(this, a2);

    // Reanimation *mCursorReanim = ReanimationTryToGet(gamepadControls->mGameObject->anApp, gamepadControls->mCursorReanimID);
    // LOGD("%d",mCursorReanim);
    // if (mCursorReanim != nullptr) {
    // if ((gamepadControls->mGamepadIndex == 0 &&(mIsZombie == TouchPlayerIndex::TOUCHPLAYER_PLAYER1 || gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER1)) ||
    // (gamepadControls->mGamepadIndex == 1
    // &&(mIsZombie == TouchPlayerIndex::TOUCHPLAYER_PLAYER2 || gPlayerIndexSecond == TouchPlayerIndex::TOUCHPLAYER_PLAYER2))) {
    // if (!Reanimation_IsAnimPlaying(mCursorReanim, "anim_depressed"))
    // Reanimation_PlayReanim(mCursorReanim, "anim_depressed", a::REANIM_LOOP, 0,12.0);
    // LOGD("456456");
    // } else if (!Reanimation_IsAnimPlaying(mCursorReanim, "anim_bounce")) {
    // Reanimation_PlayReanim(mCursorReanim, "anim_bounce", a::REANIM_LOOP, 0, 12.0);
    // }
    // }

    if (!isKeyboardTwoPlayerMode && !anApp->CanShopLevel() && mGamepadState == MOVEMENT_STATE_SELECT_SEED && mIsInShopSeedBank) {
        mIsInShopSeedBank = false;
    }
}

void GamepadControls::ButtonDownFireCobcannonTest() {
    // 解除加农炮选取半秒后才能发射的限制
    mBoard->mCobCannonCursorDelayCounter = 0;

    old_GamepadControls_ButtonDownFireCobcannonTest(this);
}

void GamepadControls::InvalidatePreviewReanim() {
    Reanimation *aPreviewReanim4 = mApp->ReanimationTryToGet(mPreviewReanimID4);
    if (aPreviewReanim4 != nullptr) {
        aPreviewReanim4->ReanimationDie();
        mPreviewReanimID4 = ReanimationID::REANIMATIONID_NULL;
    }

    if (mPreviewReanimID3 != 0) {
        mApp->RemoveReanimation(mPreviewReanimID3);
        mPreviewReanimID3 = REANIMATIONID_NULL;
    }
}

FilterEffect GetFilterEffectTypeBySeedType(SeedType mSeedType) {
    if (mSeedType == SeedType::SEED_HYPNOSHROOM || mSeedType == SeedType::SEED_SQUASH || mSeedType == SeedType::SEED_POTATOMINE || mSeedType == SeedType::SEED_GARLIC
        || mSeedType == SeedType::SEED_LILYPAD) {
        return FilterEffect::FILTEREFFECT_LESS_WASHED_OUT;
    }

    return FilterEffect::FILTEREFFECT_WASHED_OUT;
}

void GamepadControls::UpdatePreviewReanim() {
    // 动态预览!!

    // TV后续版本仅在PreviewingSeedType切换时进行一次Reanimation::Update，而TV 1.0.1则是无时无刻进行Reanimation::Update。我们恢复1.0.1的逻辑即可。

    CursorObject *aCursorObject = mPlayerIndex ? mBoard->mCursorObject[1] : mBoard->mCursorObject[0];
    SeedBank *aSeedBank = GetSeedBank();

    if (!dynamicPreview) { // 如果没开启动态预览，则开启砸罐子无尽和锤僵尸关卡的预览，并执行旧游戏函数。
        if ((mApp->IsWhackAZombieLevel() || mApp->IsScaryPotterLevel()) && mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            if (aSeedBank == nullptr || mSelectedSeedIndex < 0 || mSelectedSeedIndex >= 10) {
                old_GamepadControls_UpdatePreviewReanim(this);
                return;
            }
            SeedPacket *seedPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
            aCursorObject->mType = seedPacket->mPacketType;
            aCursorObject->mImitaterType = seedPacket->mImitaterType;
        }
        old_GamepadControls_UpdatePreviewReanim(this);
        return;
    }

    GameMode aGameMode = mApp->mGameMode;
    int aGridX = mBoard->PixelToGridXKeepOnBoard(mCursorPositionX, mCursorPositionY);
    int aGridY = mBoard->PixelToGridYKeepOnBoard(mCursorPositionX, mCursorPositionY);
    if (aSeedBank == nullptr || mSelectedSeedIndex < 0 || mSelectedSeedIndex >= 10) {
        return;
    }

    bool isImitater = aSeedBank->mSeedPackets[mSelectedSeedIndex].mPacketType == SeedType::SEED_IMITATER;

    if ((mApp->IsWhackAZombieLevel() || aGameMode == GameMode::GAMEMODE_SCARY_POTTER_ENDLESS) && mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
        // 开启砸罐子无尽和锤僵尸关卡的动态预览
        SeedPacket *seedPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
        aCursorObject->mType = seedPacket->mPacketType;
        aCursorObject->mImitaterType = seedPacket->mImitaterType;
    }

    SeedType aSeedType = aCursorObject->mType;

    if (mIsZombie) {
        aSeedType = aSeedBank->mSeedPackets[mSelectedSeedIndex].mPacketType;
    }
    if (aSeedType == SeedType::SEED_IMITATER) {
        aSeedType = aCursorObject->mImitaterType;
    }

    bool flagUpdateCanPlant = true;
    bool flagDrawGray = false;
    bool flagUpdateChangeType = false;
    if (mPreviewingSeedType != aSeedType && aSeedType != SeedType::SEED_NONE) {
        Reanimation *aNewPreviewingReanim = nullptr;
        InvalidatePreviewReanim();
        RenderLayer aRenderLayer = mIsZombie ? RENDER_LAYER_ZOMBIE : RENDER_LAYER_PLANT;
        int aRenderOrder = Board::MakeRenderOrder(aRenderLayer, aGridY, 100);
        float theDrawHeightOffset = PlantDrawHeightOffset(mBoard, nullptr, aSeedType, aGridX, aGridY);
        if (mIsZombie) {
            ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(aSeedType);
            switch (aZombieType) {
                case ZombieType::ZOMBIE_INVALID:
                    mPreviewingSeedType = aSeedType;
                    return;
                case ZombieType::ZOMBIE_GARGANTUAR:
                case ZombieType::ZOMBIE_GIGA_GARGANTUAR:
                case ZombieType::ZOMBIE_SUPER_NOVA_GARGANTUAR:
                    theDrawHeightOffset += 30.0;
                    break;
                case ZombieType::ZOMBIE_POLEVAULTER:
                case ZombieType::ZOMBIE_GIGA_POLEVAULTER:
                    theDrawHeightOffset += 15.0;
                    break;
                default:
                    break;
            }
            ZombieDefinition &aZombieDefinition = GetZombieDefinition(aZombieType);
            const float aZombiePreviewX = -20.0f;
            const float aZombiePreviewY = -35.0f - theDrawHeightOffset;
            Reanimation *aZombieReanim = mApp->AddReanimation(aZombiePreviewX, aZombiePreviewY, aRenderOrder + 1, aZombieDefinition.mReanimationType);
            Zombie::SetupReanimLayers(aZombieReanim, aZombieType);
            if (aZombieType == ZombieType::ZOMBIE_DOOR || aZombieType == ZombieType::ZOMBIE_TRASHCAN || aZombieType == ZombieType::ZOMBIE_NEWSPAPER || aZombieType == ZombieType::ZOMBIE_LADDER) {
                Zombie::SetupShieldReanims(aZombieType, aZombieReanim);
            }
            if (aZombieType == ZombieType::ZOMBIE_SUNDAY_EDITION) {
                aZombieReanim->AssignRenderGroupToPrefix("Zombie_paper_hands", RENDER_GROUP_OVER_SHIELD);
                aZombieReanim->AssignRenderGroupToTrack("Zombie_paper_paper", RENDER_GROUP_SHIELD);
            }
            aZombieReanim->mIsAttachment = true;
            if (aZombieType == ZombieType::ZOMBIE_POGO) {
                aZombieReanim->PlayReanim("anim_pogo", ReanimLoopType::REANIM_LOOP, 0, 12.0);
            } else if (aZombieType == ZombieType::ZOMBIE_DANCER) {
                aZombieReanim->PlayReanim("anim_armraise", ReanimLoopType::REANIM_LOOP, 0, 12.0);
            } else if (aZombieType == ZombieType::ZOMBIE_ZAMBONI) {
                aZombieReanim->PlayReanim("anim_drive", ReanimLoopType::REANIM_LOOP, 0, 12.0);
            } else if (aZombieType == ZombieType::ZOMBIE_IMP || aZombieType == ZombieType::ZOMBIE_SUPER_FAN_IMP) {
                aZombieReanim->PlayReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 12.0);
            } else if (aZombieType == ZombieType::ZOMBIE_EXPLORER) {
                aZombieReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 12.0);
                aZombieReanim->AssignRenderGroupToPrefix("Zombie_flaghand", RENDER_GROUP_NORMAL);
                aZombieReanim->AssignRenderGroupToPrefix("Zombie_innerarm_screendoor", RENDER_GROUP_NORMAL);
            } else if (aZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
                aZombieReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 12.0);
                aZombieReanim->HideTrackByPrefix("anim_pole1", true);
                aZombieReanim->HideTrackByPrefix("anim_pole2", true);
            } else if (aZombieType == ZombieType::ZOMBIE_DOGWALKER) {
                aZombieReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 12.0);
                aZombieReanim->HideTrackByPrefix("Zombie_dogwalker_rope2_2", true);
                ZombieDefinition &aDogDefinition = GetZombieDefinition(ZombieType::ZOMBIE_DOG);
                Reanimation *aDogPreviewReanim = mApp->AddReanimation(aZombiePreviewX - 80.0f, aZombiePreviewY, aRenderOrder, aDogDefinition.mReanimationType);
                Zombie::SetupReanimLayers(aDogPreviewReanim, ZombieType::ZOMBIE_DOG);
                aDogPreviewReanim->mIsAttachment = true;
                aDogPreviewReanim->PlayReanim("anim_walk", ReanimLoopType::REANIM_LOOP, 0, 24.0);
                mPreviewReanimID3 = mApp->ReanimationGetID(aDogPreviewReanim);
            } else {
                if (aZombieType == ZombieType::ZOMBIE_FLAG) {
                    Reanimation *zombieReanimAttachment = mApp->AddReanimation(0, 0, 0, ReanimationType::REANIM_ZOMBIE_FLAGPOLE);
                    zombieReanimAttachment->PlayReanim("Zombie_flag", ReanimLoopType::REANIM_LOOP, 0, 15.0);
                    mPreviewReanimID3 = mApp->ReanimationGetID(zombieReanimAttachment);
                    ReanimatorTrackInstance *TrackInstanceByName = aZombieReanim->GetTrackInstanceByName("Zombie_flaghand");
                    AttachReanim(TrackInstanceByName->mAttachmentID, zombieReanimAttachment, 0.0, 0.0);
                    aZombieReanim->mFrameBasePose = 0;
                } else if (aZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR) {
                    aZombieReanim->SetImageOverride("anim_head1", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD_REDEYE);
                } else if (aZombieType == ZombieType::ZOMBIE_PEA_HEAD) {
                    aZombieReanim->HideTrackByPrefix("anim_hair", true);
                    aZombieReanim->HideTrackByPrefix("anim_head2", true);
                    aZombieReanim->SetFramesForLayer("anim_walk2");
                    ReanimatorTrackInstance *aTrackInstance = aZombieReanim->GetTrackInstanceByName("anim_head1");
                    aTrackInstance->mImageOverride = IMAGE_BLANK;
                    Reanimation *aPeaHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_PEASHOOTER);
                    aPeaHeadReanim->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
                    AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aPeaHeadReanim, 0.0f, 0.0f);
                    aZombieReanim->mFrameBasePose = 0;
                    TodScaleRotateTransformMatrix((SexyMatrix3 &)aAttachEffect->mOffset, 65.0f, -8.0f, 0.2f, -1.0f, 1.0f);
                } else if (aZombieType == ZombieType::ZOMBIE_WALLNUT_HEAD) {
                    aZombieReanim->HideTrackByPrefix("anim_hair", true);
                    aZombieReanim->HideTrackByPrefix("anim_head", true);
                    aZombieReanim->HideTrackByPrefix("Zombie_tie", true);
                    aZombieReanim->SetFramesForLayer("anim_walk2");
                    ReanimatorTrackInstance *aTrackInstance = aZombieReanim->GetTrackInstanceByName("zombie_body");
                    Reanimation *aWallnutHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_WALLNUT);
                    aWallnutHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
                    AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aWallnutHeadReanim, 0.0f, 0.0f);
                    aZombieReanim->mFrameBasePose = 0;
                    TodScaleRotateTransformMatrix((SexyMatrix3 &)aAttachEffect->mOffset, 50.0f, 0.0f, 0.2f, -0.8f, 0.8f);
                } else if (aZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD) {
                    aZombieReanim->HideTrackByPrefix("anim_hair", true);
                    aZombieReanim->HideTrackByPrefix("anim_head", true);
                    aZombieReanim->HideTrackByPrefix("Zombie_tie", true);
                    aZombieReanim->SetFramesForLayer("anim_walk2");
                    ReanimatorTrackInstance *aTrackInstance = aZombieReanim->GetTrackInstanceByName("zombie_body");
                    Reanimation *aTallnutHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_TALLNUT);
                    aTallnutHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
                    AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aTallnutHeadReanim, 0.0f, 0.0f);
                    aZombieReanim->mFrameBasePose = 0;
                    TodScaleRotateTransformMatrix((SexyMatrix3 &)aAttachEffect->mOffset, 37.0f, 0.0f, 0.2f, -0.8f, 0.8f);
                } else if (aZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD) {
                    aZombieReanim->HideTrackByPrefix("anim_hair", true);
                    aZombieReanim->HideTrackByPrefix("anim_head", true);
                    aZombieReanim->HideTrackByPrefix("Zombie_tie", true);
                    aZombieReanim->SetFramesForLayer("anim_walk2");
                    ReanimatorTrackInstance *aTrackInstance = aZombieReanim->GetTrackInstanceByName("zombie_body");
                    Reanimation *aJalapenoHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_JALAPENO);
                    aJalapenoHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
                    AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aJalapenoHeadReanim, 0.0f, 0.0f);
                    aZombieReanim->mFrameBasePose = 0;
                    TodScaleRotateTransformMatrix((SexyMatrix3 &)aAttachEffect->mOffset, 55.0f, -5.0f, 0.2f, -1.0f, 1.0f);
                } else if (aZombieType == ZombieType::ZOMBIE_GATLING_HEAD) {
                    aZombieReanim->HideTrackByPrefix("anim_hair", true);
                    aZombieReanim->HideTrackByPrefix("anim_head2", true);
                    aZombieReanim->SetFramesForLayer("anim_walk2");
                    ReanimatorTrackInstance *aTrackInstance = aZombieReanim->GetTrackInstanceByName("anim_head1");
                    aTrackInstance->mImageOverride = IMAGE_BLANK;
                    Reanimation *aGatlingHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_GATLINGPEA);
                    aGatlingHeadReanim->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
                    AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aGatlingHeadReanim, 0.0f, 0.0f);
                    aZombieReanim->mFrameBasePose = 0;
                    TodScaleRotateTransformMatrix((SexyMatrix3 &)aAttachEffect->mOffset, 65.0f, -5.0f, 0.2f, -1.0f, 1.0f);
                } else if (aZombieType == ZombieType::ZOMBIE_SQUASH_HEAD) {
                    aZombieReanim->HideTrackByPrefix("anim_hair", true);
                    aZombieReanim->HideTrackByPrefix("anim_head2", true);
                    aZombieReanim->SetFramesForLayer("anim_walk2");
                    ReanimatorTrackInstance *aTrackInstance = aZombieReanim->GetTrackInstanceByName("anim_head1");
                    aTrackInstance->mImageOverride = IMAGE_BLANK;
                    Reanimation *aSquashHeadReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_SQUASH);
                    aSquashHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
                    AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aSquashHeadReanim, 0.0f, 0.0f);
                    aZombieReanim->mFrameBasePose = 0;
                    TodScaleRotateTransformMatrix((SexyMatrix3 &)aAttachEffect->mOffset, 55.0f, -15.0f, 0.2f, -0.75f, 0.75f);
                }
                aZombieReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 12.0);
            }
            aNewPreviewingReanim = aZombieReanim;
        } else {
            if (aSeedType == SeedType::NUM_SEEDS_IN_CHOOSER
                || (aSeedType >= SeedType::NUM_SEED_TYPES && aSeedType < SeedType::SEED_ICEBERG_LETTUCE && aSeedType >= SeedType::NUM_SEEDS_IN_CHOOSER_EXTENDED))
                return;
            Reanimation *plantReanim = mApp->AddReanimation(0.0, theDrawHeightOffset, aRenderOrder + 2, GetPlantDefinition(aSeedType).mReanimationType);
            plantReanim->mIsAttachment = true;
            if (isImitater)
                plantReanim->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
            plantReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 12.0);

            // 为豌豆家族加入其stem动画
            if (aSeedType == SeedType::SEED_PEASHOOTER || aSeedType == SeedType::SEED_SNOWPEA || aSeedType == SeedType::SEED_REPEATER || aSeedType == SeedType::SEED_GATLINGPEA
                || aSeedType == SeedType::SEED_LEFTPEATER) {
                Reanimation *plantReanimAttachment = mApp->AddReanimation(0.0, theDrawHeightOffset, aRenderOrder + 3, GetPlantDefinition(aSeedType).mReanimationType);
                plantReanimAttachment->mLoopType = ReanimLoopType::REANIM_LOOP;
                if (isImitater)
                    plantReanimAttachment->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
                plantReanimAttachment->SetFramesForLayer("anim_head_idle");
                const char *trackName = "anim_stem";
                if (plantReanim->TrackExists(trackName) || (trackName = "anim_idle", plantReanim->TrackExists(trackName))) {
                    plantReanimAttachment->AttachToAnotherReanimation(plantReanim, trackName);
                }
            }
            // 为反向射手的两个头、三发射手的三个头加入动画
            if (aSeedType == SeedType::SEED_SPLITPEA) {
                Reanimation *plantReanimAttachment1 = mApp->AddReanimation(0.0, 0.0, aRenderOrder + 3, GetPlantDefinition(aSeedType).mReanimationType);
                plantReanimAttachment1->mAnimRate = plantReanim->mAnimRate;
                plantReanimAttachment1->mLoopType = ReanimLoopType::REANIM_LOOP;
                if (isImitater)
                    plantReanimAttachment1->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
                plantReanimAttachment1->SetFramesForLayer("anim_head_idle");
                plantReanimAttachment1->AttachToAnotherReanimation(plantReanim, "anim_idle");
                Reanimation *plantReanimAttachment2 = mApp->AddReanimation(0.0, 0.0, aRenderOrder + 3, GetPlantDefinition(aSeedType).mReanimationType);
                plantReanimAttachment2->mAnimRate = plantReanim->mAnimRate;
                plantReanimAttachment2->mLoopType = ReanimLoopType::REANIM_LOOP;
                if (isImitater)
                    plantReanimAttachment2->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
                plantReanimAttachment2->SetFramesForLayer("anim_splitpea_idle");
                plantReanimAttachment2->AttachToAnotherReanimation(plantReanim, "anim_idle");
            } else if (aSeedType == SeedType::SEED_THREEPEATER) {
                plantReanim->mAnimRate = RandRangeFloat(15.0, 20.0);
                Reanimation *plantReanimAttachment1 = mApp->AddReanimation(0.0, 0.0, aRenderOrder + 3, GetPlantDefinition(aSeedType).mReanimationType);
                plantReanimAttachment1->mAnimRate = plantReanim->mAnimRate;
                plantReanimAttachment1->mLoopType = ReanimLoopType::REANIM_LOOP;
                if (isImitater)
                    plantReanimAttachment1->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
                plantReanimAttachment1->SetFramesForLayer("anim_head_idle1");
                plantReanimAttachment1->AttachToAnotherReanimation(plantReanim, "anim_head1");
                Reanimation *plantReanimAttachment2 = mApp->AddReanimation(0.0, 0.0, aRenderOrder + 3, GetPlantDefinition(aSeedType).mReanimationType);
                plantReanimAttachment2->mAnimRate = plantReanim->mAnimRate;
                plantReanimAttachment2->mLoopType = ReanimLoopType::REANIM_LOOP;
                if (isImitater)
                    plantReanimAttachment2->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
                plantReanimAttachment2->SetFramesForLayer("anim_head_idle2");
                plantReanimAttachment2->AttachToAnotherReanimation(plantReanim, "anim_head2");
                Reanimation *plantReanimAttachment3 = mApp->AddReanimation(0.0, 0.0, aRenderOrder + 3, GetPlantDefinition(aSeedType).mReanimationType);
                plantReanimAttachment3->mAnimRate = plantReanim->mAnimRate;
                plantReanimAttachment3->mLoopType = ReanimLoopType::REANIM_LOOP;
                if (isImitater)
                    plantReanimAttachment3->mFilterEffect = GetFilterEffectTypeBySeedType(aSeedType);
                plantReanimAttachment3->SetFramesForLayer("anim_head_idle3");
                plantReanimAttachment3->AttachToAnotherReanimation(plantReanim, "anim_head3");
            }
            aNewPreviewingReanim = plantReanim;
        }
        mPreviewingSeedType = aSeedType;
        mPreviewReanimID4 = mApp->ReanimationGetID(aNewPreviewingReanim);
        flagUpdateChangeType = true;
    } else {
        // 如果目标预览植物类型没变化, 则为模仿者上色
        Reanimation *mPreviewReanim4 = mApp->ReanimationTryToGet(mPreviewReanimID4);
        if (mPreviewReanim4 != nullptr) {
            FilterEffect aFilterEffect = isImitater ? GetFilterEffectTypeBySeedType(aSeedType) : FilterEffect::FILTEREFFECT_NONE;
            mPreviewReanim4->mFilterEffect = aFilterEffect;
            if (aSeedType == SeedType::SEED_THREEPEATER || aSeedType == SeedType::SEED_SPLITPEA || aSeedType == SeedType::SEED_PEASHOOTER || aSeedType == SeedType::SEED_SNOWPEA
                || aSeedType == SeedType::SEED_REPEATER || aSeedType == SeedType::SEED_GATLINGPEA || aSeedType == SeedType::SEED_LEFTPEATER) {
                int mTrackCount = mPreviewReanim4->mDefinition->mTrackCount;
                for (int i = 0; i < mTrackCount; i++) {
                    ReanimatorTrackInstance *reanimatorTrackInstance = mPreviewReanim4->mTrackInstances + i;
                    uint16_t mAttachmentID = reanimatorTrackInstance->mAttachmentID;
                    if (mAttachmentID == 0)
                        continue;
                    if (gEffectSystem == nullptr || gEffectSystem->mAttachmentHolder == nullptr)
                        break;
                    Attachment *attachment = gEffectSystem->mAttachmentHolder->mAttachments.DataArrayTryToGet(mAttachmentID);
                    if (attachment == nullptr)
                        continue;
                    int mNumEffects = attachment->mNumEffects;
                    for (int j = 0; j < mNumEffects; ++j) {
                        if (attachment->mEffectArray[j].mEffectType == EffectType::EFFECT_REANIM) {
                            Reanimation *attachReanim = mApp->ReanimationTryToGet(attachment->mEffectArray[j].mEffectID);
                            if (attachReanim != nullptr) {
                                attachReanim->mFilterEffect = aFilterEffect;
                            }
                        }
                    }
                }
            }
        }
    }

    Reanimation *aPreviewReanim = mApp->ReanimationTryToGet(mPreviewReanimID4);
    if (aPreviewReanim == nullptr) {
        return;
    }

    Reanimation *aDogPreviewReanim = nullptr;
    if (mIsZombie && Challenge::IZombieSeedTypeToZombieType(aSeedType) == ZombieType::ZOMBIE_DOGWALKER) {
        aDogPreviewReanim = mApp->ReanimationTryToGet(mPreviewReanimID3);
    }

    if (aCursorObject->mCursorType != CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN && mGamepadState != MOVEMENT_STATE_PLANT_CURSOR) {
        return;
    }

    if (aCursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
        if (mBoard->CanPlantAt(aGridX, aGridY, aSeedType) != PlantingReason::PLANTING_OK) {
            flagUpdateCanPlant = false;
            flagDrawGray = true;
        }
    } else {
        if (mSelectedSeedIndex == -1) {
            return;
        }

        SeedPacket *seedPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
        if (!seedPacket->mActive) {
            flagUpdateCanPlant = false;
            flagDrawGray = true;
        }
        if (mBoard->CanPlantAt(aGridX, aGridY, aSeedType) != PlantingReason::PLANTING_OK) {
            flagUpdateCanPlant = false;
            flagDrawGray = true;
        }
        if (!mBoard->HasConveyorBeltSeedBank(mGamepadIndex) && aCursorObject->mCursorType != CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            if (aGameMode == GameMode::GAMEMODE_MP_VS) {
                if (mIsZombie) {
                    if (!mBoard->CanTakeDeathMoney(mBoard->GetCurrentPlantCost(aSeedType, SeedType::SEED_NONE))) {
                        flagUpdateCanPlant = false;
                        flagDrawGray = true;
                    }
                } else {
                    if (!mBoard->CanTakeSunMoney(mBoard->GetCurrentPlantCost(aSeedType, SeedType::SEED_NONE), 0)) {
                        flagUpdateCanPlant = false;
                        flagDrawGray = true;
                    }
                }
            } else {
                if (!mBoard->CanTakeSunMoney(mBoard->GetCurrentPlantCost(aSeedType, SeedType::SEED_NONE), mGamepadIndex)) {
                    flagUpdateCanPlant = false;
                    flagDrawGray = true;
                }
            }
        }
    }


    Graphics gPreview(mPreviewImage);
    gPreview.ClearRect(0, 0, mPreviewImage->mWidth, mPreviewImage->mHeight);
    gPreview.Translate(256, 256);
    if (flagUpdateCanPlant || flagUpdateChangeType) {
        aPreviewReanim->Update();
        if (aDogPreviewReanim != nullptr) {
            aDogPreviewReanim->Update();
        }
    }
    if (flagDrawGray) {
        gPreview.SetColorizeImages(true);
        (gPreview).SetColor(gColorGray);
    }

    aPreviewReanim->Draw(&gPreview);
    aPreviewReanim->DrawRenderGroup(&gPreview, 2);
    aPreviewReanim->DrawRenderGroup(&gPreview, 1);
    aPreviewReanim->DrawRenderGroup(&gPreview, 3);

    if (aDogPreviewReanim != nullptr) {
        aDogPreviewReanim->Draw(&gPreview);
        aDogPreviewReanim->DrawRenderGroup(&gPreview, 2);
        aDogPreviewReanim->DrawRenderGroup(&gPreview, 1);
        aDogPreviewReanim->DrawRenderGroup(&gPreview, 3);
    }
}

void GamepadControls::UpdateStates(float dt) {
    BaseGamepadControls::UpdateStates(dt);

    if (mGamepadState == MOVEMENT_STATE_PLANT_CURSOR || mGamepadState == MOVEMENT_STATE_SELECT_SEED || mGamepadState == MOVEMENT_STATE_DIG_HOLD) {
        SeedBank *seedBank = GetSeedBank();
        if (!HasValidSelectedSeed(this, seedBank)) {
            LOG_WARN("[NETPLAY] cancel seed state={} with invalid selected index={} packetCount={}", int(mGamepadState), mSelectedSeedIndex, seedBank ? seedBank->mNumPackets : -1);
            mSelectedSeedIndex = -1;
            mGamepadState = MOVEMENT_STATE_NORMAL;
            return;
        }
    }

    // 在状态为MOVEMENT_STATE_PLANT_CURSOR下也要执行UpdateSeedSelect，此函数会更新CursorObject的mType为GetSeedBank->mSeedPackets[mSelectedSeedIndex].mPacketType，或许可以修复偶现的种下种子与SeedBank选中种子不一致的BUG
    if (mGamepadState == MOVEMENT_STATE_PLANT_CURSOR) {
        UpdateSeedSelect(dt);
        return;
    }

    //    if ( mGamepadState == 6 || mGamepadState == 8 )
    if (mGamepadState == MOVEMENT_STATE_SELECT_SEED || mGamepadState == MOVEMENT_STATE_DIG_HOLD) {
        UpdateSeedSelect(dt);
        float v5 = mGridCenterPositionX - mCursorPositionX;
        float v6 = mGridCenterPositionY - mCursorPositionY;
        float distanceSquared = (v5 * v5) + (v6 * v6);
        if (distanceSquared < 3.0f * 3.0f) {
            mGamepadVelocityLeftX = 0.0f;
            mGamepadVelocityLeftY = 0.0f;
            mGridCenterPositionY = mGridCenterPositionY;
            mCursorPositionX = mGridCenterPositionX;
            mCursorPositionY = mGridCenterPositionY;
        } else {
            if (distanceSquared != 0.0f) {
                const float distance = std::sqrt(distanceSquared);
                v6 = v6 / distance;
                v5 = v5 / distance;
            }
            mGamepadVelocityLeftX = v5 * 250.0f;
            mGamepadVelocityLeftY = v6 * 250.0f;
        }
    }
}

void GamepadControls::DrawPreview(Sexy::Graphics *g) {
    // 修复排山倒海、砸罐子无尽、锤僵尸、种子雨不显示植物预览的问题。
    LawnApp *anApp = mApp;
    GameMode mGameMode = anApp->mGameMode;
    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS) { // 为种子雨添加种植预览
        CursorObject *cursorObject = mPlayerIndex ? mBoard->mCursorObject[1] : mBoard->mCursorObject[0];
        if (cursorObject->mCursorType == CursorType::CURSOR_TYPE_PLANT_FROM_USABLE_COIN) {
            mGamepadState = MOVEMENT_STATE_PLANT_CURSOR;
            old_GamepadControls_DrawPreview(this, g);
            mGamepadState = MOVEMENT_STATE_NORMAL;
            return;
        }
    }

    if (anApp->IsWhackAZombieLevel() || anApp->IsScaryPotterLevel()) {
        if (mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR) {
            SeedBank *seedBank = GetSeedBank();
            SeedPacket *seedPacket = &seedBank->mSeedPackets[mSelectedSeedIndex];
            mSelectedSeedType = seedPacket->mPacketType == SeedType::SEED_IMITATER ? seedPacket->mImitaterType : seedPacket->mPacketType;
            old_GamepadControls_DrawPreview(this, g);
            return;
        }
    }
    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_COLUMN) {
        int mGridX = mBoard->PixelToGridXKeepOnBoard(mCursorPositionX, mCursorPositionY);
        int mGridY = mBoard->PixelToGridYKeepOnBoard(mCursorPositionX, mCursorPositionY);
        if (mSelectedSeedType != SeedType::SEED_NONE) {
            g->SetColorizeImages(true);
            Sexy::Color theColor = {255, 255, 255, 125};
            g->SetColor(theColor);
            g->Translate(-256, -256);
            if (dynamicPreview) { // 修复动态预览时植物错位
                int thePixelY = mBoard->GridToPixelY(mGridX, mGridY);
                for (int i = 0; i != 6; ++i) {
                    if (mBoard->CanPlantAt(mGridX, i, mSelectedSeedType) == PlantingReason::PLANTING_OK) {
                        int theGridPixelY = mBoard->GridToPixelY(mGridX, i);
                        g->DrawImage(mPreviewImage, 0, theGridPixelY - thePixelY);
                    }
                }
            } else {
                for (int i = 0; i != 6; ++i) {
                    if (mBoard->CanPlantAt(mGridX, i, mSelectedSeedType) == PlantingReason::PLANTING_OK) {
                        float offset = PlantDrawHeightOffset(mBoard, nullptr, mSelectedSeedType, mGridX, i);
                        g->DrawImage(mPreviewImage, 0, offset + (i - mGridY) * 85);
                    }
                }
            }
            g->Translate(256, 256);
            g->SetColorizeImages(false);
        }
        return;
    }

    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_BIG_TIME
        && (mSelectedSeedType == SeedType::SEED_SUNFLOWER || mSelectedSeedType == SeedType::SEED_WALLNUT || mSelectedSeedType == SeedType::SEED_MARIGOLD)) {
        // 种大突破关卡 放大植物预览
        g->SetScale(1.5, 1.5, 0, 0);
        g->Translate(-15, -25);
        old_GamepadControls_DrawPreview(this, g);
        g->SetScale(1, 1, 0, 0);
        g->Translate(15, 25);
        return;
    }

    if (mSelectedSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE || mSelectedSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
        return;
    }

    if (mGamepadState == BaseGamepadControls::MOVEMENT_STATE_PLANT_CURSOR && mSelectedSeedType == SeedType::SEED_ZOMBIE_MOUND) {
        g->SetColorizeImages(true);
        int aGridX = mBoard->PixelToGridXKeepOnBoard(int(mCursorPositionX), int(mCursorPositionY));
        int aGridY = mBoard->PixelToGridYKeepOnBoard(int(mCursorPositionX), int(mCursorPositionY));
        GridItem *aGraveStone = mBoard->GetGraveStoneAt(aGridX, aGridY);
        GridItem *aMound = mBoard->GetMoundAt(aGridX, aGridY);

        bool canPreviewPlant = false;
        if (aGraveStone) {
            canPreviewPlant = true;
        } else if (aMound && aMound->mMoundLevel < 4) {
            canPreviewPlant = true;
        }
        canPreviewPlant = canPreviewPlant && (mBoard->CanPlantAt(aGridX, aGridY, mSelectedSeedType) == PlantingReason::PLANTING_OK);
        if (canPreviewPlant) {
            SeedBank *aSeedBank = GetSeedBank();
            SeedPacket *aSeedPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
            int aCost = mBoard->GetCurrentPlantCost(aSeedPacket->mPacketType, SeedType::SEED_NONE);
            canPreviewPlant = mBoard->CanTakeDeathMoney(aCost);
        }

        Color aColor = canPreviewPlant ? Color(255, 255, 255, 125) : Color(96, 96, 96, 125);
        g->SetColor(aColor);

        Image *aImage = addonImages.seed_mounds;
        int aCelWidth = aImage->GetCelWidth();
        int aCelCol = 2;
        if (aMound) {
            if (aMound->mMoundLevel == 0) {
                aCelCol = 0;
            } else if (aMound->mMoundLevel == 1) {
                aCelCol = 3;
            } else if (aMound->mMoundLevel == 2) {
                aCelCol = 4;
            } else if (aMound->mMoundLevel == 3) {
                aCelCol = 1;
            }
        }
        Rect aRect = Rect(aCelWidth * aCelCol, 0, aCelWidth, aImage->GetCelHeight());
        g->DrawImage(aImage, 0, 0, aRect);

        g->SetColorizeImages(false);

        UpdatePreviewReanim();
        return;
    }

    old_GamepadControls_DrawPreview(this, g);
}

void GamepadControls::OnButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int unk) {
    if (netplay::IsSynchronizingSaveGame()) {
        return;
    }

    if (!mApp->IsVSMode() || theButton != Sexy::GamepadButton::GAMEPAD_BUTTON_A) {
        return old_GamepadControls_OnButtonDown(this, theButton, thePlayerIndex, unk);
    }

    SeedBank *aSeedBank = GetSeedBank();
    if (!HasValidSelectedSeed(this, aSeedBank)) {
        LOG_WARN("[NETPLAY] ignore plant input with invalid selected index={} packetCount={}", mSelectedSeedIndex, aSeedBank ? aSeedBank->mNumPackets : -1);
        mSelectedSeedIndex = -1;
        mGamepadState = MOVEMENT_STATE_NORMAL;
        return;
    }
    SeedPacket *aSeedPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
    SeedType aPacketType = aSeedPacket->mPacketType;

    if (mBoard->HasLevelAwardDropped() || (mBoard->mChallenge->IsMPSuddenDeath() && Challenge::gVSSuddenDeathMode <= 1 && Challenge::IsMPResourceProducer(aSeedPacket->mPacketType))
        || mBoard->mChallenge->ISMPSeedSuddenDeathDisabled(aSeedBank->mIsZombie, aPacketType)) {
        bool isClientGamepadControl = mGamepadIndex == 1;
        if (IsRemoteServer() && isClientGamepadControl) { // 让对方播放音效
            U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 1};
            netplay::PutEvent(event);
        } else {
            mApp->PlaySample(SOUND_BUZZER);
            if (IsRemoteServer()) {
                U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_BUZZER)};
                netplay::PutEvent(event);
            }
        }
        return;
    }

    int aPacketCost = mBoard->GetCurrentPlantCost(aSeedPacket->mPacketType, SeedType::SEED_NONE);
    int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
    int aGridY = mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);

    if (mIsZombie) {

        if (!mBoard->CanTakeDeathMoney(aPacketCost) || !aSeedPacket->CanPickUp() || mBoard->CanPlantAt(aGridX, aGridY, aPacketType) != PlantingReason::PLANTING_OK || mBoard->HasLevelAwardDropped()) {
            bool isClientGamepadControl = mGamepadIndex == 1;
            if (IsRemoteServer() && isClientGamepadControl) { // 让对方播放音效
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 1};
                netplay::PutEvent(event);
            } else {
                mApp->PlaySample(SOUND_BUZZER);
                if (IsRemoteServer()) {
                    U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_BUZZER)};
                    netplay::PutEvent(event);
                }
            }
            return;
        }


        if (aPacketType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
            std::vector<SeedType> aPlantSeeds, aZombieSeeds;
            PickShuffleSeeds(mApp, aPlantSeeds, aZombieSeeds, true);
            if (!aZombieSeeds.empty()) {
                for (int aPacketIndex = 1; aPacketIndex <= aZombieSeeds.size(); ++aPacketIndex) {
                    SeedType aSeedType = aZombieSeeds[aPacketIndex - 1];
                    aSeedBank->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                }
            }
            mBoard->TakeDeathMoney(aPacketCost);
            aSeedPacket->Deactivate();
            aSeedPacket->WasPlanted(mGamepadIndex);
            return;
        }

        if (aPacketType == SeedType::SEED_ZOMBIE_MOUND) {
            GridItem *aGraveStone = mBoard->GetGraveStoneAt(aGridX, aGridY);
            GridItem *aMound = mBoard->GetMoundAt(aGridX, aGridY);

            int aTargetLevel = -1;
            GridItem *aTargetGridItem = nullptr;

            if (aGraveStone) {
                aTargetLevel = 0;
                aTargetGridItem = aGraveStone;
            } else if (aMound && aMound->mMoundLevel < 4) {
                aTargetLevel = aMound->mMoundLevel + 1;
                aTargetGridItem = aMound;
            }

            if (aTargetLevel >= 0 && mBoard->TakeDeathMoney(aPacketCost)) {
                GridItem *aUpgradeMound = mBoard->AddAMound(aGridX, aGridY, aTargetLevel);
                if (aUpgradeMound) {
                    aUpgradeMound->mIsSpecialGrave = true;
                    aUpgradeMound->mVSGraveStoneHealth = 350 + 70 * (aTargetLevel + 1);
                }

                if (aTargetGridItem) {
                    aTargetGridItem->GridItemDie();
                }

                aSeedPacket->Deactivate();
                aSeedPacket->WasPlanted(mPlayerIndex);
                mGamepadState = MOVEMENT_STATE_NORMAL;
                return;
            }
        }

        if (aPacketType == SeedType::SEED_ZOMBIE_GRAVESTONE) {
            if (IsRemoteClientOrViewer()) {
                return;
            }
            if (mBoard->CanAddGraveStoneAt(aGridX, aGridY) && mBoard->TakeDeathMoney(aPacketCost)) {
                GridItem *aGraveStone = mBoard->AddAGraveStone(aGridX, aGridY);
                aGraveStone->mIsSpecialGrave = false;
                aGraveStone->mVSGraveStoneHealth = 350;
                aSeedPacket->Deactivate();
                aSeedPacket->WasPlanted(mPlayerIndex);
                mGamepadState = MOVEMENT_STATE_NORMAL;
                return;
            }
        }

        ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(aPacketType);
        if (aZombieType != ZombieType::ZOMBIE_INVALID && mBoard->TakeDeathMoney(aPacketCost)) {
            if (aZombieType == ZombieType::ZOMBIE_BUNGEE) {
                Zombie *aBungeeZombie = mBoard->AddZombieInRow(aZombieType, aGridY, 0, false);
                aBungeeZombie->mTargetCol = aGridX;
                aBungeeZombie->SetRow(aGridY);
                aBungeeZombie->mPosX = float(mBoard->GridToPixelX(aGridX, aGridY));
                aBungeeZombie->mPosY = aBungeeZombie->GetPosYBasedOnRow(aGridY);
                aBungeeZombie->mRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, aGridY, 7);
            } else if (aZombieType == ZombieType::ZOMBIE_FLAG) {
                mBoard->DisplayAdviceAgain("[ADVICE_HUGE_WAVE]", MessageStyle::MESSAGE_STYLE_HUGE_WAVE, AdviceType::ADVICE_HUGE_WAVE);
                mApp->PlaySample(Sexy::SOUND_HUGE_WAVE); // 播放大波僵尸音效
                mBoard->SpawnZombieWave();
            } else if (Challenge::IsMPZombieTypeAddInRow(aZombieType)) {
                mBoard->AddZombieInRow(aZombieType, aGridY, Zombie::ZOMBIE_WAVE_VS, true);
            } else {
                Zombie *aZombie = nullptr;
                if (mBoard->StageHasPool()) {
                    aZombie = mBoard->AddZombieInRow(aZombieType, aGridY, Zombie::ZOMBIE_WAVE_VS, false);
                } else {
                    aZombie = mBoard->AddZombie(aZombieType, Zombie::ZOMBIE_WAVE_VS, false);
                }
                if (aZombie != nullptr) {
                    if (mBoard->StageHasRoof()) {
                        Zombie *aBungeeZombie = mBoard->AddZombie(ZombieType::ZOMBIE_BUNGEE, Zombie::ZOMBIE_WAVE_VS, false);
                        aBungeeZombie->BungeeDropZombie(aZombie, aGridX, aGridY);
                    } else {
                        aZombie->RiseFromGrave(aGridX, aGridY);
                    }
                }
            }

            aSeedPacket->Deactivate();
            aSeedPacket->WasPlanted(mGamepadIndex);
            mGamepadState = MOVEMENT_STATE_NORMAL;
            return;
        }
    } else {
        if (!mBoard->CanTakeSunMoney(aPacketCost, 0) || !aSeedPacket->CanPickUp() || mBoard->CanPlantAt(aGridX, aGridY, aPacketType) != PlantingReason::PLANTING_OK || mBoard->HasLevelAwardDropped()) {
            // 优化玩家体验：不执行旧函数，使未成功种下的卡槽不会退回未选取状态
            //                old_GamepadControls_OnButtonDown(this, theButton, thePlayerIndex, unk);
            bool isClientGamepadControl = mGamepadIndex == 1;
            if (IsRemoteServer() && isClientGamepadControl) { // 让对方播放音效
                U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND}, 1};
                netplay::PutEvent(event);
            } else {
                mApp->PlaySample(SOUND_BUZZER);
                if (IsRemoteServer()) {
                    U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_PLAY_SOUND_SR}, 0, uint8_t(SOUND_BUZZER)};
                    netplay::PutEvent(event);
                }
            }
            return;
        }

        if ((aPacketType >= SeedType::SEED_PEASHOOTER && aPacketType < SeedType::NUM_SEED_TYPES)
            || (aPacketType >= SeedType::SEED_ICEBERG_LETTUCE && aPacketType < SeedType::NUM_SEEDS_IN_CHOOSER_EXTENDED)) {
            //            LOG_DEBUG("before MouseDownWithPlant {}", mPlayerIndex);
            mBoard->MouseDownWithPlant(mCursorPositionX, mCursorPositionY, 1, mPlayerIndex);
            mBoard->ClearCursor(mPlayerIndex);
            mGamepadState = MOVEMENT_STATE_NORMAL;
            return;
        }

        if (aPacketType == SEED_BEGHOULED_BUTTON_SHUFFLE) {
            std::vector<SeedType> aPlantSeeds, aZombieSeeds;
            PickShuffleSeeds(mApp, aPlantSeeds, aZombieSeeds, false);
            if (!aPlantSeeds.empty()) {
                for (int aPacketIndex = 1; aPacketIndex <= aPlantSeeds.size(); ++aPacketIndex) {
                    SeedType aSeedType = aPlantSeeds[aPacketIndex - 1];
                    aSeedBank->mSeedPackets[aPacketIndex].SetPacketType(aSeedType, SeedType::SEED_NONE);
                }
            }
            mBoard->TakeSunMoney(aPacketCost, 0);
            aSeedPacket->Deactivate();
            aSeedPacket->WasPlanted(mPlayerIndex);
        }
    }
}

void ZenGardenControls::Update(float a2) {
    old_ZenGardenControls_Update(this, a2);
}
