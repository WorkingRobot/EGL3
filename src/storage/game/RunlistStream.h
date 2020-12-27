#pragma once

#include "../../utils/streams/Stream.h"
#include "Archive.h"

namespace EGL3::Storage::Game {
    class RunlistStream : public Utils::Streams::Stream {
    public:
        RunlistStream(Runlist* Runlist, Archive& Archive);

        Stream& write(const char* Buf, size_t BufCount) override;

        Stream& read(char* Buf, size_t BufCount) override;

        Stream& seek(size_t Position, SeekPosition SeekFrom) override;

        size_t tell() override;

        size_t size() override;

    private:
        Runlist* Runlist;
        Archive& Archive;

        size_t Position;
    };
}