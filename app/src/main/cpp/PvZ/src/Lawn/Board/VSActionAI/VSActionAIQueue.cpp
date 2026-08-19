#include "VSActionAIQueue.h"

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
