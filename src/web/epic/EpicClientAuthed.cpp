#include "EpicClientAuthed.h"

#include "../Http.h"
#include "../RunningFunctionGuard.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Epic {
	EpicClientAuthed::EpicClientAuthed(const rapidjson::Document& OAuthResponse, const cpr::Authentication& AuthClient) : AuthClient(AuthClient)
	{
		if (!Responses::OAuthToken::Parse(OAuthResponse, AuthData)) {
			// log error
			printf("BAD OAUTH JSON DATA\n");
		}

		AuthHeader = AuthData.TokenType + " " + AuthData.AccessToken;
	}

	EpicClientAuthed::~EpicClientAuthed()
	{
		EnsureCallCompletion();

		// Token isn't expired yet
		if (AuthData.ExpiresAt > TimePoint::clock::now()) {
			// This will return a 204
			// If it fails, it's on epic's blame. We don't really need any handling
			Http::Delete(
				Http::FormatUrl("https://account-public-service-prod.ol.epicgames.com/account/api/oauth/sessions/kill/%s", AuthData.AccessToken.c_str()),
				cpr::Header{ { "Authorization", AuthHeader } }
			);
		}
	}

	Response<Responses::GetAccount> EpicClientAuthed::GetAccount()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://account-public-service-prod.ol.epicgames.com/account/api/public/account/%s", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetAccount Resp;
		if (!Responses::GetAccount::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetAccountExternalAuths> EpicClientAuthed::GetAccountExternalAuths()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://account-public-service-prod.ol.epicgames.com/account/api/public/account/%s/externalAuths", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetAccountExternalAuths Resp;
		if (!Responses::GetAccountExternalAuths::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}
	
	Response<Responses::GetAccounts> EpicClientAuthed::GetAccounts(const std::vector<std::string>& Accounts)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		cpr::Parameters Parameters;
		for (auto& Item : Accounts) {
			Parameters.AddParameter({ "accountId", Item }, cpr::CurlHolder());
		}

		auto Response = Http::Get(
			Http::FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/public/account"),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetAccounts Resp;
		if (!Responses::GetAccounts::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetDeviceAuths> EpicClientAuthed::GetDeviceAuths()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/public/account/%s/deviceAuth", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetDeviceAuths Resp;
		if (!Responses::GetDeviceAuths::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetDeviceAuths::DeviceAuth> EpicClientAuthed::CreateDeviceAuth()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Post(
			Http::FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/public/account/%s/deviceAuth", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ "{}" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetDeviceAuths::DeviceAuth Resp;
		if (!Responses::GetDeviceAuths::DeviceAuth::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetDefaultBillingAccount> EpicClientAuthed::GetDefaultBillingAccount()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/payment/accounts/%s/billingaccounts/default", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetDefaultBillingAccount Resp;
		if (!Responses::GetDefaultBillingAccount::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetAssets> EpicClientAuthed::GetAssets(const std::string& Platform, const std::string& Label)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/assets/%s", Platform.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "label", Label } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetAssets Resp;
		if (!Responses::GetAssets::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetDownloadInfo> EpicClientAuthed::GetDownloadInfo(const std::string& Platform, const std::string& Label, const std::string& CatalogItemId, const std::string& AppName)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Post(
			Http::FormatUrl("https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/assets/v2/platform/%s/catalogItem/%s/app/%s/label/%s", Platform.c_str(), CatalogItemId.c_str(), AppName.c_str(), Label.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ "{}" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetDownloadInfo Resp;
		if (!Responses::GetDownloadInfo::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetCurrencies> EpicClientAuthed::GetCurrencies(int Start, int Count)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/currencies"),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetCurrencies Resp;
		if (!Responses::GetCurrencies::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetCatalogItems> EpicClientAuthed::GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails, bool IncludeMainGameDetails)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		cpr::Parameters Parameters{
			{ "includeDLCDetails", std::to_string(IncludeDLCDetails) },
			{ "includeMainGameDetails", std::to_string(IncludeMainGameDetails) },
			{ "country", Country },
			{ "locale", Locale }
		};
		for (auto& Item : Items) {
			Parameters.AddParameter({ "id", Item }, cpr::CurlHolder());
		}

		auto Response = Http::Get(
			Http::FormatUrl("https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/namespace/%s/bulk/items", Namespace.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetCatalogItems Resp;
		if (!Responses::GetCatalogItems::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetEntitlements> EpicClientAuthed::GetEntitlements(int Start, int Count)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://entitlement-public-service-prod08.ol.epicgames.com/entitlement/api/account/%s/entitlements", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetEntitlements Resp;
		if (!Responses::GetEntitlements::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetFriendsSummary> EpicClientAuthed::GetFriendsSummary()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/summary", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetFriendsSummary Resp;
		if (!Responses::GetFriendsSummary::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetFriends> EpicClientAuthed::GetFriends()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetFriends Resp;
		if (!Responses::GetFriends::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetFriendsRequested> EpicClientAuthed::GetFriendsInboundRequests()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/incoming", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetFriendsRequested Resp;
		if (!Responses::GetFriendsRequested::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetFriendsRequested> EpicClientAuthed::GetFriendsOutboundRequests()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/outgoing", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetFriendsRequested Resp;
		if (!Responses::GetFriendsRequested::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetFriendsSuggested> EpicClientAuthed::GetFriendsSuggested()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/suggested", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetFriendsSuggested Resp;
		if (!Responses::GetFriendsSuggested::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetBlockedUsers> EpicClientAuthed::GetBlockedUsers()
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/blocklist", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetBlockedUsers Resp;
		if (!Responses::GetBlockedUsers::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetFriendsSummary::RealFriend> EpicClientAuthed::GetFriend(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetFriendsSummary::RealFriend Resp;
		if (!Responses::GetFriendsSummary::RealFriend::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<void> EpicClientAuthed::AddFriend(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Post(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::RemoveFriend(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Delete(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::SetFriendAlias(const std::string& AccountId, const std::string& Nickname)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Put(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/alias", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "text/plain" } },
			cpr::Body{ Nickname }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::ClearFriendAlias(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Delete(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/alias", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::SetFriendNote(const std::string& AccountId, const std::string& Note)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Put(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/note", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "text/plain" } },
			cpr::Body{ Note }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::ClearFriendNote(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Delete(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/note", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::BlockUser(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Post(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/blocklist/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<void> EpicClientAuthed::UnblockUser(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Delete(
			Http::FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/blocklist/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<Responses::GetAvailableSettingValues> EpicClientAuthed::GetAvailableSettingValues(const std::string& Setting)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://channels-public-service-prod.ol.epicgames.com/api/v1/user/%s/setting/%s/available", AuthData.AccountId->c_str(), Setting.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetAvailableSettingValues Resp;
		if (!Responses::GetAvailableSettingValues::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<Responses::GetSettingsForAccounts> EpicClientAuthed::GetSettingsForAccounts(const std::vector<std::string>& Accounts, const std::initializer_list<std::string>& Settings)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		cpr::Parameters Parameters;

		for (auto& Account : Accounts) {
			Parameters.AddParameter({ "accountId", Account }, cpr::CurlHolder());
		}

		for (auto& Setting : Settings) {
			Parameters.AddParameter({ "settingKey", Setting }, cpr::CurlHolder());
		}

		auto Response = Http::Get(
			Http::FormatUrl("https://channels-public-service-prod.ol.epicgames.com/api/v1/user/setting"),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetSettingsForAccounts Resp;
		if (!Responses::GetSettingsForAccounts::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	Response<void> EpicClientAuthed::UpdateAccountSetting(const std::string& Setting, const std::string& Value)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!AuthData.AccountId.has_value()) { return ErrorCode::InvalidToken; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		rapidjson::StringBuffer Buf;
		{
			rapidjson::Writer Writer(Buf);

			Writer.StartObject();

			Writer.Key("value");
			Writer.String(Value.c_str(), Value.size());

			Writer.EndObject();
		}

		auto Response = Http::Put(
			Http::FormatUrl("https://channels-public-service-prod.ol.epicgames.com/api/v1/user/%s/setting/%s", AuthData.AccountId->c_str(), Setting.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ Buf.GetString(), Buf.GetSize() }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 204) {
			return ErrorCode::Unsuccessful;
		}

		return ErrorCode::Success;
	}

	Response<Responses::GetLightswitchStatus::ServiceStatus> EpicClientAuthed::GetLightswitchStatus(const std::string& AppName)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		auto Response = Http::Get(
			Http::FormatUrl("https://lightswitch-public-service-prod06.ol.epicgames.com/lightswitch/api/service/%s/status", AppName.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetLightswitchStatus::ServiceStatus Resp;
		if (!Responses::GetLightswitchStatus::ServiceStatus::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}


	Response<Responses::GetLightswitchStatus> EpicClientAuthed::GetLightswitchStatus(const std::initializer_list<std::string>& AppNames)
	{
		RunningFunctionGuard Guard(Lock);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (!EnsureTokenValidity()) { return ErrorCode::InvalidToken; }

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		cpr::Parameters Parameters;
		for (auto& AppName : AppNames) {
			Parameters.AddParameter({ "serviceId", AppName }, cpr::CurlHolder());
		}
		auto Response = Http::Get(
			Http::FormatUrl("https://lightswitch-public-service-prod06.ol.epicgames.com/lightswitch/api/service/bulk/status"),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return ErrorCode::Cancelled; }

		if (Response.status_code != 200) {
			return ErrorCode::Unsuccessful;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ErrorCode::NotJson;
		}

		Responses::GetLightswitchStatus Resp;
		if (!Responses::GetLightswitchStatus::Parse(RespJson, Resp)) {
			return ErrorCode::BadJson;
		}

		return Resp;
	}

	bool EpicClientAuthed::EnsureTokenValidity()
	{
		// Make sure that no other thread will call in to this function and end up with the refresh token being used multiple times
		const std::lock_guard<std::mutex> lock(TokenValidityMutex);

		auto Now = TimePoint::clock::now();

		if (AuthData.ExpiresAt > Now) {
			return true; // access token isn't invalid yet
		}

		cpr::Response Response;
		// Refresh tokens were given in the oauth response
		if (AuthData.RefreshToken.has_value()) {
			if (AuthData.RefreshExpiresAt.value() < Now) {
				return false; // refresh token is also invalid, what do we do here?
			}

			Response = Http::Post(
				Http::FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token"),
				AuthClient,
				cpr::Payload{ { "grant_type", "refresh_token" }, { "refresh_token", AuthData.RefreshToken.value() } }
			);
		}
		// Refresh tokens were not given in the oauth response, use client_credentials here
		else {
			Response = Http::Post(
				Http::FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token"),
				AuthClient,
				cpr::Payload{ { "grant_type", "client_credentials" } }
			);
		}

		if (Response.status_code != 200) {
			return false;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return false;
		}

		if (!Responses::OAuthToken::Parse(RespJson, AuthData)) {
			// log error
			printf("BAD OAUTH JSON DATA\n");
			return false;
		}

		AuthHeader = AuthData.TokenType + " " + AuthData.AccessToken;
		return true;
	}
}
