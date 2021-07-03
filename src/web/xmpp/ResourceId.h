#pragma once

#include <memory>
#include <string>

namespace EGL3::Web::Xmpp {
    enum class ResourceIdVersion : uint8_t {
        Invalid,
        Initial,
        AddedPlatformUserId,

        LatestPlusOne,
        Latest = LatestPlusOne - 1,
        Ancient = LatestPlusOne
    };

    struct ResourceId {
        ResourceId() = default;
        ResourceId(const ResourceId&) = delete;
        ResourceId(ResourceId&&) = delete;

        ResourceId(const std::string& AppId, const std::string& Platform, const std::string& PlatformUserId = "");

        ResourceId(const std::string& AppId, const std::string& Platform, const std::string& PlatformUserId, const std::string& InstanceGuid);

        ResourceId(const std::string& Data);

        bool IsValid() const {
            return Version != ResourceIdVersion::Invalid;
        }

        const std::string& GetString() const {
            return Data;
        }

        ResourceIdVersion GetVersion() const {
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

        const std::string_view& GetInstanceGuid() const {
            return InstanceGuid;
        }

    private:
        static std::string CreateResourceId(const std::string& AppId, const std::string& Platform, const std::string& PlatformUserId, const std::string& InstanceGuid);

        bool ParseAsResource(int ResourcePartCount);

        bool ParseAsAncient();

        std::string Data;
        ResourceIdVersion Version;
        std::string_view AppId;
        std::string_view Platform;
        std::string_view PlatformUserId;
        std::string_view InstanceGuid;
    };
}