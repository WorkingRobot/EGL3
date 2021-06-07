#pragma once

#include <sstream>

template<class CharT>
struct std::formatter<std::basic_stringstream<CharT>, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
    template<class FormatContext>
    auto format(const std::basic_stringstream<CharT>& Stream, FormatContext& Ctx) {
        return std::formatter<std::basic_string_view<CharT>, CharT>::format(Stream.view(), Ctx);
    }
};
