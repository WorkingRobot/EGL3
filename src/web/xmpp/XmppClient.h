#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <rapidxml/rapidxml.hpp>

#include "../BaseClient.h"
#include "Responses.h"

namespace EGL3::Web::Xmpp {
    // Standards used are listed in comments inside the cpp file and in asserts
    // TODO: Create constant to easily switch environments (currently only in prod)
    class XmppClient {
    public:
        XmppClient(const std::string& AccountId, const std::string& AccessToken, const std::function<void(const std::string&, Json::Presence&&)>& OnPresenceUpdate);

        void SetPresence(const Json::Presence& NewPresence);

        ~XmppClient();

    private:
        bool HandlePong(const rapidxml::xml_node<>* Node);

        bool HandlePresence(const rapidxml::xml_node<>* Node);

        void BackgroundPingTask();

        void ReceivedMessage(const ix::WebSocketMessagePtr& Message);

        void ParseMessage(const rapidxml::xml_node<>* Node);

        static inline std::once_flag InitFlag;

        // A before value means a server response
        enum class ClientState {
            OPENING,
            BEFORE_OPEN,
            BEFORE_FEATURES,
            BEFORE_AUTH_RESPONSE,
            BEFORE_AUTHED_OPEN,
            BEFORE_AUTHED_FEATURES,
            BEFORE_AUTHED_BIND,
            BEFORE_AUTHED_SESSION,
            AUTHENTICATED
        };

        std::function<void(const std::string&, Json::Presence&&)> OnPresenceUpdate;
        std::atomic<bool> PresenceSendable;

        ClientState State;
        std::string EncodedAuthValue;
        std::string CurrentResource;
        std::string CurrentJidWithoutResource;
        std::string CurrentJid;

        std::atomic<std::chrono::steady_clock::time_point> BackgroundPingNextTime;
        // Better safe than sorry!
        std::mutex BackgroundPingIdMutex;
        std::string BackgroundPingId;
        std::future<void> BackgroundPingFuture;

        ix::WebSocket Socket;
    };
}
