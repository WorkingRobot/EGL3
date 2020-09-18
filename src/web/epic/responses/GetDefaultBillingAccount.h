#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetDefaultBillingAccount {
		// Display name of the billing account (e.g "PayPal - a***z@example.com")
		std::string BillingAccountName;

		// Country of the billing account
		std::string Country;

		// Not too sure what this is, I've only seen "BILLING"
		std::string CountrySource;

		// Currency used for the billing acocunt (e.g. "USD")
		std::string Currency;

		PARSE_DEFINE(GetDefaultBillingAccount)
			PARSE_ITEM("billingAccountName", BillingAccountName)
			PARSE_ITEM("country", Country)
			PARSE_ITEM("countrySource", CountrySource)
			PARSE_ITEM("currency", Currency)
		PARSE_END
	};
}
