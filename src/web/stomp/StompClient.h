#pragma once

#include "../../utils/Callback.h"
#include "Frame.h"

#include <future>
#include <ixwebsocket/IXWebSocket.h>

namespace EGL3::Web::Stomp {
    class StompClient {
    public:
        StompClient(const std::string& Url, const std::string& AuthToken);

        ~StompClient();

        void Start();

        void Stop();

        void Subscribe(const std::string& Id, const std::string& Destination);

        Utils::Callback<void()> OnConnected;
        Utils::Callback<void()> OnDisconnected;
        Utils::Callback<void(const std::string& SubscriptionId, const Frame& Frame)> OnMessage;

    private:
        void ReceivedMessage(const ix::WebSocketMessagePtr& Message);

        void RecievedFrame(const Frame& Frame);

        void RunHeartbeatTask();

        void SendConnect();

        void SendPing();

        void SendFrame(const Frame& Frame);

        ix::WebSocket Socket;

        std::chrono::milliseconds PingInterval, PongInterval;

        std::shared_future<void> HeartbeatTask;
        std::mutex HeartbeatMtx;
        std::condition_variable HeartbeatCV;
        bool HeartbeatClosed;
    };
}