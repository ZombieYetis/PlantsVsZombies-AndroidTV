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

#ifndef PVZ_LAWN_WIDGET_LAWN_DIALOG_H
#define PVZ_LAWN_WIDGET_LAWN_DIALOG_H

#include "PvZ/SexyAppFramework/Widget/Dialog.h"
// #include "PvZ/SexyAppFramework/Widget/GameButton.h"

class GameButton;

class LawnDialog : public Sexy::Dialog {
public:
    LawnApp *mApp; // 184
#if PVZ_VERSION == 111
    int mIsZombie;
#endif
    int mButtonDelay;                 // 185
    Sexy::Widget *mReanimationWidget; // 186
    bool mDrawStandardBack;           // 748
    GameButton *mLawnYesButton;       // 188
    GameButton *mLawnNoButton;        // 189
    bool mTallBottom;                 // 760
    bool mVerticalCenterText;         // 761
    bool unkBool;                     // 762
#if PVZ_VERSION == 111
    int unk2;
#endif
    // 115: 191, 111: 193

    void _constructor(LawnApp *theApp,
                      Sexy::Image *theImage,
                      int theId,
                      bool isModal,
                      const pvzstl::string &theDialogHeader,
                      const pvzstl::string &theDialogLines,
                      const pvzstl::string &theDialogFooter,
                      int theButtonMode) {
        reinterpret_cast<void (*)(LawnDialog *, LawnApp *, Sexy::Image *, int, bool, const pvzstl::string &, const pvzstl::string &, const pvzstl::string &, int)>(LawnDialog_LawnDialogAddr)(
            this, theApp, theImage, theId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
    }

    void _destructor() {
        reinterpret_cast<void (*)(LawnDialog *)>(LawnDialog_Delete2Addr)(this);
    }

    void Resize(int theX, int theY, int theWidth, int theHeight) {
        reinterpret_cast<void (*)(LawnDialog *, int, int, int, int)>(LawnDialog_ResizeAddr)(this, theX, theY, theWidth, theHeight);
    }

protected:
    LawnDialog() = default;
    ~LawnDialog() = default;
};

#endif // PVZ_LAWN_WIDGET_LAWN_DIALOG_H
