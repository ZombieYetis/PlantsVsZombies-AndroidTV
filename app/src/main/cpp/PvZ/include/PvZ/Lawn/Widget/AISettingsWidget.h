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
