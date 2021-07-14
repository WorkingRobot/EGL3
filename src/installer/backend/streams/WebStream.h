#pragma once

#include "../../../utils/streams/Stream.h"

#include <deque>
#include <mutex>
#include <condition_variable>

namespace EGL3::Installer::Backend::Streams {
    class WebStream : public Utils::Streams::Stream {
    public:
        WebStream(const std::wstring& Host, const std::wstring& Url);

        ~WebStream() {
            close();
        }

        void close();

        bool valid() const;

        Stream& write(const char* Buf, size_t BufCount) override;

        Stream& read(char* Buf, size_t BufCount) override;

        Stream& seek(size_t Position, SeekPosition SeekFrom) override;

        size_t tell() const override;

        size_t size() const override;

        void HandleCallback(uint32_t Status, void* StatusInfo, uint32_t StatusInfoSize);

    private:
        void WaitForHeaders();

        void MarkCompleted();

        void* SessionHandle;
        void* ConnectionHandle;
        void* RequestHandle;
        bool IsValid;

        std::mutex BufferMtx;
        std::condition_variable BufferCV;
        std::vector<char> Buffer;
        bool IsCompleted;

        std::unique_ptr<char[]> DlBuffer;
        static constexpr size_t DlBufferSize = 1 << 16; // 64 kb

        size_t Position;
    };
}
