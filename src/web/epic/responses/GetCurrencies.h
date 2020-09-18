#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetCurrencies {
		struct Currency {
			// Type of currency (only seen "REAL")
			std::string Type;

			// Code of the currency (e.g. "USD", "EUR", "GBP")
			std::string Code;

			// Symbol of the currency (e.g. "$")
			std::string Symbol;

			// Description of the currency (sometimes explains it, other times looks like a placeholder)
			std::string Description;

			// Number of decimals in the currency (e.g. .00 = 2 in USD for cents)
			int Decimals;

			// Unsure, only seen 0 here
			int TruncLength;

			// Not too sure what it's used for, but all strings are like "[0,number]"
			std::vector<std::string> PriceRanges;

			PARSE_DEFINE(Currency)
				PARSE_ITEM("type", Type)
				PARSE_ITEM("code", Code)
				PARSE_ITEM("symbol", Symbol)
				PARSE_ITEM("description", Description)
				PARSE_ITEM("decimals", Decimals)
				PARSE_ITEM("truncLength", TruncLength)
				PARSE_ITEM("priceRanges", PriceRanges)
			PARSE_END
		};

		struct PageInfo {
			// Number of total elements requested
			int Count;

			// Start index of elements requested
			int Start;

			// Total elements in the endpoint
			int Total;

			PARSE_DEFINE(PageInfo)
				PARSE_ITEM("count", Count)
				PARSE_ITEM("start", Start)
				PARSE_ITEM("total", Total)
			PARSE_END
		};

		// Listed currencies
		std::vector<Currency> Elements;

		// Page info depending on query params
		PageInfo Paging;

		PARSE_DEFINE(GetCurrencies)
			PARSE_ITEM("elements", Elements)
			PARSE_ITEM("paging", Paging)
		PARSE_END
	};
}
