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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_SCROLLBAR_WIDGET_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_SCROLLBAR_WIDGET_H

#include "PvZ/Symbols.h"

#include "Widget.h"

namespace Sexy {

class ScrollbarWidget : public Widget {
public:
    int unkMem;             // 64
    Widget *mUpButton;      // 65
    Widget *mDownButton;    // 66
    bool mInvisIfNoScroll;  // 268
    int mId;                // 68
    int unkField;           // 69 (0x114, 实际布局存在, 用途未知)
    double mValue;          // 70 ~ 71 (0x118)
    double mMaxValue;       // 72 ~ 73 (0x120)
    double mPageSize;       // 74 ~ 75 (0x128)
    bool mHorizontal;       // 76 (0x130)
    bool mPressedOnThumb;   // 0x131 (MouseDown STRB 实证)
    int mMouseDownThumbPos; // 0x134 (MouseDown STR 实证)
    int mMouseDownX;        // 0x138 (MouseDown STR 实证)
    int mMouseDownY;        // 0x13C (MouseDown STR 实证)
    int mUpdateMode;        // 0x140 (MouseDown STR/Update CMP 实证)
    int mUpdateAcc;         // 0x144 (Update 计数器实证)
    int mButtonAcc;         // 0x148 (ButtonPress/ButtonDownTick 实证)
    int mLastMouseX;        // 0x14C (MouseDown STR 实证)
    int mLastMouseY;        // 0x150 (MouseDown STR 实证)
    int *mScrollListener;   // 0x154 (SetValue 实证)
    Image *mThumbImage;     // 0x158 (构造清零实证)
    Image *mBarImage;       // 0x15C (构造清零实证)
    Image *mPagingImage;    // 0x160 (构造清零实证)
    // 大小: 0x164/4 = 89 个整数

    void SetMaxValue(double theValue) {
        reinterpret_cast<void (*)(Sexy::ScrollbarWidget *, double)>(Sexy_ScrollbarWidget_SetMaxValueAddr)(this, theValue);
    }
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_SCROLLBAR_WIDGET_H
