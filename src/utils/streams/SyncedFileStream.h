#pragma once

#include "FileStream.h"

#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

namespace EGL3::Utils::Streams {
    class SyncedFileStream {
    public:
        bool open(const fs::path& FilePath, const char* Mode) {
            return BaseStream.open(FilePath, Mode);
        }

        bool open(const char* FilePath, const char* Mode) {
            return BaseStream.open(FilePath, Mode);
        }

        bool close() {
            return BaseStream.close();
        }

        bool valid() {
            return BaseStream.valid();
        }

        class LockedStream : public Stream {
        public:
            LockedStream(Stream& Stream, std::mutex& Mutex) :
                Stream(Stream),
                Lock(Mutex)
            {}

            Stream& write(const char* Buf, size_t BufCount) override {
                Stream.write(Buf, BufCount);
                return *this;
            }

            Stream& read(char* Buf, size_t BufCount) override {
                Stream.read(Buf, BufCount);
                return *this;
            }

            Stream& seek(size_t Position, SeekPosition SeekFrom) override {
                Stream.seek(Position, SeekFrom);
                return *this;
            }

            size_t tell() override {
                return Stream.tell();
            }

            size_t size() override {
                return Stream.size();
            }

        private:
            Stream& Stream;
            const std::lock_guard<std::mutex> Lock;
        };

        LockedStream acquire() {
            return LockedStream(BaseStream, Mutex);
        }

    private:
        FileStream BaseStream;
        std::mutex Mutex;
    };
}
