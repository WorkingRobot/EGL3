#include "RegistryInfo.h"

#include "../../utils/KnownFolders.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShObjIdl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <ShlObj_core.h>

namespace EGL3::Installer::Backend {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, RegistryInfo& Val)
    {
        Stream >> Val.ProductGuid;
        Stream >> Val.DisplayIcon;
        Stream >> Val.DisplayName;
        Stream >> Val.DisplayVersion;
        Stream >> Val.EstimatedSize;
        Stream >> Val.HelpLink;
        Stream >> Val.Publisher;
        Stream >> Val.UninstallString;
        Stream >> Val.URLInfoAbout;
        Stream >> Val.URLUpdateInfo;
        Stream >> Val.VersionMajor;
        Stream >> Val.VersionMinor;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const RegistryInfo& Val)
    {
        Stream << Val.ProductGuid;
        Stream << Val.DisplayIcon;
        Stream << Val.DisplayName;
        Stream << Val.DisplayVersion;
        Stream << Val.EstimatedSize;
        Stream << Val.HelpLink;
        Stream << Val.Publisher;
        Stream << Val.UninstallString;
        Stream << Val.URLInfoAbout;
        Stream << Val.URLUpdateInfo;
        Stream << Val.VersionMajor;
        Stream << Val.VersionMinor;

        return Stream;
    }

    bool RegistryInfo::IsInstalled() const
    {
        HKEY key;
        if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, GetSubKey().c_str(), 0, KEY_ALL_ACCESS, &key)) {
            RegCloseKey(key);
            return true;
        }
        return false;
    }

    bool RegistryInfo::Uninstall() const
    {
        if (!IsInstalled()) {
            return false;
        }

        return !RegDeleteKeyEx(HKEY_LOCAL_MACHINE, GetSubKey().c_str(), KEY_WOW64_64KEY, 0);
    }

    bool SetRegistryString(HKEY Key, const char* Name, const std::string& Data) {
        return !RegSetValueEx(Key, Name, 0, REG_SZ, (BYTE*)Data.c_str(), Data.size());
    }

    bool SetRegistryDWord(HKEY Key, const char* Name, DWORD Data) {
        return !RegSetValueEx(Key, Name, 0, REG_DWORD, (BYTE*)&Data, sizeof(DWORD));
    }

    bool RegistryInfo::Install(const std::filesystem::path& InstallDirectory) const
    {
        if (IsInstalled()) {
            return false;
        }

        HKEY Key;
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, GetSubKey().c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &Key, NULL)) {
            return false;
        }

        bool Successful = SetRegistryString(Key, "DisplayIcon", (InstallDirectory / DisplayIcon).string());

        Successful = Successful && SetRegistryString(Key, "DisplayName", DisplayName);

        Successful = Successful && SetRegistryString(Key, "DisplayVersion", DisplayVersion);

        Successful = Successful && SetRegistryDWord(Key, "EstimatedSize", EstimatedSize);

        Successful = Successful && SetRegistryString(Key, "HelpLink", HelpLink);

        Successful = Successful && SetRegistryString(Key, "Publisher", Publisher);

        Successful = Successful && SetRegistryString(Key, "UninstallString", (InstallDirectory / UninstallString).string());

        Successful = Successful && SetRegistryString(Key, "URLInfoAbout", URLInfoAbout);

        Successful = Successful && SetRegistryString(Key, "URLUpdateInfo", URLUpdateInfo);

        Successful = Successful && SetRegistryDWord(Key, "VersionMajor", VersionMajor);

        Successful = Successful && SetRegistryDWord(Key, "VersionMinor", VersionMinor);

        RegCloseKey(Key);
        return Successful;
    }

    bool RegistryInfo::IsShortcutted() const
    {
        std::error_code Code;
        return std::filesystem::exists(GetShortcutPath(), Code);
    }

    bool RegistryInfo::Unshortcut() const
    {
        if (!IsShortcutted()) {
            return false;
        }

        std::error_code Code;
        return std::filesystem::remove(GetShortcutPath(), Code);
    }

    bool RegistryInfo::Shortcut(const std::filesystem::path& InstallDirectory, std::filesystem::path& LaunchPath) const
    {
        if (IsShortcutted()) {
            return false;
        }

        HRESULT Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (Result == CO_E_NOTINITIALIZED) {
            return false;
        }

        IShellLink* ShellLink;
        Result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&ShellLink);
        if (FAILED(Result)) {
            CoUninitialize();
            return false;
        }

        ShellLink->SetPath((InstallDirectory / LaunchExe).string().c_str());
        ShellLink->SetArguments("");
        ShellLink->SetWorkingDirectory(InstallDirectory.string().c_str());

        IPropertyStore* PropertyStore;
        Result = ShellLink->QueryInterface(IID_IPropertyStore, (LPVOID*)&PropertyStore);
        if (FAILED(Result)) {
            ShellLink->Release();
            CoUninitialize();
            return false;
        }

        PROPVARIANT AumiPropVar;
        InitPropVariantFromString(std::filesystem::path(ProductGuid).c_str(), &AumiPropVar);
        PropertyStore->SetValue(PKEY_AppUserModel_ID, AumiPropVar);
        PropertyStore->Commit();
        PropertyStore->Release();

        IPersistFile* PersistFile;
        Result = ShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&PersistFile);
        if (FAILED(Result)) {
            PropVariantClear(&AumiPropVar);
            ShellLink->Release();
            CoUninitialize();
            return false;
        }

        Result = PersistFile->Save(GetShortcutPath().c_str(), TRUE);

        PersistFile->Release();
        PropVariantClear(&AumiPropVar);
        ShellLink->Release();
        CoUninitialize();

        if (FAILED(Result)) {
            return false;
        }

        LaunchPath = GetShortcutPath();
        return true;
    }

    std::string RegistryInfo::GetSubKey() const
    {
        return std::format("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{}", ProductGuid);
    }

    std::filesystem::path RegistryInfo::GetShortcutPath() const
    {
        return (Utils::Platform::GetKnownFolderPath(FOLDERID_CommonPrograms) / DisplayName).replace_extension("lnk");
    }
}