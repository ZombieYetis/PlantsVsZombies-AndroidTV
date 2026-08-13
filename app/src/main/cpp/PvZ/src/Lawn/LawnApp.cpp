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

#include "PvZ/Lawn/LawnApp.h"
#include "Homura/Assert.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/System/SaveGame.h"
#include "PvZ/Lawn/System/TypingCheck.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/ConfirmBackToMainDialog.h"
#include "PvZ/Lawn/Widget/MainMenu.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/SettingsDialog.h"
#include "PvZ/Lawn/Widget/TitleScreen.h"
#include "PvZ/Lawn/Widget/VSResultsMenu.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Lawn/Widget/WaitForSecondPlayerDialog.h"
#include "PvZ/NetPlay.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Buffer.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Widget/ButtonWidget.h"
#include "PvZ/SexyAppFramework/Widget/Dialog.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <unistd.h>

#include <cstdint>
#include <algorithm>
#include <limits>
#include <ranges>

using namespace Sexy;

namespace {
constexpr int kNetPingIntervalTicks = 100; // ~1s
constexpr int kNetPingTimeoutTicks = 1200; // ~12s

void ResetNetDelayState() {
    gNetPingSendCounter = 0;
    gNetDelayNow = 0;
    gNetPingHasValidDelay = false;
    gNetPingAwaitingPong = false;
    gNetPingNowTick = 0;
    gNetPingLatestSentTick = 0;
    gNetPingLastPongTick = 0;
    gSpectatePeerPingValid = false;
    gSpectatePeerPingToken = 0;
    gSpectatePeerPingRecvTick = 0;
}

void TickNetDelayAwaitingPong() {
    if (gNetPingAwaitingPong) {
        const auto elapsedTicks = static_cast<uint16_t>(gNetPingNowTick - gNetPingLatestSentTick);
        if (elapsedTicks >= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
            gNetDelayNow = kNetPingTimeoutTicks;
            gNetPingHasValidDelay = true;
            gNetPingAwaitingPong = false;
        }
        return;
    }

    if (gNetPingHasValidDelay) {
        const auto elapsedTicks = static_cast<uint16_t>(gNetPingNowTick - gNetPingLastPongTick);
        if (elapsedTicks >= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
            gNetPingHasValidDelay = false;
            gNetDelayNow = 0;
        }
    }
}

void SendPeriodicNetPing() {
    ++gNetPingSendCounter;
    if (gNetPingSendCounter < kNetPingIntervalTicks) {
        return;
    }

    gNetPingSendCounter = 0;
    if (gIsServerModeSpectator) {
        gNetPingAwaitingPong = false;
        U16_Event eventPing = {{EVENT_PING}, static_cast<uint16_t>(gNetPingNowTick)};
        netplay::PutEvent(eventPing);
        return;
    }

    if (!gNetPingAwaitingPong) {
        gNetPingLatestSentTick = gNetPingNowTick;
        gNetPingAwaitingPong = true;
    } else {
        const auto elapsedTicks = static_cast<uint16_t>(gNetPingNowTick - gNetPingLatestSentTick);
        const uint16_t clampedElapsed = elapsedTicks > static_cast<uint16_t>(kNetPingTimeoutTicks) ? static_cast<uint16_t>(kNetPingTimeoutTicks) : elapsedTicks;
        if (!gNetPingHasValidDelay || clampedElapsed > static_cast<uint16_t>(gNetDelayNow)) {
            gNetDelayNow = static_cast<int>(clampedElapsed);
            gNetPingHasValidDelay = true;
        }
    }

    U16_Event eventPing = {{EVENT_PING}, gNetPingLatestSentTick};
    netplay::PutEvent(eventPing);
}

} // namespace

// 此处写明具体每个贴图对应哪个文件.
void LawnApp::LoadAddonImages() {
    addonImages.pole_night = GetImageByFileName("addonFiles/images/pole_night");
    addonImages.trees_night = GetImageByFileName("addonFiles/images/trees_night");
    addonImages.googlyeye = GetImageByFileName("addonFiles/images/googlyeye");
    addonImages.squirrel = GetImageByFileName("addonFiles/images/squirrel");
    addonImages.stripe_day_coop = GetImageByFileName("addonFiles/images/stripe_day_coop");
    addonImages.stripe_pool_coop = GetImageByFileName("addonFiles/images/stripe_pool_coop");
    addonImages.stripe_roof_left = GetImageByFileName("addonFiles/images/stripe_roof_left");
    addonImages.butter_glove = GetImageByFileName("addonFiles/images/butter_glove");
    addonImages.custom_cobcannon = GetImageByFileName("addonFiles/images/custom_cobcannon");
    addonImages.hood1_house = GetImageByFileName("addonFiles/images/hood1_house");
    addonImages.hood2_house = GetImageByFileName("addonFiles/images/hood2_house");
    addonImages.hood3_house = GetImageByFileName("addonFiles/images/hood3_house");
    addonImages.hood4_house = GetImageByFileName("addonFiles/images/hood4_house");
    addonImages.house_hill_house = GetImageByFileName("addonFiles/images/house_hill_house");
    addonImages.achievement_homeLawnsecurity = GetImageByFileName("addonFiles/images/achievement_homeLawnsecurity");
    addonImages.achievement_chomp = GetImageByFileName("addonFiles/images/achievement_chomp");
    addonImages.achievement_closeshave = GetImageByFileName("addonFiles/images/achievement_closeshave");
    addonImages.achievement_coop = GetImageByFileName("addonFiles/images/achievement_coop");
    addonImages.achievement_explodonator = GetImageByFileName("addonFiles/images/achievement_explodonator");
    addonImages.achievement_garg = GetImageByFileName("addonFiles/images/achievement_garg");
    addonImages.achievement_immortal = GetImageByFileName("addonFiles/images/achievement_immortal");
    addonImages.achievement_shop = GetImageByFileName("addonFiles/images/achievement_shop");
    addonImages.achievement_soilplants = GetImageByFileName("addonFiles/images/achievement_soilplants");
    addonImages.achievement_tree = GetImageByFileName("addonFiles/images/achievement_tree");
    addonImages.achievement_versusz = GetImageByFileName("addonFiles/images/achievement_versusz");
    addonImages.achievement_morticulturalist = GetImageByFileName("addonFiles/images/achievement_morticulturalist");
    addonImages.hole = GetImageByFileName("addonFiles/images/hole");
    addonImages.hole_bjorn = GetImageByFileName("addonFiles/images/hole_bjorn");
    addonImages.hole_china = GetImageByFileName("addonFiles/images/hole_china");
    addonImages.hole_gems = GetImageByFileName("addonFiles/images/hole_gems");
    addonImages.hole_chuzzle = GetImageByFileName("addonFiles/images/hole_chuzzle");
    addonImages.hole_heavyrocks = GetImageByFileName("addonFiles/images/hole_heavyrocks");
    addonImages.hole_duwei = GetImageByFileName("addonFiles/images/hole_duwei");
    addonImages.hole_pipe = GetImageByFileName("addonFiles/images/hole_pipe");
    addonImages.hole_tiki = GetImageByFileName("addonFiles/images/hole_tiki");
    addonImages.hole_worm = GetImageByFileName("addonFiles/images/hole_worm");
    addonImages.hole_top = GetImageByFileName("addonFiles/images/hole_top");
    addonImages.plant_can = GetImageByFileName("addonFiles/images/plant_can");
    addonImages.zombie_can = GetImageByFileName("addonFiles/images/zombie_can");
    addonImages.plant_pile01_stack01 = GetImageByFileName("addonFiles/images/plant_pile01_stack01");
    addonImages.plant_pile01_stack02 = GetImageByFileName("addonFiles/images/plant_pile01_stack02");
    addonImages.plant_pile02_stack01 = GetImageByFileName("addonFiles/images/plant_pile02_stack01");
    addonImages.plant_pile02_stack02 = GetImageByFileName("addonFiles/images/plant_pile02_stack02");
    addonImages.plant_pile03_stack01 = GetImageByFileName("addonFiles/images/plant_pile03_stack01");
    addonImages.plant_pile03_stack02 = GetImageByFileName("addonFiles/images/plant_pile03_stack02");
    addonImages.zombie_pile01_stack01 = GetImageByFileName("addonFiles/images/zombie_pile01_stack01");
    addonImages.zombie_pile01_stack02 = GetImageByFileName("addonFiles/images/zombie_pile01_stack02");
    addonImages.zombie_pile01_stack03 = GetImageByFileName("addonFiles/images/zombie_pile01_stack03");
    addonImages.zombie_pile02_stack01 = GetImageByFileName("addonFiles/images/zombie_pile02_stack01");
    addonImages.zombie_pile02_stack02 = GetImageByFileName("addonFiles/images/zombie_pile02_stack02");
    addonImages.zombie_pile02_stack03 = GetImageByFileName("addonFiles/images/zombie_pile02_stack03");
    addonImages.zombie_pile03_stack01 = GetImageByFileName("addonFiles/images/zombie_pile03_stack01");
    addonImages.zombie_pile03_stack02 = GetImageByFileName("addonFiles/images/zombie_pile03_stack02");
    addonImages.zombie_pile03_stack03 = GetImageByFileName("addonFiles/images/zombie_pile03_stack03");
    addonImages.survival_button = GetImageByFileName("addonFiles/images/survival_button");
    addonImages.leaderboards = GetImageByFileName("addonFiles/images/leaderboards");
    addonImages.SelectorScreen_WoodSign3 = GetImageByFileName("addonFiles/images/ZombatarWidget/SelectorScreen_WoodSign3");
    addonImages.SelectorScreen_WoodSign3_press = GetImageByFileName("addonFiles/images/ZombatarWidget/SelectorScreen_WoodSign3_press");
    addonImages.zombatar_portrait = GetImageByFileName("ZOMBATAR");
    addonImages.crater_night_roof_center = GetImageByFileName("addonFiles/images/crater_night_roof_center");
    addonImages.crater_night_roof_center->mNumRows = 1;
    addonImages.crater_night_roof_center->mNumCols = 2;
    addonImages.crater_night_roof_left = GetImageByFileName("addonFiles/images/crater_night_roof_left");
    addonImages.crater_night_roof_left->mNumRows = 1;
    addonImages.crater_night_roof_left->mNumCols = 2;
    addonImages.leaderboard_selector = GetImageByFileName("images/leaderboard_selector");
    addonImages.zombie_duckytube_inwater = GetImageByFileName("reanim/zombie_duckytube_inwater");
    addonImages.burial_mound = GetImageByFileName("addonFiles/images/burial_mound");
    addonImages.burial_mound->mNumRows = 3;
    addonImages.burial_mound->mNumCols = 5;
    addonImages.burial_mound_dirt = GetImageByFileName("addonFiles/images/burial_mound_dirt");
    addonImages.burial_mound_dirt->mNumRows = 3;
    addonImages.burial_mound_dirt->mNumCols = 5;
    addonImages.seed_mounds = GetImageByFileName("addonFiles/images/seed_mounds");
    addonImages.seed_mounds->mNumCols = 5;
    addonImages.seedpacket_Zombie_Upgrade = GetImageByFileName("addonFiles/images/seedpacket_Zombie_Upgrade");
    addonImages.VS_Button = GetImageByFileName("images/VS Button");
    addonImages.VS_Button_selected = GetImageByFileName("images/VS Button_selected");
    addonImages.zombie_target = GetImageByFileName("reanim/zombie_target");

    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_berserker_helmet2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET3 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_berserker_helmet3");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_LEFTARM_HAND = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_berserker_leftarm_hand");
    addonImages.IMAGE_SUPERFAN_ZOMBIEIMPHEAD = GetImageByFileName("addonFiles/particles/ExtendedZombies/ZombieSuperFanImpHead");
    addonImages.IMAGE_REANIM_ZOMBIE_SUPER_FAN_IMP_OUTARM_GLOVE = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_Ghost_Fans2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_IMP_ARM1_BONE = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_imp_arm1_bone");
    addonImages.IMAGE_ZOMBIEJACKSONHEAD = GetImageByFileName("particles/ZombieDancerHead");
    addonImages.IMAGE_ZOMBIEBACKUPDANCERHEAD = GetImageByFileName("particles/ZombieBackupDancerHead");
    addonImages.IMAGE_GIGA_ZOMBIEPOLEVAULTERHEAD = GetImageByFileName("addonFiles/particles/ExtendedZombies/ZombieGigaPolevaulterHead");
    addonImages.IMAGE_REANIM_ZOMBIE_EXPLORER_HEAD = GetImageByFileName("addonFiles/particles/ExtendedZombies/ZombieExplorerHead");
    addonImages.IMAGE_REANIM_ZOMBIE_DOGWALKER_HEAD = GetImageByFileName("addonFiles/particles/ExtendedZombies/ZombieDogWalkerHead");
    addonImages.IMAGE_PROJECTILEPOLE = GetImageByFileName("addonFiles/images/ExtendedZombies/Zombie_giga_polevaulter_pole");
    addonImages.IMAGE_PROJECTILEZOMBLOB = GetImageByFileName("addonFiles/images/ExtendedZombies/zombie_zomblob_split");
    addonImages.IMAGE_PROJECTILESPORE = GetImageByFileName("addonFiles/images/ProjectileSpore");
    addonImages.IMAGE_PROJECTILEBOOMERANG = GetImageByFileName("addonFiles/images/ProjectileBoomerang");
    addonImages.IMAGE_REANIM_ZOMBLOBHEAD_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombiezomblobhead_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_body_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_DYING_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_body_dying_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY2_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_body2_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY3_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_body3_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_HEAD_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_head_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_HEAD2_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_head2_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_JAW_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_jaw_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_UPPER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_upper_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_UPPER2_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_upper2_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_lower_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER2_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_lower2_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER3_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_lower3_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_hand_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND2_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_hand2_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND3_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerarm_hand3_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_UPPER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerleg_upper_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_LOWER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerleg_lower_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_FOOT_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_innerleg_foot_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_UPPER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerarm_upper_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_LOWER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerarm_lower_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_HAND_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerarm_hand_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_HAND2_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerarm_hand2_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_UPPER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerleg_upper_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_LOWER_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerleg_lower_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_FOOT_BUTTERED = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_zomblob_outerleg_foot_buttered");
    addonImages.IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_UPPER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_jackson_outerarm_upper_bone");
    addonImages.IMAGE_REANIM_ZOMBIE_BACKUP_OUTERARM_UPPER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_backup_outerarm_upper_bone2");
    addonImages.IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_HAND = GetImageByFileName("reanim/zombie_jackson_outerarm_hand");
    addonImages.IMAGE_REANIM_ZOMBIE_DANCER_INNERARM_HAND = GetImageByFileName("reanim/zombie_dancer_innerarm_hand");
    addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_sunday_edition_paper2");
    addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER3 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_sunday_edition_paper3");
    addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_LEFTARM_UPPER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_sunday_edition_leftarm_upper2");
    addonImages.IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_LEFTARM_LOWER = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_sunday_edition_leftarm_lower");
    addonImages.IMAGE_REANIM_ZOMBIE_EXPLORER_OUTERARM_UPPER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/Zombie_explorer_outerarm_upper2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_BODY1_2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_gargantuar_body1_2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_OUTERARM_LOWER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_gargantuar_outerarm_lower2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_BODY1_3 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_gargantuar_body1_3");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_FOOT2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_gargantuar_foot2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_HEAD2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_gargantuar_head2");
    addonImages.IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_TELEPHONEPOLE_COIL = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_giga_gargantuar_telephonepole_coil");
    addonImages.IMAGE_REANIM_ZOMBIE_DOGWALKER_OUTERARM_UPPER2 = GetImageByFileName("addonFiles/reanim/ExtendedZombies/zombie_dogwalker_outerarm_upper2");
    addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_LOWER2 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/celery_stalker_arm2_lower2");
    addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_LOWER3 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/celery_stalker_arm2_lower3");
    addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER2 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/celery_stalker_arm2_upper2");
    addonImages.IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER3 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/celery_stalker_arm2_upper3");
    addonImages.IMAGE_REANIM_SWEET_POTATO_BODY2 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/sweet_potato_body2");
    addonImages.IMAGE_REANIM_SWEET_POTATO_BODY3 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/sweet_potato_body3");
    addonImages.IMAGE_REANIM_SWEET_POTATO_MOUTH2 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/sweet_potato_mouth2");
    addonImages.IMAGE_REANIM_SWEET_POTATO_MOUTH3 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/sweet_potato_mouth3");
    addonImages.IMAGE_REANIM_SWEET_POTATO_EYE3 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/sweet_potato_eye3");
    addonImages.IMAGE_REANIM_ICE1 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/ice1");
    addonImages.IMAGE_REANIM_ICE2 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/ice2");
    addonImages.IMAGE_REANIM_ICE3 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/ice3");
    addonImages.IMAGE_REANIM_ICE4 = GetImageByFileName("addonFiles/reanim/ExtendedPlants/ice4");
    addonImages.IMAGE_SHOVELBANK_VERTICAL = GetImageByFileName("addonFiles/images/shovel_bank_vertical");
    addonImages.IMAGE_SHOVEL_VERTICAL = GetImageByFileName("addonFiles/images/shovel_vertical");

    //    int xClip = 130;
    //    int yClip = 130;
    //    Sexy::Rect rect = {*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr->mWidth - xClip, *Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr->mHeight - yClip, xClip, yClip};
    //    addonImages.VSDay = CopyImage(*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr, rect);
    //    addonImages.VSNight = CopyImage(*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr, rect);
    //    addonImages.VSPool = CopyImage(*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr, rect);
    //    addonImages.VSPoolNight = CopyImage(*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr, rect);
    //    addonImages.VSRoof = CopyImage(*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr, rect);
    //    addonImages.VSRoofNight = CopyImage(*Sexy_IMAGE_CHALLENGE_THUMBNAILS_Addr, rect);

    (IMAGE_BLANK)->ClearRect({0, 0, (IMAGE_BLANK)->mWidth, (IMAGE_BLANK)->mHeight}); // 手动把IMAGE_BLANK清空

    int addonImagesNum = (sizeof(AddonImages) / sizeof(Sexy::Image *));
    mCompletedLoadingThreadTasks += 9 * addonImagesNum;

    // for (int i = 0; i < addonImagesNum; ++i) {
    // if (*((Sexy::Image **) ((char *) &AddonImages + i * sizeof(Sexy::Image *))) == NULL){
    // LOGD("没成功%d",i);
    // }
    // }
}

// 此处写明具体每个音频对应哪个文件.
void LawnApp::LoadAddonSounds() {
    addonSounds.achievement = GetSoundByFileName("addonFiles/sounds/achievement");
    addonSounds.thriller = GetSoundByFileName("addonFiles/sounds/thriller");
    addonSounds.allstardbl = GetSoundByFileName("addonFiles/sounds/allstardbl");
    addonSounds.whistle = GetSoundByFileName("addonFiles/sounds/whistle");
    addonSounds.explorer = GetSoundByFileName("addonFiles/sounds/explorer");
    addonSounds.zomblob = GetSoundByFileName("addonFiles/sounds/zomblob");
    addonSounds.iceberg = GetSoundByFileName("addonFiles/sounds/iceberg");
    addonSounds.celery_stalker_rise = GetSoundByFileName("addonFiles/sounds/celery_stalker_rise");
    addonSounds.celery_stalker_attack = GetSoundByFileName("addonFiles/sounds/celery_stalker_attack");
    addonSounds.bloomerang = GetSoundByFileName("addonFiles/sounds/bloomerang");
    addonSounds.bonk_choy_punch = GetSoundByFileName("addonFiles/sounds/bonk_choy_punch");
    addonSounds.bonk_choy_uppercut = GetSoundByFileName("addonFiles/sounds/bonk_choy_uppercut");
    addonSounds.power_pole_charge = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Charge");
    addonSounds.power_pole_core = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Core");
    addonSounds.power_pole_hifi = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_HiFi");
    addonSounds.power_pole_tail = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Tail");
    addonSounds.power_pole_width = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Width");
    addonSounds.giga_laugh = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Laugh");
    addonSounds.giga_laugh2 = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Laugh2");
    addonSounds.giga_laugh3 = GetSoundByFileName("addonFiles/sounds/GigaGarg_PowerPole_Laugh3");

    int addonSoundsNum = (sizeof(addonSounds) / sizeof(int));
    mCompletedLoadingThreadTasks += 54 * addonSoundsNum;
}

Image *LawnApp::GetImageByFileName(const char *theFileName) {
    // 根据贴图文件路径获得贴图
    Image *theImage = GetImage(theFileName, true);
    LOG_DEBUG_IF(theImage == nullptr, "Failed to get image of {:?}", theFileName);
    return theImage;
}

int LawnApp::GetSoundByFileName(const char *theFileName) {
    // 根据音频文件路径获得音频
    int theSoundId = mSoundManager->LoadSound(theFileName);
    return theSoundId;
}

void LawnApp::DoConfirmBackToMain(bool theIsSave) {
    // 实现在花园直接退出而不是弹窗退出；同时实现新版暂停菜单
    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN) {
        mBoardResult = BoardResult::BOARDRESULT_QUIT;
        // if (theIsSave) Board_TryToSaveGame(lawnApp->mBoard);
        DoBackToMain();
        return;
    }
    if ((mGameMode == GameMode::GAMEMODE_MP_VS || mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM || enableNewOptionsDialog) && GetDialog(Dialogs::DIALOG_NEWOPTIONS) == nullptr) {
        DoNewOptions(false, 0);
        return;
    }
    auto *aBackDialog = new ConfirmBackToMainDialog(theIsSave);
    AddDialog(Dialogs::DIALOG_CONFIRM_BACK_TO_MAIN, aBackDialog);
    mWidgetManager->SetFocus(aBackDialog);
}


void LawnApp::ClearSecondPlayer() {
    gIsServerModeNetplay = false;
    gIsConnectedToServer = false;
    gServerModeTransport = ServerModeTransport::NONE;
    gIsServerModeSpectator = false;
    gIsReplayMode = false;
    gReplayPauseByMenu = false;
    gReplayHostName[0] = '\0';
    gReplayGuestName[0] = '\0';
    if (gTcpConnected) {
        close(gTcpServerSocket);
        gTcpServerSocket = -1;
        gTcpConnected = false;
    }
    if (gTcpClientSocket >= 0) {
        close(gTcpClientSocket);
        gTcpClientSocket = -1;
    }
    if (gTcpListenSocket >= 0) {
        close(gTcpListenSocket);
        gTcpListenSocket = -1;
    }
    if (gUdpScanSocket >= 0) {
        close(gUdpScanSocket);
        gUdpScanSocket = -1;
    }
    if (gUdpBroadcastSocket >= 0) {
        close(gUdpBroadcastSocket);
        gUdpBroadcastSocket = -1;
    }
    ResetNetDelayState();
    clientRecvBuffer.clear();
    serverRecvBuffer.clear();
    replay::ResetRecorder();
    replay::StopPlayback();
    old_LawnApp_ClearSecondPlayer(this);
}

void LawnApp::DoBackToMain() {
    // 实现每次退出游戏后都清空2P
    ClearSecondPlayer();

    old_LawnApp_DoBackToMain(this);
}

void LawnApp::DoSettingsDialog(bool theIsModal) {
    auto *aSettingsDialog = new SettingsDialog(this);
    AddDialog(aSettingsDialog);
    CenterDialog(aSettingsDialog, 413, 535);
    mWidgetManager->AddWidget(aSettingsDialog);
    aSettingsDialog->WaitForResult(true);
}

void LawnApp::DoNewOptions(bool theFromGameSelector, unsigned int a3) {
    old_LawnApp_DoNewOptions(this, theFromGameSelector, a3);
    if (gIsServerModeSpectator) { // 观战不显示投降按钮
        if (auto *dialog = GetDialog(Dialogs::DIALOG_NEWOPTIONS)) {

            if (auto *concedeButton = dialog->FindWidget(5)) {
                concedeButton->mDisabled = true;
                ((Sexy::ButtonWidget *)concedeButton)->mBtnNoDraw = true;
            }
        }
    }

    if (gIsReplayMode) { // 回放中把投降按钮替换为回放管理按钮
        if (auto *dialog = GetDialog(Dialogs::DIALOG_NEWOPTIONS)) {

            if (auto *concedeButton = dialog->FindWidget(5)) {
                *((Sexy::ButtonWidget *)concedeButton)->mLabel = TodStringTranslate("[REPLAY_MANAGE]");
            }
        }
    }
}

bool LawnApp::Is3DAccelerated() const {
    // 修复关闭3D加速后MV错位
    return mNewIs3DAccelerated || (mCreditScreen != nullptr);
}

void LawnApp::Set3DAccelerated(bool isAccelerated) {
    mNewIs3DAccelerated = isAccelerated;
    mPlayerInfo->mIs3DAcceleratedClosed = !isAccelerated;
}

void LawnApp::OnSessionTaskFailed() {
    // 用此空函数替换游戏原有的LawnApp_OnSessionTaskFailed()函数，从而去除启动游戏时的“网络错误：255”弹窗
}

int LawnApp::GamepadToPlayerIndex(unsigned int thePlayerIndex) const {
    // 实现双人结盟中1P选卡选满后自动切换为2P选卡DoConfirmBackToMain
    if (IsCoopMode()) {
        return !m1PChoosingSeeds;
    }

    if (thePlayerIndex <= 3) {
        if (mPlayerInfo && thePlayerIndex == mPlayerInfo->GetVTable()->GetId(mPlayerInfo))
            return 0;

        if (mSecondPlayerGamepadIndex != -1 && mSecondPlayerGamepadIndex == thePlayerIndex)
            return 1;
    }
    return -1;
}


void LawnApp::HandleTcpClientMessage(const std::byte *buf, size_t bufSize) {
    clientRecvBuffer.append_range(std::views::counted(buf, bufSize));
    size_t offset = 0;

    while (clientRecvBuffer.size() >= offset + sizeof(BaseEvent)) {
        const auto *clientRecvPtr = &clientRecvBuffer[offset];
        if (clientRecvBuffer.size() < offset + netplay::ParseEventSize(clientRecvPtr)) {
            break;
        }
        alignas(std::max_align_t) std::byte alignedBuf[std::numeric_limits<decltype(BaseEvent::size)>::max()];
        static_assert(std::size(alignedBuf) <= UINT8_MAX, "'alignedBuf' is too big, please use dynamically allocating");

        BaseEvent *event = netplay::GetEvent(alignedBuf, clientRecvPtr);
        replay::RecordPacket(ReplayPacketDir::InboundClient, clientRecvPtr, event->size, static_cast<std::uint32_t>(mAppCounter));
        LOG_DEBUG("event.type = {}", int(event->type));

        if (event->type == EVENT_PING) {
            auto *eventPing = static_cast<const U16_Event *>(event);
            U16_Event eventPong = {{EVENT_PONG}, eventPing->data};
            netplay::PutEvent(eventPong);
        } else if (event->type == EVENT_PONG) {
            auto *eventPong = static_cast<const U16_Event *>(event);
            if (gNetPingAwaitingPong && eventPong->data == gNetPingLatestSentTick) {
                const auto rttTicks = static_cast<uint16_t>(gNetPingNowTick - eventPong->data);
                if (rttTicks <= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
                    gNetDelayNow = static_cast<int>(rttTicks);
                    gNetPingHasValidDelay = true;
                    gNetPingLastPongTick = gNetPingNowTick;
                    gNetPingAwaitingPong = false;
                }
            }
        } else if (event->type >= EVENT_CLIENT_BOARD_TOUCH_DOWN && event->type < NUM_EVENT_BOARD) {
            if (mBoard != nullptr) {
                mBoard->processClientEvent(event);
            }
        } else if (event->type >= EVENT_SERVER_CHALLENGESCREEN_SELECT_MODE && event->type < NUM_EVENT_CHALLENGESCREEN) {
            if (mChallengeScreen != nullptr) {
                mChallengeScreen->processClientEvent(event);
            }
        } else if (event->type >= EVENT_SERVER_VSSETUPMENU_BUTTON_DEPRESS && event->type < NUM_EVENT_VSSETUPMENU) {
            if (mVSSetupMenu != nullptr) {
                mVSSetupMenu->processClientEvent(event);
            }
        } else if (event->type >= EVENT_CLIENT_VSRESULT_BUTTON_DEPRESS && event->type < NUM_EVENT_VSRESULT) {
            if (mVSResultsMenu != nullptr) {
                mVSResultsMenu->processClientEvent(event);
            }
        } else if (event->type >= EVENT_SERVER_WAITFORSECONDPALYER_VERSION_CHECK && event->type < NUM_EVENT_WAITFORSECONDPALYER) {
            if (auto *dialog = GetDialog(DIALOG_WAIT_FOR_SECOND_PLAYER)) {
                static_cast<WaitForSecondPlayerDialog *>(dialog)->processClientEvent(event);
            }
        } else {
            throw std::runtime_error{std::format("Unknown-type event (type = {}, size = {})", int(event->type), event->size)};
        }
        offset += event->size;
    }
    if (offset > 0) {
        clientRecvBuffer.erase(clientRecvBuffer.begin(), clientRecvBuffer.begin() + offset);
    }
}

void LawnApp::HandleTcpServerMessage(const std::byte *buf, size_t bufSize) {
    serverRecvBuffer.append_range(std::views::counted(buf, bufSize));
    auto *waitDialog = WaitForSecondPlayerDialog::GetInstance();
    size_t offset = 0;

    while (serverRecvBuffer.size() >= offset + sizeof(BaseEvent)) {
        const auto *serverRecvPtr = &serverRecvBuffer[offset];
        if (serverRecvBuffer.size() < offset + netplay::ParseEventSize(serverRecvPtr)) {
            break;
        }
        alignas(std::max_align_t) std::byte alignedBuf[std::numeric_limits<decltype(BaseEvent::size)>::max()];
        static_assert(std::size(alignedBuf) <= UINT8_MAX, "'alignedBuf' is too big, please use dynamically allocating");

        BaseEvent *event = netplay::GetEvent(alignedBuf, serverRecvPtr);
        replay::RecordPacket(ReplayPacketDir::InboundServer, serverRecvPtr, event->size, static_cast<std::uint32_t>(mAppCounter));
        LOG_DEBUG("event.type = {}", int(event->type));

        if (waitDialog != nullptr && waitDialog->ServerIsWaitingReservedSpectate()) {
            if (event->type == EVENT_SERVER_VSSETUPMENU_SYNC_VS_MODE) {
                waitDialog->processServerEvent(event);
            }
            offset += event->size;
            continue;
        }

        if (event->type == EVENT_PING) {
            auto *eventPing = static_cast<const U16_Event *>(event);
            if (gIsServerModeSpectator) {
                gSpectatePeerPingValid = true;
                gSpectatePeerPingToken = eventPing->data;
                gSpectatePeerPingRecvTick = gNetPingNowTick;
            } else {
                U16_Event eventPong = {{EVENT_PONG}, eventPing->data};
                netplay::PutEvent(eventPong);
            }
        } else if (event->type == EVENT_PONG) {
            auto *eventPong = static_cast<const U16_Event *>(event);
            if (gIsServerModeSpectator) {
                if (gNetPingAwaitingPong && eventPong->data == gNetPingLatestSentTick) {
                    const auto rttTicks = static_cast<uint16_t>(gNetPingNowTick - eventPong->data);
                    if (rttTicks <= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
                        gNetDelayNow = static_cast<int>(rttTicks);
                        gNetPingHasValidDelay = true;
                        gNetPingLastPongTick = gNetPingNowTick;
                        gNetPingAwaitingPong = false;
                    }
                } else if (gSpectatePeerPingValid && eventPong->data == gSpectatePeerPingToken) {
                    const auto rttTicks = static_cast<uint16_t>(gNetPingNowTick - gSpectatePeerPingRecvTick);
                    if (rttTicks <= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
                        gNetDelayNow = std::max<int>(1, static_cast<int>(rttTicks));
                        gNetPingHasValidDelay = true;
                        gNetPingLastPongTick = gNetPingNowTick;
                    }
                    gSpectatePeerPingValid = false;
                }
            } else if (gNetPingAwaitingPong && eventPong->data == gNetPingLatestSentTick) {
                const auto rttTicks = static_cast<uint16_t>(gNetPingNowTick - eventPong->data);
                if (rttTicks <= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
                    gNetDelayNow = static_cast<int>(rttTicks);
                    gNetPingHasValidDelay = true;
                    gNetPingLastPongTick = gNetPingNowTick;
                    gNetPingAwaitingPong = false;
                }
            }
        } else if (event->type >= EVENT_SERVER_CHALLENGESCREEN_SELECT_MODE && event->type < NUM_EVENT_CHALLENGESCREEN) {
            if (mChallengeScreen != nullptr) {
                mChallengeScreen->processServerEvent(event);
            }
        } else if (event->type >= EVENT_CLIENT_BOARD_TOUCH_DOWN && event->type < NUM_EVENT_BOARD) {
            if (mBoard != nullptr) {
                const bool spectatorClientTouch = (gIsServerModeSpectator || gIsReplayMode)
                    && (event->type == EVENT_CLIENT_BOARD_TOUCH_DOWN || event->type == EVENT_CLIENT_BOARD_TOUCH_DRAG || event->type == EVENT_CLIENT_BOARD_TOUCH_UP
                        || event->type == EVENT_CLIENT_BOARD_PAUSE || event->type == EVENT_CLIENT_BOARD_CONCEDE);
                if (spectatorClientTouch) {
                    // Spectator consumes selected client->host events locally.
                    mBoard->processClientEvent(event);
                } else {
                    mBoard->processServerEvent(event);
                }
            }
        } else if (waitDialog != nullptr && event->type == EVENT_SERVER_VSSETUPMENU_SYNC_VS_MODE && gIsServerModeSpectator) {
            waitDialog->processServerEvent(event);
        } else if (event->type >= EVENT_SERVER_VSSETUPMENU_BUTTON_DEPRESS && event->type < NUM_EVENT_VSSETUPMENU) {
            if (mVSSetupMenu != nullptr) {
                const bool spectatorClientVsSetupEvent = (gIsServerModeSpectator || gIsReplayMode)
                    && (event->type == EVENT_CLIENT_VSSETUPMENU_MOVE_CONTROLLER || event->type == EVENT_CLIENT_SEEDCHOOSER_SELECT_SEED || event->type == EVENT_CLIENT_SEEDCHOOSER_BAN_SEED
                        || event->type == EVENT_CLIENT_SEEDCHOOSER_BUTTON_DEPRESS);
                if (spectatorClientVsSetupEvent) {
                    // Spectator consumes selected client->host VS setup/seedchooser events locally.
                    mVSSetupMenu->processClientEvent(event);
                } else {
                    mVSSetupMenu->processServerEvent(event);
                }
            }
        } else if (event->type >= EVENT_SERVER_WAITFORSECONDPALYER_VERSION_CHECK && event->type < NUM_EVENT_WAITFORSECONDPALYER) {
            if (waitDialog != nullptr) {
                waitDialog->processServerEvent(event);
            }
        } else if (event->type >= EVENT_CLIENT_VSRESULT_BUTTON_DEPRESS && event->type < NUM_EVENT_VSRESULT) {
            if (mVSResultsMenu != nullptr) {
                mVSResultsMenu->processServerEvent(event);
            }
        } else {
            throw std::runtime_error{std::format("Unknown-type event (type = {}, size = {})", int(event->type), event->size)};
        }
        offset += event->size;
    }
    if (offset > 0) {
        serverRecvBuffer.erase(serverRecvBuffer.begin(), serverRecvBuffer.begin() + offset);
    }
}

void LawnApp::FinishLoadGame() {
    PostEnterLevel();
    MakeNewBoard();

    const bool aLoaded = LawnLoadGame(mBoard, mSaveGame);
    if (aLoaded) {
        mBoard->PostLoadGame();
        delete mSaveGame;
        mSaveGame = nullptr;
        return;
    }

    delete mSaveGame;
    mSaveGame = nullptr;

    NewGame();
    if (GetDialogCount() == 0) {
        return;
    }

    mBoard->Pause(true);

    Sexy::Dialog *aDialog = mDialogMap->empty() ? nullptr : (--mDialogMap->end())->second;
    if (aDialog != nullptr) {
        mWidgetManager->SetFocus(aDialog);
    }
}

void LawnApp::UpdateFrames() {
    const bool replayActive = replay::IsPlaybackActive();
    const bool replayPaused = replay::IsPlaybackPaused();

    bool runReplayFrame = true;

    if (replayActive && !replayPaused) {
        runReplayFrame = replay::ConsumePlaybackFrameStep();
    }
    if ((gTcpClientSocket >= 0 || gTcpConnected || replay::IsPlaybackActive()) && !replayPaused) {
        ++gNetPingNowTick;
        if (!gIsServerModeSpectator) {
            TickNetDelayAwaitingPong();
        } else if (gNetPingHasValidDelay) {
            const auto elapsedTicks = static_cast<uint16_t>(gNetPingNowTick - gNetPingLastPongTick);
            if (elapsedTicks >= static_cast<uint16_t>(kNetPingTimeoutTicks)) {
                gNetPingHasValidDelay = false;
                gNetDelayNow = 0;
                gSpectatePeerPingValid = false;
            }
        }
        if (!replayActive) {
            SendPeriodicNetPing();
        } else if (runReplayFrame) {
            replay::AdvancePlaybackOneTick();
        }
    }
    if (!replay::IsPlaybackActive()) {
        replay::TickPlayback();
    }

    std::byte buf[1024];

    if (gTcpClientSocket >= 0) {
        netplay::FlushSendBuffer(gTcpClientSocket);

        while (true) {
            ssize_t n = recv(gTcpClientSocket, buf, std::size(buf), MSG_DONTWAIT);
            if (n > 0) {
                HandleTcpClientMessage(buf, n);
            } else if (n == 0) {
                // 对端关闭连接（收到FIN）
                LOG_DEBUG("[TCP] 对方关闭连接");
                if (gTcpClientSocket >= 0) {
                    close(gTcpClientSocket);
                    gTcpClientSocket = -1;
                }
                clientRecvBuffer.clear();
                serverRecvBuffer.clear();
                netplay::ClearSendBuffer();
                ResetNetDelayState();
                if (mVSResultsMenu != nullptr) {
                    mVSResultsMenu->HandleOpponentDisconnected();
                } else if (!GetDialog(DIALOG_WAIT_FOR_SECOND_PLAYER)) {
                    if (gTcpListenSocket >= 0) {
                        close(gTcpListenSocket);
                        gTcpListenSocket = -1;
                    }
                    LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CONNECTION_CLOSED]", "[RECREATE_ROOM]", "[DIALOG_BUTTON_OK]", "", 3);
                }
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 没有更多数据可读，正常退出
                    break;
                } else if (errno == EINTR) {
                    // 被信号中断，重试
                    continue;
                } else {
                    LOG_DEBUG("[TCP] recv 出错 errno={}", errno);
                    if (gTcpClientSocket >= 0) {
                        close(gTcpClientSocket);
                        gTcpClientSocket = -1;
                    }
                    if (gTcpListenSocket >= 0) {
                        close(gTcpListenSocket);
                        gTcpListenSocket = -1;
                    }
                    clientRecvBuffer.clear();
                    serverRecvBuffer.clear();
                    netplay::ClearSendBuffer();
                    ResetNetDelayState();
                    LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CONNECTION_ERROR]", "[RECREATE_ROOM]", "[DIALOG_BUTTON_OK]", "", 3);
                    break;
                }
            }
        }
    }

    if (gTcpConnected) {
        if (gIsServerModeSpectator) {
            netplay::FlushSendBuffer(gTcpServerSocket);
        } else {
            netplay::FlushSendBuffer(gTcpServerSocket);
        }

        while (true) {
            ssize_t n = recv(gTcpServerSocket, buf, std::size(buf), MSG_DONTWAIT);
            if (n > 0) {
                HandleTcpServerMessage(buf, n);
            } else if (n == 0) {
                // 对端关闭连接（收到FIN）
                LOG_DEBUG("[TCP] 对方关闭连接");
                close(gTcpServerSocket);
                gTcpServerSocket = -1;
                gTcpConnecting = false;
                gTcpConnected = false;
                gIsConnectedToServer = false;
                gIsServerModeSpectator = false;
                clientRecvBuffer.clear();
                serverRecvBuffer.clear();
                netplay::ClearSendBuffer();
                ResetNetDelayState();
                if (mVSResultsMenu != nullptr) {
                    mVSResultsMenu->HandleOpponentDisconnected();
                } else {
                    LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CONNECTION_CLOSED]", "[REENTER_ROOM]", "[DIALOG_BUTTON_OK]", "", 3);
                }
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 没有更多数据可读，正常退出
                    break;
                } else if (errno == EINTR) {
                    // 被信号中断，重试
                    continue;
                } else {
                    LOG_DEBUG("[TCP] recv 出错 errno={}", errno);
                    close(gTcpServerSocket);
                    gTcpServerSocket = -1;
                    gTcpConnecting = false;
                    gTcpConnected = false;
                    gIsConnectedToServer = false;
                    clientRecvBuffer.clear();
                    serverRecvBuffer.clear();
                    netplay::ClearSendBuffer();
                    ResetNetDelayState();
                    LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CONNECTION_ERROR]", "[REENTER_ROOM]", "[DIALOG_BUTTON_OK]", "", 3);
                    break;
                }
            }
        }
    }

    // 下方为原版函数逻辑

    if (!mActive || mMinimized) {
        if (mBoard != nullptr) {
            mBoard->ResetFPSStats();
        }
    }

    //    UpdateSessionState(); // 连接渡维服务器用
    //    UpdatePlayTimeStats(); // 本来就是空函数，故注释

    int updateCount = 0;

    if (gSlowMo) {
        gSlowMoCounter++;
        if (gSlowMoCounter > 3) {
            gSlowMoCounter = 0;
            updateCount = 1;
        }
    } else if (gFastMo) {
        updateCount = 20;
    } else if (gStep) {
        if (gStepReady) {
            gStepReady = false;
            updateCount = 1;
        }
    } else {
        updateCount = 1;
    }

    if (replayActive && !replayPaused && !runReplayFrame) {
        updateCount = 0;
    }

    for (int i = 0; i < updateCount; ++i) {
        ++mAppCounter;

        if (mBoard != nullptr) {
            mBoard->ProcessDeleteQueue();
        }

        GamepadApp::UpdateFrames();
        mMusic->GetVTable()->MusicUpdate(mMusic);
        if (mPlayerInfo != nullptr) {
            const bool isLoaded = mPlayerInfo->GetVTable()->IsLoaded(mPlayerInfo);
            if (isLoaded) {
                mMailBox->Update();

                if (!mMailboxRefreshed) {
                    mMailBox->RefreshMessages();
                    mMailboxRefreshed = true;
                }
            }
        }

        if (mLoadingThreadCompleted && mEffectSystem != nullptr) {
            mEffectSystem->ProcessDeleteQueue();
        }

        CheckForGameEnd();
        UpdateSavingDingus();
    }

    if (mNeedLoadGame) {
        KillDialog(DIALOG_SAVING_FILE);
        mNeedLoadGame = false;
        FinishLoadGame();
        if (GetDialog(DIALOG_HANDLE_OLDGAMEFILE) == nullptr && GetDialog(DIALOG_HANDLE_INVALID_LEVEL) == nullptr) {
            mFirstTimeGameSelector = false;
            DoContinueDialog();
        }


        delete mSaveGame;
        mSaveGame = nullptr;
    }

    //    if (!mQueryCoinState.mPending)
    //        return;
    //
    //    if (mQueryCoinState.mRequestCount > 20)
    //    {
    //        mQueryCoinState.mPending = false;
    //        return;
    //    }
    //
    //    const std::uint32_t tickCount = Sexy::GetTickCount();
    //
    //    // 使用有符号差值保持原伪代码行为，同时兼容 tickCount 回绕。
    //    const std::int32_t elapsedTime =
    //        static_cast<std::int32_t>(
    //            tickCount - mQueryCoinState.mLastRequestTime);
    //
    //    if (elapsedTime <= 5000)
    //        return;
    //
    //    mQueryCoinState.mLastRequestTime = tickCount;
    //    ++mQueryCoinState.mRequestCount;
    //
    //    // IDA 给 getCurUser() 套用了错误原型。
    //    // 从实际用途看，这里应当是无参数的静态获取函数。
    //    LawnUser* currentUser = LawnUser::getCurUser();
    //
    //    // 伪代码：
    //    // atoi(*reinterpret_cast<const char**>(
    //    //     reinterpret_cast<char*>(currentUser) + 0x0C))
    //    //
    //    // 该位置很可能是旧版 libstdc++ ABI 下的 std::string 成员，
    //    // 其第一个 DWORD 是字符缓冲区指针。
    //    const char* currentUserIdText =
    //        *reinterpret_cast<const char* const*>(
    //            reinterpret_cast<const std::uint8_t*>(currentUser) + 0x0C);
    //
    //    if (std::atoi(currentUserIdText) != mQueryCoinState.mUserId)
    //    {
    //        mQueryCoinState.mPending = false;
    //        return;
    //    }
    //
    //    LawnApp::SrvQueryCoin(this);
}

void LawnApp::UpdateApp() {
    if (doCheatDialog) {
        if (!IsOnlineServerModeActive() && !gIsReplayMode) {
            DoCheatDialog();
        }
        doCheatDialog = false;
    }
    if (doCheatCodeDialog) {
        if (!isMainMenu && !IsOnlineServerModeActive() && !gIsReplayMode) {
            DoCheatCodeDialog();
        }
        doCheatCodeDialog = false;
    }
    if (doKeyboardTwoPlayerDialog && mTitleScreen == nullptr) {
        LawnMessageBox(Dialogs::DIALOG_MESSAGE, "双人模式已开启", "已经进入双人模式；再次按下切换键即可退出此模式。", "[DIALOG_BUTTON_OK]", "", 3);
        doKeyboardTwoPlayerDialog = false;
    }

    old_LawnApp_UpDateApp(this);
}

void LawnApp::ShowAwardScreen(AwardType theAwardType) {
    old_LawnApp_ShowAwardScreen(this, theAwardType);
}

void LawnApp::KillAwardScreen() {
    old_LawnApp_KillAwardScreen(this);
}

bool LawnApp::CanShopLevel() {
    // 决定是否在当前关卡显示道具栏
    if (disableShop)
        return false;
    if (mGameMode == GameMode::GAMEMODE_MP_VS || IsCoopMode())
        return false;

    return old_LawnApp_CanShopLevel(this);
}

void LawnApp::ShowCreditScreen(bool theIsFromMainMenu) {
    // 用于一周目之后点击"制作人员"按钮播放MV
    mSoundSystem->StopFoley(FoleyType::FOLEY_MENU_LEFT);
    mSoundSystem->StopFoley(FoleyType::FOLEY_MENU_CENTRE);
    mSoundSystem->StopFoley(FoleyType::FOLEY_MENU_RIGHT);
    if (theIsFromMainMenu && HasFinishedAdventure()) {
        theIsFromMainMenu = false;
        KillMainMenu();
        KillNewOptionsDialog();
        KillDialog(DIALOG_HELPOPTIONS);
    }

    old_LawnApp_ShowCreditScreen(this, theIsFromMainMenu);
}

void LawnApp::LoadLevelConfiguration(int a2, int a3) {
    // 如果开启了恢复出怪，则什么都不做，以做到禁止从levels.xml加载出怪。
    if (normalLevel)
        return;

    old_LawnApp_LoadLevelConfiguration(this, a2, a3);
}

void LawnApp::TryHelpTextScreen(HelpTextPage thePage) {
    // 初次进入对战、结盟模式或排行榜时展示帮助提示。
    if (mPlayerInfo) {
        bool &aHelpTextSeen = mPlayerInfo->mHelpTextSeen[thePage];
        if (!aHelpTextSeen) {
            aHelpTextSeen = true;
            mPlayerInfo->SaveDetails();

            ShowHelpTextScreen(thePage);
        }
    }
}

void LawnApp::_constructor() {
    old_LawnApp_LawnApp(this);

    mLawnMouseMode = true; // 开启触控
}

void LawnApp::_destructor() {
    delete addonFonts.BRIANNETOD;
    delete addonFonts.CONTINUUM_BOLD;
    delete addonFonts.DWARVEN_TODCRAFT;
    delete addonFonts.HOUSE_OF_TERROR;
    delete addonFonts.PICO;
    delete addonFonts.JN_BOBO_HEI20;
    delete addonFonts.JN_BOBO_HEI24;
    delete addonFonts.TIEJILI_SC;

    old_LawnApp__destructor(this);
}

void LawnApp::Init() {
    // 试图修复默认加载名为player用户的问题。

    DoParseCmdLine();
    if (!mTodCheatKeys) {
        mOnlyAllowOneCopyToRun = true;
    }

    unk9_2[1] = 0;
    unk9_2[2] = 0;
    mBoardResult = BOARDRESULT_NONE;
    mKilledYetiAndRestarted = false;
    unk9_2[0] = Sexy::GetTickCount() / 1000;
    mPendingRechargeAmount = 0;
    pvzstl::string strings[5];
    getGameInfo(strings, this);
    mGameInfoStrings[0] = strings[0];
    mGameInfoStrings[1] = strings[1];
    mGameInfoStrings[2] = strings[2];
    mGameInfoStrings[3] = strings[3];
    mGameInfoStrings[4] = strings[4];
    //    RpcEngine = DrRpcEngine::getRpcEngine();
    //    pvzstl::string DomainURL;
    //    ServerConfig::getDomainURL(DomainURL);
    //    DrRpcEngine::setDefaultUrl(RpcEngine, DomainURL);
    //    isEncryptionEnabled = ServerConfig::isEncryptionEnabled(RpcEngine);
    //    DrRpcEngine::setDataEncryption(RpcEngine, isEncryptionEnabled);
    //    if ( !LawnSession::Init(unk13_2) )
    //        Sexy::SexyAppBase::DoExit(lawnApp, -1);
    mSessionTaskType = SESSION_TASK_TYPE_LOGIN;
    mLoginToServer = false;
    //    LawnApp::SrvLoginToServer(lawnApp);
    //    PerfTimer aPerfTimer;
    //    Sexy::PerfTimer::PerfTimer(aPerfTimer);
    //    Sexy::PerfTimer::Start((Sexy::PerfTimer *)&v60);

    mProfileMgr->Load();
    if (mProfileMgr->GetAnyProfile() == nullptr) {
        pvzstl::string ipCode = GetLocalIpPlayerCode();
        pvzstl::string defaultName = StrFormat("Player%s", ipCode.c_str());
        mProfileMgr->AddProfile(defaultName);
        mProfileMgr->Save();
        mPlayerInfo = mProfileMgr->GetProfile(defaultName);
    }

    if (mPlayerInfo == nullptr) {
        pvzstl::string value;
        bool readSuccess = RegistryReadString("CurUser", &value);
        if (readSuccess) {
            mPlayerInfo = mProfileMgr->GetProfile(value);
        }
    }

    if (mPlayerInfo == nullptr && mProfileMgr->mNumProfiles > 0) {
        mPlayerInfo = mProfileMgr->GetAnyProfile();
    }

    mMaxExecutions = GetInteger("MaxExecutions", 0);
    mMaxPlays = GetInteger("MaxPlays", 0);
    mMaxTime = GetInteger("MaxTime", 0);
    LoadResourceManifest();
    TodLoadResources("Init");
    mTitleScreen = new TitleScreen(this);

    mTitleScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mTitleScreen);
    mWidgetManager->SetFocus(mTitleScreen);
    mEffectSystem->EffectSystemInitialize();
    //    FilterEffectInitForApp();

    mKonamiCheck = new TypingCheck;
    mKonamiCheck->AddChar('a');
    mKonamiCheck->AddChar('b');
    mKonamiCheck->AddChar('b');
    mKonamiCheck->AddChar('c');
    mKonamiCheck->AddChar('d');
    mKonamiCheck->AddChar('c');
    mKonamiCheck->AddChar('b');
    mKonamiCheck->AddChar('a');
    mMustacheCheck = new TypingCheck("mustache");
    mMoustacheCheck = new TypingCheck("moustache");
    mSuperMowerCheck = new TypingCheck("trickedout");
    mSuperMowerCheck2 = new TypingCheck("tricked out");
    mFutureCheck = new TypingCheck("future");
    mPinataCheck = new TypingCheck("pinata");
    mDanceCheck = new TypingCheck("dance");
    mDaisyCheck = new TypingCheck("daisies");
    mSukhbirCheck = new TypingCheck("sukhbir");

    ReanimatorLoadDefinitions(gLawnReanimationArray, ReanimationType::NUM_REANIMS);

    mIsFullVersion = true;
    Sexy::Graphics::SetTrackingDeviceState(false);
    //    (*(void (**)(int, int *))(*(int *)unkMem6[109] + 172))(unkMem6[109], &unkMem8[1]); // Sexy::IGameCenter::SetListener(int this, Sexy::IGameCenter::Listener *a2)

    mNewIs3DAccelerated = mPlayerInfo == nullptr || !mPlayerInfo->mIs3DAcceleratedClosed;
}

void LawnApp::Load(const char *theGroupName) {
    TodLoadResources(theGroupName);
}

// void LawnApp::DoConvertImitaterImages() {
// for (int i = 0;; ++i) {
// int holder[1];
// int holder1[1];
// int holder2[1];
// StrFormat(holder, "convertImitaterImages/pic%d", i);
// StrFormat(holder1, "ImitaterNormalpic%d.png", i);
// StrFormat(holder2, "ImitaterLesspic%d.png", i);
// Image *imageFromFile = GetImage(reinterpret_cast<string &>(holder), true);
//
// if (imageFromFile == nullptr) {
// break;
// }
// Image *imageImitater = FilterEffectGetImage(imageFromFile, FilterEffect::FILTEREFFECT_WASHED_OUT);
// Image *imageImitaterLess = FilterEffectGetImage(imageFromFile, FilterEffect::FILTEREFFECT_LESS_WASHED_OUT);
// reinterpret_cast<MemoryImage *>(imageImitater)->WriteToPng(holder1);
// reinterpret_cast<MemoryImage *>(imageImitaterLess)->WriteToPng(holder2);
// reinterpret_cast<MemoryImage *>(imageFromFile)->Delete();
// reinterpret_cast<MemoryImage *>(imageImitater)->Delete();
// reinterpret_cast<MemoryImage *>(imageImitaterLess)->Delete();
//
// StringDelete(holder);
// StringDelete(holder1);
// StringDelete(holder2);
// }
// }

void LawnApp::LoadingThreadProc() {
    // 加载新增资源用
    old_LawnApp_LoadingThreadProc(this);

    LoadAddonImages();
    LoadAddonSounds();
    // LawnApp_DoConvertImitaterImages(lawnApp);
    TodStringListLoad("addonFiles/properties/AddonStrings.txt"); // 加载自定义字符串

    // 加载新增 Foley
    TodFoleyInitialize(GetNewLawnFoleyParamArray(), std::size(GetNewLawnFoleyParamArray()));

    // 加载新增 字体文件
    addonFonts.BRIANNETOD = new FreeTypeFont(this, "addonFiles/data/BRIANNETOD.ttf", 16, false, false, false);
    addonFonts.CONTINUUM_BOLD = new FreeTypeFont(this, "addonFiles/data/ContinuumBold.ttf", 16, false, false, false);
    addonFonts.DWARVEN_TODCRAFT = new FreeTypeFont(this, "addonFiles/data/DwarvenTodcraft.ttf", 16, false, false, false);
    addonFonts.HOUSE_OF_TERROR = new FreeTypeFont(this, "addonFiles/data/HouseofTerror.ttf", 16, false, false, false);
    addonFonts.PICO = new FreeTypeFont(this, "addonFiles/data/Pico.ttf", 16, false, false, false);
    addonFonts.JN_BOBO_HEI20 = new FreeTypeFont(this, "addonFiles/data/JNBoBoHei.ttf", 20, false, false, false);
    addonFonts.JN_BOBO_HEI24 = new FreeTypeFont(this, "addonFiles/data/JNBoBoHei.ttf", 24, false, false, false);
    addonFonts.TIEJILI_SC = new FreeTypeFont(this, "addonFiles/data/TiejiliSC.ttf", 16, false, false, false);

    // //试图修复偶现的地图错位现象。不知道是否有效
    // LawnApp_Load(lawnApp,"DelayLoad_Background1");
    // LawnApp_Load(lawnApp,"DelayLoad_BackgroundUnsodded");
    // LawnApp_Load(lawnApp,"DelayLoad_Background2");
    // LawnApp_Load(lawnApp,"DelayLoad_Background3");
    // LawnApp_Load(lawnApp,"DelayLoad_Background4");
    // LawnApp_Load(lawnApp,"DelayLoad_Background5");
    // LawnApp_Load(lawnApp,"DelayLoad_Background6");

    if (showHouse) {
        ReanimatorEnsureDefinitionLoaded(ReanimationType::REANIM_LEADERBOARDS_HOUSE, true);
        mCompletedLoadingThreadTasks += 136;
    }
    // 新增动画预加载
    for (ReanimationType aReanimType = REANIM_ZOMBATAR_HEAD; aReanimType < EXTENDED_NUM_REANIMS; aReanimType = ReanimationType(aReanimType + 1)) {
        ReanimatorEnsureDefinitionLoaded(aReanimType, true);
        mCompletedLoadingThreadTasks += 136;
    }
}

bool LawnApp::IsChallengeWithoutSeedBank() {
    // 黄油爆米花专用
    return mGameMode == GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN || old_LawnApp_IsChallengeWithoutSeedBank(this);
}

int LawnApp::GetSeedsAvailable(bool theIsZombieChooser) {
    // 解锁僵尸方拓展卡片
    bool isExtendedSeedsMode = mPlayerInfo->mVSExtendedSeedsMode;
    if (mVSSetupMenu && mVSSetupMenu->mAddonWidget) {
        isExtendedSeedsMode = mVSSetupMenu->mAddonWidget->mExtendedSeedsMode;
    }
    if (theIsZombieChooser && isExtendedSeedsMode) {
        return NUM_ZOMBIE_SEEDS_IN_CHOOSER;
    }

    return old_LawnApp_GetSeedsAvailable(this, theIsZombieChooser);
}

bool LawnApp::HasSeedType(SeedType theSeedType, bool theIsZombie) {
    if (IsVSMode()) {
        if (Challenge::msVSShuffleMode) {
            return true;
        }
        if (theSeedType < NUM_ZOMBIE_SEED_IN_CHOOSER_VISIBLE) {
            return true;
        }
    }
    return old_LawnApp_HasSeedType(this, theSeedType, theIsZombie);
}

void LawnApp::HardwareInit() {
    old_LawnApp_HardwareInit(this);
    // if (useXboxMusic) {
    // Music2_Delete(lawnApp->mMusic);
    // lawnApp->mMusic = (Music2*) operator new(104u);
    // Music_Music(lawnApp->mMusic); // 使用Music而非Music2
    // }
    delete mSoundSystem;
    mSoundSystem = new TodFoley();
}

int LawnApp::GetNumPreloadingTasks() {
    int oldResult = old_LawnApp_GetNumPreloadingTasks(this);

    int addonReanimsNum = (EXTENDED_NUM_REANIMS - NUM_REANIMS) + (showHouse ? 1 : 0);
    int addonSoundsNum = (sizeof(addonSounds) / sizeof(int));
    int addonImagesNum = (sizeof(AddonImages) / sizeof(Sexy::Image *));

    oldResult += 136 * addonReanimsNum;
    oldResult += 54 * addonSoundsNum;
    oldResult += 9 * addonImagesNum;

    return oldResult;
}

bool LawnApp::GrantAchievement(AchievementType theAchievementId) {
    // 一些非Board的成就在这里处理
    if (!mPlayerInfo->mAchievements[theAchievementId]) {
        PlaySample(addonSounds.achievement);
        // int holder[1];
        // StrFormat(holder,"一二三四五六 成就达成！");
        // ((CustomMessageWidget*)board->mAdvice)->mIcon = GetIconByAchievementId(theAchievementId);
        // Board_DisplayAdviceAgain(board, holder, a::MESSAGE_STYLE_ACHIEVEMENT, AdviceType::ADVICE_NEED_ACHIVEMENT_EARNED);
        // StringDelete(holder);
        mPlayerInfo->mAchievements[theAchievementId] = true;
        return true;
    }

    return false;
}

bool LawnApp::IsNight() {
    // 添加非冒险模式（如：小游戏、花园、智慧树）关卡内进商店的昼夜判定
    if (mBoard != nullptr) {
        return mBoard->StageIsNight();
    }

    if (IsIceDemo() || mPlayerInfo == nullptr)
        return false;

    return (mPlayerInfo->mLevel >= 11 && mPlayerInfo->mLevel <= 20) || (mPlayerInfo->mLevel >= 31 && mPlayerInfo->mLevel <= 40) || mPlayerInfo->mLevel == 50;
}

int LawnApp::TrophiesNeedForGoldSunflower() {
    // 修复新增的小游戏不记入金向日葵达成条件
    int theNumMiniGames = 0;
    for (int i = 0; i < 94; ++i) {
        if (GetChallengeDefinition(i).mPage == ChallengePage::CHALLENGE_PAGE_CHALLENGE) {
            theNumMiniGames++;
        }
    }
    return theNumMiniGames + 18 + 10 - GetNumTrophies(ChallengePage::CHALLENGE_PAGE_SURVIVAL) - GetNumTrophies(ChallengePage::CHALLENGE_PAGE_CHALLENGE)
        - GetNumTrophies(ChallengePage::CHALLENGE_PAGE_PUZZLE);
}

void LawnApp::SetFoleyVolume(FoleyType theFoleyType, double theVolume) const {
    FoleyTypeData *foleyTypeData = &mSoundSystem->mTypeData[theFoleyType];
    for (FoleyInstance &foleyInstance : foleyTypeData->mFoleyInstances) {
        if (foleyInstance.mRefCount != 0) {
            foleyInstance.mInstance->GetVTable()->SetVolume(foleyInstance.mInstance, theVolume);
            //            (*(void (**)(int *, uint32_t, double))(*mInstance + 28))(mInstance, *(uint32_t *)(*mInstance + 28), theVolume);
        }
    }
}

void LawnApp::ShowLeaderboards() {
    gMainMenuLeaderboardsWidget = new LeaderboardsWidget(this);
    mWidgetManager->AddWidget(gMainMenuLeaderboardsWidget);
    mWidgetManager->SetFocus(gMainMenuLeaderboardsWidget);
}

void LawnApp::KillLeaderboards() {
    if (gMainMenuLeaderboardsWidget == nullptr)
        return;

    mWidgetManager->RemoveWidget(gMainMenuLeaderboardsWidget);
    SafeDeleteWidget(gMainMenuLeaderboardsWidget);
    gMainMenuLeaderboardsWidget = nullptr;
}

void LawnApp::ShowZombatarScreen() {
    gMainMenuZombatarWidget = new ZombatarWidget(this);
    // Sexy_Widget_Resize(gMainMenuZombatarWidget,-80,-60,960,720);
    mWidgetManager->AddWidget(gMainMenuZombatarWidget);
    mWidgetManager->SetFocus(gMainMenuZombatarWidget);
}

void LawnApp::KillZombatarScreen() {
    if (gMainMenuZombatarWidget == nullptr)
        return;

    mWidgetManager->RemoveWidget(gMainMenuZombatarWidget);
    SafeDeleteWidget(gMainMenuZombatarWidget);
    gMainMenuZombatarWidget = nullptr;
}


namespace {
char houseControl[6][15] = {"anim_house_1_1", "anim_house_1_1", "anim_house_1_2", "anim_house_1_3", "anim_house_1_4", "anim_house_1_5"};
char housePrefix[5][8] = {"house_1", "house_2", "house_3", "house_4", "house_5"};
} // namespace

void LawnApp::SetHouseReanim(Reanimation *theHouseAnim) {
    if (mPlayerInfo == nullptr)
        return;

    HouseType currentHouseType = mPlayerInfo->mGameStats.mHouseType;
    int currentHouseLevel = mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_BLUEPRINT_BLING + int(currentHouseType)];
    if (currentHouseType == HouseType::BLUEPRINT_BLING) {
        if (CanShowStore()) {
            currentHouseLevel += 3;
        } else if (CanShowAlmanac()) {
            currentHouseLevel += 2;
        } else {
            currentHouseLevel += 1;
        }
    }

    theHouseAnim->PlayReanim(houseControl[currentHouseLevel], ReanimLoopType::REANIM_LOOP, 0, 12.0f);

    for (int i = 0; i < 5; ++i) {
        theHouseAnim->HideTrackByPrefix(housePrefix[i], i != currentHouseType);
    }

    theHouseAnim->HideTrackByPrefix("achievement", true);
}

bool LawnApp::IsIZombieLevel() const {
    if (mBoard == nullptr)
        return false;

    return mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_1 || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_2 || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_3
        || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_4 || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_5 || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_6
        || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_7 || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_8 || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_9
        || mGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS;
}

bool LawnApp::IsWallnutBowlingLevel() const {
    if (mBoard == nullptr)
        return false;

    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_WALLNUT_BOWLING || mGameMode == GameMode::GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2 || mGameMode == GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING)
        return true;

    return IsAdventureMode() && mPlayerInfo->mLevel == 5;
}

bool LawnApp::IsAdventureMode() const {
    return mGameMode == GameMode::GAMEMODE_ADVENTURE;
}

bool LawnApp::IsPuzzleMode() const {
    return mGameMode >= GameMode::GAMEMODE_SCARY_POTTER_1 && mGameMode <= GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS;
}

bool LawnApp::IsSurvivalNormal(GameMode theGameMode) {
    int aLevel = theGameMode - GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
    return aLevel >= 0 && aLevel <= 4;
}

bool LawnApp::IsSurvivalHard(GameMode theGameMode) {
    int aLevel = theGameMode - GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_1;
    return aLevel >= 0 && aLevel <= 4;
}

bool LawnApp::IsSurvivalEndless(GameMode theGameMode) {
    int aLevel = theGameMode - GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_1;
    return aLevel >= 0 && aLevel <= 4;
}

bool LawnApp::IsEndlessScaryPotter(GameMode theGameMode) {
    return theGameMode == GameMode::GAMEMODE_SCARY_POTTER_ENDLESS;
}

bool LawnApp::IsEndlessIZombie(GameMode theGameMode) {
    return theGameMode == GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS;
}

bool LawnApp::IsLittleTroubleLevel() const {
    return (mBoard && (mGameMode == GameMode::GAMEMODE_CHALLENGE_LITTLE_TROUBLE || (mGameMode == GameMode::GAMEMODE_ADVENTURE && mPlayerInfo->mLevel == 25)));
}

bool LawnApp::IsScaryPotterLevel() const {
    if (mGameMode == GameMode::GAMEMODE_SCARY_POTTER_1 || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_2 || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_3
        || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_4 || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_5 || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_6
        || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_7 || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_8 || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_9
        || mGameMode == GameMode::GAMEMODE_SCARY_POTTER_ENDLESS) {
        return true;
    }
    return IsAdventureMode() && mPlayerInfo->mLevel == 35;
}

bool LawnApp::IsSlotMachineLevel() const {
    return (mBoard && mGameMode == GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE);
}

bool LawnApp::IsArtChallenge() const {
    if (mBoard == nullptr)
        return false;

    return mGameMode == GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT || mGameMode == GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER
        || mGameMode == GameMode::GAMEMODE_CHALLENGE_SEEING_STARS;
}

bool LawnApp::IsSquirrelLevel() const {
    return mBoard && mGameMode == GameMode::GAMEMODE_CHALLENGE_SQUIRREL;
}

bool LawnApp::IsWhackAZombieLevel() const {
    if (mBoard == nullptr)
        return false;

    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE)
        return true;

    return IsAdventureMode() && mPlayerInfo->mLevel == 15;
}

bool LawnApp::IsStormyNightLevel() const {
    if (mBoard == nullptr)
        return false;

    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_STORMY_NIGHT)
        return true;

    return IsAdventureMode() && mPlayerInfo->mLevel == 40;
}

bool LawnApp::IsVSMode() const {
    return mGameMode == GameMode::GAMEMODE_MP_VS || mGameMode == GameMode::GAMEMODE_MP_VS_DEBUG || mGameMode == GameMode::GAMEMODE_MP_VS_IN_PAGE;
}

bool LawnApp::IsCoopMode() const {
    return mGameMode >= GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY && mGameMode <= GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS;
}

bool LawnApp::IsTwinSunbankMode() const {
    return IsCoopMode();
}

bool LawnApp::IsMiniBossLevel() const {
    if (mBoard == nullptr)
        return false;

    return (IsAdventureMode() && mPlayerInfo->mLevel == 10) || (IsAdventureMode() && mPlayerInfo->mLevel == 20) || (IsAdventureMode() && mPlayerInfo->mLevel == 30);
}

bool LawnApp::IsFinalBossLevel() const {
    if (mBoard == nullptr)
        return false;

    if (mGameMode == GameMode::GAMEMODE_CHALLENGE_FINAL_BOSS)
        return true;

    return IsAdventureMode() && mPlayerInfo->mLevel == 50;
}

PottedPlant *LawnApp::GetPottedPlantByIndex(int thePottedPlantIndex) const {
    return &mPlayerInfo->mPottedPlants[thePottedPlantIndex];
}

void LawnApp::ShowSeedChooserScreen() {
    mSeedChooserScreen = new SeedChooserScreen(false);
    mSeedChooserScreen->Resize(0, 0, (Sexy::IMAGE_SEEDCHOOSER_BACKGROUND)->mWidth, (Sexy::IMAGE_SEEDCHOOSER_BACKGROUND)->mHeight);
    mWidgetManager->AddWidget(mSeedChooserScreen);
    mWidgetManager->BringToFront(mSeedChooserScreen);
}

void LawnApp::KillSeedChooserScreen() {

    if (mSeedChooserScreen) {
        mWidgetManager->RemoveWidget(mSeedChooserScreen);
        SafeDeleteWidget(mSeedChooserScreen);
        mSeedChooserScreen = nullptr;
    }
}

void LawnApp::ShowZombieChooserScreen() {
    mZombieChooserScreen = new SeedChooserScreen(true);
    mZombieChooserScreen->Resize(800 - (Sexy::IMAGE_SEEDCHOOSER_BACKGROUND2)->mWidth, 0, (Sexy::IMAGE_SEEDCHOOSER_BACKGROUND2)->mWidth, (Sexy::IMAGE_SEEDCHOOSER_BACKGROUND2)->mHeight);
    mWidgetManager->AddWidget(mZombieChooserScreen);
    mWidgetManager->BringToFront(mZombieChooserScreen);
}

void LawnApp::KillZombieChooserScreen() {
    if (mZombieChooserScreen) {
        mWidgetManager->RemoveWidget(mZombieChooserScreen);
        SafeDeleteWidget(mZombieChooserScreen);
        mZombieChooserScreen = nullptr;
    }
}

void LawnApp::ShowChallengeScreen(ChallengePage thePage) { // 创建小游戏界面，并将页码调整至第 thePage 页。
    mGameScene = GameScenes::SCENE_CHALLENGE;
    //    mPlayerInfo->GetId(); // IDA伪C代码，意义不明故注释
    mChallengeScreen = new ChallengeScreen(this, thePage);
    mChallengeScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mChallengeScreen);
    mWidgetManager->BringToBack(mChallengeScreen);
    mWidgetManager->SetFocus(mChallengeScreen);
}

void LawnApp::KillChallengeScreen() {
    if (mChallengeScreen != nullptr) {
        mWidgetManager->RemoveWidget(mChallengeScreen);
        SafeDeleteWidget(mChallengeScreen);
        mChallengeScreen = nullptr;
    }
}

void LawnApp::ShowVSSetupScreen() {
    mVSSetupMenu = new VSSetupMenu();
    mVSSetupMenu->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mVSSetupMenu);
    mWidgetManager->BringToFront(mVSSetupMenu);
    mWidgetManager->SetFocus(mVSSetupMenu);
}

void LawnApp::ShowVSResultsScreen() {
    mVSResultsMenu = new VSResultsMenu();
    mVSResultsMenu->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mVSResultsMenu);
    mWidgetManager->BringToFront(mVSResultsMenu);
    mWidgetManager->SetFocus(mVSResultsMenu);
    if (gIsServerModeNetplay && !mVSResultsMenu->mIsReplaySession && !gIsServerModeSpectator) {
        mVSResultsMenu->mCheckboxController = new VSResultsCheckboxController();
        mVSResultsMenu->mCheckboxController->InitCheckboxWidget(mVSResultsMenu);
    }
    const bool connected = (gTcpConnected || gTcpClientSocket >= 0);
    if (connected) {
        mVSResultsMenu->ShowReplayButton();
    }
}

void LawnApp::KillVSResultsScreen() {
    if (mVSResultsMenu == nullptr) {
        return;
    }
    if (mVSResultsMenu->mCheckboxController != nullptr) {
        mVSResultsMenu->mCheckboxController->DestroyCheckboxWidget();
        delete mVSResultsMenu->mCheckboxController;
        mVSResultsMenu->mCheckboxController = nullptr;
    }
    mVSResultsMenu->KillReplayButton();
    if (!mVSResultsMenu->mIsFading) {
        mWidgetManager->RemoveWidget(mVSResultsMenu);
        SafeDeleteWidget(mVSResultsMenu);
    }
    mVSResultsMenu = nullptr;
}

void LawnApp::LoadingCompleted() {
    mWidgetManager->RemoveWidget(mTitleScreen);
    SafeDeleteWidget(mTitleScreen);
    mTitleScreen = nullptr;
    if (mPlayerInfo) {
        GetVTable()->SetMusicVolume(this, mPlayerInfo->mMusicVolume);
        // 音效音量修复: 真实音量 = 存档值 * 0.65, 以前按存档值直设会导致设置后重启，音效自动变大
        GetVTable()->SetSfxVolume(this, mPlayerInfo->mSoundVolume * 0.65);
    }
    ShowGameSelector();
    mSoundSystem->RehookupSoundWithMusicVolume();
}

bool LawnApp::TryLoadGame() {
    int aId = mPlayerInfo->GetVTable()->GetId(mPlayerInfo);
    int aProfileId = mPlayerInfo->GetVTable()->GetProfileId(mPlayerInfo);
    pvzstl::string name;
    GetSavedGameName(name, mGameMode, aProfileId, aId);
    mMusic->GetVTable()->StopAllMusic(mMusic);
    delete mSaveGame;
    mSaveGame = new SaveGameContext();
    mSaveGame->mFailed = false;
    mSaveGame->mReading = true;
    if (ReadBufferFromFile(name, &mSaveGame->mBuffer, false)) {
        mNeedLoadGame = true;
        mSaveGameOperation = SAVE_GAME_OPERATION_LOAD;
        return true;
    }
    return false;
}

void LawnApp::PreNewGame(GameMode theGameMode, bool theLookForSavedGame) {
    // Best-effort flush queued outbound events before resetting recorder.
    if (gTcpClientSocket >= 0) {
        netplay::FlushSendBuffer(gTcpClientSocket);
    } else if (gTcpConnected && gTcpServerSocket >= 0) {
        netplay::FlushSendBuffer(gTcpServerSocket);
    }
    replay::ResetRecorder();
    old_LawnApp_PreNewGame(this, theGameMode, theLookForSavedGame);
}

void LawnApp::NewGame() {
    mFirstTimeGameSelector = false;

    MakeNewBoard();
    mBoard->InitLevel();
    mBoardResult = BoardResult::BOARDRESULT_NONE;
    mGameScene = GameScenes::SCENE_LEVEL_INTRO;

    if (mGameMode == GameMode::GAMEMODE_MULTI_PLAYER) {
        ShowSeedChooserScreen();
        ShowZombieChooserScreen();
    } else {
        if (IsVSMode()) {
            ShowVSSetupScreen();
            mBoard->mCutScene->StartLevelIntro();
            return;
        }
        ShowSeedChooserScreen();
    }

    mBoard->mCutScene->StartLevelIntro();
}

bool LawnApp::HasBeatenChallenge(GameMode theGameMode) const {
    if (mPlayerInfo == nullptr)
        return false;

    int aChallengeIndex = theGameMode - GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
    if (IsSurvivalNormal(theGameMode)) {
        return mPlayerInfo->mChallengeRecords[aChallengeIndex] >= SURVIVAL_NORMAL_FLAGS;
    }
    if (IsSurvivalHard(theGameMode)) {
        return mPlayerInfo->mChallengeRecords[aChallengeIndex] >= SURVIVAL_HARD_FLAGS;
    }
    if (IsSurvivalEndless(theGameMode) || IsEndlessScaryPotter(theGameMode) || IsEndlessIZombie(theGameMode)) {
        return false;
    }
    // 对战选关页设为未通关以取消绘制已通关奖杯贴图
    if (mChallengeScreen && mChallengeScreen->mPage == ChallengePage::CHALLENGE_PAGE_VS) {
        return false;
    }
    return mPlayerInfo->mChallengeRecords[aChallengeIndex] > 0;
}

static bool zombatarResLoaded;

void LawnApp::LoadZombatarResources() {
    if (zombatarResLoaded)
        return;

    addonZombatarImages.zombatar_main_bg = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_main_bg");
    addonZombatarImages.zombatar_widget_bg = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_widget_bg");
    addonZombatarImages.zombatar_widget_inner_bg = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_widget_inner_bg");
    addonZombatarImages.zombatar_display_window = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_display_window");
    addonZombatarImages.zombatar_mainmenuback_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_mainmenuback_highlight");
    addonZombatarImages.zombatar_finished_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_finished_button");
    addonZombatarImages.zombatar_finished_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_finished_button_highlight");
    addonZombatarImages.zombatar_view_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_view_button");
    addonZombatarImages.zombatar_view_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_view_button_highlight");
    addonZombatarImages.zombatar_skin_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_skin_button");
    addonZombatarImages.zombatar_skin_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_skin_button_highlight");
    addonZombatarImages.zombatar_hair_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_button");
    addonZombatarImages.zombatar_hair_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_button_highlight");
    addonZombatarImages.zombatar_fhair_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_fhair_button");
    addonZombatarImages.zombatar_fhair_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_fhair_button_highlight");
    addonZombatarImages.zombatar_tidbits_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_button");
    addonZombatarImages.zombatar_tidbits_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_button_highlight");
    addonZombatarImages.zombatar_eyewear_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_button");
    addonZombatarImages.zombatar_eyewear_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_button_highlight");
    addonZombatarImages.zombatar_clothes_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_button");
    addonZombatarImages.zombatar_clothes_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_button_highlight");
    addonZombatarImages.zombatar_accessory_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_button");
    addonZombatarImages.zombatar_accessory_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_button_highlight");
    addonZombatarImages.zombatar_hats_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_button");
    addonZombatarImages.zombatar_hats_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_button_highlight");
    addonZombatarImages.zombatar_next_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_next_button");
    addonZombatarImages.zombatar_prev_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_prev_button");
    addonZombatarImages.zombatar_backdrops_button = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_backdrops_button");
    addonZombatarImages.zombatar_backdrops_button_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_backdrops_button_highlight");
    addonZombatarImages.zombatar_background_crazydave = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_crazydave");
    addonZombatarImages.zombatar_background_menu = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_menu");
    addonZombatarImages.zombatar_background_menu_dos = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_menu_dos");
    addonZombatarImages.zombatar_background_roof = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_roof");
    addonZombatarImages.zombatar_background_blank = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_blank");
    addonZombatarImages.zombatar_background_aquarium = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_aquarium");
    addonZombatarImages.zombatar_background_crazydave_night = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_crazydave_night");
    addonZombatarImages.zombatar_background_day_RV = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_day_RV");
    addonZombatarImages.zombatar_background_fog_sunshade = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_fog_sunshade");
    addonZombatarImages.zombatar_background_garden_hd = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_garden_hd");
    addonZombatarImages.zombatar_background_garden_moon = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_garden_moon");
    addonZombatarImages.zombatar_background_garden_mushrooms = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_garden_mushrooms");
    addonZombatarImages.zombatar_background_hood = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_hood");
    addonZombatarImages.zombatar_background_hood_blue = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_hood_blue");
    addonZombatarImages.zombatar_background_hood_brown = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_hood_brown");
    addonZombatarImages.zombatar_background_hood_yellow = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_hood_yellow");
    addonZombatarImages.zombatar_background_mausoleum = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_mausoleum");
    addonZombatarImages.zombatar_background_moon = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_moon");
    addonZombatarImages.zombatar_background_moon_distant = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_moon_distant");
    addonZombatarImages.zombatar_background_night_RV = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_night_RV");
    addonZombatarImages.zombatar_background_pool_sunshade = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_pool_sunshade");
    addonZombatarImages.zombatar_background_roof_distant = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_roof_distant");
    addonZombatarImages.zombatar_background_sky_day = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_sky_day");
    addonZombatarImages.zombatar_background_sky_night = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_sky_night");
    addonZombatarImages.zombatar_background_sky_purple = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_sky_purple");
    addonZombatarImages.zombatar_background_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_7");
    addonZombatarImages.zombatar_background_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_8");
    addonZombatarImages.zombatar_background_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_9");
    addonZombatarImages.zombatar_background_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_10");
    addonZombatarImages.zombatar_background_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_11");
    addonZombatarImages.zombatar_background_11_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_11_1");
    addonZombatarImages.zombatar_background_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_12");
    addonZombatarImages.zombatar_background_12_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_12_1");
    addonZombatarImages.zombatar_background_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_13");
    addonZombatarImages.zombatar_background_13_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_13_1");
    addonZombatarImages.zombatar_background_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_14");
    addonZombatarImages.zombatar_background_14_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_14_1");
    addonZombatarImages.zombatar_background_15 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_15");
    addonZombatarImages.zombatar_background_15_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_15_1");
    addonZombatarImages.zombatar_background_16 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_16");
    addonZombatarImages.zombatar_background_16_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_16_1");
    addonZombatarImages.zombatar_background_17 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_17");
    addonZombatarImages.zombatar_background_17_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_17_1");
    addonZombatarImages.zombatar_background_bej3_bridge_shroom_castles = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_bridge_shroom_castles");
    addonZombatarImages.zombatar_background_bej3_canyon_wall = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_canyon_wall");
    addonZombatarImages.zombatar_background_bej3_crystal_mountain_peak = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_crystal_mountain_peak");
    addonZombatarImages.zombatar_background_bej3_dark_cave_thing = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_dark_cave_thing");
    addonZombatarImages.zombatar_background_bej3_desert_pyramids_sunset = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_desert_pyramids_sunset");
    addonZombatarImages.zombatar_background_bej3_fairy_cave_village = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_fairy_cave_village");
    addonZombatarImages.zombatar_background_bej3_floating_rock_city = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_floating_rock_city");
    addonZombatarImages.zombatar_background_bej3_horse_forset_tree = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_horse_forset_tree");
    addonZombatarImages.zombatar_background_bej3_jungle_ruins_path = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_jungle_ruins_path");
    addonZombatarImages.zombatar_background_bej3_lantern_plants_world = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_lantern_plants_world");
    addonZombatarImages.zombatar_background_bej3_lightning = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_lightning");
    addonZombatarImages.zombatar_background_bej3_lion_tower_cascade = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_lion_tower_cascade");
    addonZombatarImages.zombatar_background_bej3_pointy_ice_path = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_pointy_ice_path");
    addonZombatarImages.zombatar_background_bej3_pointy_ice_path_purple = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_pointy_ice_path_purple");
    addonZombatarImages.zombatar_background_bej3_rock_city_lake = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_rock_city_lake");
    addonZombatarImages.zombatar_background_bej3_snowy_cliffs_castle = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_snowy_cliffs_castle");
    addonZombatarImages.zombatar_background_bej3_treehouse_waterfall = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_treehouse_waterfall");
    addonZombatarImages.zombatar_background_bej3_tube_forest_night = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_tube_forest_night");
    addonZombatarImages.zombatar_background_bej3_water_bubble_city = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_water_bubble_city");
    addonZombatarImages.zombatar_background_bej3_water_fall_cliff = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bej3_water_fall_cliff");
    addonZombatarImages.zombatar_background_bejblitz_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bejblitz_6");
    addonZombatarImages.zombatar_background_bejblitz_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bejblitz_8");
    addonZombatarImages.zombatar_background_bejblitz_main_menu = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_bejblitz_main_menu");
    addonZombatarImages.zombatar_background_peggle_bunches = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_bunches");
    addonZombatarImages.zombatar_background_peggle_fever = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_fever");
    addonZombatarImages.zombatar_background_peggle_level1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_level1");
    addonZombatarImages.zombatar_background_peggle_level4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_level4");
    addonZombatarImages.zombatar_background_peggle_level5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_level5");
    addonZombatarImages.zombatar_background_peggle_menu = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_menu");
    addonZombatarImages.zombatar_background_peggle_nights_bjorn3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_bjorn3");
    addonZombatarImages.zombatar_background_peggle_nights_bjorn4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_bjorn4");
    addonZombatarImages.zombatar_background_peggle_nights_claude5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_claude5");
    addonZombatarImages.zombatar_background_peggle_nights_kalah1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_kalah1");
    addonZombatarImages.zombatar_background_peggle_nights_kalah4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_kalah4");
    addonZombatarImages.zombatar_background_peggle_nights_master5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_master5");
    addonZombatarImages.zombatar_background_peggle_nights_renfield5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_renfield5");
    addonZombatarImages.zombatar_background_peggle_nights_tut5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_tut5");
    addonZombatarImages.zombatar_background_peggle_nights_warren3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_nights_warren3");
    addonZombatarImages.zombatar_background_peggle_paperclips = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_paperclips");
    addonZombatarImages.zombatar_background_peggle_waves = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_background_peggle_waves");
    addonZombatarImages.zombatar_hair_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_1");
    addonZombatarImages.zombatar_hair_1_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_1_mask");
    addonZombatarImages.zombatar_hair_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_2");
    addonZombatarImages.zombatar_hair_2_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_2_mask");
    addonZombatarImages.zombatar_hair_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_3");
    addonZombatarImages.zombatar_hair_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_4");
    addonZombatarImages.zombatar_hair_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_5");
    addonZombatarImages.zombatar_hair_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_6");
    addonZombatarImages.zombatar_hair_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_7");
    addonZombatarImages.zombatar_hair_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_8");
    addonZombatarImages.zombatar_hair_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_9");
    addonZombatarImages.zombatar_hair_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_10");
    addonZombatarImages.zombatar_hair_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_11");
    addonZombatarImages.zombatar_hair_11_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_11_mask");
    addonZombatarImages.zombatar_hair_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_12");
    addonZombatarImages.zombatar_hair_12_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_12_mask");
    addonZombatarImages.zombatar_hair_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_13");
    addonZombatarImages.zombatar_hair_13_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_13_mask");
    addonZombatarImages.zombatar_hair_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_14");
    addonZombatarImages.zombatar_hair_14_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_14_mask");
    addonZombatarImages.zombatar_hair_15 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_15");
    addonZombatarImages.zombatar_hair_15_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_15_mask");
    addonZombatarImages.zombatar_hair_16 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hair_16");
    addonZombatarImages.zombatar_facialhair_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_1");
    addonZombatarImages.zombatar_facialhair_1_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_1_mask");
    addonZombatarImages.zombatar_facialhair_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_2");
    addonZombatarImages.zombatar_facialhair_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_3");
    addonZombatarImages.zombatar_facialhair_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_4");
    addonZombatarImages.zombatar_facialhair_4_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_4_mask");
    addonZombatarImages.zombatar_facialhair_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_5");
    addonZombatarImages.zombatar_facialhair_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_6");
    addonZombatarImages.zombatar_facialhair_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_7");
    addonZombatarImages.zombatar_facialhair_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_8");
    addonZombatarImages.zombatar_facialhair_8_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_8_mask");
    addonZombatarImages.zombatar_facialhair_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_9");
    addonZombatarImages.zombatar_facialhair_9_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_9_mask");
    addonZombatarImages.zombatar_facialhair_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_10");
    addonZombatarImages.zombatar_facialhair_10_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_10_mask");
    addonZombatarImages.zombatar_facialhair_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_11");
    addonZombatarImages.zombatar_facialhair_11_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_11_mask");
    addonZombatarImages.zombatar_facialhair_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_12");
    addonZombatarImages.zombatar_facialhair_12_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_12_mask");
    addonZombatarImages.zombatar_facialhair_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_13");
    addonZombatarImages.zombatar_facialhair_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_14");
    addonZombatarImages.zombatar_facialhair_14_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_14_mask");
    addonZombatarImages.zombatar_facialhair_15 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_15");
    addonZombatarImages.zombatar_facialhair_15_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_15_mask");
    addonZombatarImages.zombatar_facialhair_16 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_16");
    addonZombatarImages.zombatar_facialhair_16_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_16_mask");
    addonZombatarImages.zombatar_facialhair_17 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_17");
    addonZombatarImages.zombatar_facialhair_18 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_18");
    addonZombatarImages.zombatar_facialhair_18_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_18_mask");
    addonZombatarImages.zombatar_facialhair_19 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_19");
    addonZombatarImages.zombatar_facialhair_20 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_20");
    addonZombatarImages.zombatar_facialhair_21 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_21");
    addonZombatarImages.zombatar_facialhair_21_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_21_mask");
    addonZombatarImages.zombatar_facialhair_22 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_22");
    addonZombatarImages.zombatar_facialhair_22_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_22_mask");
    addonZombatarImages.zombatar_facialhair_23 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_23");
    addonZombatarImages.zombatar_facialhair_23_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_23_mask");
    addonZombatarImages.zombatar_facialhair_24 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_24");
    addonZombatarImages.zombatar_facialhair_24_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_facialhair_24_mask");
    addonZombatarImages.zombatar_eyewear_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_1");
    addonZombatarImages.zombatar_eyewear_1_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_1_mask");
    addonZombatarImages.zombatar_eyewear_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_2");
    addonZombatarImages.zombatar_eyewear_2_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_2_mask");
    addonZombatarImages.zombatar_eyewear_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_3");
    addonZombatarImages.zombatar_eyewear_3_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_3_mask");
    addonZombatarImages.zombatar_eyewear_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_4");
    addonZombatarImages.zombatar_eyewear_4_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_4_mask");
    addonZombatarImages.zombatar_eyewear_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_5");
    addonZombatarImages.zombatar_eyewear_5_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_5_mask");
    addonZombatarImages.zombatar_eyewear_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_6");
    addonZombatarImages.zombatar_eyewear_6_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_6_mask");
    addonZombatarImages.zombatar_eyewear_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_7");
    addonZombatarImages.zombatar_eyewear_7_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_7_mask");
    addonZombatarImages.zombatar_eyewear_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_8");
    addonZombatarImages.zombatar_eyewear_8_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_8_mask");
    addonZombatarImages.zombatar_eyewear_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_9");
    addonZombatarImages.zombatar_eyewear_9_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_9_mask");
    addonZombatarImages.zombatar_eyewear_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_10");
    addonZombatarImages.zombatar_eyewear_10_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_10_mask");
    addonZombatarImages.zombatar_eyewear_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_11");
    addonZombatarImages.zombatar_eyewear_11_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_11_mask");
    addonZombatarImages.zombatar_eyewear_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_12");
    addonZombatarImages.zombatar_eyewear_12_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_12_mask");
    addonZombatarImages.zombatar_eyewear_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_13");
    addonZombatarImages.zombatar_eyewear_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_14");
    addonZombatarImages.zombatar_eyewear_15 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_15");
    addonZombatarImages.zombatar_eyewear_16 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_eyewear_16");
    addonZombatarImages.zombatar_accessory_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_1");
    addonZombatarImages.zombatar_accessory_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_2");
    addonZombatarImages.zombatar_accessory_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_3");
    addonZombatarImages.zombatar_accessory_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_4");
    addonZombatarImages.zombatar_accessory_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_5");
    addonZombatarImages.zombatar_accessory_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_6");
    addonZombatarImages.zombatar_accessory_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_8");
    addonZombatarImages.zombatar_accessory_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_9");
    addonZombatarImages.zombatar_accessory_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_10");
    addonZombatarImages.zombatar_accessory_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_11");
    addonZombatarImages.zombatar_accessory_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_12");
    addonZombatarImages.zombatar_accessory_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_13");
    addonZombatarImages.zombatar_accessory_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_14");
    addonZombatarImages.zombatar_accessory_15 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_15");
    addonZombatarImages.zombatar_accessory_16 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_16");
    addonZombatarImages.zombatar_hats_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_1");
    addonZombatarImages.zombatar_hats_1_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_1_mask");
    addonZombatarImages.zombatar_hats_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_2");
    addonZombatarImages.zombatar_hats_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_3");
    addonZombatarImages.zombatar_hats_3_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_3_mask");
    addonZombatarImages.zombatar_hats_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_4");
    addonZombatarImages.zombatar_hats_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_5");
    addonZombatarImages.zombatar_hats_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_6");
    addonZombatarImages.zombatar_hats_6_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_6_mask");
    addonZombatarImages.zombatar_hats_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_7");
    addonZombatarImages.zombatar_hats_7_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_7_mask");
    addonZombatarImages.zombatar_hats_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_8");
    addonZombatarImages.zombatar_hats_8_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_8_mask");
    addonZombatarImages.zombatar_hats_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_9");
    addonZombatarImages.zombatar_hats_9_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_9_mask");
    addonZombatarImages.zombatar_hats_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_10");
    addonZombatarImages.zombatar_hats_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_11");
    addonZombatarImages.zombatar_hats_11_mask = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_11_mask");
    addonZombatarImages.zombatar_hats_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_12");
    addonZombatarImages.zombatar_hats_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_13");
    addonZombatarImages.zombatar_hats_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_hats_14");
    addonZombatarImages.zombatar_tidbits_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_1");
    addonZombatarImages.zombatar_tidbits_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_2");
    addonZombatarImages.zombatar_tidbits_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_3");
    addonZombatarImages.zombatar_tidbits_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_4");
    addonZombatarImages.zombatar_tidbits_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_5");
    addonZombatarImages.zombatar_tidbits_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_6");
    addonZombatarImages.zombatar_tidbits_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_7");
    addonZombatarImages.zombatar_tidbits_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_8");
    addonZombatarImages.zombatar_tidbits_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_9");
    addonZombatarImages.zombatar_tidbits_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_10");
    addonZombatarImages.zombatar_tidbits_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_11");
    addonZombatarImages.zombatar_tidbits_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_12");
    addonZombatarImages.zombatar_tidbits_13 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_13");
    addonZombatarImages.zombatar_tidbits_14 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_tidbits_14");
    addonZombatarImages.zombatar_clothes_1 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_1");
    addonZombatarImages.zombatar_clothes_2 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_2");
    addonZombatarImages.zombatar_clothes_3 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_3");
    addonZombatarImages.zombatar_clothes_4 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_4");
    addonZombatarImages.zombatar_clothes_5 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_5");
    addonZombatarImages.zombatar_clothes_6 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_6");
    addonZombatarImages.zombatar_clothes_7 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_7");
    addonZombatarImages.zombatar_clothes_8 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_8");
    addonZombatarImages.zombatar_clothes_9 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_9");
    addonZombatarImages.zombatar_clothes_10 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_10");
    addonZombatarImages.zombatar_clothes_11 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_11");
    addonZombatarImages.zombatar_clothes_12 = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_clothes_12");
    addonZombatarImages.zombatar_zombie_blank = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_zombie_blank");
    addonZombatarImages.zombatar_zombie_blank_skin = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_zombie_blank_skin");
    int xClip = 130;
    int yClip = 130;
    Sexy::Rect rect = {addonZombatarImages.zombatar_zombie_blank->mWidth - xClip, addonZombatarImages.zombatar_zombie_blank->mHeight - yClip, xClip, yClip};
    addonZombatarImages.zombatar_zombie_blank_part = CopyImage(addonZombatarImages.zombatar_zombie_blank, rect);
    addonZombatarImages.zombatar_zombie_blank_skin_part = CopyImage(addonZombatarImages.zombatar_zombie_blank_skin, rect);
    addonZombatarImages.zombatar_colors_bg = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_colors_bg");
    addonZombatarImages.zombatar_colorpicker = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_colorpicker");
    addonZombatarImages.zombatar_colorpicker_none = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_colorpicker_none");
    addonZombatarImages.zombatar_accessory_bg = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_bg");
    addonZombatarImages.zombatar_accessory_bg_highlight = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_bg_highlight");
    addonZombatarImages.zombatar_accessory_bg_none = GetImageByFileName("addonFiles/images/ZombatarWidget/zombatar_accessory_bg_none");
    zombatarResLoaded = true;

    //    int addonZombatarImagesNum = (sizeof(addonZombatarImages) / sizeof(Sexy::Image *));
    //    for (int i = 0; i < addonZombatarImagesNum; ++i) {
    //        if (*((Sexy::Image **)((char *)&addonZombatarImages + i * sizeof(Sexy::Image *))) == NULL) {
    //            LOG_DEBUG("没成功{}", i);
    //        }
    //    }
}

void LawnApp::MakeNewBoard() {

    KillBoard();
    mBoard = new Board(this);
    mBoard->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mBoard);
    mWidgetManager->BringToBack(mBoard);

    if (GetDialogCount() != 0) {
        mWidgetManager->SetFocus(mBoard);
    }
}
