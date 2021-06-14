#include "Humanize.h"

#include "formatters/WString.h"
#include "Log.h"

#include <chrono>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Utils {
    constexpr LPCWSTR SelectedLocale = LOCALE_NAME_USER_DEFAULT;
    constexpr LPCWSTR DurationFormat = L"hh:mm:ss";

    std::wstring HumanizeDuration(uint64_t Ticks)
    {
        int64_t BufSize = GetDurationFormatEx(SelectedLocale, 0, NULL, Ticks, DurationFormat, NULL, 0);
        auto Buf = std::make_unique<wchar_t[]>(BufSize);
        GetDurationFormatEx(SelectedLocale, 0, NULL, Ticks, DurationFormat, Buf.get(), BufSize);
        return std::wstring(Buf.get(), BufSize - 1);
    }

    NUMBERFMTW GetCurrentNumberFormat(WCHAR szDecimal[16], WCHAR szThousand[16]) {
        NUMBERFMTW fmt{};
        GetLocaleInfoEx(SelectedLocale, LOCALE_IDIGITS | LOCALE_RETURN_NUMBER, reinterpret_cast<LPWSTR>(&fmt.NumDigits), sizeof(fmt.NumDigits) / sizeof(WCHAR));
        GetLocaleInfoEx(SelectedLocale, LOCALE_ILZERO | LOCALE_RETURN_NUMBER, reinterpret_cast<LPWSTR>(&fmt.LeadingZero), sizeof(fmt.LeadingZero) / sizeof(WCHAR));
        WCHAR szGrouping[32] = L"";
        GetLocaleInfoEx(SelectedLocale, LOCALE_SGROUPING, szGrouping, 32);
        if (lstrcmpW(szGrouping, L"3;0") == 0 || lstrcmpW(szGrouping, L"30") == 0) {
            fmt.Grouping = 3;
        }
        else if (lstrcmpW(szGrouping, L"3;2;0") == 0) {
            fmt.Grouping = 32;
        }
        else {
            fmt.Grouping = 3; // default
        }
        GetLocaleInfoEx(SelectedLocale, LOCALE_SDECIMAL, szDecimal, 16);
        fmt.lpDecimalSep = szDecimal;
        GetLocaleInfoEx(SelectedLocale, LOCALE_STHOUSAND, szThousand, 16);
        fmt.lpThousandSep = szThousand;
        GetLocaleInfoEx(SelectedLocale, LOCALE_INEGNUMBER | LOCALE_RETURN_NUMBER, reinterpret_cast<LPWSTR>(&fmt.NegativeOrder), sizeof(fmt.NegativeOrder) / sizeof(WCHAR));

        return fmt;
    }

    std::wstring Detail::HumanizeNumber(const std::wstring& ValueString, int Precision)
    {
        WCHAR szDecimal[16] = L"";
        WCHAR szThousand[16] = L"";
        auto fmt = GetCurrentNumberFormat(szDecimal, szThousand);
        fmt.NumDigits = std::max(Precision, 0);
        int64_t BufSize = GetNumberFormatEx(SelectedLocale, 0, ValueString.c_str(), &fmt, NULL, 0);
        if (!EGL3_ENSUREF(BufSize != 0, LogLevel::Error, "Couldn't humanize number \"{}\" (GLE: {})", ValueString, GetLastError())) {
            return L"";
        }
        auto Buf = std::make_unique<wchar_t[]>(BufSize);
        EGL3_ENSUREF(GetNumberFormatEx(SelectedLocale, 0, ValueString.c_str(), &fmt, Buf.get(), BufSize) != 0, LogLevel::Error, "Couldn't humanize number \"{}\" (GLE: {})", ValueString, GetLastError());
        return std::wstring(Buf.get(), BufSize - 1);
    }

    std::wstring HumanizeByteSize(uint64_t Size) {
        static constexpr const wchar_t* Suffixes[] = { L" B", L" KB", L" MB", L" GB", L" TB", L" PB", L" EB" };

        int i = 0;
        double SizeD = Size;
        while (SizeD >= 1024) {
            SizeD /= 1024;
            ++i;
        }

        return Humanize(SizeD, 2 - (int64_t)floor(log10(SizeD ? SizeD : 2))) + Suffixes[i];
    }

    std::wstring HumanizeBitSize(uint64_t Size) {
        static constexpr const wchar_t* Suffixes[] = { L" b", L" Kb", L" Mb", L" Gb", L" Tb", L" Pb", L" Eb" };

        int i = 0;
        double SizeD = Size * 8;
        while (SizeD >= 1024) {
            SizeD /= 1024;
            ++i;
        }

        return Humanize(SizeD, 2 - (int64_t)floor(log10(SizeD ? SizeD : 2))) + Suffixes[i];
    }
}