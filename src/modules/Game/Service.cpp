#include "Service.h"

namespace EGL3::Modules::Game {
    ServiceModule::ServiceModule()
    {
        EGL3_CONDITIONAL_LOG(Client.IsConnected(), LogLevel::Critical, "Could not connect to service server");

        uint32_t ProtocolVersion, MountedDiskCount;
        EGL3_CONDITIONAL_LOG(Client.QueryServer(ProtocolVersion, MountedDiskCount), LogLevel::Critical, "Could not query service server");
        EGL3_CONDITIONAL_LOG(ProtocolVersion == Service::Pipe::ProtocolVersion, LogLevel::Critical, "Service server protocol version does not match");
    }

    Service::Pipe::Client& ServiceModule::GetClient()
    {
        return Client;
    }
}