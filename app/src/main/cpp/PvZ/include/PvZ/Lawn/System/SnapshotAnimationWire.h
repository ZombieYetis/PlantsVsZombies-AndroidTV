#pragma once

#include "SnapshotReanimation.h"

namespace netplay::snapshot {

inline uint32_t LayoutHash(std::span<const unsigned char> data) {
    uint32_t hash = 2166136261u;
    for (auto byte : data)
        hash = (hash ^ byte) * 16777619u;
    return hash;
}
inline void WriteU32(std::vector<unsigned char> &out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<unsigned char>(value >> shift));
}
struct LayoutReader {
    std::span<const unsigned char> data;
    size_t position = 0;
    bool valid = true;
    uint32_t U32() {
        if (data.size() - position < 4) {
            valid = false;
            return 0;
        }
        uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= uint32_t(data[position++]) << shift;
        return result;
    }
};
inline bool DecodeLayout(LayoutReader &in, AnimationLayout &layout) {
    layout = {};
    layout.frames = static_cast<int>(in.U32());
    const auto count = in.U32();
    if (!in.valid || count > kMaxLayoutTracks || layout.frames < 0 || layout.frames > kMaxLayoutFrames || (count != 0 && layout.frames == 0) || count > (in.data.size() - in.position) / 16)
        return false;
    for (uint32_t i = 0; i < count; ++i) {
        const auto length = in.U32();
        if (!in.valid || length > 1024 || length > in.data.size() - in.position)
            return false;
        TrackLayout track;
        track.name.assign(reinterpret_cast<const char *>(in.data.data() + in.position), length);
        in.position += length;
        track.frames = static_cast<int>(in.U32());
        track.start = static_cast<int>(in.U32());
        track.count = static_cast<int>(in.U32());
        if (!in.valid || track.name.find('\0') != std::string::npos || track.frames < 1 || track.frames > kMaxLayoutFrames
            || (track.count ? !ValidClip(track.start, track.count, track.frames) : track.start != 0))
            return false;
        layout.tracks.push_back(std::move(track));
    }
    return layout.tracks.empty() ? layout.frames == 0 : layout.frames == layout.tracks[0].frames;
}
inline bool EncodeLayouts(const AnimationLayouts &layouts, uint32_t snapshotHash, std::vector<unsigned char> &out) {
    out.clear();
    WriteU32(out, 0x314C4152); // RAL1: metadata schema version, not the netplay version.
    WriteU32(out, snapshotHash);
    WriteU32(out, static_cast<uint32_t>(layouts.size()));
    for (const auto &[type, layout] : layouts) {
        WriteU32(out, type);
        WriteU32(out, layout.frames);
        WriteU32(out, static_cast<uint32_t>(layout.tracks.size()));
        for (const auto &track : layout.tracks) {
            WriteU32(out, static_cast<uint32_t>(track.name.size()));
            out.insert(out.end(), track.name.begin(), track.name.end());
            WriteU32(out, track.frames);
            WriteU32(out, track.start);
            WriteU32(out, track.count);
            if (out.size() > kMaxLayoutBytes) {
                out.clear();
                return false;
            }
        }
    }
    return true;
}
inline bool DecodeLayouts(std::span<const unsigned char> data, uint32_t snapshotHash, int typeLimit, AnimationLayouts &layouts) {
    layouts.clear();
    LayoutReader in{data};
    if (data.size() > kMaxLayoutBytes || in.U32() != 0x314C4152 || in.U32() != snapshotHash)
        return false;
    const auto count = in.U32();
    if (!in.valid || count > static_cast<uint32_t>(typeLimit))
        return false;
    for (uint32_t i = 0; i < count; ++i) {
        const auto type = in.U32();
        AnimationLayout layout;
        if (!in.valid || type >= static_cast<uint32_t>(typeLimit) || layouts.contains(type) || !DecodeLayout(in, layout))
            return false;
        layouts.emplace(type, std::move(layout));
    }
    return in.valid && in.position == data.size();
}

// The 3215 event always carries 240 bytes, but old receivers read only count.
// Optional metadata occupies the unused tail. Snapshot bytes/hash, event IDs,
// event sizes and version are unchanged, including for unmodified relay servers.
inline constexpr unsigned char kLayoutTailMagic[] = {'R', 'A', 'L', 'T', 'A', 'I', 'L', 1};
constexpr size_t kLayoutTailHeader = 24; // magic, total, offset, hash, fragment size
constexpr size_t kLayoutTailCapacity = 240 - kLayoutTailHeader - 1;

inline uint16_t FillSnapshotChunk(std::span<unsigned char, 240> chunk, std::span<const unsigned char> snapshot, std::span<const unsigned char> metadata, uint32_t hash, size_t &offset) {
    std::fill(chunk.begin(), chunk.end(), static_cast<unsigned char>(0));
    if (snapshot.empty())
        return 0;
    const size_t amount = offset < metadata.size() ? std::min(metadata.size() - offset, kLayoutTailCapacity) : 0;
    const auto count = std::min(snapshot.size(), amount ? chunk.size() - kLayoutTailHeader - amount : chunk.size());
    std::copy_n(snapshot.begin(), count, chunk.begin());
    if (amount) {
        std::vector<unsigned char> header(std::begin(kLayoutTailMagic), std::end(kLayoutTailMagic));
        WriteU32(header, static_cast<uint32_t>(metadata.size()));
        WriteU32(header, static_cast<uint32_t>(offset));
        WriteU32(header, hash);
        WriteU32(header, static_cast<uint32_t>(amount));
        std::copy(header.begin(), header.end(), chunk.begin() + count);
        std::copy_n(metadata.begin() + offset, amount, chunk.begin() + count + header.size());
        offset += amount;
    }
    return static_cast<uint16_t>(count);
}
struct LayoutTailReceiver {
    std::vector<unsigned char> bytes;
    uint32_t expected = 0, hash = 0;
    bool present = false;
    bool Add(std::span<const unsigned char, 240> chunk, size_t count) {
        if (!count || count > chunk.size())
            return false;
        auto tail = chunk.subspan(count);
        if (tail.size() < sizeof(kLayoutTailMagic) || !std::equal(std::begin(kLayoutTailMagic), std::end(kLayoutTailMagic), tail.begin()))
            return !present || bytes.size() == expected;
        if (tail.size() < kLayoutTailHeader)
            return false;
        LayoutReader in{tail.subspan(sizeof(kLayoutTailMagic))};
        const auto total = in.U32(), offset = in.U32(), checksum = in.U32(), amount = in.U32();
        if (!total || total > kMaxLayoutBytes || !amount || amount > tail.size() - kLayoutTailHeader || offset != bytes.size() || offset > total || amount > total - offset)
            return false;
        if (present && (expected != total || hash != checksum || bytes.size() == expected))
            return false;
        present = true;
        expected = total;
        hash = checksum;
        bytes.insert(bytes.end(), tail.begin() + kLayoutTailHeader, tail.begin() + kLayoutTailHeader + amount);
        return bytes.size() < expected || LayoutHash(bytes) == hash;
    }
    bool Complete() const {
        return !present || (bytes.size() == expected && LayoutHash(bytes) == hash);
    }
};

} // namespace netplay::snapshot
