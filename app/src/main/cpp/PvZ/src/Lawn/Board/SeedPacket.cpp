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

#include "PvZ/Lawn/Board/SeedPacket.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Misc.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Misc/SexyMatrix.h"
#include "PvZ/Symbols.h"

#include <cmath>

using namespace Sexy;

void SeedPacket::Update() {
    if (mRefreshing && seedPacketFastCoolDown && !IsOnlineServerModeActive() && !gIsReplayMode) {
        // 正在刷新的种子立即冷却完毕
        mActive = true;
        mRefreshing = false;
    }

    if (requestPause && (!IsOnlineServerModeActive() || gIsReplayMode)) {
        // 在IZ模式不暂停刷新种子卡片
        if (!mApp->IsIZombieLevel()) {
            return;
        }
    }

    UpdateSelected();
    mLastSelectedTime += 0.006;
    unknownIntMember1 += 0.006;

    if (mApp->mGameScene != GameScenes::SCENE_PLAYING || mPacketType == SeedType::SEED_NONE) {
        return;
    }

    if (mBoard->mMainCounter == 0) {
        FlashIfReady();
    }

    if (!mActive && mRefreshing) {
        mRefreshCounter++;
        if (mRefreshCounter > mRefreshTime) {
            mRefreshCounter = 0;
            mRefreshing = false;
            Activate();
            FlashIfReady();

            SetNextRandomSeed(); // 对战刷牌模式更换卡片
        }
    }

    if (mSlotMachineCountDown > 0) {
        mSlotMachineCountDown--;
        float aFlipsPerSecond = TodAnimateCurveFloat(SLOT_MACHINE_TIME, 0, mSlotMachineCountDown, 6.0f, 2.0f, TodCurves::CURVE_LINEAR);
        mSlotMachiningPosition += aFlipsPerSecond * 0.01f;

        if (mSlotMachiningPosition >= 1.0f) {
            mPacketType = mSlotMachiningNextSeed;
            if (mSlotMachineCountDown == 0) {
                Activate();
                mSlotMachiningPosition = 0.0f;
            } else {
                mSlotMachiningPosition -= 1.0f;
                PickNextSlotMachineSeed();
            }
        } else if (mSlotMachineCountDown == 0) {
            mSlotMachineCountDown = 1;
        }
    }
}

void SeedPacket::UpdateSelected() {
    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS || mApp->IsCoopMode()) {
        // 如果是双人模式关卡(对战或结盟)，则使用下面的逻辑来更新当前选中的卡片。用于修复1P和2P的卡片选择框同时出现在两个人各自的植物栏里(也就是植物栏一共出现四个选择框)的问题。

        GamepadControls *theGamepad = GetPlayerIndex() ? mBoard->mGamepadControls[1] : mBoard->mGamepadControls[0];

        mSelected = theGamepad->mSelectedSeedIndex == mIndex;
        mSelectedBy2P = theGamepad->mPlayerIndex == 1 && theGamepad->mSelectedSeedIndex == mIndex;


        return;
    }

    old_SeedPacket_UpdateSelected(this);
}

void SeedPacket::DrawOverlay(Sexy::Graphics *g) {
    // 绘制卡片冷却进度倒计时
    old_SeedPacket_DrawOverlay(this, g);

    if (mRefreshing && showCoolDown) {
        // 如果玩家开启了“显示冷却倒计时”，则绘制倒计时
        int coolDownRemaining = mRefreshTime - mRefreshCounter;
        pvzstl::string str = StrFormat("%1.1f", coolDownRemaining / 100.0f);
        g->SetColor(GetPlayerIndex() ? gColorYellow : gColorBlue);
        g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
        g->DrawString(str, coolDownRemaining < 1000 ? 10 : 0, 39);
        g->SetFont(nullptr);
    }
}

void SeedPacket::Draw(Sexy::Graphics *g) {
    // 绘制卡片冷却进度倒计时
    old_SeedPacket_Draw(this, g);
}

void SeedPacket::MouseDown(int x, int y, int theClickCount, int thePlayerIndex) {
    old_SeedPacket_MouseDown(this, x, y, theClickCount, thePlayerIndex);
}

bool SeedPacket::BeginDraw(Sexy::Graphics *g) {
    return old_SeedPacket_BeginDraw(this, g);
}

void SeedPacket::EndDraw(Sexy::Graphics *g) {
    old_SeedPacket_EndDraw(this, g);
}

void SeedPacket::FlashIfReady() {
    // 去除对战模式下的闪光效果的缩放

    if (!CanPickUp()) {
        return;
    }
    if (mApp->mEasyPlantingCheat) {
        return;
    }

    int playerIndex = 0;
    if (mSeedBank != nullptr) {
        playerIndex = mSeedBank->mIsZombie;
    }
    if (!mBoard->HasConveyorBeltSeedBank(playerIndex) && mSeedBank != nullptr) {
        mApp->AddTodParticle(mSeedBank->mX + mX, mSeedBank->mY + mY, 100000 + 2, ParticleEffect::PARTICLE_SEED_PACKET_FLASH);
    }
    TutorialState tutorialState = mBoard->mTutorialState;
    if (tutorialState == TutorialState::TUTORIAL_LEVEL_1_REFRESH_PEASHOOTER) {
        mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER);
        return;
    }
    if (tutorialState == TutorialState::TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER && mPacketType == SeedType::SEED_SUNFLOWER) {
        mBoard->SetTutorialState(TutorialState::TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER);
        return;
    }
    if (tutorialState == TutorialState::TUTORIAL_MORESUN_REFRESH_SUNFLOWER && mPacketType == SeedType::SEED_SUNFLOWER) {
        mBoard->SetTutorialState(TutorialState::TUTORIAL_MORESUN_PICK_UP_SUNFLOWER);
        return;
    }
}

void SeedPacket::Activate() {
    if (mPacketType != SeedType::SEED_NONE)
        mActive = true;
}

void SeedPacket::SetPacketType(SeedType theSeedType, SeedType theImitaterType) {
    old_SeedPacket_SetPacketType(this, theSeedType, theImitaterType);

    // 此处修改对战开局的初始冷却
    if (mApp->IsVSMode()) {
        bool aIsBalancePatch = VSSetupAddonWidget::msBalancePatchMode;
        bool aIsStageNight = mBoard->StageIsNight();
        switch (theSeedType) {
            case SEED_PUFFSHROOM:
            case SEED_ZOMBIE_NORMAL:
                if (aIsBalancePatch) {
                    mRefreshTime = 1000;
                    mRefreshing = true;
                    mActive = false;
                }
                break;
            case SEED_SUNSHROOM:
                if (aIsStageNight) { // 清除阳光菇的初始冷却
                    mRefreshTime = 0;
                    mRefreshing = false;
                    mActive = true;
                }
                break;
            case SEED_PUMPKINSHELL:
                if (aIsBalancePatch) {
                    mRefreshTime = 0; // 35 -> 0
                    mRefreshing = false;
                    mActive = true;
                }
                break;
            case SEED_ZOMBONI:
            case SEED_ZOMBIE_CATAPULT:
                if (aIsBalancePatch) {
                    mRefreshTime = 2000; // 35 -> 20
                }
                break;
            case SEED_ZOMBIE_TRAFFIC_CONE:
            case SEED_ZOMBIE_BOBSLED:
            case SEED_ZOMBIE_SNORKEL:
            case SEED_ZOMBIE_DOLPHIN_RIDER:
            case SEED_ZOMBIE_ZOMBLOB:
                if (aIsBalancePatch) {
                    mRefreshTime = 3500; // 20 -> 35
                }
                break;
            case SEED_BEGHOULED_BUTTON_SHUFFLE:
            case SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE:
                mRefreshTime = 0;
                mRefreshing = false;
                mActive = true;
                break;
            default:
                break;
        }

        bool aIsProducer = Challenge::IsMPResourceProducer(mPacketType);
        bool aIsShuffle = mPacketType == SEED_BEGHOULED_BUTTON_SHUFFLE || mPacketType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE;
        if (Challenge::msVSShuffleMode) {
            if (!aIsProducer && !aIsShuffle) {
                mRefreshTime = 0;
                mRefreshing = false;
                mActive = true;
            }
        }
    }
}

void SeedPacket::SetNextRandomSeed() {
    // 仅刷牌模式生效
    if (!Challenge::msVSShuffleMode)
        return;

    // 不更换生产者和刷牌按钮
    if (Challenge::IsMPResourceProducer(mPacketType) || mPacketType == SEED_BEGHOULED_BUTTON_SHUFFLE || mPacketType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE)
        return;

    if (IsRemoteClientOrViewer())
        return;

    SeedType seedType = SeedType::SEED_NONE;
    std::vector<SeedType> plantSeeds, zombieSeeds;
    seedType = PickNextRandomSeed(mApp, plantSeeds, zombieSeeds, mSeedBank->mIsZombie, mIndex);
    SetPacketType(seedType, SeedType::SEED_NONE);

    if (IsRemoteServer()) {
        U8U8U16U16_Event event{};
        event.type = EventType::EVENT_SERVER_BOARD_SHUFFLE_RANDOM_PICK_NEXT;
        event.data1 = mSeedBank->mIsZombie;
        event.data3 = seedType;
        event.data4 = mIndex;
        netplay::PutEvent(event);
    }
}

void DrawSeedType(Sexy::Graphics *g, float x, float y, SeedType theSeedType, SeedType theImitaterType, float theOffsetX, float theOffsetY, float theScale) {
    // 和Plant::DrawSeedType配合使用，用于绘制卡槽内的模仿者SeedPacket变白效果。
    g->PushState();
    g->mScaleX = g->mScaleX * theScale;
    g->mScaleY = g->mScaleY * theScale;
    if (theSeedType == SeedType::SEED_ZOMBIE_GRAVESTONE) {
        TodDrawImageCelScaledF(g, Sexy::IMAGE_MP_TOMBSTONE, x + theOffsetX, y + theOffsetY, 0, 0, g->mScaleX, g->mScaleY);
    } else if (theSeedType == SeedType::SEED_ZOMBIE_MOUND) {
        int aCelCol = 2;
        Board *board = gLawnApp->mBoard;
        if (board) {
            GamepadControls *aGamepad = board->mGamepadControls[0]->mIsZombie ? board->mGamepadControls[0] : (board->mGamepadControls[1]->mIsZombie ? board->mGamepadControls[1] : nullptr);
            int aGridX = board->PixelToGridXKeepOnBoard(int(aGamepad->mCursorPositionX), int(aGamepad->mCursorPositionY));
            int aGridY = board->PixelToGridYKeepOnBoard(int(aGamepad->mCursorPositionX), int(aGamepad->mCursorPositionY));
            GridItem *aMound = board->GetMoundAt(aGridX, aGridY);
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
        }
        TodDrawImageCelScaledF(g, addonImages.seed_mounds, x + theOffsetX, y + theOffsetY - 5, aCelCol, 0, g->mScaleX, g->mScaleY);
    } else {
        if (theSeedType == SeedType::SEED_IMITATER && theImitaterType != SeedType::SEED_NONE) {
            // 卡槽内的模仿者SeedPacket卡且为冷却状态，此时需要交换theImitaterType和theSeedType。
            Plant::DrawSeedType(g, theImitaterType, theSeedType, DrawVariation::VARIATION_NORMAL, x + theOffsetX, y + theOffsetY);
        } else {
            Plant::DrawSeedType(g, theSeedType, theImitaterType, DrawVariation::VARIATION_NORMAL, x + theOffsetX, y + theOffsetY);
        }
    }
    return g->PopState();
}

void DrawSeedPacket(Sexy::Graphics *g,
                    float x,
                    float y,
                    SeedType theSeedType,
                    SeedType theImitaterType,
                    float thePercentDark,
                    int theGrayness,
                    bool theDrawCost,
                    bool theUseCurrentCost,
                    bool theIsZombieSeed,
                    bool theIsPacketSelected) {
    // 修复选中紫卡、模仿者卡时卡片背景变为普通卡片背景

    SeedType realSeedType = theImitaterType != SeedType::SEED_NONE && theSeedType == SeedType::SEED_IMITATER ? theImitaterType : theSeedType;
    if (theGrayness != 255) {
        Color theColor = {theGrayness, theGrayness, theGrayness, 255};
        g->SetColor(theColor);
        g->SetColorizeImages(true);
    } else if (thePercentDark > 0.0f) {
        Color theColor = {128, 128, 128, 255};
        g->SetColor(theColor);
        g->SetColorizeImages(true);
    }
    int celToDraw = 0;
    if (theSeedType == SeedType::SEED_IMITATER) {
        celToDraw = 0;
    } else if (Plant::IsUpgrade(realSeedType)) {
        celToDraw = 1;
    } else if (theSeedType == SeedType::SEED_BEGHOULED_BUTTON_CRATER) {
        celToDraw = 3;
    } else if (theSeedType == SeedType::SEED_BEGHOULED_BUTTON_SHUFFLE || theSeedType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
        celToDraw = 4;
    } else if (theSeedType == SeedType::SEED_SLOT_MACHINE_SUN) {
        celToDraw = 5;
    } else if (theSeedType == SeedType::SEED_SLOT_MACHINE_DIAMOND) {
        celToDraw = 6;
    } else if (theSeedType == SeedType::SEED_ZOMBIQUARIUM_SNORKLE) {
        celToDraw = 7;
    } else if (theSeedType == SeedType::SEED_ZOMBIQUARIUM_TROPHY) {
        celToDraw = 8;
    } else {
        celToDraw = 2;
    }

    if (theIsPacketSelected) {
        if (g->mScaleX > 1.0f && theSeedType < SeedType::NUM_SEEDS_IN_CHOOSER) {
            // 紫卡背景BUG就是在这里修复的
            if (celToDraw == 2) {
                TodDrawImageCelScaledF(g, Sexy::IMAGE_SEEDPACKET_LARGER, x, y, 0, 0, g->mScaleX * 0.5f, g->mScaleY * 0.5f);
            } else {
                TodDrawImageCelScaledF(g, Sexy::IMAGE_SEEDS, x, y, celToDraw, 0, g->mScaleX, g->mScaleY);
            }
        } else if (theIsZombieSeed && theSeedType != SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
            float heightOffset = g->mScaleX > 1.2 ? -1.5f : 0.0f;
            TodDrawImageScaledF(g, Sexy::IMAGE_ZOMBIE_SEEDPACKET, x, y + heightOffset, g->mScaleX, g->mScaleY);
            if (theSeedType == SeedType::SEED_ZOMBIE_MOUND) {
                TodDrawImageScaledF(g, addonImages.seedpacket_Zombie_Upgrade, x, y + heightOffset, g->mScaleX, g->mScaleY);
            }
        } else {
            TodDrawImageCelScaledF(g, Sexy::IMAGE_SEEDS, x, y, celToDraw, 0, g->mScaleX, g->mScaleY);
        }
    }
    bool isPlant = theSeedType < SeedType::SEED_BEGHOULED_BUTTON_SHUFFLE || theSeedType > SeedType::SEED_ZOMBIQUARIUM_TROPHY;
    float offsetY = NAN, offsetX = NAN, theDrawScale = NAN;
    switch (realSeedType) {
        case SeedType::SEED_TALLNUT:
            offsetY = 22.0f;
            offsetX = 12.0f;
            theDrawScale = 0.3f;
            break;
        case SeedType::SEED_INSTANT_COFFEE:
            offsetY = 9.0f;
            offsetX = 0.0f;
            theDrawScale = 0.55f;
            break;
        case SeedType::SEED_COBCANNON:
            offsetY = 22.0f;
            offsetX = 6.0f;
            theDrawScale = 0.26f;
            break;
        case SeedType::SEED_CACTUS:
            offsetY = 13.0f;
            offsetX = 9.0f;
            theDrawScale = 0.5f;
            break;
        case SeedType::SEED_MAGNETSHROOM:
            offsetY = 12.0f;
            offsetX = 5.0f;
            theDrawScale = 0.5f;
            break;
        case SeedType::SEED_TWINSUNFLOWER:
        case SeedType::SEED_GLOOMSHROOM:
            offsetY = 14.0f;
            offsetX = 7.0f;
            theDrawScale = 0.45f;
            break;
        case SeedType::SEED_CATTAIL:
            offsetY = 13.0f;
            offsetX = 8.0f;
            theDrawScale = 0.45f;
            break;
        case SeedType::SEED_UMBRELLA:
            offsetY = 10.0f;
            offsetX = 5.0f;
            theDrawScale = 0.5f;
            break;
        case SeedType::SEED_KERNELPULT:
            offsetY = 14.0f;
            offsetX = 13.0f;
            theDrawScale = 0.4f;
            break;
        case SeedType::SEED_CABBAGEPULT:
            offsetY = 14.0f;
            offsetX = 15.0f;
            theDrawScale = 0.4f;
            break;
        case SeedType::SEED_GRAVEBUSTER:
            offsetY = 15.0f;
            offsetX = 10.0f;
            theDrawScale = 0.4f;
            break;
        case SeedType::SEED_SPLITPEA:
            offsetY = 12.0f;
            offsetX = 12.0f;
            theDrawScale = 0.45f;
            break;
        case SeedType::SEED_BLOVER:
        case SeedType::SEED_BONK_CHOY:
            offsetY = 17.0f;
            offsetX = 8.0f;
            theDrawScale = 0.4f;
            break;
        case SeedType::SEED_STARFRUIT:
            offsetY = 8.0f;
            offsetX = 6.0f;
            theDrawScale = 0.5f;
            break;
        case SeedType::SEED_THREEPEATER:
            offsetY = 10.0f;
            offsetX = 5.0f;
            theDrawScale = 0.5f;
            break;
        case SeedType::SEED_GATLINGPEA:
            offsetY = 8.0f;
            offsetX = 2.0f;
            theDrawScale = 0.5f;
            break;
        case SeedType::SEED_ZOMBIE_POLEVAULTER:
        case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
            offsetY = -12.0f;
            offsetX = -8.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_WINTERMELON:
            offsetY = 19.0f;
            offsetX = 18.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_POTATOMINE:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_TANGLEKELP:
        case SeedType::SEED_PUMPKINSHELL:
        case SeedType::SEED_CHOMPER:
        case SeedType::SEED_DOOMSHROOM:
        case SeedType::SEED_SQUASH:
        case SeedType::SEED_HYPNOSHROOM:
        case SeedType::SEED_SPIKEWEED:
        case SeedType::SEED_SPIKEROCK:
        case SeedType::SEED_PLANTERN:
        case SeedType::SEED_TORCHWOOD:
        case SeedType::SEED_IMP_PEAR:
        case SeedType::SEED_CELERY_STALKER:
        case SeedType::SEED_SPORESHROOM:
        case SeedType::SEED_SWEET_POTATO:
        case SeedType::SEED_SUN_BEAN:
        case SeedType::SEED_PEANUT:
        case SeedType::SEED_ENDURIAN:
        case SeedType::SEED_RED_STINGER:
            offsetY = 12.0f;
            offsetX = 8.0f;
            theDrawScale = 0.4f;
            break;
        case SeedType::SEED_ZOMBIE_NORMAL:
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
        case SeedType::SEED_ZOMBIE_FLAG:
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
        case SeedType::SEED_ZOMBIE_PAIL:
        case SeedType::SEED_ZOMBIE_DANCER:
        case SeedType::SEED_ZOMBIE_JACKSON:
        case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
        case SeedType::SEED_ZOMBIE_ZOMBLOB:
            offsetY = -7.0f;
            offsetX = -3.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_ZOMBIE_SKELETON:
            offsetY = -10.0f;
            offsetX = -3.0f;
            theDrawScale = 0.37f;
            break;
        case SeedType::SEED_ZOMBIE_EXPLORER:
        case SeedType::SEED_ZOMBIE_DOGWALKER:
        case SeedType::SEED_ZOMBIE_TELEPORTATION:
        case SeedType::SEED_ZOMBIE_SCIENTIST:
        case SeedType::SEED_ZOMBIE_GRAVEDIGGER:
            offsetY = -7.0f;
            offsetX = 3.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_ZOMBIE_LADDER:
        case SeedType::SEED_ZOMBIE_DIGGER:
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
        case SeedType::SEED_ZOMBIE_TRASHCAN:
        case SeedType::SEED_ZOMBIE_POGO:
        case SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX:
        case SeedType::SEED_ZOMBIE_CROSSING_GUARD:
            offsetY = -10.0f;
            offsetX = -3.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_ZOMBIE_DOLPHIN_RIDER:
            offsetY = -12.0f;
            offsetX = -3.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_ZOMBIE_SNORKEL:
            offsetY = -8.0f;
            offsetX = -3.0f;
            theDrawScale = 0.32f;
            break;
        case SeedType::SEED_ZOMBIE_BUNGEE:
            offsetY = -1.0f;
            offsetX = 1.0f;
            theDrawScale = 0.3f;
            break;
        case SeedType::SEED_ZOMBIE_FOOTBALL:
        case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
            offsetY = -9.0f;
            offsetX = -7.0f;
            theDrawScale = 0.33f;
            break;
        case SeedType::SEED_ZOMBIE_BALLOON:
            offsetY = -5.0f;
            offsetX = -3.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_ZOMBIE_IMP:
        case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
            offsetY = -17.0f;
            offsetX = -12.0f;
            theDrawScale = 0.4f;
            break;
        case SeedType::SEED_ZOMBONI:
            offsetY = 3.0f;
            offsetX = -5.0f;
            theDrawScale = 0.23f;
            break;
        case SeedType::SEED_ZOMBIE_CATAPULT:
            offsetY = 3.0f;
            offsetX = 1.0f;
            theDrawScale = 0.23f;
            break;
        case SeedType::SEED_ZOMBIE_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_REDEYE_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_SUPER_NOVA_GARGANTUAR:
            offsetY = 3.0f;
            offsetX = 4.0f;
            theDrawScale = 0.23f;
            break;
        case SeedType::SEED_ZOMBIE_YETI:
            offsetY = -7.0f;
            offsetX = 1.0f;
            theDrawScale = 0.32f;
            break;
        case SeedType::SEED_ZOMBIE_BOSS:
            offsetY = 5.0f;
            offsetX = 1.0f;
            theDrawScale = 0.18f;
            break;
        case SeedType::SEED_ZOMBIE_PEA_HEAD:
        case SeedType::SEED_ZOMBIE_WALLNUT_HEAD:
        case SeedType::SEED_ZOMBIE_JALAPENO_HEAD:
        case SeedType::SEED_ZOMBIE_GATLINGPEA_HEAD:
        case SeedType::SEED_ZOMBIE_SQUASH_HEAD:
        case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
            offsetY = -7.0f;
            offsetX = -3.0f;
            theDrawScale = 0.35f;
            break;
        case SeedType::SEED_ZOMBIE_BOBSLED:
            offsetY = -10.0f;
            offsetX = -3.0f;
            theDrawScale = 0.35f;
            break;
        default:
            offsetY = 8.0f;
            offsetX = 5.0f;
            theDrawScale = 0.5f;
            break;
    }
    LawnApp *lawnApp = gLawnApp;
    float v28 = NAN, v29 = NAN;
    if (lawnApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_BIG_TIME) {
        if (realSeedType == SeedType::SEED_SUNFLOWER || realSeedType == SeedType::SEED_WALLNUT || realSeedType == SeedType::SEED_MARIGOLD) {
            offsetY = 34.0f;
            offsetX = 16.0f;
        }
    }
    v28 = offsetX * g->mScaleX;
    v29 = (offsetY + 1.0f) * g->mScaleY;
    if (realSeedType == SeedType::SEED_GIANT_WALLNUT) {
        v29 = 59.0f;
        theDrawScale = theDrawScale * 0.75f;
        v28 = 52.0f;
    }

    if (isPlant && theIsPacketSelected)
        DrawSeedType(g, x, y, realSeedType, theImitaterType, v28, v29, theDrawScale);

    if (thePercentDark > 0.0f) {
        float coolDownHeight = thePercentDark * 68.0f + 2.5f;
        Graphics aPlantG(*g);
        Color theColor = {64, 64, 64, 255};
        aPlantG.SetColor(theColor);
        aPlantG.SetColorizeImages(true);
        aPlantG.ClipRect(x, y, g->mScaleX * 50.0f, coolDownHeight * g->mScaleY);
        if (theIsPacketSelected) {
            if (Challenge::IsMPSeedType(theSeedType)) {
                TodDrawImageScaledF(&aPlantG, Sexy::IMAGE_ZOMBIE_SEEDPACKET, x, y, g->mScaleX, g->mScaleY);
                if (theSeedType == SeedType::SEED_ZOMBIE_MOUND) {
                    TodDrawImageScaledF(&aPlantG, addonImages.seedpacket_Zombie_Upgrade, x, y, g->mScaleX, g->mScaleY);
                }
            } else {
                TodDrawImageCelScaledF(&aPlantG, Sexy::IMAGE_SEEDS, x, y, celToDraw, 0, g->mScaleX, g->mScaleY);
            }
        }
        if (isPlant && theIsPacketSelected)
            DrawSeedType(&aPlantG, x, y, theSeedType, theImitaterType, v28, v29, theDrawScale);
    }
    if (theDrawCost) {
        pvzstl::string aCostStr;
        if (lawnApp->mBoard && lawnApp->mBoard->PlantUsesAcceleratedPricing(realSeedType)) {
            if (theUseCurrentCost) {
                aCostStr = pvzstl::to_string(lawnApp->mBoard->GetCurrentPlantCost(theSeedType, theImitaterType));
            } else {
                aCostStr = pvzstl::to_string(Plant::GetCost(theSeedType, theImitaterType)) + '+';
            }
        } else {
            aCostStr = pvzstl::to_string(Plant::GetCost(theSeedType, theImitaterType));
            if (realSeedType == SeedType::SEED_ZOMBIE_MOUND) {
                aCostStr = pvzstl::to_string(lawnApp->mBoard->GetCurrentPlantCost(theSeedType, theImitaterType));
            }
        }

        Sexy::Font *font = Sexy::FONT_BRIANNETOD12;

        int width = 31 - font->GetVTable()->StringWidth(font, aCostStr); // 33 -> 31，微调一下文字位置，左移2个像素点
        int height = 48 + font->GetVTable()->GetAscent(font);            // 50 -> 48, 微调一下文字位置，上移2个像素点
        Color theColor = {0, 0, 0, 255};
        g->PushState();
        if (g->mScaleX == 1.0f && g->mScaleY == 1.0f) {
            TodDrawString(g, aCostStr, width + x, height + y, font, theColor, DrawStringJustification::DS_ALIGN_LEFT);
        } else {
            SexyMatrix3 aMatrix{};
            TodScaleTransformMatrix(aMatrix, x + g->mTransX + width * g->mScaleX, y + g->mTransY + height * g->mScaleY - 1.0f, g->mScaleX, g->mScaleY);
            if (g->mScaleX > 1.8f) {
                g->SetLinearBlend(false);
            }
            TodDrawStringMatrix(g, font, aMatrix, aCostStr, theColor);
            g->SetLinearBlend(true);
        }
        g->PopState();
    }
    g->SetColorizeImages(false);
}

void SeedPacket::WasPlanted(int thePlayerIndex) {
    old_SeedPacket_WasPlanted(this, thePlayerIndex);

    if (mPacketType == SEED_BEGHOULED_BUTTON_SHUFFLE || mPacketType == SEED_ZOMBIE_BEGHOULED_BUTTON_SHUFFLE) {
        gFreeForFristShuffle[mSeedBank->mIsZombie] = false; // 首次免费在使用当下失效
    }

    if (IsRemoteServer()) {
        U8U8_Event event = {{EventType::EVENT_SERVER_BOARD_SEEDPACKET_WASPLANTED}, uint8_t(mIndex), mSeedBank == mBoard->mSeedBank[0]};
        netplay::PutEvent(event);
    }
}

void SeedPacket::SlotMachineStart() {
    mSlotMachiningPosition = 0.0f;
    mSlotMachineCountDown = 400;
    PickNextSlotMachineSeed();

    TriggerVibration(VibrationEffect::VIBRATION_SLOT_MACHINE);
}
