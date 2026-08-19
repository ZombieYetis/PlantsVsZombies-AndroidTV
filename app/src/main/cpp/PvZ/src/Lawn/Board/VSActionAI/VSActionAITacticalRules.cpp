#include "VSActionAITacticalRules.h"
#include "VSActionAICardRules.h"

#include <algorithm>

#include "PvZ/Lawn/Board/Plant.h"

namespace vsai::detail {

bool IsPlantOneShotSeed(SeedType seed) {
    return HasPlantCardRole(seed, VSCardRole::PlantOneShot);
}

bool IsPlantImmediateCounterSeed(SeedType seed) {
    return seed == SeedType::SEED_IMP_PEAR || HasPlantCardRole(seed, VSCardRole::PlantImmediateCounter);
}

bool CanPumpkinShellTarget(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_PUMPKINSHELL:
        case SeedType::SEED_SPIKEWEED:
        case SeedType::SEED_SPIKEROCK:
        case SeedType::SEED_POTATOMINE:
            return false;
        default:
            return true;
    }
}

bool IsSquashTargetZombie(const VSZombieState &zombie) {
    switch (static_cast<ZombieType>(zombie.zombieType)) {
        case ZombieType::ZOMBIE_BOBSLED:
        case ZombieType::ZOMBIE_ZAMBONI:
        case ZombieType::ZOMBIE_YETI:
        case ZombieType::ZOMBIE_DIGGER:
            return false;
        default:
            return true;
    }
}

bool CanChillyPepperAffect(const VSZombieState &zombie) {
    return zombie.canBeFrozen;
}

bool IsBungeeTargetEligible(const VSGameState &state, const VSPlantState &plant) {
    if (IsDeadOrOutside(plant) || IsPlantOneShotSeed(static_cast<SeedType>(plant.seedType)) || IsPlantProtectedByUmbrella(state, plant.position)) {
        return false;
    }

    switch (static_cast<SeedType>(plant.seedType)) {
        case SeedType::SEED_WALLNUT:
        case SeedType::SEED_TALLNUT:
        case SeedType::SEED_PUMPKINSHELL:
        case SeedType::SEED_UMBRELLA:
        case SeedType::SEED_GARLIC:
        case SeedType::SEED_SPIKEWEED:
        case SeedType::SEED_SPIKEROCK:
        case SeedType::SEED_IMP_PEAR:
            return false;
        default:
            break;
    }

    return std::max(0, Plant::GetCost(static_cast<SeedType>(plant.seedType), SeedType::SEED_NONE)) >= 100;
}

} // namespace vsai::detail
