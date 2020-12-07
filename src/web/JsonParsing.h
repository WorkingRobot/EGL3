#include <chrono>
#include <optional>
#include <string>

#include "../utils/date.h"

#ifndef EGL3_INCLUDED_JSON
#define EGL3_INCLUDED_JSON

namespace EGL3::Web::Json {
	namespace ch = std::chrono;

	typedef ch::system_clock::time_point TimePoint;
	
	__forceinline std::string GetCurrentTimePoint() {
		return date::format("%FT%TZ", std::chrono::system_clock::now());
	}

	__forceinline bool GetTimePoint(const char* Str, size_t StrSize, TimePoint& Obj) {
		std::istringstream istr(Str, StrSize);
		istr >> date::parse("%FT%TZ", Obj);
		return !istr.fail();
	}

	template<class T>
	__forceinline bool ParseObject(const rapidjson::Value& Json, T& Obj) {
		return T::Parse(Json, Obj);
	}

	template<class T>
	__forceinline bool ParseObject(const rapidjson::Value& Json, std::optional<T>& Obj) {
		return ParseObject(Json, Obj.emplace());
	}

	__forceinline bool ParseObject(const rapidjson::Value& Json, std::string& Obj) {
		Obj = std::string(Json.GetString(), Json.GetStringLength());
		return true;
	}

	__forceinline bool ParseObject(const rapidjson::Value& Json, bool& Obj) {
		Obj = Json.GetBool();
		return true;
	}

	__forceinline bool ParseObject(const rapidjson::Value& Json, int& Obj) {
		Obj = Json.GetInt();
		return true;
	}

	__forceinline bool ParseObject(const rapidjson::Value& Json, float& Obj) {
		Obj = Json.GetFloat();
		return true;
	}

	typedef rapidjson::Document JsonObject;

	__forceinline bool ParseObject(const rapidjson::Value& Json, JsonObject& Obj) {
		Obj.CopyFrom(Json, Obj.GetAllocator(), true);
		return true;
	}

	__forceinline bool ParseObject(const rapidjson::Value& Json, TimePoint& Obj) {
		return GetTimePoint(Json.GetString(), Json.GetStringLength(), Obj);
	}

	template<class T>
	__forceinline bool ParseObject(const rapidjson::Value& Json, std::vector<T>& Obj) {
		Obj.reserve(Json.GetArray().Size());
		for (auto& Value : Json.GetArray()) {
			auto& Item = Obj.emplace_back();
			if (!ParseObject(Value, Item)) { return false; }
		}
		return true;
	}

	template<class T>
	__forceinline bool ParseObject(const rapidjson::Value& Json, std::unordered_map<std::string, T>& Obj) {
		Obj.reserve(Json.GetObject().MemberCount());
		for (auto& Value : Json.GetObject()) {
			auto Item = Obj.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(Value.name.GetString(), Value.name.GetStringLength()),
				std::forward_as_tuple()
			);
			if (!Item.second) { return false; }
			if (!ParseObject(Value.value, Item.first->second)) { return false; }
		}
		return true;
	}
}

#endif

// Set to one to print any json parsing errors
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

#define PARSE_ITEM(JsonName, TargetVariable) \
		Itr = Json.FindMember(JsonName); \
		if (Itr == Json.MemberEnd()) { PRINT_JSON_ERROR_NOTFOUND; return false; } \
		if (!ParseObject(Itr->value, Obj.TargetVariable)) { PRINT_JSON_ERROR_PARSE; return false; }

#define PARSE_ITEM_OPT(JsonName, TargetVariable) \
		Itr = Json.FindMember(JsonName); \
		if (Itr != Json.MemberEnd()) { if (!ParseObject(Itr->value, Obj.TargetVariable)) { PRINT_JSON_ERROR_PARSE; return false; } }

#define PARSE_ITEM_ROOT(TargetVariable) \
		if (!ParseObject(Json, Obj.TargetVariable)) { PRINT_JSON_ERROR_PARSE; return false; }
