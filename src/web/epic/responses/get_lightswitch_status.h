#pragma once

struct RespGetLightswitchStatus {
	struct LauncherInfoDTO {
		// App name of service (used in request)
		std::string AppName;

		// Catalog item id of service
		std::string CatalogItemId;

		// Namespace of service
		std::string Namespace;

		PARSE_DEFINE(LauncherInfoDTO)
			PARSE_ITEM("appName", AppName)
			PARSE_ITEM("catalogItemId", CatalogItemId)
			PARSE_ITEM("namespace", Namespace)
		PARSE_END
	};

	struct ServiceStatus {
		// Given from request (more or less)
		std::string ServiceInstanceId;

		// Status of service (UP or DOWN)
		std::string Status;

		// Message to give (Not really shown to users, I doubt they'd want to see "Yo Fortnite's up")
		std::string Message;

		// Never seen this not be null
		std::string MantinenceUri;

		// Never seen a real use for this since I've only seen it used on Fortnite, and their one id here leads to
		// https://raw.githubusercontent.com/EpicData-info/items-tracker/master/database/items/a7f138b2e51945ffbfdacc1af0541053.json
		std::vector<std::string> OverrideCatalogIds;

		// Only isn't empty when logged in (can show "PLAY" or "DOWNLOAD", haven't seen others)
		std::vector<std::string> AllowedActions;

		// If you're banned WeirdChamp
		bool Banned;

		// Has stuff for launcher, no idea tbh (not sure if this is optional, but I don't want to risk it)
		std::optional<LauncherInfoDTO> LauncherInfoDTO;

		PARSE_DEFINE(ServiceStatus)
			PARSE_ITEM("serviceInstanceId", ServiceInstanceId)
			PARSE_ITEM("status", Status)
			PARSE_ITEM("message", Message)
			PARSE_ITEM("maintenanceUri", MantinenceUri)
			PARSE_ITEM("overrideCatalogIds", OverrideCatalogIds)
			PARSE_ITEM("allowedActions", AllowedActions)
			PARSE_ITEM("banned", Banned)
			PARSE_ITEM_OPT("launcherInfoDTO", LauncherInfoDTO)
		PARSE_END
	};

	// List of all services requested
	std::vector<ServiceStatus> Services;

	PARSE_DEFINE(RespGetLightswitchStatus)
		PARSE_ITEM_ROOT(Services)
	PARSE_END
};