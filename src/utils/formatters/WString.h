#pragma once

#include <filesystem>

template<class CharT>
struct std::formatter<std::wstring, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
    template<class FormatContext>
    auto format(const std::wstring& String, FormatContext& Ctx) {
        // This is the simplest way of doing it, it's just for logging purposes anyway
        return std::formatter<std::basic_string_view<CharT>, CharT>::format(std::filesystem::path(String).string<CharT>(), Ctx);
    }
};
