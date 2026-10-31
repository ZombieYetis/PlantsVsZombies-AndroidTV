#ifndef PVZ_RELAY_RECONNECT_H
#define PVZ_RELAY_RECONNECT_H

#include <cstddef>
#include <cstdint>
#include <vector>

class LawnApp;
struct BaseEvent;

namespace netplay::reconnect {
// Wire values are shared with RelayReconnect.java. All multibyte fields are BE.
enum Op : uint8_t {
    OP_STATE = 1,
    OP_FROZEN,
    OP_SYNC,
    OP_LOADED,
    OP_COMMIT,
    OP_RESUME,
    OP_CANCEL,
    OP_PING,
    OP_PONG,
};
enum Phase : uint8_t {
    PHASE_NONE,
    PHASE_FREEZING,
    PHASE_WAITING,
    PHASE_ATTACHING,
    PHASE_SYNCING,
    PHASE_RESUMING,
    PHASE_CLOSING,
};
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
