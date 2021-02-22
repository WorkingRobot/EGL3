#pragma once

#include "../../utils/Callback.h"

#include <vector>

namespace EGL3::Storage::Models {
    struct MountedFile {
        std::string Path;
        uint64_t FileSize;
        void* UserContext;
    };

    class MountedDisk {
    public:
        MountedDisk(const std::vector<MountedFile>& Files, uint32_t DiskSignature);

        MountedDisk(const MountedDisk&) = delete;

        ~MountedDisk();

        char GetDriveLetter();

        void Initialize();
        
        void Create();

        void Mount(uint32_t LogFlags = 0);

        void Unmount();

        Utils::Callback<void(void*, uint64_t, uint8_t[4096])> HandleFileCluster;

    private:
        void* PrivateData;
    };
}