#include "ResourceId.h"

#include "../../utils/Hex.h"
#include "../../utils/Random.h"

#include <charconv>
#include <format>

namespace EGL3::Web::Xmpp {
    std::string CreateInstanceGuid() {
        char Guid[16];
        Utils::GenerateRandomGuid(Guid);
        return Utils::ToHex<true>(Guid);
    }

    ResourceId::ResourceId(const std::string& AppId, const std::string& Platform, const std::string& PlatformUserId) :
        ResourceId(AppId, Platform, PlatformUserId, CreateInstanceGuid())
    {

    }


    ResourceId::ResourceId(const std::string& AppId, const std::string& Platform, const std::string& PlatformUserId, const std::string& InstanceGuid) :
        ResourceId(CreateResourceId(AppId, Platform, PlatformUserId, InstanceGuid))
    {

    }

    ResourceId::ResourceId(const std::string& DataRef) :
        Data(DataRef)
    {
        auto NumResources = std::count(Data.begin(), Data.end(), ':');
        if (!(
            NumResources ?
            ParseAsResource(NumResources) : // If any ':' exist, try to parse it as a normal resource
            ParseAsAncient()
            )) {
            Version = ResourceIdVersion::Invalid;
        }
    }

    std::string ResourceId::CreateResourceId(const std::string& AppId, const std::string& Platform, const std::string& PlatformUserId, const std::string& InstanceGuid) {
        return std::format("V2:{}:{}:{}:{}", AppId, Platform, PlatformUserId, InstanceGuid);
    }

    constexpr size_t FindNext(const std::string_view& View, size_t Start, char Ch) {
        return Start == std::string_view::npos ? std::string_view::npos : View.find(Ch, Start + 1);
    }

    constexpr std::string_view LazyInclusiveSubstr(const std::string_view& View, size_t Start, size_t End) {
        if (Start == std::string_view::npos) {
            return std::string_view();
        }
        return View.substr(Start + 1, End - Start - 1);
    }

    bool ResourceId::ParseAsResource(int ResourcePartCount) {
        auto Sep1Itr = Data.find(':');

        auto VersionString = std::string_view(Data.c_str(), Sep1Itr);
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

        auto Sep2Itr = FindNext(Data, Sep1Itr, ':');
        auto Sep3Itr = FindNext(Data, Sep2Itr, ':');

        AppId = LazyInclusiveSubstr(Data, Sep1Itr, Sep2Itr);
        Platform = LazyInclusiveSubstr(Data, Sep2Itr, Sep3Itr);

        if (Version >= ResourceIdVersion::AddedPlatformUserId) {
            auto Sep4Itr = FindNext(Data, Sep3Itr, ':');
            // For future compatability, in case extra stuff gets added after the GUID
            auto Sep5Itr = FindNext(Data, Sep4Itr, ':');
            PlatformUserId = LazyInclusiveSubstr(Data, Sep3Itr, Sep4Itr);
            InstanceGuid = LazyInclusiveSubstr(Data, Sep4Itr, Sep5Itr);
        }
        return true;
    }

    bool ResourceId::ParseAsAncient() {
        auto ClientSepItr = Data.find('-');
        if (ClientSepItr != 0 && ClientSepItr == std::string_view::npos) {
            return false;
        }

        Version = ResourceIdVersion::Ancient;
        AppId = std::string_view(Data.c_str(), ClientSepItr);
        // Rest after the '-' is marked "unused"

        return true;
    }
}