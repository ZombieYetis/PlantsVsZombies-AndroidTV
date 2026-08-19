#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_STRATEGY_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_STRATEGY_H

#include "VSActionAIPlacement.h"

#include <cstdint>
#include <array>
#include <memory>

namespace vsai::detail {

enum class StrategyDatabaseLoadState : std::uint8_t {
    Uninitialized,
    Unavailable,
    Invalid,
    Loaded,
};

int StrategyBucket(int value);
int StrategyBonus(const VSGameState &state, VSSide side, SeedType seed, int targetRow);
StrategyDatabaseLoadState GetStrategyDatabaseLoadState();
void ResetStrategyDatabase();

// Strategy thresholds use this instead of raw producer/grave counts. Enhanced
// local AI treats an established economy as one step further developed while
// leaving physical board state and zero-economy openings unchanged.
int EffectiveAIEconomyCount(VSSide side, int actualCount);

// These masks are replay/deck archetype features, not live tactical card
// roles or draft carry eligibility. They are stable features used by the
// replay extractor.
// They intentionally describe counterable plans rather than exact six-card
// deck hashes, so a Ban replacement still selects the correct response.
constexpr std::uint16_t kZombieDeckFastPressure = 1U << 0;
constexpr std::uint16_t kZombieDeckMetalScreen = 1U << 1;
constexpr std::uint16_t kZombieDeckVehicle = 1U << 2;
constexpr std::uint16_t kZombieDeckEconomy = 1U << 3;
constexpr std::uint16_t kZombieDeckRangedSiege = 1U << 4;
constexpr std::uint16_t kZombieDeckRaid = 1U << 5;
constexpr std::uint16_t kZombieDeckJump = 1U << 6;
constexpr std::uint16_t kZombieDeckHeavy = 1U << 7;
constexpr std::uint16_t kZombieDeckSwarm = 1U << 8;

std::uint16_t DeckArchetype(const VSGameState &state, VSSide side);
bool HasZombieDeckArchetype(const VSGameState &state, std::uint16_t mask);

// Bounded, rules-based response prior.  Unlike StrategyBonus this is not a
// replay action frequency: it encodes the answer that remains valid when a
// card is Banned or the live board has not exposed a unit yet.
int ZombieDeckCounterBonus(const VSGameState &state, SeedType seed, int targetRow);
bool IsAreaCounterSeed(SeedType seed);
int ReadyPlantAreaCounterCount(const VSGameState &state);
int PlantAreaCounterExposure(const VSGameState &state, int row);
bool IsZombieBreakthroughSeed(SeedType seed);
bool HasReadyZombieBreakthroughCard(const VSGameState &state);
bool IsHeavyZombieSeed(SeedType seed);
bool IsZombieGraveGuardSeed(SeedType seed);
bool HasZombieGraveGuardInRow(const VSGameState &state, int row);

class BuiltinVSAgent : public IVSAgent {
protected:
    std::uint16_t mSequence = 0;
    std::array<std::uint8_t, 32> mBlockedSlots{};

    void AdvanceBlockedSlots() {
        for (std::uint8_t &blocked : mBlockedSlots) {
            if (blocked > 0) {
                --blocked;
            }
        }
    }

    bool IsSlotBlocked(std::uint8_t slot) const {
        return slot < mBlockedSlots.size() && mBlockedSlots[slot] != 0;
    }

    VSAction MakePlayAction(VSSide side, const VSCardState &card, VSGridPosition target, std::uint32_t tick) {
        return {
            .side = side,
            .kind = VSActionKind::PlaySeed,
            .seedSlot = card.slot,
            .expectedSeedType = card.seedType,
            .target = target,
            .notBeforeTick = tick,
            .expiresAtTick = tick + 120,
            .sequence = ++mSequence,
        };
    }

    VSAction MakeShovelAction(VSGridPosition target, std::uint32_t tick) {
        return {
            .side = VSSide::Plants,
            .kind = VSActionKind::Shovel,
            .target = target,
            .notBeforeTick = tick,
            .expiresAtTick = tick + 120,
            .sequence = ++mSequence,
        };
    }

    VSAction MakeCollectResourceAction(VSSide side, std::uint32_t objectId) {
        return {
            .side = side,
            .kind = VSActionKind::CollectResource,
            .objectId = objectId,
            .sequence = ++mSequence,
        };
    }

public:
    void Reset() override {
        mSequence = 0;
        mBlockedSlots.fill(0);
    }

    void OnActionResult(const VSAction &action, VSActionResult result) override {
        if (result == VSActionResult::RejectedInvalidTarget || result == VSActionResult::RejectedUnsupported || result == VSActionResult::RejectedCardUnavailable) {
            if (action.seedSlot < mBlockedSlots.size()) {
                mBlockedSlots[action.seedSlot] = 4;
            }
        }
    }
};

std::unique_ptr<IVSAgent> CreatePlantAI();
std::unique_ptr<IVSAgent> CreateZombieAI();

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_STRATEGY_H
