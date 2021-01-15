#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
	struct GetAccount {
		// Account id
		std::string Id;

		// Display name
		std::string DisplayName;

		// First name
		std::string Name;

		// Last name
		std::string LastName;

		// Email
		std::string Email;

		// Whether the email has been verfied
		bool EmailVerified;

		// Failed login attempts before the successful one
		int FailedLoginAttempts;

		// Last login time
		TimePoint LastLogin;

		// Number of times the display name has changed (presumably to detect that the cached display name is different?)
		int DisplayNameChangeCount;

		// Last time the display name changed
		TimePoint LastDisplayNameChange;

		// Whether the user can change the display name
		bool CanUpdateDisplayName;

		// Age group, I've only observed "UNKNOWN"
		std::string AgeGroup;

		// Whether we know the account holder is a minor
		bool MinorVerified;

		// Whether we can assume they are a minor, but we can't verify it (I think having a switch fn account enables this)
		bool MinorExpected;

		// Minor status, I've only seen "UNKNOWN"
		std::string MinorStatus;

		// Not sure what standard this conforms to, I've only had "US"
		std::string Country;

		// Preferred language (i've only had "en")
		std::string PreferredLanguage;

		// Phone number
		std::string PhoneNumber;

		// Whether MFA is enabled (TFA = two factor authentication, not sure why it's not called MFA)
		bool TFAEnabled;

		// This isn't a "real full fledged" account. It's tied directly to a console account or something else
		bool Headless;

		PARSE_DEFINE(GetAccount)
			PARSE_ITEM("id", Id)
			PARSE_ITEM("displayName", DisplayName)
			PARSE_ITEM("name", Name)
			PARSE_ITEM("email", Email)
			PARSE_ITEM("failedLoginAttempts", FailedLoginAttempts)
			PARSE_ITEM("lastLogin", LastLogin)
			PARSE_ITEM("numberOfDisplayNameChanges", DisplayNameChangeCount)
			PARSE_ITEM("ageGroup", AgeGroup)
			PARSE_ITEM("headless", Headless)
			PARSE_ITEM("country", Country)
			PARSE_ITEM("lastName", LastName)
			PARSE_ITEM("phoneNumber", PhoneNumber)
			PARSE_ITEM("preferredLanguage", PreferredLanguage)
			PARSE_ITEM("lastDisplayNameChange", LastDisplayNameChange)
			PARSE_ITEM("canUpdateDisplayName", CanUpdateDisplayName)
			PARSE_ITEM("tfaEnabled", TFAEnabled)
			PARSE_ITEM("emailVerified", EmailVerified)
			PARSE_ITEM("minorVerified", MinorVerified)
			PARSE_ITEM("minorExpected", MinorExpected)
			PARSE_ITEM("minorStatus", MinorStatus)
		PARSE_END
	};
}