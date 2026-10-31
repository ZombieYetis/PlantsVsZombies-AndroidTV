#ifndef PVZ_RELAY_RECONNECT_H
#define PVZ_RELAY_RECONNECT_H

#include <cstddef>
#include <cstdint>
#include <vector>

class LawnApp;
struct BaseEvent;

namespace netplay::reconnect {
// Wire values are shared with RelayReconnect.java. All multibyte fields are BE.
enum Op : uint8_t { State = 1, Frozen, Sync, Loaded, Commit, Resume, Cancel, Ping, Pong };
enum Phase : uint8_t { None, Freezing, Waiting, Attaching, Syncing, Resuming, Closing };
constexpr uint16_t SnapshotVersion = 1;
constexpr std::size_t MaxSnapshotSize = 64 * 1024 * 1024;
bool IsFrozen() noexcept;
bool IsEvent(uint8_t type) noexcept;
void PrepareClient(uint32_t epoch, uint32_t transfer);
bool HandleEvent(LawnApp *app, const BaseEvent *event);
void Update(LawnApp *app);
void Reset(LawnApp *app) noexcept;
uint32_t Hash(const std::vector<unsigned char> &data);
} // namespace netplay::reconnect

#endif
