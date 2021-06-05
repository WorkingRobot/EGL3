#include "MmioFile.h"

#include "../Align.h"
#include "../Log.h"
#include "../Format.h"
#include "ntdll.h"

#include <Psapi.h>

namespace EGL3::Utils::Mmio {
    MmioFile::MmioFile(bool Readonly) :
        Readonly(Readonly),
        BaseAddress(NULL),
        HSection(NULL),
        HProcess(GetCurrentProcess()),
        SectionSize(),
        ViewSize()
    {

    }

    MmioFile::MmioFile() :
        MmioFile(true)
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
        MmioFile(true)
    {
        HANDLE HFile = CreateFile(FilePath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (HFile != INVALID_HANDLE_VALUE) {
            EGL3_VERIFY(GetFileSizeEx(HFile, (PLARGE_INTEGER)&SectionSize), "Failed to get file size");
            EGL3_VERIFY(SectionSize, "Trying to mmio open file without any data");

            auto Status = NtCreateSection(&HSection, SECTION_MAP_READ,
                NULL, (PLARGE_INTEGER)&SectionSize, PAGE_READONLY, SEC_COMMIT, HFile);

            CloseHandle(HFile);

            if (0 <= Status) {
                ViewSize = SectionSize;
                Status = NtMapViewOfSection(HSection, HProcess, &BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, 0, PAGE_READONLY);

                EGL3_VERIFY(0 <= Status, "Failed to map file");
            }
        }
    }

    MmioFile::MmioFile(const char* FilePath, Detail::EWrite) :
        MmioFile(false)
    {
        HANDLE HFile = CreateFile(FilePath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (HFile != INVALID_HANDLE_VALUE) {
            EGL3_VERIFY(GetFileSizeEx(HFile, (PLARGE_INTEGER)&SectionSize), "Failed to get file size");
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

                EGL3_VERIFY(0 <= Status, "Failed to map file");
            }
        }
    }

    MmioFile::MmioFile(MmioFile&& Other) noexcept :
        Readonly(Other.Readonly),
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
            Flush();
            NtUnmapViewOfSection(HProcess, BaseAddress);
            EmptyWorkingSet(HProcess);
        }
        if (HSection) {
            NtClose(HSection);
        }
    }

    bool ResolveDevicePath(std::string& Filename) {
        char DriveLetters[512];
        if (GetLogicalDriveStrings(512 - 1, DriveLetters)) {
            char DriveName[MAX_PATH];
            char Drive[3] = " :";
            for (auto DriveLetter = DriveLetters; *DriveLetter; ++DriveLetter) {
                Drive[0] = *DriveLetter;
                if (QueryDosDevice(Drive, DriveName, MAX_PATH)) {
                    size_t DriveNameLen = strlen(DriveName);

                    if (DriveNameLen < MAX_PATH) {
                        if (Filename.starts_with({ DriveName, DriveNameLen }) && Filename[DriveNameLen] == '\\') {
                            Filename.replace(0, DriveNameLen, Drive);
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    std::filesystem::path MmioFile::GetPath() const
    {
        char Filename[MAX_PATH];
        Filename[0] = 0;
        if (GetMappedFileName(HProcess, BaseAddress, Filename, MAX_PATH)) {
            std::string FilenameString = Filename;
            if (ResolveDevicePath(FilenameString)) {
                return FilenameString;
            }
        }
        return "";
    }

    void MmioFile::EnsureSize(size_t Size)
    {
        if (SectionSize < Size) {
            SectionSize = Size;
            auto Status = NtExtendSection(HSection, (PLARGE_INTEGER)&SectionSize);
            EGL3_VERIFY(0 <= Status, "Failed to extend section");

            if (Size > ViewSize) {
                ViewSize = Utils::Align<ViewSizeIncrement>(Size);

                // By not unmapping, we prevent code that is running in other threads from encountering an access error
                //Status = NtUnmapViewOfSection(HProcess, BaseAddress);
                //EGL3_VERIFY(0 <= Status, "Failed to unmap view of section");

                BaseAddress = NULL;
                Status = NtMapViewOfSection(HSection, HProcess, &BaseAddress, 0, 0, NULL,
                    &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE);
                EGL3_VERIFY(0 <= Status, "Failed to remap view of section");
            }
        }
    }

    void MmioFile::Flush(size_t Position, size_t Size)
    {
        VirtualUnlock(Get() + Position, Size);
    }

    void MmioFile::Flush()
    {
        SIZE_T FlushSize = 0;
        PVOID FlushAddr = BaseAddress;
        IO_STATUS_BLOCK Block;
        NtFlushVirtualMemory(HProcess, &FlushAddr, &FlushSize, &Block);
    }

    bool MmioFile::SetWorkingSize(uint64_t MaxBytes, uint64_t MinBytes)
    {
        return SetProcessWorkingSetSizeEx(GetCurrentProcess(), MinBytes, MaxBytes, QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_ENABLE);
    }
}