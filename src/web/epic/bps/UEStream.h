#pragma once

#include "../../../utils/streams/BufferStream.h"
#include "../../../utils/Log.h"
#include "../../../utils/Guid.h"

#include <memory>

namespace EGL3::Web::Epic::BPS {
    class UEStream : public Utils::Streams::BufferStream {
    public:
        UEStream(char* Start, size_t Size) :
            Utils::Streams::BufferStream(Start, Size)
        {

        }

        using Utils::Streams::BufferStream::operator<<;
        using Utils::Streams::BufferStream::operator>>;

        UEStream& operator<<(const std::string& Val);

        template<class T>
        UEStream& operator<<(const std::vector<T>& Val);



        UEStream& operator>>(Utils::Guid& Val) {
            *this >> Val.A;
            *this >> Val.B;
            *this >> Val.C;
            *this >> Val.D;

            return *this;
        }

        UEStream& operator>>(std::string& Val) {
            // > 0 for ANSICHAR, < 0 for UCS2CHAR serialization
            int SaveNum;
            *this >> SaveNum;

            if (SaveNum == 0) {
                Val.clear();
                return *this; // blank
            }

            if (SaveNum < 0) // LoadUCS2Char
            {
                // If SaveNum cannot be negated due to integer overflow, Ar is corrupted.
                EGL3_VERIFY(SaveNum != std::numeric_limits<int>::min(), "Tried to read FString with an invalid length");

                SaveNum = -SaveNum;

                std::unique_ptr<char16_t[]> StringData = std::make_unique<char16_t[]>(SaveNum);
                read((char*)StringData.get(), SaveNum * sizeof(char16_t));
                Val = { StringData.get(), StringData.get() + SaveNum };
            }
            else {
                Val.resize(SaveNum - 1);
                read(Val.data(), SaveNum);
            }

            return *this;
        }

        template<class T>
        UEStream& operator>>(std::vector<T>& Val) {
            int SerializeNum;
            *this >> SerializeNum;
            Val.reserve(SerializeNum);
            for (int i = 0; i < SerializeNum; ++i) {
                *this >> Val.emplace_back();
            }
            return *this;
        }
    };
}
