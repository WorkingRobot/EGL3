#pragma once

#include "streams/Stream.h"
#include "Log.h"

namespace EGL3::Utils {
    class JsonWrapperStream {
    public:
        JsonWrapperStream(Streams::Stream& Stream) :
            Stream(Stream)
        {

        }

        typedef char Ch;

        Ch Peek() const {
            if (Stream.size() <= Stream.tell()) {
                return '\0';
            }
            Ch Ret;
            Stream >> Ret;
            Stream.seek(-1, Streams::Stream::Cur);
            return Ret;
        }

        Ch Take() {
            if (Stream.size() <= Stream.tell()) {
                return '\0';
            }
            Ch Ret;
            Stream >> Ret;
            return Ret;
        }

        size_t Tell() {
            return Stream.tell();
        }

        Ch* PutBegin() {
            EGL3_ABORT("Insitu parsing is not supported");
            return nullptr;
        }

        void Put(Ch c) {
            EGL3_ABORT("Attempting to write to a read-only stream");
        }

        void Flush() {
            EGL3_ABORT("Attempting to flush to a read-only stream");
        }

        size_t PutEnd(Ch* begin) {
            EGL3_ABORT("Insitu parsing is not supported");
            return 0;
        }

    private:
        Streams::Stream& Stream;
    };
}
