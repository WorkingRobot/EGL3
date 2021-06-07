#pragma once

#include <regex>

template<class BidirIt, class CharT>
struct std::formatter<std::sub_match<BidirIt>, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
    template<class FormatContext>
    auto format(const std::sub_match<BidirIt>& Match, FormatContext& Ctx) {
        return std::formatter<std::basic_string_view<CharT>, CharT>::format({ Match.first, Match.second }, Ctx);
    }
};
