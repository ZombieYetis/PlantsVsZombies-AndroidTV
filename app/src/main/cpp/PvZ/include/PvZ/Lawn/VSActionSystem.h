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

#ifndef PVZ_LAWN_VSACTION_SYSTEM_H
#define PVZ_LAWN_VSACTION_SYSTEM_H

#include "PvZ/NetPlay.h"

#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <vector>

class Board;

namespace vsai {

inline constexpr std::uint16_t kAnySeedType = UINT16_MAX;

enum class VSSide : std::uint8_t {
    Plants = 0,
    Zombies = 1,
};

enum class VSActionKind : std::uint8_t {
    PlaySeed = 0,
    Shovel = 1,
    FireCobCannon = 2,
    CollectResource = 3,
    Concede = 4,
};

enum class VSActionResult : std::uint8_t {
    Queued = 0,
    Applied,
    Deferred,
    RejectedDisabled,
    RejectedNotLocalVS,
    RejectedMatchNotPlaying,
    RejectedMatchPaused,
    RejectedStale,
    RejectedUnsupported,
    RejectedInvalidSide,
    RejectedInvalidCard,
    RejectedCardUnavailable,
    RejectedInsufficientResource,
    RejectedInvalidTarget,
};

struct VSGridPosition {
    std::int8_t col = -1;
    std::int8_t row = -1;
};

struct VSAction {
    VSSide side = VSSide::Plants;
    VSActionKind kind = VSActionKind::PlaySeed;
    std::uint8_t seedSlot = 0;
    std::uint16_t expectedSeedType = kAnySeedType;
    // Plant ID for FireCobCannon, coin ID for CollectResource, otherwise zero.
    std::uint32_t objectId = 0;
    VSGridPosition target{};
    std::uint32_t notBeforeTick = 0;
    std::uint32_t expiresAtTick = 0;
    std::uint16_t sequence = 0;
};

struct VSCardState {
    std::uint8_t slot = 0;
    std::uint16_t seedType = 0;
    std::uint16_t imitaterType = 0;
    int cost = 0;
    int refreshCounter = 0;
    int refreshTime = 0;
    bool active = false;
    bool refreshing = false;
    // Disabled by a sudden-death rule even though the SeedPacket itself can
    // still report active.
    bool matchRestricted = false;
};

struct VSPlantState {
    std::uint32_t id = 0;
    std::uint16_t seedType = 0;
    std::uint16_t state = 0;
    VSGridPosition position{};
    int health = 0;
    int maxHealth = 0;
    bool asleep = false;
    bool dead = false;
};

struct VSZombieState {
    std::uint32_t id = 0;
    std::uint16_t zombieType = 0;
    std::int8_t row = -1;
    float positionX = 0.0f;
    float positionY = 0.0f;
    int bodyHealth = 0;
    int bodyMaxHealth = 0;
    int shieldHealth = 0;
    bool eating = false;
    bool mindControlled = false;
    // Snapshot the engine's current freeze eligibility. Chilly Pepper has a
    // wind-up before its row hit, so planning must not value units such as a
    // sled team or a tunneling Digger that the engine cannot freeze or harm.
    bool canBeFrozen = false;
    // The exact plant currently intersecting a Jalapeno Head's chew rect.
    // It lets the detached plant AI evacuate that plant before the row burn
    // is armed, without approximating engine collision geometry.
    std::uint32_t jalapenoContactPlantId = 0;
    // The first chewable plant within the AI's five-pixel earlier warning
    // window. The engine retains its normal 20-pixel burn trigger.
    std::uint32_t jalapenoPreContactPlantId = 0;
    // Explorer carries an active torch while this is true. Iceberg Lettuce
    // can safely extinguish it; disposable Sun-shrooms must not be fed into it.
    bool explorerTorchLit = false;
    // A Bungee may only be damaged after it has reached its target tile.
    // Other zombie types leave this false.
    bool bungeeAtTarget = false;
    bool dead = false;
};

struct VSGridItemState {
    std::uint32_t id = 0;
    std::uint16_t gridItemType = 0;
    VSGridPosition position{};
    int health = 0;
    int level = 0;
    bool dead = false;
};

struct VSResourceState {
    std::uint32_t id = 0;
    VSSide side = VSSide::Plants;
    std::uint16_t coinType = 0;
    int value = 0;
    float positionX = 0.0f;
    float positionY = 0.0f;
    bool beingCollected = false;
    bool dead = false;
};

struct VSGameState {
    std::uint32_t boardTick = 0;
    int columns = 9;
    int rows = 5;
    int plantSun = 0;
    int zombieBrains = 0;
    // Board-owned count of currently surviving zombie targets. It remains
    // valid after the death animation removes an old target grid item.
    int liveZombieTargetCount = 0;
    bool isNight = false;
    bool isSuddenDeath = false;
    bool resourceProductionDisabled = false;
    bool playing = false;
    bool paused = false;
    // Captured with Board::CanPlantAt for a normal ground plant. This keeps
    // terrain rules such as Zomboni ice and Doom-shroom craters available to
    // the detached algorithmic AI without exposing Board itself to agents.
    std::array<std::array<bool, 6>, 6> basePlantableCells{};
    // A ready mower is the final recovery resource for an overrun lane.
    std::array<bool, 6> mowerAvailable{};
    // A triggered mower is still clearing its row. It is distinct from an
    // already-spent mower, whose row can become a later breakthrough route.
    std::array<bool, 6> mowerInMotion{};
    std::array<std::vector<VSCardState>, 2> seedBanks;
    std::vector<VSPlantState> plants;
    std::vector<VSZombieState> zombies;
    std::vector<VSGridItemState> gridItems;
    std::vector<VSResourceState> resources;
};

class IVSAgent {
public:
    virtual ~IVSAgent() = default;
    virtual void Reset() {}
    virtual std::optional<VSAction> Decide(const VSGameState &state) = 0;
    virtual void OnActionResult(const VSAction &action, VSActionResult result) {}
};

// Recorded only in local replay files. This event is never sent through NetPlay.
struct VSLocalActionReplayEvent : BaseEvent {
    std::uint8_t side = 0;
    std::uint8_t kind = 0;
    std::uint8_t seedSlot = 0;
    std::uint8_t reserved = 0;
    std::uint16_t expectedSeedType = kAnySeedType;
    std::uint32_t objectId = 0;
    std::int8_t col = -1;
    std::int8_t row = -1;
    std::uint16_t sequence = 0;
    std::uint32_t notBeforeTick = 0;
    std::uint32_t expiresAtTick = 0;
};

// An agent owns only decision logic. It must not mutate Board or input state directly.
void SetAgent(VSSide side, std::unique_ptr<IVSAgent> agent);
void ClearAgent(VSSide side);
IVSAgent *GetAgent(VSSide side);
void SetThinkIntervalTicks(VSSide side, std::uint32_t ticks);
std::uint32_t GetThinkIntervalTicks(VSSide side);

bool IsSideEnabled(VSSide side);
bool IsEnhancedAIEnabled();
// The boost is intentionally confined to local non-replay VS matches so it
// cannot change the deterministic economy of online or replay sessions.
bool HasEnhancedAIProduction(Board *board, VSSide side);
int ScaleEnhancedAIProductionCooldown(int cooldown);
// Returns a point-in-time copy of all state exposed to traditional algorithmic AI.
VSGameState BuildGameState(Board *board);
// Queued actions are executed by Update when their tick window opens.
bool EnqueueAction(const VSAction &action);
// Executes a semantic action immediately. It is intentionally usable without enabling an AI checkbox.
VSActionResult ExecuteActionNow(Board *board, const VSAction &action);
void Update(Board *board);
void Reset();
void ExecuteReplayAction(Board *board, const VSLocalActionReplayEvent &event);

} // namespace vsai

#endif // PVZ_LAWN_VSACTION_SYSTEM_H
