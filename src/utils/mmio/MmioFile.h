#pragma once

#include "MmioFileReadonly.h"

namespace EGL3::Utils::Mmio {
    // Read/write memory mapped file
    class MmioFile : public MmioFileReadonly {
    public:
        MmioFile(const std::filesystem::path& FilePath);

        MmioFile(const char* FilePath);

        char* Get();

        void EnsureSize(size_t Size);

        void Flush();
    };
}