#pragma once

#include "../../utils/streams/Stream.h"
#include "ManifestData.h"

namespace EGL3::Storage::Game {
	using Utils::Streams::Stream;

	struct ManifestData {
		char LaunchExeString[256];		// Path to the executable to launch
		char LaunchCommand[256];		// Extra command arguments to add to the executable
		char AppNameString[64];			// Manifest-specific app name - example: FortniteReleaseBuilds
		uint32_t AppID;					// Manifest-specific app id - example: 1
		uint32_t BaseUrlCount;			// Base urls (or CloudDirs), used to download chunks
		// A list of zero terminated strings (no alignment)
		// Example: ABCD\0DEFG\0 is a url count of 2 with ABCD and DEFG
		std::vector<std::string> BaseUrls;

		friend Stream& operator>>(Stream& Stream, ManifestData& ManifestData) {
			Stream >> ManifestData.LaunchExeString;
			Stream >> ManifestData.LaunchCommand;
			Stream >> ManifestData.AppNameString;
			Stream >> ManifestData.AppID;
			Stream >> ManifestData.BaseUrlCount;
			for (uint32_t i = 0; i < ManifestData.BaseUrlCount; ++i) {
				auto& BaseUrl = ManifestData.BaseUrls.emplace_back();
				Stream >> BaseUrl;
			}

			return Stream;
		}

		friend Stream& operator<<(Stream& Stream, ManifestData& ManifestData) {
			Stream << ManifestData.LaunchExeString;
			Stream << ManifestData.LaunchCommand;
			Stream << ManifestData.AppNameString;
			Stream << ManifestData.AppID;
			Stream << ManifestData.BaseUrlCount;
			for (auto& BaseUrl : ManifestData.BaseUrls) {
				Stream << BaseUrl;
			}

			return Stream;
		}
	};
}