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

#include "VSActionAIStrategy.h"
#include "VSActionAICardRules.h"
#include "VSActionAIPolicy.h"

#include "PvZ/Lawn/VSActionSystem.h"
#include "PvZ/SexyAppFramework/Buffer.h"
#include "PvZ/SexyAppFramework/SexyAppBase.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <array>
#include <vector>

namespace vsai::detail {

int EffectiveAIEconomyCount(VSSide side, int actualCount) {
    return GetAIEnhancementPolicy(side).EffectiveEconomyCount(actualCount);
}

struct StrategyRule {
    VSSide side = VSSide::Plants;
    std::uint16_t seed = 0;
    std::uint32_t deckSignature = 0;
    std::uint16_t opponentArchetype = 0;
    int phase = 0;
    std::array<int, 4> buckets = {-1, -1, -1, -1};
    int bonus = 0;
    int samples = 0;
};

constexpr std::array<unsigned char, 8> kStrategyDatabaseMagic = {'P', 'V', 'Z', 'V', 'S', 'D', 'B', '\0'};
constexpr std::uint16_t kStrategyDatabaseVersion = 3;
constexpr std::uint16_t kPreviousStrategyDatabaseVersion = 2;
constexpr std::uint16_t kLegacyStrategyDatabaseVersion = 1;
constexpr std::size_t kStrategyDatabaseHeaderSize = 12;
constexpr std::size_t kLegacyStrategyDatabaseRuleSize = 12;
constexpr std::size_t kPreviousStrategyDatabaseRuleSize = 16;
constexpr std::size_t kStrategyDatabaseRuleSize = 18;
constexpr std::uint32_t kStrategyDatabaseRetryIntervalTicks = 300;

std::uint16_t ReadStrategyU16(const std::vector<unsigned char> &data, std::size_t offset) {
    return static_cast<std::uint16_t>(data[offset]) | (static_cast<std::uint16_t>(data[offset + 1]) << 8);
}

std::uint32_t ReadStrategyU32(const std::vector<unsigned char> &data, std::size_t offset) {
    return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8) | (static_cast<std::uint32_t>(data[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

std::uint32_t DeckSignature(const VSGameState &state, VSSide side) {
    const std::size_t sideIndex = side == VSSide::Plants ? 0 : 1;
    std::vector<std::uint16_t> seeds;
    for (const VSCardState &card : state.seedBanks[sideIndex]) {
        if (!card.active || card.matchRestricted || card.seedType == static_cast<std::uint16_t>(SeedType::SEED_NONE)) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        // Economy cards are baseline slots, not the tactical identity of a
        // replay template. Some recordings omit them from metadata while the
        // local chooser includes them, so omit them from both hash builders.
        if ((side == VSSide::Plants && (seed == SeedType::SEED_SUNFLOWER || seed == SeedType::SEED_SUNSHROOM)) || (side == VSSide::Zombies && seed == SeedType::SEED_ZOMBIE_GRAVESTONE)) {
            continue;
        }
        seeds.push_back(card.seedType);
    }
    std::sort(seeds.begin(), seeds.end());

    // FNV-1a over sorted 16-bit seed ids. Keep this byte-for-byte aligned
    // with the external replay extractor: deck order is UI noise, whereas
    // its card composition identifies the tactical template.
    std::uint32_t value = 2166136261U;
    for (const std::uint16_t seed : seeds) {
        for (const unsigned char byte : {static_cast<unsigned char>(seed & 0xFF), static_cast<unsigned char>((seed >> 8) & 0xFF)}) {
            value ^= byte;
            value *= 16777619U;
        }
    }
    value ^= static_cast<std::uint32_t>(seeds.size());
    return value * 16777619U;
}

// These bits are shared with parse_replay.py. They describe the tactical
// identity that survives a Ban replacement, unlike a full deck hash.
constexpr std::uint16_t kArchetypeDirectFire = 1U << 0;
constexpr std::uint16_t kArchetypeLobbedFire = 1U << 1;
constexpr std::uint16_t kArchetypeMushroomControl = 1U << 2;
constexpr std::uint16_t kArchetypeCloseControl = 1U << 3;
constexpr std::uint16_t kArchetypeAreaCounter = 1U << 4;
constexpr std::uint16_t kArchetypeMetalCounter = 1U << 5;
constexpr std::uint16_t kArchetypeBarrier = 1U << 6;
constexpr std::uint16_t kArchetypeSpike = 1U << 7;
constexpr std::uint16_t kArchetypeLanePressure = 1U << 8;
constexpr std::uint16_t kArchetypeFastPressure = 1U << 0;
constexpr std::uint16_t kArchetypeMetalScreen = 1U << 1;
constexpr std::uint16_t kArchetypeVehicle = 1U << 2;
constexpr std::uint16_t kArchetypeEconomy = 1U << 3;
constexpr std::uint16_t kArchetypeRangedSiege = 1U << 4;
constexpr std::uint16_t kArchetypeRaid = 1U << 5;
constexpr std::uint16_t kArchetypeJump = 1U << 6;
constexpr std::uint16_t kArchetypeHeavy = 1U << 7;
constexpr std::uint16_t kArchetypeSwarm = 1U << 8;

std::uint16_t DeckArchetype(const VSGameState &state, VSSide side) {
    const std::size_t sideIndex = side == VSSide::Plants ? 0 : 1;
    std::uint16_t archetype = 0;
    for (const VSCardState &card : state.seedBanks[sideIndex]) {
        if (!card.active || card.matchRestricted) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        if (side == VSSide::Plants) {
            if (seed == SeedType::SEED_ICESHROOM || seed == SeedType::SEED_DOOMSHROOM) {
                archetype |= kArchetypeMushroomControl;
            }
            switch (seed) {
                case SeedType::SEED_PEASHOOTER:
                case SeedType::SEED_SNOWPEA:
                case SeedType::SEED_REPEATER:
                case SeedType::SEED_THREEPEATER:
                case SeedType::SEED_SPLITPEA:
                case SeedType::SEED_CACTUS:
                case SeedType::SEED_SCAREDYSHROOM:
                    archetype |= kArchetypeDirectFire;
                    break;
                case SeedType::SEED_CABBAGEPULT:
                case SeedType::SEED_KERNELPULT:
                case SeedType::SEED_MELONPULT:
                case SeedType::SEED_SPORESHROOM:
                    archetype |= kArchetypeLobbedFire;
                    break;
                case SeedType::SEED_PUFFSHROOM:
                case SeedType::SEED_FUMESHROOM:
                case SeedType::SEED_HYPNOSHROOM:
                    archetype |= kArchetypeMushroomControl;
                    break;
                case SeedType::SEED_BONK_CHOY:
                case SeedType::SEED_CHOMPER:
                case SeedType::SEED_CELERY_STALKER:
                    archetype |= kArchetypeCloseControl;
                    break;
                case SeedType::SEED_SQUASH:
                case SeedType::SEED_CHERRYBOMB:
                case SeedType::SEED_JALAPENO:
                case SeedType::SEED_CHILLY_PEPPER:
                case SeedType::SEED_ICESHROOM:
                case SeedType::SEED_DOOMSHROOM:
                    archetype |= kArchetypeAreaCounter;
                    break;
                case SeedType::SEED_MAGNETSHROOM:
                    archetype |= kArchetypeMetalCounter;
                    break;
                case SeedType::SEED_WALLNUT:
                case SeedType::SEED_TALLNUT:
                case SeedType::SEED_PUMPKINSHELL:
                    archetype |= kArchetypeBarrier;
                    break;
                case SeedType::SEED_SPIKEWEED:
                case SeedType::SEED_SPIKEROCK:
                    archetype |= kArchetypeSpike;
                    break;
                case SeedType::SEED_BLOOMERANG:
                case SeedType::SEED_STARFRUIT:
                case SeedType::SEED_GARLIC:
                    archetype |= kArchetypeLanePressure;
                    break;
                default:
                    break;
            }
            continue;
        }

        switch (seed) {
            case SeedType::SEED_ZOMBIE_NORMAL:
            case SeedType::SEED_ZOMBIE_DOGWALKER:
            case SeedType::SEED_ZOMBIE_IMP:
            case SeedType::SEED_ZOMBIE_SUPER_FAN_IMP:
                archetype |= kArchetypeFastPressure;
                break;
            case SeedType::SEED_ZOMBIE_NEWSPAPER:
            case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
            case SeedType::SEED_ZOMBIE_TRASHCAN:
                archetype |= kArchetypeMetalScreen;
                break;
            case SeedType::SEED_ZOMBIE_BOBSLED:
            case SeedType::SEED_ZOMBONI:
                archetype |= kArchetypeVehicle;
                break;
            case SeedType::SEED_ZOMBIE_GRAVESTONE:
            case SeedType::SEED_ZOMBIE_MOUND:
                archetype |= kArchetypeEconomy;
                break;
            case SeedType::SEED_ZOMBIE_PEA_HEAD:
            case SeedType::SEED_ZOMBIE_CATAPULT:
                archetype |= kArchetypeRangedSiege;
                break;
            case SeedType::SEED_ZOMBIE_BUNGEE:
            case SeedType::SEED_ZOMBIE_DIGGER:
                archetype |= kArchetypeRaid;
                break;
            case SeedType::SEED_ZOMBIE_LADDER:
            case SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER:
                archetype |= kArchetypeJump;
                break;
            case SeedType::SEED_ZOMBIE_FOOTBALL:
            case SeedType::SEED_ZOMBIE_GIGA_FOOTBALL:
            case SeedType::SEED_ZOMBIE_GARGANTUAR:
            case SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR:
                archetype |= kArchetypeHeavy;
                break;
            case SeedType::SEED_ZOMBIE_FLAG:
            case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
            case SeedType::SEED_ZOMBIE_SQUASH_HEAD:
                archetype |= kArchetypeSwarm;
                break;
            default:
                break;
        }
    }
    return archetype;
}

int ArchetypeSpecificity(std::uint16_t archetype) {
    int bits = 0;
    while (archetype != 0) {
        bits += archetype & 1U;
        archetype >>= 1U;
    }
    return bits;
}

bool HasZombieDeckArchetype(const VSGameState &state, std::uint16_t mask) {
    return mask != 0 && (DeckArchetype(state, VSSide::Zombies) & mask) == mask;
}

int ZombieDeckCounterBonus(const VSGameState &state, SeedType seed, int targetRow) {
    const std::uint16_t archetype = DeckArchetype(state, VSSide::Zombies);
    if (archetype == 0) {
        return 0;
    }

    const auto has = [archetype](std::uint16_t mask) { return (archetype & mask) != 0; };
    const auto isLobbed = [](SeedType candidate) {
        switch (candidate) {
            case SeedType::SEED_CABBAGEPULT:
            case SeedType::SEED_KERNELPULT:
            case SeedType::SEED_MELONPULT:
            case SeedType::SEED_WINTERMELON:
            case SeedType::SEED_COBCANNON:
            case SeedType::SEED_SPORESHROOM:
                return true;
            default:
                return false;
        }
    };
    const auto seedInRow = [&state, targetRow](ZombieType zombieType) { return targetRow >= 0 && targetRow < state.rows && HasZombieTypeInRow(state, targetRow, zombieType); };
    int bonus = 0;

    // Fast-pressure recordings spend cheap walkers before the plant side has
    // a complete firing line. Mines, slow/control and low-cost front answers
    // must beat a second producer in this matchup.
    if (has(kZombieDeckFastPressure)) {
        if (seed == SeedType::SEED_POTATOMINE) {
            bonus += 150;
        } else if (seed == SeedType::SEED_ICEBERG_LETTUCE || seed == SeedType::SEED_SNOWPEA) {
            bonus += 100;
        } else if (seed == SeedType::SEED_PUFFSHROOM || seed == SeedType::SEED_FUMESHROOM || seed == SeedType::SEED_BONK_CHOY || seed == SeedType::SEED_CELERY_STALKER) {
            bonus += 80;
        } else if (seed == SeedType::SEED_SUNFLOWER || seed == SeedType::SEED_SUNSHROOM) {
            bonus -= 75;
        }
    }

    // Door/Trashcan/Football screens are exactly the matchup where direct
    // peas lose tempo. Magnet and lobbed fire convert the same sun into a
    // useful answer instead of feeding a protected zombie.
    if (has(kZombieDeckMetalScreen)) {
        if (seed == SeedType::SEED_MAGNETSHROOM) {
            bonus += 260;
        } else if (isLobbed(seed)) {
            bonus += 105;
        } else if (seed == SeedType::SEED_PEASHOOTER || seed == SeedType::SEED_REPEATER || seed == SeedType::SEED_THREEPEATER || seed == SeedType::SEED_SNOWPEA) {
            bonus -= 55;
        }
    }

    // A vehicle plan is answered by a front trigger. Do not wait for a
    // Zomboni to reach the row before valuing Spikeweed/Potato Mine.
    if (has(kZombieDeckVehicle)) {
        if (seed == SeedType::SEED_SPIKEWEED || seed == SeedType::SEED_SPIKEROCK) {
            bonus += 250;
        } else if (seed == SeedType::SEED_POTATOMINE) {
            bonus += 135;
        } else if (seed == SeedType::SEED_JALAPENO || seed == SeedType::SEED_CHILLY_PEPPER || seed == SeedType::SEED_CHERRYBOMB || seed == SeedType::SEED_SQUASH) {
            bonus += 75;
        }
    }

    // Pea-head/Catapult pressure attacks economy from range. A front shell
    // protects an already-funded lane while a lobbed carry can retaliate;
    // pure direct fire is deliberately not rewarded into the screen.
    if (has(kZombieDeckRangedSiege)) {
        if (seed == SeedType::SEED_PUMPKINSHELL || seed == SeedType::SEED_WALLNUT || seed == SeedType::SEED_TALLNUT) {
            bonus += 150;
        } else if (isLobbed(seed) || seed == SeedType::SEED_GARLIC) {
            bonus += 90;
        }
    }

    // Bungee/Digger plans punish a single exposed high-value plant. Prefer
    // compact, protected formations and leave disposable pads available.
    if (has(kZombieDeckRaid)) {
        if (seed == SeedType::SEED_PUMPKINSHELL || seed == SeedType::SEED_UMBRELLA) {
            bonus += 120;
        } else if (seed == SeedType::SEED_SUNSHROOM) {
            bonus += 35;
        }
    }

    if (has(kZombieDeckJump)) {
        if (seed == SeedType::SEED_SPIKEWEED || seed == SeedType::SEED_SPIKEROCK || seed == SeedType::SEED_SQUASH || seed == SeedType::SEED_CHERRYBOMB) {
            bonus += 100;
        } else if (seed == SeedType::SEED_WALLNUT || seed == SeedType::SEED_TALLNUT) {
            bonus -= 35;
        }
    }

    // Giant finishers require a real Ash/convert reserve; spending the last
    // sun on income while a breakthrough is affordable is a losing exchange.
    if (has(kZombieDeckHeavy)) {
        if (IsAreaCounterSeed(seed) || seed == SeedType::SEED_HYPNOSHROOM) {
            bonus += 180;
        } else if (seed == SeedType::SEED_SUNFLOWER || seed == SeedType::SEED_SUNSHROOM) {
            bonus -= 95;
        }
    }

    if (has(kZombieDeckSwarm)) {
        if (IsAreaCounterSeed(seed) || seed == SeedType::SEED_FUMESHROOM || seed == SeedType::SEED_BLOOMERANG || seed == SeedType::SEED_MELONPULT || seed == SeedType::SEED_SPORESHROOM) {
            bonus += 170;
        } else if (seed == SeedType::SEED_SNOWPEA) {
            bonus += 45;
        }
    }

    // Grave-economy decks are not a reason to tunnel on defense forever:
    // once a live lane reaches the mound, removal/output has the highest
    // value because each shot also destroys future Brain income.
    if (has(kZombieDeckEconomy)) {
        if (seed == SeedType::SEED_GRAVEBUSTER) {
            bonus += 280;
        } else if (IsSustainedOutputSeed(seed) || seed == SeedType::SEED_SPIKEWEED) {
            bonus += 90;
        } else if (seed == SeedType::SEED_SUNFLOWER || seed == SeedType::SEED_SUNSHROOM) {
            bonus -= 70;
        }
    }

    // Live evidence on the chosen row tightens the generic prior without
    // allowing a deck label alone to place an illegal or suicidal card.
    if (seedInRow(ZombieType::ZOMBIE_ZAMBONI) && (seed == SeedType::SEED_SPIKEWEED || seed == SeedType::SEED_POTATOMINE)) {
        bonus += 150;
    }
    if ((seedInRow(ZombieType::ZOMBIE_PEA_HEAD) || seedInRow(ZombieType::ZOMBIE_CATAPULT)) && (seed == SeedType::SEED_PUMPKINSHELL || isLobbed(seed))) {
        bonus += 110;
    }
    if (seedInRow(ZombieType::ZOMBIE_TRASHCAN) || seedInRow(ZombieType::ZOMBIE_DOOR)) {
        bonus += seed == SeedType::SEED_MAGNETSHROOM ? 130 : (isLobbed(seed) ? 55 : 0);
    }
    return std::clamp(bonus, -120, 320);
}

class StrategyDatabase {
    std::vector<StrategyRule> mRules;
    StrategyDatabaseLoadState mLoadState = StrategyDatabaseLoadState::Uninitialized;
    std::uint32_t mNextRetryTick = 0;

    bool ShouldRetryAt(std::uint32_t tick) const {
        return static_cast<std::int32_t>(tick - mNextRetryTick) >= 0;
    }

    void MarkUnavailable(std::uint32_t tick) {
        mLoadState = StrategyDatabaseLoadState::Unavailable;
        mNextRetryTick = tick + kStrategyDatabaseRetryIntervalTicks;
    }

    void Load(std::uint32_t tick) {
        if (mLoadState == StrategyDatabaseLoadState::Loaded || mLoadState == StrategyDatabaseLoadState::Invalid || (mLoadState == StrategyDatabaseLoadState::Unavailable && !ShouldRetryAt(tick))) {
            return;
        }
        if (Sexy::gSexyAppBase == nullptr) {
            MarkUnavailable(tick);
            return;
        }

        Sexy::Buffer buffer;
        if (!Sexy::gSexyAppBase->ReadBufferFromFile("addonFiles/data/vs_ai_strategy_db.bin", &buffer, false)) {
            MarkUnavailable(tick);
            return;
        }

        const auto &data = buffer.mData;
        if (data.size() < kStrategyDatabaseHeaderSize || !std::equal(kStrategyDatabaseMagic.begin(), kStrategyDatabaseMagic.end(), data.begin())) {
            mLoadState = StrategyDatabaseLoadState::Invalid;
            return;
        }
        const std::uint16_t version = ReadStrategyU16(data, 8);
        const bool legacyDatabase = version == kLegacyStrategyDatabaseVersion;
        const bool previousDatabase = version == kPreviousStrategyDatabaseVersion;
        if (!legacyDatabase && !previousDatabase && version != kStrategyDatabaseVersion) {
            mLoadState = StrategyDatabaseLoadState::Invalid;
            return;
        }
        const std::size_t ruleCount = ReadStrategyU16(data, 10);
        const std::size_t ruleSize = legacyDatabase ? kLegacyStrategyDatabaseRuleSize : (previousDatabase ? kPreviousStrategyDatabaseRuleSize : kStrategyDatabaseRuleSize);
        if (ruleCount > (data.size() - kStrategyDatabaseHeaderSize) / ruleSize || kStrategyDatabaseHeaderSize + ruleCount * ruleSize != data.size()) {
            mLoadState = StrategyDatabaseLoadState::Invalid;
            return;
        }

        mRules.clear();
        for (std::size_t index = 0; index < ruleCount; ++index) {
            const std::size_t offset = kStrategyDatabaseHeaderSize + index * ruleSize;
            const unsigned char sideCode = data[offset];
            const int phase = data[offset + (legacyDatabase ? 3 : (previousDatabase ? 7 : 9))];
            const int bonus = data[offset + (legacyDatabase ? 8 : (previousDatabase ? 12 : 14))];
            if (sideCode > 1 || phase < 0 || phase > 2 || bonus <= 0 || bonus > 100) {
                continue;
            }

            StrategyRule rule{};
            rule.side = sideCode == 0 ? VSSide::Plants : VSSide::Zombies;
            rule.seed = ReadStrategyU16(data, offset + 1);
            rule.deckSignature = legacyDatabase ? 0 : ReadStrategyU32(data, offset + 3);
            rule.opponentArchetype = legacyDatabase || previousDatabase ? 0 : ReadStrategyU16(data, offset + 7);
            rule.phase = phase;
            bool validRule = true;
            for (std::size_t bucketIndex = 0; bucketIndex < rule.buckets.size(); ++bucketIndex) {
                const std::size_t bucketOffset = legacyDatabase ? 4 : (previousDatabase ? 8 : 10);
                const int bucket = static_cast<int>(static_cast<std::int8_t>(data[offset + bucketOffset + bucketIndex]));
                if (bucket < -1 || bucket > 3) {
                    validRule = false;
                    break;
                }
                rule.buckets[bucketIndex] = bucket;
            }
            if (!validRule) {
                continue;
            }
            rule.bonus = bonus;
            const std::size_t samplesOffset = legacyDatabase ? 10 : (previousDatabase ? 14 : 16);
            rule.samples = ReadStrategyU16(data, offset + samplesOffset);
            mRules.push_back(rule);
        }
        mLoadState = StrategyDatabaseLoadState::Loaded;
    }

public:
    StrategyDatabaseLoadState LoadState() const {
        return mLoadState;
    }

    void Reset() {
        mRules.clear();
        mLoadState = StrategyDatabaseLoadState::Uninitialized;
        mNextRetryTick = 0;
    }

    int Bonus(const VSGameState &state, VSSide side, SeedType seed, int targetRow) {
        Load(state.boardTick);
        if (mRules.empty() || targetRow < 0 || targetRow >= state.rows) {
            return 0;
        }

        const int actualEconomy = side == VSSide::Plants ? CountPlantIncome(state) : CountZombieEconomy(state);
        const int ownEconomy = EffectiveAIEconomyCount(side, actualEconomy);
        const int opponentUnits = side == VSSide::Plants ? CountActiveZombies(state) : CountLivePlants(state);
        const int ownLaneUnits = side == VSSide::Plants ? CountPlantsInRow(state, targetRow) : CountZombiesInRow(state, targetRow);
        const int opponentLaneUnits = side == VSSide::Plants ? CountZombiesInRow(state, targetRow) : CountPlantsInRow(state, targetRow);
        const int totalLiveUnits = CountLivePlants(state) + CountActiveZombies(state);
        const int phase = ownEconomy < 3 && totalLiveUnits < 11 ? 0 : totalLiveUnits < 25 ? 1 : 2;
        const std::uint32_t deckSignature = DeckSignature(state, side);
        const std::uint16_t opponentArchetype = DeckArchetype(state, side == VSSide::Plants ? VSSide::Zombies : VSSide::Plants);
        const std::array<int, 4> buckets = {
            StrategyBucket(ownEconomy),
            StrategyBucket(opponentUnits),
            StrategyBucket(ownLaneUnits),
            StrategyBucket(opponentLaneUnits),
        };

        int baselineBonus = 0;
        int matchupBonus = 0;
        int matchupSamples = 0;
        for (const StrategyRule &rule : mRules) {
            if (rule.side != side || rule.seed != static_cast<std::uint16_t>(seed) || rule.phase != phase || (rule.deckSignature != 0 && rule.deckSignature != deckSignature)
                || (rule.opponentArchetype != 0 && (opponentArchetype & rule.opponentArchetype) != rule.opponentArchetype)) {
                continue;
            }
            bool matches = true;
            for (std::size_t index = 0; index < buckets.size(); ++index) {
                matches = matches && (rule.buckets[index] < 0 || rule.buckets[index] == buckets[index]);
            }
            if (matches) {
                // An opponent mask is a required tactical subset rather than
                // an exact six-card hash. This preserves replay knowledge
                // when a Ban swaps one answer card but the opposing rush,
                // pult, vehicle or heavy-game plan remains the same.
                const int matchupSpecificity = ArchetypeSpecificity(rule.opponentArchetype);
                const int ruleBonus = std::min(100, rule.bonus + matchupSpecificity * 2);
                if (rule.opponentArchetype == 0) {
                    baselineBonus = std::max(baselineBonus, ruleBonus);
                } else if (ruleBonus > matchupBonus || (ruleBonus == matchupBonus && rule.samples > matchupSamples)) {
                    matchupBonus = ruleBonus;
                    matchupSamples = rule.samples;
                }
            }
        }
        if (matchupBonus == 0 || baselineBonus == 0) {
            return std::max(baselineBonus, matchupBonus);
        }

        // The generic template remains the primary replay prior. A matched
        // enemy archetype adds a bounded, sample-weighted adjustment instead
        // of competing through max(), which previously discarded almost all
        // one- and two-feature matchup rules behind common generic actions.
        // Four observations are enough to reach the cap; this preserves a
        // real learned distinction without allowing replay data to bypass
        // threat, legality, or placement checks in the decision layers.
        const int confidence = std::clamp(matchupSamples, 1, 4);
        const int matchupAdjustment = std::max(2, matchupBonus * confidence / 12);
        return std::min(100, baselineBonus + matchupAdjustment);
    }
};

StrategyDatabase &GetStrategyDatabase() {
    static StrategyDatabase database;
    return database;
}

int StrategyBonus(const VSGameState &state, VSSide side, SeedType seed, int targetRow) {
    return GetStrategyDatabase().Bonus(state, side, seed, targetRow);
}

StrategyDatabaseLoadState GetStrategyDatabaseLoadState() {
    return GetStrategyDatabase().LoadState();
}

void ResetStrategyDatabase() {
    GetStrategyDatabase().Reset();
}

bool IsReadyCard(const VSCardState &card, int resource) {
    return card.seedType != static_cast<std::uint16_t>(SeedType::SEED_NONE) && !card.matchRestricted && card.active && !card.refreshing && card.refreshCounter <= 0 && card.cost <= resource;
}

int ReadyPlantAreaCounterCount(const VSGameState &state) {
    return static_cast<int>(std::count_if(state.seedBanks[0].begin(), state.seedBanks[0].end(), [&state](const VSCardState &card) {
        return IsAreaCounterSeed(static_cast<SeedType>(card.seedType)) && IsReadyCard(card, state.plantSun);
    }));
}

int PlantAreaCounterExposure(const VSGameState &state, int row) {
    const int readyCounters = ReadyPlantAreaCounterCount(state);
    const VSZombieState *closest = FindClosestZombie(state, row);
    if (readyCounters == 0 || closest == nullptr) {
        return 0;
    }

    const int zombieCount = CountZombiesInRow(state, row);
    const int stackCount = LargestZombieStackInRow(state, row);
    int score = 0;
    if (zombieCount >= 2) {
        score += 130 + (zombieCount - 2) * 90;
    }
    if (stackCount >= 2) {
        score += 150 + (stackCount - 2) * 120;
    }
    // Once a front reaches the plant half, its exact position is already a
    // legal Squash/Cherry target.  Do not make that trade easier for plants.
    if (closest->positionX < 760.0f) {
        score += 110;
    }
    return score * std::min(readyCounters, 2);
}

bool IsAreaCounterSeed(SeedType seed) {
    return HasPlantCardRole(seed, VSCardRole::PlantAreaCounter);
}

bool IsZombieBreakthroughSeed(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieBreakthrough);
}

bool HasReadyZombieBreakthroughCard(const VSGameState &state) {
    return std::any_of(state.seedBanks[1].begin(), state.seedBanks[1].end(), [&state](const VSCardState &card) {
        return IsZombieBreakthroughSeed(static_cast<SeedType>(card.seedType)) && IsReadyCard(card, state.zombieBrains);
    });
}

bool IsHeavyZombieSeed(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieHeavy);
}

bool IsZombieGraveGuardSeed(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieGraveGuard);
}

bool HasZombieGraveGuardInRow(const VSGameState &state, int row) {
    return HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_TRASHCAN) || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_DOOR) || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_WALLNUT_HEAD)
        || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_TALLNUT_HEAD) || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_PAIL) || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_NEWSPAPER)
        || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_SUNDAY_EDITION) || HasZombieTypeInRow(state, row, ZombieType::ZOMBIE_TRAFFIC_CONE);
}

} // namespace vsai::detail
