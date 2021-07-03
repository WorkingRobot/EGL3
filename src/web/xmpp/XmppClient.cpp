#include "XmppClient.h"

#include "../../utils/Crc32.h"
#include "../../utils/Hex.h"
#include "../../utils/Log.h"
#include "../../utils/Random.h"

namespace EGL3::Web::Xmpp {
    constexpr auto HeartbeatInterval = std::chrono::seconds(60);

    void* XmppAlloc(size_t Size, void* Ctx) {
        return malloc(Size);
    }

    void XmppFree(void* Ptr, void* Ctx) {
        free(Ptr);
    }

    void* XmppRealloc(void* Ptr, size_t Size, void* Ctx) {
        return realloc(Ptr, Size);
    }

    void XmppLog(void* Ctx, xmpp_log_level_t Level, const char* Area, const char* Msg) {
        switch (Level) {
        case XMPP_LEVEL_DEBUG:
            //EGL3_LOGF(LogLevel::Debug, "[XMPP] {} - {}", Area, Msg);
            break;
        case XMPP_LEVEL_INFO:
            EGL3_LOGF(LogLevel::Info, "[XMPP] {} - {}", Area, Msg);
            break;
        case XMPP_LEVEL_WARN:
            EGL3_LOGF(LogLevel::Warning, "[XMPP] {} - {}", Area, Msg);
            break;
        case XMPP_LEVEL_ERROR:
        default:
            EGL3_LOGF(LogLevel::Error, "[XMPP] {} - {}", Area, Msg);
            break;
        }
    }

    static constexpr xmpp_log_t Log{
            .handler = XmppLog,
            .userdata = nullptr
    };

    static constexpr xmpp_mem_t Mem{
            .alloc = XmppAlloc,
            .free = XmppFree,
            .realloc = XmppRealloc,
            .userdata = nullptr
    };

    XmppClient::XmppClient(const std::string& Url, const UserJid& Jid, const std::string& AuthToken) :
        Jid(Jid.GetString()),
        Domain(Jid.GetDomain()),
        MucDomain("muc." + Domain),
        HeartbeatTime(std::chrono::steady_clock::time_point::min())
    {
        xmpp_initialize();

        Ctx = xmpp_ctx_new(&Mem, &Log);
        EGL3_VERIFY(Ctx, "Could not create xmpp context");

        Connection = xmpp_conn_new(Ctx);
        EGL3_VERIFY(Connection, "Could not create xmpp connection");

        xmpp_conn_extsock_t ExtSock{
            .connect_handler = ExtSockConnect,
            .close_handler = ExtSockClose,
            .send_handler = ExtSockSend,
            .is_websocket = 1,
            .userdata = this
        };

        xmpp_conn_set_extsock_handlers(Connection, &ExtSock);

        Socket.setUrl(Url);
        Socket.addSubProtocol("xmpp");
        Socket.disableAutomaticReconnection();

        Socket.setOnMessageCallback([this](auto& Message) { ReceivedMessage(Message); });

        xmpp_conn_set_jid(Connection, Jid.GetString().c_str());
        xmpp_conn_set_pass(Connection, AuthToken.c_str());

        EGL3_VERIFY(xmpp_extsock_connect_client(Connection, HandleConn, this) == XMPP_EOK, "Websocket could not connect");

        xmpp_handler_add(Connection, HandleStanza, nullptr, nullptr, nullptr, this);
    }

    XmppClient::~XmppClient()
    {
        Stop();

        xmpp_shutdown();
    }

    void XmppClient::Start()
    {
        Socket.start();
    }

    void XmppClient::Stop()
    {
        ResetHeartbeatTime(true);

        if (Connection) {
            xmpp_conn_release(Connection);
            Connection = nullptr;
        }

        if (Ctx) {
            xmpp_ctx_free(Ctx);
            Ctx = nullptr;
        }
    }

    void XmppClient::SendChat(const UserJid& To, const std::string& Data)
    {
        Stanza Stanza = xmpp_stanza_new(Ctx);
        Stanza.SetName("message");
        Stanza.SetType("chat");
        Stanza.SetTo(To.GetString().c_str());
        Stanza.SetBodyText(Data.c_str());

        SendStanza(Stanza);
    }

    void XmppClient::SendPresence(const Presence& Presence)
    {
        Stanza PresenceStanza = xmpp_stanza_new(Ctx);
        PresenceStanza.SetName("presence");

        if (Presence.Available) {
            {
                Stanza StatusStanza = xmpp_stanza_new(Ctx);
                StatusStanza.SetName("show");
                StatusStanza.SetText(StatusToString(Presence.Status));

                PresenceStanza.AddChild(StatusStanza);
            }

            if (!Presence.StatusText.empty()) {
                Stanza StatusTextStanza = xmpp_stanza_new(Ctx);
                StatusTextStanza.SetName("status");
                StatusTextStanza.SetText(Presence.StatusText.c_str());

                PresenceStanza.AddChild(StatusTextStanza);
            }

            {
                Stanza DelayStanza = xmpp_stanza_new(Ctx);
                DelayStanza.SetName("delay");
                DelayStanza.SetNamespace("urn:xmpp:delay");
                DelayStanza.SetAttribute("stamp", GetTimePoint(Presence.Timestamp).c_str());

                PresenceStanza.AddChild(DelayStanza);
            }
        }
        else {
            PresenceStanza.SetType("unavailable");
        }

        SendStanza(PresenceStanza);
    }

    void XmppClient::HandleStanza(const Stanza& Stanza)
    {
        if (HandleMessageStanza(Stanza)) {
            return;
        }
        if (HandleMultiUserChatStanza(Stanza)) {
            return;
        }
        if (HandlePingStanza(Stanza)) {
            return;
        }
        if (HandlePresenceStanza(Stanza)) {
            return;
        }
        if (HandlePrivateChatStanza(Stanza)) {
            return;
        }
        if (HandlePubSubStanza(Stanza)) {
            return;
        }

        printf("unhandled stanza: %s\n", Stanza.ToString().c_str());
    }

    void XmppClient::ExtSockConnect(void* Ctx)
    {
        ((XmppClient*)Ctx)->Socket.start();
    }

    void XmppClient::ExtSockClose(void* Ctx)
    {
        try {
            ((XmppClient*)Ctx)->Socket.stop();
        }
        catch (...) {

        }
    }

    void XmppClient::ExtSockSend(const char* Data, size_t Size, void* Ctx)
    {
        try {
            auto& Client = *(XmppClient*)Ctx;
            Client.Socket.sendText(std::string(Data, Size));
            Client.ResetHeartbeatTime();
        }
        catch (...) {

        }
    }

    void XmppClient::HandleConn(xmpp_conn_t* Connection, xmpp_conn_event_t Event, int Error, xmpp_stream_error_t* StreamError, void* Ctx)
    {
        auto& Client = *(XmppClient*)Ctx;
        switch (Event) {
        case XMPP_CONN_CONNECT:
        case XMPP_CONN_RAW_CONNECT:
            Client.OnConnected();
            Client.ResetHeartbeatTime();
            Client.HeartbeatCV.notify_all();
            Client.HeartbeatTask = std::async(std::launch::async, [&Client]() { Client.RunHeartbeatTask(); });
            break;
        case XMPP_CONN_DISCONNECT:
        case XMPP_CONN_FAIL:
        default:
            Client.ResetHeartbeatTime(true);
            if (Client.HeartbeatTask.valid()) {
                Client.HeartbeatTask.wait();
            }
            Client.OnDisconnected();
            break;
        }

        if (StreamError) {
            printf("connection error\n");
        }
    }

    void XmppClient::ReceivedMessage(const ix::WebSocketMessagePtr& Message)
    {
        switch (Message->type)
        {
        case ix::WebSocketMessageType::Message:
            xmpp_extsock_receive(Connection, Message->str.c_str(), Message->str.size());
            xmpp_extsock_parser_reset(Connection);
            ResetHeartbeatTime();
            break;
        case ix::WebSocketMessageType::Open:
            xmpp_extsock_connected(Connection);
            break;
        case ix::WebSocketMessageType::Close:
            break;
        case ix::WebSocketMessageType::Error:
            xmpp_extsock_connection_error(Connection, Message->errorInfo.reason.c_str());
            break;
        }
    }

    int XmppClient::HandleStanza(xmpp_conn_t* Connection, xmpp_stanza_t* StanzaPtr, void* Ctx)
    {
        Stanza Stanza = StanzaPtr;
        if (Stanza.GetId() != "_xmpp_session1") {
            ((XmppClient*)Ctx)->HandleStanza(Stanza);
        }
        return 1;
    }

    bool XmppClient::HandleMessageStanza(const Stanza& Stanza)
    {
        if (Stanza.GetName() != "message" || Stanza.GetType() == "chat" || Stanza.GetType() == "groupchat") {
            return false;
        }

        auto ErrorStanza = Stanza.GetChild("error");
        if (ErrorStanza) {
            auto Errors = ErrorStanza.GetChildren();
            if (!Errors.empty()) {
                for (auto& Error : Errors) {
                    std::string ErrorReadable;
                    switch (Utils::Crc32(Error.GetName()))
                    {
                    case Utils::Crc32("recipient-unavailable"):
                        ErrorReadable = "The recipient is no longer available.";
                        break;
                    default:
                        ErrorReadable = Error.GetName();
                        break;
                    }

                    printf("Error: %s\n", ErrorReadable.c_str());
                }
            }
            else {
                printf("Error: unknown message stanza\n");
            }
        }
        else {
            Message Message{
                .From = Stanza.GetFrom(),
                .To = Stanza.GetTo(),
                .Payload = Stanza.GetBodyText()
            };

            OnMessage(Message);
            return true;
        }
        return true;
    }

    bool XmppClient::HandleMultiUserChatStanza(const Stanza& Stanza)
    {
        switch (Utils::Crc32(Stanza.GetName()))
        {
        case Utils::Crc32("presence"):
        {
            UserJid FromJid = Stanza.GetFrom();
            if (!FromJid.IsValid() || FromJid.GetDomain() != MucDomain) {
                return false;
            }
            if (Stanza.GetType() != "error") {
                EGL3_LOGF(LogLevel::Warning, "Unhandled xmpp muc presence recieved");
                return true;
            }
            else {
                EGL3_LOGF(LogLevel::Error, "Unhandled xmpp muc presence error recieved");
                return true;
            }
        }
        case Utils::Crc32("message"):
            switch (Utils::Crc32(Stanza.GetType()))
            {
            case Utils::Crc32("groupchat"):
                EGL3_LOGF(LogLevel::Warning, "Unhandled xmpp muc groupchat recieved");
                return true;
            case Utils::Crc32("error"):
                EGL3_LOGF(LogLevel::Error, "Unhandled xmpp muc groupchat error recieved");
                return true;
            default:
                return false;
            }
        case Utils::Crc32("iq"):
        {
            auto Ping = Stanza.GetChild("ping");
            if (Ping.IsValid() && Ping.GetNamespace() == "urn:xmpp:ping") {
                return false;
            }

            auto Query = Stanza.GetChild("query");
            if (Query.IsValid() && Query.GetNamespace() == "http://jabber.org/protocol/muc#owner") {
                return false;
            }

            switch (Utils::Crc32(Stanza.GetType()))
            {
            case Utils::Crc32("error"):
                return Stanza.GetChild("error").IsValid();
            case Utils::Crc32("result"):
            {
                UserJid From(Stanza.GetFrom());
                // Pongs from the domain (no user or resource attached)
                if (From.IsValid() && From.GetNode().empty() && !From.GetDomain().empty() && From.GetResource().empty()) {
                    return false;
                }
                break;
            }
            }
            // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Online/XMPP/Private/XmppStrophe/XmppMultiUserChatStrophe.cpp#L319
            // Always returns true
            return true;
        }
        default:
            return false;
        }
    }

    bool XmppClient::HandlePingStanza(const Stanza& Stanza)
    {
        if (Stanza.GetName() != "iq") {
            return false;
        }

        switch (Utils::Crc32(Stanza.GetType()))
        {
        case Utils::Crc32("result"):
            // TODO: reset ping timer
            return true;
        case Utils::Crc32("error"):
            return true;
        case Utils::Crc32("get"):
            // clientbound pings aren't used
            return false;
        default:
            return false;
        }
    }

    bool XmppClient::HandlePresenceStanza(const Stanza& Stanza)
    {
        if (Stanza.GetName() != "presence") {
            return false;
        }

        UserJid FromJid = Stanza.GetFrom();
        if (FromJid.GetResource().empty()) {
            return true;
        }

        Presence Presence{};
        if (Stanza.GetType() == "unavailable") {
            Presence.Available = false;
        }
        else {
            Presence.Available = true;

            auto Status = Stanza.GetChild("status");
            if (Status) {
                Presence.StatusText = Status.GetText();
            }

            Presence.Status = Status::Online;

            auto Show = Stanza.GetChild("show");
            if (Show) {
                Presence.Status = StringToStatus(Show.GetText());
            }

            auto Timestamp = Stanza.GetChild("delay");
            if (Timestamp) {
                auto StampStr = Stanza.GetAttribute("stamp");
                GetTimePoint(StampStr.c_str(), StampStr.size(), Presence.Timestamp);
            }
            else {
                Presence.Timestamp = TimePoint::clock::now();
            }
        }

        OnPresence(FromJid, Presence);
        return true;
    }

    bool XmppClient::HandlePrivateChatStanza(const Stanza& Stanza)
    {
        if (Stanza.GetName() != "message" || Stanza.GetType() != "chat") {
            return false;
        }

        auto Body = Stanza.GetBodyText();
        if (Body.empty()) {
            return true;
        }

        Message Message{
            .From = Stanza.GetFrom(),
            .To = Stanza.GetTo(),
            .Payload = Body
        };

        OnChat(Message);
        return true;
    }

    bool XmppClient::HandlePubSubStanza(const Stanza& Stanza)
    {
        // does nothing
        return false;
    }

    void XmppClient::RunHeartbeatTask()
    {
        std::unique_lock Lock(HeartbeatMtx);
        while (HeartbeatTime != std::chrono::steady_clock::time_point::max()) {
            if (HeartbeatCV.wait_until(Lock, HeartbeatTime) == std::cv_status::timeout) {
                Lock.unlock();
                SendPing();
                ResetHeartbeatTime();
                Lock.lock();
            }
        }
    }

    void XmppClient::ResetHeartbeatTime(bool StopHeartbeat)
    {
        bool ModifiedTime = false;
        {
            std::lock_guard Guard(HeartbeatMtx);
            if (HeartbeatTime != std::chrono::steady_clock::time_point::max()) {
                HeartbeatTime = StopHeartbeat ? std::chrono::steady_clock::time_point::max() : std::chrono::steady_clock::now() + HeartbeatInterval;
                ModifiedTime = true;
            }
        }
        if (ModifiedTime) {
            HeartbeatCV.notify_all();
        }
    }

    void XmppClient::SendPing()
    {
        Stanza PingStanza = xmpp_stanza_new(Ctx);
        PingStanza.SetName("iq");
        PingStanza.SetFrom(Jid.GetString().c_str());
        PingStanza.SetTo(std::string(Jid.GetDomain()).c_str());
        PingStanza.SetType("get");
        {
            char Guid[16];
            Utils::GenerateRandomGuid(Guid);
            PingStanza.SetId(Utils::ToHex<true>(Guid).c_str());
        }

        {
            Stanza PingChild = xmpp_stanza_new(Ctx);
            PingChild.SetName("ping");
            PingChild.SetNamespace("urn:xmpp:ping");
            PingStanza.AddChild(PingChild);
        }

        SendStanza(PingStanza);
    }

    void XmppClient::SendStanza(const Stanza& Stanza)
    {
        xmpp_send(Connection, Stanza.Get());
    }
}