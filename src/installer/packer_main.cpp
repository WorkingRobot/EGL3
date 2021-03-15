#include "../utils/streams/FileStream.h"
#include "backend/Packer.h"

using namespace EGL3;
using namespace EGL3::Installer;

int main() {
	Backend::RegistryInfo RegInfo{
		.ProductGuid = "EGL3",
		.DisplayIcon = "EGL3.exe",
		.DisplayName = "EGL3",
		.DisplayVersion = CONFIG_VERSION_SHORT,
		.EstimatedSize = 0,
		.HelpLink = "https://epic.gl/discord",
		.Publisher = "WorkingRobot",
		.UninstallString = "uninstall.exe",
		.URLInfoAbout = "https://epic.gl",
		.URLUpdateInfo = "https://epic.gl/changelog",
		.VersionMajor = CONFIG_VERSION_MAJOR,
		.VersionMinor = CONFIG_VERSION_MINOR,
		.LaunchExe = "EGL3.exe"
	};

	Utils::Streams::FileStream Stream;
	if (Stream.open("installdata.eglu", "wb")) {
		Backend::Packer inst(R"(J:\Code\Visual Studio 2017\Projects\EGL3\out\install\x64-Release)", RegInfo, Stream);
	}
}