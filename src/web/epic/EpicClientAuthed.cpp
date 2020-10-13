#include "EpicClientAuthed.h"

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

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/public/account/" + AuthData.AccountId.value() },
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

		Responses::GetAccount Resp;
		if (!Responses::GetAccount::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetAccountExternalAuths> EpicClientAuthed::GetAccountExternalAuths()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/public/account/" + AuthData.AccountId.value() + "/externalAuths" },
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

		Responses::GetAccountExternalAuths Resp;
		if (!Responses::GetAccountExternalAuths::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDeviceAuths> EpicClientAuthed::GetDeviceAuths()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/public/account/" + AuthData.AccountId.value() + "/deviceAuth" },
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

		Responses::GetDeviceAuths Resp;
		if (!Responses::GetDeviceAuths::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDeviceAuths::DeviceAuth> EpicClientAuthed::CreateDeviceAuth()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Post(
			cpr::Url{ "https://account-public-service-prod.ol.epicgames.com/account/api/public/account/" + AuthData.AccountId.value() + "/deviceAuth" },
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ "{}" }
		);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (Response.status_code != 200) {
			return ERROR_CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ERROR_CODE_NOT_JSON;
		}

		Responses::GetDeviceAuths::DeviceAuth Resp;
		if (!Responses::GetDeviceAuths::DeviceAuth::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDefaultBillingAccount> EpicClientAuthed::GetDefaultBillingAccount()
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/payment/accounts/" + AuthData.AccountId.value() + "/billingaccounts/default" },
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

		Responses::GetDefaultBillingAccount Resp;
		if (!Responses::GetDefaultBillingAccount::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetAssets> EpicClientAuthed::GetAssets(const std::string& Platform, const std::string& Label)
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

		Responses::GetAssets Resp;
		if (!Responses::GetAssets::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetDownloadInfo> EpicClientAuthed::GetDownloadInfo(const std::string& Platform, const std::string& Label, const std::string& CatalogItemId, const std::string& AppName)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Post(
			cpr::Url{ "https://launcher-public-service-prod-m.ol.epicgames.com/launcher/api/public/assets/v2/platform/" + Platform + "/catalogItem/" + CatalogItemId + "/app/" + AppName + "/label/" + Label },
			cpr::Header{ { "Authorization", AuthHeader }, { "Content-Type", "application/json"} },
			cpr::Body{ "{}" }
		);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (Response.status_code != 200) {
			return ERROR_CODE_NOT_200;
		}

		auto RespJson = Http::ParseJson(Response);

		if (RespJson.HasParseError()) {
			return ERROR_CODE_NOT_JSON;
		}

		Responses::GetDownloadInfo Resp;
		if (!Responses::GetDownloadInfo::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetCurrencies> EpicClientAuthed::GetCurrencies(int Start, int Count)
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

		Responses::GetCurrencies Resp;
		if (!Responses::GetCurrencies::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetCatalogItems> EpicClientAuthed::GetCatalogItems(const std::string& Namespace, const std::initializer_list<std::string>& Items, const std::string& Country, const std::string& Locale, bool IncludeDLCDetails, bool IncludeMainGameDetails)
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

		Responses::GetCatalogItems Resp;
		if (!Responses::GetCatalogItems::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetEntitlements> EpicClientAuthed::GetEntitlements(int Start, int Count)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://entitlement-public-service-prod08.ol.epicgames.com/entitlement/api/account/" + AuthData.AccountId.value() + "/entitlements" },
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

		Responses::GetEntitlements Resp;
		if (!Responses::GetEntitlements::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetExternalSourceSettings> EpicClientAuthed::GetExternalSourceSettings(const std::string& Platform)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://friends-public-service-prod06.ol.epicgames.com/friends/api/v1/" + AuthData.AccountId.value() + "/settings/externalSources/" + Platform },
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

		Responses::GetExternalSourceSettings Resp;
		if (!Responses::GetExternalSourceSettings::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetFriends> EpicClientAuthed::GetFriends(bool IncludePending)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!AuthData.AccountId.has_value()) { return ERROR_INVALID_TOKEN; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://friends-public-service-prod06.ol.epicgames.com/friends/api/public/friends/" + AuthData.AccountId.value() },
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

		Responses::GetFriends Resp;
		if (!Responses::GetFriends::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetBlockedUsers> EpicClientAuthed::GetBlockedUsers()
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

		Responses::GetBlockedUsers Resp;
		if (!Responses::GetBlockedUsers::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	BaseClient::Response<Responses::GetLightswitchStatus::ServiceStatus> EpicClientAuthed::GetLightswitchStatus(const std::string& AppName)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		auto Response = Http::Get(
			cpr::Url{ "https://lightswitch-public-service-prod06.ol.epicgames.com/lightswitch/api/service/" + AppName + "/status" },
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

		Responses::GetLightswitchStatus::ServiceStatus Resp;
		if (!Responses::GetLightswitchStatus::ServiceStatus::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}


	BaseClient::Response<Responses::GetLightswitchStatus> EpicClientAuthed::GetLightswitchStatus(const std::initializer_list<std::string>& AppNames)
	{
		RunningFunctionGuard Guard(*this);

		if (GetCancelled()) { return ERROR_CANCELLED; }

		if (!EnsureTokenValidity()) { return ERROR_INVALID_TOKEN; }

		if (GetCancelled()) { return ERROR_CANCELLED; }

		cpr::Parameters Parameters;
		for (auto& AppName : AppNames) {
			Parameters.AddParameter({ "serviceId", AppName });
		}
		auto Response = Http::Get(
			cpr::Url{ "https://lightswitch-public-service-prod06.ol.epicgames.com/lightswitch/api/service/bulk/status" },
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

		Responses::GetLightswitchStatus Resp;
		if (!Responses::GetLightswitchStatus::Parse(RespJson, Resp)) {
			return ERROR_CODE_BAD_JSON;
		}

		return Resp;
	}

	void EpicClientAuthed::KillAuthentication()
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
				cpr::Url{ "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token" },
				AuthClient,
				cpr::Payload{ { "grant_type", "refresh_token" }, { "refresh_token", AuthData.RefreshToken.value() } }
			);
		}
		// Refresh tokens were not given in the oauth response, use client_credentials here
		else {
			Response = Http::Post(
				cpr::Url{ "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token" },
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
