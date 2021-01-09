#pragma once

#include "../../utils/Callback.h"
#include "Presence.h"
#include "messages/SystemMessage.h"

#include <future>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <ixwebsocket/IXWebSocket.h>
#include <rapidxml/rapidxml.hpp>
#include <shared_mutex>

namespace EGL3::Web::Xmpp {
    // Standards used are listed in comments inside the cpp file and in asserts
    // TODO: Create constant to easily switch environments (currently only in prod)
    class XmppClient {
    public:
        XmppClient(const std::string& AccountId, const std::string& AccessToken);

        void SetPresence(const Json::Presence& NewPresence);

        void SendChat(const std::string& AccountId, const std::string& Content);

        void Close();

        ~XmppClient();

        Utils::Callback<void(const std::string& AccountId, Json::Presence&& NewPresence)> PresenceUpdate;
        Utils::Callback<void(const std::string& AccountId, std::string&& NewMessage)> ChatRecieved;
        Utils::Callback<void(Messages::SystemMessage&& NewMessage)> SystemMessage;

    private:
        void CloseInternal(bool Errored);

        // Consider the authentication process finished
        void FinishAuthentication();
        
        // True doesn't mean it was a valid presence. Just that it was one so other functions don't try reading it.
        bool HandlePresence(const rapidxml::xml_node<>* Node);

        bool HandleSystemMessage(const rapidjson::Document& Data);

        bool HandleSystemMessage(const rapidxml::xml_node<>* Node);

        bool HandleChat(const rapidxml::xml_node<>* Node);

        void BackgroundPingTask();

        void ReceivedMessage(const ix::WebSocketMessagePtr& Message);

        bool ParseMessage(const rapidxml::xml_node<>* Node);

        static inline std::once_flag InitFlag;

        // A before value means a server response
        enum class ClientState : uint8_t {
            OPENING,
            BEFORE_OPEN,
            BEFORE_FEATURES,
            BEFORE_AUTH_RESPONSE,
            BEFORE_AUTHED_OPEN,
            BEFORE_AUTHED_FEATURES,
            BEFORE_AUTHED_BIND,
            BEFORE_AUTHED_SESSION,
            AUTHENTICATED,

            CLOSED,
            CLOSED_ERRORED
        };

        std::atomic<bool> Authenticated;

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
