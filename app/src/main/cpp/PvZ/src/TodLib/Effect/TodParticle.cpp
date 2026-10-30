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

#include "PvZ/TodLib/Effect/TodParticle.h"
#include "Homura/Logger.h"
#include "PvZ/SexyAppFramework/SexyAppBase.h"

#include <algorithm>
#include <array>

namespace {

// 与 ParticleEffect 扩展区一一对应；文件名不包含最后的 .compiled。
constexpr std::array<ParticleParams, EXTENDED_NUM_PARTICLES - NUM_PARTICLES> gExtendedParticleParamArray = {{
    {PARTICLE_RED_STINGER_SPLAT, "addonFiles/compiled/particles/RedStingerSplat.xml"},
}};

static_assert(NUM_PARTICLES == 108);
static_assert(sizeof(ParticleParams) == 8);
static_assert(sizeof(TodParticleDefinition) == 8);
static_assert(
    [] {
        for (std::size_t i = 0; i < gExtendedParticleParamArray.size(); ++i) {
            const ParticleParams &aParam = gExtendedParticleParamArray[i];
            if (aParam.mParticleEffect != NUM_PARTICLES + static_cast<int>(i) || aParam.mParticleFileName == nullptr || aParam.mParticleFileName[0] == '\0') {
                return false;
            }
        }
        return true;
    }(),
    "Extended particle definitions must match their enum order and have a file name");

// 原加载器会保存参数表指针，所以合并表必须在加载完成后仍然有效。
// 粒子定义仍由原加载器分配，并由原 TodParticleFreeDefinitions 按实际数量释放。
std::array<ParticleParams, EXTENDED_NUM_PARTICLES> gMergedParticleParamArray{};

} // namespace

void TodParticleLoadDefinitions(ParticleParams *theParticleParamArray, int theParticleParamArraySize) {
    if (theParticleParamArraySize < 0 || (theParticleParamArraySize > 0 && theParticleParamArray == nullptr)) {
        LOG_ERROR("Invalid particle parameter table: count={}", theParticleParamArraySize);
        return;
    }

    // 只扩展完整的游戏参数表；空扩展表和编辑器的 (nullptr, 0) 调用直接透传。
    if (!gExtendedParticleParamArray.empty() && theParticleParamArraySize == NUM_PARTICLES) {
        std::copy_n(theParticleParamArray, NUM_PARTICLES, gMergedParticleParamArray.begin());
        std::ranges::copy(gExtendedParticleParamArray, gMergedParticleParamArray.begin() + NUM_PARTICLES);

        // 原加载器每读一个定义会增加 6 个完成任务，同步扩展总任务数。
        if (Sexy::gSexyAppBase != nullptr) {
            Sexy::gSexyAppBase->mNumLoadingThreadTasks += static_cast<int>(gExtendedParticleParamArray.size()) * 6;
        }
        theParticleParamArray = gMergedParticleParamArray.data();
        theParticleParamArraySize = static_cast<int>(gMergedParticleParamArray.size());
    } else if (!gExtendedParticleParamArray.empty() && theParticleParamArraySize > 0) {
        LOG_WARN("Skip particle extensions for a non-game table: count={}", theParticleParamArraySize);
    }
    old_TodParticleLoadDefinitions(theParticleParamArray, theParticleParamArraySize);
}
