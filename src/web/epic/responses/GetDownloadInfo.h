#pragma once

#include "../../JsonParsing.h"
#include "../../../utils/Random.h"

#include <filesystem>

namespace EGL3::Web::Epic::Responses {
    struct GetDownloadInfo {
        struct QueryParameter {
            std::string Name;

            std::string Value;

            PARSE_DEFINE(QueryParameter)
                PARSE_ITEM("name", Name)
                PARSE_ITEM("value", Value)
            PARSE_END
        };

        struct Manifest {
            std::string Uri;

            std::vector<QueryParameter> QueryParams;

            std::vector<QueryParameter> Headers;

            std::string GetCloudDir() const {
                return std::filesystem::path(Uri).parent_path().string();
            }

            PARSE_DEFINE(Manifest)
                PARSE_ITEM("uri", Uri)
                PARSE_ITEM_OPT("queryParams", QueryParams)
                PARSE_ITEM_OPT("headers", Headers)
            PARSE_END
        };

        struct Element {
            // Name of the app
            std::string AppName;

            // Label of the app
            std::string LabelName;

            // Version of the app
            std::string BuildVersion;

            // SHA1 of manifest file
            std::string Hash;

            // Any metadata for the element (optional)
            // Here are some possible keys:
            // androidSigningFingerprintSHA1
            // androidPackageVersionCode
            // androidSigningFingerprintSHA256
            // androidPackageName
            // status
            std::unordered_map<std::string, std::string> Metadata;

            // Manifest URLs
            std::vector<Manifest> Manifests;

            PARSE_DEFINE(Element)
                PARSE_ITEM("appName", AppName)
                PARSE_ITEM("labelName", LabelName)
                PARSE_ITEM("buildVersion", BuildVersion)
                PARSE_ITEM("hash", Hash)
                PARSE_ITEM_OPT("metadata", Metadata)
                PARSE_ITEM("manifests", Manifests)
            PARSE_END

            const Manifest& PickManifest() const {
                return *Utils::RandomChoice(Manifests.begin(), Manifests.end());
            }
        };

        // A list of all assets available to the user
        std::vector<Element> Elements;

        PARSE_DEFINE(GetDownloadInfo)
            PARSE_ITEM("elements", Elements)
        PARSE_END

        const Element* GetElement(const std::string& AppName) const {
            auto Itr = std::find_if(Elements.begin(), Elements.end(), [&AppName](const Element& Element) {
                return Element.AppName == AppName;
            });

            if (Itr != Elements.end()) {
                return &*Itr;
            }
            return nullptr;
        }
    };
}