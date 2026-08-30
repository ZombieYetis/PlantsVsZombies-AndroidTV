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

#ifndef PVZ_LAWN_WIDGET_VSSETUPADDONWIDGET_H
#define PVZ_LAWN_WIDGET_VSSETUPADDONWIDGET_H

#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/CheckboxListener.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"

inline int VS_ADDON_BUTTON_X = 800;
inline int VS_BUTTON_EXTRA_PACKET_Y = 280;
inline int VS_BUTTON_EXTENDED_SEEDS_Y = 240;
inline int VS_BUTTON_BAN_MODE_Y = 320;
inline int VS_BUTTON_BALANCE_PATCH_Y = 200;
inline int VS_BUTTON_AI_SETTINGS_Y = 440;

namespace Sexy {
class ButtonWidget;
class Widget;
} // namespace Sexy

class AISettingsWidget;

class VSSetupAddonWidget final : public Sexy::CheckboxListener {
public:
    enum {
        VSSetupAddonWidget_ExtraPacket = 12,
        VSSetupAddonWidget_ExtendedSeeds = 13,
        VSSetupAddonWidget_BanMode = 14,
        VSSetupAddonWidget_BalancePatch = 15,
        VSSetupAddonWidget_Back = 16,
        VSSetupAddonWidget_GlobalBP = 17,
        // Local-only AI preferences are appended so existing setup/network
        // IDs remain stable for events, saves, and replays.
        VSSetupAddonWidget_PlantAI = 18,
        VSSetupAddonWidget_ZombieAI = 19,
        VSSetupAddonWidget_AIEnhancement = 20,
        VSSetupAddonWidget_AIDraftDisabled = 21,
        VSSetupAddonWidget_AITemplateDeckDisabled = 22,
        VSSetupAddonWidget_AISettings = 23,
        VSSetupAddonWidget_AISettingsClose = 24,
    };

    enum GlobalBpMode {
        GLOBALBP_CLOSED = -1,
        GLOBALBP_BO3,
        GLOBALBP_BO5,
    };

    static inline bool msBalancePatchMode = false;
    static inline bool msExtraPacketMode = false;
    static inline bool msExtendedSeedsMode = false;
    static inline bool msPlantAIMode = false;
    static inline bool msZombieAIMode = false;
    static inline bool msAIEnhancementMode = false;
    static inline bool msAIDraftDisabledMode = false;
    static inline bool msAITemplateDeckDisabledMode = false;
    static inline GlobalBpMode msGlobalBpMode = GlobalBpMode::GLOBALBP_CLOSED;
    static inline bool msGlobalBpSeedsInitialized = false;
    static inline int msGlobalBpWins[2] = {0, 0};
    static inline SeedType msGlobalBpSeeds[2][VSSetupGlobalBpSyncEvent::kMaxSeedsPerPlayer];

    LawnApp *mApp = gLawnApp;
    Board *mBoard = mApp->mBoard;
    Sexy::ButtonListener *mButtonListener;
    NewLawnButton *mBackButton = nullptr;
    GameButton *mGlobalBpButton = nullptr;
    Sexy::Checkbox *mExtraPacketCheckbox = nullptr;
    Sexy::Checkbox *mExtendedSeedsCheckbox = nullptr;
    Sexy::Checkbox *mBanModeCheckbox = nullptr;
    Sexy::Checkbox *mBalancePatchCheckbox = nullptr;
    GameButton *mAISettingsButton = nullptr;
    AISettingsWidget *mAISettingsWidget = nullptr;
    bool mExtraPacketMode = false;
    bool mExtendedSeedsMode = false;
    bool mBanMode = false;
    bool mBalancePatchMode = false;
    bool mPlantAIMode = false;
    bool mZombieAIMode = false;
    bool mAIEnhancementMode = false;
    bool mAIDraftDisabledMode = false;
    bool mAITemplateDeckDisabledMode = false;
    bool mDrawString = true;

    VSSetupAddonWidget(VSSetupMenu *theVSSetupMenu);
    ~VSSetupAddonWidget();
    static void ResetGlobalBpState();
    void SetDisable(Sexy::Widget *theWidget);
    void ButtonDepress(this VSSetupAddonWidget &self, int theId);
    void CheckboxChecked(int theId, bool checked) override;
    void Draw(Sexy::Graphics *g) const;
    bool GetAddonMode(int theId) const;
    void SetAddonMode(int theId, bool checked, bool saveDetails);
    void UpdateGlobalBpButtonState() const;
    void OpenAISettings();
    void CloseAISettings();

    // Builtin AI preferences belong to the local VS overlay and never enter
    // the network setup event stream.
    static constexpr bool IsLocalAIOption(int theId) {
        switch (theId) {
            case VSSetupAddonWidget_PlantAI:
            case VSSetupAddonWidget_ZombieAI:
            case VSSetupAddonWidget_AIEnhancement:
            case VSSetupAddonWidget_AIDraftDisabled:
            case VSSetupAddonWidget_AITemplateDeckDisabled:
                return true;
            default:
                return false;
        }
    }

private:
    static inline const Sexy::ButtonListener::VTable sButtonListenerVtable{
        .ButtonDepress = (void *)&VSSetupAddonWidget::ButtonDepress,
    };

    static inline Sexy::ButtonListener sButtonListener{&sButtonListenerVtable};
};

void PickMPRandomSeeds(LawnApp *theApp, std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> &theZombieSeeds, bool theIsZombie, bool theAllowCoffee = true);
void PickShuffleSeeds(LawnApp *theApp, std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> &theZombieSeeds, bool theIsZombie);
SeedType PickNextRandomSeed(LawnApp *theApp, std::vector<SeedType> &thePlantSeeds, std::vector<SeedType> &theZombieSeeds, bool theIsZombie, int theSeedIndex);
bool NeedSeedInstantCoffee(LawnApp *theApp, const std::vector<SeedType> &thePlantSeeds = {}, bool theIsShuffle = false);
bool NeedSeedTallnut(LawnApp *theApp);
bool NeedSeedUmbrella(LawnApp *theApp);
bool NeedSeedMagnetshroom(LawnApp *theApp);
bool NeedSeedSplitPea(LawnApp *theApp);
bool IsPeaSeedType(SeedType theSeedType);
bool IsPultSeedType(SeedType theSeedType);
int CountPeasOnScreen(LawnApp *theApp);
int CountPultsOnScreen(LawnApp *theApp);
bool NeedSeedTorchwood(LawnApp *theApp, const std::vector<SeedType> &thePlantSeeds = {}, bool theIsShuffle = false);
bool NeedSeedZombieImp(LawnApp *theApp);
bool NeedSeedZombieScreenDoor(LawnApp *theApp);
bool NeedSeedZombieYeti(LawnApp *theApp);
bool NeedSeedZombieSuperFanImp(LawnApp *theApp, const std::vector<SeedType> &theZombieSeeds = {}, bool theIsShuffle = false);

#endif // PVZ_LAWN_WIDGET_VSSETUPADDONWIDGET_H
