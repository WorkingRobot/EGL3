#pragma once

#include <string>
#include <vector>

namespace EGL3::Service {
    struct MountedFile {
        std::string Path;
        uint64_t FileSize;
        void* UserContext;
    };

    class MountedDisk {
    public:
        using ClusterCallback = void (*)(void* Ctx, uint64_t LCN, uint32_t Count, uint8_t* Buffer);

        MountedDisk(const std::vector<MountedFile>& Files, uint32_t DiskSignature, ClusterCallback Callback);

        MountedDisk(const MountedDisk&) = delete;

        ~MountedDisk();

        char GetDriveLetter() const;

        void Initialize();
        
        void Create();

        void Mount(uint32_t LogFlags = 0);

        void Unmount();

    private:
        void* PrivateData;
    };
}