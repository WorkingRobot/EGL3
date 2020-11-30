#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <rapidxml/rapidxml.hpp>

#include "../BaseClient.h"

namespace EGL3::Web::Xmpp {
    // Standards used are listed in comments inside the cpp file and in asserts
    // TODO: Create constant to easily switch environments (currently only in prod)
    class XmppClient : public BaseClient {
    public:
        XmppClient(const std::string& AccountId, const std::string& AccessToken);

    protected:
        void KillAuthentication() override;

    private:
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

        ClientState State;
        std::string EncodedAuthValue;
        std::string CurrentResource;
        std::string CurrentJid;

        std::atomic<std::chrono::steady_clock::time_point> BackgroundPingNextTime;
        std::future<void> BackgroundPingFuture;

        ix::WebSocket Socket;
    };
}
