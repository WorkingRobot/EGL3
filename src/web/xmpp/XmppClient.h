#pragma once

#include "../../utils/Callback.h"
#include "Presence.h"
#include "messages/SystemMessage.h"

#include <future>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <ixwebsocket/IXWebSocket.h>
#include <rapidxml/rapidxml.hpp>

namespace EGL3::Web::Xmpp {
    // Standards used are listed in comments inside the cpp file and in asserts
    // TODO: Create constant to easily switch environments (currently only in prod)
    // TODO: HANDLE PARTY REQESTS
    // <message id="F3BA65454F1533372F6FCBA8F03DD65D" from="d4d0142cc75141ff841b1b41b40bed3a@prod.ol.epicgames.com/V2:Fortnite:WIN::FA7BAD364934BD9B8CF445BD31D81622" xmlns="jabber:client" to="9baf4e2cbd4947d4bd0f2dcde49058b8@prod.ol.epicgames.com/V2:launcher:WIN::00E4DD674A943E94A35A2DB8D6B0F5E9"><body>{"type":"com.epicgames.party.invitation","payload":{"partyId":"c0169f5a67784d71ab4a0468ceb2c139","partyTypeId":286331153,"displayName":"Jawschamp","accessKey":"k","appId":"Fortnite","buildId":"15555257"},"timestamp":"2021-03-19T18:47:58.317Z"}</body></message>
    // <message id="2d085420-4db7-4196-b5eb-61c1cd41ef60" from="xmpp-admin@prod.ol.epicgames.com" corr-id="ITS-IHYXmPPLw1cXw6nCyDR_VQ" xmlns="jabber:client" to="9baf4e2cbd4947d4bd0f2dcde49058b8@prod.ol.epicgames.com/V2:launcher:WIN::00E4DD674A943E94A35A2DB8D6B0F5E9"><body>{"type":"com.epicgames.social.interactions.notification.v1","interactions":[{"fromAccountId":"d4d0142cc75141ff841b1b41b40bed3a","toAccountId":"9baf4e2cbd4947d4bd0f2dcde49058b8","interactionType":"PingSent","happenedAt":1616179678140,"app":null,"namespace":"Fortnite"}]}</body></message>
    // <message id="fc19d21a-4bed-48a9-a407-511c32d19de6" from="xmpp-admin@prod.ol.epicgames.com" corr-id="ITS-YCbVu3ur4Gkm0itmby-ngg" xmlns="jabber:client" to="9baf4e2cbd4947d4bd0f2dcde49058b8@prod.ol.epicgames.com/V2:launcher:WIN::00E4DD674A943E94A35A2DB8D6B0F5E9"><body>{"type":"com.epicgames.social.interactions.notification.v1","interactions":[{"fromAccountId":"d4d0142cc75141ff841b1b41b40bed3a","toAccountId":"9baf4e2cbd4947d4bd0f2dcde49058b8","interactionType":"PartyInviteSent","happenedAt":1616179678140,"app":null,"namespace":"Fortnite"}]}</body></message>
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

        bool HandlePong(const rapidxml::xml_node<>* Node);

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

        std::mutex BackgroundPingMutex;
        std::condition_variable BackgroundPingCV;
        std::chrono::steady_clock::time_point BackgroundPingNextTime;
        std::future<void> BackgroundPingFuture;

        ix::WebSocket Socket;
    };
}
