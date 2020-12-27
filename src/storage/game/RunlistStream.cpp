#include "RunlistStream.h"

namespace EGL3::Storage::Game {
    using namespace Utils::Streams;

    RunlistStream::RunlistStream(Game::Runlist* Runlist, Game::Archive& Archive) :
        Runlist(Runlist),
        Archive(Archive),
        Position(0)
    {

    }

    Stream& RunlistStream::write(const char* Buf, size_t BufCount) {
        if (Position + BufCount > Runlist->TotalSize) {
            Archive.Resize(Runlist, Position + BufCount);
        }

        uint32_t RunStartIndex, RunByteOffset;
        if (Runlist->GetRunIndex(Position, RunStartIndex, RunByteOffset)) {
            uint32_t BytesRead = 0;
            for (auto CurrentRun = Runlist->Runs + RunStartIndex; CurrentRun != Runlist->Runs + Runlist->RunCount; CurrentRun++) {
                size_t Offset = CurrentRun->SectorOffset * 512ull + RunByteOffset;
                if ((BufCount - BytesRead) > CurrentRun->SectorSize * 512ull - RunByteOffset) { // write the entire buffer over
                    memcpy(Archive.Backend.Get() + Offset, Buf + BytesRead, CurrentRun->SectorSize * 512ull - RunByteOffset);
                    BytesRead += CurrentRun->SectorSize * 512 - RunByteOffset;
                }
                else { // write what it needs to use up the rest
                    memcpy(Archive.Backend.Get(), Buf + BytesRead, BufCount - BytesRead);
                    BytesRead += BufCount - BytesRead;
                    break;
                }
                RunByteOffset = 0;
            }
            Position += BytesRead;
        }
        return *this;
    }

    Stream& RunlistStream::read(char* Buf, size_t BufCount) {
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

    Stream& RunlistStream::seek(size_t Position, SeekPosition SeekFrom) {
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

    size_t RunlistStream::tell() {
        return Position;
    }

    size_t RunlistStream::size() {
        return Runlist->TotalSize;
    }
}