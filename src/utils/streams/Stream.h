#pragma once

#include <chrono>
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace EGL3::Utils::Streams {
    class Stream {
    public:
        enum SeekPosition : int8_t {
            Beg,
            Cur,
            End
        };

        virtual Stream& write(const char* Buf, size_t BufCount) = 0;
        virtual Stream& read(char* Buf, size_t BufCount) = 0;
        virtual Stream& seek(size_t Position, SeekPosition SeekFrom) = 0;
        virtual size_t tell() = 0;
        virtual size_t size() = 0;

        // Write ops

        Stream& operator<<(bool Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(char Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(int8_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(int16_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(int32_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(int64_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(uint8_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(uint16_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(uint32_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(uint64_t Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(float Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(double Val) {
            write((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator<<(const std::string& Val) {
            write((char*)Val.c_str(), Val.size() + 1);
            return *this;
        }

        template<class T, uint32_t Size>
        Stream& operator<<(const T(&Val)[Size]) {
            for (int i = 0; i < Size; ++i) {
                *this << Val[i];
            }
            return *this;
        }

        template<class K, class V>
        Stream& operator<<(const std::pair<K, V>& Val) {
            *this << Val.first;
            *this << Val.second;
            return *this;
        }

        template<class K, class V>
        Stream& operator<<(const std::unordered_map<K, V>& Val) {
            *this << Val.size();
            for (auto& Item : Val) {
                *this << Item;
            }
            return *this;
        }

        Stream& operator<<(const std::chrono::system_clock::time_point& Val) {
            *this << Val.time_since_epoch().count();
            return *this;
        }

        // Read ops

        Stream& operator>>(bool& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(char& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(int8_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(int16_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(int32_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(int64_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(uint8_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(uint16_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(uint32_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(uint64_t& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(float& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(double& Val) {
            read((char*)&Val, sizeof(Val));
            return *this;
        }

        Stream& operator>>(std::string& Val) {
            Val.clear();
            char n;
            do {
                *this >> n;
                if (n) {
                    Val.append(1, n);
                }
            } while (n);
            return *this;
        }

        template<class T, uint32_t Size>
        Stream& operator>>(T(&Val)[Size]) {
            for (int i = 0; i < Size; ++i) {
                *this >> Val[i];
            }
            return *this;
        }

        template<class K, class V>
        Stream& operator>>(std::pair<K, V>& Val) {
            *this >> Val.first;
            *this >> Val.second;
            return *this;
        }

        template<class K, class V>
        Stream& operator>>(std::unordered_map<K, V>& Val) {
            Val.clear();
            size_t Size;
            *this >> Size;
            Val.reserve(Size);
            K Key;
            V Value;
            for (int i = 0; i < Size; ++i) {
                *this >> Key;
                *this >> Value;
                Val.emplace(Key, Value);
            }
            return *this;
        }

        Stream& operator>>(std::chrono::system_clock::time_point& Val) {
            std::chrono::system_clock::rep Data;
            *this >> Data;
            Val = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(Data));
            return *this;
        }
    };
}
