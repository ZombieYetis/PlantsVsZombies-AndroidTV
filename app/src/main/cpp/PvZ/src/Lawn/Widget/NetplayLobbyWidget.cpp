/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 */

#include "PvZ/Lawn/Widget/NetplayLobbyWidget.h"

#include "Homura/BitUtils.h"
#include "Homura/Logger.h"
#include "Homura/MemberUtils.h"
#include "Homura/StringUtils.h"
#include "PvZ/Android/Native/BridgeApp.h"
#include "PvZ/Android/Native/NativeApp.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/NetPlay.h"
#include "PvZ/RelayReconnect.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Widget/ScrollWidget.h"
#include "PvZ/TodLib/Common/TodCommon.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/endian.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <mutex>
#include <vector>

#include <cstdio>
#include <cstring>

using namespace Sexy;

namespace {
constexpr int kLeftPanelX = 40;
constexpr int kPanelYOffset = 15;
constexpr int kLeftPanelY = 120 + kPanelYOffset;
constexpr int kLeftPanelWidth = 330;
constexpr int kPanelHeight = 490;
constexpr int kRightPanelX = 390;
constexpr int kRightPanelY = 120 + kPanelYOffset;
constexpr int kRightPanelWidth = 850;
constexpr int kSelectedServerLatencyX = kRightPanelX + 20;
constexpr int kSelectedServerLatencyY = 170 + kPanelYOffset;
constexpr int kServerRowY = 230 + kPanelYOffset;
constexpr int kServerRowHeight = 55;
constexpr int kRoomScrollX = 410;
constexpr int kRoomScrollY = 193 + kPanelYOffset;
constexpr int kRoomScrollWidth = 810;
constexpr int kRoomScrollHeight = 330;
constexpr int kRoomCardWidth = 390;
constexpr int kRoomCardHeight = 100;
constexpr int kRoomCardGap = 15;
constexpr int kRoomScrollbarX = kRightPanelX + kRightPanelWidth - 14;
constexpr int kRoomScrollbarWidth = 7;
constexpr int kRoomScrollbarMinThumbHeight = 42;
constexpr int kMaxSpectatorNamesShown = 6;
constexpr int kLobbyStatusY = 630;
constexpr int kRoomExitedStatusY = 640;
constexpr int kRoomActionY = 545;
constexpr int kRoomActionWidth = 190;
constexpr int kRoomActionHeight = 45;
constexpr int kRoomActionGap = 15;
constexpr uint16_t kCoopLanProtocolFlag = 1 << 15;
constexpr std::string_view kVsRoomNamePrefix = "[VS]";
constexpr std::string_view kCoopRoomNamePrefix = "[COOP]";

bool RoomNameHasPrefix(std::string_view roomName, std::string_view prefix) {
    return roomName.size() >= prefix.size() && roomName.substr(0, prefix.size()) == prefix;
}

uint16_t GetLanRoomProtocol(bool isCoopLobby) {
    return uint16_t(NETPLAY_VERSION) | (isCoopLobby ? kCoopLanProtocolFlag : 0);
}

bool GetServerRoomDisplayName(std::string_view wireName, bool isCoopLobby, std::string_view &displayName) {
    const bool isCoopRoom = RoomNameHasPrefix(wireName, kCoopRoomNamePrefix);
    if (isCoopRoom != isCoopLobby) {
        return false;
    }

    if (isCoopRoom) {
        displayName = wireName.substr(kCoopRoomNamePrefix.size());
    } else if (RoomNameHasPrefix(wireName, kVsRoomNamePrefix)) {
        displayName = wireName.substr(kVsRoomNamePrefix.size());
    } else {
        displayName = wireName;
    }
    return true;
}

void DrawPanel(Graphics *g, const Rect &rect) {
    g->SetColor(Color(66, 36, 20, 210));
    g->FillRect(rect);
    g->SetColor(Color(210, 164, 91, 230));
    g->DrawRect(rect);
}

int GetRoomSortPriority(const ServerRoomItem &room) {
    if (room.protocolVersion != 0 && room.protocolVersion != NETPLAY_VERSION)
        return 3;
    if (room.reconnectPending)
        return 2;
    if (!room.full && !room.gaming)
        return 0;
    if (room.full && room.spectateAllowed)
        return 1;
    return 2;
}

pvzstl::string BuildRoomTag(const ServerRoomItem &room, Color &color) {
    color = Color(190, 225, 190);
    if (room.protocolVersion != 0 && room.protocolVersion != NETPLAY_VERSION) {
        color = Color(128, 128, 128);
        return TodStringTranslate(room.protocolVersion < NETPLAY_VERSION ? "[SERVER_ROOM_VERSION_ERROR_LOWER]" : "[SERVER_ROOM_VERSION_ERROR_HIGHER]");
    }
    if (room.reconnectPending) {
        color = Color(255, 255, 255);
        return TodStringTranslate(room.reconnectBusy ? "[TAG_RECONNECT_BUSY]" : "[TAG_RECONNECT_WAIT]");
    }
    if (room.full && room.spectateAllowed) {
        color = Color(255, 255, 0);
        return TodStringTranslate("[SPECTATE_AVAILABLE]");
    }
    if (room.gaming) {
        return TodStringTranslate("[TAG_GAMING]");
    }
    if (room.full) {
        return TodStringTranslate("[TAG_FULL]");
    }
    color = Color(0, 255, 0);
    return TodStringTranslate("[JOIN_AVAILABLE]");
}

pvzstl::string BuildSpectatorsText(const NetplayLobbyWidget *dialog) {
    pvzstl::string text = StrFormat(TodStringTranslate("[SPECTATORS_NUM]").c_str(), dialog->mServerSpectatorCount);
    if (dialog->mServerSpectatorCount <= 0) {
        text += "-";
        return text;
    }

    for (int i = 0; i < dialog->mServerSpectatorCount && i < kMaxSpectatorNamesShown; ++i) {
        if (i > 0) {
            text += ", ";
        }
        text += dialog->mServerSpectatorNames[i][0] != '\0' ? dialog->mServerSpectatorNames[i] : "-";
    }
    if (dialog->mServerSpectatorCount > kMaxSpectatorNamesShown) {
        text += ", ...";
    }
    return text;
}

bool IsRoomActive(const NetplayLobbyWidget *dialog) {
    return dialog != nullptr && (dialog->mIsCreatingRoom || dialog->mIsJoiningRoom || dialog->mServerHosting || dialog->mServerJoined || dialog->mServerSpectating);
}

void DrawActiveRoomInfo(Graphics *g, const NetplayLobbyWidget *dialog) {
    const int centerX = kRightPanelX + kRightPanelWidth / 2;
    if (dialog->mUIMode == UIMode::MODE2_WIFI) {
        const char *title = dialog->mIsCreatingRoom ? "[ROOM_CREATED_FMT]" : "[JOINING_MANUAL]";
        const char *playerName = (dialog->mApp != nullptr && dialog->mApp->mPlayerInfo != nullptr && dialog->mApp->mPlayerInfo->mName != nullptr) ? dialog->mApp->mPlayerInfo->mName : "-";
        const pvzstl::string text = dialog->mIsCreatingRoom ? StrFormat(TodStringTranslate(title).c_str(), playerName) : TodStringTranslate(title);
        TodDrawString(g, text, centerX, 300, FONT_DWARVENTODCRAFT18, Color(255, 238, 175), DS_ALIGN_CENTER);

        const pvzstl::string detail = dialog->mIsCreatingRoom
            ? (IsRemoteServer() ? StrFormat(TodStringTranslate("[OTHER_JOINED_FMT]").c_str(), gSecondPlayerName) : TodStringTranslate("[WAIT_OTHER_JOIN]"))
            : (IsRemoteClient() ? StrFormat(TodStringTranslate("[JOINED_MANUAL_FMT]").c_str(), gSecondPlayerName) : TodStringTranslate("[JOINING_MANUAL]"));
        TodDrawString(g, detail, centerX, 355, FONT_HOUSEOFTERROR20, Color(235, 220, 185), DS_ALIGN_CENTER);
        return;
    }

    const char *roleKey = dialog->mServerHosting ? "[HOST]" : (dialog->mServerSpectating ? "[SPECTATING]" : "[CLIENT]");
    const char *localName = (dialog->mApp != nullptr && dialog->mApp->mPlayerInfo != nullptr && dialog->mApp->mPlayerInfo->mName != nullptr) ? dialog->mApp->mPlayerInfo->mName : "-";
    const char *hostName = dialog->mServerHosting ? localName : (gServerHostName[0] != '\0' ? gServerHostName : "-");
    pvzstl::string guestName = "-";
    if (dialog->mServerHosting) {
        guestName = dialog->mServerHostHasGuest ? (gSecondPlayerName[0] != '\0' ? gSecondPlayerName : "-") : TodStringTranslate("[WAIT_OTHER_JOIN]");
    } else if (dialog->mServerJoined) {
        guestName = localName;
    } else if (gSecondPlayerName[0] != '\0') {
        guestName = gSecondPlayerName;
    }

    TodDrawString(g, roleKey, centerX, 245, FONT_DWARVENTODCRAFT24, Color(255, 238, 175), DS_ALIGN_CENTER);
    TodDrawString(g, StrFormat("%s: %s", TodStringTranslate("[HOST]").c_str(), hostName), centerX, 285, FONT_HOUSEOFTERROR20, Color(235, 220, 185), DS_ALIGN_CENTER);
    TodDrawString(g, StrFormat("%s: %s", TodStringTranslate("[CLIENT]").c_str(), guestName.c_str()), centerX, 325, FONT_HOUSEOFTERROR20, Color(235, 220, 185), DS_ALIGN_CENTER);

    if (dialog->mServerHostSpectateAllowed || dialog->mServerJoinedSpectateAllowed || dialog->mServerSpectating || dialog->mServerSpectatorCount > 0) {
        TodDrawStringWrapped(g, BuildSpectatorsText(dialog), Rect(kRightPanelX + 145, 345, 560, 65), FONT_HOUSEOFTERROR16, Color(235, 220, 185), DS_ALIGN_CENTER, false);
    }
}
} // namespace

class NetplayRoomListWidget : public Widget {
public:
    NetplayLobbyWidget *mOwner;

    explicit NetplayRoomListWidget(NetplayLobbyWidget *owner) {
        Widget::_constructor();
        static void *sVTable[122];
        static std::once_flag sVTableInit;
        std::call_once(sVTableInit, [this] {
            std::memcpy(sVTable, vTable, sizeof(sVTable));
            sVTable[0] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::_destructor);
            sVTable[1] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::_destructor2);
            sVTable[36] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::Draw);
            sVTable[78] = (void *)homura::ExtractMemFuncPtr(&NetplayRoomListWidget::MouseDown);
        });
        vTable = sVTable;
        mOwner = owner;
    }

    ~NetplayRoomListWidget() {
        Widget::_destructor();
    }

    void Draw(Graphics *g) {
        if (mOwner == nullptr) {
            return;
        }
        NetplayLobbyWidget *dialog = mOwner;
        const int count = mOwner->GetRoomCount();

        if (count <= 0) {
            if (IsRoomActive(dialog)) {
                return;
            }
            const char *message = dialog->mUIMode == UIMode::MODE3_SERVER ? "[SERVER_NO_ROOMS_TIP]" : "[NO_AVAILABLE_ROOMS]";
            TodDrawString(g, message, kRoomScrollWidth / 2, 150, FONT_HOUSEOFTERROR20, Color(255, 230, 170), DS_ALIGN_CENTER);
            return;
        }

        for (int i = 0; i < count; ++i) {
            const int column = i % 2;
            const int row = i / 2;
            const int x = column * (kRoomCardWidth + kRoomCardGap);
            const int y = row * (kRoomCardHeight + kRoomCardGap);
            const int selected = dialog->mUIMode == UIMode::MODE2_WIFI ? dialog->mSelectedServerIndex : dialog->mSelectedRoomIndex_Server;
            const bool isSelected = i == selected;

            g->SetColor(isSelected ? Color(104, 82, 35, 245) : Color(92, 58, 37, 230));
            g->FillRect(Rect(x, y, kRoomCardWidth, kRoomCardHeight));
            g->SetColor(isSelected ? Color(255, 224, 92) : Color(151, 111, 72));
            g->DrawRect(Rect(x, y, kRoomCardWidth, kRoomCardHeight));

            if (dialog->mUIMode == UIMode::MODE2_WIFI) {
                const auto &room = gServers[i];
                TodDrawString(g, room.name, x + kRoomCardWidth / 2, y + 38, FONT_DWARVENTODCRAFT18, Color(255, 244, 195), DS_ALIGN_CENTER);
                TodDrawString(g, StrFormat("%s:%d", room.ip, room.tcpPort), x + kRoomCardWidth / 2, y + 72, FONT_HOUSEOFTERROR16, Color(208, 208, 208), DS_ALIGN_CENTER);
            } else {
                const ServerRoomItem &room = dialog->mServerRooms[i];
                Color tagColor;
                const pvzstl::string tag = BuildRoomTag(room, tagColor);
                TodDrawString(g, room.name, x + kRoomCardWidth / 2, y + 38, FONT_DWARVENTODCRAFT18, Color(255, 244, 195), DS_ALIGN_CENTER);
                TodDrawString(g, tag, x + kRoomCardWidth / 2, y + 72, FONT_HOUSEOFTERROR16, tagColor, DS_ALIGN_CENTER);
            }
        }
    }

    void MouseDown(int x, int y, int theClickCount) {
        (void)theClickCount;
        if (mOwner == nullptr) {
            return;
        }
        const int column = x / (kRoomCardWidth + kRoomCardGap);
        const int row = y / (kRoomCardHeight + kRoomCardGap);
        if (column < 0 || column > 1 || x - column * (kRoomCardWidth + kRoomCardGap) >= kRoomCardWidth || y - row * (kRoomCardHeight + kRoomCardGap) >= kRoomCardHeight) {
            return;
        }
        mOwner->SelectRoom(row * 2 + column);
    }

protected:
    void _destructor() {
        Widget::_destructor();
    }
    void _destructor2() {
        delete this;
    }
};

NetplayLobbyWidget *NetplayLobbyWidget::gInstance = nullptr;

NetplayLobbyWidget *NetplayLobbyWidget::GetInstance() {
    return gInstance;
}

NetplayLobbyWidget::NetplayLobbyWidget(LawnApp *app, bool isCoopLobby) {
    Widget::_constructor();
    static void *sVTable[122];
    static std::once_flag sVTableInit;
    std::call_once(sVTableInit, [&] {
        std::memcpy(sVTable, Sexy::Widget::vTable, sizeof(sVTable));
        sVTable[0] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::_destructor);
        sVTable[1] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::_destructor2);
        sVTable[29] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::AddedToManager);
        sVTable[30] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::RemovedFromManager);
        sVTable[31] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::Update);
        sVTable[36] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::Draw);
        sVTable[78] = (void *)homura::ExtractMemFuncPtr(&NetplayLobbyWidget::MouseDown);
    });
    Sexy::Widget::vTable = sVTable;
    Sexy::ButtonListener::vTable = &sButtonListenerVtable;
    mApp = app;
    mApp->mPlayerInfo->EnsureNetplayPlayerId();
    mResult = 0;
    mCloseRequested = false;
    gInstance = this;

    Resize(LawnApp::FULLSCREEN_RECT.mX, LawnApp::FULLSCREEN_RECT.mY, LawnApp::FULLSCREEN_RECT.mWidth, LawnApp::FULLSCREEN_RECT.mHeight);
    mClip = true;
    mSelectedServerListIndex = 0;
    mZombieBackground = Rand(2);
    mIsCoopLobby = isCoopLobby;

    mRoomScrollWidget = new ScrollWidget();
    mRoomScrollWidget->Resize(kRoomScrollX, kRoomScrollY, kRoomScrollWidth, kRoomScrollHeight);
    mRoomScrollWidget->SetScrollMode(ScrollWidget::SCROLL_VERTICAL);
    mRoomScrollWidget->EnableBounce(false);
    mRoomListWidget = new NetplayRoomListWidget(this);
    mRoomListWidget->Resize(0, 0, kRoomScrollWidth, kRoomScrollHeight);

    mAddServerButton = MakeButton(NetplayLobbyWidget_AddServer, this, this, "[CONNECT_CUSTOM_SERVER]");
    mAddServerButton->Resize(60, 545 + kPanelYOffset, 290, 50);
    mCreateRoomButton = MakeButton(NetplayLobbyWidget_CreateRoom, this, this, "[CREATE_ROOM_BUTTON]");
    mCreateRoomButton->Resize(410, 545 + kPanelYOffset, 390, 50);
    mJoinRoomButton = MakeButton(NetplayLobbyWidget_JoinRoom, this, this, "[JOIN_ROOM_BUTTON]");
    mJoinRoomButton->Resize(830, 545 + kPanelYOffset, 390, 50);
    mReplayManageButton = MakeButton(NetplayLobbyWidget_ReplayManage, this, this, "[REPLAY_MANAGE]");
    mReplayManageButton->Resize(40, 650, 230, 50);
    mLocalBattleButton = MakeButton(NetplayLobbyWidget_LocalBattle, this, this, "[PLAY_OFFLINE]");
    mLocalBattleButton->Resize(525, 650, 230, 50);
    mBackButton = MakeButton(NetplayLobbyWidget_Back, this, this, "[BACK]");
    mBackButton->Resize(1010, 650, 230, 50);
    mPrimaryActionButton = MakeButton(NetplayLobbyWidget_PrimaryAction, this, this, "[START_GAME]");
    mPrimaryActionButton->Resize(410, 465 + kPanelYOffset, 390, 48);
    mRoomOptionButton = MakeButton(NetplayLobbyWidget_RoomOption, this, this, "[ENABLE_SPECTATE]");
    mRoomOptionButton->Resize(830, 465 + kPanelYOffset, 390, 48);
    mPrimaryActionButton->SetVisible(false);
    mRoomOptionButton->SetVisible(false);

    mIsCreatingRoom = false;
    mIsJoiningRoom = false;
    mReplayManageWidget = nullptr;
    mSelectedServerIndex = 0;
    mUseManualTarget = false;
    mManualIp[0] = '\0';
    mManualPort = 0;
    mUIMode = UIMode::MODE1_INIT;
    mInputPurpose = InputPurpose::NONE;
    mServerConnected = false;
    mSelectedRoomIndex_Server = 0;
    mServerLatencyMs = -1;
    mServerQuerySentTick = 0;
    mServerQueryPending = false;
    std::fill_n(mServerTargetLatencyMs, 5, -1);
    std::fill_n(mServerTargetProbeSock, 5, -1);
    std::fill_n(mServerTargetProbeStartTick, 5, 0);
    mServerTargetNextRefreshTick = 0;
    mServerSock = -1;
    mServerConnecting = false;
    mServerHosting = false;
    mServerJoined = false;
    mServerSpectating = false;
    mServerCreatePending = false;
    mServerHostProbeDone = false;
    mServerGuestProbeDone = false;
    mServerHostHasGuest = false;
    mServerHostSpectateAllowed = false;
    mServerJoinedSpectateAllowed = false;
    mServerHostForceRelay = false;
    mServerClientWantStart = false;
    mServerAskedWantStart = false;
    mServerJoinedRoomGaming = false;
    mServerSpectateReservationActive = false;
    mServerHostedRoomId = 0;
    mServerJoinedRoomId = 0;
    mServerLastQueryTick = 0;
    mServerLastRecvTick = 0;
    mServerSpectateReserveTick = 0;
    mServerSpectateReserveWarnTick = 0;
    mServerHostedRoomName[0] = '\0';
    mServerJoinedRoomName[0] = '\0';
    mServerSpectatorCount = 0;
    std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));
    mServerIp[0] = '\0';
    mServerPort = 0;
    mServerRoomCount = 0;
    mServerRoomPage = 0;
    mSrvRecvLen = 0;
    mServerP2PListenSock = -1;
    mServerP2PPendingSock = -1;
    mServerP2PConnectingSock = -1;
    mServerP2PPendingFromAccept = false;
    mServerP2PListenerFailed = false;
    mServerP2PNatSent = false;
    mServerP2POkSent = false;
    mServerP2PFailSent = false;
    mServerP2PDoneReceived = false;
    mServerGameStarting = false;
    mServerGameStartingTick = 0;
    mServerRelayEpoch = 0;
    mServerP2PLocalPort = 0;
    mServerP2PProbePort = 0;
    mServerP2PProbePort2 = 0;
    mServerP2PProbeToken = 0;
    mServerP2PProbeDone = false;
    mServerP2PProbeActive = false;
    mServerP2PProbeSocketConnected = false;
    mServerP2PProbeTargetOk[0] = false;
    mServerP2PProbeTargetOk[1] = false;
    mServerP2PProbeSock = -1;
    mServerP2PProbeAttempt = 0;
    mServerP2PProbeTargetIndex = 0;
    mServerP2PProbeStartTick = 0;
    mServerP2PProbeTokenBytesSent = 0;
    mServerP2PDeadlineTick = 0;
    mServerP2PNextRetryTick = 0;
    mServerP2PTick = 0;
    mServerP2PTargetRoomId = 0;
    mServerP2PPeerPort = 0;
    mServerP2PTimeoutSec = 0;
    mServerP2PPeerIp[0] = '\0';
    std::memset(mServerRooms, 0, sizeof(mServerRooms));
    std::memset(mSrvRecvBuf, 0, sizeof(mSrvRecvBuf));
    mServerStatusText = TodStringTranslate("[STATUS_NOT_CONNECTED]");
    mServerP2PStatusText = "P2P: idle";

    gSecondPlayerName[0] = '\0';
    gServerHostName[0] = '\0';
    gIsServerModeNetplay = false;
    gServerModeTransport = ServerModeTransport::NONE;
    gIsServerModeSpectator = false;

    TodLoadResources("DelayLoad_Almanac");
    SetMode(UIMode::MODE2_WIFI);
}

NetplayLobbyWidget::~NetplayLobbyWidget() {
    _destructor();
}

void NetplayLobbyWidget::_destructor() {
    CloseReplayManageWidget();
    ServerDisconnect("lobby destroy");
    if (gInstance == this) {
        gInstance = nullptr;
    }
    delete mRoomOptionButton;
    delete mPrimaryActionButton;
    delete mBackButton;
    delete mLocalBattleButton;
    delete mReplayManageButton;
    delete mJoinRoomButton;
    delete mCreateRoomButton;
    delete mAddServerButton;
    delete mRoomListWidget;
    delete mRoomScrollWidget;
    Widget::_destructor();
}

void NetplayLobbyWidget::_destructor2() {
    delete this;
}

void NetplayLobbyWidget::AddedToManager(WidgetManager *theWidgetManager) {
    WidgetContainer::AddedToManager(theWidgetManager);
    AddWidget(mRoomScrollWidget);
    mRoomScrollWidget->AddWidget(mRoomListWidget);
    AddWidget(mAddServerButton);
    AddWidget(mCreateRoomButton);
    AddWidget(mJoinRoomButton);
    AddWidget(mReplayManageButton);
    AddWidget(mLocalBattleButton);
    AddWidget(mBackButton);
    AddWidget(mPrimaryActionButton);
    AddWidget(mRoomOptionButton);
}

void NetplayLobbyWidget::RemovedFromManager(WidgetManager *theWidgetManager) {
    WidgetContainer::RemovedFromManager(theWidgetManager);
    mRoomScrollWidget->RemoveWidget(mRoomListWidget);
    RemoveWidget(mRoomScrollWidget);
    RemoveWidget(mAddServerButton);
    RemoveWidget(mCreateRoomButton);
    RemoveWidget(mJoinRoomButton);
    RemoveWidget(mReplayManageButton);
    RemoveWidget(mLocalBattleButton);
    RemoveWidget(mBackButton);
    RemoveWidget(mPrimaryActionButton);
    RemoveWidget(mRoomOptionButton);
}

int NetplayLobbyWidget::GetRoomCount() const {
    if (mIsCreatingRoom || mIsJoiningRoom || mServerHosting || mServerJoined || mServerSpectating) {
        return 0;
    }
    if (mUIMode == UIMode::MODE2_WIFI) {
        return std::max(0, gScannedServerCount);
    }
    if (mUIMode == UIMode::MODE3_SERVER && mServerConnected) {
        return std::max(0, mServerRoomCount);
    }
    return 0;
}

void NetplayLobbyWidget::RefreshControls() {
    RefreshButtons();

    const bool lanRoomActive = mIsCreatingRoom || mIsJoiningRoom;
    const bool serverRoomActive = mServerHosting || mServerJoined || mServerSpectating;
    const bool roomActive = lanRoomActive || serverRoomActive;
    const bool serverIdleDisconnected = mUIMode == UIMode::MODE3_SERVER && !mServerConnected && !mServerConnecting && !serverRoomActive;
    if (serverIdleDisconnected) {
        mCreateRoomButton->SetLabel("[CREATE_ROOM_BUTTON]");
        mCreateRoomButton->mDisabled = true;
    }

    // These two controls are fixed parts of the lobby layout. Context-sensitive
    // room actions are presented separately inside the room panel below.
    mAddServerButton->SetLabel("[CONNECT_CUSTOM_SERVER]");
    mAddServerButton->mDisabled = lanRoomActive || serverRoomActive || mServerConnecting;
    mReplayManageButton->SetLabel("[REPLAY_MANAGE]");
    mReplayManageButton->mDisabled = roomActive;
    mLocalBattleButton->SetLabel("[PLAY_OFFLINE]");
    mLocalBattleButton->mDisabled = roomActive || mServerConnecting;

    const bool showPrimaryAction = mIsCreatingRoom || serverRoomActive;
    mPrimaryActionButton->SetVisible(showPrimaryAction);

    const bool showRoomOption = mUIMode == UIMode::MODE3_SERVER && mServerHosting;
    mRoomOptionButton->SetVisible(showRoomOption);

    if (roomActive) {
        GameButton *roomActions[4];
        int roomActionCount = 0;
        if (showPrimaryAction) {
            roomActions[roomActionCount++] = mPrimaryActionButton;
        }
        if (showRoomOption) {
            roomActions[roomActionCount++] = mRoomOptionButton;
        }
        // Hosts keep "kick guest" before "leave room". Guests and spectators
        // put the role-switch action before "leave room".
        if (mServerJoined || mServerSpectating) {
            roomActions[roomActionCount++] = mCreateRoomButton;
            roomActions[roomActionCount++] = mJoinRoomButton;
        } else {
            roomActions[roomActionCount++] = mJoinRoomButton;
            roomActions[roomActionCount++] = mCreateRoomButton;
        }

        const int actionsWidth = roomActionCount * kRoomActionWidth + (roomActionCount - 1) * kRoomActionGap;
        const int actionsX = kRoomScrollX + (kRoomScrollWidth - actionsWidth) / 2;
        for (int i = 0; i < roomActionCount; ++i) {
            roomActions[i]->Resize(actionsX + i * (kRoomActionWidth + kRoomActionGap), kRoomActionY, kRoomActionWidth, kRoomActionHeight);
        }
    } else {
        mCreateRoomButton->Resize(410, 545 + kPanelYOffset, 390, 50);
        mJoinRoomButton->Resize(830, 545 + kPanelYOffset, 390, 50);
        mPrimaryActionButton->Resize(410, 465 + kPanelYOffset, 390, 48);
        mRoomOptionButton->Resize(830, 465 + kPanelYOffset, 390, 48);
    }
}

void NetplayLobbyWidget::Draw(Graphics *g) {
    RefreshControls();
    const int roomRows = (GetRoomCount() + 1) / 2;
    const int roomContentHeight = std::max(kRoomScrollHeight, roomRows * kRoomCardHeight + std::max(0, roomRows - 1) * kRoomCardGap);
    if (mRoomListWidget->mHeight != roomContentHeight) {
        mRoomListWidget->Resize(0, 0, kRoomScrollWidth, roomContentHeight);
        // ScrollWidget caches its scroll range. Room lists are refreshed by the
        // networking layer, so explicitly update the cached range when their
        // content height changes.
        mRoomScrollWidget->ClientSizeChanged();
    }
    g->DrawImage(mZombieBackground ? IMAGE_ALMANAC_ZOMBIEBACK : IMAGE_ALMANAC_PLANTBACK, 0, 0);
    g->SetColor(Color(25, 15, 10, 75));
    g->FillRect(Rect(0, 0, mWidth, mHeight));
    DrawPanel(g, Rect(kLeftPanelX, kLeftPanelY, kLeftPanelWidth, kPanelHeight));
    DrawPanel(g, Rect(kRightPanelX, kRightPanelY, kRightPanelWidth, kPanelHeight));

    if (roomContentHeight > kRoomScrollHeight) {
        const int maxScroll = roomContentHeight - kRoomScrollHeight;
        const float scrollOffset = std::clamp(-mRoomScrollWidget->GetScrollOffset().mY, 0.0f, static_cast<float>(maxScroll));
        const float scrollProgress = scrollOffset / static_cast<float>(maxScroll);
        const int thumbHeight = std::max(kRoomScrollbarMinThumbHeight, kRoomScrollHeight * kRoomScrollHeight / roomContentHeight);
        const int thumbTravel = kRoomScrollHeight - thumbHeight;
        const int thumbY = kRoomScrollY + static_cast<int>(thumbTravel * scrollProgress);

        g->SetColor(Color(35, 22, 15, 190));
        g->FillRect(Rect(kRoomScrollbarX, kRoomScrollY, kRoomScrollbarWidth, kRoomScrollHeight));
        g->SetColor(Color(178, 132, 70, 230));
        g->DrawRect(Rect(kRoomScrollbarX, kRoomScrollY, kRoomScrollbarWidth, kRoomScrollHeight));
        g->SetColor(Color(235, 199, 104, 245));
        g->FillRect(Rect(kRoomScrollbarX + 1, thumbY + 1, kRoomScrollbarWidth - 2, thumbHeight - 2));
    }

    const pvzstl::string lobbyModeTitle = TodStringTranslate(mIsCoopLobby ? "[XBOX_COOP]" : "[VS]");
    const pvzstl::string lobbyTitle = StrFormat("%s - %s", lobbyModeTitle.c_str(), TodStringTranslate("[NETPLAY_LOBBY_TITLE]").c_str());
    TodDrawString(g, lobbyTitle, mWidth / 2, 115, addonFonts.JN_BOBO_HEI36, Color(255, 248, 195), DS_ALIGN_CENTER);
    TodDrawString(g, "[MODE_SERVER_TITLE]", kLeftPanelX + kLeftPanelWidth / 2, 170 + kPanelYOffset, FONT_DWARVENTODCRAFT18, Color(255, 226, 154), DS_ALIGN_CENTER);
    TodDrawString(g, "[AVAILABLE_ROOMS]", kRightPanelX + kRightPanelWidth / 2, 170 + kPanelYOffset, FONT_DWARVENTODCRAFT18, Color(255, 226, 154), DS_ALIGN_CENTER);

    if (IsRoomActive(this)) {
        DrawActiveRoomInfo(g, this);
    }

    const int targetCount = GetLobbyServerTargetCount();
    const int itemCount = 1 + targetCount;
    mSelectedServerListIndex = std::clamp(mSelectedServerListIndex, 0, std::max(0, itemCount - 1));
    for (int i = 0; i < itemCount; ++i) {
        const int y = kServerRowY + i * kServerRowHeight;
        const bool selected = i == mSelectedServerListIndex;
        g->SetColor(selected ? Color(95, 105, 45, 245) : Color(82, 58, 42, 225));
        g->FillRect(Rect(kLeftPanelX + 18, y - 30, kLeftPanelWidth - 36, 45));
        g->SetColor(selected ? Color(155, 225, 70) : Color(145, 105, 72));
        g->DrawRect(Rect(kLeftPanelX + 18, y - 30, kLeftPanelWidth - 36, 45));

        pvzstl::string label;
        Font *font = FONT_HOUSEOFTERROR16;
        if (i == 0) {
            label = TodStringTranslate("[LAN_MULTIPLAYER]");
            font = addonFonts.JN_BOBO_HEI24;
        } else {
            char address[32]{};
            GetLobbyServerTargetAddress(i - 1, address, sizeof(address));
            if (i <= 2) {
                label = StrFormat(TodStringTranslate("[OFFICIAL_SERVER_NAME]").c_str(), i, address);
            } else {
                label = StrFormat(TodStringTranslate("[CUSTOM_SERVER_NAME]").c_str(), i - 2, address);
            }
        }
        TodDrawString(g, label, kLeftPanelX + kLeftPanelWidth / 2, y, font, selected ? Color(185, 255, 105) : Color(255, 238, 195), DS_ALIGN_CENTER);
    }

    if (mSelectedServerListIndex > 0 && mSelectedServerListIndex <= targetCount) {
        // Target probes are stopped and cleared after connecting. From that
        // point the room-list query round trip is the current server latency.
        const int latency = mServerConnected ? mServerLatencyMs : mServerTargetLatencyMs[mSelectedServerListIndex - 1];
        const pvzstl::string latencyText = latency >= 0 ? StrFormat("%dms", latency) : "--ms";
        TodDrawString(g, latencyText, kSelectedServerLatencyX, kSelectedServerLatencyY, FONT_HOUSEOFTERROR16, Color(235, 220, 185), DS_ALIGN_LEFT);
    }

    {
        pvzstl::string status;
        if (mUIMode == UIMode::MODE3_SERVER) {
            status = mServerStatusText;
        } else if (mIsCreatingRoom) {
            status = TodStringTranslate("[WAIT_OTHER_JOIN]");
        } else if (mIsJoiningRoom) {
            status = TodStringTranslate("[JOINING_MANUAL]");
        } else {
            status = TodStringTranslate(gScannedServerCount > 0 ? "[AVAILABLE_ROOMS]" : "[SCANNING_ROOMS]");
        }
        const bool roomExited = status == TodStringTranslate("[STATUS_ROOM_EXITED]");
        Font *statusFont = addonFonts.JN_BOBO_HEI20 && std::strstr(status.c_str(), "局") ? static_cast<Font *>(addonFonts.JN_BOBO_HEI20) : FONT_HOUSEOFTERROR16;
        if (statusFont == addonFonts.JN_BOBO_HEI20 && statusFont->GetVTable()->StringWidth(statusFont, status) > kRightPanelWidth - 40) {
            TodDrawStringWrapped(g, status, Rect(kRightPanelX + 20, kLobbyStatusY - 20, kRightPanelWidth - 40, 60), statusFont, Color(235, 220, 185), DS_ALIGN_CENTER, false);
        } else {
            TodDrawString(g, status, kRightPanelX + kRightPanelWidth / 2, roomExited ? kRoomExitedStatusY : kLobbyStatusY, statusFont, Color(235, 220, 185), DS_ALIGN_CENTER);
        }
    }
}

void NetplayLobbyWidget::MouseDown(int x, int y, int theClickCount) {
    (void)theClickCount;
    const int firstTop = kServerRowY - 30;
    if (x < kLeftPanelX + 18 || x >= kLeftPanelX + kLeftPanelWidth - 18 || y < firstTop) {
        return;
    }
    const int index = (y - firstTop) / kServerRowHeight;
    if (y - (firstTop + index * kServerRowHeight) >= 45) {
        return;
    }
    SelectServer(index);
}

void NetplayLobbyWidget::SelectServer(int listIndex) {
    if (listIndex < 0 || listIndex > GetLobbyServerTargetCount()) {
        return;
    }
    if (listIndex != mSelectedServerListIndex && ServerHostRoomLocked()) {
        return;
    }
    if (listIndex == mSelectedServerListIndex && ((listIndex == 0 && mUIMode == UIMode::MODE2_WIFI) || (listIndex > 0 && (mServerConnected || mServerConnecting)))) {
        return;
    }
    mSelectedServerListIndex = listIndex;
    mRoomScrollWidget->ScrollToMin(false);
    mApp->PlaySample(SOUND_GRAVEBUTTON);
    if (listIndex == 0) {
        SetMode(UIMode::MODE2_WIFI);
    } else {
        SetMode(UIMode::MODE3_SERVER);
        ConnectLobbyServerTarget(listIndex - 1);
    }
}

void NetplayLobbyWidget::SelectRoom(int roomIndex) {
    if (roomIndex < 0 || roomIndex >= GetRoomCount()) {
        return;
    }
    if (mUIMode == UIMode::MODE2_WIFI) {
        mSelectedServerIndex = roomIndex;
    } else {
        mSelectedRoomIndex_Server = roomIndex;
    }
    mApp->PlaySample(SOUND_GRAVEBUTTON);
}


namespace {
constexpr int kServerRoomListTitleY = 200;
constexpr int kServerRoomListItemStartY = 200;
constexpr int kServerRoomListLineH = 45;
constexpr int kServerRoomListPageSize = 5;
constexpr int kServerRoomListPrevPageX = 150;
constexpr int kServerRoomListNextPageX = 610;
constexpr int kServerRoomListPageArrowY = 325;
constexpr int kServerRoomListPageNumberY = 440;
constexpr int kServerP2PConnectRetryTicks = 8;
constexpr int kServerP2PProbeAttempts = 3;
constexpr int kServerP2PProbeTimeoutMs = 5000;
constexpr int kMode3ServerOfficialItemStartY = 190;
constexpr int kMode3ServerRecentItemStartY = 304;
constexpr int kMode3ServerTargetLineH = 38;
constexpr int kMode3ServerTargetMaxLen = 22;
constexpr int kMode3ServerRecentCount = 3;
constexpr int kMode3ServerTargetCountMax = 2 + kMode3ServerRecentCount;
constexpr const char *kOfficialServer1Addr = "8.134.55.112:26667";
constexpr const char *kOfficialServer2Addr = "47.122.122.51:26667";

static void CloseSocketFd(int &fd, bool do_shutdown = true) {
    if (fd < 0)
        return;
    if (do_shutdown)
        shutdown(fd, SHUT_RDWR);
    close(fd);
    fd = -1;
}

static void ConfigureTcpSocket(int fd) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
    int idle = 30;
    int intvl = 10;
    int cnt = 3;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void EnableReuseOptions(int fd) {
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
}

static bool BindSocketToAnyPort(int fd, int port) {
    sockaddr_in sa{
        .sin_family = AF_INET,
        .sin_port = htons((uint16_t)port),
        .sin_addr{.s_addr = INADDR_ANY},
    };
    return bind(fd, (sockaddr *)&sa, sizeof(sa)) == 0;
}

static void ResetVsStreamBuffersForServerMode() {
    clientRecvBuffer.clear();
    serverRecvBuffer.clear();
    netplay::ClearSendBuffer();
}

static void Mode3DrawListTextWithShadow(Sexy::Graphics *g, const pvzstl::string &text, int x, int y, Sexy::Font *font, const Sexy::Color &color) {
    TodDrawString(g, text, x - 1, y - 1, font, Sexy::Color(0, 0, 0, color.mAlpha), DS_ALIGN_CENTER);
    TodDrawString(g, text, x, y, font, color, DS_ALIGN_CENTER);
}
static bool ParseMode3IpPort(std::string_view inputRaw, std::string &outIp, int &outPort) {
    const std::string input = homura::Trim(inputRaw);
    const size_t colonPos = input.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }

    const std::string ip = homura::Trim(std::string_view{input}.substr(0, colonPos));
    const std::string portStr = homura::Trim(std::string_view{input}.substr(colonPos + 1));
    const int port = std::atoi(portStr.c_str());
    if (port < 1 || port > 65535) {
        return false;
    }

    in_addr addr{};
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        return false;
    }

    outIp = ip;
    outPort = port;
    return true;
}


static bool Mode3LoadRecentServer(const LawnPlayerInfo *playerInfo, int idx, char outAddr[kMode3ServerTargetMaxLen]) {
    if (!playerInfo || idx < 0 || idx >= kMode3ServerRecentCount) {
        return false;
    }

    std::memset(outAddr, 0, kMode3ServerTargetMaxLen);
    std::memcpy(outAddr, playerInfo->serverStorage.mRecentServerAddr[idx], kMode3ServerTargetMaxLen - 1);
    outAddr[kMode3ServerTargetMaxLen - 1] = '\0';

    std::string ip;
    int port = 0;
    if (!ParseMode3IpPort(outAddr, ip, port)) {
        outAddr[0] = '\0';
        return false;
    }
    return true;
}

static void Mode3RememberRecentServer(LawnPlayerInfo *playerInfo, std::string_view addrRaw) {
    if (!playerInfo) {
        return;
    }

    std::string ip;
    int port = 0;
    if (!ParseMode3IpPort(addrRaw, ip, port)) {
        return;
    }

    const std::string normalized = ip + ':' + std::to_string(port);
    char ordered[kMode3ServerRecentCount][kMode3ServerTargetMaxLen]{};
    std::strncpy(ordered[0], normalized.c_str(), kMode3ServerTargetMaxLen - 1);

    int writeIdx = 1;
    for (int i = 0; i < kMode3ServerRecentCount && writeIdx < kMode3ServerRecentCount; ++i) {
        char oldAddr[kMode3ServerTargetMaxLen]{};
        if (!Mode3LoadRecentServer(playerInfo, i, oldAddr)) {
            continue;
        }
        if (normalized == oldAddr) {
            continue;
        }
        std::strncpy(ordered[writeIdx], oldAddr, kMode3ServerTargetMaxLen - 1);
        ++writeIdx;
    }

    for (int i = 0; i < kMode3ServerRecentCount; ++i) {
        std::memset(playerInfo->serverStorage.mRecentServerAddr[i], 0, kMode3ServerTargetMaxLen);
        std::memcpy(playerInfo->serverStorage.mRecentServerAddr[i], ordered[i], kMode3ServerTargetMaxLen - 1);
    }
    playerInfo->SaveDetails();
}

static bool Mode3HasAnyRecentServer(const NetplayLobbyWidget *dialog) {
    if (!dialog || !dialog->mApp || !dialog->mApp->mPlayerInfo) {
        return false;
    }

    for (int i = 0; i < kMode3ServerRecentCount; ++i) {
        char recentAddr[kMode3ServerTargetMaxLen]{};
        if (Mode3LoadRecentServer(dialog->mApp->mPlayerInfo, i, recentAddr)) {
            return true;
        }
    }
    return false;
}

static bool Mode3ConnectToTarget(NetplayLobbyWidget *dialog, std::string_view ipRaw, int port) {
    if (!dialog) {
        return false;
    }

    const std::string ip = homura::Trim(ipRaw);
    if (dialog->mServerSock >= 0) {
        dialog->ServerDisconnect("reconnect");
    }

    dialog->mServerIp[ip.copy(dialog->mServerIp, INET_ADDRSTRLEN - 1)] = '\0';
    dialog->mServerPort = port;
    netplay::MetricsSetEndpoint(dialog->mServerIp, dialog->mServerPort);
    LOG_DEBUG("target: {}:{}", &dialog->mServerIp[0], dialog->mServerPort);

    dialog->mServerSock = socket(AF_INET, SOCK_STREAM, 0);
    if (dialog->mServerSock < 0) {
        dialog->mServerStatusText = TodStringTranslate("[STATUS_SOCKET_FAIL]");
        return false;
    }

    int one = 1;
    setsockopt(dialog->mServerSock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    int flags = fcntl(dialog->mServerSock, F_GETFL, 0);
    fcntl(dialog->mServerSock, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in sa{
        .sin_family = AF_INET,
        .sin_port = htons(uint16_t(dialog->mServerPort)),
    };
    inet_pton(AF_INET, dialog->mServerIp, &sa.sin_addr);

    int ret = connect(dialog->mServerSock, (sockaddr *)&sa, sizeof(sa));
    int err = errno;
    if (ret == 0) {
        gIsConnectedToServer = true;
        dialog->mServerConnecting = false;
        dialog->mServerConnected = true;
        dialog->mServerStatusText = TodStringTranslate("[STATUS_CONNECTED]");
        dialog->ServerResetP2PState(false);
        dialog->ServerOpenP2PListener();
        if (dialog->mServerP2PListenSock >= 0) {
            dialog->ServerSendNatPort();
        }
        dialog->mServerRoomCount = 0;
        dialog->mServerRoomPage = 0;
        dialog->mSrvRecvLen = 0;
        dialog->ServerSendQuery();
        return true;
    }

    if (err == EINPROGRESS) {
        dialog->mServerConnecting = true;
        dialog->mServerConnected = false;
        dialog->mServerStatusText = TodStringTranslate("[STATUS_CONNECTING]");
        return true;
    }

    dialog->ServerDisconnect("connect fail");
    pvzstl::string strFmt = TodStringTranslate("[STATUS_CONNECT_FAIL_ERRNO_FMT]");
    dialog->mServerStatusText = StrFormat(strFmt.c_str(), std::strerror(err));
    return false;
}

static int Mode3ServerTargetCount(const NetplayLobbyWidget *dialog) {
    if (!dialog) {
        return 0;
    }
    return 2 + (Mode3HasAnyRecentServer(dialog) ? kMode3ServerRecentCount : 0);
}

static bool Mode3GetSelectedTargetAddr(NetplayLobbyWidget *dialog, std::string &outAddr) {
    if (!dialog) {
        return false;
    }
    int idx = dialog->mSelectedRoomIndex_Server;
    const int count = Mode3ServerTargetCount(dialog);
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= count) {
        idx = count - 1;
    }
    dialog->mSelectedRoomIndex_Server = idx;

    if (idx == 0) {
        outAddr = kOfficialServer1Addr;
        return true;
    }
    if (idx == 1) {
        outAddr = kOfficialServer2Addr;
        return true;
    }
    if (dialog->mApp && dialog->mApp->mPlayerInfo) {
        char recentAddr[kMode3ServerTargetMaxLen]{};
        if (Mode3LoadRecentServer(dialog->mApp->mPlayerInfo, idx - 2, recentAddr)) {
            outAddr = recentAddr;
            return true;
        }
    }
    return false;
}

[[maybe_unused]] static bool Mode3ConnectSelectedTarget(NetplayLobbyWidget *dialog) {
    std::string targetAddr;
    if (!Mode3GetSelectedTargetAddr(dialog, targetAddr)) {
        return false;
    }
    std::string ip;
    int port = 0;
    if (!ParseMode3IpPort(targetAddr, ip, port)) {
        return false;
    }
    return Mode3ConnectToTarget(dialog, ip, port);
}

static bool Mode3CanSwitchGuestToSpectator(const NetplayLobbyWidget *dialog) {
    if (!dialog || !dialog->mServerJoined || dialog->mServerGameStarting || !dialog->mServerConnected || dialog->mServerConnecting) {
        return false;
    }
    if (!dialog->mServerJoinedSpectateAllowed) {
        return false;
    }
    return true;
}

static bool Mode3CanSwitchSpectatorToGuest(const NetplayLobbyWidget *dialog) {
    if (!dialog || !dialog->mServerSpectating || dialog->mServerGameStarting || !dialog->mServerConnected || dialog->mServerConnecting) {
        return false;
    }
    if (dialog->mServerJoinedRoomGaming) {
        return false;
    }
    return gSecondPlayerName[0] == '\0';
}

static void Mode3ResetTargetLatencyProbes(NetplayLobbyWidget *dialog) {
    if (!dialog) {
        return;
    }
    for (int i = 0; i < kMode3ServerTargetCountMax; ++i) {
        CloseSocketFd(dialog->mServerTargetProbeSock[i]);
        dialog->mServerTargetLatencyMs[i] = -1;
        dialog->mServerTargetProbeStartTick[i] = 0;
    }
    dialog->mServerTargetNextRefreshTick = 0;
}

} // namespace

bool NetplayLobbyWidget::ServerHostRoomLocked() const {
    return mUIMode == UIMode::MODE3_SERVER && mServerConnected && mServerHosting && (mServerHostHasGuest || mServerSpectating);
}

bool NetplayLobbyWidget::ServerIsWaitingReservedSpectate() const {
    return mServerSpectating && mServerJoinedRoomGaming && mServerSpectateReservationActive && gIsServerModeSpectator;
}

void NetplayLobbyWidget::SetMode(UIMode mode) {
    if (mReplayManageWidget != nullptr) {
        CloseReplayManageWidget();
    }
    if (mode != mUIMode && mode != UIMode::MODE3_SERVER && ServerHostRoomLocked()) {
        RefreshButtons();
        return;
    }

    // 退出旧模式时做必要清理


    if (mUIMode == UIMode::MODE2_WIFI) {
        // 离开 WIFI 模式：停止广播、退出/离开、关闭扫描
        StopUdpBroadcastRoom();
        LeaveRoom();
        ExitRoom();
        CloseUdpScanSocket();
    }
    if (mUIMode == UIMode::MODE3_SERVER) {
        // 这里先只清状态；真正断开服务器连接你后续接入socket再处理
        ServerDisconnect("mode change");
    }

    mUIMode = mode;
    // 进入 WIFI 模式默认开始扫描
    if (mUIMode == UIMode::MODE2_WIFI) {
        mIsCreatingRoom = false;
        mIsJoiningRoom = false;
        InitUdpScanSocket();
    } else if (mUIMode == UIMode::MODE3_SERVER) {
        mSelectedRoomIndex_Server = 0; // 默认选中官方服第1项
        mServerRoomPage = 0;
    }

    RefreshButtons();
}

void NetplayLobbyWidget::RefreshButtons() {
    switch (mUIMode) {
        case UIMode::MODE1_INIT: {
            mJoinRoomButton->SetLabel("[WIFI_VS]");
            mJoinRoomButton->mDisabled = false;

            mCreateRoomButton->SetLabel("[SERVER_VS]");
            mCreateRoomButton->mDisabled = false;

            mPrimaryActionButton->SetLabel("[PLAY_OFFLINE]");
            mPrimaryActionButton->mDisabled = false;

            mRoomOptionButton->SetLabel("[BACK]");
            mRoomOptionButton->mDisabled = false;
        } break;
        case UIMode::MODE2_WIFI: {
            //  如果正在创建房间（Host），左按钮改成“设置房间端口”
            if (mIsCreatingRoom) {
                // left: 设置端口
                mJoinRoomButton->SetLabel("[SET_ROOM_PORT]");
                mJoinRoomButton->mDisabled = IsRemoteServer();

                // right: 退出房间
                mCreateRoomButton->SetLabel("[EXIT_ROOM_BUTTON]");
                mCreateRoomButton->mDisabled = false;

                // Yes: 开始游戏（有人加入才可点）
                mPrimaryActionButton->SetLabel("[START_GAME]");
                mPrimaryActionButton->mDisabled = !IsRemoteServer();

                // No: 返回模式选择
                mRoomOptionButton->SetLabel("[BACK_TO_MODE_SELECT]");
                mRoomOptionButton->mDisabled = false;

                break;
            }

            // left: 加入/离开
            mJoinRoomButton->SetLabel(mIsJoiningRoom ? "[LEAVE_ROOM_BUTTON]" : "[JOIN_ROOM_BUTTON]");
            if (mIsJoiningRoom) {
                mJoinRoomButton->mDisabled = false;
            } else {
                // 扫描模式下：没房间就禁用“加入房间”
                bool inScanMode = (!mIsCreatingRoom && !mIsJoiningRoom);
                if (inScanMode) {
                    mJoinRoomButton->mDisabled = (gScannedServerCount == 0);
                } else {
                    // 其他情况（例如创建房间时 left 通常禁用）
                    mJoinRoomButton->mDisabled = true;
                }
            }

            // right: 创建/退出
            mCreateRoomButton->SetLabel(mIsCreatingRoom ? "[EXIT_ROOM_BUTTON]" : "[CREATE_ROOM_BUTTON]");
            mCreateRoomButton->mDisabled = mIsJoiningRoom;

            // Yes：未创建房间 -> “加入指定IP房间”；创建房间 -> “开始游戏”
            mPrimaryActionButton->SetLabel("[JOIN_SPECIFIED_IP_ROOM]");
            mPrimaryActionButton->mDisabled = mIsJoiningRoom;

            mRoomOptionButton->SetLabel("[BACK_TO_MODE_SELECT]");
            mRoomOptionButton->mDisabled = false;
        } break;
        case UIMode::MODE3_SERVER: {
            const bool startBusy = mServerGameStarting;
            const bool hostLocked = ServerHostRoomLocked();
            const bool inServerListMode = (!mServerConnected && !mServerConnecting && !mServerHosting && !mServerJoined && !mServerSpectating);
            if (mServerHosting) {
                mJoinRoomButton->SetLabel("[KICK_GUEST_BUTTON]");
            } else if (mServerSpectating) {
                mJoinRoomButton->SetLabel("[LEAVE_ROOM_BUTTON]");
            } else if (mServerJoined) {
                mJoinRoomButton->SetLabel("[LEAVE_ROOM_BUTTON]");
            } else if (inServerListMode) {
                mJoinRoomButton->SetLabel("[CONNECT_THIS_SERVER]");
            } else {
                mJoinRoomButton->SetLabel("[JOIN_ROOM_BUTTON]");
            }

            // right: 创建 / 退出
            if (mServerHosting) {
                mCreateRoomButton->SetLabel("[EXIT_ROOM_BUTTON]");
            } else if (mServerJoined) {
                mCreateRoomButton->SetLabel("[SWITCH_TO_SPECTATOR]");
            } else if (mServerSpectating) {
                mCreateRoomButton->SetLabel("[SWITCH_TO_GUEST]");
            } else if (inServerListMode) {
                mCreateRoomButton->SetLabel("[REPLAY_MANAGE]");
            } else {
                mCreateRoomButton->SetLabel("[CREATE_ROOM_BUTTON]");
            }

            // ✅ YesButton：host/joined 都显示“开始游戏”
            if (mServerHosting) {
                mPrimaryActionButton->SetLabel("[START_GAME]");
                mPrimaryActionButton->mDisabled = (!mServerConnected || mServerConnecting || !mServerHostHasGuest || startBusy);
            } else if (mServerSpectating) {
                if (mServerJoinedRoomGaming) {
                    mPrimaryActionButton->SetLabel("[SPECTATE]");
                    mPrimaryActionButton->mDisabled = mServerSpectateReservationActive || ((!mServerConnected && gTcpServerSocket < 0) || startBusy);
                } else {
                    mPrimaryActionButton->SetLabel("[START_GAME]");
                    mPrimaryActionButton->mDisabled = true;
                }
            } else if (mServerJoined) {
                mPrimaryActionButton->SetLabel("[START_GAME]");
                mPrimaryActionButton->mDisabled = true; // ✅ guest 永远禁用
            } else if (mServerConnecting) {
                mPrimaryActionButton->SetLabel("[CONNECT_STOP]");
                mPrimaryActionButton->mDisabled = startBusy || hostLocked;
            } else if (mServerConnected) {
                mPrimaryActionButton->SetLabel("[DISCONNECT_SERVER]");
                mPrimaryActionButton->mDisabled = startBusy || hostLocked;
            } else {
                mPrimaryActionButton->SetLabel("[CONNECT_CUSTOM_SERVER]");
                mPrimaryActionButton->mDisabled = startBusy || hostLocked;
            }

            if (mServerHosting) {
                mRoomOptionButton->SetLabel(mServerHostSpectateAllowed ? "[DISABLE_SPECTATE]" : "[ENABLE_SPECTATE]");
                mRoomOptionButton->mDisabled = (!mServerConnected || mServerConnecting || startBusy);
            } else {
                mRoomOptionButton->SetLabel("[BACK_TO_MODE_SELECT]");
                mRoomOptionButton->mDisabled = hostLocked;
            }

            bool canJoinServer = false;
            if (inServerListMode) {
                std::string targetAddr;
                canJoinServer = Mode3GetSelectedTargetAddr(this, targetAddr);
            }

            bool canJoinIdle = (mServerConnected && !mServerConnecting && !mServerHosting && !mServerJoined && !mServerSpectating && !mServerCreatePending && !startBusy && (mServerRoomCount > 0));
            if (inServerListMode) {
                mJoinRoomButton->mDisabled = !canJoinServer || startBusy || hostLocked;
            } else {
                mJoinRoomButton->mDisabled = !canJoinIdle && !mServerJoined && !mServerSpectating; // 离开房间时应可点
            }
            if (mServerHosting) {
                mJoinRoomButton->mDisabled = (!mServerConnected || mServerConnecting || !mServerHostHasGuest || startBusy);
            }
            if (mServerJoined) {
                mJoinRoomButton->mDisabled = (!mServerConnected || mServerConnecting || startBusy);
            }
            if (mServerSpectating) {
                if (mServerJoinedRoomGaming && !mServerSpectateReservationActive) {
                    mJoinRoomButton->mDisabled = false;
                } else {
                    mJoinRoomButton->mDisabled = mServerSpectateReservationActive || (((!mServerConnected || mServerConnecting) && gTcpServerSocket < 0) || startBusy);
                }
            }

            // 创建按钮：空闲态可创建；hosting 时可退出
            bool canCreateIdle = (mServerConnected && !mServerConnecting && !mServerJoined && !mServerSpectating && !mServerCreatePending && !startBusy);
            if (mServerJoined) {
                mCreateRoomButton->mDisabled = !Mode3CanSwitchGuestToSpectator(this);
            } else if (mServerSpectating) {
                mCreateRoomButton->mDisabled = mServerJoinedRoomGaming || mServerSpectateReservationActive || !Mode3CanSwitchSpectatorToGuest(this);
            } else if (inServerListMode) {
                mCreateRoomButton->mDisabled = false;
            } else {
                mCreateRoomButton->mDisabled = !canCreateIdle;
            }
        } break;
    }

    if (mUIMode == UIMode::MODE3_SERVER && mServerJoined) {
        mPrimaryActionButton->SetLabel("[ASK_START]");
        mPrimaryActionButton->mDisabled = (!mServerConnected || mServerConnecting || mServerGameStarting);
    }
    if (mUIMode == UIMode::MODE3_SERVER && mReconnectPending) {
        // READY starts the raw-stream handoff. Do not send lobby actions while
        // waiting for GO; the back button can still close the connection.
        mJoinRoomButton->mDisabled = true;
        mCreateRoomButton->mDisabled = true;
        mPrimaryActionButton->mDisabled = true;
    }
}

void NetplayLobbyWidget::ShowTextInput(const char *titleKey, const char *hintKey) {
    Native::BridgeApp *bridgeApp = Native::BridgeApp::getSingleton();
    JNIEnv *env = bridgeApp->getJNIEnv();
    jobject view = bridgeApp->mNativeApp->getView();
    jclass viewCls = env->GetObjectClass(view);
    jmethodID mid = env->GetMethodID(viewCls, "showTextInputDialog2", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jstring jTitle = env->NewStringUTF(TodStringTranslate(titleKey).c_str());
    jstring jHint = env->NewStringUTF(TodStringTranslate(hintKey).c_str());
    jstring jInitial = env->NewStringUTF("");
    env->CallVoidMethod(view, mid, 0, jTitle, jHint, jInitial);
    env->DeleteLocalRef(jTitle);
    env->DeleteLocalRef(jHint);
    env->DeleteLocalRef(jInitial);
    env->DeleteLocalRef(viewCls);
}

void NetplayLobbyWidget::OpenReplayManageWidget() {
    if (mReplayManageWidget != nullptr) {
        return;
    }
    mReplayManageWidget = new ReplayManageWidget(mApp, this);
    AddWidget(mReplayManageWidget);
}

void NetplayLobbyWidget::CloseReplayManageWidget() {
    if (mReplayManageWidget == nullptr) {
        return;
    }
    RemoveWidget(mReplayManageWidget);
    delete mReplayManageWidget;
    mReplayManageWidget = nullptr;
}

int NetplayLobbyWidget::GetLobbyServerTargetCount() const {
    int count = 2;
    if (mApp == nullptr || mApp->mPlayerInfo == nullptr) {
        return count;
    }
    for (int i = 0; i < kMode3ServerRecentCount; ++i) {
        char address[kMode3ServerTargetMaxLen]{};
        if (Mode3LoadRecentServer(mApp->mPlayerInfo, i, address)) {
            ++count;
        }
    }
    return count;
}

bool NetplayLobbyWidget::GetLobbyServerTargetAddress(int index, char *outAddress, int outSize) const {
    if (outAddress == nullptr || outSize <= 0) {
        return false;
    }
    outAddress[0] = '\0';
    const char *address = nullptr;
    char recentAddress[kMode3ServerTargetMaxLen]{};
    if (index == 0) {
        address = kOfficialServer1Addr;
    } else if (index == 1) {
        address = kOfficialServer2Addr;
    } else if (mApp != nullptr && mApp->mPlayerInfo != nullptr && Mode3LoadRecentServer(mApp->mPlayerInfo, index - 2, recentAddress)) {
        address = recentAddress;
    }
    if (address == nullptr) {
        return false;
    }
    std::strncpy(outAddress, address, static_cast<size_t>(outSize - 1));
    outAddress[outSize - 1] = '\0';
    return true;
}

bool NetplayLobbyWidget::ConnectLobbyServerTarget(int index) {
    if (index < 0 || index >= GetLobbyServerTargetCount()) {
        return false;
    }
    mSelectedRoomIndex_Server = index;
    return Mode3ConnectSelectedTarget(this);
}

void NetplayLobbyWidget::OpenCustomServerInput() {
    if (mUIMode != UIMode::MODE3_SERVER) {
        SetMode(UIMode::MODE3_SERVER);
    }
    mInputPurpose = InputPurpose::SERVER_CONNECT_ADDR;
    ShowTextInput("[INPUT_TITLE_CONNECT_SERVER]", "[HINT_IP_PORT]");
}

void NetplayLobbyWidget::ExitNetplayLobby() {
    if (mUIMode == UIMode::MODE3_SERVER) {
        if (mServerHosting) {
            ServerSendExitRoom();
        } else if (mServerJoined || mServerSpectating) {
            ServerSendLeaveRoom();
        }
        ServerDisconnect("leave netplay lobby");
    }
    SetMode(UIMode::MODE1_INIT);
    RequestClose(NetplayLobbyWidget_BackResult);
}

bool NetplayLobbyWidget::ServerTryReadOneFrame(uint8_t &outType, uint8_t *outPayload, uint16_t &outLen) {
    if (mSrvRecvLen < 3)
        return false;

    uint8_t type = mSrvRecvBuf[0];
    uint16_t len = (uint16_t(mSrvRecvBuf[1]) << 8) | uint16_t(mSrvRecvBuf[2]);
    const bool knownType = (type == 0x81 || type == 0x82 || type == 0x83 || type == 0x84 || type == 0x85 || type == 0x86 || type == 0x87 || type == 0x88 || type == 0x89 || type == 0x8A || type == 0x8B
                            || type == 0x8C || type == 0x8D || type == 0x8E || type == 0x8F || type == 0x90 || type == 0x91 || type == 0x92 || type == 0xFF);
    if (!knownType) {
        std::memmove(mSrvRecvBuf, mSrvRecvBuf + 1, mSrvRecvLen - 1);
        --mSrvRecvLen;
        return false;
    }
    if (len > sizeof(mSrvRecvBuf) - 3) {
        std::memmove(mSrvRecvBuf, mSrvRecvBuf + 1, mSrvRecvLen - 1);
        --mSrvRecvLen;
        return false;
    }
    if (mSrvRecvLen < 3 + (int)len) {
        if (mSrvRecvLen >= 32) {}
        return false;
    }

    outType = type;
    outLen = len;
    if (len > 0 && outPayload) {
        std::memcpy(outPayload, mSrvRecvBuf + 3, len);
    }

    // consume
    int remain = mSrvRecvLen - (3 + (int)len);
    if (remain > 0) {
        std::memmove(mSrvRecvBuf, mSrvRecvBuf + 3 + len, remain);
    }
    mSrvRecvLen = remain;
    return true;
}

void NetplayLobbyWidget::ServerMoveBufferedRelayBytesToVsStream(bool asHost) {
    if (mSrvRecvLen <= 0) {
        return;
    }

    auto &recvBuffer = asHost ? clientRecvBuffer : serverRecvBuffer;
    const auto *begin = reinterpret_cast<const std::byte *>(mSrvRecvBuf);
    recvBuffer.insert(recvBuffer.end(), begin, begin + mSrvRecvLen);
    mSrvRecvLen = 0;
}

static bool SendAll(int sock, const void *data, size_t len);

void NetplayLobbyWidget::ServerUpdateIO() {
    if (mServerSock < 0)
        return;

    // 1) connect 完成检测
    if (mServerConnecting && !mServerConnected) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(mServerSock, &wfds);
        timeval tv{0, 0};
        int r = select(mServerSock + 1, nullptr, &wfds, nullptr, &tv);
        if (r > 0 && FD_ISSET(mServerSock, &wfds)) {
            int err = 0;
            socklen_t elen = sizeof(err);
            getsockopt(mServerSock, SOL_SOCKET, SO_ERROR, &err, &elen);
            if (err == 0) {
                gIsConnectedToServer = true;
                mServerConnecting = false;
                mServerConnected = true;
                mServerStatusText = TodStringTranslate("[STATUS_CONNECTED]");
                ServerResetP2PState(false);
                ServerOpenP2PListener();
                if (mServerP2PListenSock >= 0) {
                    ServerSendNatPort();
                }
                ServerSendQuery();
            } else {
                ServerDisconnect("connect error");
                mServerStatusText = TodStringTranslate("[STATUS_CONNECT_FAILED]");
            }
        }
    }

    // 2) 读数据（非阻塞）
    while (true) {
        if (mSrvRecvLen >= (int)sizeof(mSrvRecvBuf)) {
            // RELAY_GO may be followed immediately by a large snapshot. Parse
            // the control frames and hand off the buffered raw bytes before
            // reading more; leave the rest in the socket for the game reader.
            break;
        }

        ssize_t n = recv(mServerSock, mSrvRecvBuf + mSrvRecvLen, sizeof(mSrvRecvBuf) - mSrvRecvLen, 0);
        if (n > 0) {
            mSrvRecvLen += (int)n;
        } else if (n == 0) {
            ServerDisconnect("server closed");
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            ServerDisconnect("recv error");
            return;
        }
    }

    // 3) 解析帧并处理
    uint8_t type = 0;
    uint16_t len = 0;
    uint8_t payload[sizeof(mSrvRecvBuf)];

    while (ServerTryReadOneFrame(type, payload, len)) {
        // 服务器对战：RespType
        switch (type) {
            case 0x81: { // ROOM_CREATED
                if (len >= 4) {
                    int id = homura::ReadBEI32(payload);
                    ServerResetP2PState(true);
                    mServerCreatePending = false;
                    mServerHostProbeDone = false;
                    mServerGuestProbeDone = false;
                    mServerHosting = true;
                    mServerJoined = false;
                    mServerSpectating = false;
                    mServerHostHasGuest = false;
                    mServerHostSpectateAllowed = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mHostAllowSpectate);
                    mServerJoinedSpectateAllowed = false;
                    mServerHostForceRelay = false;
                    mServerClientWantStart = false;
                    mServerAskedWantStart = false;
                    mServerHostedRoomId = id;
                    netplay::MetricsSetRoomId(id);
                    netplay::MetricsResetSettlementEvents();
                    mServerJoinedRoomId = 0;
                    mServerJoinedRoomName[0] = '\0';
                    mServerRoomCount = 0;
                    mSelectedRoomIndex_Server = 0;
                    mServerRoomPage = 0;
                    gSecondPlayerName[0] = '\0';
                    mServerSpectatorCount = 0;
                    std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));
                    if (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) {
                        std::strncpy(mServerHostedRoomName, mApp->mPlayerInfo->mName, sizeof(mServerHostedRoomName) - 1);
                        mServerHostedRoomName[sizeof(mServerHostedRoomName) - 1] = '\0';
                    } else {
                        mServerHostedRoomName[0] = '\0';
                    }
                    // Relay rooms do not use the LAN player-name handshake.
                    // Keep the host name available to spectators and replays.
                    std::strncpy(gServerHostName, mServerHostedRoomName, sizeof(gServerHostName) - 1);
                    gServerHostName[sizeof(gServerHostName) - 1] = '\0';
                    mServerStatusText = TodStringTranslate("[STATUS_ROOM_CREATED]");
                    if (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mHostAllowSpectate) {
                        ServerSendSetSpectate(true);
                    }
                }
                break;
            }
            case 0x82: { // ROOM_LIST
                // payload: [count:1] + count*([roomId:4][flags:1][version:4][nameLen:1][nameBytes])
                if (mServerQueryPending) {
                    mServerLatencyMs = std::max(0, Sexy::GetTickCount() - mServerQuerySentTick);
                    mServerQueryPending = false;
                }
                if (mServerHosting || mServerJoined || mServerSpectating || mServerCreatePending) {
                    mServerRoomCount = 0;
                    mSelectedRoomIndex_Server = 0;
                    mServerRoomPage = 0;
                    break;
                }
                const int selectedRoomId = mServerRoomCount > 0 ? mServerRooms[mSelectedRoomIndex_Server].roomId : 0;
                mServerRoomCount = 0;
                bool foundCurrentRoomProbe = false;
                if (len < 1)
                    break;
                int count = payload[0] & 0xFF;
                int off = 1;

                for (int i = 0; i < count && mServerRoomCount < 255; i++) {
                    if (off + 10 > (int)len)
                        break;
                    int id = homura::ReadBEI32(payload + off);
                    off += 4;
                    int flags = payload[off++] & 0xFF;
                    int version = homura::ReadBEI32(payload + off);
                    off += 4;
                    int nameLen = payload[off++] & 0xFF;
                    if (off + nameLen > (int)len)
                        break;

                    const std::string_view wireRoomName(reinterpret_cast<const char *>(payload + off), nameLen);
                    off += nameLen;
                    std::string_view displayRoomName;
                    if (!GetServerRoomDisplayName(wireRoomName, mIsCoopLobby, displayRoomName)) {
                        continue;
                    }

                    ServerRoomItem &it = mServerRooms[mServerRoomCount++];
                    it.roomId = id;
                    it.protocolVersion = version;
                    it.full = (flags & 1) != 0;
                    it.gaming = (flags & 2) != 0;
                    it.hostProbeDone = (flags & 4) != 0;
                    it.guestProbeDone = (flags & 8) != 0;
                    it.spectateAllowed = (flags & 16) != 0;
                    it.forceRelay = (flags & 32) != 0;
                    it.reconnectPending = (flags & 64) != 0;
                    it.reconnectBusy = (flags & 128) != 0;
                    std::memset(it.name, 0, sizeof(it.name));
                    int cp = (int)displayRoomName.size();
                    if (cp > (int)sizeof(it.name) - 1)
                        cp = (int)sizeof(it.name) - 1;
                    std::memcpy(it.name, displayRoomName.data(), cp);

                    const bool inCurrentHostRoom = mServerHosting && id == mServerHostedRoomId;
                    const bool inCurrentGuestRoom = (mServerJoined || mServerSpectating) && id == mServerJoinedRoomId;
                    if (inCurrentHostRoom || inCurrentGuestRoom) {
                        mServerHostProbeDone = it.hostProbeDone;
                        mServerGuestProbeDone = it.guestProbeDone;
                        if (inCurrentHostRoom) {
                            mServerHostSpectateAllowed = it.spectateAllowed;
                            mServerHostForceRelay = it.forceRelay;
                        } else {
                            mServerJoinedSpectateAllowed = it.spectateAllowed;
                        }
                        foundCurrentRoomProbe = true;
                    }
                }

                if ((mServerHosting || mServerJoined || mServerSpectating) && !foundCurrentRoomProbe) {
                    mServerHostProbeDone = false;
                    mServerGuestProbeDone = false;
                }

                std::stable_sort(mServerRooms, mServerRooms + mServerRoomCount, [](const ServerRoomItem &a, const ServerRoomItem &b) { return GetRoomSortPriority(a) < GetRoomSortPriority(b); });
                for (int i = 0; i < mServerRoomCount; ++i) {
                    if (mServerRooms[i].roomId == selectedRoomId) {
                        mSelectedRoomIndex_Server = i;
                        break;
                    }
                }

                if (mSelectedRoomIndex_Server < 0)
                    mSelectedRoomIndex_Server = 0;
                if (mSelectedRoomIndex_Server >= mServerRoomCount)
                    mSelectedRoomIndex_Server = mServerRoomCount - 1;
                if (mSelectedRoomIndex_Server < 0)
                    mSelectedRoomIndex_Server = 0;
                int totalPages = (mServerRoomCount + kServerRoomListPageSize - 1) / kServerRoomListPageSize;
                if (totalPages < 1)
                    totalPages = 1;
                if (mServerRoomPage < 0)
                    mServerRoomPage = 0;
                if (mServerRoomPage >= totalPages)
                    mServerRoomPage = totalPages - 1;
                break;
            }
            case 0x83: { // JOIN_RESULT
                bool ok = (len >= 1 && payload[0] == 1);
                int rid = (len >= 5) ? homura::ReadBEI32(payload + 1) : 0;
                int roomVersion = (len >= 9) ? homura::ReadBEI32(payload + 5) : 0;
                int hostNameLen = (len >= 10) ? (payload[9] & 0xFF) : 0;
                const bool hostNameValid = (len >= 10 && 10 + hostNameLen <= len);
                int joinRole = (len >= 11 + hostNameLen) ? (payload[10 + hostNameLen] & 0xFF) : 0;
                int roomFlags = (len >= 12 + hostNameLen) ? (payload[11 + hostNameLen] & 0xFF) : 0;
                if (roomVersion != 0 && roomVersion != NETPLAY_VERSION) {
                    ok = false;
                }
                if (ok) {
                    ServerResetP2PState(true);
                    mServerCreatePending = false;
                    mServerHostProbeDone = false;
                    mServerGuestProbeDone = false;
                    mServerJoined = (joinRole == 0);
                    mServerSpectating = (joinRole == 1);
                    gIsServerModeSpectator = mServerSpectating;
                    mServerHosting = false;
                    mServerHostedRoomId = 0;
                    mServerJoinedRoomId = rid;
                    netplay::MetricsSetRoomId(rid);
                    netplay::MetricsResetSettlementEvents();
                    mServerHostHasGuest = false;
                    mServerHostSpectateAllowed = false;
                    mServerJoinedSpectateAllowed = false;
                    mServerHostForceRelay = false;
                    mServerClientWantStart = false;
                    mServerAskedWantStart = false;
                    mServerJoinedRoomGaming = (roomFlags & 1) != 0;
                    mServerSpectateReservationActive = false;
                    mServerSpectateReserveTick = 0;
                    mServerSpectateReserveWarnTick = 0;
                    mServerHostedRoomName[0] = '\0';
                    if (hostNameValid) {
                        const std::string_view wireHostName(reinterpret_cast<const char *>(payload + 10), hostNameLen);
                        std::string_view displayHostName;
                        if (!GetServerRoomDisplayName(wireHostName, mIsCoopLobby, displayHostName)) {
                            displayHostName = wireHostName;
                        }
                        int copyLen = (int)displayHostName.size();
                        if (copyLen > (int)sizeof(mServerJoinedRoomName) - 1)
                            copyLen = (int)sizeof(mServerJoinedRoomName) - 1;
                        std::memcpy(mServerJoinedRoomName, displayHostName.data(), copyLen);
                        mServerJoinedRoomName[copyLen] = '\0';
                    }
                    if (mServerJoined) {
                        // Guest join: JOIN_RESULT carries host name.
                        std::strncpy(gServerHostName, mServerJoinedRoomName, sizeof(gServerHostName) - 1);
                        gServerHostName[sizeof(gServerHostName) - 1] = '\0';
                        const char *localName = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) ? mApp->mPlayerInfo->mName : "";
                        std::strncpy(gSecondPlayerName, localName, sizeof(gSecondPlayerName) - 1);
                        gSecondPlayerName[sizeof(gSecondPlayerName) - 1] = '\0';
                        mServerStatusText = TodStringTranslate("[STATUS_JOINED_ROOM]");
                    } else {
                        // Spectator: host name comes from JOIN_RESULT hostName field.
                        std::strncpy(gServerHostName, mServerJoinedRoomName, sizeof(gServerHostName) - 1);
                        gServerHostName[sizeof(gServerHostName) - 1] = '\0';
                        gSecondPlayerName[0] = '\0';
                        mServerStatusText = mServerJoinedRoomGaming ? TodStringTranslate("[ROOM_STARTED]") : TodStringTranslate("[JOINED_AS_SPECTATOR]");
                    }
                    mServerSpectatorCount = 0;
                    std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));

                } else {
                    mServerCreatePending = false;
                    if (roomVersion != 0 && roomVersion != NETPLAY_VERSION) {
                        mServerStatusText = TodStringTranslate("[STATUS_ROOM_VERSION_ERR]");
                    } else {
                        mServerStatusText = TodStringTranslate("[STATUS_JOIN_FAILED]");
                    }
                }
                break;
            }
            case 0x84: { // GUEST_JOINED
                if (len >= 4) {
                    int rid = homura::ReadBEI32(payload);
                    if (mServerHosting && rid == mServerHostedRoomId) {
                        mServerHostHasGuest = true;
                        mServerClientWantStart = false;
                        if (len >= 5) {
                            int guestNameLen = payload[4] & 0xFF;
                            if (5 + guestNameLen <= len) {
                                int copyLen = guestNameLen;
                                if (copyLen > (int)sizeof(gSecondPlayerName) - 1)
                                    copyLen = (int)sizeof(gSecondPlayerName) - 1;
                                std::memcpy(gSecondPlayerName, payload + 5, copyLen);
                                gSecondPlayerName[copyLen] = '\0';
                            }
                        }
                        mServerStatusText = TodStringTranslate("[STATUS_GUEST_JOINED]");
                    } else if (mServerSpectating && rid == mServerJoinedRoomId) {
                        if (len >= 5) {
                            int guestNameLen = payload[4] & 0xFF;
                            if (5 + guestNameLen <= len) {
                                int copyLen = guestNameLen;
                                if (copyLen > (int)sizeof(gSecondPlayerName) - 1)
                                    copyLen = (int)sizeof(gSecondPlayerName) - 1;
                                std::memcpy(gSecondPlayerName, payload + 5, copyLen);
                                gSecondPlayerName[copyLen] = '\0';
                            }
                        }
                    }
                }
                break;
            }
            case 0x87: { // GUEST_LEFT
                if (len >= 4) {
                    int rid = homura::ReadBEI32(payload);
                    if (mServerHosting && rid == mServerHostedRoomId) {
                        mServerHostHasGuest = false;
                        mServerHostForceRelay = false;
                        mServerClientWantStart = false;
                        gSecondPlayerName[0] = '\0';
                        mServerStatusText = TodStringTranslate("[STATUS_GUEST_LEFT]");
                    } else if (mServerSpectating && rid == mServerJoinedRoomId) {
                        gSecondPlayerName[0] = '\0';
                    }
                }
                break;
            }
            case 0x8C: { // ROOM_PROBE_STATE
                if (len >= 6) {
                    int rid = homura::ReadBEI32(payload);
                    bool hostReady = payload[4] != 0;
                    bool guestReady = payload[5] != 0;
                    if ((mServerHosting && rid == mServerHostedRoomId) || (mServerJoined && rid == mServerJoinedRoomId)) {
                        mServerHostProbeDone = hostReady;
                        mServerGuestProbeDone = guestReady;
                    }
                }
                break;
            }
            case 0x8D: { // CLIENT_WANT_START
                if (mServerHosting && mServerHostHasGuest) {
                    mServerClientWantStart = true;
                    mApp->PlaySample(Sexy::SOUND_POTATO_MINE);
                }
                break;
            }
            case 0x8E: { // SPECTATE_STATE
                if (len >= 6) {
                    int rid = homura::ReadBEI32(payload);
                    if (mServerHosting && rid == mServerHostedRoomId) {
                        mServerHostSpectateAllowed = (payload[4] != 0);
                        mServerHostForceRelay = (payload[5] != 0);
                        if (mApp && mApp->mPlayerInfo) {
                            mApp->mPlayerInfo->mHostAllowSpectate = mServerHostSpectateAllowed;
                            mApp->mPlayerInfo->SaveDetails();
                        }
                    } else if (mServerJoined && rid == mServerJoinedRoomId) {
                        mServerJoinedSpectateAllowed = (payload[4] != 0);
                    }
                }
                break;
            }
            case 0x8F: { // SPECTATOR_LIST
                if (len >= 5) {
                    int rid = homura::ReadBEI32(payload);
                    const bool sameRoom = (mServerHosting && rid == mServerHostedRoomId) || ((mServerJoined || mServerSpectating) && rid == mServerJoinedRoomId);
                    if (sameRoom) {
                        int cnt = payload[4] & 0xFF;
                        int off = 5;
                        mServerSpectatorCount = cnt;
                        std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));
                        for (int i = 0; i < cnt && i < kMaxSpectatorNamesShown && off < len; ++i) {
                            int nlen = payload[off++] & 0xFF;
                            if (off + nlen > len) {
                                break;
                            }
                            int copyLen = nlen;
                            if (copyLen > (int)sizeof(mServerSpectatorNames[i]) - 1) {
                                copyLen = (int)sizeof(mServerSpectatorNames[i]) - 1;
                            }
                            if (copyLen > 0) {
                                std::memcpy(mServerSpectatorNames[i], payload + off, copyLen);
                            }
                            mServerSpectatorNames[i][copyLen] = '\0';
                            off += nlen;
                        }
                    }
                }
                break;
            }
            case 0x90: { // RESERVE_SPECTATE_ACK
                if (len >= 11) {
                    int rid = homura::ReadBEI32(payload);
                    bool reserve = payload[4] != 0;
                    [[maybe_unused]] bool relayMode = payload[5] != 0;
                    std::uint32_t relayEpoch = (std::uint32_t)homura::ReadBEI32(payload + 7);
                    if (mServerSpectating && rid == mServerJoinedRoomId) {
                        mServerSpectateReservationActive = reserve;
                        mServerRelayEpoch = relayEpoch;
                        mServerSpectateReserveTick = 0;
                        mServerSpectateReserveWarnTick = 0;
                        mServerStatusText = TodStringTranslate(reserve ? "[LIVE_SPECTATE_LOADING]" : "[LIVE_SPECTATE_WAIT_NEXT]");
                    }
                }
                break;
            }
            case 0x89: { // P2P_READY
                if (len >= 2) {
                    mServerP2PLocalPort = homura::ReadBEU16(payload);

                    LOG_DEBUG("[P2P_READY] recv len={} localPort={}", len, mServerP2PLocalPort);

                    if (len >= 8) {
                        mServerP2PProbePort = homura::ReadBEU16(payload + 2);
                        if (len >= 10) {
                            mServerP2PProbePort2 = homura::ReadBEU16(payload + 4);
                            mServerP2PProbeToken = (uint32_t)homura::ReadBEI32(payload + 6);
                        } else {
                            mServerP2PProbePort2 = mServerP2PProbePort;
                            mServerP2PProbeToken = (uint32_t)homura::ReadBEI32(payload + 4);
                        }
                        mServerP2PProbeDone = false;

                        LOG_DEBUG("[P2P_READY] probePort1={} probePort2={} token={} localPort={}", mServerP2PProbePort, mServerP2PProbePort2, mServerP2PProbeToken, mServerP2PLocalPort);

                        const bool started = ServerSendP2PProbe();
                        LOG_DEBUG("[P2P_READY] probe started={} probeDone={}", started, mServerP2PProbeDone);
                    }

                    if (!mServerGameStarting) {
                        if (mServerP2PProbeDone) {
                            mServerP2PStatusText = StrFormat("P2P: server registered %d, probe complete", mServerP2PLocalPort);
                        } else {
                            mServerP2PStatusText = StrFormat("P2P: server accepted local port %d", mServerP2PLocalPort);
                        }
                    }
                }
                break;
            }
            case 0x88: { // P2P_INFO
                LOG_DEBUG(
                    "[P2P_INFO] role={} hosting={} localPort={} probeDone={} rawLen={}", mServerHosting ? "host" : "guest", (int)mServerHosting, mServerP2PLocalPort, (int)mServerP2PProbeDone, len);
                ServerHandleP2PInfo(payload, len);
                LOG_DEBUG("[P2P_INFO] parsed roomId={} peer={}:{} timeoutSec={} status='{}'",
                          mServerP2PTargetRoomId,
                          mServerP2PPeerIp,
                          mServerP2PPeerPort,
                          mServerP2PTimeoutSec,
                          mServerP2PStatusText.c_str());
                break;
            }
            case 0x8A: { // P2P_DONE
                mServerP2PDoneReceived = true;
                mServerGameStartingTick = 0;
                mServerStatusText = TodStringTranslate("[STATUS_BATTLE_BEGIN]");
                if (mServerP2PPendingSock >= 0) {
                    mServerP2PStatusText = "P2P: server confirmed direct channel";
                    ServerAdoptP2PSocket();
                    return;
                }
                mServerP2PStatusText = "P2P: confirmed by server, waiting local socket";
                break;
            }
            case 0x86: { // ROOM_EXITED
                // 不管 host/guest 哪边退出成功，回到空闲
                mServerHosting = false;
                mServerJoined = false;
                mServerSpectating = false;
                gIsServerModeSpectator = false;
                mServerCreatePending = false;
                mServerHostProbeDone = false;
                mServerGuestProbeDone = false;
                mServerHostHasGuest = false;
                mServerHostSpectateAllowed = false;
                mServerJoinedSpectateAllowed = false;
                mServerHostForceRelay = false;
                mServerClientWantStart = false;
                mServerAskedWantStart = false;
                mServerJoinedRoomGaming = false;
                mServerSpectateReservationActive = false;
                mServerSpectateReserveTick = 0;
                mServerSpectateReserveWarnTick = 0;
                mServerGameStartingTick = 0;
                mServerHostedRoomId = 0;
                mServerJoinedRoomId = 0;
                mServerHostedRoomName[0] = '\0';
                mServerJoinedRoomName[0] = '\0';
                mServerSpectatorCount = 0;
                std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));
                gSecondPlayerName[0] = '\0';
                gServerHostName[0] = '\0';
                ServerResetP2PState(true);

                mServerStatusText = TodStringTranslate("[STATUS_ROOM_EXITED]");

                // 退出后拉一次列表
                ServerSendQuery();
                break;
            }
            case 0x85: { // RELAY_BEGIN
                mServerRelayEpoch = (len >= 4) ? (std::uint32_t)homura::ReadBEI32(payload) : 0;
                mServerStatusText = TodStringTranslate("[STATUS_BATTLE_BEGIN]");
                mServerP2PStatusText = "P2P: relay fallback active";
                mServerP2PFailSent = true;
                mServerP2PTargetRoomId = 0;
                mServerP2PPeerPort = 0;
                mServerP2PPeerIp[0] = '\0';
                CloseSocketFd(mServerP2PConnectingSock);
                CloseSocketFd(mServerP2PPendingSock);
                CloseSocketFd(mServerP2PListenSock, false);
                if (mServerSock < 0) {
                    break;
                }

                // === 交接：把服务器 socket 复用给 MODE2 的全局收发 ===
                // 先清掉 WIFI 的两个 socket，避免 UpdateFrames 同时读两路
                if (gTcpClientSocket >= 0) {
                    close(gTcpClientSocket);
                    gTcpClientSocket = -1;
                }
                if (gTcpServerSocket >= 0) {
                    close(gTcpServerSocket);
                    gTcpServerSocket = -1;
                }
                gTcpConnected = false;
                gTcpConnecting = false;
                ResetVsStreamBuffersForServerMode();

                const bool isRelayPlayer = (mServerHosting || mServerJoined);
                if (isRelayPlayer && mServerRelayEpoch != 0 && !ServerSendRelayReady(mServerRelayEpoch)) {
                    ServerDisconnect("relay ready send fail");
                    break;
                }
                mServerStatusText = TodStringTranslate("[STATUS_RELAY_WAIT_START]");
                mServerP2PStatusText = "P2P: relay fallback active, waiting relay go";
                break;
            }

            case 0x8B: { // RELAY_GO
                const std::uint32_t relayGoEpoch = (len >= 4) ? (std::uint32_t)homura::ReadBEI32(payload) : 0;
                if (mServerRelayEpoch != 0 && relayGoEpoch != 0 && relayGoEpoch != mServerRelayEpoch) {
                    break;
                }

                mServerStatusText = TodStringTranslate("[STATUS_BATTLE_BEGIN]");
                mServerP2PStatusText = "P2P: relay active";

                if (mServerSock < 0) {
                    break;
                }

                if (gTcpClientSocket >= 0) {
                    close(gTcpClientSocket);
                    gTcpClientSocket = -1;
                }
                if (gTcpServerSocket >= 0) {
                    close(gTcpServerSocket);
                    gTcpServerSocket = -1;
                }
                gTcpConnected = false;
                gTcpConnecting = false;
                ResetVsStreamBuffersForServerMode();
                ServerMoveBufferedRelayBytesToVsStream(mServerHosting);

                if (mServerHosting) {
                    gTcpClientSocket = mServerSock;
                    mServerSock = -1;
                } else if (mServerJoined) {
                    gTcpServerSocket = mServerSock;
                    gTcpConnected = true;
                    gTcpConnecting = false;
                    mServerSock = -1;
                } else if (mServerSpectating) {
                    gTcpServerSocket = mServerSock;
                    gTcpConnected = true;
                    gTcpConnecting = false;
                    mServerSock = -1;
                    if (mServerJoinedRoomGaming && mServerSpectateReservationActive) {
                        gIsServerModeNetplay = true;
                        gServerModeTransport = ServerModeTransport::RELAY;
                        gIsServerModeSpectator = true;
                        mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_LOADING]");
                        break;
                    }
                } else {
                    ServerDisconnect("relay role unknown");
                    break;
                }

                gIsServerModeNetplay = true;
                gServerModeTransport = ServerModeTransport::RELAY;
                RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
                return;
            }

            case 0x91: { // RECONNECT_RESULT
                if (!mReconnectPending || len != 13)
                    break;
                if (payload[0] != 0) {
                    mReconnectPending = false;
                    static const char *errors[] = {"",
                                                   "[STATUS_RECONNECT_ROOM_ENDED]",
                                                   "[STATUS_RECONNECT_UNAVAILABLE]",
                                                   "[STATUS_RECONNECT_ID_MISMATCH]",
                                                   "[STATUS_RECONNECT_FAILED]",
                                                   "[STATUS_RECONNECT_VERSION_MISMATCH]",
                                                   "[STATUS_RECONNECT_RETRY_LATER]"};
                    mServerStatusText = TodStringTranslate(payload[0] < std::size(errors) ? errors[payload[0]] : "[STATUS_RECONNECT_FAILED]");
                    break;
                }
                if (homura::ReadBEI32(payload + 1) != mReconnectRoomId) {
                    ServerDisconnect("reconnect room mismatch");
                    break;
                }
                mReconnectEpoch = std::uint32_t(homura::ReadBEI32(payload + 5));
                mReconnectTransfer = std::uint32_t(homura::ReadBEI32(payload + 9));
                mServerJoinedRoomId = mReconnectRoomId;
                mServerJoined = true;
                mServerSpectating = mServerHosting = false;
                uint8_t ready[9]{0x13};
                homura::WriteBEI32(ready + 1, mReconnectEpoch);
                homura::WriteBEI32(ready + 5, mReconnectTransfer);
                if (!SendAll(mServerSock, ready, sizeof(ready))) {
                    ServerDisconnect("reconnect ready failed");
                    mServerStatusText = TodStringTranslate("[STATUS_RECONNECT_FAILED]");
                    break;
                }
                mServerStatusText = TodStringTranslate("[STATUS_RECONNECT_JOINING]");
                break;
            }
            case 0x92: { // RECONNECT_GO: only this new guest switches from framed to raw.
                if (!mReconnectPending || len != 8 || mServerSock < 0 || std::uint32_t(homura::ReadBEI32(payload)) != mReconnectEpoch
                    || std::uint32_t(homura::ReadBEI32(payload + 4)) != mReconnectTransfer)
                    break;
                ResetVsStreamBuffersForServerMode();
                ServerMoveBufferedRelayBytesToVsStream(false);
                gTcpServerSocket = mServerSock;
                mServerSock = -1;
                gTcpConnected = true;
                gTcpConnecting = false;
                gIsServerModeNetplay = true;
                gServerModeTransport = ServerModeTransport::RELAY;
                gIsServerModeSpectator = false;
                mReconnectPending = false;
                netplay::MetricsSetRoomId(mServerJoinedRoomId);
                netplay::reconnect::PrepareClient(mReconnectEpoch, mReconnectTransfer);
                // The snapshot loader closes the lobby, without starting a new Board here.
                return;
            }
            case 0xFF: { // ERROR
                int ec = (len >= 1) ? (payload[0] & 0xFF) : -1;
                mServerCreatePending = false;
                mServerStatusText = StrFormat("Server error code: %d", ec);
                break;
            }
            default:
                break;
        }
    }
}

static bool SendAll(int sock, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(sock, p + off, len - off, 0);
        if (n > 0) {
            off += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
        return false;
    }
    return true;
}

void NetplayLobbyWidget::ServerResetP2PState(bool keepListener) {
    const bool p2pNegotiating = (mServerP2PTargetRoomId != 0 || mServerP2PConnectingSock >= 0 || mServerP2PPendingSock >= 0 || mServerP2PDoneReceived);

    CloseSocketFd(mServerP2PConnectingSock);
    CloseSocketFd(mServerP2PPendingSock);
    mServerP2PPendingFromAccept = false;

    if (!keepListener) {
        CloseSocketFd(mServerP2PListenSock, false);
        CloseSocketFd(mServerP2PProbeSock, false);
        mServerP2PLocalPort = 0;
        mServerP2PNatSent = false;
        mServerP2PListenerFailed = false;
        mServerP2PProbePort = 0;
        mServerP2PProbePort2 = 0;
        mServerP2PProbeToken = 0;
        mServerP2PProbeDone = false;
        mServerP2PProbeActive = false;
        mServerP2PProbeSocketConnected = false;
        mServerP2PProbeTargetOk[0] = false;
        mServerP2PProbeTargetOk[1] = false;
        mServerP2PProbeAttempt = 0;
        mServerP2PProbeTargetIndex = 0;
        mServerP2PProbeStartTick = 0;
        mServerP2PProbeTokenBytesSent = 0;
    }

    mServerP2POkSent = false;
    mServerP2PFailSent = false;
    mServerP2PDoneReceived = false;
    mServerGameStarting = false;
    mServerRelayEpoch = 0;
    mServerP2PDeadlineTick = 0;
    mServerP2PNextRetryTick = 0;
    mServerP2PTargetRoomId = 0;
    mServerP2PPeerPort = 0;
    mServerP2PTimeoutSec = 0;
    mServerP2PPeerIp[0] = '\0';

    if (mServerP2PListenSock >= 0 && p2pNegotiating) {
        mServerP2PStatusText = mServerP2PNatSent ? StrFormat("P2P: local port %d registered", mServerP2PLocalPort) : StrFormat("P2P: listener ready on %d", mServerP2PLocalPort);
    } else if (mServerP2PListenerFailed) {
        mServerP2PStatusText = "P2P: listener unavailable, relay only";
    } else {
        mServerP2PStatusText = "P2P: idle";
    }
}

bool NetplayLobbyWidget::ServerOpenP2PListener() {
    if (mServerP2PListenSock >= 0) {
        LOG_DEBUG("[P2P_LISTEN] already open fd={} localPort={}", mServerP2PListenSock, mServerP2PLocalPort);
        return true;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        mServerP2PListenerFailed = true;
        mServerP2PStatusText = "P2P: listener socket failed";
        LOG_ERROR("[P2P_LISTEN] socket failed errno={}", errno);
        return false;
    }

    EnableReuseOptions(sock);

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int preferredPort = (mApp && mApp->mPlayerInfo) ? mApp->mPlayerInfo->mVSRoomPort : 0;

    sockaddr_in sa{
        .sin_family = AF_INET,
        .sin_port = htons(preferredPort),
        .sin_addr{.s_addr = INADDR_ANY},
    };

    LOG_DEBUG("[P2P_LISTEN] try bind preferredPort={}", preferredPort);

    bool bound = bind(sock, (sockaddr *)&sa, sizeof(sa)) == 0;
    if (!bound) {
        LOG_WARN("[P2P_LISTEN] bind preferredPort={} failed errno={}", preferredPort, errno);
    }

    if (!bound && ntohs(sa.sin_port) != 0) {
        sa.sin_port = 0;
        LOG_DEBUG("[P2P_LISTEN] fallback bind ephemeral");
        bound = bind(sock, (sockaddr *)&sa, sizeof(sa)) == 0;
        if (!bound) {
            LOG_ERROR("[P2P_LISTEN] fallback bind failed errno={}", errno);
        }
    }

    if (!bound || listen(sock, 1) < 0) {
        LOG_ERROR("[P2P_LISTEN] listen/bind failed errno={}", errno);
        CloseSocketFd(sock, false);
        mServerP2PListenerFailed = true;
        mServerP2PStatusText = "P2P: listener bind failed";
        return false;
    }

    socklen_t salen = sizeof(sa);
    getsockname(sock, (sockaddr *)&sa, &salen);

    mServerP2PListenSock = sock;
    mServerP2PLocalPort = ntohs(sa.sin_port);
    mServerP2PListenerFailed = false;
    mServerP2PStatusText = StrFormat("P2P: listener ready on %d", mServerP2PLocalPort);

    LOG_DEBUG("[P2P_LISTEN] ready fd={} localPort={} preferredPort={}", mServerP2PListenSock, mServerP2PLocalPort, preferredPort);
    return true;
}

bool NetplayLobbyWidget::ServerSendNatPort() {
    if (mServerSock < 0 || mServerP2PLocalPort <= 0) {
        LOG_WARN("[P2P_NAT_PORT] skip serverSock={} localPort={}", mServerSock, mServerP2PLocalPort);
        return false;
    }

    uint8_t buf[3];
    buf[0] = 0x08;
    homura::WriteBEU16(buf + 1, (uint16_t)mServerP2PLocalPort);

    LOG_DEBUG("[P2P_NAT_PORT] send localPort={} server={}:{}", mServerP2PLocalPort, mServerIp, mServerPort);

    if (!SendAll(mServerSock, buf, sizeof(buf))) {
        LOG_ERROR("[P2P_NAT_PORT] send failed localPort={}", mServerP2PLocalPort);
        ServerDisconnect("nat port send fail");
        return false;
    }

    mServerP2PNatSent = true;
    if (!mServerGameStarting) {
        mServerP2PStatusText = StrFormat("P2P: local port %d sent to server", mServerP2PLocalPort);
    }
    return true;
}

bool NetplayLobbyWidget::ServerSendP2PProbe() {
    if (mServerP2PProbePort2 <= 0) {
        mServerP2PProbePort2 = mServerP2PProbePort;
    }
    if (mServerP2PLocalPort <= 0 || mServerP2PProbePort <= 0 || mServerP2PProbePort2 <= 0) {
        return false;
    }

    CloseSocketFd(mServerP2PProbeSock, false);
    mServerP2PProbeDone = false;
    mServerP2PProbeActive = true;
    mServerP2PProbeSocketConnected = false;
    mServerP2PProbeTargetOk[0] = false;
    mServerP2PProbeTargetOk[1] = false;
    mServerP2PProbeAttempt = 1;
    mServerP2PProbeTargetIndex = 0;
    mServerP2PProbeStartTick = 0;
    mServerP2PProbeTokenBytesSent = 0;
    return true;
}

bool NetplayLobbyWidget::ServerStartP2PProbeTarget() {
    const int targetPort = mServerP2PProbeTargetIndex == 0 ? mServerP2PProbePort : mServerP2PProbePort2;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    EnableReuseOptions(sock);
    ConfigureTcpSocket(sock);
    if (!BindSocketToAnyPort(sock, mServerP2PLocalPort)) {
        CloseSocketFd(sock, false);
        return false;
    }

    sockaddr_in probeSa{
        .sin_family = AF_INET,
        .sin_port = htons((uint16_t)targetPort),
    };
    if (inet_pton(AF_INET, mServerIp, &probeSa.sin_addr) != 1) {
        CloseSocketFd(sock, false);
        return false;
    }

    const int ret = connect(sock, (sockaddr *)&probeSa, sizeof(probeSa));
    if (ret != 0 && errno != EINPROGRESS) {
        CloseSocketFd(sock, false);
        return false;
    }

    mServerP2PProbeSock = sock;
    mServerP2PProbeSocketConnected = ret == 0;
    mServerP2PProbeStartTick = Sexy::GetTickCount();
    mServerP2PProbeTokenBytesSent = 0;
    LOG_DEBUG("[P2P_PROBE] attempt={} target={} port={} started", mServerP2PProbeAttempt, mServerP2PProbeTargetIndex, targetPort);
    return true;
}

void NetplayLobbyWidget::ServerAdvanceP2PProbe(bool success) {
    CloseSocketFd(mServerP2PProbeSock, false);
    mServerP2PProbeSocketConnected = false;
    mServerP2PProbeTokenBytesSent = 0;
    mServerP2PProbeTargetOk[mServerP2PProbeTargetIndex] = success;

    LOG_DEBUG("[P2P_PROBE] attempt={} target={} success={}", mServerP2PProbeAttempt, mServerP2PProbeTargetIndex, success);
    if (++mServerP2PProbeTargetIndex < 2) {
        return;
    }

    if (mServerP2PProbeTargetOk[0] && mServerP2PProbeTargetOk[1]) {
        mServerP2PProbeDone = true;
        mServerP2PProbeActive = false;
        if (!mServerGameStarting) {
            mServerP2PStatusText = StrFormat("P2P: server registered %d, probe complete", mServerP2PLocalPort);
        }
        return;
    }

    if (++mServerP2PProbeAttempt > kServerP2PProbeAttempts) {
        mServerP2PProbeActive = false;
        LOG_DEBUG("[P2P_PROBE] exhausted all attempts");
        return;
    }

    mServerP2PProbeTargetOk[0] = false;
    mServerP2PProbeTargetOk[1] = false;
    mServerP2PProbeTargetIndex = 0;
}

void NetplayLobbyWidget::ServerUpdateP2PProbe() {
    if (!mServerP2PProbeActive) {
        return;
    }
    if (mServerP2PProbeSock < 0) {
        if (!ServerStartP2PProbeTarget()) {
            ServerAdvanceP2PProbe(false);
        }
        return;
    }

    if (Sexy::GetTickCount() - mServerP2PProbeStartTick >= kServerP2PProbeTimeoutMs) {
        ServerAdvanceP2PProbe(false);
        return;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(mServerP2PProbeSock, &wfds);
    timeval tv{0, 0};
    const int sel = select(mServerP2PProbeSock + 1, nullptr, &wfds, nullptr, &tv);
    if (sel < 0) {
        ServerAdvanceP2PProbe(false);
        return;
    }
    if (sel == 0 || !FD_ISSET(mServerP2PProbeSock, &wfds)) {
        return;
    }

    if (!mServerP2PProbeSocketConnected) {
        int err = 0;
        socklen_t errLen = sizeof(err);
        if (getsockopt(mServerP2PProbeSock, SOL_SOCKET, SO_ERROR, &err, &errLen) != 0 || err != 0) {
            ServerAdvanceP2PProbe(false);
            return;
        }
        mServerP2PProbeSocketConnected = true;
    }

    uint8_t tokenBuf[4];
    homura::WriteBEI32(tokenBuf, (int32_t)mServerP2PProbeToken);
    const ssize_t sent = send(mServerP2PProbeSock, tokenBuf + mServerP2PProbeTokenBytesSent, sizeof(tokenBuf) - (size_t)mServerP2PProbeTokenBytesSent, 0);
    if (sent > 0) {
        mServerP2PProbeTokenBytesSent += (int)sent;
        if (mServerP2PProbeTokenBytesSent == (int)sizeof(tokenBuf)) {
            ServerAdvanceP2PProbe(true);
        }
        return;
    }
    if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    ServerAdvanceP2PProbe(false);
}

void NetplayLobbyWidget::ServerHandleP2PInfo(const uint8_t *payload, uint16_t len) {
    ServerResetP2PState(true);
    mServerGameStarting = true;

    if (len < 8) {
        mServerP2PFailSent = true;
        mServerP2PStatusText = "P2P: malformed peer info, waiting relay";
        ServerSendU8(0x0A);
        return;
    }

    int off = 0;
    mServerP2PTargetRoomId = homura::ReadBEI32(payload + off);
    off += 4;

    int ipLen = payload[off++] & 0xFF;
    if (off + ipLen + 3 > (int)len || ipLen <= 0 || ipLen >= INET_ADDRSTRLEN) {
        mServerP2PFailSent = true;
        mServerP2PStatusText = "P2P: invalid peer endpoint, waiting relay";
        ServerSendU8(0x0A);
        return;
    }

    std::memcpy(mServerP2PPeerIp, payload + off, ipLen);
    mServerP2PPeerIp[ipLen] = '\0';
    off += ipLen;

    mServerP2PPeerPort = homura::ReadBEU16(payload + off);
    off += 2;

    mServerP2PTimeoutSec = payload[off] > 0 ? (payload[off] & 0xFF) : 5;
    mServerP2PDeadlineTick = mServerP2PTick + mServerP2PTimeoutSec * 100;
    mServerP2PNextRetryTick = mServerP2PTick;
    if (mServerHosting) {
        mServerP2PStatusText = StrFormat("P2P: waiting inbound from %s:%d", mServerP2PPeerIp, mServerP2PPeerPort);
    } else {
        mServerP2PStatusText = StrFormat("P2P: trying %s:%d", mServerP2PPeerIp, mServerP2PPeerPort);
    }
}

void NetplayLobbyWidget::ServerAdoptP2PSocket() {
    if (mServerP2PPendingSock < 0) {
        return;
    }

    if (gTcpClientSocket >= 0) {
        close(gTcpClientSocket);
        gTcpClientSocket = -1;
    }
    if (gTcpServerSocket >= 0) {
        close(gTcpServerSocket);
        gTcpServerSocket = -1;
    }
    gTcpConnected = false;
    gTcpConnecting = false;
    ResetVsStreamBuffersForServerMode();

    int directSock = mServerP2PPendingSock;
    mServerP2PPendingSock = -1;
    mServerP2PPendingFromAccept = false;
    CloseSocketFd(mServerP2PConnectingSock);
    CloseSocketFd(mServerP2PListenSock, false);
    CloseSocketFd(mServerSock);

    // Once the match is handed off to a direct P2P socket, the lobby server is no longer active.
    mServerConnected = false;
    mServerConnecting = false;
    mServerP2PStatusText = "P2P: direct channel active";
    gIsServerModeNetplay = true;
    gServerModeTransport = ServerModeTransport::P2P;

    if (mServerHosting) {
        gTcpClientSocket = directSock;
    } else if (mServerJoined) {
        gTcpServerSocket = directSock;
        gTcpConnected = true;
        gTcpConnecting = false;
    } else {
        CloseSocketFd(directSock);
        ServerDisconnect("p2p role unknown");
        return;
    }

    RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
}

void NetplayLobbyWidget::ServerUpdateP2P() {
    ++mServerP2PTick;

    if (mServerP2PListenSock >= 0) {
        sockaddr_in peerAddr{};
        socklen_t peerLen = sizeof(peerAddr);
        int accepted = accept(mServerP2PListenSock, (sockaddr *)&peerAddr, &peerLen);
        if (accepted >= 0) {
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &peerAddr.sin_addr, ip, sizeof(ip));
            LOG_DEBUG("[P2P_ACCEPT] role={} accepted from {}:{} localPort={}", mServerHosting ? "host" : "guest", ip, ntohs(peerAddr.sin_port), mServerP2PLocalPort);
            ConfigureTcpSocket(accepted);
            if (mServerP2PTargetRoomId == 0 || mServerP2PFailSent) {
                CloseSocketFd(accepted);
            } else if (mServerP2PPendingSock < 0 || !mServerP2PPendingFromAccept) {
                if (mServerP2PPendingSock >= 0) {
                    CloseSocketFd(mServerP2PPendingSock);
                }
                mServerP2PPendingSock = accepted;
                mServerP2PPendingFromAccept = true;
                if (!mServerP2POkSent) {
                    if (ServerSendU8(0x09)) {
                        mServerP2POkSent = true;
                        mServerP2PStatusText = "P2P: inbound direct ready, waiting confirm";
                    } else {
                        CloseSocketFd(mServerP2PPendingSock);
                        ServerDisconnect("p2p ok send fail");
                        return;
                    }
                }
            } else {
                CloseSocketFd(accepted);
            }
        }
    }

    if (mServerP2PConnectingSock >= 0) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(mServerP2PConnectingSock, &wfds);
        timeval tv{0, 0};
        int r = select(mServerP2PConnectingSock + 1, nullptr, &wfds, nullptr, &tv);
        if (r > 0 && FD_ISSET(mServerP2PConnectingSock, &wfds)) {
            int err = 0;
            socklen_t elen = sizeof(err);
            getsockopt(mServerP2PConnectingSock, SOL_SOCKET, SO_ERROR, &err, &elen);
            if (err == 0) {
                if (!mServerP2PFailSent && mServerP2PPendingSock < 0) {
                    mServerP2PPendingSock = mServerP2PConnectingSock;
                    mServerP2PConnectingSock = -1;
                    mServerP2PPendingFromAccept = false;
                    if (!mServerP2POkSent) {
                        if (ServerSendU8(0x09)) {
                            mServerP2POkSent = true;
                            mServerP2PStatusText = "P2P: outbound direct ready, waiting confirm";
                        } else {
                            CloseSocketFd(mServerP2PPendingSock);
                            ServerDisconnect("p2p ok send fail");
                            return;
                        }
                    }
                } else {
                    CloseSocketFd(mServerP2PConnectingSock);
                }
            } else {
                CloseSocketFd(mServerP2PConnectingSock);
                mServerP2PNextRetryTick = mServerP2PTick + kServerP2PConnectRetryTicks;
            }
        }
    }

    if (mServerP2PDoneReceived && mServerP2PPendingSock >= 0) {
        ServerAdoptP2PSocket();
        return;
    }

    if (mServerP2PFailSent || mServerP2POkSent || mServerP2PTargetRoomId == 0) {
        return;
    }

    if (mServerP2PDeadlineTick > 0 && mServerP2PTick >= mServerP2PDeadlineTick) {
        CloseSocketFd(mServerP2PConnectingSock);
        mServerP2PFailSent = true;
        mServerP2PStatusText = "P2P: direct timeout, waiting relay";
        if (!ServerSendU8(0x0A)) {
            ServerDisconnect("p2p fail send fail");
        }
        return;
    }

    if (mServerP2PConnectingSock >= 0 || mServerP2PTick < mServerP2PNextRetryTick) {
        return;
    }

    in_addr addr{};
    if (inet_pton(AF_INET, mServerP2PPeerIp, &addr) != 1 || mServerP2PPeerPort <= 0) {
        mServerP2PFailSent = true;
        mServerP2PStatusText = "P2P: invalid peer address, waiting relay";
        if (!ServerSendU8(0x0A)) {
            ServerDisconnect("p2p invalid peer");
        }
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        mServerP2PNextRetryTick = mServerP2PTick + kServerP2PConnectRetryTicks;
        return;
    }

    EnableReuseOptions(sock);
    if (!BindSocketToAnyPort(sock, mServerP2PLocalPort)) {
        CloseSocketFd(sock, false);
        mServerP2PStatusText = "P2P: same-port bind failed, waiting relay";
        mServerP2PNextRetryTick = mServerP2PTick + kServerP2PConnectRetryTicks;
        return;
    }

    ConfigureTcpSocket(sock);

    sockaddr_in peerSa{
        .sin_family = AF_INET,
        .sin_port = htons((uint16_t)mServerP2PPeerPort),
        .sin_addr = addr,
    };

    LOG_DEBUG("[P2P_DIRECT_TRY] role={} peer={}:{} localPort={} tick={} deadline={} nextRetry={}",
              mServerHosting ? "host" : "guest",
              mServerP2PPeerIp,
              mServerP2PPeerPort,
              mServerP2PLocalPort,
              mServerP2PTick,
              mServerP2PDeadlineTick,
              mServerP2PNextRetryTick);

    int ret = connect(sock, (sockaddr *)&peerSa, sizeof(peerSa));
    LOG_DEBUG("[P2P_DIRECT_CONNECT] ret={} errno={}", ret, errno);
    if (ret == 0) {
        mServerP2PPendingSock = sock;
        mServerP2PPendingFromAccept = false;
        if (!ServerSendU8(0x09)) {
            CloseSocketFd(mServerP2PPendingSock);
            ServerDisconnect("p2p ok send fail");
            return;
        }
        mServerP2POkSent = true;
        mServerP2PStatusText = "P2P: outbound direct ready, waiting confirm";
    } else if (errno == EINPROGRESS) {
        mServerP2PConnectingSock = sock;
        mServerP2PNextRetryTick = mServerP2PTick + kServerP2PConnectRetryTicks;
    } else {
        CloseSocketFd(sock, false);
        mServerP2PNextRetryTick = mServerP2PTick + kServerP2PConnectRetryTicks;
    }
}

void NetplayLobbyWidget::DrawServerP2PStatus(Sexy::Graphics *g, int x, int y) {
    if (mServerP2PListenSock >= 0) {
        g->DrawString(StrFormat("P2P listener: %d", mServerP2PLocalPort), x, y);
    } else if (mServerP2PListenerFailed) {
        g->DrawString("P2P listener: unavailable", x, y);
    } else {
        g->DrawString("P2P listener: not ready", x, y);
    }

    pvzstl::string statusLine = mServerP2PStatusText.empty() ? "P2P: idle" : mServerP2PStatusText;
    g->DrawString(statusLine, x, y + 35);

    if (mServerP2PPeerPort > 0) {
        g->DrawString(StrFormat("P2P peer: %s:%d room %d", mServerP2PPeerIp, mServerP2PPeerPort, mServerP2PTargetRoomId), x, y + 70);
    }
}

bool NetplayLobbyWidget::ServerSendU8(uint8_t b) const {
    if (mServerSock < 0)
        return false;
    return SendAll(mServerSock, &b, 1);
}

bool NetplayLobbyWidget::ServerSendRelayReady(std::uint32_t relayEpoch) const {
    if (mServerSock < 0 || relayEpoch == 0) {
        return false;
    }

    uint8_t buf[5];
    buf[0] = 0x0B;
    homura::WriteBEI32(buf + 1, (int32_t)relayEpoch);
    return SendAll(mServerSock, buf, sizeof(buf));
}

void NetplayLobbyWidget::ServerSendQuery() {
    // MsgType.QUERY = 0x02
    if (ServerSendU8(0x02)) {
        mServerQuerySentTick = Sexy::GetTickCount();
        mServerQueryPending = true;
    }
}

void NetplayLobbyWidget::ServerSendCreate() {
    if (mServerSock < 0)
        return;
    if (!mServerConnected || mServerConnecting || mServerHosting || mServerJoined || mServerSpectating || mServerCreatePending)
        return;
    if (!mApp || !mApp->mPlayerInfo || !mApp->mPlayerInfo->mName)
        return;

    std::string wireName = mIsCoopLobby ? std::string(kCoopRoomNamePrefix) : std::string(kVsRoomNamePrefix);
    wireName += mApp->mPlayerInfo->mName;
    int nlen = (int)wireName.size();
    if (nlen > 255)
        nlen = 255;

    uint8_t head[2];
    head[0] = 0x01;          // MsgType.CREATE
    head[1] = (uint8_t)nlen; // nameLen

    mServerCreatePending = true;
    mServerRoomCount = 0;
    mSelectedRoomIndex_Server = 0;
    mServerRoomPage = 0;
    if (!SendAll(mServerSock, head, 2)) {
        mServerCreatePending = false;
        mServerStatusText = TodStringTranslate("[STATUS_SEND_CREATE_FAIL]");
        return;
    }
    if (nlen > 0 && !SendAll(mServerSock, wireName.data(), (size_t)nlen)) {
        mServerCreatePending = false;
        mServerStatusText = TodStringTranslate("[STATUS_SEND_CREATE_FAIL]");
        return;
    }
    uint8_t versionBuf[4 + 16];
    homura::WriteBEI32(versionBuf, NETPLAY_VERSION);
    std::memcpy(versionBuf + 4, mApp->mPlayerInfo->mNetplayPlayerId, 16);
    if (!SendAll(mServerSock, versionBuf, sizeof(versionBuf))) {
        mServerCreatePending = false;
        mServerStatusText = TodStringTranslate("[STATUS_SEND_CREATE_FAIL]");
        return;
    }
    mServerStatusText = TodStringTranslate("[STATUS_CREATING_ROOM]");
}


void NetplayLobbyWidget::DrawServerRoomList(Sexy::Graphics *g) {
    if (mServerRoomCount <= 0) {
        TodDrawString(g, TodStringTranslate("[SERVER_NO_ROOMS_TIP]"), 400, kServerRoomListTitleY, g->GetFont(), g->GetColor(), DS_ALIGN_CENTER);
        return;
    }

    int totalPages = (mServerRoomCount + kServerRoomListPageSize - 1) / kServerRoomListPageSize;
    if (totalPages < 1)
        totalPages = 1;
    if (mServerRoomPage < 0)
        mServerRoomPage = 0;
    if (mServerRoomPage >= totalPages)
        mServerRoomPage = totalPages - 1;

    const int pageStart = mServerRoomPage * kServerRoomListPageSize;
    int pageEnd = pageStart + kServerRoomListPageSize;
    if (pageEnd > mServerRoomCount) {
        pageEnd = mServerRoomCount;
    }

    int yPos = kServerRoomListItemStartY;
    Sexy::Color oldColor = g->mColor;

    g->SetFont(Sexy::FONT_DWARVENTODCRAFT18);
    int idx = mSelectedRoomIndex_Server;
    if (idx < 0)
        idx = 0;
    if (idx >= mServerRoomCount)
        idx = mServerRoomCount - 1;
    mSelectedRoomIndex_Server = idx;

    for (int i = pageStart; i < pageEnd; i++) {
        if (i == mSelectedRoomIndex_Server) {
            TodDrawImageScaledF(g, addonImages.leaderboard_selector, 140, yPos - 35, 0.7, 0.7);
            g->SetColor(Sexy::Color(0, 255, 0));
        } else {
            g->SetColor(oldColor);
        }

        const ServerRoomItem &r = mServerRooms[i];
        pvzstl::string tagGaming = TodStringTranslate("[TAG_GAMING]");
        pvzstl::string tagFull = TodStringTranslate("[TAG_FULL]");
        const bool versionLower = (r.protocolVersion != 0 && r.protocolVersion < NETPLAY_VERSION);
        const bool versionHigher = (r.protocolVersion != 0 && r.protocolVersion > NETPLAY_VERSION);
        pvzstl::string tag = r.gaming ? tagGaming : (r.full ? tagFull : "");

        pvzstl::string probeTag = TodStringTranslate(r.hostProbeDone ? "[P2P_READY]" : "[P2P_NOT_READY]");
        tag = tag.empty() ? probeTag : tag + ' ' + probeTag;

        const bool canSpectate = r.spectateAllowed && r.full;
        if (canSpectate) {
            tag = TodStringTranslate("[SPECTATE_AVAILABLE]");
        }
        if (r.gaming && !canSpectate) {
            tag = tag.empty() ? TodStringTranslate("[ROOM_STARTED]") : tag + ' ' + TodStringTranslate("[ROOM_STARTED]");
        }

        if (r.reconnectPending)
            tag = TodStringTranslate(r.reconnectBusy ? "[TAG_RECONNECT_BUSY]" : "[TAG_RECONNECT_WAIT]");

        if (versionLower) {
            tag = TodStringTranslate("[SERVER_ROOM_VERSION_ERROR_LOWER]");
        }
        if (versionHigher) {
            tag = TodStringTranslate("[SERVER_ROOM_VERSION_ERROR_HIGHER]");
        }

        pvzstl::string roomTitle = StrFormat(TodStringTranslate("[SERVER_ROOM_JOINED]").c_str(), r.name);
        pvzstl::string line = tag.empty() ? roomTitle : StrFormat("%s [%s]", roomTitle.c_str(), tag.c_str());
        Mode3DrawListTextWithShadow(g, line, 400, yPos, g->GetFont(), g->GetColor());
        yPos += kServerRoomListLineH;
    }

    if (totalPages > 1) {
        if (mServerRoomPage > 0) {
            g->DrawImageMirror(Sexy::IMAGE_ZEN_NEXTGARDEN, kServerRoomListPrevPageX, kServerRoomListPageArrowY, true);
        }
        if (mServerRoomPage + 1 < totalPages) {
            g->DrawImage(Sexy::IMAGE_ZEN_NEXTGARDEN, kServerRoomListNextPageX, kServerRoomListPageArrowY);
        }
        TodDrawString(g, StrFormat("%d/%d", mServerRoomPage + 1, totalPages), 390, kServerRoomListPageNumberY, g->GetFont(), oldColor, DS_ALIGN_CENTER);
    }

    g->SetColor(oldColor);
}

void NetplayLobbyWidget::ServerSelectRoomByMouse(int x, int y) {
    (void)x;

    if (!mServerConnected) {
        if (mServerConnecting || mServerHosting || mServerJoined) {
            return;
        }

        const bool hasRecentServers = Mode3HasAnyRecentServer(this);
        const int officialStartY = kMode3ServerOfficialItemStartY + (hasRecentServers ? 0 : 50);
        const int officialLineH = kMode3ServerTargetLineH + (hasRecentServers ? 0 : 30);
        int targetIndex = -1;
        const int officialListY = officialStartY - 24;
        if (y >= officialListY && y < officialListY + 2 * officialLineH) {
            targetIndex = (y - officialListY) / officialLineH;
        } else if (Mode3HasAnyRecentServer(this)) {
            const int recentListY = kMode3ServerRecentItemStartY - 24;
            if (y >= recentListY && y < recentListY + kMode3ServerRecentCount * kMode3ServerTargetLineH) {
                targetIndex = 2 + (y - recentListY) / kMode3ServerTargetLineH;
            }
        }
        if (targetIndex < 0) {
            return;
        }
        if (mSelectedRoomIndex_Server != targetIndex) {
            mSelectedRoomIndex_Server = targetIndex;
            mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
        }
        return;
    }

    if (mServerHosting || mServerJoined) {
        return;
    }

    const int listY = kServerRoomListItemStartY - 30;
    const int lineH = kServerRoomListLineH;

    if (mServerRoomCount <= 0) {
        return;
    }

    int totalPages = (mServerRoomCount + kServerRoomListPageSize - 1) / kServerRoomListPageSize;
    if (totalPages < 1)
        totalPages = 1;
    if (mServerRoomPage < 0)
        mServerRoomPage = 0;
    if (mServerRoomPage >= totalPages)
        mServerRoomPage = totalPages - 1;

    const int pageStart = mServerRoomPage * kServerRoomListPageSize;
    int pageCount = mServerRoomCount - pageStart;
    if (pageCount > kServerRoomListPageSize) {
        pageCount = kServerRoomListPageSize;
    }

    if (y >= listY && y < listY + pageCount * lineH) {
        int idx = pageStart + (y - listY) / lineH;
        if (idx >= 0 && idx < mServerRoomCount) {
            if (mSelectedRoomIndex_Server != idx) {
                mSelectedRoomIndex_Server = idx;
                mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
            }
        }
    }
}

void NetplayLobbyWidget::ServerSendJoinSelected() {
    if (mReconnectPending)
        return;
    if (mServerSock < 0)
        return;
    if (mServerRoomCount <= 0)
        return;

    int idx = mSelectedRoomIndex_Server;
    if (idx < 0)
        idx = 0;
    if (idx >= mServerRoomCount)
        idx = mServerRoomCount - 1;
    const ServerRoomItem &room = mServerRooms[idx];
    if (room.protocolVersion != 0 && room.protocolVersion != NETPLAY_VERSION) {
        mServerStatusText = TodStringTranslate("[STATUS_ROOM_VERSION_ERR]");
        return;
    }
    std::strncpy(mServerJoinedRoomName, room.name, sizeof(mServerJoinedRoomName) - 1);
    mServerJoinedRoomName[sizeof(mServerJoinedRoomName) - 1] = '\0';
    int roomId = room.roomId;
    if (room.reconnectPending) {
        if (room.reconnectBusy) {
            mServerStatusText = TodStringTranslate("[STATUS_RECONNECT_BUSY]");
            return;
        }
        uint8_t request[1 + 4 + 4 + 16]{0x12}; // RECONNECT_JOIN
        homura::WriteBEI32(request + 1, roomId);
        homura::WriteBEI32(request + 5, NETPLAY_VERSION);
        std::memcpy(request + 9, mApp->mPlayerInfo->mNetplayPlayerId, 16);
        mReconnectRoomId = roomId;
        mReconnectPending = true;
        mReconnectStartedAt = Sexy::GetTickCount();
        if (!SendAll(mServerSock, request, sizeof(request)))
            ServerDisconnect("reconnect join failed");
        else
            mServerStatusText = TodStringTranslate("[STATUS_RECONNECT_VERIFYING]");
        return;
    }

    const char *playerName = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) ? mApp->mPlayerInfo->mName : "";
    int nameLen = (int)std::strlen(playerName);
    if (nameLen > 255)
        nameLen = 255;

    uint8_t buf[1 + 4 + 4 + 1 + 255 + 16];
    buf[0] = 0x03; // JOIN
    homura::WriteBEI32(buf + 1, roomId);
    homura::WriteBEI32(buf + 5, NETPLAY_VERSION);
    buf[9] = (uint8_t)nameLen;
    if (nameLen > 0) {
        std::memcpy(buf + 10, playerName, nameLen);
    }
    std::memcpy(buf + 10 + nameLen, mApp->mPlayerInfo->mNetplayPlayerId, 16);

    const bool wantSpectate = (room.full && room.spectateAllowed);
    buf[0] = wantSpectate ? 0x0F : 0x03; // JOIN_SPECTATE / JOIN
    if (!SendAll(mServerSock, buf, size_t(10 + nameLen + 16))) {
        mServerStatusText = TodStringTranslate("[STATUS_SEND_JOIN_FAIL]");
        ServerDisconnect("join send fail");
    }
}

void NetplayLobbyWidget::ServerSendSetSpectate(bool allow) {
    if (!mServerHosting || !mServerConnected || mServerConnecting) {
        return;
    }
    uint8_t buf[2];
    buf[0] = 0x0E; // SET_SPECTATE
    buf[1] = allow ? 1 : 0;
    if (!SendAll(mServerSock, buf, sizeof(buf))) {
        mServerStatusText = "set spectate failed";
        ServerDisconnect("set spectate send fail");
    }
}

void NetplayLobbyWidget::ServerSendSwitchRole(bool toSpectator) {
    if (!mServerConnected || mServerConnecting || mServerSock < 0) {
        return;
    }
    if (toSpectator) {
        if (!Mode3CanSwitchGuestToSpectator(this)) {
            return;
        }
    } else {
        if (!Mode3CanSwitchSpectatorToGuest(this)) {
            return;
        }
    }

    uint8_t buf[2];
    buf[0] = 0x10; // SWITCH_ROLE
    buf[1] = toSpectator ? 1 : 0;
    if (!SendAll(mServerSock, buf, sizeof(buf))) {
        mServerStatusText = "switch role failed";
        ServerDisconnect("switch role send fail");
    }
}

void NetplayLobbyWidget::ServerSendReserveSpectate(bool reserve) {
    int sock = mServerSock;
    if (sock < 0 && mServerSpectating && gTcpServerSocket >= 0) {
        sock = gTcpServerSocket;
    }
    if (sock < 0 || !mServerSpectating) {
        return;
    }
    uint8_t buf[2];
    buf[0] = 0x11; // RESERVE_SPECTATE
    buf[1] = reserve ? 1 : 0;
    if (!SendAll(sock, buf, sizeof(buf))) {
        mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_REQUEST_FAILED]");
        return;
    }
    mServerSpectateReserveTick = 0;
    mServerSpectateReserveWarnTick = 0;
}

void NetplayLobbyWidget::ServerSendRejoinRole(bool toSpectator) {
    if (!mServerConnected || mServerConnecting || mServerSock < 0 || mServerJoinedRoomId == 0) {
        mServerStatusText = "switch role unavailable";
        return;
    }
    if (toSpectator) {
        if (!Mode3CanSwitchGuestToSpectator(this)) {
            mServerStatusText = "cannot switch to spectator";
            return;
        }
    } else {
        if (!Mode3CanSwitchSpectatorToGuest(this)) {
            mServerStatusText = "cannot switch to guest";
            return;
        }
    }

    const char *playerName = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) ? mApp->mPlayerInfo->mName : "";
    int nameLen = (int)std::strlen(playerName);
    if (nameLen > 255)
        nameLen = 255;

    uint8_t buf[1 + 4 + 4 + 1 + 255 + 16];
    buf[0] = toSpectator ? 0x0F : 0x03; // JOIN_SPECTATE / JOIN
    homura::WriteBEI32(buf + 1, mServerJoinedRoomId);
    homura::WriteBEI32(buf + 5, NETPLAY_VERSION);
    buf[9] = (uint8_t)nameLen;
    if (nameLen > 0) {
        std::memcpy(buf + 10, playerName, nameLen);
    }
    std::memcpy(buf + 10 + nameLen, mApp->mPlayerInfo->mNetplayPlayerId, 16);

    if (!ServerSendU8(0x07)) { // LEAVE_ROOM
        mServerStatusText = TodStringTranslate("[STATUS_SEND_LEAVE_FAIL]");
        return;
    }
    if (!SendAll(mServerSock, buf, size_t(10 + nameLen + 16))) {
        mServerStatusText = TodStringTranslate("[STATUS_SEND_JOIN_FAIL]");
        return;
    }
    mServerStatusText = toSpectator ? "switching to spectator..." : "switching to guest...";
}


void NetplayLobbyWidget::ServerSendExitRoom() {
    // EXIT_ROOM = 0x06
    if (!ServerSendU8(0x06)) {
        mServerStatusText = TodStringTranslate("[STATUS_SEND_EXIT_FAIL]");
        ServerDisconnect("exit send fail");
    }
}

void NetplayLobbyWidget::ServerSendLeaveRoom() {
    if (mServerSpectating && gTcpServerSocket >= 0) {
        shutdown(gTcpServerSocket, SHUT_RDWR);
        close(gTcpServerSocket);
        gTcpServerSocket = -1;
        gTcpConnected = false;
        ServerOnBorrowedSocketClosed("spectator leave local close");
        return;
    }
    // LEAVE_ROOM = 0x07
    if (!ServerSendU8(0x07)) {
        mServerStatusText = TodStringTranslate("[STATUS_SEND_LEAVE_FAIL]");
        ServerDisconnect("leave send fail");
    }
}

void NetplayLobbyWidget::ServerOnBorrowedSocketClosed(const char *why) {
    ServerDisconnect(why);
    mServerStatusText = TodStringTranslate("[SPECTATE_ROOM_CONNECTION_CLOSED]");
    RefreshButtons();
}

void NetplayLobbyWidget::ServerSendKickGuest() {
    // KICK_GUEST = 0x0C
    if (!mServerHosting || !mServerHostHasGuest) {
        return;
    }
    if (!ServerSendU8(0x0C)) {
        mServerStatusText = "kick guest failed";
        ServerDisconnect("kick send fail");
        return;
    }
    mServerClientWantStart = false;
}

void NetplayLobbyWidget::ServerSendStart() {
    // START = 0x05
    ServerResetP2PState(true);
    mServerGameStarting = true;
    mServerGameStartingTick = 0;
    if (!mServerP2PNatSent) {
        mServerP2PStatusText = "P2P: no local listener, waiting relay";
    } else if (!mServerP2PProbeDone) {
        mServerP2PStatusText = "P2P: start sent, waiting probe/public endpoint";
    } else {
        mServerP2PStatusText = "P2P: start sent, waiting peer info";
    }
    if (!ServerSendU8(0x05)) {
        mServerStatusText = TodStringTranslate("[STATUS_SEND_START_FAIL]");
        ServerDisconnect("start send fail");
    }
}

void NetplayLobbyWidget::ServerSendAskStart() {
    if (!mServerJoined || !mServerConnected || mServerConnecting) {
        return;
    }
    if (!ServerSendU8(0x0D)) {
        mServerStatusText = "ask start failed";
        ServerDisconnect("ask start send fail");
        return;
    }
    mServerAskedWantStart = true;
}


bool NetplayLobbyWidget::ServerConnectFromInput() {
    const std::string input = std::move(gInputString);
    gHasInputContent = false;
    gHasInputContent.notify_one();
    LOG_DEBUG("input: '{}'", input);

    std::string ip;
    int port = 0;
    if (!ParseMode3IpPort(input, ip, port)) {
        mServerStatusText = TodStringTranslate("[STATUS_ADDR_FORMAT_ERROR]");
        return false;
    }

    if (mApp && mApp->mPlayerInfo) {
        const std::string normalizedAddr = ip + ':' + std::to_string(port);
        Mode3RememberRecentServer(mApp->mPlayerInfo, normalizedAddr);
        // The newly entered endpoint is always moved to recent-server slot 0.
        mSelectedServerListIndex = 3;
    }

    return Mode3ConnectToTarget(this, ip, port);
}

void NetplayLobbyWidget::ServerDisconnect([[maybe_unused]] const char *why) {
    const bool hasActiveVsSocket = IsRemoteServer() || IsRemoteClient() || (gTcpServerSocket >= 0);
    CloseSocketFd(mServerSock);
    mReconnectPending = false;
    mReconnectStartedAt = 0;
    mReconnectRoomId = 0;
    mReconnectEpoch = mReconnectTransfer = 0;
    Mode3ResetTargetLatencyProbes(this);
    ServerResetP2PState(false);

    gIsConnectedToServer = false;
    mServerConnecting = false;
    mServerConnected = false;

    mServerHosting = false;
    mServerJoined = false;
    mServerSpectating = false;
    mServerCreatePending = false;
    mServerHostProbeDone = false;
    mServerGuestProbeDone = false;
    mServerHostHasGuest = false;
    mServerHostSpectateAllowed = false;
    mServerJoinedSpectateAllowed = false;
    mServerHostForceRelay = false;
    mServerClientWantStart = false;
    mServerAskedWantStart = false;
    mServerJoinedRoomGaming = false;
    mServerSpectateReservationActive = false;
    mServerHostedRoomId = 0;
    mServerJoinedRoomId = 0;
    mServerLatencyMs = -1;
    mServerQuerySentTick = 0;
    mServerQueryPending = false;
    netplay::MetricsResetSettlementEvents();
    mServerHostedRoomName[0] = '\0';
    mServerJoinedRoomName[0] = '\0';
    mServerSpectatorCount = 0;
    std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));

    mServerRoomCount = 0;
    mSelectedRoomIndex_Server = 0;
    mServerRoomPage = 0;
    mSrvRecvLen = 0;
    mServerP2PTick = 0;
    mServerGameStartingTick = 0;
    mServerSpectateReserveTick = 0;
    mServerSpectateReserveWarnTick = 0;

    if (!hasActiveVsSocket) {
        gSecondPlayerName[0] = '\0';
        gServerHostName[0] = '\0';
        gIsServerModeNetplay = false;

        gServerModeTransport = ServerModeTransport::NONE;
        gIsServerModeSpectator = false;
    }
    mServerStatusText = TodStringTranslate("[STATUS_NOT_CONNECTED]");
}

namespace {
constexpr int kMode3StartTimeoutTicks = 1000;
constexpr int kMode3SpectateReserveWarnTicks = 300;
constexpr int kMode3ServerProbeRefreshTicks = 500;
constexpr int kMode3ServerProbeTimeoutMs = 2500;
struct BroadcastTarget {
    sockaddr_in addr{};
    std::string ifname;
    std::string local_ip;
};

std::vector<BroadcastTarget> gBroadcastTargets;

static bool HasSameBroadcastAddr(const std::vector<BroadcastTarget> &targets, const sockaddr_in &addr) {
    for (const auto &t : targets) {
        if (t.addr.sin_addr.s_addr == addr.sin_addr.s_addr && t.addr.sin_port == addr.sin_port) {
            return true;
        }
    }
    return false;
}

static void PushBroadcastTarget(std::vector<BroadcastTarget> &targets, const sockaddr_in &addr, const char *ifname, const char *local_ip) {
    if (HasSameBroadcastAddr(targets, addr)) {
        return;
    }
    BroadcastTarget t;
    t.addr = addr;
    t.ifname = ifname ? ifname : "";
    t.local_ip = local_ip ? local_ip : "";
    targets.push_back(t);
}


static bool CollectAllBroadcastTargets(std::vector<BroadcastTarget> &out_targets) {
    out_targets.clear();

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    ifconf ifc{};
    char buf[4096]{};
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
        close(fd);
        return false;
    }

    std::vector<BroadcastTarget> wifi_like;
    std::vector<BroadcastTarget> eth_like;
    std::vector<BroadcastTarget> other_like;

    for (ifreq *it = (ifreq *)buf, *end = (ifreq *)(buf + ifc.ifc_len); it < end; ++it) {
        ifreq ifr{};
        strncpy(ifr.ifr_name, it->ifr_name, IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
            continue;
        }
        if ((ifr.ifr_flags & IFF_LOOPBACK) || !(ifr.ifr_flags & IFF_UP)) {
            continue;
        }

        const char *n = ifr.ifr_name;
        if (strncmp(n, "rmnet", 5) == 0 || strncmp(n, "ccmni", 5) == 0 || strncmp(n, "pdp", 3) == 0) {
            continue;
        }

        if (!(ifr.ifr_flags & IFF_BROADCAST)) {
            continue;
        }

        ifreq ifr_address = ifr;
        if (ioctl(fd, SIOCGIFADDR, &ifr_address) < 0) {
            continue;
        }

        if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
            continue;
        }

        sockaddr_in *local = (sockaddr_in *)&ifr_address.ifr_addr;
        sockaddr_in *sin = (sockaddr_in *)&ifr.ifr_broadaddr;
        if (local->sin_family != AF_INET || sin->sin_family != AF_INET || sin->sin_addr.s_addr == 0) {
            continue;
        }

        char local_ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &local->sin_addr, local_ip, sizeof(local_ip));

        sockaddr_in bcast = *sin;
        bcast.sin_family = AF_INET;
        bcast.sin_port = htons(UDP_PORT);

        if (strncasecmp(n, "wlan", 4) == 0 || strncasecmp(n, "ap", 2) == 0 || strncasecmp(n, "en", 2) == 0) {
            PushBroadcastTarget(wifi_like, bcast, n, local_ip);
        } else if (strncasecmp(n, "eth", 3) == 0) {
            PushBroadcastTarget(eth_like, bcast, n, local_ip);
        } else {
            PushBroadcastTarget(other_like, bcast, n, local_ip);
        }
    }

    close(fd);

    out_targets.append_range(wifi_like);
    out_targets.append_range(eth_like);
    out_targets.append_range(other_like);

    if (out_targets.empty()) {
        sockaddr_in fallback{};
        fallback.sin_family = AF_INET;
        fallback.sin_port = htons(UDP_PORT);
        inet_pton(AF_INET, "255.255.255.255", &fallback.sin_addr);
        PushBroadcastTarget(out_targets, fallback, "fallback", "255.255.255.255");
    }

    return !out_targets.empty();
}

[[maybe_unused]] static pvzstl::string MakeRandomPlayerCode() {
    char code[7]{};
    snprintf(code, sizeof(code), "%06d", 100000 + Sexy::Rand(900000));
    return code;
}

static void Mode3StartTargetLatencyProbe(NetplayLobbyWidget *dialog, int targetIndex) {
    if (!dialog || targetIndex < 0 || targetIndex >= kMode3ServerTargetCountMax) {
        return;
    }

    std::string targetAddr;
    const int selectedIndex = dialog->mSelectedRoomIndex_Server;
    dialog->mSelectedRoomIndex_Server = targetIndex;
    const bool hasTarget = Mode3GetSelectedTargetAddr(dialog, targetAddr);
    dialog->mSelectedRoomIndex_Server = selectedIndex;
    if (!hasTarget) {
        dialog->mServerTargetLatencyMs[targetIndex] = -1;
        return;
    }

    std::string ip;
    int port = 0;
    if (!ParseMode3IpPort(targetAddr, ip, port)) {
        dialog->mServerTargetLatencyMs[targetIndex] = -1;
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        dialog->mServerTargetLatencyMs[targetIndex] = -1;
        return;
    }
    ConfigureTcpSocket(sock);

    sockaddr_in sa{
        .sin_family = AF_INET,
        .sin_port = htons(uint16_t(port)),
    };
    if (inet_pton(AF_INET, ip.c_str(), &sa.sin_addr) != 1) {
        CloseSocketFd(sock);
        dialog->mServerTargetLatencyMs[targetIndex] = -1;
        return;
    }

    dialog->mServerTargetProbeStartTick[targetIndex] = Sexy::GetTickCount();
    dialog->mServerTargetProbeSock[targetIndex] = sock;

    const int ret = connect(sock, (sockaddr *)&sa, sizeof(sa));
    if (ret == 0) {
        dialog->mServerTargetLatencyMs[targetIndex] = std::max(0, Sexy::GetTickCount() - dialog->mServerTargetProbeStartTick[targetIndex]);
        CloseSocketFd(dialog->mServerTargetProbeSock[targetIndex]);
        dialog->mServerTargetProbeStartTick[targetIndex] = 0;
        return;
    }

    if (errno != EINPROGRESS) {
        dialog->mServerTargetLatencyMs[targetIndex] = -1;
        CloseSocketFd(dialog->mServerTargetProbeSock[targetIndex]);
        dialog->mServerTargetProbeStartTick[targetIndex] = 0;
    }
}

static void Mode3UpdateTargetLatencyProbes(NetplayLobbyWidget *dialog) {
    if (!dialog) {
        return;
    }
    if (dialog->mUIMode != UIMode::MODE3_SERVER || dialog->mServerConnected || dialog->mServerConnecting) {
        Mode3ResetTargetLatencyProbes(dialog);
        return;
    }

    const int targetCount = Mode3ServerTargetCount(dialog);
    if (targetCount <= 0) {
        Mode3ResetTargetLatencyProbes(dialog);
        return;
    }

    if (dialog->mServerTargetNextRefreshTick <= 0) {
        for (int i = 0; i < kMode3ServerTargetCountMax; ++i) {
            CloseSocketFd(dialog->mServerTargetProbeSock[i]);
            dialog->mServerTargetProbeStartTick[i] = 0;
        }
        for (int i = 0; i < targetCount; ++i) {
            Mode3StartTargetLatencyProbe(dialog, i);
        }
        dialog->mServerTargetNextRefreshTick = kMode3ServerProbeRefreshTicks;
    } else {
        --dialog->mServerTargetNextRefreshTick;
    }

    const int nowTick = Sexy::GetTickCount();
    for (int i = 0; i < targetCount; ++i) {
        const int sock = dialog->mServerTargetProbeSock[i];
        if (sock < 0) {
            continue;
        }

        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        timeval tv{0, 0};
        const int ready = select(sock + 1, nullptr, &wfds, nullptr, &tv);
        if (ready > 0 && FD_ISSET(sock, &wfds)) {
            int err = 0;
            socklen_t errLen = sizeof(err);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errLen);
            dialog->mServerTargetLatencyMs[i] = (err == 0) ? std::max(0, nowTick - dialog->mServerTargetProbeStartTick[i]) : -1;
            CloseSocketFd(dialog->mServerTargetProbeSock[i]);
            dialog->mServerTargetProbeStartTick[i] = 0;
            continue;
        }

        if (dialog->mServerTargetProbeStartTick[i] > 0 && nowTick - dialog->mServerTargetProbeStartTick[i] >= kMode3ServerProbeTimeoutMs) {
            dialog->mServerTargetLatencyMs[i] = -1;
            CloseSocketFd(dialog->mServerTargetProbeSock[i]);
            dialog->mServerTargetProbeStartTick[i] = 0;
        }
    }
}
} // namespace

pvzstl::string GetLocalIpPlayerCode() {
    std::vector<BroadcastTarget> targets;
    if (!CollectAllBroadcastTargets(targets) || targets.empty() || targets.front().ifname == "fallback") {
        return MakeRandomPlayerCode();
    }

    in_addr address{};
    if (inet_pton(AF_INET, targets.front().local_ip.c_str(), &address) != 1) {
        return MakeRandomPlayerCode();
    }

    const uint32_t hostAddress = ntohl(address.s_addr);
    const unsigned int thirdOctet = (hostAddress >> 8U) & 0xFFU;
    const unsigned int fourthOctet = hostAddress & 0xFFU;
    char code[7]{};
    snprintf(code, sizeof(code), "%03u%03u", thirdOctet, fourthOctet);
    return code;
}

bool NetplayLobbyWidget::ManualIpConnect() {
    const std::string input = std::move(gInputString);
    gHasInputContent = false;
    gHasInputContent.notify_one();
    LOG_DEBUG("raw input='{}'", input);

    const size_t colonPos = input.find(':');
    if (colonPos == std::string::npos) {
        LOG_ERROR("No colon in input");
        return false;
    }

    // 校验端口
    const std::string portStr = homura::Trim(std::string_view{input}.substr(colonPos + 1));
    const int port = std::atoi(portStr.c_str());
    if (port < 1 || port > 65535) {
        LOG_ERROR("invalid port: '{}'", portStr);
        return false;
    }

    // 校验 IP
    const std::string ip = homura::Trim(std::string_view{input}.substr(0, colonPos));
    in_addr addr{};
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        LOG_DEBUG("invalid ip '{}'", ip);
        return false;
    }

    // 保存目标
    mManualIp[ip.copy(mManualIp, INET_ADDRSTRLEN - 1)] = '\0';
    mManualPort = port;
    mUseManualTarget = true;
    LOG_DEBUG("target {}:{}", &mManualIp[0], mManualPort);

    // 切换到 joining 状态，重置连接状态（避免旧状态干扰）
    mIsJoiningRoom = true;
    CloseUdpScanSocket();

    mCreateRoomButton->mDisabled = true;
    mJoinRoomButton->SetLabel("[LEAVE_ROOM_BUTTON]");

    mPrimaryActionButton->mDisabled = true;
    mPrimaryActionButton->SetLabel("[PLAY_ONLINE]");

    if (gTcpServerSocket >= 0) {
        shutdown(gTcpServerSocket, SHUT_RDWR);
        close(gTcpServerSocket);
        gTcpServerSocket = -1;
    }
    gTcpConnecting = false;
    gTcpConnected = false;

    // 关闭扫描 socket（避免 scan 模式逻辑干扰）
    CloseUdpScanSocket();

    // （可选）如果你希望这里同步更新按钮状态/文字，也可以放在这里

    return true;
}


void NetplayLobbyWidget::Update() {
    UpdateNetplay();
}

void NetplayLobbyWidget::UpdateNetplay() {
    // =========================================================
    // 1) 统一处理输入框回填（gInputString）
    //    关键点：
    //    - 只在“真的消费了输入”时才清 mInputPurpose
    //    - 若用途/模式不匹配：兜底清掉输入，避免每帧刷屏
    // =========================================================
    if (gHasInputContent) {
        assert(!gInputString.empty());

        // MODE2：WIFI 手动加入指定 IP
        if (mInputPurpose == InputPurpose::LAN_JOIN_MANUAL && mUIMode == UIMode::MODE2_WIFI) {
            mUseManualTarget = true;
            ManualIpConnect(); // 内部会消费 gInputString
            mInputPurpose = InputPurpose::NONE;
            RefreshButtons(); // 状态变化后立即刷新按钮
        }
        // MODE2：WIFI 房主设置房间端口
        else if (mInputPurpose == InputPurpose::HOST_SET_PORT) {
            // 取走输入并清空
            const std::string input = std::move(gInputString);
            gHasInputContent = false;
            gHasInputContent.notify_one();

            // 允许输入 0（随机端口），范围 0~65535
            const int port = std::atoi(input.c_str());
            if (port < 0 || port > 65535) {
                mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[PORT_INVALID_TITLE]", "[PORT_INVALID_DESC]", "[DIALOG_BUTTON_OK]", "", 3);
                mInputPurpose = InputPurpose::NONE;
                return;
            }

            // 保存设置
            mApp->mPlayerInfo->mVSRoomPort = port;
            mApp->mPlayerInfo->SaveDetails();

            mInputPurpose = InputPurpose::NONE;
            // ✅ 关键：重建房间，让 gTcpPort / 广播端口真正改变
            ExitRoom();   // 会关 tcpClient/tcpListen/udpBroadcast
            CreateRoom(); // 你已改为使用 mVSRoomPort bind

            // CreateRoom() 失败时：回到扫描模式避免卡死
            if (!mIsCreatingRoom) {
                InitUdpScanSocket();
                mIsJoiningRoom = false;
            } else {
                // 创建成功：不需要扫描
                CloseUdpScanSocket();
            }
            RefreshButtons();
        }
        // MODE3：连接服务器 IP:PORT
        else if (mInputPurpose == InputPurpose::SERVER_CONNECT_ADDR && mUIMode == UIMode::MODE3_SERVER) {
            ServerConnectFromInput(); // 内部会消费 gInputString
            mInputPurpose = InputPurpose::NONE;
            RefreshButtons(); // 状态变化后立即刷新按钮
        } else {
            // 兜底：收到输入但用途/模式不匹配
            // 防止 gInputString 永远不空导致每帧重复触发/刷日志
            LOG_WARN("[Input] drop input='{}' purpose={} mode={}", gInputString, int(mInputPurpose), int(mUIMode));
            gInputString.clear();
            gHasInputContent = false;
            gHasInputContent.notify_one();
            mInputPurpose = InputPurpose::NONE; // 这里不强制清 mInputPurpose 也行；清掉更安全
        }
    }

    // =========================================================
    // 2) MODE2：WIFI 才跑 UDP 扫描/广播节拍
    // =========================================================
    if (mUIMode == UIMode::MODE2_WIFI) {
        bool inScanMode = (!mIsCreatingRoom && !mIsJoiningRoom);
        if (inScanMode) {
            // 扫描模式下：没房间就禁用“加入房间”
            mJoinRoomButton->mDisabled = (gScannedServerCount == 0);

            // 选中索引修正
            mSelectedServerIndex = std::clamp(mSelectedServerIndex, 0, std::max(0, gScannedServerCount - 1));
        }

        // 创建房间时：开始游戏按钮是否可点
        if (mIsCreatingRoom) {
            mPrimaryActionButton->mDisabled = !IsRemoteServer();
        }

        // UDP 广播/扫描节拍
        gLastBroadcastTime++;
        if (gLastBroadcastTime >= 100) { // ~1秒
            if (mIsCreatingRoom) {
                UdpBroadcastRoom();
            } else if (!mIsJoiningRoom) {
                ScanUdpBroadcastRoom();
            }
        }

        // TCP accept / connect
        if (gTcpListenSocket >= 0) {
            CheckTcpAccept();
        }
        if (mIsJoiningRoom && !IsRemoteClient()) {
            TryTcpConnect();
        }
    }

    // =========================================================
    // 3) MODE3：服务器联机 IO（connect 完成检测 + 收包） + 自动 query
    // =========================================================
    if (mUIMode == UIMode::MODE3_SERVER) {
        if (mReconnectEpoch != 0 && mServerSock < 0 && gTcpServerSocket < 0) {
            // A failed snapshot/timeout closes the borrowed raw socket before
            // the lobby itself closes. Release its stale joined-room state.
            ServerDisconnect("reconnect borrowed socket closed");
            mServerStatusText = TodStringTranslate("[STATUS_RECONNECT_FAILED]");
        }
        if (mServerSpectating && mServerJoinedRoomGaming && mServerSock < 0 && gTcpServerSocket < 0) {
            ServerOnBorrowedSocketClosed("borrowed socket missing in update");
        }
        if (mServerSpectating && mServerJoinedRoomGaming && mServerSpectateReservationActive && mServerSock >= 0 && gTcpServerSocket < 0 && mServerRelayEpoch == 0) {
            ++mServerSpectateReserveTick;
            ++mServerSpectateReserveWarnTick;
            if (mServerSpectateReserveWarnTick >= kMode3SpectateReserveWarnTicks) {
                mServerSpectateReserveWarnTick = 0;
            }
        } else {
            mServerSpectateReserveTick = 0;
            mServerSpectateReserveWarnTick = 0;
        }
        // 网络 IO（你实现：包含 connect 完成检测、收包解析等）
        ServerUpdateIO();
        if (netplay::reconnect::IsFrozen())
            return;
        if (mReconnectPending) {
            // In particular, QUERY must not follow READY onto the raw stream.
            if (std::uint32_t(Sexy::GetTickCount() - mReconnectStartedAt) >= 20000) {
                ServerDisconnect("reconnect lobby handshake timed out");
                mServerStatusText = TodStringTranslate("[STATUS_RECONNECT_FAILED]");
            }
            RefreshButtons();
            return;
        }
        ServerUpdateP2PProbe();
        ServerUpdateP2P();
        Mode3UpdateTargetLatencyProbes(this);
        if (mServerGameStarting) {
            mServerGameStartingTick++;
            if (mServerGameStartingTick >= kMode3StartTimeoutTicks) {
                const bool wasHosting = mServerHosting;
                const bool wasJoinedOrSpectating = mServerJoined || mServerSpectating;
                mServerGameStarting = false;
                mServerGameStartingTick = 0;
                mServerP2PStatusText = "P2P: start timeout";
                mServerStatusText = TodStringTranslate("[STATUS_START_TIMEOUT]");

                // Start timeout always forces local exit from the current room
                // and returns MODE3 back to the room list.
                if (wasHosting) {
                    ServerSendU8(0x06); // EXIT_ROOM
                    ExitRoom();
                } else if (wasJoinedOrSpectating) {
                    ServerSendU8(0x07); // LEAVE_ROOM
                    LeaveRoom();
                }

                mServerHosting = false;
                mServerJoined = false;
                mServerSpectating = false;
                gIsServerModeSpectator = false;
                mServerCreatePending = false;
                mServerHostProbeDone = false;
                mServerGuestProbeDone = false;
                mServerHostHasGuest = false;
                mServerHostSpectateAllowed = false;
                mServerJoinedSpectateAllowed = false;
                mServerHostForceRelay = false;
                mServerClientWantStart = false;
                mServerAskedWantStart = false;
                mServerJoinedRoomGaming = false;
                mServerSpectateReservationActive = false;
                mServerSpectateReserveTick = 0;
                mServerSpectateReserveWarnTick = 0;
                mServerHostedRoomId = 0;
                mServerJoinedRoomId = 0;
                mServerHostedRoomName[0] = '\0';
                mServerJoinedRoomName[0] = '\0';
                mServerSpectatorCount = 0;
                std::memset(mServerSpectatorNames, 0, sizeof(mServerSpectatorNames));
                gSecondPlayerName[0] = '\0';
                gServerHostName[0] = '\0';
                ServerResetP2PState(true);
                if (mServerConnected) {
                    ServerSendQuery();
                }
            }
        }

        // 自动 Query：空闲房间列表每秒刷新；进房/观战席也低频发送，避免服务端按应用层空闲断开。
        mServerLastQueryTick++;
        if (mServerConnected && !mServerGameStarting && !mServerCreatePending && mServerSock >= 0) {
            const bool inRoom = mServerHosting || mServerJoined || mServerSpectating;
            const int queryInterval = inRoom ? 1500 : 100; // ~15s in-room keepalive, ~1s room-list refresh.
            if (mServerLastQueryTick >= queryInterval) {
                mServerLastQueryTick = 0;
                ServerSendQuery();
            }
        } else {
            // 不在空闲态就不刷列表，tick 防溢出
            if (mServerLastQueryTick > 1000000)
                mServerLastQueryTick = 0;
        }
    }

    // =========================================================
    // 4) 每帧根据状态刷新文字/禁用（避免状态变化后没更新）
    // =========================================================
    RefreshButtons();
}


void NetplayLobbyWidget::ProcessClientEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_CLIENT_WAITFORSECONDPALYER_PLAYER_NAME: {
            auto *nameEvent = static_cast<const CHARx32_Event *>(event);
            const char *localName = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) ? mApp->mPlayerInfo->mName : "";
            std::strncpy(gServerHostName, localName, sizeof(gServerHostName) - 1);
            gServerHostName[sizeof(gServerHostName) - 1] = '\0';
            std::strncpy(gSecondPlayerName, nameEvent->charx32_0, sizeof(gSecondPlayerName) - 1);
            gSecondPlayerName[sizeof(gSecondPlayerName) - 1] = '\0';

            CHARx32_Event nameEventReply{};
            nameEventReply.type = EVENT_SERVER_WAITFORSECONDPALYER_PLAYER_NAME;
            strncpy(nameEventReply.charx32_0, mApp->mPlayerInfo->mName, sizeof(nameEventReply.charx32_0) - 1);
            netplay::PutEvent(nameEventReply);
        } break;
        default:
            break;
    }
}

void NetplayLobbyWidget::ProcessServerEvent(const BaseEvent *event) {
    LOG_DEBUG("TYPE:{}", (int)event->type);
    switch (event->type) {
        case EVENT_SERVER_WAITFORSECONDPALYER_VERSION_CHECK: {
            auto *event1 = static_cast<const U16_Event *>(event);
            const uint16_t expectedProtocol = GetLanRoomProtocol(mIsCoopLobby);
            if (event1->u16_0 != expectedProtocol) {
                LOG_ERROR("Room Version Mismatch!");
                // 弹出提示并断开连接
                LeaveRoom();
                InitUdpScanSocket();
                mApp->LawnMessageBox(
                    Dialogs::DIALOG_MESSAGE, "[VERSION_ERROR_TITLE]", event1->u16_0 > expectedProtocol ? "[VERSION_ERROR_HIGN_DESC]" : "[VERSION_ERROR_LOW_DESC]", "[DIALOG_BUTTON_OK]", "", 3);
            } else {
                CHARx32_Event nameEvent{};
                nameEvent.type = EVENT_CLIENT_WAITFORSECONDPALYER_PLAYER_NAME;
                strncpy(nameEvent.charx32_0, mApp->mPlayerInfo->mName, sizeof(nameEvent.charx32_0) - 1);
                netplay::PutEvent(nameEvent);
            }
        } break;
        case EVENT_SERVER_WAITFORSECONDPALYER_PLAYER_NAME: {
            auto *nameEvent = static_cast<const CHARx32_Event *>(event);
            const char *localName = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) ? mApp->mPlayerInfo->mName : "";
            std::strncpy(gServerHostName, nameEvent->charx32_0, sizeof(gServerHostName) - 1);
            gServerHostName[sizeof(gServerHostName) - 1] = '\0';
            std::strncpy(gSecondPlayerName, localName, sizeof(gSecondPlayerName) - 1);
            gSecondPlayerName[sizeof(gSecondPlayerName) - 1] = '\0';
        } break;
        case EVENT_WAITFORSECONDPALYER_START_GAME:
            //            GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 1, 0);
            //            GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 1, 0);
            RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
            break;
        case EVENT_SERVER_VSSETUPMENU_SYNC_VS_MODE: {
            if (mServerSpectating && mServerJoinedRoomGaming && mServerSpectateReservationActive) {
                auto *syncEvent = static_cast<const U8U8_Event *>(event);
                Challenge::msVSShuffleMode = syncEvent->u8_0 == 1;
                Challenge::msVSLegacyMode = syncEvent->u8_0 == 2;
                gVSBackground = BackgroundType(syncEvent->u8_1);
                mServerJoinedRoomGaming = false;
                mServerSpectateReservationActive = false;
                mServerSpectateReserveTick = 0;
                mServerSpectateReserveWarnTick = 0;
                RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
                mApp->KillChallengeScreen();
                mApp->PreNewGame(GAMEMODE_MP_VS, false);
            } else {
            }
        } break;
        default:
            break;
    }
}


void NetplayLobbyWidget::InitUdpScanSocket() {
    gScannedServerCount = 0;
    gUdpScanSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (gUdpScanSocket < 0) {
        LOG_DEBUG("socket ERROR");
        return;
    }

    int flags = fcntl(gUdpScanSocket, F_GETFL, 0);
    fcntl(gUdpScanSocket, F_SETFL, flags | O_NONBLOCK);

    int opt = 1;
    setsockopt(gUdpScanSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // ✅ 新增：接收端也需要允许广播
    setsockopt(gUdpScanSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    sockaddr_in recv_addr{
        .sin_family = AF_INET,
        .sin_port = htons(UDP_PORT),
        .sin_addr{.s_addr = INADDR_ANY},
    };
    if (bind(gUdpScanSocket, (sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
        LOG_DEBUG("bind ERROR errno={}", errno);
        close(gUdpScanSocket);
        gUdpScanSocket = -1;
        return;
    }
    CollectAllBroadcastTargets(gBroadcastTargets);
    //    LOG_DEBUG("[UDP Scan] Listening on port {}", UDP_PORT);
}

void NetplayLobbyWidget::CloseUdpScanSocket() {
    if (gUdpScanSocket >= 0) {
        close(gUdpScanSocket);
        gUdpScanSocket = -1;
    }
    // gScannedServerCount = 0;
}

bool NetplayLobbyWidget::GetActiveBroadcast(sockaddr_in &out_bcast, std::string *out_ifname) {
    // 优先级：wlan/ap/en > eth > 其他（跳过回环和 rmnet/ccmni 移动数据接口）
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return false;

    ifconf ifc;
    char buf[2048]; // 加大缓冲，模拟器接口多
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
        close(fd);
        return false;
    }

    // 候选槽
    sockaddr_in cand[3]{}; // 0=wifi/en  1=eth  2=other
    std::string cand_if[3];
    bool cand_ok[3]{false, false, false};

    for (ifreq *it = (ifreq *)buf, *end = (ifreq *)(buf + ifc.ifc_len); it < end; ++it) {
        ifreq ifr{};
        strncpy(ifr.ifr_name, it->ifr_name, IFNAMSIZ);

        // 过滤：回环 / 未启用
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
            continue;
        if ((ifr.ifr_flags & IFF_LOOPBACK) || !(ifr.ifr_flags & IFF_UP))
            continue;

        // 跳过移动数据虚拟接口（rmnet / ccmni / pdp）
        const char *n = ifr.ifr_name;
        if (strncmp(n, "rmnet", 5) == 0 || strncmp(n, "ccmni", 5) == 0 || strncmp(n, "pdp", 3) == 0)
            continue;

        if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0)
            continue;
        sockaddr_in *sin = (sockaddr_in *)&ifr.ifr_broadaddr;
        if (sin->sin_family != AF_INET)
            continue;

        // 分槽存放
        if (strncasecmp(n, "wlan", 4) == 0 || strncasecmp(n, "ap", 2) == 0 || strncasecmp(n, "en", 2) == 0) {
            cand[0] = *sin;
            cand_if[0] = n;
            cand_ok[0] = true;
        } else if (strncasecmp(n, "eth", 3) == 0) {
            // ✅ 新增：eth0 是模拟器最常见的局域网接口
            if (!cand_ok[1]) {
                cand[1] = *sin;
                cand_if[1] = n;
                cand_ok[1] = true;
            }
        } else {
            if (!cand_ok[2]) {
                cand[2] = *sin;
                cand_if[2] = n;
                cand_ok[2] = true;
            }
        }
    }
    close(fd);

    for (int i = 0; i < 3; i++) {
        if (cand_ok[i]) {
            out_bcast = cand[i];
            if (out_ifname)
                *out_ifname = cand_if[i];
            char ipstr[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &cand[i].sin_addr, ipstr, sizeof(ipstr));
            LOG_DEBUG("[UDP] selected if={} bcast={}", cand_if[i], ipstr);
            return true;
        }
    }
    return false;
}


void NetplayLobbyWidget::CreateRoom() {
    const char *localName = (mApp && mApp->mPlayerInfo && mApp->mPlayerInfo->mName) ? mApp->mPlayerInfo->mName : "";
    std::strncpy(gServerHostName, localName, sizeof(gServerHostName) - 1);
    gServerHostName[sizeof(gServerHostName) - 1] = '\0';
    gSecondPlayerName[0] = '\0';

    // 1) 创建TCP监听socket
    gTcpListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (gTcpListenSocket < 0) {
        LOG_DEBUG("TCP socket failed errno={}", errno);
        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CREATE_ROOM_FAIL_TITLE]", "[CREATE_ROOM_FAIL_SOCKET]", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }

    int flags = fcntl(gTcpListenSocket, F_GETFL, 0);
    fcntl(gTcpListenSocket, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(mApp->mPlayerInfo->mVSRoomPort), // 允许0
        .sin_addr{.s_addr = INADDR_ANY},
    };
    int opt = 1;
    setsockopt(gTcpListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(gTcpListenSocket, (sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_DEBUG("TCP bind failed errno={}", errno);

        close(gTcpListenSocket);
        gTcpListenSocket = -1;

        InitUdpScanSocket();

        pvzstl::string strFmt = TodStringTranslate("[CREATE_ROOM_FAIL_BIND]");

        int result = mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CREATE_ROOM_FAIL_TITLE]", StrFormat(strFmt.c_str(), mApp->mPlayerInfo->mVSRoomPort).c_str(), "[DIALOG_BUTTON_OK]", "", 3);
        if (result == 1000) {
            mInputPurpose = InputPurpose::HOST_SET_PORT;
            ShowTextInput("[INPUT_TITLE_SET_PORT]", "[HINT_PORT]");
        }
        return;
    }

    if (listen(gTcpListenSocket, 1) < 0) {
        LOG_DEBUG("TCP listen failed errno={}", errno);

        close(gTcpListenSocket);
        gTcpListenSocket = -1;

        mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[CREATE_ROOM_FAIL_TITLE]", "[CREATE_ROOM_FAIL_LISTEN]", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }

    socklen_t addr_len = sizeof(addr);
    getsockname(gTcpListenSocket, (sockaddr *)&addr, &addr_len);
    gTcpPort = ntohs(addr.sin_port);

    gUdpBroadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (gUdpBroadcastSocket < 0) {
        mIsCreatingRoom = true;
        LOG_DEBUG("UDP socket failed errno={}", errno);
        return;
    }

    int on = 1;
    setsockopt(gUdpBroadcastSocket, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    setsockopt(gUdpBroadcastSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    gBroadcastTargets.clear();
    CollectAllBroadcastTargets(gBroadcastTargets);

    if (!gBroadcastTargets.empty()) {
        gBroadcastAddr = gBroadcastTargets.front().addr;
        gIfname = gBroadcastTargets.front().ifname;

        for (const auto &target : gBroadcastTargets) {
            char ipstr[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &target.addr.sin_addr, ipstr, sizeof(ipstr));
            LOG_DEBUG("[UDP] use if={} local={} bcast={}", target.ifname, target.local_ip, ipstr);
        }
    } else {
        std::memset(&gBroadcastAddr, 0, sizeof(gBroadcastAddr));
        gBroadcastAddr.sin_family = AF_INET;
        gBroadcastAddr.sin_port = htons(UDP_PORT);
        inet_pton(AF_INET, "255.255.255.255", &gBroadcastAddr.sin_addr);
        gIfname = "fallback";
        LOG_WARN("[UDP] fallback broadcast 255.255.255.255:{}", UDP_PORT);
    }

    flags = fcntl(gUdpBroadcastSocket, F_GETFL, 0);
    fcntl(gUdpBroadcastSocket, F_SETFL, flags | O_NONBLOCK);

    LOG_DEBUG("[Host] Room created. TCP port={}, UDP port={}", gTcpPort, UDP_PORT);

    UdpBroadcastRoom();
    mIsCreatingRoom = true;
}

void NetplayLobbyWidget::ExitRoom() {

    mIsCreatingRoom = false;

    if (gTcpClientSocket >= 0) {
        shutdown(gTcpClientSocket, SHUT_RDWR); // 关闭读写
        close(gTcpClientSocket);
        gTcpClientSocket = -1;
    }

    if (gTcpListenSocket >= 0) {
        shutdown(gTcpListenSocket, SHUT_RDWR);
        close(gTcpListenSocket);
        gTcpListenSocket = -1;
    }

    if (gUdpBroadcastSocket >= 0) {
        close(gUdpBroadcastSocket);
        gUdpBroadcastSocket = -1;
    }
    gBroadcastTargets.clear();

    // 其他清理操作
}


void NetplayLobbyWidget::JoinRoom() {
    mIsJoiningRoom = true;
}

void NetplayLobbyWidget::LeaveRoom() {
    mIsJoiningRoom = false;
    if (gTcpServerSocket >= 0) {
        shutdown(gTcpServerSocket, SHUT_RDWR); // 关闭读写
        close(gTcpServerSocket);
        gTcpServerSocket = -1;
        gTcpConnecting = false;
        gTcpConnected = false;
    }

    mUseManualTarget = false;
    mManualIp[0] = '\0';
    mManualPort = 0;
    gSecondPlayerName[0] = '\0';
    gServerHostName[0] = '\0';
}

void NetplayLobbyWidget::UdpBroadcastRoom() {
    gLastBroadcastTime = 0;
    if (gUdpBroadcastSocket < 0)
        return;
    LawnApp *lawnApp = gLawnApp;
    if (!lawnApp || !lawnApp->mPlayerInfo || !lawnApp->mPlayerInfo->mName)
        return;

    const char *message = lawnApp->mPlayerInfo->mName;

    if (gTcpPort != 0) {
        size_t msg_len = strlen(message) + 1; // 含 '�'
        size_t total_len = msg_len + sizeof(gTcpPort) + 1;

        char send_buf[NAME_LENGTH + sizeof(gTcpPort) + 1];
        if (total_len > sizeof(send_buf))
            return; // 防止溢出

        memcpy(send_buf, message, msg_len);
        memcpy(send_buf + msg_len, &gTcpPort, sizeof(gTcpPort));
        send_buf[msg_len + sizeof(gTcpPort)] = mIsCoopLobby ? 1 : 0;

        bool sent_any = false;
        if (!gBroadcastTargets.empty()) {
            for (const auto &target : gBroadcastTargets) {
                ssize_t sent = sendto(gUdpBroadcastSocket, send_buf, total_len, 0, (sockaddr *)&target.addr, sizeof(target.addr));

                if (sent > 0) {
                    sent_any = true;
                    char ipstr[INET_ADDRSTRLEN]{};
                    inet_ntop(AF_INET, &target.addr.sin_addr, ipstr, sizeof(ipstr));
                    LOG_DEBUG("[Send] if={}, bcast={}, msg='{}', num={}", target.ifname, ipstr, message, gTcpPort);
                } else if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                    char ipstr[INET_ADDRSTRLEN]{};
                    inet_ntop(AF_INET, &target.addr.sin_addr, ipstr, sizeof(ipstr));
                    LOG_DEBUG("sendto ERROR if={} bcast={} errno={}", target.ifname, ipstr, errno);
                }
            }
        } else {
            ssize_t sent = sendto(gUdpBroadcastSocket, send_buf, total_len, 0, (sockaddr *)&gBroadcastAddr, sizeof(gBroadcastAddr));
            if (sent > 0) {
                sent_any = true;
                LOG_DEBUG("[Send] msg: '{}', num: {}", message, gTcpPort);
            } else if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                LOG_DEBUG("sendto ERROR {}", errno);
            }
        }

        if (!sent_any) {
            LOG_DEBUG("[Send] no broadcast target sent, msg='{}', num={}", message, gTcpPort);
        }
    }
}

bool NetplayLobbyWidget::CheckTcpAccept() {
    if (gTcpListenSocket < 0)
        return false;
    if (IsRemoteServer()) {
        return true;
    }
    sockaddr_in clientAddr{};
    socklen_t addrlen = sizeof(clientAddr);
    gTcpClientSocket = accept(gTcpListenSocket, (sockaddr *)&clientAddr, &addrlen);
    if (gTcpClientSocket < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return false; // 没有连接
        LOG_DEBUG("accept ERROR");
        return false;
    }
    int one = 1;
    setsockopt(gTcpClientSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)); // 禁用 Nagle 算法
    int on = 1;
    setsockopt(gTcpClientSocket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
    int idle = 30;
    setsockopt(gTcpClientSocket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    int intvl = 10;
    setsockopt(gTcpClientSocket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    int cnt = 3;
    setsockopt(gTcpClientSocket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

    int flags = fcntl(gTcpClientSocket, F_GETFL, 0);
    fcntl(gTcpClientSocket, F_SETFL, flags | O_NONBLOCK);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
    LOG_DEBUG("[TCP] Client connected: {}", ip);

    // 检查version
    U16_Event event = {{EVENT_SERVER_WAITFORSECONDPALYER_VERSION_CHECK}, GetLanRoomProtocol(mIsCoopLobby)};
    netplay::PutEvent(event);
    return true;
}

void NetplayLobbyWidget::ScanUdpBroadcastRoom() {
    gLastBroadcastTime = 0;
    sockaddr_in recv_addr{};
    socklen_t addr_len = sizeof(recv_addr);
    char buffer[NAME_LENGTH + sizeof(int) + 1] = {0};

    // 循环读取所有可用包
    while (true) {
        ssize_t n = recvfrom(gUdpScanSocket, buffer, sizeof(buffer), 0, (sockaddr *)&recv_addr, &addr_len);
        if (n > 0) {
            // 解析消息
            char *msg = buffer;
            size_t msg_len = strnlen(msg, NAME_LENGTH - 1) + 1;

            if (n < (ssize_t)(msg_len + sizeof(int)))
                continue; // 包太短，跳过

            int tcpPort = 0;
            memcpy(&tcpPort, buffer + msg_len, sizeof(tcpPort));
            const bool roomIsCoop = n >= (ssize_t)(msg_len + sizeof(int) + 1) && buffer[msg_len + sizeof(int)] != 0;
            if (roomIsCoop != mIsCoopLobby)
                continue;

            char serverIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &recv_addr.sin_addr, serverIp, sizeof(serverIp));

            time_t now = time(nullptr);
            bool found = false;

            // 更新已存在的server
            for (int i = 0; i < gScannedServerCount; i++) {
                if (strcmp(gServers[i].ip, serverIp) == 0) {
                    gServers[i].tcpPort = tcpPort;
                    strncpy(gServers[i].name, msg, NAME_LENGTH - 1);
                    gServers[i].lastSeen = now;
                    found = true;
                    LOG_DEBUG("[Scan] Update server: {}:{} ({})", serverIp, tcpPort, msg);
                    break;
                }
            }

            // 新server
            if (!found && gScannedServerCount < MAX_SERVERS) {
                strncpy(gServers[gScannedServerCount].ip, serverIp, INET_ADDRSTRLEN - 1);
                strncpy(gServers[gScannedServerCount].name, msg, NAME_LENGTH - 1);
                gServers[gScannedServerCount].tcpPort = tcpPort;
                gServers[gScannedServerCount].lastSeen = now;
                gScannedServerCount++;
                LOG_DEBUG("[Scan] New server: {}:{} ({})", serverIp, tcpPort, msg);
            }

        } else if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break; // 没有更多数据可读
            else
                LOG_DEBUG("recvfrom ERROR");
            break; // 数据错误
        } else {
            break; // 没有数据
        }
    }

    // 检查超时
    time_t current_time = time(nullptr);
    for (int i = 0; i < gScannedServerCount;) {
        if (difftime(current_time, gServers[i].lastSeen) > UDP_TIMEOUT) {

            // 如果选中的是最后一个，而我们要把最后一个删掉
            int last = gScannedServerCount - 1;

            // 1) 如果选中项就是被删除的 i：
            //    删除后，当前位置会被 last 覆盖，所以让选中保持在 i（继续指向“被搬过来的那一项”）
            if (mSelectedServerIndex == i) {
                // 选中保持 i，不变
            }
            // 2) 如果选中项是 last，而 last 要被搬到 i：
            //    选中项应该跟着搬到 i（否则你会“莫名丢选中”）
            else if (mSelectedServerIndex == last) {
                mSelectedServerIndex = i;
            }
            // 3) 其他情况不用改

            gServers[i] = gServers[last];
            gScannedServerCount--;

            // 删除后防越界
            if (gScannedServerCount <= 0) {
                mSelectedServerIndex = 0;
            } else if (mSelectedServerIndex >= gScannedServerCount) {
                mSelectedServerIndex = gScannedServerCount - 1;
            }

            continue;
        }
        i++;
    }
}

void NetplayLobbyWidget::TryTcpConnect() {
    if (IsRemoteClient() || gIsReplayMode)
        return;

    // 既不是手动目标，也没有扫描到房间，就没法连
    if (!mUseManualTarget && gScannedServerCount == 0)
        return;

    // 统一得到目标 ip/port（用于 connect + 日志）
    char targetIp[INET_ADDRSTRLEN] = {0};
    int targetPort = 0;

    if (mUseManualTarget) {
        strncpy(targetIp, mManualIp, INET_ADDRSTRLEN - 1);
        targetPort = mManualPort;
    } else {
        int idx = mSelectedServerIndex;
        if (idx < 0)
            idx = 0;
        if (idx >= gScannedServerCount)
            idx = gScannedServerCount - 1;

        strncpy(targetIp, gServers[idx].ip, INET_ADDRSTRLEN - 1);
        targetPort = gServers[idx].tcpPort;
    }

    // 组装 sockaddr
    sockaddr_in server_addr{
        .sin_family = AF_INET,
        .sin_port = htons(targetPort),
    };
    inet_pton(AF_INET, targetIp, &server_addr.sin_addr);

    if (!gTcpConnecting) {
        gTcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (gTcpServerSocket < 0) {
            LOG_DEBUG("[Client] socket ERROR errno={}", errno);
            return;
        }

        // 非阻塞
        int flags = fcntl(gTcpServerSocket, F_GETFL, 0);
        fcntl(gTcpServerSocket, F_SETFL, flags | O_NONBLOCK);

        // 发起非阻塞 connect
        int ret = connect(gTcpServerSocket, (sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
            if (errno == EINPROGRESS) {
                gTcpConnecting = true;
                LOG_DEBUG("[Client] Connecting to {}:{} ...", targetIp, targetPort);
            } else {
                LOG_DEBUG("[Client] connect ERROR errno={}", errno);
                close(gTcpServerSocket);
                gTcpServerSocket = -1;
                gTcpConnecting = false;
                gTcpConnected = false;
            }
        } else {
            // 立即连接成功
            int one = 1;
            setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

            int on = 1;
            setsockopt(gTcpServerSocket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
            int idle = 30;
            setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
            int intvl = 10;
            setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
            int cnt = 3;
            setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

            gTcpConnected = true;
            gTcpConnecting = false;
            LOG_DEBUG("[Client] Connected immediately to {}:{}", targetIp, targetPort);
        }

    } else {
        // 检查连接是否完成
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(gTcpServerSocket, &writefds);

        timeval tv{0, 0};
        int ret = select(gTcpServerSocket + 1, nullptr, &writefds, nullptr, &tv);
        if (ret > 0 && FD_ISSET(gTcpServerSocket, &writefds)) {
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(gTcpServerSocket, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                LOG_DEBUG("[Client] getsockopt ERROR errno={}", errno);
                close(gTcpServerSocket);
                gTcpServerSocket = -1;
                gTcpConnecting = false;
                gTcpConnected = false;
                return;
            }

            if (err == 0) {
                int one = 1;
                setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

                int on = 1;
                setsockopt(gTcpServerSocket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
                int idle = 30;
                setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
                int intvl = 10;
                setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
                int cnt = 3;
                setsockopt(gTcpServerSocket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

                gTcpConnected = true;
                gTcpConnecting = false;
                LOG_DEBUG("[Client] Connected to {}:{}", targetIp, targetPort);
            } else {
                LOG_DEBUG("[Client] Connect failed to {}:{} err={}", targetIp, targetPort, err);
                close(gTcpServerSocket);
                gTcpServerSocket = -1;
                gTcpConnecting = false;
                gTcpConnected = false;
            }
        }
        // select==0 表示还在连接中，下次 Update 再检查
    }
}


void NetplayLobbyWidget::StopUdpBroadcastRoom() {
    if (gUdpBroadcastSocket >= 0) {
        close(gUdpBroadcastSocket);
        gUdpBroadcastSocket = -1;
    }
    gBroadcastTargets.clear();
    LOG_DEBUG("[UDP] Broadcast closed\n");
}


void NetplayLobbyWidget::HandleButtonDepress(int theId) {
    auto *aDialog = this;
    const UIMode aUIMode = aDialog->mUIMode;
    switch (theId) {
        case NetplayLobbyWidget::NetplayLobbyWidget_Enter:
            switch (aUIMode) {
                case UIMode::MODE1_INIT:
                    // 本地游戏：按两下A
                    //                    aDialog->GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 1, 0);
                    //                    aDialog->GameButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_A, 1, 0);
                    aDialog->RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
                    break;
                case UIMode::MODE2_WIFI:
                    if (aDialog->mIsCreatingRoom) {
                        // 开始游戏（房主）：根据是否有玩家加入决定是否可点（RefreshButtons里已禁用）
                        aDialog->RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
                        if (IsRemoteServer()) {
                            BaseEvent event = {EventType::EVENT_WAITFORSECONDPALYER_START_GAME};
                            netplay::PutEvent(event);
                        }
                    } else {
                        // 加入指定IP房间：弹输入框
                        aDialog->mInputPurpose = InputPurpose::LAN_JOIN_MANUAL;
                        ShowTextInput("[INPUT_TITLE_JOIN_IP]", "[HINT_IP_PORT]");
                        return;
                    }
                    break;
                case UIMode::MODE3_SERVER:
                    if (aDialog->mServerHosting) {
                        aDialog->ServerSendStart();
                        return;
                    }
                    if (aDialog->mServerSpectating) {
                        if (!aDialog->mServerJoinedRoomGaming || aDialog->mServerSpectateReservationActive) {
                            RefreshButtons();
                            return;
                        }
                        aDialog->mServerStatusText = TodStringTranslate("[LIVE_SPECTATE_LOADING]");
                        aDialog->ServerSendReserveSpectate(true);
                        RefreshButtons();
                        return;
                    }
                    if (aDialog->mServerJoined) {
                        aDialog->ServerSendAskStart();
                        return;
                    }
                    if (aDialog->mServerConnecting) {
                        aDialog->ServerDisconnect("user stop connect");
                        RefreshButtons();
                        return;
                    }
                    if (aDialog->mServerConnected && !aDialog->mServerJoined) {
                        aDialog->ServerDisconnect("user disconnect");
                        RefreshButtons();
                        return;
                    }

                    aDialog->mInputPurpose = InputPurpose::SERVER_CONNECT_ADDR;
                    ShowTextInput("[INPUT_TITLE_CONNECT_SERVER]", "[HINT_IP_PORT]");
                    return;
            }
            break;
        case NetplayLobbyWidget::NetplayLobbyWidget_BackResult:
            if (aUIMode == UIMode::MODE1_INIT) {
                // 返回：沿用你原来的清理
                aDialog->StopUdpBroadcastRoom();
                aDialog->LeaveRoom();
                aDialog->ExitRoom();
                aDialog->CloseUdpScanSocket();
            } else {
                // 模式2/3：返回到模式1
                if (aUIMode == UIMode::MODE3_SERVER && aDialog->mServerHosting) {
                    aDialog->ServerSendSetSpectate(!aDialog->mServerHostSpectateAllowed);
                    RefreshButtons();
                    return;
                }
                if (aUIMode == UIMode::MODE3_SERVER && IsRemoteClient() && gIsServerModeSpectator) {
                    aDialog->mApp->ClearSecondPlayer();
                }
                if (ServerHostRoomLocked()) {
                    RefreshButtons();
                    return;
                }
                SetMode(UIMode::MODE1_INIT);
                return;
            }
            break;
        case NetplayLobbyWidget::NetplayLobbyWidget_JoinRoom:
            switch (aUIMode) {
                case UIMode::MODE1_INIT:
                    SetMode(UIMode::MODE2_WIFI);
                    break;
                case UIMode::MODE2_WIFI:
                    // ✅ Host（创建房间中）：leftButton 改为“设置房间端口”
                    if (aDialog->mIsCreatingRoom) {
                        aDialog->mInputPurpose = InputPurpose::HOST_SET_PORT;
                        ShowTextInput("[INPUT_TITLE_SET_PORT]", "[HINT_PORT]");
                        return;
                    }

                    // ===== 下面保持你原来的 Join/Leave 逻辑 =====
                    // 加入房间 / 离开房间（沿用你原逻辑）
                    if (aDialog->mIsJoiningRoom) {
                        aDialog->LeaveRoom();
                        aDialog->InitUdpScanSocket();
                    } else {
                        aDialog->JoinRoom();
                        aDialog->CloseUdpScanSocket();
                    }
                    RefreshButtons();
                    break;
                case UIMode::MODE3_SERVER:
                    if (aDialog->mServerGameStarting) {
                        return;
                    }
                    if (!aDialog->mServerConnected) {
                        if (!aDialog->mServerConnecting && !aDialog->mServerHosting && !aDialog->mServerJoined && !aDialog->mServerSpectating) {
                            Mode3ConnectSelectedTarget(aDialog);
                            RefreshButtons();
                        }
                        return;
                    }
                    if (aDialog->mServerHosting) {
                        aDialog->ServerSendKickGuest();
                    } else if (aDialog->mServerJoined || aDialog->mServerSpectating) {
                        aDialog->ServerSendLeaveRoom(); // LEAVE_ROOM(0x07)
                    } else {
                        // 空闲态才能 join
                        aDialog->ServerSendJoinSelected(); // JOIN(0x03)
                    }
                    RefreshButtons();
                    return;
            }
            break;
        case NetplayLobbyWidget::NetplayLobbyWidget_CreateRoom:
            switch (aUIMode) {
                case UIMode::MODE1_INIT:
                    SetMode(UIMode::MODE3_SERVER);
                    break;
                case UIMode::MODE2_WIFI:
                    // 创建房间 / 退出房间（沿用你原逻辑）
                    if (aDialog->mIsCreatingRoom) {
                        aDialog->ExitRoom();
                        aDialog->InitUdpScanSocket();
                    } else {
                        aDialog->CreateRoom();
                        aDialog->CloseUdpScanSocket();
                    }
                    RefreshButtons();
                    break;
                case UIMode::MODE3_SERVER:
                    if (aDialog->mServerGameStarting) {
                        return;
                    }
                    if (!aDialog->mServerConnected && !aDialog->mServerConnecting && !aDialog->mServerHosting && !aDialog->mServerJoined && !aDialog->mServerSpectating) {
                        OpenReplayManageWidget();
                        return;
                    }
                    if (!aDialog->mServerConnected) {
                        return;
                    }
                    if (aDialog->mServerHosting) {
                        aDialog->ServerSendExitRoom(); // EXIT_ROOM(0x06)
                    } else if (aDialog->mServerJoined) {
                        aDialog->ServerSendSwitchRole(true);
                    } else if (aDialog->mServerSpectating) {
                        aDialog->ServerSendSwitchRole(false);
                    } else {
                        aDialog->ServerSendCreate(); // CREATE(0x01)
                    }
                    RefreshButtons();
                    return;
            }
            break;
        case ReplayManageWidget::ReplayManageWidget_Import:
            if (aDialog->mReplayManageWidget != nullptr) {
                aDialog->mReplayManageWidget->RequestImportReplay();
            }
            return;
        case ReplayManageWidget::ReplayManageWidget_Export:
            if (aDialog->mReplayManageWidget != nullptr) {
                aDialog->mReplayManageWidget->RequestExportReplay();
            }
            return;
        case ReplayManageWidget::ReplayManageWidget_Delete:
            if (aDialog->mReplayManageWidget != nullptr) {
                aDialog->mReplayManageWidget->DeleteSelectedReplay();
            }
            return;
        case ReplayManageWidget::ReplayManageWidget_Play:
            if (aDialog->mReplayManageWidget != nullptr) {
                aDialog->mReplayManageWidget->PlaySelectedReplay();
            }
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_ReplayClose:
            CloseReplayManageWidget();
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_AddServer:
            if (!aDialog->mIsCreatingRoom && !aDialog->mIsJoiningRoom && !aDialog->mServerConnecting && !aDialog->mServerHosting && !aDialog->mServerJoined && !aDialog->mServerSpectating) {
                OpenCustomServerInput();
            }
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_ReplayManage:
            if (!aDialog->mIsCreatingRoom && !aDialog->mIsJoiningRoom && !aDialog->mServerHosting && !aDialog->mServerJoined && !aDialog->mServerSpectating) {
                OpenReplayManageWidget();
            }
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_LocalBattle:
            if (!aDialog->mIsCreatingRoom && !aDialog->mIsJoiningRoom && !aDialog->mServerConnecting && !aDialog->mServerHosting && !aDialog->mServerJoined && !aDialog->mServerSpectating) {
                SetMode(UIMode::MODE1_INIT);
                aDialog->RequestClose(NetplayLobbyWidget::NetplayLobbyWidget_Enter);
            }
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_Back:
            ExitNetplayLobby();
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_PrimaryAction:
            HandleButtonDepress(NetplayLobbyWidget_Enter);
            return;
        case NetplayLobbyWidget::NetplayLobbyWidget_RoomOption:
            HandleButtonDepress(NetplayLobbyWidget_BackResult);
            return;
        default:
            break;
    }
}

void NetplayLobbyWidget::ButtonDepress_Thunk(this Sexy::ButtonListener &self, int id) {
    static_cast<NetplayLobbyWidget &>(self).HandleButtonDepress(id);
}

void NetplayLobbyWidget::RequestClose(int result) {
    mResult = result;
    mCloseRequested = true;
}
