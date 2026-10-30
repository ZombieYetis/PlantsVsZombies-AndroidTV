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

#include "PvZ/TodLib/Common/TodFoley.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"

#include <algorithm>
#include <mutex>

void TodFoleyInitialize(FoleyParams *theFoleyParamArray, int theFoleyParamArraySize) {
    gFoleyParamArray = theFoleyParamArray;
    gFoleyParamArraySize = theFoleyParamArraySize;
}

FoleyParams *LookupFoley(FoleyType theFoleyType) {
    return &gFoleyParamArray[theFoleyType];
}

void SoundSystemReleaseFinishedInstances(TodFoley *theSoundSystem) {
    for (int aFoleyIndex = 0; aFoleyIndex < gFoleyParamArraySize; ++aFoleyIndex) {

        FoleyTypeData &aTypeData = theSoundSystem->mTypeData[aFoleyIndex];

        for (int anInstanceIndex = 0; anInstanceIndex < MAX_FOLEY_INSTANCES; ++anInstanceIndex) {

            FoleyInstance &aFoleyInstance = aTypeData.mFoleyInstances[anInstanceIndex];

            if (aFoleyInstance.mRefCount == 0) {
                continue;
            }
            if (aFoleyInstance._paused) {
                continue;
            }
            if (aFoleyInstance.mInstance->GetVTable()->IsPlaying(aFoleyInstance.mInstance)) {
                continue;
            }
            aFoleyInstance.mInstance->GetVTable()->Release(aFoleyInstance.mInstance);
            aFoleyInstance.mInstance = nullptr;
            aFoleyInstance.mRefCount = 0;
        }
    }
}

void TodFoley::RehookupSoundWithMusicVolume() {
    SoundSystemReleaseFinishedInstances(this);
    for (int aFoleyIndex = 0; aFoleyIndex < gFoleyParamArraySize; ++aFoleyIndex) {
        FoleyParams *aFoleyParams = LookupFoley(static_cast<FoleyType>(aFoleyIndex));

        if ((aFoleyParams->mFoleyFlags & 0x8u) == 0) {
            continue;
        }

        FoleyTypeData &aTypeData = mTypeData[aFoleyIndex];

        for (int anInstanceIndex = 0; anInstanceIndex < MAX_FOLEY_INSTANCES; ++anInstanceIndex) {

            FoleyInstance *anInstance = &aTypeData.mFoleyInstances[anInstanceIndex];

            if (anInstance->mRefCount != 0) {
                ApplyMusicVolume(anInstance);
            }
        }
    }
}

auto GetNewLawnFoleyParamArray() -> FoleyParams (&)[FoleyType::EXTENDED_NUM_FOLEY] {
    static FoleyParams newArray[FoleyType::EXTENDED_NUM_FOLEY] = {};

    static std::once_flag flag;
    std::call_once(flag, [] {
        std::ranges::copy(gLawnFoleyParamArray, newArray);

        const FoleyParams extendedArray[FoleyType::EXTENDED_NUM_FOLEY - FoleyType::NUM_FOLEY] = {
            {FoleyType::FOLEY_MENU_LEFT, 0.0f, {&Sexy::SOUND_MENU_L_ST}, 1U},
            {FoleyType::FOLEY_MENU_CENTRE, 0.0f, {&Sexy::SOUND_MENU_C_ST}, 1U},
            {FoleyType::FOLEY_MENU_RIGHT, 0.0f, {&Sexy::SOUND_MENU_R_ST}, 1U},
            {FoleyType::FOLEY_ALLSTAR_TACKLE, 5.0f, {&addonSounds.allstardbl}, 0U},
            {FoleyType::FOLEY_THRILLER, 0.0f, {&addonSounds.thriller}, 6U},
            {FoleyType::FOLEY_EXPLORER_IGNITE, 0.0f, {&addonSounds.explorer}, 0U},
            {FoleyType::FOLEY_ZOMBLOB, 0.0f, {&addonSounds.zomblob}, 0U},
            {FoleyType::FOLEY_POWER_POLE_CHARGE, 0.0f, {&addonSounds.power_pole_charge}, 0U},
            {FoleyType::FOLEY_POWER_POLE_CORE, 0.0f, {&addonSounds.power_pole_core}, 0U},
            {FoleyType::FOLEY_POWER_POLE_HIFI, 0.0f, {&addonSounds.power_pole_hifi}, 0U},
            {FoleyType::FOLEY_POWER_POLE_TAIL, 0.0f, {&addonSounds.power_pole_tail}, 0U},
            {FoleyType::FOLEY_POWER_POLE_WIDTH, 0.0f, {&addonSounds.power_pole_width}, 0U},
            {FoleyType::FOLEY_GIGA_LAUGH, 0.0f, {&addonSounds.giga_laugh}, 0U},
            {FoleyType::FOLEY_GIGA_LAUGH2, 0.0f, {&addonSounds.giga_laugh2}, 0U},
            {FoleyType::FOLEY_GIGA_LAUGH3, 0.0f, {&addonSounds.giga_laugh3}, 0U},
            {FoleyType::FOLEY_TELEPORTATION, 0.0f, {&addonSounds.teleportation}, 0U},
            {FoleyType::FOLEY_ICEBERG, 0.0f, {&addonSounds.iceberg}, 0U},
            {FoleyType::FOLEY_CELERY_STALKER_RISE, 0.0f, {&addonSounds.celery_stalker_rise}, 0U},
            {FoleyType::FOLEY_CELERY_STALKER_ATTACK, 0.0f, {&addonSounds.celery_stalker_attack}, 0U},
            {FoleyType::FOLEY_BLOOMERANG, 0.0f, {&addonSounds.bloomerang}, 0U},
            {FoleyType::FOLEY_BONK_CHOY_PUNCH, 5.0f, {&addonSounds.bonk_choy_punch}, 0U},
            {FoleyType::FOLEY_BONK_CHOY_UPPERCUT, 0.0f, {&addonSounds.bonk_choy_uppercut}, 0U},
            {FoleyType::FOLEY_RED_STINGER_PRIMARY, 0.0f, {&addonSounds.red_stinger_primary}, 0U},
            {FoleyType::FOLEY_RED_STINGER_SECONDARY, 0.0f, {&addonSounds.red_stinger_secondary}, 0U},
        };
        std::ranges::copy(extendedArray, newArray + FoleyType::NUM_FOLEY);
    });

    return newArray;
}
