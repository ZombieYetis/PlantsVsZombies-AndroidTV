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

#ifndef PVZ_LAWN_SYSTEM_REANIMATON_LAWN_H
#define PVZ_LAWN_SYSTEM_REANIMATON_LAWN_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodList.h"

namespace Sexy {
class Graphics;
class MemoryImage;
} // namespace Sexy

class LawnApp;

class ReanimCacheImageVariation {
public:
    SeedType mSeedType;
    DrawVariation mDrawVariation;
    Sexy::MemoryImage *mImage;
};

using ImageVariationList = TodList<ReanimCacheImageVariation>;

class Reanimation;

class ReanimatorCache {
public:
    static inline Sexy::MemoryImage *msExtendedPlantImages[EXTENDED_NUM_SEED_TYPES];
    static inline Sexy::MemoryImage *msExtendedZombieImages[EXTENDED_NUM_ZOMBIE_TYPES - NUM_CACHED_ZOMBIE_TYPES];

    ImageVariationList mImageVariationList;
    Sexy::MemoryImage *mPlantImages[SeedType::NUM_SEED_TYPES];             // 4 ~ 57
    Sexy::MemoryImage *mLawnMowers[LawnMowerType::NUM_MOWER_TYPES];        // 58 ~ 61
    Sexy::MemoryImage *mZombieImages[ZombieType::NUM_CACHED_ZOMBIE_TYPES]; // 62 ~ 97
    LawnApp *mApp;                                                         // 98
    // 大小99个整数

    Sexy::MemoryImage *MakeCachedPlantFrame(SeedType theSeedType, DrawVariation theDrawVariation) {
        return reinterpret_cast<Sexy::MemoryImage *(*)(ReanimatorCache *, SeedType, DrawVariation)>(ReanimatorCache_MakeCachedPlantFrameAddr)(this, theSeedType, theDrawVariation);
    }
    Sexy::MemoryImage *MakeBlankCanvasImage(int x, int y) {
        return reinterpret_cast<Sexy::MemoryImage *(*)(ReanimatorCache *, int, int)>(ReanimatorCache_MakeBlankCanvasImageAddr)(this, x, y);
    }
    void DrawReanimatorFrame(Sexy::Graphics *g, float thePosX, float thePosY, ReanimationType theReanimationType, const char *theTrackName, DrawVariation theDrawVariation) {
        reinterpret_cast<Sexy::MemoryImage *(*)(ReanimatorCache *, Sexy::Graphics *, float, float, ReanimationType, const char *, DrawVariation)>(ReanimatorCache_DrawReanimatorFrameAddr)(
            this, g, thePosX, thePosY, theReanimationType, theTrackName, theDrawVariation);
    }

    void ReanimatorCacheInitialize();
    void ReanimatorCacheDispose();
    void GetPlantImageSize(SeedType theSeedType, int &theOffsetX, int &theOffsetY, int &theWidth, int &theHeight);
    void UpdateReanimationForVariation(Reanimation *theReanim, DrawVariation theDrawVariation);
    void LoadCachedImages();
    Sexy::MemoryImage *MakeCachedZombieFrame(ZombieType theZombieType);
    void DrawCachedPlant(Sexy::Graphics *g, float thePosX, float thePosY, SeedType theSeedType, DrawVariation theDrawVariation);
    void DrawCachedExtendedPlant(Sexy::Graphics *g, float thePosX, float thePosY, SeedType theSeedType, DrawVariation theDrawVariation);
    void DrawCachedZombie(Sexy::Graphics *g, float thePosX, float thePosY, ZombieType theZombieType);
    void DrawCachedExtendedZombie(Sexy::Graphics *g, float thePosX, float thePosY, ZombieType theZombieType);
    Sexy::MemoryImage *MakeBlankMemoryImage(int theWidth, int theHeight);
};

inline void (*old_ReanimatorCache_LoadCachedImages)(ReanimatorCache *a1);

inline void (*old_ReanimatorCache_UpdateReanimationForVariation)(ReanimatorCache *a1, Reanimation *a, DrawVariation theDrawVariation);

inline void (*old_ReanimatorCache_DrawCachedPlant)(ReanimatorCache *a1, Sexy::Graphics *graphics, float thePosX, float thePosY, SeedType theSeedType, DrawVariation drawVariation);

#endif // PVZ_LAWN_SYSTEM_REANIMATON_LAWN_H
