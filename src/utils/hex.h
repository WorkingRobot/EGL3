#pragma once

#include <string>

namespace EGL3::Utils {
    template<bool Uppercase, class T, uint32_t Size, std::enable_if_t<sizeof(T) == 1 && std::is_integral_v<T>, bool> = true>
    static std::string ToHex(T(&Data)[Size]) {
        static constexpr char HexMapLower[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        static constexpr char HexMapUpper[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

        std::string s(Size * 2, ' ');
        for (int i = 0; i < Size; ++i) {
            if constexpr (Uppercase) {
                s[2 * i] = HexMapUpper[((uint8_t)Data[i] & 0xF0) >> 4];
                s[2 * i + 1] = HexMapUpper[(uint8_t)Data[i] & 0x0F];
            }
            else {
                s[2 * i] = HexMapLower[((uint8_t)Data[i] & 0xF0) >> 4];
                s[2 * i + 1] = HexMapLower[(uint8_t)Data[i] & 0x0F];
            }
        }
        return s;
    }

    static void FromHex(const char* Hex, char* Output, uint32_t OutputSize) {
        char ConvBuf[3];
        ConvBuf[2] = '\0';
        char* End = nullptr;
        for (uint32_t i = 0; i < OutputSize; ++i) {
            ConvBuf[0] = Hex[i * 2];
            ConvBuf[1] = Hex[i * 2 + 1];
            Output[i] = strtol(ConvBuf, &End, 16);
        }
    }
}