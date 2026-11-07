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

#include "PvZ/SexyAppFramework/Widget/Dialog.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"

#include <cstring>

using namespace Sexy;

void Dialog::SetLinesFont(Font *font) {
    // Choose before the native dialog measures/wraps its text. The native
    // setter duplicates the font and also updates TextLayout's font.
    if (addonFonts.JN_BOBO_HEI20 && std::strstr(mDialogLines->c_str(), "局"))
        font = addonFonts.JN_BOBO_HEI20;
    old_Sexy_Dialog_SetLinesFont(this, font);
}

void Dialog::SetHeaderFont(Font *font) {
    if (addonFonts.JN_BOBO_HEI24 && std::strstr(mDialogHeader->c_str(), "局"))
        font = addonFonts.JN_BOBO_HEI24;
    old_Sexy_Dialog_SetHeaderFont(this, font);
}

void Dialog::AddedToManager(WidgetManager *theWidgetManager) {
    old_Sexy_Dialog_AddedToManager(this, theWidgetManager);
}

void Dialog::RemovedFromManager(WidgetManager *theWidgetManager) {
    old_Sexy_Dialog_RemovedFromManager(this, theWidgetManager);
}
