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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIQueue.h"

#include <algorithm>
#include <utility>

namespace vsai::detail {
namespace {

    bool IsTickBefore(std::uint32_t tick, std::uint32_t deadline) {
        return static_cast<std::int32_t>(tick - deadline) < 0;
    }

    bool IsActionExpired(const VSAction &action, std::uint32_t tick) {
        return action.expiresAtTick != 0 && IsTickBefore(action.expiresAtTick, tick);
    }

    bool IsActionDeferred(const VSAction &action, std::uint32_t tick) {
        return action.notBeforeTick != 0 && IsTickBefore(tick, action.notBeforeTick);
    }

} // namespace

bool VSActionQueue::IsFull() const {
    return mActions.size() >= kMaxActions;
}

bool VSActionQueue::Enqueue(VSAction action, std::optional<VSSide> sourceSide) {
    if (IsFull()) {
        return false;
    }
    mActions.push_back({std::move(action), sourceSide});
    return true;
}

void VSActionQueue::RemoveActionsFrom(VSSide side) {
    std::erase_if(mActions, [side](const QueuedVSAction &queuedAction) { return queuedAction.sourceSide == side; });
}

void VSActionQueue::Clear() {
    mActions.clear();
}

VSActionQueuePoll VSActionQueue::TakeNextReady(std::uint32_t tick) {
    VSActionQueuePoll poll{};
    for (auto iterator = mActions.begin(); iterator != mActions.end();) {
        if (IsActionExpired(iterator->action, tick)) {
            poll.expired.push_back(std::move(*iterator));
            iterator = mActions.erase(iterator);
            continue;
        }
        if (!IsActionDeferred(iterator->action, tick)) {
            poll.ready = std::move(*iterator);
            mActions.erase(iterator);
            break;
        }
        ++iterator;
    }
    return poll;
}

} // namespace vsai::detail
