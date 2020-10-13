#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetDownloadInfo {
		struct Metadata {
			// Not too sure what this is, I've only seen Android have this metadata
			std::optional<std::string> AndroidSigningFingerprintSHA1;

			std::optional<std::string> AndroidPackageVersionCode;

			std::optional<std::string> AndroidSigningFingerprintSHA256;

			std::optional<std::string> AndroidPackageName;

			PARSE_DEFINE(Metadata)
				PARSE_ITEM_OPT("androidSigningFingerprintSHA1", AndroidSigningFingerprintSHA1)
				PARSE_ITEM_OPT("androidPackageVersionCode", AndroidPackageVersionCode)
				PARSE_ITEM_OPT("androidSigningFingerprintSHA256", AndroidSigningFingerprintSHA256)
				PARSE_ITEM_OPT("androidPackageName", AndroidPackageName)
			PARSE_END
		};

		struct QueryParameter {
			std::string Name;

			std::string Value;

			PARSE_DEFINE(QueryParameter)
				PARSE_ITEM("name", Name)
				PARSE_ITEM("value", Value)
			PARSE_END
		};

		struct Manifest {
			std::string URI;

			std::optional<std::vector<QueryParameter>> QueryParams;

			PARSE_DEFINE(Manifest)
				PARSE_ITEM("uri", URI)
				PARSE_ITEM_OPT("queryParams", QueryParams)
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
			std::optional<Metadata> Metadata;

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
		};

		// A list of all assets available to the user
		std::vector<Element> Elements;

		PARSE_DEFINE(GetDownloadInfo)
			PARSE_ITEM("elements", Elements)
		PARSE_END
	};
}