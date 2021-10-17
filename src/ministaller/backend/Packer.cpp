#include "Packer.h"

#include "Internal.h"

#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define ZDICT_STATIC_LINKING_ONLY
#include <zdict.h>
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

namespace EGL3::Installer::Backend {
    Packer::Packer(const char* Path, int CompressionLevel) :
        Handle(CreateFile(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)),
        CCtx(ZSTD_createCCtx_advanced(ZstdAllocator))
    {
        ZSTD_CCtx_setParameter((ZSTD_CCtx*)CCtx, ZSTD_c_compressionLevel, CompressionLevel);
    }

    Packer::~Packer()
    {
        Close();

        ZSTD_freeCCtx((ZSTD_CCtx*)CCtx);
    }

    bool Packer::IsValid() const
    {
        return Handle != INVALID_HANDLE_VALUE;
    }

    void Packer::Close()
    {
        Write(CommandType::End);

        CloseHandle(Handle);
        Handle = INVALID_HANDLE_VALUE;
    }

    bool Packer::Initialize()
    {
        Write(FileMagic);
        Write(DataVersion::Latest);
        return IsValid();
    }

    bool Packer::AddCommand(const char* Command)
    {
        Write(CommandType::Command);
        uint16_t Length = lstrlen(Command);
        Write(Length);
        Write(Command, Length);
        return IsValid();
    }

    bool Packer::AddRegistry(const char* DisplayName, const char* Publisher, const char* HelpLink, const char* URLUpdateInfo, const char* URLInfoAbout, const char* DisplayVersion)
    {
        Write(CommandType::Registry);
        // Manually written mask (note that this is in reverse order, meaning DisplayName is the last '1' value and EstimatedSize is the first '0')
        //  EstimatedSize v        v DisplayName
        Write<uint16_t>(0b0111110001);
        auto WriteString = [this](const char* String) {
            uint16_t Size = lstrlen(String);
            Write(Size);
            Write(String, Size);
        };
        WriteString(DisplayName);
        WriteString(Publisher);
        WriteString(HelpLink);
        WriteString(URLUpdateInfo);
        WriteString(URLInfoAbout);
        WriteString(DisplayVersion);
        return IsValid();
    }

    //bool Packer::AddCleanMsiInstall(const char* Path)
    //{
    //    Write(CommandType::InstallMsiClean);
    //    uint16_t Length = lstrlen(Path);
    //    Write(Length);
    //    Write(Path, Length);
    //    return IsValid();
    //}

    template<class FuncT>
    bool IterateFiles(const char* Path, FuncT Func)
    {
        char Buffer[MAX_PATH];
        lstrcpy(Buffer, Path);
        lstrcat(Buffer, "\\*");

        WIN32_FIND_DATA FindData;
        auto Handle = FindFirstFileEx(Buffer, FindExInfoBasic, &FindData, FindExSearchNameMatch, NULL, 0);
        if (Handle != INVALID_HANDLE_VALUE) {
            do {
                if (lstrcmp(FindData.cFileName, ".") == 0 || lstrcmp(FindData.cFileName, "..") == 0) {
                    continue;
                }

                lstrcpy(Buffer, Path);
                lstrcat(Buffer, "\\");
                lstrcat(Buffer, FindData.cFileName);

                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!IterateFiles(Buffer, Func)) {
                        FindClose(Handle);
                        return false;
                    }
                }
                else {
                    if (!Func(Buffer, FindData)) {
                        FindClose(Handle);
                        return false;
                    }
                }
            } while (FindNextFile(Handle, &FindData));
            FindClose(Handle);
        }

        return true;
    }

    /*
    bool Packer::AddDictionary(const char* Folder, size_t Capacity)
    {
        std::vector<char, Allocator<char>> Samples;
        std::vector<size_t, Allocator<size_t>> SampleSizes;
        IterateFiles(Folder, [&](const char* Path, const WIN32_FIND_DATA& FindData) {
            auto Handle = CreateFile(Path, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (Handle == INVALID_HANDLE_VALUE) {
                Log("%s CreateFile\n", Path);
                return false;
            }

            DWORD FileSize = FindData.nFileSizeLow;
            Samples.resize(Samples.size() + FileSize);
            DWORD ReadAmt;
            if (ReadFile(Handle, Samples.data() + Samples.size() - FileSize, FileSize, &ReadAmt, NULL) == FALSE || ReadAmt != FileSize) {
                Log("%s ReadFile\n", Path);
                return false;
            }
            SampleSizes.emplace_back(FileSize);

            CloseHandle(Handle);
            return true;
        });

        void* Dictionary = Alloc(Capacity);

        ZDICT_fastCover_params_t Params{
            .k = 0, // 0 means this will be optimized
            .d = 8,
            .steps = 4,
            .zParams = {
                .customMem = ZstdAllocator
            }
        };
        ZSTD_CCtx_getParameter((ZSTD_CCtx*)CCtx, ZSTD_c_compressionLevel, &Params.zParams.compressionLevel);

        auto DictionarySize = ZDICT_optimizeTrainFromBuffer_fastCover(Dictionary, Capacity, Samples.data(), SampleSizes.data(), SampleSizes.size(), &Params);
        if (ZDICT_isError(DictionarySize)) {
            Log("Error: %s\n", ZDICT_getErrorName(DictionarySize));
        }
        else {
            Write(CommandType::Dictionary);
            Write(DictionarySize);
            Write(Dictionary, DictionarySize);

            ZSTD_CCtx_loadDictionary_advanced((ZSTD_CCtx*)CCtx, Dictionary, DictionarySize, ZSTD_dlm_byCopy, ZSTD_dct_fullDict);
        }

        Free(Dictionary);

        return IsValid();
    }
    */

    template<ZSTD_EndDirective Directive = ZSTD_e_continue, class WriteT>
    bool WriteCompressed(ZSTD_CCtx* CCtx, const void* In, size_t InSize, void* Out, size_t OutSize, WriteT Write)
    {
        ZSTD_inBuffer InBuf{
            .src = In,
            .size = InSize,
            .pos = 0
        };
        while (true) {
            ZSTD_outBuffer OutBuf{
                .dst = Out,
                .size = OutSize,
                .pos = 0
            };

            auto RemainingBytes = ZSTD_compressStream2(CCtx, &OutBuf, &InBuf, Directive);
            if (ZSTD_isError(RemainingBytes)) {
                Log("Error: %s\n", ZSTD_getErrorName(RemainingBytes));
                return false;
            }

            Write(OutBuf.dst, OutBuf.pos);

            if (InBuf.size == 0 ? RemainingBytes == 0 : InBuf.pos == InBuf.size) {
                return true;
            }
        }
    }

    bool Packer::AddDirectory(const char* Folder)
    {
        Write(CommandType::Data);
        Write<size_t>(0);
        auto SizeOffset = Tell();

        auto InputSize = ZSTD_CStreamInSize();
        auto InputBuffer = Alloc(InputSize);
        auto OutputSize = ZSTD_CStreamOutSize();
        auto OutputBuffer = Alloc(OutputSize);

        auto WriteCallback = [this](const void* In, size_t Size) { Write(In, Size); };

        auto PathPrefixSize = lstrlen(Folder) + 1; // Folder + '/'
        IterateFiles(Folder, [&](const char* Path, const WIN32_FIND_DATA& FindData) {
            {
                union {
                    struct {
                        uint32_t FileSize;
                        uint16_t PathSize;
                        char Filename[];
                    };
                    char Buffer[512];
                } Metadata;
                Metadata.FileSize = FindData.nFileSizeLow;
                Metadata.PathSize = lstrlen(Path) - PathPrefixSize;
                memcpy(Metadata.Filename, Path + PathPrefixSize, Metadata.PathSize);

                if (!WriteCompressed((ZSTD_CCtx*)CCtx, Metadata.Buffer, 4 + 2 + Metadata.PathSize, OutputBuffer, OutputSize, WriteCallback)) {
                    return false;
                }
            }

            auto Handle = CreateFile(Path, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (Handle == INVALID_HANDLE_VALUE) {
                Log("%s CreateFile\n", Path);
                return false;
            }

            while (true) {
                DWORD ReadAmt;
                if (ReadFile(Handle, InputBuffer, InputSize, &ReadAmt, NULL) == FALSE) {
                    Log("%s ReadFile\n", Path);
                    CloseHandle(Handle);
                    return false;
                }

                if (!WriteCompressed((ZSTD_CCtx*)CCtx, InputBuffer, ReadAmt, OutputBuffer, OutputSize, WriteCallback)) {
                    break;
                }

                if (ReadAmt < InputSize) {
                    break;
                }
            };

            return true;
        });

        WriteCompressed<ZSTD_e_end>((ZSTD_CCtx*)CCtx, nullptr, 0, OutputBuffer, OutputSize, WriteCallback);

        Free(InputBuffer);
        Free(OutputBuffer);

        PWrite(Tell() - SizeOffset, SizeOffset - sizeof(size_t));
        return IsValid();
    }

    size_t Packer::Tell()
    {
        LARGE_INTEGER Ret{ .QuadPart = 0 };
        return SetFilePointerEx(Handle, Ret, &Ret, FILE_CURRENT) != 0 ? Ret.QuadPart : 0;
    }

    size_t Packer::PWriteLazy(const void* In, size_t Size, size_t Offset)
    {
        if (IsValid()) {
            OVERLAPPED Overlapped{};
            LARGE_INTEGER LOffset{ .QuadPart = (LONGLONG)Offset };
            Overlapped.Offset = LOffset.LowPart;
            Overlapped.OffsetHigh = LOffset.HighPart;
            DWORD BytesWritten;
            WriteFile(Handle, In, Size, &BytesWritten, &Overlapped);
            SetFilePointer(Handle, 0, NULL, FILE_END);
            return BytesWritten;
        }
        return 0;
    }

    size_t Packer::WriteLazy(const void* In, size_t Size)
    {
        if (IsValid()) {
            DWORD BytesWritten;
            WriteFile(Handle, In, Size, &BytesWritten, NULL);
            return BytesWritten;
        }
        return 0;
    }
}