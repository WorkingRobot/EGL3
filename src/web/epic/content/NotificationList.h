#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Content {
    struct NotificationList {
        struct Notification {
            std::string Id;

            std::string DismissId;

            std::string DisplayCondition;

            std::string ImagePath;

            std::string UriLink;
            
            bool IsAdvert;

            bool IsFreeGame;

            JsonLocalized Title;

            JsonLocalized Description;

            PARSE_DEFINE(Notification)
                PARSE_ITEM("NotificationId", Id)
                PARSE_ITEM("DisplayCondition", DisplayCondition)
                PARSE_ITEM("ImagePath", ImagePath)
                PARSE_ITEM("UriLink", UriLink)
                PARSE_ITEM_OPT("DismissId", DismissId)
                PARSE_ITEM_DEF("IsFreeGame", IsFreeGame, false)
                PARSE_ITEM_DEF("IsAdvert", IsAdvert, true)
                PARSE_ITEM_LOC("Title", Title)
                PARSE_ITEM_LOC("Description", Description)
                // Blacklists are ignored, there's no real reason to hide anything
            PARSE_END
        };

        std::vector<Notification> Notifications;

        PARSE_DEFINE(NotificationList)
            PARSE_ITEM("BuildNotifications", Notifications)
        PARSE_END
    };
}