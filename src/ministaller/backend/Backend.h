#pragma once

#include <stdint.h>

namespace EGL3::Installer::Backend {
    static constexpr uint32_t FileMagic = 0xA3F4A23F;

    enum class DataVersion : uint8_t {
        Initial,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    enum class CommandType : uint8_t {
        Command,
        Registry,
        Data,

        End = 0xFF
    };
}