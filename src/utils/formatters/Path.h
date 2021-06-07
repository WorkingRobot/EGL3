#pragma once

#include <filesystem>

template<class CharT>
struct std::formatter<std::filesystem::path, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
    template<class FormatContext>
    auto format(const std::filesystem::path& Path, FormatContext& Ctx) {
        return std::formatter<std::basic_string_view<CharT>, CharT>::format(Path.string<CharT>(), Ctx);
    }
};
