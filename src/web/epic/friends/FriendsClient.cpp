#include "FriendsClient.h"

#include "../../../utils/Hex.h"
#include "../../../utils/Random.h"
#include "../../xmpp/ResourceId.h"
#include "../../ClientSecrets.h"
#include "../EpicClient.h"

#include <rapidjson/writer.h>

namespace EGL3::Web::Epic::Friends {
    constexpr const char* Domain = "prod.ol.epicgames.com";

    FriendsClient::FriendsClient(EpicClientAuthed& LauncherClient, EpicClientAuthed& FortniteClient, const UserPresence& Presence) :
        LauncherClient(LauncherClient),
        FortniteClient(FortniteClient),
        EOSClient(GetEOSFromLauncher(LauncherClient)),
        Launcher(Web::GetHostUrl<Host::LauncherStomp>(), LauncherClient.GetAuthData().AccessToken),
        EOS(Web::GetHostUrl<Host::EOSStomp>(), EOSClient.GetAuthData().AccessToken),
        Fortnite(Web::GetHostUrl<Host::XMPP>(), Xmpp::UserJid(*FortniteClient.GetAuthData().AccountId, Domain, Xmpp::ResourceId("Fortnite", "WIN", "").GetString()), FortniteClient.GetAuthData().AccessToken),
        CachedPresence(Presence),
        XmppConnected(false)
    {
        Launcher.OnConnected.Set([this]() {
            char Guid[16];
            Utils::GenerateRandomGuid(Guid);
            Launcher.Subscribe("sub-0", std::format("{}/account/{}", Utils::ToHex<false>(Guid), this->LauncherClient.GetAuthData().AccountId.value()));
            EGL3_LOG(LogLevel::Info, "Launcher subscribed");
        });
        Launcher.OnDisconnected.Set([]() {
            EGL3_LOG(LogLevel::Info, "Launcher disconnected");
        });
        Launcher.OnMessage.Set([this](const std::string& SubscriptionId, const Stomp::Frame& Frame) {
            ReadStompMessage<false>(Frame);
        });

        EOS.OnConnected.Set([this]() {
            char Guid[16];
            Utils::GenerateRandomGuid(Guid);
            EOS.Subscribe("sub-0", std::format("{}/account/{}", Utils::ToHex<false>(Guid), this->EOSClient.GetAuthData().AccountId.value()));
            EGL3_LOG(LogLevel::Info, "EOS subscribed");
        });
        EOS.OnDisconnected.Set([]() {
            EGL3_LOG(LogLevel::Info, "EOS disconnected");
        });
        EOS.OnMessage.Set([this](const std::string& SubscriptionId, const Stomp::Frame& Frame) {
            ReadStompMessage<true>(Frame);
        });

        Fortnite.OnConnected.Set([this]() {
            {
                std::lock_guard Guard(PresenceMtx);
                XmppConnected = true;
            }
            SendPresenceFortnite();
            EGL3_LOG(LogLevel::Info, "Fortnite initialized");
        });
        Fortnite.OnDisconnected.Set([]() {
            EGL3_LOG(LogLevel::Info, "Fortnite disconnected");
        });
        Fortnite.OnMessage.Set([](const Xmpp::Message& Message) {
            //EGL3_LOGF(LogLevel::Debug, "(message) {} -> {}: {}", Message.From.GetString(), Message.To.GetString(), Message.Payload);
        });
        Fortnite.OnChat.Set([this](const Xmpp::Message& Message) {
            OnChatReceived(std::string(Message.From.GetNode()), Message.Payload);
        });
        Fortnite.OnPresence.Set([](const Xmpp::UserJid& From, const Xmpp::Presence& Presence) {
            //EGL3_LOGF(LogLevel::Warning, "(presence) {}: {}", From.GetString(), Presence.StatusText);
        });

        Launcher.Start();
        EOS.Start();
        Fortnite.Start();
    }

    FriendsClient::~FriendsClient()
    {
        Launcher.Stop();
        EOS.Stop();
        Fortnite.Stop();
    }

    void FriendsClient::SetPresence(const UserPresence& NewPresence)
    {
        {
            std::lock_guard Guard(PresenceMtx);
            CachedPresence = NewPresence;
        }

        SendPresenceEOS();
        SendPresenceLauncher();
        SendPresenceFortnite();
    }

    void FriendsClient::SendChat(const std::string& AccountId, const std::string& Message)
    {
        Fortnite.SendChat(Xmpp::UserJid(AccountId, Domain), Message);
    }

    void FriendsClient::SendPresenceEOS()
    {
        std::lock_guard Guard(PresenceMtx);
        if (!EOSConnectionId.empty()) {
            EOSClient.SetPresence(EOSConnectionId, CachedPresence);
        }
    }

    void FriendsClient::SendPresenceLauncher()
    {
        std::lock_guard Guard(PresenceMtx);
        if (!LauncherConnectionId.empty()) {
            LauncherClient.UpdatePresenceStatus("_", LauncherConnectionId, CachedPresence.Status);
        }
    }

    void FriendsClient::SendPresenceFortnite()
    {
        std::lock_guard Guard(PresenceMtx);
        if (XmppConnected) {
            rapidjson::StringBuffer StatusBuffer;
            {
                rapidjson::Writer<rapidjson::StringBuffer> Writer(StatusBuffer);

                Writer.StartObject();

                Writer.Key("Status");
                Writer.String(CachedPresence.Activity.c_str(), CachedPresence.Activity.size());

                Writer.Key("bIsPlaying");
                Writer.Bool(false);

                Writer.Key("bIsJoinable");
                Writer.Bool(false);

                Writer.Key("bHasVoiceSupport");
                Writer.Bool(false);

                Writer.Key("ProductName");
                Writer.String("EGL3");

                Writer.Key("SessionId");
                Writer.String("");

                Writer.Key("Properties");
                Writer.StartObject();
                Writer.EndObject();

                Writer.EndObject();
            }

            Xmpp::Presence XmppPresence{
                .Available = true,
                .Status = Xmpp::StringToStatus(CachedPresence.Status),
                .StatusText = std::string(StatusBuffer.GetString(), StatusBuffer.GetSize()),
                .Timestamp = TimePoint::clock::now()
            };
            Fortnite.SendPresence(XmppPresence);
        }
    }

    bool GetFriendId(const JsonObject& Payload, const std::string& AccountId, std::string& FriendId, bool& IsOutbound)
    {
        auto Itr = Payload.FindMember("from");
        if (Itr == Payload.MemberEnd() || !Itr->value.IsString()) {
            return false;
        }
        std::string From(Itr->value.GetString(), Itr->value.GetStringLength());

        Itr = Payload.FindMember("to");
        if (Itr == Payload.MemberEnd() || !Itr->value.IsString()) {
            return false;
        }
        std::string To(Itr->value.GetString(), Itr->value.GetStringLength());

        IsOutbound = AccountId == From;
        if (IsOutbound) {
            FriendId = To;
        }
        else {
            FriendId = From;
        }
        return true;
    }

    template<bool IsEOS>
    void FriendsClient::ReadStompMessage(const Stomp::Frame& Frame)
    {
        auto& Headers = Frame.GetHeaders();
        auto ContentTypeItr = Headers.find("Content-Type");
        if (ContentTypeItr == Headers.end()) {
            return;
        }
        if (ContentTypeItr->second != "application/json") {
            return;
        }

        auto Document = Web::ParseJson(Frame.GetBody());
        if (Document.HasParseError()) {
            return;
        }

        Stomp::Messages::Message Message;
        if (!Stomp::Messages::Message::Parse(Document, Message)) {
            return;
        }

        if constexpr (IsEOS) {
            switch (Utils::Crc32(Message.Type))
            {
            case Utils::Crc32("core.connect.v1.connected"):
            {
                {
                    std::lock_guard Guard(PresenceMtx);
                    EOSConnectionId = Message.ConnectionId;
                }
                SendPresenceEOS();
                EGL3_LOG(LogLevel::Info, "EOS initialized");
                break;
            }
            case Utils::Crc32("friends.v1.FRIEND_ENTRY_ADDED"):
            case Utils::Crc32("friends.v1.FRIEND_ENTRY_REMOVED"):
            case Utils::Crc32("friends.v1.FRIEND_ENTRY_UPDATED"):
            case Utils::Crc32("friends.v1.FRIEND_INVITE_ENTRY_ADDED"):
            case Utils::Crc32("friends.v1.FRIEND_INVITE_ENTRY_REMOVED"):
            case Utils::Crc32("friends.v1.BLOCK_LIST_ENTRY_ADDED"):
            case Utils::Crc32("friends.v1.BLOCK_LIST_ENTRY_REMOVED"):
            case Utils::Crc32("party.v0.PING"):
            case Utils::Crc32("party.v0.INITIAL_INVITE"):
            case Utils::Crc32("presence.v1.UPDATE"):
                break;
            default:
                EGL3_LOGF(LogLevel::Warning, "Unhandled EOS {} message - {}", Message.Type, Frame.GetBody());
                break;
            }
        }
        else {
            switch (Utils::Crc32(Message.Type))
            {
            case Utils::Crc32("core.connect.v1.connected"):
            {
                {
                    std::lock_guard Guard(PresenceMtx);
                    LauncherConnectionId = Message.ConnectionId;
                }
                SendPresenceLauncher();
                EGL3_LOG(LogLevel::Info, "Launcher initialized");
                break;
            }
            case Utils::Crc32("presence.v1.UPDATE"):
            {
                if (!Message.Payload.has_value()) {
                    break;
                }
                Stomp::Messages::Presence Presence;
                if (!Stomp::Messages::Presence::Parse(Message.Payload.value(), Presence)) {
                    break;
                }
                OnPresenceUpdate(Presence);
                break;
            }
            case Utils::Crc32("friends.v1.FRIEND_ENTRY_ADDED"):
            {
                if (!Message.Payload.has_value()) {
                    return;
                }

                std::string AccountId;
                bool IsOutbound;
                if (!GetFriendId(Message.Payload.value(), LauncherClient.GetAuthData().AccountId.value(), AccountId, IsOutbound)) {
                    return;
                }

                OnFriendEvent(AccountId, FriendEventType::Added);
                break;
            }
            case Utils::Crc32("friends.v1.FRIEND_ENTRY_REMOVED"):
            case Utils::Crc32("friends.v1.FRIEND_INVITE_ENTRY_REMOVED"):
            {
                if (!Message.Payload.has_value()) {
                    return;
                }

                std::string AccountId;
                bool IsOutbound;
                if (!GetFriendId(Message.Payload.value(), LauncherClient.GetAuthData().AccountId.value(), AccountId, IsOutbound)) {
                    return;
                }

                OnFriendEvent(AccountId, FriendEventType::Removed);
                break;
            }
            case Utils::Crc32("friends.v1.FRIEND_INVITE_ENTRY_ADDED"):
            {
                if (!Message.Payload.has_value()) {
                    return;
                }

                std::string AccountId;
                bool IsOutbound;
                if (!GetFriendId(Message.Payload.value(), LauncherClient.GetAuthData().AccountId.value(), AccountId, IsOutbound)) {
                    return;
                }

                OnFriendEvent(AccountId, IsOutbound ? FriendEventType::RequestOutbound : FriendEventType::RequestInbound);
                break;
            }
            case Utils::Crc32("friends.v1.FRIEND_ENTRY_UPDATED"):
            {
                if (!Message.Payload.has_value()) {
                    return;
                }

                auto Itr = Message.Payload->FindMember("friendId");
                if (Itr == Message.Payload->MemberEnd() || !Itr->value.IsString()) {
                    return;
                }

                OnFriendEvent(std::string(Itr->value.GetString(), Itr->value.GetStringLength()), FriendEventType::Updated);
                break;
            }
            case Utils::Crc32("friends.v1.BLOCK_LIST_ENTRY_ADDED"):
            {
                if (!Message.Payload.has_value()) {
                    return;
                }

                auto Itr = Message.Payload->FindMember("accountId");
                if (Itr == Message.Payload->MemberEnd() || !Itr->value.IsString()) {
                    return;
                }

                OnFriendEvent(std::string(Itr->value.GetString(), Itr->value.GetStringLength()), FriendEventType::Blocked);
                break;
            }
            case Utils::Crc32("friends.v1.BLOCK_LIST_ENTRY_REMOVED"):
            {
                if (!Message.Payload.has_value()) {
                    return;
                }

                auto Itr = Message.Payload->FindMember("accountId");
                if (Itr == Message.Payload->MemberEnd() || !Itr->value.IsString()) {
                    return;
                }

                OnFriendEvent(std::string(Itr->value.GetString(), Itr->value.GetStringLength()), FriendEventType::Unblocked);
                break;
            }
            case Utils::Crc32("party.v0.PING"):
                // First time request (with a INITIAL_INVITE following afterwards): {"correlationId":"FN-uCNRCvGAW06uje2ztiopAA","timestamp":1625272245446,"id":"CONNECT-74ebd24d-3180-4d69-9513-b1b4cb63d466","connectionId":"0242acfffe110002-00000001-00ccffb8-e79d1617f4c0452c-4dfa5c86","payload":{"sent":"2021-07-03T00:30:45.445Z","ns":"Fortnite","pinger_id":"9baf4e2cbd4947d4bd0f2dcde49058b8","pinger_dn":"Asriel_Dev","expires":"2021-07-03T01:15:45.445Z","meta":{"urn:epic:conn:type_s":"game","urn:epic:member:dn_s":"Asriel_Dev","urn:epic:conn:platform_s":"WIN","urn:epic:cfg:build-id_s":"1:3:16635603","urn:epic:invite:platformdata_s":""}},"type":"party.v0.PING"}
                // Subsequent requests (xmpp sees a interaction of PingSent): {"correlationId":"FN-B8-Iz7uQM06qFZsAXIY4yA","timestamp":1625272419816,"id":"CONNECT-60409364-fcd3-46fd-99e0-0ab4117338aa","connectionId":"0242acfffe110002-00000001-00ccffb8-e79d1617f4c0452c-4dfa5c86","payload":{"sent":"2021-07-03T00:33:39.815Z","ns":"Fortnite","pinger_id":"9baf4e2cbd4947d4bd0f2dcde49058b8","pinger_dn":"Asriel_Dev","expires":"2021-07-03T01:18:39.815Z","meta":{"urn:epic:conn:platform_s":"WIN","urn:epic:invite:platformdata_s":""}},"type":"party.v0.PING"}
                EGL3_LOGF(LogLevel::Info, "Party ping");
                break;
            case Utils::Crc32("party.v0.INITIAL_INVITE"):
                // Party invite (only appears once, xmpp sees PartyInviteSent interaction): {"correlationId":"FN-h5ml53Y92QiR46KN1S8w4A","timestamp":1625247770211,"id":"CONNECT-38072861-67b6-4fb6-94ac-ddded1f9b9b3","connectionId":"0242acfffe110002-00000001-00c50b0c-ee8e1adc6f013eac-82d650f5","payload":{"sent":"2021-07-02T17:42:50.210Z","ns":"Fortnite","pinger_id":"674f910f41b14a32a205687b4d9d6088","pinger_dn":"DriftyD_","pinger_pl":"psn","pinger_pl_dn":"Drifterrz543benn","expires":"2021-07-02T18:27:50.210Z","meta":{"urn:epic:conn:platform_s":"PSN","urn:epic:invite:platformdata_s":""}},"type":"party.v0.PING"}
                EGL3_LOGF(LogLevel::Info, "Party invite");
                break;
            default:
                EGL3_LOGF(LogLevel::Warning, "Unhandled launcher {} message - {}", Message.Type, Frame.GetBody());
                break;
            }
        }
    }

    template<FriendEventType Event>
    void FriendsClient::ReadFriendEventPayload(const Stomp::Messages::Message& Message)
    {
        if (!Message.Payload.has_value()) {
            return;
        }

        std::string AccountId;
        switch (Event)
        {
        case FriendEventType::Added:
        case FriendEventType::Removed:
        case FriendEventType::RequestInbound:
        case FriendEventType::RequestOutbound:
            break;
        case FriendEventType::Updated:
            break;
        case FriendEventType::Blocked:
            break;
        case FriendEventType::Unblocked:
            break;
        default:
            break;
        }
    }

    EpicClientEOS FriendsClient::GetEOSFromLauncher(EpicClientAuthed& LauncherClient)
    {
        EpicClient EpicClient;

        auto CsrfResp = EpicClient.GetCsrfToken();
        EGL3_VERIFY(!CsrfResp.HasError(), "Could not get csrf token");

        auto AuthorizeResp = LauncherClient.AuthorizeEOSClient(ClientIdEOS, { "basic_profile", "friends_list", "presence" }, CsrfResp.Get());
        EGL3_VERIFY(!AuthorizeResp.HasError(), "Could not authorize EOS client");

        auto CodeResp = LauncherClient.GetExchangeCode();
        EGL3_VERIFY(!CodeResp.HasError(), "Could not get exchange code from launcher");

        auto AuthResp = EpicClient.ExchangeCodeEOS(AuthClientEOS, DeploymentIdEOS, CodeResp->Code);
        EGL3_VERIFY(!AuthResp.HasError(), "Could not authorize from launcher exchange code");

        return EpicClientEOS(AuthResp.Get(), AuthClientEOS, DeploymentIdEOS);
    }
}