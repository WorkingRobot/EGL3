#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
	struct GetExchangeCode {
		int ExpiresInSeconds;

		std::string Code;

		std::string CreatingClientId;

		PARSE_DEFINE(GetExchangeCode)
			PARSE_ITEM("expiresInSeconds", ExpiresInSeconds)
			PARSE_ITEM("code", Code)
			PARSE_ITEM("creatingClientId", CreatingClientId)
		PARSE_END
	};
}
