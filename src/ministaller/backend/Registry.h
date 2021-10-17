#pragma once

#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Installer::Backend {
    class Registry {
    public:
        enum class Value : uint8_t {
            DisplayName,
            UninstallString,
            InstallLocation,
            DisplayIcon,
            Publisher,
            HelpLink,
            URLUpdateInfo,
            URLInfoAbout,
            DisplayVersion,
            EstimatedSize,

            Count
        };

    public:
        template<Value Key>
        struct ValueType {
            static constexpr Value Key = Key;
            using T = char[260];
        };

        template<>
        struct ValueType<Value::EstimatedSize> {
            static constexpr Value Key = Value::EstimatedSize;
            using T = DWORD;
        };

        template<Value Value>
        using ValueT = ValueType<Value>::T;

        Registry();

        ~Registry();

        bool Get(Value Value, char Out[260]) const;

        bool Get(Value Value, DWORD& Out) const;

        bool Set(Value Value, const char In[260]);

        bool Set(Value Value, DWORD In);

    private:
        bool GetKey(HKEY& Key) const;

        bool GetOrCreateKey(HKEY& Key);

        mutable HKEY Handle;
        mutable bool IsHandlePopulated;
    };
}