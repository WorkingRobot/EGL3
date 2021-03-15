#pragma once

#include "../../../utils/streams/Stream.h"

#include <memory>

namespace EGL3::Installer::Backend::Streams {
    class LZ4Stream : public Utils::Streams::Stream {
    public:
        LZ4Stream(Utils::Streams::Stream& BaseStream);

        ~LZ4Stream();

        Stream& write(const char* Buf, size_t BufCount) override;

        Stream& read(char* Buf, size_t BufCount) override {
            return *this;
        }

        Stream& seek(size_t Position, SeekPosition SeekFrom) override {
            return *this;
        }

        size_t tell() const override {
            return 0;
        }

        size_t size() const override {
            return 0;
        }

    private:
        bool Compress(int BufCount);

        Utils::Streams::Stream& BaseStream;

        void* LZ4Ctx;
        std::unique_ptr<char[]> InputBuffer;
        std::unique_ptr<char[]> OutputBuffer;
        size_t BufPos;
    };
}
