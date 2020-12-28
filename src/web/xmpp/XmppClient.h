#pragma once

#include "Presence.h"
#include "messages/SystemMessage.h"

#include <ixwebsocket/IXWebSocket.h>
#include <rapidxml/rapidxml.hpp>
#include <shared_mutex>

namespace EGL3::Web::Xmpp {
    struct Callbacks {
        std::function<void(const std::string& AccountId, Json::Presence&& NewPresence)> PresenceUpdate;
        std::function<void(Messages::SystemMessage&& NewMessage)> SystemMessage;

        void Clear() {
            PresenceUpdate = [](auto, auto) {};
            SystemMessage = [](auto) {};
        }
    };

    // Standards used are listed in comments inside the cpp file and in asserts
    // TODO: Create constant to easily switch environments (currently only in prod)
    class XmppClient {
    public:
        XmppClient(const std::string& AccountId, const std::string& AccessToken, const Callbacks& Callbacks);

        void SetPresence(const Json::Presence& NewPresence);

        ~XmppClient();

    private:
        bool HandlePresence(const rapidxml::xml_node<>* Node);

        bool HandleSystemMessage(const rapidjson::Document& Data);

        bool HandleSystemMessage(const rapidxml::xml_node<>* Node);

        bool HandleChat(const rapidxml::xml_node<>* Node);

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

        Callbacks Callbacks;
        std::atomic<bool> PresenceSendable;

        ClientState State;
        std::string EncodedAuthValue;
        std::string CurrentResource;
        std::string CurrentAccountId;
        std::string CurrentJidWithoutResource;
        std::string CurrentJid;

        std::shared_mutex BackgroundPingMutex;
        std::condition_variable_any BackgroundPingCV;
        std::chrono::steady_clock::time_point BackgroundPingNextTime;
        std::future<void> BackgroundPingFuture;

        ix::WebSocket Socket;
    };
}
