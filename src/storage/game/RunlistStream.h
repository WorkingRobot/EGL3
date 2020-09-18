#pragma once

#include "../../utils/streams/Stream.h"
#include "Archive.h"

#include <numeric>

namespace EGL3::Storage::Game {
    class RunlistStream : public Utils::Streams::Stream {
    public:
        RunlistStream(Runlist& Runlist, Archive& Archive, Stream& BaseStream) : Runlist(Runlist), Archive(Archive), BaseStream(BaseStream), Position(0) {}

        Stream& write(const char* Buf, size_t BufCount) override {
            if (Position + BufCount > Runlist.TotalSize) {
                Archive.Resize(Runlist, Position + BufCount);
            }

            uint32_t RunStartIndex, RunByteOffset;
            if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
                uint32_t BytesRead = 0;
                for (auto CurrentRun = Runlist.Runs.begin() + RunStartIndex; CurrentRun != Runlist.Runs.end(); CurrentRun++) {
                    BaseStream.seek(CurrentRun->SectorOffset * 512ull + RunByteOffset, Stream::Beg);
                    if ((BufCount - BytesRead) > CurrentRun->SectorSize * 512ull - RunByteOffset) { // write the entire buffer over
                        BaseStream.write(Buf + BytesRead, CurrentRun->SectorSize * 512ull - RunByteOffset);
                        BytesRead += CurrentRun->SectorSize * 512 - RunByteOffset;
                    }
                    else { // write what it needs to use up the rest
                        BaseStream.write(Buf + BytesRead, BufCount - BytesRead);
                        BytesRead += BufCount - BytesRead;
                        break;
                    }
                    RunByteOffset = 0;
                }
                Position += BytesRead;
            }
            return *this;
        }

        Stream& read(char* Buf, size_t BufCount) override {
            if (Position + BufCount > Runlist.TotalSize) {
                BufCount = Runlist.TotalSize - Position;
            }
            uint32_t RunStartIndex, RunByteOffset;
            if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
                uint32_t BytesRead = 0;
                for (auto CurrentRun = Runlist.Runs.begin() + RunStartIndex; CurrentRun != Runlist.Runs.end(); CurrentRun++) {
                    BaseStream.seek(CurrentRun->SectorOffset * 512ull + RunByteOffset, Stream::Beg);
                    if ((BufCount - BytesRead) > CurrentRun->SectorSize * 512ull - RunByteOffset) { // copy the entire buffer over
                        BaseStream.read(Buf + BytesRead, CurrentRun->SectorSize * 512ull - RunByteOffset);
                        BytesRead += CurrentRun->SectorSize * 512 - RunByteOffset;
                    }
                    else { // copy what it needs to fill up the rest
                        BaseStream.read(Buf + BytesRead, BufCount - BytesRead);
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
            return Runlist.TotalSize;
        }

    private:
        Runlist& Runlist;
        Archive& Archive;
        Stream& BaseStream;

        size_t Position;
    };
}