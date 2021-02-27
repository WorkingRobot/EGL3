#include "Service.h"

namespace EGL3::Modules::Game {
    ServiceModule::ServiceModule() :
        Client(Service::Pipe::PipeName)
    {
        
    }

    Service::Pipe::Client& ServiceModule::GetClient()
    {
        return Client;
    }
}