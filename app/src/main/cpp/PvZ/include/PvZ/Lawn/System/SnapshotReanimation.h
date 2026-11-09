#pragma once

#include "PvZ/Lawn/Common/ConstEnums.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class ReanimatorDefinition;
class Board;
class Reanimation;
class ReanimatorTrackInstance;

namespace netplay::snapshot {

constexpr size_t kMaxLayoutBytes = 2 * 1024 * 1024;
constexpr int kMaxLayoutTracks = 4096;
constexpr int kMaxLayoutFrames = 1000000;

struct TrackLayout {
    std::string name;
    int frames = 0;
    int start = 0;
    int count = 0;
    bool operator==(const TrackLayout &) const = default;
};
struct AnimationLayout {
    int frames = 0; // First track's count, as used by ReanimationInitialize.
    std::vector<TrackLayout> tracks;
    bool operator==(const AnimationLayout &) const = default;
    int SafeFrames() const {
        int result = frames;
        for (const auto &track : tracks)
            result = std::min(result, track.frames);
        return result;
    }
};
using AnimationLayouts = std::map<int, AnimationLayout>;

bool DescribeAnimation(const ReanimatorDefinition &definition, AnimationLayout &layout);
const AnimationLayout *GetBundledAnimationLayout(int type);
std::vector<std::string_view> LocalAnimationActions(Board &board, Reanimation &animation);
void RestoreLocalAnimationLayers(Board &board, Reanimation &animation);
bool RestoreLocalAnimationAttachments(Board &board, Reanimation &animation, std::span<const ReanimatorTrackInstance> source);

enum class LocalCosmeticAnimation { None, Sleeping, PotatoLight, Sun, Coin, Diamond };
inline bool IsTransientCosmeticAnimation(ReanimationType type) {
    switch (type) {
        case REANIM_SPLASH:
        case REANIM_RAIN_CIRCLE:
        case REANIM_RAIN_SPLASH:
        case REANIM_HEAL_PARTICLES:
        case REANIM_HEAL_MIST:
        case REANIM_LIGHTNING_HIT:
        case REANIM_TELEPORTATION_BULLET_FLASH:
            return true;
        default:
            return false;
    }
}
inline LocalCosmeticAnimation CoinCosmeticAnimation(ReanimationType type) {
    switch (type) {
        case REANIM_SUN:
        case REANIM_ZOMBIESUN:
            return LocalCosmeticAnimation::Sun;
        case REANIM_COIN_SILVER:
        case REANIM_COIN_GOLD:
            return LocalCosmeticAnimation::Coin;
        case REANIM_DIAMOND:
            return LocalCosmeticAnimation::Diamond;
        default:
            return LocalCosmeticAnimation::None;
    }
}
struct CosmeticSnapshotState {
    bool prepared = false;
    std::vector<uint32_t> animations;
    std::vector<uint32_t> attachments;
    std::vector<uint32_t> eyeBodies;
    std::map<uint32_t, LocalCosmeticAnimation> localAnimations;
    bool DiscardsAnimation(uint32_t id) const {
        return std::find(animations.begin(), animations.end(), id) != animations.end();
    }
    LocalCosmeticAnimation LocalAnimation(uint32_t id) const {
        const auto entry = localAnimations.find(id);
        return entry == localAnimations.end() ? LocalCosmeticAnimation::None : entry->second;
    }
};
bool PrepareSnapshotCosmetics(Board &board, CosmeticSnapshotState &state);
void FinishSnapshotCosmetics(Board &board, const CosmeticSnapshotState &state);
bool RestoreLocalCosmeticAnimation(Board &board, Reanimation &animation, LocalCosmeticAnimation kind, std::span<const ReanimatorTrackInstance> source);

// Preserve the order/offsets of every surviving effect in a mixed attachment.
// Only an explicitly classified cosmetic effect may be removed.
template <typename Attachment, typename Predicate>
bool StripCosmeticEffects(Attachment &attachment, Predicate isCosmetic) {
    const int original = attachment.mNumEffects;
    if (original < 0 || original > static_cast<int>(std::size(attachment.mEffectArray)))
        return false;
    int kept = 0;
    for (int i = 0; i < original; ++i) {
        if (!isCosmetic(attachment.mEffectArray[i]))
            attachment.mEffectArray[kept++] = attachment.mEffectArray[i];
    }
    for (int i = kept; i < original; ++i)
        attachment.mEffectArray[i] = {};
    attachment.mNumEffects = kept;
    if (original != 0 && kept == 0)
        attachment.mDead = true;
    return true;
}
template <typename Track>
void ClearDiscardedAttachment(Track &track, std::span<const uint32_t> discarded) {
    if (std::find(discarded.begin(), discarded.end(), static_cast<uint32_t>(track.mAttachmentID)) != discarded.end())
        track.mAttachmentID = {};
}

inline bool SameName(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    const auto lower = [](unsigned char ch) { return ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch; };
    for (size_t i = 0; i < a.size(); ++i)
        if (lower(a[i]) != lower(b[i]))
            return false;
    return true;
}
inline int FindTrack(const AnimationLayout &layout, std::string_view name) {
    if (name.empty())
        return -1;
    int result = -1;
    for (size_t i = 0; i < layout.tracks.size(); ++i) {
        if (SameName(layout.tracks[i].name, name)) {
            if (result >= 0)
                return -1;
            result = static_cast<int>(i);
        }
    }
    return result;
}
inline bool ValidClip(int start, int count, int frames) {
    return start >= 0 && count > 0 && start < frames && count <= frames - start;
}

struct TrackBlock {
    int bytes = -1;
    int count = 0;
    bool complete = false;
};
inline TrackBlock PeekTrackBlock(std::span<const unsigned char> data, int dataBits, int readBit, int instanceSize) {
    TrackBlock result;
    const int64_t end = std::min<int64_t>(dataBits, static_cast<int64_t>(data.size()) * 8);
    if (readBit < 0 || end - readBit < 32 || instanceSize <= 0)
        return result;
    uint32_t word = 0;
    for (unsigned bit = 0; bit < 32; ++bit) {
        const auto position = static_cast<size_t>(readBit) + bit;
        word |= uint32_t((data[position / 8] >> (position % 8)) & 1) << bit;
    }
    if (word > INT32_MAX)
        return result;
    result.bytes = static_cast<int>(word);
    if (!result.bytes || result.bytes % instanceSize)
        return result;
    result.count = result.bytes / instanceSize;
    if (result.count > kMaxLayoutTracks)
        return result;
    const int64_t required = 4 + int64_t(result.bytes) + int64_t(result.count) * 4;
    result.complete = required <= (end - readBit) / 8;
    return result;
}

struct AnimationPlayback {
    int start, count, basePose;
    float rate, time, lastTime;
};

// Persistent decoration starts from local resources. Its host clip/time is not
// an input; fixed rates avoid consuming the gameplay RNG during snapshot load.
inline bool MakeCosmeticPlayback(const AnimationLayout &local, LocalCosmeticAnimation kind, float localFPS, bool frozen, AnimationPlayback &playback) {
    const int frames = local.SafeFrames();
    if (kind == LocalCosmeticAnimation::None || frames < 2 || frames > kMaxLayoutFrames)
        return false;
    int start = 0, count = frames;
    float rate = 0;
    switch (kind) {
        case LocalCosmeticAnimation::Sleeping:
            rate = 7;
            break;
        case LocalCosmeticAnimation::PotatoLight: {
            const int index = FindTrack(local, "anim_glow");
            if (index < 0)
                return false;
            start = local.tracks[index].start;
            count = std::min({10, local.tracks[index].count, frames - start});
            if (!ValidClip(start, count, frames))
                return false;
            rate = 12;
            break;
        }
        case LocalCosmeticAnimation::Sun:
            rate = 6;
            break;
        case LocalCosmeticAnimation::Coin:
            rate = std::isfinite(localFPS) && localFPS > 0 ? localFPS * 0.8f : 12;
            break;
        case LocalCosmeticAnimation::Diamond:
            rate = 65;
            break;
        default:
            return false;
    }
    playback = {start, count, std::min(start, frames - 2), frozen ? 0 : rate, 0, -1};
    return true;
}

// Old hosts do not send track names. Rebuild the local action without copying
// host track indices, image pointers, blend state or attachment ownership.
inline bool RebuildLocalPlayback(const AnimationLayout &local, AnimationPlayback &playback, std::span<const std::string_view> actions) {
    const int frames = local.SafeFrames();
    if (frames < 2 || playback.count <= 0 || playback.count > kMaxLayoutFrames || playback.start < 0 || playback.start > kMaxLayoutFrames - playback.count || !std::isfinite(playback.rate)
        || !std::isfinite(playback.time) || !std::isfinite(playback.lastTime) || playback.time < 0 || playback.time > 1 || playback.lastTime < -1 || playback.lastTime > 1)
        return false;

    int start = 0, count = frames;
    const auto select = [&](std::string_view name) {
        const int index = FindTrack(local, name);
        if (index < 0)
            return false;
        const auto &track = local.tracks[index];
        const int safeCount = std::min(track.count, frames - track.start);
        if (!ValidClip(track.start, safeCount, frames))
            return false;
        start = track.start;
        count = safeCount;
        return true;
    };
    bool selected = false;
    for (const auto action : actions) {
        if (select(action)) {
            selected = true;
            break;
        }
    }
    // Preserve an identifiable action when the object has no more specific
    // state (standalone effects, auxiliary animations, etc.).
    if (!selected) {
        for (const auto &track : local.tracks) {
            if (track.name.starts_with("anim_") && track.start == playback.start && track.count == playback.count && select(track.name)) {
                selected = true;
                break;
            }
        }
    }
    if (!selected) {
        for (const auto name : {"anim_idle", "anim_normal", "anim_walk"}) {
            if (select(name))
                break;
        }
    }
    const float rate = playback.rate * (float(count) / playback.count);
    if (!std::isfinite(rate))
        return false;
    playback.start = start;
    playback.count = count;
    playback.rate = rate;                            // Keep the host's action duration and normalized time.
    playback.basePose = std::min(start, frames - 2); // Native samples the pose and pose + 1.
    return true;
}

template <typename Context, typename Track>
bool ReadDiscardedTracks(Context &context, std::span<Track> source) {
    if (source.empty() || source.size() > kMaxLayoutTracks)
        return false;
    context.SyncBytes(source.data(), static_cast<int>(source.size_bytes()));
    if (context.mFailed)
        return false;
    for (size_t i = 0; i < source.size(); ++i) {
        // SyncImage stores one SyncInt, with no length prefix. Do not resolve
        // image IDs belonging to a resource pack that is not installed here.
        int ignoredImage = 0;
        context.SyncInt(ignoredImage);
        if (context.mFailed)
            return false;
    }
    return true;
}
inline int RemapFrame(int frame, int start, int count, int newStart, int newCount) {
    return newStart + (count > 1 ? int(int64_t(frame - start) * (newCount - 1) / (count - 1)) : 0);
}

inline bool RemapPlayback(const AnimationLayout &source, const AnimationLayout &local, AnimationPlayback &playback) {
    const int safeFrames = local.SafeFrames();
    if (safeFrames < 2 || !ValidClip(playback.start, playback.count, source.frames) || playback.basePose < -2 || playback.basePose >= source.frames || !std::isfinite(playback.rate)
        || !std::isfinite(playback.time) || !std::isfinite(playback.lastTime) || playback.time < 0 || playback.time > 1 || playback.lastTime < -1 || playback.lastTime > 1)
        return false;

    // Art layers can share a visible range with an action. Prefer conventional
    // anim_* markers; otherwise require an unambiguous destination range.
    int start = -1, count = 0, priority = -1;
    bool ambiguous = false;
    for (const auto &track : source.tracks) {
        if (track.start != playback.start || track.count != playback.count)
            continue;
        const int index = FindTrack(local, track.name);
        if (index < 0)
            continue;
        const auto &target = local.tracks[index];
        const int targetCount = std::min(target.count, safeFrames - target.start);
        if (!ValidClip(target.start, targetCount, safeFrames))
            continue;
        const int candidatePriority = track.name.size() >= 5 && SameName(std::string_view(track.name).substr(0, 5), "anim_") ? 1 : 0;
        if (candidatePriority > priority) {
            start = target.start;
            count = targetCount;
            priority = candidatePriority;
            ambiguous = false;
        } else if (candidatePriority == priority && (start != target.start || count != targetCount)) {
            ambiguous = true;
        }
    }
    if (playback.start == 0 && playback.count == source.frames && priority < 1) {
        start = 0;
        count = safeFrames;
        ambiguous = false;
    }
    if (start < 0 || ambiguous)
        return false;

    int basePose = playback.basePose;
    if (basePose >= 0) {
        // An attachment's reference pose may belong to an earlier action, not
        // the currently playing clip. Locate that named range independently.
        int mappedPose = -1, posePriority = -1, poseRange = INT32_MAX;
        bool poseAmbiguous = false;
        for (const auto &track : source.tracks) {
            if (basePose < track.start || basePose >= track.start + track.count)
                continue;
            const int index = FindTrack(local, track.name);
            if (index < 0)
                continue;
            const auto &target = local.tracks[index];
            const int targetCount = std::min(target.count, safeFrames - target.start);
            if (!ValidClip(target.start, targetCount, safeFrames))
                continue;
            const int candidatePriority = (basePose == track.start ? 2 : 0) + (track.name.size() >= 5 && SameName(std::string_view(track.name).substr(0, 5), "anim_") ? 1 : 0);
            const int value = RemapFrame(basePose, track.start, track.count, target.start, targetCount);
            if (candidatePriority > posePriority || (candidatePriority == posePriority && track.count < poseRange)) {
                mappedPose = value;
                posePriority = candidatePriority;
                poseRange = track.count;
                poseAmbiguous = false;
            } else if (candidatePriority == posePriority && track.count == poseRange && value != mappedPose) {
                poseAmbiguous = true;
            }
        }
        if (mappedPose < 0 || poseAmbiguous)
            return false;
        basePose = mappedPose;
        basePose = std::min(basePose, safeFrames - 2); // Native also samples frame + 1.
    }
    const float rate = playback.rate * (float(count) / playback.count);
    if (!std::isfinite(rate))
        return false;
    playback.start = start;
    playback.count = count;
    playback.basePose = basePose;
    playback.rate = rate;
    return true;
}

template <typename Context, typename Track>
bool ReadRemappedTracks(
    Context &context, std::span<Track> source, std::span<Track> local, const AnimationLayout &sourceLayout, const AnimationLayout &localLayout, std::span<const uint32_t> discardedAttachments = {}) {
    if (source.size() != sourceLayout.tracks.size() || local.size() != localLayout.tracks.size())
        return false;
    context.SyncBytes(source.data(), static_cast<int>(source.size_bytes()));
    if (context.mFailed)
        return false;
    bool mapped = true;
    for (size_t i = 0; i < source.size(); ++i) {
        auto &track = source[i];
        context.SyncImage(track.mImageOverride); // Consume even removed source tracks.
        if (context.mFailed)
            return false;
        ClearDiscardedAttachment(track, discardedAttachments);
        const auto &name = sourceLayout.tracks[i].name;
        const int destination = FindTrack(sourceLayout, name) == static_cast<int>(i) ? FindTrack(localLayout, name) : -1;
        if (destination < 0) {
            if (static_cast<uint32_t>(track.mAttachmentID) != 0)
                mapped = false; // Do not orphan a live effect or attach it to the wrong limb.
            continue;
        }
        auto &restored = local[destination];
        restored = track;
        restored.mBlendCounter = restored.mBlendTime = 0;
        restored.mBlendTransform.mImage = nullptr;
        restored.mBlendTransform.mImage2 = nullptr;
        restored.mBlendTransform.mFont = nullptr;
        restored.mBlendTransform.mText = "";
    }
    return mapped;
}

} // namespace netplay::snapshot
