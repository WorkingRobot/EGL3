#pragma once

#include "../../utils/streams/Stream.h"
#include "ReadonlyArchive.h"

namespace EGL3::Storage::Game {
    class ReadonlyRunlistStream : public Utils::Streams::Stream {
    public:
        ReadonlyRunlistStream(const Runlist* Runlist, const ReadonlyArchive& Archive);

        Stream& write(const char* Buf, size_t BufCount) override;

        Stream& read(char* Buf, size_t BufCount) override;

        Stream& seek(size_t Position, SeekPosition SeekFrom) override;

        size_t tell() override;

        size_t size() override;

    private:
        const Runlist* Runlist;
        const ReadonlyArchive& Archive;

        size_t Position;
    };
}