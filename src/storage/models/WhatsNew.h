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

        static constexpr const char* SourceToString(ItemSource Source);

        static const char* SubGameToString(const std::string& SubGame);

        static const char* PlatformToString(const std::string& Platform);
    };
}