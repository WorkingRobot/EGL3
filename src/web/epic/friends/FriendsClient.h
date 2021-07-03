#pragma once

#include "../../../utils/Callback.h"
#include "../../stomp/messages/Message.h"
#include "../../stomp/messages/Presence.h"
#include "../../stomp/StompClient.h"
#include "../../xmpp/XmppClient.h"
#include "../EpicClientAuthed.h"
#include "../EpicClientEOS.h"
#include "UserPresence.h"

namespace EGL3::Web::Epic::Friends {
    using Presence = Stomp::Messages::Presence;

    enum class FriendEventType : uint8_t {
        Added,
        RequestInbound,
        RequestOutbound,
        Removed,
        Blocked,
        Unblocked,
        Updated
    };

    // Online connections:
    //  STOMP - Launcher - Recieve all presences and notifications
    //  STOMP - EOS Game - Send customized presence to launcher
    //  XMPP  - Fortnite - Send chat and presence to Fortnite users
    class FriendsClient {
    public:
        FriendsClient(EpicClientAuthed& LauncherClient, EpicClientAuthed& FortniteClient, const UserPresence& Presence);

        ~FriendsClient();

        void SetPresence(const UserPresence& NewPresence);

        void SendChat(const std::string& AccountId, const std::string& Message);

        Utils::Callback<void(const Presence& Presence)> OnPresenceUpdate;
        Utils::Callback<void(const std::string& AccountId, const std::string& Message)> OnChatReceived;
        Utils::Callback<void(const std::string& AccountId, FriendEventType Event)> OnFriendEvent;

    private:
        void SendPresenceEOS();
        void SendPresenceLauncher();
        void SendPresenceFortnite();

        template<bool IsEOS>
        void ReadStompMessage(const Stomp::Frame& Frame);

        template<FriendEventType Event>
        void ReadFriendEventPayload(const Stomp::Messages::Message& Message);

        static EpicClientEOS GetEOSFromLauncher(EpicClientAuthed& LauncherClient);

        EpicClientAuthed& LauncherClient;
        EpicClientAuthed& FortniteClient;
        EpicClientEOS EOSClient;

        Stomp::StompClient Launcher;
        Stomp::StompClient EOS;
        Xmpp::XmppClient Fortnite;

        std::mutex PresenceMtx;
        UserPresence CachedPresence;
        std::string EOSConnectionId;
        std::string LauncherConnectionId;
        bool XmppConnected;
    };
}