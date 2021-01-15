#pragma once

#include "GetDownloadInfo.h"

namespace EGL3::Web::Epic::Responses {
	struct GetLauncherDownloadInfo : public GetDownloadInfo {
		struct BuildStatus {
			DEFINE_JSON_ENUM(StatusEnum, DEPRECATED, NOT_DEPRECATED)

			// Not given by CheckLauncherVersion
			std::optional<std::string> App;

			StatusEnumJson Status;

			PARSE_DEFINE(BuildStatus)
				PARSE_ITEM_OPT("app", App)
				PARSE_ITEM("status", Status)
			PARSE_END
		};

		// Only provided if ClientVersion is given: A list of build statuses (only 1 item)
		std::vector<BuildStatus> Statuses;

		PARSE_DEFINE(GetLauncherDownloadInfo)
			PARSE_BASE(GetDownloadInfo)
			PARSE_ITEM_OPT("buildStatuses", Elements)
		PARSE_END
	};
}