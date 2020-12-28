#include "ResourceId.h"

#include "../../utils/StringCompare.h"

#include <charconv>

namespace EGL3::Web::Xmpp::Json {
	ResourceId::ResourceId() :
		Version(ResourceIdVersion::Invalid)
	{

	}

	ResourceId::ResourceId(std::string&& Data) : Resource(std::make_unique<std::string>(std::move(Data))) {
		std::string_view ResourceView = *Resource;

		auto NumResources = std::count(ResourceView.begin(), ResourceView.end(), ':');
		if (!(
			NumResources ?
			ParseAsResource(ResourceView, NumResources) : // If any ':' exist, try to parse it as a normal resource
			ParseAsAncient(ResourceView)
			)) {
			Version = ResourceIdVersion::Invalid;
		}
	}

	std::weak_ordering ResourceId::operator<=>(const ResourceId& that) const {
		if (auto cmp = CompareApps(AppId, that.AppId); cmp != 0)
			return cmp;
		if (auto cmp = ComparePlatforms(Platform, that.Platform); cmp != 0)
			return cmp;

		return std::weak_ordering::equivalent;
	}

	constexpr std::weak_ordering ResourceId::ComparePlatforms(const std::string_view& A, const std::string_view& B) {
		if (auto cmp = FavorString(A, B, "WIN"); cmp != 0)
			return cmp;
		return Utils::CompareStringsSensitive(A, B);
	}

	constexpr std::weak_ordering ResourceId::CompareApps(const std::string_view& A, const std::string_view& B) {
		if (auto cmp = FavorString(A, B, "Fortnite"); cmp != 0)
			return cmp;
		if (auto cmp = FavorStringOver(A, B, "EGL3", "launcher"); cmp != 0)
			return cmp;
		return Utils::CompareStringsSensitive(A, B);
	}

	constexpr std::weak_ordering ResourceId::FavorString(const std::string_view& A, const std::string_view& B, const char* String) {
		if (A == String && B != String) {
			return std::weak_ordering::greater;
		}
		else if (A != String && B == String) {
			return std::weak_ordering::less;
		}
		return std::weak_ordering::equivalent;
	}

	constexpr std::weak_ordering ResourceId::FavorStringOver(const std::string_view& A, const std::string_view& B, const char* Better, const char* Worse) {
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

	const std::string& ResourceId::GetString() const {
		return *Resource;
	}

	bool ResourceId::IsValid() const {
		return Version != ResourceIdVersion::Invalid;
	}

	ResourceIdVersion ResourceId::GetResourceIdVersion() const {
		return Version;
	}

	const std::string_view& ResourceId::GetAppId() const {
		return AppId;
	}

	const std::string_view& ResourceId::GetPlatform() const {
		return Platform;
	}

	const std::string_view& ResourceId::GetPlatformUserId() const {
		return PlatformUserId;
	}

	const std::string_view& ResourceId::GetInstanceGUID() const {
		return InstanceGUID;
	}

	constexpr std::string_view::size_type ResourceId::FindNext(const std::string_view& View, std::string_view::size_type Start, char Ch) {
		return Start == std::string_view::npos ? std::string_view::npos : View.find(Ch, Start + 1);
	}

	constexpr std::string_view ResourceId::LazyInclusiveSubstr(const std::string_view& View, std::string_view::size_type Start, std::string_view::size_type End) {
		if (Start == std::string_view::npos) {
			return std::string_view();
		}
		return View.substr(Start + 1, End - Start - 1);
	}

	bool ResourceId::ParseAsResource(const std::string_view& ResourceView, int ResourcePartCount) {
		auto Sep1Itr = ResourceView.find(':');

		auto VersionString = ResourceView.substr(0, Sep1Itr);
		if (VersionString.size() <= 1 || !VersionString.starts_with('V')) {
			return false;
		}

		auto VersionParseResult = std::from_chars(VersionString.data() + 1, VersionString.data() + VersionString.size(), (uint8_t&)Version);
		if (VersionParseResult.ptr != VersionString.data() + VersionString.size()) { // If everything from the version was not parsed as a valid number
			return false;
		}

		if (Version < ResourceIdVersion::Initial) {
			return false;
		}

		if (ResourcePartCount < 3) {
			return false;
		}

		auto Sep2Itr = FindNext(ResourceView, Sep1Itr, ':');
		auto Sep3Itr = FindNext(ResourceView, Sep2Itr, ':');

		AppId = LazyInclusiveSubstr(ResourceView, Sep1Itr, Sep2Itr);
		Platform = LazyInclusiveSubstr(ResourceView, Sep2Itr, Sep3Itr);

		if (Version >= ResourceIdVersion::AddedPlatformUserId) {
			auto Sep4Itr = FindNext(ResourceView, Sep3Itr, ':');
			// For future compatability, in case extra stuff gets added after the GUID
			auto Sep5Itr = FindNext(ResourceView, Sep4Itr, ':');
			PlatformUserId = LazyInclusiveSubstr(ResourceView, Sep3Itr, Sep4Itr);
			InstanceGUID = LazyInclusiveSubstr(ResourceView, Sep4Itr, Sep5Itr);
		}
		return true;
	}

	bool ResourceId::ParseAsAncient(const std::string_view& ResourceView) {
		auto ClientSepItr = ResourceView.find('-');
		if (ClientSepItr != 0 && ClientSepItr == std::string_view::npos) {
			return false;
		}

		Version = ResourceIdVersion::Ancient;
		AppId = ResourceView.substr(0, ClientSepItr);
		// Rest after the '-' is marked "unused"

		return true;
	}
}
