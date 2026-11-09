#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/Projectile.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/SnapshotReanimation.h"
#include "PvZ/TodLib/Effect/Attachment.h"
#include "PvZ/TodLib/Effect/EffectSystem.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

namespace netplay::snapshot {
namespace {
    uint32_t AnimationID(Board &board, Reanimation &animation) {
        return board.mApp->mEffectSystem->mReanimationHolder->mReanimations.DataArrayGetID(&animation);
    }

    Plant *PlantOwner(Board &board, uint32_t id) {
        Plant *plant = nullptr;
        while (board.mPlants.IterateNext(plant)) {
            if (id == plant->mBodyReanimID || id == plant->mHeadReanimID || id == plant->mHeadReanimID2 || id == plant->mHeadReanimID3 || id == plant->mBlinkReanimID || id == plant->mLightReanimID
                || id == plant->mSleepingReanimID)
                return plant;
        }
        return nullptr;
    }

    Zombie *ZombieBodyOwner(Board &board, uint32_t id) {
        Zombie *zombie = nullptr;
        while (board.mZombies.IterateNext(zombie))
            if (id == zombie->mBodyReanimID)
                return zombie;
        return nullptr;
    }

    std::vector<std::string_view> PlantActions(const Plant &plant, uint32_t id, const Reanimation &animation) {
        if (id == plant.mSleepingReanimID)
            return {"anim_idle"};
        if (id == plant.mLightReanimID)
            return {"anim_glow"};
        if (id == plant.mHeadReanimID || id == plant.mHeadReanimID2 || id == plant.mHeadReanimID3) {
            const bool shooting = animation.mLoopType == REANIM_PLAY_ONCE_AND_HOLD;
            if (plant.mSeedType == SEED_THREEPEATER) {
                if (id == plant.mHeadReanimID)
                    return {shooting ? "anim_shooting1" : "anim_head_idle1"};
                if (id == plant.mHeadReanimID2)
                    return {shooting ? "anim_shooting2" : "anim_head_idle2"};
                return {shooting ? "anim_shooting3" : "anim_head_idle3"};
            }
            if (plant.mSeedType == SEED_SPLITPEA && id == plant.mHeadReanimID2)
                return {shooting ? "anim_splitpea_shooting" : "anim_splitpea_idle"};
            return {shooting ? "anim_shooting" : "anim_head_idle"};
        }
        if (plant.mIsAsleep)
            return {plant.mSeedType == SEED_SUNSHROOM ? "anim_bigsleep" : "anim_sleep", "anim_idle"};
        switch (plant.mState) {
            case STATE_SQUASH_LOOK:
                return {plant.mTargetX < plant.mX ? "anim_lookleft" : "anim_lookright"};
            case STATE_SQUASH_PRE_LAUNCH:
            case STATE_SQUASH_RISING:
                return {"anim_jumpup"};
            case STATE_SQUASH_FALLING:
            case STATE_SQUASH_DONE_FALLING:
                return {"anim_jumpdown"};
            case STATE_CHOMPER_BITING:
            case STATE_CHOMPER_BITING_GOT_ONE:
            case STATE_CHOMPER_BITING_MISSED:
                return {"anim_bite"};
            case STATE_CHOMPER_SWALLOWING:
                return {"anim_swallow"};
            case STATE_CHOMPER_DIGESTING:
                return {"anim_chew"};
            case STATE_SPIKEWEED_ATTACKING:
            case STATE_SPIKEWEED_ATTACKING_2:
                return {"anim_attack"};
            case STATE_SUNSHROOM_GROWING:
                return {"anim_grow"};
            case STATE_SUNSHROOM_BIG:
                return {"anim_bigidle", "anim_idle"};
            case STATE_POTATO_RISING:
                return {"anim_rise"};
            case STATE_POTATO_ARMED:
                return {"anim_armed"};
            case STATE_POTATO_MASHED:
                return {"anim_explode"};
            case STATE_GRAVEBUSTER_LANDING:
                return {"anim_land"};
            case STATE_MAGNETSHROOM_SUCKING:
                return {"anim_shooting"};
            case STATE_MAGNETSHROOM_CHARGING:
                return {"anim_nonactive_idle2"};
            case STATE_COBCANNON_ARMING:
                return {"anim_unarmed_idle"};
            case STATE_COBCANNON_LOADING:
                return {"anim_charge"};
            case STATE_COBCANNON_FIRING:
                return {"anim_shooting"};
            case STATE_IMITATER_MORPHING:
                return {"anim_explode"};
            case STATE_CACTUS_HIGH:
                return {plant.mShootingCounter != 0 ? "anim_shootinghigh" : "anim_idlehigh"};
            case STATE_UMBRELLA_TRIGGERED:
            case STATE_UMBRELLA_REFLECTING:
                return {"anim_block"};
            case STATE_CELERY_STALKER_LOW:
                return {"anim_idlelow"};
            case STATE_CELERY_STALKER_HIGH:
                return {"anim_idle"};
            case STATE_CELERY_STALKER_RISING:
                return {"anim_rise"};
            case STATE_CELERY_STALKER_LOWERING:
                return {"anim_lower"};
            case STATE_CELERY_STALKER_PUNCHING:
                return {"anim_punch"};
            case STATE_CELERY_STALKER_ATTACKING:
                return {"anim_attack"};
            case STATE_CELERY_STALKER_STOPPING:
            case STATE_ENDURIAN_STOPPING:
                return {"anim_stop"};
            case STATE_ENDURIAN_STARTING:
            case STATE_ENDURIAN_ATTACKING:
                return {"anim_start"};
            case STATE_BONK_CHOY_PUNCHING:
                return {"anim_rightpunch"};
            case STATE_BONK_CHOY_UPPERCUTTING:
                return {"anim_rightuppercut"};
            default:
                break;
        }
        if (plant.mSeedType == SEED_RED_STINGER) {
            const int form = plant.mTargetX;
            if (plant.mShootingCounter != 0)
                return {form == 1 ? "anim_shooting1" : form == 2 ? "anim_shooting2" : "anim_shooting3"};
            return {form == 1 ? "anim_idle1" : form == 2 ? "anim_idle2" : "anim_idle3", "anim_idle"};
        }
        if (plant.mHeadReanimID != REANIMATIONID_NULL)
            return {"anim_idle"}; // Shooting belongs to the head copies, not this body.
        if (plant.mSeedType == SEED_PEANUT && plant.mShootingCounter != 0)
            return {plant.mShootingCounter > 40 ? "anim_shooting1" : "anim_shooting2"};
        if (plant.mShootingCounter != 0)
            return {"anim_shooting", "anim_shoot", "anim_attack"};
        return {}; // Keep recognizable special/idle clips when no state identifies them.
    }

    std::vector<std::string_view> ZombieActions(const Zombie &zombie) {
        switch (zombie.mZombiePhase) {
            case PHASE_ZOMBIE_DYING:
                return {"anim_death", "anim_superlongdeath"};
            case PHASE_POLEVAULTER_IN_VAULT:
            case PHASE_DOLPHIN_IN_JUMP:
                return {"anim_jump"};
            case PHASE_POLEVAULTER_PRE_VAULT:
            case PHASE_IMP_RUNNING:
            case PHASE_DOG_RUNNING:
            case PHASE_YETI_RUNNING:
                return {"anim_run", "anim_walk"};
            case PHASE_GARGANTUAR_SMASHING:
                return {"anim_smash"};
            case PHASE_GARGANTUAR_THROWING:
                return {"anim_throw"};
            case PHASE_IMP_GETTING_THROWN:
                return {"anim_thrown"};
            case PHASE_IMP_LANDING:
                return {"anim_land"};
            case PHASE_RISING_FROM_GRAVE:
                return {"anim_raise"};
            case PHASE_DIGGER_RISING:
            case PHASE_DIGGER_RISE_WITHOUT_AXE:
                return {"anim_dig"};
            case PHASE_LADDER_PLACING:
                return {"anim_placeladder"};
            case PHASE_FOOTBALL_PRE_CHARGE:
                return {"anim_prepare"};
            case PHASE_FOOTBALL_CHARGING:
                return {"anim_charge"};
            default:
                break;
        }
        if (zombie.mIsEating) {
            if (zombie.mZombiePhase == PHASE_LADDER_CARRYING)
                return {"anim_laddereat", "anim_eat"};
            if (zombie.mZombiePhase == PHASE_NEWSPAPER_MAD)
                return {"anim_eat_nopaper", "anim_eat"};
            return {"anim_eat"};
        }
        if (zombie.mZombiePhase == PHASE_LADDER_CARRYING)
            return {"anim_ladderwalk", "anim_walk"};
        if (zombie.mZombiePhase == PHASE_NEWSPAPER_MAD)
            return {"anim_walk_nopaper", "anim_walk"};
        if (zombie.mInPool)
            return {"anim_swim", "anim_walk"};
        return {"anim_walk", "anim_walk2", "anim_idle"};
    }
} // namespace

bool PrepareSnapshotCosmetics(Board &board, CosmeticSnapshotState &state) {
    state.prepared = true;
    auto &animations = board.mApp->mEffectSystem->mReanimationHolder->mReanimations;
    Reanimation *animation = nullptr;
    while (animations.IterateNext(animation)) {
        if (IsTransientCosmeticAnimation(animation->mReanimationType))
            state.animations.push_back(animations.DataArrayGetID(animation));
    }
    const auto useLocalAnimation = [&](uint32_t id, LocalCosmeticAnimation kind) {
        if (id != REANIMATIONID_NULL && kind != LocalCosmeticAnimation::None && animations.DataArrayTryToGet(id))
            state.localAnimations.emplace(id, kind);
    };
    Plant *plant = nullptr;
    while (board.mPlants.IterateNext(plant)) {
        if (plant->mBlinkReanimID != REANIMATIONID_NULL) {
            state.animations.push_back(plant->mBlinkReanimID);
            state.eyeBodies.push_back(plant->mBodyReanimID);
            plant->mBlinkReanimID = REANIMATIONID_NULL;
        }
        useLocalAnimation(plant->mSleepingReanimID, LocalCosmeticAnimation::Sleeping);
        if (plant->mSeedType == SEED_POTATOMINE)
            useLocalAnimation(plant->mLightReanimID, LocalCosmeticAnimation::PotatoLight);
        // Keep mBlinkCountdown: the next blink runs locally on the normal
        // schedule, without consuming RNG or changing gameplay counters here.
    }
    auto &attachments = board.mApp->mEffectSystem->mAttachmentHolder->mAttachments;
    Coin *coin = nullptr;
    while (board.mCoins.IterateNext(coin)) {
        for (const auto attachmentID : coin->mAttachmentID) {
            if (attachmentID == ATTACHMENTID_NULL)
                continue;
            const auto *attachment = attachments.DataArrayTryToGet(attachmentID);
            if (!attachment || attachment->mDead)
                continue;
            if (attachment->mNumEffects < 0 || attachment->mNumEffects > 16)
                return false;
            for (int i = 0; i < attachment->mNumEffects; ++i) {
                const auto &effect = attachment->mEffectArray[i];
                if (effect.mEffectType != EFFECT_REANIM)
                    continue;
                const auto *child = animations.DataArrayTryToGet(effect.mEffectID);
                if (child)
                    useLocalAnimation(effect.mEffectID, CoinCosmeticAnimation(child->mReanimationType));
            }
        }
    }
    if (state.animations.empty())
        return true;
    Attachment *attachment = nullptr;
    while (attachments.IterateNext(attachment)) {
        const int original = attachment->mNumEffects;
        if (!StripCosmeticEffects(*attachment, [&](const AttachEffect &effect) { return effect.mEffectType == EFFECT_REANIM && state.DiscardsAnimation(effect.mEffectID); }))
            return false;
        if (original != 0 && attachment->mNumEffects == 0)
            state.attachments.push_back(attachments.DataArrayGetID(attachment));
    }
    return true;
}

void FinishSnapshotCosmetics(Board &board, const CosmeticSnapshotState &state) {
    auto &animations = board.mApp->mEffectSystem->mReanimationHolder->mReanimations;
    for (const auto id : state.eyeBodies) {
        auto *body = animations.DataArrayTryToGet(id);
        if (body && body->mDefinition && body->mTrackInstances)
            body->AssignRenderGroupToPrefix("anim_eye", RENDER_GROUP_NORMAL);
    }
    // Animation tracks have already been filtered by SyncReanimation. Other
    // entity owners can also point at an attachment emptied by cosmetic pruning.
    const auto clearAttachment = [&](AttachmentID &id) {
        if (std::find(state.attachments.begin(), state.attachments.end(), static_cast<uint32_t>(id)) != state.attachments.end())
            id = ATTACHMENTID_NULL;
    };
    Coin *coin = nullptr;
    while (board.mCoins.IterateNext(coin))
        for (auto &id : coin->mAttachmentID)
            clearAttachment(id);
    Projectile *projectile = nullptr;
    while (board.mProjectiles.IterateNext(projectile))
        clearAttachment(projectile->mAttachmentID);
    Zombie *zombie = nullptr;
    while (board.mZombies.IterateNext(zombie))
        clearAttachment(zombie->mAttachmentID);
}

bool RestoreLocalCosmeticAnimation(Board &board, Reanimation &animation, LocalCosmeticAnimation kind, std::span<const ReanimatorTrackInstance> source) {
    AnimationLayout local;
    AnimationPlayback playback{};
    if (!DescribeAnimation(*animation.mDefinition, local) || !MakeCosmeticPlayback(local, kind, animation.mDefinition->mFPS, animation.mAnimRate == 0, playback))
        return false;
    if (!RestoreLocalAnimationAttachments(board, animation, source))
        return false;
    animation.mFrameStart = playback.start;
    animation.mFrameCount = playback.count;
    animation.mFrameBasePose = playback.basePose;
    animation.mAnimTime = playback.time;
    animation.mLastFrameTime = playback.lastTime;
    animation.mLoopCount = 0;
    animation.mLoopType = REANIM_LOOP;
    animation.unk2[0] = 7; // Recompute cached frame time/track transforms locally.
    animation.SetAnimRate(playback.rate);
    if (kind == LocalCosmeticAnimation::PotatoLight) {
        animation.ShowOnlyTrack("anim_glow");
        animation.SetTruncateDisappearingFrames("anim_glow", false);
    }
    // Keep ID, parent attachment, position, scale, tint, dead/frozen state and
    // every owner field. Never call CoinInitialize/SetSleeping/UpdatePotato.
    return true;
}

std::vector<std::string_view> LocalAnimationActions(Board &board, Reanimation &animation) {
    const auto id = AnimationID(board, animation);
    // At this point SyncBoard has loaded scalar state/IDs, but object pointers
    // still belong to the host. Do not call Plant/Zombie initialization or their
    // action methods: they can change counters, allocate effects and consume RNG.
    if (const auto *plant = PlantOwner(board, id))
        return PlantActions(*plant, id, animation);
    if (const auto *zombie = ZombieBodyOwner(board, id))
        return ZombieActions(*zombie);
    GridItem *item = nullptr;
    while (board.mGridItems.IterateNext(item)) {
        if (item->mGridItemReanimID != id || item->mGridItemType != GRIDITEM_MP_TARGET_ZOMBIE)
            continue;
        if (item->mVSTargetZombieHealth <= 0)
            return {item->mVSTargetZombieDieCounter > 0 ? "anim_death2" : "anim_death", "anim_death"};
        // Leave a completed hit on its final frame. GridItem::Update performs
        // the normal transition to a looping idle on the next frame.
        if (animation.mLoopType == REANIM_PLAY_ONCE_AND_HOLD)
            return {"anim_hit", "anim_idle"};
        return {"anim_idle"};
    }
    return {};
}

void RestoreLocalAnimationLayers(Board &board, Reanimation &animation) {
    const auto id = AnimationID(board, animation);
    if (auto *zombie = ZombieBodyOwner(board, id)) {
        Zombie::SetupReanimLayers(&animation, zombie->mZombieType);
        Zombie::SetupShieldReanims(zombie->mZombieType, &animation);
        if (!zombie->mHasHead)
            animation.AssignRenderGroupToPrefix("anim_head", RENDER_GROUP_HIDDEN);
        if (zombie->mHelmHealth <= 0) {
            animation.AssignRenderGroupToPrefix("anim_cone", RENDER_GROUP_HIDDEN);
            animation.AssignRenderGroupToPrefix("anim_bucket", RENDER_GROUP_HIDDEN);
        }
        // FixBoardAfterLoad applies lost-arm and damaged-equipment images later.
    }
    if (const auto *plant = PlantOwner(board, id)) {
        if (id == plant->mLightReanimID) {
            if (animation.TrackExists("anim_glow")) {
                animation.ShowOnlyTrack("anim_glow");
                animation.SetTruncateDisappearingFrames("anim_glow", false);
            }
        } else if (id == plant->mBodyReanimID) {
            if (plant->mSeedType == SEED_POTATOMINE)
                animation.AssignRenderGroupToPrefix("anim_glow", RENDER_GROUP_HIDDEN);
            if (plant->mSeedType == SEED_PUMPKINSHELL)
                animation.AssignRenderGroupToPrefix("Pumpkin_back", 1);
            if (plant->mSeedType == SEED_KERNELPULT) {
                animation.AssignRenderGroupToPrefix("Cornpult_butter", plant->mState == STATE_KERNELPULT_BUTTER ? 0 : RENDER_GROUP_HIDDEN);
                animation.AssignRenderGroupToPrefix("Cornpult_kernal", plant->mState == STATE_KERNELPULT_BUTTER ? RENDER_GROUP_HIDDEN : 0);
            }
        }
    }
}

bool RestoreLocalAnimationAttachments(Board &board, Reanimation &animation, std::span<const ReanimatorTrackInstance> source) {
    const auto id = AnimationID(board, animation);
    const auto *plant = PlantOwner(board, id);
    AnimationLayout local;
    if (!DescribeAnimation(*animation.mDefinition, local))
        return false;
    std::map<int, AttachmentID> bindings;
    std::map<AttachmentID, int> owners;
    auto &attachments = board.mApp->mEffectSystem->mAttachmentHolder->mAttachments;
    for (const auto &track : source) {
        if (track.mAttachmentID == ATTACHMENTID_NULL)
            continue;
        const auto *attachment = attachments.DataArrayTryToGet(track.mAttachmentID);
        if (!attachment || attachment->mDead || attachment->mNumEffects < 0 || attachment->mNumEffects > 16)
            return false;
        if (attachment->mNumEffects == 0)
            continue;
        int destination = -1;
        for (int i = 0; i < attachment->mNumEffects; ++i) {
            const auto &effect = attachment->mEffectArray[i];
            if (!plant || effect.mEffectType != EFFECT_REANIM || effect.mEffectID == REANIMATIONID_NULL || effect.mEffectID == id
                || !board.mApp->mEffectSystem->mReanimationHolder->mReanimations.DataArrayTryToGet(effect.mEffectID))
                return false;
            std::vector<std::string_view> anchors;
            if (id != plant->mBodyReanimID)
                return false;
            else if (effect.mEffectID == plant->mLightReanimID)
                anchors = {"anim_light"};
            else if (effect.mEffectID == plant->mHeadReanimID)
                anchors = plant->mSeedType == SEED_THREEPEATER ? std::vector<std::string_view>{"anim_head1"} : std::vector<std::string_view>{"anim_stem", "anim_idle"};
            else if (effect.mEffectID == plant->mHeadReanimID2)
                anchors = plant->mSeedType == SEED_THREEPEATER ? std::vector<std::string_view>{"anim_head2"} : std::vector<std::string_view>{"anim_idle"};
            else if (effect.mEffectID == plant->mHeadReanimID3)
                anchors = {"anim_head3"};
            int target = -1;
            for (const auto anchor : anchors) {
                target = FindTrack(local, anchor);
                if (target >= 0)
                    break;
            }
            if (target < 0 || (destination >= 0 && destination != target))
                return false;
            destination = target;
        }
        const auto [entry, inserted] = bindings.emplace(destination, track.mAttachmentID);
        if (!inserted && entry->second != track.mAttachmentID)
            return false;
        // Each attachment must have exactly one parent track, otherwise the
        // engine would update/draw/free the same child more than once.
        if (!owners.emplace(track.mAttachmentID, destination).second)
            return false;
    }
    // Commit only after every attachment has an unambiguous local anchor.
    for (const auto &[index, attachment] : bindings)
        animation.mTrackInstances[index].mAttachmentID = attachment;
    return true;
}
} // namespace netplay::snapshot
