#pragma once

#include "Stream.h"

#include <filesystem>
#include <stdio.h>

namespace EGL3::Utils::Streams {
    class FileStream : public Stream {
    public:
        FileStream() : BaseStream(NULL), OpenAndValid(false) {}

        FileStream(const FileStream&) = delete;
        FileStream& operator=(const FileStream&) = delete;

        ~FileStream() {
            close();
        }

        // weird crashes happen when i try RAII with a destructor
        bool open(const std::filesystem::path& FilePath, const char* Mode) {
            BaseStream = fopen(FilePath.string().c_str(), Mode);
            return OpenAndValid = valid();
        }

        bool open(const char* FilePath, const char* Mode) {
            BaseStream = fopen(FilePath, Mode);
            return OpenAndValid = valid();
        }

        bool close() {
            if (OpenAndValid && valid()) {
                fclose(BaseStream);
                BaseStream = NULL;
                return true;
            }
            return false;
        }

        bool valid() {
            return BaseStream;
        }

        Stream& write(const char* Buf, size_t BufCount) override {
            fwrite(Buf, 1, BufCount, BaseStream);
            return *this;
        }

        Stream& read(char* Buf, size_t BufCount) override {
            fread(Buf, 1, BufCount, BaseStream);
            return *this;
        }

        Stream& seek(size_t Position, SeekPosition SeekFrom) override {
            switch (SeekFrom)
            {
            case Stream::Beg:
                _fseeki64(BaseStream, Position, SEEK_SET);
                break;
            case Stream::Cur:
                _fseeki64(BaseStream, Position, SEEK_CUR);
                break;
            case Stream::End:
                _fseeki64(BaseStream, Position, SEEK_END);
                break;
            }

            return *this;
        }

        size_t tell() const override {
            return _ftelli64(BaseStream);
        }

        size_t size() const override {
            auto cur = tell();
            _fseeki64(BaseStream, 0, SEEK_END);
            auto ret = tell();
            _fseeki64(BaseStream, cur, SEEK_SET);
            return ret;
        }

    private:
        FILE* BaseStream;
        bool OpenAndValid;
    };
}
