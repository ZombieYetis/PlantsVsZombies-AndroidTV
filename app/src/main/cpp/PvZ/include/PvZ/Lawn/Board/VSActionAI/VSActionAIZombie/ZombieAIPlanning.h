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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_AI_PLANNING_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_AI_PLANNING_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIZombie/ZombieDecisionContext.h"

#include <array>
#include <optional>

namespace vsai::detail {

class ZombieAIPlanning : public BuiltinVSAgent {
protected:
    static bool HasLobbedPlantInRow(const VSGameState &state, int row);
    const VSCardState *FindReadyCard(const VSGameState &state, SeedType seedType) const;

    // Match-local pressure cadence. Reset starts a fresh opening; applied
    // actions update attack rows, grave progress, and lane cooldowns.
    int mLastAttackRow = -1;
    int mLastPressureEconomyCount = -1;
    std::array<std::uint8_t, 6> mLaneAttackCooldown{};
    bool mOpeningEconomyPlaced = false;

    int HeavyZombieReserve(const VSGameState &state) const;
    bool HasReadyFrontlineProbe(const VSGameState &state) const;
    bool HasReadyEarlyHeavyCommit(const VSGameState &state, const ZombieDecisionContext &context) const;
    ZombieDecisionContext BuildDecisionContext(const VSGameState &state) const;
    std::optional<VSAction> TryTemplateSundayRelease(const VSGameState &state, const ZombieDecisionContext &context);
    bool IsEarlyHeavyCommitCard(const VSGameState &state, SeedType seed, const ZombieDecisionContext &context) const;
    std::optional<VSAction> TryBuildEconomy(const VSGameState &state, int row);
    std::optional<VSAction> TryProtectEconomy(const VSGameState &state, int row, bool force = false);
    std::optional<VSAction> TryCounterLobbedGravePressure(const VSGameState &state, const ZombieDecisionContext &context, int row);
    std::optional<VSGridPosition> FindTargetForCard(const VSGameState &state, const VSCardState &card, int row) const;
    int LeastCommittedZombieRow(const VSGameState &state) const;
    static int BungeeTargetScore(const VSGameState &state, const VSPlantState &plant, int row);
    static int CardScore(const VSCardState &card, const VSGameState &state, const ZombieDecisionContext &context, int targetRow, int effectiveCost);

public:
    void Reset() override;
    void OnActionResult(const VSAction &action, VSActionResult result) override;
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_ZOMBIE_AI_PLANNING_H
