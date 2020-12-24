#include "EpicClientAuthed.h"

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

	BaseClient::Response<Responses::GetAccount> EpicClientAuthed::GetAccount()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://account-public-service-prod.ol.epicgames.com/account/api/public/account/%s", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetAccount Resp;
		if (!Responses::GetAccount::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetAccountExternalAuths> EpicClientAuthed::GetAccountExternalAuths()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://account-public-service-prod.ol.epicgames.com/account/api/public/account/%s/externalAuths", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetAccountExternalAuths Resp;
		if (!Responses::GetAccountExternalAuths::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}
	
	BaseClient::Response<Responses::GetAccounts> EpicClientAuthed::GetAccounts(const std::vector<std::string>& Accounts)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		cpr::Parameters Parameters;
		for (auto& Item : Accounts) {
			Parameters.AddParameter({ "accountId", Item }, cpr::CurlHolder());
		}

		auto Response = Http::Get(
			FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/public/account"),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetAccounts Resp;
		if (!Responses::GetAccounts::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDeviceAuths> EpicClientAuthed::GetDeviceAuths()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/public/account/%s/deviceAuth", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetDeviceAuths Resp;
		if (!Responses::GetDeviceAuths::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDeviceAuths::DeviceAuth> EpicClientAuthed::CreateDeviceAuth()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Post(
			FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/public/account/%s/deviceAuth", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ "{}" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetDeviceAuths::DeviceAuth Resp;
		if (!Responses::GetDeviceAuths::DeviceAuth::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDefaultBillingAccount> EpicClientAuthed::GetDefaultBillingAccount()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/payment/accounts/%s/billingaccounts/default", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetDefaultBillingAccount Resp;
		if (!Responses::GetDefaultBillingAccount::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetAssets> EpicClientAuthed::GetAssets(const std::string& Platform, const std::string& Label)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/assets/%s", Platform.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "label", Label } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetAssets Resp;
		if (!Responses::GetAssets::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDownloadInfo> EpicClientAuthed::GetDownloadInfo(const std::string& Platform, const std::string& Label, const std::string& CatalogItemId, const std::string& AppName)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Post(
			FormatUrl("https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/assets/v2/platform/%s/catalogItem/%s/app/%s/label/%s", Platform.c_str(), CatalogItemId.c_str(), AppName.c_str(), Label.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ "{}" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetDownloadInfo Resp;
		if (!Responses::GetDownloadInfo::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetCurrencies> EpicClientAuthed::GetCurrencies(int Start, int Count)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/currencies"),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetCurrencies Resp;
		if (!Responses::GetCurrencies::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetCatalogItems> EpicClientAuthed::GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails, bool IncludeMainGameDetails)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

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
			FormatUrl("https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/namespace/%s/bulk/items", Namespace.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetCatalogItems Resp;
		if (!Responses::GetCatalogItems::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetEntitlements> EpicClientAuthed::GetEntitlements(int Start, int Count)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://entitlement-public-service-prod08.ol.epicgames.com/entitlement/api/account/%s/entitlements", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetEntitlements Resp;
		if (!Responses::GetEntitlements::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriendsSummary> EpicClientAuthed::GetFriendsSummary()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/summary", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetFriendsSummary Resp;
		if (!Responses::GetFriendsSummary::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriends> EpicClientAuthed::GetFriends()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetFriends Resp;
		if (!Responses::GetFriends::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriendsRequested> EpicClientAuthed::GetFriendsInboundRequests()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/incoming", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetFriendsRequested Resp;
		if (!Responses::GetFriendsRequested::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriendsRequested> EpicClientAuthed::GetFriendsOutboundRequests()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/outgoing", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetFriendsRequested Resp;
		if (!Responses::GetFriendsRequested::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriendsSuggested> EpicClientAuthed::GetFriendsSuggested()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/suggested", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetFriendsSuggested Resp;
		if (!Responses::GetFriendsSuggested::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetBlockedUsers> EpicClientAuthed::GetBlockedUsers()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/blocklist", AuthData.AccountId->c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetBlockedUsers Resp;
		if (!Responses::GetBlockedUsers::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriendsSummary::RealFriend> EpicClientAuthed::GetFriend(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Parameters{ { "displayNames", "true" } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetFriendsSummary::RealFriend Resp;
		if (!Responses::GetFriendsSummary::RealFriend::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<void> EpicClientAuthed::AddFriend(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Post(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::RemoveFriend(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Delete(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::SetFriendAlias(const std::string& AccountId, const std::string& Nickname)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Put(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/alias", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "text/plain" } },
			cpr::Body{ Nickname }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::ClearFriendAlias(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Delete(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/alias", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::SetFriendNote(const std::string& AccountId, const std::string& Note)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Put(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/note", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "text/plain" } },
			cpr::Body{ Note }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::ClearFriendNote(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Delete(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/friends/%s/note", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::BlockUser(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Post(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/blocklist/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<void> EpicClientAuthed::UnblockUser(const std::string& AccountId)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Delete(
			FormatUrl("https://friends-public-service-prod.ol.epicgames.com/friends/api/v1/%s/blocklist/%s", AuthData.AccountId->c_str(), AccountId.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } },
			cpr::Body{ "" }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<Responses::GetAvailableSettingValues> EpicClientAuthed::GetAvailableSettingValues(const std::string& Setting)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://channels-public-service-prod.ol.epicgames.com/api/v1/user/%s/setting/%s/available", AuthData.AccountId->c_str(), Setting.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetAvailableSettingValues Resp;
		if (!Responses::GetAvailableSettingValues::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetSettingsForAccounts> EpicClientAuthed::GetSettingsForAccounts(const std::vector<std::string>& Accounts, const std::initializer_list<std::string>& Settings)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		cpr::Parameters Parameters;

		for (auto& Account : Accounts) {
			Parameters.AddParameter({ "accountId", Account }, cpr::CurlHolder());
		}

		for (auto& Setting : Settings) {
			Parameters.AddParameter({ "settingKey", Setting }, cpr::CurlHolder());
		}

		auto Response = Http::Get(
			FormatUrl("https://channels-public-service-prod.ol.epicgames.com/api/v1/user/setting"),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetSettingsForAccounts Resp;
		if (!Responses::GetSettingsForAccounts::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<void> EpicClientAuthed::UpdateAccountSetting(const std::string& Setting, const std::string& Value)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		rapidjson::StringBuffer Buf;
		{
			rapidjson::Writer Writer(Buf);

			Writer.StartObject();

			Writer.Key("value");
			Writer.String(Value.c_str(), Value.size());

			Writer.EndObject();
		}

		auto Response = Http::Put(
			FormatUrl("https://channels-public-service-prod.ol.epicgames.com/api/v1/user/%s/setting/%s", AuthData.AccountId->c_str(), Setting.c_str()),
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ Buf.GetString(), Buf.GetSize() }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 204) {
			return CODE_NOT_200;
		}

		return SUCCESS;
	}

	BaseClient::Response<Responses::GetLightswitchStatus::ServiceStatus> EpicClientAuthed::GetLightswitchStatus(const std::string& AppName)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		auto Response = Http::Get(
			FormatUrl("https://lightswitch-public-service-prod06.ol.epicgames.com/lightswitch/api/service/%s/status", AppName.c_str()),
			cpr::Header{ { "Authorization", AuthHeader } }
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetLightswitchStatus::ServiceStatus Resp;
		if (!Responses::GetLightswitchStatus::ServiceStatus::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}


	BaseClient::Response<Responses::GetLightswitchStatus> EpicClientAuthed::GetLightswitchStatus(const std::initializer_list<std::string>& AppNames)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return CANCELLED; }

		if (!EnsureTokenValidity()) { return INVALID_TOKEN; }

		if (GetCancelled()) { return CANCELLED; }

		cpr::Parameters Parameters;
		for (auto& AppName : AppNames) {
			Parameters.AddParameter({ "serviceId", AppName }, cpr::CurlHolder());
		}
		auto Response = Http::Get(
			FormatUrl("https://lightswitch-public-service-prod06.ol.epicgames.com/lightswitch/api/service/bulk/status"),
			cpr::Header{ { "Authorization", AuthHeader } },
			Parameters
		);

		if (GetCancelled()) { return CANCELLED; }

		if (Response.status_code != 200) {
			return CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return CODE_NOT_JSON;
		}

		Responses::GetLightswitchStatus Resp;
		if (!Responses::GetLightswitchStatus::Parse(RespJson, Resp)) {
			return CODE_BAD_JSON;
		}

		return Resp;
	}

	void EpicClientAuthed::KillAuthentication()
	{
		// Token isn't expired yet
		if (AuthData.ExpiresAt > Responses::TimePoint::clock::now()) {
			// This will return a 204
			// If it fails, it's on epic's blame. We don't really need any handling
			Http::Delete(
				FormatUrl("https://account-public-service-prod.ol.epicgames.com/account/api/oauth/sessions/kill/%s", AuthData.AccessToken.c_str()),
				cpr::Header{ { "Authorization", AuthHeader } }
			);
		}
	}

	bool EpicClientAuthed::EnsureTokenValidity()
	{
		// Make sure that no other thread will call in to this function and end up with the refresh token being used multiple times
		const std::lock_guard<std::mutex> lock(TokenValidityMutex);

		auto Now = Responses::TimePoint::clock::now();

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
				FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token"),
				AuthClient,
				cpr::Payload{ { "grant_type", "refresh_token" }, { "refresh_token", AuthData.RefreshToken.value() } }
			);
		}
		// Refresh tokens were not given in the oauth response, use client_credentials here
		else {
			Response = Http::Post(
				FormatUrl("https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token"),
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
