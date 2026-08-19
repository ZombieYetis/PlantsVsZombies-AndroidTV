#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_CARD_RULES_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_CARD_RULES_H

#include "PvZ/Lawn/Common/ConstEnums.h"

#include <cstdint>

namespace vsai::detail {

// Live tactical roles used after a card is in the active deck. Draft carry
// eligibility and replay deck archetypes intentionally use separate rules.
enum class VSCardRole : std::uint32_t {
    None = 0,
    PlantOneShot = 1U << 0,
    PlantImmediateCounter = 1U << 1,
    PlantAreaCounter = 1U << 2,
    PlantCombat = 1U << 3,
    PlantSustainedOutput = 1U << 4,
    ZombieEconomy = 1U << 5,
    ZombieTargeted = 1U << 6,
    ZombieHeavy = 1U << 7,
    ZombieGraveGuard = 1U << 8,
    ZombieFastAttack = 1U << 9,
    ZombieLobbedScreen = 1U << 10,
    ZombieBreakthrough = 1U << 11,
};

constexpr VSCardRole operator|(VSCardRole left, VSCardRole right) {
    return static_cast<VSCardRole>(static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right));
}

constexpr bool HasCardRole(VSCardRole roles, VSCardRole role) {
    return (static_cast<std::uint32_t>(roles) & static_cast<std::uint32_t>(role)) != 0;
}

VSCardRole PlantCardRoles(SeedType seed);
VSCardRole ZombieCardRoles(SeedType seed);

inline bool HasPlantCardRole(SeedType seed, VSCardRole role) {
    return HasCardRole(PlantCardRoles(seed), role);
}

inline bool HasZombieCardRole(SeedType seed, VSCardRole role) {
    return HasCardRole(ZombieCardRoles(seed), role);
}

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_CARD_RULES_H
