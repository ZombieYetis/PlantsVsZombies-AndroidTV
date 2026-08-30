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

#include "PvZ/Lawn/Widget/VSActionAIDraftPolicy.h"

#include "PvZ/SexyAppFramework/Buffer.h"
#include "PvZ/SexyAppFramework/SexyAppBase.h"

#include <cstdint>
#include <algorithm>
#include <array>
#include <vector>

namespace vsai::draft {

namespace {

    struct BanRule {
        bool targetsZombies = false;
        SeedType seed = SeedType::SEED_NONE;
        int priority = 0;
    };

    constexpr std::array<unsigned char, 8> kBanDatabaseMagic = {'P', 'V', 'Z', 'V', 'B', 'A', 'N', '\0'};
    constexpr std::uint16_t kBanDatabaseVersion = 1;
    constexpr std::size_t kBanDatabaseHeaderSize = 12;
    constexpr std::size_t kBanDatabaseRuleSize = 7;
    constexpr std::uint32_t kBanDatabaseRetryIntervalTicks = 300;

    std::uint16_t ReadBanU16(const std::vector<unsigned char> &data, std::size_t offset) {
        return static_cast<std::uint16_t>(data[offset]) | (static_cast<std::uint16_t>(data[offset + 1]) << 8);
    }

    class BanDatabase {
        std::vector<BanRule> mRules;
        BanDatabaseLoadState mLoadState = BanDatabaseLoadState::Uninitialized;
        std::uint32_t mNextRetryTick = 0;

        bool ShouldRetryAt(std::uint32_t tick) const {
            return static_cast<std::int32_t>(tick - mNextRetryTick) >= 0;
        }

        void MarkUnavailable(std::uint32_t tick) {
            mLoadState = BanDatabaseLoadState::Unavailable;
            mNextRetryTick = tick + kBanDatabaseRetryIntervalTicks;
        }

        void Load(std::uint32_t tick) {
            if (mLoadState == BanDatabaseLoadState::Loaded || mLoadState == BanDatabaseLoadState::Invalid || (mLoadState == BanDatabaseLoadState::Unavailable && !ShouldRetryAt(tick))) {
                return;
            }
            if (Sexy::gSexyAppBase == nullptr) {
                MarkUnavailable(tick);
                return;
            }

            Sexy::Buffer buffer;
            if (!Sexy::gSexyAppBase->ReadBufferFromFile("addonFiles/data/vs_ai_ban_db.bin", &buffer, false)) {
                MarkUnavailable(tick);
                return;
            }

            const std::vector<unsigned char> &data = buffer.mData;
            if (data.size() < kBanDatabaseHeaderSize || !std::equal(kBanDatabaseMagic.begin(), kBanDatabaseMagic.end(), data.begin()) || ReadBanU16(data, 8) != kBanDatabaseVersion) {
                mLoadState = BanDatabaseLoadState::Invalid;
                return;
            }

            const std::size_t ruleCount = ReadBanU16(data, 10);
            if (ruleCount > (data.size() - kBanDatabaseHeaderSize) / kBanDatabaseRuleSize || kBanDatabaseHeaderSize + ruleCount * kBanDatabaseRuleSize != data.size()) {
                mLoadState = BanDatabaseLoadState::Invalid;
                return;
            }

            mRules.clear();
            for (std::size_t index = 0; index < ruleCount; ++index) {
                const std::size_t offset = kBanDatabaseHeaderSize + index * kBanDatabaseRuleSize;
                const unsigned char sideCode = data[offset];
                const int averageOrder = data[offset + 3];
                const int samples = data[offset + 4];
                const int priority = ReadBanU16(data, offset + 5);
                if (sideCode > 1 || averageOrder > 15 || samples <= 0 || priority <= 0 || priority > 1000) {
                    continue;
                }
                mRules.push_back({sideCode == 1, static_cast<SeedType>(ReadBanU16(data, offset + 1)), priority});
            }
            mLoadState = BanDatabaseLoadState::Loaded;
        }

    public:
        int Priority(bool targetsZombies, SeedType seed, std::uint32_t tick) {
            Load(tick);
            for (const BanRule &rule : mRules) {
                if (rule.targetsZombies == targetsZombies && rule.seed == seed) {
                    return rule.priority;
                }
            }
            return 0;
        }

        BanDatabaseLoadState LoadState() const {
            return mLoadState;
        }

        void Reset() {
            mRules.clear();
            mLoadState = BanDatabaseLoadState::Uninitialized;
            mNextRetryTick = 0;
        }
    };

    BanDatabase &GetBanDatabase() {
        static BanDatabase database;
        return database;
    }

} // namespace

int BanDatabasePriority(bool targetsZombies, SeedType seed, std::uint32_t tick) {
    return GetBanDatabase().Priority(targetsZombies, seed, tick);
}

BanDatabaseLoadState GetBanDatabaseLoadState() {
    return GetBanDatabase().LoadState();
}

void ResetBanDatabase() {
    GetBanDatabase().Reset();
}

BuiltinAIDraftSession &GetBuiltinAIDraftSession() {
    static BuiltinAIDraftSession session;
    return session;
}

void ResetBuiltinAIDraftSession() {
    GetBuiltinAIDraftSession() = {};
}

BuiltinAIDraftHistory &GetBuiltinAIDraftHistory() {
    static BuiltinAIDraftHistory history;
    return history;
}

int BanBaseThreat(bool targetsZombies, SeedType seed) {
    if (targetsZombies) {
        switch (seed) {
            case SeedType::SEED_ZOMBIE_GARGANTUAR:
            case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
            case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
                return 560;
            case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
                return 525;
            case SeedType::SEED_ZOMBIE_DANCER:
            case SeedType::SEED_ZOMBIE_BUNGEE:
            case SeedType::SEED_ZOMBIE_DIGGER:
                return 470;
            case SeedType::SEED_ZOMBIE_NORMAL:
                return 430;
            case SeedType::SEED_ZOMBIE_DOGWALKER:
                return 545;
            case SeedType::SEED_ZOMBONI:
            case SeedType::SEED_ZOMBIE_BOBSLED:
            case SeedType::SEED_ZOMBIE_PEA_HEAD:
            case SeedType::SEED_ZOMBIE_NEWSPAPER:
                return 390;
            case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
            case SeedType::SEED_ZOMBIE_PAIL:
            case SeedType::SEED_ZOMBIE_TRASHCAN:
            case SeedType::SEED_ZOMBIE_FOOTBALL:
                return 305;
            case SeedType::SEED_ZOMBIE_IMP:
            case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
            case SeedType::SEED_ZOMBIE_SQUASH_HEAD:
                return 240;
            default:
                return 90;
        }
    }

    switch (seed) {
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_REPEATER:
            return 560;
        case SeedType::SEED_POTATOMINE:
        case SeedType::SEED_SQUASH:
        case SeedType::SEED_CHERRYBOMB:
            return 535;
        case SeedType::SEED_SNOWPEA:
        case SeedType::SEED_SCAREDYSHROOM:
        case SeedType::SEED_MELONPULT:
            return 435;
        case SeedType::SEED_JALAPENO:
        case SeedType::SEED_CELERY_STALKER:
            return 405;
        case SeedType::SEED_STARFRUIT:
            return 400;
        case SeedType::SEED_BLOOMERANG:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_SPORESHROOM:
            return 295;
        case SeedType::SEED_ICESHROOM:
        case SeedType::SEED_HYPNOSHROOM:
        case SeedType::SEED_DOOMSHROOM:
        case SeedType::SEED_CHOMPER:
            return 270;
        case SeedType::SEED_KERNELPULT:
            return 185;
        case SeedType::SEED_WALLNUT:
        case SeedType::SEED_SUNSHROOM:
        case SeedType::SEED_ICEBERG_LETTUCE:
        case SeedType::SEED_PUMPKINSHELL:
            return 145;
        default:
            return 75;
    }
}

std::span<const SeedType> PlantBanPriority() {
    static constexpr SeedType kPriority[] = {
        SeedType::SEED_ZOMBIE_DOGWALKER,
        SeedType::SEED_ZOMBIE_NORMAL,
        SeedType::SEED_ZOMBIE_DANCER,
        SeedType::SEED_ZOMBIE_PEA_HEAD,
        SeedType::SEED_ZOMBIE_NEWSPAPER,
        SeedType::SEED_ZOMBIE_IMP,
        SeedType::SEED_ZOMBIE_SCREEN_DOOR,
        SeedType::SEED_ZOMBIE_BUNGEE,
        SeedType::SEED_ZOMBIE_DIGGER,
        SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER,
        SeedType::SEED_ZOMBIE_SUNDAY_EDITION,
        SeedType::SEED_ZOMBIE_GARGANTUAR,
        SeedType::SEED_ZOMBIE_POGO,
    };
    return kPriority;
}

std::span<const SeedType> ZombieBanPriority() {
    static constexpr SeedType kPriority[] = {
        SeedType::SEED_STARFRUIT,
        SeedType::SEED_REPEATER,
        SeedType::SEED_POTATOMINE,
        SeedType::SEED_CELERY_STALKER,
        SeedType::SEED_CHERRYBOMB,
        SeedType::SEED_JALAPENO,
        SeedType::SEED_SQUASH,
        SeedType::SEED_COBCANNON,
        SeedType::SEED_SNOWPEA,
        SeedType::SEED_PUMPKINSHELL,
        SeedType::SEED_ICEBERG_LETTUCE,
        SeedType::SEED_TORCHWOOD,
    };
    return kPriority;
}

bool IsPlantTempoMushroom(SeedType seed) {
    return seed == SeedType::SEED_PUFFSHROOM;
}

bool IsPlantCarrySeed(SeedType seed) {
    if (IsPlantTempoMushroom(seed)) {
        return false;
    }
    switch (seed) {
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_CACTUS:
        case SeedType::SEED_CABBAGEPULT:
        case SeedType::SEED_KERNELPULT:
        case SeedType::SEED_MELONPULT:
        case SeedType::SEED_BLOOMERANG:
        case SeedType::SEED_STARFRUIT:
        case SeedType::SEED_SCAREDYSHROOM:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_SPORESHROOM:
            return true;
        default:
            return false;
    }
}

bool IsPeaMainDamageSeed(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_PEASHOOTER:
        case SeedType::SEED_REPEATER:
        case SeedType::SEED_THREEPEATER:
        case SeedType::SEED_SPLITPEA:
        case SeedType::SEED_GATLINGPEA:
            return true;
        default:
            return false;
    }
}

bool IsCoffeeDependentPlant(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_PUFFSHROOM:
        case SeedType::SEED_SCAREDYSHROOM:
        case SeedType::SEED_FUMESHROOM:
        case SeedType::SEED_GLOOMSHROOM:
        case SeedType::SEED_SPORESHROOM:
        case SeedType::SEED_HYPNOSHROOM:
        case SeedType::SEED_ICESHROOM:
        case SeedType::SEED_DOOMSHROOM:
        case SeedType::SEED_MAGNETSHROOM:
            return true;
        default:
            return false;
    }
}

bool IsMagnetTargetZombieSeed(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_ZOMBIE_PAIL:
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
        case SeedType::SEED_ZOMBIE_FOOTBALL:
        case SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX:
        case SeedType::SEED_ZOMBIE_DIGGER:
        case SeedType::SEED_ZOMBIE_POGO:
        case SeedType::SEED_ZOMBIE_LADDER:
        case SeedType::SEED_ZOMBIE_TRASHCAN:
            return true;
        default:
            return false;
    }
}

} // namespace vsai::draft
