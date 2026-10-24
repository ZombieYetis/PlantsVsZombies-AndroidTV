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

#ifndef PVZ_REPLAY_SYSTEM_H
#define PVZ_REPLAY_SYSTEM_H

#include <cstddef>
#include <cstdint>

#include <string>
#include <vector>

enum EventType : std::uint8_t;
struct BaseEvent;

enum class ReplayPacketDir : std::uint8_t {
    Outbound = 0,
    InboundClient = 1,
    InboundServer = 2,
};

struct ReplayMetaInfo {
    std::string filePath;
    std::string fileName;
    std::string hostName;
    std::string guestName;
    std::string winnerName;
    std::string winnerCamp;
    std::string mapName;
    std::string createdAt;
    std::string hostCamp;
    std::string guestCamp;
    std::string plantDeck;
    std::string zombieDeck;
    int netplayVersion = 0;
    int vsBackground = 0;
    int durationTicks = 0;
    int boardTicks = 0;
    std::size_t packetCount = 0;
};

namespace replay {
struct PlaybackEvent {
    int tick = 0;
    const BaseEvent *event = nullptr;
};
using EventList = std::vector<PlaybackEvent>;

void ResetRecorder();
void RecordPacket(ReplayPacketDir dir, const std::byte *data, std::size_t len, std::uint32_t tick);
bool SaveCurrentMatchReplay(const ReplayMetaInfo &meta);
std::vector<ReplayMetaInfo> ListReplayFiles();
bool BeginPlaybackFromFile(const std::string &path);
void AdvancePlaybackTick();
void AdvancePlaybackOneTick();
int EstimateRecordedDurationTicks();
void StopPlayback();
bool IsPlaybackActive();
bool IsPlaybackPaused();
void SetPlaybackPaused(bool paused);
int GetPlaybackSpeedLevel();
float GetPlaybackSpeedMultiplier();
bool ConsumePlaybackFrameStep();
void SetPlaybackSpeedLevel(int speedLevel);
void CyclePlaybackSpeed();
int GetPlaybackTick();
int GetPlaybackDurationTicks();
int GetPlaybackBoardTicks();
const std::string &GetPlaybackFilePath();
int GetPlaybackVsBackground();
int GetPlaybackBoardStartTick();
const std::vector<int> &GetPlaybackGridItemDieTicks();
const std::vector<int> &GetPlaybackLawnMowerStartTicks();
int FindPlaybackEventTick(EventType eventType);
EventList FindPlaybackEvents(EventType eventType);
std::vector<int> FindPlaybackEventTicks(EventType eventType);
void TickPlayback();
} // namespace replay

#endif // PVZ_REPLAY_SYSTEM_H
