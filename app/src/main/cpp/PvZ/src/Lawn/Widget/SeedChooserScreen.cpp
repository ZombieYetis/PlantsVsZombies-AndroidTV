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

#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/Widget/AlmanacDialog.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/ImitaterDialog.h"
#include "PvZ/Lawn/Widget/StoreScreen.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Misc/MTRand.h"
#include "PvZ/SexyAppFramework/SexyAppBase.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "VSActionAIDraftPolicy.h"

#include <unistd.h>

#include <climits>
#include <cstddef>
#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <vector>

using namespace Sexy;

namespace {
SeedChooserScreen *gSeedChooserTouchOwner = nullptr;
constexpr uint32_t kSeedChooserDragSyncIntervalMs = 50;
constexpr uintptr_t kSeedChooserButtonListenerVtableOffset = 0x20C;
SeedType gLastDragSyncSeedType[2][2] = {{SeedType::SEED_NONE, SeedType::SEED_NONE}, {SeedType::SEED_NONE, SeedType::SEED_NONE}};
uint32_t gLastDragSyncTickMs[2][2] = {{0, 0}, {0, 0}};

bool HasBuiltinAIOpponentLobbedPressure(SeedChooserScreen *screen);

bool IsLocalChooserInputAllowed(SeedChooserScreen *screen) {
    if (!screen->mApp->IsVSMode()) {
        return true;
    }

    VSSetupMenu *vsSetup = screen->mApp->mVSSetupMenu;
    if (vsSetup == nullptr) {
        return true;
    }

    VSSide chooserSide = screen->mIsZombieChooser ? VSSide::VS_SIDE_ZOMBIE : VSSide::VS_SIDE_PLANT;
    VSSide localSide = VSSide::VS_SIDE_NONE;
    if (gTcpConnected) {
        localSide = vsSetup->mSides[1];
    } else if (gTcpClientSocket >= 0) {
        localSide = vsSetup->mSides[0];
    } else {
        return true;
    }

    VSSide controlledSide = localSide;
    if (screen->mBanningPhase) {
        if (localSide == VSSide::VS_SIDE_PLANT) {
            controlledSide = VSSide::VS_SIDE_ZOMBIE;
        } else if (localSide == VSSide::VS_SIDE_ZOMBIE) {
            controlledSide = VSSide::VS_SIDE_PLANT;
        }
    }
    return controlledSide == chooserSide;
}

bool IsExtendedSeedsModeEnabled(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mApp == nullptr || !screen->mApp->IsVSMode()) {
        return false;
    }

    VSSetupMenu *vsSetup = screen->mApp->mVSSetupMenu;
    if (vsSetup == nullptr || vsSetup->mAddonWidget == nullptr) {
        return false;
    }

    return vsSetup->mAddonWidget->mExtendedSeedsMode;
}

int GetZombieFirstPageSeedCount(const SeedChooserScreen *screen) {
    const SeedType lastSeedType = screen->mShowExtendedSeeds ? SeedType::SEED_ZOMBIE_BALLOON : SeedType::SEED_ZOMBIE_GARGANTUAR;
    return int(lastSeedType) - int(SeedType::SEED_ZOMBIE_GRAVESTONE) + 1;
}

SeedType GetZombieFirstPageLastSeedType(const SeedChooserScreen *screen) {
    return screen->mShowExtendedSeeds ? SeedType::SEED_ZOMBIE_BALLOON : SeedType::SEED_ZOMBIE_GARGANTUAR;
}

void NormalizeLocalPoint(SeedChooserScreen *screen, int &x, int &y) {
    // Some platforms/reporting paths send global coordinates; normalize to widget-local.
    if (x < 0 || x >= screen->mWidth || y < 0 || y >= screen->mHeight) {
        x -= screen->mX;
        y -= screen->mY;
    }
}

int GetZombieIndexBySeedType(SeedType theSeedType) {
    return theSeedType >= SeedType::SEED_ZOMBIE_GRAVESTONE ? SeedType(theSeedType - SeedType::SEED_ZOMBIE_GRAVESTONE) : SeedType::SEED_NONE;
}

bool IsLocalBuiltinAIChooser(const SeedChooserScreen *screen) {
    return screen != nullptr && screen->mApp != nullptr && screen->mApp->IsVSMode() && !gTcpConnected && gTcpClientSocket < 0 && !gIsReplayMode && !gIsServerModeSpectator;
}

bool IsChooserFilled(const SeedChooserScreen *screen) {
    return screen != nullptr && screen->mSeedBank1 != nullptr && screen->mSeedsInFlight == 0 && screen->mSeedsInBank >= screen->mSeedBank1->mNumPackets;
}

constexpr int kBuiltinAIBaseDeckSize = 6;
constexpr int kBuiltinAIMaxDeckSize = 7;

struct BuiltinAIPlantProfile {
    int id = -1;
    SeedType dayDeck[kBuiltinAIMaxDeckSize]{};
    SeedType nightDeck[kBuiltinAIMaxDeckSize]{};
    int weight = 0;
    SeedType mainCarry = SeedType::SEED_NONE;

    const SeedType *Deck(bool isNight) const {
        return isNight ? nightDeck : dayDeck;
    }
};

// Each record keeps both map variants, selection weight, and declared carry
// together. A new replay profile cannot silently inherit adjacent metadata.
static constexpr std::array<BuiltinAIPlantProfile, 43> kBuiltinAIPlantProfiles = {{
    {0,
     {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_SUNSHROOM},
     {SEED_SUNSHROOM, SEED_PEASHOOTER, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     5,
     SEED_PEASHOOTER},
    {1,
     {SEED_SUNFLOWER, SEED_REPEATER, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     {SEED_SUNSHROOM, SEED_REPEATER, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     5,
     SEED_REPEATER},
    {2,
     {SEED_SUNFLOWER, SEED_SNOWPEA, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_JALAPENO},
     {SEED_SUNSHROOM, SEED_SNOWPEA, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_JALAPENO},
     4,
     SEED_SNOWPEA},
    {3,
     {SEED_SUNFLOWER, SEED_SCAREDYSHROOM, SEED_INSTANT_COFFEE, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB},
     {SEED_SUNSHROOM, SEED_SCAREDYSHROOM, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     3,
     SEED_SCAREDYSHROOM},
    {4,
     {SEED_SUNFLOWER, SEED_MELONPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_WALLNUT},
     {SEED_SUNSHROOM, SEED_MELONPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_WALLNUT},
     3,
     SEED_MELONPULT},
    {5,
     {SEED_SUNFLOWER, SEED_BLOOMERANG, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_CELERY_STALKER},
     {SEED_SUNSHROOM, SEED_BLOOMERANG, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_CELERY_STALKER},
     2,
     SEED_BLOOMERANG},
    {6,
     {SEED_SUNFLOWER, SEED_CABBAGEPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_JALAPENO},
     {SEED_SUNSHROOM, SEED_CABBAGEPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_JALAPENO},
     2,
     SEED_CABBAGEPULT},
    {7,
     {SEED_SUNFLOWER, SEED_SPORESHROOM, SEED_INSTANT_COFFEE, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB},
     {SEED_SUNSHROOM, SEED_SPORESHROOM, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     2,
     SEED_SPORESHROOM},
    {8,
     {SEED_SUNFLOWER, SEED_KERNELPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     {SEED_SUNSHROOM, SEED_KERNELPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_ICEBERG_LETTUCE},
     1,
     SEED_KERNELPULT},
    {9,
     {SEED_SUNFLOWER, SEED_STARFRUIT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_PUMPKINSHELL},
     {SEED_SUNSHROOM, SEED_STARFRUIT, SEED_POTATOMINE, SEED_SQUASH, SEED_CHERRYBOMB, SEED_PUMPKINSHELL},
     2,
     SEED_STARFRUIT},
    {10,
     {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_DOOMSHROOM, SEED_CHILLY_PEPPER, SEED_HYPNOSHROOM, SEED_INSTANT_COFFEE},
     {SEED_SUNSHROOM, SEED_PEASHOOTER, SEED_DOOMSHROOM, SEED_CHILLY_PEPPER, SEED_HYPNOSHROOM, SEED_POTATOMINE},
     3,
     SEED_PEASHOOTER},
    {11,
     {SEED_SUNFLOWER, SEED_SPORESHROOM, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_WALLNUT, SEED_SPIKEWEED},
     {SEED_SUNSHROOM, SEED_SPORESHROOM, SEED_PUFFSHROOM, SEED_WALLNUT, SEED_SPIKEWEED, SEED_POTATOMINE},
     2,
     SEED_SPORESHROOM},
    {12,
     {SEED_SUNFLOWER, SEED_FUMESHROOM, SEED_INSTANT_COFFEE, SEED_WALLNUT, SEED_IMP_PEAR, SEED_DOOMSHROOM},
     {SEED_SUNSHROOM, SEED_FUMESHROOM, SEED_WALLNUT, SEED_IMP_PEAR, SEED_POTATOMINE, SEED_SQUASH},
     1,
     SEED_FUMESHROOM},
    {13,
     {SEED_SUNFLOWER, SEED_SNOWPEA, SEED_BONK_CHOY, SEED_WALLNUT, SEED_SQUASH, SEED_IMP_PEAR},
     {SEED_SUNSHROOM, SEED_SNOWPEA, SEED_BONK_CHOY, SEED_WALLNUT, SEED_SQUASH, SEED_IMP_PEAR},
     2,
     SEED_SNOWPEA},
    {14,
     {SEED_SUNFLOWER, SEED_STARFRUIT, SEED_GARLIC, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_POTATOMINE},
     {SEED_SUNSHROOM, SEED_STARFRUIT, SEED_GARLIC, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_POTATOMINE},
     1,
     SEED_STARFRUIT},
    {15,
     {SEED_SUNFLOWER, SEED_REPEATER, SEED_WALLNUT, SEED_IMP_PEAR, SEED_SQUASH, SEED_SUNSHROOM},
     {SEED_SUNSHROOM, SEED_REPEATER, SEED_WALLNUT, SEED_IMP_PEAR, SEED_SQUASH, SEED_POTATOMINE},
     2,
     SEED_REPEATER},
    {16,
     {SEED_SUNFLOWER, SEED_KERNELPULT, SEED_CELERY_STALKER, SEED_POTATOMINE, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_JALAPENO},
     {SEED_SUNSHROOM, SEED_KERNELPULT, SEED_CELERY_STALKER, SEED_POTATOMINE, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_JALAPENO},
     1,
     SEED_KERNELPULT},
    {17,
     {SEED_SUNFLOWER, SEED_MELONPULT, SEED_POTATOMINE, SEED_SQUASH, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_JALAPENO},
     {SEED_SUNSHROOM, SEED_MELONPULT, SEED_SCAREDYSHROOM, SEED_POTATOMINE, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_JALAPENO},
     2,
     SEED_MELONPULT},
    {18,
     {SEED_SUNFLOWER, SEED_SCAREDYSHROOM, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_DOOMSHROOM, SEED_SPIKEWEED},
     {SEED_SUNSHROOM, SEED_SCAREDYSHROOM, SEED_PUFFSHROOM, SEED_DOOMSHROOM, SEED_SPIKEWEED, SEED_WALLNUT},
     1,
     SEED_SCAREDYSHROOM},
    {19,
     {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_SUNSHROOM, SEED_ICEBERG_LETTUCE},
     {SEED_SUNSHROOM, SEED_PEASHOOTER, SEED_PUFFSHROOM, SEED_ICEBERG_LETTUCE, SEED_POTATOMINE, SEED_WALLNUT},
     1,
     SEED_PEASHOOTER},
    {20,
     {SEED_SUNFLOWER, SEED_CACTUS, SEED_SPIKEWEED, SEED_POTATOMINE, SEED_WALLNUT, SEED_SQUASH},
     {SEED_SUNSHROOM, SEED_CACTUS, SEED_SPIKEWEED, SEED_POTATOMINE, SEED_WALLNUT, SEED_SQUASH},
     1,
     SEED_CACTUS},
    {21,
     {SEED_SUNFLOWER, SEED_THREEPEATER, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_WALLNUT, SEED_CHILLY_PEPPER},
     {SEED_SUNSHROOM, SEED_THREEPEATER, SEED_PUFFSHROOM, SEED_WALLNUT, SEED_CHILLY_PEPPER, SEED_POTATOMINE},
     1,
     SEED_THREEPEATER},
    {22,
     {SEED_SUNFLOWER, SEED_SNOWPEA, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_POTATOMINE, SEED_ICEBERG_LETTUCE},
     {SEED_SUNSHROOM, SEED_SNOWPEA, SEED_PUFFSHROOM, SEED_POTATOMINE, SEED_ICEBERG_LETTUCE, SEED_WALLNUT},
     1,
     SEED_SNOWPEA},
    {23,
     {SEED_SUNFLOWER, SEED_REPEATER, SEED_CELERY_STALKER, SEED_JALAPENO, SEED_POTATOMINE, SEED_WALLNUT},
     {SEED_SUNSHROOM, SEED_REPEATER, SEED_CELERY_STALKER, SEED_JALAPENO, SEED_POTATOMINE, SEED_WALLNUT},
     2,
     SEED_REPEATER},
    {24,
     {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_ICEBERG_LETTUCE, SEED_WALLNUT},
     {SEED_SUNSHROOM, SEED_PEASHOOTER, SEED_PUFFSHROOM, SEED_ICEBERG_LETTUCE, SEED_POTATOMINE, SEED_WALLNUT},
     2,
     SEED_PEASHOOTER},
    {25,
     {SEED_SUNFLOWER, SEED_FUMESHROOM, SEED_DOOMSHROOM, SEED_INSTANT_COFFEE, SEED_CHILLY_PEPPER, SEED_WALLNUT},
     {SEED_SUNSHROOM, SEED_FUMESHROOM, SEED_DOOMSHROOM, SEED_CHILLY_PEPPER, SEED_POTATOMINE, SEED_WALLNUT},
     2,
     SEED_FUMESHROOM},
    {26,
     {SEED_SUNFLOWER, SEED_SNOWPEA, SEED_BONK_CHOY, SEED_SQUASH, SEED_WALLNUT, SEED_CHILLY_PEPPER},
     {SEED_SUNSHROOM, SEED_SNOWPEA, SEED_BONK_CHOY, SEED_SQUASH, SEED_WALLNUT, SEED_CHILLY_PEPPER},
     2,
     SEED_SNOWPEA},
    {27,
     {SEED_SUNFLOWER, SEED_STARFRUIT, SEED_CHOMPER, SEED_CHERRYBOMB, SEED_WALLNUT, SEED_POTATOMINE},
     {SEED_SUNSHROOM, SEED_STARFRUIT, SEED_CHOMPER, SEED_CHERRYBOMB, SEED_WALLNUT, SEED_POTATOMINE},
     1,
     SEED_STARFRUIT},
    {28,
     {SEED_SUNFLOWER, SEED_BLOOMERANG, SEED_DOOMSHROOM, SEED_HYPNOSHROOM, SEED_INSTANT_COFFEE, SEED_CHERRYBOMB, SEED_WALLNUT},
     {SEED_SUNSHROOM, SEED_BLOOMERANG, SEED_DOOMSHROOM, SEED_HYPNOSHROOM, SEED_CHERRYBOMB, SEED_POTATOMINE, SEED_WALLNUT},
     1,
     SEED_BLOOMERANG},
    {29,
     {SEED_SUNFLOWER, SEED_SPORESHROOM, SEED_PUMPKINSHELL, SEED_SQUASH, SEED_INSTANT_COFFEE, SEED_CHERRYBOMB},
     {SEED_SUNSHROOM, SEED_SPORESHROOM, SEED_PUMPKINSHELL, SEED_SQUASH, SEED_CHERRYBOMB, SEED_POTATOMINE},
     1,
     SEED_SPORESHROOM},
    {30,
     {SEED_SUNFLOWER, SEED_SNOWPEA, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_ICEBERG_LETTUCE, SEED_MAGNETSHROOM, SEED_POTATOMINE},
     {SEED_SUNSHROOM, SEED_SNOWPEA, SEED_PUFFSHROOM, SEED_ICEBERG_LETTUCE, SEED_POTATOMINE, SEED_MAGNETSHROOM},
     1,
     SEED_SNOWPEA},
    {31,
     {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_ICEBERG_LETTUCE, SEED_SUNSHROOM},
     {SEED_SUNSHROOM, SEED_PEASHOOTER, SEED_PUFFSHROOM, SEED_ICEBERG_LETTUCE, SEED_POTATOMINE, SEED_WALLNUT},
     2,
     SEED_PEASHOOTER},
    {32,
     {SEED_SUNFLOWER, SEED_REPEATER, SEED_CELERY_STALKER, SEED_JALAPENO, SEED_WALLNUT, SEED_SUNSHROOM},
     {SEED_SUNSHROOM, SEED_REPEATER, SEED_CELERY_STALKER, SEED_JALAPENO, SEED_WALLNUT, SEED_POTATOMINE},
     2,
     SEED_REPEATER},
    {33,
     {SEED_SUNFLOWER, SEED_POTATOMINE, SEED_PUFFSHROOM, SEED_SNOWPEA, SEED_INSTANT_COFFEE, SEED_ICEBERG_LETTUCE},
     {SEED_SUNSHROOM, SEED_POTATOMINE, SEED_PUFFSHROOM, SEED_SNOWPEA, SEED_ICEBERG_LETTUCE, SEED_WALLNUT},
     2,
     SEED_SNOWPEA},
    {34,
     {SEED_SUNFLOWER, SEED_SPORESHROOM, SEED_PUFFSHROOM, SEED_SCAREDYSHROOM, SEED_INSTANT_COFFEE, SEED_DOOMSHROOM},
     {SEED_SUNSHROOM, SEED_SPORESHROOM, SEED_PUFFSHROOM, SEED_SCAREDYSHROOM, SEED_DOOMSHROOM, SEED_WALLNUT},
     2,
     SEED_SPORESHROOM},
    {35,
     {SEED_SUNFLOWER, SEED_MELONPULT, SEED_POTATOMINE, SEED_WALLNUT, SEED_CHILLY_PEPPER, SEED_SQUASH},
     {SEED_SUNSHROOM, SEED_MELONPULT, SEED_POTATOMINE, SEED_WALLNUT, SEED_CHILLY_PEPPER, SEED_SQUASH},
     2,
     SEED_MELONPULT},
    {36,
     {SEED_SUNFLOWER, SEED_REPEATER, SEED_SUNSHROOM, SEED_WALLNUT, SEED_SQUASH, SEED_CHILLY_PEPPER},
     {SEED_SUNSHROOM, SEED_REPEATER, SEED_WALLNUT, SEED_SQUASH, SEED_CHILLY_PEPPER, SEED_POTATOMINE},
     3,
     SEED_REPEATER},
    {37,
     {SEED_SUNFLOWER, SEED_STARFRUIT, SEED_PUFFSHROOM, SEED_INSTANT_COFFEE, SEED_WALLNUT, SEED_CHERRYBOMB},
     {SEED_SUNSHROOM, SEED_STARFRUIT, SEED_PUFFSHROOM, SEED_WALLNUT, SEED_CHERRYBOMB, SEED_POTATOMINE},
     1,
     SEED_STARFRUIT},
    {38, {SEED_BLOOMERANG, SEED_PEASHOOTER, SEED_SQUASH, SEED_SPIKEWEED, SEED_PUMPKINSHELL}, {SEED_BLOOMERANG, SEED_PEASHOOTER, SEED_SQUASH, SEED_SPIKEWEED, SEED_PUMPKINSHELL}, 1, SEED_BLOOMERANG},
    {39, {SEED_REPEATER, SEED_CHOMPER, SEED_CHILLY_PEPPER, SEED_WALLNUT, SEED_SUNSHROOM}, {SEED_REPEATER, SEED_CHOMPER, SEED_CHILLY_PEPPER, SEED_WALLNUT, SEED_SUNSHROOM}, 1, SEED_REPEATER},
    {40,
     {SEED_SUNFLOWER, SEED_BLOOMERANG, SEED_WALLNUT, SEED_SQUASH, SEED_SUNSHROOM, SEED_GARLIC},
     {SEED_SUNSHROOM, SEED_BLOOMERANG, SEED_WALLNUT, SEED_SQUASH, SEED_POTATOMINE, SEED_GARLIC},
     2,
     SEED_BLOOMERANG},
    {41,
     {SEED_SUNFLOWER, SEED_SCAREDYSHROOM, SEED_INSTANT_COFFEE, SEED_ICEBERG_LETTUCE, SEED_BONK_CHOY, SEED_HYPNOSHROOM},
     {SEED_SUNSHROOM, SEED_SCAREDYSHROOM, SEED_ICEBERG_LETTUCE, SEED_BONK_CHOY, SEED_HYPNOSHROOM, SEED_WALLNUT},
     1,
     SEED_SCAREDYSHROOM},
    {42,
     {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_ICEBERG_LETTUCE, SEED_PUMPKINSHELL, SEED_CHERRYBOMB, SEED_JALAPENO},
     {SEED_SUNSHROOM, SEED_PEASHOOTER, SEED_ICEBERG_LETTUCE, SEED_PUMPKINSHELL, SEED_CHERRYBOMB, SEED_JALAPENO},
     2,
     SEED_PEASHOOTER},
}};

const BuiltinAIPlantProfile *GetBuiltinAIPlantProfile(int profile) {
    return profile >= 0 && profile < static_cast<int>(kBuiltinAIPlantProfiles.size()) ? &kBuiltinAIPlantProfiles[static_cast<std::size_t>(profile)] : nullptr;
}

static constexpr SeedType kBuiltinAIZombieDecks[][kBuiltinAIMaxDeckSize] = {
    {SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_SCREEN_DOOR, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_GIGA_FOOTBALL, SEED_ZOMBIE_GRAVESTONE},
    {SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_IMP, SEED_ZOMBONI, SEED_ZOMBIE_SCREEN_DOOR, SEED_ZOMBIE_MOUND, SEED_ZOMBIE_TRASHCAN},
    {SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_IMP, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_GRAVESTONE},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_CATAPULT, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_BUNGEE},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_PEA_HEAD, SEED_ZOMBIE_IMP, SEED_ZOMBIE_SUNDAY_EDITION, SEED_ZOMBIE_GARGANTUAR},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_GIGA_GARGANTUAR, SEED_ZOMBIE_CATAPULT},
    {SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_FLAG, SEED_ZOMBIE_SQUASH_HEAD, SEED_ZOMBIE_SCREEN_DOOR, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_GIGA_GARGANTUAR},
    {SEED_ZOMBIE_IMP, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_SCREEN_DOOR, SEED_ZOMBIE_CATAPULT, SEED_ZOMBIE_GARGANTUAR},
    {SEED_ZOMBIE_SQUASH_HEAD, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_SCREEN_DOOR, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_GIGA_FOOTBALL, SEED_ZOMBIE_SUPER_FAN_IMP},
    {SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_TALLNUT_HEAD, SEED_ZOMBIE_MOUND},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_GIGA_GARGANTUAR, SEED_ZOMBIE_DOGWALKER, SEED_ZOMBIE_FOOTBALL},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_CATAPULT, SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_GARGANTUAR},
    // Replay fast-pressure template. The battle scorer still fans these
    // probes across economy lanes and rejects its metal screens versus pults.
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBONI, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_DOGWALKER},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_MOUND, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_IMP, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBONI},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_LADDER, SEED_ZOMBIE_GIGA_POLEVAULTER, SEED_ZOMBIE_NORMAL},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_DIGGER, SEED_ZOMBIE_GIGA_GARGANTUAR, SEED_ZOMBIE_TRAFFIC_CONE},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_IMP, SEED_ZOMBONI, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_GIGA_POLEVAULTER},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_PEA_HEAD, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_TRASHCAN},
    // Recent recordings use these as distinct grave-economy profiles:
    // pail/imp/sled multi-lane pressure, pea-head/zomboni with a giant
    // finisher, and a cone/pail/polevaulter breakthrough plan.
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_IMP, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_SUNDAY_EDITION, SEED_ZOMBIE_SCREEN_DOOR},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_PEA_HEAD, SEED_ZOMBONI, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_TRASHCAN},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBONI, SEED_ZOMBIE_IMP, SEED_ZOMBIE_GIGA_POLEVAULTER},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_DOGWALKER, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_IMP, SEED_ZOMBIE_SUNDAY_EDITION},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_DOGWALKER, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_GIGA_FOOTBALL},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_LADDER, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_GIGA_POLEVAULTER, SEED_ZOMBIE_NORMAL},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_IMP, SEED_ZOMBIE_PEA_HEAD, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_SUNDAY_EDITION, SEED_ZOMBIE_GARGANTUAR},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_PEA_HEAD},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_IMP, SEED_ZOMBONI, SEED_ZOMBIE_GARGANTUAR},
    // These three are exact replay profiles. Their battle logic still
    // requires lane spread, legal targets and Ash awareness before using
    // their early heavy release or their Bobsled transition.
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_LADDER, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_GIGA_POLEVAULTER},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_DIGGER, SEED_ZOMBIE_GIGA_GARGANTUAR},
    {SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_SUPER_FAN_IMP, SEED_ZOMBIE_GIGA_FOOTBALL, SEED_ZOMBIE_GIGA_POLEVAULTER, SEED_ZOMBIE_DOGWALKER},
    // Ladder/Football recordings use the Imp probe first, then convert a
    // developed Wall-nut lane with Ladder and a heavy runner. The scorer
    // still rejects the ladder when no nut target exists and keeps Door away
    // from lobbed plants.
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_IMP, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_LADDER, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_SCREEN_DOOR},
    // The Pea-head/Flag recording is a ranged, multi-lane plan. The
    // gravestone remains available as the built-in zombie economy card;
    // keep the six recorded combat choices intact instead of replacing the
    // Flag or Bungee with another generic grave screen.
    {SEED_ZOMBIE_PEA_HEAD, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_FLAG},
    // Five-card Ban-residual fast attack and giant-finisher profiles.
    {SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_PEA_HEAD, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_ZOMBLOB, SEED_ZOMBIE_GIGA_GARGANTUAR},
    {SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_IMP, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_FOOTBALL},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_SUNDAY_EDITION, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_POLEVAULTER, SEED_ZOMBIE_TALLNUT_HEAD},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_IMP, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_GIGA_GARGANTUAR},
    {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_LADDER, SEED_ZOMBONI, SEED_ZOMBIE_JACK_IN_THE_BOX},
};

std::size_t GetBuiltinAIPlanSize(const SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mSeedBank1 == nullptr || screen->mSeedBank1->mNumPackets <= 0) {
        return kBuiltinAIBaseDeckSize;
    }
    return static_cast<std::size_t>(screen->mSeedBank1->mNumPackets >= kBuiltinAIMaxDeckSize ? kBuiltinAIMaxDeckSize : screen->mSeedBank1->mNumPackets);
}

using BuiltinAIDeckPlans = vsai::draft::BuiltinAIDraftSession;

BuiltinAIDeckPlans &BuiltinAIPlans() {
    return vsai::draft::GetBuiltinAIDraftSession();
}

int PickBuiltinAIPlantProfile() {
    // The strongest replay-compatible templates use a tier-one carry and
    // the Potato/Squash/Cherry answer package. Lower-tier carries still
    // occur, but do not drown out those reliable opening plans.
    int totalWeight = 0;
    for (const BuiltinAIPlantProfile &profile : kBuiltinAIPlantProfiles) {
        totalWeight += profile.weight;
    }
    int ticket = Sexy::Rand(totalWeight);
    for (const BuiltinAIPlantProfile &profile : kBuiltinAIPlantProfiles) {
        ticket -= profile.weight;
        if (ticket < 0) {
            return profile.id;
        }
    }
    return kBuiltinAIPlantProfiles.back().id;
}

void EnsureBuiltinAIDeckPlans(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mApp == nullptr) {
        return;
    }

    BuiltinAIDeckPlans &plans = BuiltinAIPlans();
    if (plans.app == screen->mApp && plans.plantProfile >= 0 && plans.zombieProfile >= 0) {
        return;
    }

    plans.app = screen->mApp;
    plans.plantProfile = PickBuiltinAIPlantProfile();
    plans.zombieProfile = Sexy::Rand(static_cast<int>(std::size(kBuiltinAIZombieDecks)));
    // Replay templates are the default human-like opening. Keep a smaller
    // constrained-random path so local AI matches do not repeat a recorded
    // line every game; both paths still pass through the same legality and
    // matchup filters below.
    plans.usePlantTemplate = Sexy::Rand(100) < 80;
    plans.useZombieTemplate = Sexy::Rand(100) < 80;
    vsai::draft::BuiltinAIDraftHistory &history = vsai::draft::GetBuiltinAIDraftHistory();
    // A new chooser is a new match plan. Avoid repeating the same archetype
    // when the engine's deterministic RNG starts consecutive local matches
    // from the same seed.
    if (kBuiltinAIPlantProfiles.size() > 1 && plans.plantProfile == history.lastPlantProfile) {
        plans.plantProfile = PickBuiltinAIPlantProfile();
        if (plans.plantProfile == history.lastPlantProfile) {
            plans.plantProfile = (plans.plantProfile + 1) % static_cast<int>(kBuiltinAIPlantProfiles.size());
        }
    }
    if (std::size(kBuiltinAIZombieDecks) > 1 && plans.zombieProfile == history.lastZombieProfile) {
        plans.zombieProfile = (plans.zombieProfile + 1 + Sexy::Rand(static_cast<int>(std::size(kBuiltinAIZombieDecks) - 1))) % static_cast<int>(std::size(kBuiltinAIZombieDecks));
    }
    history.lastPlantProfile = plans.plantProfile;
    history.lastZombieProfile = plans.zombieProfile;
}

bool UsesBuiltinAITemplate(SeedChooserScreen *screen) {
    EnsureBuiltinAIDeckPlans(screen);
    if (VSSetupAddonWidget::msAITemplateDeckDisabledMode) {
        return false;
    }
    return screen != nullptr && (screen->mIsZombieChooser ? BuiltinAIPlans().useZombieTemplate : BuiltinAIPlans().usePlantTemplate);
}

int BuiltinAIPlantMainPickSlot(SeedChooserScreen *screen) {
    if (screen == nullptr) {
        return kBuiltinAIBaseDeckSize;
    }
    if (BuiltinAIPlans().plantMainPickSlot < 0) {
        const int planSize = static_cast<int>(GetBuiltinAIPlanSize(screen));
        // Preserve one final packet for a response/support card. This keeps
        // the carry timing varied without ever revealing the main C as the
        // last pick in either six- or seven-slot VS selection.
        const int earliestSlot = std::min(3, std::max(1, planSize - 1));
        const int latestSlot = std::max(earliestSlot, planSize - 1);
        BuiltinAIPlans().plantMainPickSlot = earliestSlot + Sexy::Rand(std::max(1, latestSlot - earliestSlot + 1));
    }
    return BuiltinAIPlans().plantMainPickSlot;
}

const SeedType *GetBuiltinAIDeckPriority(SeedChooserScreen *screen) {
    EnsureBuiltinAIDeckPlans(screen);
    if (screen->mIsZombieChooser) {
        return kBuiltinAIZombieDecks[BuiltinAIPlans().zombieProfile];
    }
    const BuiltinAIPlantProfile *profile = GetBuiltinAIPlantProfile(BuiltinAIPlans().plantProfile);
    return profile == nullptr ? nullptr : profile->Deck(screen->mBoard != nullptr && screen->mBoard->StageIsNight());
}

void TryAutoStartBuiltinVSMatch(SeedChooserScreen *screen) {
    if (!IsLocalBuiltinAIChooser(screen) || screen->mApp == nullptr || screen->mApp->mSeedChooserScreen == nullptr || screen->mApp->mZombieChooserScreen == nullptr
        || (!VSSetupAddonWidget::msPlantAIMode && !VSSetupAddonWidget::msZombieAIMode)) {
        return;
    }

    SeedChooserScreen *plantChooser = screen->mApp->mSeedChooserScreen;
    SeedChooserScreen *zombieChooser = screen->mApp->mZombieChooserScreen;
    if (plantChooser->mBanningPhase || zombieChooser->mBanningPhase || !IsChooserFilled(plantChooser) || !IsChooserFilled(zombieChooser)) {
        return;
    }

    // The VS start button is intentionally hidden. Close the two chooser widgets
    // only after both banks have landed, otherwise the second bank can be lost.
    static bool closing = false;
    if (closing) {
        return;
    }
    closing = true;
    if (plantChooser != screen) {
        plantChooser->CloseSeedChooser();
    }
    if (zombieChooser != screen) {
        zombieChooser->CloseSeedChooser();
    }
    screen->CloseSeedChooser();
    closing = false;
}

bool HasBuiltinAIOpponentBalloon(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser || screen->mApp == nullptr || screen->mApp->mZombieChooserScreen == nullptr) {
        return false;
    }

    SeedChooserScreen *zombieScreen = screen->mApp->mZombieChooserScreen;
    const int storageCount = zombieScreen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (zombieScreen->GetChosenSeed(seedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK && zombieScreen->GetZombieSeedType(seedIndex) == SeedType::SEED_ZOMBIE_BALLOON) {
            return true;
        }
    }
    return false;
}

bool HasBuiltinAIPlantSeed(SeedChooserScreen *screen, SeedType seedType);

bool IsBuiltinAICandidate(SeedChooserScreen *screen, SeedType seedType) {
    if (seedType == SeedType::SEED_NONE || seedType == SeedType::SEED_IMITATER) {
        return false;
    }
    // Upgrade plants are not legal VS cards, including during the opponent's
    // Ban phase. Keep this before both priority and generic-card fallbacks.
    if (!screen->mIsZombieChooser && Plant::IsUpgrade(seedType)) {
        return false;
    }
    // Gravebuster and Sweet Potato are intentionally not part of the built-in
    // plant AI deck. Both are narrow tactical cards, but there is no replay-
    // backed Sweet Potato routing plan yet, so generic selection only turns
    // it into weak filler. They remain valid Ban targets.
    if (!screen->mBanningPhase && !screen->mIsZombieChooser && (seedType == SeedType::SEED_GRAVEBUSTER || seedType == SeedType::SEED_SWEET_POTATO)) {
        return false;
    }
    if (!screen->mBanningPhase && !screen->mIsZombieChooser && (seedType == SeedType::SEED_TALLNUT || (seedType == SeedType::SEED_BLOVER && !HasBuiltinAIOpponentBalloon(screen)))) {
        return false;
    }
    if (!screen->mShowExtendedSeeds && ((screen->mIsZombieChooser && seedType > SeedType::SEED_ZOMBIE_GARGANTUAR) || (!screen->mIsZombieChooser && seedType >= SeedType::SEED_ICEBERG_LETTUCE))) {
        return false;
    }

    // A Door/Newspaper/Trashcan is a direct-fire screen. It is a bad deck
    // choice against a plant-side pult carry, even before battle scoring.
    if (!screen->mBanningPhase && screen->mIsZombieChooser && HasBuiltinAIOpponentLobbedPressure(screen)
        && (seedType == SeedType::SEED_ZOMBIE_SCREEN_DOOR || seedType == SeedType::SEED_ZOMBIE_NEWSPAPER || seedType == SeedType::SEED_ZOMBIE_TRASHCAN)) {
        return false;
    }

    // Sun-shroom is a cheap front-line pad. Squash Head and Football
    // both give that pad an efficient trade, so do not draft either after
    // the plant side has already committed to Sun-shroom.
    if (!screen->mBanningPhase && screen->mIsZombieChooser && (seedType == SeedType::SEED_ZOMBIE_SQUASH_HEAD || seedType == SeedType::SEED_ZOMBIE_FOOTBALL) && screen->mApp != nullptr
        && HasBuiltinAIPlantSeed(screen->mApp->mSeedChooserScreen, SeedType::SEED_SUNSHROOM)) {
        return false;
    }

    // Spikeweed and Spikerock hard-counter the Zomboni path. Once the plant
    // player has committed either card, do not let a replay template spend a
    // zombie slot on a vehicle that its opponent already answers directly.
    if (!screen->mBanningPhase && screen->mIsZombieChooser && seedType == SeedType::SEED_ZOMBONI && screen->mApp != nullptr && screen->mApp->mSeedChooserScreen != nullptr) {
        SeedChooserScreen *plantScreen = screen->mApp->mSeedChooserScreen;
        const int plantStorageCount = plantScreen->GetSeedStorageCount();
        for (int seedIndex = 0; seedIndex < plantStorageCount; ++seedIndex) {
            if (plantScreen->GetChosenSeed(seedIndex).mSeedState != ChosenSeedState::SEED_IN_BANK) {
                continue;
            }
            const SeedType plantSeed = plantScreen->GetPlantSeedType(seedIndex);
            if (plantSeed == SeedType::SEED_SPIKEWEED || plantSeed == SeedType::SEED_SPIKEROCK) {
                return false;
            }
        }
    }

    const int seedIndex = screen->GetSeedPacketIndex(seedType);
    if (seedIndex < 0 || seedIndex >= screen->GetSeedStorageCount() || !screen->HasPacket(seedType, screen->mIsZombieChooser)) {
        return false;
    }
    if (screen->GetChosenSeed(seedIndex).mSeedState != ChosenSeedState::SEED_IN_CHOOSER || screen->SeedNotAllowedToPick(seedType)) {
        return false;
    }

    const int bannedSeedIndex = static_cast<int>(seedType);
    return !screen->mBanningPhase || (bannedSeedIndex >= 0 && bannedSeedIndex < NUM_SEEDS_IN_CHOOSER_EXTENDED && screen->mBannedSeed[bannedSeedIndex].mSeedState != BannedSeedState::SEED_BANNED);
}

bool HasBuiltinAIPlantSeed(SeedChooserScreen *screen, SeedType seedType) {
    if (screen == nullptr || screen->mIsZombieChooser || seedType == SeedType::SEED_NONE) {
        return false;
    }

    const int seedIndex = screen->GetSeedPacketIndex(seedType);
    return seedIndex >= 0 && seedIndex < screen->GetSeedStorageCount() && screen->GetChosenSeed(seedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK;
}

bool HasBuiltinAIPlantPeaMain(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser) {
        return false;
    }

    const int storageCount = screen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (screen->GetChosenSeed(seedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK && vsai::draft::IsPeaMainDamageSeed(screen->GetPlantSeedType(seedIndex))) {
            return true;
        }
    }
    return false;
}

bool HasBuiltinAIPlantWakeableMushroom(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser) {
        return false;
    }
    const int storageCount = screen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (screen->GetChosenSeed(seedIndex).mSeedState != ChosenSeedState::SEED_IN_BANK) {
            continue;
        }
        switch (screen->GetPlantSeedType(seedIndex)) {
            case SeedType::SEED_PUFFSHROOM:
            case SeedType::SEED_SCAREDYSHROOM:
            case SeedType::SEED_FUMESHROOM:
            case SeedType::SEED_GLOOMSHROOM:
            case SeedType::SEED_SPORESHROOM:
            case SeedType::SEED_ICESHROOM:
            case SeedType::SEED_DOOMSHROOM:
                return true;
            default:
                break;
        }
    }
    return false;
}

bool IsBuiltinAIDaytimeChooser(const SeedChooserScreen *screen) {
    return screen != nullptr && screen->mBoard != nullptr && !screen->mBoard->StageIsNight();
}

bool CanReserveBuiltinAICoffee(SeedChooserScreen *screen) {
    if (screen == nullptr || !IsBuiltinAIDaytimeChooser(screen) || HasBuiltinAIPlantSeed(screen, SeedType::SEED_INSTANT_COFFEE)) {
        return true;
    }

    const std::size_t planSize = GetBuiltinAIPlanSize(screen);
    const std::size_t selectedCount = static_cast<std::size_t>(std::max(0, screen->mSeedsInBank));
    // A coffee-dependent card occupies this choice. Do not select it unless
    // one further real packet remains for Coffee in both six- and seven-slot
    // games. Candidate legality also covers Bans and map restrictions.
    return selectedCount < planSize && planSize - selectedCount > 1 && IsBuiltinAICandidate(screen, SeedType::SEED_INSTANT_COFFEE);
}

bool HasBuiltinAIOpponentMetalTargets(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser || screen->mApp == nullptr || screen->mApp->mZombieChooserScreen == nullptr) {
        return false;
    }

    SeedChooserScreen *zombieScreen = screen->mApp->mZombieChooserScreen;
    int targetCount = 0;
    const int storageCount = zombieScreen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (zombieScreen->GetChosenSeed(seedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK && vsai::draft::IsMagnetTargetZombieSeed(zombieScreen->GetZombieSeedType(seedIndex))) {
            ++targetCount;
        }
    }

    // A single target is too narrow for a six-card VS deck. The shuffle
    // selector uses the same two-card threshold for Magnet-shroom.
    return targetCount >= 2;
}

bool HasBuiltinAIOpponentLobbedPressure(SeedChooserScreen *screen) {
    if (screen == nullptr || !screen->mIsZombieChooser || screen->mApp == nullptr || screen->mApp->mSeedChooserScreen == nullptr) {
        return false;
    }

    const auto IsPultCarry = [](SeedType seedType) {
        switch (seedType) {
            case SeedType::SEED_CABBAGEPULT:
            case SeedType::SEED_KERNELPULT:
            case SeedType::SEED_MELONPULT:
            case SeedType::SEED_WINTERMELON:
            case SeedType::SEED_COBCANNON:
            case SeedType::SEED_SPORESHROOM:
                return true;
            default:
                return false;
        }
    };

    SeedChooserScreen *plantScreen = screen->mApp->mSeedChooserScreen;
    const int storageCount = plantScreen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (plantScreen->GetChosenSeed(seedIndex).mSeedState != ChosenSeedState::SEED_IN_BANK) {
            continue;
        }
        if (IsPultCarry(plantScreen->GetPlantSeedType(seedIndex))) {
            return true;
        }
    }

    // The two local selectors can fill in either order. When the plant AI
    // has not yet clicked its main card, inspect its already-chosen replay
    // template so the zombie picker cannot race ahead into a dead Door plan.
    if (VSSetupAddonWidget::msPlantAIMode && UsesBuiltinAITemplate(plantScreen)) {
        const SeedType *plannedDeck = GetBuiltinAIDeckPriority(plantScreen);
        for (std::size_t index = 0; index < GetBuiltinAIPlanSize(plantScreen); ++index) {
            if (IsPultCarry(plannedDeck[index])) {
                return true;
            }
        }
    }
    return false;
}

bool HasBuiltinAIPlantMainDamage(SeedChooserScreen *screen);
bool HasBuiltinAIOpponentZombieSeed(SeedChooserScreen *screen, SeedType seedType);

bool IsBuiltinAIPlantSupportCandidate(SeedChooserScreen *screen, SeedType seedType) {
    if (screen == nullptr || screen->mIsZombieChooser) {
        return true;
    }

    if (seedType == SeedType::SEED_UMBRELLA) {
        return HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_BUNGEE) || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_CATAPULT);
    }

    if (seedType == SeedType::SEED_CHOMPER) {
        // Chomper is a close-range answer, not a durable main damage plant.
        // Keep its low-priority template role behind a real carry so a Ban
        // residual cannot open with an all-answer deck.
        return HasBuiltinAIPlantMainDamage(screen);
    }

    if (seedType == SeedType::SEED_TORCHWOOD) {
        // Torchwood amplifies peas, not pults or starfruit. Require the pea
        // main card to be in the bank before allowing the support card. A
        // Snow Pea loses its slow when sent through Torchwood, so that pair
        // is a deck-level conflict rather than a valid pea support package.
        return HasBuiltinAIPlantPeaMain(screen) && !HasBuiltinAIPlantSeed(screen, SeedType::SEED_SNOWPEA);
    }

    if (seedType == SeedType::SEED_SNOWPEA && HasBuiltinAIPlantSeed(screen, SeedType::SEED_TORCHWOOD)) {
        return false;
    }

    if (seedType == SeedType::SEED_INSTANT_COFFEE) {
        // At night every combat mushroom wakes naturally. In daylight Coffee
        // is only valid when it completes a playable mushroom package.
        return IsBuiltinAIDaytimeChooser(screen) && HasBuiltinAIPlantWakeableMushroom(screen);
    }

    if (IsBuiltinAIDaytimeChooser(screen) && vsai::draft::IsCoffeeDependentPlant(seedType)) {
        // Never let a six- or seven-slot fallback split a daytime mushroom
        // from Coffee. This is a deck legality gate, not a scoring preference.
        if (!CanReserveBuiltinAICoffee(screen)) {
            return false;
        }
    }

    if (seedType == SeedType::SEED_MAGNETSHROOM) {
        // Magnet-shroom needs Coffee in daytime and is too narrow when the
        // opponent brings few removable metal targets.
        return (!IsBuiltinAIDaytimeChooser(screen) || CanReserveBuiltinAICoffee(screen)) && HasBuiltinAIOpponentMetalTargets(screen);
    }

    if (seedType == SeedType::SEED_ICEBERG_LETTUCE) {
        // Iceberg is a tempo/support card, not a standalone deck. Select it
        // only after a real damage plant is already in the bank so a Ban or
        // shuffled fallback cannot produce a Bonk/Iceberg-only opening.
        return HasBuiltinAIPlantMainDamage(screen);
    }

    return true;
}

bool HasBuiltinAIPlantMainDamage(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser) {
        return false;
    }
    const int storageCount = screen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (screen->GetChosenSeed(seedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK && vsai::draft::IsPlantCarrySeed(screen->GetPlantSeedType(seedIndex))) {
            return true;
        }
    }
    return false;
}

SeedType PlannedBuiltinAIPlantMainDamageSeed(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser) {
        return SeedType::SEED_NONE;
    }
    const SeedType *deck = GetBuiltinAIDeckPriority(screen);
    for (std::size_t index = 0; index < GetBuiltinAIPlanSize(screen); ++index) {
        if (vsai::draft::IsPlantCarrySeed(deck[index])) {
            return deck[index];
        }
    }
    return SeedType::SEED_NONE;
}

static constexpr SeedType kBuiltinAIPlantMainFallbacks[] = {
    SeedType::SEED_PEASHOOTER,
    SeedType::SEED_REPEATER,
    SeedType::SEED_SCAREDYSHROOM,
    SeedType::SEED_MELONPULT,
    SeedType::SEED_BLOOMERANG,
    SeedType::SEED_CABBAGEPULT,
    SeedType::SEED_SPORESHROOM,
    SeedType::SEED_KERNELPULT,
    SeedType::SEED_STARFRUIT,
    SeedType::SEED_CACTUS,
};

// Rule-random decks deliberately use the high-confidence carries. The
// recorded template selector still retains Cabbage-pult and Kernel-pult;
// this path avoids making either a generic default against an unknown deck.
static constexpr SeedType kBuiltinAIRulePlantMainFallbacks[] = {
    SeedType::SEED_PEASHOOTER,
    SeedType::SEED_REPEATER,
    SeedType::SEED_SCAREDYSHROOM,
    SeedType::SEED_MELONPULT,
    SeedType::SEED_BLOOMERANG,
    SeedType::SEED_SPORESHROOM,
    SeedType::SEED_STARFRUIT,
    SeedType::SEED_CACTUS,
};

static constexpr SeedType kBuiltinAIPlantPostCarryFallbacks[] = {
    SeedType::SEED_CHERRYBOMB,
    SeedType::SEED_JALAPENO,
    SeedType::SEED_SQUASH,
    SeedType::SEED_CHILLY_PEPPER,
    SeedType::SEED_DOOMSHROOM,
    SeedType::SEED_POTATOMINE,
    SeedType::SEED_WALLNUT,
    SeedType::SEED_PUMPKINSHELL,
    SeedType::SEED_SUNSHROOM,
    SeedType::SEED_ICEBERG_LETTUCE,
    SeedType::SEED_SUNFLOWER,
};

bool HasBuiltinAIOpponentZombieSeed(SeedChooserScreen *screen, SeedType seedType) {
    if (screen == nullptr || screen->mIsZombieChooser || screen->mApp == nullptr || screen->mApp->mZombieChooserScreen == nullptr) {
        return false;
    }

    SeedChooserScreen *zombieScreen = screen->mApp->mZombieChooserScreen;
    bool hasChosenZombie = false;
    const int storageCount = zombieScreen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        if (zombieScreen->GetChosenSeed(seedIndex).mSeedState != ChosenSeedState::SEED_IN_BANK) {
            continue;
        }
        hasChosenZombie = true;
        if (zombieScreen->GetZombieSeedType(seedIndex) == seedType) {
            return true;
        }
    }

    // Local AIs can fill their two choosers in either order. Before the
    // zombie side clicks its first card, its retained replay template is the
    // only reliable deck information available to a plant Ban replacement.
    if (!hasChosenZombie && VSSetupAddonWidget::msZombieAIMode && UsesBuiltinAITemplate(zombieScreen)) {
        const SeedType *plannedDeck = GetBuiltinAIDeckPriority(zombieScreen);
        return std::any_of(plannedDeck, plannedDeck + GetBuiltinAIPlanSize(zombieScreen), [seedType](SeedType plannedSeed) { return plannedSeed == seedType; });
    }
    return false;
}

int BuiltinAIPlantCarryMatchupScore(SeedChooserScreen *screen, SeedType seedType) {
    int score = 0;
    switch (seedType) {
        case SeedType::SEED_PEASHOOTER:
            score = 108;
            break;
        case SeedType::SEED_REPEATER:
            score = 112;
            break;
        case SeedType::SEED_SNOWPEA:
            score = 106;
            break;
        case SeedType::SEED_CABBAGEPULT:
            score = 104;
            break;
        case SeedType::SEED_MELONPULT:
            score = 96;
            break;
        case SeedType::SEED_SPORESHROOM:
            score = 102;
            break;
        case SeedType::SEED_BLOOMERANG:
            score = 92;
            break;
        case SeedType::SEED_SCAREDYSHROOM:
            score = 88;
            break;
        case SeedType::SEED_CACTUS:
            score = 76;
            break;
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_STARFRUIT:
            score = 82;
            break;
        default:
            return std::numeric_limits<int>::min();
    }

    const bool metalScreen = HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_SCREEN_DOOR) || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_NEWSPAPER)
        || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_TRASHCAN);
    const bool vehicle = HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBONI) || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_BOBSLED);
    const bool rangedSiege = HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_CATAPULT) || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_PEA_HEAD)
        || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_SUNDAY_EDITION);
    const bool heavy = HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_GARGANTUAR) || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR)
        || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_GIGA_FOOTBALL);
    const bool fastPressure = HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_DOGWALKER) || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_FOOTBALL)
        || HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_IMP);
    const bool balloon = HasBuiltinAIOpponentZombieSeed(screen, SeedType::SEED_ZOMBIE_BALLOON);
    const bool lobbedCarry = seedType == SeedType::SEED_CABBAGEPULT || seedType == SeedType::SEED_MELONPULT || seedType == SeedType::SEED_SPORESHROOM || seedType == SeedType::SEED_KERNELPULT;
    const bool peaCarry = seedType == SeedType::SEED_PEASHOOTER || seedType == SeedType::SEED_REPEATER || seedType == SeedType::SEED_SNOWPEA;

    if (metalScreen) {
        score += lobbedCarry ? 145 : (seedType == SeedType::SEED_BLOOMERANG ? 65 : -35);
    }
    if (vehicle) {
        score += peaCarry ? 70 : (lobbedCarry ? 35 : 0);
    }
    if (rangedSiege) {
        score += lobbedCarry ? 125 : -15;
    }
    if (heavy) {
        score += seedType == SeedType::SEED_MELONPULT ? 150 : (seedType == SeedType::SEED_REPEATER || seedType == SeedType::SEED_SPORESHROOM ? 65 : 0);
    }
    if (fastPressure) {
        score += seedType == SeedType::SEED_SNOWPEA ? 90 : (peaCarry ? 50 : 0);
    }
    if (balloon) {
        score += seedType == SeedType::SEED_CACTUS ? 145 : 0;
    }
    return score;
}

SeedType FindBuiltinAICounterCarry(SeedChooserScreen *screen, bool useTemplateFallbacks = true) {
    SeedType bestSeed = SeedType::SEED_NONE;
    int bestScore = std::numeric_limits<int>::min();
    const std::span<const SeedType> candidates = useTemplateFallbacks ? std::span<const SeedType>(kBuiltinAIPlantMainFallbacks) : std::span<const SeedType>(kBuiltinAIRulePlantMainFallbacks);
    for (const SeedType seedType : candidates) {
        if (!IsBuiltinAICandidate(screen, seedType) || !IsBuiltinAIPlantSupportCandidate(screen, seedType)) {
            continue;
        }
        const int score = BuiltinAIPlantCarryMatchupScore(screen, seedType);
        if (bestSeed == SeedType::SEED_NONE || score > bestScore || (score == bestScore && seedType < bestSeed)) {
            bestSeed = seedType;
            bestScore = score;
        }
    }
    return bestSeed;
}

SeedType FindBuiltinAICandidate(SeedChooserScreen *screen, const SeedType *prioritySeeds, std::size_t priorityCount) {
    if (priorityCount > 0) {
        const std::size_t firstPriority = static_cast<std::size_t>(Sexy::Rand(static_cast<int>(priorityCount)));
        const SeedType candidate =
            vsai::draft::FindRotatedEligibleSeed(std::span<const SeedType>(prioritySeeds, priorityCount), firstPriority, [screen](SeedType seed) { return IsBuiltinAICandidate(screen, seed); });
        if (candidate != SeedType::SEED_NONE) {
            return candidate;
        }
    }

    const int storageCount = screen->GetSeedStorageCount();
    if (storageCount <= 0) {
        return SeedType::SEED_NONE;
    }
    std::vector<SeedType> chooserSeeds;
    chooserSeeds.reserve(static_cast<std::size_t>(storageCount));
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        chooserSeeds.push_back(screen->mIsZombieChooser ? screen->GetZombieSeedType(seedIndex) : screen->GetPlantSeedType(seedIndex));
    }
    return vsai::draft::FindRotatedEligibleSeed(chooserSeeds, static_cast<std::size_t>(Sexy::Rand(storageCount)), [screen](SeedType seed) { return IsBuiltinAICandidate(screen, seed); });
}

int BuiltinAIZombieRuleMatchupScore(SeedChooserScreen *screen, SeedType seedType) {
    if (screen == nullptr || !screen->mIsZombieChooser || screen->mApp == nullptr || screen->mApp->mSeedChooserScreen == nullptr) {
        return std::numeric_limits<int>::min();
    }

    SeedChooserScreen *plantScreen = screen->mApp->mSeedChooserScreen;
    const auto HasPlantSeed = [plantScreen](SeedType target) { return HasBuiltinAIPlantSeed(plantScreen, target); };
    const bool lobbed = HasBuiltinAIOpponentLobbedPressure(screen);
    const bool peas = HasPlantSeed(SeedType::SEED_PEASHOOTER) || HasPlantSeed(SeedType::SEED_REPEATER) || HasPlantSeed(SeedType::SEED_SNOWPEA) || HasPlantSeed(SeedType::SEED_SCAREDYSHROOM)
        || HasPlantSeed(SeedType::SEED_THREEPEATER);
    const bool nuts = HasPlantSeed(SeedType::SEED_WALLNUT) || HasPlantSeed(SeedType::SEED_PUMPKINSHELL);
    const bool highValue = HasPlantSeed(SeedType::SEED_MELONPULT) || HasPlantSeed(SeedType::SEED_REPEATER) || HasPlantSeed(SeedType::SEED_STARFRUIT) || HasPlantSeed(SeedType::SEED_SPORESHROOM);

    int score = 40 + Sexy::Rand(31);
    switch (seedType) {
        case SeedType::SEED_ZOMBIE_NORMAL:
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
        case SeedType::SEED_ZOMBIE_IMP:
            score += 120;
            break;
        case SeedType::SEED_ZOMBIE_DOGWALKER:
            score += 175;
            break;
        case SeedType::SEED_ZOMBIE_PAIL:
            score += 100;
            break;
        case SeedType::SEED_ZOMBIE_LADDER:
        case SeedType::SEED_ZOMBONI:
            score += nuts ? 165 : 25;
            break;
        case SeedType::SEED_ZOMBIE_BUNGEE:
            score += highValue ? 175 : -120;
            break;
        case SeedType::SEED_ZOMBIE_CATAPULT:
            score += lobbed ? 170 : -80;
            break;
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
        case SeedType::SEED_ZOMBIE_TRASHCAN:
            score += lobbed ? -500 : (peas ? 95 : 30);
            break;
        case SeedType::SEED_ZOMBIE_BOBSLED:
        case SeedType::SEED_ZOMBIE_FOOTBALL:
        case SeedType::SEED_ZOMBIE_GARGANTUAR:
            score += 70;
            break;
        default:
            break;
    }
    return score;
}

SeedType FindBuiltinAIRuleZombieCandidate(SeedChooserScreen *screen) {
    SeedType bestSeed = SeedType::SEED_NONE;
    int bestScore = std::numeric_limits<int>::min();
    const int storageCount = screen == nullptr ? 0 : screen->GetSeedStorageCount();
    for (int index = 0; index < storageCount; ++index) {
        const SeedType seedType = screen->GetZombieSeedType(index);
        if (!IsBuiltinAICandidate(screen, seedType)) {
            continue;
        }
        const int score = BuiltinAIZombieRuleMatchupScore(screen, seedType);
        if (bestSeed == SeedType::SEED_NONE || score > bestScore || (score == bestScore && Sexy::Rand(2) == 0)) {
            bestSeed = seedType;
            bestScore = score;
        }
    }
    return bestSeed;
}

SeedType FindBuiltinAIPlantDeckCandidate(SeedChooserScreen *screen, const SeedType *prioritySeeds, std::size_t priorityCount, bool useTemplate) {
    bool hasUnpairedDaytimeMushroom = false;
    if (IsBuiltinAIDaytimeChooser(screen)) {
        const int storageCount = screen->GetSeedStorageCount();
        for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
            if (screen->GetChosenSeed(seedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK && vsai::draft::IsCoffeeDependentPlant(screen->GetPlantSeedType(seedIndex))) {
                hasUnpairedDaytimeMushroom = true;
                break;
            }
        }
    }
    if (hasUnpairedDaytimeMushroom && !HasBuiltinAIPlantSeed(screen, SeedType::SEED_INSTANT_COFFEE)) {
        // This must run before the regular role picker. A mushroom selected
        // in an earlier slot never waits for a random fallback to remember
        // Coffee, which fixes both six- and seven-slot selection flows.
        if (IsBuiltinAICandidate(screen, SeedType::SEED_INSTANT_COFFEE)) {
            return SeedType::SEED_INSTANT_COFFEE;
        }
    }
    const bool hasSelectedCarry = HasBuiltinAIPlantMainDamage(screen);
    const SeedType plannedMain = useTemplate ? PlannedBuiltinAIPlantMainDamageSeed(screen) : SeedType::SEED_NONE;
    const int plannedIndex = plannedMain == SeedType::SEED_NONE ? -1 : screen->GetSeedPacketIndex(plannedMain);
    const bool plannedMainAvailable = plannedIndex >= 0 && plannedIndex < screen->GetSeedStorageCount()
        && (screen->GetChosenSeed(plannedIndex).mSeedState == ChosenSeedState::SEED_IN_BANK || (IsBuiltinAICandidate(screen, plannedMain) && IsBuiltinAIPlantSupportCandidate(screen, plannedMain)));
    // A recorded plan can intentionally add one inexpensive tempo shooter
    // before its actual carry. That supporting attacker must not make the
    // selector believe the designated main card has already been chosen.
    const bool alreadyHasMainDamage = useTemplate && plannedMainAvailable ? HasBuiltinAIPlantSeed(screen, plannedMain) : hasSelectedCarry;
    const std::size_t selectedCount = static_cast<std::size_t>(std::max(0, screen->mSeedsInBank));
    const bool templateMainBannedAfterSelection = useTemplate && !plannedMainAvailable && selectedCount > 0;
    // Normal opening bans reroll the template before this picker runs. If a
    // map restriction or a late Ban makes the planned carry unavailable,
    // recover with a matchup carry now; never let Puff/Coffee fill its role.
    const bool needsCounterMain = templateMainBannedAfterSelection || !useTemplate || !plannedMainAvailable;
    const SeedType counterMain = !alreadyHasMainDamage && needsCounterMain ? FindBuiltinAICounterCarry(screen, useTemplate) : SeedType::SEED_NONE;
    const std::size_t planSize = GetBuiltinAIPlanSize(screen);
    const std::size_t slotsRemaining = selectedCount < planSize ? planSize - selectedCount : 0;
    // Pick one persistent target slot per match. The main carry is still
    // protected from the opening two picks, but it can appear at any later
    // slot rather than being predictably forced into the final packet.
    const int mainPickSlot = BuiltinAIPlantMainPickSlot(screen);
    const bool plannedMainNeedsCoffee = IsBuiltinAIDaytimeChooser(screen) && vsai::draft::IsCoffeeDependentPlant(plannedMain) && !HasBuiltinAIPlantSeed(screen, SeedType::SEED_INSTANT_COFFEE);
    // A daytime mushroom carry may use the random target slot, but Coffee
    // must remain selectable immediately after it when the dependency is
    // still missing. This preserves both six- and seven-slot legality.
    const bool beforeRandomMainSlot = static_cast<int>(selectedCount) + 1 < mainPickSlot;
    const bool coffeeEscape = plannedMainNeedsCoffee && slotsRemaining <= 2U;
    const bool deferMainDamage = !alreadyHasMainDamage && beforeRandomMainSlot && !coffeeEscape;
    const auto IsMainDamageCandidate = [&](SeedType seedType) {
        if (plannedMain != SeedType::SEED_NONE && plannedMainAvailable) {
            return seedType == plannedMain;
        }
        if (counterMain != SeedType::SEED_NONE) {
            return seedType == counterMain;
        }
        return vsai::draft::IsPlantCarrySeed(seedType) && !vsai::draft::IsPlantTempoMushroom(seedType);
    };
    // Blover is a mandatory matchup answer, but it must not displace the
    // plant deck's only carry during a six-card selection. The main-card
    // gate below always chooses that carry first; every later slot can then
    // reserve one card for Balloon Zombie.
    if (alreadyHasMainDamage && HasBuiltinAIOpponentBalloon(screen) && !HasBuiltinAIPlantSeed(screen, SeedType::SEED_BLOVER) && IsBuiltinAICandidate(screen, SeedType::SEED_BLOVER)) {
        return SeedType::SEED_BLOVER;
    }
    const auto IsAvailableMain = [&](SeedType seedType) { return IsMainDamageCandidate(seedType) && IsBuiltinAICandidate(screen, seedType) && IsBuiltinAIPlantSupportCandidate(screen, seedType); };
    bool hasAvailableMainDamage = false;
    for (std::size_t index = 0; index < priorityCount && !hasAvailableMainDamage; ++index) {
        hasAvailableMainDamage = IsAvailableMain(prioritySeeds[index]);
    }
    const std::span<const SeedType> mainFallbacks = useTemplate ? std::span<const SeedType>(kBuiltinAIPlantMainFallbacks) : std::span<const SeedType>(kBuiltinAIRulePlantMainFallbacks);
    for (const SeedType seedType : mainFallbacks) {
        hasAvailableMainDamage = hasAvailableMainDamage || IsAvailableMain(seedType);
    }
    const bool mustPickMainDamage = !alreadyHasMainDamage && hasAvailableMainDamage && !deferMainDamage;
    const auto IsPlannedTemplateSecondaryOutput = [&](SeedType seedType) {
        if (!useTemplate || !vsai::draft::IsPlantCarrySeed(seedType)) {
            return false;
        }
        const SeedType *plannedDeck = GetBuiltinAIDeckPriority(screen);
        return std::any_of(plannedDeck, plannedDeck + GetBuiltinAIPlanSize(screen), [seedType](SeedType plannedSeed) { return plannedSeed == seedType; });
    };
    auto IsCompatible = [&](SeedType seedType) {
        if (deferMainDamage && vsai::draft::IsPlantCarrySeed(seedType)) {
            // A recorded template may explicitly include a low-cost secondary
            // output before the real carry. Generic decks remain one-carry.
            if (!useTemplate || seedType == plannedMain || !IsPlannedTemplateSecondaryOutput(seedType)) {
                return false;
            }
        }
        return IsBuiltinAICandidate(screen, seedType)
            && IsBuiltinAIPlantSupportCandidate(screen, seedType)
            // A replay template is allowed to preserve its recorded
            // secondary output, but only when that exact card belongs to its
            // template. Generic fallback remains single-carry so a Ban
            // substitution cannot accidentally create two unrelated mains.
            && (!alreadyHasMainDamage || !vsai::draft::IsPlantCarrySeed(seedType) || IsPlannedTemplateSecondaryOutput(seedType)) && (!mustPickMainDamage || IsMainDamageCandidate(seedType));
    };

    const auto IsPreferredCompatible = [&](SeedType seedType) {
        if (!IsCompatible(seedType)) {
            return false;
        }
        if (seedType != SeedType::SEED_ICEBERG_LETTUCE) {
            return true;
        }
        static constexpr SeedType kHigherPriorityAnswers[] = {
            SeedType::SEED_POTATOMINE,
            SeedType::SEED_SQUASH,
            SeedType::SEED_CHERRYBOMB,
            SeedType::SEED_JALAPENO,
            SeedType::SEED_CHILLY_PEPPER,
        };
        return std::none_of(std::begin(kHigherPriorityAnswers), std::end(kHigherPriorityAnswers), [&](SeedType answer) { return IsCompatible(answer); });
    };

    if (priorityCount > 0) {
        const std::size_t firstPriority = static_cast<std::size_t>(Sexy::Rand(static_cast<int>(priorityCount)));
        const SeedType candidate = vsai::draft::FindRotatedEligibleSeed(std::span<const SeedType>(prioritySeeds, priorityCount), firstPriority, IsPreferredCompatible);
        if (candidate != SeedType::SEED_NONE) {
            return candidate;
        }
    }

    // If the planned main card was banned, fill that role before economy or
    // support cards.  This is what keeps Ban substitutions playable instead
    // of producing a melee-only deck.
    if (!alreadyHasMainDamage) {
        const std::size_t firstFallback = useTemplate ? 0 : static_cast<std::size_t>(Sexy::Rand(static_cast<int>(mainFallbacks.size())));
        const SeedType candidate = vsai::draft::FindRotatedEligibleSeed(mainFallbacks, firstFallback, IsPreferredCompatible);
        if (candidate != SeedType::SEED_NONE) {
            return candidate;
        }
    }

    if (alreadyHasMainDamage) {
        for (const SeedType seedType : kBuiltinAIPlantPostCarryFallbacks) {
            // Doom-shroom is a real daytime counter only after Coffee has
            // been locked in. Otherwise a final-slot Doom would be left
            // asleep with no remaining opportunity to select its pairing.
            if (seedType == SeedType::SEED_DOOMSHROOM && IsBuiltinAIDaytimeChooser(screen) && !HasBuiltinAIPlantSeed(screen, SeedType::SEED_INSTANT_COFFEE)) {
                continue;
            }
            if (IsPreferredCompatible(seedType)) {
                return seedType;
            }
        }
    }

    // Every legal carry was exhausted. Do not leave the chooser frozen on
    // SEED_NONE: global Ban combinations can remove every valid main card.
    // The normal support predicate still preserves Coffee and matchup
    // legality; this is only a progress fallback for an unwinnable draft.
    if (!alreadyHasMainDamage) {
        for (const SeedType seedType : kBuiltinAIPlantPostCarryFallbacks) {
            if (IsPreferredCompatible(seedType)) {
                return seedType;
            }
        }
    }

    const int storageCount = screen->GetSeedStorageCount();
    if (storageCount <= 0) {
        return SeedType::SEED_NONE;
    }
    const int firstSeedIndex = Sexy::Rand(storageCount);
    for (int offset = 0; offset < storageCount; ++offset) {
        const int seedIndex = (firstSeedIndex + offset) % storageCount;
        const SeedType seedType = screen->GetPlantSeedType(seedIndex);
        if (IsPreferredCompatible(seedType)) {
            return seedType;
        }
    }
    return SeedType::SEED_NONE;
}

SeedType FindBuiltinAILegalProgressCandidate(SeedChooserScreen *screen) {
    if (screen == nullptr) {
        return SeedType::SEED_NONE;
    }

    const int storageCount = screen->GetSeedStorageCount();
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        const SeedType seedType = screen->mIsZombieChooser ? screen->GetZombieSeedType(seedIndex) : screen->GetPlantSeedType(seedIndex);
        if (seedType == SeedType::SEED_NONE || seedType == SeedType::SEED_IMITATER || (!screen->mIsZombieChooser && Plant::IsUpgrade(seedType))
            || (!screen->mShowExtendedSeeds && ((screen->mIsZombieChooser && seedType > SeedType::SEED_ZOMBIE_GARGANTUAR) || (!screen->mIsZombieChooser && seedType >= SeedType::SEED_ICEBERG_LETTUCE)))
            || !screen->HasPacket(seedType, screen->mIsZombieChooser) || screen->GetChosenSeed(seedIndex).mSeedState != ChosenSeedState::SEED_IN_CHOOSER || screen->SeedNotAllowedToPick(seedType)) {
            continue;
        }
        return seedType;
    }
    return SeedType::SEED_NONE;
}

SeedType PlantTemplateMainSeed(SeedChooserScreen *screen, int profile) {
    const BuiltinAIPlantProfile *templateProfile = GetBuiltinAIPlantProfile(profile);
    if (screen == nullptr || templateProfile == nullptr) {
        return SeedType::SEED_NONE;
    }
    const SeedType *deck = templateProfile->Deck(screen->mBoard != nullptr && screen->mBoard->StageIsNight());
    if (templateProfile->mainCarry != SeedType::SEED_NONE && std::find(deck, deck + GetBuiltinAIPlanSize(screen), templateProfile->mainCarry) != deck + GetBuiltinAIPlanSize(screen)) {
        return templateProfile->mainCarry;
    }
    for (std::size_t index = 0; index < GetBuiltinAIPlanSize(screen); ++index) {
        if (vsai::draft::IsPlantCarrySeed(deck[index])) {
            return deck[index];
        }
    }
    return SeedType::SEED_NONE;
}

bool IsBuiltinAIPlantSeedBanned(const SeedChooserScreen *screen, SeedType seedType) {
    const int index = static_cast<int>(seedType);
    return screen != nullptr && index >= 0 && index < NUM_SEEDS_IN_CHOOSER_EXTENDED && screen->mBannedSeed[index].mSeedState == BannedSeedState::SEED_BANNED;
}

void ReplaceBuiltinAIPlantTemplateAfterOpeningBan(SeedChooserScreen *screen) {
    if (screen == nullptr || screen->mIsZombieChooser || screen->mSeedsInBank != 0 || !UsesBuiltinAITemplate(screen)) {
        return;
    }

    const int currentProfile = BuiltinAIPlans().plantProfile;
    const SeedType bannedMain = PlantTemplateMainSeed(screen, currentProfile);
    if (!IsBuiltinAIPlantSeedBanned(screen, bannedMain)) {
        return;
    }

    int bestProfile = -1;
    int bestScore = std::numeric_limits<int>::min();
    for (int offset = 0; offset < static_cast<int>(kBuiltinAIPlantProfiles.size()); ++offset) {
        const int candidateProfile = offset;
        const SeedType candidateMain = PlantTemplateMainSeed(screen, candidateProfile);
        if (candidateProfile == currentProfile || candidateMain == SeedType::SEED_NONE || candidateMain == bannedMain || IsBuiltinAIPlantSeedBanned(screen, candidateMain)
            || !IsBuiltinAICandidate(screen, candidateMain)) {
            continue;
        }
        const int score = BuiltinAIPlantCarryMatchupScore(screen, candidateMain);
        if (bestProfile < 0 || score > bestScore || (score == bestScore && candidateProfile < bestProfile)) {
            bestProfile = candidateProfile;
            bestScore = score;
        }
    }
    if (bestProfile >= 0) {
        BuiltinAIPlans().plantProfile = bestProfile;
        vsai::draft::GetBuiltinAIDraftHistory().lastPlantProfile = bestProfile;
        BuiltinAIPlans().plantMainPickSlot = -1;
        return;
    }

    // If every template main has been removed by global/first-round bans,
    // retain the existing constrained fallback picker for the whole deck.
    BuiltinAIPlans().usePlantTemplate = false;
}

SeedType FindBuiltinAIBanCandidate(SeedChooserScreen *screen, const SeedType *fallbackSeeds, std::size_t fallbackCount) {
    const int storageCount = screen->GetSeedStorageCount();
    if (storageCount <= 0) {
        return SeedType::SEED_NONE;
    }

    SeedType bestSeed = SeedType::SEED_NONE;
    int bestScore = std::numeric_limits<int>::min();
    // After the plant player has committed a carry, denying another carry
    // does not weaken the chosen deck. Spend later zombie-side Bans on its
    // answer package instead of removing a card the opponent cannot use.
    const bool preserveUnusedPlantCarries = screen->mBanningPhase && !screen->mIsZombieChooser && HasBuiltinAIPlantMainDamage(screen);
    for (int seedIndex = 0; seedIndex < storageCount; ++seedIndex) {
        const SeedType seedType = screen->mIsZombieChooser ? screen->GetZombieSeedType(seedIndex) : screen->GetPlantSeedType(seedIndex);
        if (!IsBuiltinAICandidate(screen, seedType)) {
            continue;
        }
        if (preserveUnusedPlantCarries && vsai::draft::IsPlantCarrySeed(seedType)) {
            continue;
        }

        int fallbackScore = 0;
        for (std::size_t index = 0; index < fallbackCount; ++index) {
            if (fallbackSeeds[index] == seedType) {
                fallbackScore = 180 - static_cast<int>(index) * 8;
                break;
            }
        }
        const int baseThreat = vsai::draft::BanBaseThreat(screen->mIsZombieChooser, seedType);
        // Keep the replay database and matchup-specific priority as additive
        // evidence. A baseline tier-one threat cannot be eclipsed solely by
        // sample frequency from a narrow replay set.
        const int score = baseThreat * 2 + fallbackScore + vsai::draft::BanDatabasePriority(screen->mIsZombieChooser, seedType, static_cast<std::uint32_t>(Sexy::GetTickCount()));
        if (bestSeed == SeedType::SEED_NONE || score > bestScore || (score == bestScore && seedType < bestSeed)) {
            bestSeed = seedType;
            bestScore = score;
        }
    }
    return bestSeed;
}

int GetBuiltinAIPageForSeed(const SeedChooserScreen *screen, SeedType seedType) {
    if (!screen->mShowExtendedSeeds) {
        return 0;
    }
    if (screen->mIsZombieChooser) {
        return seedType > GetZombieFirstPageLastSeedType(screen) ? 1 : 0;
    }
    return seedType >= SeedType::SEED_ICEBERG_LETTUCE ? 1 : 0;
}

} // namespace

int SeedChooserScreen::ResolveGlobalBpPlayerIndex() const {
    if (mApp == nullptr || !mApp->IsVSMode()) {
        return -1;
    }

    const auto *vsSetup = mApp->mVSSetupMenu;
    if (vsSetup == nullptr) {
        return (mPlayerIndex >= 0 && mPlayerIndex <= 1) ? mPlayerIndex : -1;
    }

    const VSSide chooserSide = mIsZombieChooser ? VSSide::VS_SIDE_ZOMBIE : VSSide::VS_SIDE_PLANT;
    for (int slot = 0; slot < 2; ++slot) {
        if (vsSetup->mSides[slot] == chooserSide) {
            const int playerIndex = vsSetup->mControllerIndex[slot];
            if (playerIndex >= 0 && playerIndex <= 1) {
                return playerIndex;
            }
        }
    }

    return -1;
}

void SeedChooserScreen::ApplyGlobalBpBans() {
    if (!mApp->IsVSMode()) {
        return;
    }

    if (VSSetupAddonWidget::msGlobalBpMode == VSSetupAddonWidget::GLOBALBP_CLOSED) {
        return;
    }

    const int playerIndex = ResolveGlobalBpPlayerIndex();
    if (playerIndex < 0 || playerIndex > 1) {
        return;
    }

    for (SeedType selectedSeedType : VSSetupAddonWidget::msGlobalBpSeeds[playerIndex]) {
        if (selectedSeedType == SeedType::SEED_NONE) {
            continue;
        }
        if (!mIsZombieChooser && GetSeedPacketIndex(selectedSeedType) < 0) {
            continue;
        }
        if (mIsZombieChooser && (int(selectedSeedType) < int(SEED_ZOMBIE_GRAVESTONE) || int(selectedSeedType) >= NUM_ZOMBIE_SEEDS_IN_CHOOSER)) {
            continue;
        }
        const int bannedSeedIndex = int(selectedSeedType);
        if (bannedSeedIndex < 0 || bannedSeedIndex >= NUM_SEEDS_IN_CHOOSER_EXTENDED) {
            continue;
        }

        const int chooserSeedIndex = GetSeedPacketIndex(selectedSeedType);
        if (chooserSeedIndex < 0) {
            continue;
        }
        if (!mIsZombieChooser && chooserSeedIndex >= GetSeedStorageCount()) {
            continue;
        }
        if (mIsZombieChooser && chooserSeedIndex >= NUM_ZOMBIE_SEEDS_IN_CHOOSER) {
            continue;
        }

        BannedSeed &bannedSeed = mBannedSeed[bannedSeedIndex];
        bannedSeed.mSeedType = selectedSeedType;
        GetSeedPositionInChooser(chooserSeedIndex, bannedSeed.mX, bannedSeed.mY);
        bannedSeed.mSeedState = BannedSeedState::SEED_BANNED;
    }
}


void SeedChooserScreen::_constructor(bool theIsZombieChooser) {
    // 修复在没解锁商店图鉴时依然显示相应按钮的问题、对战选种子界面的按钮问题；
    // 还添加了生存模式保留上次选卡，添加坚果艺术关卡默认选择坚果，添加向日葵艺术关卡默认选择坚果、杨桃、萝卜伞
    int repickPacketCount = 0;
    std::vector<SeedType> repickSeeds;
    SeedType repickImitaterType = SeedType::SEED_NONE;

    Sexy::Widget::_constructor();
    Widget::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(vTableForSeedChooserScreenAddr) + 8);
    ButtonListener::vTable = reinterpret_cast<const Sexy::ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(Widget::vTable) + kSeedChooserButtonListenerVtableOffset);

    auto &buttonList = mButtons.Construct();
    mApp = reinterpret_cast<LawnApp *>(Sexy::gSexyAppBase);
    mBoard = mApp->mBoard;
    if (mApp->IsVSMode() && !theIsZombieChooser) {
        // A VS match always constructs the plant chooser first.  Clearing the
        // old plan here gives a fresh random archetype even when a human
        // performs the opening pick before the local AI gets its turn.
        vsai::draft::ResetBuiltinAIDraftSession();
        vsai::draft::ResetBanDatabase();
    }

    if (mBoard->mCutScene->IsSurvivalRepick() && !mApp->IsCoopMode()) {
        GamepadControls *gamePad = mBoard->mGamepadControls[0];
        SeedBank *seedBank = gamePad != nullptr ? gamePad->GetSeedBank() : nullptr;
        if (seedBank != nullptr) {
            repickPacketCount = seedBank->mNumPackets;
            repickSeeds.reserve(repickPacketCount);
            for (int i = 0; i < repickPacketCount; ++i) {
                const SeedPacket &seedPacket = seedBank->mSeedPackets[i];
                repickSeeds.push_back(seedPacket.mPacketType);
                if (seedPacket.mPacketType == SeedType::SEED_IMITATER && repickImitaterType == SeedType::SEED_NONE) {
                    repickImitaterType = seedPacket.mImitaterType;
                }
            }
        }
    }

    mClip = false;
    mShowHelpText = false;
    mSeedsInFlight = 0;
    mSeedsInBank = 0;
    mSeedsIn1PBank = 0;
    mSeedsIn2PBank = 0;
    mSeedIndex1 = 0;
    mSeedIndex2 = 0;
    mCursorPositionX1 = -1;
    mCursorPositionX2 = -1;
    mCursorPositionY1 = -1;
    mCursorPositionY2 = -1;
    mChooseState = SeedChooserState::CHOOSE_NORMAL;
    mViewLawnTime = 0;
    GetSeedPositionInChooser(0, mCursorPositionX1, mCursorPositionY1);
    GetSeedPositionInChooser(mSeedIndex2, mCursorPositionX2, mCursorPositionY2);

    mToolTip1 = new ToolTipWidget();
    mToolTip2 = new ToolTipWidget();

    if (VSSetupAddonWidget::msExtendedSeedsMode) {
        mToolTip1->mTitleFont = mToolTip2->mTitleFont = addonFonts.JN_BOBO_HEI20;
        mToolTip1->mWarningTextFont = mToolTip2->mWarningTextFont = addonFonts.JN_BOBO_HEI20;
    }

    mIsZombieChooser = theIsZombieChooser;
    mToolTipSeed1 = -1;
    mToolTipSeed2 = -1;
    mImitaterDialog = nullptr;

    if (mIsZombieChooser) {
        TodLoadResources("DelayLoad_Almanac");
        mSeedBank1 = mBoard->mSeedBank[1];
        mPlayerIndex = -1;
    } else {
        mSeedBank1 = mBoard->mSeedBank[0];
        mPlayerIndex = mApp->mPlayerInfo->GetId();
    }

    mSeedBank2 = nullptr;
    if (mApp->IsCoopMode()) {
        mSeedBank2 = mBoard->mSeedBank[1];
    }

    mDimCounter = 0;
    mImitaterButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Imitater, nullptr);
    mImitaterButton->mButtonImage = Sexy::IMAGE_IMITATERSEED;
    mImitaterButton->mOverImage = Sexy::IMAGE_IMITATERSEED;
    mImitaterButton->mDownImage = Sexy::IMAGE_IMITATERSEED;
    mImitaterButton->mDisabledImage = Sexy::IMAGE_IMITATERSEEDDISABLED;
    mImitaterButton->Resize(464, 490, Sexy::IMAGE_IMITATERSEED->mWidth, Sexy::IMAGE_IMITATERSEED->mHeight);
    mImitaterButton->mParentWidget = this;

    auto addButton = [&](GameButton *button) {
        button->mParentWidget = this;
        buttonList.push_back(button);
    };

    if (mBoard->mCutScene->IsSurvivalRepick() && mChooseState == SeedChooserState::CHOOSE_NORMAL) {
        mViewLawnButton = MakeButton(SeedChooserScreen::SeedChooserScreen_ViewLawn, this, this, "[VIEW_LAWN]");
        addButton(mViewLawnButton);
    } else {
        mViewLawnButton = nullptr;
    }

    mStoreButton = MakeButton(SeedChooserScreen::SeedChooserScreen_Store, this, this, "[SHOP_BUTTON]");
    addButton(mStoreButton);
    mStartButton = MakeButton(SeedChooserScreen::SeedChooserScreen_Start, this, this, "[LETS_ROCK_BUTTON]");
    addButton(mStartButton);
    mAlmanacButton = MakeButton(SeedChooserScreen::SeedChooserScreen_Almanac, this, this, "[ALMANAC_BUTTON]");
    addButton(mAlmanacButton);
    mButtonSlotState = 0;

    if (!buttonList.empty()) {
        std::vector<int> buttonWidths(buttonList.size(), 0);
        int totalWidth = 0;
        for (size_t buttonIndex = 0; buttonIndex < buttonList.size(); ++buttonIndex) {
            GameButton *button = buttonList[buttonIndex];
            const int labelWidth = Sexy::FONT_DWARVENTODCRAFT18->GetVTable()->StringWidth(Sexy::FONT_DWARVENTODCRAFT18, *button->mLabel);
            const int leftWidth = Sexy::IMAGE_BUTTON_LEFT->mWidth;
            const int middleWidth = Sexy::IMAGE_BUTTON_MIDDLE->mWidth;
            const int rightWidth = Sexy::IMAGE_BUTTON_RIGHT->mWidth;
            const int stretchWidth = labelWidth - leftWidth - rightWidth;
            const int minimumMiddleWidth = labelWidth > (3 * leftWidth) / 2;
            const int contentWidth = minimumMiddleWidth < stretchWidth ? middleWidth + stretchWidth : middleWidth + minimumMiddleWidth;
            const int buttonWidth = leftWidth + rightWidth + middleWidth * ((contentWidth - 1) / middleWidth);
            buttonWidths[buttonIndex] = buttonWidth;
            totalWidth += buttonWidth;
        }

        const int gap = (Sexy::IMAGE_SEEDCHOOSER_BACKGROUND->mWidth - 23 - totalWidth) / (int(buttonList.size()) + 1);
        int buttonX = gap + 10;
        for (size_t buttonIndex = 0; buttonIndex < buttonList.size(); ++buttonIndex) {
            GameButton *button = buttonList[buttonIndex];
            button->Resize(buttonX, 548, buttonWidths[buttonIndex] + 1, button->mHeight);
            buttonX += buttonWidths[buttonIndex] + gap;
        }
    }

    EnableStartButton(false);
    if (mViewLawnButton != nullptr) {
        if (!mBoard->mCutScene->IsSurvivalRepick() || mChooseState != SeedChooserState::CHOOSE_NORMAL) {
            mViewLawnButton->SetDisabled(true);
        } else {
            mViewLawnButton->SetDisabled(false);
        }
    }

    memset(mChosenSeeds, 0, sizeof(mChosenSeeds));
    memset(mChosenSeedsExtended, 0, sizeof(mChosenSeedsExtended));
    for (int storageIndex = 0; storageIndex < GetSeedStorageCount(); ++storageIndex) {
        ChosenSeed &aChosenSeed = GetChosenSeed(storageIndex);
        aChosenSeed.mSeedType = mIsZombieChooser ? GetZombieSeedType(storageIndex) : GetPlantSeedType(storageIndex);
        int seedIndex = storageIndex;
        if (mIsZombieChooser) {
            if (storageIndex >= 25) {
                seedIndex -= 25;
            }
        } else if (storageIndex >= NUM_SEEDS_IN_CHOOSER) {
            seedIndex -= NUM_SEEDS_IN_CHOOSER;
        }
        GetSeedPositionInChooser(seedIndex, aChosenSeed.mX, aChosenSeed.mY);
        aChosenSeed.mTimeStartMotion = 0;
        aChosenSeed.mTimeEndMotion = 0;
        aChosenSeed.mStartX = aChosenSeed.mX;
        aChosenSeed.mStartY = aChosenSeed.mY;
        aChosenSeed.mEndX = aChosenSeed.mX;
        aChosenSeed.mEndY = aChosenSeed.mY;
        aChosenSeed.mChosenPlayerIndex = 0;
        aChosenSeed.mSeedState = ChosenSeedState::SEED_IN_CHOOSER;
        aChosenSeed.mSeedIndexInBank = -1;
        aChosenSeed.mRefreshing = false;
        aChosenSeed.mRefreshCounter = 0;
        aChosenSeed.mImitaterType = SeedType::SEED_NONE;
        aChosenSeed.mCrazyDavePicked = false;
    }
    GetChosenSeed(SeedType::SEED_IMITATER).mSeedState = ChosenSeedState::SEED_PACKET_HIDDEN;

    if (mBoard->mCutScene->IsSurvivalRepick()) {
        for (int packetIndex = 0; packetIndex < mSeedBank1->mNumPackets; ++packetIndex) {
            const SeedPacket &seedPacket = mSeedBank1->mSeedPackets[packetIndex];
            const int seedIndex = int(seedPacket.mPacketType);
            if (seedIndex < 0 || seedIndex >= NUM_SEED_TYPES) {
                continue;
            }
            GetChosenSeed(seedIndex).mRefreshing = seedPacket.mRefreshing;
            GetChosenSeed(seedIndex).mRefreshCounter = seedPacket.mRefreshCounter;
        }
        mSeedBank1->mNumPackets = 0;

        if (mApp->IsCoopMode() && mSeedBank2 != nullptr) {
            for (int packetIndex = 0; packetIndex < mSeedBank2->mNumPackets; ++packetIndex) {
                const SeedPacket &seedPacket = mSeedBank2->mSeedPackets[packetIndex];
                const int seedIndex = int(seedPacket.mPacketType);
                if (seedIndex < 0 || seedIndex >= NUM_SEED_TYPES) {
                    continue;
                }
                GetChosenSeed(seedIndex).mRefreshing = seedPacket.mRefreshing;
                GetChosenSeed(seedIndex).mRefreshCounter = seedPacket.mRefreshCounter;
                GetChosenSeed(seedIndex).mChosenPlayerIndex = 1;
            }
            mSeedBank2->mNumPackets = 0;
        }
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SEEING_STARS) {
        ChosenSeed &aStarFruit = GetChosenSeed(SEED_STARFRUIT);
        int aX = mBoard->GetSeedPacketPositionX(0, 0, false);
        aStarFruit.mX = aX, aStarFruit.mY = 8;
        aStarFruit.mStartX = aX, aStarFruit.mStartY = 8;
        aStarFruit.mEndX = aX, aStarFruit.mEndY = 8;
        aStarFruit.mSeedState = SEED_IN_BANK;
        aStarFruit.mSeedIndexInBank = 0;
        ++mSeedsInBank;
        ++mSeedsIn1PBank;
    }

    auto pickSeedsFromLevelConfig = [&]() {
        Sexy::Level &level = *mApp->mLevel;

        int *seedConfigBegin = std::to_address(level.mSeedConfig.begin());
        int *seedConfigEnd = std::to_address(level.mSeedConfig.end());

        int bankIndex = 0;

        for (int *seedIt = seedConfigBegin; seedIt != seedConfigEnd; ++seedIt) {
            if ((level.mSeedBankLimit > 0 && bankIndex >= level.mSeedBankLimit) || bankIndex >= mSeedBank1->mNumPackets) {
                break;
            }

            const int seedIndex = *seedIt;

            if (seedIndex < 0 || seedIndex >= NUM_SEED_TYPES)
                continue;

            ChosenSeed &chosenSeed = GetChosenSeed(seedIndex);
            chosenSeed.mX = mBoard->GetSeedPacketPositionX(bankIndex, 0, false);
            chosenSeed.mY = 8;
            chosenSeed.mStartX = chosenSeed.mX;
            chosenSeed.mStartY = chosenSeed.mY;
            chosenSeed.mEndX = chosenSeed.mX;
            chosenSeed.mEndY = chosenSeed.mY;
            chosenSeed.mSeedState = ChosenSeedState::SEED_IN_BANK;
            chosenSeed.mSeedIndexInBank = bankIndex;
            chosenSeed.mCrazyDavePicked = true;
            ++bankIndex;
            ++mSeedsInBank;
        }
    };

    if (mBoard->IsLevelDataLoaded()) {
        const Sexy::Level &level = *mApp->mLevel;

        if (!level.mSeedConfig.empty()) {
            pickSeedsFromLevelConfig();
        }
    } else if (mApp->IsAdventureMode() && !mApp->IsFirstTimeAdventureMode()) {
        CrazyDavePickSeeds();
    }

    UpdateImitaterButton();
    mCursorBobPhase = 0.0f;
    mDimCounter = 0;
    mOpeningDialog = false;
    mSeedChooserAge = 0;

    if (!repickSeeds.empty()) {
        // 实现无尽模式保留上次选卡。为什么不直接像WP版那样一一对应地选卡呢？因为玩家有可能通过爆炸坚果修改卡槽选中了多个相同类型的卡片或不在SeedChooser内的卡片，一一对应的话会有BUG
        int theValidChosenSeedNum = 0;
        for (SeedType theSeed : repickSeeds) {
            if (theSeed < 0 || theSeed >= SeedType::NUM_SEEDS_IN_CHOOSER) {
                continue;
            }

            ChosenSeed *theChosenSeed = &GetChosenSeed(theSeed);
            if (theChosenSeed->mSeedType == SeedType::SEED_IMITATER) {
                theChosenSeed->mImitaterType = repickImitaterType;
            }

            if (theChosenSeed->mSeedState == ChosenSeedState::SEED_IN_BANK) {
                continue;
            }

            GetSeedPositionInBank(theValidChosenSeedNum, theChosenSeed->mX, theChosenSeed->mY, 0);
            theChosenSeed->mEndX = theChosenSeed->mX;
            theChosenSeed->mEndY = theChosenSeed->mY;
            theChosenSeed->mStartX = theChosenSeed->mX;
            theChosenSeed->mStartY = theChosenSeed->mY;
            theChosenSeed->mSeedState = ChosenSeedState::SEED_IN_BANK;
            theChosenSeed->mSeedIndexInBank = theValidChosenSeedNum;
            ++theValidChosenSeedNum;
        }

        mSeedsInBank = theValidChosenSeedNum;
        mSeedsIn1PBank = theValidChosenSeedNum;
        if (theValidChosenSeedNum == repickPacketCount) {
            EnableStartButton(true);
        }
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT) {
        ChosenSeed *theChosenSeed = &(GetChosenSeed(SeedType::SEED_WALLNUT));
        theChosenSeed->mX = mBoard->GetSeedPacketPositionX(0, 0, false);
        theChosenSeed->mY = 8;
        theChosenSeed->mEndX = theChosenSeed->mX;
        theChosenSeed->mEndY = theChosenSeed->mY;
        theChosenSeed->mStartX = theChosenSeed->mX;
        theChosenSeed->mStartY = theChosenSeed->mY;
        theChosenSeed->mSeedState = ChosenSeedState::SEED_IN_BANK;
        theChosenSeed->mSeedIndexInBank = 0;
        mSeedsInBank += 1;
        mSeedsIn1PBank += 1;
    } else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER) {
        SeedType types[] = {SeedType::SEED_WALLNUT, SeedType::SEED_STARFRUIT, SeedType::SEED_UMBRELLA};
        for (int i = 0; i < std::size(types); ++i) {
            ChosenSeed *theChosenSeed = &(GetChosenSeed(types[i]));
            GetSeedPositionInBank(i, theChosenSeed->mX, theChosenSeed->mY, 0);
            theChosenSeed->mEndX = theChosenSeed->mX;
            theChosenSeed->mEndY = theChosenSeed->mY;
            theChosenSeed->mStartX = theChosenSeed->mX;
            theChosenSeed->mStartY = theChosenSeed->mY;
            theChosenSeed->mSeedState = ChosenSeedState::SEED_IN_BANK;
            theChosenSeed->mSeedIndexInBank = i;
            mSeedsInBank += 1;
            mSeedsIn1PBank += 1;
        }
    }

    if (mApp->IsVSMode()) {
        // 去除对战中的冗余按钮
        if (mStoreButton) {
            mStoreButton->mDisabled = true;
            mStoreButton->mBtnNoDraw = true;
        }
        if (mAlmanacButton) {
            mAlmanacButton->mDisabled = true;
            mAlmanacButton->mBtnNoDraw = true;
        }
        if (mStartButton) { // 此处仿照PS3版处理，同时去除双方的开始按钮
            mStartButton->mDisabled = true;
            mStartButton->mBtnNoDraw = true;
        }

        mBanningPhase = mApp->mVSSetupMenu->mAddonWidget->mBanMode;
        mShowExtendedSeeds = mApp->mVSSetupMenu->mAddonWidget->mExtendedSeedsMode;
        mHas7Packets = mApp->mVSSetupMenu->mAddonWidget->mExtraPacketMode;
        if (mShowExtendedSeeds) {
            mPageButton = MakeNewButton(SeedChooserScreen::SeedChooserScreen_Page, this, this, "", nullptr, Sexy::IMAGE_ZEN_NEXTGARDEN, Sexy::IMAGE_ZEN_NEXTGARDEN, Sexy::IMAGE_ZEN_NEXTGARDEN);
            mPageButton->Resize(mIsZombieChooser ? 225 : 25, 525, 60, 60);
        }
    } else {
        if (mStoreButton) {
            if (!mApp->CanShowStore()) { // 去除在未解锁商店时商店按钮
                mStoreButton->mDisabled = true;
                mStoreButton->mBtnNoDraw = true;
            }
        }
        if (mAlmanacButton) {
            if (!mApp->CanShowAlmanac()) { // 去除在未解锁图鉴时的图鉴按钮
                mAlmanacButton->mDisabled = true;
                mAlmanacButton->mBtnNoDraw = true;
            }
        }
    }

    if (mApp->mGameMode != GameMode::GAMEMODE_MP_VS && !mIsZombieChooser) {
        mMainMenuButton = MakeButton(104, this, this, "[MENU_BUTTON]");
        mMainMenuButton->Resize(mApp->IsCoopMode() ? 345 : 650, -3, 120, 80);
    }
}

void SeedChooserScreen::_destructor() {
    delete mMainMenuButton;
    delete mPageButton;

    old_SeedChooserScreen__destructor(this);
}

void SeedChooserScreen::AddedToManager(Sexy::WidgetManager *theWidgetManager) {
    old_SeedChooserScreen_AddedToManager(this, theWidgetManager);

    if (mPageButton != nullptr) {
        AddWidget(mPageButton);
    }
    if (mMainMenuButton != nullptr) {
        AddWidget(mMainMenuButton);
    }
}

void SeedChooserScreen::RemovedFromManager(Sexy::WidgetManager *theWidgetManager) {
    if (mPageButton != nullptr) {
        RemoveWidget(mPageButton);
    }
    if (mMainMenuButton != nullptr) {
        RemoveWidget(mMainMenuButton);
    }

    old_SeedChooserScreen_RemovedFromManager(this, theWidgetManager);
}

void SeedChooserScreen::RebuildHelpbar() {
    // 拓宽Widget大小
    if (mApp->mGameMode != GameMode::GAMEMODE_MP_VS && !mIsZombieChooser) {
        Resize(mX, mY, 800, 600); // 原本(472,521)，改为(800,600)，不然没办法点击模仿者按钮和底栏三按钮。
    } else {
        Resize(mX, mY, mWidth, 600);
    }

    old_SeedChooserScreen_RebuildHelpbar(this);
}

void SeedChooserScreen::UpdateBuiltinAIPick() {
    if (!IsLocalBuiltinAIChooser(this) || mApp->mVSSetupMenu == nullptr || !CanPickNow() || mSeedsInFlight != 0 || VSSetupAddonWidget::msAIDraftDisabledMode) {
        return;
    }

    const VSSide controlledSide = mBanningPhase ? (mIsZombieChooser ? VS_SIDE_PLANT : VS_SIDE_ZOMBIE) : (mIsZombieChooser ? VS_SIDE_ZOMBIE : VS_SIDE_PLANT);
    const bool aiEnabled = controlledSide == VS_SIDE_PLANT ? VSSetupAddonWidget::msPlantAIMode : VSSetupAddonWidget::msZombieAIMode;
    if (!aiEnabled || (mBanningPhase && mSeedsInBanned >= mNumBanPackets)) {
        return;
    }
    EnsureBuiltinAIDeckPlans(this);

    int controllerIndex = -1;
    for (int sideSlot = 0; sideSlot < 2; ++sideSlot) {
        if (mApp->mVSSetupMenu->mSides[sideSlot] == controlledSide) {
            controllerIndex = mApp->mVSSetupMenu->mControllerIndex[sideSlot];
            break;
        }
    }
    if (controllerIndex < 0 || controllerIndex > 1) {
        return;
    }

    const SeedType *prioritySeeds = nullptr;
    std::size_t priorityCount = 0;
    if (mBanningPhase) {
        const std::span<const SeedType> priority = mIsZombieChooser ? vsai::draft::PlantBanPriority() : vsai::draft::ZombieBanPriority();
        prioritySeeds = priority.data();
        priorityCount = priority.size();
    } else if (UsesBuiltinAITemplate(this)) {
        prioritySeeds = GetBuiltinAIDeckPriority(this);
        priorityCount = GetBuiltinAIPlanSize(this);
    }

    SeedType selectedSeedType = mBanningPhase ? FindBuiltinAIBanCandidate(this, prioritySeeds, priorityCount)
                                              : (mIsZombieChooser ? (UsesBuiltinAITemplate(this) ? FindBuiltinAICandidate(this, prioritySeeds, priorityCount) : FindBuiltinAIRuleZombieCandidate(this))
                                                                  : FindBuiltinAIPlantDeckCandidate(this, prioritySeeds, priorityCount, UsesBuiltinAITemplate(this)));
    if (!mBanningPhase && !mIsZombieChooser && selectedSeedType == SeedType::SEED_NONE) {
        // The deck picker has already attempted every role-aware main and
        // support alternative. A map restriction plus Ban can still make
        // each of those constraints mutually exclusive. The raw chooser
        // predicate is the final legal-progress fallback, preventing the
        // local AI from leaving the last seed slot permanently unpicked.
        selectedSeedType = FindBuiltinAICandidate(this, nullptr, 0);
        if (selectedSeedType == SeedType::SEED_NONE) {
            // All tactical deck constraints were exhausted. This fallback
            // retains engine and Ban legality while dropping only optional
            // composition preferences, so a local draft can always finish.
            selectedSeedType = FindBuiltinAILegalProgressCandidate(this);
        }
    }
    if (!mBanningPhase && mIsZombieChooser) {
        constexpr SeedType kDogwalker = SeedType::SEED_ZOMBIE_DOGWALKER;
        const int dogwalkerIndex = GetSeedPacketIndex(kDogwalker);
        const bool dogwalkerAlreadyChosen = dogwalkerIndex >= 0 && dogwalkerIndex < GetSeedStorageCount() && GetChosenSeed(dogwalkerIndex).mSeedState == ChosenSeedState::SEED_IN_BANK;
        SeedChooserScreen *plantChooser = mApp->mSeedChooserScreen;
        const int bannedIndex = static_cast<int>(kDogwalker);
        const bool dogwalkerBannedByPlant =
            plantChooser != nullptr && bannedIndex >= 0 && bannedIndex < NUM_SEEDS_IN_CHOOSER_EXTENDED && plantChooser->mBannedSeed[bannedIndex].mSeedState == BannedSeedState::SEED_BANNED;
        // Plant-side Ban is the only intended way to deny Dogwalker. Do not
        // let a replay template or its randomized order omit this card.
        if (!dogwalkerAlreadyChosen && !dogwalkerBannedByPlant && IsBuiltinAICandidate(this, kDogwalker)) {
            selectedSeedType = kDogwalker;
        }
    }
    if (selectedSeedType == SeedType::SEED_NONE) {
        return;
    }

    const int seedIndex = GetSeedPacketIndex(selectedSeedType);
    const int targetPage = GetBuiltinAIPageForSeed(this, selectedSeedType);
    if (targetPage != mPageIndex) {
        SetPageIndex(targetPage);
    }

    int seedX = 0;
    int seedY = 0;
    GetSeedPositionInChooser(seedIndex, seedX, seedY);
    ChosenSeed &selectedSeed = GetChosenSeed(seedIndex);
    selectedSeed.mX = seedX;
    selectedSeed.mY = seedY;

    // Ban processing reads the active selector, so keep both local selectors on the chosen card.
    mCursorPositionX1 = mCursorPositionX2 = seedX;
    mCursorPositionY1 = mCursorPositionY2 = seedY;
    const int pageSeedIndex = targetPage == 0 ? seedIndex : (mIsZombieChooser ? seedIndex - GetZombieFirstPageSeedCount(this) : seedIndex - NUM_SEEDS_IN_CHOOSER);
    mSeedIndex1 = mSeedIndex2 = pageSeedIndex;

    const int playerIndex = mApp->GamepadToPlayerIndex(controllerIndex);
    if (playerIndex < 0 || playerIndex > 1) {
        return;
    }

    const bool mouseWasVisible = mMouseVisible;
    mMouseVisible = true;
    ClickedSeedInChooser_Orgin(selectedSeed, playerIndex);
    mMouseVisible = mouseWasVisible;
}


void SeedChooserScreen::Update() {
    Sexy::Widget::Update();
    mSeedChooserAge++;

    // 记录当前1P选卡是否选满
    if (mApp->IsCoopMode()) {
        m1PChoosingSeeds = mSeedsIn1PBank < 4;
    }

    if (!mGlobalBpBansApplied) {
        ApplyGlobalBpBans();
        mGlobalBpBansApplied = true;
    }

    UpdateBuiltinAIPick();

    mDimCounter = CanPickNow() ? 0 : (mDimCounter + 1);
    mCursorBobPhase = (mCursorBobPhase + 0.01f <= 6.2832f) ? (mCursorBobPhase + 0.01f) : 0.0f;

    mToolTip1->Update();
    mToolTip2->Update();

    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        const SeedType seedType = mIsZombieChooser ? GetZombieSeedType(seedIndex) : GetPlantSeedType(seedIndex);
        if (seedType == SeedType::SEED_NONE || !HasPacket(seedType, mIsZombieChooser)) {
            continue;
        }

        ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        if (aChosenSeed.mSeedState != SEED_FLYING_TO_BANK && aChosenSeed.mSeedState != SEED_FLYING_TO_CHOOSER) {
            continue;
        }

        aChosenSeed.mX = TodAnimateCurve(aChosenSeed.mTimeStartMotion, aChosenSeed.mTimeEndMotion, mSeedChooserAge, aChosenSeed.mStartX, aChosenSeed.mEndX, CURVE_EASE_IN_OUT);
        aChosenSeed.mY = TodAnimateCurve(aChosenSeed.mTimeStartMotion, aChosenSeed.mTimeEndMotion, mSeedChooserAge, aChosenSeed.mStartY, aChosenSeed.mEndY, CURVE_EASE_IN_OUT);

        if (mSeedChooserAge >= aChosenSeed.mTimeEndMotion) {
            LandFlyingSeed(aChosenSeed);
        }
    }

    int aPlayerIndex = mApp->GamepadToPlayerIndex(mPlayerIndex);
    ShowToolTip(aPlayerIndex);
    if (mApp->IsCoopMode() || (mApp->IsAdventureMode() && mApp->mSecondPlayerGamepadIndex != -1)) {
        ShowToolTip(aPlayerIndex == 0);
    }

    mImitaterButton->Update();

    if (mButtonSlotState) {
        if (mApp->HasGamepad() || (mApp->mGamePad1IsOn && mApp->mGamePad2IsOn)) {
            auto *pageButtonWidget = reinterpret_cast<Sexy::Widget *>(mButtonSlotState);
            pageButtonWidget->mHasFocus = false;
            pageButtonWidget->mDisabled = false;
            mButtonSlotState = 0;
        }
    }

    for (GameButton *button : *mButtons) {
        button->Update();
    }

    UpdateViewLawn();
    MarkDirty();
    TryAutoStartBuiltinVSMatch(this);
}


void SeedChooserScreen::EnableStartButton(int theIsEnabled) {
    // 双人键盘模式下结盟选满后直接开始
    if (theIsEnabled && mApp->IsCoopMode() && isKeyboardTwoPlayerMode) {
        old_SeedChooserScreen_EnableStartButton(this, theIsEnabled);
        OnStartButton();
        mBoard->mSeedBank[1]->mSeedPackets[3].mPacketType = SeedType(mSeedIndex2);
        mBoard->mSeedBank[1]->mSeedPackets[3].mImitaterType = SeedType::SEED_NONE;
        return;
    }

    old_SeedChooserScreen_EnableStartButton(this, theIsEnabled);
}

void SeedChooserScreen::OnStartButton() {
    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        // 如果是对战模式，则直接关闭种子选择界面。用于修复对战模式选卡完毕后点击开始按钮导致的闪退
        CloseSeedChooser();
        return;
    }

    old_SeedChooserScreen_OnStartButton(this);
}

void SeedChooserScreen::CloseSeedChooser() {
    auto syncBankPackets = [&](SeedBank *seedBank, int chosenPlayerIndex) {
        if (seedBank == nullptr) {
            return;
        }

        for (int bankIndex = 0; bankIndex < seedBank->mNumPackets; ++bankIndex) {
            ChosenSeed *matchedSeed = nullptr;
            for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
                ChosenSeed &chosenSeed = GetChosenSeed(seedIndex);
                if (chosenSeed.mSeedState != ChosenSeedState::SEED_IN_BANK) {
                    continue;
                }
                if (chosenSeed.mSeedIndexInBank != bankIndex) {
                    continue;
                }
                if (mApp->IsCoopMode() && chosenSeed.mChosenPlayerIndex != chosenPlayerIndex) {
                    continue;
                }
                matchedSeed = &chosenSeed;
                break;
            }

            if (matchedSeed == nullptr) {
                continue;
            }

            SeedPacket &seedPacket = seedBank->mSeedPackets[bankIndex];
            seedPacket.SetPacketType(matchedSeed->mSeedType, matchedSeed->mImitaterType);
            if (!matchedSeed->mRefreshing) {
                continue;
            }

            seedPacket.mRefreshCounter = matchedSeed->mRefreshCounter;
            seedPacket.mRefreshing = true;
            seedPacket.mRefreshTime = Plant::GetRefreshTime(seedPacket.mPacketType, seedPacket.mImitaterType);
            seedPacket.mActive = false;
        }
    };

    syncBankPackets(mSeedBank1, 0);
    if (mApp->IsCoopMode()) {
        syncBankPackets(mSeedBank2, 1);
    }

    if (!mApp->IsVSMode()) {
        if (mApp->mBoard != nullptr && mApp->mBoard->mCutScene != nullptr) {
            mApp->mBoard->mCutScene->mSeedChoosing = false;
        }

        if (mIsZombieChooser) {
            mApp->KillZombieChooserScreen();
        } else {
            mApp->KillSeedChooserScreen();
        }
        return;
    }

    if (mIsZombieChooser) {
        mApp->KillZombieChooserScreen();
        if (mApp->mZombieChooserScreen != nullptr) {
            return;
        }
    } else {
        mApp->KillSeedChooserScreen();
        if (mApp->mSeedChooserScreen != nullptr) {
            return;
        }
    }

    if (mApp->mSeedChooserScreen == nullptr && mApp->mZombieChooserScreen == nullptr && mApp->mVSSetupMenu != nullptr) {
        mApp->mVSSetupMenu->CloseVSSetup(false);
    }
}

void SeedChooserScreen::UpdateImitaterButton() {
    if (mImitaterButton == nullptr) {
        return;
    }

    if (mIsZombieChooser || !HasPacket(SeedType::SEED_IMITATER, false) || mApp->IsVSMode()) {
        mImitaterButton->mBtnNoDraw = true;
        mImitaterButton->mDisabled = true;
        return;
    }

    mImitaterButton->mBtnNoDraw = false;
    mImitaterButton->SetDisabled(GetChosenSeed(SEED_IMITATER).mSeedState != SEED_PACKET_HIDDEN);
}

void SeedChooserScreen::UpdateCursor() {
    if (mApp->GetDialogCount() != 0) {
        return;
    }

    if (mBoard->mCutScene->IsInShovelTutorial()) {
        return;
    }

    if (mApp->mGameMode == GameMode::GAMEMODE_UPSELL) {
        return;
    }

    SeedType aSeedType = SeedHitTest(mCursorPositionX1, mCursorPositionY1);
    if (aSeedType != SeedType::SEED_NONE) {
        ChosenSeed &aChosenSeed = GetChosenSeed(GetSeedPacketIndex(aSeedType));
        if (aChosenSeed.mSeedState == ChosenSeedState::SEED_IN_BANK && aChosenSeed.mCrazyDavePicked) {
            aSeedType = SeedType::SEED_NONE;
        }
    }

    if (mMouseVisible && mChooseState != SeedChooserState::CHOOSE_VIEW_LAWN) {
        if (aSeedType == SeedType::SEED_NONE) {
            if (mImitaterButton->IsMouseOver()) {
                mApp->SetCursor(1);
                return;
            }
        } else if (!SeedNotAllowedToPick(aSeedType)) {
            mApp->SetCursor(1);
            return;
        }
    }

    mApp->SetCursor(0);
}

void SeedChooserScreen::LandFlyingSeed(ChosenSeed &theChosenSeed) {
    if (theChosenSeed.mSeedState == SEED_FLYING_TO_BANK) {
        theChosenSeed.mX = theChosenSeed.mEndX;
        theChosenSeed.mY = theChosenSeed.mEndY;
        theChosenSeed.mTimeStartMotion = 0;
        theChosenSeed.mTimeEndMotion = 0;
        theChosenSeed.mSeedState = SEED_IN_BANK;
        mSeedsInFlight--;
    } else if (theChosenSeed.mSeedState == SEED_FLYING_TO_CHOOSER) {
        theChosenSeed.mX = theChosenSeed.mEndX;
        theChosenSeed.mY = theChosenSeed.mEndY;
        theChosenSeed.mTimeStartMotion = 0;
        theChosenSeed.mTimeEndMotion = 0;
        theChosenSeed.mSeedState = SEED_IN_CHOOSER;
        mSeedsInFlight--;
        if (theChosenSeed.mSeedType == SEED_IMITATER) {
            theChosenSeed.mSeedState = SEED_PACKET_HIDDEN;
            theChosenSeed.mImitaterType = SEED_NONE;
            UpdateImitaterButton();
        }
    }
}

void SeedChooserScreen::UpdateAfterPurchase() {
    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        if (aChosenSeed.mSeedState == SEED_IN_BANK) {
            GetSeedPositionInBank(aChosenSeed.mSeedIndexInBank, aChosenSeed.mX, aChosenSeed.mY, aChosenSeed.mChosenPlayerIndex);
        } else if (aChosenSeed.mSeedState == SEED_IN_CHOOSER) {
            GetSeedPositionInChooser(seedIndex, aChosenSeed.mX, aChosenSeed.mY);
        } else {
            continue;
        }
        aChosenSeed.mStartX = aChosenSeed.mX;
        aChosenSeed.mStartY = aChosenSeed.mY;
        aChosenSeed.mEndX = aChosenSeed.mX;
        aChosenSeed.mEndY = aChosenSeed.mY;
    }
    const int aNumPackets = mApp->IsCoopMode() ? (mSeedBank1->mNumPackets + mSeedBank2->mNumPackets) : mSeedBank1->mNumPackets;
    EnableStartButton(mSeedsInBank == aNumPackets);
    UpdateImitaterButton();
}

void SeedChooserScreen::PickRandomSeeds() {
    for (int anIndex = mSeedsInBank; anIndex < mSeedBank1->mNumPackets; ++anIndex) {
        const int seedsAvailable = mApp->GetSeedsAvailable(mIsZombieChooser);
        if (seedsAvailable <= 0) {
            break;
        }

        const auto aSeedType = SeedType(Sexy::Rand(seedsAvailable));
        if (!HasPacket(aSeedType, mIsZombieChooser)) {
            continue;
        }
        if (aSeedType == SeedType::SEED_IMITATER) {
            continue;
        }

        const int seedIndex = GetSeedPacketIndex(aSeedType);
        if (seedIndex < 0 || seedIndex >= GetSeedStorageCount()) {
            continue;
        }

        ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        if (aChosenSeed.mSeedState != ChosenSeedState::SEED_IN_CHOOSER) {
            continue;
        }

        aChosenSeed.mTimeStartMotion = mSeedChooserAge;
        aChosenSeed.mTimeEndMotion = mSeedChooserAge + 25;
        aChosenSeed.mStartX = aChosenSeed.mX;
        aChosenSeed.mStartY = aChosenSeed.mY;
        GetSeedPositionInBank(mSeedsInBank, aChosenSeed.mEndX, aChosenSeed.mEndY, 0);
        aChosenSeed.mSeedIndexInBank = mSeedsInBank;
        aChosenSeed.mSeedState = ChosenSeedState::SEED_FLYING_TO_BANK;
        ++mSeedsInFlight;
        ++mSeedsInBank;
        ++mSeedsIn1PBank;
    }

    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        LandFlyingSeed(GetChosenSeed(seedIndex));
    }
    CloseSeedChooser();
}

bool SeedChooserScreen::SeedNotAllowedToPick(SeedType theSeedType) {
    // 解除更多对战场地中的某些植物不能选取的问题，如泳池对战不能选荷叶，屋顶对战不能选花盆
    if (mApp->IsVSMode()) {
        if (theSeedType == mBannedSeed[theSeedType].mSeedType) {
            return true;
        }
        if (theSeedType == SeedType::SEED_INSTANT_COFFEE) {
            if (mBoard->StageIsNight() || mBanningPhase)
                return true;
        }
        if (mBanningPhase && (theSeedType == SeedType::SEED_LILYPAD || theSeedType == SeedType::SEED_FLOWERPOT)) {
            return true;
        }
        if (mBoard->StageHasPool()) {
            if (theSeedType == SeedType::SEED_LILYPAD || theSeedType == SeedType::SEED_TANGLEKELP || theSeedType == SeedType::SEED_SEASHROOM) {
                return false;
            }
            if (mBoard->StageIsNight() && theSeedType == SeedType::SEED_PLANTERN) {
                return false;
            }
        } else {
            if (theSeedType == SeedType::SEED_ZOMBIE_SNORKEL || theSeedType == SeedType::SEED_ZOMBIE_DOLPHIN_RIDER) {
                return true;
            }
        }
        if (mBoard->StageHasRoof()) {
            if (theSeedType == SeedType::SEED_FLOWERPOT) {
                return false;
            }
            if (theSeedType == SeedType::SEED_ZOMBIE_DANCER || theSeedType == SeedType::SEED_ZOMBIE_DIGGER) {
                return true;
            }
        }
        if (mShowExtendedSeeds && theSeedType == SeedType::SEED_BLOVER) {
            return false;
        }
    }

    return old_SeedChooserScreen_SeedNotAllowedToPick(this, theSeedType);
}

unsigned int SeedChooserScreen::SeedNotRecommendedToPick(SeedType theSeedType) {
    uint aRecFlags = mBoard->SeedNotRecommendedForLevel(theSeedType);
    if (TestBit(aRecFlags, NOT_RECOMMENDED_NOCTURNAL) && PickedPlantType(SEED_INSTANT_COFFEE)) {
        SetBit(aRecFlags, NOT_RECOMMENDED_NOCTURNAL, false);
    }
    ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(theSeedType);
    if (mIsZombieChooser && mBoard->StageHasPool() && (!Challenge::IsMPZombieTypeCanGoInPool(aZombieType) || theSeedType == SeedType::SEED_ZOMBIE_MOUND)) {
        aRecFlags = 0;
        SetBit(aRecFlags, NotRecommend::NOT_RECOMMENDED_ON_POOL, true);
    }
    return aRecFlags;
}

bool SeedChooserScreen::HasPacket(SeedType theSeedType, bool theIsZombie) {
    if (mPageIndex == 1 && theSeedType >= SEED_ICEBERG_LETTUCE && theSeedType < NUM_SEEDS_IN_CHOOSER_EXTENDED) {
        return true;
    }
    if (!mApp->IsVSMode()) {
        return mApp->HasSeedType(theSeedType, theIsZombie);
    }
    if (mIsZombieChooser || theSeedType >= SeedType::NUM_SEED_TYPES) {
        return true;
    }
    return mApp->HasSeedType(theSeedType, theIsZombie);
}

SeedType SeedChooserScreen::GetZombieSeedType(int theSeedIndex) {
    int aSeedType = theSeedIndex + SEED_ZOMBIE_GRAVESTONE;
    // 解锁更多对战僵尸
    return aSeedType < NUM_ZOMBIE_SEEDS_IN_CHOOSER ? SeedType(aSeedType) : SEED_NONE;
}

bool SeedChooserScreen::PickedPlantType(SeedType theSeedType) {
    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        const ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        if (aChosenSeed.mSeedState == SEED_IN_BANK) {
            if (aChosenSeed.mSeedType == theSeedType || (aChosenSeed.mSeedType == SEED_IMITATER && aChosenSeed.mImitaterType == theSeedType)) {
                return true;
            }
        }
    }
    return false;
}

SeedType SeedChooserScreen::GetPlantSeedType(int theSeedIndex) const {
    if (theSeedIndex < 0) {
        return SeedType::SEED_NONE;
    }

    // 原版植物：索引和 SeedType 相同
    if (theSeedIndex < NUM_SEEDS_IN_CHOOSER) {
        return SeedType(theSeedIndex);
    }

    // 新增植物：存储在 NUM_SEEDS_IN_CHOOSER 之后，但实际枚举从 SEED_ICEBERG_LETTUCE 开始
    const int extendedIndex = theSeedIndex - NUM_SEEDS_IN_CHOOSER;

    if (extendedIndex >= 0 && extendedIndex < NUM_SEED_TYPES_EXTENDED) {
        return SeedType(SEED_ICEBERG_LETTUCE + extendedIndex);
    }

    return SeedType::SEED_NONE;
}

int SeedChooserScreen::GetSeedStorageCount() const {
    if (mIsZombieChooser) {
        return NUM_ZOMBIE_SEEDS_IN_CHOOSER - SEED_ZOMBIE_GRAVESTONE;
    }

    return NUM_SEED_TYPES + NUM_SEED_TYPES_EXTENDED;
}

int SeedChooserScreen::GetCurrentPageSeedCount() const {
    if (mIsZombieChooser) {
        const int zombieFirstPageSeedCount = GetZombieFirstPageSeedCount(this);
        if (mPageIndex == 0) {
            return zombieFirstPageSeedCount;
        }
        return GetSeedStorageCount() - zombieFirstPageSeedCount;
    }

    if (mPageIndex == 0) {
        // VS 第一页目前只显示 0～39
        if (mApp->IsVSMode()) {
            return SEED_MELONPULT + 1;
        }
        return NUM_SEEDS_IN_CHOOSER;
    }

    return mShowExtendedSeeds ? NUM_SEED_TYPES_EXTENDED : 0;
}

int SeedChooserScreen::GetPageSeedStorageIndex(int theSeedIndex) const {
    if (theSeedIndex < 0 || theSeedIndex >= GetCurrentPageSeedCount()) {
        return -1;
    }

    if (mPageIndex == 0) {
        return theSeedIndex;
    }

    if (mIsZombieChooser) {
        return GetZombieFirstPageSeedCount(this) + theSeedIndex;
    }

    return NUM_SEEDS_IN_CHOOSER + theSeedIndex;
}

int SeedChooserScreen::GetSeedPacketIndex(int theSeedIndex) const {
    if (mIsZombieChooser) {
        if (theSeedIndex < SEED_ZOMBIE_GRAVESTONE || theSeedIndex >= NUM_ZOMBIE_SEEDS_IN_CHOOSER) {
            return -1;
        }
        return theSeedIndex - SEED_ZOMBIE_GRAVESTONE;
    }

    // 原版植物
    if (theSeedIndex >= SEED_PEASHOOTER && theSeedIndex < NUM_SEEDS_IN_CHOOSER) {
        return theSeedIndex;
    }

    // 新增植物
    if (theSeedIndex >= SEED_ICEBERG_LETTUCE && theSeedIndex < NUM_SEEDS_IN_CHOOSER_EXTENDED) {
        return NUM_SEEDS_IN_CHOOSER + (theSeedIndex - SEED_ICEBERG_LETTUCE);
    }

    return -1;
}

void SeedChooserScreen::OnPlayerPickedSeed(int thePlayerIndex) {
    VSSetupMenu *aVSSetupScreen = mApp->mVSSetupMenu;
    if (aVSSetupScreen)
        aVSSetupScreen->OnPlayerPickedSeed(thePlayerIndex);
}

SeedType SeedChooserScreen::FindSeedInBank(int theIndexInBank, int thePlayerIndex) {
    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        SeedType aPacketSeedType = mIsZombieChooser ? GetZombieSeedType(seedIndex) : GetPlantSeedType(seedIndex);
        if (HasPacket(aPacketSeedType, mIsZombieChooser) && aChosenSeed.mSeedState == SEED_IN_BANK && aChosenSeed.mSeedIndexInBank == theIndexInBank
            && aChosenSeed.mChosenPlayerIndex == thePlayerIndex) {
            return aChosenSeed.mSeedType;
        }
    }
    return SEED_NONE;
}

void SeedChooserScreen::ClickedSeedInChooser(ChosenSeed &theChosenSeed, int thePlayerIndex) {
    int selectedIndex = GetChosenSeedIndex(theChosenSeed);
    if (mApp->IsVSMode() && thePlayerIndex >= 0 && thePlayerIndex <= 1) {
        int cursorX = (thePlayerIndex == 0) ? mCursorPositionX1 : mCursorPositionX2;
        int cursorY = (thePlayerIndex == 0) ? mCursorPositionY1 : mCursorPositionY2;
        SeedType cursorSeedType = SeedHitTest(cursorX, cursorY);
        int cursorIndex = (cursorSeedType == SeedType::SEED_NONE) ? -1 : GetSeedPacketIndex(cursorSeedType);
        if (cursorIndex >= 0 && cursorIndex < GetSeedStorageCount()) {
            selectedIndex = cursorIndex;
        }
    }

    if (selectedIndex < 0 || selectedIndex >= GetSeedStorageCount()) {
        return;
    }

    ChosenSeed &selectedSeed = GetChosenSeed(selectedIndex);
    SeedType selectedSeedType = mIsZombieChooser ? GetZombieSeedType(selectedIndex) : GetPlantSeedType(selectedIndex);
    if (selectedSeedType == SeedType::SEED_NONE || !HasPacket(selectedSeedType, mIsZombieChooser)) {
        return;
    }

    // Keep local chosen-seed payload coherent with index->type mapping.
    selectedSeed.mSeedType = selectedSeedType;

    if (mApp->IsVSMode()) {
        const uint8_t cursorFlags = (mPageIndex == 1) ? kCursorPageOneEventFlag : 0;
        if (gTcpConnected) {
            // 客户端始终上报点击事件：即使选卡失败，也用于同步光标位置。
            U8x3_Event event = {{mBanningPhase ? EventType::EVENT_CLIENT_SEEDCHOOSER_BAN_SEED : EventType::EVENT_CLIENT_SEEDCHOOSER_SELECT_SEED},
                                {uint8_t(selectedSeedType), uint8_t(mIsZombieChooser), cursorFlags}};
            netplay::PutEvent(event);
            return;
        } else if (gTcpClientSocket >= 0) {
            // 主机也广播该点击，远端可据此同步光标，再由主机权威决定是否入槽。
            U8x3_Event event = {{mBanningPhase ? EventType::EVENT_SERVER_SEEDCHOOSER_BAN_SEED : EventType::EVENT_SERVER_SEEDCHOOSER_SELECT_SEED},
                                {uint8_t(selectedSeedType), uint8_t(mIsZombieChooser), cursorFlags}};
            netplay::PutEvent(event);
        }

        if (mSeedsInBank == mSeedBank1->mNumPackets || !CanPickNow()) {
            return;
        }
    }

    ClickedSeedInChooser_Orgin(selectedSeed, thePlayerIndex);
}

void SeedChooserScreen::ClickedSeedInChooser_Orgin(ChosenSeed &theChosenSeed, int thePlayerIndex) {
    int chosenSeedIndex = GetChosenSeedIndex(theChosenSeed);
    if (chosenSeedIndex < 0 || chosenSeedIndex >= GetSeedStorageCount()) {
        return;
    }

    SeedType canonicalSeedType = mIsZombieChooser ? GetZombieSeedType(chosenSeedIndex) : GetPlantSeedType(chosenSeedIndex);
    if (canonicalSeedType == SeedType::SEED_NONE || !HasPacket(canonicalSeedType, mIsZombieChooser)) {
        return;
    }
    theChosenSeed.mSeedType = canonicalSeedType;
    // 实现1P结盟选卡选满后自动转换为2P选卡
    if (mApp->IsCoopMode())
        thePlayerIndex = !m1PChoosingSeeds;

    int aGamepadIndex = mApp->PlayerToGamepadIndex(thePlayerIndex);

    // 检查是否允许选择种子
    bool canPickSeed = true;

    // 合作模式检查
    if (mApp->IsCoopMode()) {
        if (mSeedsInBank > 8) {
            canPickSeed = false;
        }
    }
    // 非合作模式检查
    else if (mSeedsInBank == mSeedBank1->mNumPackets) {
        canPickSeed = false;
    }

    // VS模式检查
    if (mApp->IsVSMode() && !CanPickNow()) {
        mApp->PlaySample(Sexy::SOUND_BUZZER);
        canPickSeed = false;
    }

    // 检查玩家种子栏容量
    if (mApp->IsCoopMode()) {
        int *aNumSeedsInBank = (&mSeedsIn1PBank + thePlayerIndex);
        if (*aNumSeedsInBank > 3) {
            canPickSeed = false;
        }
    }

    if (!canPickSeed) {
        return;
    }

    // 禁选模式（BP）
    if (mApp->IsVSMode()) {
        if (mBanningPhase) { // 如果当前处于禁用阶段
            int x = (aGamepadIndex == 1) ? mCursorPositionX2 : mCursorPositionX1;
            int y = (aGamepadIndex == 1) ? mCursorPositionY2 : mCursorPositionY1;
            SeedType aSeedType = SeedHitTest(x, y);
            if (aSeedType != SEED_NONE && !SeedNotAllowedToPick(aSeedType)) {
                BannedSeed &aBannedSeed = mBannedSeed[aSeedType];
                aBannedSeed.mSeedType = theChosenSeed.mSeedType;

                int aSeedBanned = aBannedSeed.mSeedType;
                if (mSeedsInBanned >= mNumBanPackets || aSeedBanned < 0 || aSeedBanned >= NUM_SEEDS_IN_CHOOSER_EXTENDED || mBannedSeed[aSeedBanned].mSeedState == BannedSeedState::SEED_BANNED) {
                    return;
                }

                mBannedSeed[aSeedBanned].mX = theChosenSeed.mX;
                mBannedSeed[aSeedBanned].mY = theChosenSeed.mY;
                mBannedSeed[aSeedBanned].mSeedState = BannedSeedState::SEED_BANNED; // 将被选卡设为禁用状态

                mSeedsInBanned++; // 已禁用卡片数量 + 1
                // Before the plant player locks any card, every Ban can still
                // remove the current template carry. Later Ban losses retain
                // the existing role-based fallback rather than rerolling.
                if (!mIsZombieChooser && mSeedsInBank == 0) {
                    ReplaceBuiltinAIPlantTemplateAfterOpeningBan(this);
                }
                bool banRoundFinished = false;
                SeedChooserScreen *plantChooser = mApp->mSeedChooserScreen;
                SeedChooserScreen *zombieChooser = mApp->mZombieChooserScreen;
                if (plantChooser != nullptr && zombieChooser != nullptr && plantChooser->mSeedsInBanned >= plantChooser->mNumBanPackets
                    && zombieChooser->mSeedsInBanned >= zombieChooser->mNumBanPackets) {
                    // Each chooser tracks the ban round for the opposite
                    // side. End the round only after both counters agree.
                    plantChooser->mBanningPhase = false;
                    zombieChooser->mBanningPhase = false;
                    banRoundFinished = true;
                }

                // 记录禁卡
                netplay::MetricsRecordSeedEvent(mIsZombieChooser, true, int(aSeedBanned));
                mApp->PlayFoley(FoleyType::FOLEY_FLOOP);
                OnPlayerPickedSeed(aGamepadIndex);
                if (banRoundFinished && mIsZombieChooser) {
                    // Preserve the original BP turn contract: the zombie chooser
                    // performs the final ban and the extra callback keeps the
                    // following normal pick on the expected side.
                    OnPlayerPickedSeed(aGamepadIndex);
                }
            }
            return;
        }
    }

    // 确定种子栏
    int aSeedsInBank = 0;
    if (mApp->IsCoopMode() && thePlayerIndex == 1) {
        aSeedsInBank = mSeedsIn2PBank;
    } else {
        aSeedsInBank = mSeedsIn1PBank;
    }

    // 设置种子动画参数
    theChosenSeed.mStartX = theChosenSeed.mX;
    theChosenSeed.mStartY = theChosenSeed.mY;
    theChosenSeed.mTimeStartMotion = mSeedChooserAge;
    theChosenSeed.mTimeEndMotion = mSeedChooserAge + 25;

    // 确定实际玩家索引
    int aActualPlayerIndex = 0;
    int aGlobalBpPlayerIndex = -1;
    if (mApp->IsAdventureMode()) {
        aActualPlayerIndex = 0;
        theChosenSeed.mChosenPlayerIndex = 0;
    } else {
        if (mApp->IsVSMode()) {
            VSSetupMenu *aVSSetupScreen = mApp->mVSSetupMenu;
            aActualPlayerIndex = (thePlayerIndex == 1) ? aVSSetupScreen->mSides[1] : aVSSetupScreen->mSides[0];
            theChosenSeed.mChosenPlayerIndex = aActualPlayerIndex;
            aGlobalBpPlayerIndex = thePlayerIndex;
        } else {
            aActualPlayerIndex = thePlayerIndex;
            theChosenSeed.mChosenPlayerIndex = thePlayerIndex;
        }
    }

    // 获取种子在种子栏中的位置
    GetSeedPositionInBank(aSeedsInBank, theChosenSeed.mEndX, theChosenSeed.mEndY, aActualPlayerIndex);

    // 更新种子状态和计数
    theChosenSeed.mSeedIndexInBank = aSeedsInBank;
    theChosenSeed.mSeedState = SEED_FLYING_TO_BANK;

    if (mApp->IsVSMode() && !mBanningPhase && VSSetupAddonWidget::msGlobalBpMode != VSSetupAddonWidget::GLOBALBP_CLOSED && aGlobalBpPlayerIndex >= 0 && aGlobalBpPlayerIndex <= 1) {
        SeedType *globalBpSeeds = VSSetupAddonWidget::msGlobalBpSeeds[aGlobalBpPlayerIndex];
        for (int i = 0; i < VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer; ++i) {
            if (globalBpSeeds[i] == theChosenSeed.mSeedType) {
                break;
            }
            if (globalBpSeeds[i] == SeedType::SEED_NONE) {
                if (theChosenSeed.mSeedType == SeedType::SEED_INSTANT_COFFEE) {
                    break; // 咖啡豆不参与全局禁用
                }
                globalBpSeeds[i] = theChosenSeed.mSeedType;
                break;
            }
        }
    }

    mSeedsInFlight++;
    mSeedsInBank++;
    // 记录选卡
    netplay::MetricsRecordSeedEvent(mIsZombieChooser, mBanningPhase, int(theChosenSeed.mSeedType));

    if (mApp->IsCoopMode() && thePlayerIndex == 1) {
        mSeedsIn2PBank++;
    } else {
        mSeedsIn1PBank++;
    }

    // 播放音效并更新UI
    RemoveToolTip(thePlayerIndex);
    mApp->PlaySample(Sexy::SOUND_TAP);

    // 检查是否启用开始按钮
    if (!mApp->IsCoopMode() && mSeedsInBank == mSeedBank1->mNumPackets) {
        EnableStartButton(true);
    }

    if (mApp->IsCoopMode() && mSeedsInBank == (mSeedBank2->mNumPackets + mSeedBank1->mNumPackets)) {
        EnableStartButton(true);
    }

    // VS模式特殊处理
    if (mApp->IsVSMode()) {
        OnPlayerPickedSeed(aGamepadIndex);

        // 当植物完成第三次选卡，开启第二轮禁用
        if (!mIsZombieChooser && mSeedsInBanned > 0 && mSeedsIn1PBank == 4) {
            // 需禁用数量增加 2
            mApp->mSeedChooserScreen->mNumBanPackets += 2;
            mApp->mZombieChooserScreen->mNumBanPackets += 2;
            // 重新开启禁用阶段
            mApp->mSeedChooserScreen->mBanningPhase = true;
            mApp->mZombieChooserScreen->mBanningPhase = true;
            OnPlayerPickedSeed(aGamepadIndex);
        }
    }
}

void SeedChooserScreen::CrazyDavePickSeeds() {
    if (daveNoPickSeeds && !IsOnlineServerModeActive() && !gIsReplayMode) {
        return;
    }

    TodWeightedArray aSeedArray[NUM_SEED_TYPES];
    for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1)) {
        aSeedArray[aSeedType].mItem = aSeedType;
        aSeedArray[aSeedType].mWeight = 0;
        if (!HasPacket(aSeedType, mIsZombieChooser) || SeedNotRecommendedToPick(aSeedType) != 0 || SeedNotAllowedToPick(aSeedType) || Plant::IsUpgrade(aSeedType) || aSeedType == SEED_IMITATER
            || aSeedType == SEED_UMBRELLA || aSeedType == SEED_BLOVER) {
            continue;
        }
        aSeedArray[aSeedType].mWeight = 1;
    }

    if (mBoard->mZombieAllowed[ZOMBIE_BUNGEE] || mBoard->mZombieAllowed[ZOMBIE_CATAPULT]) {
        aSeedArray[SEED_UMBRELLA].mWeight = 1;
    }

    if (mBoard->mZombieAllowed[ZOMBIE_BALLOON] || mBoard->StageHasFog()) {
        aSeedArray[SEED_BLOVER].mWeight = 1;
    }

    if (mBoard->StageHasRoof()) {
        aSeedArray[SEED_TORCHWOOD].mWeight = 0;
    }

    auto aLevelRNG = MTRand(mBoard->GetLevelRandSeed());

    for (int i = 0; i < 3; ++i) {
        auto aPickedSeed = SeedType(PickFromWeightedArrayUsingSpecialRandSeed(aSeedArray, NUM_SEEDS_IN_CHOOSER, aLevelRNG));
        aSeedArray[aPickedSeed].mWeight = 0;
        ChosenSeed &aChosenSeed = GetChosenSeed(aPickedSeed);

        int aPosX = 0;
        int aPosY = 0;
        GetSeedPositionInBank(i, aPosX, aPosY, 0);
        aChosenSeed.mX = aPosX;
        aChosenSeed.mY = aPosY;
        aChosenSeed.mStartX = aPosX;
        aChosenSeed.mStartY = aPosY;
        aChosenSeed.mEndX = aPosX;
        aChosenSeed.mEndY = aPosY;
        aChosenSeed.mSeedState = SEED_IN_BANK;
        aChosenSeed.mSeedIndexInBank = i;
        aChosenSeed.mChosenPlayerIndex = 0;
        aChosenSeed.mCrazyDavePicked = true;
        ++mSeedsInBank;
        ++mSeedsIn1PBank;
    }
}

void SeedChooserScreen::ClickedSeedInBank(ChosenSeed &theChosenSeed, int thePlayerIndex) {
    // 解决结盟1P选够4个种子之后，无法点击种子栏内的已选种子来退选的问题
    if (mApp->IsCoopMode()) {
        thePlayerIndex = theChosenSeed.mChosenPlayerIndex;
    }

    int chosenPlayerIndex = 0;
    GamepadControls *controls = mBoard->mGamepadControls[thePlayerIndex];
    if (controls != nullptr) {
        chosenPlayerIndex = controls->mPlayerIndex;
    }


    if (mApp->IsCoopMode() && chosenPlayerIndex != theChosenSeed.mChosenPlayerIndex) {
        return;
    }

    int seedSlotsInBank = mSeedBank1->mNumPackets;
    if (mApp->IsCoopMode()) {
        seedSlotsInBank = 4;
    }

    for (int bankIndex = theChosenSeed.mSeedIndexInBank + 1; bankIndex < seedSlotsInBank; ++bankIndex) {
        int bankOwner = chosenPlayerIndex;
        if (!mApp->IsCoopMode() && !mApp->IsVSMode()) {
            bankOwner = 0;
        }

        int newBankIndex = bankIndex - 1;
        SeedType seedInBank = FindSeedInBank(bankIndex, bankOwner);
        if (seedInBank == SeedType::SEED_NONE) {
            continue;
        }

        ChosenSeed &bankSeed = GetChosenSeed(GetSeedPacketIndex(seedInBank));
        bankSeed.mStartX = bankSeed.mX;
        bankSeed.mStartY = bankSeed.mY;
        bankSeed.mTimeStartMotion = mSeedChooserAge;
        bankSeed.mTimeEndMotion = mSeedChooserAge + 15;
        GetSeedPositionInBank(newBankIndex, bankSeed.mEndX, bankSeed.mEndY, bankSeed.mChosenPlayerIndex);
        bankSeed.mSeedState = ChosenSeedState::SEED_FLYING_TO_BANK;
        bankSeed.mSeedIndexInBank = newBankIndex;
        ++mSeedsInFlight;
    }

    theChosenSeed.mTimeStartMotion = mSeedChooserAge;
    theChosenSeed.mStartX = theChosenSeed.mX;
    theChosenSeed.mStartY = theChosenSeed.mY;
    theChosenSeed.mTimeEndMotion = mSeedChooserAge + 25;
    GetSeedPositionInChooser(GetSeedPacketIndex(theChosenSeed.mSeedType), theChosenSeed.mEndX, theChosenSeed.mEndY);
    theChosenSeed.mSeedState = ChosenSeedState::SEED_FLYING_TO_CHOOSER;
    theChosenSeed.mSeedIndexInBank = 0;
    ++mSeedsInFlight;
    --mSeedsInBank;

    if (chosenPlayerIndex == 1 && mApp->IsCoopMode()) {
        --mSeedsIn2PBank;
    } else {
        --mSeedsIn1PBank;
    }

    RemoveToolTip(chosenPlayerIndex);
    EnableStartButton(false);
    mApp->PlaySample(Sexy::SOUND_TAP);
}

bool SeedChooserScreen::OnKeyDown(KeyCode theKey, unsigned int theEventFlag) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return false;
    }

    if (theKey == KEYCODE_GAMEPAD_A || theKey == KEYCODE_GAMEPAD_B) {
        return false;
    }

    auto confirmQuit = [&]() {
        Sexy::Dialog *confirmQuitDialog = mApp->ConfirmQuit();
        mOpeningDialog = true;
        int result = confirmQuitDialog->WaitForResult(true);
        mOpeningDialog = false;
        if (result == 1000) {
            mApp->PostLeaveLevel();
            mApp->SetBoardResult(BOARDRESULT_QUIT);
            mApp->DoBackToMain();
        }
    };

    auto clearButtonFocus = [&]() {
        if (mButtonSlotState == 0) {
            return;
        }
        auto *buttonWidget = reinterpret_cast<Sexy::Widget *>(mButtonSlotState);
        buttonWidget->mIsOver = false;
        buttonWidget->mIsDown = false;
        mButtonSlotState = 0;
    };

    auto setButtonFocus = [&](Sexy::Widget *widget) {
        clearButtonFocus();
        if (widget == nullptr) {
            return;
        }
        widget->mIsOver = true;
        mButtonSlotState = reinterpret_cast<int>(widget);
    };

    auto getFocusedButtonIndex = [&](const ButtonVector &buttons) {
        if (mButtonSlotState == 0) {
            return -1;
        }
        auto *focused = reinterpret_cast<Sexy::Widget *>(mButtonSlotState);
        for (int i = 0; i < int(buttons.size()); ++i) {
            if (buttons[i] == focused) {
                return i;
            }
        }
        return -1;
    };

    auto advanceCursorAfterPick = [&]() {
        int rowCount = 5;
        if (mApp->mGameMode != GameMode::GAMEMODE_MP_VS && Has7Rows()) {
            rowCount = 6;
        }

        bool searchFailed = false;
        while (true) {
            SeedType seedType = SeedHitTest(mCursorPositionX1, mCursorPositionY1);
            if (seedType == SeedType::SEED_NONE) {
                searchFailed = true;
                break;
            }

            const int seedIndex = GetSeedPacketIndex(seedType);
            if (seedIndex >= 0 && seedIndex < GetSeedStorageCount() && GetChosenSeed(seedIndex).mSeedState > ChosenSeedState::SEED_IN_BANK && !SeedNotRecommendedToPick(seedType)) {
                return;
            }

            ++mSeedIndex1;
            const int maxSeedIndex = NumColumns() * rowCount;
            if (mSeedIndex1 >= maxSeedIndex) {
                searchFailed = true;
                mSeedIndex1 = maxSeedIndex - 1;
            }
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            if (searchFailed) {
                break;
            }
        }

        const int maxSeedIndex = NumColumns() * rowCount - 1;
        while (mSeedIndex1 < maxSeedIndex) {
            ++mSeedIndex1;
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            SeedType seedType = SeedHitTest(mCursorPositionX1, mCursorPositionY1);
            const int seedIndex = GetSeedPacketIndex(seedType);
            if (seedType != SeedType::SEED_NONE && !SeedNotRecommendedToPick(seedType) && seedIndex >= 0 && seedIndex < GetSeedStorageCount()
                && unsigned(GetChosenSeed(seedIndex).mSeedState - ChosenSeedState::SEED_FLYING_TO_CHOOSER) <= 1) {
                return;
            }
        }

        while (mSeedIndex1 > SeedType::SEED_PEASHOOTER) {
            --mSeedIndex1;
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            SeedType seedType = SeedHitTest(mCursorPositionX1, mCursorPositionY1);
            const int seedIndex = GetSeedPacketIndex(seedType);
            if (seedType != SeedType::SEED_NONE && !SeedNotRecommendedToPick(seedType) && seedIndex >= 0 && seedIndex < GetSeedStorageCount()
                && unsigned(GetChosenSeed(seedIndex).mSeedState - ChosenSeedState::SEED_FLYING_TO_CHOOSER) <= 1) {
                return;
            }
        }
    };

    if (mSeedsInFlight > 0) {
        for (int i = 0; i < GetSeedStorageCount(); ++i) {
            LandFlyingSeed(GetChosenSeed(i));
        }
    }

    if (mBoard->mGamepadControls[0]->mGamepadIndex == -1) {
        return false;
    }

    if (mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN && theKey == KEYCODE_RETURN) {
        if (CancelLawnView()) {
            RebuildHelpbar();
            return true;
        }
    }

    const int nextDownSeed = GetNextSeedInDir(mSeedIndex1, SeedDir::SEED_DIR_DOWN);
    if (nextDownSeed == mSeedIndex1 && theKey == KEYCODE_DOWN && mButtonSlotState == 0) {
        const ButtonVector &buttons = *mButtons;
        Sexy::Widget *bestButton = nullptr;
        int bestDistance = INT_MAX;
        for (GameButton *button : buttons) {
            if (button == nullptr || !button->mVisible || button->mDisabled) {
                continue;
            }

            const int dx = button->mX - mCursorPositionX1;
            const int dy = button->mY - mCursorPositionY1;
            const int distance = dx * dx + dy * dy;
            if (distance < bestDistance) {
                bestDistance = distance;
                bestButton = button;
            }
        }
        if (bestButton != nullptr) {
            setButtonFocus(bestButton);
            return true;
        }
    }

    if (mButtonSlotState != 0) {
        const ButtonVector &buttons = *mButtons;
        const int focusedIndex = getFocusedButtonIndex(buttons);
        if (theKey == KEYCODE_LEFT || theKey == KEYCODE_RIGHT) {
            if (focusedIndex >= 0 && !buttons.empty()) {
                const int step = (theKey == KEYCODE_LEFT) ? -1 : 1;
                for (int offset = 1; offset < int(buttons.size()); ++offset) {
                    const int index = (focusedIndex + step * offset + int(buttons.size())) % int(buttons.size());
                    GameButton *button = buttons[index];
                    if (button != nullptr && button->mVisible && !button->mDisabled) {
                        if (button != reinterpret_cast<Sexy::Widget *>(mButtonSlotState)) {
                            setButtonFocus(button);
                        }
                        return true;
                    }
                }
            }
        } else if (theKey == KEYCODE_UP) {
            clearButtonFocus();
            return true;
        } else if (theKey == KEYCODE_RETURN) {
            reinterpret_cast<Sexy::Widget *>(mButtonSlotState)->mIsDown = true;
            return true;
        }
    }

    switch (theKey) {
        case KEYCODE_LEFT:
            mSeedIndex1 = GetNextSeedInDir(mSeedIndex1, SeedDir::SEED_DIR_LEFT);
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            return true;

        case KEYCODE_UP:
            mSeedIndex1 = GetNextSeedInDir(mSeedIndex1, SeedDir::SEED_DIR_UP);
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            return true;

        case KEYCODE_RIGHT:
            if (mSeedIndex1 % 8 == 7 && HasPacket(SeedType::SEED_IMITATER, false) && mApp->mGameMode != GameMode::GAMEMODE_MP_VS) {
                mSeedIndex1 = SeedType::SEED_IMITATER;
            } else {
                mSeedIndex1 = GetNextSeedInDir(mSeedIndex1, SeedDir::SEED_DIR_RIGHT);
            }
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            return true;

        case KEYCODE_DOWN:
            mSeedIndex1 = GetNextSeedInDir(mSeedIndex1, SeedDir::SEED_DIR_DOWN);
            GetSeedPositionInChooser(mSeedIndex1, mCursorPositionX1, mCursorPositionY1);
            return true;

        case KEYCODE_RETURN: {
            if (mSeedIndex1 == SeedType::SEED_IMITATER && mSeedsInBank < mSeedBank1->mNumPackets) {
                if (GetChosenSeed(SeedType::SEED_IMITATER).mSeedState == ChosenSeedState::SEED_IN_BANK) {
                    ClickedSeedInBank(GetChosenSeed(SeedType::SEED_IMITATER), 0);
                } else {
                    mImitaterDialog = new ImitaterDialog(0);
                    AddWidget(mImitaterDialog);
                    mImitaterDialog->LawnDialog::Resize((800 - mImitaterDialog->mWidth) / 2, (600 - mImitaterDialog->mHeight) / 2, mImitaterDialog->mWidth, mImitaterDialog->mHeight);
                    mApp->mWidgetManager->SetFocus(mImitaterDialog);
                }
                return true;
            }

            const SeedType seedType = SeedHitTest(mCursorPositionX1, mCursorPositionY1);
            if (seedType == SeedType::SEED_NONE) {
                return false;
            }
            if (SeedNotAllowedToPick(seedType)) {
                mApp->PlaySample(Sexy::SOUND_BUZZER);
                return true;
            }
            if (SeedNotAllowedDuringTrial(seedType)) {
                mApp->PlaySample(Sexy::SOUND_TAP);
                if (mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[GET_FULL_VERSION_TITLE]", "[GET_FULL_VERSION_BODY]", "[GET_FULL_VERSION_YES_BUTTON]", "[GET_FULL_VERSION_NO_BUTTON]", 1) == 1000) {
                    mApp->BuyFullVersion();
                    mApp->DoBackToMain();
                }
                return true;
            }

            const int seedIndex = GetSeedPacketIndex(seedType);
            if (seedIndex < 0 || seedIndex >= GetSeedStorageCount()) {
                return true;
            }

            ChosenSeed &chosenSeed = GetChosenSeed(seedIndex);
            if (chosenSeed.mSeedState == ChosenSeedState::SEED_IN_BANK) {
                if (chosenSeed.mCrazyDavePicked) {
                    mApp->PlaySample(Sexy::SOUND_BUZZER);
                    mToolTip1->FlashWarning();
                } else {
                    if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
                        return false;
                    }
                    ClickedSeedInBank(chosenSeed, 0);
                }
                return true;
            }

            if (chosenSeed.mSeedState != ChosenSeedState::SEED_IN_CHOOSER) {
                return false;
            }

            ClickedSeedInChooser(chosenSeed, 0);
            advanceCursorAfterPick();
            return true;
        }

        case KEYCODE_ESCAPE: {
            if (mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN) {
                return true;
            }
            if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
                confirmQuit();
                return true;
            }

            bool hasSelectableBankSeed = false;
            for (int i = 0; i < GetSeedStorageCount(); ++i) {
                if (!GetChosenSeed(i).mCrazyDavePicked && GetChosenSeed(i).mSeedState == ChosenSeedState::SEED_IN_BANK) {
                    hasSelectableBankSeed = true;
                    break;
                }
            }

            if (!hasSelectableBankSeed || mSeedsInBank == 0) {
                confirmQuit();
                return true;
            }

            int selectedCount = mSeedsInBank;
            if (mApp->IsCoopMode()) {
                selectedCount = 0;
                for (int i = 0; i < GetSeedStorageCount(); ++i) {
                    if (GetChosenSeed(i).mChosenPlayerIndex == 0 && GetChosenSeed(i).mSeedState == ChosenSeedState::SEED_IN_BANK) {
                        ++selectedCount;
                    }
                }
            }

            const int removeBankIndex = selectedCount - 1;
            for (int i = 0; i < GetSeedStorageCount(); ++i) {
                ChosenSeed &chosenSeed = GetChosenSeed(i);
                if (chosenSeed.mSeedState == ChosenSeedState::SEED_IN_BANK && chosenSeed.mSeedIndexInBank == removeBankIndex && !chosenSeed.mCrazyDavePicked && chosenSeed.mChosenPlayerIndex == 0) {
                    ClickedSeedInBank(chosenSeed, 0);
                    return true;
                }
            }
            return true;
        }

        default:
            return false;
    }
}

void SeedChooserScreen::GameButtonDown(GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }

    struct SeedsInBankRestoreGuard {
        SeedChooserScreen *screen;
        bool active = false;
        int value = 0;

        ~SeedsInBankRestoreGuard() {
            if (active) {
                screen->mSeedsInBank = value;
            }
        }
    } seedsInBankRestore{this};

    unsigned int gamepadIndex = static_cast<unsigned int>(thePlayerIndex);
    if (mPlayerIndex == 0) {
        if (gamepadIndex != 0) {
            bool singlePad = false;

            Sexy::Gamepad *runtime = mApp->mGamepads[0];

            if (runtime != nullptr) {
                singlePad = *reinterpret_cast<const int *>(reinterpret_cast<const std::uint8_t *>(runtime) + 0x19C) <= 1;
            } else {
                singlePad = static_cast<int>(mApp->mGamePad1IsOn) + static_cast<int>(mApp->mGamePad2IsOn) <= 1;
            }


            if (singlePad) {
                mApp->SwapGamepadId(0, thePlayerIndex);
                gamepadIndex = 0;
            }
        }
    }

    if (gamepadIndex != static_cast<unsigned int>(mPlayerIndex)) {
        int secondPlayerGamepadIndex = mApp->mSecondPlayerGamepadIndex;
        if (secondPlayerGamepadIndex == -1) {
            if (theButton != GamepadButton::GAMEPAD_BUTTON_START) {
                return;
            }

            if (!mApp->IsAdventureMode()) {
                return;
            }

            mApp->SetSecondPlayer(static_cast<int>(gamepadIndex));
            secondPlayerGamepadIndex = mApp->mSecondPlayerGamepadIndex;
        }

        if (secondPlayerGamepadIndex != static_cast<int>(gamepadIndex)) {
            return;
        }
    }

    int playerIndex = mApp->GamepadToPlayerIndex(gamepadIndex);

    if (mSeedsInFlight > 0) {
        for (int i = 0; i < GetSeedStorageCount(); i++) {
            LandFlyingSeed(GetChosenSeed(i));
        }
    }

    if (mBoard->mGamepadControls[playerIndex]->mGamepadIndex == -1) {
        return;
    }

    if (mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN && theButton == GamepadButton::GAMEPAD_BUTTON_A) {
        if (CancelLawnView()) {
            RebuildHelpbar();
        }
        return;
    }

    int &cursorX = playerIndex == 0 ? mCursorPositionX1 : mCursorPositionX2;
    int &cursorY = playerIndex == 0 ? mCursorPositionY1 : mCursorPositionY2;
    int &cursorSeed = playerIndex == 0 ? mSeedIndex1 : mSeedIndex2;

    auto moveCursor = [&](SeedDir dir) {
        cursorSeed = GetNextSeedInDir(cursorSeed, dir);
        GetSeedPositionInChooser(cursorSeed, cursorX, cursorY);
    };

    switch (theButton) {
        case GamepadButton::GAMEPAD_BUTTON_UP:
        case GamepadButton::GAMEPAD_BUTTON_DPAD_UP:
            moveCursor(SeedDir::SEED_DIR_UP);
            return;

        case GamepadButton::GAMEPAD_BUTTON_DOWN:
        case GamepadButton::GAMEPAD_BUTTON_DPAD_DOWN:
            moveCursor(SeedDir::SEED_DIR_DOWN);
            return;

        case GamepadButton::GAMEPAD_BUTTON_LEFT:
        case GamepadButton::GAMEPAD_BUTTON_DPAD_LEFT:
            moveCursor(SeedDir::SEED_DIR_LEFT);
            return;

        case GamepadButton::GAMEPAD_BUTTON_RIGHT:
        case GamepadButton::GAMEPAD_BUTTON_DPAD_RIGHT:
            if (cursorSeed % 8 == 7 && HasPacket(SeedType::SEED_IMITATER, false) && mApp->mGameMode != GameMode::GAMEMODE_MP_VS) {
                cursorSeed = SeedType::SEED_IMITATER;
                GetSeedPositionInChooser(cursorSeed, cursorX, cursorY);
                return;
            }

            moveCursor(SeedDir::SEED_DIR_RIGHT);
            return;

        case GamepadButton::GAMEPAD_BUTTON_SELECT:
        case GamepadButton::GAMEPAD_BUTTON_THUMBL:
            if (mBoard->mCutScene->IsSurvivalRepick() && mChooseState == SeedChooserState::CHOOSE_NORMAL) {
                ButtonDepress(SeedChooserScreen::SeedChooserScreen_ViewLawn);
            }
            return;

        case GamepadButton::GAMEPAD_BUTTON_START:
            if (mShowHelpText && mApp->mGameMode != GameMode::GAMEMODE_MP_VS) {
                ButtonDepress(SeedChooserScreen::SeedChooserScreen_Start);
            }
            return;

        case GamepadButton::GAMEPAD_BUTTON_A: {
            if (mApp->mSecondPlayerGamepadIndex == -1 && mPlayerIndex != static_cast<int>(gamepadIndex)) {
                return;
            }

            // 修复结盟 2P 无法选择模仿者。原版这里用的是单人 4 卡判断，结盟扩展到 8 卡后会错误拦截。
            if (mApp->IsCoopMode() && cursorSeed == SeedType::SEED_IMITATER && mSeedsInBank < 8) {
                if (GetChosenSeed(SeedType::SEED_IMITATER).mSeedState != ChosenSeedState::SEED_IN_BANK) {
                    seedsInBankRestore.value = mSeedsInBank;
                    mSeedsInBank = 0;
                    seedsInBankRestore.active = true;
                }
            }

            if (cursorSeed == SeedType::SEED_IMITATER && mSeedsInBank < mSeedBank1->mNumPackets) {
                if (GetChosenSeed(SeedType::SEED_IMITATER).mSeedState != ChosenSeedState::SEED_IN_BANK) {
                    mImitaterDialog = new ImitaterDialog(static_cast<int>(gamepadIndex));
                    AddWidget(mImitaterDialog);
                    mImitaterDialog->LawnDialog::Resize((BOARD_WIDTH - mImitaterDialog->mWidth) / 2, (BOARD_HEIGHT - mImitaterDialog->mHeight) / 2, mImitaterDialog->mWidth, mImitaterDialog->mHeight);
                    mApp->mWidgetManager->SetFocus(mImitaterDialog);
                    return;
                }

                ClickedSeedInBank(GetChosenSeed(SeedType::SEED_IMITATER), playerIndex);
            }

            SeedType seedType = SeedHitTest(cursorX, cursorY);
            if (seedType == SeedType::SEED_NONE) {
                return;
            }

            if (SeedNotAllowedToPick(seedType)) {
                mApp->PlaySample(Sexy::SOUND_BUZZER);
                return;
            }

            if (SeedNotAllowedDuringTrial(seedType)) {
                mApp->PlaySample(Sexy::SOUND_TAP);
                if (mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[GET_FULL_VERSION_TITLE]", "[GET_FULL_VERSION_BODY]", "[GET_FULL_VERSION_YES_BUTTON]", "[GET_FULL_VERSION_NO_BUTTON]", 1) != 1000) {
                    return;
                }

                mApp->BuyFullVersion();
                mApp->DoBackToMain();
                return;
            }

            ChosenSeed &chosenSeed = GetChosenSeed(GetSeedPacketIndex(seedType));
            if (chosenSeed.mSeedState != ChosenSeedState::SEED_IN_BANK) {
                if (chosenSeed.mSeedState == ChosenSeedState::SEED_IN_CHOOSER) {
                    ClickedSeedInChooser(chosenSeed, playerIndex);
                }
                return;
            }

            if (chosenSeed.mCrazyDavePicked) {
                mApp->PlaySample(Sexy::SOUND_BUZZER);
                ToolTipWidget *toolTip = playerIndex == 0 ? mToolTip1 : mToolTip2;
                toolTip->FlashWarning();
                return;
            }

            if (mApp->IsVSMode()) {
                if (IsLocalChooserInputAllowed(this)) {
                    int aPlayerIndex = mApp->GamepadToPlayerIndex(thePlayerIndex);
                    int x = (aPlayerIndex == 0) ? mCursorPositionX1 : mCursorPositionX2;
                    int y = (aPlayerIndex == 0) ? mCursorPositionY1 : mCursorPositionY2;
                    SeedType aSeedType = SeedHitTest(x, y);
                    if (aSeedType != SeedType::SEED_NONE) {
                        const uint8_t cursorFlags = kCursorMoveOnlyEventFlag | ((mPageIndex == 1) ? kCursorPageOneEventFlag : 0);
                        if (gTcpConnected) {
                            U8x3_Event event = {{EventType::EVENT_CLIENT_SEEDCHOOSER_SELECT_SEED}, {uint8_t(aSeedType), uint8_t(mIsZombieChooser), cursorFlags}};
                            netplay::PutEvent(event);
                        } else if (gTcpClientSocket >= 0) {
                            U8x3_Event event = {{EventType::EVENT_SERVER_SEEDCHOOSER_SELECT_SEED}, {uint8_t(aSeedType), uint8_t(mIsZombieChooser), cursorFlags}};
                            netplay::PutEvent(event);
                        }
                    }
                }
                return;
            }

            ClickedSeedInBank(chosenSeed, playerIndex);
            return;
        }

        case GamepadButton::GAMEPAD_BUTTON_B: {
            if (mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN) {
                return;
            }

            if (mApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
                Sexy::Dialog *confirmQuitDialog = mApp->ConfirmQuit();
                mOpeningDialog = true;
                int result = confirmQuitDialog->WaitForResult(true);
                mOpeningDialog = false;
                if (result == 1000) {
                    mApp->PostLeaveLevel();
                    mApp->SetBoardResult(BOARDRESULT_QUIT);
                    mApp->DoBackToMain();
                }
                return;
            }

            bool hasSelectableBankSeed = false;
            for (int i = 0; i < GetSeedStorageCount(); i++) {
                if (!GetChosenSeed(i).mCrazyDavePicked && GetChosenSeed(i).mSeedState == ChosenSeedState::SEED_IN_BANK) {
                    hasSelectableBankSeed = true;
                    break;
                }
            }

            if (!hasSelectableBankSeed || mSeedsInBank == 0) {
                Sexy::Dialog *confirmQuitDialog = mApp->ConfirmQuit();
                mOpeningDialog = true;
                int result = confirmQuitDialog->WaitForResult(true);
                mOpeningDialog = false;
                if (result == 1000) {
                    mApp->PostLeaveLevel();
                    mApp->SetBoardResult(BOARDRESULT_QUIT);
                    mApp->DoBackToMain();
                }
                return;
            }

            int selectedCount = 0;
            if (mApp->IsCoopMode()) {
                for (int i = 0; i < GetSeedStorageCount(); i++) {
                    if (GetChosenSeed(i).mChosenPlayerIndex == playerIndex && GetChosenSeed(i).mSeedState == ChosenSeedState::SEED_IN_BANK) {
                        ++selectedCount;
                    }
                }
            } else {
                selectedCount = mSeedsInBank;
            }

            int removeBankIndex = selectedCount - 1;
            int chosenPlayerIndex = mApp->IsCoopMode() ? playerIndex : 0;
            for (int i = 0; i < GetSeedStorageCount(); i++) {
                if (GetChosenSeed(i).mSeedState == ChosenSeedState::SEED_IN_BANK && GetChosenSeed(i).mSeedIndexInBank == removeBankIndex && !GetChosenSeed(i).mCrazyDavePicked
                    && GetChosenSeed(i).mChosenPlayerIndex == chosenPlayerIndex) {
                    ClickedSeedInBank(GetChosenSeed(i), playerIndex);
                    return;
                }
            }
            return;
        }

        case GamepadButton::GAMEPAD_BUTTON_X:
            if (mApp->CanShowStore() && mApp->mGameMode != GameMode::GAMEMODE_MP_VS) {
                ButtonDepress(SeedChooserScreen::SeedChooserScreen_Store);
            }
            return;

        case GamepadButton::GAMEPAD_BUTTON_Y:
            if (mApp->CanShowAlmanac() && mApp->mGameMode != GameMode::GAMEMODE_MP_VS) {
                ButtonDepress(SeedChooserScreen::SeedChooserScreen_Almanac);
            }
            return;

        case GamepadButton::GAMEPAD_BUTTON_TR:
            if (mPageButton) {
                ButtonDepress(SeedChooserScreen::SeedChooserScreen_Page);
            }
            return;

        default:
            return;
    }
}

void SeedChooserScreen::DrawPacket(
    Sexy::Graphics *g, int x, int y, SeedType theSeedType, SeedType theImitaterType, float thePercentDark, int theGrayness, Color *theColor, bool theDrawCost, bool theUseCurrentCost) {
    if (theSeedType == SEED_NONE) {
        return; // 绘制SEED_NONE会导致闪退，这里做个安全检查
    }


    // 修复SeedChooser里的卡片亮度不正确。
    // 已选的卡片grayness为55，不推荐的卡片grayness为115。theColor则固定为{255,255,255,255}。

    // int aConvertedGrayness = ((theColor->mRed + theColor->mGreen + theColor->mBlue) / 3 + theGrayness) / 2;
    // 此算法用于在对战模式将非选卡的一方的卡片整体变暗。但这种算法下，55亮度会变成155亮度，115亮度会变成185亮度，严重影响非对战模式的选卡体验。所以需要修复。
    int aConvertedGrayness = (mApp->IsVSMode()) ? ((theColor->mRed + theColor->mGreen + theColor->mBlue) / 3 + theGrayness) / 2 : theGrayness;
    if (mApp->IsVSMode()) {
        if (mIsZombieChooser && SeedNotAllowedToPick(theSeedType)) {
            if (CanPickNow())
                aConvertedGrayness = 115;
            else
                aConvertedGrayness = 55;
        }

        // 禁用阶段种子栏中的卡变灰
        if (mBanningPhase) {
            for (int i = 0; i < GetSeedStorageCount(); i++) {
                if (GetChosenSeed(i).mSeedType == theSeedType && GetChosenSeed(i).mSeedState == ChosenSeedState::SEED_IN_BANK) {
                    aConvertedGrayness = 115;
                }
            }
        }
    }
    DrawSeedPacket(g, x, y, theSeedType, theImitaterType, thePercentDark, aConvertedGrayness, theDrawCost, false, mIsZombieChooser, theUseCurrentCost);
}

void SeedChooserScreen::ButtonPress(int theId) {
    LawnApp *lawnApp = gLawnApp;
    lawnApp->mSeedChooserScreen->mFocusedChildWidget = nullptr; // 修复点击菜单后无法按键选取植物卡片
}

void SeedChooserScreen::ButtonDepress(int theId) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (mApp->IsVSMode() && !IsLocalChooserInputAllowed(this)) {
        return;
    }

    if (mApp->IsVSMode()) {
        if (gTcpConnected) {
            U8U8_Event event = {{EventType::EVENT_CLIENT_SEEDCHOOSER_BUTTON_DEPRESS}, uint8_t(theId), uint8_t(mIsZombieChooser)};
            netplay::PutEvent(event);
        } else if (gTcpClientSocket >= 0) {
            U8U8_Event event = {{EventType::EVENT_SERVER_SEEDCHOOSER_BUTTON_DEPRESS}, uint8_t(theId), uint8_t(mIsZombieChooser)};
            netplay::PutEvent(event);
        }
    }

    ButtonDepress_Origin(theId);
}

void SeedChooserScreen::ButtonDepress_Origin(int theId) {
    if (mSeedsInFlight > 0 || mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN || !mMouseVisible) {
        return;
    }

    if (theId == SeedChooserScreen_Menu) {
        mApp->PlaySample(Sexy::SOUND_PAUSE);
        mApp->DoNewOptions(false, 0);
        return;
    }

    if (theId == SeedChooserScreen_Page) {
        SetPageIndex(mPageIndex == 0 ? 1 : 0);
        return;
    }

    if (theId == SeedChooserScreen_ViewLawn) {
        mChooseState = SeedChooserState::CHOOSE_VIEW_LAWN;
        mViewLawnTime = 0;
        gLawnApp->HideHelpBarWidget();
        mApp->GetSeedsAvailable(mIsZombieChooser);
    } else if (theId == SeedChooserScreen_Almanac) {
        AlmanacDialog *almanacDialog = mApp->DoAlmanacDialog();
        almanacDialog->WaitForResult();
        mApp->mMusic->MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
        mApp->GetSeedsAvailable(mIsZombieChooser);
    } else if (theId == SeedChooserScreen_Store) {
        StoreScreen *storeScreen = mApp->ShowStoreScreen();
        storeScreen->WaitForResult(true);
        if (storeScreen->mGoToTreeNow) {
            mApp->KillBoard();
            mApp->PreNewGame(GameMode::GAMEMODE_TREE_OF_WISDOM, false);
            return;
        }
        mWidgetManager->SetFocus(this);
        mApp->mMusic->MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
        mApp->GetSeedsAvailable(mIsZombieChooser);
    } else if (mApp->GetSeedsAvailable(mIsZombieChooser) >= mSeedBank1->mNumPackets) {
        if (theId == SeedChooserScreen_Start) {
            OnStartButton();
        } else if (theId == SeedChooserScreen_Random) {
            PickRandomSeeds();
        }
    }
}

void SeedChooserScreen::GetSeedPositionInBank(int theIndex, int &x, int &y, int thePlayerIndex) {
    SeedBank *seedBank = mSeedBank1;
    int seedBankIndex = 1;
    if (mApp->IsCoopMode() && thePlayerIndex) {
        seedBank = mSeedBank2;
    } else if (mApp->IsCoopMode()) {
        seedBankIndex = thePlayerIndex ? 1 : 0;
    } else {
        seedBankIndex = 0;
    }

    int boardPlayerIndex = 1;
    if (!mIsZombieChooser) {
        if (mApp->mSeedChooserScreen != nullptr) {
            boardPlayerIndex = this != mApp->mSeedChooserScreen;
        } else {
            boardPlayerIndex = 0;
        }
    }

    int seedPacketPositionX = mBoard->GetSeedPacketPositionX(theIndex, seedBankIndex, boardPlayerIndex != 0);
    x = seedBank->mX + seedPacketPositionX - mX;
    y = seedBank->mY + 8 - mY;
}

void SeedChooserScreen::GetSeedPositionInChooser(int theIndex, int &x, int &y) {
    if (!mIsZombieChooser && theIndex == SeedType::SEED_IMITATER) {
        x = mImitaterButton->mX;
        y = mImitaterButton->mY;
        return;
    }
    if (mPageIndex == 1) {
        if (mIsZombieChooser) {
            const int zombieFirstPageSeedCount = GetZombieFirstPageSeedCount(this);
            if (theIndex >= zombieFirstPageSeedCount && theIndex < GetSeedStorageCount()) {
                theIndex -= zombieFirstPageSeedCount;
            }
        } else {
            if (theIndex >= SEED_ICEBERG_LETTUCE && theIndex < NUM_SEEDS_IN_CHOOSER_EXTENDED) {
                theIndex -= SEED_ICEBERG_LETTUCE;
            }
        }
    }

    int aRow = theIndex / NumColumns();
    int aCol = theIndex % NumColumns();
    if (mIsZombieChooser && aRow == 3 && !IsExtendedSeedsModeEnabled(this)) {
        x = 53 * aCol + 48;
    } else {
        x = 53 * aCol + 22;
    }
    if (!mIsZombieChooser && Has7Rows()) {
        y = 70 * aRow + 123;
    } else {
        y = 73 * aRow + 128;
    }
}

int SeedChooserScreen::NumColumns() const {
    return mIsZombieChooser ? 5 : 8;
}


void SeedChooserScreen::ShowToolTip(int thePlayerIndex) {
    if (thePlayerIndex < 0 || thePlayerIndex > 1) {
        return;
    }

    ToolTipWidget *aToolTip = (thePlayerIndex == 1) ? mToolTip2 : mToolTip1;
    int &aToolTipSeed = (thePlayerIndex == 1) ? mToolTipSeed2 : mToolTipSeed1;

    if (aToolTip == nullptr) {
        return;
    }

    if (mChooseState == SeedChooserState::CHOOSE_VIEW_LAWN) {
        RemoveToolTip(thePlayerIndex);
        return;
    }
    if (mSeedsInFlight > 0) {
        return;
    }

    const SeedType cursorSeed = (thePlayerIndex == 1) ? SeedType(mSeedIndex2) : SeedType(mSeedIndex1);
    const int cursorX = (thePlayerIndex == 1) ? mCursorPositionX2 : mCursorPositionX1;
    const int cursorY = (thePlayerIndex == 1) ? mCursorPositionY2 : mCursorPositionY1;

    if (!mIsZombieChooser && cursorSeed == SeedType::SEED_IMITATER && mMouseVisible) {
        aToolTip->SetLabel(Plant::GetToolTip(SeedType::SEED_IMITATER));
        aToolTip->SetTitle(Plant::GetNameString(SeedType::SEED_IMITATER, SeedType::SEED_NONE));
        aToolTip->SetWarningText("");
        aToolTip->mVisible = true;
        aToolTip->mX = mImitaterButton->mX + 63;
        aToolTip->mY = mImitaterButton->mY;
        aToolTipSeed = SeedType::SEED_IMITATER;
    } else {
        const SeedType aSeedType = SeedHitTest(cursorX, cursorY);
        if (aSeedType == SEED_NONE) {
            RemoveToolTip(thePlayerIndex);
            return;
        }
        if (aSeedType != aToolTipSeed) {
            RemoveToolTip(thePlayerIndex);

            const int seedPacketIndex = GetSeedPacketIndex(aSeedType);
            const unsigned int aRecFlags = SeedNotRecommendedToPick(aSeedType);
            if (SeedNotAllowedToPick(aSeedType)) {
                aToolTip->SetWarningText("[NOT_ALLOWED_ON_THIS_LEVEL]");
            } else if (SeedNotAllowedDuringTrial(aSeedType)) {
                aToolTip->SetWarningText("[FULL_VERSION_ONLY]");
            } else if (GetChosenSeed(seedPacketIndex).mSeedState == SEED_IN_BANK && GetChosenSeed(seedPacketIndex).mCrazyDavePicked) {
                aToolTip->SetWarningText("[CRAZY_DAVE_WANTS]");
            } else if (aRecFlags != 0U) {
                if (TestBit(aRecFlags, NOT_RECOMMENDED_NOCTURNAL)) {
                    aToolTip->SetWarningText("[NOCTURNAL_WARNING]");
                } else if (TestBit(aRecFlags, NOT_RECOMMENDED_ON_POOL)) {
                    aToolTip->SetWarningText("[NOT_ALLOWED_ON_WATER]");
                } else {
                    aToolTip->SetWarningText("[NOT_RECOMMENDED_FOR_LEVEL]");
                }
            } else {
                aToolTip->SetWarningText("");
            }

            if (aSeedType == SEED_IMITATER) {
                aToolTip->SetTitle(Plant::GetNameString(SeedType::SEED_IMITATER, GetChosenSeed(seedPacketIndex).mImitaterType));
                aToolTip->SetLabel(Plant::GetToolTip(GetChosenSeed(seedPacketIndex).mImitaterType));
            } else if (!mIsZombieChooser) {
                aToolTip->SetTitle(Plant::GetNameString(aSeedType, SeedType::SEED_NONE));
                aToolTip->SetLabel(Plant::GetToolTip(aSeedType));
            } else if (aSeedType == SEED_ZOMBIE_GRAVESTONE) {
                aToolTip->SetTitle(TodStringTranslate("[ZOMBIE_GRAVESTONE]"));
                aToolTip->SetLabel(TodStringTranslate("[ZOMBIE_GRAVESTONE_DESCRIPTION]"));
            } else if (aSeedType == SEED_ZOMBIE_MOUND) {
                aToolTip->SetTitle(TodStringTranslate("[ZOMBIE_MOUND]"));
                aToolTip->SetLabel(TodStringTranslate("[ZOMBIE_MOUND_DESCRIPTION]"));
            } else {
                ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(aSeedType);
                if (aZombieType == ZombieType(-1)) {
                    return;
                }
                ZombieDefinition &aZombieDefinition = GetZombieDefinition(aZombieType);
                aToolTip->SetTitle(TodStringTranslate(StrFormat("[%s]", aZombieDefinition.mZombieName).c_str()));
                aToolTip->SetLabel(TodStringTranslate(StrFormat("[%s_DESCRIPTION_HEADER]", aZombieDefinition.mZombieName).c_str()));
            }

            int aSeedX = 0;
            int aSeedY = 0;
            GetSeedPositionInChooser(seedPacketIndex, aSeedX, aSeedY);

            int toolTipX = mX + 14;
            const int centeredX = mX + aSeedX + (SEED_PACKET_WIDTH - aToolTip->mWidth) / 2;
            if (centeredX > mX + 14) {
                toolTipX = mX + mWidth - aToolTip->mWidth - 14;
                if (toolTipX >= centeredX) {
                    toolTipX = centeredX;
                }
            }

            int toolTipY = aSeedY;
            if (seedPacketIndex > 39) {
                toolTipX = aSeedX + 53;
            } else {
                toolTipY += 70;
            }

            aToolTip->mX = toolTipX;
            aToolTip->mY = toolTipY;
            aToolTip->mMaxBottom = (toolTipY <= 529) ? 600 : 529;
            aToolTip->mVisible = true;
            aToolTipSeed = aSeedType;
        }
    }

    if (mApp->IsVSMode()) {
        int aGamepadIndex = mApp->PlayerToGamepadIndex(thePlayerIndex);
        int x = (aGamepadIndex == 1) ? mCursorPositionX2 : mCursorPositionX1;
        int y = (aGamepadIndex == 1) ? mCursorPositionY2 : mCursorPositionY1;
        const SeedType aSeedType = SeedHitTest(x, y);
        for (auto &aBannedSeed : mBannedSeed) {
            if (aSeedType == aBannedSeed.mSeedType) {
                aToolTip->SetWarningText("[BANNED_ON_THIS_TURN]");
            }
        }

        if (mIsZombieChooser) {
            int aSeedX = 0, aSeedY = 0;
            int aZombieSeedIdx = GetZombieIndexBySeedType(aSeedType);
            GetSeedPositionInChooser(aZombieSeedIdx, aSeedX, aSeedY);

            if (GetChosenSeed(aSeedType - SEED_ZOMBIE_GRAVESTONE).mSeedState == ChosenSeedState::SEED_IN_BANK && GetChosenSeed(aSeedType - SEED_ZOMBIE_GRAVESTONE).mCrazyDavePicked) {
                aToolTip->SetWarningText(aToolTipSeed == SEED_ZOMBIE_GRAVESTONE ? "[ZOMBIE_BOSS_WANTS]" : "");
            }

            switch (aSeedType) {
                case SeedType::SEED_ZOMBIE_IMP:
                case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
                    aToolTip->mX = aSeedX + 5 * (SEED_PACKET_WIDTH + 12);
                    break;
                case SeedType::SEED_ZOMBIE_POLEVAULTER:
                case SeedType::SEED_ZOMBIE_BALLOON:
                case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
                    aToolTip->mX = aSeedX + 4 * (SEED_PACKET_WIDTH + 12);
                    break;
                default:
                    break;
            }

            // 已选的卡不再展示描述文本
            // 禁用阶段不再展示已禁卡的描述文本
            if (GetChosenSeed(aZombieSeedIdx).mSeedState == ChosenSeedState::SEED_IN_BANK || (mBanningPhase && mBannedSeed[aSeedType].mSeedState == BannedSeedState::SEED_BANNED)) {
                aToolTip->mVisible = false;
            }
        } else {
            int aSeedX = 0, aSeedY = 0;
            GetSeedPositionInChooser(GetSeedPacketIndex(aSeedType), aSeedX, aSeedY);
            if (mSeedsInFlight <= 0 && mPageIndex == 1) {
                aToolTip->mX = aSeedX - (SEED_PACKET_WIDTH + 3);
                aToolTip->mY = aSeedY - 5 * (SEED_PACKET_HEIGHT + 3);
            }

            if (mBanningPhase) {
                if (GetChosenSeed(GetSeedPacketIndex(aSeedType)).mSeedState == ChosenSeedState::SEED_IN_CHOOSER) {
                    if (aToolTipSeed == SeedType::SEED_INSTANT_COFFEE || aToolTipSeed == SeedType::SEED_LILYPAD || aToolTipSeed == SeedType::SEED_FLOWERPOT) {
                        aToolTip->SetWarningText("[NOT_ALLOWED_ON_THIS_PHASE]");
                    }
                }
            }

            if (GetChosenSeed(GetSeedPacketIndex(aSeedType)).mSeedState == ChosenSeedState::SEED_IN_BANK || (mBanningPhase && mBannedSeed[aSeedType].mSeedState == BannedSeedState::SEED_BANNED)) {
                aToolTip->mVisible = false;
            }
        }
    }
}

void SeedChooserScreen::MouseMove(int x, int y) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (mApp->IsVSMode() && !IsLocalChooserInputAllowed(this)) {
        return;
    }
    if (mApp->GetDialogCount() != 0 || mImitaterDialog != nullptr) {
        return; // 存在模仿者选框、不建议种子选框等时
    }
    NormalizeLocalPoint(this, x, y);
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) {
        return;
    }

    SeedType aSeedType = SeedHitTest(x, y);
    // 该函数探测不到模仿者位置
    if (aSeedType == SeedType::SEED_NONE) {
        return;
    }

    if (mIsZombieChooser) {
        if ((mPageIndex == 0 && aSeedType > GetZombieFirstPageLastSeedType(this)) || (mPageIndex == 1 && aSeedType >= SeedType::NUM_ZOMBIE_SEEDS_IN_CHOOSER)) {
            return;
        }
        if (mPageIndex == 1) {
            aSeedType = SeedType(aSeedType - GetZombieFirstPageSeedCount(this));
        }

        int aZombieSeedIdx = GetZombieIndexBySeedType(aSeedType);
        GetSeedPositionInChooser(aZombieSeedIdx, mCursorPositionX1, mCursorPositionY1);
        GetSeedPositionInChooser(aZombieSeedIdx, mCursorPositionX2, mCursorPositionY2);
        mSeedIndex1 = mSeedIndex2 = aZombieSeedIdx;
    } else if (m1PChoosingSeeds) {
        if (mApp->IsVSMode() && aSeedType > SeedType::SEED_MELONPULT)
            return;

        if (mApp->IsVSMode()) {
            GetSeedPositionInChooser(aSeedType, mCursorPositionX1, mCursorPositionY1);
            GetSeedPositionInChooser(aSeedType, mCursorPositionX2, mCursorPositionY2);
        } else {
            GetSeedPositionInChooser(aSeedType, mCursorPositionX1, mCursorPositionY1);
        }
        mSeedIndex1 = aSeedType;
    } else {
        GetSeedPositionInChooser(aSeedType, mCursorPositionX2, mCursorPositionY2);
        mSeedIndex2 = aSeedType;
    }
}

void SeedChooserScreen::MouseDown(int x, int y, int theClickCount) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (mApp->GetDialogCount() != 0 || mImitaterDialog != nullptr) {
        return; // 存在模仿者选框、不建议种子选框等时
    }
    NormalizeLocalPoint(this, x, y);
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) {
        return;
    }

    if (mApp->IsVSMode() && !IsLocalChooserInputAllowed(this)) {
        gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_NONE;
        gSeedChooserTouchOwner = this;
        return;
    }
    if (gSeedChooserTouchOwner != nullptr && gSeedChooserTouchOwner != this) {
        return;
    }
    if (gSeedChooserTouchOwner == this) {
        gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_NONE;
    }

    m1PChoosingSeeds = !mApp->IsCoopMode() || mSeedsIn1PBank < 4;

    bool mViewLawnButtonDisabled = mViewLawnButton == nullptr || !mBoard->mCutScene->IsSurvivalRepick();
    bool mStoreButtonDisabled = mStoreButton == nullptr || mStoreButton->mBtnNoDraw || mStoreButton->mDisabled;
    bool mStartButtonDisabled = mStartButton == nullptr || mStartButton->mBtnNoDraw || mStartButton->mDisabled;
    bool mAlmanacButtonDisabled = mAlmanacButton == nullptr || mAlmanacButton->mBtnNoDraw || mAlmanacButton->mDisabled;

    if (!mViewLawnButtonDisabled) { // !mDisabled
        Sexy::Rect mViewLawnButtonRect = {mViewLawnButton->mX, mViewLawnButton->mY, mViewLawnButton->mWidth, 50};
        // LOGD("mStoreButtonRect:%d %d %d %d",mStoreButtonRect[0],mStoreButtonRect[1],mStoreButtonRect[2],mStoreButtonRect[3]);
        if (mViewLawnButtonRect.Contains(x, y)) {
            mApp->PlaySample(Sexy::SOUND_TAP);
            gSeedChooserTouchState = SeedChooserTouchState::VIEW_LAWN_BUTTON;
            gSeedChooserTouchOwner = this;
            // GameButtonDown(seedChooserScreen, 8, 0, 0);
            return;
        }
    }

    if (!mStoreButtonDisabled) { // !mDisabled
        Sexy::Rect mStoreButtonRect = {mStoreButton->mX, mStoreButton->mY, mStoreButton->mWidth, 50};
        // LOGD("mStoreButtonRect:%d %d %d %d",mStoreButtonRect[0],mStoreButtonRect[1],mStoreButtonRect[2],mStoreButtonRect[3]);
        if (mStoreButtonRect.Contains(x, y)) {
            mApp->PlaySample(Sexy::SOUND_TAP);
            gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_STORE_BUTTON;
            gSeedChooserTouchOwner = this;
            // GameButtonDown(seedChooserScreen, 8, 0, 0);
            return;
        }
    }

    if (!mStartButtonDisabled) { // !mDisabled
        Sexy::Rect mStartButtonRect = {mStartButton->mX, mStartButton->mY, mStartButton->mWidth, 50};
        if (mStartButtonRect.Contains(x, y)) {
            mApp->PlaySample(Sexy::SOUND_TAP);
            gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_START_BUTTON;
            gSeedChooserTouchOwner = this;

            // SeedChooserScreen_OnStartButton(seedChooserScreen);
            return;
        }
    }

    if (!mAlmanacButtonDisabled) { // !mDisabled
        Sexy::Rect mAlmanacButtonRect = {mAlmanacButton->mX, mAlmanacButton->mY, mAlmanacButton->mWidth, 50};
        if (mAlmanacButtonRect.Contains(x, y)) {
            mApp->PlaySample(Sexy::SOUND_TAP);
            gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_ALMANAC_BUTTON;
            gSeedChooserTouchOwner = this;

            // GameButtonDown(seedChooserScreen, 9, 0, 0);
            return;
        }
    }

    if (HasPacket(SeedType::SEED_IMITATER, false) && !mApp->IsVSMode() && !mIsZombieChooser) {
        int mImitaterPositionX = 0;
        int mImitaterPositionY = 0;
        GetSeedPositionInChooser(SeedType::SEED_IMITATER, mImitaterPositionX, mImitaterPositionY);
        Sexy::Rect mImitaterPositionRect = {mImitaterPositionX, mImitaterPositionY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT};
        if (mImitaterPositionRect.Contains(x, y)) {
            if (m1PChoosingSeeds) {
                mCursorPositionX1 = mImitaterPositionX;
                mCursorPositionY1 = mImitaterPositionY;
                mSeedIndex1 = SeedType::SEED_IMITATER;
            } else {
                mCursorPositionX2 = mImitaterPositionX;
                mCursorPositionY2 = mImitaterPositionY;
                mSeedIndex2 = SeedType::SEED_IMITATER;
            }
            GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, !m1PChoosingSeeds, 0);
            return;
        }
    }
    SeedType aSeedType = SeedHitTest(x, y);
    // 该函数探测不到模仿者位置

    if (aSeedType == SeedType::SEED_NONE) {
        return;
    }

    if (!mIsZombieChooser
        && (GetChosenSeed(GetSeedPacketIndex(aSeedType)).mSeedState == ChosenSeedState::SEED_FLYING_TO_BANK
            || GetChosenSeed(GetSeedPacketIndex(aSeedType)).mSeedState == ChosenSeedState::SEED_FLYING_TO_CHOOSER)) {
        return;
    }

    if (mIsZombieChooser) {
        if ((mPageIndex == 0 && aSeedType > GetZombieFirstPageLastSeedType(this)) || (mPageIndex == 1 && aSeedType >= SeedType::NUM_ZOMBIE_SEEDS_IN_CHOOSER)) {
            return;
        }
        if (mPageIndex == 1) {
            aSeedType = SeedType(aSeedType - GetZombieFirstPageSeedCount(this));
        }

        int aZombieSeedIdx = GetZombieIndexBySeedType(aSeedType);
        GetSeedPositionInChooser(aZombieSeedIdx, mCursorPositionX1, mCursorPositionY1);
        GetSeedPositionInChooser(aZombieSeedIdx, mCursorPositionX2, mCursorPositionY2);
        mSeedIndex1 = mSeedIndex2 = aZombieSeedIdx;
    } else if (m1PChoosingSeeds) {
        if (mApp->IsVSMode()) {
            if ((mPageIndex == 0 && aSeedType > SeedType::SEED_MELONPULT) || (mPageIndex == 1 && aSeedType < SeedType::SEED_ICEBERG_LETTUCE && aSeedType >= SeedType::NUM_SEEDS_IN_CHOOSER_EXTENDED)) {
                return;
            }
            if (mPageIndex == 1) {
                aSeedType = SeedType(aSeedType - 200);
            }
        }

        if (mApp->IsVSMode()) {
            GetSeedPositionInChooser(aSeedType, mCursorPositionX1, mCursorPositionY1);
            GetSeedPositionInChooser(aSeedType, mCursorPositionX2, mCursorPositionY2);
        } else {
            GetSeedPositionInChooser(aSeedType, mCursorPositionX1, mCursorPositionY1);
        }
        mSeedIndex1 = aSeedType;
    } else {
        GetSeedPositionInChooser(aSeedType, mCursorPositionX2, mCursorPositionY2);
        mSeedIndex2 = aSeedType;
    }
    gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_SEED_CHOOSER;
    gSeedChooserTouchOwner = this;
}

void SeedChooserScreen::MouseDrag(int x, int y) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (mApp->IsVSMode() && !IsLocalChooserInputAllowed(this)) {
        return;
    }
    if (mApp->GetDialogCount() != 0 || mImitaterDialog != nullptr) {
        return; // 存在模仿者选框、不建议种子选框等时
    }
    NormalizeLocalPoint(this, x, y);
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) {
        return;
    }
    if (gSeedChooserTouchOwner != this) {
        return;
    }

    if (gSeedChooserTouchState == SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_SEED_CHOOSER) {
        SeedType aSeedType = SeedHitTest(x, y);
        // 该函数探测不到模仿者位置
        if (aSeedType == SeedType::SEED_NONE) {
            return;
        }
        if (mIsZombieChooser) {
            if ((mPageIndex == 0 && aSeedType > GetZombieFirstPageLastSeedType(this)) || (mPageIndex == 1 && aSeedType >= SeedType::NUM_ZOMBIE_SEEDS_IN_CHOOSER)) {
                return;
            }
            if (mPageIndex == 1) {
                aSeedType = SeedType(aSeedType - GetZombieFirstPageSeedCount(this));
            }

            int aZombieSeedIdx = GetZombieIndexBySeedType(aSeedType);
            GetSeedPositionInChooser(aZombieSeedIdx, mCursorPositionX1, mCursorPositionY1);
            GetSeedPositionInChooser(aZombieSeedIdx, mCursorPositionX2, mCursorPositionY2);
            mSeedIndex1 = mSeedIndex2 = aZombieSeedIdx;
        } else if (m1PChoosingSeeds) {
            if (mApp->IsVSMode()) {
                if ((mPageIndex == 0 && aSeedType > SeedType::SEED_MELONPULT)
                    || (mPageIndex == 1 && aSeedType < SeedType::SEED_ICEBERG_LETTUCE && aSeedType >= SeedType::NUM_SEEDS_IN_CHOOSER_EXTENDED)) {
                    return;
                }
                if (mPageIndex == 1) {
                    aSeedType = SeedType(aSeedType - 200);
                }
            }

            if (mApp->IsVSMode()) {
                GetSeedPositionInChooser(aSeedType, mCursorPositionX1, mCursorPositionY1);
                GetSeedPositionInChooser(aSeedType, mCursorPositionX2, mCursorPositionY2);
            } else {
                GetSeedPositionInChooser(aSeedType, mCursorPositionX1, mCursorPositionY1);
            }
            mSeedIndex1 = aSeedType;
        } else {
            GetSeedPositionInChooser(aSeedType, mCursorPositionX2, mCursorPositionY2);
            mSeedIndex2 = aSeedType;
        }
    }

    if (mApp->IsVSMode() && IsLocalChooserInputAllowed(this)) {
        int ownerPlayerIndex = mPlayerIndex;
        if (ownerPlayerIndex < 0 || ownerPlayerIndex > 1) {
            ownerPlayerIndex = 0;
        }
        int chooserIndex = mIsZombieChooser ? 1 : 0;
        int cursorX = (ownerPlayerIndex == 0) ? mCursorPositionX1 : mCursorPositionX2;
        int cursorY = (ownerPlayerIndex == 0) ? mCursorPositionY1 : mCursorPositionY2;
        SeedType hoverSeedType = SeedHitTest(cursorX, cursorY);
        if (hoverSeedType != SeedType::SEED_NONE) {
            uint32_t nowMs = Sexy::GetTickCount();
            bool sameSeed = (gLastDragSyncSeedType[chooserIndex][ownerPlayerIndex] == hoverSeedType);
            uint32_t elapsedMs = nowMs - gLastDragSyncTickMs[chooserIndex][ownerPlayerIndex];
            if (sameSeed && elapsedMs < kSeedChooserDragSyncIntervalMs) {
                return;
            }

            // data3 flags: bit0 = moveOnly(sync cursor without picking), bit1 = sender page index for zombie chooser
            const uint8_t cursorFlags = kCursorMoveOnlyEventFlag | ((mPageIndex == 1) ? kCursorPageOneEventFlag : 0);
            if (gTcpConnected) {
                U8x3_Event event = {{EventType::EVENT_CLIENT_SEEDCHOOSER_SELECT_SEED}, {uint8_t(hoverSeedType), uint8_t(mIsZombieChooser), cursorFlags}};
                netplay::PutEvent(event);
            } else if (gTcpClientSocket >= 0) {
                U8x3_Event event = {{EventType::EVENT_SERVER_SEEDCHOOSER_SELECT_SEED}, {uint8_t(hoverSeedType), uint8_t(mIsZombieChooser), cursorFlags}};
                netplay::PutEvent(event);
            }
            gLastDragSyncSeedType[chooserIndex][ownerPlayerIndex] = hoverSeedType;
            gLastDragSyncTickMs[chooserIndex][ownerPlayerIndex] = nowMs;
        }
    }
}

void SeedChooserScreen::MouseUp(int x, int y) {
    if (mApp->GetDialogCount() != 0 || mImitaterDialog != nullptr) {
        return; // 存在模仿者选框、不建议种子选框等时
    }
    if (gIsServerModeSpectator || gIsReplayMode) {
        if (gSeedChooserTouchOwner == this) {
            gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_NONE;
            gSeedChooserTouchOwner = nullptr;
        }
        return;
    }
    if (mApp->IsVSMode() && !IsLocalChooserInputAllowed(this)) {
        if (gSeedChooserTouchOwner == this) {
            gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_NONE;
            gSeedChooserTouchOwner = nullptr;
        }
        return;
    }
    if (gSeedChooserTouchOwner != this) {
        return;
    }

    switch (gSeedChooserTouchState) {
        case SeedChooserTouchState::VIEW_LAWN_BUTTON:
            ButtonDepress(SeedChooserScreen_ViewLawn);
            break;
        case SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_SEED_CHOOSER:
            if (mApp->IsVSMode()) {
                GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, mPlayerIndex, 0);
            } else if (!mIsZombieChooser && m1PChoosingSeeds && mApp->IsCoopMode()) {
                GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 0, 0);
            } else {
                GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 1, 0);
            }
            break;
        case SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_STORE_BUTTON:
            ButtonDepress(SeedChooserScreen_Store);
            break;
        case SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_START_BUTTON:
            ButtonDepress(SeedChooserScreen_Start);
            break;
        case SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_ALMANAC_BUTTON:
            ButtonDepress(SeedChooserScreen_Almanac);
            break;
        default:
            break;
    }
    gSeedChooserTouchState = SeedChooserTouchState::SEEDCHOOSER_TOUCHSTATE_NONE;
    gSeedChooserTouchOwner = nullptr;
}

int SeedChooserScreen::GetNextSeedInDir(int theNumSeed, SeedDir theMoveDirection) {
    if (mIsZombieChooser) {
        if (!mShowExtendedSeeds) {
            // 右下角边缘
            if ((theNumSeed == 14 && theMoveDirection == SeedDir::SEED_DIR_DOWN) || //
                (theNumSeed == 18 && theMoveDirection == SeedDir::SEED_DIR_RIGHT)) {
                return 18;
            }
        } else {
            if ((theNumSeed == 4 && theMoveDirection == SeedDir::SEED_DIR_DOWN) || //
                (theNumSeed == 5 && theMoveDirection == SeedDir::SEED_DIR_RIGHT)) {
                return NUM_ZOMBIE_SEEDS_IN_CHOOSER - SEED_ZOMBIE_BALLOON - 2;
            }
        }
    }

    const int aNumCol = NumColumns();
    int aRow = 0;
    int aCol = 0;
    if (theNumSeed == SeedType::SEED_IMITATER) {
        aCol = 8;
        aRow = 5;
    } else {
        aRow = theNumSeed / aNumCol;
        aCol = theNumSeed % aNumCol;
    }

    switch (theMoveDirection) {
        case SeedDir::SEED_DIR_UP:
            if (aRow > 0) {
                --aRow;
            }
            break;
        case SeedDir::SEED_DIR_DOWN: {
            int aMaxRow = mIsZombieChooser ? (mShowExtendedSeeds ? 4 : 3) // 拓展僵尸选卡适配键盘选取
                                           : (Has7Rows() ? 5 : 4);
            if (mPageIndex == 1) {
                aMaxRow = 1;
            }
            if (aRow < aMaxRow) {
                ++aRow;
            }
        } break;
        case SeedDir::SEED_DIR_LEFT:
            if (aCol > 0) {
                --aCol;
            }
            break;
        case SeedDir::SEED_DIR_RIGHT:
            if (aCol < aNumCol - 1) {
                ++aCol;
            }
            break;
        default:
            break;
    }
    int aNextSeed = aCol + aNumCol * aRow;
    return aNextSeed;
}

void SeedChooserScreen::Draw(Graphics *g) { // Early returns for dialogsif (mApp->GetDialog(DIALOG_STORE) || mApp->GetDialog(DIALOG_ALMANAC))return;

    g->SetLinearBlend(true);

    if (!mBoard->ChooseSeedsOnCurrentLevel() || (mBoard->mCutScene && mBoard->mCutScene->IsBeforePreloading()))
        return;

    // Setup base color
    Color aBaseColor(255, 255, 255);

    // Handle two-player mode dimming
    if (mApp->IsVSMode() && !CanPickNow()) {
        float aDimAmount = TodAnimateCurveFloat(0, 25, mDimCounter, 1.0f, 0.45f, CURVE_EASE_IN_OUT);
        g->SetColorizeImages(true);
        aBaseColor = Color((int)(aDimAmount * 255.0f), (int)(aDimAmount * 255.0f), (int)(aDimAmount * 255.0f));
        g->SetColor(aBaseColor);
    }

    // Draw background
    Image *aBackgroundImage = mIsZombieChooser ? Sexy::IMAGE_SEEDCHOOSER_BACKGROUND2 : Sexy::IMAGE_SEEDCHOOSER_BACKGROUND;
    g->DrawImage(aBackgroundImage, 0, 87);

    // Draw imitater addon for plant chooser
    if (!mIsZombieChooser && HasPacket(SEED_IMITATER, false) && !mApp->IsVSMode()) {
        g->DrawImage(Sexy::IMAGE_SEEDCHOOSER_IMITATERADDON, mImitaterButton->mX - 5, mImitaterButton->mY - 12);
    }

    // Draw title text
    Color aTitleColor;
    const char *aTitleText = nullptr;
    if (mIsZombieChooser) {
        aTitleColor = Color(0, 255, 0);
        aTitleText = "[CHOOSE_YOUR_ZOMBIES]";
    } else {
        aTitleColor = Color(213, 159, 43);
        aTitleText = "[CHOOSE_YOUR_PLANTS]";
    }

    TodDrawString(g, aTitleText, aBackgroundImage->mWidth / 2, 114, Sexy::FONT_DWARVENTODCRAFT18, aTitleColor, DS_ALIGN_CENTER);

    // Calculate seed count
    int aNumSeeds = 19;
    if (!mIsZombieChooser) {
        if (mApp->IsVSMode()) {
            aNumSeeds = GetCurrentPageSeedCount();
        } else if (!Has7Rows()) {
            aNumSeeds = 40;
        } else if (HasPacket(SEED_IMITATER, false)) {
            aNumSeeds = 49;
        } else {
            aNumSeeds = 48;
        }
    } else {
        if (mShowExtendedSeeds) {
            if (mPageIndex == 0) {
                aNumSeeds = GetZombieFirstPageSeedCount(this);
            } else if (mPageIndex == 1) {
                aNumSeeds = NUM_ZOMBIE_SEEDS_IN_CHOOSER - SEED_ZOMBIE_GRAVESTONE;
            }
        } else {
            aNumSeeds = GetCurrentPageSeedCount();
        }
    }

    // Draw seed packet shadows (two passes)
    for (int aPass = 0; aPass < 2; aPass++) {
        bool aDrawShadow = (aPass == 0);

        for (SeedType aSeedShadow = SEED_PEASHOOTER; aSeedShadow < aNumSeeds; aSeedShadow = SeedType(aSeedShadow + 1)) {
            const int storageIndex = GetPageSeedStorageIndex(aSeedShadow);

            if (storageIndex < 0 || storageIndex >= GetSeedStorageCount()) {
                continue;
            }

            SeedType aDisplaySeedType = mIsZombieChooser ? GetZombieSeedType(storageIndex) : GetPlantSeedType(storageIndex);

            if (aDisplaySeedType == SEED_IMITATER)
                continue;

            int x = 0, y = 0;
            GetSeedPositionInChooser(aSeedShadow, x, y);
            if (aDisplaySeedType == SEED_NONE || !HasPacket(aDisplaySeedType, mIsZombieChooser)) {
                if (aDrawShadow)
                    g->DrawImage(Sexy::IMAGE_SEEDPACKETSILHOUETTE, x, y);
            } else {
                ChosenSeed &aChosenSeed = GetChosenSeed(storageIndex);
                if (aChosenSeed.mSeedState != SEED_IN_CHOOSER) {
                    // Determine grayness based on selection state
                    int aGrayness = 55;
                    //                    if (mSeedIndex1 == aSeedShadow || mSeedIndex2 == aSeedShadow)
                    //                        aGrayness = 55;
                    //                    else
                    //                        aGrayness = 255;

                    DrawPacket(g, x, y, aDisplaySeedType, SEED_NONE, 0.0f, aGrayness, &aBaseColor, true, true);
                }
            }
        }
    }

    // Draw empty seed bank slots
    int aNumSeedsInBank = mSeedBank1->mNumPackets;
    for (int anIndex = 0; anIndex < aNumSeedsInBank; anIndex++) {
        if (FindSeedInBank(anIndex, false) == SEED_NONE) {
            int x = 0, y = 0;
            GetSeedPositionInBank(anIndex, x, y, 0);
            // 修复植物方选卡时僵尸方翻页后空卡槽绘制变暗
            if (mApp->IsVSMode()) {
                g->SetColorizeImages(true);
                g->SetColor(Color(255, 255, 255));
            }
            g->DrawImage(Sexy::IMAGE_SEEDPACKETSILHOUETTE, x, y);
        }
    }

    // Draw coop mode second bank slots
    if (mApp->IsCoopMode() && mSeedBank2) {
        for (int anIndex = 0; anIndex < aNumSeedsInBank; anIndex++) {
            if (FindSeedInBank(anIndex, true) == SEED_NONE) {
                int x = 0, y = 0;
                GetSeedPositionInBank(anIndex, x, y, 1);
                g->DrawImage(Sexy::IMAGE_SEEDPACKETSILHOUETTE, x, y);
            }
        }
    }

    // Draw seeds in chooser and bank
    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        SeedType aDisplaySeedType = mIsZombieChooser ? GetZombieSeedType(seedIndex) : GetPlantSeedType(seedIndex);

        if (aDisplaySeedType == SEED_NONE || !HasPacket(aDisplaySeedType, mIsZombieChooser))
            continue;

        ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        ChosenSeedState aSeedState = aChosenSeed.mSeedState;

        if (aSeedState == SEED_FLYING_TO_BANK || aSeedState == SEED_FLYING_TO_CHOOSER || aSeedState == SEED_PACKET_HIDDEN)
            continue;

        if (aSeedState != SEED_IN_CHOOSER && !mBoard->mCutScene->mSeedChoosing)
            continue;

        // Calculate position
        int aPosX = aChosenSeed.mX;
        int aPosY = aChosenSeed.mY;

        if (aSeedState == SEED_IN_BANK) {
            GetSeedPositionInBank(aChosenSeed.mSeedIndexInBank, aPosX, aPosY, aChosenSeed.mChosenPlayerIndex);
            aChosenSeed.mX = aPosX;
            aChosenSeed.mY = aPosY;
        } else {
            if (mIsZombieChooser) {
                if (mPageIndex == 0 && aDisplaySeedType > GetZombieFirstPageLastSeedType(this)) {
                    continue;
                }
                if (mPageIndex == 1) {
                    if (aDisplaySeedType <= GetZombieFirstPageLastSeedType(this)) {
                        continue;
                    }
                }
            } else {
                if (mPageIndex == 0 && aDisplaySeedType >= aNumSeeds) {
                    continue;
                }
                if (mPageIndex == 1) {
                    if (aDisplaySeedType < SeedType::SEED_ICEBERG_LETTUCE || aDisplaySeedType >= SeedType::NUM_SEEDS_IN_CHOOSER_EXTENDED) {
                        continue;
                    }
                }
            }
        }

        // Determine grayness
        bool aGrayed = false;

        if (!mIsZombieChooser) {
            if (aSeedState == SEED_IN_CHOOSER) {
                if (SeedNotRecommendedToPick(aChosenSeed.mSeedType) || SeedNotAllowedToPick(aChosenSeed.mSeedType)) {
                    aGrayed = true;
                }
            }

            if (SeedNotAllowedDuringTrial(aChosenSeed.mSeedType))
                aGrayed = true;
        }

        // Check if being dragged
        if (mSeedIndex1 == seedIndex && mBoard->mGamepadControls[0]->mPlayerIndex != -1 && aSeedState == SEED_IN_CHOOSER) {
            mSeedIndex1 = seedIndex;
        }

        if (mSeedIndex2 == seedIndex && mBoard->mGamepadControls[1]->mGamepadIndex != -1 && aSeedState == SEED_IN_CHOOSER) {
            mSeedIndex2 = seedIndex;
        }

        DrawPacket(g, aPosX, aPosY, aChosenSeed.mSeedType, aChosenSeed.mImitaterType, 0.0f, aGrayed ? 115 : 255, &aBaseColor, true, true);
    }

    // Draw imitater button
    if (!mIsZombieChooser && !mApp->IsVSMode()) {
        g->Translate(mImitaterButton->mX, mImitaterButton->mY);
        mImitaterButton->Draw(g);
        g->Translate(-mImitaterButton->mX, -mImitaterButton->mY);
    }

    // Draw cursor selectors for two players
    for (int aPlayerIndex = 0; aPlayerIndex < 2; aPlayerIndex++) {
        int aPlayerState = (aPlayerIndex ? mBoard->mGamepadControls[1] : mBoard->mGamepadControls[0])->mGamepadIndex;
        if (aPlayerState != -1 && !mButtonSlotState) {
            if (aPlayerState == mPlayerIndex || !mApp->IsVSMode()) {
                int aCursorX = aPlayerIndex ? mCursorPositionX2 : mCursorPositionX1;
                int aCursorY = aPlayerIndex ? mCursorPositionY2 : mCursorPositionY1;
                Image *aSelectorImage = (aPlayerState == mApp->mSecondPlayerGamepadIndex) ? Sexy::IMAGE_SEED_SELECTOR_BLUE : Sexy::IMAGE_SEED_SELECTOR;
                if (mBanningPhase) {
                    aSelectorImage = (aSelectorImage == Sexy::IMAGE_SEED_SELECTOR_BLUE) ? Sexy::IMAGE_SEED_SELECTOR : Sexy::IMAGE_SEED_SELECTOR_BLUE;
                }
                g->DrawImage(aSelectorImage, aCursorX - 8, aCursorY - 4, 64, 85);
            }
        }
    }

    auto DrawDraggingSeed = [&](int thePageSeedIndex, int thePlayerIndex) {
        if (thePageSeedIndex == SEED_NONE || !ShouldDisplayCursor(thePlayerIndex)) {
            return;
        }
        int x = 0;
        int y = 0;
        GetSeedPositionInChooser(thePageSeedIndex, x, y);
        const int aPageOffset = (mPageIndex == 1) ? (mIsZombieChooser ? GetZombieFirstPageSeedCount(this) : 49) : 0;
        const int aChosenSeedIndex = thePageSeedIndex + aPageOffset;
        ChosenSeed &aChosenSeed = GetChosenSeed(aChosenSeedIndex);
        const SeedType aDisplaySeedType = mIsZombieChooser ? GetZombieSeedType(aChosenSeedIndex) : GetPlantSeedType(aChosenSeedIndex);
        int aGrayness = 255;
        // 模仿者在未选中时状态为SEED_PACKET_HIDDEN，而不是SEED_IN_CHOOSER
        if (aChosenSeed.mSeedState != (aChosenSeed.mSeedType == SEED_IMITATER ? SEED_PACKET_HIDDEN : SEED_IN_CHOOSER)) {
            aGrayness = 55;
        }
        const SeedType aSeedType = aChosenSeed.mSeedType;
        if ((!mIsZombieChooser && (SeedNotRecommendedToPick(aSeedType) || SeedNotAllowedToPick(aSeedType)) && aChosenSeed.mSeedState == SEED_IN_CHOOSER) || SeedNotAllowedDuringTrial(aSeedType)) {
            aGrayness = 115;
        }
        DrawPacket(g, x, y + 5, aDisplaySeedType, SEED_NONE, 0.0f, aGrayness, &aBaseColor, true, true);
    };

    DrawDraggingSeed(mSeedIndex1, 0);
    DrawDraggingSeed(mSeedIndex2, 1);

    // 绘制对战禁用叉叉
    DrawBanIcon(g);

    // Draw cursor arrows for players
    for (int aPlayerIndex = 0; aPlayerIndex < 2; aPlayerIndex++) {
        if (ShouldDisplayCursor(aPlayerIndex) && (aPlayerIndex ? mBoard->mGamepadControls[1] : mBoard->mGamepadControls[0])->mGamepadIndex != -1) {
            int aCursorX = aPlayerIndex ? mCursorPositionX2 : mCursorPositionX1;
            int aCursorY = aPlayerIndex ? mCursorPositionY2 : mCursorPositionY1;
            Image *aArrowImage = aPlayerIndex ? Sexy::IMAGE_CURSOR_ARROW_P2 : Sexy::IMAGE_CURSOR_ARROW_P1;
            Image *aTextImage = aPlayerIndex ? Sexy::IMAGE_CURSOR_P2_TEXT : Sexy::IMAGE_CURSOR_P1_TEXT;

            if (mApp->IsVSMode() && mBanningPhase) {
                aArrowImage = aPlayerIndex ? Sexy::IMAGE_CURSOR_ARROW_P1 : Sexy::IMAGE_CURSOR_ARROW_P2;
                aTextImage = aPlayerIndex ? Sexy::IMAGE_CURSOR_P1_TEXT : Sexy::IMAGE_CURSOR_P2_TEXT;
            }

            float aBounce = sinf(mCursorBobPhase * 5.0f) * 2.0f;

            // 联机光标上绘制双方玩家昵称
            char *firstPlayerName = mBoard->mApp->mPlayerInfo->mName;
            if (gTcpConnected || gTcpClientSocket >= 0 || gIsReplayMode) {
                const bool localIsClient = gTcpConnected;
                const bool hasServerHostName = (gServerHostName[0] != '\0');
                const bool hasSecondPlayerName = (gSecondPlayerName[0] != '\0');
                const bool hasReplayHostName = (gReplayHostName[0] != '\0');
                const bool hasReplayGuestName = (gReplayGuestName[0] != '\0');

                const char *hostName = nullptr;
                const char *guestName = nullptr;
                if (gIsReplayMode) {
                    hostName = hasReplayHostName ? gReplayHostName : (hasServerHostName ? gServerHostName : "Host");
                    guestName = hasReplayGuestName ? gReplayGuestName : (hasSecondPlayerName ? gSecondPlayerName : "Guest");
                } else if (hasServerHostName || gIsServerModeSpectator) {
                    hostName = hasServerHostName ? gServerHostName : "Host";
                    guestName = hasSecondPlayerName ? gSecondPlayerName : "Guest";
                } else if (hasSecondPlayerName) {
                    hostName = localIsClient ? gSecondPlayerName : firstPlayerName;
                    guestName = localIsClient ? firstPlayerName : gSecondPlayerName;
                }

                if (hostName != nullptr && guestName != nullptr) {
                    bool isHostSide = (aPlayerIndex == 0);
                    if (mBanningPhase) {
                        isHostSide = !isHostSide;
                    }
                    const char *name = isHostSide ? hostName : guestName;
                    Color color = isHostSide ? Color(255, 242, 14, 255) : Color(68, 207, 255, 255);
                    g->DrawImageF(aArrowImage, float(aCursorX + 25 - aArrowImage->mWidth / 2), float(aCursorY - 8) + aBounce);
                    TodDrawString(g, name, aCursorX + 25 - aArrowImage->mWidth / 2, aCursorY - 10, Sexy::FONT_DWARVENTODCRAFT18, color, DrawStringJustification::DS_ALIGN_CENTER);
                }
            } else {
                g->DrawImageF(aArrowImage, float(aCursorX + 25 - aArrowImage->mWidth / 2), float(aCursorY - 8) + aBounce);
                g->DrawImageF(aTextImage, float(aCursorX + 25 - aTextImage->mWidth / 2), float(aCursorY - 32));
            }
        }
    }

    // Draw flying seed packets
    for (int seedIndex = 0; seedIndex < GetSeedStorageCount(); ++seedIndex) {
        SeedType aDisplaySeedType = mIsZombieChooser ? GetZombieSeedType(seedIndex) : GetPlantSeedType(seedIndex);

        if (!HasPacket(aDisplaySeedType, mIsZombieChooser))
            continue;

        ChosenSeed &aChosenSeed = GetChosenSeed(seedIndex);
        ChosenSeedState aSeedState = aChosenSeed.mSeedState;

        if (aSeedState == SEED_FLYING_TO_BANK || aSeedState == SEED_FLYING_TO_CHOOSER) {
            DrawPacket(g, aChosenSeed.mX, aChosenSeed.mY, aChosenSeed.mSeedType, aChosenSeed.mImitaterType, 0.0f, 255, &aBaseColor, true, true);
        }
    }

    // Draw UI widgets
    if (!mApp->HasGamepad() && (!mApp->mGamePad1IsOn || !mApp->mGamePad2IsOn)) {
        // Draw button widgets
        for (size_t i = 0; i < 4; i++) {
            GameButton *aButton[4] = {mViewLawnButton, mStoreButton, mStartButton, mAlmanacButton};
            if (aButton[i] && aButton[i]->mVisible) {
                g->Translate(aButton[i]->mX, aButton[i]->mY);
                aButton[i]->Draw(g);
                g->Translate(-aButton[i]->mX, -aButton[i]->mY);
            }
        }
    } else if (mShowHelpText && !mApp->IsVSMode()) {
        // Draw help text with flashing effect
        int aFlashPhase = mSeedChooserAge % 100;
        int aTextX = aBackgroundImage->mWidth / 2;
        int aTextY = aBackgroundImage->mHeight - 63;

        Color aTextColor;
        if (aFlashPhase <= 50)
            aTextColor = Color(127, 127, 127, 255);
        else
            aTextColor = Color::White;

        TodDrawString(g, "[HELP_TEXT_2_START]", aTextX, aTextY, Sexy::FONT_DWARVENTODCRAFT24, aTextColor, DS_ALIGN_CENTER);
    }
    //    else {
    //        // Check for disconnected controller warning
    //        int aTwoPlayerState = mApp->mSecondPlayerGamepadIndex;
    //        if (aTwoPlayerState != -1 && aTwoPlayerState == mPlayerIndex) {
    //            // if (mBoard->mGamepadControls[aTwoPlayerState] &&
    //            //     !mBoard->mGamepadControls[aTwoPlayerState]->mControllerConnected)
    //            // { // 这是AI给出的结果，很显然还原是对的，但是实际上没有这个成员 | 故此还原TV伪C的判断
    //
    //            if (!*(bool *)(mApp->unkMem6[aTwoPlayerState + 135] + 412)) {
    //
    //                // Warninig警告: 不得简化sDisconnectTimer这个变量，更不能删除static字样！
    //                static int sDisconnectTimer = 0;
    //                sDisconnectTimer++;
    //                /* 这一段看的我很迷糊，首先是mGamepadControls的判断，TV的伪C是判断!mApp->Unk6[aTwoPlayerState + 139], PSV又是调用函数判断成立
    //                        经过我的分析，PSV调用的是一个判断控制器的状态的函数，有以下返回值: 0已连接，1167控制器未连接，TV很有可能函数已经被阉割了，不过我有空看看1.0.1的ida
    //
    //                   还有sDisconnectTimer这个变量，在PSV与TV中都是全局变量，在进行这一步时会进行X++。
    //                        在这里我就不声明为全局变量了，声明为一个函数内静态变量(相当于全局变量但是只有此作用域可使用)*/
    //
    //                int aSeconds = (sDisconnectTimer / 60) % 60;
    //                if (aSeconds > 30) {
    //                    pvzstl::string aWarningText = TodStringTranslate("[RECONNECT_SECOND_CONTROLLER_FMT]");
    //                    aWarningText = StrFormat(aWarningText.c_str(), aTwoPlayerState + 1); // 此处的StrFormat在TV中传入2，PSV则是mApp->mSecondPlayerGamepadIndex
    //
    //                    int aTextX = aBackgroundImage->mWidth / 2;
    //                    int aTextY = aBackgroundImage->mHeight - 63;
    //                    Color aWarningColor(255, 0, 0);
    //
    //                    TodDrawString(g, aWarningText, aTextX, aTextY, Sexy::FONT_DWARVENTODCRAFT24, aWarningColor, DS_ALIGN_CENTER);
    //                }
    //            }
    //        }
    //    }

    DeferOverlay(0);
}

void SeedChooserScreen::SetPageIndex(int thePageIndex) {
    mPageIndex = thePageIndex == 0 ? 0 : 1;
    // 翻至第一页时光标移动回第一张卡，翻至第二页时光标移动至最后一张卡
    int x = 0, y = 0;
    int aSeedIndex = (mPageIndex == 1) ? GetCurrentPageSeedCount() - 1 : 0;
    GetSeedPositionInChooser(aSeedIndex, x, y);
    mCursorPositionX1 = mCursorPositionX2 = x;
    mCursorPositionY1 = mCursorPositionY2 = y;
    mSeedIndex1 = mSeedIndex2 = aSeedIndex;
}

void SeedChooserScreen::DrawBanIcon(Sexy::Graphics *g) {
    if (!mApp->IsVSMode())
        return;

    if (mBanningPhase) {
        Graphics aBanGraphics(*g);
        aBanGraphics.mTransX = 0;
        aBanGraphics.mTransY = 0;
        aBanGraphics.SetColor(Color(205, 0, 0, 255));
        aBanGraphics.SetFont(Sexy::FONT_DWARVENTODCRAFT18);
        aBanGraphics.DrawString(TodStringTranslate("[VS_UI_BAN_PHASE_BIG]"), 440, 110);
    }

    const int firstPageSeedCount = mIsZombieChooser ? GetZombieFirstPageSeedCount(this) : int(SeedType::SEED_MELONPULT) + 1;
    const int secondPageStorageStart = mIsZombieChooser ? firstPageSeedCount : NUM_SEEDS_IN_CHOOSER;

    auto getSeedPage = [&](int theSeedPacketIndex) {
        if (theSeedPacketIndex < 0 || theSeedPacketIndex >= GetSeedStorageCount()) {
            return -1;
        }
        if (theSeedPacketIndex < firstPageSeedCount) {
            return 0;
        }
        if (mShowExtendedSeeds && theSeedPacketIndex >= secondPageStorageStart) {
            return 1;
        }
        return -1;
    };

    const int globalBpPlayerIndex = ResolveGlobalBpPlayerIndex();
    auto isGlobalBpSeed = [&](SeedType theSeedType) {
        if (VSSetupAddonWidget::msGlobalBpMode == VSSetupAddonWidget::GLOBALBP_CLOSED || globalBpPlayerIndex < 0 || globalBpPlayerIndex > 1) {
            return false;
        }

        for (SeedType globalBpSeed : VSSetupAddonWidget::msGlobalBpSeeds[globalBpPlayerIndex]) {
            if (globalBpSeed == theSeedType) {
                return true;
            }
        }
        return false;
    };

    SeedType otherPageBannedSeeds[NUM_SEEDS_IN_CHOOSER_EXTENDED]{};
    int otherPageBannedSeedCount = 0;

    for (const auto &aBannedSeed : mBannedSeed) {
        if (aBannedSeed.mSeedState != BannedSeedState::SEED_BANNED) {
            continue;
        }

        const int seedPacketIndex = GetSeedPacketIndex(aBannedSeed.mSeedType);
        const int seedPage = getSeedPage(seedPacketIndex);
        if (seedPage < 0) {
            continue;
        }

        if (seedPage != mPageIndex) {
            // 全局 BP 禁卡只在它所属页面正常显示，不加入底部的“另一页禁卡”列表。
            if (!isGlobalBpSeed(aBannedSeed.mSeedType)) {
                otherPageBannedSeeds[otherPageBannedSeedCount++] = aBannedSeed.mSeedType;
            }
            continue;
        }

        int drawSeedIndex = seedPacketIndex;
        if (mPageIndex == 1) {
            drawSeedIndex -= secondPageStorageStart;
        }

        int x = 0;
        int y = 0;
        GetSeedPositionInChooser(drawSeedIndex, x, y);
        g->DrawImage(IMAGE_MP_TARGETS_X, x + 5, y + 5);
    }

    if (otherPageBannedSeedCount == 0) {
        return;
    }

    // 在翻页按钮旁的底栏集中显示另一页的普通 BP 禁卡。
    constexpr int kCardSpacing = 53;
    constexpr int kBottomMargin = 8;
    constexpr int kPlantBottomCardsOffsetX = 40;
    constexpr int kZombieBottomCardsOffsetX = 8;
    const int cardWidth = Sexy::IMAGE_SEEDPACKETSILHOUETTE->mWidth;
    const int cardHeight = Sexy::IMAGE_SEEDPACKETSILHOUETTE->mHeight;
    const int bottomY = 500;
    const int smallBanIconWidth = IMAGE_MP_TARGETS_X->mWidth / 2;
    const int smallBanIconHeight = IMAGE_MP_TARGETS_X->mHeight / 2;

    Image *backgroundImage = mIsZombieChooser ? Sexy::IMAGE_SEEDCHOOSER_BACKGROUND2 : Sexy::IMAGE_SEEDCHOOSER_BACKGROUND;
    int areaLeft = kBottomMargin;
    int areaRight = backgroundImage->mWidth - kBottomMargin;

    if (mPageButton != nullptr) {
        if (mIsZombieChooser) {
            areaRight = mPageButton->mX - kBottomMargin;
        } else {
            areaLeft = mPageButton->mX + mPageButton->mWidth + kBottomMargin;
        }
    }

    int drawCount = otherPageBannedSeedCount;
    const int areaWidth = areaRight - areaLeft;
    const int maxCards = areaWidth >= cardWidth ? 1 + (areaWidth - cardWidth) / kCardSpacing : 0;
    if (drawCount > maxCards) {
        drawCount = maxCards;
    }
    if (drawCount <= 0) {
        return;
    }

    const int totalWidth = cardWidth + (drawCount - 1) * kCardSpacing;
    int drawX = areaLeft;
    if (areaWidth > totalWidth) {
        drawX += (areaWidth - totalWidth) / 2;
    }
    if (mIsZombieChooser) {
        drawX += kZombieBottomCardsOffsetX;
    } else {
        drawX -= kPlantBottomCardsOffsetX;
    }

    Color cardColor(255, 255, 255, 255);
    for (int i = 0; i < drawCount; ++i) {
        const SeedType seedType = otherPageBannedSeeds[i];
        DrawPacket(g, drawX, bottomY, seedType, SeedType::SEED_NONE, 0.0f, 155, &cardColor, true, true);

        const int banX = drawX + (cardWidth - smallBanIconWidth) / 2;
        const int banY = bottomY + (cardHeight - smallBanIconHeight) / 2;
        g->DrawImage(IMAGE_MP_TARGETS_X, banX, banY, smallBanIconWidth, smallBanIconHeight);

        drawX += kCardSpacing;
    }
}

SeedType SeedChooserScreen::SeedHitTest(int x, int y) {
    if (!mMouseVisible) {
        return SEED_NONE;
    }

    const int pageSeedCount = GetCurrentPageSeedCount();

    for (int pageSeedIndex = 0; pageSeedIndex < pageSeedCount; ++pageSeedIndex) {
        const int storageIndex = GetPageSeedStorageIndex(pageSeedIndex);

        if (storageIndex < 0 || storageIndex >= GetSeedStorageCount()) {
            continue;
        }

        const SeedType seedType = mIsZombieChooser ? GetZombieSeedType(storageIndex) : GetPlantSeedType(storageIndex);

        if (seedType == SeedType::SEED_NONE || !HasPacket(seedType, mIsZombieChooser)) {
            continue;
        }

        ChosenSeed &aChosenSeed = GetChosenSeed(storageIndex);
        if (aChosenSeed.mSeedState == SEED_PACKET_HIDDEN) {
            continue;
        }

        int chooserX = 0;
        int chooserY = 0;
        GetSeedPositionInChooser(pageSeedIndex, chooserX, chooserY);

        if (Rect(aChosenSeed.mX, aChosenSeed.mY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT).Contains(x, y)) {
            return seedType;
        }

        if (Rect(chooserX, chooserY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT).Contains(x, y)) {
            return seedType;
        }
    }
    return SEED_NONE;
}

void SeedChooserScreen::VSAutoPickResourceGen() {
    SeedType aSeedType;
    int aX = 0, aY = 0;
    if (mSeedsInBank == 0) {
        if (mIsZombieChooser) {
            aSeedType = SeedType::SEED_ZOMBIE_GRAVESTONE;
        } else {
            aSeedType = mBoard->StageIsNight() ? SeedType::SEED_SUNSHROOM : SeedType::SEED_SUNFLOWER;
        }
        int aIndex = GetSeedPacketIndex(aSeedType);
        GetSeedPositionInBank(0, aX, aY, 0);
        GetChosenSeed(aIndex).mX = GetChosenSeed(aIndex).mStartX = GetChosenSeed(aIndex).mEndX = aX;
        GetChosenSeed(aIndex).mY = GetChosenSeed(aIndex).mStartY = GetChosenSeed(aIndex).mEndY = aY;
        GetChosenSeed(aIndex).mChosenPlayerIndex = mIsZombieChooser;
        GetChosenSeed(aIndex).mSeedIndexInBank = 0;
        GetChosenSeed(aIndex).mSeedState = ChosenSeedState::SEED_IN_BANK;
        GetChosenSeed(aIndex).mCrazyDavePicked = true;
        ++mSeedsInBank;
        ++mSeedsIn1PBank;
    }
}

bool SeedChooserScreen::KeyDown(Sexy::KeyCode theKey) {
    // 联机对战屏蔽按键，仅允许返回键
    if (gTcpConnected || gTcpClientSocket >= 0) {
        return theKey == KEYCODE_BACK;
    }

    return old_SeedChooserScreen_KeyDown(this, theKey);
}

bool SeedChooserScreen::KeyUp(Sexy::KeyCode theKey) {
    // 联机对战屏蔽按键，仅允许返回键
    if (gTcpConnected || gTcpClientSocket >= 0) {
        return theKey == KEYCODE_BACK;
    }

    return old_SeedChooserScreen_KeyUp(this, theKey);
}
