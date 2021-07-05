#pragma once

#include "Stream.h"

#include <vector>

namespace EGL3::Utils::Streams {
    class MemoryStream : public Stream {
    public:
        MemoryStream() {
            Buffer.resize(1024);
            Size = 0;
            Position = 0;
        }

        MemoryStream(size_t Capacity) {
            Buffer.resize(Capacity);
            Size = 0;
            Position = 0;
        }

        MemoryStream(Stream& CopyFrom, size_t Amount) {
            Buffer.resize(Amount);
            CopyFrom.read(Buffer.data(), Amount);
            Size = Amount;
            Position = 0;
        }

        Stream& write(const char* Buf, size_t BufCount) override {
            while (Position + BufCount > Buffer.size()) {
                Buffer.resize(Buffer.size() * 2);
            }
            memcpy(Buffer.data() + Position, Buf, BufCount);
            Position += BufCount;
            if (Position > Size) {
                Size = Position;
            }
            return *this;
        }

        Stream& read(char* Buf, size_t BufCount) override {
            if (Position + BufCount > Size) {
                BufCount = Size - Position;
            }
            memcpy(Buf, Buffer.data() + Position, BufCount);
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

        size_t tell() const override {
            return Position;
        }

        size_t size() const override {
            return Size;
        }

        const char* get() const {
            return Buffer.data();
        }

    private:
        std::vector<char> Buffer;
        size_t Position;
        size_t Size;
    };
}
