#pragma once

#include <stdint.h>
#include <string>

template<uint32_t Size>
std::string ToHex(uint8_t Data[Size]) {
    constexpr char HexMap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    std::string s(Size * 2, ' ');
    for (int i = 0; i < Size; ++i) {
        s[2 * i] = HexMap[(Data[i] & 0xF0) >> 4];
        s[2 * i + 1] = HexMap[Data[i] & 0x0F];
    }
    return s;
}