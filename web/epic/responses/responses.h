#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "../../../utils/date.h"

namespace ch = std::chrono;

typedef ch::system_clock::time_point TimePoint;

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

__forceinline bool ParseObject(const rapidjson::Value& Json, TimePoint& Obj) {
	std::istringstream istr(Json.GetString(), Json.GetStringLength());
	istr >> date::parse("%FT%TZ", Obj);
	return !istr.fail();
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

#define PARSE_DEFINE(ClassName) \
	static bool Parse(const rapidjson::Value& Json, ClassName& Obj) { \
		rapidjson::Document::ConstMemberIterator Itr;

#define PARSE_END \
		return true; \
	}

#define PARSE_ITEM(JsonName, TargetVariable) \
		Itr = Json.FindMember(JsonName); \
		if (Itr == Json.MemberEnd()) { return false; } \
		if (!ParseObject(Itr->value, Obj.TargetVariable)) { return false; }

#define PARSE_ITEM_OPT(JsonName, TargetVariable) \
		Itr = Json.FindMember(JsonName); \
		if (Itr != Json.MemberEnd()) { if (!ParseObject(Itr->value, Obj.TargetVariable)) { return false; } }

#define PARSE_ITEM_ROOT(TargetVariable) \
		if (!ParseObject(Json, Obj.TargetVariable)) { return false; }

#include "get_account.h"
#include "get_account_external_auths.h"
#include "get_assets.h"
#include "get_blocked_users.h"
#include "get_catalog_items.h"
#include "get_currencies.h"
#include "get_default_billing_account.h"
#include "get_entitlements.h"
#include "get_external_source_settings.h"
#include "get_friends.h"
#include "oauth_token.h"

#undef PARSE_DEFINE
#undef PARSE_END
#undef PARSE_ITEM
#undef PARSE_ITEM_OPT
#undef PARSE_ITEM_ROOT
