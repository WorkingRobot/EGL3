#pragma once

#include "PresenceKairosProfile.h"
#include "PresenceProperties.h"

namespace EGL3::Web::Xmpp::Json {
	struct PresenceStatus {
		const std::string& GetStatus() const {
			return Status;
		}

		const std::string& GetSessionId() const {
			return SessionId;
		}

		const std::string& GetProductName() const {
			return ProductName;
		}

		const PresenceProperties& GetProperties() const {
			return Properties;
		}

		bool IsPlaying() const {
			return Playing;
		}

		bool IsJoinable() const {
			return Joinable;
		}

		bool HasVoiceSupport() const {
			return Joinable;
		}

		const PresenceKairosProfile* GetKairosProfile() const {
			if (!KairosProfileParsed) {
				std::string KairosString;
				if (Properties.GetValue("KairosProfile", KairosString)) {
					rapidjson::Document KairosJson;
					KairosJson.Parse(KairosString.c_str(), KairosString.size());
					if (!KairosJson.HasParseError()) {
						if (!PresenceKairosProfile::Parse(KairosJson, KairosProfile.emplace())) {
							KairosProfile.reset();
							// TODO: Provide warning here and basically everywhere else with invalid json relating to presences
						}
					}
				}
				KairosProfileParsed = true;
			}
			return KairosProfile.has_value() ? &KairosProfile.value() : nullptr;
		}

		void SetProductName(const std::string& NewProduct) {
			ProductName = NewProduct;
		}

		void SetStatus(const std::string& NewStatus) {
			Status = NewStatus;
		}

		void SetKairosProfile(const PresenceKairosProfile& NewProfile) {
			KairosProfileParsed = true;
			Properties.SetValue("KairosProfile", KairosProfile.emplace(NewProfile));
		}

		void Dump() const {
			rapidjson::StringBuffer Buf;
			{
				rapidjson::Writer Writer(Buf);
				Properties.WriteTo(Writer);
			}

			printf("Status: %s\nPlaying: %s\nJoinable: %s\nVoice: %s\nSession Id: %s\nProduct: %s\nProperties: %.*s\n",
				Status.c_str(),
				Playing ? "Yes" : "No",
				Joinable ? "Yes" : "No",
				VoiceSupport ? "Yes" : "No",
				SessionId.c_str(),
				ProductName.c_str(),
				Buf.GetLength(), Buf.GetString()
			);
		}

		PARSE_DEFINE(PresenceStatus)
			PARSE_ITEM_DEF("Status", Status, "")
			PARSE_ITEM_DEF("bIsPlaying", Playing, false)
			PARSE_ITEM_DEF("bIsJoinable", Joinable, false)
			PARSE_ITEM_DEF("bHasVoiceSupport", VoiceSupport, false)
			PARSE_ITEM_DEF("ProductName", ProductName, "")
			PARSE_ITEM_DEF("SessionId", SessionId, "")
			PARSE_ITEM_DEF("Properties", Properties, PresenceProperties())
		PARSE_END

	private:
		std::string Status;

		bool Playing;

		bool Joinable;

		bool VoiceSupport;

		std::string SessionId;

		std::string ProductName;

		PresenceProperties Properties;

		// Dynamically made
		mutable bool KairosProfileParsed = false;
		mutable std::optional<PresenceKairosProfile> KairosProfile;
	};
}
