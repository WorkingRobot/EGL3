#pragma once

#include "PresenceProperties.h"

namespace EGL3::Web::Xmpp::Json {
    struct PresenceStatus {
        const std::string& GetStatus() const;

        const std::string& GetSessionId() const;

        const std::string& GetProductName() const;

        const PresenceProperties& GetProperties() const;

        bool IsPlaying() const;

        bool IsJoinable() const;

        bool HasVoiceSupport() const;

        void SetProductName(const std::string& NewProduct);

        void SetStatus(const std::string& NewStatus);

        void Dump() const;

        static std::string GetProductImageUrl(std::string_view ProductId);

        // https://github.com/EpicGames/UnrealEngine/blob/4da880f790851cff09ea33dadfd7aae3287878bd/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystemNames.h
        static std::string GetPlatformImageUrl(const std::string_view Platform);

        static const char* GetPlatformName(const std::string_view Platform);

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
    };
}
