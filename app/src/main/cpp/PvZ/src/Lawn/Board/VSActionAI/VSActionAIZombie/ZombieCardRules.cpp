#include "ZombieCardRules.h"
#include "../VSActionAICardRules.h"
#include "../VSActionAIPolicy.h"

#include <algorithm>
#include <initializer_list>

namespace vsai::detail {

namespace {

    constexpr std::uint64_t TemplateMask(ZombieTemplate value) {
        return 1ULL << static_cast<std::uint8_t>(value);
    }

    struct ZombieTemplatePlan {
        ZombieTemplate templateId;
        SeedType openingSeed;
        int openingEconomyFloor;
        int openingEconomyCeiling;
        int openingRowTarget;
        SeedType conversionSeed;
        int conversionEconomyFloor;
        int conversionMinPressureRows;
        SeedType finisherSeed;
        int finisherEconomyFloor;
        int finisherMinPressureRows;
    };

    constexpr ZombieTemplatePlan kZombieTemplatePlans[] = {
        {ZombieTemplate::ZamboniPole, SeedType::SEED_ZOMBIE_IMP, 3, 6, 3, SeedType::SEED_ZOMBONI, 3, 1, SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER, 4, 1},
        {ZombieTemplate::PeaHeadGiant, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_TRASHCAN, 4, 1, SeedType::SEED_ZOMBIE_GARGANTUAR, 7, 2},
        {ZombieTemplate::ImpSledSunday, SeedType::SEED_ZOMBIE_IMP, 3, 10, 3, SeedType::SEED_ZOMBIE_PAIL, 3, 1, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, 8, 2},
        {ZombieTemplate::ArmoredNormalRush, SeedType::SEED_ZOMBIE_NORMAL, 1, 7, 3, SeedType::SEED_ZOMBIE_TRASHCAN, 3, 1, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR, 8, 2},
        {ZombieTemplate::NewspaperSledDiggerGiga, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 7, 3, SeedType::SEED_ZOMBIE_BOBSLED, 5, 2, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR, 9, 2},
        {ZombieTemplate::NewspaperDiggerGiga, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 7, 3, SeedType::SEED_ZOMBIE_DIGGER, 5, 1, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR, 9, 2},
        {ZombieTemplate::ConeImpFootballGiant, SeedType::SEED_ZOMBIE_IMP, 2, 7, 3, SeedType::SEED_ZOMBIE_PAIL, 3, 1, SeedType::SEED_ZOMBIE_FOOTBALL, 5, 2},
        {ZombieTemplate::NormalNewsSled, SeedType::SEED_ZOMBIE_NORMAL, 1, 7, 3, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 1, SeedType::SEED_ZOMBONI, 3, 2},
        {ZombieTemplate::NormalNewsImpSunday, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 5, 3, SeedType::SEED_ZOMBIE_IMP, 3, 1, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, 4, 2},
        {ZombieTemplate::LadderPole, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 5, 2, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, 2, 1, SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER, 2, 1},
        {ZombieTemplate::NewspaperFanPole, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 6, 3, SeedType::SEED_ZOMBIE_SUPER_FAN_IMP, 2, 1, SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER, 3, 2},
        {ZombieTemplate::PeaHeadSunday, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_IMP, 3, 1, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, 7, 2},
        {ZombieTemplate::PeaHeadZamboni, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_PAIL, 4, 1, SeedType::SEED_ZOMBONI, 4, 1},
        {ZombieTemplate::PeaHeadFlagBungee, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 8, 3, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, 3, 1, SeedType::SEED_ZOMBIE_FLAG, 8, 2},
        {ZombieTemplate::MoundSkirmish, SeedType::SEED_ZOMBIE_NORMAL, 2, 7, 3, SeedType::SEED_ZOMBIE_MOUND, 3, 0, SeedType::SEED_ZOMBONI, 4, 1},
        {ZombieTemplate::FlagSquash, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, 2, 7, 3, SeedType::SEED_ZOMBIE_SQUASH_HEAD, 3, 1, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR, 8, 2},
        {ZombieTemplate::FanImp, SeedType::SEED_ZOMBIE_SUPER_FAN_IMP, 2, 7, 3, SeedType::SEED_ZOMBIE_SQUASH_HEAD, 3, 1, SeedType::SEED_ZOMBIE_SCREEN_DOOR, 4, 1},
        {ZombieTemplate::MoundTallnutSled, SeedType::SEED_ZOMBIE_MOUND, 2, 7, 1, SeedType::SEED_ZOMBIE_TRASHCAN, 4, 0, SeedType::SEED_ZOMBIE_BOBSLED, 5, 1},
        {ZombieTemplate::ImpLadderFootball, SeedType::SEED_ZOMBIE_IMP, 2, 7, 3, SeedType::SEED_ZOMBIE_LADDER, 4, 1, SeedType::SEED_ZOMBIE_FOOTBALL, 5, 2},
        {ZombieTemplate::SledDogHeavy, SeedType::SEED_ZOMBIE_DOGWALKER, 2, 7, 3, SeedType::SEED_ZOMBIE_BOBSLED, 5, 2, SeedType::SEED_ZOMBIE_GARGANTUAR, 7, 2},
        {ZombieTemplate::DogSledPea, SeedType::SEED_ZOMBIE_DOGWALKER, 2, 7, 3, SeedType::SEED_ZOMBIE_PEA_HEAD, 3, 1, SeedType::SEED_ZOMBIE_BOBSLED, 5, 2},
        {ZombieTemplate::LadderBalloonZamboni, SeedType::SEED_ZOMBIE_BALLOON, 2, 7, 3, SeedType::SEED_ZOMBIE_LADDER, 4, 1, SeedType::SEED_ZOMBONI, 6, 1},
        {ZombieTemplate::MoundBungeeFootball, SeedType::SEED_ZOMBIE_MOUND, 2, 7, 1, SeedType::SEED_ZOMBIE_TRASHCAN, 3, 1, SeedType::SEED_ZOMBIE_GIGA_FOOTBALL, 5, 2},
        {ZombieTemplate::NewspaperImpFootballGiant, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 7, 3, SeedType::SEED_ZOMBIE_IMP, 3, 1, SeedType::SEED_ZOMBIE_FOOTBALL, 5, 2},
        {ZombieTemplate::PeaHeadZomblobGiant, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_ZOMBLOB, 5, 2, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR, 8, 2},
        {ZombieTemplate::ImpPailSledFootball, SeedType::SEED_ZOMBIE_IMP, 2, 7, 3, SeedType::SEED_ZOMBIE_PAIL, 3, 1, SeedType::SEED_ZOMBIE_FOOTBALL, 4, 2},
        {ZombieTemplate::NewspaperScreenFootball, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 7, 3, SeedType::SEED_ZOMBIE_PAIL, 3, 1, SeedType::SEED_ZOMBIE_GIGA_FOOTBALL, 6, 2},
        {ZombieTemplate::DogPeaFootball, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, 3, 1, SeedType::SEED_ZOMBIE_FOOTBALL, 5, 2},
        {ZombieTemplate::NewspaperFootballPole, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 7, 3, SeedType::SEED_ZOMBIE_NORMAL, 3, 1, SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER, 5, 2},
        {ZombieTemplate::DancerRaid, SeedType::SEED_ZOMBIE_DANCER, 3, 8, 3, SeedType::SEED_ZOMBIE_PAIL, 5, 1, SeedType::SEED_ZOMBIE_JACKSON, 6, 2},
        {ZombieTemplate::PeaHeadRaid, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_SQUASH_HEAD, 3, 1, SeedType::SEED_ZOMBIE_BUNGEE, 5, 1},
        {ZombieTemplate::PeaHeadDancerRaid, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_DANCER, 3, 1, SeedType::SEED_ZOMBIE_JALAPENO_HEAD, 5, 2},
        {ZombieTemplate::MoundPeaZomblobFootball, SeedType::SEED_ZOMBIE_PEA_HEAD, 2, 7, 3, SeedType::SEED_ZOMBIE_MOUND, 2, 0, SeedType::SEED_ZOMBIE_ZOMBLOB, 5, 2},
        {ZombieTemplate::SundayLadderRaid, SeedType::SEED_ZOMBIE_NORMAL, 1, 7, 3, SeedType::SEED_ZOMBIE_LADDER, 3, 1, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, 5, 2},
        {ZombieTemplate::MoundNewspaperZamboni, SeedType::SEED_ZOMBIE_IMP, 3, 7, 3, SeedType::SEED_ZOMBIE_MOUND, 3, 0, SeedType::SEED_ZOMBONI, 5, 1},
        {ZombieTemplate::MoundTallnutGuard, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, 2, 7, 3, SeedType::SEED_ZOMBIE_MOUND, 3, 0, SeedType::SEED_ZOMBIE_TALLNUT_HEAD, 5, 1},
        {ZombieTemplate::ConeSundayTallnut, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, 2, 6, 3, SeedType::SEED_ZOMBIE_TALLNUT_HEAD, 4, 1, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, 6, 2},
        {ZombieTemplate::NormalNewsImpGiga, SeedType::SEED_ZOMBIE_NORMAL, 2, 6, 3, SeedType::SEED_ZOMBIE_NEWSPAPER, 3, 1, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR, 7, 2},
        {ZombieTemplate::NewspaperLadderZamboniJack, SeedType::SEED_ZOMBIE_NEWSPAPER, 2, 6, 3, SeedType::SEED_ZOMBIE_LADDER, 3, 1, SeedType::SEED_ZOMBONI, 5, 2},
    };

    static_assert(sizeof(kZombieTemplatePlans) / sizeof(kZombieTemplatePlans[0]) == static_cast<std::size_t>(ZombieTemplate::NewspaperLadderZamboniJack) + 1);

    int ZombieTemplatePlanPriority(ZombieTemplate templateId) {
        // These are deliberate broad fallback profiles. More specific replay
        // decks must keep their observed conversion rather than inheriting a
        // generic Pea Head or Newspaper finisher.
        switch (templateId) {
            case ZombieTemplate::PeaHeadGiant:
            case ZombieTemplate::NewspaperDiggerGiga:
                return 10;
            default:
                return 100;
        }
    }

    const ZombieTemplatePlan *FindZombieTemplatePlan(const ZombieTemplateProfile &profile) {
        const ZombieTemplatePlan *bestPlan = nullptr;
        for (const ZombieTemplatePlan &plan : kZombieTemplatePlans) {
            if (profile.Has(plan.templateId) && (bestPlan == nullptr || ZombieTemplatePlanPriority(plan.templateId) > ZombieTemplatePlanPriority(bestPlan->templateId))) {
                bestPlan = &plan;
            }
        }
        return bestPlan;
    }

} // namespace

bool ZombieTemplateProfile::Has(ZombieTemplate value) const {
    return (templates & TemplateMask(value)) != 0;
}

ZombieTemplateProfile DetectZombieTemplateProfile(const VSGameState &state) {
    const auto has = [&state](SeedType seed) { return HasActiveDeckCard(state, VSSide::Zombies, seed); };
    const auto hasAll = [&has](std::initializer_list<SeedType> seeds) {
        for (const SeedType seed : seeds) {
            if (!has(seed)) {
                return false;
            }
        }
        return true;
    };
    const auto add = [](ZombieTemplateProfile &profile, ZombieTemplate value, bool matches) {
        if (matches) {
            profile.templates |= TemplateMask(value);
        }
    };

    ZombieTemplateProfile profile{};
    profile.fastPressure = (has(SeedType::SEED_ZOMBIE_NORMAL) || has(SeedType::SEED_ZOMBIE_DOGWALKER) || has(SeedType::SEED_ZOMBIE_SUPER_FAN_IMP) || has(SeedType::SEED_ZOMBIE_FLAG))
        && (has(SeedType::SEED_ZOMBIE_NEWSPAPER) || has(SeedType::SEED_ZOMBIE_IMP) || has(SeedType::SEED_ZOMBIE_TRAFFIC_CONE));
    profile.rangedSiege = has(SeedType::SEED_ZOMBIE_PEA_HEAD) && (has(SeedType::SEED_ZOMBIE_TRASHCAN) || has(SeedType::SEED_ZOMBIE_PAIL) || has(SeedType::SEED_ZOMBIE_FOOTBALL));
    profile.sundayPressure = has(SeedType::SEED_ZOMBIE_SUNDAY_EDITION) && (has(SeedType::SEED_ZOMBIE_NORMAL) || has(SeedType::SEED_ZOMBIE_IMP) || has(SeedType::SEED_ZOMBIE_NEWSPAPER));

    add(profile,
        ZombieTemplate::ZamboniPole,
        hasAll({SeedType::SEED_ZOMBONI, SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_IMP}));
    add(profile,
        ZombieTemplate::PeaHeadGiant,
        hasAll({SeedType::SEED_ZOMBIE_PEA_HEAD, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_TRASHCAN}) && (has(SeedType::SEED_ZOMBIE_GARGANTUAR) || has(SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR)));
    add(profile,
        ZombieTemplate::ImpSledSunday,
        hasAll({SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, SeedType::SEED_ZOMBIE_SCREEN_DOOR}));
    add(profile,
        ZombieTemplate::ArmoredNormalRush,
        hasAll({SeedType::SEED_ZOMBIE_NORMAL, SeedType::SEED_ZOMBIE_TRASHCAN, SeedType::SEED_ZOMBIE_DOGWALKER, SeedType::SEED_ZOMBIE_FOOTBALL})
            && (has(SeedType::SEED_ZOMBIE_GARGANTUAR) || has(SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR)));
    add(profile, ZombieTemplate::NewspaperDiggerGiga, hasAll({SeedType::SEED_ZOMBIE_NORMAL, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_DIGGER, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR}));
    add(profile, ZombieTemplate::NewspaperSledDiggerGiga, profile.Has(ZombieTemplate::NewspaperDiggerGiga) && has(SeedType::SEED_ZOMBIE_BOBSLED));
    add(profile,
        ZombieTemplate::ConeImpFootballGiant,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_FOOTBALL, SeedType::SEED_ZOMBIE_GARGANTUAR}));
    add(profile,
        ZombieTemplate::NormalNewsSled,
        hasAll({SeedType::SEED_ZOMBIE_NORMAL, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBONI, SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_DOGWALKER}));
    // Dogwalker was the sixth selectable card in an earlier recording, but
    // the actual Normal/Newspaper/Imp/Sunday sequence only depends on this
    // four-card core. Treat the sixth slot as a Ban replacement instead of
    // letting it erase the recorded opening when it is unavailable.
    add(profile, ZombieTemplate::NormalNewsImpSunday, hasAll({SeedType::SEED_ZOMBIE_NORMAL, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_SUNDAY_EDITION}));
    add(profile,
        ZombieTemplate::LadderPole,
        hasAll({SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_LADDER, SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER}));
    add(profile,
        ZombieTemplate::NewspaperFanPole,
        hasAll({SeedType::SEED_ZOMBIE_NORMAL,
                SeedType::SEED_ZOMBIE_NEWSPAPER,
                SeedType::SEED_ZOMBIE_SUPER_FAN_IMP,
                SeedType::SEED_ZOMBIE_GIGA_FOOTBALL,
                SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER,
                SeedType::SEED_ZOMBIE_DOGWALKER}));
    add(profile,
        ZombieTemplate::PeaHeadSunday,
        hasAll({SeedType::SEED_ZOMBIE_PEA_HEAD, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_TRASHCAN, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, SeedType::SEED_ZOMBIE_GARGANTUAR}));
    add(profile,
        ZombieTemplate::PeaHeadZamboni,
        hasAll({SeedType::SEED_ZOMBIE_PEA_HEAD, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_TRASHCAN, SeedType::SEED_ZOMBONI, SeedType::SEED_ZOMBIE_GARGANTUAR}));
    add(profile,
        ZombieTemplate::PeaHeadFlagBungee,
        hasAll({SeedType::SEED_ZOMBIE_PEA_HEAD,
                SeedType::SEED_ZOMBIE_TRAFFIC_CONE,
                SeedType::SEED_ZOMBIE_PAIL,
                SeedType::SEED_ZOMBIE_FOOTBALL,
                SeedType::SEED_ZOMBIE_BUNGEE,
                SeedType::SEED_ZOMBIE_FLAG}));
    add(profile,
        ZombieTemplate::MoundSkirmish,
        hasAll({SeedType::SEED_ZOMBIE_MOUND, SeedType::SEED_ZOMBIE_NORMAL, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBONI}));
    add(profile,
        ZombieTemplate::FlagSquash,
        hasAll({SeedType::SEED_ZOMBIE_FLAG, SeedType::SEED_ZOMBIE_SQUASH_HEAD, SeedType::SEED_ZOMBIE_SCREEN_DOOR, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR}));
    add(profile,
        ZombieTemplate::FanImp,
        hasAll({SeedType::SEED_ZOMBIE_SUPER_FAN_IMP, SeedType::SEED_ZOMBIE_SQUASH_HEAD, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_SCREEN_DOOR, SeedType::SEED_ZOMBIE_TRASHCAN}));
    add(profile,
        ZombieTemplate::MoundTallnutSled,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE,
                SeedType::SEED_ZOMBIE_PAIL,
                SeedType::SEED_ZOMBIE_BOBSLED,
                SeedType::SEED_ZOMBIE_TRASHCAN,
                SeedType::SEED_ZOMBIE_TALLNUT_HEAD,
                SeedType::SEED_ZOMBIE_MOUND}));
    add(profile,
        ZombieTemplate::ImpLadderFootball,
        hasAll({SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_GARGANTUAR, SeedType::SEED_ZOMBIE_LADDER, SeedType::SEED_ZOMBIE_FOOTBALL, SeedType::SEED_ZOMBIE_SCREEN_DOOR}));
    add(profile,
        ZombieTemplate::SledDogHeavy,
        hasAll({SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_DOGWALKER, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_GARGANTUAR, SeedType::SEED_ZOMBIE_GIGA_FOOTBALL}));
    add(profile,
        ZombieTemplate::DogSledPea,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_DOGWALKER, SeedType::SEED_ZOMBIE_PEA_HEAD, SeedType::SEED_ZOMBIE_IMP}));
    add(profile,
        ZombieTemplate::LadderBalloonZamboni,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE,
                SeedType::SEED_ZOMBIE_LADDER,
                SeedType::SEED_ZOMBONI,
                SeedType::SEED_ZOMBIE_BALLOON,
                SeedType::SEED_ZOMBIE_TALLNUT_HEAD,
                SeedType::SEED_ZOMBIE_JALAPENO_HEAD}));
    add(profile,
        ZombieTemplate::MoundBungeeFootball,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_TRASHCAN, SeedType::SEED_ZOMBIE_MOUND, SeedType::SEED_ZOMBIE_GIGA_FOOTBALL, SeedType::SEED_ZOMBIE_BUNGEE}));
    add(profile,
        ZombieTemplate::NewspaperImpFootballGiant,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_FOOTBALL, SeedType::SEED_ZOMBIE_GARGANTUAR}));
    add(profile,
        ZombieTemplate::PeaHeadZomblobGiant,
        hasAll({SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_PEA_HEAD, SeedType::SEED_ZOMBIE_TRASHCAN, SeedType::SEED_ZOMBIE_ZOMBLOB, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR}));
    add(profile, ZombieTemplate::ImpPailSledFootball, hasAll({SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_FOOTBALL}));
    add(profile,
        ZombieTemplate::NewspaperScreenFootball,
        hasAll({SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_SCREEN_DOOR, SeedType::SEED_ZOMBIE_GIGA_FOOTBALL, SeedType::SEED_ZOMBIE_TALLNUT_HEAD}));
    add(profile,
        ZombieTemplate::DogPeaFootball,
        hasAll({SeedType::SEED_ZOMBIE_DOGWALKER,
                SeedType::SEED_ZOMBIE_TRAFFIC_CONE,
                SeedType::SEED_ZOMBIE_PEA_HEAD,
                SeedType::SEED_ZOMBIE_SCREEN_DOOR,
                SeedType::SEED_ZOMBIE_FOOTBALL,
                SeedType::SEED_ZOMBIE_GIGA_FOOTBALL}));
    add(profile,
        ZombieTemplate::NewspaperFootballPole,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE,
                SeedType::SEED_ZOMBIE_NEWSPAPER,
                SeedType::SEED_ZOMBIE_FOOTBALL,
                SeedType::SEED_ZOMBIE_DOGWALKER,
                SeedType::SEED_ZOMBIE_NORMAL,
                SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER}));
    add(profile,
        ZombieTemplate::DancerRaid,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE,
                SeedType::SEED_ZOMBIE_PAIL,
                SeedType::SEED_ZOMBIE_DANCER,
                SeedType::SEED_ZOMBIE_TRASHCAN,
                SeedType::SEED_ZOMBIE_BUNGEE,
                SeedType::SEED_ZOMBIE_JACKSON}));
    add(profile,
        ZombieTemplate::PeaHeadRaid,
        hasAll({SeedType::SEED_ZOMBIE_BUNGEE, SeedType::SEED_ZOMBIE_PEA_HEAD, SeedType::SEED_ZOMBIE_SQUASH_HEAD, SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_SCREEN_DOOR}));
    add(profile,
        ZombieTemplate::PeaHeadDancerRaid,
        hasAll({SeedType::SEED_ZOMBIE_BUNGEE,
                SeedType::SEED_ZOMBIE_PAIL,
                SeedType::SEED_ZOMBIE_PEA_HEAD,
                SeedType::SEED_ZOMBIE_DANCER,
                SeedType::SEED_ZOMBIE_TRASHCAN,
                SeedType::SEED_ZOMBIE_JALAPENO_HEAD}));
    add(profile,
        ZombieTemplate::MoundPeaZomblobFootball,
        hasAll({SeedType::SEED_ZOMBIE_BUNGEE,
                SeedType::SEED_ZOMBIE_PEA_HEAD,
                SeedType::SEED_ZOMBIE_MOUND,
                SeedType::SEED_ZOMBIE_ZOMBLOB,
                SeedType::SEED_ZOMBIE_SQUASH_HEAD,
                SeedType::SEED_ZOMBIE_GIGA_FOOTBALL}));
    add(profile,
        ZombieTemplate::SundayLadderRaid,
        hasAll({SeedType::SEED_ZOMBIE_ZOMBLOB,
                SeedType::SEED_ZOMBIE_SUNDAY_EDITION,
                SeedType::SEED_ZOMBIE_BUNGEE,
                SeedType::SEED_ZOMBIE_LADDER,
                SeedType::SEED_ZOMBIE_JALAPENO_HEAD,
                SeedType::SEED_ZOMBIE_NORMAL}));
    add(profile,
        ZombieTemplate::MoundNewspaperZamboni,
        hasAll({SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBONI, SeedType::SEED_ZOMBIE_SCREEN_DOOR, SeedType::SEED_ZOMBIE_MOUND}));
    add(profile,
        ZombieTemplate::MoundTallnutGuard,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_TRASHCAN, SeedType::SEED_ZOMBIE_TALLNUT_HEAD, SeedType::SEED_ZOMBIE_MOUND}));
    add(profile,
        ZombieTemplate::ConeSundayTallnut,
        hasAll({SeedType::SEED_ZOMBIE_TRAFFIC_CONE, SeedType::SEED_ZOMBIE_SUNDAY_EDITION, SeedType::SEED_ZOMBIE_BOBSLED, SeedType::SEED_ZOMBIE_POLEVAULTER, SeedType::SEED_ZOMBIE_TALLNUT_HEAD}));
    add(profile,
        ZombieTemplate::NormalNewsImpGiga,
        hasAll({SeedType::SEED_ZOMBIE_NORMAL, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_IMP, SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR}));
    add(profile,
        ZombieTemplate::NewspaperLadderZamboniJack,
        hasAll({SeedType::SEED_ZOMBIE_PAIL, SeedType::SEED_ZOMBIE_NEWSPAPER, SeedType::SEED_ZOMBIE_LADDER, SeedType::SEED_ZOMBONI, SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX}));
    return profile;
}

bool IsZombieTemplatePhaseSeed(const ZombieTemplateProfile &profile, SeedType seed, ZombieTemplatePhase phase) {
    const ZombieTemplatePlan *plan = FindZombieTemplatePlan(profile);
    if (plan == nullptr) {
        return false;
    }
    switch (phase) {
        case ZombieTemplatePhase::Opening:
            return plan->openingSeed == seed;
        case ZombieTemplatePhase::Conversion:
            return plan->conversionSeed == seed;
        case ZombieTemplatePhase::Finisher:
            return plan->finisherSeed == seed;
        default:
            return false;
    }
}

bool IsZombieTemplatePhaseAvailable(
    const ZombieTemplateProfile &profile, const ZombieTempoPolicy &tempo, SeedType seed, int actualEconomyCount, int activePressureRows, int rows, ZombieTemplatePhase phase) {
    const ZombieTemplatePlan *plan = FindZombieTemplatePlan(profile);
    if (plan == nullptr) {
        return false;
    }
    const int economyCount = tempo.EffectiveEconomyCount(actualEconomyCount);
    switch (phase) {
        case ZombieTemplatePhase::Opening:
            return plan->openingSeed == seed && economyCount >= tempo.OpeningEconomyFloor(plan->openingEconomyFloor) && economyCount <= tempo.OpeningEconomyCeiling(plan->openingEconomyCeiling)
                && activePressureRows < tempo.OpeningPressureRowTarget(std::min(plan->openingRowTarget, rows), rows);
        case ZombieTemplatePhase::Conversion:
            return plan->conversionSeed == seed && economyCount >= tempo.CommitEconomyFloor(plan->conversionEconomyFloor)
                && tempo.HasCommitPressure(activePressureRows, plan->conversionMinPressureRows, rows);
        case ZombieTemplatePhase::Finisher:
            return plan->finisherSeed == seed && economyCount >= tempo.CommitEconomyFloor(plan->finisherEconomyFloor)
                && tempo.HasAttackCommitPressure(activePressureRows, plan->finisherMinPressureRows, rows);
        default:
            return false;
    }
}

bool HasReadyZombieTemplateCommit(const VSGameState &state, const ZombieTemplateProfile &profile, const ZombieTempoPolicy &tempo, int actualEconomyCount, int activePressureRows) {
    for (const VSCardState &card : state.seedBanks[1]) {
        if (!card.active || card.matchRestricted || card.refreshing || card.refreshCounter > 0 || !IsReadyCard(card, state.zombieBrains)) {
            continue;
        }
        const SeedType seed = static_cast<SeedType>(card.seedType);
        if (IsZombieTemplatePhaseAvailable(profile, tempo, seed, actualEconomyCount, activePressureRows, state.rows, ZombieTemplatePhase::Conversion)
            || IsZombieTemplatePhaseAvailable(profile, tempo, seed, actualEconomyCount, activePressureRows, state.rows, ZombieTemplatePhase::Finisher)) {
            return true;
        }
    }
    return false;
}

int ZombieTemplatePhaseBonus(const ZombieTemplateProfile &profile, const ZombieTempoPolicy &tempo, SeedType seed, int actualEconomyCount, int activePressureRows, int zombiesInRow, int rows) {
    const ZombieTemplatePlan *plan = FindZombieTemplatePlan(profile);
    if (plan == nullptr) {
        return 0;
    }
    int bonus = 0;
    if (IsZombieTemplatePhaseAvailable(profile, tempo, seed, actualEconomyCount, activePressureRows, rows, ZombieTemplatePhase::Opening)) {
        bonus += zombiesInRow == 0 ? 165 : -185;
    }
    if (IsZombieTemplatePhaseAvailable(profile, tempo, seed, actualEconomyCount, activePressureRows, rows, ZombieTemplatePhase::Conversion)) {
        bonus += zombiesInRow == 0 ? 180 : -150;
    }
    if (IsZombieTemplatePhaseAvailable(profile, tempo, seed, actualEconomyCount, activePressureRows, rows, ZombieTemplatePhase::Finisher)) {
        bonus += zombiesInRow == 0 ? 210 : -120;
    }
    return bonus;
}

bool ZombieTempoPolicy::IsEnhanced() const {
    return mEnhanced;
}

int ZombieTempoPolicy::EffectiveEconomyCount(int actualCount) const {
    return GetAIEnhancementPolicy(VSSide::Zombies).EffectiveEconomyCount(actualCount);
}

int ZombieTempoPolicy::EconomyTarget(int baseline, int rows, int activePressureRows) const {
    // Enhanced AI produces brains faster, but has no free complete grave
    // field. Retain a near-normal target so midgame losses are rebuilt.
    return std::max(rows, baseline - (mEnhanced ? 1 : 0));
}

int ZombieTempoPolicy::OpeningEconomyFloor(int baseline) const {
    return std::max(1, baseline - (mEnhanced ? 1 : 0));
}

int ZombieTempoPolicy::OpeningEconomyCeiling(int baseline) const {
    return std::max(1, baseline + (mEnhanced ? 1 : 0));
}

int ZombieTempoPolicy::OpeningPressureRowTarget(int baseline, int rows) const {
    return mEnhanced ? std::min(rows, baseline + 1) : baseline;
}

bool ZombieTempoPolicy::ShouldExtendPressure(int economyCount, int activePressureRows, int rows) const {
    if (!mEnhanced) {
        return false;
    }
    const int desiredRows = OpeningPressureRowTarget(std::min(3, rows), rows);
    const int minimumEconomy = OpeningEconomyFloor(std::min(2, std::max(1, rows)));
    // Keep one effective economy step ahead of every live opening route.
    // This preserves the replay cadence of grave, probe, grave, probe at
    // low economy, but lets enhanced AI restore a cleared pressure lane
    // instead of rebuilding the entire rear field first.
    return activePressureRows < desiredRows && economyCount >= std::max(minimumEconomy, activePressureRows + 2);
}

int ZombieTempoPolicy::CommitPressureRowTarget(int baseline, int rows) const {
    return std::clamp(baseline - (mEnhanced ? 1 : 0), 0, rows);
}

int ZombieTempoPolicy::AttackCommitPressureRowTarget(int baseline, int rows) const {
    return std::max(1, CommitPressureRowTarget(baseline, rows));
}

bool ZombieTempoPolicy::HasCommitPressure(int activePressureRows, int baseline, int rows) const {
    return activePressureRows >= CommitPressureRowTarget(baseline, rows);
}

bool ZombieTempoPolicy::HasAttackCommitPressure(int activePressureRows, int baseline, int rows) const {
    return activePressureRows >= AttackCommitPressureRowTarget(baseline, rows);
}

int ZombieTempoPolicy::CommitEconomyFloor(int baseline) const {
    return std::max(1, baseline - (mEnhanced ? 1 : 0));
}

int ZombieTempoPolicy::HeavyBankEconomyThreshold(int rows, int heavyEconomyThreshold) const {
    return std::max(rows + (mEnhanced ? 1 : 2), heavyEconomyThreshold - (mEnhanced ? 3 : 2));
}

int ZombieTempoPolicy::HeavyCommitEconomyThreshold(int rows, int heavyEconomyThreshold) const {
    return std::max(rows * 2, heavyEconomyThreshold - (mEnhanced ? 3 : 2));
}

int ZombieTempoPolicy::EconomyRepairDeficitThreshold() const {
    return 3;
}

int ZombieTempoPolicy::PressureRepairDeficitTolerance() const {
    return EconomyRepairDeficitThreshold() - 1;
}

std::uint8_t ZombieTempoPolicy::LaneAttackCooldown(SeedType seed) const {
    const std::uint8_t baseline = IsZombieFastAttackSeed(seed) ? 4 : 3;
    return mEnhanced && baseline > 1 ? static_cast<std::uint8_t>(baseline - 1) : baseline;
}

ZombieTempoPolicy GetZombieTempoPolicy() {
    return ZombieTempoPolicy(vsai::IsEnhancedAIEnabled() && vsai::IsSideEnabled(VSSide::Zombies));
}

bool IsZombieTargetedSeed(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieTargeted);
}

bool IsZombieEconomySeed(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieEconomy);
}

bool IsZombieFrontlineProbeSeed(SeedType seed) {
    if (IsZombieEconomySeed(seed) || IsZombieTargetedSeed(seed) || IsHeavyZombieSeed(seed)) {
        return false;
    }
    return seed != SeedType::SEED_ZOMBIE_TRASHCAN && seed != SeedType::SEED_ZOMBIE_WALLNUT_HEAD && seed != SeedType::SEED_ZOMBIE_TALLNUT_HEAD;
}

bool IsZombieFastAttackSeed(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieFastAttack);
}

bool IsZombieMetalGraveGuard(SeedType seed) {
    return seed == SeedType::SEED_ZOMBIE_PAIL || seed == SeedType::SEED_ZOMBIE_SCREEN_DOOR || seed == SeedType::SEED_ZOMBIE_TRASHCAN;
}

bool IsZombieLobbedScreenDonation(SeedType seed) {
    return HasZombieCardRole(seed, VSCardRole::ZombieLobbedScreen);
}

int ZombieGraveGuardPriority(SeedType seed) {
    switch (seed) {
        case SeedType::SEED_ZOMBIE_TRASHCAN:
            return 520;
        case SeedType::SEED_ZOMBIE_TALLNUT_HEAD:
            return 465;
        case SeedType::SEED_ZOMBIE_WALLNUT_HEAD:
            return 410;
        case SeedType::SEED_ZOMBIE_SCREEN_DOOR:
            return 380;
        case SeedType::SEED_ZOMBIE_PAIL:
            return 340;
        case SeedType::SEED_ZOMBIE_SUNDAY_EDITION:
            return 285;
        case SeedType::SEED_ZOMBIE_NEWSPAPER:
            return 255;
        case SeedType::SEED_ZOMBIE_TRAFFIC_CONE:
            return 160;
        default:
            return 0;
    }
}

} // namespace vsai::detail
