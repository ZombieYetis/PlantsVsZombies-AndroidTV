#include "PvZ/RelayReconnect.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/SaveGame.h"
#include "PvZ/Lawn/Widget/LawnDialog.h"
#include "PvZ/Lawn/Widget/NetplayLobbyWidget.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/NetPlay.h"
#include "PvZ/ReplaySystem.h"
#include "PvZ/SexyAppFramework/Misc/Common.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <sys/socket.h>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <chrono>

namespace netplay::reconnect {
namespace {
    using Clock = std::chrono::steady_clock;
    std::atomic<Phase> phase{PHASE_NONE};
    uint32_t epoch = 0, transfer = 0, expected = 0, expectedHash = 0, snapshotTick = 0;
    GameMode snapshotMode = GAMEMODE_MP_VS;
    std::vector<unsigned char> incoming, outgoing;
    std::size_t offset = 0;
    bool receiving = false, cancelRequested = false, previousPause = false;
    int code = -1, shownPhase = -1, heartbeatSocket = -1;
    auto lastPing = Clock::now(), lastPong = Clock::now(), progress = Clock::now();
    constexpr auto dialogId = DIALOG_RECONNECT;

    uint32_t Read32(const unsigned char *p) {
        return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
    }
    uint16_t Read16(const unsigned char *p) {
        return uint16_t(p[0]) << 8 | p[1];
    }
    void Write32(unsigned char *p, uint32_t n) {
        p[0] = n >> 24;
        p[1] = n >> 16;
        p[2] = n >> 8;
        p[3] = n;
    }
    void Write16(unsigned char *p, uint16_t n) {
        p[0] = n >> 8;
        p[1] = n;
    }
    void Send(const unsigned char *p, std::size_t n) {
        netplay::detail::PutEventData(reinterpret_cast<const std::byte *>(p), n);
    }
    void Control(Op op, uint32_t value = 0) {
        unsigned char data[16]{EVENT_RECONNECT_CONTROL, 16, op, phase.load()};
        Write32(data + 4, epoch);
        Write32(data + 8, transfer);
        Write32(data + 12, value);
        Send(data, sizeof(data));
    }
    void ClearHeldInput() {
        gButtonDown = gButtonDownP1 = gButtonDownP2 = gButtonDownSeedChooser = gButtonDownVSSetup = false;
        gGamepadP1VelX = gGamepadP1VelY = gGamepadP2VelX = gGamepadP2VelY = 0;
    }
    void ClearTransfer() {
        incoming.clear();
        outgoing.clear();
        offset = 0;
        expected = 0;
        receiving = false;
    }
    void HideDialog(LawnApp *app) {
        if (app->GetDialog(dialogId))
            app->KillDialog(dialogId);
        shownPhase = -1;
    }
    void ShowDialog(LawnApp *app) {
        if (shownPhase == int(phase.load()) && app->GetDialog(dialogId))
            return;
        HideDialog(app);
        pvzstl::string message;
        if (gIsServerModeSpectator) {
            message = TodStringTranslate("[RECONNECT_SPECTATOR_WAIT]");
        } else if (IsRemoteServer() && code >= 0) {
            auto status = TodStringTranslate(phase == PHASE_WAITING ? "[RECONNECT_WAITING]" : "[RECONNECT_SYNCING]");
            message = Sexy::StrFormat(TodStringTranslate("[RECONNECT_HOST_CODE_FMT]").c_str(), code, status.c_str());
        } else {
            message = TodStringTranslate(IsRemoteServer() ? "[RECONNECT_PREPARING]" : "[RECONNECT_CLIENT_SYNCING]");
        }
        auto *dialog = app->GetVTable()->DoDialog(app, dialogId, true, TodStringTranslate("[RECONNECT_TITLE]"), message, TodStringTranslate("[DISCONNECT]"), 3);
        static const Sexy::ButtonListener::VTable listener{
            .ButtonDepress = reinterpret_cast<void *>(+[](Sexy::ButtonListener *, int) { cancelRequested = true; }),
        };
        dialog->Sexy::ButtonListener::vTable = &listener;
        static_cast<LawnDialog *>(dialog)->mButtonDelay = 0;
        app->mWidgetManager->SetFocus(dialog);
        shownPhase = phase.load();
    }
    void FailClient() {
        Control(OP_LOADED, 0);
        phase = PHASE_RESUMING;
        receiving = false;
        incoming.clear();
    }
} // namespace

bool IsFrozen() noexcept {
    return phase != PHASE_NONE;
}
bool IsEvent(uint8_t type) noexcept {
    return type >= EVENT_RECONNECT_CONTROL && type <= EVENT_RECONNECT_SNAPSHOT_END;
}
uint32_t Hash(const std::vector<unsigned char> &data) {
    uint32_t result = 2166136261u;
    for (auto b : data)
        result = (result ^ b) * 16777619u;
    return result;
}
void PrepareClient(uint32_t newEpoch, uint32_t newTransfer) {
    epoch = newEpoch;
    transfer = newTransfer;
    phase = PHASE_ATTACHING;
    previousPause = false;
    progress = Clock::now();
    ClearTransfer();
    replay::ResetRecorder();
    replay::MarkRecordingIncomplete(); // A disconnected client's partial recording is not a complete match.
}
void Reset(LawnApp *app) noexcept {
    HideDialog(app);
    phase = PHASE_NONE;
    epoch = transfer = 0;
    code = -1;
    cancelRequested = false;
    previousPause = false;
    heartbeatSocket = -1;
    ClearTransfer();
}

bool HandleEvent(LawnApp *app, const BaseEvent *event) {
    if (!IsEvent(event->type))
        return false;
    if (gIsReplayMode || !gIsServerModeNetplay || gServerModeTransport != ServerModeTransport::RELAY)
        return true;
    const auto *p = reinterpret_cast<const unsigned char *>(event);
    if (event->type == EVENT_RECONNECT_CONTROL) {
        if (event->size != 16)
            return true;
        const auto op = Op(p[2]);
        if (op == OP_PONG) {
            lastPong = Clock::now();
            return true;
        }
        const auto newEpoch = Read32(p + 4), newTransfer = Read32(p + 8);
        if (op == OP_STATE) {
            if (p[3] < PHASE_FREEZING || p[3] > PHASE_CLOSING)
                return true;
            if (newEpoch < epoch)
                return true;
            if (phase == PHASE_NONE && app->mBoard)
                previousPause = app->mBoard->mPaused;
            epoch = newEpoch;
            transfer = newTransfer;
            phase = Phase(p[3]);
            ClearHeldInput();
            if (phase == PHASE_FREEZING || phase == PHASE_WAITING)
                ClearTransfer();
            const auto value = Read32(p + 12);
            if (IsRemoteServer() && phase == PHASE_WAITING && value < 100) {
                code = int(value);
                shownPhase = -1;
            }
            if (phase == PHASE_FREEZING && IsRemoteServer()) {
                // Already queued events stay in order; the relay treats FROZEN as a barrier.
                // Preserve any spectator snapshot and catchup already in flight.
                // Its sender pauses with this board and continues after RESUME.
                Control(OP_FROZEN, netplay::CanCaptureReconnectSnapshot(app) ? 1 : 0);
            }
            if (phase == PHASE_CLOSING)
                cancelRequested = true;
            progress = Clock::now();
            return true;
        }
        if (newEpoch != epoch || (!gIsServerModeSpectator && op != OP_SYNC && newTransfer != transfer))
            return true;
        if (op == OP_SYNC && IsRemoteServer() && (phase == PHASE_WAITING || phase == PHASE_ATTACHING)) {
            transfer = newTransfer;
            phase = PHASE_SYNCING;
            ClearTransfer();
            // Snapshot the pre-dialog pause state, without modifying the live board permanently.
            bool paused = app->mBoard && app->mBoard->mPaused;
            if (app->mBoard)
                app->mBoard->mPaused = previousPause;
            bool captured = netplay::CaptureReconnectSnapshot(app, outgoing);
            if (app->mBoard)
                app->mBoard->mPaused = paused;
            if (!captured) {
                Control(OP_LOADED, 0);
                return true;
            }
            unsigned char begin[26]{EVENT_RECONNECT_SNAPSHOT_BEGIN, 26};
            Write32(begin + 2, epoch);
            Write32(begin + 6, transfer);
            Write16(begin + 10, SnapshotVersion);
            Write16(begin + 12, uint16_t(app->mGameMode));
            Write32(begin + 14, outgoing.size());
            Write32(begin + 18, Hash(outgoing));
            Write32(begin + 22, app->mAppCounter);
            Send(begin, sizeof(begin));
        } else if (op == OP_LOADED && IsRemoteServer() && phase == PHASE_SYNCING && Read32(p + 12) == 1 && outgoing.empty()) {
            phase = PHASE_RESUMING;
            Control(OP_COMMIT);
        } else if (op == OP_RESUME && IsFrozen()) {
            ClearTransfer();
            ClearHeldInput();
            code = -1;
            // Modal removal calls Board::Pause(false). Keep the reconnect
            // barrier active and suppress pause echoes while closing both
            // dialogs, then resume through Pause so audio is restored too.
            const bool wasPauseSyncFromRemote = gPauseSyncFromRemote;
            gPauseSyncFromRemote = true;
            HideDialog(app);
            app->KillNewOptionsDialog();
            if (app->mBoard)
                app->mBoard->Pause(false);
            gPauseSyncFromRemote = wasPauseSyncFromRemote;
            phase = PHASE_NONE;
            if (IsRemoteServer() && previousPause && app->mBoard && !app->mVSSetupMenu) {
                // Reconnect controls are excluded from replays. Record one
                // authoritative resume to balance the pre-disconnect pause.
                U8_Event resumed = {{EVENT_SERVER_BOARD_PAUSE}, false};
                netplay::PutEvent(resumed);
            }
            previousPause = false;
            if (app->mBoard && app->GetDialogCount() == 0) {
                Sexy::Widget *focus = app->mBoard;
                if (app->mVSSetupMenu)
                    focus = app->mVSSetupMenu;
                else if (app->mSeedChooserScreen)
                    focus = app->mSeedChooserScreen;
                app->mWidgetManager->SetFocus(focus);
            }
        }
        progress = Clock::now();
        return true;
    }
    if (IsRemoteServer() || gIsServerModeSpectator || !IsFrozen() || event->size < 10 || Read32(p + 2) != epoch || Read32(p + 6) != transfer)
        return true;
    if (event->type == EVENT_RECONNECT_SNAPSHOT_BEGIN) {
        if (event->size != 26 || phase != PHASE_ATTACHING || Read16(p + 10) != SnapshotVersion) {
            FailClient();
            return true;
        }
        expected = Read32(p + 14);
        expectedHash = Read32(p + 18);
        snapshotTick = Read32(p + 22);
        snapshotMode = GameMode(Read16(p + 12));
        if (!expected || expected > MaxSnapshotSize) {
            FailClient();
            return true;
        }
        incoming.clear();
        incoming.reserve(expected);
        receiving = true;
        phase = PHASE_SYNCING;
    } else if (event->type == EVENT_RECONNECT_SNAPSHOT_CHUNK && receiving) {
        if (event->size < 16 || Read32(p + 10) != incoming.size() || Read16(p + 14) != event->size - 16 || event->size == 16 || incoming.size() + event->size - 16 > expected) {
            FailClient();
            return true;
        }
        incoming.insert(incoming.end(), p + 16, p + event->size);
    } else if (event->type == EVENT_RECONNECT_SNAPSHOT_END && receiving) {
        receiving = false;
        bool valid = event->size == 10 && incoming.size() == expected && Hash(incoming) == expectedHash;
        HideDialog(app);
        valid = valid && netplay::ApplyReconnectSnapshot(app, incoming, snapshotMode);
        incoming.clear();
        if (valid) {
            app->mAppCounter = snapshotTick;
        }
        phase = PHASE_RESUMING;
        Control(OP_LOADED, valid ? 1 : 0);
    }
    progress = Clock::now();
    return true;
}

void Update(LawnApp *app) {
    if (gIsReplayMode)
        return;
    const bool relay = gIsServerModeNetplay && gServerModeTransport == ServerModeTransport::RELAY;
    const int socket = IsRemoteServer() ? gTcpClientSocket : (gTcpConnected ? gTcpServerSocket : -1);
    if (!relay || socket < 0) {
        if (IsFrozen())
            Reset(app);
        return;
    }
    auto now = Clock::now();
    if (!gIsServerModeSpectator) {
        if (socket != heartbeatSocket) {
            heartbeatSocket = socket;
            lastPing = lastPong = now;
        }
        if (now - lastPing >= std::chrono::seconds(2)) {
            Control(OP_PING);
            lastPing = now;
        }
        if (now - lastPong > std::chrono::seconds(25))
            shutdown(socket, SHUT_RDWR);
    }
    if (IsFrozen()) {
        ShowDialog(app);
        // World updates are stopped; keep the modal's button delay/animations alive.
        if (auto *dialog = app->GetDialog(dialogId))
            dialog->GetVTable()->Update(dialog);
        if (cancelRequested) {
            if (IsRemoteServer()) {
                Control(OP_CANCEL);
                FlushSendBuffer(socket);
            }
            app->DoBackToMain();
            return;
        }
    }
    if (IsRemoteServer() && phase == PHASE_SYNCING && !outgoing.empty() && !HasPendingSendData()) {
        const auto end = std::min(offset + 16 * 1024, outgoing.size());
        while (offset < end) {
            unsigned char data[240]{EVENT_RECONNECT_SNAPSHOT_CHUNK};
            auto count = std::min<std::size_t>(224, outgoing.size() - offset);
            data[1] = uint8_t(16 + count);
            Write32(data + 2, epoch);
            Write32(data + 6, transfer);
            Write32(data + 10, offset);
            Write16(data + 14, count);
            std::memcpy(data + 16, outgoing.data() + offset, count);
            Send(data, data[1]);
            offset += count;
        }
        if (offset == outgoing.size()) {
            unsigned char data[10]{EVENT_RECONNECT_SNAPSHOT_END, 10};
            Write32(data + 2, epoch);
            Write32(data + 6, transfer);
            Send(data, sizeof(data));
            outgoing.clear();
        }
        progress = now;
    }
    if (!IsRemoteServer() && !gIsServerModeSpectator && IsFrozen() && now - progress > std::chrono::seconds(70))
        shutdown(socket, SHUT_RDWR);
}
} // namespace netplay::reconnect
