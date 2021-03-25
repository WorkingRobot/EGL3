#pragma once

#include <chrono>
#include <compare>
#include <string>

namespace EGL3::Utils {
    namespace Detail {
        struct ci_char_traits : public std::char_traits<char> {
            static char to_upper(char ch) {
                return std::toupper((unsigned char)ch);
            }
            static bool eq(char c1, char c2) {
                return to_upper(c1) == to_upper(c2);
            }
            static bool lt(char c1, char c2) {
                return to_upper(c1) < to_upper(c2);
            }
            static int compare(const char* s1, const char* s2, std::size_t n) {
                while (n-- != 0) {
                    if (to_upper(*s1) < to_upper(*s2)) return -1;
                    if (to_upper(*s1) > to_upper(*s2)) return 1;
                    ++s1; ++s2;
                }
                return 0;
            }
            static const char* find(const char* s, std::size_t n, char a) {
                auto const ua(to_upper(a));
                while (n-- != 0)
                {
                    if (to_upper(*s) == ua)
                        return s;
                    s++;
                }
                return nullptr;
            }
        };

        template<class DstTraits>
        constexpr std::basic_string_view<char, DstTraits> traits_cast(const char* Ptr, const size_t Size) noexcept
        {
            return { Ptr, Size };
        }

        template<class Traits>
        constexpr static int TraitsCompare(const char* A, const size_t ASize, const char* B, const size_t BSize) noexcept {
            const int Cmp = Traits::compare(A, B, std::min(ASize, BSize));
            if (Cmp != 0) {
                return Cmp;
            }
            if (ASize < BSize) {
                return -1;
            }
            if (ASize > BSize) {
                return 1;
            }
            return 0;
        }

        template<class Traits>
        constexpr static size_t TraitsContains(const char* A, const size_t ASize, const char* B, const size_t BSize) noexcept {
            return traits_cast<Traits>(A, ASize).find(traits_cast<Traits>(B, BSize));
        }

        // A <=> B
        // Workaround until visual studio decides to implement <=> for std::basic_string
        template<bool Insensitive, class Str, class Traits = typename std::conditional<Insensitive, ci_char_traits, std::char_traits<char>>::type>
        constexpr static std::strong_ordering CompareStrings(const Str& A, const Str& B) {
            return 0 <=> TraitsCompare<Traits>(A.data(), A.size(), B.data(), B.size());
        }

        template<bool Insensitive, class Str, class Traits = typename std::conditional<Insensitive, ci_char_traits, std::char_traits<char>>::type>
        constexpr static size_t ContainsStrings(const Str& A, const Str& B) {
            return TraitsContains<Traits>(A.data(), A.size(), B.data(), B.size());
        }
    }

    template<class Str>
    constexpr static std::strong_ordering CompareStringsSensitive(const Str& A, const Str& B) {
        return Detail::CompareStrings<false>(A, B);
    }

    template<class Str>
    constexpr static std::weak_ordering CompareStringsInsensitive(const Str& A, const Str& B) {
        return Detail::CompareStrings<true>(A, B);
    }

    template<class Str>
    constexpr static size_t ContainsStringsInsensitive(const Str& A, const Str& B) {
        return Detail::ContainsStrings<true>(A, B);
    }

    template<class Clock>
    constexpr static std::strong_ordering CompareTimePoint(const std::chrono::time_point<Clock>& A, const std::chrono::time_point<Clock>& B) {
        return A.time_since_epoch().count() <=> B.time_since_epoch().count();
    }
}
