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

        WhatsNew(decltype(Item) && Item, decltype(Date) Date, ItemSource Source);

        static constexpr const char* WhatsNew::SourceToString(ItemSource Source) {
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

        static const char* WhatsNew::SubGameToString(const std::string& SubGame) {
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

        static const char* WhatsNew::PlatformToString(const std::string& Platform) {
            switch (Utils::Crc32(Platform))
            {
            case Utils::Crc32("mac"):
                return "Mac";
            case Utils::Crc32("windows"):
                return "Windows";
            case Utils::Crc32("XBoxOne"):
                return "Xbox One";
            case Utils::Crc32("PS4"):
                return "PS4";
            case Utils::Crc32("android"):
                return "Android";
            case Utils::Crc32("androidGP"):
                return "Android Google Play";
            case Utils::Crc32("ios"):
                return "iOS";
            case Utils::Crc32("switch"):
                return "Switch";
            default:
                return Platform.c_str();
            }
        }
    };
}