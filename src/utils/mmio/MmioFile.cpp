#include "MmioFile.h"

#include "ntdll.h"

namespace EGL3::Utils::Mmio {
    MmioFile::MmioFile(const fs::path& FilePath) :
        MmioFile(FilePath.string().c_str())
    {

    }

    MmioFile::MmioFile(const char* FilePath) :
        BaseAddress(NULL),
        HSection(NULL),
        HProcess(GetCurrentProcess()),
        SectionSize()
    {
        HANDLE HFile = CreateFile(FilePath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (HFile != INVALID_HANDLE_VALUE) {
            SYSTEM_INFO Info;
            GetSystemInfo(&Info);
            LARGE_INTEGER SectionSize = { Info.dwPageSize };

            auto Status = NtCreateSection(&HSection, SECTION_EXTEND_SIZE | SECTION_MAP_READ | SECTION_MAP_WRITE,
                NULL, &SectionSize, PAGE_READWRITE, SEC_COMMIT, HFile);

            CloseHandle(HFile);
            
            if (0 <= Status) {
                SIZE_T ViewSize = 0;

                Status = NtMapViewOfSection(HSection, HProcess, &BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE);

                if (0 <= Status) {
                    SectionSize.QuadPart = ViewSize;
                }
            }
        }
    }

    MmioFile::~MmioFile()
    {
        if (BaseAddress) {
            Flush();
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

    char* MmioFile::Get() const {
        return (char*)BaseAddress;
    }

    size_t MmioFile::Size() const {
        return SectionSize;
    }

    bool MmioFile::IsValidOffset(size_t Offset) const
    {
        return Size() > Offset;
    }

    void MmioFile::EnsureSize(size_t Size)
    {
        if (SectionSize < Size) {
            SectionSize = Size;
            NtExtendSection(HSection, (PLARGE_INTEGER)&SectionSize);
        }
    }

    void MmioFile::Flush()
    {
        SIZE_T FlushSize = 0;
        PVOID FlushAddr = BaseAddress;
        IO_STATUS_BLOCK Block;
        NtFlushVirtualMemory(HProcess, &FlushAddr, &FlushSize, &Block);
    }


    MmioReadonlyFile::MmioReadonlyFile(const fs::path& FilePath) : MmioReadonlyFile(FilePath.string().c_str()) {}

    MmioReadonlyFile::MmioReadonlyFile(const char* FilePath) :
        HProcess(GetCurrentProcess()),
        HSection(NULL),
        BaseAddress(NULL),
        FileSize(0)
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
                Status = NtMapViewOfSection(HSection, HProcess, (PVOID*)&BaseAddress, 0, 0, NULL,
                    &FileSize, ViewUnmap, 0, PAGE_READONLY);
            }
        }
    }

    MmioReadonlyFile::~MmioReadonlyFile()
    {
        if (BaseAddress) {
            NtUnmapViewOfSection(HProcess, BaseAddress);
        }
        if (HSection) {
            NtClose(HSection);
        }
    }

    bool MmioReadonlyFile::Valid() const
    {
        return BaseAddress;
    }

}