#include "PresenceStatus.h"

#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
	const std::string& PresenceStatus::GetStatus() const {
		return Status;
	}

	const std::string& PresenceStatus::GetSessionId() const {
		return SessionId;
	}

	const std::string& PresenceStatus::GetProductName() const {
		return ProductName;
	}

	const PresenceProperties& PresenceStatus::GetProperties() const {
		return Properties;
	}

	bool PresenceStatus::IsPlaying() const {
		return Playing;
	}

	bool PresenceStatus::IsJoinable() const {
		return Joinable;
	}

	bool PresenceStatus::HasVoiceSupport() const {
		return Joinable;
	}

	const PresenceKairosProfile* PresenceStatus::GetKairosProfile() const {
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

	void PresenceStatus::SetProductName(const std::string& NewProduct) {
		ProductName = NewProduct;
	}

	void PresenceStatus::SetStatus(const std::string& NewStatus) {
		Status = NewStatus;
	}

	void PresenceStatus::SetKairosProfile(const PresenceKairosProfile& NewProfile) {
		KairosProfileParsed = true;
		Properties.SetValue("KairosProfile", KairosProfile.emplace(NewProfile));
	}

	void PresenceStatus::Dump() const {
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
			(int)Buf.GetLength(), Buf.GetString()
		);
	}
}
