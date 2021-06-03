#include "Quote.h"

#include <iomanip>
#include <sstream>

namespace EGL3::Utils {
    std::string Quote(const std::string& Input)
    {
        std::ostringstream Stream;
        Stream << std::quoted(Input);
        return Stream.str();
    }

    std::string Unquote(const std::string& Input)
    {
        std::string Result;
        std::istringstream Stream(Input);
        Stream >> std::quoted(Result);
        return Result;
    }
}
