#include "epic_client.h"

EpicClient::EpicClient(const rapidjson::Document& OAuthResponse, const cpr::Authentication& AuthClient) : AuthClient(AuthClient)
{
	if (!RespOAuthToken::Parse(OAuthResponse, AuthData)) {
		// log error
		printf("BAD OAUTH JSON DATA\n");
	}

	AuthHeader = AuthData.TokenType + " " + AuthData.AccessToken;
}

EpicClient::Response<RespGetAccount> EpicClient::GetAccount()
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/public/account/" + AuthData.AccountId },
		cpr::Header{ { "Authorization", AuthHeader } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetAccount Resp;
	if (!RespGetAccount::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetAccountExternalAuths> EpicClient::GetAccountExternalAuths()
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/public/account/" + AuthData.AccountId + "/externalAuths" },
		cpr::Header{ { "Authorization", AuthHeader } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetAccountExternalAuths Resp;
	if (!RespGetAccountExternalAuths::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetDefaultBillingAccount> EpicClient::GetDefaultBillingAccount()
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/payment/accounts/" + AuthData.AccountId + "/billingaccounts/default" },
		cpr::Header{ { "Authorization", AuthHeader } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetDefaultBillingAccount Resp;
	if (!RespGetDefaultBillingAccount::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetAssets> EpicClient::GetAssets(const std::string& Platform, const std::string& Label)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/assets/" + Platform },
		cpr::Header{ { "Authorization", AuthHeader } },
		cpr::Parameters{ { "label", Label } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetAssets Resp;
	if (!RespGetAssets::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetCurrencies> EpicClient::GetCurrencies(int Start, int Count)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/currencies" },
		cpr::Header{ { "Authorization", AuthHeader } },
		cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetCurrencies Resp;
	if (!RespGetCurrencies::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetCatalogItems> EpicClient::GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails, bool IncludeMainGameDetails)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	cpr::Parameters Parameters;
	for (auto& Item : Items) {
		Parameters.AddParameter({ "id", Item });
	}
	Parameters.AddParameter({ "includeDLCDetails", IncludeDLCDetails });
	Parameters.AddParameter({ "includeMainGameDetails", IncludeMainGameDetails });
	Parameters.AddParameter({ "country", Country });
	Parameters.AddParameter({ "locale", Locale });

	auto Response = Http::Get(
		cpr::Url{ "https://catalog-public-service-prod06.ol.epicgames.com/catalog/api/shared/namespace/" + Namespace + "/bulk/items" },
		cpr::Header{ { "Authorization", AuthHeader } },
		Parameters
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetCatalogItems Resp;
	if (!RespGetCatalogItems::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetEntitlements> EpicClient::GetEntitlements(int Start, int Count)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://entitlement-public-service-prod08.ol.epicgames.com/entitlement/api/account/" + AuthData.AccountId + "/entitlements" },
		cpr::Header{ { "Authorization", AuthHeader } },
		cpr::Parameters{ { "start", std::to_string(Start) }, { "count", std::to_string(Count) } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetEntitlements Resp;
	if (!RespGetEntitlements::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetExternalSourceSettings> EpicClient::GetExternalSourceSettings(const std::string& Platform)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://friends-public-service-prod06.ol.epicgames.com/friends/api/v1/" + AuthData.AccountId + "/settings/externalSources/" + Platform },
		cpr::Header{ { "Authorization", AuthHeader } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetExternalSourceSettings Resp;
	if (!RespGetExternalSourceSettings::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetFriends> EpicClient::GetFriends(bool IncludePending)
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://friends-public-service-prod06.ol.epicgames.com/friends/api/public/friends/" + AuthData.AccountId },
		cpr::Header{ { "Authorization", AuthHeader } },
		cpr::Parameters{ { "includePending", IncludePending } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetFriends Resp;
	if (!RespGetFriends::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

EpicClient::Response<RespGetBlockedUsers> EpicClient::GetBlockedUsers()
{
	RunningFunctionGuard Guard(*this);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

	if (GetCancelled()) { return ERROR_CANCELLED; }

	auto Response = Http::Get(
		cpr::Url{ "https://friends-public-service-prod06.ol.epicgames.com/friends/api/public/blocklist/" },
		cpr::Header{ { "Authorization", AuthHeader } }
	);

	if (GetCancelled()) { return ERROR_CANCELLED; }

	if (Response.status_code != 200) {
		return ERROR_CODE_NOT_200;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return ERROR_CODE_NOT_JSON;
	}

	RespGetBlockedUsers Resp;
	if (!RespGetBlockedUsers::Parse(RespJson, Resp)) {
		return ERROR_CODE_BAD_JSON;
	}

	return Resp;
}

void EpicClient::KillAuthentication()
{
	// Token isn't expired yet
	if (AuthData.ExpiresAt > TimePoint::clock::now()) {
		// This will return a 204
		// If it fails, it's on epic's blame. We don't really need any handling
		Http::Delete(
			cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/oauth/sessions/kill/" + AuthData.AccessToken },
			cpr::Header{ { "Authorization", AuthHeader } }
		);
	}
}

bool EpicClient::EnsureTokenValidity()
{
	// Make sure that no other thread will call in to this function and end up with the refresh token being used multiple times
	const std::lock_guard<std::mutex> lock(TokenValidityMutex);

	auto Now = TimePoint::clock::now();

	if (AuthData.ExpiresAt > Now) {
		return true; // access token isn't invalid yet
	}

	if (AuthData.RefreshExpiresAt < Now) {
		return false; // refresh token is also invalid, what do we do here?
	}

	auto Response = Http::Post(
		cpr::Url{ "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token" },
		AuthClient,
		cpr::Payload{ { "grant_type", "refresh_token" }, { "refresh_token", AuthData.RefreshToken } }
	);

	if (Response.status_code != 200) {
		return false;
	}

	auto RespJson = Http::ParseJson(Response);

	if (RespJson.HasParseError()) {
		return false;
	}

	if (!RespOAuthToken::Parse(RespJson, AuthData)) {
		// log error
		printf("BAD OAUTH JSON DATA\n");
		return false;
	}

	AuthHeader = AuthData.TokenType + " " + AuthData.AccessToken;
	return true;
}
