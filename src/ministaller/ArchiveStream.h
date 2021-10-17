#pragma once

#include "Language.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winhttp.h>

namespace EGL3::Installer {
    class ArchiveStream {
    public:
        enum class Section : uint8_t {
            Version,
            License,
            Data
        };

        enum class Error : uint8_t {
            Success,

            BadSession,
            BadConnection,
            BadRequest,
            BadDecompressionOption,
            BadCallbackOption,
            BadRequestGeneric,
            BadRequestSend,
            BadRequestRecieve,
            BadQueryStatusCode,
            BadStatusCode,
            BadDataRead,
            BadDnsResponse,
            BadPrematureCompletion
        };

        static Language::Entry GetErrorString(Error Error);

        using ErrorCallback = void(*)(void* Ctx, Error Error);

        ArchiveStream(Section Section, ErrorCallback OnError, void* ErrorCtx);

        ~ArchiveStream();

        bool HasError() const noexcept;

        Error GetError() const noexcept;

        size_t Read(void* Out, size_t Size);

        template<class T>
        void Read(T& Out)
        {
            auto BytesRead = Read(&Out, sizeof(Out));
            if (BytesRead != sizeof(Out) && !HasError()) {
                SetError(Error::BadPrematureCompletion);
            }
        }

    private:
        void SetError(Error NewError) noexcept;

        Error CurrentError;
        ErrorCallback OnError;
        void* ErrorCtx;

        void HandleCallback(DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

        void RequestData();

        void MarkCompleted();

        static constexpr size_t BufferAvailSize = 1 << 12;
        HINTERNET SessionHandle;
        HINTERNET ConnectionHandle;
        HINTERNET RequestHandle;
        char DownloadBuffer[BufferAvailSize];
        size_t BufferPos;
        size_t BufferSize;
        bool ConnectionReadable;
        bool BufferRequested;
        bool IsCompleted;
        CRITICAL_SECTION BufferMtx;
        CONDITION_VARIABLE BufferCV;
    };
}