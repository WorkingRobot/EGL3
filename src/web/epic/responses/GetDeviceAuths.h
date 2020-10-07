#pragma once

namespace EGL3::Web::Epic::Responses {
	struct GetDeviceAuths {
		struct DeviceInfo {
			// Manufacturer? e.g: Google
			std::string Type;

			// Phone model e.g: Pixel 2
			std::string Model;

			// OS version e.g: 10
			std::string OS;

			PARSE_DEFINE(DeviceInfo)
				PARSE_ITEM("type", Type)
				PARSE_ITEM("model", Model)
				PARSE_ITEM("os", OS)
			PARSE_END
		};

		struct DeviceEventData {
			// City, Country (probably gets this from a URL similar to https://www.epicgames.com/id/api/location)
			std::string Location;

			// Ipv4 address, haven't seen/tried this with ipv6
			std::string IPAddress;

			// Time of event occurrence
			TimePoint DateTime;

			PARSE_DEFINE(DeviceEventData)
				PARSE_ITEM("location", Location)
				PARSE_ITEM("ipAddress", IPAddress)
				PARSE_ITEM("dateTime", DateTime)
			PARSE_END
		};

		struct DeviceAuth {
			// Unique id of device, created at time of auth creation
			std::string DeviceId;

			// Account id of device holder
			std::string AccountId;

			// Secret is only provided when creating the auth
			std::optional<std::string> Secret;

			// If a user agent was given during the creation, it'll be here
			std::optional<std::string> UserAgent;

			// Not sure how to set this from creation
			std::optional<DeviceInfo> DeviceInfo;

			// When the auth was created
			DeviceEventData Created;

			// When the auth was last used
			std::optional<DeviceEventData> LastAccess;

			PARSE_DEFINE(DeviceAuth)
				PARSE_ITEM("deviceId", DeviceId)
				PARSE_ITEM("accountId", AccountId)
				PARSE_ITEM_OPT("secret", Secret)
				PARSE_ITEM_OPT("userAgent", UserAgent)
				PARSE_ITEM_OPT("deviceInfo", DeviceInfo)
				PARSE_ITEM("created", Created)
				PARSE_ITEM_OPT("lastAccess", LastAccess)
			PARSE_END
		};

		std::vector<DeviceAuth> Auths;

		PARSE_DEFINE(GetDeviceAuths)
			PARSE_ITEM_ROOT(Auths)
		PARSE_END
	};
}