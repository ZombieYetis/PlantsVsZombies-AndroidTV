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

#ifndef PVZ_GLOBAL_VARIABLE_H
#define PVZ_GLOBAL_VARIABLE_H

#include "PvZ/SexyAppFramework/Misc/GamepadButtons.h"

#include <atomic>
#include <string>

class GameButton;

namespace Sexy {
class Image;
class FreeTypeFont;
} // namespace Sexy

inline uintptr_t gLibGameMainBaseAddr;

inline bool isMainMenu = true;
inline int mBackgroundType = -1; // 当前的场景，白天、黑夜、泳池、雾夜、屋顶、月夜、花园、蘑菇园、水族馆、智慧树
inline bool requestPause;
inline bool isKeyboardTwoPlayerMode;
inline bool doKeyboardTwoPlayerDialog;
inline bool requestDrawShovelInCursor;
inline bool requestDrawButterInCursor; // 2P黄油绘制
inline int VSBackGround;
inline bool seedPacketFastCoolDown; // 选卡冷却开关
inline bool projectilePierce;
inline bool m1PChoosingSeeds = true;
inline bool normalLevel; // 恢复一二周目正常出怪
inline bool useNewShovel;
inline bool useNewCobCannon;
inline bool showHouse;
inline bool imitater;
inline bool positionAutoFix; // 自动光标归位
inline bool useXboxMusic;
inline bool seedBankPin;    // 置顶种子栏
inline bool dynamicPreview; // 动态种植预览
inline bool jumpLogo;       // 跳过加载界面的宝开Logo
inline bool playVideo;
inline bool heavyWeaponAccel;
inline bool disableDeleteUserdata;
inline bool disableSaveUserdata;

inline bool gKeyDown = false;
inline bool gButtonDown = false;
inline bool gButtonDownP1 = false;
inline bool gButtonDownP2 = false;
inline bool gButtonDownSeedChooser = false;
inline bool gButtonDownVSSetup = false;
inline Sexy::GamepadButton gButtonCode = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
inline Sexy::GamepadButton gButtonCodeP1 = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
inline Sexy::GamepadButton gButtonCodeP2 = Sexy::GamepadButton::GAMEPAD_BUTTON_NONE;
inline int gGamePlayerIndex = -1;
inline int gGamepad1ToPlayerIndex = -1;

inline int speedUpMode;
inline int speedUpCounter;

inline std::atomic_bool gHasInputContent;
inline std::string gInputString;

// 重型武器角度设定
inline float angle1 = 0;
inline float angle2 = 1;

inline float gGamepadP1VelX;
inline float gGamepadP1VelY;
inline float gGamepadP2VelX;
inline float gGamepadP2VelY;


enum TouchPlayerIndex {
    TOUCHPLAYER_NONE = -1,
    TOUCHPLAYER_PLAYER1 = 0,
    TOUCHPLAYER_PLAYER2 = 1,
};

inline TouchPlayerIndex gPlayerIndex = TouchPlayerIndex::TOUCHPLAYER_NONE;
inline TouchPlayerIndex gPlayerIndexSecond = TouchPlayerIndex::TOUCHPLAYER_NONE;

inline bool gKeyboardMode;

inline class LeaderboardsWidget *gMainMenuLeaderboardsWidget;
inline class ZombatarWidget *gMainMenuZombatarWidget;

inline int xx, yy, xw, yh;
inline int xx1, yy1, xw1, yh1;

inline bool enableNewOptionsDialog = false;

enum class ServerModeTransport {
    NONE = 0,
    RELAY = 1,
    P2P = 2,
};

inline ServerModeTransport gServerModeTransport = ServerModeTransport::NONE;
inline bool gIsServerModeSpectator = false;
inline bool gIsReplayMode = false;
inline bool gReplayPauseByMenu = false;

struct AddonImages {
    Sexy::Image *pole_night;
    Sexy::Image *trees_night;
    Sexy::Image *googlyeye;
    Sexy::Image *squirrel;
    Sexy::Image *stripe_day_coop;
    Sexy::Image *stripe_pool_coop;
    Sexy::Image *stripe_roof_left;
    Sexy::Image *butter_glove;
    Sexy::Image *custom_cobcannon;
    Sexy::Image *hood1_house;
    Sexy::Image *hood2_house;
    Sexy::Image *hood3_house;
    Sexy::Image *hood4_house;
    Sexy::Image *house_hill_house;
    Sexy::Image *achievement_homeLawnsecurity;
    Sexy::Image *achievement_chomp;
    Sexy::Image *achievement_closeshave;
    Sexy::Image *achievement_coop;
    Sexy::Image *achievement_explodonator;
    Sexy::Image *achievement_garg;
    Sexy::Image *achievement_immortal;
    Sexy::Image *achievement_morticulturalist;
    Sexy::Image *achievement_shop;
    Sexy::Image *achievement_soilplants;
    Sexy::Image *achievement_tree;
    Sexy::Image *achievement_versusz;
    Sexy::Image *hole;
    Sexy::Image *hole_bjorn;
    Sexy::Image *hole_china;
    Sexy::Image *hole_gems;
    Sexy::Image *hole_chuzzle;
    Sexy::Image *hole_heavyrocks;
    Sexy::Image *hole_duwei;
    Sexy::Image *hole_pipe;
    Sexy::Image *hole_tiki;
    Sexy::Image *hole_worm;
    Sexy::Image *hole_top;
    Sexy::Image *plant_can;
    Sexy::Image *zombie_can;
    Sexy::Image *plant_pile01_stack01;
    Sexy::Image *plant_pile01_stack02;
    Sexy::Image *plant_pile02_stack01;
    Sexy::Image *plant_pile02_stack02;
    Sexy::Image *plant_pile03_stack01;
    Sexy::Image *plant_pile03_stack02;
    Sexy::Image *zombie_pile01_stack01;
    Sexy::Image *zombie_pile01_stack02;
    Sexy::Image *zombie_pile01_stack03;
    Sexy::Image *zombie_pile02_stack01;
    Sexy::Image *zombie_pile02_stack02;
    Sexy::Image *zombie_pile02_stack03;
    Sexy::Image *zombie_pile03_stack01;
    Sexy::Image *zombie_pile03_stack02;
    Sexy::Image *zombie_pile03_stack03;
    Sexy::Image *survival_button;
    Sexy::Image *leaderboards;
    Sexy::Image *SelectorScreen_WoodSign3;
    Sexy::Image *SelectorScreen_WoodSign3_press;
    Sexy::Image *zombatar_portrait;
    Sexy::Image *gamerpic;
    Sexy::Image *crater_night_roof_center;
    Sexy::Image *crater_night_roof_left;
    Sexy::Image *leaderboard_selector;
    Sexy::Image *zombie_duckytube_inwater;
    Sexy::Image *burial_mound;
    Sexy::Image *burial_mound_dirt;
    Sexy::Image *seed_mounds;
    Sexy::Image *seedpacket_Zombie_Upgrade;
    Sexy::Image *VS_Button;
    Sexy::Image *VS_Button_selected;
    Sexy::Image *zombie_target;

    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_HELMET3;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_FOOTBALL_LEFTARM_HAND;
    Sexy::Image *IMAGE_SUPERFAN_ZOMBIEIMPHEAD;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SUPER_FAN_IMP_OUTARM_GLOVE;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_IMP_ARM1_BONE;
    Sexy::Image *IMAGE_ZOMBIEJACKSONHEAD;
    Sexy::Image *IMAGE_ZOMBIEBACKUPDANCERHEAD;
    Sexy::Image *IMAGE_GIGA_ZOMBIEPOLEVAULTERHEAD;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_EXPLORER_HEAD;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_DOGWALKER_HEAD;
    Sexy::Image *IMAGE_ZOMBIE_TELEPORTATION_HEAD;
    Sexy::Image *IMAGE_ZOMBIE_CROSSING_GUARD_ARM;
    Sexy::Image *IMAGE_ZOMBIE_CROSSING_GUARD_HEAD;
    Sexy::Image *IMAGE_ZOMBIE_SCIENTIST_HEAD;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_CROSSING_GUARD_HAT;
    Sexy::Image *IMAGE_PROJECTILEPOLE;
    Sexy::Image *IMAGE_PROJECTILEZOMBLOB;
    Sexy::Image *IMAGE_PROJECTILESPORE;
    Sexy::Image *IMAGE_PROJECTILEBOOMERANG;
    Sexy::Image *IMAGE_PROJECTILETELEPORTATION;
    Sexy::Image *IMAGE_PROJECTILEACKEE;
    Sexy::Image *IMAGE_REANIM_ZOMBLOBHEAD_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY_DYING_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY2_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_BODY3_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_HEAD_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_HEAD2_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_JAW_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_UPPER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_UPPER2_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER2_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_LOWER3_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND2_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERARM_HAND3_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_UPPER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_LOWER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_INNERLEG_FOOT_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_UPPER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_LOWER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_HAND_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERARM_HAND2_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_UPPER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_LOWER_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_ZOMBLOB_OUTERLEG_FOOT_BUTTERED;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_BACKUP_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_JACKSON_OUTERARM_HAND;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_DANCER_INNERARM_HAND;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_PAPER3;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_LEFTARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SUNDAY_EDITION_LEFTARM_LOWER;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_EXPLORER_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_BODY1_2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_OUTERARM_LOWER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_BODY1_3;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_FOOT2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_HEAD2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_GIGA_GARGANTUAR_TELEPHONEPOLE_COIL;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_DOGWALKER_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_TELEPORTATION_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_TELEPORTATION_TELEPHONE;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_CROSSING_GUARD_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SCIENTIST_OUTERARM_UPPER2;
    Sexy::Image *IMAGE_REANIM_ZOMBIE_SCIENTIST_HAND;
    Sexy::Image *IMAGE_REANIM_CELERY_STALKER_ARM2_LOWER2;
    Sexy::Image *IMAGE_REANIM_CELERY_STALKER_ARM2_LOWER3;
    Sexy::Image *IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER2;
    Sexy::Image *IMAGE_REANIM_CELERY_STALKER_ARM2_UPPER3;
    Sexy::Image *IMAGE_REANIM_SWEET_POTATO_BODY2;
    Sexy::Image *IMAGE_REANIM_SWEET_POTATO_BODY3;
    Sexy::Image *IMAGE_REANIM_SWEET_POTATO_MOUTH2;
    Sexy::Image *IMAGE_REANIM_SWEET_POTATO_MOUTH3;
    Sexy::Image *IMAGE_REANIM_SWEET_POTATO_EYE3;
    Sexy::Image *IMAGE_REANIM_PEANUT_HEAD1_2;
    Sexy::Image *IMAGE_REANIM_PEANUT_HEAD2_2;
    Sexy::Image *IMAGE_REANIM_ENDURIAN_BODY2;
    Sexy::Image *IMAGE_REANIM_ENDURIAN_BODY3;
    Sexy::Image *IMAGE_REANIM_ENDURIAN_BODY_UPPER2;
    Sexy::Image *IMAGE_REANIM_ENDURIAN_EYE2;
    Sexy::Image *IMAGE_REANIM_ENDURIAN_STEM2;
    Sexy::Image *IMAGE_REANIM_ICE1;
    Sexy::Image *IMAGE_REANIM_ICE2;
    Sexy::Image *IMAGE_REANIM_ICE3;
    Sexy::Image *IMAGE_REANIM_ICE4;
    Sexy::Image *IMAGE_SHOVELBANK_VERTICAL;
    Sexy::Image *IMAGE_SHOVEL_VERTICAL;
} inline addonImages; // 此处是新增贴图的列表；

struct AddonSounds {
    int achievement;
    int thriller;
    int allstardbl;
    int whistle;
    int explorer;
    int zomblob;
    int iceberg;
    int celery_stalker_rise;
    int celery_stalker_attack;
    int bloomerang;
    int bonk_choy_punch;
    int bonk_choy_uppercut;
    int power_pole_charge;
    int power_pole_core;
    int power_pole_hifi;
    int power_pole_tail;
    int power_pole_width;
    int giga_laugh;
    int giga_laugh2;
    int giga_laugh3;
    int teleportation;
} inline addonSounds; // 此处是新增音频的列表。注意新增的音频数量有限制，最多新增62个。（最大总音频256个，原版游戏已经使用了194个，还剩62个空余。未来我也许会拓宽此限制。）

struct AddonZombatarImages {
    Sexy::Image *zombatar_main_bg;
    Sexy::Image *zombatar_widget_bg;
    Sexy::Image *zombatar_widget_inner_bg;
    Sexy::Image *zombatar_background_crazydave;
    Sexy::Image *zombatar_background_menu;
    Sexy::Image *zombatar_background_menu_dos;
    Sexy::Image *zombatar_background_roof;
    Sexy::Image *zombatar_background_blank;
    Sexy::Image *zombatar_background_aquarium;
    Sexy::Image *zombatar_background_crazydave_night;
    Sexy::Image *zombatar_background_day_RV;
    Sexy::Image *zombatar_background_fog_sunshade;
    Sexy::Image *zombatar_background_garden_hd;
    Sexy::Image *zombatar_background_garden_moon;
    Sexy::Image *zombatar_background_garden_mushrooms;
    Sexy::Image *zombatar_background_hood;
    Sexy::Image *zombatar_background_hood_blue;
    Sexy::Image *zombatar_background_hood_brown;
    Sexy::Image *zombatar_background_hood_yellow;
    Sexy::Image *zombatar_background_mausoleum;
    Sexy::Image *zombatar_background_moon;
    Sexy::Image *zombatar_background_moon_distant;
    Sexy::Image *zombatar_background_night_RV;
    Sexy::Image *zombatar_background_pool_sunshade;
    Sexy::Image *zombatar_background_roof_distant;
    Sexy::Image *zombatar_background_sky_day;
    Sexy::Image *zombatar_background_sky_night;
    Sexy::Image *zombatar_background_sky_purple;
    Sexy::Image *zombatar_background_7;
    Sexy::Image *zombatar_background_8;
    Sexy::Image *zombatar_background_9;
    Sexy::Image *zombatar_background_10;
    Sexy::Image *zombatar_background_11;
    Sexy::Image *zombatar_background_11_1;
    Sexy::Image *zombatar_background_12;
    Sexy::Image *zombatar_background_12_1;
    Sexy::Image *zombatar_background_13;
    Sexy::Image *zombatar_background_13_1;
    Sexy::Image *zombatar_background_14;
    Sexy::Image *zombatar_background_14_1;
    Sexy::Image *zombatar_background_15;
    Sexy::Image *zombatar_background_15_1;
    Sexy::Image *zombatar_background_16;
    Sexy::Image *zombatar_background_16_1;
    Sexy::Image *zombatar_background_17;
    Sexy::Image *zombatar_background_17_1;
    Sexy::Image *zombatar_background_bej3_bridge_shroom_castles;
    Sexy::Image *zombatar_background_bej3_canyon_wall;
    Sexy::Image *zombatar_background_bej3_crystal_mountain_peak;
    Sexy::Image *zombatar_background_bej3_dark_cave_thing;
    Sexy::Image *zombatar_background_bej3_desert_pyramids_sunset;
    Sexy::Image *zombatar_background_bej3_fairy_cave_village;
    Sexy::Image *zombatar_background_bej3_floating_rock_city;
    Sexy::Image *zombatar_background_bej3_horse_forset_tree;
    Sexy::Image *zombatar_background_bej3_jungle_ruins_path;
    Sexy::Image *zombatar_background_bej3_lantern_plants_world;
    Sexy::Image *zombatar_background_bej3_lightning;
    Sexy::Image *zombatar_background_bej3_lion_tower_cascade;
    Sexy::Image *zombatar_background_bej3_pointy_ice_path;
    Sexy::Image *zombatar_background_bej3_pointy_ice_path_purple;
    Sexy::Image *zombatar_background_bej3_rock_city_lake;
    Sexy::Image *zombatar_background_bej3_snowy_cliffs_castle;
    Sexy::Image *zombatar_background_bej3_treehouse_waterfall;
    Sexy::Image *zombatar_background_bej3_tube_forest_night;
    Sexy::Image *zombatar_background_bej3_water_bubble_city;
    Sexy::Image *zombatar_background_bej3_water_fall_cliff;
    Sexy::Image *zombatar_background_bejblitz_6;
    Sexy::Image *zombatar_background_bejblitz_8;
    Sexy::Image *zombatar_background_bejblitz_main_menu;
    Sexy::Image *zombatar_background_peggle_bunches;
    Sexy::Image *zombatar_background_peggle_fever;
    Sexy::Image *zombatar_background_peggle_level1;
    Sexy::Image *zombatar_background_peggle_level4;
    Sexy::Image *zombatar_background_peggle_level5;
    Sexy::Image *zombatar_background_peggle_menu;
    Sexy::Image *zombatar_background_peggle_nights_bjorn3;
    Sexy::Image *zombatar_background_peggle_nights_bjorn4;
    Sexy::Image *zombatar_background_peggle_nights_claude5;
    Sexy::Image *zombatar_background_peggle_nights_kalah1;
    Sexy::Image *zombatar_background_peggle_nights_kalah4;
    Sexy::Image *zombatar_background_peggle_nights_master5;
    Sexy::Image *zombatar_background_peggle_nights_renfield5;
    Sexy::Image *zombatar_background_peggle_nights_tut5;
    Sexy::Image *zombatar_background_peggle_nights_warren3;
    Sexy::Image *zombatar_background_peggle_paperclips;
    Sexy::Image *zombatar_background_peggle_waves;
    Sexy::Image *zombatar_hair_1;
    Sexy::Image *zombatar_hair_1_mask;
    Sexy::Image *zombatar_hair_2;
    Sexy::Image *zombatar_hair_2_mask;
    Sexy::Image *zombatar_hair_3;
    Sexy::Image *zombatar_hair_4;
    Sexy::Image *zombatar_hair_5;
    Sexy::Image *zombatar_hair_6;
    Sexy::Image *zombatar_hair_7;
    Sexy::Image *zombatar_hair_8;
    Sexy::Image *zombatar_hair_9;
    Sexy::Image *zombatar_hair_10;
    Sexy::Image *zombatar_hair_11;
    Sexy::Image *zombatar_hair_11_mask;
    Sexy::Image *zombatar_hair_12;
    Sexy::Image *zombatar_hair_12_mask;
    Sexy::Image *zombatar_hair_13;
    Sexy::Image *zombatar_hair_13_mask;
    Sexy::Image *zombatar_hair_14;
    Sexy::Image *zombatar_hair_14_mask;
    Sexy::Image *zombatar_hair_15;
    Sexy::Image *zombatar_hair_15_mask;
    Sexy::Image *zombatar_hair_16;
    Sexy::Image *zombatar_facialhair_1;
    Sexy::Image *zombatar_facialhair_1_mask;
    Sexy::Image *zombatar_facialhair_2;
    Sexy::Image *zombatar_facialhair_3;
    Sexy::Image *zombatar_facialhair_4;
    Sexy::Image *zombatar_facialhair_4_mask;
    Sexy::Image *zombatar_facialhair_5;
    Sexy::Image *zombatar_facialhair_6;
    Sexy::Image *zombatar_facialhair_7;
    Sexy::Image *zombatar_facialhair_8;
    Sexy::Image *zombatar_facialhair_8_mask;
    Sexy::Image *zombatar_facialhair_9;
    Sexy::Image *zombatar_facialhair_9_mask;
    Sexy::Image *zombatar_facialhair_10;
    Sexy::Image *zombatar_facialhair_10_mask;
    Sexy::Image *zombatar_facialhair_11;
    Sexy::Image *zombatar_facialhair_11_mask;
    Sexy::Image *zombatar_facialhair_12;
    Sexy::Image *zombatar_facialhair_12_mask;
    Sexy::Image *zombatar_facialhair_13;
    Sexy::Image *zombatar_facialhair_14;
    Sexy::Image *zombatar_facialhair_14_mask;
    Sexy::Image *zombatar_facialhair_15;
    Sexy::Image *zombatar_facialhair_15_mask;
    Sexy::Image *zombatar_facialhair_16;
    Sexy::Image *zombatar_facialhair_16_mask;
    Sexy::Image *zombatar_facialhair_17;
    Sexy::Image *zombatar_facialhair_18;
    Sexy::Image *zombatar_facialhair_18_mask;
    Sexy::Image *zombatar_facialhair_19;
    Sexy::Image *zombatar_facialhair_20;
    Sexy::Image *zombatar_facialhair_21;
    Sexy::Image *zombatar_facialhair_21_mask;
    Sexy::Image *zombatar_facialhair_22;
    Sexy::Image *zombatar_facialhair_22_mask;
    Sexy::Image *zombatar_facialhair_23;
    Sexy::Image *zombatar_facialhair_23_mask;
    Sexy::Image *zombatar_facialhair_24;
    Sexy::Image *zombatar_facialhair_24_mask;
    Sexy::Image *zombatar_eyewear_1;
    Sexy::Image *zombatar_eyewear_1_mask;
    Sexy::Image *zombatar_eyewear_2;
    Sexy::Image *zombatar_eyewear_2_mask;
    Sexy::Image *zombatar_eyewear_3;
    Sexy::Image *zombatar_eyewear_3_mask;
    Sexy::Image *zombatar_eyewear_4;
    Sexy::Image *zombatar_eyewear_4_mask;
    Sexy::Image *zombatar_eyewear_5;
    Sexy::Image *zombatar_eyewear_5_mask;
    Sexy::Image *zombatar_eyewear_6;
    Sexy::Image *zombatar_eyewear_6_mask;
    Sexy::Image *zombatar_eyewear_7;
    Sexy::Image *zombatar_eyewear_7_mask;
    Sexy::Image *zombatar_eyewear_8;
    Sexy::Image *zombatar_eyewear_8_mask;
    Sexy::Image *zombatar_eyewear_9;
    Sexy::Image *zombatar_eyewear_9_mask;
    Sexy::Image *zombatar_eyewear_10;
    Sexy::Image *zombatar_eyewear_10_mask;
    Sexy::Image *zombatar_eyewear_11;
    Sexy::Image *zombatar_eyewear_11_mask;
    Sexy::Image *zombatar_eyewear_12;
    Sexy::Image *zombatar_eyewear_12_mask;
    Sexy::Image *zombatar_eyewear_13;
    Sexy::Image *zombatar_eyewear_14;
    Sexy::Image *zombatar_eyewear_15;
    Sexy::Image *zombatar_eyewear_16;
    Sexy::Image *zombatar_accessory_1;
    Sexy::Image *zombatar_accessory_2;
    Sexy::Image *zombatar_accessory_3;
    Sexy::Image *zombatar_accessory_4;
    Sexy::Image *zombatar_accessory_5;
    Sexy::Image *zombatar_accessory_6;
    Sexy::Image *zombatar_accessory_8;
    Sexy::Image *zombatar_accessory_9;
    Sexy::Image *zombatar_accessory_10;
    Sexy::Image *zombatar_accessory_11;
    Sexy::Image *zombatar_accessory_12;
    Sexy::Image *zombatar_accessory_13;
    Sexy::Image *zombatar_accessory_14;
    Sexy::Image *zombatar_accessory_15;
    Sexy::Image *zombatar_accessory_16;
    Sexy::Image *zombatar_hats_1;
    Sexy::Image *zombatar_hats_1_mask;
    Sexy::Image *zombatar_hats_2;
    Sexy::Image *zombatar_hats_3;
    Sexy::Image *zombatar_hats_3_mask;
    Sexy::Image *zombatar_hats_4;
    Sexy::Image *zombatar_hats_5;
    Sexy::Image *zombatar_hats_6;
    Sexy::Image *zombatar_hats_6_mask;
    Sexy::Image *zombatar_hats_7;
    Sexy::Image *zombatar_hats_7_mask;
    Sexy::Image *zombatar_hats_8;
    Sexy::Image *zombatar_hats_8_mask;
    Sexy::Image *zombatar_hats_9;
    Sexy::Image *zombatar_hats_9_mask;
    Sexy::Image *zombatar_hats_10;
    Sexy::Image *zombatar_hats_11;
    Sexy::Image *zombatar_hats_11_mask;
    Sexy::Image *zombatar_hats_12;
    Sexy::Image *zombatar_hats_13;
    Sexy::Image *zombatar_hats_14;
    Sexy::Image *zombatar_tidbits_1;
    Sexy::Image *zombatar_tidbits_2;
    Sexy::Image *zombatar_tidbits_3;
    Sexy::Image *zombatar_tidbits_4;
    Sexy::Image *zombatar_tidbits_5;
    Sexy::Image *zombatar_tidbits_6;
    Sexy::Image *zombatar_tidbits_7;
    Sexy::Image *zombatar_tidbits_8;
    Sexy::Image *zombatar_tidbits_9;
    Sexy::Image *zombatar_tidbits_10;
    Sexy::Image *zombatar_tidbits_11;
    Sexy::Image *zombatar_tidbits_12;
    Sexy::Image *zombatar_tidbits_13;
    Sexy::Image *zombatar_tidbits_14;
    Sexy::Image *zombatar_clothes_1;
    Sexy::Image *zombatar_clothes_2;
    Sexy::Image *zombatar_clothes_3;
    Sexy::Image *zombatar_clothes_4;
    Sexy::Image *zombatar_clothes_5;
    Sexy::Image *zombatar_clothes_6;
    Sexy::Image *zombatar_clothes_7;
    Sexy::Image *zombatar_clothes_8;
    Sexy::Image *zombatar_clothes_9;
    Sexy::Image *zombatar_clothes_10;
    Sexy::Image *zombatar_clothes_11;
    Sexy::Image *zombatar_clothes_12;
    Sexy::Image *zombatar_zombie_blank;
    Sexy::Image *zombatar_zombie_blank_skin;
    Sexy::Image *zombatar_zombie_blank_part;
    Sexy::Image *zombatar_zombie_blank_skin_part;
    Sexy::Image *zombatar_display_window;
    Sexy::Image *zombatar_mainmenuback_highlight;
    Sexy::Image *zombatar_finished_button;
    Sexy::Image *zombatar_finished_button_highlight;
    Sexy::Image *zombatar_view_button;
    Sexy::Image *zombatar_view_button_highlight;
    Sexy::Image *zombatar_colors_bg;
    Sexy::Image *zombatar_skin_button;
    Sexy::Image *zombatar_skin_button_highlight;
    Sexy::Image *zombatar_hair_button;
    Sexy::Image *zombatar_hair_button_highlight;
    Sexy::Image *zombatar_fhair_button;
    Sexy::Image *zombatar_fhair_button_highlight;
    Sexy::Image *zombatar_tidbits_button;
    Sexy::Image *zombatar_tidbits_button_highlight;
    Sexy::Image *zombatar_eyewear_button;
    Sexy::Image *zombatar_eyewear_button_highlight;
    Sexy::Image *zombatar_clothes_button;
    Sexy::Image *zombatar_clothes_button_highlight;
    Sexy::Image *zombatar_accessory_button;
    Sexy::Image *zombatar_accessory_button_highlight;
    Sexy::Image *zombatar_hats_button;
    Sexy::Image *zombatar_hats_button_highlight;
    Sexy::Image *zombatar_next_button;
    Sexy::Image *zombatar_prev_button;
    Sexy::Image *zombatar_backdrops_button;
    Sexy::Image *zombatar_backdrops_button_highlight;
    Sexy::Image *zombatar_colorpicker;
    Sexy::Image *zombatar_colorpicker_none;
    Sexy::Image *zombatar_accessory_bg;
    Sexy::Image *zombatar_accessory_bg_highlight;
    Sexy::Image *zombatar_accessory_bg_none;
} inline addonZombatarImages; // 此处是新增贴图的列表；

struct AddonFonts {
    Sexy::FreeTypeFont *BRIANNETOD;
    Sexy::FreeTypeFont *CONTINUUM_BOLD;
    Sexy::FreeTypeFont *DWARVEN_TODCRAFT;
    Sexy::FreeTypeFont *HOUSE_OF_TERROR;
    Sexy::FreeTypeFont *PICO;
    // 以下字体含中文
    Sexy::FreeTypeFont *JN_BOBO_HEI20;
    Sexy::FreeTypeFont *JN_BOBO_HEI24;
    Sexy::FreeTypeFont *JN_BOBO_HEI36;
    Sexy::FreeTypeFont *TIEJILI_SC;
} inline addonFonts;

#endif // PVZ_GLOBAL_VARIABLE_H
