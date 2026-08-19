#include "VSActionAICardRules.h"

namespace vsai::detail {

VSCardRole PlantCardRoles(SeedType seed) {
    VSCardRole roles = VSCardRole::None;
    switch (seed) {
        case SeedType::SEED_POTATOMINE:
            roles = roles | VSCardRole::PlantOneShot | VSCardRole::PlantImmediateCounter;
            break;
        case SeedType::SEED_SQUASH:
        case SeedType::SEED_CHERRYBOMB:
        case SeedType::SEED_JALAPENO:
        case SeedType::SEED_CHILLY_PEPPER:
        case SeedType::SEED_ICESHROOM:
        case SeedType::SEED_DOOMSHROOM:
            roles = roles | VSCardRole::PlantOneShot | VSCardRole::PlantImmediateCounter | VSCardRole::PlantAreaCounter;
            break;
        case SeedType::SEED_ICEBERG_LETTUCE:
        case SeedType::SEED_HYPNOSHROOM:
            roles = roles | VSCardRole::PlantOneShot | VSCardRole::PlantImmediateCounter;
            break;
        case SeedType::SEED_GRAVEBUSTER:
        case SeedType::SEED_BLOVER:
        case SeedType::SEED_TANGLEKELP:
            roles = roles | VSCardRole::PlantOneShot;
            break;
        default:
            break;
    }

    switch (seed) {
        case SeedType::SEED_SNOWPEA:
        case SeedType::SEED_PUFFSHROOM:
        case SeedType::SEED_SCAREDYSHROOM:
        case SeedType::SEED_BONK_CHOY:
        case SeedType::SEED_CELERY_STALKER:
        case SeedType::SEED_CHOMPER:
        case SeedType::SEED_STARFRUIT:
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_GLOOMSHROOM:
        case SeedType::SEED_SPORESHROOM:
        case SeedType::SEED_BLOOMERANG:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_WINTERMELON:
        case SeedType::SEED_GATLINGPEA:
        case SeedType::SEED_COBCANNON:
        case SeedType::SEED_SPIKEWEED:
        case SeedType::SEED_SPIKEROCK:
            roles = roles | VSCardRole::PlantCombat;
            break;
        default:
            break;
    }
    switch (seed) {
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_SNOWPEA:
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_BLOOMERANG:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_STARFRUIT:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_SPORESHROOM:
        case SeedType::SEED_GATLINGPEA:
        case SeedType::SEED_WINTERMELON:
        case SeedType::SEED_GLOOMSHROOM:
            roles = roles | VSCardRole::PlantSustainedOutput;
            break;
        default:
            break;
    }
    return roles;
}

VSCardRole ZombieCardRoles(SeedType seed) {
    VSCardRole roles = VSCardRole::None;
    switch (seed) {
        case SeedType::SEED_ZOMBIE_GRAVESTONE:
        case SeedType::SEED_ZOMBIE_MOUND:
            roles = roles | VSCardRole::ZombieEconomy;
            break;
        case SeedType::SEED_ZOMBIE_BUNGEE:
            roles = roles | VSCardRole::ZombieTargeted;
            break;
        case SeedType::SEED_ZOMBIE_BOBSLED:
        case SeedType::SEED_ZOMBONI:
        case SeedType::SEED_ZOMBIE_FOOTBALL:
        case SeedType::SEED_ZOMBIE_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
        case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
            roles = roles | VSCardRole::ZombieBreakthrough;
            break;
        default:
            break;
    }
    switch (seed) {
        case SeedType::SEED_ZOMBIE_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
        case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
            roles = roles | VSCardRole::ZombieHeavy;
            break;
        default:
            break;
    }
    switch (seed) {
        case SeedType::SEED_ZOMBIE_TRASHCAN:
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
        case SeedType::SEED_ZOMBIE_WALLNUT_HEAD:
        case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
        case SeedType::SEED_ZOMBIE_PAIL:
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
        case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
            roles = roles | VSCardRole::ZombieGraveGuard;
            break;
        default:
            break;
    }
    switch (seed) {
        case SeedType::SEED_ZOMBIE_NORMAL:
        case SeedType::SEED_ZOMBIE_IMP:
        case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
        case SeedType::SEED_ZOMBIE_DOGWALKER:
        case SeedType::SEED_ZOMBIE_FLAG:
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
            roles = roles | VSCardRole::ZombieFastAttack;
            break;
        default:
            break;
    }
    switch (seed) {
        case SeedType::SEED_ZOMBIE_TRASHCAN:
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
            roles = roles | VSCardRole::ZombieLobbedScreen;
            break;
        default:
            break;
    }
    return roles;
}

} // namespace vsai::detail
