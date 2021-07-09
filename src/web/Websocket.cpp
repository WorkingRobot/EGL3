#include "Websocket.h"

#include <ixwebsocket/IXNetSystem.h>

namespace EGL3::Web::Websocket {
    void Initialize()
    {
        ix::initNetSystem();
    }

    void Uninitialize()
    {
        ix::uninitNetSystem();
    }
}