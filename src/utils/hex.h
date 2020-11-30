#pragma once

#include <string>

namespace EGL3::Utils {
    template<bool Uppercase, uint32_t Size>
    constexpr std::string ToHex(uint8_t(&Data)[Size]) {
        constexpr char HexMapLower[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        constexpr char HexMapUpper[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

        std::string s(Size * 2, ' ');
        for (int i = 0; i < Size; ++i) {
            if constexpr (Uppercase) {
                s[2 * i] = HexMapUpper[(Data[i] & 0xF0) >> 4];
                s[2 * i + 1] = HexMapUpper[Data[i] & 0x0F];
            }
            else {
                s[2 * i] = HexMapLower[(Data[i] & 0xF0) >> 4];
                s[2 * i + 1] = HexMapLower[Data[i] & 0x0F];
            }
        }
        return s;
    }
}