#include "ZombieCardRules.h"

#include <algorithm>

namespace vsai::detail {

int ZombieTemplateTacticalBonus(const ZombieTemplateProfile &profile, SeedType seed, const ZombieTemplateTacticalState &state) {
    const int economyCount = state.economyCount;
    const int activePressureRows = state.activePressureRows;
    const int attackCommitPressureRows = state.attackCommitPressureRows;
    const int zombiesInRow = state.zombiesInRow;
    const int rows = state.rows;
    const int peaHeadCount = state.peaHeadCount;
    const int plantCount = state.plantCount;
    const int economyValue = state.economyValue;
    const int sustainedOutput = state.sustainedOutput;
    const int areaCounterExposure = state.areaCounterExposure;
    const bool hasWallnut = state.hasWallnut;
    const bool graveUnderDirectFire = state.graveUnderDirectFire;
    const bool plantHasNutCard = state.plantHasNutCard;
    const bool plantHasHighValueCarryCard = state.plantHasHighValueCarryCard;
    const bool emptyRoute = zombiesInRow == 0;
    const bool openingWindow = economyCount >= 2 && economyCount <= rows + 2 && activePressureRows < std::min(rows, 3);
    const bool conversionWindow = activePressureRows >= attackCommitPressureRows && areaCounterExposure < 140;
    const bool developedTarget = plantCount >= 2 || economyValue >= 80 || sustainedOutput >= 65 || hasWallnut;
    int bonus = 0;

    // Template tactics only refine candidates accepted by the placement,
    // counter, grave-protection, and lane-spread rules.
    if (profile.fastPressure && (seed == SeedType::SEED_ZOMBIE_NORMAL || seed == SeedType::SEED_ZOMBIE_DOGWALKER || seed == SeedType::SEED_ZOMBIE_FLAG)) {
        bonus += 120;
    }
    if (profile.rangedSiege && seed == SeedType::SEED_ZOMBIE_PEA_HEAD) {
        bonus += 145 + economyValue;
    }
    if (profile.sundayPressure && !profile.Has(ZombieTemplate::ImpSledSunday) && seed == SeedType::SEED_ZOMBIE_SUNDAY_EDITION && economyCount >= std::max(3, rows - 1) && conversionWindow
        && areaCounterExposure < 150) {
        bonus += 155;
    }
    if (profile.Has(ZombieTemplate::ZamboniPole)) {
        if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount <= rows + 2)
            bonus += emptyRoute ? 145 : -115;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && economyCount <= rows + 2)
            bonus += emptyRoute ? 95 : -100;
        else if (seed == SeedType::SEED_ZOMBONI && economyCount >= 3 && economyCount <= rows + 3)
            bonus += emptyRoute ? 155 : -135;
    }
    if (profile.Has(ZombieTemplate::PeaHeadGiant)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 230 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && peaHeadCount < std::min(rows, 3))
            bonus -= 360;
        else if (seed == SeedType::SEED_ZOMBIE_TRASHCAN && peaHeadCount < std::min(rows, 3) && !graveUnderDirectFire)
            bonus -= 340;
        else if ((seed == SeedType::SEED_ZOMBIE_FOOTBALL || seed == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL) && peaHeadCount >= 2 && economyCount >= rows + 2)
            bonus += 180;
    }
    if (profile.Has(ZombieTemplate::ImpSledSunday)) {
        if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount >= 3 && economyCount <= rows * 2)
            bonus += emptyRoute ? 155 : -135;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 3 && economyCount <= rows * 2)
            bonus += emptyRoute ? 90 : -110;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED) {
            const bool lateSledWindow = economyCount >= std::max(rows + 2, 8) && activePressureRows >= std::max(attackCommitPressureRows, 2) && areaCounterExposure < 135;
            bonus += lateSledWindow && emptyRoute ? 130 : -240;
        } else if (seed == SeedType::SEED_ZOMBIE_SUNDAY_EDITION) {
            const bool sundayWindow = economyCount >= std::max(rows + 2, 8) && activePressureRows >= attackCommitPressureRows && areaCounterExposure < 145;
            bonus += sundayWindow && developedTarget ? 190 : -165;
        }
    }
    if (profile.Has(ZombieTemplate::ArmoredNormalRush)) {
        if (seed == SeedType::SEED_ZOMBIE_NORMAL && economyCount >= 1 && economyCount <= rows + 2)
            bonus += emptyRoute ? 135 : -180;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR && economyCount >= 8 && conversionWindow && developedTarget)
            bonus += emptyRoute ? 165 : -150;
    }
    if (profile.Has(ZombieTemplate::NewspaperDiggerGiga)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount <= rows + 2)
            bonus += emptyRoute ? 135 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_NORMAL && economyCount <= rows + 2)
            bonus += emptyRoute ? 110 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_DIGGER)
            bonus += economyCount >= rows && developedTarget ? 245 : -320;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR && economyCount >= std::max(rows * 2, 9) && conversionWindow && developedTarget)
            bonus += emptyRoute ? 165 : -155;
    }
    if (profile.Has(ZombieTemplate::NewspaperSledDiggerGiga)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount <= rows + 2)
            bonus += emptyRoute ? 135 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED && economyCount >= rows && conversionWindow)
            bonus += emptyRoute ? 145 : -190;
    }
    if (profile.Has(ZombieTemplate::ConeImpFootballGiant)) {
        if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 115 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount <= rows + 2)
            bonus += emptyRoute ? 130 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount < 3 && !graveUnderDirectFire)
            bonus -= 210;
        else if ((seed == SeedType::SEED_ZOMBIE_FOOTBALL || seed == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL) && conversionWindow && (hasWallnut || sustainedOutput >= 75))
            bonus += emptyRoute ? 155 : -140;
    }
    if (profile.Has(ZombieTemplate::NormalNewsSled)) {
        if (seed == SeedType::SEED_ZOMBIE_NORMAL && openingWindow)
            bonus += emptyRoute ? 110 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && openingWindow)
            bonus += emptyRoute ? 150 : -140;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED && economyCount >= 3 && conversionWindow)
            bonus += emptyRoute ? 175 : -210;
        else if (seed == SeedType::SEED_ZOMBONI && economyCount >= 3 && developedTarget)
            bonus += emptyRoute ? 155 : -185;
    }
    if (profile.Has(ZombieTemplate::NormalNewsImpSunday)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount >= 2 && economyCount <= rows)
            bonus += emptyRoute ? 185 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount >= 3 && economyCount <= rows + 2)
            bonus += emptyRoute ? 165 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_NORMAL && economyCount >= 3 && economyCount <= rows + 2)
            bonus += emptyRoute ? 145 : -185;
    }
    if (profile.Has(ZombieTemplate::LadderPole)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount <= rows + 2)
            bonus += emptyRoute ? 135 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 115 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER && economyCount >= 2 && activePressureRows >= 1 && (plantCount >= 1 || economyValue >= 50) && areaCounterExposure < 120)
            bonus += emptyRoute ? 235 : -145;
    }
    if (profile.Has(ZombieTemplate::NewspaperFanPole)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount <= rows + 2)
            bonus += emptyRoute ? 135 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_SUPER_FAN_IMP && openingWindow)
            bonus += emptyRoute ? 125 : -115;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER && economyCount >= 3 && conversionWindow && developedTarget)
            bonus += emptyRoute ? 180 : -145;
    }
    if (profile.Has(ZombieTemplate::PeaHeadSunday)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 175 : -160;
        else if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount <= rows + 2)
            bonus += emptyRoute ? 130 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_SUNDAY_EDITION && economyCount >= rows + 1 && peaHeadCount >= 2 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 205 : -155;
    }
    if (profile.Has(ZombieTemplate::PeaHeadZamboni)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 150 : -155;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 3 && peaHeadCount >= 2)
            bonus += emptyRoute ? 100 : -125;
        else if (seed == SeedType::SEED_ZOMBONI && economyCount >= 3 && developedTarget)
            bonus += emptyRoute ? 155 : -185;
    }
    if (profile.Has(ZombieTemplate::PeaHeadFlagBungee)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && economyCount >= 2 && economyCount <= rows + 3 && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 185 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && peaHeadCount < std::min(rows, 3))
            bonus -= 260;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && economyCount >= 3 && peaHeadCount >= 2)
            bonus += emptyRoute ? 130 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_FLAG) {
            const bool release = economyCount >= rows * 2 && peaHeadCount >= std::min(rows, 3) && conversionWindow && areaCounterExposure < 120;
            bonus += release ? (emptyRoute ? 235 : 80) : -430;
        }
    }
    if (profile.Has(ZombieTemplate::MoundSkirmish)) {
        if (seed == SeedType::SEED_ZOMBIE_MOUND && economyCount >= 3)
            bonus += graveUnderDirectFire ? -160 : 115;
        else if (seed == SeedType::SEED_ZOMBIE_NORMAL && openingWindow)
            bonus += emptyRoute ? 110 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount <= rows + 2)
            bonus += emptyRoute ? 130 : -145;
        else if (seed == SeedType::SEED_ZOMBONI && economyCount >= 3 && developedTarget)
            bonus += emptyRoute ? 155 : -185;
    }
    if (profile.Has(ZombieTemplate::FlagSquash)) {
        if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 115 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_SQUASH_HEAD && economyCount >= 2 && developedTarget)
            bonus += emptyRoute ? 130 : -135;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR && economyCount >= 8 && conversionWindow && developedTarget)
            bonus += emptyRoute ? 155 : -150;
    }
    if (profile.Has(ZombieTemplate::FanImp)) {
        if (seed == SeedType::SEED_ZOMBIE_SUPER_FAN_IMP && openingWindow)
            bonus += emptyRoute ? 125 : -115;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 115 : -120;
        else if (seed == SeedType::SEED_ZOMBIE_SQUASH_HEAD && economyCount >= 2 && developedTarget)
            bonus += emptyRoute ? 130 : -135;
    }
    if (profile.Has(ZombieTemplate::MoundTallnutSled)) {
        if (seed == SeedType::SEED_ZOMBIE_MOUND && economyCount >= 3)
            bonus += graveUnderDirectFire ? -160 : 115;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED && economyCount >= rows && hasWallnut)
            bonus += emptyRoute ? 115 : -160;
    }
    if (profile.Has(ZombieTemplate::ImpLadderFootball)) {
        if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount <= rows + 2)
            bonus += emptyRoute ? 130 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_LADDER && (hasWallnut || plantHasNutCard))
            bonus += emptyRoute ? 125 : -95;
        else if ((seed == SeedType::SEED_ZOMBIE_FOOTBALL || seed == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL) && conversionWindow && (hasWallnut || sustainedOutput >= 75))
            bonus += emptyRoute ? 155 : -140;
    }
    if (profile.Has(ZombieTemplate::SledDogHeavy)) {
        if (seed == SeedType::SEED_ZOMBIE_DOGWALKER && openingWindow)
            bonus += emptyRoute ? 160 : -155;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED && economyCount >= rows && conversionWindow)
            bonus += emptyRoute ? 135 : -180;
    }
    if (profile.Has(ZombieTemplate::DogSledPea)) {
        if (seed == SeedType::SEED_ZOMBIE_DOGWALKER && economyCount >= 2 && economyCount <= rows + 2)
            bonus += emptyRoute ? 175 : -200;
        else if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && economyCount >= 3 && economyCount <= rows + 2 && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 185 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED && economyCount >= rows && peaHeadCount >= 2 && conversionWindow && areaCounterExposure < 135)
            bonus += emptyRoute ? 170 : -230;
    }
    if (profile.Has(ZombieTemplate::LadderBalloonZamboni)) {
        if (seed == SeedType::SEED_ZOMBIE_BALLOON && openingWindow)
            bonus += emptyRoute ? 185 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 135 : -135;
        else if (seed == SeedType::SEED_ZOMBIE_LADDER && openingWindow && (hasWallnut || plantHasNutCard))
            bonus += 125;
        else if (seed == SeedType::SEED_ZOMBONI && economyCount >= 2 && emptyRoute && (hasWallnut || plantHasNutCard || sustainedOutput >= 65))
            bonus += 175;
    }
    if (profile.Has(ZombieTemplate::MoundBungeeFootball)) {
        if (seed == SeedType::SEED_ZOMBIE_MOUND && economyCount >= 2 && economyCount <= rows + 2 && !graveUnderDirectFire)
            bonus += 310;
        else if (seed == SeedType::SEED_ZOMBIE_BUNGEE && economyCount >= rows && plantHasHighValueCarryCard)
            bonus += 145;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL && economyCount >= rows && conversionWindow)
            bonus += emptyRoute ? 165 : -150;
    }
    if (profile.Has(ZombieTemplate::NewspaperImpFootballGiant)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && openingWindow)
            bonus += emptyRoute ? 175 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_IMP && openingWindow)
            bonus += emptyRoute ? 170 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 120 : -140;
        else if (seed == SeedType::SEED_ZOMBIE_FOOTBALL && economyCount >= rows && conversionWindow && (hasWallnut || sustainedOutput >= 65))
            bonus += emptyRoute ? 165 : -150;
        else if (seed == SeedType::SEED_ZOMBIE_GARGANTUAR && economyCount >= rows + 2 && conversionWindow && developedTarget)
            bonus += emptyRoute ? 145 : -145;
    }
    if (profile.Has(ZombieTemplate::PeaHeadZomblobGiant)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 180 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_BOBSLED && economyCount >= rows && peaHeadCount >= 2)
            bonus += emptyRoute ? 150 : -150;
        else if (seed == SeedType::SEED_ZOMBIE_ZOMBLOB && economyCount >= 3 && peaHeadCount >= 2 && conversionWindow)
            bonus += emptyRoute ? 185 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR && economyCount >= 8 && peaHeadCount >= 2 && conversionWindow && developedTarget)
            bonus += emptyRoute ? 165 : -155;
    }
    if (profile.Has(ZombieTemplate::ImpPailSledFootball)) {
        if (seed == SeedType::SEED_ZOMBIE_IMP && openingWindow)
            bonus += emptyRoute ? 180 : -180;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && openingWindow)
            bonus += emptyRoute ? 150 : -160;
        else if (seed == SeedType::SEED_ZOMBIE_FOOTBALL && economyCount >= 2 && conversionWindow)
            bonus += emptyRoute ? 175 : -170;
    }
    if (profile.Has(ZombieTemplate::NewspaperScreenFootball)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && openingWindow)
            bonus += emptyRoute ? 175 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 3 && economyCount <= rows + 2)
            bonus += emptyRoute ? 145 : -125;
        else if (seed == SeedType::SEED_ZOMBIE_SCREEN_DOOR && economyCount >= rows && graveUnderDirectFire && emptyRoute)
            bonus += 175;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL && economyCount >= 6 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 205 : -155;
    }
    if (profile.Has(ZombieTemplate::DogPeaFootball)) {
        if (seed == SeedType::SEED_ZOMBIE_DOGWALKER && openingWindow)
            bonus += emptyRoute ? 170 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 195 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && economyCount >= 3 && peaHeadCount >= 2)
            bonus += emptyRoute ? 135 : -140;
        else if (seed == SeedType::SEED_ZOMBIE_FOOTBALL && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 190 : -160;
    }
    if (profile.Has(ZombieTemplate::NewspaperFootballPole)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && openingWindow)
            bonus += emptyRoute ? 185 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_NORMAL && economyCount >= 3 && economyCount <= rows + 2)
            bonus += emptyRoute ? 155 : -160;
        else if (seed == SeedType::SEED_ZOMBIE_FOOTBALL && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 170 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_POLEVAULTER && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 215 : -145;
    }
    if (profile.Has(ZombieTemplate::DancerRaid)) {
        if (seed == SeedType::SEED_ZOMBIE_DANCER && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 225 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 5 && activePressureRows >= 1)
            bonus += emptyRoute ? 135 : -130;
        else if (seed == SeedType::SEED_ZOMBIE_JACKSON && economyCount >= 6 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 245 : -175;
    }
    if (profile.Has(ZombieTemplate::PeaHeadRaid)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 200 : -180;
        else if (seed == SeedType::SEED_ZOMBIE_SQUASH_HEAD && economyCount >= 3 && activePressureRows >= 1)
            bonus += emptyRoute && developedTarget ? 190 : -135;
        else if (seed == SeedType::SEED_ZOMBIE_BUNGEE && economyCount >= 5 && conversionWindow)
            bonus += developedTarget ? 155 : -130;
    }
    if (profile.Has(ZombieTemplate::PeaHeadDancerRaid)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 195 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_DANCER && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 205 : -145;
        else if (seed == SeedType::SEED_ZOMBIE_JALAPENO_HEAD && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 215 : -165;
    }
    if (profile.Has(ZombieTemplate::MoundPeaZomblobFootball)) {
        if (seed == SeedType::SEED_ZOMBIE_PEA_HEAD && openingWindow && peaHeadCount < std::min(rows, 3))
            bonus += emptyRoute ? 185 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_MOUND && economyCount >= 2 && economyCount <= rows + 3)
            bonus += graveUnderDirectFire ? -175 : 215;
        else if (seed == SeedType::SEED_ZOMBIE_ZOMBLOB && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && peaHeadCount >= 2 && developedTarget ? 205 : -160;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_FOOTBALL && economyCount >= 7 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 190 : -155;
    }
    if (profile.Has(ZombieTemplate::SundayLadderRaid)) {
        if (seed == SeedType::SEED_ZOMBIE_NORMAL && economyCount >= 1 && economyCount <= rows + 1)
            bonus += emptyRoute ? 180 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_LADDER && economyCount >= 3 && (hasWallnut || developedTarget))
            bonus += emptyRoute ? 175 : -130;
        else if (seed == SeedType::SEED_ZOMBIE_SUNDAY_EDITION && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 225 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_JALAPENO_HEAD && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 195 : -150;
    }
    if (profile.Has(ZombieTemplate::MoundNewspaperZamboni)) {
        if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount >= 3 && economyCount <= rows + 2)
            bonus += emptyRoute ? 180 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_MOUND && economyCount >= 3 && economyCount <= rows + 3)
            bonus += graveUnderDirectFire ? -170 : 200;
        else if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount >= 3 && economyCount <= rows + 3)
            bonus += emptyRoute ? 155 : -145;
        else if (seed == SeedType::SEED_ZOMBONI && economyCount >= 5 && conversionWindow)
            bonus += emptyRoute && developedTarget ? 205 : -170;
    }
    if (profile.Has(ZombieTemplate::MoundTallnutGuard)) {
        if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 165 : -155;
        else if (seed == SeedType::SEED_ZOMBIE_MOUND && economyCount >= 3 && economyCount <= rows + 3)
            bonus += graveUnderDirectFire ? -180 : 220;
        else if (seed == SeedType::SEED_ZOMBIE_TALLNUT_HEAD && economyCount >= 5 && (graveUnderDirectFire || sustainedOutput >= 80))
            bonus += emptyRoute ? 205 : -130;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 3 && activePressureRows >= 1)
            bonus += emptyRoute ? 125 : -115;
    }
    if (profile.Has(ZombieTemplate::ConeSundayTallnut)) {
        if (seed == SeedType::SEED_ZOMBIE_TRAFFIC_CONE && openingWindow)
            bonus += emptyRoute ? 165 : -160;
        else if (seed == SeedType::SEED_ZOMBIE_TALLNUT_HEAD) {
            // The Tall-nut head in this replay is a grave guard, never a
            // generic slow attacker. Only expose it where direct fire has
            // already made the economic route worth protecting.
            bonus += economyCount >= 4 && (graveUnderDirectFire || sustainedOutput >= 85) ? (emptyRoute ? 195 : -125) : -220;
        } else if (seed == SeedType::SEED_ZOMBIE_SUNDAY_EDITION) {
            const bool release = economyCount >= 6 && conversionWindow && developedTarget && areaCounterExposure < 135;
            bonus += release ? (emptyRoute ? 220 : 55) : -210;
        } else if (seed == SeedType::SEED_ZOMBIE_BOBSLED) {
            const bool sledWindow = economyCount >= 5 && conversionWindow && areaCounterExposure < 125;
            bonus += sledWindow ? (emptyRoute ? 135 : -185) : -230;
        }
    }
    if (profile.Has(ZombieTemplate::NormalNewsImpGiga)) {
        if (seed == SeedType::SEED_ZOMBIE_NORMAL && openingWindow)
            bonus += emptyRoute ? 165 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && economyCount >= 2 && economyCount <= rows + 2)
            bonus += emptyRoute ? 185 : -175;
        else if (seed == SeedType::SEED_ZOMBIE_IMP && economyCount >= 3 && economyCount <= rows + 3)
            bonus += emptyRoute ? 145 : -165;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 3 && activePressureRows >= 2)
            bonus += emptyRoute ? 100 : -130;
        else if (seed == SeedType::SEED_ZOMBIE_GIGA_GARGANTUAR) {
            const bool release = economyCount >= 7 && conversionWindow && developedTarget && areaCounterExposure < 125;
            bonus += release ? (emptyRoute ? 230 : -135) : -280;
        }
    }
    if (profile.Has(ZombieTemplate::NewspaperLadderZamboniJack)) {
        if (seed == SeedType::SEED_ZOMBIE_NEWSPAPER && openingWindow)
            bonus += emptyRoute ? 180 : -170;
        else if (seed == SeedType::SEED_ZOMBIE_PAIL && economyCount >= 2 && economyCount <= rows + 2)
            bonus += emptyRoute ? 105 : -125;
        else if (seed == SeedType::SEED_ZOMBIE_LADDER) {
            bonus += economyCount >= 3 && (hasWallnut || developedTarget) ? (emptyRoute ? 175 : -135) : -190;
        } else if (seed == SeedType::SEED_ZOMBIE_JACK_IN_THE_BOX) {
            const bool jackWindow = economyCount >= 4 && conversionWindow && developedTarget && areaCounterExposure < 110;
            bonus += jackWindow ? (emptyRoute ? 150 : -180) : -250;
        } else if (seed == SeedType::SEED_ZOMBONI) {
            const bool zamboniWindow = economyCount >= 5 && conversionWindow && developedTarget && areaCounterExposure < 120;
            bonus += zamboniWindow ? (emptyRoute ? 210 : -200) : -260;
        }
    }
    return bonus;
}

} // namespace vsai::detail
