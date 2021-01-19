#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetEntitlements {
        struct Entitlement {
            // Id of the entitlement
            std::string Id;

            // Name of the entitlement
            std::string EntitlementName;

            // Namespace of the entitlement
            std::string Namespace;

            // Catalog id of the entitlement
            std::string CatalogItemId;

            // Account id that this entitlement is given to
            std::string AccountId;

            // Identity id that this entitlement is given to (seems equal to the account id)
            std::string IdentityId;

            // Type of entitlement (e.g. "EXECUTABLE" "ENTITLEMENT" "AUDIENCE")
            std::string EntitlementType;

            // When the entitlement was granted
            TimePoint GrantDate;

            // When the entitlement started being given out (I think)
            std::optional<TimePoint> StartDate;

            // Whether the entitlement is cosumable (?)
            bool Consumable;

            // Status of the entitlement (I've only seen "ACTIVE")
            std::string Status;

            // Whether the entitlement is active
            bool Active;

            // Number of times the entitlement has been used (?)
            int UseCount;

            // Not sure what this means
            std::optional<int> OriginalUseCount;

            // Not sure what this means (I've seen "EPIC")
            std::optional<std::string> PlatformType;

            // When the entitlement was created
            TimePoint Created;
        
            // Last time the entitlement was updated
            TimePoint Updated;

            // Whether the entitlement was given to a group (?)
            bool GroupEntitlement;

            // Country? Unsure what this would do here
            std::optional<std::string> Country;

            // Not sure, I've only seen "anonymous"
            std::optional<std::string> Operator;

            PARSE_DEFINE(Entitlement)
                PARSE_ITEM("id", Id)
                PARSE_ITEM("entitlementName", EntitlementName)
                PARSE_ITEM("namespace", Namespace)
                PARSE_ITEM("catalogItemId", CatalogItemId)
                PARSE_ITEM("accountId", AccountId)
                PARSE_ITEM("identityId", IdentityId)
                PARSE_ITEM("entitlementType", EntitlementType)
                PARSE_ITEM("grantDate", GrantDate)
                PARSE_ITEM_OPT("startDate", StartDate)
                PARSE_ITEM("consumable", Consumable)
                PARSE_ITEM("status", Status)
                PARSE_ITEM("active", Active)
                PARSE_ITEM("useCount", UseCount)
                PARSE_ITEM_OPT("originalUseCount", OriginalUseCount)
                PARSE_ITEM_OPT("platformType", PlatformType)
                PARSE_ITEM("created", Created)
                PARSE_ITEM("updated", Updated)
                PARSE_ITEM("groupEntitlement", GroupEntitlement)
                PARSE_ITEM_OPT("country", Country)
                PARSE_ITEM_OPT("operator", Operator)
            PARSE_END
        };

        // A list of all entitlements recieved
        std::vector<Entitlement> Entitlements;

        PARSE_DEFINE(GetEntitlements)
            PARSE_ITEM_ROOT(Entitlements)
        PARSE_END
    };
}
