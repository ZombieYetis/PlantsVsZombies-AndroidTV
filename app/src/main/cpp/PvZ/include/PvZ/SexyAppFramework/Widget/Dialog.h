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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_DIALOG_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_DIALOG_H

#include "Homura/TypeUtils.h"
#include "PvZ/SexyAppFramework/Misc/TextLayout.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/Insets.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"

namespace Sexy {

class ButtonWidget;
class DialogButton;
class Font;
class Image;

class Dialog : public Widget, public ButtonListener {
public:
    int *mDialogListener;                          // 65
    Image *mComponentImage;                        // 66
    DialogButton *mYesButton;                      // 67
    DialogButton *mNoButton;                       // 68
    int mNumButtons;                               // 69
    homura::Storage<pvzstl::string> mDialogHeader; // 70
    homura::Storage<pvzstl::string> mDialogFooter; // 71
    homura::Storage<pvzstl::string> mDialogLines;  // 72
    homura::Storage<pvzstl::string> mUnkText;      // 73
    TextLayout mTextLayout;                        // 74 ~ 157
    int unk1[3];                                   // 158 ~ 160
    int mButtonMode;                               // 161
    int *mHeaderFont;                              // 162
    int *mLinesFont;                               // 163
    int mTextAlign;                                // 164
    int mLineSpacingOffset;                        // 165
    int mButtonHeight;                             // 166
    Insets mBackgroundInsets;                      // 167 ~ 170
    Insets mContentInsets;                         // 171 ~ 174
    int mSpaceAfterHeader;                         // 175
    bool mDragging;                                // 176
    int mDragMouseX;                               // 177
    int mDragMouseY;                               // 178
    int mId;                                       // 179
    bool mIsModal;                                 // 180
    int mResult;                                   // 181
    int mButtonHorzSpacing;                        // 182
    int mButtonSidePadding;                        // 183
    // 大小184个整数

    int WaitForResult(bool autokill = true) {
        return reinterpret_cast<int (*)(Dialog *, bool)>(Sexy_Dialog_WaitForResultAddr)(this, autokill);
    }
    void ButtonDepress(int id) {
        return reinterpret_cast<void (*)(Dialog *, int)>(Sexy_Dialog_ButtonDepressAddr)(this, id);
    }

    void AddedToManager(WidgetManager *theWidgetManager);
    void RemovedFromManager(WidgetManager *theWidgetManager);
    void SetLinesFont(Font *font);
    void SetHeaderFont(Font *font);

protected:
    Dialog() = default;
    ~Dialog() = default;
};

} // namespace Sexy

inline void (*old_Sexy_Dialog_AddedToManager)(Sexy::Dialog *, Sexy::WidgetManager *);

inline void (*old_Sexy_Dialog_RemovedFromManager)(Sexy::Dialog *, Sexy::WidgetManager *);
inline void (*old_Sexy_Dialog_SetLinesFont)(Sexy::Dialog *, Sexy::Font *);
inline void (*old_Sexy_Dialog_SetHeaderFont)(Sexy::Dialog *, Sexy::Font *);

inline void CenterDialog(Sexy::Dialog *theDialog, int theWidth, int theHeight) {
    reinterpret_cast<void (*)(Sexy::Dialog *, int, int)>(CenterDialogAddr)(theDialog, theWidth, theHeight);
}

#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_DIALOG_H
