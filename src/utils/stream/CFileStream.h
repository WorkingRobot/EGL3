#pragma once

#include "CStream.h"

#include <filesystem>
#include <stdio.h>

namespace fs = std::filesystem;

class CFileStream : public CStream {
public:
    CFileStream() : FileStream(NULL), OpenAndValid(false) {}

    CFileStream(const CFileStream&) = delete;
    CFileStream& operator=(const CFileStream&) = delete;

    ~CFileStream() {
        close();
    }

    // weird crashes happen when i try RAII with a destructor
    bool open(fs::path FilePath, const char* Mode) {
        FileStream = fopen(FilePath.string().c_str(), Mode);
        return OpenAndValid = valid();
    }

    bool open(const char* FilePath, const char* Mode) {
        FileStream = fopen(FilePath, Mode);
        return OpenAndValid = valid();
    }

    bool close() {
        if (OpenAndValid && valid()) {
            fclose(FileStream);
            FileStream = NULL;
            return true;
        }
        return false;
    }

    bool valid() {
        return FileStream;
    }

    CStream& write(const char* Buf, size_t BufCount) override {
        fwrite(Buf, 1, BufCount, FileStream);
        return *this;
    }

    CStream& read(char* Buf, size_t BufCount) override {
        fread(Buf, 1, BufCount, FileStream);
        return *this;
    }

    CStream& seek(size_t Position, SeekPosition SeekFrom) override {
        switch (SeekFrom)
        {
        case CStream::Beg:
            _fseeki64(FileStream, Position, SEEK_SET);
            break;
        case CStream::Cur:
            _fseeki64(FileStream, Position, SEEK_CUR);
            break;
        case CStream::End:
            _fseeki64(FileStream, Position, SEEK_END);
            break;
        }

        return *this;
    }

    size_t tell() override {
        return _ftelli64(FileStream);
    }

    size_t size() override {
        auto cur = tell();
        _fseeki64(FileStream, 0, SEEK_END);
        auto ret = tell();
        _fseeki64(FileStream, cur, SEEK_SET);
        return ret;
    }

private:
    FILE* FileStream;
    bool OpenAndValid;
};