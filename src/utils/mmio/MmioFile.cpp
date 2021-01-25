#include "MmioFile.h"

#include "../Align.h"
#include "../Assert.h"
#include "ntdll.h"

namespace EGL3::Utils::Mmio {
    MmioFile::MmioFile() :
        BaseAddress(NULL),
        HSection(NULL),
        HProcess(GetCurrentProcess()),
        SectionSize(),
        ViewSize()
    {

    }

    MmioFile::MmioFile(const std::filesystem::path& FilePath, Detail::ERead) :
        MmioFile(FilePath.string().c_str(), OptionRead)
    {

    }

    MmioFile::MmioFile(const std::filesystem::path& FilePath, Detail::EWrite) :
        MmioFile(FilePath.string().c_str(), OptionWrite)
    {

    }

    MmioFile::MmioFile(const char* FilePath, Detail::ERead) :
        MmioFile()
    {
        HANDLE HFile = CreateFile(FilePath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (HFile != INVALID_HANDLE_VALUE) {
            EGL3_CONDITIONAL_LOG(GetFileSizeEx(HFile, (PLARGE_INTEGER)&SectionSize), LogLevel::Critical, "Failed to get file size");
            EGL3_CONDITIONAL_LOG(SectionSize, LogLevel::Critical, "Trying to mmio open file without any data");

            auto Status = NtCreateSection(&HSection, SECTION_MAP_READ,
                NULL, (PLARGE_INTEGER)&SectionSize, PAGE_READONLY, SEC_COMMIT, HFile);

            CloseHandle(HFile);

            if (0 <= Status) {
                ViewSize = SectionSize;
                Status = NtMapViewOfSection(HSection, HProcess, &BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, 0, PAGE_READONLY);

                printf("%08X\n", Status);
                EGL3_CONDITIONAL_LOG(0 <= Status, LogLevel::Critical, "Failed to map file");
            }
        }
    }

    MmioFile::MmioFile(const char* FilePath, Detail::EWrite) :
        MmioFile()
    {
        HANDLE HFile = CreateFile(FilePath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (HFile != INVALID_HANDLE_VALUE) {
            EGL3_CONDITIONAL_LOG(GetFileSizeEx(HFile, (PLARGE_INTEGER)&SectionSize), LogLevel::Critical, "Failed to get file size");
            if (SectionSize == 0) {
                SectionSize = 0x1000;
            }

            auto Status = NtCreateSection(&HSection, SECTION_EXTEND_SIZE | SECTION_MAP_READ | SECTION_MAP_WRITE,
                NULL, (PLARGE_INTEGER)&SectionSize, PAGE_READWRITE, SEC_COMMIT, HFile);

            CloseHandle(HFile);

            if (0 <= Status) {
                ViewSize = InitialViewSize;
                Status = NtMapViewOfSection(HSection, HProcess, &BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE);

                EGL3_CONDITIONAL_LOG(0 <= Status, LogLevel::Critical, "Failed to map file");
            }
        }
    }

    MmioFile::MmioFile(MmioFile&& Other) noexcept :
        HProcess(Other.HProcess),
        HSection(Other.HSection),
        BaseAddress(Other.BaseAddress),
        SectionSize(Other.SectionSize),
        ViewSize(Other.ViewSize)
    {
        Other.BaseAddress = NULL;
        Other.HSection = NULL;
    }

    MmioFile::~MmioFile()
    {
        if (BaseAddress) {
            NtUnmapViewOfSection(HProcess, BaseAddress);
        }
        if (HSection) {
            NtClose(HSection);
        }
    }

    bool MmioFile::IsValid() const
    {
        return BaseAddress;
    }

    size_t MmioFile::Size() const
    {
        return SectionSize;
    }

    bool MmioFile::IsValidPosition(size_t Position) const
    {
        return Size() > Position;
    }

    void MmioFile::EnsureSize(size_t Size)
    {
        if (SectionSize < Size) {
            SectionSize = Size;
            auto Status = NtExtendSection(HSection, (PLARGE_INTEGER)&SectionSize);
            EGL3_CONDITIONAL_LOG(0 <= Status, LogLevel::Critical, "Failed to extend section");

            if (Size > ViewSize) {
                ViewSize = Utils::Align<ViewSizeIncrement>(Size);

                Status = NtUnmapViewOfSection(HProcess, BaseAddress);
                EGL3_CONDITIONAL_LOG(0 <= Status, LogLevel::Critical, "Failed to unmap view of section");

                BaseAddress = 0;
                Status = NtMapViewOfSection(HSection, HProcess, &BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE);
                EGL3_CONDITIONAL_LOG(0 <= Status, LogLevel::Critical, "Failed to remap view of section");
            }
        }
    }

    void MmioFile::Flush()
    {
        SIZE_T FlushSize = 0;
        PVOID FlushAddr = BaseAddress;
        IO_STATUS_BLOCK Block;
        NtFlushVirtualMemory(HProcess, &FlushAddr, &FlushSize, &Block);
    }
}