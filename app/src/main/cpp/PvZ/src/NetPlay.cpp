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

#include "PvZ/NetPlay.h"
#include "Homura/Logger.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/SaveGame.h"
#include "PvZ/ReplaySystem.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include <algorithm>
#include <chrono>
#include <ranges>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

static std::vector<std::byte> sendBuffer;
static std::vector<netplay::SettleEvent> settleEvents;
static int settleSeq = 1;

static std::string ToBase36(long long value) {
    static constexpr char kDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (value <= 0)
        return "0";
    std::string out;
    while (value > 0) {
        out.push_back(kDigits[value % 36]);
        value /= 36;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

static std::string UrlEncode(std::string_view s) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        const bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
        if (keep) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(kHex[(c >> 4) & 0x0F]);
            out.push_back(kHex[c & 0x0F]);
        }
    }
    return out;
}

void netplay::detail::PutEventData(const std::byte *data, std::size_t n) {
    if (IsApplyingOnlineSaveGame()) {
        return; // Loading a snapshot must not emit gameplay side effects.
    }
    sendBuffer.append_range(std::views::counted(data, n));
    const std::uint32_t replayTick = static_cast<std::uint32_t>(gLawnApp->mAppCounter);
    replay::RecordPacket(ReplayPacketDir::Outbound, data, n, replayTick);
}

bool netplay::FlushSendBuffer(int socket) {
    if (sendBuffer.empty()) {
        return true;
    }

    std::size_t sent = 0;
    while (sent < sendBuffer.size()) {
        ssize_t ret = send(socket, sendBuffer.data() + sent, sendBuffer.size() - sent, MSG_NOSIGNAL);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            LOG_ERROR("Failed to send event: {}", std::strerror(errno));
            shutdown(socket, SHUT_RDWR); // Let the existing receive path handle disconnect.
            break;
        }
        if (ret == 0) {
            break;
        }
        sent += static_cast<std::size_t>(ret);
    }

    sendBuffer.erase(sendBuffer.begin(), sendBuffer.begin() + sent);
    return sendBuffer.empty();
}

void netplay::ClearSendBuffer() noexcept {
    sendBuffer.clear();
    ResetSaveGameTransfer();
}

bool netplay::HasPendingSendData() noexcept {
    return !sendBuffer.empty();
}

std::size_t netplay::ParseEventSize(const std::byte *data) {
    decltype(BaseEvent::size) sz = 0;
    std::memcpy(&sz, data + offsetof(BaseEvent, size), sizeof(sz));
    return sz;
}

BaseEvent *netplay::GetEvent(std::byte *dest, const std::byte *src) {
    std::memcpy(dest, src, ParseEventSize(src));
    return reinterpret_cast<BaseEvent *>(dest);
}

void netplay::MetricsSetEndpoint(const std::string &ip, int roomPort) {
    gMetricsServerIp = ip;
    gMetricsServerPort = roomPort > 0 ? (roomPort + 3000) : 0;
}

void netplay::MetricsSetRoomId(int roomId) {
    gMetricsRoomId = roomId;
}

void netplay::MetricsResetSettlementEvents() {
    settleEvents.clear();
    settleSeq = 1;
    gMetricsMowerLoss = 0;
    gMetricsTargetLoss = 0;
    gMetricsGraveLoss = 0;
    gMetricsSunflowerLoss = 0;
    gMetricsShuffleMode = false;
    gMetricsPlantUseCount.clear();
    gMetricsZombieUseCount.clear();
    gMetricsHostSendNameAllowed = true;
}

void netplay::MetricsRecordSeedEvent(bool zombieSide, bool banEvent, int seedType) {
    // 仅记录自定义对战的选卡和禁卡，其他模式不计入对战统计。
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode() || gMetricsBattleType != 10) {
        return;
    }

    if (seedType < 0) {
        return;
    }
    settleEvents.push_back(SettleEvent{settleSeq++, zombieSide ? 'Z' : 'P', banEvent ? 'B' : 'K', seedType});
}

void netplay::MetricsSetBattleType(int battleType) {
    gMetricsBattleType = battleType;
}
void netplay::MetricsSetShuffleMode(bool shuffleMode) {
    gMetricsShuffleMode = shuffleMode;
}
void netplay::MetricsSetAddonFlags(bool extraPackets, bool extraSeeds, bool banMode, bool balancePatch) {
    gMetricsExtraPacket = extraPackets;
    gMetricsExtendedSeeds = extraSeeds;
    gMetricsBanMode = banMode;
    gMetricsBalancePatch = balancePatch;
}
void netplay::MetricsSetVsBackground(int background) {
    gMetricsVsBackground = background;
}
void netplay::MetricsRecordPlantUsed(int seedType) {
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return;
    }
    if (seedType >= 0) {
        gMetricsPlantUseCount[seedType]++;
    }
}
void netplay::MetricsRecordZombieUsed(int zombieType) {
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return;
    }
    if (zombieType >= 0) {
        gMetricsZombieUseCount[zombieType]++;
    }
}
void netplay::MetricsRecordMowerLoss() {
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return;
    }
    gMetricsMowerLoss++;
}
void netplay::MetricsRecordTargetLoss() {
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return;
    }
    gMetricsTargetLoss++;
}
void netplay::MetricsRecordGraveLoss() {
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return;
    }
    gMetricsGraveLoss++;
}
void netplay::MetricsRecordSunflowerLoss() {
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return;
    }
    gMetricsSunflowerLoss++;
}

static bool SendSettlementPayloadBlocking(const std::string &serverIp, int serverPort, const std::string &payload, int roomIdForSettle, int mainCounter, std::size_t eventCount) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("[SETTLE] socket create failed: {}", std::strerror(errno));
        return false;
    }

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(uint16_t(serverPort));
    if (inet_pton(AF_INET, serverIp.c_str(), &sa.sin_addr) != 1) {
        LOG_ERROR("[SETTLE] bad ip '{}'", serverIp);
        close(sock);
        return false;
    }
    if (connect(sock, (sockaddr *)&sa, sizeof(sa)) != 0) {
        LOG_ERROR("[SETTLE] connect {}:{} failed: {}", serverIp, serverPort, std::strerror(errno));
        close(sock);
        return false;
    }
    ssize_t sent = send(sock, payload.data(), payload.size(), 0);
    if (sent < 0 || size_t(sent) != payload.size()) {
        LOG_ERROR("[SETTLE] send failed: {}", std::strerror(errno));
        close(sock);
        return false;
    }

    char resp[64]{};
    ssize_t n = recv(sock, resp, sizeof(resp) - 1, 0);
    close(sock);
    if (n <= 0) {
        LOG_ERROR("[SETTLE] no response");
        return false;
    }
    std::string r(resp, size_t(n));
    while (!r.empty() && (r.back() == '\n' || r.back() == '\r'))
        r.pop_back();
    LOG_DEBUG("[SETTLE] resp={} room={} mainCounter={} events={}", r, roomIdForSettle, mainCounter, eventCount);
    bool ok = (r == "OK" || r == "OK_DUP");
    return ok;
}

bool netplay::MetricsSendSettlement(bool plantWin, int mainCounter) {
    // 非对战模式不能构造或上传对战结算数据。
    if (gLawnApp == nullptr || !gLawnApp->IsVSMode()) {
        return false;
    }
    if (gMetricsServerIp.empty() || gMetricsServerPort <= 0) {
        LOG_WARN("[SETTLE] endpoint not ready ip='{}' port={}", gMetricsServerIp, gMetricsServerPort);
        return false;
    }
    int roomIdForSettle = gMetricsRoomId;
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (roomIdForSettle <= 0) {
        roomIdForSettle = int((nowMs / 1000LL) & 0x7fffffff);
        LOG_WARN("[SETTLE] room id not set, fallback to {}", roomIdForSettle);
    }

    std::ostringstream settleId;
    settleId << roomIdForSettle << "-" << mainCounter << "-" << (plantWin ? "P" : "Z") << "-" << ToBase36(nowMs);

    std::ostringstream oss;
    oss << "SETTLE|" << settleId.str() << "|" << roomIdForSettle << "|" << (plantWin ? "PLANT_WIN" : "ZOMBIE_WIN") << "|" << mainCounter << "|";

    for (size_t i = 0; i < settleEvents.size(); ++i) {
        if (i)
            oss << ';';
        const auto &e = settleEvents[i];
        oss << e.seq << "," << e.side << "," << e.eventType << "," << e.seedType;
    }
    auto appendUsage = [](std::ostringstream &o, const std::unordered_map<int, int> &m) {
        std::vector<std::pair<int, int>> items(m.begin(), m.end());
        std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
        for (size_t i = 0; i < items.size(); ++i) {
            if (i)
                o << ',';
            o << items[i].first << ':' << items[i].second;
        }
    };
    oss << "|bg=" << gMetricsVsBackground << "&battle=" << gMetricsBattleType << "&shuffle=" << (gMetricsShuffleMode ? 1 : 0) << "&addon_ep=" << (gMetricsExtraPacket ? 1 : 0)
        << "&addon_es=" << (gMetricsExtendedSeeds ? 1 : 0) << "&addon_ban=" << (gMetricsBanMode ? 1 : 0) << "&addon_bp=" << (gMetricsBalancePatch ? 1 : 0) << "&mower_loss=" << gMetricsMowerLoss
        << "&target_loss=" << gMetricsTargetLoss << "&grave_loss=" << gMetricsGraveLoss << "&sunflower_loss=" << gMetricsSunflowerLoss << "&plant_use=";
    appendUsage(oss, gMetricsPlantUseCount);
    oss << "&zombie_use=";
    appendUsage(oss, gMetricsZombieUseCount);
    const bool hostAllowName = gMetricsHostSendNameAllowed;
    const bool guestAllowName = gLawnApp->mPlayerInfo->mVSResultsSendPlayerName;
    if (hostAllowName || guestAllowName) {
        const char *localName = (gLawnApp && gLawnApp->mPlayerInfo && gLawnApp->mPlayerInfo->mName) ? gLawnApp->mPlayerInfo->mName : "";
        const char *hostName = (gServerHostName[0] != '\0') ? gServerHostName : localName;
        const char *guestName = (gSecondPlayerName[0] != '\0') ? gSecondPlayerName : localName;
        const int hostSide = gGamepad1ToPlayerIndex; // mSides[0]: host selected side
        const bool hostIsPlant = (hostSide == 0);
        const bool hostIsZombie = (hostSide == 1);
        const char *plantName = hostAllowName ? hostName : "";
        const char *zombieName = guestAllowName ? guestName : "";
        if (hostIsPlant) {
            plantName = hostAllowName ? hostName : "";
            zombieName = guestAllowName ? guestName : "";
        } else if (hostIsZombie) {
            plantName = guestAllowName ? guestName : "";
            zombieName = hostAllowName ? hostName : "";
        }
        oss << "&plant_name=" << UrlEncode(plantName);
        oss << "&zombie_name=" << UrlEncode(zombieName);
    }
    oss << '\n';

    std::string payload = oss.str();
    std::string serverIp = gMetricsServerIp;
    const int serverPort = gMetricsServerPort;
    const std::size_t eventCount = settleEvents.size();
    MetricsResetSettlementEvents();

    std::thread([serverIp = std::move(serverIp), serverPort, payload = std::move(payload), roomIdForSettle, mainCounter, eventCount]() {
        const bool ok = SendSettlementPayloadBlocking(serverIp, serverPort, payload, roomIdForSettle, mainCounter, eventCount);
        if (!ok) {
            LOG_WARN("[SETTLE] async send failed room={} mainCounter={} events={}", roomIdForSettle, mainCounter, eventCount);
        }
    }).detach();
    return true;
}
