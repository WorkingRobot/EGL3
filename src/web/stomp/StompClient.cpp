#include "StompClient.h"

#include "../../utils/Log.h"
#include <charconv>

namespace EGL3::Web::Stomp {
    StompClient::StompClient(const std::string& Url, const std::string& AuthToken) :
        PingInterval(30000),
        PongInterval(0),
        HeartbeatClosed(false)
    {
        Socket.setUrl(Url);
        Socket.addSubProtocol("v10.stomp");
        Socket.addSubProtocol("v11.stomp");
        Socket.addSubProtocol("v12.stomp");
        Socket.disableAutomaticReconnection();

        if (!AuthToken.empty()) {
            Socket.setExtraHeaders({ {"Authorization", AuthToken } });
        }

        Socket.setOnMessageCallback([this](auto& Message) { ReceivedMessage(Message); });
    }

    StompClient::~StompClient()
    {
        Stop();
    }

    void StompClient::Start()
    {
        Socket.start();
    }

    void StompClient::Stop()
    {
        {
            std::lock_guard Guard(HeartbeatMtx);
            HeartbeatClosed = true;
        }
        HeartbeatCV.notify_all();

        Socket.stop();
    }

    void StompClient::Subscribe(const std::string& Id, const std::string& Destination)
    {
        SendFrame({ CommandType::Subscribe, { { "id", Id }, { "destination", Destination } }, "" });
    }

    void StompClient::ReceivedMessage(const ix::WebSocketMessagePtr& Message)
    {
        switch (Message->type)
        {
        case ix::WebSocketMessageType::Message:
            //EGL3_LOGF(LogLevel::Debug, "[STOMP] RECV - {}", Message->str);
            RecievedFrame(Frame(Message->str));
            break;
        case ix::WebSocketMessageType::Open:
            SendConnect();
            break;
        case ix::WebSocketMessageType::Close:
            OnDisconnected();
            {
                std::lock_guard Guard(HeartbeatMtx);
                HeartbeatClosed = true;
            }
            HeartbeatCV.notify_all();
            if (HeartbeatTask.valid()) {
                HeartbeatTask.wait();
            }
            break;
        }
    }

    void StompClient::RecievedFrame(const Frame& Frame)
    {
        auto& Headers = Frame.GetHeaders();
        switch (Frame.GetCommand()) {
        case CommandType::Connected:
        {
            auto HeartbeatItr = Headers.find("heart-beat");
            if (HeartbeatItr != Headers.end()) {
                std::string_view Intervals = HeartbeatItr->second;
                auto CommaIdx = Intervals.find(',');
                if (CommaIdx != std::string_view::npos) {
                    std::chrono::milliseconds::rep ServerPing = 0, ServerPong = 0;
                    std::from_chars(Intervals.data(), Intervals.data() + CommaIdx, ServerPong);
                    std::from_chars(Intervals.data() + CommaIdx + 1, Intervals.data() + Intervals.size(), ServerPing);

                    std::chrono::milliseconds::rep ClientPing = PingInterval.count(), ClientPong = PongInterval.count();

                    PingInterval = std::chrono::milliseconds((ServerPing == 0 || ClientPing == 0) ? 0 : std::max(ServerPing, ClientPing));
                    PongInterval = std::chrono::milliseconds((ServerPong == 0 || ClientPong == 0) ? 0 : std::max(ServerPong, ClientPong));
                }
            }
            else {
                PingInterval = std::chrono::milliseconds(0);
                PongInterval = std::chrono::milliseconds(0);
            }

            auto VersionItr = Headers.find("version");
            if (VersionItr != Headers.end()) {
                // printf("version %s\n", VersionItr->second.c_str());
            }

            OnConnected();
            HeartbeatTask = std::async(std::launch::async, [this]() { RunHeartbeatTask(); });
            break;
        }
        case CommandType::Message:
        {
            auto SubscriptionItr = Headers.find("subscription");
            if (SubscriptionItr != Headers.end()) {
                OnMessage(SubscriptionItr->second, Frame);
            }
            break;
        }
        case CommandType::Error:
            printf("error\n");
            break;
        case CommandType::Heartbeat:
            printf("heartbeat\n");
            break;
        default:
            printf("unknown command\n");
            break;
        }
    }

    void StompClient::RunHeartbeatTask()
    {
        std::unique_lock Lock(HeartbeatMtx);
        while (!HeartbeatCV.wait_for(Lock, PingInterval, [this]() { return HeartbeatClosed; })) {
            SendPing();
        }
    }

    void StompClient::SendConnect()
    {
        SendFrame({ CommandType::Connect, { { "accept-version", "1.0,1.1,1.2" }, { "heartbeat", std::format("{},{}", PingInterval, PongInterval) } }, "" });
    }

    void StompClient::SendPing()
    {
        SendFrame({ CommandType::Heartbeat, { }, "" });
    }

    void StompClient::SendFrame(const Frame& Frame)
    {
        auto Data = Frame.Encode();
        // EGL3_LOGF(LogLevel::Debug, "[STOMP] SEND - {}", Data);
        Socket.sendText(Data);
    }
}