#pragma once

#include "../../web/epic/responses/Responses.h"

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

        WhatsNew(const decltype(Item) && Item, const decltype(Date) && Date, const decltype(Source) && Source) :
            Item(Item),
            Date(Date),
            Source(Source)
        {}

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
            }
        }

        static const char* SubGameToString(const std::string& SubGame) {
            if (SubGame == "br") {
                return "Battle Royale";
            }
            if (SubGame == "stw") {
                return "Save the World";
            }
            if (SubGame == "creative") {
                return "Creative";
            }

            return SubGame.c_str();
        }

        static const char* PlatformToString(const std::string& Platform) {
            if (Platform == "mac") {
                return "Mac";
            }
            if (Platform == "windows") {
                return "Windows";
            }
            if (Platform == "XBoxOne") {
                return "Xbox One";
            }
            if (Platform == "PS4") {
                return "PS4";
            }
            if (Platform == "android") {
                return "Android";
            }
            if (Platform == "androidGP") {
                return "Android Google Play";
            }
            if (Platform == "ios") {
                return "iOS";
            }
            if (Platform == "switch") {
                return "Switch";
            }

            return Platform.c_str();
        }
    };
}