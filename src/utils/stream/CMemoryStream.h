#pragma once

#include "CStream.h"

#include <vector>

class CMemoryStream : public CStream {
public:
    CMemoryStream() {
        Buffer.resize(1024);
        Size = 0;
        Position = 0;
    }

    CStream& write(const char* Buf, size_t BufCount) override {
        while (Buffer.size() < BufCount + Position) {
            Buffer.resize(Buffer.size() * 2);
        }
        memcpy(Buffer.data() + Position, Buf, BufCount);
        Position += BufCount;
        if (Position > Size) {
            Size = Position;
        }
        return *this;
    }

    CStream& read(char* Buf, size_t BufCount) override {
        if (Position + BufCount < Size) {
            BufCount = Size - Position;
        }
        memcpy(Buf, Buffer.data() + Position, BufCount);
        Position += BufCount;
        return *this;
    }

    CStream& seek(size_t Position, SeekPosition SeekFrom) override {
        switch (SeekFrom)
        {
        case CStream::Beg:
            this->Position = Position;
            break;
        case CStream::Cur:
            this->Position += Position;
            break;
        case CStream::End:
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
    std::vector<char> Buffer;
    size_t Position;
    size_t Size;
};