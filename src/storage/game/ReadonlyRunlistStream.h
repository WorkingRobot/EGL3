#pragma once

#include "../../utils/streams/Stream.h"
#include "ReadonlyArchive.h"

namespace EGL3::Storage::Game {
    class ReadonlyRunlistStream : public Utils::Streams::Stream {
    public:
        ReadonlyRunlistStream(const Runlist* Runlist, const ReadonlyArchive& Archive) : Runlist(Runlist), Archive(Archive), Position(0) {}

        Stream& write(const char* Buf, size_t BufCount) override {
            return *this;
        }

        Stream& read(char* Buf, size_t BufCount) override {
            if (Position + BufCount > Runlist->TotalSize) {
                BufCount = Runlist->TotalSize - Position;
            }
            uint32_t RunStartIndex, RunByteOffset;
            if (Runlist->GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
                uint32_t BytesRead = 0;
                for (auto CurrentRun = Runlist->Runs + RunStartIndex; CurrentRun != Runlist->Runs + Runlist->RunCount; CurrentRun++) {
                    size_t Offset = CurrentRun->SectorOffset * 512ull + RunByteOffset;
                    if ((BufCount - BytesRead) > CurrentRun->SectorSize * 512ull - RunByteOffset) { // copy the entire buffer over
                        memcpy(Buf + BytesRead, Archive.Backend.Get() + Offset, CurrentRun->SectorSize * 512ull - RunByteOffset);
                        BytesRead += CurrentRun->SectorSize * 512ull - RunByteOffset;
                    }
                    else { // copy what it needs to fill up the rest
                        memcpy(Buf + BytesRead, Archive.Backend.Get() + Offset, BufCount - BytesRead);
                        BytesRead += BufCount - BytesRead;
                        break;
                    }
                    RunByteOffset = 0;
                }
                Position += BytesRead;
            }

            return *this;
        }

        Stream& seek(size_t Position, SeekPosition SeekFrom) override {
            switch (SeekFrom)
            {
            case Stream::Beg:
                this->Position = Position;
                break;
            case Stream::Cur:
                this->Position += Position;
                break;
            case Stream::End:
                this->Position = Position + size();
                break;
            }
            return *this;
        }

        size_t tell() override {
            return Position;
        }

        size_t size() override {
            return Runlist->TotalSize;
        }

    private:
        const Runlist* Runlist;
        const ReadonlyArchive& Archive;

        size_t Position;
    };
}