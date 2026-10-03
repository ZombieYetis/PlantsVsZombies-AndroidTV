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

#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/System/PlayerInfo.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <algorithm>
#include <iterator>

using namespace Sexy;

namespace {
const char *GetServerModeTransportSuffix() {
    if (!gIsServerModeNetplay) {
        return "";
    }
    return gServerModeTransport == ServerModeTransport::P2P ? " [P2P]" : gServerModeTransport == ServerModeTransport::RELAY ? " [Relay]" : "";
}

bool IsValidVsMode(int mode) {
    switch (mode) {
        case GAMEMODE_MP_VS_DAY:
        case GAMEMODE_MP_VS_NIGHT:
        case GAMEMODE_MP_VS_POOL_DAY:
        case GAMEMODE_MP_VS_POOL_NIGHT:
        case GAMEMODE_MP_VS_ROOF:
        case GAMEMODE_MP_VS_SHUFFLE_MODE:
            return true;
        default:
            return false;
    }
}

bool IsValidCoopMode(int mode) {
    return (mode >= GAMEMODE_TWO_PLAYER_COOP_DAY && mode <= GAMEMODE_TWO_PLAYER_COOP_ENDLESS) || mode == GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD;
}

bool IsNetplayChallengePage(ChallengePage page) {
    return page == ChallengePage::CHALLENGE_PAGE_VS || page == ChallengePage::CHALLENGE_PAGE_COOP;
}

bool IsValidModeForPage(ChallengePage page, int mode) {
    return page == ChallengePage::CHALLENGE_PAGE_VS ? IsValidVsMode(mode) : page == ChallengePage::CHALLENGE_PAGE_COOP ? IsValidCoopMode(mode) : false;
}

// Definition indices are UI-local; the wire protocol keeps its existing mode IDs.
int SelectionToNetplayMode(ChallengePage page, int selection) {
    if (selection < 0 || selection >= NUM_CHALLENGE_MODES) {
        return -1;
    }
    const ChallengeDefinition &aDef = GetChallengeDefinition(selection);
    if (page == CHALLENGE_PAGE_COOP) {
        return aDef.mPage == page ? int(aDef.mChallengeMode) : -1;
    }
    if (page == CHALLENGE_PAGE_VS && aDef.mChallengeMode == GAMEMODE_MP_VS && aDef.mChallengeName != nullptr && aDef.mChallengeName[0] != '\0'
        && (aDef.mPage == CHALLENGE_PAGE_VS || aDef.mPage == CHALLENGE_PAGE_LIMBO)) {
        const int aMode = GAMEMODE_MP_VS_DAY + aDef.mRow * 5 + aDef.mCol;
        return IsValidVsMode(aMode) ? aMode : -1;
    }
    return -1;
}

int NetplayModeToSelection(ChallengePage page, int mode) {
    if (!IsValidModeForPage(page, mode)) {
        return -1;
    }
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        if (SelectionToNetplayMode(page, i) == mode) {
            return i;
        }
    }
    return -1;
}

pvzstl::string GetNetplayModeName(int mode) {
    switch (mode) {
        case GAMEMODE_MP_VS_DAY:
            return TodStringTranslate("[MP_VS_DAY]");
        case GAMEMODE_MP_VS_NIGHT:
            return TodStringTranslate("[MP_VS_NIGHT]");
        case GAMEMODE_MP_VS_POOL_DAY:
            return TodStringTranslate("[MP_VS_POOL_DAY]");
        case GAMEMODE_MP_VS_POOL_NIGHT:
            return TodStringTranslate("[MP_VS_POOL_NIGHT]");
        case GAMEMODE_MP_VS_ROOF:
            return TodStringTranslate("[MP_VS_ROOF]");
        case GAMEMODE_MP_VS_SHUFFLE_MODE:
            return TodStringTranslate("[MP_VS_SHUFFLE_MODE]");
        default:
            break;
    }

    switch (mode) {
        case GAMEMODE_TWO_PLAYER_COOP_DAY:
            return TodStringTranslate("[COOP_1]");
        case GAMEMODE_TWO_PLAYER_COOP_NIGHT:
            return TodStringTranslate("[COOP_2]");
        case GAMEMODE_TWO_PLAYER_COOP_POOL:
            return TodStringTranslate("[COOP_3]");
        case GAMEMODE_TWO_PLAYER_COOP_ROOF:
            return TodStringTranslate("[COOP_4]");
        case GAMEMODE_TWO_PLAYER_COOP_BOWLING:
            return TodStringTranslate("[COOP_BOWLING]");
        case GAMEMODE_TWO_PLAYER_COOP_DAY_HARD:
            return TodStringTranslate("[COOP_HARD_1]");
        case GAMEMODE_TWO_PLAYER_COOP_NIGHT_HARD:
            return TodStringTranslate("[COOP_HARD_2]");
        case GAMEMODE_TWO_PLAYER_COOP_POOL_HARD:
            return TodStringTranslate("[COOP_HARD_3]");
        case GAMEMODE_TWO_PLAYER_COOP_ROOF_HARD:
            return TodStringTranslate("[COOP_HARD_4]");
        case GAMEMODE_TWO_PLAYER_COOP_BOSS:
            return TodStringTranslate("[COOP_FINAL_BOSS]");
        case GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD:
            return TodStringTranslate("[COOP_FINAL_BOSS_HARD]");
        case GAMEMODE_TWO_PLAYER_COOP_ENDLESS:
            return TodStringTranslate("[COOP_ENDLESS]");
        default:
            break;
    }
    return "unknown";
}

bool gNetplayLobbyFinished = false;
} // namespace

ChallengeDefinition gChallengeDefs[] = {
    {GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_1, 0, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 0, 0, "[SURVIVAL_DAY_NORMAL]"},
    {GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_2, 1, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 0, 1, "[SURVIVAL_NIGHT_NORMAL]"},
    {GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_3, 2, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 0, 2, "[SURVIVAL_POOL_NORMAL]"},
    {GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_4, 3, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 0, 3, "[SURVIVAL_FOG_NORMAL]"},
    {GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_5, 4, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 0, 4, "[SURVIVAL_ROOF_NORMAL]"},
    {GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_1, 5, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 1, 0, "[SURVIVAL_DAY_HARD]"},
    {GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_2, 6, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 1, 1, "[SURVIVAL_NIGHT_HARD]"},
    {GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_3, 7, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 1, 2, "[SURVIVAL_POOL_HARD]"},
    {GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_4, 8, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 1, 3, "[SURVIVAL_FOG_HARD]"},
    {GameMode::GAMEMODE_SURVIVAL_HARD_STAGE_5, 9, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 1, 4, "[SURVIVAL_ROOF_HARD]"},
    {GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_1, 10, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 2, 0, "[SURVIVAL_DAY_ENDLESS]"},
    {GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_2, 11, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 2, 1, "[SURVIVAL_NIGHT_ENDLESS]"},
    {GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_3, 12, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 2, 2, "[SURVIVAL_POOL_ENDLESS]"},
    {GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_4, 13, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 2, 3, "[SURVIVAL_FOG_ENDLESS]"},
    {GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_5, 14, ChallengePage::CHALLENGE_PAGE_SURVIVAL, 2, 4, "[SURVIVAL_ROOF_ENDLESS]"},
    {GameMode::GAMEMODE_CHALLENGE_WAR_AND_PEAS, 0, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 0, 0, "[WAR_AND_PEAS]"},
    {GameMode::GAMEMODE_CHALLENGE_WALLNUT_BOWLING, 1, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 0, 1, "[WALL_NUT_BOWLING]"},
    {GameMode::GAMEMODE_CHALLENGE_SLOT_MACHINE, 2, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 0, 2, "[SLOT_MACHINE]"},
    {GameMode::GAMEMODE_CHALLENGE_HEAVY_WEAPON, 36, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 0, 3, "[HEAVY_WEAPON]"},
    {GameMode::GAMEMODE_CHALLENGE_BEGHOULED, 4, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 0, 4, "[BEGHOULED]"},
    {GameMode::GAMEMODE_CHALLENGE_INVISIGHOUL, 5, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 1, 0, "[INVISIGHOUL]"},
    {GameMode::GAMEMODE_CHALLENGE_SEEING_STARS, 6, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 1, 1, "[SEEING_STARS]"},
    {GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM, 7, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 1, 2, "[ZOMBIQUARIUM]"},
    {GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST, 8, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 1, 3, "[BEGHOULED_TWIST]"},
    {GameMode::GAMEMODE_CHALLENGE_LITTLE_TROUBLE, 9, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 1, 4, "[LITTLE_TROUBLE]"},
    {GameMode::GAMEMODE_CHALLENGE_PORTAL_COMBAT, 10, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 2, 0, "[PORTAL_COMBAT]"},
    {GameMode::GAMEMODE_CHALLENGE_COLUMN, 11, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 2, 1, "[COLUMN_AS_YOU_SEE_EM]"},
    {GameMode::GAMEMODE_CHALLENGE_BOBSLED_BONANZA, 12, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 2, 2, "[BOBSLED_BONANZA]"},
    {GameMode::GAMEMODE_CHALLENGE_SPEED, 13, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 2, 3, "[ZOMBIES_ON_SPEED]"},
    {GameMode::GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE, 14, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 2, 4, "[WHACK_A_ZOMBIE]"},
    {GameMode::GAMEMODE_CHALLENGE_LAST_STAND, 15, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 3, 0, "[LAST_STAND]"},
    {GameMode::GAMEMODE_CHALLENGE_WAR_AND_PEAS_2, 16, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 3, 1, "[WAR_AND_PEAS_2]"},
    {GameMode::GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2, 17, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 3, 2, "[WALL_NUT_BOWLING_EXTREME]"},
    {GameMode::GAMEMODE_CHALLENGE_POGO_PARTY, 18, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 3, 3, "[POGO_PARTY]"},
    {GameMode::GAMEMODE_CHALLENGE_FINAL_BOSS, 19, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 3, 4, "[FINAL_BOSS]"},
    {GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT, 20, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 4, 0, "[ART_CHALLENGE_WALL_NUT]"},
    {GameMode::GAMEMODE_CHALLENGE_SUNNY_DAY, 21, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 4, 1, "[SUNNY_DAY]"},
    {GameMode::GAMEMODE_CHALLENGE_RESODDED, 22, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 4, 2, "[UNSODDED]"},
    {GameMode::GAMEMODE_CHALLENGE_BIG_TIME, 23, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 4, 3, "[BIG_TIME]"},
    {GameMode::GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER, 24, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 4, 4, "[ART_CHALLENGE_SUNFLOWER]"},
    {GameMode::GAMEMODE_CHALLENGE_AIR_RAID, 25, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 5, 0, "[AIR_RAID]"},
    {GameMode::GAMEMODE_CHALLENGE_ICE, 6, ChallengePage::CHALLENGE_PAGE_LIMBO, 5, 1, "[ICE_LEVEL]"},
    {GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN, 7, ChallengePage::CHALLENGE_PAGE_LIMBO, 5, 2, "[ZEN_GARDEN]"},
    {GameMode::GAMEMODE_CHALLENGE_HIGH_GRAVITY, 26, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 5, 3, "[HIGH_GRAVITY]"},
    {GameMode::GAMEMODE_CHALLENGE_GRAVE_DANGER, 27, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 5, 4, "[GRAVE_DANGER]"},
    {GameMode::GAMEMODE_CHALLENGE_SHOVEL, 28, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 6, 0, "[CAN_YOU_DIG_IT]"},
    {GameMode::GAMEMODE_CHALLENGE_STORMY_NIGHT, 29, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 6, 1, "[DARK_STORMY_NIGHT]"},
    {GameMode::GAMEMODE_CHALLENGE_BUNGEE_BLITZ, 30, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 6, 2, "[BUNGEE_BLITZ]"},
    {GameMode::GAMEMODE_CHALLENGE_SQUIRREL, 31, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 6, 3, "[SQUIRREL]"},
    {GameMode::GAMEMODE_TREE_OF_WISDOM, 10, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 0, "[TREE_OF_WISDOM]"},
    {GameMode::GAMEMODE_SCARY_POTTER_1, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 0, 0, "[SCARY_POTTER_1]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_1, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 0, 1, "[I_ZOMBIE_1]"},
    {GameMode::GAMEMODE_SCARY_POTTER_2, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 0, 2, "[SCARY_POTTER_2]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_2, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 0, 3, "[I_ZOMBIE_2]"},
    {GameMode::GAMEMODE_SCARY_POTTER_3, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 0, 4, "[SCARY_POTTER_3]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_3, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 1, 0, "[I_ZOMBIE_3]"},
    {GameMode::GAMEMODE_SCARY_POTTER_4, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 1, 1, "[SCARY_POTTER_4]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_4, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 1, 2, "[I_ZOMBIE_4]"},
    {GameMode::GAMEMODE_SCARY_POTTER_5, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 1, 3, "[SCARY_POTTER_5]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_5, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 1, 4, "[I_ZOMBIE_5]"},
    {GameMode::GAMEMODE_SCARY_POTTER_6, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 2, 0, "[SCARY_POTTER_6]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_6, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 2, 1, "[I_ZOMBIE_6]"},
    {GameMode::GAMEMODE_SCARY_POTTER_7, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 2, 2, "[SCARY_POTTER_7]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_7, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 2, 2, "[I_ZOMBIE_7]"},
    {GameMode::GAMEMODE_SCARY_POTTER_8, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 2, 4, "[SCARY_POTTER_8]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_8, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 3, 0, "[I_ZOMBIE_8]"},
    {GameMode::GAMEMODE_SCARY_POTTER_9, 32, ChallengePage::CHALLENGE_PAGE_PUZZLE, 3, 1, "[SCARY_POTTER_9]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_9, 34, ChallengePage::CHALLENGE_PAGE_PUZZLE, 3, 2, "[I_ZOMBIE_9]"},
    {GameMode::GAMEMODE_SCARY_POTTER_ENDLESS, 33, ChallengePage::CHALLENGE_PAGE_PUZZLE, 3, 3, "[SCARY_POTTER_ENDLESS]"},
    {GameMode::GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS, 35, ChallengePage::CHALLENGE_PAGE_PUZZLE, 3, 4, "[I_ZOMBIE_ENDLESS]"},
    {GameMode::GAMEMODE_UPSELL, 10, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 0, "[UPSELL]"},
    {GameMode::GAMEMODE_INTRO, 10, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 1, "[INTRO]"},
    {GameMode::GAMEMODE_MULTI_PLAYER, 10, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 2, ""},
    {GameMode::GAMEMODE_MP_VS_DEBUG, 10, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 3, ""},
    {GameMode::GAMEMODE_MP_VS, 10, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 0, ""},
    {GameMode::GAMEMODE_MP_VS_COOP, 18, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 1, "[COOP]"},
    {GameMode::GAMEMODE_MP_VS_UNKONWN, 18, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 2, ""},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY, 0, ChallengePage::CHALLENGE_PAGE_COOP, 0, 0, "[COOP_1]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_NIGHT, 1, ChallengePage::CHALLENGE_PAGE_COOP, 0, 1, "[COOP_2]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_POOL, 2, ChallengePage::CHALLENGE_PAGE_COOP, 0, 2, "[COOP_3]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_ROOF, 4, ChallengePage::CHALLENGE_PAGE_COOP, 0, 3, "[COOP_4]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_BOWLING, 1, ChallengePage::CHALLENGE_PAGE_COOP, 0, 4, "[COOP_BOWLING]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_DAY_HARD, 5, ChallengePage::CHALLENGE_PAGE_COOP, 1, 0, "[COOP_HARD_1]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_NIGHT_HARD, 6, ChallengePage::CHALLENGE_PAGE_COOP, 1, 1, "[COOP_HARD_2]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_POOL_HARD, 7, ChallengePage::CHALLENGE_PAGE_COOP, 1, 2, "[COOP_HARD_3]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_ROOF_HARD, 9, ChallengePage::CHALLENGE_PAGE_COOP, 1, 3, "[COOP_HARD_4]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS, 19, ChallengePage::CHALLENGE_PAGE_COOP, 1, 4, "[COOP_FINAL_BOSS]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS, 12, ChallengePage::CHALLENGE_PAGE_COOP, 2, 0, "[COOP_ENDLESS]"},
    {GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS, 3, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 6, 4, "[ITS_RAINING_SEEDS]"},
    {GameMode::GAMEMODE_CHALLENGE_BUTTERED_POPCORN, 37, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 7, 0, "[BUTTERED_POPCORN]"},
    {GameMode::GAMEMODE_MP_VS_IN_PAGE, 0, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 0, "[MP_VS_DAY]"},
    {GameMode::GAMEMODE_MP_VS_IN_PAGE, 1, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 1, "[MP_VS_NIGHT]"},
    {GameMode::GAMEMODE_MP_VS_IN_PAGE, 2, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 2, "[MP_VS_POOL_DAY]"},
    {GameMode::GAMEMODE_MP_VS_IN_PAGE, 3, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 3, "[MP_VS_POOL_NIGHT]"},
    {GameMode::GAMEMODE_MP_VS_IN_PAGE, 4, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 4, "[MP_VS_ROOF]"},
    // 新增关卡
    {GameMode::GAMEMODE_MP_VS, 0, ChallengePage::CHALLENGE_PAGE_VS, 0, 0, "[MP_VS_DAY]"},
    {GameMode::GAMEMODE_MP_VS, 1, ChallengePage::CHALLENGE_PAGE_VS, 0, 1, "[MP_VS_NIGHT]"},
    {GameMode::GAMEMODE_MP_VS, 2, ChallengePage::CHALLENGE_PAGE_VS, 0, 2, "[MP_VS_POOL_DAY]"},
    {GameMode::GAMEMODE_MP_VS, 3, ChallengePage::CHALLENGE_PAGE_VS, 0, 3, "[MP_VS_POOL_NIGHT]"},
    {GameMode::GAMEMODE_MP_VS, 4, ChallengePage::CHALLENGE_PAGE_LIMBO, 0, 4, "[MP_VS_ROOF]"},
    {GameMode::GAMEMODE_MP_VS, 0, ChallengePage::CHALLENGE_PAGE_VS, 1, 0, "[MP_VS_SHUFFLE_MODE]"},
    {GameMode::GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD, 19, ChallengePage::CHALLENGE_PAGE_COOP, 2, 1, "[COOP_FINAL_BOSS_HARD]"},
};

const int NUM_CHALLENGE_MODES = int(std::size(gChallengeDefs));
static_assert(std::size(gChallengeDefs) <= MAX_CHALLENGE_MODES);

void ChallengeScreen::_constructor(LawnApp *theApp, ChallengePage thePage) {
    Widget::_constructor();
    Widget::vTable = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(vTableForChallengeScreenAddr) + 8);
    ButtonListener::vTable = reinterpret_cast<const ButtonListener::VTable *>(reinterpret_cast<uintptr_t>(Widget::vTable) + 0x1F8);
    mUtil = Curve1DUtil();
    mApp = theApp;
    mPage = thePage;
    mLockShakeX = 0.0f;
    mLockShakeY = 0.0f;
    mScrollAnimationTime = 0.0f;
    mClip = false;
    mCheatEnableChallenges = false;
    mUnlockState = UnlockingState::UNLOCK_OFF;
    mUnlockStateCounter = 0;
    mScrollPosition = 0;
    mScrollTargetPosition = 0;
    mPageChallengeCount = 0;
    mUnlockChallengeIndex = -1;
    mSelectedGameMode = GameMode::GAMEMODE_ADVENTURE;
    mSelectedChallengeIndex = -1;
    mSurvivalCount = 0;
    mBackButton = nullptr;
    mNetplayLobbyWidget = nullptr;
    for (int i = 0; i < MAX_CHALLENGE_MODES; i++) {
        GetChallengeButton(i) = nullptr;
        GetPageChallengeIndex(i) = 0;
        GetPageChallengeAnimTime(i) = 0.0f;
    }
    for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++) {
        ChallengeDefinition &aDef = GetChallengeDefinition(aChallengeMode);
        if (aDef.mChallengeName == nullptr) {
            continue;
        }
        auto *aButton = new ButtonWidget_(ChallengeScreen_Mode + aChallengeMode, this);
        GetChallengeButton(aChallengeMode) = aButton;
        aButton->mDoFinger = true;
        aButton->mFrameNoDraw = true;
        if (MoreTrophiesNeeded(aChallengeMode) != 0) {
            aButton->mDisabled = true;
            aButton->mDoFinger = false;
        }
        if (aDef.mPage == mPage) {
            aButton->Resize(35, mPageChallengeCount * 120 + 80, 112, 65);
            GetPageChallengeIndex(mPageChallengeCount++) = aChallengeMode;
        }
    }
    mToolTip = new ToolTipWidget();
    mToolTip->mCenter = true;
    mToolTip->mVisible = false;
    UpdateButtons();
    if (mApp->mGameMode != GameMode::GAMEMODE_UPSELL || mApp->mGameScene != GameScenes::SCENE_LEVEL_INTRO) {
        mApp->mMusic->MakeSureMusicIsPlaying(MusicTune::MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
    }
    auto *aPlayerInfo = mApp->mPlayerInfo;
    if (mPage == CHALLENGE_PAGE_SURVIVAL && aPlayerInfo->GetFlag(512)) {
        SetUnlockChallengeIndex(mPage, false);
        aPlayerInfo->SetFlag(512, false);
    } else if (mPage == CHALLENGE_PAGE_CHALLENGE && aPlayerInfo->GetFlag(64)) {
        SetUnlockChallengeIndex(mPage, false);
        aPlayerInfo->SetFlag(64, false);
    } else if (mPage == CHALLENGE_PAGE_PUZZLE) {
        if (aPlayerInfo->GetFlag(128)) {
            SetUnlockChallengeIndex(mPage, false);
            aPlayerInfo->SetFlag(128, false);
        } else if (aPlayerInfo->GetFlag(256)) {
            SetUnlockChallengeIndex(mPage, true);
            aPlayerInfo->SetFlag(256, false);
        }
    }
    mHelpBarWidget = mApp->mHelpBarWidget;
    mHelpBarWidget->ClearButtons(0);
    mHelpBarWidget->AddButton(GamepadButton::GAMEPAD_BUTTON_B, "[BACK]", HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->mUnk[24] = 0;
    gNetplayLobbyFinished = false;
    mBackButton =
        MakeNewButton(ChallengeScreen_Back, this, this, "[CLOSE]", nullptr, Sexy::IMAGE_SEEDCHOOSER_BUTTON_DISABLED, Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW);
    mBackButton->mTextOffsetX = -2;
    mBackButton->mTextOffsetY = -4;
    mBackButton->mTextDownOffsetX = 1;
    mBackButton->mTextDownOffsetY = 1;
    mBackButton->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
    (*mBackButton->mColors)[ButtonWidget::COLOR_LABEL_HILITE] = Color(0, 205, 0);
    mBackButton->Resize(800, 520, 160, 50);
    // Keep the existing touch list; native buttons must not intercept its clicks.
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        if (auto *aButton = GetChallengeButton(i)) {
            aButton->Resize(aButton->mX, aButton->mY, 0, 0);
        }
    }
    if (mPage == CHALLENGE_PAGE_VS) {
        Challenge::msVSShuffleMode = false;
        mApp->TryHelpTextScreen(HelpTextPage::HELP_TEXT_PAGE_VS);
    }
    if (mPage == CHALLENGE_PAGE_COOP) {
        mApp->TryHelpTextScreen(HelpTextPage::HELP_TEXT_PAGE_COOP);
    }
    if (IsNetplayChallengePage(mPage)) {
        gChallengeScreenRequestState = 0;
    }
}

void ChallengeScreen::_destructor() {
    if (mNetplayLobbyWidget != nullptr) {
        if (mNetplayLobbyWidget->mParent == this) {
            RemoveWidget(mNetplayLobbyWidget);
        }
        delete mNetplayLobbyWidget;
        mNetplayLobbyWidget = nullptr;
    }
    delete mBackButton;
    for (int i = 0; i < MAX_CHALLENGE_MODES; i++) {
        delete static_cast<ButtonWidget_ *>(GetChallengeButton(i));
        GetChallengeButton(i) = nullptr;
    }
    delete mToolTip;
    mToolTip = nullptr;
    Widget::_destructor();
}

ChallengeDefinition &GetChallengeDefinition(int theChallengeMode) {
    return gChallengeDefs[theChallengeMode];
}

bool ChallengePageHasEntry(ChallengePage thePage, int theRow, int theCol) {
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        const ChallengeDefinition &aDef = GetChallengeDefinition(i);
        if (aDef.mChallengeName != nullptr && aDef.mPage == thePage && aDef.mRow == theRow && aDef.mCol == theCol) {
            return true;
        }
    }
    return false;
}

int GetChallengeByRowColumn(ChallengePage thePage, int theRow, int theCol) {
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        const ChallengeDefinition &aDef = GetChallengeDefinition(i);
        if (aDef.mChallengeName != nullptr && aDef.mPage == thePage && aDef.mRow == theRow && aDef.mCol == theCol) {
            return i;
        }
    }
    return 2;
}

GameMode GetModeByRowColumn(ChallengePage thePage, int theRow, int theCol) {
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        const ChallengeDefinition &aDef = GetChallengeDefinition(i);
        if (aDef.mChallengeName != nullptr && aDef.mPage == thePage && aDef.mRow == theRow && aDef.mCol == theCol) {
            return aDef.mChallengeMode;
        }
    }
    return GameMode(-1);
}

bool ChallengeScreen::IsScaryPotterLevel(GameMode theGameMode) {
    // TV interleaves Scary Potter and I, Zombie entries.
    return theGameMode >= GAMEMODE_SCARY_POTTER_1 && theGameMode <= GAMEMODE_SCARY_POTTER_ENDLESS && (theGameMode - GAMEMODE_SCARY_POTTER_1) % 2 == 0;
}

bool ChallengeScreen::IsIZombieLevel(GameMode theGameMode) {
    return theGameMode >= GAMEMODE_PUZZLE_I_ZOMBIE_1 && theGameMode <= GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS && (theGameMode - GAMEMODE_PUZZLE_I_ZOMBIE_1) % 2 == 0;
}

void ChallengeScreen::SetUnlockChallengeIndex(ChallengePage thePage, bool theIsIZombie) {
    mUnlockState = UnlockingState::UNLOCK_SHAKING;
    mUnlockStateCounter = 100;
    mUnlockChallengeIndex = 0;
    for (int aChallengeMode = 0; aChallengeMode < NUM_CHALLENGE_MODES; aChallengeMode++) {
        const ChallengeDefinition &aDef = GetChallengeDefinition(aChallengeMode);
        if (aDef.mChallengeName == nullptr || aDef.mPage != thePage) {
            continue;
        }
        if (thePage == CHALLENGE_PAGE_PUZZLE && !(theIsIZombie ? IsIZombieLevel(aDef.mChallengeMode) : IsScaryPotterLevel(aDef.mChallengeMode))) {
            continue;
        }
        if (AccomplishmentsNeeded(aChallengeMode) > 0) {
            continue;
        }
        mUnlockChallengeIndex = aChallengeMode;
        mSelectedChallengeIndex = aChallengeMode;
        for (int i = 0; i < mPageChallengeCount; i++) {
            if (GetPageChallengeIndex(i) == aChallengeMode) {
                SetScrollTarget(i - 2);
                break;
            }
        }
    }
}

int ChallengeScreen::MoreTrophiesNeeded(int theChallengeIndex) {
    const ChallengeDefinition &aDef = GetChallengeDefinition(theChallengeIndex);
    const GameMode aMode = aDef.mChallengeMode;
    if (aMode == GAMEMODE_MP_VS) {
        return 0;
    }
    if (IsValidCoopMode(aMode)) {
        if (mApp->mPlayerInfo->GetFlag(1)) {
            return 0;
        }
        if (aMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS) {
            return 1;
        }
        const GameMode aUnlockMode = aMode == GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD ? GAMEMODE_TWO_PLAYER_COOP_BOSS : aMode;
        const int aLevelsNeeded = int(aUnlockMode) - GAMEMODE_TWO_PLAYER_COOP_DAY - mApp->mPlayerInfo->mLevel / 10;
        return aLevelsNeeded <= 0 ? 0 : aLevelsNeeded == 1 ? 1 : 2;
    }
    if (mApp->mGameMode == GAMEMODE_UPSELL && mApp->mGameScene == SCENE_LEVEL_INTRO) {
        return aMode == GAMEMODE_CHALLENGE_FINAL_BOSS ? 1 : 0;
    }
    if (mApp->IsTrialStageLocked()) {
        if (aDef.mPage == CHALLENGE_PAGE_PUZZLE && aMode >= GAMEMODE_SCARY_POTTER_4) {
            return aMode == GAMEMODE_SCARY_POTTER_4 ? 1 : 2;
        }
        if (aDef.mPage == CHALLENGE_PAGE_CHALLENGE && aMode >= GAMEMODE_CHALLENGE_RAINING_SEEDS) {
            return aMode == GAMEMODE_CHALLENGE_RAINING_SEEDS ? 1 : 2;
        }
        if (aDef.mPage == CHALLENGE_PAGE_SURVIVAL && aMode >= GAMEMODE_SURVIVAL_NORMAL_STAGE_4) {
            return aMode == GAMEMODE_SURVIVAL_NORMAL_STAGE_4 ? 1 : 2;
        }
    }
    if (aDef.mPage == CHALLENGE_PAGE_PUZZLE && (IsScaryPotterLevel(aMode) || IsIZombieLevel(aMode))) {
        const bool aIsIZombie = IsIZombieLevel(aMode);
        int aLevelsCompleted = 0;
        for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
            const ChallengeDefinition &aPuzzleDef = GetChallengeDefinition(i);
            if (aPuzzleDef.mChallengeName != nullptr && (aIsIZombie ? IsIZombieLevel(aPuzzleDef.mChallengeMode) : IsScaryPotterLevel(aPuzzleDef.mChallengeMode))
                && mApp->HasBeatenChallenge(aPuzzleDef.mChallengeMode)) {
                aLevelsCompleted++;
            }
        }
        const GameMode aFirstMode = aIsIZombie ? GAMEMODE_PUZZLE_I_ZOMBIE_1 : GAMEMODE_SCARY_POTTER_1;
        const GameMode aFourthMode = aIsIZombie ? GAMEMODE_PUZZLE_I_ZOMBIE_4 : GAMEMODE_SCARY_POTTER_4;
        if (aMode >= aFourthMode && !mApp->HasFinishedAdventure() && aLevelsCompleted >= 3) {
            return aMode == aFourthMode ? 1 : 2;
        }
        return std::clamp((int(aMode) - int(aFirstMode)) / 2 - aLevelsCompleted, 0, 9);
    }
    const int aIndexInPage = aDef.mRow * 5 + aDef.mCol;
    if ((aDef.mPage == CHALLENGE_PAGE_CHALLENGE || aDef.mPage == CHALLENGE_PAGE_SURVIVAL) && !mApp->HasFinishedAdventure()) {
        return aIndexInPage < 3 ? 0 : aIndexInPage == 3 ? 1 : 2;
    }
    const int aNumTrophies = mApp->GetNumTrophies(aDef.mPage);
    if (aDef.mPage == CHALLENGE_PAGE_LIMBO) {
        return 0;
    }
    if (mApp->IsSurvivalEndless(aMode)) {
        return 10 - aNumTrophies;
    }
    return std::max(aIndexInPage - (aNumTrophies + 3) + 1, 0);
}

int ChallengeScreen::AccomplishmentsNeeded(int theChallengeIndex) {
    int aTrophiesNeeded = MoreTrophiesNeeded(theChallengeIndex);
    const GameMode aMode = GetChallengeDefinition(theChallengeIndex).mChallengeMode;
    if (mApp->IsSurvivalEndless(aMode) && aTrophiesNeeded <= 3 && mApp->GetNumTrophies(CHALLENGE_PAGE_SURVIVAL) < 10 && mApp->HasFinishedAdventure() && !mApp->IsTrialStageLocked()) {
        aTrophiesNeeded = 1;
    }
    return mCheatEnableChallenges ? 0 : aTrophiesNeeded;
}

void ChallengeScreen::UpdateToolTip() {
    if (mSelectedChallengeIndex < 0 || mSelectedChallengeIndex >= NUM_CHALLENGE_MODES) {
        mToolTip->mVisible = false;
        return;
    }
    ButtonWidget *aButton = GetChallengeButton(mSelectedChallengeIndex);
    if (aButton == nullptr || !aButton->mVisible || !aButton->mDisabled || AccomplishmentsNeeded(mSelectedChallengeIndex) > 1 || MoreTrophiesNeeded(mSelectedChallengeIndex) <= 0) {
        mToolTip->mVisible = false;
        return;
    }
    mToolTip->mX = aButton->mX + aButton->mWidth / 2;
    mToolTip->mY = aButton->mY + 56 - mToolTip->mHeight / 2;
    const GameMode aMode = GetChallengeDefinition(mSelectedChallengeIndex).mChallengeMode;
    const char *aLabel = nullptr;
    if (mPage == CHALLENGE_PAGE_PUZZLE && IsScaryPotterLevel(aMode)) {
        aLabel = !mApp->HasFinishedAdventure() && aMode == GAMEMODE_SCARY_POTTER_4 ? "[FINISH_ADVENTURE_TOOLTIP]" : "[ONE_MORE_SCARY_POTTER_TOOLTIP]";
    } else if (mPage == CHALLENGE_PAGE_PUZZLE && IsIZombieLevel(aMode)) {
        aLabel = !mApp->HasFinishedAdventure() && aMode == GAMEMODE_PUZZLE_I_ZOMBIE_4 ? "[FINISH_ADVENTURE_TOOLTIP]" : "[ONE_MORE_IZOMBIE_TOOLTIP]";
    } else if (mPage == CHALLENGE_PAGE_COOP) {
        mToolTip->SetLabel(aMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS ? "[FINISH_ADVENTURE_TOOLTIP]" : "[PLAY_MORE_ADVENTURE_TOOLTIP]");
        // TV hides the tooltip for locked coop entries.
        mToolTip->mVisible = false;
        return;
    } else if (!mApp->HasFinishedAdventure() || mApp->IsTrialStageLocked()) {
        aLabel = "[FINISH_ADVENTURE_TOOLTIP]";
    } else if (mApp->IsSurvivalEndless(aMode)) {
        aLabel = "[10_SURVIVAL_TOOLTIP]";
    } else if (mPage == CHALLENGE_PAGE_SURVIVAL) {
        aLabel = "[ONE_MORE_SURVIVAL_TOOLTIP]";
    } else if (mPage == CHALLENGE_PAGE_CHALLENGE) {
        aLabel = "[ONE_MORE_CHALLENGE_TOOLTIP]";
    }
    if (aLabel == nullptr) {
        mToolTip->mVisible = false;
        return;
    }
    mToolTip->SetLabel(aLabel);
}

void ChallengeScreen::KeyChar(char theChar) {
    if (!mApp->mDebugKeysEnabled || (gDisableDebugKeysAddr != nullptr && *static_cast<const bool *>(gDisableDebugKeysAddr))) {
        return;
    }
    if (theChar == 'c' || theChar == 'C') {
        mCheatEnableChallenges = !mCheatEnableChallenges;
        for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
            if (auto *aButton = GetChallengeButton(i)) {
                const bool aEnabled = mCheatEnableChallenges || MoreTrophiesNeeded(i) == 0;
                aButton->mDoFinger = aEnabled;
                aButton->mDisabled = !aEnabled;
            }
        }
    } else if (theChar == 'u') {
        SetUnlockChallengeIndex(mPage, false);
    }
}

void ChallengeScreen::GameButtonDown(GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag) {
    switch (theButton) {
        case GAMEPAD_BUTTON_UP:
            KeyDown(KEYCODE_UP);
            break;
        case GAMEPAD_BUTTON_DOWN:
            KeyDown(KEYCODE_DOWN);
            break;
        case GAMEPAD_BUTTON_A:
            KeyDown(KEYCODE_RETURN);
            break;
        case GAMEPAD_BUTTON_B:
            KeyDown(KEYCODE_ESCAPE);
            break;
        default:
            break;
    }
}

void ChallengeScreen::Draw(Sexy::Graphics *g) {
    g->DrawImage(Sexy::IMAGE_CHALLENGE_BACKGROUND, LawnApp::FULLSCREEN_RECT.mX, -60);

    pvzstl::string aTitleString = mPage == CHALLENGE_PAGE_SURVIVAL ? "[PICK_AREA]"
        : mPage == CHALLENGE_PAGE_PUZZLE                           ? "[SCARY_POTTER]"
        : mPage == CHALLENGE_PAGE_VS                               ? "[VS_MODE]"
        : mPage == CHALLENGE_PAGE_COOP                             ? "[XBOX_COOP]"
                                                                   : "[PICK_CHALLENGE]";
    TodDrawString(g, aTitleString, 400, 45, Sexy::FONT_HOUSEOFTERROR28, Color(220, 220, 220), DS_ALIGN_CENTER);

    int aTrophiesGot = mApp->GetNumTrophies(mPage);
    int aTrophiesTotal = (mPage == CHALLENGE_PAGE_SURVIVAL || mPage == CHALLENGE_PAGE_COOP) ? 10 : mPage == CHALLENGE_PAGE_PUZZLE ? 18 : 0;
    if (mPage == CHALLENGE_PAGE_CHALLENGE) {
        for (int i = 0; i < NUM_CHALLENGE_MODES; ++i) {
            if (GetChallengeDefinition(i).mPage == ChallengePage::CHALLENGE_PAGE_CHALLENGE) {
                aTrophiesTotal++;
            }
        }
    }
    if (aTrophiesTotal > 0) {
        pvzstl::string aTrophyString = StrFormat(TodStringTranslate("[NUMBER_OF_TROPHIES]").c_str(), aTrophiesGot, aTrophiesTotal);
        TodDrawString(g, aTrophyString, 711, 62, Sexy::FONT_BRIANNETOD16, Color(255, 240, 0), DS_ALIGN_CENTER);
    }
    if (mPage != CHALLENGE_PAGE_VS) {
        TodDrawImageScaledF(g, Sexy::IMAGE_TROPHY, 690.0f, 15.0f, 0.5f, 0.5f);
    }

    g->PushState();

    int scrollBarX = 760;
    int scrollBarY = 80;
    int scrollBarWidth = 40;
    int scrollBarHeight = 460;
    int scrollBarRectX = 766;
    int scrollBarRectY = 28;

    float scrollPosition = float(mScrollPosition);
    float scrollBarHeightFloat = float(scrollBarHeight);

    if (mScrollPosition == mScrollTargetPosition) {
        // 未滚动时的状态
        scrollBarHeightFloat = 448.0f;
        scrollBarHeight = 448;
        scrollBarY = 86;
    } else {
        // 滚动时的动画效果
        scrollPosition = TodAnimateCurveFloatTime(0.0f, 0.15f, mScrollAnimationTime, scrollPosition, mScrollTargetPosition, TodCurves::CURVE_LINEAR);
        scrollBarHeight = scrollBarHeight - 12;
        scrollBarY = scrollBarY + 6;
        scrollBarRectX = scrollBarX + 6;
        scrollBarRectY = scrollBarWidth - 12;
        scrollBarHeightFloat = float(scrollBarHeight);
    }

    int thumbPosition = int((scrollPosition / std::max(mPageChallengeCount, 1)) * scrollBarHeightFloat);
    int thumbHeight = int(scrollBarHeightFloat * (5.0f / std::max(mPageChallengeCount, 5)));

    int thumbY = scrollBarY + thumbPosition;
    int actualThumbHeight = (thumbHeight > scrollBarHeight) ? scrollBarHeight : thumbHeight;

    // 设置裁剪区域并绘制滚动条
    g->ClipRect(scrollBarRectX, scrollBarY, scrollBarRectY, scrollBarHeight);

    Color scrollBarBgColor(0, 128);
    g->SetColor(scrollBarBgColor);
    g->FillRect(Rect(scrollBarX + 6, scrollBarY + 6, scrollBarWidth - 12, scrollBarHeight - 12));

    Color scrollBarThumbColor(140, 140, 140, 255);
    g->SetColor(scrollBarThumbColor);
    g->FillRect(Rect(scrollBarRectX, thumbY, scrollBarRectY, actualThumbHeight));

    g->ClearClipRect();
    g->SetColorizeImages(false);
    g->DrawImageBox(Rect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight), Sexy::IMAGE_DLG_SELECTORFRAME);

    g->PopState();

    g->PushState();
    g->ClipRect(-20, 80, 1000, 475);
    g->TranslateF(0.0f, -(scrollPosition * 120.0f));

    if (mPageChallengeCount > 0) {
        for (int i = 0; i < mPageChallengeCount; ++i) {
            DrawButton(g, GetPageChallengeIndex(i), i);
        }
    }

    g->ClearClipRect();

    if (mToolTip) {
        mToolTip->Draw(g);
    }

    g->PopState();


    if (IsNetplayChallengePage(mPage)) {

        Color aColor = Color(0, 205, 0, 255);

        if (gIsReplayMode) {
            if (gNetDelayNow == 0) {
                TodDrawString(g, "[REPLAY]", 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            } else {
                TodDrawString(g, StrFormat("[REPLAY] %dms", gNetDelayNow * 10), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            }
        } else if (gIsServerModeSpectator) {

            if (gNetDelayNow == 0) {
                pvzstl::string status = TodStringTranslate(gIsServerModeSpectator ? "[SPECTATE]" : "[VS_STATUS_IN_ROOM]");
                TodDrawString(g, GetServerModeTransportSuffix() + std::move(status), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            } else {
                pvzstl::string fmt = TodStringTranslate("[VS_STATUS_IN_ROOM_MS_FMT]");
                pvzstl::string delayText = gIsServerModeSpectator ? StrFormat("%s %dms", TodStringTranslate("[SPECTATE]").c_str(), gNetDelayNow * 10) : StrFormat(fmt.c_str(), gNetDelayNow * 10);
                TodDrawString(g, GetServerModeTransportSuffix() + std::move(delayText), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            }
        } else if (IsRemoteClient()) {
            if (gNetDelayNow == 0) {
                TodDrawString(g, GetServerModeTransportSuffix() + TodStringTranslate("[VS_STATUS_IN_ROOM]"), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            } else {
                pvzstl::string fmt = TodStringTranslate("[VS_STATUS_IN_ROOM_MS_FMT]");
                TodDrawString(g, GetServerModeTransportSuffix() + StrFormat(fmt.c_str(), gNetDelayNow * 10), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            }
        } else if (IsRemoteServer()) {
            if (gNetDelayNow == 0) {
                TodDrawString(g, GetServerModeTransportSuffix() + TodStringTranslate("[VS_STATUS_HOST]"), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            } else {
                pvzstl::string fmt = TodStringTranslate("[VS_STATUS_HOST_MS_FMT]");
                TodDrawString(g, GetServerModeTransportSuffix() + StrFormat(fmt.c_str(), gNetDelayNow * 10), 400, -20, Sexy::FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_CENTER);
            }
        }

        if (!gIsServerModeSpectator && !gIsReplayMode && gChallengeScreenRequestState != 0) {
            // ======================
            // 我是 guest：已提醒房主...
            // (gTcpConnected == true 代表我作为 client 连接到 host)
            // ======================


            if (IsRemoteClient()) {
                pvzstl::string fmt = TodStringTranslate("[CHALLENGESCREEN_TIP_REMIND_HOST_FMT]");
                pvzstl::string name = GetNetplayModeName(gChallengeScreenRequestState);


                TodDrawString(g, StrFormat(fmt.c_str(), name.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153, 255), DrawStringJustification::DS_ALIGN_LEFT);
            }

            // ======================
            // 我是 host：对方想玩/想要...
            // (IsRemoteServer() 表示我作为 host 收到了 client 连接)
            // ======================
            if (IsRemoteServer()) {
                pvzstl::string fmt = TodStringTranslate("[CHALLENGESCREEN_TIP_OPPONENT_WANTS_PLAY_FMT]");
                pvzstl::string name = GetNetplayModeName(gChallengeScreenRequestState);
                TodDrawString(g, StrFormat(fmt.c_str(), name.c_str()), 140, 620, Sexy::FONT_HOUSEOFTERROR28, Color(255, 255, 153, 255), DrawStringJustification::DS_ALIGN_LEFT);
            }
        }
    }
}

void ChallengeScreen::Update() {
    // 记录当前游戏状态
    Widget::Update();
    UpdateToolTip();
    if (mUnlockStateCounter > 0) {
        mUnlockStateCounter--;
    }
    if (mUnlockState == UnlockingState::UNLOCK_SHAKING) {
        if (mUnlockStateCounter > 0) {
            mLockShakeX = RandRangeFloat(-2.0f, 2.0f);
            mLockShakeY = RandRangeFloat(-2.0f, 2.0f);
        } else {
            mApp->PlayFoley(FoleyType::FOLEY_PAPER);
            mUnlockState = UnlockingState::UNLOCK_FADING;
            mUnlockStateCounter = 50;
            mLockShakeX = 0.0f;
            mLockShakeY = 0.0f;
        }
    } else if (mUnlockState == UnlockingState::UNLOCK_FADING && mUnlockStateCounter == 0) {
        mUnlockState = UnlockingState::UNLOCK_OFF;
        mUnlockChallengeIndex = -1;
    }
    MarkDirty();
    for (int i = 0; i < mPageChallengeCount; i++) {
        float &aAnimTime = GetPageChallengeAnimTime(i);
        aAnimTime = GetPageChallengeIndex(i) == mSelectedChallengeIndex ? std::min(aAnimTime + 0.016f, 0.25f) : std::max(aAnimTime - 0.016f, 0.0f);
    }
    mScrollAnimationTime += 0.016f;
    if (mScrollAnimationTime > 0.15f) {
        mScrollPosition = mScrollTargetPosition;
    }

    if (IsNetplayChallengePage(mPage)) {
        if (!gNetplayLobbyFinished && mNetplayLobbyWidget == nullptr && mApp->mHelpTextScreen == nullptr && !IsRemoteClient() && !IsRemoteServer()) {
            mNetplayLobbyWidget = new NetplayLobbyWidget(mApp, mPage == ChallengePage::CHALLENGE_PAGE_COOP);
            AddWidget(mNetplayLobbyWidget);
            VSSetupAddonWidget::ResetGlobalBpState();
            if (gChallengeScreenOpenReplayManage) {
                gChallengeScreenOpenReplayManage = false;
                mNetplayLobbyWidget->SetMode(UIMode::MODE3_SERVER);
                mNetplayLobbyWidget->OpenReplayManageWidget();
            }
        }

        if (mNetplayLobbyWidget != nullptr && mNetplayLobbyWidget->mCloseRequested) {
            const int aButtonId = mNetplayLobbyWidget->mResult;
            gNetplayLobbyFinished = true;
            RemoveWidget(mNetplayLobbyWidget);
            delete mNetplayLobbyWidget;
            mNetplayLobbyWidget = nullptr;
            if (aButtonId == NetplayLobbyWidget::NetplayLobbyWidget_BackResult) {
                LawnApp *aApp = mApp;
                aApp->KillChallengeScreen();
                aApp->ShowGameSelector();
                return;
            }
        }
    }
}

void ChallengeScreen::AddedToManager(WidgetManager *theWidgetManager) {
    WidgetContainer::AddedToManager(theWidgetManager);
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        if (auto *aButton = GetChallengeButton(i)) {
            AddWidget(aButton);
        }
    }
    reinterpret_cast<void (*)(LawnApp *)>(LawnApp_ShowHelpBarWidgetAddr)(mApp);

    AddWidget(mBackButton);
    if (gIsReplayMode) {
        mBackButton->mDisabled = true;
        mBackButton->mBtnNoDraw = true;
    }
}

void ChallengeScreen::RemovedFromManager(WidgetManager *theWidgetManager) {
    if (mNetplayLobbyWidget != nullptr && mNetplayLobbyWidget->mParent == this) {
        RemoveWidget(mNetplayLobbyWidget);
    }
    RemoveWidget(mBackButton);

    WidgetContainer::RemovedFromManager(theWidgetManager);
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        if (auto *aButton = GetChallengeButton(i)) {
            RemoveWidget(aButton);
        }
    }
    mApp->HideHelpBarWidget();
}

void ChallengeScreen::ButtonPress(int theButtonId) {
    // 空函数替换，去除原有的点击进入关卡的功能
}

void ChallengeScreen::ButtonDepress(int theId) {
    if (theId == ChallengeScreen::ChallengeScreen_Back) {
        if (gIsReplayMode) {
            return;
        }
        LawnApp *aApp = mApp;
        aApp->KillChallengeScreen();
        aApp->DoBackToMain();
        return;
    }

    int aChallengeMode = theId - ChallengeScreen::ChallengeScreen_Mode;
    if (aChallengeMode >= 0 && aChallengeMode < NUM_CHALLENGE_MODES) {
        mSelectedChallengeIndex = aChallengeMode;
        mSelectedGameMode = GetChallengeDefinition(aChallengeMode).mChallengeMode;
        KeyDown(KEYCODE_RETURN);
        return;
    }

    int aPageIndex = theId - ChallengeScreen::ChallengeScreen_Page;
    if (aPageIndex >= 0 && aPageIndex < MAX_CHALLANGE_PAGES) {
        mPage = (ChallengePage)aPageIndex;
        UpdateButtons();
    }
}

void ChallengeScreen::UpdateButtons() {
    for (int i = 0; i < NUM_CHALLENGE_MODES; i++) {
        if (auto *aButton = GetChallengeButton(i)) {
            aButton->mVisible = GetChallengeDefinition(i).mPage == mPage;
        }
    }
    // Keep the existing touch UI's policy: do not automatically select the first entry.
}

void ChallengeScreen::DrawButton(Graphics *g, int theChallengeIndex, int theChallengeMode) {
    ButtonWidget *aButton = GetChallengeButton(theChallengeIndex);
    if (aButton == nullptr || !aButton->mVisible) {
        return;
    }
    const ChallengeDefinition &aDef = GetChallengeDefinition(theChallengeIndex);
    g->PushState();
    const float aAnimTime = GetPageChallengeAnimTime(theChallengeMode);
    g->Translate(int(TodAnimateCurveFloatTime(0.0f, 0.25f, aAnimTime, 0.0f, -25.0f, CURVE_EASE_OUT)), 0);
    int aPosX = aButton->mX;
    int aPosY = aButton->mY;
    if (aButton->mIsDown) {
        aPosX += 4;
        aPosY += 4;
    }
    if (aButton->mDisabled || AccomplishmentsNeeded(theChallengeIndex) > 1) {
        g->SetColor(Color(92, 92, 92));
        g->SetColorizeImages(true);
    }
    if (theChallengeIndex == mUnlockChallengeIndex) {
        if (mUnlockState == UnlockingState::UNLOCK_SHAKING) {
            g->SetColor(Color(92, 92, 92));
        } else if (mUnlockState == UnlockingState::UNLOCK_FADING) {
            int aColor = TodAnimateCurve(50, 25, mUnlockStateCounter, 92, 255, CURVE_LINEAR);
            g->SetColor(Color(aColor, aColor, aColor));
        }
        g->SetColorizeImages(true);
    }
    // TV's page value 5 is the VS page here, not the extended page-count sentinel.
    const bool aSurvivalThumbnail = mPage == CHALLENGE_PAGE_SURVIVAL || mPage == CHALLENGE_PAGE_VS
        || (mPage == CHALLENGE_PAGE_COOP && aDef.mChallengeMode != GAMEMODE_TWO_PLAYER_COOP_BOWLING && aDef.mChallengeMode != GAMEMODE_TWO_PLAYER_COOP_BOSS
            && aDef.mChallengeMode != GAMEMODE_TWO_PLAYER_COOP_BOSS_HARD);
    Image *aThumbnail = aSurvivalThumbnail ? Sexy::IMAGE_SURVIVAL_THUMBNAILS : Sexy::IMAGE_CHALLENGE_THUMBNAILS;
    const int aCelWidth = aThumbnail->GetCelWidth();
    const int aCelHeight = aThumbnail->GetCelHeight();
    Rect aSourceRect((aDef.mChallengeIconIndex % aThumbnail->mNumCols) * aCelWidth, (aDef.mChallengeIconIndex / aThumbnail->mNumCols) * aCelHeight, aCelWidth, aCelHeight);
    g->DrawImage(aThumbnail, Rect(aPosX + 6, aPosY + 2, 104, 104), aSourceRect);
    g->SetColorizeImages(false);
    g->DrawImage(Sexy::IMAGE_CHALLENGE_NAME_BACK, aPosX - 6, aPosY - 2, 720, 118);
    Font *aFont = Sexy::FONT_DWARVENTODCRAFT24;
    Color aTextColor = mSelectedChallengeIndex == theChallengeIndex && theChallengeIndex != mUnlockChallengeIndex ? Color(22, 221, 45) : Color::White;
    pvzstl::string aName = TodStringTranslate(aDef.mChallengeName);
    const bool aShaking = theChallengeIndex == mUnlockChallengeIndex && mUnlockState == UnlockingState::UNLOCK_SHAKING;
    const bool aLocked = aShaking || aButton->mDisabled;
    if (aLocked) {
        aName = TodStringTranslate("[MODE_LOCKED]");
        aFont = Sexy::FONT_HOUSEOFTERROR28;
        if (aShaking) {
            aTextColor = Color::White;
        }
    }
    Rect aNameRect(aPosX + 150 + (aLocked ? -40 : 0), aPosY + 60 + (aLocked ? 20 : 0) - aFont->GetHeight(), 550, 100);
    g->PushState();
    TodDrawStringWrappedHelper(g, aName, aNameRect, aFont, aTextColor, aLocked ? DS_ALIGN_CENTER : DS_ALIGN_LEFT, true, false);
    g->PopState();

    // Record storage is still the native profile array, not the button capacity.
    const int aRecordIndex = int(aDef.mChallengeMode) - GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
    const int aRecord = mPage != CHALLENGE_PAGE_VS && aRecordIndex >= 0 && aRecordIndex < int(std::size(mApp->mPlayerInfo->mChallengeRecords)) ? mApp->mPlayerInfo->mChallengeRecords[aRecordIndex] : 0;
    if (theChallengeIndex == mUnlockChallengeIndex) {
        if (mUnlockState == UnlockingState::UNLOCK_FADING) {
            g->SetColor(Color(255, 255, 255, TodAnimateCurve(25, 0, mUnlockStateCounter, 255, 0, CURVE_LINEAR)));
            g->SetColorizeImages(true);
            g->DrawImage(Sexy::IMAGE_LOCK, int(aPosX + 10 + mLockShakeX), int(aPosY + 10 + mLockShakeY));
        } else {
            g->DrawImage(Sexy::IMAGE_LOCK, aPosX + 10, aPosY + 10);
        }
        g->SetColorizeImages(false);
    } else if (aRecord > 0) {
        if (mApp->HasBeatenChallenge(aDef.mChallengeMode)) {
            g->DrawImage(Sexy::IMAGE_MINIGAME_TROPHY, aPosX - 6, aPosY - 2);
        } else if (mApp->IsEndlessScaryPotter(aDef.mChallengeMode) || mApp->IsEndlessIZombie(aDef.mChallengeMode)) {
            pvzstl::string aAchievement = TodReplaceNumberString("[LONGEST_STREAK]", "{STREAK}", aRecord);
            Rect aRect(aPosX, aPosY + 15, 120, 50);
            TodDrawStringWrapped(g, aAchievement, aRect, Sexy::FONT_CONTINUUMBOLD14OUTLINE, Color::White, DS_ALIGN_CENTER_VERTICAL_MIDDLE, false);
            TodDrawStringWrapped(g, aAchievement, aRect, Sexy::FONT_CONTINUUMBOLD14, Color(255, 0, 0), DS_ALIGN_CENTER_VERTICAL_MIDDLE, false);
        } else if ((aDef.mChallengeMode >= GAMEMODE_SURVIVAL_NORMAL_STAGE_1 && aDef.mChallengeMode <= GAMEMODE_SURVIVAL_ENDLESS_STAGE_5)
                   || (aDef.mChallengeMode >= GAMEMODE_TWO_PLAYER_COOP_DAY && aDef.mChallengeMode <= GAMEMODE_TWO_PLAYER_COOP_ROOF)
                   || (aDef.mChallengeMode >= GAMEMODE_TWO_PLAYER_COOP_DAY_HARD && aDef.mChallengeMode <= GAMEMODE_TWO_PLAYER_COOP_ROOF_HARD)
                   || aDef.mChallengeMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS) {
            pvzstl::string aAchievement = aRecord == 1 ? TodStringTranslate("[ONE_FLAG]") : TodReplaceNumberString("[COUNT_FLAGS]", "{COUNT}", aRecord);
            TodDrawString(g, aAchievement, aPosX + 48, aPosY + 48, Sexy::FONT_CONTINUUMBOLD14OUTLINE, Color::White, DS_ALIGN_CENTER);
            TodDrawString(g, aAchievement, aPosX + 48, aPosY + 48, Sexy::FONT_CONTINUUMBOLD14, Color(255, 0, 0), DS_ALIGN_CENTER);
        }
    } else if (aButton->mDisabled) {
        g->DrawImage(Sexy::IMAGE_LOCK, aPosX + 10, aPosY + 10);
    }
    g->PopState();
}

namespace {
int gChallengeScreenTouchDownX;
int gChallengeScreenTouchDownY;
int gChallengeItemHeight;
int gChallengeScreenGameIndex;
bool gChallengeItemMoved;
bool gTouchOutSide;

constexpr int mPageTop = 75;
constexpr int mPageBottom = 555;
} // namespace

void ChallengeScreen::MouseDown(int x, int y, int theClickCount) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (y > mPageBottom || y < mPageTop) {
        gTouchOutSide = true;
    }
    gChallengeScreenTouchDownX = x;
    gChallengeScreenTouchDownY = y;
    gChallengeItemHeight = (Sexy::IMAGE_CHALLENGE_NAME_BACK)->GetHeight() + 2; // 2为缝隙大小

    gChallengeScreenGameIndex = mScrollPosition;

    // int totalGamesInThisPage = a[376];//如果这个值是33
    // int currentSelectedGameIndex = ChallengeScreen_GetCurrentSelectedGameIndex(
    // a);//这里取值就是0~32。种子雨是32。

    // int firstGameInPageIndex = a->mScrollPosition;
    // int firstGameInPageIndex2 = a[186];
    // a->mSelectedChallengeIndex = a[currentSelectedGameIndex + 1 + 188];//向下移动绿色光标，不可循环滚动
    // a->mSelectedChallengeIndex = a[currentSelectedGameIndex - 1 + 188];//向上移动绿色光标，不可循环滚动

    // LOGD("dOWN:%d %d %d %d", x, y, firstGameInPageIndex, firstGameInPageIndex2);
}

void ChallengeScreen::MouseDrag(int x, int y) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        return;
    }
    if (gTouchOutSide) {
        return;
    }
    int triggerHeight = gChallengeItemHeight / 2; // 调节此处以修改小游戏列表的滚动速度。滚动太快就会有BUG，好烦。
    if (gChallengeScreenTouchDownY - y > triggerHeight) {
        int totalGamesInThisPage = mPageChallengeCount;
        gChallengeScreenGameIndex += 1;
        gChallengeScreenTouchDownY -= triggerHeight;
        int gameIndexToScroll = gChallengeScreenGameIndex >= totalGamesInThisPage - 4 ? totalGamesInThisPage - 4 : gChallengeScreenGameIndex;
        SetScrollTarget(gameIndexToScroll);
        // ChallengeScreen_UpdateButtons(a);
        gChallengeItemMoved = true;
    } else if (y - gChallengeScreenTouchDownY > triggerHeight) {
        gChallengeScreenGameIndex -= 1;
        gChallengeScreenTouchDownY += triggerHeight;
        int gameIndexToScroll = gChallengeScreenGameIndex <= 0 ? 0 : gChallengeScreenGameIndex;
        SetScrollTarget(gameIndexToScroll);
        // ChallengeScreen_UpdateButtons(a);
        gChallengeItemMoved = true;
    }
}

void ChallengeScreen::MouseUp(int x, int y) {
    if (gIsServerModeSpectator || gIsReplayMode) {
        gTouchOutSide = false;
        gChallengeItemMoved = false;
        return;
    }
    if (!gTouchOutSide && !gChallengeItemMoved) {
        if (gChallengeItemHeight <= 0) {
            LOG_WARN("[ChallengeScreen] invalid item height={}", gChallengeItemHeight);
            gTouchOutSide = false;
            gChallengeItemMoved = false;
            return;
        }
        int gameIndex = mScrollPosition + (y - mPageTop) / gChallengeItemHeight;
        if (gameIndex < 0 || gameIndex >= mPageChallengeCount) {
            LOG_WARN("[ChallengeScreen] drop MouseUp out-of-range gameIndex={} top={} y={} total={}", gameIndex, mScrollPosition, y, mPageChallengeCount);
            gTouchOutSide = false;
            gChallengeItemMoved = false;
            return;
        }
        int nextSelection = GetPageChallengeIndex(gameIndex);
        int nextMode = SelectionToNetplayMode(mPage, nextSelection);
        if (IsNetplayChallengePage(mPage) && !IsValidModeForPage(mPage, nextMode)) {
            LOG_WARN("[ChallengeScreen] drop MouseUp invalid netplay mode={} selection={} page={} gameIndex={}", nextMode, nextSelection, int(mPage), gameIndex);
            gTouchOutSide = false;
            gChallengeItemMoved = false;
            return;
        }
        if (mSelectedChallengeIndex == nextSelection) {
            KeyDown(Sexy::KEYCODE_RETURN);
        } else {
            mApp->PlaySample(Sexy::SOUND_BUTTONCLICK);
            if (IsRemoteClient()) {
                // 房客
                U16_Event event = {{EventType::EVENT_CLIENT_CHALLENGESCREEN_SELECT_MODE}, uint16_t(nextMode)};
                netplay::PutEvent(event);
                gChallengeScreenRequestState = nextMode;
            } else if (IsRemoteServer()) {
                // 房主
                mSelectedChallengeIndex = nextSelection;
                if (mPage == ChallengePage::CHALLENGE_PAGE_COOP) {
                    mSelectedGameMode = GameMode(nextMode);
                }
                U16_Event event = {{EventType::EVENT_SERVER_CHALLENGESCREEN_SELECT_MODE}, uint16_t(nextMode)};
                netplay::PutEvent(event);
            } else {
                // 单机
                mSelectedChallengeIndex = nextSelection;
            }
        }
    }
    gTouchOutSide = false;
    gChallengeItemMoved = false;
}

void ChallengeScreen::KeyDown(Sexy::KeyCode theKey) {
    if (gIsReplayMode && (theKey == Sexy::KEYCODE_BACK || theKey == Sexy::KEYCODE_ESCAPE || theKey == Sexy::KEYCODE_GAMEPAD_B)) {
        return;
    }
    if (theKey == Sexy::KEYCODE_RETURN && IsNetplayChallengePage(mPage)) {
        const int selectedMode = SelectionToNetplayMode(mPage, mSelectedChallengeIndex);
        if (!IsValidModeForPage(mPage, selectedMode)) {
            LOG_WARN("[ChallengeScreen] drop KeyDown invalid netplay mode={} selection={} page={}", selectedMode, int(mSelectedChallengeIndex), int(mPage));
            return;
        }

        if (IsRemoteClient()) {
            U16_Event event = {{EventType::EVENT_CLIENT_CHALLENGESCREEN_SELECT_MODE}, uint16_t(selectedMode)};
            netplay::PutEvent(event);
            gChallengeScreenRequestState = selectedMode;
            return;
        }

        if (IsRemoteServer()) {
            U16_Event event = {{EventType::EVENT_SERVER_CHALLENGESCREEN_BUTTON_DEPRESS}, uint16_t(selectedMode)};
            netplay::PutEvent(event);
        }
    }

    KeyDown_Origin(theKey);
}

void ChallengeScreen::KeyDown_Origin(Sexy::KeyCode theKey) {
    if (mPageChallengeCount <= 0) {
        if (theKey == KEYCODE_ESCAPE) {
            LawnApp *aApp = mApp;
            aApp->KillChallengeScreen();
            aApp->DoBackToMain();
        }
        return;
    }
    int aSelectedIndex = 0;
    for (int i = 0; i < mPageChallengeCount; i++) {
        if (GetPageChallengeIndex(i) == mSelectedChallengeIndex) {
            aSelectedIndex = i;
        }
    }
    if (theKey == KEYCODE_UP) {
        if (mScrollPosition == mScrollTargetPosition) {
            if (aSelectedIndex > 0) {
                aSelectedIndex--;
                if (mScrollPosition > aSelectedIndex) {
                    SetScrollTarget(mScrollPosition - 2);
                }
            } else {
                aSelectedIndex = mPageChallengeCount - 1;
                SetScrollTarget(((mPageChallengeCount - 1) / 4) * 4);
            }
        }
        mApp->PlaySample(Sexy::SOUND_BLEEP);
    } else if (theKey == KEYCODE_DOWN) {
        if (mScrollPosition == mScrollTargetPosition) {
            aSelectedIndex++;
            if (aSelectedIndex >= mPageChallengeCount) {
                aSelectedIndex = 0;
                SetScrollTarget(0);
            } else if (aSelectedIndex > mScrollPosition + 3) {
                SetScrollTarget(mScrollPosition + 2);
            }
        }
        mApp->PlaySample(Sexy::SOUND_BLEEP);
    }
    mSelectedChallengeIndex = GetPageChallengeIndex(aSelectedIndex);
    mSelectedGameMode = GetChallengeDefinition(mSelectedChallengeIndex).mChallengeMode;
    if (theKey == Sexy::KEYCODE_RETURN) {
        // 更新对战战场选择
        if (mPage == ChallengePage::CHALLENGE_PAGE_VS) {
            const int aMode = SelectionToNetplayMode(mPage, mSelectedChallengeIndex);
            if (gChallengeScreenRequestState == aMode) {
                gChallengeScreenRequestState = 0;
            }

            switch (aMode) {
                case GAMEMODE_MP_VS_DAY:
                    gVSBackground = BackgroundType::BACKGROUND_1_DAY;
                    break;
                case GAMEMODE_MP_VS_NIGHT:
                    gVSBackground = BackgroundType::BACKGROUND_2_NIGHT;
                    break;
                case GAMEMODE_MP_VS_POOL_DAY:
                    gVSBackground = BackgroundType::BACKGROUND_3_POOL;
                    break;
                case GAMEMODE_MP_VS_POOL_NIGHT:
                    gVSBackground = BackgroundType::BACKGROUND_4_FOG;
                    break;
                case GAMEMODE_MP_VS_ROOF:
                    gVSBackground = BackgroundType::BACKGROUND_5_ROOF;
                    break;
                case GAMEMODE_MP_VS_SHUFFLE_MODE:
                    gVSBackground = BackgroundType::BACKGROUND_1_DAY;
                    Challenge::msVSShuffleMode = true;
                    break;
                default:
                    break;
            }
        }
    }
    //    LOG_DEBUG("IN");
    if (theKey == KEYCODE_RETURN) {
        ButtonWidget *aButton = GetChallengeButton(mSelectedChallengeIndex);
        if (aButton == nullptr) {
            return;
        }
        if (aButton->mDisabled) {
            if (AccomplishmentsNeeded(mSelectedChallengeIndex) <= 1) {
                mApp->PlaySample(Sexy::SOUND_CERAMIC);
                mApp->GetVTable()->DoDialog(mApp, 75, true, "[MODE_LOCKED]", mToolTip->mLabel, "[OK]", 3);
            } else {
                mApp->PlaySample(Sexy::SOUND_BUZZER);
            }
            return;
        }
        LawnApp *aApp = mApp;
        const GameMode aMode = mSelectedGameMode;
        aApp->PlaySample(Sexy::SOUND_CERAMIC);
        aApp->KillChallengeScreen();
        aApp->PreNewGame(aMode, aMode != GAMEMODE_MP_VS);
        return;
    }
    if (theKey == KEYCODE_ESCAPE) {
        LawnApp *aApp = mApp;
        aApp->KillChallengeScreen();
        aApp->DoBackToMain();
        return;
    }
}

void ChallengeScreen::processClientEvent(const BaseEvent *event) const {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_CLIENT_CHALLENGESCREEN_SELECT_MODE: {
            auto *eventButtonDepress = static_cast<const U16_Event *>(event);
            if (IsNetplayChallengePage(mPage) && !IsValidModeForPage(mPage, eventButtonDepress->data)) {
                LOG_WARN("[ChallengeScreen] ignore invalid client netplay request mode={} page={}", eventButtonDepress->data, int(mPage));
                break;
            }
            gChallengeScreenRequestState = eventButtonDepress->data;
        } break;

        default:
            break;
    }
}

void ChallengeScreen::processServerEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_SERVER_CHALLENGESCREEN_BUTTON_DEPRESS: {
            auto *eventBtnDepress = static_cast<const U16_Event *>(event);
            int theId = eventBtnDepress->data;
            if (!IsValidModeForPage(mPage, theId)) {
                LOG_WARN("[ChallengeScreen] ignore invalid netplay button depress mode={} page={}", theId, int(mPage));
                break;
            }
            const int aSelection = NetplayModeToSelection(mPage, theId);
            if (aSelection < 0) {
                LOG_WARN("[ChallengeScreen] no definition for netplay button depress mode={} page={}", theId, int(mPage));
                break;
            }
            mSelectedChallengeIndex = aSelection;
            mSelectedGameMode = GetChallengeDefinition(aSelection).mChallengeMode;
            if (gChallengeScreenRequestState == theId) {
                gChallengeScreenRequestState = 0;
            }

            if (mPage == ChallengePage::CHALLENGE_PAGE_COOP) {
                KeyDown_Origin(Sexy::KEYCODE_RETURN);
                return;
            }

            switch (theId) {
                case GAMEMODE_MP_VS_DAY:
                    gVSBackground = BackgroundType::BACKGROUND_1_DAY;
                    break;
                case GAMEMODE_MP_VS_NIGHT:
                    gVSBackground = BackgroundType::BACKGROUND_2_NIGHT;
                    break;
                case GAMEMODE_MP_VS_POOL_DAY:
                    gVSBackground = BackgroundType::BACKGROUND_3_POOL;
                    break;
                case GAMEMODE_MP_VS_POOL_NIGHT:
                    gVSBackground = BackgroundType::BACKGROUND_4_FOG;
                    break;
                case GAMEMODE_MP_VS_ROOF:
                    gVSBackground = BackgroundType::BACKGROUND_5_ROOF;
                    break;
                case GAMEMODE_MP_VS_SHUFFLE_MODE:
                    gVSBackground = BackgroundType::BACKGROUND_1_DAY;
                    Challenge::msVSShuffleMode = true;
                    break;
                default:
                    break;
            }
            LawnApp *app = mApp;
            app->KillChallengeScreen();
            app->PreNewGame(GAMEMODE_MP_VS, false);
            return;
        } break;
        case EVENT_SERVER_CHALLENGESCREEN_SELECT_MODE: {
            auto *event1 = static_cast<const U16_Event *>(event);
            int theId = event1->data;
            if (!IsValidModeForPage(mPage, theId)) {
                LOG_WARN("[ChallengeScreen] ignore invalid netplay select mode={} page={}", theId, int(mPage));
                break;
            }
            const int aSelection = NetplayModeToSelection(mPage, theId);
            if (aSelection < 0) {
                LOG_WARN("[ChallengeScreen] no definition for netplay select mode={} page={}", theId, int(mPage));
                break;
            }
            mSelectedChallengeIndex = aSelection;
            mSelectedGameMode = GetChallengeDefinition(aSelection).mChallengeMode;
        } break;
        default:
            break;
    }
}
