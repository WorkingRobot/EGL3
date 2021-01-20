#pragma once

#include <filesystem>

namespace EGL3::Utils::Mmio {
    typedef void* MM_HANDLE;
    typedef void* MM_PVOID;
    typedef long long MM_LARGE_INTEGER;
    typedef size_t MM_SIZE_T;

    // Read/write memory mapped file
    class MmioFileReadonly {
    public:
        MmioFileReadonly(const std::filesystem::path& FilePath);

        MmioFileReadonly(const char* FilePath);

        MmioFileReadonly(const MmioFileReadonly&) = delete;

        MmioFileReadonly(MmioFileReadonly&&) noexcept;

        ~MmioFileReadonly();

        bool IsValid() const;

        const char* Get() const;

        size_t Size() const;

        bool IsValidPosition(size_t Position) const;

    protected:
        MmioFileReadonly();

        MM_HANDLE HProcess;
        MM_HANDLE HSection;
        MM_PVOID BaseAddress;
        MM_LARGE_INTEGER SectionSize;
    };
}