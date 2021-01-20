#include "MmioFileReadonly.h"

#include "ntdll.h"

namespace EGL3::Utils::Mmio {
    MmioFileReadonly::MmioFileReadonly(const std::filesystem::path& FilePath) :
        MmioFileReadonly(FilePath.string().c_str())
    {

    }

    MmioFileReadonly::MmioFileReadonly(const char* FilePath) :
        MmioFileReadonly()
    {
        HANDLE HFile = CreateFile(FilePath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (HFile != INVALID_HANDLE_VALUE) {
            SYSTEM_INFO Info;
            GetSystemInfo(&Info);
            LARGE_INTEGER SectionSize = { Info.dwPageSize };

            auto Status = NtCreateSection(&HSection, SECTION_MAP_READ,
                NULL, &SectionSize, PAGE_READONLY, SEC_COMMIT, HFile);

            CloseHandle(HFile);

            if (0 <= Status) {
                SIZE_T ViewSize = 0;

                Status = NtMapViewOfSection(HSection, HProcess, (PVOID*)&BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, 0, PAGE_READONLY);

                if (0 <= Status) {
                    SectionSize.QuadPart = ViewSize;
                }
            }
        }
    }

    MmioFileReadonly::MmioFileReadonly(MmioFileReadonly&& Other) noexcept :
        HProcess(Other.HProcess),
        HSection(Other.HSection),
        BaseAddress(Other.BaseAddress),
        SectionSize(Other.SectionSize)
    {
        Other.BaseAddress = NULL;
        Other.HSection = NULL;
    }

    MmioFileReadonly::~MmioFileReadonly()
    {
        if (BaseAddress) {
            NtUnmapViewOfSection(HProcess, BaseAddress);
        }
        if (HSection) {
            NtClose(HSection);
        }
    }

    bool MmioFileReadonly::IsValid() const
    {
        return BaseAddress;
    }

    const char* MmioFileReadonly::Get() const {
        return (char*)BaseAddress;
    }

    size_t MmioFileReadonly::Size() const {
        return SectionSize;
    }

    bool MmioFileReadonly::IsValidPosition(size_t Position) const
    {
        return Size() > Position;
    }

    MmioFileReadonly::MmioFileReadonly() :
        BaseAddress(NULL),
        HSection(NULL),
        HProcess(GetCurrentProcess()),
        SectionSize()
    {
    }
}