/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 */

#ifndef PVZ_LAWN_WIDGET_NETPLAY_LOBBY_WIDGET_H
#define PVZ_LAWN_WIDGET_NETPLAY_LOBBY_WIDGET_H

#include "PvZ/Lawn/Widget/ReplayManageWidget.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Widget/ButtonListener.h"
#include "PvZ/SexyAppFramework/Widget/Widget.h"

#include <string>

#include <cstdint>

struct BaseEvent;
struct sockaddr_in;

enum class UIMode {
    MODE1_INIT = 1,
    MODE2_WIFI = 2,
    MODE3_SERVER = 3,
};

enum class InputPurpose {
    NONE = 0,
    LAN_JOIN_MANUAL,
    HOST_SET_PORT,
    SERVER_CONNECT_ADDR,
    RECONNECT_CODE,
};

struct ServerRoomItem {
    int roomId;
    int protocolVersion;
    char name[128];
    bool full;
    bool gaming;
    bool hostProbeDone;
    bool guestProbeDone;
    bool spectateAllowed;
    bool forceRelay;
    bool reconnectPending;
    bool reconnectBusy;
};

class GameButton;
class NetplayRoomListWidget;
class LawnApp;

namespace Sexy {
class ScrollWidget;
class WidgetManager;
} // namespace Sexy

class NetplayLobbyWidget : public Sexy::Widget, public Sexy::ButtonListener {
public:
    enum {
        NetplayLobbyWidget_Enter = 1000,
        NetplayLobbyWidget_BackResult = 1001,
        NetplayLobbyWidget_JoinRoom = 1002,
        NetplayLobbyWidget_CreateRoom = 1003,
        NetplayLobbyWidget_ReplayClose = 1100,
        NetplayLobbyWidget_AddServer = 1200,
        NetplayLobbyWidget_ReplayManage = 1201,
        NetplayLobbyWidget_Back = 1202,
        NetplayLobbyWidget_PrimaryAction = 1203,
        NetplayLobbyWidget_RoomOption = 1204,
        NetplayLobbyWidget_LocalBattle = 1205,
    };

    static NetplayLobbyWidget *GetInstance();

    LawnApp *mApp;
    int mResult;
    bool mCloseRequested;
    bool mIsCreatingRoom;
    bool mIsJoiningRoom;
    ReplayManageWidget *mReplayManageWidget;
    int mSelectedServerIndex;
    bool mUseManualTarget;
    char mManualIp[INET_ADDRSTRLEN];
    int mManualPort;
    UIMode mUIMode;
    InputPurpose mInputPurpose;
    bool mServerConnected;
    int mSelectedRoomIndex_Server;
    int mServerLatencyMs;
    int mServerQuerySentTick;
    bool mServerQueryPending;
    int mServerTargetLatencyMs[5];
    int mServerTargetProbeSock[5];
    int mServerTargetProbeStartTick[5];
    int mServerTargetNextRefreshTick;
    int mServerSock;
    int mReconnectRoomId = 0;
    bool mReconnectPending = false;
    std::uint32_t mReconnectStartedAt = 0;
    std::uint32_t mReconnectEpoch = 0, mReconnectTransfer = 0;
    bool mServerConnecting;
    bool mServerHosting;
    bool mServerJoined;
    bool mServerSpectating;
    bool mServerCreatePending;
    bool mServerHostProbeDone;
    bool mServerGuestProbeDone;
    bool mServerHostHasGuest;
    bool mServerHostSpectateAllowed;
    bool mServerJoinedSpectateAllowed;
    bool mServerHostForceRelay;
    bool mServerClientWantStart;
    bool mServerAskedWantStart;
    bool mServerJoinedRoomGaming;
    bool mServerSpectateReservationActive;
    int mServerHostedRoomId;
    int mServerJoinedRoomId;
    int mServerLastQueryTick;
    int mServerLastRecvTick;
    int mServerSpectateReserveTick;
    int mServerSpectateReserveWarnTick;
    char mServerHostedRoomName[128];
    char mServerJoinedRoomName[128];
    char mServerSpectatorNames[6][32];
    int mServerSpectatorCount;
    char mServerIp[INET_ADDRSTRLEN];
    int mServerPort;
    ServerRoomItem mServerRooms[255];
    int mServerRoomCount;
    int mServerRoomPage;
    uint8_t mSrvRecvBuf[8192];
    int mSrvRecvLen;
    int mServerP2PListenSock;
    int mServerP2PPendingSock;
    int mServerP2PConnectingSock;
    bool mServerP2PPendingFromAccept;
    bool mServerP2PListenerFailed;
    bool mServerP2PNatSent;
    bool mServerP2POkSent;
    bool mServerP2PFailSent;
    bool mServerP2PDoneReceived;
    bool mServerGameStarting;
    int mServerGameStartingTick;
    std::uint32_t mServerRelayEpoch;
    int mServerP2PLocalPort;
    int mServerP2PProbePort;
    int mServerP2PProbePort2;
    std::uint32_t mServerP2PProbeToken;
    bool mServerP2PProbeDone;
    bool mServerP2PProbeActive;
    bool mServerP2PProbeSocketConnected;
    bool mServerP2PProbeTargetOk[2];
    int mServerP2PProbeSock;
    int mServerP2PProbeAttempt;
    int mServerP2PProbeTargetIndex;
    int mServerP2PProbeStartTick;
    int mServerP2PProbeTokenBytesSent;
    int mServerP2PDeadlineTick;
    int mServerP2PNextRetryTick;
    int mServerP2PTick;
    int mServerP2PTargetRoomId;
    int mServerP2PPeerPort;
    int mServerP2PTimeoutSec;
    char mServerP2PPeerIp[INET_ADDRSTRLEN];
    pvzstl::string mServerStatusText;
    pvzstl::string mServerP2PStatusText;

    Sexy::ScrollWidget *mRoomScrollWidget;
    NetplayRoomListWidget *mRoomListWidget;
    GameButton *mAddServerButton;
    GameButton *mCreateRoomButton;
    GameButton *mJoinRoomButton;
    GameButton *mReplayManageButton;
    GameButton *mLocalBattleButton;
    GameButton *mBackButton;
    GameButton *mPrimaryActionButton;
    GameButton *mRoomOptionButton;
    int mSelectedServerListIndex;
    bool mZombieBackground;
    bool mIsCoopLobby;

    explicit NetplayLobbyWidget(LawnApp *app, bool isCoopLobby = false);
    ~NetplayLobbyWidget();

    void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    void Draw(Sexy::Graphics *g);
    void Update();
    void MouseDown(int x, int y, int theClickCount);
    void RefreshControls();
    void SelectServer(int listIndex);
    void SelectRoom(int roomIndex);
    int GetRoomCount() const;
    bool ServerHostRoomLocked() const;
    bool ServerIsWaitingReservedSpectate() const;
    void SetMode(UIMode mode);
    void RefreshButtons();
    void ShowTextInput(const char *titleKey, const char *hintKey);
    void OpenReplayManageWidget();
    void CloseReplayManageWidget();
    int GetLobbyServerTargetCount() const;
    bool GetLobbyServerTargetAddress(int index, char *outAddress, int outSize) const;
    bool ConnectLobbyServerTarget(int index);
    void OpenCustomServerInput();
    void ExitNetplayLobby();
    void UpdateNetplay();
    bool ManualIpConnect();
    void ProcessClientEvent(const BaseEvent *event);
    void ProcessServerEvent(const BaseEvent *event);
    void InitUdpScanSocket();
    void CloseUdpScanSocket();
    bool GetActiveBroadcast(sockaddr_in &outBroadcast, std::string *outInterfaceName);
    void CreateRoom();
    void ExitRoom();
    void JoinRoom();
    void LeaveRoom();
    void UdpBroadcastRoom();
    bool CheckTcpAccept();
    void ScanUdpBroadcastRoom();
    void TryTcpConnect();
    void StopUdpBroadcastRoom();
    void HandleButtonDepress(int id);
    void ButtonDepress_Thunk(this Sexy::ButtonListener &self, int id);
    void RequestClose(int result);

    bool ServerTryReadOneFrame(uint8_t &outType, uint8_t *outPayload, uint16_t &outLen);
    void ServerMoveBufferedRelayBytesToVsStream(bool asHost);
    void ServerUpdateIO();
    void ServerResetP2PState(bool keepListener);
    bool ServerOpenP2PListener();
    bool ServerSendNatPort();
    bool ServerSendP2PProbe();
    bool ServerStartP2PProbeTarget();
    void ServerAdvanceP2PProbe(bool success);
    void ServerUpdateP2PProbe();
    void ServerHandleP2PInfo(const uint8_t *payload, uint16_t len);
    void ServerAdoptP2PSocket();
    void ServerUpdateP2P();
    void DrawServerP2PStatus(Sexy::Graphics *g, int x, int y);
    bool ServerSendU8(uint8_t b) const;
    bool ServerSendRelayReady(std::uint32_t relayEpoch) const;
    void ServerSendQuery();
    void ServerSendCreate();
    void DrawServerRoomList(Sexy::Graphics *g);
    void ServerSelectRoomByMouse(int x, int y);
    void ServerSendJoinSelected();
    void ServerSendSetSpectate(bool allow);
    void ServerSendSwitchRole(bool toSpectator);
    void ServerSendReserveSpectate(bool reserve);
    void ServerSendRejoinRole(bool toSpectator);
    void ServerSendExitRoom();
    void ServerSendLeaveRoom();
    void ServerOnBorrowedSocketClosed(const char *why);
    void ServerSendKickGuest();
    void ServerSendStart();
    void ServerSendAskStart();
    bool ServerConnectFromInput();
    void ServerDisconnect(const char *why);

protected:
    static NetplayLobbyWidget *gInstance;
    static inline const Sexy::ButtonListener::VTable sButtonListenerVtable{
        .ButtonDepress = (void *)&NetplayLobbyWidget::ButtonDepress_Thunk,
    };

    void _destructor();
    void _destructor2();
};

#endif // PVZ_LAWN_WIDGET_NETPLAY_LOBBY_WIDGET_H
