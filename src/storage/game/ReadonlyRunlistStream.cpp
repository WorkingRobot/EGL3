#include "ReadonlyRunlistStream.h"

namespace EGL3::Storage::Game {
    using namespace Utils::Streams;

    ReadonlyRunlistStream::ReadonlyRunlistStream(const Game::Runlist* Runlist, const ReadonlyArchive& Archive) :
        Runlist(Runlist),
        Archive(Archive),
        Position(0)
    {

    }

    Stream& ReadonlyRunlistStream::write(const char* Buf, size_t BufCount) {
        return *this;
    }

    Stream& ReadonlyRunlistStream::read(char* Buf, size_t BufCount) {
        if (Position + BufCount > Runlist->TotalSize) {
            BufCount = Runlist->TotalSize - Position;
        }
        uint32_t RunStartIndex, RunByteOffset;
        if (Runlist->GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
            uint32_t BytesRead = 0;
            for (auto CurrentRun = Runlist->Runs + RunStartIndex; CurrentRun != Runlist->Runs + Runlist->RunCount; CurrentRun++) {
                size_t Offset = CurrentRun->SectorOffset * Header::SectorSize + RunByteOffset;
                if ((BufCount - BytesRead) > CurrentRun->SectorCount * Header::SectorSize - RunByteOffset) { // copy the entire buffer over
                    memcpy(Buf + BytesRead, Archive.Backend.Get() + Offset, CurrentRun->SectorCount * Header::SectorSize - RunByteOffset);
                    BytesRead += CurrentRun->SectorCount * Header::SectorSize - RunByteOffset;
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

    Stream& ReadonlyRunlistStream::seek(size_t Position, SeekPosition SeekFrom) {
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

    size_t ReadonlyRunlistStream::tell() {
        return Position;
    }

    size_t ReadonlyRunlistStream::size() {
        return Runlist->TotalSize;
    }
}