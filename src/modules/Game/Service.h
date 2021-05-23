#pragma once

#include "../../srv/pipe/client/Client.h"
#include "../ModuleList.h"

namespace EGL3::Modules::Game {
    class ServiceModule : public BaseModule {
    public:
        ServiceModule(ModuleList& Ctx);

        Service::Pipe::Client& GetClient();

    private:
        int PatchService();

        Service::Pipe::Client Client;
    };
}