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

#include "PvZ/Lawn/System/PlayerInfo.h"
#include "PvZ/Lawn/LawnApp.h"

#include <algorithm>
#include <iterator>
#include <random>

void LawnPlayerInfo::EnsureNetplayPlayerId() {
    if (std::any_of(std::begin(mNetplayPlayerId), std::end(mNetplayPlayerId), [](unsigned char value) { return value != 0; }))
        return;

    std::random_device random;
    for (auto &value : mNetplayPlayerId)
        value = static_cast<unsigned char>(random());
    mNetplayPlayerId[6] = (mNetplayPlayerId[6] & 0x0f) | 0x40;
    mNetplayPlayerId[8] = (mNetplayPlayerId[8] & 0x3f) | 0x80;
    SaveDetails();
}

void LawnPlayerInfo::AddCoins(int theAmount) {
    // 用于 购物愉快 成就
    int aNumCoin = mCoins + theAmount;
    if (aNumCoin > 99999) {
        aNumCoin = 99999;
    }
    if (aNumCoin <= 0) {
        aNumCoin = 0;
    }
    mCoins = aNumCoin;
    if (theAmount < 0) {
        mUsedCoins -= theAmount;
        if (mUsedCoins >= 2500) {
            gLawnApp->GrantAchievement(AchievementType::ACHIEVEMENT_SHOP);
        }
    }
}