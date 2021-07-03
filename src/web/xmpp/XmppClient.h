#pragma once

#include "../../utils/Callback.h"
#include "Message.h"
#include "Presence.h"
#include "Stanza.h"
#include "UserJid.h"

#include <future>
#include <ixwebsocket/IXWebSocket.h>
#include <strophe.h>

namespace EGL3::Web::Xmpp {
    class XmppClient {
    public:
        XmppClient(const std::string& Url, const UserJid& Jid, const std::string& AuthToken);

        ~XmppClient();

        void Start();

        void Stop();

        void SendChat(const UserJid& To, const std::string& Data);

        void SendPresence(const Presence& Presence);

        Utils::Callback<void()> OnConnected;
        Utils::Callback<void()> OnDisconnected;
        Utils::Callback<void(const Message& Message)> OnMessage;
        Utils::Callback<void(const Message& Message)> OnChat;
        Utils::Callback<void(const UserJid& From, const Presence& Presence)> OnPresence;

    private:
        static void ExtSockConnect(void* Ctx);
        static void ExtSockClose(void* Ctx);
        static void ExtSockSend(const char* Data, size_t Size, void* Ctx);
        static void HandleConn(xmpp_conn_t* Connection, xmpp_conn_event_t Event, int Error, xmpp_stream_error_t* StreamError, void* Ctx);
        static int HandleStanza(xmpp_conn_t* Connection, xmpp_stanza_t* StanzaPtr, void* Ctx);

        void ReceivedMessage(const ix::WebSocketMessagePtr& Message);

        void HandleStanza(const Stanza& Stanza);
        bool HandleMessageStanza(const Stanza& Stanza);
        bool HandleMultiUserChatStanza(const Stanza& Stanza);
        bool HandlePingStanza(const Stanza& Stanza);
        bool HandlePresenceStanza(const Stanza& Stanza);
        bool HandlePrivateChatStanza(const Stanza& Stanza);
        bool HandlePubSubStanza(const Stanza& Stanza);

        void RunHeartbeatTask();
        void ResetHeartbeatTime(bool StopHeartbeat = false);

        void SendPing();

        void SendStanza(const Stanza& Stanza);

        ix::WebSocket Socket;

        xmpp_ctx_t* Ctx;
        xmpp_conn_t* Connection;

        UserJid Jid;
        std::string Domain;
        std::string MucDomain;
        std::shared_future<void> HeartbeatTask;
        std::mutex HeartbeatMtx;
        std::condition_variable HeartbeatCV;
        std::chrono::steady_clock::time_point HeartbeatTime;
    };
}