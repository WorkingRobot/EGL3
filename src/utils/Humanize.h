#pragma once

#include <chrono>

namespace EGL3::Utils {
    namespace {
        // Added in c++20, but msvc hasn't added this yet
        typedef std::chrono::duration<int, std::ratio<86400>> ChronoDays;
        typedef std::chrono::duration<int, std::ratio<604800>> ChronoWeeks;
        typedef std::chrono::duration<int, std::ratio<2629746>> ChronoMonths;
        typedef std::chrono::duration<int, std::ratio<31556952>> ChronoYears;

        template<class T>
        std::string ConvertDate(bool IsFutureTense, bool IsSingular, int64_t Amount) {

        }

        template<>
        std::string ConvertDate<std::chrono::milliseconds>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "1 millisecond from now";
                }
                else {
                    return std::to_string(Amount) + " milliseconds from now";
                }
            }
            else {
                if (IsSingular) {
                    return "a millisecond ago";
                }
                else {
                    return std::to_string(Amount) + " milliseconds ago";
                }
            }
        }

        template<>
        std::string ConvertDate<std::chrono::seconds>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "1 second from now";
                }
                else {
                    return std::to_string(Amount) + " seconds from now";
                }
            }
            else {
                if (IsSingular) {
                    return "a second ago";
                }
                else {
                    return std::to_string(Amount) + " seconds ago";
                }
            }
        }

        template<>
        std::string ConvertDate<std::chrono::minutes>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "1 minute from now";
                }
                else {
                    return std::to_string(Amount) + " minutes from now";
                }
            }
            else {
                if (IsSingular) {
                    return "a minute ago";
                }
                else {
                    return std::to_string(Amount) + " minutes ago";
                }
            }
        }

        template<>
        std::string ConvertDate<std::chrono::hours>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "1 hour from now";
                }
                else {
                    return std::to_string(Amount) + " hours from now";
                }
            }
            else {
                if (IsSingular) {
                    return "an hour ago";
                }
                else {
                    return std::to_string(Amount) + " hours ago";
                }
            }
        }

        template<>
        std::string ConvertDate<ChronoDays>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "tomorrow";
                }
                else {
                    return std::to_string(Amount) + " days from now";
                }
            }
            else {
                if (IsSingular) {
                    return "yesterday";
                }
                else {
                    return std::to_string(Amount) + " days ago";
                }
            }
        }

        template<>
        std::string ConvertDate<ChronoMonths>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "1 month from now";
                }
                else {
                    return std::to_string(Amount) + " months from now";
                }
            }
            else {
                if (IsSingular) {
                    return "a month ago";
                }
                else {
                    return std::to_string(Amount) + " months ago";
                }
            }
        }

        template<>
        std::string ConvertDate<ChronoYears>(bool IsFutureTense, bool IsSingular, int64_t Amount) {
            if (IsFutureTense) {
                if (IsSingular) {
                    return "1 year from now";
                }
                else {
                    return std::to_string(Amount) + " years from now";
                }
            }
            else {
                if (IsSingular) {
                    return "a year ago";
                }
                else {
                    return std::to_string(Amount) + " years ago";
                }
            }
        }

        template<class T>
        std::string ConvertDate(bool IsFutureTense, int64_t Amount) {
            if (Amount == 0) {
                return "now";
            }
            return ConvertDate<T>(IsFutureTense, Amount == 1, Amount);
        }

        template<class T>
        std::string ConvertDate(bool IsFutureTense, const std::chrono::system_clock::duration& Date) {
            auto Amount = std::chrono::duration_cast<T>(Date);
            return ConvertDate<T>(IsFutureTense, Amount.count());
        }

        template<class T>
        std::string ConvertDate(bool IsFutureTense, const T& Amount) {
            return ConvertDate<T>(IsFutureTense, Amount.count());
        }
    }

    // Sparsely based off of https://github.com/Humanizr/Humanizer/blob/2e45bca3d4bfc8c9ff651a32490c8e7676558f14/src/Humanizer/DateTimeHumanizeStrategy/DateTimeHumanizeAlgorithms.cs#L110
    std::string Humanize(const std::chrono::system_clock::time_point& Date) {
        // Primarily used by GetPageInfo::EmergencyNotice so people don't see 23434 years from now or something
        if (Date == Date.max()) {
            return "";
        }

        auto CurrentTime = std::chrono::system_clock::now();
        bool IsFutureTense = Date > CurrentTime;
        auto Distance = std::chrono::abs(CurrentTime - Date);

        if (Distance < std::chrono::milliseconds(500)) {
            return ConvertDate<std::chrono::milliseconds>(IsFutureTense, 0);
        }

        if (Distance < std::chrono::seconds(60)) {
            return ConvertDate<std::chrono::seconds>(IsFutureTense, Distance);
        }

        if (Distance < std::chrono::seconds(120)) {
            return ConvertDate<std::chrono::minutes>(IsFutureTense, 1);
        }

        if (Distance < std::chrono::minutes(60)) {
            return ConvertDate<std::chrono::minutes>(IsFutureTense, Distance);
        }

        if (Distance < std::chrono::minutes(90)) {
            return ConvertDate<std::chrono::hours>(IsFutureTense, 1);
        }

        if (Distance < std::chrono::hours(24)) {
            return ConvertDate<std::chrono::hours>(IsFutureTense, Distance);
        }

        if (Distance < ChronoMonths(1)) {
            return ConvertDate<ChronoDays>(IsFutureTense, Distance);
        }

        if (Distance < ChronoDays(345)) {
            return ConvertDate<ChronoMonths>(IsFutureTense, Distance);
        }

        if (Distance < ChronoYears(1)) {
            return ConvertDate<ChronoYears>(IsFutureTense, 1);
        }

        return ConvertDate<ChronoYears>(IsFutureTense, Distance);
    }
}