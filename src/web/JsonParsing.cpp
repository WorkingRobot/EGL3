#include "JsonParsing.h"

#include "../utils/date.h"

namespace EGL3::Web {
    bool from_stream(const std::string& String, TimePoint& Obj)
    {
        std::istringstream istr(String);
        date::from_stream(istr, "%FT%TZ", Obj);
        return !istr.fail();
    }
}