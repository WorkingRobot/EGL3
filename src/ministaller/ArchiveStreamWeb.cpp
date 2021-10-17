#include "ArchiveStream.h"

#include <utility>

namespace EGL3::Installer {
    constexpr const wchar_t* UserAgent = L"EGL3 Installer/0.0.0";

    constexpr const wchar_t* GetHost(ArchiveStream::Section Section)
    {
        switch (Section)
        {
        case ArchiveStream::Section::Version:
            return L"";
        case ArchiveStream::Section::License:
            return L"l7y.ca";
        case ArchiveStream::Section::Data:
            return L"cdn.discordapp.com";
        default:
            return L"";
        }
    }

    constexpr const wchar_t* GetUrl(ArchiveStream::Section Section)
    {
        switch (Section)
        {
        case ArchiveStream::Section::Version:
            return L"";
        case ArchiveStream::Section::License:
            return L"/files/sx/6E8y.txt";
        case ArchiveStream::Section::Data:
            return L"/attachments/404161944346427402/896666355129004082/out.egli";
        default:
            return L"";
        }
    }

    struct MtxGuard {
        explicit MtxGuard(CRITICAL_SECTION& Mtx) :
            Mtx(Mtx)
        {
            EnterCriticalSection(&Mtx);
        }

        ~MtxGuard() noexcept
        {
            LeaveCriticalSection(&Mtx);
        }

        MtxGuard(const MtxGuard&) = delete;
        MtxGuard& operator=(const MtxGuard&) = delete;

    private:
        CRITICAL_SECTION& Mtx;
    };

    Language::Entry ArchiveStream::GetErrorString(Error Error)
    {
        switch (Error)
        {
        case Error::BadDnsResponse:
            return Language::Entry::DnsWebError;
        case Error::BadStatusCode:
            return Language::Entry::StatusCodeWebError;
        case Error::BadSession:
        case Error::BadConnection:
        case Error::BadRequest:
        case Error::BadDecompressionOption:
        case Error::BadCallbackOption:
        case Error::BadRequestGeneric:
        case Error::BadRequestSend:
        case Error::BadRequestRecieve:
        case Error::BadQueryStatusCode:
        case Error::BadDataRead:
        default:
            return Language::Entry::GenericWebError;
        }
    }

    ArchiveStream::ArchiveStream(Section Section, ErrorCallback OnError, void* ErrorCtx) :
        CurrentError(),
        OnError(OnError),
        ErrorCtx(ErrorCtx),
        SessionHandle(NULL),
        ConnectionHandle(NULL),
        RequestHandle(NULL),
        DownloadBuffer{},
        BufferPos(0),
        BufferSize(0),
        ConnectionReadable(false),
        BufferRequested(false),
        IsCompleted(false),
        BufferMtx(),
        BufferCV()
    {
        InitializeCriticalSection(&BufferMtx);
        InitializeConditionVariable(&BufferCV);

        SessionHandle = WinHttpOpen(UserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
        if (SessionHandle == NULL) {
            SetError(Error::BadSession);
            return;
        }

        ConnectionHandle = WinHttpConnect(SessionHandle, GetHost(Section), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (ConnectionHandle == NULL) {
            SetError(Error::BadConnection);
            return;
        }

        RequestHandle = WinHttpOpenRequest(ConnectionHandle, NULL, GetUrl(Section), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (RequestHandle == NULL) {
            SetError(Error::BadRequest);
            return;
        }

        static constexpr DWORD DecompOption = WINHTTP_DECOMPRESSION_FLAG_ALL;
        if (WinHttpSetOption(RequestHandle, WINHTTP_OPTION_DECOMPRESSION, (LPVOID)&DecompOption, sizeof(DecompOption)) == FALSE) {
            SetError(Error::BadDecompressionOption);
            return;
        }

        WINHTTP_STATUS_CALLBACK Callback = [](HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) -> void {
            if (dwContext) {
                ((ArchiveStream*)dwContext)->HandleCallback(dwInternetStatus, lpvStatusInformation, dwStatusInformationLength);
            }
        };
        if (WinHttpSetStatusCallback(RequestHandle, Callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0) == WINHTTP_INVALID_STATUS_CALLBACK) {
            SetError(Error::BadCallbackOption);
            return;
        }

        if (WinHttpSendRequest(RequestHandle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD_PTR)this) == FALSE) {
            SetError(Error::BadRequestSend);
            return;
        }
    }

    ArchiveStream::~ArchiveStream()
    {
        if (RequestHandle) {
            WinHttpCloseHandle(RequestHandle);
        }

        if (ConnectionHandle) {
            WinHttpCloseHandle(ConnectionHandle);
        }

        if (SessionHandle) {
            WinHttpCloseHandle(SessionHandle);
        }

        DeleteCriticalSection(&BufferMtx);
    }

    size_t ArchiveStream::Read(void* Out, size_t Size)
    {
        size_t BytesRead = 0;
        auto Dst = (char*)Out;
        while (Size) {
            RequestData();

            MtxGuard Guard(BufferMtx);
            if ((HasError() || IsCompleted) && BufferSize == BufferPos) {
                break;
            }

            auto ReadAmt = std::min(Size, BufferSize - BufferPos);
            memcpy(Dst, DownloadBuffer + BufferPos, ReadAmt);
            BufferPos += ReadAmt;
            Dst += ReadAmt;
            BytesRead += ReadAmt;
            Size -= ReadAmt;
        }

        return BytesRead;
    }

    void ArchiveStream::HandleCallback(DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
    {
        switch (dwInternetStatus)
        {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            if (WinHttpReceiveResponse(RequestHandle, 0) == FALSE) {
                MarkCompleted();

                SetError(Error::BadRequestRecieve);
                return;
            }
            {
                MtxGuard Guard(BufferMtx);
                ConnectionReadable = true;
            }
            WakeAllConditionVariable(&BufferCV);
            break;
        }
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            DWORD StatusCode;
            DWORD StatusSize = sizeof(DWORD);

            if (WinHttpQueryHeaders(RequestHandle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &StatusCode, &StatusSize, WINHTTP_NO_HEADER_INDEX) == FALSE) {
                MarkCompleted();

                SetError(Error::BadQueryStatusCode);
                return;
            }

            if (StatusCode != HTTP_STATUS_OK) {
                MarkCompleted();

                SetError(Error::BadStatusCode);
                return;
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            if (dwStatusInformationLength > 0) {
                {
                    MtxGuard Guard(BufferMtx);
                    BufferPos = 0;
                    BufferSize = dwStatusInformationLength;
                    BufferRequested = false;
                }
                WakeAllConditionVariable(&BufferCV);
            }
            else {
                MarkCompleted();
            }

            break;
        }
        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            MarkCompleted();

            auto Result = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;
            switch (Result->dwError)
            {
            case ERROR_WINHTTP_NAME_NOT_RESOLVED:
                SetError(Error::BadDnsResponse);
                break;
            default:
                switch (Result->dwResult)
                {
                case API_RECEIVE_RESPONSE:
                    SetError(Error::BadRequestRecieve);
                    break;
                case API_READ_DATA:
                    SetError(Error::BadDataRead);
                    break;
                case API_SEND_REQUEST:
                    SetError(Error::BadRequestSend);
                    break;
                    // These aren't used in our code
                case API_QUERY_DATA_AVAILABLE:
                case API_WRITE_DATA:
                default:
                    SetError(Error::BadRequestGeneric);
                    break;
                }
            }
            break;
        }
        }
    }

    void ArchiveStream::RequestData()
    {
        MtxGuard Guard(BufferMtx);
        if (BufferPos != BufferSize || IsCompleted || HasError()) {
            return;
        }

        if (!BufferRequested) {
            while (!ConnectionReadable && !IsCompleted && !HasError()) {
                SleepConditionVariableCS(&BufferCV, &BufferMtx, INFINITE);
            }
            BufferRequested = true;
            if (WinHttpReadData(RequestHandle, DownloadBuffer, BufferAvailSize, 0) == FALSE) {
                MarkCompleted();

                SetError(Error::BadDataRead);
                return;
            }
        }

        while (BufferPos == BufferSize && BufferRequested && !IsCompleted && !HasError()) {
            SleepConditionVariableCS(&BufferCV, &BufferMtx, INFINITE);
        }
    }

    void ArchiveStream::MarkCompleted()
    {
        {
            MtxGuard Guard(BufferMtx);
            IsCompleted = true;
        }
        WakeAllConditionVariable(&BufferCV);
    }
}