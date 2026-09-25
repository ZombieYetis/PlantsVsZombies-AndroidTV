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

#ifndef PVZ_LAWN_WIDGET_VS_SETUP_MENU_H
#define PVZ_LAWN_WIDGET_VS_SETUP_MENU_H

#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/MenuWidget.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"
#include "PvZ/Symbols.h"

inline constexpr int MAX_SEEDS_IN_POOL = 9;

enum VSSetupMode {
    VS_SETUP_MODE_QUICK_PLAY = 0,    // 快速游戏
    VS_SETUP_MODE_CUSTOM_BATTLE = 1, // 自定义战场
    VS_SETUP_MODE_RANDOM_BATTLE = 2, // 随机战场
};

enum VSSide {
    VS_SIDE_NONE = -1,  // 未分配阵营
    VS_SIDE_PLANT = 0,  // 植物方
    VS_SIDE_ZOMBIE = 1, // 僵尸方
};

namespace Sexy {
class ButtonWidget;
class DefaultPlayerInfo;
} // namespace Sexy

class LawnApp;
class Board;


class VSSetupMenu : public Sexy::MenuWidget {
public:
    enum {
        VSSetupMenu_Quick_Play = 9,     // 快速游戏
        VSSetupMenu_Custom_Battle = 10, // 自定义战场
        VSSetupMenu_Random_Battle = 11, // 随机战场
        VSSetupMenu_Enter = 1000,
        VSSetupMenu_Back = 1001,
    };

    enum WidgetIds {
        BACKGROUND_FRAME,
        PLANT_SIDE,
        PLANT_SIDE_MID,
        PLANT_SIDE_FRONT,
        ZOMBIE_SIDE,
        ZOMBIE_SIDE_MID,
        ZOMBIE_SIDE_FRONT,
        CONTROLLER_0,
        CONTROLLER_1,
        QUICK_BUTTON,
        CUSTOM_BUTTON,
        RANDOM_BUTTON
    };

    enum VSSetupState {
        VS_SETUP_STATE_CONTROLLERS = 0,   // WaitForSecondPlayerDialog
        VS_SETUP_STATE_SIDES = 1,         // 未分配手柄阵营
        VS_SETUP_STATE_SELECT_BATTLE = 2, // 已分配手柄阵营
        VS_SETUP_STATE_CUSTOM_BATTLE = 3, // 自定义战场选卡中
    };

    static constexpr SeedType msQuickPlayDecks[2][6] = {
        // DeckPlant
        {SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_WALLNUT, SEED_POTATOMINE, SEED_JALAPENO, SEED_SQUASH},
        // DeckZombie
        {SEED_ZOMBIE_GRAVESTONE, SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_FLAG},
    };
    static constexpr SeedType msRandomPools[9][8] = {
        // plant pools - normal
        {SEED_PEASHOOTER, SEED_REPEATER, SEED_CABBAGEPULT, SEED_KERNELPULT, SEED_REPEATER, SEED_SNOWPEA, SEED_NONE, SEED_PEASHOOTER},
        {SEED_WALLNUT, SEED_CHERRYBOMB, SEED_POTATOMINE, SEED_JALAPENO, SEED_SQUASH, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER},
        {SEED_CHOMPER, SEED_GRAVEBUSTER, SEED_THREEPEATER, SEED_SPIKEWEED, SEED_STARFRUIT, SEED_MELONPULT, SEED_GARLIC, SEED_NONE},
        // plant pools - has coffee
        {SEED_PUFFSHROOM, SEED_FUMESHROOM, SEED_SCAREDYSHROOM, SEED_PEASHOOTER, /*SEED_GRAVEBUSTER*/ SEED_NONE, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER},
        {SEED_WALLNUT, SEED_HYPNOSHROOM, SEED_ICESHROOM, SEED_DOOMSHROOM, SEED_JALAPENO, SEED_SQUASH, SEED_NONE, SEED_PEASHOOTER},
        {SEED_GRAVEBUSTER, SEED_SPIKEWEED, SEED_GARLIC, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER},
        // zombie pools
        {SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_LADDER, SEED_ZOMBIE_NEWSPAPER, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER},
        {SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_PAIL, SEED_ZOMBIE_DANCER, SEED_ZOMBIE_POLEVAULTER, SEED_ZOMBIE_FOOTBALL, SEED_ZOMBIE_BUNGEE, SEED_ZOMBIE_POGO, SEED_NONE},
        {SEED_ZOMBONI, SEED_ZOMBIE_CATAPULT, SEED_ZOMBIE_GARGANTUAR, SEED_ZOMBIE_FLAG, SEED_ZOMBIE_JACK_IN_THE_BOX, SEED_ZOMBIE_DIGGER, SEED_NONE, SEED_PEASHOOTER},
    };
    static constexpr SeedType msShufflePools[9][MAX_SEEDS_IN_POOL] = {
        // plant pools - normal
        {SEED_PEASHOOTER, SEED_REPEATER, SEED_CABBAGEPULT, SEED_KERNELPULT, SEED_SNOWPEA, SEED_SPLITPEA, SEED_BLOOMERANG, SEED_NONE, SEED_PEASHOOTER},
        {SEED_WALLNUT, SEED_CHERRYBOMB, SEED_POTATOMINE, SEED_JALAPENO, SEED_SQUASH, SEED_ICEBERG_LETTUCE, SEED_CELERY_STALKER, SEED_NONE, SEED_NONE},
        {SEED_CHOMPER, SEED_THREEPEATER, SEED_SPIKEWEED, SEED_STARFRUIT, SEED_MELONPULT, SEED_GARLIC, SEED_CACTUS, SEED_BONK_CHOY, SEED_SWEET_POTATO},
        // plant pools - has coffee
        {SEED_PUFFSHROOM, SEED_FUMESHROOM, SEED_SCAREDYSHROOM, SEED_SPORESHROOM, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER},
        {SEED_WALLNUT, SEED_HYPNOSHROOM, SEED_ICESHROOM, SEED_DOOMSHROOM, SEED_JALAPENO, SEED_SQUASH, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER},
        {SEED_SPIKEWEED, SEED_GARLIC, SEED_NONE, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER, SEED_PEASHOOTER},
        // zombie pools
        {SEED_ZOMBIE_NORMAL, SEED_ZOMBIE_TRASHCAN, SEED_ZOMBIE_TRAFFIC_CONE, SEED_ZOMBIE_POLEVAULTER, SEED_ZOMBIE_NEWSPAPER, SEED_ZOMBIE_BOBSLED, SEED_ZOMBIE_YETI, SEED_ZOMBIE_SCREEN_DOOR, SEED_NONE},
        {SEED_ZOMBIE_PAIL,
         SEED_ZOMBIE_DANCER,
         SEED_ZOMBIE_LADDER,
         SEED_ZOMBIE_FOOTBALL,
         SEED_ZOMBIE_BUNGEE,
         SEED_ZOMBIE_GIGA_POLEVAULTER,
         SEED_ZOMBIE_SUNDAY_EDITION,
         SEED_ZOMBIE_EXPLORER,
         SEED_ZOMBIE_DIGGER},
        {SEED_ZOMBONI,
         SEED_ZOMBIE_CATAPULT,
         SEED_ZOMBIE_GARGANTUAR,
         SEED_ZOMBIE_FLAG,
         SEED_ZOMBIE_JACK_IN_THE_BOX,
         SEED_ZOMBIE_POGO,
         SEED_ZOMBIE_GIGA_FOOTBALL,
         SEED_ZOMBIE_ZOMBLOB,
         SEED_ZOMBIE_GIGA_GARGANTUAR},
    };
    static inline VSSide msNextFirstPick;
    static inline int msNextSidePickPlayerIndex = 0;

    int mInt70;                   // 70
    int mInt71;                   // 71
    int mInt72;                   // 72
    LawnApp *mApp;                // 73
    VSSetupState mState;          // 74
    int mControllerIndex[2];      // 75  // 0:手柄1, 1:手柄2
    VSSide mSides[2];             // 77 - 78
    bool mSideLocked[2];          // 79
    VSSide mSeedPickTurn;         // 80
    int mChooserAnimateUpdateCnt; // 81
    VSSetupMode mSetupMode;       // 82
    int unkInt83[85];             // 83 ~ 167
    int mInt168;                  // 168
    int unkInt169[59];            // 169 ~ 227
    int mInt228;                  // 228
    int unkInt229[63];            // 229 ~ 291
    int mInt292;                  // 292
    int unkInt293[3];             // 293 ~ 295
    int mInt296;                  // 296
    int unkInt297[7];             // 297 ~ 303
    int mInt304;                  // 304
    int unkInt305[11];            // 305 ~ 315
    bool mBool1264;               // 1264
    bool mBool1265;               // 1265
    int unkInt318[3];             // 317 ~ 319
    int mInt320;                  // 320
    int unkInt321[3];             // 321 ~ 323
    int mInt324;                  // 324
    int unkInt325[3];             // 325 ~ 327
    int mInt328;                  // 328
    int unkInt329[3];             // 329 ~ 331
    int mInt332;                  // 332
    int unkInt333[3];             // 333 ~ 335
    // 大小336个整数, 以下是新增成员
    VSSetupAddonWidget *mAddonWidget;

    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag);
    void SetSecondPlayerIndex(int thePlayerIndex) {
        reinterpret_cast<void (*)(VSSetupMenu *, int)>(VSSetupMenu_SetSecondPlayerIndexAddr)(this, thePlayerIndex);
    }
    void GoToState(VSSetupState theState) {
        reinterpret_cast<void (*)(VSSetupMenu *, int)>(VSSetupMenu_GoToStateAddr)(this, theState);
    }
    void OnPlayerPickedSeed(int thePlayerIndex) {
        reinterpret_cast<void (*)(VSSetupMenu *, int)>(VSSetupMenu_OnPlayerPickedSeedAddr)(this, thePlayerIndex);
    }

    VSSetupMenu() {
        _constructor();
    }
    ~VSSetupMenu() {
        _destructor();
    }

    void Draw(Sexy::Graphics *g);
    void DrawOverlay(Sexy::Graphics *g);
    void Update();
    void OnStateEnter(VSSetupState theState);
    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void CloseVSSetup(bool theShowGameSelector);

    void MouseDown(int x, int y, int theCount);
    void MouseDrag(int x, int y);
    void MouseUp(int x, int y, int theCount);
    void ButtonPress(int theId);
    void ButtonDepress(int theId);
    void ButtonDepress_Origin(int theId);
    void KeyDown(Sexy::KeyCode theKey);

    void processClientEvent(const BaseEvent *event);
    void processServerEvent(const BaseEvent *event);

protected:
    friend void InitHookFunction();
    void _constructor();
    void _destructor();

    void PickRandomZombies(std::vector<SeedType> &theZombieSeeds) const;
    void PickRandomPlants(std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> const &theZombieSeeds) const;

    //    void PickBackgroundImmediately();
};

inline int gVSSetupRequestState = 0;

inline bool is1PControllerMoving;
inline bool is2PControllerMoving;
inline int touchDownX;
inline int touchingOnWhichController; // 0 NONE, 1 1P, 2 2P
inline int drawTipArrowAlphaCounter;


inline void (*old_VSSetupMenu_Constructor)(VSSetupMenu *);

inline void (*old_VSSetupMenu_Destructor)(VSSetupMenu *);

inline void (*old_VSSetupMenu_Draw)(VSSetupMenu *, Sexy::Graphics *g);

inline void (*old_VSSetupMenu_DrawOverlay)(VSSetupMenu *, Sexy::Graphics *g);

inline void (*old_VSSetupMenu_Update)(VSSetupMenu *a);

inline void (*old_VSSetupMenu_KeyDown)(VSSetupMenu *a, Sexy::KeyCode a2);

inline void (*old_VSSetupMenu_OnStateEnter)(VSSetupMenu *menu, VSSetupMenu::VSSetupState theState);

inline void (*old_VSSetupMenu_ButtonPress)(VSSetupMenu *, int theId);

inline void (*old_VSSetupMenu_ButtonDepress)(VSSetupMenu *, int theId);

inline void (*old_VSSetupMenu_AddedToManager)(VSSetupMenu *, Sexy::WidgetManager *a2);

inline void (*old_VSSetupMenu_CloseVSSetup)(VSSetupMenu *, bool a2);

#endif // PVZ_LAWN_WIDGET_VS_SETUP_MENU_H
