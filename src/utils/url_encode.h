#pragma once

#include <cpr/util.h>

namespace EGL3::Utils {
    static std::string UrlEncode(const std::string& Input) {
        return cpr::util::urlEncode(Input);
    }

    static std::string UrlDecode(const std::string& Input) {
        std::vector<char> dst;
        auto src = Input.data();
        auto end = Input.data() + Input.size();
        char a, b;
        while (src < end) {
            if ((*src == '%') &&
                ((a = src[1]) && (b = src[2])) &&
                (isxdigit(a) && isxdigit(b))) {
                if (a >= 'a')
                    a -= 'a' - 'A';
                if (a >= 'A')
                    a -= ('A' - 10);
                else
                    a -= '0';
                if (b >= 'a')
                    b -= 'a' - 'A';
                if (b >= 'A')
                    b -= ('A' - 10);
                else
                    b -= '0';
                dst.emplace_back(16 * a + b);
                src += 3;
            }
            else if (*src == '+') {
                dst.emplace_back(' ');
                src++;
            }
            else {
                dst.emplace_back(*src++);
            }
        }
        return std::string(dst.begin(), dst.end());
    }
}
