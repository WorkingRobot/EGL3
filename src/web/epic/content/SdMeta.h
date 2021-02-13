#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Content {
    struct SdMeta {
        struct Build {
            std::string Item;

            std::string Asset;

            std::string Version;

            PARSE_DEFINE(Build)
                PARSE_ITEM("Item", Item)
                PARSE_ITEM("Asset", Asset)
                PARSE_ITEM("Version", Version)
            PARSE_END
        };

        struct Data {
            std::string Id;

            // If this id is installed (previous update), then this new option is selected
            std::string UpgradePathLogic;

            std::vector<std::string> InstallTags;

            bool IsRequired;

            JsonLocalized Title;

            JsonLocalized Description;

            std::string ConfigHandler; // only DefaultLanguage

            std::string ConfigValue; // for the ConfigHandler

            // If IsRequired, these 2 values (Default...) are ignored
            std::string DefaultSelectedExpression;

            // This value is ignored if DefaultSelectedExpression isn't empty
            bool IsDefaultSelected;

            bool IsDefaultExpanded;

            std::vector<Data> Children;

            bool Invisible;

            // To which id these install tags sizes would count to
            std::string InvisibleSizeCountTowards;

            // The size is only added if this evaluates to true (or is selected and empty)
            std::string InvisibleSizeCountCondition;

            // This option is only selected if this evaluates to true
            std::string InvisibleSelectedExpression;

            PARSE_DEFINE(Data)
                PARSE_ITEM("UniqueId", Id)
                PARSE_ITEM_OPT("UpgradePathLogic", UpgradePathLogic)
                PARSE_ITEM_OPT("InstallTags", InstallTags)
                PARSE_ITEM_DEF("IsRequired", IsRequired, false)
                PARSE_ITEM_DEF("Invisible", Invisible, false)
                if (Obj.Invisible) {
                    PARSE_ITEM("InvisibleSizeCountTowards", InvisibleSizeCountTowards)
                    PARSE_ITEM_OPT("InvisibleSizeCountCondition", InvisibleSizeCountCondition)
                    PARSE_ITEM("InvisibleSelectedExpression", InvisibleSelectedExpression)
                }
                else {
                    PARSE_ITEM_LOC("Title", Title)
                    PARSE_ITEM_LOC("Description", Description)
                    PARSE_ITEM_OPT("ConfigHandler", ConfigHandler)
                    PARSE_ITEM_OPT("ConfigValue", ConfigValue)
                    if (!Obj.IsRequired) {
                        PARSE_ITEM_OPT("DefaultSelectedExpression", DefaultSelectedExpression)
                        PARSE_ITEM_DEF("IsDefaultSelected", IsDefaultSelected, false)
                    }
                    PARSE_ITEM_DEF("IsDefaultExpanded", IsDefaultExpanded, false)
                    PARSE_ITEM_OPT("Children", Children)
                }
            PARSE_END
        };

        std::vector<Build> Builds;

        std::vector<Data> Datas;

        PARSE_DEFINE(SdMeta)
            PARSE_ITEM("Builds", Builds)
            PARSE_ITEM("Data", Datas)
        PARSE_END
    };
}