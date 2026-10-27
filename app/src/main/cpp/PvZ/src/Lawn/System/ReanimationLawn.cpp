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

#include "PvZ/Lawn/System/ReanimationLawn.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Graphics/MemoryImage.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

using namespace Sexy;

void ReanimatorCache::LoadCachedImages() {
    // 此时尚未加载AddonImages！不可在此处替换贴图们
    old_ReanimatorCache_LoadCachedImages(this);
}

void ReanimatorCache::UpdateReanimationForVariation(Reanimation *theReanim, DrawVariation theDrawVariation) {
    // 修复商店花盆不显示、修复花园花盆不显示、修复花园手套和推车预览不显示。原理就是Reanimation::Update。
    old_ReanimatorCache_UpdateReanimationForVariation(this, theReanim, theDrawVariation);
    // Reanimation_SetAnimRate(theReanim, 0.0f);
    theReanim->Update();
    // Reanimation_SetAnimRate(theReanim, tmp);
}

void ReanimatorCache::GetPlantImageSize(SeedType theSeedType, int &theOffsetX, int &theOffsetY, int &theWidth, int &theHeight) {
    theOffsetX = -20;
    theOffsetY = -20;
    theWidth = 120;
    theHeight = 120;

    if (theSeedType == SeedType::SEED_TALLNUT) {
        theOffsetY = -40;
        theHeight += 40;
    } else if (theSeedType == SeedType::SEED_MELONPULT || theSeedType == SeedType::SEED_WINTERMELON) {
        theOffsetX = -40;
        theWidth += 40;
    } else if (theSeedType == SeedType::SEED_COBCANNON) {
        theWidth += 80;
    }
}

void ReanimatorCache::ReanimatorCacheInitialize() {
    mApp = reinterpret_cast<LawnApp *>(Sexy::gSexyAppBase);
    for (int i = 0; i < SeedType::NUM_SEED_TYPES; i++)
        mPlantImages[i] = nullptr;
    for (int i = 0; i < LawnMowerType::NUM_MOWER_TYPES; i++)
        mLawnMowers[i] = nullptr;
    for (int i = 0; i < ZombieType::NUM_ZOMBIE_TYPES; i++)
        mZombieImages[i] = nullptr;

    for (int i = 0; i < EXTENDED_NUM_SEED_TYPES; i++)
        msExtendedPlantImages[i] = nullptr;
    for (int i = 0; i < EXTENDED_NUM_ZOMBIE_TYPES - NUM_CACHED_ZOMBIE_TYPES; i++)
        msExtendedZombieImages[i] = nullptr;
}

void ReanimatorCache::ReanimatorCacheDispose() {
    for (int i = 0; i < SeedType::NUM_SEED_TYPES; i++)
        delete mPlantImages[i];
    while (mImageVariationList.mSize > 0) {
        ReanimCacheImageVariation aImageVariation = mImageVariationList.RemoveHead();
        delete aImageVariation.mImage;
    }
    for (int i = 0; i < LawnMowerType::NUM_MOWER_TYPES; i++)
        delete mLawnMowers[i];
    for (int i = 0; i < ZombieType::NUM_ZOMBIE_TYPES; i++)
        delete mZombieImages[i];

    for (int i = 0; i < EXTENDED_NUM_SEED_TYPES; i++)
        delete msExtendedPlantImages[i];
    for (int i = 0; i < EXTENDED_NUM_ZOMBIE_TYPES - NUM_CACHED_ZOMBIE_TYPES; i++)
        delete msExtendedZombieImages[i];
}

void ReanimatorCache::DrawCachedPlant(Graphics *g, float thePosX, float thePosY, SeedType theSeedType, DrawVariation theDrawVariation) {
    if (theSeedType == SEED_NONE || theSeedType > NUM_SEED_TYPES)
        return;

    if (theDrawVariation == DrawVariation::VARIATION_IMITATER_LESS || theDrawVariation == DrawVariation::VARIATION_IMITATER || theDrawVariation == DrawVariation::VARIATION_NORMAL) {
        Image *aPlantImage = mPlantImages[theSeedType];
        if (aPlantImage == nullptr) {
            return;
        }
        if (theDrawVariation == DrawVariation::VARIATION_IMITATER) {
            aPlantImage = FilterEffectGetImage(aPlantImage, FilterEffect::FILTEREFFECT_WASHED_OUT);
        } else if (theDrawVariation == DrawVariation::VARIATION_IMITATER_LESS) {
            aPlantImage = FilterEffectGetImage(aPlantImage, FilterEffect::FILTEREFFECT_LESS_WASHED_OUT);
        }
        int a = 0, b = 0, c = 0, d = 0;
        GetPlantImageSize(theSeedType, a, b, c, d);
        float xScaled = g->mScaleX;
        float yScaled = g->mScaleY;
        // 修复关闭3D加速后SeedPacket上不显示植物
        // if (Sexy_SexyAppBase_Is3DAccelerated(a1->mApp)) {
        TodDrawImageScaledF(g, aPlantImage, thePosX + xScaled * a, thePosY + yScaled * b, xScaled, yScaled);
        // } else {
        // if (xScaled == 1.0 && yScaled == 1.0) {
        // DrawImage(g, aPlantImage, thePosX + a, thePosY + b);
        // return;
        // }
        // }
    } else {
        old_ReanimatorCache_DrawCachedPlant(this, g, thePosX, thePosY, theSeedType, theDrawVariation);
    }
}

void ReanimatorCache::DrawCachedExtendedPlant(Graphics *g, float thePosX, float thePosY, SeedType theSeedType, DrawVariation theDrawVariation) {
    if (theSeedType < SEED_ICEBERG_LETTUCE || theSeedType >= EXTENDED_NUM_SEEDS_IN_CHOOSER) {
        return;
    }

    MemoryImage *aImage = nullptr;
    if (theDrawVariation != DrawVariation::VARIATION_NORMAL) {
        //        for (TodListNode<ReanimCacheImageVariation> *aNode = mImageVariationList.mHead; aNode != nullptr; aNode = aNode->mNext) {
        //            ReanimCacheImageVariation &aImageVariation = aNode->mValue;
        //            if (aImageVariation.mSeedType == theSeedType && aImageVariation.mDrawVariation == theDrawVariation) {
        //                aImage = aImageVariation.mImage;
        //                break;
        //            }
        //        }

        if (aImage == nullptr) {
            return;
            //            aImage = MakeCachedPlantFrame(theSeedType, theDrawVariation);
            //            ReanimCacheImageVariation aNewImageVariation;
            //            aNewImageVariation.mSeedType = theSeedType;
            //            aNewImageVariation.mDrawVariation = theDrawVariation;
            //            aNewImageVariation.mImage = aImage;
            //            mImageVariationList.AddHead(aNewImageVariation);
        }
    } else {
        aImage = msExtendedPlantImages[theSeedType - SEED_ICEBERG_LETTUCE];
        if (aImage == nullptr) {
            //            aImage = MakeCachedPlantFrame(theSeedType, DrawVariation::VARIATION_NORMAL);
            int aOffsetX, aOffsetY, aWidth, aHeight;
            GetPlantImageSize(theSeedType, aOffsetX, aOffsetY, aWidth, aHeight);
            MemoryImage *aMemoryImage = MakeBlankCanvasImage(aWidth, aHeight);
            Graphics aMemoryGraphics(aMemoryImage);
            aMemoryGraphics.SetLinearBlend(true);

            PlantDefinition &aPlantDef = GetPlantDefinition(theSeedType);
            DrawReanimatorFrame(&aMemoryGraphics, -aOffsetX, -aOffsetY, aPlantDef.mReanimationType, "anim_idle", theDrawVariation);
            aImage = aMemoryImage;
            msExtendedPlantImages[theSeedType - SEED_ICEBERG_LETTUCE] = aImage;
        }
    }

    int aOffsetX, aOffsetY, aWidth, aHeight;
    GetPlantImageSize(theSeedType, aOffsetX, aOffsetY, aWidth, aHeight);
    if (!mApp->Is3DAccelerated() && g->mScaleX == 1.0f && g->mScaleY == 1.0f)
        g->DrawImage(aImage, thePosX + aOffsetX, thePosY + aOffsetY);
    else
        TodDrawImageScaledF(g, aImage, thePosX + (aOffsetX * g->mScaleX), thePosY + (aOffsetY * g->mScaleY), g->mScaleX, g->mScaleY);
}

void ReanimatorCache::DrawCachedZombie(Graphics *g, float thePosX, float thePosY, ZombieType theZombieType) {
    if (mZombieImages[theZombieType])
        TodDrawImageScaledF(g, mZombieImages[theZombieType], thePosX, thePosY, g->mScaleX, g->mScaleY);
}

void ReanimatorCache::DrawCachedExtendedZombie(Sexy::Graphics *g, float thePosX, float thePosY, ZombieType theZombieType) {
    int aExtendedIndex = theZombieType - NUM_CACHED_ZOMBIE_TYPES;
    if (msExtendedZombieImages[aExtendedIndex] == nullptr)
        msExtendedZombieImages[aExtendedIndex] = MakeCachedZombieFrame(theZombieType);
    TodDrawImageScaledF(g, msExtendedZombieImages[aExtendedIndex], thePosX, thePosY, g->mScaleX, g->mScaleY);
}

MemoryImage *ReanimatorCache::MakeBlankMemoryImage(int theWidth, int theHeight) {
    MemoryImage *aImage = new MemoryImage();

    int aBitsCount = theWidth * theHeight;
    aImage->mBits = new unsigned long[aBitsCount + 1];
    aImage->mWidth = theWidth;
    aImage->mHeight = theHeight;
    aImage->mHasTrans = true;
    aImage->mHasAlpha = true;
    memset(aImage->mBits, 0, aBitsCount * 4);
    aImage->mBits[aBitsCount] = Sexy::MEMORYCHECK_ID;
    return aImage;
}

Sexy::MemoryImage *ReanimatorCache::MakeCachedZombieFrame(ZombieType theZombieType) {
    int maxWidth = 256;
    int maxHeight = 256;

    if (theZombieType == ZombieType::ZOMBIE_ZAMBONI) {
        maxWidth = 512;
    }

    MemoryImage *aMemoryImage = MakeBlankCanvasImage(maxWidth, maxHeight);
    Graphics aMemoryGraphics(aMemoryImage);
    aMemoryGraphics.SetLinearBlend(true);

    ZombieType aUseZombieType = theZombieType;
    if (theZombieType == ZombieType::ZOMBIE_CACHED_POLEVAULTER_WITH_POLE) {
        aUseZombieType = ZombieType::ZOMBIE_POLEVAULTER;
    }
    ZombieDefinition &aZombieDef = GetZombieDefinition(aUseZombieType);

    float aPosX = 40.0f, aPosY = 40.0f;

    if (theZombieType >= ZombieType::NUM_CACHED_ZOMBIE_TYPES && theZombieType < EXTENDED_NUM_ZOMBIE_TYPES) {
        int aExtendedIndex = theZombieType - NUM_CACHED_ZOMBIE_TYPES;
        if (theZombieType == ZombieType::ZOMBIE_GIGA_POLEVAULTER) {
            Reanimation aReanim;
            aReanim.ReanimationInitializeType(aPosX, aPosY, aZombieDef.mReanimationType);
            aReanim.PlayReanim("anim_walk", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            aReanim.AssignRenderGroupToPrefix("anim_pole1", RENDER_GROUP_HIDDEN);
            aReanim.AssignRenderGroupToPrefix("anim_pole2", RENDER_GROUP_HIDDEN);
            aReanim.AssignRenderGroupToPrefix("anim_pole3", RENDER_GROUP_HIDDEN);
            aReanim.Update();
            aReanim.Draw(&aMemoryGraphics);
        } else if (theZombieType == ZombieType::ZOMBIE_DOGWALKER) {
            Reanimation aReanim;
            aReanim.ReanimationInitializeType(aPosX, aPosY, aZombieDef.mReanimationType);
            aReanim.PlayReanim("anim_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            aReanim.AssignRenderGroupToTrack("Zombie_dogwalker_rope2_2", RENDER_GROUP_HIDDEN);
            aReanim.Update();
            aReanim.Draw(&aMemoryGraphics);
            Reanimation aDogeReanim;
            aDogeReanim.ReanimationInitializeType(aPosX - 40, aPosY, ReanimationType::REANIM_DOG);
            aDogeReanim.PlayReanim("anim_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            aDogeReanim.Update();
            aDogeReanim.Draw(&aMemoryGraphics);
        } else {
            if (Zombie::IsGargantuar(theZombieType)) {
                aPosY = 60.0f;
            }
            Reanimation aReanim;
            aReanim.ReanimationInitializeType(aPosX, aPosY, aZombieDef.mReanimationType);
            aReanim.PlayReanim("anim_idle", ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
            aReanim.Update();
            aReanim.Draw(&aMemoryGraphics);
        }
        msExtendedZombieImages[aExtendedIndex] = aMemoryImage;
        return aMemoryImage;
    }

    if (theZombieType == ZombieType::ZOMBIE_PEA_HEAD || theZombieType == ZombieType::ZOMBIE_GATLING_HEAD || theZombieType == ZombieType::ZOMBIE_SQUASH_HEAD) { // 为植物僵尸增加SeedPacket图标
        Reanimation aReanim;
        aReanim.ReanimationInitializeType(aPosX, aPosY, aZombieDef.mReanimationType);
        aReanim.mIsAttachment = true;
        aReanim.SetFramesForLayer("anim_idle");
        Zombie::SetupReanimLayers(&aReanim, aUseZombieType);
        aReanim.AssignRenderGroupToPrefix("anim_hair", RENDER_GROUP_HIDDEN);
        aReanim.AssignRenderGroupToPrefix("anim_head2", RENDER_GROUP_HIDDEN);

        ReanimatorTrackInstance *aTrackInstance = aReanim.GetTrackInstanceByName("anim_head1");
        aTrackInstance->mImageOverride = IMAGE_BLANK;

        ReanimationType aHeadType;

        if (theZombieType == ZombieType::ZOMBIE_PEA_HEAD)
            aHeadType = ReanimationType::REANIM_PEASHOOTER;
        else if (theZombieType == ZombieType::ZOMBIE_GATLING_HEAD)
            aHeadType = ReanimationType::REANIM_GATLINGPEA;
        else // if (theZombieType == ZombieType::ZOMBIE_SQUASH_HEAD)
            aHeadType = ReanimationType::REANIM_SQUASH;

        Reanimation *aHeadReanim = mApp->AddReanimation(0, 0, 0, aHeadType);
        if (theZombieType == ZombieType::ZOMBIE_SQUASH_HEAD)
            aHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);
        else
            aHeadReanim->PlayReanim("anim_head_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);

        AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aHeadReanim, 0.0f, 0.0f);
        aReanim.mFrameBasePose = 0;

        if (theZombieType == ZombieType::ZOMBIE_SQUASH_HEAD)
            TodScaleRotateTransformMatrix(aAttachEffect->mOffset, 55.0f, -15.0f, 0.2f, -0.75f, 0.75f);
        else
            TodScaleRotateTransformMatrix(aAttachEffect->mOffset, 65.0f, -5.0f, 0.2f, -1.0f, 1.0f);

        SexyTransform2D aOverlayMatrix;
        aReanim.GetAttachmentOverlayMatrix(aReanim.FindTrackIndex("anim_head1"), aOverlayMatrix);
        AttachmentUpdateAndSetMatrix(aTrackInstance->mAttachmentID, aOverlayMatrix);

        aReanim.Update();
        aReanim.Draw(&aMemoryGraphics);
        mZombieImages[theZombieType] = aMemoryImage;
    } else if (theZombieType == ZombieType::ZOMBIE_WALLNUT_HEAD || theZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD || theZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD) {
        Reanimation aReanim;
        aReanim.ReanimationInitializeType(aPosX, aPosY, aZombieDef.mReanimationType);
        aReanim.mIsAttachment = true;
        aReanim.SetFramesForLayer("anim_idle");
        Zombie::SetupReanimLayers(&aReanim, aUseZombieType);
        aReanim.AssignRenderGroupToPrefix("anim_hair", RENDER_GROUP_HIDDEN);
        aReanim.AssignRenderGroupToPrefix("anim_head", RENDER_GROUP_HIDDEN);
        aReanim.AssignRenderGroupToPrefix("anim_head2", RENDER_GROUP_HIDDEN);
        aReanim.AssignRenderGroupToPrefix("Zombie_tie", RENDER_GROUP_HIDDEN);

        ReanimationType aHeadType;

        if (theZombieType == ZombieType::ZOMBIE_WALLNUT_HEAD)
            aHeadType = ReanimationType::REANIM_WALLNUT;
        else if (theZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD)
            aHeadType = ReanimationType::REANIM_TALLNUT;
        else // if (theZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD)
            aHeadType = ReanimationType::REANIM_JALAPENO;

        Reanimation *aHeadReanim = mApp->AddReanimation(0, 0, 0, aHeadType);
        aHeadReanim->PlayReanim("anim_idle", ReanimLoopType::REANIM_LOOP, 0, 15.0f);

        ReanimatorTrackInstance *aTrackInstance = aReanim.GetTrackInstanceByName("Zombie_body");
        AttachEffect *aAttachEffect = AttachReanim(aTrackInstance->mAttachmentID, aHeadReanim, 0.0f, 0.0f);
        aReanim.mFrameBasePose = 0;

        if (theZombieType == ZombieType::ZOMBIE_WALLNUT_HEAD)
            TodScaleRotateTransformMatrix(aAttachEffect->mOffset, 50.0f, 0.0f, 0.2f, -0.8f, 0.8f);
        else if (theZombieType == ZombieType::ZOMBIE_TALLNUT_HEAD)
            TodScaleRotateTransformMatrix(aAttachEffect->mOffset, 37.0f, 0.0f, 0.2f, -0.8f, 0.8f);
        else if (theZombieType == ZombieType::ZOMBIE_JALAPENO_HEAD)
            TodScaleRotateTransformMatrix(aAttachEffect->mOffset, 55.0f, -5.0f, 0.2f, -1.0f, 1.0f);

        SexyTransform2D aOverlayMatrix;
        aReanim.GetAttachmentOverlayMatrix(aReanim.FindTrackIndex("Zombie_body"), aOverlayMatrix);
        AttachmentUpdateAndSetMatrix(aTrackInstance->mAttachmentID, aOverlayMatrix);

        aReanim.Update();
        aReanim.Draw(&aMemoryGraphics);
        mZombieImages[theZombieType] = aMemoryImage;
    } else if (aZombieDef.mReanimationType == ReanimationType::REANIM_ZOMBIE) {
        Reanimation aReanim;
        aReanim.ReanimationInitializeType(aPosX, aPosY, aZombieDef.mReanimationType);
        aReanim.SetFramesForLayer("anim_idle");
        Zombie::SetupReanimLayers(&aReanim, aUseZombieType);

        if (theZombieType == ZombieType::ZOMBIE_DOOR) {
            aReanim.AssignRenderGroupToTrack("anim_screendoor", RENDER_GROUP_NORMAL);
            aReanim.AssignRenderGroupToTrack("Zombie_outerarm_screendoor", RENDER_GROUP_NORMAL);
        } else if (theZombieType == ZombieType::ZOMBIE_TRASHCAN) {
            aReanim.AssignRenderGroupToTrack("anim_screendoor", RENDER_GROUP_NORMAL);
            aReanim.AssignRenderGroupToTrack("Zombie_outerarm_screendoor", RENDER_GROUP_NORMAL);
            aReanim.SetImageOverride("anim_screendoor", Sexy::IMAGE_REANIM_ZOMBIE_TRASHCAN1);
        } else if (theZombieType == ZombieType::ZOMBIE_FLAG) {
            Reanimation aReanimFlag;
            aReanimFlag.ReanimationInitializeType(aPosX, aPosY, ReanimationType::REANIM_ZOMBIE_FLAGPOLE);
            aReanimFlag.SetFramesForLayer("Zombie_flag");
            aReanimFlag.Update();
            aReanimFlag.Draw(&aMemoryGraphics);
        }
        aReanim.Update();
        aReanim.Draw(&aMemoryGraphics);
        mZombieImages[theZombieType] = aMemoryImage;
    } else if (aZombieDef.mReanimationType == ReanimationType::REANIM_BOSS) {
        Reanimation aReanim;
        aReanim.ReanimationInitializeType(-524.0f, -88.0f, aZombieDef.mReanimationType);
        aReanim.SetFramesForLayer("anim_head_idle");
        Reanimation aReanimDriver;
        aReanimDriver.ReanimationInitializeType(46.0f, 22.0f, ReanimationType::REANIM_BOSS_DRIVER);
        aReanimDriver.SetFramesForLayer("anim_idle");
        aReanim.Update();
        aReanim.Draw(&aMemoryGraphics);
        aReanimDriver.Update();
        aReanimDriver.Draw(&aMemoryGraphics);
        aReanim.AssignRenderGroupToTrack("boss_body1", RENDER_GROUP_HIDDEN);
        aReanim.AssignRenderGroupToTrack("boss_neck", RENDER_GROUP_HIDDEN);
        aReanim.AssignRenderGroupToTrack("boss_head2", RENDER_GROUP_HIDDEN);

        aReanim.Draw(&aMemoryGraphics);
        mZombieImages[theZombieType] = aMemoryImage;
    } else if (theZombieType == ZombieType::ZOMBIE_REDEYE_GARGANTUAR) { // 为红眼巨人增加SeedPacket图标
        Reanimation aReanim;
        aReanim.ReanimationInitializeType(aPosX, aPosY + 20, aZombieDef.mReanimationType);
        aReanim.SetFramesForLayer("anim_idle");
        Zombie::SetupReanimLayers(&aReanim, ZombieType::ZOMBIE_REDEYE_GARGANTUAR);
        aReanim.SetImageOverride("anim_head1", Sexy::IMAGE_REANIM_ZOMBIE_GARGANTUAR_HEAD_REDEYE);
        aReanim.Update();
        aReanim.Draw(&aMemoryGraphics);
        mZombieImages[theZombieType] = aMemoryImage;
    } else {
        const char *aTrackName = "anim_idle";
        if (theZombieType == ZombieType::ZOMBIE_POGO) {
            aTrackName = "anim_pogo";
        } else if (theZombieType == ZombieType::ZOMBIE_CACHED_POLEVAULTER_WITH_POLE) {
            aTrackName = "anim_idle";
        } else if (theZombieType == ZombieType::ZOMBIE_POLEVAULTER) {
            aTrackName = "anim_walk";
        } else if (theZombieType == ZombieType::ZOMBIE_GARGANTUAR) {
            aPosY = 60.0f;
        } else if (theZombieType == ZombieType::ZOMBIE_ZAMBONI) {
            aPosX = 140.0f;
        }
        DrawReanimatorFrame(&aMemoryGraphics, aPosX, aPosY, aZombieDef.mReanimationType, aTrackName, DrawVariation::VARIATION_NORMAL);
        mZombieImages[theZombieType] = aMemoryImage;
    }

    return aMemoryImage;
}
