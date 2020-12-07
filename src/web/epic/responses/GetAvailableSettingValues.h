#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetAvailableSettingValues {
		// List of all available setting values
		std::vector<std::string> Values;

		PARSE_DEFINE(GetAvailableSettingValues)
			PARSE_ITEM_ROOT(Values)
		PARSE_END
	};
}
