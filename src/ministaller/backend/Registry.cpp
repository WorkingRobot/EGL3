#include "Registry.h"

namespace EGL3::Installer::Backend {
    constexpr const char* SubKey = R"(Software\Microsoft\Windows\CurrentVersion\Uninstall\EGL3)";

    constexpr const char* GetValue(Registry::Value Value)
    {
        switch (Value)
        {
#define CASE(Name) case Registry::Value::Name: return #Name;

            CASE(DisplayName);
            CASE(UninstallString);
            CASE(InstallLocation);
            CASE(DisplayIcon);
            CASE(Publisher);
            CASE(HelpLink);
            CASE(URLUpdateInfo);
            CASE(URLInfoAbout);
            CASE(DisplayVersion);
            CASE(VersionMajor);
            CASE(VersionMinor);
            CASE(EstimatedSize);

#undef CASE
        default:
            return "";
        }
    }

    Registry::Registry() :
        Handle(),
        IsHandlePopulated(false)
    {

    }

    Registry::~Registry()
    {
        if (IsHandlePopulated) {
            RegCloseKey(Handle);
        }
    }

    bool Registry::Get(Value Value, char Out[260]) const
    {
        HKEY Key;
        if (!GetKey(Key)) {
            return false;
        }

        DWORD OutSize = sizeof(Out);
        return RegGetValue(Key, NULL, GetValue(Value), RRF_RT_REG_SZ | RRF_ZEROONFAILURE, NULL, Out, &OutSize) == ERROR_SUCCESS;
    }

    bool Registry::Get(Value Value, DWORD& Out) const
    {
        HKEY Key;
        if (!GetKey(Key)) {
            return false;
        }

        DWORD OutSize = sizeof(Out);
        return RegGetValue(Key, NULL, GetValue(Value), RRF_RT_REG_DWORD | RRF_ZEROONFAILURE, NULL, &Out, &OutSize) == ERROR_SUCCESS;
    }

    bool Registry::Set(Value Value, const char In[260])
    {
        HKEY Key;
        if (!GetOrCreateKey(Key)) {
            return false;
        }

        return RegSetValueEx(Key, GetValue(Value), 0, REG_SZ, (const BYTE*)In, lstrlen(In)) == ERROR_SUCCESS;
    }

    bool Registry::Set(Value Value, DWORD In)
    {
        HKEY Key;
        if (!GetOrCreateKey(Key)) {
            return false;
        }

        return RegSetValueEx(Key, GetValue(Value), 0, REG_DWORD, (const BYTE*)&In, sizeof(In)) == ERROR_SUCCESS;
    }

    bool Registry::GetKey(HKEY& Key) const
    {
        if (!IsHandlePopulated) {
            IsHandlePopulated = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_ALL_ACCESS, &Handle) == ERROR_SUCCESS;
        }

        if (IsHandlePopulated) {
            Key = Handle;
            return true;
        }
        return false;
    }

    bool Registry::GetOrCreateKey(HKEY& Key)
    {
        if (!IsHandlePopulated) {
            IsHandlePopulated = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &Handle, NULL) == ERROR_SUCCESS;
        }

        if (IsHandlePopulated) {
            Key = Handle;
            return true;
        }
        return false;
    }
}