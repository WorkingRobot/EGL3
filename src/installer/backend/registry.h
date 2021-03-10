#pragma once

#include "consts.h"

#include <filesystem>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winreg.h>

namespace fs = std::filesystem;

struct ProgramRegistryInfo {
    std::string DisplayIcon; // exe or ico
    std::string DisplayName; // program name
    std::string DisplayVersion; // doesn't need to be semantic (1.0.0.0)
    std::string Publisher;
    std::string HelpLink; // "Help link"
    std::string InstallDate; // yyyymmdd
    std::string InstallLocation; // path to install folder
    std::string URLInfoAbout; // "Support link"
    std::string URLUpdateInfo; // "Update information"
    int EstimatedSize; // in kb
    int NoModify; // "Change" button
    std::string ModifyPath;
    int NoRepair; // "Repair" button
    std::string UninstallString;
};

fs::path registry_install_location() {
    return fs::path(getenv("PROGRAMFILES")).append(DOWNLOAD_REPO);
}

bool registry_installed() {
    HKEY key;
    if (!RegOpenKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, 0, KEY_ALL_ACCESS, &key)) {
        RegCloseKey(key);
        return true;
    }
    return false;
}

bool registry_uninstall() {
    if (!registry_installed()) {
        return false;
    }
    return RegDeleteKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, KEY_WOW64_64KEY, 0);
}

std::string registry_datetime() {
    time_t t;
    time(&t);
    tm* tmp = localtime(&t);
    char buffer[9];
    strftime(buffer, 9, "%Y%m%d", tmp);
    return std::string(buffer, 9);
}

bool registry_install(ProgramRegistryInfo* data) {
    if (registry_installed()) {
        return false;
    }
    HKEY key;
    DWORD disposition;
    if (int st = RegCreateKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &disposition)) {
        return false;
    }
#define SET_KEY_STRING(field) RegSetValueExA(key, #field, 0, REG_SZ, (BYTE*)data->##field.c_str(), data->##field.size())
#define SET_KEY_DWORD(field) RegSetValueExA(key, #field, 0, REG_DWORD, (BYTE*)&data->##field, 4)

    SET_KEY_STRING(DisplayIcon);
    SET_KEY_STRING(DisplayName);
    SET_KEY_STRING(DisplayVersion);
    SET_KEY_STRING(Publisher);
    SET_KEY_STRING(HelpLink);
    SET_KEY_STRING(InstallDate);
    SET_KEY_STRING(InstallLocation);
    SET_KEY_STRING(URLInfoAbout);
    SET_KEY_STRING(URLUpdateInfo);
    SET_KEY_STRING(ModifyPath);
    SET_KEY_STRING(UninstallString);
    SET_KEY_DWORD(EstimatedSize);
    SET_KEY_DWORD(NoModify);
    SET_KEY_DWORD(NoRepair);

#undef SET_KEY_STRING
#undef SET_KEY_DWORD
    RegCloseKey(key);
    return true;
}