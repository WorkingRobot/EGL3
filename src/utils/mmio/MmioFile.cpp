#include "MmioFile.h"

#include "ntdll.h"

namespace EGL3::Utils::Mmio {
    MmioFile::MmioFile(const std::filesystem::path& FilePath) :
        MmioFile(FilePath.string().c_str())
    {

    }

    MmioFile::MmioFile(const char* FilePath) :
        MmioFileReadonly()
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

            this->SectionSize = SectionSize.QuadPart;
        }
    }

    char* MmioFile::Get() {
        return (char*)BaseAddress;
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
}