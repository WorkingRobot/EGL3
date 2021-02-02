#include "LauncherContentStream.h"

namespace EGL3::Web::Epic {
    LauncherContentStream::LauncherContentStream(const BPS::Manifest& Manifest, const BPS::FileManifest& File, const ChunkDataRequest& Request) :
        Manifest(Manifest),
        File(File),
        Request(Request),
        Position()
    {

    }

    Utils::Streams::Stream& LauncherContentStream::write(const char* Buf, size_t BufCount)
    {
        return *this;
    }

    Utils::Streams::Stream& LauncherContentStream::read(char* Buf, size_t BufCount)
    {
        if (Position + BufCount > size()) {
            BufCount = size() - Position;
        }
        uint32_t PartStartIndex, PartByteOffset;
        if (GetPartIndex(Position, PartStartIndex, PartByteOffset)) {
            uint32_t BytesRead = 0;
            for (auto CurrentPartItr = File.ChunkParts.begin() + PartStartIndex; CurrentPartItr != File.ChunkParts.end(); ++CurrentPartItr) {
                auto ChunkData = Request(CurrentPartItr->Guid);
                if (!ChunkData) {
                    return *this;
                }
                if ((BufCount - BytesRead) > CurrentPartItr->Size - PartByteOffset) { // copy the entire buffer over
                    memcpy(Buf + BytesRead, ChunkData->get() + CurrentPartItr->Offset, CurrentPartItr->Size - PartByteOffset);
                    BytesRead += CurrentPartItr->Size - PartByteOffset;
                }
                else { // copy what it needs to fill up the rest
                    memcpy(Buf + BytesRead, ChunkData->get() + CurrentPartItr->Offset, BufCount - BytesRead);
                    BytesRead += BufCount - BytesRead;
                    break;
                }
                PartByteOffset = 0;
            }
            Position += BytesRead;
        }

        return *this;
    }

    Utils::Streams::Stream& LauncherContentStream::seek(size_t Position, SeekPosition SeekFrom)
    {
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

    size_t LauncherContentStream::tell() const
    {
        return Position;
    }

    size_t LauncherContentStream::size() const
    {
        return File.FileSize;
    }

    bool LauncherContentStream::GetPartIndex(uint64_t ByteOffset, uint32_t& PartIndex, uint32_t& PartByteOffset) const
    {
        if (size() <= ByteOffset) {
            return false;
        }

        for (PartIndex = 0; PartIndex < File.ChunkParts.size(); ++PartIndex) {
            if (ByteOffset < File.ChunkParts[PartIndex].Size) {
                PartByteOffset = ByteOffset;
                return true;
            }
            ByteOffset -= File.ChunkParts[PartIndex].Size;
        }
        return false;
    }
}