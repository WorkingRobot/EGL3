#pragma once

#include "../../JsonParsing.h"

namespace EGL3::Web::Epic::Responses {
    struct GetDeviceAuths {
        struct DeviceInfo {
            // Manufacturer? e.g: Google
            std::optional<std::string> Type;

            // Phone model e.g: Pixel 2
            std::optional<std::string> Model;

            // OS version e.g: 10
            std::optional<std::string> OS;

            PARSE_DEFINE(DeviceInfo)
                PARSE_ITEM_OPT("type", Type)
                PARSE_ITEM_OPT("model", Model)
                PARSE_ITEM_OPT("os", OS)
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

            // Provided with X-Epic-Device-Info header on creation, e.g. {"type":"Google","model":"Pixel 2","os":"10"}
            // One of the fields must be not empty/null in order to set it
            // If a field is null or does not exist, it isn't set, but if it's an empty string (as long as another field is set), it is
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