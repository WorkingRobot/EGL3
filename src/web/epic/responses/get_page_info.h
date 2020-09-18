#pragma once

struct RespGetPageInfo {
	struct GenericNewsPost {
		std::optional<std::string> PlaylistId;
		std::optional<std::string> Image;
		bool Hidden;
		std::optional<std::string> MessageType;
		std::optional<std::string> SubGame;
		std::optional<std::string> AdSpace;
		std::string Title;
		std::string Body;
		bool Spotlight;

		PARSE_DEFINE(GenericNewsPost)
			PARSE_ITEM_OPT("playlistId", PlaylistId)
			PARSE_ITEM_OPT("image", Image)
			PARSE_ITEM("hidden", Hidden)
			PARSE_ITEM_OPT("messagetype", MessageType)
			PARSE_ITEM_OPT("subgame", SubGame)
			PARSE_ITEM_OPT("adspace", AdSpace)
			PARSE_ITEM("title", Title)
			PARSE_ITEM("body", Body)
			PARSE_ITEM("spotlight", Spotlight)
		PARSE_END
	};

	struct GenericPlatformPost {
		bool Hidden;
		GenericNewsPost Message;
		std::string Platform;

		PARSE_DEFINE(GenericPlatformPost)
			PARSE_ITEM("hidden", Hidden)
			PARSE_ITEM("message", Message)
			PARSE_ITEM("platform", Platform)
		PARSE_END
	};

	struct GenericRegionPost {
		std::string Region;
		GenericNewsPost Message;

		PARSE_DEFINE(GenericRegionPost)
			PARSE_ITEM("region", Region)
			PARSE_ITEM("message", Message)
		PARSE_END
	};

	struct GenericMotd {
		std::string EntryType;
		std::string Image;
		std::string TileImage;
		std::optional<std::string> NavigateToTabValue;
		bool VideoMute;
		bool Hidden;
		std::string TabTitleOverride;
		std::string Title;
		std::string Body;
		bool VideoLoop;
		bool VideoStreamingEnabled;
		int SortingPriority;
		std::optional<std::string> WebsiteUrl;
		std::optional<std::string> ButtonTextOverride;
		std::optional<std::string> VideoUID;
		std::string Id;
		bool VideoAutoplay;
		bool VideoFullscreen;
		bool Spotlight;

		PARSE_DEFINE(GenericMotd)
			PARSE_ITEM("entryType", EntryType)
			PARSE_ITEM("image", Image)
			PARSE_ITEM("tileImage", TileImage)
			PARSE_ITEM_OPT("navigateToTabValue", NavigateToTabValue)
			PARSE_ITEM("videoMute", VideoMute)
			PARSE_ITEM("hidden", Hidden)
			PARSE_ITEM("tabTitleOverride", TabTitleOverride)
			PARSE_ITEM("title", Title)
			PARSE_ITEM("body", Body)
			PARSE_ITEM("videoLoop", VideoLoop)
			PARSE_ITEM("videoStreamingEnabled", VideoStreamingEnabled)
			PARSE_ITEM("sortingPriority", SortingPriority)
			PARSE_ITEM_OPT("websiteURL", WebsiteUrl)
			PARSE_ITEM_OPT("buttonTextOverride", ButtonTextOverride)
			PARSE_ITEM_OPT("videoUID", VideoUID)
			PARSE_ITEM("id", Id)
			PARSE_ITEM("videoAutoplay", VideoAutoplay)
			PARSE_ITEM("videoFullscreen", VideoFullscreen)
			PARSE_ITEM("spotlight", Spotlight)
		PARSE_END
	};

	struct GenericPlatformMotd {
		bool Hidden;
		GenericMotd Message;
		std::string Platform;

		PARSE_DEFINE(GenericPlatformMotd)
			PARSE_ITEM("hidden", Hidden)
			PARSE_ITEM("message", Message)
			PARSE_ITEM("platform", Platform)
		PARSE_END
	};

	struct GenericNewsContainer {
		std::vector<GenericNewsPost> Messages;
		std::optional<std::vector<GenericPlatformPost>> PlatformMessages;
		std::optional<std::vector<GenericRegionPost>> RegionMessages;
		std::optional<std::vector<GenericMotd>> Motds;
		std::optional<std::vector<GenericPlatformMotd>> PlatformMotds;

		PARSE_DEFINE(GenericNewsContainer)
			PARSE_ITEM("messages", Messages)
			PARSE_ITEM_OPT("platform_messages", PlatformMessages)
			PARSE_ITEM_OPT("region_messages", RegionMessages)
			PARSE_ITEM_OPT("motds", Motds)
			PARSE_ITEM_OPT("platform_motds", PlatformMotds)
		PARSE_END
	};

	struct STWNewsContainer {
		GenericNewsContainer News;

		PARSE_DEFINE(STWNewsContainer)
			PARSE_ITEM("news", News)
		PARSE_END
	};

	struct EmegencyNoticeContainer {
		GenericNewsContainer News;

		PARSE_DEFINE(EmegencyNoticeContainer)
			PARSE_ITEM("news", News)
		PARSE_END
	};

	struct PlaylistItem {
		std::optional<std::string> Image;
		std::string PlaylistName;
		bool Hidden;
		std::optional<std::string> Violator;
		std::optional<std::string> Description;
		std::optional<std::string> DisplaySubname;
		std::optional<std::string> DisplayName;
		std::optional<std::string> SpecialBorder;

		PARSE_DEFINE(PlaylistItem)
			PARSE_ITEM_OPT("image", Image)
			PARSE_ITEM("playlist_name", PlaylistName)
			PARSE_ITEM("hidden", Hidden)
			PARSE_ITEM_OPT("violator", Violator)
			PARSE_ITEM_OPT("description", Description)
			PARSE_ITEM_OPT("display_subname", DisplaySubname)
			PARSE_ITEM_OPT("display_name", DisplayName)
			PARSE_ITEM_OPT("special_border", SpecialBorder)
		PARSE_END
	};

	struct PlaylistInfoList {
		std::vector<PlaylistItem> Playlists;

		PARSE_DEFINE(PlaylistInfoList)
			PARSE_ITEM("playlists", Playlists)
		PARSE_END
	};

	struct PlaylistInfoContainer {
		// (Example)
		// https://i.imgur.com/7YKDnAf.png In-game
		// https://i.imgur.com/vtHJ669.png JSON

		// I've yet to look over other examples

		std::string FrontendMatchmakingHeaderStyle;
		std::string FrontendMatchmakingHeaderTextDescription;
		std::string FrontendMatchmakingHeaderText;


		PlaylistInfoList PlaylistInfo;

		PARSE_DEFINE(PlaylistInfoContainer)
			PARSE_ITEM("playlist_info", PlaylistInfo)
			PARSE_ITEM("frontend_matchmaking_header_style", FrontendMatchmakingHeaderStyle)
			PARSE_ITEM("frontend_matchmaking_header_text_description", FrontendMatchmakingHeaderTextDescription)
			PARSE_ITEM("frontend_matchmaking_header_text", FrontendMatchmakingHeaderText)
		PARSE_END
	};

	struct TournamentItem {
		std::string TournamentDisplayId;
		std::string PlaylistTileImage;
		std::optional<std::string> TitleColor;
		std::optional<std::string> PrimaryColor;
		std::optional<std::string> SecondaryColor;
		std::optional<std::string> BaseColor;
		std::optional<std::string> HighlightColor;
		std::optional<std::string> ShadowColor;
		std::optional<std::string> BackgroundTextColor;
		std::optional<std::string> BackgroundLeftColor;
		std::optional<std::string> BackgroundRightColor;
		std::optional<std::string> PosterFadeColor;
		std::optional<std::string> PosterFrontImage;
		std::optional<std::string> PosterBackImage;
		std::optional<std::string> LoadingScreenImage;
		std::optional<std::string> ScheduleInfo;
		std::optional<std::string> LongFormatTitle;
		std::optional<std::string> ShortFormatTitle;
		std::optional<std::string> BackgroundTitle;
		std::string TitleLine1;
		std::optional<std::string> TitleLine2;
		std::optional<std::string> DetailsDescription;
		std::optional<std::string> FlavorDescription;
		std::optional<int> PinScoreRequirement;
		std::optional<std::string> PinEarnedText;

		PARSE_DEFINE(TournamentItem)
			PARSE_ITEM("tournament_display_id", TournamentDisplayId)
			PARSE_ITEM("playlist_tile_image", PlaylistTileImage)
			PARSE_ITEM_OPT("title_color", TitleColor)
			PARSE_ITEM_OPT("primary_color", PrimaryColor)
			PARSE_ITEM_OPT("secondary_color", SecondaryColor)
			PARSE_ITEM_OPT("base_color", BaseColor)
			PARSE_ITEM_OPT("highlight_color", HighlightColor)
			PARSE_ITEM_OPT("shadow_color", ShadowColor)
			PARSE_ITEM_OPT("background_text_color", BackgroundTextColor)
			PARSE_ITEM_OPT("background_left_color", BackgroundLeftColor)
			PARSE_ITEM_OPT("background_right_color", BackgroundRightColor)
			PARSE_ITEM_OPT("poster_fade_color", PosterFadeColor)
			PARSE_ITEM_OPT("poster_front_image", PosterFrontImage)
			PARSE_ITEM_OPT("poster_back_image", PosterBackImage)
			PARSE_ITEM_OPT("loading_screen_image", LoadingScreenImage)
			PARSE_ITEM_OPT("schedule_info", ScheduleInfo)
			PARSE_ITEM_OPT("long_format_title", LongFormatTitle)
			PARSE_ITEM_OPT("short_format_title", ShortFormatTitle)
			PARSE_ITEM_OPT("background_title", BackgroundTitle)
			PARSE_ITEM("title_line_1", TitleLine1)
			PARSE_ITEM_OPT("title_line_2", TitleLine2)
			PARSE_ITEM_OPT("details_description", DetailsDescription)
			PARSE_ITEM_OPT("flavor_description", FlavorDescription)
			PARSE_ITEM_OPT("pin_score_requirement", PinScoreRequirement)
			PARSE_ITEM_OPT("pin_earned_text", PinEarnedText)
		PARSE_END
	};

	struct TournamentInfoList {
		std::vector<TournamentItem> Tournaments;

		PARSE_DEFINE(TournamentInfoList)
			PARSE_ITEM("tournaments", Tournaments)
		PARSE_END
	};

	struct TournamentInfoContainer {
		TournamentInfoList TournamentInfo;

		PARSE_DEFINE(TournamentInfoContainer)
			PARSE_ITEM("tournament_info", TournamentInfo)
		PARSE_END
	};

	struct BRNewsV2Container {
		GenericNewsContainer News;

		PARSE_DEFINE(BRNewsV2Container)
			PARSE_ITEM("news", News)
		PARSE_END
	};

	struct CreativeNewsV2Container {
		GenericNewsContainer News;

		PARSE_DEFINE(CreativeNewsV2Container)
			PARSE_ITEM("news", News)
		PARSE_END
	};

	// No real need for this
	// TimePoint _activeDate = "2017-08-30T03:20:48.050Z"

	// Last modification time of the page content
	TimePoint LastModified;

	STWNewsContainer SaveTheWorldNews;

	EmegencyNoticeContainer EmergencyNotice;

	PlaylistInfoContainer PlaylistInfo;

	TournamentInfoContainer TournamentInfo;

	BRNewsV2Container BRNewsV2;

	CreativeNewsV2Container CreativeNewsV2;

	PARSE_DEFINE(RespGetPageInfo)
		PARSE_ITEM("lastModified", LastModified)
		PARSE_ITEM("savetheworldnews", SaveTheWorldNews)
		PARSE_ITEM("emergencynotice", EmergencyNotice)
		PARSE_ITEM("playlistinformation", PlaylistInfo)
		PARSE_ITEM("tournamentinformation", TournamentInfo)
		PARSE_ITEM("battleroyalenewsv2", BRNewsV2)
		PARSE_ITEM("creativenewsv2", CreativeNewsV2)
	PARSE_END
};