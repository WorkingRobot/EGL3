#pragma once

struct RespGetAssets {
	struct Metadata {
		// Not too sure what this is, looks like a guid like the other values
		std::optional<std::string> InstallationPoolId;

		PARSE_DEFINE(Metadata)
			PARSE_ITEM_OPT("installationPoolId", InstallationPoolId)
		PARSE_END
	};

	struct Asset {
		// Name of the app
		std::string AppName;

		// Label of the app
		std::string LabelName;

		// Version of the app
		std::string BuildVersion;

		// Catalog id of the app
		std::string CatalogItemId;

		// Namespace of the app
		std::string Namespace;

		// Any metadata for the app (optional)
		std::optional<Metadata> Metadata;

		// Asset id?
		std::string AssetId;

		PARSE_DEFINE(Asset)
			PARSE_ITEM("appName", AppName)
			PARSE_ITEM("labelName", LabelName)
			PARSE_ITEM("buildVersion", BuildVersion)
			PARSE_ITEM("catalogItemId", CatalogItemId)
			PARSE_ITEM("namespace", Namespace)
			PARSE_ITEM_OPT("metadata", Metadata)
			PARSE_ITEM("assetId", AssetId)
		PARSE_END
	};

	// A list of all assets available to the user
	std::vector<Asset> Assets;

	PARSE_DEFINE(RespGetAssets)
		PARSE_ITEM_ROOT(Assets)
	PARSE_END
};