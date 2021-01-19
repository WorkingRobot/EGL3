#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetCatalogItems {
        struct KeyImage {
            // Type of key image
            std::string Type;

            // URL of key image
            std::string Url;

            // MD5 of the image at the url
            std::string Md5;

            // Width of the image
            int Width;

            // Height of the image
            int Height;

            // Size of the image (file size)
            int Size;

            // Date of when the image was uploaded
            TimePoint UploadedDate;

            PARSE_DEFINE(KeyImage)
                PARSE_ITEM("type", Type)
                PARSE_ITEM("url", Url)
                PARSE_ITEM("md5", Md5)
                PARSE_ITEM("width", Width)
                PARSE_ITEM("height", Height)
                PARSE_ITEM("size", Size)
                PARSE_ITEM("uploadedDate", UploadedDate)
            PARSE_END
        };

        struct Category {
            // Path of the category? This is the only field so idfk
            std::string Path;

            PARSE_DEFINE(Category)
                PARSE_ITEM("path", Path)
            PARSE_END
        };

        struct CustomAttribute {
            // Type of the value (I've only seen STRING, so I think value can stay as just a string
            std::string Type;

            // Value of the attribute
            std::string Value;

            PARSE_DEFINE(CustomAttribute)
                PARSE_ITEM("type", Type)
                PARSE_ITEM("value", Value)
            PARSE_END
        };

        struct ReleaseInfo {
            // Id of the release
            std::string Id;

            // App id of the release
            std::string AppId;

            // compatibleApps is a list here, but it's empty and optional (I saw see it for fortnite)

            // Platforms the release is available on
            std::vector<std::string> Platform;

            // Date when the release was added
            std::optional<TimePoint> DateAdded;

            // These 2 fields were seen in poodle/twinmotion, but both were empty
            std::optional<std::string> ReleaseNote;

            std::optional<std::string> VersionTitle;

            PARSE_DEFINE(ReleaseInfo)
                PARSE_ITEM("id", Id)
                PARSE_ITEM("appId", AppId)
                PARSE_ITEM("platform", Platform)
                PARSE_ITEM_OPT("dateAdded", DateAdded)
                PARSE_ITEM_OPT("releaseNote", ReleaseNote)
                PARSE_ITEM_OPT("versionTitle", VersionTitle)
            PARSE_END
        };

        struct DLCItem {
            // Id of the DLC
            std::string Id;

            // Namespace of the DLC
            std::string Namespace;

            // Whether the DLC can't appear in search
            bool Unsearchable;

            PARSE_DEFINE(DLCItem)
                PARSE_ITEM("id", Id)
                PARSE_ITEM("namespace", Namespace)
                PARSE_ITEM("unsearchable", Unsearchable)
            PARSE_END
        };

        struct CatalogItem {
            // Id of the catalog item (redundant in both the url request and the key in the map)
            std::string Id;

            // Title of the item
            std::string Title;

            // Description of the item
            std::string Description;

            // Description, but lengthy
            std::optional<std::string> LongDescription;

            // Technical requirements of the app/item
            std::optional<std::string> TechnicalDetails;

            // Key images of the item
            std::vector<KeyImage> KeyImages;

            // Categories of the item
            std::vector<Category> Categories;

            // Namespace of the catalog item (redundant since it's in the url request)
            std::string Namespace;

            // Status of the catalog (I've seen "ACTIVE", not sure about others)
            std::string Status;

            // Date created
            TimePoint CreationDate;

            // Last time the item was modified
            TimePoint LastModifiedDate;

            // Custom attributes, e.g. can run offline, presence id, cloud save folders, UAC, etc.
            std::optional<std::unordered_map<std::string, CustomAttribute>> CustomAttributes;

            // Entitlement name of the item (same as id usually)
            std::string EntitlementName;

            // Entitlement type of the item (e.g. "EXECUTABLE")
            std::string EntitlementType;

            // Item type? not sure what this is, I've seen "DURABLE"
            std::string ItemType;

            // Release info of the item
            std::vector<ReleaseInfo> ReleaseInfo;

            // Developer of the item
            std::string Developer;

            // Id of the developer
            std::string DeveloperId;

            // EULA ids for the item
            std::vector<std::string> EulaIds;

            // Whether it's end of support for the item
            bool EndOfSupport;

            // DLCs for the item
            std::optional<std::vector<DLCItem>> DlcItemList;

            // Self refundable, not too sure what this actually means
            std::optional<bool> SelfRefundable;
        
            // ageGatings dictionary/map/object here, but it's empty (only in fortnite)

            // Saw this in twinmotion/poodle, it's set to false. i wonder what qualifies as "secure"
            std::optional<bool> RequiresSecureAccount;

            // Unsearchable (like twinmotion education edition)
            bool Unsearchable;

            PARSE_DEFINE(CatalogItem)
                PARSE_ITEM("id", Id)
                PARSE_ITEM("title", Title)
                PARSE_ITEM("description", Description)
                PARSE_ITEM_OPT("longDescription", LongDescription)
                PARSE_ITEM_OPT("technicalDetails", TechnicalDetails)
                PARSE_ITEM("keyImages", KeyImages)
                PARSE_ITEM("categories", Categories)
                PARSE_ITEM("namespace", Namespace)
                PARSE_ITEM("status", Status)
                PARSE_ITEM("creationDate", CreationDate)
                PARSE_ITEM("lastModifiedDate", LastModifiedDate)
                PARSE_ITEM_OPT("customAttributes", CustomAttributes)
                PARSE_ITEM("entitlementName", EntitlementName)
                PARSE_ITEM("entitlementType", EntitlementType)
                PARSE_ITEM("itemType", ItemType)
                PARSE_ITEM("releaseInfo", ReleaseInfo)
                PARSE_ITEM("developer", Developer)
                PARSE_ITEM("developerId", DeveloperId)
                PARSE_ITEM("eulaIds", EulaIds)
                PARSE_ITEM("endOfSupport", EndOfSupport)
                PARSE_ITEM_OPT("dlcItemList", DlcItemList)
                PARSE_ITEM_OPT("selfRefundable", SelfRefundable)
                PARSE_ITEM_OPT("requiresSecureAccount", RequiresSecureAccount)
                PARSE_ITEM("unsearchable", Unsearchable)
            PARSE_END
        };

        // A list of all queried items
        std::unordered_map<std::string, CatalogItem> Items;

        PARSE_DEFINE(GetCatalogItems)
            PARSE_ITEM_ROOT(Items)
        PARSE_END
    };
}
