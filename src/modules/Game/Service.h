#pragma once

#include "../../srv/sock/client/Client.h"
#include "../BaseModule.h"

namespace EGL3::Modules::Game {
    class ServiceModule : public BaseModule {
    public:
        ServiceModule();

        Service::Sock::Client& GetClient();

    private:
        int PatchService();

        Service::Sock::Client Client;
    };
}