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

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_QUEUE_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_QUEUE_H

#include "PvZ/Lawn/VSActionSystem.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace vsai::detail {

struct QueuedVSAction {
    VSAction action;
    std::optional<VSSide> sourceSide;
};

struct VSActionQueuePoll {
    std::vector<QueuedVSAction> expired;
    std::optional<QueuedVSAction> ready;
};

class VSActionQueue {
public:
    static constexpr std::size_t kMaxActions = 64;

    bool IsFull() const;
    bool Enqueue(VSAction action, std::optional<VSSide> sourceSide);
    void RemoveActionsFrom(VSSide side);
    void Clear();
    VSActionQueuePoll TakeNextReady(std::uint32_t tick);

private:
    std::vector<QueuedVSAction> mActions;
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_QUEUE_H
