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
        MmioFile(bool Readonly);

    public:
        MmioFile();

        MmioFile(const std::filesystem::path& FilePath, Detail::ERead);
        MmioFile(const std::filesystem::path& FilePath, Detail::EWrite);

        MmioFile(const char* FilePath, Detail::ERead);
        MmioFile(const char* FilePath, Detail::EWrite);

        MmioFile(const MmioFile&) = delete;

        MmioFile(MmioFile&&) noexcept;

        ~MmioFile();

        bool IsValid() const {
            return BaseAddress;
        }

        bool IsReadonly() const {
            return Readonly;
        }

        const char* Get() const {
            return (char*)BaseAddress;
        }

        char* Get() {
            return (char*)BaseAddress;
        }

        size_t Size() const {
            return SectionSize;
        }

        bool IsValidPosition(size_t Position) const {
            return Size() > Position;
        }

        void EnsureSize(size_t Size);

        void Flush(size_t Position, size_t Size);

        void Flush();

        static constexpr uint64_t DownloadWorkingSize = 512ull * 1024 * 1024;
        static constexpr uint64_t PlayWorkingSize = 2ull * 1024 * 1024 * 1024;
        static bool SetWorkingSize(uint64_t MaxBytes, uint64_t MinBytes = 1024 * 1024 * 64);

    private:
        const bool Readonly;
        MM_HANDLE HProcess;
        MM_HANDLE HSection;
        MM_PVOID BaseAddress;
        MM_LARGE_INTEGER SectionSize;
        MM_SIZE_T ViewSize;

        static constexpr MM_SIZE_T InitialViewSize = 1ull << 37; // 128 gb
        static constexpr MM_SIZE_T ViewSizeIncrement = 1ull << 37; // 128 gb
    };
}