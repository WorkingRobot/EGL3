#include "WebStream.h"

#include <iterator>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winhttp.h>

namespace EGL3::Installer::Backend::Streams {
    void CALLBACK Callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
        if (dwContext) {
            ((WebStream*)dwContext)->HandleCallback(dwInternetStatus, lpvStatusInformation, dwStatusInformationLength);
        }
    }

    WebStream::WebStream(const std::wstring& Host, const std::wstring& Url) :
        SessionHandle(NULL),
        ConnectionHandle(NULL),
        RequestHandle(NULL),
        IsValid(false),
        IsCompleted(false),
        Position(0)
    {
        SessionHandle = WinHttpOpen(L"EGL3 Installer/" CONFIG_VERSION_LONG, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
        if (SessionHandle == NULL) {
            return;
        }

        ConnectionHandle = WinHttpConnect(SessionHandle, Host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (ConnectionHandle == NULL) {
            return;
        }

        RequestHandle = WinHttpOpenRequest(ConnectionHandle, NULL, Url.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (RequestHandle == NULL) {
            return;
        }

        DWORD DecompOption = WINHTTP_DECOMPRESSION_FLAG_ALL;
        bool Successful = WinHttpSetOption(RequestHandle, WINHTTP_OPTION_DECOMPRESSION, &DecompOption, sizeof(DecompOption));
        if (!Successful) {
            return;
        }
        if (WinHttpSetStatusCallback(RequestHandle, Callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0) == WINHTTP_INVALID_STATUS_CALLBACK) {
            return;
        }

        DlBuffer = std::make_unique<char[]>(DlBufferSize);

        Successful = WinHttpSendRequest(RequestHandle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD_PTR)this);
        if (!Successful) {
            return;
        }

        IsValid = true;

        WaitForHeaders();
    }

    void WebStream::close()
    {
        if (RequestHandle) {
            WinHttpCloseHandle(RequestHandle);

            if (IsValid) {
                std::unique_lock Lock(BufferMtx);
                BufferCV.wait(Lock, [this]() {
                    return IsCompleted;
                });
            }
        }
        if (ConnectionHandle) {
            WinHttpCloseHandle(ConnectionHandle);
        }
        if (SessionHandle) {
            WinHttpCloseHandle(SessionHandle);
        }
    }

    bool WebStream::valid() const
    {
        return IsValid;
    }

    Utils::Streams::Stream& WebStream::write(const char* Buf, size_t BufCount)
    {
        return *this;
    }

    Utils::Streams::Stream& WebStream::read(char* Buf, size_t BufCount)
    {
        std::unique_lock Lock(BufferMtx);
        BufferCV.wait(Lock, [this, BufCount]() {
            return IsCompleted || Buffer.size() >= BufCount;
        });

        if (Buffer.size() >= BufCount) {
            //memcpy(Buf, Buffer.data(), BufCount);
            auto Begin = Buffer.begin();
            std::copy(Begin, Begin + BufCount, Buf);
            Buffer.erase(Begin, Begin + BufCount);
            Position += BufCount;
        }

        return *this;
    }

    Utils::Streams::Stream& WebStream::seek(size_t Position, SeekPosition SeekFrom)
    {
        return *this;
    }

    size_t WebStream::tell() const
    {
        return Position;
    }

    size_t WebStream::size() const
    {
        return 0;
    }

    void WebStream::HandleCallback(uint32_t Status, void* StatusInfo, uint32_t StatusInfoSize)
    {
        if (IsCompleted) {
            return;
        }

        switch (Status)
        {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            if (!WinHttpReceiveResponse(RequestHandle, 0)) {
                MarkCompleted();

                printf("WinHttpReceiveResponse error\n");
                // error
                return;
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            DWORD StatusCode;
            DWORD StatusSize = sizeof(DWORD);

            if (!WinHttpQueryHeaders(RequestHandle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &StatusCode, &StatusSize, WINHTTP_NO_HEADER_INDEX)) {
                MarkCompleted();

                printf("can't query status code\n");
                // error
                return;
            }

            if (StatusCode != HTTP_STATUS_OK) {
                MarkCompleted();

                printf("not 200 error\n");
                // not 200
                return;
            }

            if (!WinHttpReadData(RequestHandle, DlBuffer.get(), DlBufferSize, 0)) {
                MarkCompleted();

                printf("WinHttpReadData error\n");
                // error
                return;
            }

            break;
        }
        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            if (StatusInfoSize > 0) {
                // pass buffer to callback
                printf("size (cb): %d\n", StatusInfoSize);

                {
                    {
                        std::lock_guard Guard(BufferMtx);
                        Buffer.reserve(Buffer.size() + StatusInfoSize);
                        std::copy(DlBuffer.get(), DlBuffer.get() + StatusInfoSize, std::back_inserter(Buffer));
                    }

                    if (!WinHttpReadData(RequestHandle, DlBuffer.get(), DlBufferSize, 0)) {
                        MarkCompleted();

                        printf("WinHttpReadData error\n");
                        // error
                        return;
                    }
                }

                BufferCV.notify_all();
            }
            else {
                MarkCompleted();

                //printf("%*s %zu\n", (int)Resp.size(), Resp.data(), Resp.size());
                printf("complete %zu\n", Buffer.size());
                // complete
            }

            break;
        }
        default:
            break;
        }
    }

    void WebStream::WaitForHeaders()
    {
        std::unique_lock Lock(BufferMtx);
        BufferCV.wait(Lock, [this] () { return IsCompleted || !Buffer.empty(); });
    }

    void WebStream::MarkCompleted()
    {
        {
            std::lock_guard Guard(BufferMtx);
            IsCompleted = true;
        }
        BufferCV.notify_all();
    }
}
