#pragma once

#include <filesystem>

namespace EGL3::Utils::Mmio {
    namespace Detail {
        struct ERead {
            explicit ERead() = default;
        };

        struct EWrite {
            explicit EWrite() = default;
        };
    }

    inline constexpr Detail::ERead OptionRead{};
    inline constexpr Detail::EWrite OptionWrite{};

    typedef void* MM_HANDLE;
    typedef void* MM_PVOID;
    typedef long long MM_LARGE_INTEGER;
    typedef size_t MM_SIZE_T;

    // Read/write memory mapped file
    class MmioFile {
    public:
        MmioFile();

        MmioFile(const std::filesystem::path& FilePath, Detail::ERead);
        MmioFile(const std::filesystem::path& FilePath, Detail::EWrite);

        MmioFile(const char* FilePath, Detail::ERead);
        MmioFile(const char* FilePath, Detail::EWrite);

        MmioFile(const MmioFile&) = delete;

        MmioFile(MmioFile&&) noexcept;

        ~MmioFile();

        bool IsValid() const;

        const char* Get() const {
            return (char*)BaseAddress;
        }

        char* Get() {
            return (char*)BaseAddress;
        }

        size_t Size() const;

        bool IsValidPosition(size_t Position) const;

        void EnsureSize(size_t Size);

        void Flush();

    private:
        MM_HANDLE HProcess;
        MM_HANDLE HSection;
        MM_PVOID BaseAddress;
        MM_LARGE_INTEGER SectionSize;
        MM_SIZE_T ViewSize;

        static constexpr MM_SIZE_T InitialViewSize = 1 << 24; // 16 mb
        static constexpr MM_SIZE_T ViewSizeIncrement = 1 << 24; // 16 mb increments
    };
}