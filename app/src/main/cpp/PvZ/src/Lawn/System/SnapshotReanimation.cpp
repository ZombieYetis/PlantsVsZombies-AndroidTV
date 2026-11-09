#include "Homura/Logger.h"
#include "PvZ/Android/Native/BridgeApp.h"
#include "PvZ/Android/Native/NativeApp.h"
#include "PvZ/Lawn/System/SnapshotAnimationWire.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

#include <optional>

namespace netplay::snapshot {

bool DescribeAnimation(const ReanimatorDefinition &definition, AnimationLayout &layout) {
    layout = {};
    if (definition.mTrackCount < 0 || definition.mTrackCount > kMaxLayoutTracks || (definition.mTrackCount && !definition.mTracks))
        return false;
    for (int index = 0; index < definition.mTrackCount; ++index) {
        const auto &track = definition.mTracks[index];
        if (!track.mName || strnlen(track.mName, 1025) > 1024 || !track.mTransforms || track.mTransformCount < 1 || track.mTransformCount > kMaxLayoutFrames)
            return false;
        int start = -1, end = -1;
        for (int frame = 0; frame < track.mTransformCount; ++frame) {
            if (track.mTransforms[frame].mFrame >= 0) {
                if (start < 0)
                    start = frame;
                end = frame;
            }
        }
        layout.tracks.push_back({track.mName, track.mTransformCount, start < 0 ? 0 : start, start < 0 ? 0 : end - start + 1});
    }
    layout.frames = layout.tracks.empty() ? 0 : layout.tracks[0].frames;
    return true;
}

const AnimationLayout *GetBundledAnimationLayout(int type) {
    const char *name = GetReanimationFileName(static_cast<ReanimationType>(type));
    if (!name)
        return nullptr;
    // Packaged assets are immutable for this process. Do not read the installed
    // pak/extracted files here: those are precisely the modified local resource.
    static std::map<std::string, std::optional<AnimationLayout>> cache;
    const auto found = cache.find(name);
    if (found != cache.end())
        return found->second ? &*found->second : nullptr;
    auto *bridge = Native::BridgeApp::getSingleton();
    if (!bridge || !bridge->mNativeApp)
        return nullptr;
    auto *env = bridge->getJNIEnv();
    auto activity = bridge->mNativeApp->getActivity();
    if (!env || !activity || env->ExceptionCheck())
        return nullptr;
    if (env->PushLocalFrame(4) < 0) {
        env->ExceptionClear();
        return nullptr;
    }
    std::optional<AnimationLayout> result;
    auto clazz = env->GetObjectClass(activity);
    auto method = clazz ? env->GetMethodID(clazz, "readOriginalAnimationLayout", "(Ljava/lang/String;)[B") : nullptr;
    auto path = method && !env->ExceptionCheck() ? env->NewStringUTF(name) : nullptr;
    auto bytes = path && !env->ExceptionCheck() ? static_cast<jbyteArray>(env->CallObjectMethod(activity, method, path)) : nullptr;
    if (bytes && !env->ExceptionCheck()) {
        const auto length = env->GetArrayLength(bytes);
        if (length > 0 && static_cast<size_t>(length) <= kMaxLayoutBytes) {
            std::vector<unsigned char> data(length);
            env->GetByteArrayRegion(bytes, 0, length, reinterpret_cast<jbyte *>(data.data()));
            LayoutReader reader{data};
            AnimationLayout layout;
            if (!env->ExceptionCheck() && DecodeLayout(reader, layout) && reader.position == data.size())
                result = std::move(layout);
        }
    }
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->PopLocalFrame(nullptr);
    auto &entry = cache.emplace(name, std::move(result)).first->second;
    if (!entry)
        LOG_ERROR("[SNAPSHOT] bundled_animation_layout_unavailable type={} file={}", type, name);
    return entry ? &*entry : nullptr;
}

} // namespace netplay::snapshot
