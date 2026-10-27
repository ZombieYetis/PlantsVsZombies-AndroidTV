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

#include "PvZ/TodLib/Effect/Reanimator.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/Widget/ZombatarWidget.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"

#include <format>
#include <regex>

bool Reanimation::DrawTrack(Sexy::Graphics *g, int theTrackIndex, int theRenderGroup, TodTriangleGroup *theTriangleGroup) {
    // 修复模仿者植物变白
    if (mFilterEffect != FilterEffect::FILTEREFFECT_NONE) {
        ReanimatorTransform *reanimatorTransform = mReanimatorTransforms + theTrackIndex;
        Sexy::Image *image = reanimatorTransform->mImage;
        ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + theTrackIndex;
        Sexy::Image *mImageOverride = reanimatorTrackInstance->mImageOverride;
        if (image != nullptr) {
            reanimatorTransform->mImage = FilterEffectGetImage(image, mFilterEffect);
        }
        if (mImageOverride != nullptr) {
            reanimatorTrackInstance->mImageOverride = FilterEffectGetImage(mImageOverride, mFilterEffect);
        }
        bool result = old_Reanimation_DrawTrack(this, g, theTrackIndex, theRenderGroup, theTriangleGroup);
        reanimatorTransform->mImage = image;
        reanimatorTrackInstance->mImageOverride = mImageOverride;
        return result;
    }

    return old_Reanimation_DrawTrack(this, g, theTrackIndex, theRenderGroup, theTriangleGroup);
}

int Reanimation::HideTrack(const char *theTrackName, bool theIsHide) {
    int trackIndex = FindTrackIndex(theTrackName);
    if (trackIndex != -1) {
        ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + trackIndex;
        reanimatorTrackInstance->mRenderGroup = theIsHide ? RENDER_GROUP_HIDDEN : RENDER_GROUP_NORMAL;
    }
    return trackIndex;
}

void Reanimation::HideTrackById(int theTrackIndex, bool theIsHide) const {
    if (theTrackIndex != -1) {
        ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + theTrackIndex;
        reanimatorTrackInstance->mRenderGroup = theIsHide ? RENDER_GROUP_HIDDEN : RENDER_GROUP_NORMAL;
    }
}

void Reanimation::HideTrackByPrefix(const char *theTrackPrefix, bool theIsHide) {
    int aTrackCount = mDefinition->mTrackCount;
    if (aTrackCount <= 0) {
        return;
    }

    for (int i = 0; i < aTrackCount; ++i) {
        const char *aName = (mDefinition->mTracks + i)->mName;
        if (theTrackPrefix == nullptr || strstr(aName, theTrackPrefix) != nullptr) {
            HideTrackById(i, theIsHide);
        }
    }
}

void Reanimation::SetImageOrigin(const char *theTrackName, Sexy::Image *theImage) {
    // 和Reanimation_SetImageOverride不一样的是，这个直接替换原始图像。
    int aTrackIndex = FindTrackIndex(theTrackName);
    if (aTrackIndex != -1) {
        ReanimatorTransform *reanimatorTrack = mReanimatorTransforms + aTrackIndex;
        reanimatorTrack->mImage = theImage;
    }
}

void Reanimation::SetImageDefinition(const char *theTrackName, Sexy::Image *theImage) {
    // 和Reanimation_SetImageOrigin不一样的是，这个能对默认动画中没有贴图的生效。
    int aTrackIndex = FindTrackIndex(theTrackName);
    if (aTrackIndex != -1) {
        // ReanimatorFrameTime theFrameTime;
        // Reanimation_GetFrameTime(reanim, &theFrameTime);
        ReanimatorTrack *reanimatorTrack = mDefinition->mTracks + aTrackIndex;
        int aTransformCount = reanimatorTrack->mTransformCount;
        for (int i = 0; i < aTransformCount; ++i) {
            reanimatorTrack->mTransforms[i].mImage = theImage;
        }
    }
}

static ReanimationParams gExtendedReanimationParamArray[] = {
    {ReanimationType::REANIM_ZOMBATAR_HEAD, "addonFiles/compiled/reanim/zombatar_zombie_head.reanim", 1},
    {ReanimationType::REANIM_VS_MOUNDS, "addonFiles/compiled/reanim/VSmounds.reanim", 0},
    {ReanimationType::REANIM_GIGA_FOOTBALL, "addonFiles/compiled/reanim/Zombie_giga_football.reanim", 0},
    {ReanimationType::REANIM_SUPER_FAN_IMP, "addonFiles/compiled/reanim/Zombie_super_fan_imp.reanim", 0},
    {ReanimationType::REANIM_JACKSON, "addonFiles/compiled/reanim/Zombie_jackson.reanim", 0},
    {ReanimationType::REANIM_BACKUP_JACKSON, "addonFiles/compiled/reanim/Zombie_backup.reanim", 0},
    {ReanimationType::REANIM_GIGA_POLEVAULTER, "addonFiles/compiled/reanim/Zombie_giga_polevaulter.reanim", 0},
    {ReanimationType::REANIM_SUNDAY_EDITION, "addonFiles/compiled/reanim/Zombie_sunday_edition.reanim", 0},
    {ReanimationType::REANIM_EXPLORER, "addonFiles/compiled/reanim/Zombie_explorer.reanim", 0},
    {ReanimationType::REANIM_ZOMBLOB, "addonFiles/compiled/reanim/Zombie_zomblob.reanim", 0},
    {ReanimationType::REANIM_ZOMBLOB_MIDDLE, "addonFiles/compiled/reanim/Zombie_zomblob_middle.reanim", 0},
    {ReanimationType::REANIM_ZOMBLOB_SMALL, "addonFiles/compiled/reanim/Zombie_zomblob_small.reanim", 0},
    {ReanimationType::REANIM_GIGA_GARGANTUAR, "addonFiles/compiled/reanim/Zombie_giga_gargantuar.reanim", 0},
    {ReanimationType::REANIM_GIGA_IMP, "addonFiles/compiled/reanim/Zombie_giga_imp.reanim", 0},
    {ReanimationType::REANIM_LIGHTNING_HIT, "addonFiles/compiled/reanim/lightning_hit.reanim", 0},
    {ReanimationType::REANIM_DOGWALKER, "addonFiles/compiled/reanim/Zombie_dogwalker.reanim", 0},
    {ReanimationType::REANIM_DOG, "addonFiles/compiled/reanim/Zombie_dog.reanim", 0},
    {ReanimationType::REANIM_ZOMBIE_TELEPORTATION, "addonFiles/compiled/reanim/Zombie_teleportation.reanim", 0},
    {ReanimationType::REANIM_TELEPORTATION, "addonFiles/compiled/reanim/teleportation.reanim", 0},
    {ReanimationType::REANIM_TELEPORTATION_BULLET_FLASH, "addonFiles/compiled/reanim/teleportation_bullet_flash.reanim", 0},
    {ReanimationType::REANIM_SUPER_NOVA_GARGANTUAR, "addonFiles/compiled/reanim/Zombie_super_nova_gargantuar.reanim", 0},
    {ReanimationType::REANIM_SUPER_NOVA, "addonFiles/compiled/reanim/super_nova.reanim", 0},
    {ReanimationType::REANIM_ZOMBIE_CROSSING_GUARD, "addonFiles/compiled/reanim/Zombie_crossing_guard.reanim", 0},
    {ReanimationType::REANIM_ZOMBIE_SCIENTIST, "addonFiles/compiled/reanim/Zombie_scientist.reanim", 0},
    {ReanimationType::REANIM_HEAL_PARTICLES, "addonFiles/compiled/reanim/heal_particles.reanim", 0},
    {ReanimationType::REANIM_HEAL_MIST, "addonFiles/compiled/reanim/heal_mist.reanim", 0},
    {ReanimationType::REANIM_ZOMBIE_GRAVEDIGGER, "addonFiles/compiled/reanim/Zombie_gravedigger.reanim", 0},
    {ReanimationType::REANIM_ICEBERG_LETTUCE, "addonFiles/compiled/reanim/IcebergLettuce.reanim", 0},
    {ReanimationType::REANIM_CELERY_STALKER, "addonFiles/compiled/reanim/CeleryStalker.reanim", 0},
    {ReanimationType::REANIM_SPORE_SHROOM, "addonFiles/compiled/reanim/SporeShroom.reanim", 0},
    {ReanimationType::REANIM_BLOOMERANG, "addonFiles/compiled/reanim/Bloomerang.reanim", 0},
    {ReanimationType::REANIM_BONK_CHOY, "addonFiles/compiled/reanim/BonkChoy.reanim", 0},
    {ReanimationType::REANIM_SWEET_POTATO, "addonFiles/compiled/reanim/SweetPotato.reanim", 0},
    {ReanimationType::REANIM_CHILLY_PEPPER, "addonFiles/compiled/reanim/ChillyPepper.reanim", 0},
    {ReanimationType::REANIM_ICE, "addonFiles/compiled/reanim/ice.reanim", 0},
    {ReanimationType::REANIM_CHILOOSH, "addonFiles/compiled/reanim/chiloosh.reanim", 0},
    {ReanimationType::REANIM_SUN_BEAN, "addonFiles/compiled/reanim/SunBean.reanim", 0},
    {ReanimationType::REANIM_PEANUT, "addonFiles/compiled/reanim/Peanut.reanim", 0},
    {ReanimationType::REANIM_ENDURIAN, "addonFiles/compiled/reanim/Endurian.reanim", 0},
    {ReanimationType::REANIM_RED_STINGER, "addonFiles/compiled/reanim/RedStinger.reanim", 0},
    {ReanimationType::REANIM_IMP_PEAR, "addonFiles/compiled/reanim/ImpPear.reanim", 0},
    {ReanimationType::REANIM_AKEE, "addonFiles/compiled/reanim/AKEE.reanim", 0},
    {ReanimationType::REANIM_APPLE_CLOCK, "addonFiles/compiled/reanim/apple_clock.reanim", 0},
};

void ReanimatorLoadDefinitions(ReanimationParams *theReanimationParamArray, int theReanimationParamArraySize) {
    if (theReanimationParamArraySize == 0) {
        old_ReanimatorLoadDefinitions(theReanimationParamArray, theReanimationParamArraySize);
        return;
    }
    int newReanimationArraySize = std::size(gExtendedReanimationParamArray);
    auto *newReanimationParamArray = (ReanimationParams *)malloc((theReanimationParamArraySize + newReanimationArraySize) * sizeof(ReanimationParams));
    for (int i = 0; i < theReanimationParamArraySize; ++i) {
        newReanimationParamArray[i].mReanimationType = theReanimationParamArray[i].mReanimationType;
        newReanimationParamArray[i].mReanimFileName = theReanimationParamArray[i].mReanimFileName;
        newReanimationParamArray[i].mReanimParamFlags = theReanimationParamArray[i].mReanimParamFlags;
    }
    for (int i = 0; i < newReanimationArraySize; ++i) {
        newReanimationParamArray[i + theReanimationParamArraySize].mReanimationType = gExtendedReanimationParamArray[i].mReanimationType;
        newReanimationParamArray[i + theReanimationParamArraySize].mReanimFileName = gExtendedReanimationParamArray[i].mReanimFileName;
        newReanimationParamArray[i + theReanimationParamArraySize].mReanimParamFlags = gExtendedReanimationParamArray[i].mReanimParamFlags;
    }
    old_ReanimatorLoadDefinitions(newReanimationParamArray, theReanimationParamArraySize + newReanimationArraySize);
}

void Reanimation::SetZombatarReanim() {
    HideTrackByPrefix("hats", true);
    HideTrackByPrefix("hair", true);
    HideTrackByPrefix("facialHair", true);
    HideTrackByPrefix("accessories", true);
    HideTrackByPrefix("eyeWear", true);
    HideTrackByPrefix("tidBits", true);
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::SetZombatarHats(unsigned char theHats, unsigned char theColor) {
    HideTrackByPrefix("hats", true);
    if (theHats != 255) {
        char hatsChar[] = "hats_00";
        std::format_to_n(std::end(hatsChar) - 3, 2, "{:02}", theHats);
        HideTrackByPrefix(hatsChar, false);
        if (theColor != 255 && ZombatarWidget::AccessoryIsColorized(ZombatarWidget::HAT, theHats)) {
            int aTrackIndex = FindTrackIndex(hatsChar);
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + aTrackIndex;
            reanimatorTrackInstance->mTrackColor = gZombatarAccessoryColor2[theColor];
        }
    }
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::SetZombatarHair(unsigned char theHair, unsigned char theColor) {
    HideTrackByPrefix("hair", true);
    if (theHair != 255) {
        char hairChar[] = "hair_00";
        std::format_to_n(std::end(hairChar) - 3, 2, "{:02}", theHair);
        HideTrackByPrefix(hairChar, false);
        if (theColor != 255 && ZombatarWidget::AccessoryIsColorized(ZombatarWidget::HAIR, theHair)) {
            int aTrackIndex = FindTrackIndex(hairChar);
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + aTrackIndex;
            reanimatorTrackInstance->mTrackColor = gZombatarAccessoryColor[theColor];
        }
    }
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::SetZombatarFHair(unsigned char theFacialHair, unsigned char theColor) {
    HideTrackByPrefix("facialHair", true);
    if (theFacialHair != 255) {
        char facialHairChar[] = "facialHair_00";
        std::format_to_n(std::end(facialHairChar) - 3, 2, "{:02}", theFacialHair);
        HideTrackByPrefix(facialHairChar, false);
        if (theColor != 255 && ZombatarWidget::AccessoryIsColorized(ZombatarWidget::FHAIR, theFacialHair)) {
            int aTrackIndex = FindTrackIndex(facialHairChar);
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + aTrackIndex;
            reanimatorTrackInstance->mTrackColor = gZombatarAccessoryColor[theColor];
        }
    }
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::SetZombatarAccessories(unsigned char theAccessories, unsigned char theColor) {
    HideTrackByPrefix("accessories", true);
    if (theAccessories != 255) {
        unsigned char accessoriesFix = 0;
        switch (theAccessories) {
            case 5:
                accessoriesFix = 14;
                break;
            case 6:
                accessoriesFix = 5;
                break;
            case 7:
                accessoriesFix = 6;
                break;
            case 8:
                accessoriesFix = 12;
                break;
            case 9:
                accessoriesFix = 7;
                break;
            case 10:
                accessoriesFix = 9;
                break;
            case 11:
                accessoriesFix = 10;
                break;
            case 12:
                accessoriesFix = 11;
                break;
            case 14:
                accessoriesFix = 8;
                break;
            default:
                accessoriesFix = theAccessories;
                break;
        }
        char accessoriesChar[] = "accessories_00";
        std::format_to_n(std::end(accessoriesChar) - 3, 2, "{:02}", accessoriesFix);
        HideTrackByPrefix(accessoriesChar, false);
        if (theColor != 255 && ZombatarWidget::AccessoryIsColorized(ZombatarWidget::ACCESSORY, theAccessories)) {
            int aTrackIndex = FindTrackIndex(accessoriesChar);
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + aTrackIndex;
            reanimatorTrackInstance->mTrackColor = gZombatarAccessoryColor2[theColor];
        }
    }
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::SetZombatarEyeWear(unsigned char theEyeWear, unsigned char theColor) {
    HideTrackByPrefix("eyeWear", true);
    if (theEyeWear != 255) {
        char eyeWearChar[] = "eyeWear_00";
        std::format_to_n(std::end(eyeWearChar) - 3, 2, "{:02}", theEyeWear);
        HideTrackByPrefix(eyeWearChar, false);
        if (theColor != 255 && ZombatarWidget::AccessoryIsColorized(ZombatarWidget::EYEWEAR, theEyeWear)) {
            int aTrackIndex = FindTrackIndex(eyeWearChar);
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + aTrackIndex;
            reanimatorTrackInstance->mTrackColor = gZombatarAccessoryColor2[theColor];
        }
    }
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::SetZombatarTidBits(unsigned char theTidBits, unsigned char theColor) {
    HideTrackByPrefix("tidBits", true);
    if (theTidBits != 255) {
        char tidBitsChar[] = "tidBits_00";
        std::format_to_n(std::end(tidBitsChar) - 3, 2, "{:02}", theTidBits);
        HideTrackByPrefix(tidBitsChar, false);
        if (theColor != 255 && ZombatarWidget::AccessoryIsColorized(ZombatarWidget::TIDBIT, theTidBits)) {
            int aTrackIndex = FindTrackIndex(tidBitsChar);
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + aTrackIndex;
            reanimatorTrackInstance->mTrackColor = gZombatarAccessoryColor2[theColor];
        }
    }
    Update(); // 一次Update是必要的，否则绘制出来是Empty
}

void Reanimation::GetZombatarTrackIndex(int *theIndexArray) const {
    int aTrackCount = mDefinition->mTrackCount;
    if (aTrackCount <= 0) {
        return;
    }
    const char *stringArray[] = {"hats", "eyeWear"};
    // char *stringArray[] = {"hats","hair","facialHair","accessories","eyeWear","tidBits"};
    for (int i = 0; i < 2; ++i) {
        theIndexArray[i] = -1;
        for (int j = 0; j < aTrackCount; ++j) {
            const char *aName = (mDefinition->mTracks + j)->mName;
            // LOGD("%s",mName);
            if (strstr(aName, stringArray[i]) != nullptr) {
                ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + j;
                // LOGD("%d",reanimatorTrackInstance->mRenderGroup);
                if (reanimatorTrackInstance->mRenderGroup != -1) {
                    theIndexArray[i] = j;
                    break;
                }
            }
        }
    }
}

int Reanimation::GetZombatarHatTrackIndex() const {
    int aTrackCount = mDefinition->mTrackCount;
    if (aTrackCount <= 0) {
        return -1;
    }
    std::regex pattern(R"(hats_\d{2})");


    // char *stringArray[] = {"hats","hair","facialHair","accessories","eyeWear","tidBits"};
    for (int j = 0; j < aTrackCount; ++j) {
        const char *aName = (mDefinition->mTracks + j)->mName;
        // LOGD("%s,%d",mName,std::regex_match(mName, pattern));
        if (std::regex_match(aName, pattern)) {
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + j;
            if (reanimatorTrackInstance->mRenderGroup != -1) {
                return j;
            }
        }
    }
    return -1;
}

int Reanimation::GetZombatarEyeWearTrackIndex() const {
    int aTrackCount = mDefinition->mTrackCount;
    if (aTrackCount <= 0) {
        return -1;
    }
    std::regex pattern(R"(eyeWear_\d{2})");

    for (int j = 0; j < aTrackCount; ++j) {
        const char *aName = (mDefinition->mTracks + j)->mName;
        // LOGD("%s,%d",mName,std::regex_match(mName, pattern));
        if (std::regex_match(aName, pattern)) {
            ReanimatorTrackInstance *reanimatorTrackInstance = mTrackInstances + j;
            if (reanimatorTrackInstance->mRenderGroup != -1) {
                return j;
            }
        }
    }
    return -1;
}

pvzstl::string DefinitionGetCompiledFilePathFromXMLFilePath(const pvzstl::string &defPathString) {
    // 扩展动画、粒子等定义直接在 addonFiles 路径后追加 .compiled。
    if (defPathString.contains("addonFiles")) {
        return defPathString + ".compiled";
    }
    return old_DefinitionGetCompiledFilePathFromXMLFilePath(defPathString);
}

bool Reanimation::ShouldTriggerTimedEvent(float theEventTime) const {
    if (mFrameCount == 0 || mLastFrameTime <= 0.0f || mAnimRate <= 0.0f) // 没有动画或倒放或未播放
        return false;

    if (mAnimTime >= mLastFrameTime) // 一般情况下，可触发的范围为 [mLastFrameTime, mAnimTime]
        return theEventTime >= mLastFrameTime && theEventTime < mAnimTime;
    else // 若动画正好完成一次循环而重新进入下一次循环，则可触发的范围为 [0, mAnimTime] ∪ [mLastFrameTime, 1]
        return theEventTime >= mLastFrameTime || theEventTime < mAnimTime;
}

void Reanimation::AssignRenderGroupToTrack(const char *theTrackName, int theRenderGroup) const {
    for (int i = 0; i < mDefinition->mTrackCount; i++)
        if (strcasecmp(mDefinition->mTracks[i].mName, theTrackName) == 0) {
            mTrackInstances[i].mRenderGroup = theRenderGroup; // 仅设置首个名称恰好为 theTrackName 的轨道
            return;
        }
}

void Reanimation::Draw(Sexy::Graphics *g) {
    DrawRenderGroup(g, RENDER_GROUP_NORMAL);
}

ReanimatorTrackInstance *Reanimation::GetTrackInstanceByName(const char *theTrackName) {
    return &mTrackInstances[FindTrackIndex(theTrackName)];
}
