#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
	struct QueryProfile {
		struct Item {
			std::string TemplateId;

			JsonObject Attributes;

			int Quantity;

			PARSE_DEFINE(Item)
				PARSE_ITEM("templateId", TemplateId)
				PARSE_ITEM("attributes", Attributes)
				PARSE_ITEM("quantity", Quantity)
			PARSE_END
		};

		struct Stats {
			JsonObject Attributes;

			PARSE_DEFINE(Stats)
				PARSE_ITEM("attributes", Attributes)
			PARSE_END
		};

		struct Profile {
			std::string Id;

			TimePoint Created;

			TimePoint Updated;

			int Rvn;

			int WipeNumber;

			std::string AccountId;

			std::string ProfileId;

			std::string Version;

			std::unordered_map<std::string, Item> Items;

			Stats Stats;

			int CommandRevision;

			PARSE_DEFINE(Profile)
				PARSE_ITEM("_id", Id)
				PARSE_ITEM("created", Created)
				PARSE_ITEM("updated", Updated)
				PARSE_ITEM("rvn", Rvn)
				PARSE_ITEM("wipeNumber", WipeNumber)
				PARSE_ITEM("accountId", AccountId)
				PARSE_ITEM("profileId", ProfileId)
				PARSE_ITEM("version", Version)
				PARSE_ITEM("items", Items)
				PARSE_ITEM("stats", Stats)
				PARSE_ITEM("commandRevision", CommandRevision)
			PARSE_END
		};

		struct ProfileChange {
			std::string ChangeType;

			Profile Profile;

			PARSE_DEFINE(ProfileChange)
				PARSE_ITEM("changeType", ChangeType)
				PARSE_ITEM("profile", Profile)
			PARSE_END
		};

		int ProfileRevision;

		std::string ProfileId;

		int ProfileChangesBaseRevision;

		std::vector<ProfileChange> ProfileChanges;

		int ProfileCommandRevision;

		TimePoint ServerTime;

		// A multiUpdate array can also exist here which is basically QueryProfile but without ServerTime or ResponseVersion (seen in campaign)

		int ResponseVersion;

		PARSE_DEFINE(QueryProfile)
			PARSE_ITEM("profileRevision", ProfileRevision)
			PARSE_ITEM("profileId", ProfileId)
			PARSE_ITEM("profileChangesBaseRevision", ProfileChangesBaseRevision)
			PARSE_ITEM("profileChanges", ProfileChanges)
			PARSE_ITEM("profileCommandRevision", ProfileCommandRevision)
			PARSE_ITEM("serverTime", ServerTime)
			PARSE_ITEM("responseVersion", ResponseVersion)
		PARSE_END
	};
}
