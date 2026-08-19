/*
 * Copyright (C) 2023-2026 PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#include "PvZ/Lawn/VSActionSystem.h"
#include "PvZ/Lawn/VSActionAIDecision.h"
#include "VSActionAIExecutor.h"
#include "VSActionAIGameState.h"
#include "VSActionAIPolicy.h"
#include "VSActionAIQueue.h"
#include "VSActionAIStrategy.h"

#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"
#include "PvZ/ReplaySystem.h"

#include <cstddef>
#include <algorithm>
#include <array>
#include <optional>
#include <utility>

namespace vsai {
namespace {

    constexpr std::size_t kSideCount = 2;
    constexpr std::uint32_t kDefaultThinkIntervalTicks = 10;

    struct RuntimeState {
        // This is the single local-match owner for a Board, its agents, timers,
        // and queued actions. Draft state remains in the chooser layer.
        Board *board = nullptr;
        std::array<std::unique_ptr<IVSAgent>, kSideCount> agents;
        std::array<bool, kSideCount> builtinAgents = {false, false};
        std::array<std::uint32_t, kSideCount> thinkIntervals = {kDefaultThinkIntervalTicks, kDefaultThinkIntervalTicks};
        std::array<std::uint32_t, kSideCount> nextThinkTicks = {0, 0};
        detail::VSActionQueue queuedActions;
        bool matchActive = false;
    };

    RuntimeState gRuntime;

    constexpr std::size_t SideIndex(VSSide side) {
        return static_cast<std::size_t>(side);
    }

    bool IsValidSide(VSSide side) {
        return SideIndex(side) < kSideCount;
    }

    bool IsTickBefore(std::uint32_t tick, std::uint32_t deadline) {
        return static_cast<std::int32_t>(tick - deadline) < 0;
    }

    bool IsActionExpired(const VSAction &action, std::uint32_t tick) {
        return action.expiresAtTick != 0 && IsTickBefore(action.expiresAtTick, tick);
    }

    bool IsActionDeferred(const VSAction &action, std::uint32_t tick) {
        return action.notBeforeTick != 0 && IsTickBefore(tick, action.notBeforeTick);
    }

    bool IsLocalVSMatch(const Board *board) {
        return board != nullptr && board->mApp != nullptr && board->mApp->IsVSMode() && !IsOnlineModeActive();
    }

    bool IsMatchPlaying(const Board *board) {
        return board != nullptr && board->mApp != nullptr && board->mApp->mGameScene == GameScenes::SCENE_PLAYING;
    }

    bool IsMatchPaused(const Board *board) {
        return board != nullptr && (board->mPaused || requestPause);
    }

    VSActionResult ExecuteAction(Board *board, const VSAction &action, bool replayExecution) {
        return detail::ExecuteBoardAction(board,
                                          action,
                                          {
                                              .replayExecution = replayExecution,
                                              .localVSMatch = IsLocalVSMatch(board),
                                              .matchPlaying = IsMatchPlaying(board),
                                              .matchPaused = IsMatchPaused(board),
                                          });
    }

    VSLocalActionReplayEvent MakeReplayEvent(const VSAction &action) {
        VSLocalActionReplayEvent event{};
        event.type = EventType::EVENT_LOCAL_BOARD_ACTION;
        event.size = static_cast<std::uint8_t>(sizeof(event));
        event.side = static_cast<std::uint8_t>(action.side);
        event.kind = static_cast<std::uint8_t>(action.kind);
        event.seedSlot = action.seedSlot;
        event.expectedSeedType = action.expectedSeedType;
        event.objectId = action.objectId;
        event.col = action.target.col;
        event.row = action.target.row;
        event.sequence = action.sequence;
        event.notBeforeTick = action.notBeforeTick;
        event.expiresAtTick = action.expiresAtTick;
        return event;
    }

    void RecordAppliedAction(Board *board, const VSAction &action) {
        if (board == nullptr || board->mApp == nullptr || gIsReplayMode) {
            return;
        }

        const VSLocalActionReplayEvent event = MakeReplayEvent(action);
        replay::RecordPacket(ReplayPacketDir::Outbound, reinterpret_cast<const std::byte *>(&event), sizeof(event), static_cast<std::uint32_t>(board->mApp->mAppCounter));
    }

    void Notify(IVSAgent *agent, const VSAction &action, VSActionResult result) {
        if (agent != nullptr) {
            agent->OnActionResult(action, result);
        }
    }

    void NotifySide(std::optional<VSSide> side, const VSAction &action, VSActionResult result) {
        if (side.has_value()) {
            Notify(GetAgent(*side), action, result);
        }
    }

    void ResetMatchRuntime(bool resetStrategyDatabase = false) {
        gRuntime.queuedActions.Clear();
        gRuntime.nextThinkTicks = {0, 0};
        gRuntime.matchActive = false;
        if (resetStrategyDatabase) {
            detail::ResetStrategyDatabase();
        }
        for (const std::unique_ptr<IVSAgent> &agent : gRuntime.agents) {
            if (agent != nullptr) {
                agent->Reset();
            }
        }
    }

    void ResetForBoard(Board *board) {
        if (gRuntime.board == board) {
            return;
        }

        gRuntime.board = board;
        ResetMatchRuntime();
    }

    void ExecuteQueuedAction(Board *board, const detail::QueuedVSAction &queuedAction) {
        if (queuedAction.sourceSide.has_value() && !IsSideEnabled(*queuedAction.sourceSide)) {
            NotifySide(queuedAction.sourceSide, queuedAction.action, VSActionResult::RejectedDisabled);
            return;
        }
        const VSActionResult result = ExecuteAction(board, queuedAction.action, false);
        if (result == VSActionResult::Applied) {
            RecordAppliedAction(board, queuedAction.action);
        }
        NotifySide(queuedAction.sourceSide, queuedAction.action, result);
    }

    void RunAgent(Board *board, VSSide side, const VSGameState &state) {
        const std::size_t index = SideIndex(side);
        IVSAgent *agent = gRuntime.agents[index].get();
        if (agent == nullptr || !IsSideEnabled(side)) {
            return;
        }

        const std::uint32_t tick = state.boardTick;
        if (IsTickBefore(tick, gRuntime.nextThinkTicks[index])) {
            return;
        }
        gRuntime.nextThinkTicks[index] = tick + gRuntime.thinkIntervals[index];

        std::optional<VSAction> action = agent->Decide(state);
        if (!action.has_value()) {
            return;
        }
        if (action->side != side) {
            Notify(agent, *action, VSActionResult::RejectedInvalidSide);
            return;
        }
        if (IsActionExpired(*action, tick)) {
            Notify(agent, *action, VSActionResult::RejectedStale);
            return;
        }
        if (IsActionDeferred(*action, tick)) {
            if (!gRuntime.queuedActions.Enqueue(*action, side)) {
                Notify(agent, *action, VSActionResult::RejectedUnsupported);
                return;
            }
            Notify(agent, *action, VSActionResult::Queued);
            return;
        }

        const VSActionResult result = ExecuteAction(board, *action, false);
        if (result == VSActionResult::Applied) {
            RecordAppliedAction(board, *action);
        }
        Notify(agent, *action, result);
    }

    void SyncBuiltinAgents() {
        const std::array<bool, kSideCount> enabled = {IsSideEnabled(VSSide::Plants), IsSideEnabled(VSSide::Zombies)};
        const std::size_t plantIndex = SideIndex(VSSide::Plants);
        const std::size_t zombieIndex = SideIndex(VSSide::Zombies);
        if (enabled[plantIndex] && gRuntime.agents[plantIndex] == nullptr) {
            gRuntime.agents[plantIndex] = CreateBuiltinVSAgent(VSSide::Plants);
            gRuntime.builtinAgents[plantIndex] = true;
        } else if (!enabled[plantIndex] && gRuntime.builtinAgents[plantIndex]) {
            gRuntime.agents[plantIndex].reset();
            gRuntime.builtinAgents[plantIndex] = false;
        }
        if (enabled[zombieIndex] && gRuntime.agents[zombieIndex] == nullptr) {
            gRuntime.agents[zombieIndex] = CreateBuiltinVSAgent(VSSide::Zombies);
            gRuntime.builtinAgents[zombieIndex] = true;
        } else if (!enabled[zombieIndex] && gRuntime.builtinAgents[zombieIndex]) {
            gRuntime.agents[zombieIndex].reset();
            gRuntime.builtinAgents[zombieIndex] = false;
        }
    }

} // namespace

void SetAgent(VSSide side, std::unique_ptr<IVSAgent> agent) {
    if (!IsValidSide(side)) {
        return;
    }

    const std::size_t index = SideIndex(side);
    gRuntime.queuedActions.RemoveActionsFrom(side);
    gRuntime.agents[index] = std::move(agent);
    gRuntime.builtinAgents[index] = false;
    gRuntime.nextThinkTicks[index] = 0;
    if (gRuntime.agents[index] != nullptr) {
        gRuntime.agents[index]->Reset();
    }
}

void ClearAgent(VSSide side) {
    SetAgent(side, nullptr);
}

IVSAgent *GetAgent(VSSide side) {
    return IsValidSide(side) ? gRuntime.agents[SideIndex(side)].get() : nullptr;
}

void SetThinkIntervalTicks(VSSide side, std::uint32_t ticks) {
    if (!IsValidSide(side)) {
        return;
    }
    gRuntime.thinkIntervals[SideIndex(side)] = std::max(ticks, std::uint32_t{1});
}

std::uint32_t GetThinkIntervalTicks(VSSide side) {
    return IsValidSide(side) ? gRuntime.thinkIntervals[SideIndex(side)] : 0;
}

bool IsSideEnabled(VSSide side) {
    switch (side) {
        case VSSide::Plants:
            return VSSetupAddonWidget::msPlantAIMode;
        case VSSide::Zombies:
            return VSSetupAddonWidget::msZombieAIMode;
    }
    return false;
}

bool IsEnhancedAIEnabled() {
    return VSSetupAddonWidget::msAIEnhancementMode;
}

bool HasEnhancedAIProduction(Board *board, VSSide side) {
    return IsLocalVSMatch(board) && !gIsReplayMode && VSSetupAddonWidget::msAIEnhancementMode && IsSideEnabled(side);
}

int ScaleEnhancedAIProductionCooldown(int cooldown) {
    return detail::AIEnhancementPolicy{.enabled = IsEnhancedAIEnabled()}.ScaleProductionCooldown(cooldown);
}

VSGameState BuildGameState(Board *board) {
    return detail::BuildGameStateSnapshot(board);
}

bool EnqueueAction(const VSAction &action) {
    if (!IsValidSide(action.side)) {
        return false;
    }
    return gRuntime.queuedActions.Enqueue(action, std::nullopt);
}

VSActionResult ExecuteActionNow(Board *board, const VSAction &action) {
    const VSActionResult result = ExecuteAction(board, action, false);
    if (result == VSActionResult::Applied) {
        RecordAppliedAction(board, action);
    }
    return result;
}

void Update(Board *board) {
    ResetForBoard(board);
    SyncBuiltinAgents();
    if (!IsLocalVSMatch(board) || !IsMatchPlaying(board)) {
        if (gRuntime.matchActive) {
            ResetMatchRuntime();
        }
        return;
    }

    if (!gRuntime.matchActive) {
        ResetMatchRuntime();
        gRuntime.matchActive = true;
    }

    // Pause freezes the board simulation, so AI actions and deferred queues
    // must remain untouched until the same board resumes.
    if (IsMatchPaused(board)) {
        return;
    }

    const std::uint32_t tick = static_cast<std::uint32_t>(board->mMainCounter);
    std::optional<VSSide> actionProcessedForSide;
    detail::VSActionQueuePoll queuePoll = gRuntime.queuedActions.TakeNextReady(tick);
    for (const detail::QueuedVSAction &expiredAction : queuePoll.expired) {
        NotifySide(expiredAction.sourceSide, expiredAction.action, VSActionResult::RejectedStale);
    }
    if (queuePoll.ready.has_value()) {
        ExecuteQueuedAction(board, *queuePoll.ready);
        actionProcessedForSide = queuePoll.ready->action.side;
    }

    if (actionProcessedForSide != VSSide::Plants) {
        RunAgent(board, VSSide::Plants, BuildGameState(board));
    }
    if (IsMatchPlaying(board) && actionProcessedForSide != VSSide::Zombies) {
        RunAgent(board, VSSide::Zombies, BuildGameState(board));
    }
}

void Reset() {
    gRuntime.board = nullptr;
    ResetMatchRuntime(true);
}

void ExecuteReplayAction(Board *board, const VSLocalActionReplayEvent &event) {
    if (event.size != sizeof(VSLocalActionReplayEvent)) {
        return;
    }

    const VSAction action{
        .side = static_cast<VSSide>(event.side),
        .kind = static_cast<VSActionKind>(event.kind),
        .seedSlot = event.seedSlot,
        .expectedSeedType = event.expectedSeedType,
        .objectId = event.objectId,
        .target = {event.col, event.row},
        .notBeforeTick = event.notBeforeTick,
        .expiresAtTick = event.expiresAtTick,
        .sequence = event.sequence,
    };
    ExecuteAction(board, action, true);
}

} // namespace vsai
