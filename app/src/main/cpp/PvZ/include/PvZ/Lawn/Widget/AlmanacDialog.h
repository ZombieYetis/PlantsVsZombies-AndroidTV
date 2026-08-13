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

#ifndef PVZ_LAWN_WIDGET_ALMANAC_DIALOG_H
#define PVZ_LAWN_WIDGET_ALMANAC_DIALOG_H

#include "Homura/TypeUtils.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Widget/CustomScrollbarWidget.h"
#include "PvZ/Symbols.h"

#include "LawnDialog.h"

inline constexpr int NUM_ALMANAC_SEEDS = 49;
inline constexpr int NUM_ALMANAC_ZOMBIES = 26;

// ButtonX和ButtonY是按钮的左上角坐标.
inline constexpr int ALMANAC_BUTTON_BACK_X = -170;
inline constexpr int ALMANAC_BUTTON_BACK_Y = 520;
inline constexpr int ALMANAC_BUTTON_CLOSE_X = 800;
inline constexpr int ALMANAC_BUTTON_CLOSE_Y = 520;

// 按钮长度和宽度为150和50，两个按钮都读取此长宽.
inline constexpr int ALMANAC_BUTTON_WIDTH = 170;
inline constexpr int ALMANAC_BUTTON_HEIGHT = 50;

inline constexpr int ALMANAC_RECT_PLANT_X = 513;
inline constexpr int ALMANAC_RECT_PLANT_Y = 127;

class Plant;
class Zombie;
class AlmanacDialog : public LawnDialog {
private:
    enum {
        ALMANAC_BUTTON_BACK = 110,
        ALMANAC_BUTTON_CLOSE = 111,
    };

public:
    int *mScrollListener;                                     // 191
    LawnApp *mApp;                                            // 192
    GameButton *mPlantButton;                                 // 193
    GameButton *mZombieButton;                                // 194
    Sexy::CustomScrollbarWidget *mScrollTextView;             // 195
    AlmanacPage mOpenPage;                                    // 196
    SeedType mSelectedSeed;                                   // 197
    ZombieType mSelectedZombie;                               // 198
    Plant *mPlant;                                            // 199
    Zombie *mZombie;                                          // 200
    Sexy::Rect mUnkRect;                                      // 201 ~ 204
    Sexy::Rect mDescriptionRect;                              // 205 ~ 208
    Sexy::Rect mCostRect;                                     // 209 ~ 212
    Sexy::Rect mWaitTimeRect;                                 // 213 ~ 216
    int unk2[2];                                              // 217 ~ 218
    homura::Storage<pvzstl::string> mNameString;              // 219
    homura::Storage<pvzstl::string> mDescriptionHeaderString; // 220
    homura::Storage<pvzstl::string> mDescriptionStringTmp;    // 221
    homura::Storage<pvzstl::string> mDescriptionString;       // 222
    homura::Storage<pvzstl::string> mCostString;              // 223
    homura::Storage<pvzstl::string> mWaitTimeString;          // 224
    DrawStringJustification mWaitTimeJustification;           // 225
    DrawStringJustification mJustification;                   // 226
    bool mSetupFinished;                                      // 227 * 4
    double unk3;                                              // 228 ~ 229
    int *mHelpBarWidget;                                      // 230
    int unk4[11];                                             // 231 ~ 233
    // 115: 234, 111: 236

    void KeyDown(Sexy::KeyCode theKey) {
        reinterpret_cast<void (*)(AlmanacDialog *, Sexy::KeyCode)>(AlmanacDialog_KeyDownAddr)(this, theKey);
    }
    SeedType SeedHitTest(int x, int y) {
        return reinterpret_cast<SeedType (*)(AlmanacDialog *, int, int)>(AlmanacDialog_SeedHitTestAddr)(this, x, y);
    }
    ZombieType ZombieHitTest(int x, int y) {
        return reinterpret_cast<ZombieType (*)(AlmanacDialog *, int, int)>(AlmanacDialog_ZombieHitTestAddr)(this, x, y);
    }
    void SetupPlant() {
        reinterpret_cast<void (*)(AlmanacDialog *)>(AlmanacDialog_SetupPlantAddr)(this);
    }
    void SetupZombie() {
        reinterpret_cast<void (*)(AlmanacDialog *)>(AlmanacDialog_SetupZombieAddr)(this);
    }
    void GetSeedPosition(SeedType theSeedType, int &x, int &y) {
        reinterpret_cast<void (*)(AlmanacDialog *, SeedType, int &, int &)>(AlmanacDialog_GetSeedPositionAddr)(this, theSeedType, x, y);
    }
    ZombieType GetZombieType(int theIndex) {
        return reinterpret_cast<ZombieType (*)(AlmanacDialog *, int)>(AlmanacDialog_GetZombieTypeAddr)(this, theIndex);
    }
    void GetZombiePosition(ZombieType theZombieType, int &x, int &y) {
        reinterpret_cast<void (*)(AlmanacDialog *, ZombieType, int &, int &)>(AlmanacDialog_GetZombiePositionAddr)(this, theZombieType, x, y);
    }

    void SetPage(AlmanacPage thePage);
    void Update();
    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void ButtonDepress(int theId);
    void DrawPlants_Unmodified(Sexy::Graphics *g);
    void DrawPlants(Sexy::Graphics *g);
    void DrawZombies(Sexy::Graphics *g);
    bool ZombieIsShown(ZombieType theZombieType);
    bool ZombieHasSilhouette(ZombieType theZombieType) const;
    void SetupLayoutPlants(Sexy::Graphics *g);
    void SetupLayoutZombies(Sexy::Graphics *g);

    void MouseDown(int x, int y, int theClickCount);
    void MouseDrag(int x, int y);
    void MouseUp(int x, int y, int theClickCount);

protected:
    friend void InitHookFunction();

    void _constructor(LawnApp *theApp);
    void _destructor();
};


inline void (*old_AlmanacDialog_AlmanacDialog)(AlmanacDialog *almanacDialog, LawnApp *lawnApp);

inline void (*old_AlmanacDialog_AddedToManager)(AlmanacDialog *almanacDialog, Sexy::WidgetManager *manager);

inline void (*old_AlmanacDialog_SetPage)(AlmanacDialog *almanacDialog, AlmanacPage thePage);

inline void (*old_AlmanacDialog_MouseDrag)(AlmanacDialog *almanacDialog, int, int);

inline void (*old_AlmanacDialog_RemovedFromManager)(AlmanacDialog *almanacDialog, Sexy::WidgetManager *manager);

inline void (*old_AlmanacDialog_Delete2)(AlmanacDialog *almanacDialog);

inline void (*old_AlmanacDialog_DrawPlants)(AlmanacDialog *almanacDialog, Sexy::Graphics *graphics);

inline void (*old_AlmanacDialog_Update)(AlmanacDialog *almanacDialog);

inline void (*old_AlmanacDialog_SetupLayoutPlants)(AlmanacDialog *almanacDialog, Sexy::Graphics *graphics);

inline void (*old_AlmanacDialog_SetupLayoutZombies)(AlmanacDialog *almanacDialog, Sexy::Graphics *graphics);

#endif // PVZ_LAWN_WIDGET_ALMANAC_DIALOG_H
