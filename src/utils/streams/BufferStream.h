#pragma once

#include "Stream.h"

namespace EGL3::Utils::Streams {
    class BufferStream : public Stream {
    public:
        BufferStream(char* Start, size_t Size) :
            Buffer(Start),
            Position(0),
            Size(Size)
        {}

        Stream& write(const char* Buf, size_t BufCount) override {
            if (Position + BufCount > Size) {
                BufCount = Size - Position;
            }
            memcpy(Buffer + Position, Buf, BufCount);
            Position += BufCount;
            return *this;
        }

        Stream& read(char* Buf, size_t BufCount) override {
            if (Position + BufCount > Size) {
                BufCount = Size - Position;
            }
            memcpy(Buf, Buffer + Position, BufCount);
            Position += BufCount;
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
                this->Position = Size + Position;
            }
            return *this;
        }

        size_t tell() override {
            return Position;
        }

        size_t size() {
            return Size;
        }

    private:
        char* Buffer;
        size_t Position;
        size_t Size;
    };
}
