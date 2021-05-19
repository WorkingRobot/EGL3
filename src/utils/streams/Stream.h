#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace EGL3::Utils::Streams {
    namespace Detail {
        // MSVC doesn't have this yet, so I just used cppreference's example here
        template<class>
        struct is_scoped_enum : std::false_type {};

        template<class T>
        requires std::is_enum_v<T>
            struct is_scoped_enum<T> : std::bool_constant<
            !std::is_convertible_v<T, std::underlying_type_t<T>>
            > {};

        template <class T>
        inline constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;
    }

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
        virtual size_t tell() const = 0;
        virtual size_t size() const = 0;

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

        Stream& operator<<(const std::filesystem::path& Val) {
            *this << Val.string();
            return *this;
        }

        template<class T, std::enable_if_t<Detail::is_scoped_enum_v<T>, bool> = true>
        Stream& operator<<(T& Val) {
            write((char*)&Val, sizeof(std::underlying_type_t<T>));
            return *this;
        }

        template<class T, uint32_t Size>
        Stream& operator<<(const T(&Val)[Size]) {
            for (uint32_t i = 0; i < Size; ++i) {
                *this << Val[i];
            }
            return *this;
        }

        template<class T>
        Stream& operator<<(const std::vector<T>& Val) {
            *this << Val.size();
            for (auto& Item : Val) {
                *this << Item;
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

        template<class Clock, class Duration>
        Stream& operator<<(const std::chrono::time_point<Clock, Duration>& Val) {
            *this << Val.time_since_epoch();
            return *this;
        }

        template<class Rep, class Period>
        Stream& operator<<(const std::chrono::duration<Rep, Period>& Val) {
            *this << Val.count();
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

        Stream& operator>>(std::filesystem::path& Val) {
            std::string Tmp;
            *this >> Tmp;
            Val = Tmp;
            return *this;
        }

        template<class T, std::enable_if_t<Detail::is_scoped_enum_v<T>, bool> = true>
        Stream& operator>>(T& Val) {
            read((char*)&Val, sizeof(std::underlying_type_t<T>));
            return *this;
        }

        template<class T, uint32_t Size>
        Stream& operator>>(T(&Val)[Size]) {
            for (uint32_t i = 0; i < Size; ++i) {
                *this >> Val[i];
            }
            return *this;
        }
        
        template<class T>
        Stream& operator>>(std::vector<T>& Val) {
            Val.clear();
            size_t Size;
            *this >> Size;
            Val.reserve(Size);
            for (size_t i = 0; i < Size; ++i) {
                *this >> Val.emplace_back();
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
            K Key{};
            V Value{};
            for (size_t i = 0; i < Size; ++i) {
                *this >> Key;
                *this >> Value;
                Val.emplace(Key, Value);
            }
            return *this;
        }

        template<class Clock, class Duration>
        Stream& operator>>(std::chrono::time_point<Clock, Duration>& Val) {
            Duration Data;
            *this >> Data;
            Val = decltype(Val)(Data);
            return *this;
        }

        template<class Rep, class Period>
        Stream& operator>>(std::chrono::duration<Rep, Period>& Val) {
            Rep Data;
            *this >> Data;
            Val = decltype(Val)(Data);
            return *this;
        }
    };
}
