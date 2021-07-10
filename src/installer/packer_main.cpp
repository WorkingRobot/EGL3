#include "../utils/streams/FileStream.h"
#include "backend/Packer.h"

using namespace EGL3;
using namespace EGL3::Installer;

int main() {
    Backend::VersionInfo VersionInfo{
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
        .LaunchExe = "EGL3.exe",
        .VersionFull = CONFIG_VERSION_LONG,
        .VersionNum = (uint64_t(CONFIG_VERSION_MAJOR) << 16) | (uint64_t(CONFIG_VERSION_MINOR) << 8) | (uint64_t(CONFIG_VERSION_PATCH) << 0)
    };

    {
        Utils::Streams::FileStream PatchNotesStream;
        PatchNotesStream.open("installdata.txt", "rb");
        VersionInfo.PatchNotes.resize(PatchNotesStream.size());
        PatchNotesStream.read(VersionInfo.PatchNotes.data(), PatchNotesStream.size());
    }

    Utils::Streams::FileStream EgluStream;
    EgluStream.open("installdata.eglu", "wb");
    Utils::Streams::FileStream JsonStream;
    JsonStream.open("installdata.json", "wb");
    Backend::Packer inst(R"(J:\Code\Visual Studio 2017\Projects\EGL3\out\install\x64-Release)", VersionInfo, EgluStream, JsonStream);
}