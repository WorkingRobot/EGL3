#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetStatuspageSummary {
		struct PageInfo {
			// Id of the page
			std::string Id;

			// Name of the page
			std::string Name;

			// URL to view the page online
			std::string Url;

			// Timezone of the status page (?)
			std::string Timezone;

			// Last time the page was updated
			TimePoint UpdatedAt;

			PARSE_DEFINE(PageInfo)
				PARSE_ITEM("id", Id)
				PARSE_ITEM("name", Name)
				PARSE_ITEM("url", Url)
				PARSE_ITEM("time_zone", Timezone)
				PARSE_ITEM("updated_at", UpdatedAt)
			PARSE_END
		};

		struct Component {
			// Id of the component
			std::string Id;

			// Display name of the component
			std::string Name;

			// Status of the component: "operational", "degraded_performance", "partial_outage", "major_outage"
			std::string Status;

			// When the component was created
			TimePoint CreatedAt;

			// When the component was last updated
			TimePoint UpdatedAt;

			// Position of the component in its parent
			int Position;

			// description is null everywhere

			// Always false?
			bool Showcase;

			// start_date is null or "yyyy-mm-dd", not really necessary

			// null if it's a parent group, otherwise, it's the id to the parent
			// std::string GroupId;

			// Always the id of the page in PageInfo
			// std::string PageId;

			// If true, it contains the components array
			bool Group;

			// Self explanatory
			bool OnlyShowIfDegraded;

			// Child components of the group
			std::optional<std::vector<std::string>> Components;

			PARSE_DEFINE(Component)
				PARSE_ITEM("id", Id)
				PARSE_ITEM("name", Name)
				PARSE_ITEM("status", Status)
				PARSE_ITEM("created_at", CreatedAt)
				PARSE_ITEM("updated_at", UpdatedAt)
				PARSE_ITEM("position", Position)
				PARSE_ITEM("showcase", Showcase)
				PARSE_ITEM("group", Group)
				PARSE_ITEM("only_show_if_degraded", OnlyShowIfDegraded)
				PARSE_ITEM_OPT("components", Components)
			PARSE_END
		};

		struct StatusInfo {
			// Indicator: "none", "minor", "major", "critical"
			std::string Indicator;

			// Human readable version of indicator
			std::string Description;

			PARSE_DEFINE(StatusInfo)
				PARSE_ITEM("indicator", Indicator)
				PARSE_ITEM("description", Description)
			PARSE_END
		};

		// Metadata about the page
		PageInfo Page;

		// Components for the page
		std::vector<Component> Components;

		// Overall status info for the page
		StatusInfo Status;

		PARSE_DEFINE(GetStatuspageSummary)
			PARSE_ITEM("page", Page)
			PARSE_ITEM("components", Components)
			// I don't care about incidents or scheduled_matinences as of right now
			PARSE_ITEM("status", Status)
		PARSE_END
	};
}
