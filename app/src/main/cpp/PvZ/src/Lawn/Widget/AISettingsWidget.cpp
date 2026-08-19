#include "PvZ/Lawn/Widget/AISettingsWidget.h"

#include "Homura/MemberUtils.h"
#include "PvZ/Lawn/Common/LawnCommon.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/SexyAppFramework/Graphics/Color.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Widget/Checkbox.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstring>
#include <iterator>
#include <mutex>

using namespace Sexy;

namespace {
constexpr int kPanelWidth = 540;
constexpr int kPanelHeight = 470;
constexpr int kCheckboxX = 58;
constexpr int kFirstCheckboxY = 86;
constexpr int kCheckboxStep = 54;

using LawnDialogDraw = void (*)(LawnDialog *, Sexy::Graphics *);
LawnDialogDraw gBaseLawnDialogDraw = nullptr;
} // namespace

AISettingsWidget::AISettingsWidget(VSSetupAddonWidget *owner)
    : mOwner(owner) {
    reinterpret_cast<void (*)(LawnDialog *, LawnApp *, Sexy::Image *, int, bool, const pvzstl::string &, const pvzstl::string &, const pvzstl::string &, int)>(LawnDialog_LawnDialogAddr)(
        this, gLawnApp, Sexy::IMAGE_OPTIONS_MENUBACK, VSSetupAddonWidget::VSSetupAddonWidget_AISettings, true, "", "", "", 0);
    // Keep the standard LawnDialog pass enabled so IMAGE_OPTIONS_MENUBACK is
    // drawn behind the AI controls.
    mDrawStandardBack = true;

    static void *sVTable[122];
    static std::once_flag vtableInitFlag;
    std::call_once(vtableInitFlag, [&] {
        std::memcpy(sVTable, this->Sexy::Widget::vTable, sizeof(sVTable));
        gBaseLawnDialogDraw = reinterpret_cast<LawnDialogDraw>(sVTable[36]);
        sVTable[0] = (void *)homura::ExtractMemFuncPtr(&AISettingsWidget::_destructor);
        sVTable[1] = (void *)homura::ExtractMemFuncPtr(&AISettingsWidget::_destructor2);
        sVTable[29] = (void *)homura::ExtractMemFuncPtr(&AISettingsWidget::AddedToManager);
        sVTable[30] = (void *)homura::ExtractMemFuncPtr(&AISettingsWidget::RemovedFromManager);
        sVTable[36] = (void *)homura::ExtractMemFuncPtr(&AISettingsWidget::Draw);
    });
    this->Sexy::Widget::vTable = sVTable;

    LawnDialog::Resize(370, 110, kPanelWidth, kPanelHeight);
    mClip = true;

    mPlantAICheckbox = MakeNewCheckbox(VSSetupAddonWidget::VSSetupAddonWidget_PlantAI, owner, this, false);
    mZombieAICheckbox = MakeNewCheckbox(VSSetupAddonWidget::VSSetupAddonWidget_ZombieAI, owner, this, false);
    mEnhancementCheckbox = MakeNewCheckbox(VSSetupAddonWidget::VSSetupAddonWidget_AIEnhancement, owner, this, false);
    mManualDraftCheckbox = MakeNewCheckbox(VSSetupAddonWidget::VSSetupAddonWidget_AIDraftDisabled, owner, this, false);
    mDisableTemplatesCheckbox = MakeNewCheckbox(VSSetupAddonWidget::VSSetupAddonWidget_AITemplateDeckDisabled, owner, this, false);
    mCloseButton = MakeButton(VSSetupAddonWidget::VSSetupAddonWidget_AISettingsClose, owner == nullptr ? nullptr : owner->mButtonListener, this, "[VS_UI_AI_SETTINGS_CLOSE]");
    mCloseButton->mDrawStoneButton = true;

    Sexy::Checkbox *checkboxes[] = {
        mPlantAICheckbox,
        mZombieAICheckbox,
        mEnhancementCheckbox,
        mManualDraftCheckbox,
        mDisableTemplatesCheckbox,
    };
    for (int index = 0; index < static_cast<int>(std::size(checkboxes)); ++index) {
        checkboxes[index]->Resize(kCheckboxX, kFirstCheckboxY + index * kCheckboxStep, 420, 46);
        checkboxes[index]->mFocusLinks[0] = index == 0 ? static_cast<Widget *>(mCloseButton) : static_cast<Widget *>(checkboxes[index - 1]);
        checkboxes[index]->mFocusLinks[1] = index + 1 == static_cast<int>(std::size(checkboxes)) ? static_cast<Widget *>(mCloseButton) : static_cast<Widget *>(checkboxes[index + 1]);
    }
    mCloseButton->Resize(185, 360, 170, 50);
    mCloseButton->mFocusLinks[0] = mDisableTemplatesCheckbox;

    SyncState();
}

AISettingsWidget::~AISettingsWidget() {
    _destructor();
}

void AISettingsWidget::_destructor() {
    delete mCloseButton;
    delete mDisableTemplatesCheckbox;
    delete mManualDraftCheckbox;
    delete mEnhancementCheckbox;
    delete mZombieAICheckbox;
    delete mPlantAICheckbox;
    reinterpret_cast<void (*)(LawnDialog *)>(LawnDialog_Delete2Addr)(this);
}

void AISettingsWidget::_destructor2() {
    delete this;
}

void AISettingsWidget::AddedToManager(WidgetManager *manager) {
    Sexy::Dialog::AddedToManager(manager);
    AddWidget(mPlantAICheckbox);
    AddWidget(mZombieAICheckbox);
    AddWidget(mEnhancementCheckbox);
    AddWidget(mManualDraftCheckbox);
    AddWidget(mDisableTemplatesCheckbox);
    AddWidget(mCloseButton);
}

void AISettingsWidget::RemovedFromManager(WidgetManager *manager) {
    RemoveWidget(mCloseButton);
    RemoveWidget(mDisableTemplatesCheckbox);
    RemoveWidget(mManualDraftCheckbox);
    RemoveWidget(mEnhancementCheckbox);
    RemoveWidget(mZombieAICheckbox);
    RemoveWidget(mPlantAICheckbox);
    Sexy::Dialog::RemovedFromManager(manager);
}

void AISettingsWidget::Draw(Graphics *graphics) {
    gBaseLawnDialogDraw(this, graphics);
    TodDrawString(graphics, "[VS_UI_AI_SETTINGS]", mWidth / 2, 45, FONT_DWARVENTODCRAFT24, Color(255, 244, 180), DrawStringJustification::DS_ALIGN_CENTER);

    struct Label {
        Sexy::Checkbox *checkbox;
        const char *text;
    };
    const Label labels[] = {
        {mPlantAICheckbox, "[VS_UI_PLANT_AI]"},
        {mZombieAICheckbox, "[VS_UI_ZOMBIE_AI]"},
        {mEnhancementCheckbox, "[VS_UI_AI_ENHANCEMENT]"},
        {mManualDraftCheckbox, "[VS_UI_AI_MANUAL_DRAFT]"},
        {mDisableTemplatesCheckbox, "[VS_UI_AI_DISABLE_TEMPLATES]"},
    };
    graphics->SetFont(FONT_DWARVENTODCRAFT18);
    for (const Label &label : labels) {
        const Color color = label.checkbox == mFocusedChildWidget ? Color(255, 255, 153) : Color(218, 230, 215);
        graphics->SetColor(mSettingsDisabled ? Color(120, 120, 120) : color);
        graphics->DrawString(TodStringTranslate(label.text), label.checkbox->mX + 62, label.checkbox->mY + 28);
    }
}

void AISettingsWidget::SyncState() {
    if (mOwner == nullptr) {
        return;
    }
    mPlantAICheckbox->SetChecked(mOwner->mPlantAIMode, false);
    mZombieAICheckbox->SetChecked(mOwner->mZombieAIMode, false);
    mEnhancementCheckbox->SetChecked(mOwner->mAIEnhancementMode, false);
    mManualDraftCheckbox->SetChecked(mOwner->mAIDraftDisabledMode, false);
    mDisableTemplatesCheckbox->SetChecked(mOwner->mAITemplateDeckDisabledMode, false);
}

void AISettingsWidget::SetDisabled(bool disabled) {
    mSettingsDisabled = disabled;
    Sexy::Checkbox *checkboxes[] = {
        mPlantAICheckbox,
        mZombieAICheckbox,
        mEnhancementCheckbox,
        mManualDraftCheckbox,
        mDisableTemplatesCheckbox,
    };
    for (Sexy::Checkbox *checkbox : checkboxes) {
        checkbox->mDisabled = disabled;
    }
    mCloseButton->mDisabled = false;
}
