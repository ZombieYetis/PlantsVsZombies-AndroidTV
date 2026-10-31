#pragma once

#include "PvZ/ReplaySystem.h"
#include <cstring>
#include <algorithm>
#include <limits>

// Portable codec used by the live-history transport and its native unit tests.
// Records use the replay file's direction/tick/length fields, explicitly LE.
namespace replay::history {
constexpr std::size_t kMaxBytes = 128 * 1024 * 1024;
constexpr std::size_t kMaxPacket = 1024 * 1024;
constexpr uint32_t kHashInitial = 2166136261u;

struct Record {
    ReplayPacketDir dir;
    uint32_t tick;
    std::vector<std::byte> data;
};

inline uint32_t Hash(uint32_t hash, const std::byte *data, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i)
        hash = (hash ^ std::to_integer<uint8_t>(data[i])) * 16777619u;
    return hash;
}
inline uint32_t Read32(const std::byte *data) {
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
        value |= uint32_t(std::to_integer<uint8_t>(data[i])) << (8 * i);
    return value;
}
inline void Write32(std::byte *data, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        data[i] = std::byte(value >> (8 * i));
}
inline bool ValidEvents(const std::byte *data, std::size_t size) {
    std::size_t pos = 0;
    while (pos < size) {
        if (size - pos < 2)
            return false;
        const auto length = std::to_integer<uint8_t>(data[pos + 1]);
        if (length < 2 || length > size - pos)
            return false;
        pos += length;
    }
    return pos == size && size != 0;
}

struct Log {
    std::vector<std::byte> bytes;
    uint32_t hash = kHashInitial;
    bool failed = false;

    bool Append(ReplayPacketDir dir, uint32_t tick, const std::byte *data, std::size_t size) {
        if (failed || size > kMaxPacket || size + 9 > kMaxBytes - bytes.size() || uint8_t(dir) > uint8_t(ReplayPacketDir::InboundServer) || !ValidEvents(data, size)) {
            failed = true;
            return false;
        }
        std::byte header[9];
        header[0] = std::byte(dir);
        Write32(header + 1, tick);
        Write32(header + 5, uint32_t(size));
        bytes.insert(bytes.end(), header, header + 9);
        bytes.insert(bytes.end(), data, data + size);
        hash = Hash(Hash(hash, header, 9), data, size);
        return true;
    }
};

struct Receiver : Log {
    std::vector<Record> records;
    std::size_t parsed = 0;
    bool complete = false;

    // Duplicate prefixes are allowed when another viewer triggers a retransmit.
    // Gaps and conflicting overlaps are never silently spliced into a replay.
    bool Receive(std::size_t offset, const std::byte *data, std::size_t size) {
        if (failed || offset > bytes.size() || offset > kMaxBytes || size > kMaxBytes - offset) {
            failed = true;
            return false;
        }
        const auto overlap = std::min(size, bytes.size() - offset);
        if (overlap && std::memcmp(bytes.data() + offset, data, overlap) != 0) {
            failed = true;
            return false;
        }
        if (complete && overlap != size) {
            failed = true;
            return false;
        }
        bytes.insert(bytes.end(), data + overlap, data + size);
        hash = Hash(hash, data + overlap, size - overlap);
        while (bytes.size() - parsed >= 9) {
            const auto *header = bytes.data() + parsed;
            const auto dir = std::to_integer<uint8_t>(header[0]);
            const auto tick = Read32(header + 1);
            const auto length = Read32(header + 5);
            if (dir > uint8_t(ReplayPacketDir::InboundServer) || length < 2 || length > kMaxPacket || (!records.empty() && tick < records.back().tick)) {
                failed = true;
                return false;
            }
            if (bytes.size() - parsed - 9 < length)
                break;
            if (!ValidEvents(header + 9, length)) {
                failed = true;
                return false;
            }
            if (!records.empty() && records.back().tick == tick && uint8_t(records.back().dir) == dir && records.back().data.size() + length <= kMaxPacket) {
                records.back().data.insert(records.back().data.end(), header + 9, header + 9 + length);
            } else {
                records.push_back({ReplayPacketDir(dir), tick, {header + 9, header + 9 + length}});
            }
            parsed += 9 + length;
        }
        return true;
    }

    bool Checkpoint(std::size_t size, uint32_t expectedHash, bool final) {
        if (failed || size != bytes.size() || hash != expectedHash || parsed != bytes.size() || (final && records.empty())) {
            failed = true;
            return false;
        }
        if (final)
            complete = true;
        return true;
    }
};
} // namespace replay::history
