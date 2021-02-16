#pragma once

#include "../../utils/streams/Stream.h"
#include "Archive.h"

namespace EGL3::Storage::Game {
    template<uint32_t MaxRunCount>
    class RunlistStream : public Utils::Streams::Stream {
    public:
        RunlistStream(Runlist<MaxRunCount>& Runlist, Archive& Archive) :
            Runlist(Runlist),
            Archive(Archive),
            Position(0)
        {

        }

        Stream& write(const char* Buf, size_t BufCount) override {
            if (Position + BufCount > size()) {
                Archive.Resize(Runlist, Position + BufCount);
            }

            uint32_t RunStartIndex;
            uint64_t RunByteOffset;
            if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
                uint32_t BytesRead = 0;
                for (auto CurrentRunItr = Runlist.GetRuns().begin() + RunStartIndex; CurrentRunItr != Runlist.GetRuns().end(); ++CurrentRunItr) {
                    size_t Offset = CurrentRunItr->SectorOffset * Header::GetSectorSize() + RunByteOffset;
                    if ((BufCount - BytesRead) > CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset) { // write the entire buffer over
                        memcpy(Archive.Get() + Offset, Buf + BytesRead, CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset);
                        BytesRead += CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset;
                    }
                    else { // write what it needs to use up the rest
                        memcpy(Archive.Get(), Buf + BytesRead, BufCount - BytesRead);
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
            if (Position + BufCount > Runlist.GetValidSize()) {
                BufCount = Runlist.GetValidSize() - Position;
            }

            uint32_t RunStartIndex;
            uint64_t RunByteOffset;
            if (Runlist.GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
                size_t BytesRead = 0;
                for (auto CurrentRunItr = Runlist.GetRuns().begin() + RunStartIndex; CurrentRunItr != Runlist.GetRuns().end(); ++CurrentRunItr) {
                    size_t Offset = CurrentRunItr->SectorOffset * Header::GetSectorSize() + RunByteOffset;
                    if ((BufCount - BytesRead) > CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset) { // copy the entire buffer over
                        memcpy(Buf + BytesRead, Archive.Get() + Offset, CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset);
                        BytesRead += CurrentRunItr->SectorCount * Header::GetSectorSize() - RunByteOffset;
                    }
                    else { // copy what it needs to fill up the rest
                        memcpy(Buf + BytesRead, Archive.Get() + Offset, BufCount - BytesRead);
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
            return Runlist.GetValidSize();
        }

    private:
        Runlist<MaxRunCount>& Runlist;
        Archive& Archive;

        size_t Position;
    };
}