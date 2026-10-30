/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PvZ/ReplaySystem.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/NetPlay.h"

#include <cstdlib>
#include <cstring>

#include <cctype>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace {
constexpr std::array<char, 6> kReplayMagic = {'P', 'V', 'R', 'P', '1', '\0'};
constexpr const char *kReplayDir = "replays";

struct ReplayPacketRecord {
    ReplayPacketDir dir;
    std::uint32_t tick;
    std::vector<std::byte> data;
};

struct PlaybackState {
    bool active = false;
    bool paused = false;
    std::uint32_t playbackTick = 0;
    std::uint32_t durationTicks = 0;
    std::uint32_t boardTicks = 0;
    std::size_t nextIndex = 0;
    int speedLevel = 0;
    bool halfSpeedPhase = false;
    int vsBackground = 0;
    int boardStartTick = -1;
    std::vector<int> gridItemDieTicks;
    std::vector<int> lawnMowerStartTicks;
    std::string filePath;
    std::vector<ReplayPacketRecord> packets;
};

std::vector<ReplayPacketRecord> gRecordedPackets;
PlaybackState gPlaybackState;
std::uint32_t gRecordStartTick = 0;
bool gRecordStartTickReady = false;

std::string BuildMetaText(const ReplayMetaInfo &meta) {
    return "host=" + meta.hostName + "\n" + "guest=" + meta.guestName + "\n" + "winner=" + meta.winnerName + "\n" + "map=" + meta.mapName + "\n" + "vs_background=" + std::to_string(meta.vsBackground)
        + "\n" + "created_at=" + meta.createdAt + "\n" + "host_camp=" + meta.hostCamp + "\n" + "guest_camp=" + meta.guestCamp + "\n" + "plant_deck=" + meta.plantDeck + "\n"
        + "zombie_deck=" + meta.zombieDeck + "\n" + "duration_ticks=" + std::to_string(meta.durationTicks) + "\n" + "board_tick=" + std::to_string(meta.boardTicks) + "\n"
        + "netplay_version=" + std::to_string(NETPLAY_VERSION) + "\n" + "winner_camp=" + meta.winnerCamp + "\n";
}

std::string ReadMetaValue(const std::string &meta, const char *key) {
    const std::string needle = std::string(key) + "=";
    const std::size_t pos = meta.find(needle);
    if (pos == std::string::npos) {
        return {};
    }
    const std::size_t lineEnd = meta.find('\n', pos);
    const std::size_t valueStart = pos + needle.size();
    std::string value = meta.substr(valueStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - valueStart);
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == '\t' || value.back() == ' ')) {
        value.pop_back();
    }
    return value;
}

std::string ReadLegacyWinnerCamp(std::istream &in, const ReplayMetaInfo &info) {
    // Names only identify a winner when exactly one player matches.
    const bool hostWon = !info.winnerName.empty() && info.winnerName == info.hostName;
    const bool guestWon = !info.winnerName.empty() && info.winnerName == info.guestName;
    if (hostWon != guestWon) {
        const std::string &camp = hostWon ? info.hostCamp : info.guestCamp;
        if (camp == "Plant" || camp == "Zombie") {
            return camp;
        }
    }

    // Event IDs may change between protocol versions. Do not guess the winner
    // of an incompatible replay or one without an explicit result event.
    if (info.netplayVersion != NETPLAY_VERSION) {
        return {};
    }
    std::vector<std::byte> data;
    for (std::size_t i = 0; i < info.packetCount; ++i) {
        std::uint8_t dir = 0;
        std::uint32_t tick = 0;
        std::uint32_t len = 0;
        in.read(reinterpret_cast<char *>(&dir), sizeof(dir));
        in.read(reinterpret_cast<char *>(&tick), sizeof(tick));
        in.read(reinterpret_cast<char *>(&len), sizeof(len));
        if (!in.good() || dir > static_cast<std::uint8_t>(ReplayPacketDir::InboundServer) || len > 1024 * 1024) {
            return {};
        }
        data.resize(len);
        in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(len));
        if (!in.good()) {
            return {};
        }
        std::size_t offset = 0;
        while (offset < data.size()) {
            if (data.size() - offset < sizeof(BaseEvent)) {
                return {};
            }
            BaseEvent event;
            std::memcpy(&event, data.data() + offset, sizeof(event));
            if (event.size < sizeof(BaseEvent) || event.size > data.size() - offset) {
                return {};
            }
            if (event.type == EVENT_SERVER_BOARD_ZOMBIE_WIN && event.size >= sizeof(U16_Event)) {
                return "Zombie";
            }
            if (event.type == EVENT_SERVER_BOARD_PLANT_WIN && event.size >= sizeof(U16_Event)) {
                return "Plant";
            }
            if (event.type == EVENT_SERVER_BOARD_CONCEDE || event.type == EVENT_CLIENT_BOARD_CONCEDE) {
                const std::string &losingCamp = event.type == EVENT_SERVER_BOARD_CONCEDE ? info.hostCamp : info.guestCamp;
                if (losingCamp == "Plant") {
                    return "Zombie";
                }
                if (losingCamp == "Zombie") {
                    return "Plant";
                }
            }
            offset += event.size;
        }
    }
    return {};
}

void CopyNameToGlobal(char *dst, std::size_t dstLen, const std::string &src) {
    if (dst == nullptr || dstLen == 0) {
        return;
    }
    std::strncpy(dst, src.c_str(), dstLen - 1);
    dst[dstLen - 1] = '\0';
}

std::string EscapePrintable(const char *src) {
    if (src == nullptr) {
        return {};
    }
    std::string s(src);
    for (char &c : s) {
        if (c == '\r' || c == '\n' || c == '\t') {
            c = ' ';
        }
    }
    return s;
}

template <typename Fn>
void ForEachPlaybackEvent(const std::vector<ReplayPacketRecord> &packets, Fn &&fn) {
    for (const auto &pkt : packets) {
        std::size_t offset = 0;
        while (offset + sizeof(BaseEvent) <= pkt.data.size()) {
            const auto *event = reinterpret_cast<const BaseEvent *>(pkt.data.data() + offset);
            if (event->size == 0 || offset + event->size > pkt.data.size()) {
                break;
            }

            fn(static_cast<int>(pkt.tick), event);
            offset += event->size;
        }
    }
}

void BuildPlaybackProgressMarkerCache() {
    gPlaybackState.boardStartTick = -1;
    gPlaybackState.gridItemDieTicks.clear();
    gPlaybackState.lawnMowerStartTicks.clear();

    ForEachPlaybackEvent(gPlaybackState.packets, [](int tick, const BaseEvent *event) {
        switch (event->type) {
            case EVENT_SERVER_BOARD_START_LEVEL:
                if (gPlaybackState.boardStartTick < 0) {
                    gPlaybackState.boardStartTick = tick;
                }
                break;
            case EVENT_SERVER_BOARD_GRIDITEM_TAEGETZOMBIE_DIE:
                gPlaybackState.gridItemDieTicks.push_back(tick);
                break;
            case EVENT_SERVER_BOARD_LAWNMOWER_START:
                gPlaybackState.lawnMowerStartTicks.push_back(tick);
                break;
            default:
                break;
        }
    });
}

} // namespace

void replay::ResetRecorder() {
    gRecordedPackets.clear();
    gRecordStartTick = 0;
    gRecordStartTickReady = false;
}

void replay::AdvancePlaybackTick() {
    if (!gPlaybackState.active) {
        return;
    }
    ++gPlaybackState.playbackTick;
}

void replay::AdvancePlaybackOneTick() {
    if (!gPlaybackState.active || gPlaybackState.paused) {
        return;
    }
    AdvancePlaybackTick();
    TickPlayback();
}

void replay::RecordPacket(ReplayPacketDir dir, const std::byte *data, std::size_t len, std::uint32_t tick) {
    if (gPlaybackState.active) {
        return;
    }
    if (data == nullptr || len == 0) {
        return;
    }
    if (!gRecordStartTickReady) {
        gRecordStartTick = tick;
        gRecordStartTickReady = true;
    }

    const std::uint32_t normalizedTick = tick - gRecordStartTick;
    if (!gRecordedPackets.empty()) {
        ReplayPacketRecord &last = gRecordedPackets.back();
        if (last.dir == dir && last.tick == normalizedTick) {
            last.data.insert(last.data.end(), data, data + len);
            return;
        }
    }

    ReplayPacketRecord record;
    record.dir = dir;
    record.tick = normalizedTick;
    record.data.assign(data, data + len);
    gRecordedPackets.push_back(std::move(record));
}

bool replay::SaveCurrentMatchReplay(const ReplayMetaInfo &meta) {
    namespace fs = std::filesystem;
    if (gRecordedPackets.empty()) {
        LOG_WARN("[REPLAY] skip save: no packet recorded");
        return false;
    }
    const std::vector<ReplayPacketRecord> &packetsToSave = gRecordedPackets;
    std::error_code ec;
    fs::create_directories(kReplayDir, ec);
    if (ec) {
        LOG_ERROR("[REPLAY] create dir failed: {}", ec.message());
        return false;
    }

    ReplayMetaInfo metaToWrite = meta;
    metaToWrite.durationTicks = static_cast<int>(packetsToSave.back().tick);

    const std::string outPath = std::string(kReplayDir) + "/" + metaToWrite.fileName;
    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        LOG_ERROR("[REPLAY] open failed path={}", outPath);
        return false;
    }

    const std::string metaText = BuildMetaText(metaToWrite);
    const std::uint32_t metaLen = static_cast<std::uint32_t>(metaText.size());
    const std::uint32_t packetCount = static_cast<std::uint32_t>(packetsToSave.size());
    out.write(kReplayMagic.data(), kReplayMagic.size());
    out.write(reinterpret_cast<const char *>(&metaLen), sizeof(metaLen));
    out.write(metaText.data(), static_cast<std::streamsize>(metaText.size()));
    out.write(reinterpret_cast<const char *>(&packetCount), sizeof(packetCount));
    for (const auto &p : packetsToSave) {
        const std::uint8_t dir = static_cast<std::uint8_t>(p.dir);
        const std::uint32_t len = static_cast<std::uint32_t>(p.data.size());
        out.write(reinterpret_cast<const char *>(&dir), sizeof(dir));
        out.write(reinterpret_cast<const char *>(&p.tick), sizeof(p.tick));
        out.write(reinterpret_cast<const char *>(&len), sizeof(len));
        out.write(reinterpret_cast<const char *>(p.data.data()), static_cast<std::streamsize>(p.data.size()));
    }
    out.close();
    LOG_INFO("[REPLAY] saved path={} packets={}", outPath, packetsToSave.size());
    return true;
}

std::vector<ReplayMetaInfo> replay::ListReplayFiles() {
    namespace fs = std::filesystem;
    struct ReplayFile {
        ReplayMetaInfo info;
        fs::file_time_type savedAt;
    };
    std::vector<ReplayFile> files;
    std::vector<ReplayMetaInfo> infos;
    std::error_code ec;
    if (!fs::exists(kReplayDir, ec)) {
        return infos;
    }
    for (const auto &entry : fs::directory_iterator(kReplayDir, ec)) {
        if (ec || !entry.is_regular_file()) {
            continue;
        }
        std::string extension = entry.path().extension().string();
        std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (extension != ".rpl") {
            continue;
        }
        std::ifstream in(entry.path(), std::ios::binary);
        if (!in.is_open()) {
            continue;
        }
        std::array<char, 6> magic{};
        in.read(magic.data(), static_cast<std::streamsize>(magic.size()));
        if (magic != kReplayMagic) {
            continue;
        }
        std::uint32_t metaLen = 0;
        in.read(reinterpret_cast<char *>(&metaLen), sizeof(metaLen));
        if (metaLen == 0 || metaLen > 64 * 1024) {
            continue;
        }
        std::string meta(metaLen, '\0');
        in.read(meta.data(), static_cast<std::streamsize>(metaLen));
        std::uint32_t packetCount = 0;
        in.read(reinterpret_cast<char *>(&packetCount), sizeof(packetCount));

        ReplayMetaInfo info;
        info.filePath = entry.path().string();
        info.fileName = entry.path().filename().string();
        info.hostName = ReadMetaValue(meta, "host");
        info.guestName = ReadMetaValue(meta, "guest");
        info.winnerName = ReadMetaValue(meta, "winner");
        info.winnerCamp = ReadMetaValue(meta, "winner_camp");
        info.mapName = ReadMetaValue(meta, "map");
        info.vsBackground = std::atoi(ReadMetaValue(meta, "vs_background").c_str());
        info.createdAt = ReadMetaValue(meta, "created_at");
        info.hostCamp = ReadMetaValue(meta, "host_camp");
        info.guestCamp = ReadMetaValue(meta, "guest_camp");
        info.plantDeck = ReadMetaValue(meta, "plant_deck");
        info.zombieDeck = ReadMetaValue(meta, "zombie_deck");
        info.durationTicks = std::atoi(ReadMetaValue(meta, "duration_ticks").c_str());
        info.boardTicks = std::atoi(ReadMetaValue(meta, "board_tick").c_str());
        if (info.boardTicks == 0) {
            info.boardTicks = info.durationTicks;
        }
        info.netplayVersion = std::atoi(ReadMetaValue(meta, "netplay_version").c_str());
        info.packetCount = packetCount;
        if (info.winnerCamp != "Plant" && info.winnerCamp != "Zombie") {
            info.winnerCamp = ReadLegacyWinnerCamp(in, info);
        }
        // 保存和导入都会重新写入本地文件，以修改时间记录最近加入列表的顺序。
        std::error_code timeError;
        const auto savedAt = entry.last_write_time(timeError);
        files.push_back({std::move(info), timeError ? fs::file_time_type::min() : savedAt});
    }
    std::ranges::sort(files, [](const ReplayFile &a, const ReplayFile &b) {
        // 最近保存或导入的回放置顶，与回放内记录的对局时间无关。
        if (a.savedAt != b.savedAt) {
            return a.savedAt > b.savedAt;
        }
        return a.info.fileName > b.info.fileName;
    });
    infos.reserve(files.size());
    for (auto &file : files) {
        infos.push_back(std::move(file.info));
    }
    return infos;
}

bool replay::BeginPlaybackFromFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        LOG_ERROR("[REPLAY] playback open failed path={}", path);
        return false;
    }
    std::array<char, 6> magic{};
    in.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (magic != kReplayMagic) {
        LOG_ERROR("[REPLAY] invalid magic path={}", path);
        return false;
    }
    std::uint32_t metaLen = 0;
    in.read(reinterpret_cast<char *>(&metaLen), sizeof(metaLen));
    if (metaLen == 0 || metaLen > 64 * 1024) {
        LOG_ERROR("[REPLAY] invalid meta length={} path={}", metaLen, path);
        return false;
    }
    std::string meta(metaLen, '\0');
    in.read(meta.data(), static_cast<std::streamsize>(metaLen));
    const std::string hostName = ReadMetaValue(meta, "host");
    const std::string guestName = ReadMetaValue(meta, "guest");
    std::string hostNameFallback = hostName;
    std::string guestNameFallback = guestName;
    if (hostNameFallback.empty() || guestNameFallback.empty()) {
        const std::string base = std::filesystem::path(path).stem().string(); // replay_xxx_HOST_vs_GUEST
        const std::string marker = "_vs_";
        const std::size_t pos = base.rfind(marker);
        if (pos != std::string::npos) {
            const std::size_t leftUnderscore = base.rfind('_', pos - 1);
            if (leftUnderscore != std::string::npos && leftUnderscore + 1 < pos) {
                if (hostNameFallback.empty()) {
                    hostNameFallback = base.substr(leftUnderscore + 1, pos - (leftUnderscore + 1));
                }
                if (guestNameFallback.empty()) {
                    guestNameFallback = base.substr(pos + marker.size());
                }
            }
        }
    }
    const int replayVersion = std::atoi(ReadMetaValue(meta, "netplay_version").c_str());
    if (replayVersion != 0 && replayVersion != static_cast<int>(NETPLAY_VERSION)) {
        LOG_WARN("[REPLAY] netplay version mismatch replay={} local={}", replayVersion, NETPLAY_VERSION);
    }
    CopyNameToGlobal(gServerHostName, sizeof(gServerHostName), hostNameFallback);
    CopyNameToGlobal(gSecondPlayerName, sizeof(gSecondPlayerName), guestNameFallback);
    CopyNameToGlobal(gReplayHostName, sizeof(gReplayHostName), hostNameFallback);
    CopyNameToGlobal(gReplayGuestName, sizeof(gReplayGuestName), guestNameFallback);
    LOG_INFO("[REPLAY] loaded names global host='{}' guest='{}' replayHost='{}' replayGuest='{}'",
             EscapePrintable(gServerHostName),
             EscapePrintable(gSecondPlayerName),
             EscapePrintable(gReplayHostName),
             EscapePrintable(gReplayGuestName));
    std::uint32_t packetCount = 0;
    in.read(reinterpret_cast<char *>(&packetCount), sizeof(packetCount));

    std::vector<ReplayPacketRecord> packets;
    packets.reserve(packetCount);
    for (std::uint32_t i = 0; i < packetCount; ++i) {
        std::uint8_t dir = 0;
        std::uint32_t tick = 0;
        std::uint32_t len = 0;
        in.read(reinterpret_cast<char *>(&dir), sizeof(dir));
        in.read(reinterpret_cast<char *>(&tick), sizeof(tick));
        in.read(reinterpret_cast<char *>(&len), sizeof(len));
        if (!in.good() || len > 1024 * 1024) {
            LOG_ERROR("[REPLAY] corrupted packet at idx={}", i);
            return false;
        }
        ReplayPacketRecord rec;
        rec.dir = static_cast<ReplayPacketDir>(dir);
        rec.tick = tick;
        rec.data.resize(len);
        in.read(reinterpret_cast<char *>(rec.data.data()), static_cast<std::streamsize>(len));
        if (!in.good()) {
            LOG_ERROR("[REPLAY] read packet payload failed idx={}", i);
            return false;
        }
        packets.push_back(std::move(rec));
    }

    gPlaybackState.active = true;
    gPlaybackState.paused = false;
    requestPause = false;
    gPlaybackState.playbackTick = 0;
    gPlaybackState.durationTicks = static_cast<std::uint32_t>(std::max(0, std::atoi(ReadMetaValue(meta, "duration_ticks").c_str())));
    gPlaybackState.boardTicks = static_cast<std::uint32_t>(std::max(0, std::atoi(ReadMetaValue(meta, "board_tick").c_str())));
    gPlaybackState.nextIndex = 0;
    gPlaybackState.speedLevel = 0;
    gPlaybackState.halfSpeedPhase = false;
    gPlaybackState.vsBackground = std::atoi(ReadMetaValue(meta, "vs_background").c_str());
    gPlaybackState.filePath = path;
    gPlaybackState.packets = std::move(packets);
    if (gPlaybackState.durationTicks == 0 && !gPlaybackState.packets.empty()) {
        gPlaybackState.durationTicks = gPlaybackState.packets.back().tick;
    }
    if (gPlaybackState.boardTicks == 0) {
        gPlaybackState.boardTicks = gPlaybackState.durationTicks;
    }
    BuildPlaybackProgressMarkerCache();
    LOG_INFO("[REPLAY] playback begin path={} packets={}", path, gPlaybackState.packets.size());
    return true;
}

int replay::EstimateRecordedDurationTicks() {
    if (gRecordedPackets.size() < 2) {
        return 0;
    }
    const std::uint32_t firstTick = gRecordedPackets.front().tick;
    const std::uint32_t lastTick = gRecordedPackets.back().tick;
    return (lastTick >= firstTick) ? int(lastTick - firstTick) : 0;
}

void replay::StopPlayback() {
    if (!gPlaybackState.active) {
        return;
    }
    LOG_INFO("[REPLAY] playback stopped");
    gPlaybackState = {};
    gIsReplayMode = false;
    gReplayPauseByMenu = false;
    requestPause = false;
}

bool replay::IsPlaybackActive() {
    return gPlaybackState.active;
}

bool replay::IsPlaybackPaused() {
    return gPlaybackState.active && gPlaybackState.paused;
}

void replay::SetPlaybackPaused(bool paused) {
    if (!gPlaybackState.active) {
        return;
    }
    gPlaybackState.paused = paused;
    requestPause = paused;
}

int replay::GetPlaybackSpeedLevel() {
    return gPlaybackState.speedLevel;
}

float replay::GetPlaybackSpeedMultiplier() {
    switch (gPlaybackState.speedLevel) {
        case 1:
            return 2.0f;
        case 2:
            return 3.0f;
        case 3:
            return 5.0f;
        case 4:
            return 10.0f;
        case 5:
            return 0.5f;
        default:
            return 1.0f;
    }
}

bool replay::ConsumePlaybackFrameStep() {
    if (!gPlaybackState.active || gPlaybackState.paused) {
        gPlaybackState.halfSpeedPhase = false;
        return false;
    }

    // 正常速度和高速档位每帧都执行。
    if (gPlaybackState.speedLevel != 5) {
        gPlaybackState.halfSpeedPhase = false;
        return true;
    }

    // 0.5 倍速：执行一帧，跳过一帧。
    gPlaybackState.halfSpeedPhase = !gPlaybackState.halfSpeedPhase;

    return gPlaybackState.halfSpeedPhase;
}

void replay::SetPlaybackSpeedLevel(int speedLevel) {
    if (!gPlaybackState.active) {
        return;
    }
    gPlaybackState.speedLevel = std::clamp(speedLevel, 0, 5);
    gPlaybackState.halfSpeedPhase = false;
}

void replay::CyclePlaybackSpeed() {
    if (!gPlaybackState.active) {
        return;
    }
    gPlaybackState.speedLevel = (gPlaybackState.speedLevel + 1) % 6;
    gPlaybackState.halfSpeedPhase = false;
}

int replay::GetPlaybackTick() {
    return static_cast<int>(gPlaybackState.playbackTick);
}

int replay::GetPlaybackDurationTicks() {
    return static_cast<int>(gPlaybackState.durationTicks);
}

int replay::GetPlaybackBoardTicks() {
    return static_cast<int>(gPlaybackState.boardTicks);
}

const std::string &replay::GetPlaybackFilePath() {
    return gPlaybackState.filePath;
}

int replay::GetPlaybackVsBackground() {
    return gPlaybackState.vsBackground;
}

int replay::GetPlaybackBoardStartTick() {
    return gPlaybackState.boardStartTick;
}

const std::vector<int> &replay::GetPlaybackGridItemDieTicks() {
    return gPlaybackState.gridItemDieTicks;
}

const std::vector<int> &replay::GetPlaybackLawnMowerStartTicks() {
    return gPlaybackState.lawnMowerStartTicks;
}

int replay::FindPlaybackEventTick(EventType eventType) {
    EventList events = FindPlaybackEvents(eventType);
    return events.empty() ? -1 : events.front().tick;
}

replay::EventList replay::FindPlaybackEvents(EventType eventType) {
    EventList events;
    if (!gPlaybackState.active) {
        return events;
    }

    ForEachPlaybackEvent(gPlaybackState.packets, [&](int tick, const BaseEvent *event) {
        if (event->type == eventType) {
            events.push_back({tick, event});
        }
    });
    return events;
}

std::vector<int> replay::FindPlaybackEventTicks(EventType eventType) {
    if (eventType == EVENT_SERVER_BOARD_START_LEVEL) {
        const int tick = GetPlaybackBoardStartTick();
        return tick >= 0 ? std::vector<int>{tick} : std::vector<int>{};
    }
    if (eventType == EVENT_SERVER_BOARD_GRIDITEM_DIE) {
        return gPlaybackState.gridItemDieTicks;
    }
    if (eventType == EVENT_SERVER_BOARD_LAWNMOWER_START) {
        return gPlaybackState.lawnMowerStartTicks;
    }

    std::vector<int> ticks;
    for (const PlaybackEvent &entry : FindPlaybackEvents(eventType)) {
        ticks.push_back(entry.tick);
    }
    return ticks;
}

void replay::TickPlayback() {
    if (!gPlaybackState.active || gLawnApp == nullptr) {
        return;
    }
    const std::uint32_t elapsedTick = gPlaybackState.playbackTick;
    while (gPlaybackState.nextIndex < gPlaybackState.packets.size()) {
        const auto &pkt = gPlaybackState.packets[gPlaybackState.nextIndex];
        if (pkt.tick > elapsedTick) {
            break;
        }
        // Replay is played in spectator-like mode:
        // route all packets through server-path dispatcher so EVENT_CLIENT_*
        // can follow the same spectator branch handling as live spectate.
        if (pkt.dir == ReplayPacketDir::InboundClient || pkt.dir == ReplayPacketDir::InboundServer || pkt.dir == ReplayPacketDir::Outbound) {
            gLawnApp->HandleTcpServerMessage(pkt.data.data(), pkt.data.size());
        }
        ++gPlaybackState.nextIndex;
    }
    if (gPlaybackState.nextIndex >= gPlaybackState.packets.size() && gPlaybackState.playbackTick >= gPlaybackState.durationTicks) {
        StopPlayback();
    }
}
