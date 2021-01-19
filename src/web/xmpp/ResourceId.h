#pragma once

#include <memory>
#include <string_view>

namespace EGL3::Web::Xmpp::Json {
    enum class ResourceIdVersion : uint8_t {
        Invalid = 0,
        Initial = 1,
        AddedPlatformUserId = 2,

        LatestPlusOne,
        Latest = LatestPlusOne - 1,
        Ancient = LatestPlusOne
    };

    struct ResourceId {
        ResourceId();

        // https://github.com/EpicGames/UnrealEngine/blob/c1f54c5103e418b88e8b981a6b20da4c88aa3245/Engine/Source/Runtime/Online/XMPP/Private/XmppConnection.cpp#L45
        ResourceId(std::string&& Data);

        std::weak_ordering operator<=>(const ResourceId& that) const;

        static constexpr std::weak_ordering ComparePlatforms(const std::string_view& A, const std::string_view& B);

        // EGL is worst
        // EGL3 is better
        // Other games are even better
        // Fortnite is best
        static constexpr std::weak_ordering CompareApps(const std::string_view& A, const std::string_view& B);

        static constexpr std::weak_ordering FavorString(const std::string_view& A, const std::string_view& B, const char* String);

        // If one is neither of the two given, that string is the better
        // If they are both either Better or Worse, pick the Better
        static constexpr std::weak_ordering FavorStringOver(const std::string_view& A, const std::string_view& B, const char* Better, const char* Worse);

        const std::string& GetString() const;

        bool IsValid() const;

        ResourceIdVersion GetResourceIdVersion() const;

        const std::string_view& GetAppId() const;

        const std::string_view& GetPlatform() const;

        const std::string_view& GetPlatformUserId() const;

        const std::string_view& GetInstanceGUID() const;

    private:
        // Easier subsequent find() functions
        static constexpr std::string_view::size_type FindNext(const std::string_view& View, std::string_view::size_type Start, char Ch);

        // This is a variation of std::basic_string_view::substr where
        //  - both Start and End are iterators
        //  - Start can be npos (where it returns default)
        //  - Start is inclusive, where it advances an extra character compared to the original substr
        static constexpr std::string_view LazyInclusiveSubstr(const std::string_view& View, std::string_view::size_type Start, std::string_view::size_type End);

        bool ParseAsResource(const std::string_view& ResourceView, int ResourcePartCount);

        bool ParseAsAncient(const std::string_view& ResourceView);

        // When ResourceId is moved, the string_view types get invalidated
        // Making this a unique ptr fixes that :)
        std::unique_ptr<std::string> Resource;

        ResourceIdVersion Version;
        std::string_view AppId;
        std::string_view Platform;
        std::string_view PlatformUserId;
        std::string_view InstanceGUID;
    };
}
