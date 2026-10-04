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

#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/MessageWidget.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Board/ZenGarden.h"
#include "PvZ/Lawn/Common/GameConstants.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/NetPlay.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <algorithm>

#include <cmath>

using namespace Sexy;

void Coin::CoinInitialize(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion) {
    old_Coin_CoinInitialize(this, theX, theY, theCoinType, theCoinMotion);

    if (mType == CoinType::COIN_SMALL_VS_ZOMBIE_BRAIN) {
        mScale = 0.5f;

        Reanimation *aReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_ZOMBIESUN);
        aReanim->SetPosition(mPosX + mWidth * 0.5f, mPosY + mHeight * 0.5f);
        aReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
        aReanim->SetAnimRate(6.0f);
        AttachReanim(mAttachmentID[0], aReanim, mWidth * 0.5f, mHeight * 0.5f);
    }
    if (mType == CoinType::COIN_LARGE_VS_ZOMBIE_BRAIN) {
        mScale = 2.0f;

        Reanimation *aReanim = mApp->AddReanimation(0.0f, 0.0f, 0, ReanimationType::REANIM_ZOMBIESUN);
        aReanim->SetPosition(mPosX + mWidth * 2.0f, mPosY + mHeight * 2.0f);
        aReanim->mLoopType = ReanimLoopType::REANIM_LOOP;
        aReanim->SetAnimRate(6.0f);
        AttachReanim(mAttachmentID[0], aReanim, mWidth * 2.0f, mHeight * 2.0f);
    }
    if (mType == CoinType::COIN_MINISUN) {
        mScale *= 0.25f;
    }
}

void Coin::Collect(int thePlayerIndex) {
    const bool aIsCoopSun = mApp->IsCoopMode() && (IsSun() || mType == CoinType::COIN_COOP_DOUBLE_SUN);
    if (aIsCoopSun && (mDead || mIsBeingCollected || IsRemoteClientOrViewer())) {
        // 结盟阳光的收集由主机确认，对战保留原来的自动采集逻辑。
        return;
    }

    old_Coin_Collect(this, thePlayerIndex);

    if (aIsCoopSun && IsRemoteServer() && mIsBeingCollected) {
        CoinCollectEvent aEvent{};
        aEvent.type = EventType::EVENT_SERVER_BOARD_COIN_COLLECT;
        aEvent.coinID = uint16_t(mBoard->mCoins.DataArrayGetID(this));
        aEvent.playerIndex = uint8_t(mCollectedByPlayerIndex);
        netplay::PutEvent(aEvent);
    }
}

void Coin::GamepadCursorOver(int thePlayerIndex) {
    if (mApp->IsCoopMode() && IsSun() && IsRemoteClientOrViewer()) {
        // 结盟客户端不能独立吸附阳光；收到主机收集事件后再播放收集动画。
        return;
    }
    //*((uint32_t *)a + 29) == 16 则意味着是砸罐子种子雨老虎机中的植物卡片

    if (!gKeyboardMode && mType == CoinType::COIN_USABLE_SEED_PACKET) {
        return;
    }


    //    old_Coin_GamepadCursorOver(this, thePlayerIndex);

    if (mBoard && !mBoard->mPaused && mApp->mGameScene == SCENE_PLAYING && !mDead) {
        if (mType == COIN_USABLE_SEED_PACKET) {
            Collect(thePlayerIndex);
        } else if (mType != COIN_COOP_DOUBLE_SUN && !mIsBeingCollected && mCoinMotion != COIN_MOTION_FROM_NEAR_CURSOR) {
            if (!IsSun() || mScale >= GetSunScale()) {
                mPlayerIndex = thePlayerIndex;
                mCoinMotion = COIN_MOTION_FROM_NEAR_CURSOR;
                unk2 = 0.0;
                if (IsSun())
                    mScale = GetSunScale();
                if (mApp->IsFirstTimeAdventureMode()) {
                    if (mBoard->mLevel == 1) {
                        pvzstl::string str = TodStringTranslate("[ADVICE_CLICKED_ON_SUN]");
                        mBoard->DisplayAdvice(str, MESSAGE_STYLE_TUTORIAL_LEVEL1_STAY, ADVICE_CLICKED_ON_SUN);
                    }
                }
            }
        }
    }
}

void Coin::Update() {
    if (BanDropCoin && !IsOnlineServerModeActive() && !gIsReplayMode && (IsMoney() || IsSun() || mType == CoinType::COIN_COOP_DOUBLE_SUN || IsDeath())) {
        // 开启了"禁止掉落阳光金币"时
        Die();
        return;
    }

    if (mType == CoinType::COIN_VS_PLANT_TROPHY || mType == CoinType::COIN_VS_ZOMBIE_TROPHY) {
        old_Coin_Update(this);
        return;
    }

    // 观战/回放强制自动收集，不受“手动收集”选项影响。
    const bool forceAutoCollectSun = gIsServerModeSpectator || gIsReplayMode;
    // 联机模式强制自动采集
    if (enableManualCollect && !IsOnlineModeActive() && !forceAutoCollectSun) {
        // 如果开了手动拾取，则重置Coin的存在时间计数器为0，从而不会触发自动拾取。
        GameMode aGameMode = mApp->mGameMode;
        // 在重型武器中、花园中依然自动收集；在关卡结束后依然自动收集。
        if (aGameMode != GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON && aGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && aGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM
            && mApp->mGameScene == GameScenes::SCENE_PLAYING && mBoard->mBoardFadeOutCounter <= 0) {
            mCoinAge = 0;
        }
    } else if (IsDeath()) {
        // 如果没有关闭自动拾取，则为对战模式的僵尸方阳光也加入自动拾取。
        if (mCoinAge > 79 && !mIsBeingCollected) {
            Collect(0);
        }
    }
    if (mApp->IsCoopMode() && (IsSun() || mType == CoinType::COIN_COOP_DOUBLE_SUN)) {
        // 在结盟模式关闭阳光自动拾取。
        mCoinAge = 0;
    }

    if (IsLevelAward()) {
        // 为关卡结束后的奖励掉落物关闭自动拾取。
        mCoinAge = 0;
    }

    old_Coin_Update(this);
}

void Coin::PlayCollectSound() {
    if (mType == CoinType::COIN_USABLE_SEED_PACKET) {
        mApp->PlaySample(SOUND_SEEDLIFT);
        return;
    }

    if (mType == CoinType::COIN_SILVER || mType == CoinType::COIN_GOLD) {
        mApp->PlayFoley(FoleyType::FOLEY_COIN);
        return;
    }

    if (mType == CoinType::COIN_DIAMOND) {
        mApp->PlaySample(SOUND_DIAMOND);
        return;
    }

    if (mType == CoinType::COIN_CHOCOLATE || mType == CoinType::COIN_PRESENT_PLANT || IsPresentWithAdvice() || mType == CoinType::COIN_AWARD_PRESENT || mType == CoinType::COIN_AWARD_CHOCOLATE) {
        mApp->PlayFoley(FoleyType::FOLEY_PRIZE);
        return;
    }

    if (IsSun() || mType == CoinType::COIN_COOP_DOUBLE_SUN) {
        mApp->PlayFoley(FoleyType::FOLEY_SUN);
        return;
    }

    if (IsDeath()) {
        mApp->PlayFoley(FoleyType::FOLEY_SLURP);
    }
}

void Coin::FanOutCoins(CoinType theCoinType, int theNumCoins) {
    if (IsRemoteClientOrViewer()) {
        return;
    }
    old_Coin_FanOutCoins(this, theCoinType, theNumCoins);
}

void Coin::ScoreCoin() {
    Die();

    if (IsSun()) {
        mBoard->AddSunMoney(GetSunValue(), mCollectedByPlayerIndex);
    }

    if (mType == CoinType::COIN_COOP_DOUBLE_SUN) {
        int aSunValue = GetSunValue();
        mBoard->AddSunMoney(aSunValue, 0);
        mBoard->AddSunMoney(aSunValue, 1);
    }

    if (IsDeath()) {
        mBoard->AddDeathMoney(GetSunValue());
    } else if (IsMoney()) {
        int aCoinValue = Coin::GetCoinValue(mType);
        if (mApp->mPlayerInfo) {
            mApp->mPlayerInfo->AddCoins(aCoinValue);
        }
        if (mBoard) {
            mBoard->mCoinsCollected += aCoinValue;
        }
    }

    if (mType == CoinType::COIN_DIAMOND && mBoard) {
        ++mBoard->mDiamondsCollected;
    }
}

void Coin::UpdateCollected() {
    const auto *platformData = static_cast<const std::uint8_t *>(mApp->mPlatformDriverOrQueue);

    int aDoubleSunDestX = *reinterpret_cast<const bool *>(platformData + 0x60) ? 600 : 480;
    int aDestX = 15;
    int aDestY = 0;

    if (IsSun()) {
        if (mCollectedByPlayerIndex == 1) {
            aDestX = aDoubleSunDestX;
            aDoubleSunDestX = 0;
        } else {
            aDoubleSunDestX = 0;
        }
    } else if (mType == CoinType::COIN_COOP_DOUBLE_SUN) {
        aDestX = 15;
        aDestY = 0;
    } else if (IsDeath()) {
        aDestX = 760;
        aDestY = 0;
        aDoubleSunDestX = 0;
    } else if (IsMoney()) {
        aDestX = 39;
        aDestY = 558;
        aDoubleSunDestX = 0;

        if (mApp->GetDialog(Dialogs::DIALOG_STORE)) {
            aDestX = 662;
            aDestY = 546;
        } else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
            aDestX = 442;
        } else if (mApp->mCrazyDaveState != CrazyDaveState::CRAZY_DAVE_OFF) {
            aDestX = 442;
        }
    } else if (IsPresentWithAdvice()) {
        aDestX = 35;
        aDestY = 487;
        aDoubleSunDestX = 0;
    } else if (mType == CoinType::COIN_AWARD_PRESENT || mType == CoinType::COIN_PRESENT_PLANT) {
        ++mDisappearCounter;
        if (mDisappearCounter >= 200) {
            StartFade();
        }
        return;
    } else if (!IsLevelAward()) {
        if (mType == CoinType::COIN_USABLE_SEED_PACKET) {
            ++mDisappearCounter;
        }
        return;
    } else {
        aDestX = 400 - mWidth / 2;
        aDestY = 200 - mHeight / 2;
        ++mDisappearCounter;
    }

    if (IsLevelAward()) {
        mScale = TodAnimateCurveFloat(0, 400, mDisappearCounter, 1.01f, 2.0f, TodCurves::CURVE_EASE_IN_OUT);
        mPosX = TodAnimateCurveFloat(0, 350, mDisappearCounter, mCollectX, aDestX, TodCurves::CURVE_EASE_OUT);
        mPosY = TodAnimateCurveFloat(0, 350, mDisappearCounter, mCollectY, aDestY, TodCurves::CURVE_EASE_OUT);
        return;
    }
    float aDeltaX = fabsf(mPosX - aDestX);
    float aDeltaY = fabsf(mPosY - aDestY);
    if (mPosX > aDestX) {
        mPosX -= aDeltaX * 0.047619f;
    } else if (mPosX < aDestX) {
        mPosX += aDeltaX * 0.047619f;
    }
    if (mPosY > aDestY) {
        mPosY -= aDeltaY * 0.047619f;
    } else if (mPosY < aDestY) {
        mPosY += aDeltaY * 0.047619f;
    }

    if (mType == CoinType::COIN_COOP_DOUBLE_SUN) {
        float aDoubleSunDeltaX = fabsf(mPrevPosX - aDoubleSunDestX);
        float aDoubleSunDeltaY = fabsf(mPrevPosY);
        if (mPrevPosX > aDoubleSunDestX) {
            mPrevPosX -= aDoubleSunDeltaX * 0.047619f;
        } else if (mPrevPosX < aDoubleSunDestX) {
            mPrevPosX += aDoubleSunDeltaX * 0.047619f;
        }
        if (mPrevPosY > 0.0f) {
            mPrevPosY -= aDoubleSunDeltaY * 0.047619f;
        } else if (mPrevPosY < 0.0f) {
            mPrevPosY += aDoubleSunDeltaY * 0.047619f;
        }
    }

    mCollectionDistance = sqrtf(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
    if (IsPresentWithAdvice()) {
        if (mCollectionDistance >= 15.0f) {
            return;
        }

        if (mBoard->mHelpDisplayed[ADVICE_NEED_ACHIVEMENT_EARNED]) {
            if (mBoard->mHelpIndex != ADVICE_NEED_ACHIVEMENT_EARNED || mBoard->mAdvice == nullptr || mBoard->mAdvice->mDuration <= 0 /* IsBeingDisplayed() */) {
                Die();
            }
            return;
        }

        switch (mType) {
            case CoinType::COIN_PRESENT_MINIGAMES:
                mBoard->DisplayAdvice("[UNLOCKED_MINIGAMES]", MessageStyle::MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE, AdviceType::ADVICE_NEED_ACHIVEMENT_EARNED);
                break;

            case CoinType::Present32:
                mBoard->DisplayAdvice("[UNLOCKED_PUZZLE_MODE]", MessageStyle::MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE, AdviceType::ADVICE_NEED_ACHIVEMENT_EARNED);
                break;

            case CoinType::Present1024:
                mBoard->DisplayAdvice("[UNLOCKED_SURVIVAL_MODE]", MessageStyle::MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE, AdviceType::ADVICE_NEED_ACHIVEMENT_EARNED);
                break;

            case CoinType::COIN_PRESENT_PUZZLE_MODE:
                mBoard->DisplayAdvice("[UNLOCKED_COOP_CHALLENGES]", MessageStyle::MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE, AdviceType::ADVICE_NEED_ACHIVEMENT_EARNED);
                break;

            default:
                break;
        }
        return;
    }

    float aScoringDistance = 8.0f;
    if (IsMoney()) {
        aScoringDistance = 12.0f;
    }

    if (mCollectionDistance < aScoringDistance) {
        ScoreCoin();
    }

    mScale = ClampFloat(mCollectionDistance * 0.05f, 0.5f, 1.0f);
    mScale *= GetSunScale();
}

void Coin::UpdateFallForAward() {
    // 试图去除关卡掉落物在关卡结束后的自动收集。

    if (unk3)
        return;

    if (mCoinMotion == CoinMotion::COIN_MOTION_FROM_PRESENT) {
        mPosX += mVelX;
        mPosY += mVelY;
        mVelX *= 0.95f;
        mVelY *= 0.95f;
        if (mCoinAge >= 80) {
            Collect(0);
        }
    } else if (mCoinMotion == CoinMotion::COIN_MOTION_FROM_FROM_VS_WON) {
        float v30 = mVelY + 0.2f;
        float v34 = v30 + mPosY;
        mPosY = v34;
        mVelY = v30 * 0.95f;
        mVelX *= 0.95f;
        mPosX += mVelX;
        if (v34 >= mGroundY) {
            if (mVelY * mVelY + mVelX * mVelX > 0.5f * 0.5f) {
                mApp->PlayFoley(FoleyType::FOLEY_GRASSSTEP);
                mVelY *= -1;
            } else {
                mPosY = mGroundY;
                mVelY = 0.0f;
            }
        }
        if (mCoinAge >= 200) {
            Collect(0);
        }
    } else if (mCoinMotion == CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR) {
        if (mPlayerIndex >= 0) {
            GamepadControls *gamepadControls = mBoard->GetGamepadControlsByPlayerIndex(mPlayerIndex);
            float v55 = gamepadControls->mCursorPositionX;
            float v56 = gamepadControls->mCursorPositionY;
            float v40 = (float)mWidth / 2;
            v56 = v56 - ((float)mHeight / 2);
            float v41 = v56 - mPosY;
            v55 = v55 - v40;
            float v42 = v55 - mPosX;
            if ((v41 * v41 + v42 * v42) < 1225.0) {
                Collect(mPlayerIndex);
                return;
            }
            float v43 = v41 * 400.0f;
            float v44 = v42 * 400.0f;
            float v45 = sqrtf(v43 * v43 + v44 * v44);
            float v46 = unk2 + ((6.4f / (v45 / 100.0f)) * (v45 / 100.0f));
            if (v46 > 600.0f)
                v46 = 600.0f;
            unk2 = v46;
            if (v45 != 0.0f) {
                v43 = v43 / v45;
                v44 = v44 / v45;
            }
            mPosX += (unk2 * v44) * 0.016f;
            mPosY += (unk2 * v43) * 0.016f;
        }
    } else if (mPosY + mVelY < mGroundY) {
        mPosY += mVelY;
        if (mCoinMotion == CoinMotion::COIN_MOTION_FROM_PLANT || mCoinMotion == CoinMotion::COIN_MOTION_FROM_GRAVE_STONE) {
            mVelY += 0.09;
        } else if (mCoinMotion == CoinMotion::COIN_MOTION_COIN || mCoinMotion == CoinMotion::COIN_MOTION_FROM_BOSS) {
            mVelY += 0.15;
        }

        mPosX += mVelX;
        if (mPosX > BOARD_WIDTH - mWidth && mCoinMotion != CoinMotion::COIN_MOTION_FROM_BOSS) {
            mPosX = BOARD_WIDTH - mWidth;
            // mVelX = -0.4f - RandRangeFloat(0.0f, 0.4f);
        } else if (mPosX < 0.0f) {
            mPosX = 0.0f;
            // mVelX = 0.4f + RandRangeFloat(0.0f, 0.4f);
        }
    } else {
        if (mNeedsBouncyArrow && !mHasBouncyArrow) {
            float aParticleOffsetX = mWidth / 2.0f;
            float aParticleOffsetY = mHeight / 2.0f - 60.0f;
            if (mType == CoinType::COIN_TROPHY) {
                aParticleOffsetX += 2.0;
            } else if (mType == CoinType::COIN_VS_PLANT_TROPHY || mType == CoinType::COIN_VS_ZOMBIE_TROPHY) {
                aParticleOffsetY -= 20.0;
                int aRenderOrder = Board::MakeRenderOrder(RenderLayer::RENDER_LAYER_UI_TOP, mRow, mHasBouncyArrow);
                TodParticleSystem *aParticle = mApp->AddTodParticle(mPosX, mPosY, aRenderOrder, ParticleEffect::PARTICLE_TROPHY_SPARKLE);
                AttachParticle(*(mAttachmentID + 2), aParticle, 0, 0.0);
            } else if (mType == CoinType::COIN_AWARD_MONEY_BAG || mType == CoinType::COIN_AWARD_BAG_DIAMOND) {
                aParticleOffsetY -= 2.0;
                aParticleOffsetX += 2.0;
            } else if (mType == CoinType::COIN_AWARD_PRESENT || IsPresentWithAdvice()) {
                aParticleOffsetY -= 20.0;
            } else if (mType == CoinType::COIN_AWARD_SILVER_SUNFLOWER || mType == CoinType::COIN_AWARD_GOLD_SUNFLOWER) {
                aParticleOffsetX -= 6.0f;
                aParticleOffsetY -= 40.0f;
            } else if (IsMoney()) {
                aParticleOffsetX += 12.0f;
                aParticleOffsetY += 21.0f;
            }

            ParticleEffect aEffect;
            if (mType == CoinType::COIN_FINAL_SEED_PACKET) {
                aEffect = ParticleEffect::PARTICLE_SEED_PACKET;
            } else if (IsMoney()) {
                aEffect = ParticleEffect::PARTICLE_COIN_PICKUP_ARROW;
            } else {
                aEffect = ParticleEffect::PARTICLE_AWARD_PICKUP_ARROW;
            }

            TodParticleSystem *aParticle = mApp->AddTodParticle(mPosX + aParticleOffsetX, mPosY + aParticleOffsetY, 0, aEffect);
            AttachParticle(*mAttachmentID, aParticle, aParticleOffsetX, aParticleOffsetY);
            mHasBouncyArrow = true;
        }

        if (!mHitGround) {
            mHitGround = true;
            PlayGroundSound();
        }

        mPosY = mGroundY;
        mPosX = std::round(mPosX);


        if (mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_LAST_STAND || mBoard == nullptr || mBoard->mChallenge->mChallengeState == ChallengeState::STATECHALLENGE_LAST_STAND_ONSLAUGHT) {
            if (!IsLevelAward() && !IsPresentWithAdvice()) {
                ++mDisappearCounter;
                if (mDisappearCounter >= GetDisappearTime()) {
                    StartFade();
                }
            }
        }
    }

    if (mCoinMotion == CoinMotion::COIN_MOTION_FROM_PLANT || mCoinMotion == CoinMotion::COIN_MOTION_FROM_GRAVE_STONE) {
        float aFinalScale = GetSunScale();
        if (mScale < aFinalScale) {
            mScale += 0.02f;
        } else {
            mScale = aFinalScale;
        }
    }
}

void Coin::UpdateFall() {
    if (mType == CoinType::COIN_MINISUN && mCoinAge >= mAutoCollectAge) {
        Collect(0);
        return;
    }

    // 去除关卡掉落物在关卡结束后的自动收集。
    if ((mType >= CoinType::COIN_AWARD_MONEY_BAG && mType <= CoinType::COIN_AWARD_GOLD_SUNFLOWER) || mType == CoinType::COIN_FINAL_SEED_PACKET) {
        UpdateFallForAward();
        return;
    }

    old_Coin_UpdateFall(this);
}

bool Coin::MouseHitTest(int theX, int theY, int **theHitResult, int thePlayerIndex) {
    // 去除在玩家按A键时的阳光金币检测，以防止玩家种植、铲除、发射加农炮时的操作被阳光金币遮挡。
    if (IsMoney() || IsSun() || mType == CoinType::COIN_COOP_DOUBLE_SUN || IsDeath()) {
        return false;
    }

    return old_Coin_MouseHitTest(this, theX, theY, theHitResult, thePlayerIndex);
}

bool Coin::IsSun() const {
    return mType == CoinType::COIN_SUN || mType == CoinType::COIN_SMALLSUN || mType == CoinType::COIN_LARGESUN || mType == CoinType::COIN_MINISUN;
}

bool Coin::IsDeath() const {
    return mType == CoinType::COIN_VS_ZOMBIE_BRAIN || mType == CoinType::COIN_SMALL_VS_ZOMBIE_BRAIN || mType == CoinType::COIN_LARGE_VS_ZOMBIE_BRAIN;
}

int Coin::GetSunValue() {
    if (mCustomSunValue != 0) {
        return mCustomSunValue;
    }

    switch (mType) {
        case CoinType::COIN_MINISUN:
            return 5;
        case CoinType::COIN_SMALLSUN:
        case CoinType::COIN_SMALL_VS_ZOMBIE_BRAIN:
            return 15;
        case CoinType::COIN_SUN:
        case CoinType::COIN_COOP_DOUBLE_SUN:
        case CoinType::COIN_VS_ZOMBIE_BRAIN:
            return 25;
        case CoinType::COIN_LARGESUN:
        case CoinType::COIN_LARGE_VS_ZOMBIE_BRAIN:
            return 50;
        default:
            return mCustomSunValue;
    }
}

float Coin::GetSunScale() {
    switch (mType) {
        case CoinType::COIN_MINISUN:
            return 0.25f;
        case CoinType::COIN_SMALLSUN:
        case CoinType::COIN_SMALL_VS_ZOMBIE_BRAIN:
            return 0.5f;
        case CoinType::COIN_LARGESUN:
        case CoinType::COIN_LARGE_VS_ZOMBIE_BRAIN:
            return 2.0f;
        default:
            return 1.0f;
    }
}

void Coin::Draw(Graphics *g) {
    g->SetColor(GetColor());

    if (mType == CoinType::COIN_DIAMOND) {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, static_cast<int>(mPosX - 56.0f), static_cast<int>(mPosY - 66.0f));
        g->SetColorizeImages(false);
    }
    if (mType == CoinType::COIN_PRESENT_PLANT) {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, static_cast<int>(mPosX - 50.0f), static_cast<int>(mPosY - 64.0f));
        g->SetColorizeImages(false);
    }
    if (mType == CoinType::COIN_AWARD_PRESENT && mIsBeingCollected) {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, static_cast<int>(mPosX - 50.0f), static_cast<int>(mPosY - 64.0f));
        g->SetColorizeImages(false);
    }
    if (mType == CoinType::COIN_CHOCOLATE || mType == CoinType::COIN_AWARD_CHOCOLATE) {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, static_cast<int>(mPosX - 56.0f), static_cast<int>(mPosY - 50.0f));
        g->SetColorizeImages(false);
    }

    auto aDrawHighlight = [this, g](Image *theImage, float theCenterX, float theCenterY) {
        const float aScale = std::sin(static_cast<float>(mCoinAge) * 0.02f) * 0.2f + 0.9f;
        const float aOldScaleX = g->mScaleX;
        const float aOldScaleY = g->mScaleY;
        const float aOldScaleOrigX = g->mScaleOrigX;
        const float aOldScaleOrigY = g->mScaleOrigY;
        g->SetScale(aScale, aScale, theCenterX, theCenterY);
        g->DrawImage(theImage, static_cast<int>(theCenterX - 65.0f), static_cast<int>(theCenterY - 65.0f));
        g->mScaleX = aOldScaleX;
        g->mScaleY = aOldScaleY;
        g->mScaleOrigX = aOldScaleOrigX;
        g->mScaleOrigY = aOldScaleOrigY;
    };
    if (unk7) {
        aDrawHighlight(IMAGE_COOP_SUN_HIGHLIGHT_1, mPosX + mWidth * 0.5f, mPosY + mHeight * 0.5f);
    }
    if (unk8) {
        aDrawHighlight(IMAGE_COOP_SUN_HIGHLIGHT_2, mPosX + 30.0f + mWidth * 0.5f, mPosY + 6.0f + mHeight * 0.5f);
    }

    auto aDrawAttachment = [this, g](AttachmentID theAttachmentID) {
        if (theAttachmentID == AttachmentID::ATTACHMENTID_NULL) {
            return;
        }
        g->PushState();
        MakeParentGraphicsFrame(g);
        AttachmentDraw(theAttachmentID, g, false);
        g->PopState();
    };
    aDrawAttachment(mAttachmentID[0]);
    aDrawAttachment(mAttachmentID[1]);

    if ((mType == CoinType::COIN_SILVER || mType == CoinType::COIN_GOLD) && mHitGround && !mIsBeingCollected) {
        return;
    }
    if (mType == CoinType::COIN_DIAMOND) {
        return;
    }

    if (IsLevelAward() && !mIsBeingCollected) {
        g->SetColor(GetFlashingColor(mCoinAge, 75));
    }

    if (mType == CoinType::COIN_SILVER || mType == CoinType::COIN_GOLD) {
        g->SetColorizeImages(true);
        TodDrawImageCenterScaledF(g, IMAGE_REANIM_COINGLOW, mPosX - 14.0f, mPosY - 12.0f, mScale, mScale);
        g->SetColorizeImages(false);
    }

    Image *aImage = nullptr;
    int aImageCelCol = 0;
    float aDrawScale = mScale;
    float aOffsetX = 0.0f;
    float aOffsetY = 0.0f;

    if (mType == CoinType::COIN_SILVER) {
        aImage = IMAGE_REANIM_COIN_SILVER_DOLLAR;
        aOffsetX = 8.0f;
        aOffsetY = 10.0f;
    } else if (mType == CoinType::COIN_GOLD) {
        aImage = IMAGE_REANIM_COIN_GOLD_DOLLAR;
        aOffsetX = 8.0f;
        aOffsetY = 10.0f;
    } else if (IsSun() || mType == CoinType::COIN_COOP_DOUBLE_SUN || IsDeath()) {
        return;
    } else if (mType == CoinType::COIN_FINAL_SEED_PACKET) {
        const SeedType aSeedType = GetFinalSeedPacketType();
        g->SetScale(mScale, mScale, 0.0f, 0.0f);
        DrawSeedPacket(g, 0.5f * (mWidth - mScale * mWidth) + mPosX, 0.5f * (mHeight - mScale * mHeight) + mPosY, aSeedType, SeedType::SEED_NONE, 0.0f, 255, true, false, false, true);
        g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
        return;
    } else if (mType == CoinType::COIN_PRESENT_PLANT || mType == CoinType::COIN_AWARD_PRESENT) {
        if (mIsBeingCollected) {
            mApp->mZenGarden->DrawPottedPlantIcon(g, mPosX + 10.0f, mPosY - 20.0f, &mPottedPlantSpec);
            return;
        }
        aImage = IMAGE_PRESENT;
        aOffsetY = -20.0f;
    } else if (IsPresentWithAdvice()) {
        if (mIsBeingCollected) {
            aImage = IMAGE_PRESENTOPEN;
            aOffsetX = -10.0f;
            aOffsetY = -10.0f;
        } else {
            aImage = IMAGE_PRESENT;
            aOffsetY = -20.0f;
        }
    } else if (mType == CoinType::COIN_AWARD_MONEY_BAG || mType == CoinType::COIN_AWARD_BAG_DIAMOND) {
        aImage = IMAGE_MONEYBAG_HI_RES;
        aOffsetX = -mWidth * 0.5f;
        aOffsetY = -mHeight * 0.5f;
        aDrawScale *= 0.5f;
    } else if (mType == CoinType::COIN_CHOCOLATE || mType == CoinType::COIN_AWARD_CHOCOLATE) {
        aImage = IMAGE_CHOCOLATE;
    } else if (mType == CoinType::COIN_TROPHY) {
        aImage = IMAGE_TROPHY_HI_RES;
        aOffsetX = -mWidth * 0.5f;
        aOffsetY = -mHeight * 0.5f;
        aDrawScale *= 0.5f;
    } else if (mType == CoinType::COIN_AWARD_SILVER_SUNFLOWER) {
        aImage = IMAGE_SUNFLOWER_TROPHY;
        aOffsetY = -5.0f;
        aDrawScale *= 0.6f;
    } else if (mType == CoinType::COIN_AWARD_GOLD_SUNFLOWER) {
        aImage = IMAGE_SUNFLOWER_TROPHY;
        aImageCelCol = 1;
        aOffsetY = -5.0f;
        aDrawScale *= 0.6f;
    } else if (mType == CoinType::COIN_SHOVEL) {
        aImage = IMAGE_SHOVEL_HI_RES;
        aOffsetX = -20.0f;
        aOffsetY = -20.0f;
        aDrawScale *= 0.5f;
    } else if (mType == CoinType::COIN_CARKEYS) {
        aImage = IMAGE_CARKEYS;
    } else if (mType == CoinType::COIN_ALMANAC) {
        aImage = IMAGE_ALMANAC;
    } else if (mType == CoinType::COIN_TACO) {
        aImage = IMAGE_TACO;
    } else if (mType == CoinType::COIN_VASE) {
        aImage = IMAGE_SCARY_POT;
    } else if (mType == CoinType::COIN_WATERING_CAN) {
        aImage = IMAGE_WATERINGCAN;
    } else if (mType == CoinType::COIN_NOTE) {
        aImage = IMAGE_ZOMBIE_NOTE_SMALL;
    } else if (mType == CoinType::COIN_VS_PLANT_TROPHY || mType == CoinType::COIN_VS_ZOMBIE_TROPHY) {
        aImage = mType == CoinType::COIN_VS_PLANT_TROPHY ? IMAGE_MP_PLANT_TROPHY_HI_RES : IMAGE_MP_ZOMBIE_TROPHY_HI_RES;
        aOffsetX = -mWidth * 0.5f;
        aOffsetY = -mHeight * 0.5f;
        aDrawScale *= 0.5f;
    } else if (mType == CoinType::COIN_USABLE_SEED_PACKET) {
        int aGrayness = 255;
        if (mIsBeingCollected) {
            aGrayness = 128;
        } else {
            const int aDisappearTime = GetDisappearTime();
            if (mDisappearCounter > aDisappearTime - 300 && mDisappearCounter % 60 < 30) {
                aGrayness = 192;
            }
        }

        g->SetColorizeImages(true);
        DrawSeedPacket(g, static_cast<int>(mPosX), static_cast<int>(mPosY), mUsableSeedType, SeedType::SEED_NONE, 0.0f, aGrayness, false, false, false, true);
        g->SetColorizeImages(false);
        return;
    }

    if (aImage == nullptr) {
        return;
    }

    if ((mType == CoinType::COIN_VS_PLANT_TROPHY || mType == CoinType::COIN_VS_ZOMBIE_TROPHY) && !mIsBeingCollected) {
        float aShadowScale = (mPosY - static_cast<float>(mGroundY)) * 0.01f + 1.0f;
        aShadowScale = std::clamp(aShadowScale, 0.2f, 1.0f);
        TodDrawImageCelCenterScaledF(g, IMAGE_PLANTSHADOW, mPosX + 18.0f, static_cast<float>(mGroundY + 96), 0, aShadowScale, aShadowScale);
    }

    g->SetColorizeImages(true);
    TodDrawImageCelCenterScaledF(g, aImage, mPosX + aOffsetX, mPosY + aOffsetY, aImageCelCol, aDrawScale, aDrawScale);
    g->SetColorizeImages(false);

    aDrawAttachment(mAttachmentID[2]);
}

Color Coin::GetColor() {
    if ((IsSun() || IsDeath() || IsMoney()) && mIsBeingCollected) {
        int aAlpha = ClampFloat(mCollectionDistance * 0.035f, 0.35f, 1.0f) * 255.0f;
        return {255, 255, 255, aAlpha};
    }

    if (mFadeCount > 0) {
        int aAlpha = TodAnimateCurve(15, 0, mFadeCount, 255, 0, TodCurves::CURVE_LINEAR);
        return {255, 255, 255, aAlpha};
    }

    return Color::White;
}
