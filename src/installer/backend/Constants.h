#pragma once

#include <string_view>

namespace EGL3::Installer::Backend {
    static constexpr uint32_t FileMagic = 0x044C4733;
    static constexpr uint64_t BufferSize = 1 << 17; // 128 kb

    std::string_view GetInstallVersion();
    std::wstring_view GetInstallVersionWide();
}