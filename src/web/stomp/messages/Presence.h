#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Stomp::Messages {
    struct Presence {
        struct Connection {
            std::string Id;
            std::unordered_map<std::string, std::string> Properties;

            PARSE_DEFINE(Connection)
                PARSE_ITEM("id", Id)
                PARSE_ITEM("props", Properties)
            PARSE_END
        };

        struct ActivityItem {
            std::string Value;

            PARSE_DEFINE(ActivityItem)
                PARSE_ITEM_OPT("value", Value)
            PARSE_END
        };

        struct NamespacePresence {
            std::optional<std::string> ProductId;
            std::optional<std::string> AppId;
            std::string Status;
            ActivityItem Activity;
            std::string Namespace;
            std::unordered_map<std::string, std::string> Properties;
            std::vector<Connection> Connections;

            std::string GetProductId() const;
            std::string GetPlatformId() const;

            std::weak_ordering operator<=>(const NamespacePresence&) const;

            static std::string GetProductImageUrl(const std::string& ProductId);
            static const std::string& GetProductName(const std::string& ProductId);

            static std::string GetPlatformImageUrl(const std::string& PlatformId);
            static const char* GetPlatformName(const std::string& PlatformId);

            PARSE_DEFINE(NamespacePresence)
                PARSE_ITEM_OPT("productId", ProductId)
                PARSE_ITEM_OPT("appId", AppId)
                PARSE_ITEM("status", Status)
                PARSE_ITEM_OPT("activity", Activity)
                PARSE_ITEM("ns", Namespace)
                PARSE_ITEM_OPT("props", Properties)
                PARSE_ITEM("conns", Connections)
            PARSE_END
        };

        std::string AccountId;
        std::string Status;
        std::vector<NamespacePresence> NamespacePresences;

        PARSE_DEFINE(Presence)
            PARSE_ITEM("accountId", AccountId)
            PARSE_ITEM("status", Status)
            PARSE_ITEM("perNs", NamespacePresences)
        PARSE_END
    };
}
