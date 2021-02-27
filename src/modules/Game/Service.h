#pragma once

#include "../../srv/pipe/Client.h"
#include "../BaseModule.h"

namespace EGL3::Modules::Game {
    class ServiceModule : public BaseModule {
    public:
        ServiceModule();

        Service::Pipe::Client& GetClient();

    private:
        Service::Pipe::Client Client;
    };
}