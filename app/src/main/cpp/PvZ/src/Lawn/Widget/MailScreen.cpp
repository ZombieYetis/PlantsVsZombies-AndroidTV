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

#include "PvZ/Lawn/Widget/MailScreen.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

using namespace Sexy;

namespace {
GameButton *gMailScreenCloseButton;
GameButton *gMailScreenReadButton;
GameButton *gMailScreenSwitchButton;
} // namespace

void MailScreen::_constructor(LawnApp *theApp) {
    // 修复MailScreen的可触控区域不为全屏。
    old_MailScreen_MailScreen(this, theApp);

    gMailScreenReadButton = MakeButton(1002, this, this, "[MARK_MESSAGE_READ]");
    gMailScreenReadButton->Resize(-150, 450, 170, 80);

    gMailScreenSwitchButton = MakeButton(1001, this, this, "[GO_TO_READ_MAIL]");
    gMailScreenSwitchButton->Resize(-150, 520, 170, 80);

    gMailScreenCloseButton = MakeButton(1000, this, this, "[CLOSE]");
    gMailScreenCloseButton->Resize(800, 520, 170, 80);

    Resize(0, 0, 800, 600);
}

void MailScreen::AddedToManager(WidgetManager *theWidgetManager) {
    old_MailScreen_AddedToManager(this, theWidgetManager);

    AddWidget(gMailScreenReadButton);
    AddWidget(gMailScreenSwitchButton);
    AddWidget(gMailScreenCloseButton);
    // Sexy_Widget_Resize(mailScreen, -240, -60, 1280, 720);
}

void MailScreen::RemovedFromManager(WidgetManager *theWidgetManager) {
    RemoveWidget(gMailScreenCloseButton);
    RemoveWidget(gMailScreenSwitchButton);
    RemoveWidget(gMailScreenReadButton);

    old_MailScreen_RemovedFromManager(this, theWidgetManager);
}

void MailScreen::_destructor2() {
    delete gMailScreenCloseButton;
    gMailScreenCloseButton = nullptr;

    delete gMailScreenReadButton;
    gMailScreenReadButton = nullptr;

    delete gMailScreenSwitchButton;
    gMailScreenSwitchButton = nullptr;

    old_MailScreen_Delete2(this);
}

void MailScreen::ButtonPress(int theId) {
    old_MailScreen_ButtonPress(this, theId);
}

void MailScreen::ButtonDepress(int theId) {
    MailScreen *aRealMailScreen = (MailScreen *)gLawnApp->GetDialog(Dialogs::DIALOG_MAIL);
    if (theId == 1002) {
        aRealMailScreen->KeyDown(Sexy::KEYCODE_RETURN);
        // MARK AS READ 后立即写用户配置存档
        gLawnApp->WriteCurrentUserConfig();
    } else if (theId == 1001) {
        aRealMailScreen->KeyDown(Sexy::KEYCODE_GAMEPAD_X);
        bool isAtInBox = aRealMailScreen->mPage == 0;
        gMailScreenReadButton->mDisabled = !isAtInBox;
        gMailScreenReadButton->mBtnNoDraw = !isAtInBox;
        pvzstl::string str = isAtInBox ? "[GO_TO_READ_MAIL]" : "[GO_TO_INBOX]";
        gMailScreenSwitchButton->SetLabel(str);
    } else
        old_MailScreen_ButtonDepress(this, theId);
}


namespace {
constexpr int MAIL_TRIGGER_DISTANCE = 20;
int mMailTouchDownX;
int mMailTouchDownY;
} // namespace

void MailScreen::MouseDown(int x, int y, int theClickCount) {
    mMailTouchDownX = x;
    mMailTouchDownY = y;
}

void MailScreen::MouseDrag(int x, int y) {}

void MailScreen::MouseUp(int x, int y) {
    const int distance = x - mMailTouchDownX;
    if (distance > MAIL_TRIGGER_DISTANCE || (-distance <= MAIL_TRIGGER_DISTANCE && mMailTouchDownX < 400)) {
        KeyDown(Sexy::KEYCODE_LEFT);
    } else {
        KeyDown(Sexy::KEYCODE_RIGHT);
    }
}
