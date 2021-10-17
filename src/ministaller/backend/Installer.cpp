#include "Installer.h"

#include "Internal.h"
#include "Registry.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string_view>

namespace EGL3::Installer::Backend {
    Installer::Installer(const char* InstallLocation, void* ReadCtx, ReadCallback OnRead, void* Ctx, ProgressCallback OnProgress, LogCallback OnLog, ErrorCallback OnError) :
        InstallLocation(InstallLocation),
        ReadCtx(ReadCtx),
        OnRead(OnRead),
        Ctx(Ctx),
        OnProgress(OnProgress),
        OnLog(OnLog),
        OnError(OnError),
        CurrentError(),
        DCtx(ZSTD_createDCtx_advanced(ZstdAllocator))
    {
        Initialize();
        while (!HasError()) {
            CommandType Cmd;
            Read(Cmd);
            if (!HandleCommand(Cmd)) {
                break;
            }
        }
    }

    Installer::~Installer()
    {
        ZSTD_freeDCtx((ZSTD_DCtx*)DCtx);
    }

    bool Installer::HasError() const noexcept
    {
        return CurrentError != Error::Success;
    }

    Installer::Error Installer::GetError() const noexcept
    {
        return CurrentError;
    }

    void Installer::SetProgress(size_t Count, size_t Total) noexcept
    {
        OnProgress(Ctx, Count, Total);
    }

    void Installer::SetError(Error NewError, ...) noexcept
    {
        va_list Args;
        va_start(Args, NewError);
        OnError(Ctx, CurrentError = NewError, Args);
        va_end(Args);
    }

    void Installer::AddLog(Log Log, ...) noexcept
    {
        va_list Args;
        va_start(Args, Log);
        OnLog(Ctx, Log, Args);
        va_end(Args);
    }

    size_t Installer::Read(void* Out, size_t Size)
    {
        return OnRead(ReadCtx, Out, Size);
    }

    template<class ReadT>
    size_t ReadCompressed(ZSTD_DCtx* DCtx, void* In, size_t InSize, size_t* InPos, size_t* InReadSize, void* Out, size_t OutSize, ReadT Read)
    {
        ZSTD_outBuffer OutBuf{
            .dst = Out,
            .size = OutSize,
            .pos = 0
        };
        ZSTD_inBuffer InBuf{
            .src = In,
            .size = *InReadSize,
            .pos = *InPos
        };
        while (OutBuf.size > OutBuf.pos) {
            if (InBuf.pos == InBuf.size) {
                InBuf = {
                    .src = In,
                    .size = Read(In, InSize),
                    .pos = 0
                };
            }
            auto RemainingBytes = ZSTD_decompressStream(DCtx, &OutBuf, &InBuf);
            if (ZSTD_isError(RemainingBytes)) {
                return RemainingBytes;
            }
        }
        *InReadSize = InBuf.size;
        *InPos = InBuf.pos;

        return 0;
    }

    bool Installer::Initialize()
    {
        uint32_t Magic;
        Read(Magic);
        if (Magic != FileMagic) {
            SetError(Error::BadMagic, Magic);
        }
        if (!HasError()) {
            DataVersion Version;
            Read(Version);
            if (Version != DataVersion::Latest) {
                SetError(Error::BadVersion, Version);
            }
        }
        return HasError();
    }

    void CreateDirectories(std::string_view Directory)
    {
        char SubDir[1024]{};
        int LastDirPos = 0;

        while (LastDirPos < Directory.size()) {
            auto DirPos = Directory.find('\\', LastDirPos);
            if (DirPos++ == std::string_view::npos || DirPos == Directory.size()) {
                return;
            }
            memcpy(SubDir + LastDirPos, Directory.data() + LastDirPos, DirPos - LastDirPos);
            CreateDirectory(SubDir, NULL);
            LastDirPos = DirPos;
        }
    }

    bool Installer::HandleCommand(CommandType Command)
    {
        switch (Command)
        {
        case CommandType::Command:
        {
            uint16_t CommandSize;
            Read(CommandSize);
            auto CommandData = (char*)Alloc(CommandSize + 1);
            Read(CommandData, CommandSize);
            if (HasError()) {
                return false;
            }

            AddLog(Log::ExecutingCommand, CommandData);
            STARTUPINFO StartInfo{
                .cb = sizeof(StartInfo)
            };
            PROCESS_INFORMATION ProcInfo{};
            if (CreateProcess(NULL, CommandData, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &StartInfo, &ProcInfo)) {
                CloseHandle(ProcInfo.hThread);
                WaitForSingleObject(ProcInfo.hProcess, INFINITE);
                DWORD ExitCode;
                GetExitCodeProcess(ProcInfo.hProcess, &ExitCode);
                CloseHandle(ProcInfo.hProcess);

                AddLog(Log::ExecutedCommand, CommandData, ExitCode);
                if (ExitCode != ERROR_SUCCESS) {
                    SetError(Error::BadReturnCode, CommandData, ExitCode);
                }
            }
            else {
                SetError(Error::BadCreateProcess, CommandData);
            }
            return !HasError();
        }
        case CommandType::Registry:
        {
            Registry Registry;
            static constexpr auto EntryCount = (size_t)Registry::Value::Count;
            // Divide by 8 (round up)
            char ReadMask[(EntryCount + 7) >> 3];
            Read(ReadMask);
            auto ReadValue = [this, &Registry, &ReadMask](auto ValueType) {
                static constexpr auto MaskIdx = (size_t)decltype(ValueType)::Key;
                if (!(ReadMask[MaskIdx / CHAR_BIT] & (1 << (MaskIdx % CHAR_BIT)))) {
                    return;
                }

                using T = decltype(ValueType)::T;
                T Val{};
                if constexpr (std::is_array_v<T>) {
                    uint16_t ArraySize;
                    Read(ArraySize);
                    Read(Val, ArraySize);
                }
                else {
                    Read(Val);
                }
                Registry.Set(decltype(ValueType)::Key, Val);
            };
            ReadValue(Registry::ValueType<Registry::Value::DisplayName>());
            ReadValue(Registry::ValueType<Registry::Value::UninstallString>());
            ReadValue(Registry::ValueType<Registry::Value::InstallLocation>());
            ReadValue(Registry::ValueType<Registry::Value::DisplayIcon>());
            ReadValue(Registry::ValueType<Registry::Value::Publisher>());
            ReadValue(Registry::ValueType<Registry::Value::HelpLink>());
            ReadValue(Registry::ValueType<Registry::Value::URLUpdateInfo>());
            ReadValue(Registry::ValueType<Registry::Value::URLInfoAbout>());
            ReadValue(Registry::ValueType<Registry::Value::DisplayVersion>());
            ReadValue(Registry::ValueType<Registry::Value::EstimatedSize>());
            return true;
        }
        /*
        case CommandType::Dictionary:
        {
            uint32_t DictSize;
            Read(DictSize);
            auto Dictionary = Alloc(DictSize);
            Read(Dictionary, DictSize);
            auto Ret = ZSTD_DCtx_loadDictionary_advanced((ZSTD_DCtx*)DCtx, Dictionary, DictSize, ZSTD_dlm_byCopy, ZSTD_dct_fullDict);
            Free(Dictionary);
            if (ZSTD_isError(Ret)) {
                SetError(Error::BadDictionaryLoad, DictSize);
                return false;
            }
            return true;
        }
        */
        case CommandType::Data:
        {
            size_t DataSize;
            Read(DataSize);

            AddLog(Log::InstallingData, DataSize);

            auto InputSize = ZSTD_DStreamInSize();
            auto InputBuffer = Alloc(InputSize);
            auto OutputSize = ZSTD_DStreamOutSize();
            auto OutputBuffer = Alloc(OutputSize);
            char FullFilename[512]{};
            size_t InstallLocationSize = lstrlen(InstallLocation);
            memcpy(FullFilename, InstallLocation, InstallLocationSize);

            size_t DataRead = 0;
            auto ReadCallback = [&, this](void* Out, size_t Size) {
                auto ReadAmt = Read(Out, std::min(Size, DataSize - DataRead));
                DataRead += ReadAmt;
                return ReadAmt;
            };

            size_t InputPos = InputSize;
            size_t InputReadSize = InputSize;
            while (DataRead != DataSize && !HasError()) {
                SetProgress(DataRead, DataSize);

                union {
                    struct {
                        uint32_t FileSize;
                        uint16_t PathSize;
                        char Filename[];
                    };
                    char Buffer[512];
                } Metadata{};

                size_t ZstdError = 0;
                if (ZSTD_isError(ZstdError = ReadCompressed((ZSTD_DCtx*)DCtx, InputBuffer, InputSize, &InputPos, &InputReadSize, Metadata.Buffer, 4 + 2, ReadCallback))) {
                    SetError(Error::BadMetadataRead, ZSTD_getErrorName(ZstdError));
                    continue;
                }
                if (ZSTD_isError(ZstdError = ReadCompressed((ZSTD_DCtx*)DCtx, InputBuffer, InputSize, &InputPos, &InputReadSize, Metadata.Filename, Metadata.PathSize, ReadCallback))) {
                    SetError(Error::BadFilenameRead, ZSTD_getErrorName(ZstdError));
                    continue;
                }

                AddLog(Log::InstallingFile, Metadata.Filename, Metadata.FileSize);

                strcpy(FullFilename + InstallLocationSize, Metadata.Filename);
                CreateDirectories(FullFilename);
                auto Handle = CreateFile(FullFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (Handle == INVALID_HANDLE_VALUE) {
                    SetError(Error::BadFileOpen, Metadata.Filename);
                    continue;
                }
                while (Metadata.FileSize != 0) {
                    auto ReadAmt = std::min<size_t>(OutputSize, Metadata.FileSize);
                    if (ZSTD_isError(ZstdError = ReadCompressed((ZSTD_DCtx*)DCtx, InputBuffer, InputSize, &InputPos, &InputReadSize, OutputBuffer, ReadAmt, ReadCallback))) {
                        SetError(Error::BadFileDecompression, Metadata.Filename, ZSTD_getErrorName(ZstdError));
                        break;
                    }
                    DWORD BytesWritten;
                    WriteFile(Handle, OutputBuffer, ReadAmt, &BytesWritten, NULL);
                    if (BytesWritten != ReadAmt) {
                        SetError(Error::BadFileWrite, Metadata.Filename);
                        break;
                    }
                    Metadata.FileSize -= ReadAmt;
                }
                CloseHandle(Handle);
            }

            Free(InputBuffer);
            Free(OutputBuffer);

            AddLog(Log::InstalledData, DataSize);

            return !HasError();
        }
        case CommandType::End:
        {
            Registry Registry;
            Registry.Set(Registry::Value::)
        }
        default:
            return false;
        }
    }
}