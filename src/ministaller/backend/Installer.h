#pragma once

#include "Backend.h"

namespace EGL3::Installer::Backend {
    class Installer {
    public:
        enum class Error : uint8_t {
            Success,

            BadRead,
            BadMagic,
            BadVersion,
            BadCreateProcess,
            BadReturnCode,
            BadDictionaryLoad,
            BadMetadataRead,
            BadFilenameRead,
            BadFileOpen,
            BadFileDecompression,
            BadFileWrite
        };

        enum class Log : uint8_t {
            ExecutingCommand,
            ExecutedCommand,
            InstallingData,
            InstalledData,
            InstallingFile
        };

        using ReadCallback = size_t(*)(void* ReadCtx, void* Out, size_t Size);
        using ProgressCallback = void(*)(void* Ctx, size_t Count, size_t Total);
        using LogCallback = void(*)(void* Ctx, Log Log, va_list Args);
        using ErrorCallback = void(*)(void* Ctx, Error Error, va_list Args);

        Installer(const char* InstallLocation, void* ReadCtx, ReadCallback OnRead, void* Ctx, ProgressCallback OnProgress, LogCallback OnLog, ErrorCallback OnError);

        ~Installer();

        bool HasError() const noexcept;

        Error GetError() const noexcept;

    private:
        void SetProgress(size_t Count, size_t Total) noexcept;

        void SetError(Error NewError, ...) noexcept;

        void AddLog(Log Log, ...) noexcept;

        size_t Read(void* Out, size_t Size);

        template<class T>
        void Read(T& Out)
        {
            auto BytesRead = Read(&Out, sizeof(Out));
            if (BytesRead != sizeof(Out)) {
                SetError(Error::BadRead);
            }
        }

        void* ReadCtx;
        ReadCallback OnRead;

        void* Ctx;
        ProgressCallback OnProgress;
        LogCallback OnLog;
        ErrorCallback OnError;

        Error CurrentError;

        bool Initialize();

        bool HandleCommand(CommandType Command);

        void* DCtx;
    };
}