#pragma once

#include "../../utils/Crc32.h"
#include "../../utils/StringCompare.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <charconv>

namespace EGL3::Web::Xmpp::Json {
	// There's a bunch of enums that this is based off of,
	// but the most high level one is EOnlinePresenceState
	// https://github.com/EpicGames/UnrealEngine/blob/f8f4b403eb682ffc055613c7caf9d2ba5df7f319/Engine/Source/Runtime/Online/XMPP/Private/XmppJingle/XmppPresenceJingle.cpp#L170
	enum class ShowStatus : uint8_t {
		DoNotDisturb, // dnd
		Chat, // chat
		Online,
		Away, // away
		ExtendedAway, // xa
		Offline,
	};

	struct PresenceKairosProfile {
		static constexpr char* DefaultAvatar = "cid_001_athena_commando_m_default";
		static constexpr char* DefaultBackground = R"(["#FF81AE","#D8033C","#790625"])";

		std::string Avatar = DefaultAvatar;

		std::string Background = DefaultBackground;

		std::optional<std::string> AppInstalled;

		static std::string GetKairosAvatarUrl(const std::string& Avatar) {
			return "http://cdn2.unrealengine.com/Kairos/portraits/" + Avatar + ".png?preview=1";
		}

		static std::string GetKairosBackgroundUrl(const std::string& Background) {
			auto Hash = Utils::Crc32(Background.c_str(), Background.size());
			char Buf[16];
			sprintf(Buf, "%04X", Hash);
			return "https://fnbot.shop/egl3/backgrounds/" + std::string(Buf) + ".png";
		}

		rapidjson::StringBuffer ToProperty() const {
			rapidjson::StringBuffer Buffer;
			rapidjson::Writer Writer(Buffer);

			Writer.StartObject();

			Writer.Key("avatar");
			Writer.String(Avatar.c_str());

			Writer.Key("avatarBackground");
			Writer.String(Background.c_str());

			if (AppInstalled.has_value()) {
				Writer.Key("appInstalled");
				Writer.String(AppInstalled->c_str(), AppInstalled->size());
			}

			Writer.EndObject();

			return Buffer;
		}

		PARSE_DEFINE(PresenceKairosProfile)
			PARSE_ITEM_DEF("avatar", Avatar, DefaultAvatar)
			PARSE_ITEM_DEF("avatarBackground", Background, DefaultBackground)
			PARSE_ITEM_OPT("appInstalled", AppInstalled)
		PARSE_END
	};

	enum class ResourceVersion : uint8_t {
		Invalid = 0,
		Initial = 1,
		AddedPlatformUserId = 2,

		LatestPlusOne,
		Latest = LatestPlusOne - 1,
		Ancient = LatestPlusOne
	};

	struct ResourceId {
		ResourceId() : Version(ResourceVersion::Invalid) {}

		// https://github.com/EpicGames/UnrealEngine/blob/c1f54c5103e418b88e8b981a6b20da4c88aa3245/Engine/Source/Runtime/Online/XMPP/Private/XmppConnection.cpp#L45
		ResourceId(std::string&& Data) : Resource(std::make_unique<std::string>(std::move(Data))) {
			std::string_view ResourceView = *Resource;

			auto NumResources = std::count(ResourceView.begin(), ResourceView.end(), ':');
			if (!(
				NumResources ?
					ParseAsResource(ResourceView, NumResources) : // If any ':' exist, try to parse it as a normal resource
					ParseAsAncient(ResourceView)
				)) {
				Version = ResourceVersion::Invalid;
			}
		}

		std::weak_ordering operator<=>(const ResourceId& that) const {
			if (auto cmp = CompareApps(AppId, that.AppId); cmp != 0)
				return cmp;
			if (auto cmp = ComparePlatforms(Platform, that.Platform); cmp != 0)
				return cmp;

			return std::weak_ordering::equivalent;
		}

		constexpr static std::weak_ordering ComparePlatforms(const std::string_view& A, const std::string_view& B) {
			return FavorString(A, B, "WIN");
		}

		// EGL is worst
		// EGL3 is better
		// Other games are even better
		// Fortnite is best
		constexpr static std::weak_ordering CompareApps(const std::string_view& A, const std::string_view& B) {
			if (auto cmp = FavorString(A, B, "Fortnite"); cmp != 0)
				return cmp;
			if (auto cmp = FavorStringOver(A, B, "EGL3", "launcher"); cmp != 0)
				return cmp;
			return Utils::CompareStrings(A, B);
		}

		constexpr static std::weak_ordering FavorString(const std::string_view& A, const std::string_view& B, const char* String) {
			if (A == String && B != String) {
				return std::weak_ordering::greater;
			}
			else if (B != String && B == String) {
				return std::weak_ordering::less;
			}
			return std::weak_ordering::equivalent;
		}

		// If one is neither of the two given, that string is the better
		// If they are both either Better or Worse, pick the Better
		constexpr static std::weak_ordering FavorStringOver(const std::string_view& A, const std::string_view& B, const char* Better, const char* Worse) {
			bool AOk = A != Better && A != Worse;
			bool BOk = B != Better && B != Worse;
			if (auto cmp = AOk <=> BOk; cmp != 0) {
				return cmp;
			}
			if (A == Better && B == Worse) {
				return std::weak_ordering::greater;
			}
			else if (B == Better && A == Worse) {
				return std::weak_ordering::less;
			}
			return std::weak_ordering::equivalent;
		}

		const std::string& GetString() const {
			return *Resource;
		}

		bool IsValid() const {
			return Version != ResourceVersion::Invalid;
		}

		ResourceVersion GetResourceVersion() const {
			return Version;
		}

		const std::string_view& GetAppId() const {
			return AppId;
		}

		const std::string_view& GetPlatform() const {
			return Platform;
		}

		const std::string_view& GetPlatformUserId() const {
			return PlatformUserId;
		}

		const std::string_view& GetInstanceGUID() const {
			return InstanceGUID;
		}

	private:
		// Easier subsequent find() functions
		static constexpr std::string_view::size_type FindNext(const std::string_view& View, std::string_view::size_type Start, char Ch) {
			return Start == std::string_view::npos ? std::string_view::npos : View.find(Ch, Start + 1);
		}

		// This is a variation of std::basic_string_view::substr where
		//  - both Start and End are iterators
		//  - Start can be npos (where it returns default)
		//  - Start is inclusive, where it advances an extra character compared to the original substr
		static constexpr std::string_view LazyInclusiveSubstr(const std::string_view& View, std::string_view::size_type Start, std::string_view::size_type End) {
			if (Start == std::string_view::npos) {
				return std::string_view();
			}
			return View.substr(Start + 1, End - Start - 1);
		}

		bool ParseAsResource(const std::string_view& ResourceView, int ResourcePartCount) {
			auto Sep1Itr = ResourceView.find(':');

			auto VersionString = ResourceView.substr(0, Sep1Itr);
			if (VersionString.size() <= 1 || !VersionString.starts_with('V')) {
				return false;
			}

			auto VersionParseResult = std::from_chars(VersionString.data() + 1, VersionString.data() + VersionString.size(), (uint8_t&)Version);
			if (VersionParseResult.ptr != VersionString.data() + VersionString.size()) { // If everything from the version was not parsed as a valid number
				return false;
			}

			if (Version < ResourceVersion::Initial) {
				return false;
			}

			if (ResourcePartCount < 3) {
				return false;
			}

			auto Sep2Itr = FindNext(ResourceView, Sep1Itr, ':');
			auto Sep3Itr = FindNext(ResourceView, Sep2Itr, ':');

			AppId = LazyInclusiveSubstr(ResourceView, Sep1Itr, Sep2Itr);
			Platform = LazyInclusiveSubstr(ResourceView, Sep2Itr, Sep3Itr);

			if (Version >= ResourceVersion::AddedPlatformUserId) {
				auto Sep4Itr = FindNext(ResourceView, Sep3Itr, ':');
				// For future compatability, in case extra stuff gets added after the GUID
				auto Sep5Itr = FindNext(ResourceView, Sep4Itr, ':');
				PlatformUserId = LazyInclusiveSubstr(ResourceView, Sep3Itr, Sep4Itr);
				InstanceGUID = LazyInclusiveSubstr(ResourceView, Sep4Itr, Sep5Itr);
			}
			return true;
		}

		bool ParseAsAncient(const std::string_view& ResourceView) {
			auto ClientSepItr = ResourceView.find('-');
			if (ClientSepItr != 0 && ClientSepItr == std::string_view::npos) {
				return false;
			}

			Version = ResourceVersion::Ancient;
			AppId = ResourceView.substr(0, ClientSepItr);
			// Rest after the '-' is marked "unused"

			return true;
		}

		// When ResourceId is moved, the string_view types get invalidated
		// Making this a unique ptr fixes that :)
		std::unique_ptr<std::string> Resource;

		ResourceVersion Version;
		std::string_view AppId;
		std::string_view Platform;
		std::string_view PlatformUserId;
		std::string_view InstanceGUID;
	};

	struct PresenceProperties {
		PresenceProperties() = default;

		PresenceProperties(const PresenceProperties& that) {
			Properties.CopyFrom(that.Properties, Properties.GetAllocator(), true);
		}

		PresenceProperties& operator=(const PresenceProperties& that) {
			Properties.CopyFrom(that.Properties, Properties.GetAllocator(), true);
			return *this;
		}

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

		PARSE_DEFINE(PresenceProperties)
			PARSE_ITEM_ROOT(Properties)
		PARSE_END
	};

	struct PresenceStatus {
		const std::string& GetStatus() const {
			return Status;
		}

		const std::string& GetSessionId() const {
			return SessionId;
		}

		const std::string& GetProductName() const {
			return ProductName;
		}

		const PresenceProperties& GetProperties() const {
			return Properties;
		}

		bool IsPlaying() const {
			return Playing;
		}

		bool IsJoinable() const {
			return Joinable;
		}

		bool HasVoiceSupport() const {
			return Joinable;
		}

		const PresenceKairosProfile* GetKairosProfile() const {
			if (!KairosProfileParsed) {
				std::string KairosString;
				if (Properties.GetValue("KairosProfile", KairosString)) {
					rapidjson::Document KairosJson;
					KairosJson.Parse(KairosString.c_str(), KairosString.size());
					if (!KairosJson.HasParseError()) {
						if (!PresenceKairosProfile::Parse(KairosJson, KairosProfile.emplace())) {
							KairosProfile.reset();
							// TODO: Provide warning here and basically everywhere else with invalid json relating to presences
						}
					}
				}
				KairosProfileParsed = true;
			}
			return KairosProfile.has_value() ? &KairosProfile.value() : nullptr;
		}

		void SetStatus(const std::string& NewStatus) {
			Status = NewStatus;
		}

		void SetKairosProfile(const PresenceKairosProfile& NewProfile) {
			KairosProfileParsed = true;
			Properties.SetValue("KairosProfile", KairosProfile.emplace(NewProfile));
		}

		void Dump() const {
			rapidjson::StringBuffer Buf;
			{
				rapidjson::Writer Writer(Buf);
				Properties.WriteTo(Writer);
			}

			printf("Status: %s\nPlaying: %s\nJoinable: %s\nVoice: %s\nSession Id: %s\nProduct: %s\nProperties: %.*s\n",
				Status.c_str(),
				Playing ? "Yes" : "No",
				Joinable ? "Yes" : "No",
				VoiceSupport ? "Yes" : "No",
				SessionId.c_str(),
				ProductName.c_str(),
				Buf.GetLength(), Buf.GetString()
			);
		}
		
		PARSE_DEFINE(PresenceStatus)
			PARSE_ITEM_DEF("Status", Status, "")
			PARSE_ITEM_DEF("bIsPlaying", Playing, false)
			PARSE_ITEM_DEF("bIsJoinable", Joinable, false)
			PARSE_ITEM_DEF("bHasVoiceSupport", VoiceSupport, false)
			PARSE_ITEM_DEF("ProductName", ProductName, "")
			PARSE_ITEM_DEF("SessionId", SessionId, "")
			PARSE_ITEM_DEF("Properties", Properties, PresenceProperties())
		PARSE_END

	private:
		std::string Status;

		bool Playing;

		bool Joinable;

		bool VoiceSupport;

		std::string SessionId;

		std::string ProductName;

		PresenceProperties Properties;
		
		// Dynamically made
		mutable bool KairosProfileParsed = false;
		mutable std::optional<PresenceKairosProfile> KairosProfile;
	};

	struct Presence {
		ResourceId Resource;

		ShowStatus ShowStatus;

		TimePoint LastUpdated;

		PresenceStatus Status;

		std::weak_ordering operator<=>(const Presence& that) const {
			return Resource <=> that.Resource;
		}

		void Dump() const {
			printf("Resource: %s\nStatus: %d\nTime: %s\n",
				Resource.GetString().c_str(),
				(int)ShowStatus,
				GetTimePoint(LastUpdated).c_str()
			);
			Status.Dump();
			printf("\n");
		}
	};
}
