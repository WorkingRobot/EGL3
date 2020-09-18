#pragma once

struct RespGetBlogPosts {
	// This isn't everything since I'm not going to use all of it unless I add a browser element or html parsing
	// for the fields that are HTML
	struct BlogItem {
		std::string Image;
		std::string TrendingImage;
		std::string Author;
		std::string Title;
		TimePoint Date;
		std::optional<std::string> ExternalLink;
		std::string Slug;
		std::string ShareDescription;
		// std::optional<std::string> Content;

		PARSE_DEFINE(BlogItem)
			PARSE_ITEM("image", Image)
			PARSE_ITEM("trendingImage", TrendingImage)
			PARSE_ITEM("author", Author)
			PARSE_ITEM("title", Title)
			PARSE_ITEM("date", Date)
			PARSE_ITEM_OPT("externalLink", ExternalLink)
			PARSE_ITEM("slug", Slug)
			PARSE_ITEM("shareDescription", ShareDescription)
		PARSE_END
	};

	std::vector<BlogItem> BlogList;

	int BlogTotal;

	int PostCount;

	int IncrementCount;

	int ArticlesToLoad;

	PARSE_DEFINE(RespGetBlogPosts)
		PARSE_ITEM("blogList", BlogList)
		PARSE_ITEM("blogTotal", BlogTotal)
		PARSE_ITEM("postCount", PostCount)
		PARSE_ITEM("incrementCount", IncrementCount)
		PARSE_ITEM("articlesToLoad", ArticlesToLoad)
	PARSE_END
};