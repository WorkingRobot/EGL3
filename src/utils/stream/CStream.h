#pragma once

#include <stdint.h>
#include <string>

class CStream {
public:
    enum SeekPosition : int8_t {
        Beg,
        Cur,
        End
    };

    virtual CStream& write(const char* Buf, size_t BufCount) = 0;
    virtual CStream& read(char* Buf, size_t BufCount) = 0;
    virtual CStream& seek(size_t Position, SeekPosition SeekFrom) = 0;
    virtual size_t tell() = 0;
    virtual size_t size() = 0;

    // Write ops

    CStream& operator<<(bool Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(char Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(int8_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(int16_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(int32_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(int64_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(uint8_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(uint16_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(uint32_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(uint64_t Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(float Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(double Val) {
        write((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator<<(std::string Val) {
        write((char*)Val.c_str(), Val.size() + 1);
        return *this;
    }

    template<class T, uint32_t Size>
    CStream& operator<<(T(&Val)[Size]) {
        for (int i = 0; i < Size; ++i) {
            *this << Val[i];
        }
        return *this;
    }

    // Read ops

    CStream& operator>>(bool& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }
    
    CStream& operator>>(char& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(int8_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(int16_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(int32_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(int64_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(uint8_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(uint16_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(uint32_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(uint64_t& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(float& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(double& Val) {
        read((char*)&Val, sizeof(Val));
        return *this;
    }

    CStream& operator>>(std::string& Val) {
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
    CStream& operator>>(T (&Val)[Size]) {
        for (int i = 0; i < Size; ++i) {
            *this >> Val[i];
        }
        return *this;
    }
};