#include "ServicedMount.h"

namespace EGL3::Service::Pipe {
    ServicedMount::ServicedMount(Pipe::Client& Client, const std::filesystem::path& Path) :
        Client(&Client),
        Handle()
    {
        if (Client.IsConnected()) {
            Client.BeginMount(Path, Handle);
        }
    }

    ServicedMount::~ServicedMount()
    {
        Unmount();
    }

    ServicedMount::operator bool() const noexcept
    {
        return IsMounted();
    }

    bool ServicedMount::IsMounted() const
    {
        return Client && Handle;
    }

    const std::filesystem::path& ServicedMount::GetMountPath()
    {
        if (MountPath.empty()) {
            if (IsMounted()) {
                Client->GetMountPath(Handle, MountPath);
            }
        }

        return MountPath;
    }

    void ServicedMount::Unmount()
    {
        if (IsMounted()) {
            Client->EndMount(Handle);
            Client = nullptr;
            Handle = nullptr;
        }
    }
}