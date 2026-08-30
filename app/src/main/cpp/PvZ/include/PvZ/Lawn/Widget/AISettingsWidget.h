/*
 * Copyright (C) 2023-2026 PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PVZ_LAWN_WIDGET_AI_SETTINGS_WIDGET_H
#define PVZ_LAWN_WIDGET_AI_SETTINGS_WIDGET_H

#include "PvZ/Lawn/Widget/LawnDialog.h"

class GameButton;
class VSSetupAddonWidget;

namespace Sexy {
class Checkbox;
class WidgetManager;
} // namespace Sexy

class AISettingsWidget final : public LawnDialog {
public:
    explicit AISettingsWidget(VSSetupAddonWidget *owner);
    ~AISettingsWidget();

    void AddedToManager(Sexy::WidgetManager *manager);
    void RemovedFromManager(Sexy::WidgetManager *manager);
    void Draw(Sexy::Graphics *graphics);
    void SyncState();
    void SetDisabled(bool disabled);

private:
    VSSetupAddonWidget *mOwner = nullptr;
    Sexy::Checkbox *mPlantAICheckbox = nullptr;
    Sexy::Checkbox *mZombieAICheckbox = nullptr;
    Sexy::Checkbox *mEnhancementCheckbox = nullptr;
    Sexy::Checkbox *mManualDraftCheckbox = nullptr;
    Sexy::Checkbox *mDisableTemplatesCheckbox = nullptr;
    GameButton *mCloseButton = nullptr;
    bool mSettingsDisabled = false;

    void _destructor();
    void _destructor2();
};

#endif // PVZ_LAWN_WIDGET_AI_SETTINGS_WIDGET_H
