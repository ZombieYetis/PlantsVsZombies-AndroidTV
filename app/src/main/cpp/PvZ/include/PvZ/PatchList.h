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

#ifndef PVZ_PATCHLIST_H
#define PVZ_PATCHLIST_H

#include "Homura/MemoryUtils.h"

class PatchList {
public:
    homura::Patcher autoPickupSeedPacketDisable; // 禁止光标自动拾取植物卡片

    [[nodiscard]] static PatchList &GetInstance() {
        static PatchList instance;
        return instance;
    }

    PatchList(const PatchList &) = delete;
    PatchList(PatchList &&) = delete;
    PatchList &operator=(const PatchList &) = delete;
    PatchList &operator=(PatchList &&) = delete;

protected:
    PatchList() = default;
    ~PatchList() = default;
};

[[nodiscard]] inline PatchList &GetPatchList() {
    return PatchList::GetInstance();
}

inline void ApplyPatches() {
    constexpr auto *libGameMain = "libGameMain.so";

    homura::Patcher whackAZombieNormalSpeed; // 锤僵尸关卡的僵尸速度恢复为原速
    homura::Patcher repairShopA;             // 破解商店
    homura::Patcher repairShopB;             // 破解商店

#if PVZ_VERSION == 111
    whackAZombieNormalSpeed = homura::Patcher::CreateWithStr(libGameMain, 0x183448, "4F F0 01 00");
    repairShopA = homura::Patcher::CreateWithStr(libGameMain, 0x1C3B06, "05 E0");
    repairShopB = homura::Patcher::CreateWithStr(libGameMain, 0x1C3C6C, "06 E0");
    GetPatchList().autoPickupSeedPacketDisable = homura::Patcher::CreateWithStr(libGameMain, 0x1C6068, "16");
#elif PVZ_VERSION == 115
    whackAZombieNormalSpeed = homura::Patcher::CreateWithStr(libGameMain, 0x1814F0, "4F F0 01 00");
    repairShopA = homura::Patcher::CreateWithStr(libGameMain, 0x1C1BB6, "05 E0");
    repairShopB = homura::Patcher::CreateWithStr(libGameMain, 0x1C1D1C, "06 E0");
    GetPatchList().autoPickupSeedPacketDisable = homura::Patcher::CreateWithStr(libGameMain, 0x1C4114, "16");
#endif // PVZ_VERSION

    whackAZombieNormalSpeed.Modify();
    repairShopA.Modify();
    repairShopB.Modify();
}

#endif // PVZ_PATCHLIST_H
