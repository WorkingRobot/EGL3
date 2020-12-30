#pragma once

#include "../utils/Crc32.h"
#include "../utils/date.h"
#include "../utils/map.h"

#include <chrono>
#include <optional>
#include <string>
#include <rapidjson/document.h>
#include <vector>
#include <unordered_map>

namespace EGL3::Web {
    using TimePoint = std::chrono::system_clock::time_point;
    using JsonObject = rapidjson::Document;

    template<class Enum, class Converter, std::enable_if_t<std::is_enum_v<Enum>, bool> = true>
    struct JsonEnum {
        constexpr JsonEnum() : Value() {}

        constexpr JsonEnum(Enum Val) : Value(Val) {}

        constexpr operator Enum() const { return Value; }

        constexpr const char* ToString() const {
            return Converter::ToString(Value);
        }

        static constexpr Enum ToEnum(const char* String, size_t StringSize) {
            return Converter::ToEnum(String, StringSize);
        }

    private:
        Enum Value;
    };

    __forceinline std::string GetTimePoint(const TimePoint& Time) {
        return date::format("%FT%TZ", Time);
    }
    
    __forceinline std::string GetCurrentTimePoint() {
        return GetTimePoint(TimePoint::clock::now());
    }

    __forceinline bool GetTimePoint(const char* Str, size_t StrSize, TimePoint& Obj) {
        std::istringstream istr(std::string(Str, StrSize));
        istr >> date::parse("%FT%TZ", Obj);
        return !istr.fail();
    }

    template<typename T>
    struct Parser {
        __forceinline bool operator()(const rapidjson::Value& Json, T& Obj) const {
            return T::Parse(Json, Obj);
        }
    };

    template<>
    struct Parser<std::string> {
        __forceinline bool operator()(const rapidjson::Value& Json, std::string& Obj) const {
            Obj = std::string(Json.GetString(), Json.GetStringLength());
            return true;
        }
    };

    template<>
    struct Parser<bool> {
        __forceinline bool operator()(const rapidjson::Value& Json, bool& Obj) const {
            Obj = Json.GetBool();
            return true;
        }
    };

    template<>
    struct Parser<int> {
        __forceinline bool operator()(const rapidjson::Value& Json, int& Obj) const {
            Obj = Json.GetInt();
            return true;
        }
    };

    template<>
    struct Parser<float> {
        __forceinline bool operator()(const rapidjson::Value& Json, float& Obj) const {
            Obj = Json.GetFloat();
            return true;
        }
    };

    template<>
    struct Parser<JsonObject> {
        __forceinline bool operator()(const rapidjson::Value& Json, JsonObject& Obj) const {
            Obj.CopyFrom(Json, Obj.GetAllocator(), true);
            return true;
        }
    };

    template<>
    struct Parser<TimePoint> {
        __forceinline bool operator()(const rapidjson::Value& Json, TimePoint& Obj) const {
            return GetTimePoint(Json.GetString(), Json.GetStringLength(), Obj);
        }
    };

    template<typename T, typename C>
    struct Parser<JsonEnum<T, C>> {
        __forceinline bool operator()(const rapidjson::Value& Json, JsonEnum<T, C>& Obj) const {
            Obj = JsonEnum<T, C>::ToEnum(Json.GetString(), Json.GetStringLength());
            return true;
        }
    };

    template<typename T>
    struct Parser<std::optional<T>> {
        __forceinline bool operator()(const rapidjson::Value& Json, std::optional<T>& Obj) const {
            return Parser<T>{}(Json, Obj.emplace());
        }
    };

    template<typename T>
    struct Parser<std::vector<T>> {
        __forceinline bool operator()(const rapidjson::Value& Json, std::vector<T>& Obj) const {
            Obj.reserve(Json.GetArray().Size());
            for (auto& Value : Json.GetArray()) {
                auto& Item = Obj.emplace_back();
                if (!Parser<T>{}(Value, Item)) { return false; }
            }
            return true;
        }
    };

    template<typename T>
    struct Parser<std::unordered_map<std::string, T>> {
        __forceinline bool operator()(const rapidjson::Value& Json, std::unordered_map<std::string, T>& Obj) const {
            Obj.reserve(Json.GetObject().MemberCount());
            for (auto& Value : Json.GetObject()) {
                auto Item = Obj.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(Value.name.GetString(), Value.name.GetStringLength()),
                    std::forward_as_tuple()
                );
                if (!Item.second) { return false; }
                if (!Parser<T>{}(Value.value, Item.first->second)) { return false; }
            }
            return true;
        }
    };
}

#define DEFINE_JSON_ENUM_MAP1(V) case Enum::V: return #V;
#define DEFINE_JSON_ENUM_MAP2(V) case Utils::Crc32(#V): return Enum::V;
#define DEFINE_JSON_ENUM(ClassName, ...) \
enum class ClassName { \
    __VA_ARGS__, \
    UNKNOWN \
}; \
template<class Enum = ClassName> \
struct ClassName##Converter { \
    static constexpr const char* ToString(Enum Val) { \
        switch (Val) \
        { \
        FOR_EACH(DEFINE_JSON_ENUM_MAP1, __VA_ARGS__) \
        default: return "UNKNOWN"; \
        } \
    } \
    static constexpr Enum ToEnum(const char* String, size_t StringSize) { \
        switch (Utils::Crc32(String, StringSize)) { \
        FOR_EACH(DEFINE_JSON_ENUM_MAP2, __VA_ARGS__) \
        default: return Enum::UNKNOWN; \
        } \
    } \
}; \
typedef JsonEnum<ClassName, ClassName##Converter<>> ClassName##Json;

// Set to not print any json parsing errors
#ifdef EGL3_DISABLE_JSON_VERBOSITY
#define PRINT_JSON_ERROR_NOTFOUND
#define PRINT_JSON_ERROR_PARSE
#else
#define PRINT_JSON_ERROR_NOTFOUND printf("JSON parsing error (not found) at %d @ %s\n", __LINE__, __FILE__)
#define PRINT_JSON_ERROR_PARSE printf("JSON parsing error (bad parse) at %d @ %s\n", __LINE__, __FILE__)
#endif

#define PARSE_DEFINE(ClassName) \
    static bool Parse(const rapidjson::Value& Json, ClassName& Obj) { \
        rapidjson::Document::ConstMemberIterator Itr;

#define PARSE_END \
        return true; \
    }

#define PARSE_BASE(BaseClass) \
        if (!BaseClass::Parse(Json, Obj)) { PRINT_JSON_ERROR_PARSE; return false; }

#define PARSE_ITEM(JsonName, TargetVariable) \
        Itr = Json.FindMember(JsonName); \
        if (Itr == Json.MemberEnd()) { PRINT_JSON_ERROR_NOTFOUND; return false; } \
        if (!Parser<decltype(Obj.TargetVariable)>{}(Itr->value, Obj.TargetVariable)) { PRINT_JSON_ERROR_PARSE; return false; }

#define PARSE_ITEM_OPT(JsonName, TargetVariable) \
        Itr = Json.FindMember(JsonName); \
        if (Itr != Json.MemberEnd()) { if (!Parser<decltype(Obj.TargetVariable)>{}(Itr->value, Obj.TargetVariable)) { PRINT_JSON_ERROR_PARSE; return false; } }

#define PARSE_ITEM_DEF(JsonName, TargetVariable, Default) \
        Itr = Json.FindMember(JsonName); \
        if (Itr != Json.MemberEnd()) { if (!Parser<decltype(Obj.TargetVariable)>{}(Itr->value, Obj.TargetVariable)) { Obj.TargetVariable = Default; } }

#define PARSE_ITEM_ROOT(TargetVariable) \
        if (!Parser<decltype(Obj.TargetVariable)>{}(Json, Obj.TargetVariable)) { PRINT_JSON_ERROR_PARSE; return false; }
