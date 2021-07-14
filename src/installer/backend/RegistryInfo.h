#pragma once

#include "../../utils/streams/Stream.h"

#include <filesystem>

namespace EGL3::Installer::Backend {
    struct RegistryInfo {
        std::string ProductGuid;
        std::string LaunchExe;

        // std::string AuthorizedCDFPrefix;
        // std::string Comments;
        // std::string Contact;
        std::string DisplayIcon;
        std::string DisplayName;
        std::string DisplayVersion;
        uint32_t EstimatedSize;
        std::string HelpLink;
        // std::string HelpTelephone;
        // std::string InstallDate;
        // std::string InstallLocation;
        // std::string InstallSource;
        // uint32_t Language;
        // std::string ModifyPath;
        // uint32_t NoModify;
        // uint32_t NoRepair;
        std::string Publisher;
        // std::string Readme;
        // std::string Size;
        // uint32_t SystemComponent;
        std::string UninstallString;
        std::string URLInfoAbout;
        std::string URLUpdateInfo;
        // uint32_t Version;
        uint32_t VersionMajor;
        uint32_t VersionMinor;

        bool IsInstalled() const;

        bool Uninstall() const;

        bool Install(const std::filesystem::path& InstallDirectory) const;

        bool IsShortcutted() const;

        bool Unshortcut() const;

        bool Shortcut(const std::filesystem::path& InstallDirectory, std::filesystem::path& LaunchPath) const;

    private:
        std::string GetSubKey() const;

        std::filesystem::path GetShortcutPath() const;
    };
}