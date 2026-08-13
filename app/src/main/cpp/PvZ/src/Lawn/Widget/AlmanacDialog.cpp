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

#include "PvZ/Lawn/Widget/AlmanacDialog.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/PoolEffect.h"
#include "PvZ/Lawn/System/ReanimationLawn.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstddef>

#include <algorithm>
#include <numbers>

using namespace Sexy;

namespace {

GameButton *gAlmanacBackButton;
GameButton *gAlmanacCloseButton;

int gAlmanacDialogTouchDownY;
bool gTouchDownInTextRect;
Rect ALMANAC_RECT_TEXT = Rect(482, 360, 258, 173);

} // namespace

void AlmanacDialog::_constructor(LawnApp *theApp) {
    // TODO: 解决部分植物的介绍文本显示不全问题

    old_AlmanacDialog_AlmanacDialog(this, theApp);

    gAlmanacBackButton = MakeButton(ALMANAC_BUTTON_BACK, this, this, "[ALMANAC_INDEX]");
    gAlmanacBackButton->Resize(0, 0, 0, 0);
    gAlmanacBackButton->mBtnNoDraw = true;
    gAlmanacBackButton->mDisabled = true;

    gAlmanacCloseButton = MakeButton(ALMANAC_BUTTON_CLOSE, this, this, "[CLOSE]");
    gAlmanacCloseButton->Resize(ALMANAC_BUTTON_CLOSE_X, ALMANAC_BUTTON_CLOSE_Y, ALMANAC_BUTTON_WIDTH, ALMANAC_BUTTON_HEIGHT);


    // 为泳池背景加入PoolEffect。这里挖空背景图，挖出一块透明方形
    Sexy::Image *gPlantBackImage = Sexy::IMAGE_ALMANAC_PLANTBACK;
    Sexy::Image *gPoolBackImage = Sexy::IMAGE_ALMANAC_GROUNDNIGHTPOOL;
    Sexy::Rect aBlankRect = Rect(ALMANAC_RECT_PLANT_X + 240, ALMANAC_RECT_PLANT_Y + 60, gPoolBackImage->mWidth, gPoolBackImage->mHeight);
    static_cast<MemoryImage *>(gPlantBackImage)->ClearRect(aBlankRect);
}

void AlmanacDialog::_destructor() {
    old_AlmanacDialog_Delete2(this);

    gAlmanacBackButton->~GameButton();
    gAlmanacCloseButton->~GameButton();
    gAlmanacBackButton = nullptr;
    gAlmanacCloseButton = nullptr;
}

void AlmanacDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager) {
    old_AlmanacDialog_AddedToManager(this, theWidgetManager);

    AddWidget(gAlmanacBackButton);
    AddWidget(gAlmanacCloseButton);
}

void AlmanacDialog::RemovedFromManager(WidgetManager *theWidgetManager) {
    RemoveWidget(gAlmanacCloseButton);
    RemoveWidget(gAlmanacBackButton);

    old_AlmanacDialog_RemovedFromManager(this, theWidgetManager);
}

void AlmanacDialog::SetPage(AlmanacPage thePage) {
    // 修复点击气球僵尸进植物图鉴、点击介绍文字进植物图鉴
    if (thePage != AlmanacPage::ALMANAC_PAGE_INDEX) {
        // 在前往其他图鉴页面时，显示返回按钮
        if (gAlmanacBackButton != nullptr) {
            gAlmanacBackButton->Resize(ALMANAC_BUTTON_BACK_X, ALMANAC_BUTTON_BACK_Y, ALMANAC_BUTTON_WIDTH, ALMANAC_BUTTON_HEIGHT);
            gAlmanacBackButton->mBtnNoDraw = false;
            gAlmanacBackButton->mDisabled = false;
        }
        // 在前往其他图鉴页面时，将按钮缩小为0x0
        mPlantButton->Resize(0, 0, 0, 0);
        mZombieButton->Resize(0, 0, 0, 0);
    } else {
        // 回到图鉴首页时，将返回按钮禁用
        if (gAlmanacBackButton != nullptr) {
            gAlmanacBackButton->Resize(0, 0, 0, 0);
            gAlmanacBackButton->mBtnNoDraw = true;
            gAlmanacBackButton->mDisabled = true;
        }
        // 回到图鉴首页时，将按钮恢复为正常大小
        mPlantButton->Resize(130, 345, 156, 42);
        mZombieButton->Resize(487, 345, 210, 48);
    }

    old_AlmanacDialog_SetPage(this, thePage);
}

void AlmanacDialog::MouseDown(int x, int y, int theClickCount) {
    // 修复点击气球僵尸进植物图鉴、点击介绍文字进植物图鉴
    if (mOpenPage == 0) {
        // 如果当前的Page是Index Page
        if (mPlantButton->IsMouseOver())
            mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        if (mZombieButton->IsMouseOver())
            mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        return;
    } else if (ALMANAC_RECT_TEXT.Contains(x, y)) {
        gTouchDownInTextRect = true;
        gAlmanacDialogTouchDownY = y;
    }

    SeedType aSeedType = SeedHitTest(x, y);
    if (aSeedType != SeedType::SEED_NONE && aSeedType != mSelectedSeed) {
        mSelectedSeed = aSeedType;
        SetupPlant();
        mApp->PlaySample(Sexy::SOUND_TAP);
    }
    ZombieType aZombieType = ZombieHitTest(x, y);
    if (aZombieType != -1 && aZombieType != mSelectedZombie) {
        mSelectedZombie = aZombieType;
        SetupZombie();
        mApp->PlaySample(Sexy::SOUND_TAP);
    }
}

void AlmanacDialog::MouseDrag(int x, int y) {
    // 滚动图鉴文字

    if (gTouchDownInTextRect && gAlmanacDialogTouchDownY != y) {
        // 触摸拖拽标志 (this+932, unk4[2]): GameMain AlmanacDialog::Update 读它,
        // 非 0 时进入输入处理块 (摇杆/触摸位移 SetValue(±1)), 处理完清零;
        // AxisMoved 摇杆位移≥0.2 时置 1, 回中清 0
        *(unsigned char *)&unk4[2] = 1;
        // 拖拽产生的滚动值钳制到 [0, mMaxValue], 防止拖出边界后 mValue 越界
        double aMaxValue = mScrollTextView->mMaxValue > 0.0 ? mScrollTextView->mMaxValue : 100.0;
        double aNewValue = std::clamp(mScrollTextView->mValue + 0.6 * (gAlmanacDialogTouchDownY - y), 0.0, aMaxValue);
        (*(void (**)(Sexy::Widget *, uint32_t, double))(*(uint32_t *)mScrollTextView + 500))((Widget *)mScrollTextView, *(uint32_t *)(*(uint32_t *)mScrollTextView + 500), aNewValue);
        gAlmanacDialogTouchDownY = y;
    }
}

void AlmanacDialog::MouseUp(int x, int y, int theClickCount) {
    // 空函数替换，修复点击图鉴Index界面中任何位置都会跳转植物图鉴的问题
    gTouchDownInTextRect = false;
    *(unsigned char *)&unk4[2] = 0;
}

void AlmanacDialog::Update() {
    // 取消 TV 原版的 Description 自动滚动 (Update 中 SetValue(mValue+1))
    // 无触摸拖拽(unk4[2]=0)时恢复 mValue, 同时保留手动滚动
    double aSavedValue = mScrollTextView != nullptr ? mScrollTextView->mValue : 0.0;
    old_AlmanacDialog_Update(this);
    if (mScrollTextView != nullptr && !*(unsigned char *)&unk4[2]) {
        mScrollTextView->mValue = aSavedValue;
    }
}

void AlmanacDialog::ButtonDepress(int theId) {
    if (theId == 0) {
        SetPage(AlmanacPage::ALMANAC_PAGE_PLANTS);
    } else if (theId == 1) {
        SetPage(AlmanacPage::ALMANAC_PAGE_ZOMBIES);
    } else if (theId == ALMANAC_BUTTON_BACK) {
        KeyDown(KeyCode::KEYCODE_ESCAPE);
    } else if (theId == ALMANAC_BUTTON_CLOSE) {
        mApp->KillAlmanacDialog();
    }
}

void AlmanacDialog::DrawPlants_Unmodified(Sexy::Graphics *g) {
    // old_AlmanacDialog_DrawPlants(almanacDialog,g);

    // TODO:解决PoolEffect图层问题，和部分植物的介绍文本显示不全问题
    g->DrawImage(Sexy::IMAGE_ALMANAC_PLANTBACK, -240, -60);
    Color aHeaderColor = {213, 159, 43, 255};
    TodDrawString(g, "[SUBURBAN_ALMANAC_PLANTS]", 400, 50, Sexy::FONT_HOUSEOFTERROR20, aHeaderColor, DrawStringJustification::DS_ALIGN_CENTER);
    int theAlpha = std::sin((mUpdateCnt % 100) * 0.01f * std::numbers::pi) * 255.0f;
    int x = 0, y = 0;
    for (SeedType aSeedType = SeedType::SEED_PEASHOOTER; aSeedType < SeedType::NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1)) {
        GetSeedPosition(aSeedType, x, y);
        if (aSeedType == SeedType::SEED_IMITATER) {
            bool tmp = g->GetColorizeImages();
            g->SetColorizeImages(true);
            if (mSelectedSeed == SeedType::SEED_IMITATER) {
                Color v39 = {255, 255, 0, theAlpha};
                g->SetColor(v39);
            } else {
                Color v39 = {255, 255, 255, 64};
                g->SetColor(v39);
            }
            g->DrawImage(Sexy::IMAGE_ALMANAC_IMITATER, 18, 20);
            g->SetColor(gColorWhite);
            g->SetColorizeImages(tmp);
        } else {
            if (mSelectedSeed == aSeedType) {
                g->SetScale(1.1, 1.1, x, y);
                DrawSeedPacket(g, x - 2, y - 4, mSelectedSeed, SeedType::SEED_NONE, 0.0, 255, true, false, false, true);
                bool tmp = g->GetColorizeImages();
                g->SetColorizeImages(true);
                Color v39 = {255, 255, 0, theAlpha};
                g->SetColor(v39);
                g->DrawImage(Sexy::IMAGE_SEEDPACKETFLASH, x - 3, y - 5);
                g->SetColor(gColorWhite);
                g->SetColorizeImages(tmp);
                g->SetScale(1.0, 1.0, 0.0, 0.0);
            } else {
                DrawSeedPacket(g, x, y, (SeedType)aSeedType, SeedType::SEED_NONE, 0.0, 255, true, false, false, true);
            }
        }
    }

    if (Plant::IsAquatic(mSelectedSeed)) {
        if (Plant::IsNocturnal(mSelectedSeed)) {
            g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDNIGHTPOOL, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
            if (mApp->Is3DAccelerated()) {
                g->SetClipRect(475, 0, 397, 500);
                g->mTransY = g->mTransY - 145.0f;
                mApp->mPoolEffect->PoolEffectDraw(g, true);
                g->mTransY = g->mTransY + 145.0f;
                g->ClearClipRect();
            }
        } else {
            g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDPOOL, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
            if (mApp->Is3DAccelerated()) {
                g->SetClipRect(475, 0, 397, 500);
                g->mTransY = g->mTransY - 145.0f;
                mApp->mPoolEffect->PoolEffectDraw(g, false);
                g->mTransY = g->mTransY + 145.0f;
                g->ClearClipRect();
            }
        }
    } else if (Plant::IsNocturnal(mSelectedSeed) || mSelectedSeed == SeedType::SEED_GRAVEBUSTER || mSelectedSeed == SeedType::SEED_PLANTERN) {
        g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDNIGHT, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
    } else if (mSelectedSeed == SeedType::SEED_FLOWERPOT) {
        g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDROOF, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
    } else {
        g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDDAY, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
    }


    if (mPlant != nullptr) {
        g->PushState();
        g->mTransX = g->mTransX + mPlant->mX;
        g->mTransY = g->mTransY + mPlant->mY;
        mPlant->Draw(g);
        g->PopState();
    }

    g->DrawImage(Sexy::IMAGE_ALMANAC_PLANTCARD, 459, 80);

    Color color = {213, 159, 43, 255};
    TodDrawString(g, *mNameString, 617, 108, Sexy::FONT_DWARVENTODCRAFT18, color, DrawStringJustification::DS_ALIGN_CENTER);

    if (mSelectedSeed != SeedType::SEED_IMITATER) {
        TodDrawStringWrapped(g, *mCostString, mCostRect, Sexy::FONT_BRIANNETOD16, gColorWhite, DrawStringJustification::DS_ALIGN_LEFT, false);
        TodDrawStringWrapped(g, *mWaitTimeString, mWaitTimeRect, Sexy::FONT_BRIANNETOD16, gColorWhite, DrawStringJustification::DS_ALIGN_RIGHT, false);
    }

    g->PushState();
    g->ClipRect(mDescriptionRect.mX, mDescriptionRect.mY - 14, mDescriptionRect.mWidth, mDescriptionRect.mHeight + 8);
    float v22 = mScrollTextView->mValue * 0.01f * mDescriptionRect.mY;
    float v23 = g->mTransY + 2.0f - v22;
    *(float *)unk2 = -v22;
    g->mTransY = v23;
    Color v39 = {143, 67, 27, 255};
    TodDrawStringWrappedHelper(g, *mDescriptionString, mDescriptionRect, Sexy::FONT_BRIANNETOD16, v39, DrawStringJustification::DS_ALIGN_LEFT, true, true);
    g->PopState();
}

void AlmanacDialog::DrawPlants(Sexy::Graphics *g) {
    // return old_AlmanacDialog_DrawPlants(almanacDialog,g);
    // 为泳池背景加入PoolEffect。此函数改变了原版绘制顺序，将背景图放在泳池的后面绘制

    // 加入和 DrawZombies 对称的防护，虽然之前植物图鉴无 bug。但这样更保险。
    // 滚动条 mValue 钳制到 [0, mMaxValue]: 滚动偏移 = mValue*0.01*rectY, 防越界值导致描述移出屏幕
    if (mScrollTextView != nullptr) {
        double aMaxValue = mScrollTextView->mMaxValue > 0.0 ? mScrollTextView->mMaxValue : 100.0;
        mScrollTextView->mValue = std::clamp(mScrollTextView->mValue, 0.0, aMaxValue);
    }
    // 布局完成但描述为空，则重新布局生成文本
    if (mSetupFinished && mDescriptionString->empty()) {
        mSetupFinished = false;
        SetupLayoutPlants(g);
    }

    if (Plant::IsAquatic(mSelectedSeed)) {
        if (Plant::IsNocturnal(mSelectedSeed)) {
            g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDNIGHTPOOL, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y + 10);
            if (mApp->Is3DAccelerated()) {
                // Sexy_Graphics_SetClipRect(g, 475, 0, 397, 500);
                g->mTransY = g->mTransY - 115;
                mApp->mPoolEffect->PoolEffectDraw(g, true);
                g->mTransY = g->mTransY + 115;
                // Sexy_Graphics_ClearClipRect(g);
            }
        } else {
            g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDPOOL, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y + 10);
            if (mApp->Is3DAccelerated()) {
                // Sexy_Graphics_SetClipRect(g, 475, 0, 397, 500);
                g->mTransY = g->mTransY - 115;
                mApp->mPoolEffect->PoolEffectDraw(g, false);
                g->mTransY = g->mTransY + 115;
                // Sexy_Graphics_ClearClipRect(g);
            }
        }
    } else if (Plant::IsNocturnal(mSelectedSeed) || mSelectedSeed == SeedType::SEED_GRAVEBUSTER || mSelectedSeed == SeedType::SEED_PLANTERN) {
        g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDNIGHT, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
    } else if (mSelectedSeed == SeedType::SEED_FLOWERPOT) {
        g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDROOF, ALMANAC_RECT_PLANT_X + 10, ALMANAC_RECT_PLANT_Y + 12);
    } else {
        g->DrawImage(Sexy::IMAGE_ALMANAC_GROUNDDAY, ALMANAC_RECT_PLANT_X, ALMANAC_RECT_PLANT_Y);
    }

    g->DrawImage(Sexy::IMAGE_ALMANAC_PLANTBACK, -240, -60);
    TodDrawString(g, "[SUBURBAN_ALMANAC_PLANTS]", BOARD_WIDTH / 2, 50, Sexy::FONT_HOUSEOFTERROR20, Color(213, 159, 43), DrawStringJustification::DS_ALIGN_CENTER);

    int aAlpha = sin((mUpdateCnt % 100) * 0.01 * std::numbers::pi) * 255.0;
    for (SeedType aSeedType = SeedType::SEED_PEASHOOTER; aSeedType < NUM_ALMANAC_SEEDS; aSeedType = (SeedType)(aSeedType + 1)) {
        int aPosX = 0, aPosY = 0;
        GetSeedPosition(aSeedType, aPosX, aPosY);
        if (mApp->HasSeedType(aSeedType, false)) {
            if (aSeedType == SeedType::SEED_IMITATER) {
                g->SetColorizeImages(true);
                if (mSelectedSeed == SeedType::SEED_IMITATER) {
                    g->SetColor(Color(255, 255, 0, aAlpha));
                } else {
                    g->SetColor(Color(255, 255, 255, 64));
                }
                g->DrawImage(Sexy::IMAGE_ALMANAC_IMITATER, 18, 20);
                g->SetColor(gColorWhite);
                g->SetColorizeImages(g->GetColorizeImages());
            } else {
                if (mSelectedSeed == aSeedType) {
                    g->SetScale(1.1, 1.1, aPosX, aPosY);
                    DrawSeedPacket(g, aPosX - 2, aPosY - 4, mSelectedSeed, SeedType::SEED_NONE, 0.0, 255, true, false, false, true);
                    g->SetColorizeImages(true);
                    g->SetColor(Color(255, 255, 0, aAlpha));
                    g->DrawImage(Sexy::IMAGE_SEEDPACKETFLASH, aPosX - 3, aPosY - 5);
                    g->SetColor(gColorWhite);
                    g->SetColorizeImages(g->GetColorizeImages());
                    g->SetScale(1.0, 1.0, 0.0, 0.0);
                } else {
                    DrawSeedPacket(g, aPosX, aPosY, aSeedType, SeedType::SEED_NONE, 0.0, 255, true, false, false, true);
                }
            }
        }
    }

    if (mPlant) {
        g->PushState();
        g->mTransX = g->mTransX + mPlant->mX;
        g->mTransY = g->mTransY + mPlant->mY;
        mPlant->Draw(g);
        g->PopState();
    }

    g->DrawImage(Sexy::IMAGE_ALMANAC_PLANTCARD, 459, 80);
    TodDrawString(g, *mNameString, 617, 108, Sexy::FONT_DWARVENTODCRAFT18, Color(213, 159, 43, 255), DrawStringJustification::DS_ALIGN_CENTER);

    if (mSelectedSeed != SeedType::SEED_IMITATER) {
        TodDrawStringWrapped(g, *mCostString, mCostRect, Sexy::FONT_BRIANNETOD16, gColorWhite, DrawStringJustification::DS_ALIGN_LEFT, false);
        TodDrawStringWrapped(g, *mWaitTimeString, mWaitTimeRect, Sexy::FONT_BRIANNETOD16, gColorWhite, DrawStringJustification::DS_ALIGN_RIGHT, false);
    }

    g->PushState();
    g->ClipRect(mDescriptionRect.mX, mDescriptionRect.mY - 14, mDescriptionRect.mWidth, mDescriptionRect.mHeight + 8);
    float v22 = mScrollTextView->mValue * 0.01f * mDescriptionRect.mY;
    float v23 = g->mTransY + 2.0f - v22;
    *(float *)unk2 = -v22;
    g->mTransY = v23;
    unk2[1] = TodDrawStringWrappedHelper(g, *mDescriptionString, mDescriptionRect, Sexy::FONT_BRIANNETOD16, Color(143, 67, 27, 255), DrawStringJustification::DS_ALIGN_LEFT, true, true);
    g->PopState();
}

bool AlmanacDialog::ZombieHasSilhouette(ZombieType theZombieType) const {
    // 除雪人僵尸以外的其他僵尸，或者雪人僵尸已经可以刷出（已经到达或完成冒险模式二周目 4-10 关卡），则不会显示为剪影
    if (theZombieType != ZombieType::ZOMBIE_YETI || mApp->CanSpawnYetis())
        return false;

    // 排除上述情况后，若已完成雪人僵尸出现的关卡（冒险模式一周目 4-10 关卡），则雪人僵尸显示为剪影
    return mApp->HasFinishedAdventure() || mApp->mPlayerInfo->GetLevel() > GetZombieDefinition(ZombieType::ZOMBIE_YETI).mStartingLevel;
}

bool AlmanacDialog::ZombieIsShown(ZombieType theZombieType) {
    if (theZombieType < ZombieType::ZOMBIE_NORMAL || theZombieType > ZombieType::ZOMBIE_BOSS) {
        return false;
    }

    // 试玩模式下，仅展示潜水僵尸及其之前出现的僵尸
    if (mApp->IsTrialStageLocked() && theZombieType > ZombieType::ZOMBIE_SNORKEL)
        return false;

    // 对于雪人僵尸，要求其可以在刷怪中出现（已经到达或完成冒险模式二周目 4-10 关卡），
    // 或已得知其存在但未解锁其形象（已经完成冒险模式一周目 4-10 关卡，但未到达二周目 4-10 关卡）
    if (theZombieType == ZombieType::ZOMBIE_YETI)
        return mApp->CanSpawnYetis() || ZombieHasSilhouette(ZombieType::ZOMBIE_YETI);

    // 对于冒险模式中出现的僵尸
    if (theZombieType <= ZombieType::ZOMBIE_BOSS) {
        // 冒险模式一周目完成后，图鉴展示所有僵尸
        if (mApp->HasFinishedAdventure())
            return true;

        int aLevel = mApp->mPlayerInfo->GetLevel();
        int aStart = GetZombieDefinition(theZombieType).mStartingLevel;
        // 要求已经达到僵尸首次出现的关卡
        // 对于不能通过自然刷怪出现的僵尸（小鬼僵尸、雪橇僵尸小队、伴舞僵尸），额外要求已通过其首次出现的关卡或已击败过该僵尸
        return aStart <= aLevel && (aStart != aLevel || !Board::IsZombieTypeSpawnedOnly(theZombieType) || gZombieDefeated[theZombieType]);
    }

    return false;
}

void AlmanacDialog::DrawZombies(Graphics *g) {
    // 滚动条 mValue 钳制到 [0, mMaxValue]: 滚动偏移 = mValue*0.01*rectY, 防越界值导致描述移出屏幕
    if (mScrollTextView != nullptr) {
        double aMaxValue = mScrollTextView->mMaxValue > 0.0 ? mScrollTextView->mMaxValue : 100.0;
        mScrollTextView->mValue = std::clamp(mScrollTextView->mValue, 0.0, aMaxValue);
    }
    // 布局完成但描述为空，则重新布局生成文本
    if (mSetupFinished && mDescriptionString->empty()) {
        mSetupFinished = false;
        SetupLayoutZombies(g);
    }

    g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEBACK, LawnApp::FULLSCREEN_RECT.mX, -60);
    int aHeaderOffsetY = Sexy::FONT_DWARVENTODCRAFT24->GetHeight() / 2;
    TodDrawString(g, "[SUBURBAN_ALMANAC_ZOMBIES]", BOARD_WIDTH / 2, aHeaderOffsetY + 42, Sexy::FONT_DWARVENTODCRAFT24, Color(0, 196, 0), DS_ALIGN_CENTER);

    ZombieType aSelectedZombie = mSelectedZombie;
    for (int i = 0; i < NUM_ALMANAC_ZOMBIES; ++i) {
        ZombieType aZombieType = GetZombieType(i);
        int aPosX = 0, aPosY = 0;
        GetZombiePosition(aZombieType, aPosX, aPosY);
        if (aZombieType == ZombieType::ZOMBIE_INVALID) {
            continue;
        }

        if (!ZombieIsShown(aZombieType)) {
            g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEBLANK, aPosX, aPosY);
            continue;
        }

        if (aSelectedZombie == aZombieType) {
            g->mTransX -= 4.0f;
            g->mTransY -= 4.0f;
            g->SetScale(1.1, 1.1, (float)aPosX, (float)aPosY);
        }

        g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEWINDOW, aPosX, aPosY);
        g->PushState();
        g->SetClipRect(aPosX + 2, aPosY + 2, 72, 72);

        float transX = g->mTransX;
        float transY = g->mTransY;
        g->mScaleX = 0.5f;
        g->mScaleY = 0.5f;
        g->mTransX = transX + (float)(aPosX + 1);
        g->mTransY = transY + (float)(aPosY - 6);

        ZombieType aDrawZombie = aZombieType;
        switch (aZombieType) {
            case ZombieType::ZOMBIE_POLEVAULTER:
                aDrawZombie = ZombieType::ZOMBIE_CACHED_POLEVAULTER_WITH_POLE;
                g->mTransX += 2.0f;
                g->mTransY -= 3.0f;
                break;
            case ZombieType::ZOMBIE_FLAG:
                g->mTransX += 2.0f;
                g->mTransY += 10.0f;
                break;
            case ZombieType::ZOMBIE_TRAFFIC_CONE:
                g->mTransY += 12.0f;
                break;
            case ZombieType::ZOMBIE_PAIL:
                g->mTransY += 9.0f;
                break;
            case ZombieType::ZOMBIE_FOOTBALL:
                g->mTransX -= 15.0f;
                g->mTransY -= 1.0f;
                break;
            case ZombieType::ZOMBIE_ZAMBONI:
                g->mTransX -= 54.0f;
                g->mTransY += 3.0f;
                break;
            case ZombieType::ZOMBIE_DOLPHIN_RIDER:
                g->mTransX -= 2.0f;
                g->mTransY -= 10.0f;
                break;
            case ZombieType::ZOMBIE_POGO:
                g->mTransY -= 3.0f;
                break;
            case ZombieType::ZOMBIE_DANCER:
                g->mTransY += 15.0f;
                break;
            case ZombieType::ZOMBIE_BACKUP_DANCER:
                g->mTransX -= 5.0f;
                g->mTransY += 17.0f;
                break;
            case ZombieType::ZOMBIE_GARGANTUAR:
                g->mTransX += 15.0f;
                g->mTransY += 17.0f;
                break;
            case ZombieType::ZOMBIE_IMP:
                g->mTransX -= 8.0f;
                g->mTransY -= 7.0f;
                break;
            case ZombieType::ZOMBIE_BUNGEE:
                g->mTransX -= 4.0f;
                g->mTransY += 3.0f;
                break;
            case ZombieType::ZOMBIE_SNORKEL:
                g->mTransX -= 10.0f;
                break;
            case ZombieType::ZOMBIE_YETI:
                g->mTransY += 4.0f;
                break;
            case ZombieType::ZOMBIE_CATAPULT:
                g->mTransX -= 24.0f;
                g->mTransY -= 1.0f;
                break;
            case ZombieType::ZOMBIE_BOBSLED:
                g->mTransY -= 8.0f;
                break;
            case ZombieType::ZOMBIE_LADDER:
                g->mTransY -= 3.0f;
                break;
            default:
                break;
        }

        if (ZombieHasSilhouette(aZombieType)) {
            g->SetColor(Color(0, 0, 0, 64));
            g->SetColorizeImages(true);
        }

        if (aSelectedZombie != aZombieType) {
            mApp->mReanimatorCache->DrawCachedZombie(g, 0.0f, 0.0f, aDrawZombie);
            g->PopState();
            g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEWINDOW2, aPosX, aPosY);
            continue;
        } else {
            g->mTransX -= 4.0f;
            g->mTransY -= 4.0f;
            mApp->mReanimatorCache->DrawCachedZombie(g, 0.0f, 0.0f, aDrawZombie);
            g->PopState();
            g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
            g->SetColor(Color(255, 255, 255, 100));
            g->SetColorizeImages(true);
            g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEWINDOW, aPosX, aPosY);
            g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
            g->SetColorizeImages(false);

            g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEWINDOW2, aPosX, aPosY);

            g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
            g->SetColor(Color(255, 255, 255, 48));
            g->SetColorizeImages(true);
            g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIEWINDOW2, aPosX, aPosY);
            g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
            g->SetColorizeImages(false);

            g->mTransX += 4.0f;
            g->mTransY += 4.0f;
            g->SetScale(1.0, 1.0, 0.0, 0.0);
        }
    }

    Image *aGround = Sexy::IMAGE_ALMANAC_GROUNDICE;
    if (mZombie == nullptr || (mZombie->mZombieType != ZombieType::ZOMBIE_ZAMBONI && mZombie->mZombieType != ZombieType::ZOMBIE_BOBSLED)) {
        aGround = Sexy::IMAGE_ALMANAC_GROUNDDAY;
    }
    g->DrawImage(aGround, 518, 110);

    if (mZombie && !ZombieHasSilhouette(mZombie->mZombieType)) {
        g->PushState();
        g->mTransX += (float)mZombie->mX;
        g->mTransY += (float)mZombie->mY;
        g->SetClipRect(-42, -51, 197, 187);
        bool drawShadow = false;
        switch (mZombie->mZombieType) {
            case ZombieType::ZOMBIE_ZAMBONI:
                g->mTransX -= 30.0f;
                g->mTransY += 5.0f;
                break;
            case ZombieType::ZOMBIE_GARGANTUAR:
                g->mTransY += 30.0f;
                break;
            case ZombieType::ZOMBIE_FOOTBALL:
                g->mTransX -= 10.0f;
                drawShadow = true;
                break;
            case ZombieType::ZOMBIE_BALLOON:
                g->mTransY -= 20.0f;
                drawShadow = true;
                break;
            case ZombieType::ZOMBIE_BUNGEE:
                g->mTransX += 15.0f;
                break;
            case ZombieType::ZOMBIE_CATAPULT:
                g->mTransX -= 10.0f;
                break;
            case ZombieType::ZOMBIE_BOSS:
                g->mTransX -= 540.0f;
                g->mTransY -= 175.0f;
                break;
            default:
                if (mZombie->mZombieType != ZombieType::ZOMBIE_DANCER && mZombie->mZombieType != ZombieType::ZOMBIE_BACKUP_DANCER) {
                    drawShadow = true;
                }
                break;
        }
        if (drawShadow) {
            mZombie->DrawShadow(g);
        }
        mZombie->Draw(g);
        g->PopState();
    }

    g->DrawImage(Sexy::IMAGE_ALMANAC_ZOMBIECARD, 455, 78);
    TodDrawString(g, *mNameString, 613, 112, Sexy::FONT_DWARVENTODCRAFT18, Color(213, 159, 43, 255), DrawStringJustification::DS_ALIGN_CENTER);
    for (TodStringListFormat &aFormat : gLawnStringFormats) {
        if (TestBit(aFormat.mFormatFlags, TodStringFormatFlag::TOD_FORMAT_HIDE_UNTIL_MAGNETSHROOM)) {
            if (mApp->HasSeedType(SeedType::SEED_MAGNETSHROOM, false)) {
                aFormat.mNewColor.mAlpha = 255;
                aFormat.mLineSpacingOffset = 0;
            } else {
                aFormat.mNewColor.mAlpha = 0;
                aFormat.mLineSpacingOffset = -17;
            }
        }
    }

    g->PushState();
    g->ClipRect(mDescriptionRect.mX, mDescriptionRect.mY, mDescriptionRect.mWidth, mDescriptionRect.mHeight);
    float scrollY = mScrollTextView->mValue * 0.01f * unk2[1];
    g->mTransY -= scrollY;
    *(float *)unk2 = -scrollY;
    unk2[1] = TodDrawStringWrappedHelper(g, *mDescriptionString, mDescriptionRect, Sexy::FONT_BRIANNETOD16, Color(40, 50, 90), mJustification, true, true);
    g->PopState();
}

void AlmanacDialog::SetupLayoutPlants(Sexy::Graphics *g) {
    // 修复介绍文字过长时的显示不全
    old_AlmanacDialog_SetupLayoutPlants(this, g);
    // 之前 SetMaxValue 为 100/115 时，某些英文版 Description 的最后一行有下伸部的字母 (y/g/p) 会被裁掉一半
    // 所以在原有基础上各 +5 留出余量: 短文本 105；长文本 120
    if (unk2[1] > 398) {
        // 文字过长
        unk2[1] *= 1.15f;
        mScrollTextView->SetMaxValue(120);
    } else {
        mScrollTextView->SetMaxValue(105);
    }
}

void AlmanacDialog::SetupLayoutZombies(Sexy::Graphics *g) {
    old_AlmanacDialog_SetupLayoutZombies(this, g);
    // 与 SetupLayoutPlants 相同的滚动范围处理
    if (unk2[1] > 398) {
        unk2[1] *= 1.15f;
        mScrollTextView->SetMaxValue(120);
    } else {
        mScrollTextView->SetMaxValue(105);
    }
}
