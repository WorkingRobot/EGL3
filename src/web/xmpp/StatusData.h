#pragma once

#include "../../utils/Crc32.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
	// There's a bunch of enums that this is based off of,
	// but the most high level one is EOnlinePresenceState
	// https://github.com/EpicGames/UnrealEngine/blob/f8f4b403eb682ffc055613c7caf9d2ba5df7f319/Engine/Source/Runtime/Online/XMPP/Private/XmppJingle/XmppPresenceJingle.cpp#L170
	enum class ShowStatus {
		Online,
		Chat, // chat
		Away, // away
		ExtendedAway, // xa
		DoNotDisturb, // dnd
		Offline,
	};

	struct KairosProfile {
		static constexpr char* DefaultAvatar = "cid_001_athena_commando_m_default";
		static constexpr char* DefaultBackground = R"(["#FF81AE","#D8033C","#790625"])";

		std::string Avatar = DefaultAvatar;

		std::string Background = DefaultBackground;

		std::optional<std::string> AppInstalled;

		static std::string StaticGetAvatarUrl(const std::string& Avatar) {
			return "http://cdn2.unrealengine.com/Kairos/portraits/" + Avatar + ".png?preview=1";
		}

		static std::string StaticGetBackgroundUrl(const std::string& Background) {
			auto Hash = Utils::Crc32(Background.c_str(), Background.size());
			char Buf[16];
			sprintf(Buf, "%04X", Hash);
			return "https://fnbot.shop/egl3/backgrounds/" + std::string(Buf) + ".png";
		}

		std::string GetAvatarUrl() const {
			return StaticGetAvatarUrl(Avatar);
		}

		std::string GetBackgroundUrl() const {
			return StaticGetBackgroundUrl(Background);
		}

		rapidjson::StringBuffer ToProperty() const {
			rapidjson::StringBuffer Buffer;
			rapidjson::Writer Writer(Buffer);

			Writer.StartObject();

			Writer.Key("avatar");
			Writer.String(Avatar.c_str());

			Writer.Key("avatarBackground");
			Writer.String(Background.c_str());

			Writer.Key("appInstalled");
			if (AppInstalled.has_value()) {
				Writer.String(AppInstalled->c_str(), AppInstalled->size());
			}
			else {
				Writer.String("init");
			}

			Writer.EndObject();

			return Buffer;
		}

		PARSE_DEFINE(KairosProfile)
			PARSE_ITEM("avatar", Avatar)
			PARSE_ITEM("avatarBackground", Background)
			PARSE_ITEM_OPT("appInstalled", AppInstalled)
		PARSE_END
	};

	struct StatusProperties {
	private:
		JsonObject Properties;

		template<size_t SuffixSize>
		void SetValue(const std::string& Key, rapidjson::Value&& Value, const char(&Suffix)[SuffixSize]) {
			if (Properties.IsNull()) {
				Properties.SetObject();
			}

			auto KeyPtr = (char*)Properties.GetAllocator().Malloc(Key.size() + SuffixSize - 1);
			memcpy(KeyPtr, Key.c_str(), Key.size());
			memcpy(KeyPtr + Key.size(), Suffix, SuffixSize - 1);

			Properties.EraseMember(std::move(rapidjson::Value(KeyPtr, Key.size() + SuffixSize - 1)));
			Properties.AddMember(std::move(rapidjson::Value(KeyPtr, Key.size() + SuffixSize - 1)), std::move(Value), Properties.GetAllocator());
		}

		template<typename T, size_t SuffixSize>
		bool GetValue(const std::string& Key, T& Value, const char(&Suffix)[SuffixSize]) const {
			if (!Properties.IsObject()) {
				return false;
			}
			auto KeySuffixed = Key + Suffix;
			auto Itr = Properties.FindMember(KeySuffixed.c_str());
			if (Itr != Properties.MemberEnd()) {
				if constexpr (std::is_base_of<rapidjson::Value, T>::value) {
					Value.CopyFrom(Itr->value, Value.GetAllocator(), true);
				}
				else if constexpr (std::is_base_of<std::string, T>::value) {
					Value = std::string(Itr->value.GetString(), Itr->value.GetStringLength());
				}
				else {
					Value = Itr->value.Get<T>();
				}
				return true;
			}
			return false;
		}

	public:
		void SetValue(const std::string& Name, int32_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_i");
		}

		void SetValue(const std::string& Name, uint32_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_u");
		}

		void SetValue(const std::string& Name, float Value) {
			SetValue(Name, rapidjson::Value(Value), "_f");
		}

		void SetValue(const std::string& Name, const std::string& Value) {
			SetValue(Name, rapidjson::Value(Value.c_str(), Value.size(), Properties.GetAllocator()), "_s");
		}

		void SetValue(const std::string& Name, const rapidjson::StringBuffer& Value) {
			SetValue(Name, rapidjson::Value(Value.GetString(), Value.GetLength(), Properties.GetAllocator()), "_s");
		}

		void SetValue(const std::string& Name, bool Value) {
			SetValue(Name, rapidjson::Value(Value), "_b");
		}

		void SetValue(const std::string& Name, int64_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_I");
		}

		void SetValue(const std::string& Name, uint64_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_U");
		}

		void SetValue(const std::string& Name, double Value) {
			SetValue(Name, rapidjson::Value(Value), "_d");
		}

		void SetValue(const std::string& Name, const JsonObject& Value) {
			JsonObject ValueCopy;
			ValueCopy.CopyFrom(Value, ValueCopy.GetAllocator());
			SetValue(Name, std::move(ValueCopy), "_j");
		}

		template<typename Serializable>
		void SetValue(const std::string& Name, const Serializable& Value) {
			SetValue(Name, Value.ToProperty());
		}


		bool GetValue(const std::string& Name, int32_t& Value) const {
			return GetValue(Name, Value, "_i");
		}

		bool GetValue(const std::string& Name, uint32_t& Value) const {
			return GetValue(Name, Value, "_u");
		}

		bool GetValue(const std::string& Name, float& Value) const {
			return GetValue(Name, Value, "_f");
		}

		bool GetValue(const std::string& Name, std::string& Value) const {
			return GetValue(Name, Value, "_s");
		}

		bool GetValue(const std::string& Name, bool& Value) const {
			return GetValue(Name, Value, "_b");
		}

		bool GetValue(const std::string& Name, int64_t& Value) const {
			return GetValue(Name, Value, "_I");
		}

		bool GetValue(const std::string& Name, uint64_t& Value) const {
			return GetValue(Name, Value, "_U");
		}

		bool GetValue(const std::string& Name, double& Value) const {
			return GetValue(Name, Value, "_d");
		}

		bool GetValue(const std::string& Name, JsonObject& Value) const {
			return GetValue(Name, Value, "_j");
		}

		template<typename RapidJsonWriter>
		void WriteTo(RapidJsonWriter& Writer) const {
			if (Properties.IsObject()) {
				Properties.Accept(Writer);
			}
			else {
				Writer.StartObject();
				Writer.EndObject();
			}
		}

		PARSE_DEFINE(StatusProperties)
			PARSE_ITEM_ROOT(Properties)
		PARSE_END
	};

	// Everything is optional here, we need to lazy load everything, this data can be essentially anything (user submitted)
    struct StatusData {
		// This is not serialized, it's instead given as another node in the xml presence
		ShowStatus ShowStatus;

		// This is not serialized, it's instead given as another node in the xml presence
		TimePoint UpdatedTime;

		std::string Status;

		bool Playing;

		bool Joinable;

		// Rocket League doesn't wanna give it
		std::optional<bool> HasVoiceSupport;

		std::string SessionId;

		std::optional<std::string> ProductName;

		StatusProperties Properties;

		PARSE_DEFINE(StatusData)
			PARSE_ITEM("Status", Status)
			PARSE_ITEM("bIsPlaying", Playing)
			PARSE_ITEM("bIsJoinable", Joinable)
			PARSE_ITEM_OPT("bHasVoiceSupport", HasVoiceSupport)
			PARSE_ITEM_OPT("ProductName", ProductName)
			PARSE_ITEM("SessionId", SessionId)
			PARSE_ITEM("Properties", Properties)
		PARSE_END
    };
}
