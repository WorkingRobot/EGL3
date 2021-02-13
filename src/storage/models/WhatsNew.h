#pragma once

#include "../../web/epic/responses/GetBlogPosts.h"
#include "../../web/epic/responses/GetPageInfo.h"

#include <chrono>
#include <variant>

namespace EGL3::Storage::Models {
    struct WhatsNew {
        enum class ItemSource : uint8_t {
            BLOG        = 1,
            BR          = 2,
            STW         = 4,
            CREATIVE    = 8,
            NOTICE      = 16
        };

        std::variant<
            Web::Epic::Responses::GetBlogPosts::BlogItem,
            Web::Epic::Responses::GetPageInfo::GenericMotd,
            Web::Epic::Responses::GetPageInfo::GenericPlatformMotd,
            Web::Epic::Responses::GetPageInfo::GenericNewsPost,
            Web::Epic::Responses::GetPageInfo::GenericPlatformPost,
            Web::Epic::Responses::GetPageInfo::GenericRegionPost
        > Item;

        std::chrono::system_clock::time_point Date;

        ItemSource Source;

        WhatsNew(decltype(Item)&& Item, decltype(Date) Date, ItemSource Source);

        static constexpr const char* SourceToString(ItemSource Source) {
            switch (Source)
            {
            case ItemSource::BLOG:
                return "Blog";
            case ItemSource::BR:
                return "Battle Royale";
            case ItemSource::STW:
                return "Save the World";
            case ItemSource::CREATIVE:
                return "Creative";
            case ItemSource::NOTICE:
                return "Notice";
            default:
                return "Unknown";
            }
        }

        static const char* SubGameToString(const std::string& SubGame) {
            switch (Utils::Crc32(SubGame))
            {
            case Utils::Crc32("br"):
                return "Battle Royale";
            case Utils::Crc32("stw"):
                return "Save the World";
            case Utils::Crc32("creative"):
                return "Creative";
            default:
                return SubGame.c_str();
            }
        }

        // Seems like it loosely follows https://github.com/EpicGames/UnrealEngine/blob/e744dd44214743240bf6a67c6800ac60b8eb49e3/Engine/Source/Programs/AutomationTool/AutomationUtils/MCPPublic.cs#L87
        static const char* PlatformToString(const std::string& Platform) {
            switch (Utils::Crc32(Platform))
            {
            case Utils::Crc32("windows"):
                return "Windows";
            case Utils::Crc32("mac"):
                return "Mac";
            case Utils::Crc32("ios"):
                return "iOS";
            case Utils::Crc32("android"):
                return "Android";
            case Utils::Crc32("androidGP"):
                return "Android Google Play";
            case Utils::Crc32("PS4"):
                return "PS4";
            case Utils::Crc32("PS5"):
                return "PS5";
            case Utils::Crc32("switch"):
                return "Switch";
            case Utils::Crc32("XSX"):
                return "Xbox Series X";
            case Utils::Crc32("XBoxOne"):
            case Utils::Crc32("XboxOneGDK"):
                return "Xbox One";
            default:
                return Platform.c_str();
            }
        }
    };
}