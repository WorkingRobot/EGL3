#pragma once

#include "Client.h"

namespace EGL3::Service::Pipe {
    class ServicedMount {
    public:
        ServicedMount() = default;
        ServicedMount(Client& Client, const std::filesystem::path& Path);

        ~ServicedMount();

        explicit operator bool() const noexcept;

        bool IsMounted() const;

        const std::filesystem::path& GetMountPath();

        // GetMountInfo();

        void Unmount();

    private:
        Client* Client;
        MountHandle Handle;
        std::filesystem::path MountPath;
    };
}