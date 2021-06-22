#include "PresenceStatus.h"

#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
    const std::string& PresenceStatus::GetStatus() const {
        return Status;
    }

    const std::string& PresenceStatus::GetSessionId() const {
        return SessionId;
    }

    const std::string& PresenceStatus::GetProductName() const {
        return ProductName;
    }

    const PresenceProperties& PresenceStatus::GetProperties() const {
        return Properties;
    }

    bool PresenceStatus::IsPlaying() const {
        return Playing;
    }

    bool PresenceStatus::IsJoinable() const {
        return Joinable;
    }

    bool PresenceStatus::HasVoiceSupport() const {
        return Joinable;
    }

    void PresenceStatus::SetProductName(const std::string& NewProduct) {
        ProductName = NewProduct;
    }

    void PresenceStatus::SetStatus(const std::string& NewStatus) {
        Status = NewStatus;
    }

    void PresenceStatus::Dump() const {
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
            (int)Buf.GetLength(), Buf.GetString()
        );
    }

    std::string PresenceStatus::GetProductImageUrl(std::string_view ProductId) {
        switch (Utils::Crc32(ProductId.data(), ProductId.size())) {
        case Utils::Crc32("EGL3"):
            return std::format("{}launcher-icon.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("Fortnite"):
            ProductId = "fortnite";
            [[fallthrough]];
        default:
            return std::format("{}launcher-resources/0.1_b76b28ed708e4efcbb6d0e843fcc6456/{}/icon.png", Web::GetHostUrl<Web::Host::UnrealEngineCdn1>(), ProductId);
        }
    }

    // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
    std::string PresenceStatus::GetPlatformImageUrl(const std::string_view Platform) {
        switch (Utils::Crc32(Platform.data(), Platform.size()))
        {
        case Utils::Crc32("PSN"):
        case Utils::Crc32("PS5"):
            return std::format("{}platforms/ps4.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("XBL"):
        case Utils::Crc32("XSX"):
            return std::format("{}platforms/xbox.png", Web::GetHostUrl<Web::Host::EGL3>());
        case Utils::Crc32("WIN"):
        case Utils::Crc32("MAC"):
        case Utils::Crc32("LIN"): // In the future? :)
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

    const char* PresenceStatus::GetPlatformName(const std::string_view Platform)
    {
        switch (Utils::Crc32(Platform.data(), Platform.size()))
        {
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
        case Utils::Crc32("LIN"): // In the future? :)
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
