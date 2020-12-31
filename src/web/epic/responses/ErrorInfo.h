#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
	struct ErrorInfo {
		std::string ErrorCode;
		std::string ErrorMessage;
		std::vector<std::string> MessageVars;
		int NumericErrorCode;
		std::string OriginatingService;
		std::string Intent;

		PARSE_DEFINE(ErrorInfo)
			PARSE_ITEM("errorCode", ErrorCode)
			PARSE_ITEM("errorMessage", ErrorMessage)
			PARSE_ITEM_OPT("messageVars", MessageVars)
			PARSE_ITEM("numericErrorCode", NumericErrorCode)
			PARSE_ITEM("originatingService", OriginatingService)
			PARSE_ITEM("intent", Intent)
		PARSE_END
	};
}
