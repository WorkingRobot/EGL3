#pragma once

#include "../../../utils/streams/Stream.h"

#include <memory>

namespace EGL3::Installer::Backend::Streams {
    class LZ4DecompStream : public Utils::Streams::Stream {
    public:
        LZ4DecompStream(Utils::Streams::Stream& BaseStream);

        ~LZ4DecompStream();

        Stream& write(const char* Buf, size_t BufCount) override {
            return *this;
        }

        Stream& read(char* Buf, size_t BufCount) override;

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
        Utils::Streams::Stream& BaseStream;

        void* LZ4Ctx;
        std::unique_ptr<char[]> CompBuffer;
        std::unique_ptr<char[]> DecompBuffer;
        int DecompLeft;
        int DecompPos;
    };
}
