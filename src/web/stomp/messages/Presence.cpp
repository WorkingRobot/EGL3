#include "Presence.h"

#include "../../../utils/StringCompare.h"
#include "../../xmpp/ResourceId.h"

namespace EGL3::Web::Stomp::Messages {
    constexpr std::weak_ordering FavorString(const std::string_view& A, const std::string_view& B, const char* String) {
        if (A == String && B != String) {
            return std::weak_ordering::greater;
        }
        else if (A != String && B == String) {
            return std::weak_ordering::less;
        }
        return std::weak_ordering::equivalent;
    }

    constexpr std::weak_ordering FavorStringOver(const std::string_view& A, const std::string_view& B, const char* Better, const char* Worse) {
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

    constexpr std::weak_ordering CompareProducts(const std::string_view& A, const std::string_view& B) {
        if (auto cmp = FavorString(A, B, "Fortnite"); cmp != 0)
            return cmp;
        if (auto cmp = FavorStringOver(A, B, "EGL3", "_"); cmp != 0)
            return cmp;
        return Utils::CompareStringsSensitive(A, B);
    }

    std::string Presence::NamespacePresence::GetProductName() const
    {
        if (ProductId.has_value()) {
            auto Itr = Properties.find("EOS_ProductName");
            if (Itr != Properties.end()) {
                return Itr->second;
            }
        }
        {
            auto Itr = Properties.find("X_NonEOS_ProductName");
            if (Itr != Properties.end()) {
                return Itr->second;
            }
        }
        return Namespace;
    }

    std::string Presence::NamespacePresence::GetPlatform() const
    {
        if (ProductId.has_value()) {
            auto Itr = Properties.find("EOS_Platform");
            if (Itr != Properties.end()) {
                return Itr->second;
            }
        }
        for (auto& Connection : Connections) {
            Web::Xmpp::ResourceId Resource(Connection.Id);
            if (Resource.IsValid()) {
                if (!Resource.GetPlatform().empty()) {
                    return std::string(Resource.GetPlatform());
                }
            }
        }
        return "";
    }

    std::weak_ordering Presence::NamespacePresence::operator<=>(const NamespacePresence& that) const
    {
        if (auto cmp = CompareProducts(this->GetProductName(), that.GetProductName()); cmp != 0)
            return cmp;
        return std::weak_ordering::equivalent;
    }

    std::string GetLegacyProductImageUrl(const std::string& ProductId)
    {
        return std::format("{}launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/{}/icon.png", Web::GetHostUrl<Web::Host::UnrealEngineCdn1>(), ProductId);
    }

    std::string Presence::NamespacePresence::GetProductImageUrl(const std::string& ProductName)
    {
        switch (Utils::Crc32(ProductName)) {
        case Utils::Crc32("EGL3"):
            return std::format("{}launcher-icon.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("Fortnite"):
            return GetLegacyProductImageUrl("fortnite");
        case Utils::Crc32("launcher"):
            return GetLegacyProductImageUrl("launcher");
        default:
            return std::format("{}launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/{}/icon.png", Web::GetHostUrl<Web::Host::UnrealEngineCdn1>(), ProductName);
        }
    }

    // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
    std::string Presence::NamespacePresence::GetPlatformImageUrl(const std::string& Platform)
    {
        switch (Utils::Crc32(Platform)) {
        case Utils::Crc32("PSN"):
        case Utils::Crc32("PS5"):
            return std::format("{}platforms/ps4.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("XBL"):
        case Utils::Crc32("XSX"):
            return std::format("{}platforms/xbox.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("WIN"):
        case Utils::Crc32("MAC"):
        case Utils::Crc32("LIN"):
        case Utils::Crc32("LNX"): // In the future? :)
            return std::format("{}platforms/pc.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("IOS"):
        case Utils::Crc32("AND"):
            return std::format("{}platforms/mobile.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("SWT"):
            return std::format("{}platforms/switch.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("OTHER"):
        default:
            return std::format("{}platforms/earth.png", Web::GetHostUrl<Web::Host::EGL3>());
        }
    }

    const char* Presence::NamespacePresence::GetPlatformName(const std::string& Platform)
    {
        switch (Utils::Crc32(Platform)) {
        case Utils::Crc32("PSN"):
            return "PS4";
        case Utils::Crc32("PS5"):
            return "PS5";
        case Utils::Crc32("XBL"):
            return "Xbox One";
        case Utils::Crc32("XSX"):
            return "Xbox Series X";
        case Utils::Crc32("WIN"):
            return "Windows";
        case Utils::Crc32("MAC"):
            return "macOS";
        case Utils::Crc32("LIN"):
        case Utils::Crc32("LNX"): // In the future? :)
            return "Linux";
        case Utils::Crc32("IOS"):
            return "iOS";
        case Utils::Crc32("AND"):
            return "Android";
        case Utils::Crc32("SWT"):
            return "Switch";
        case Utils::Crc32("OTHER"):
            return "Other";
        default:
            return "Unknown";
        }
    }
}
