#include "Encrypt.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dpapi.h>

#include "Log.h"

namespace EGL3::Utils {
    // Split into a seperate file so including this header doesn't include windows.h
    std::unique_ptr<char[], std::function<void(char*)>> Encrypt(const char* Input, size_t InputSize, size_t& OutputSize)
    {
        DATA_BLOB DataIn, DataOut;
        DataIn.cbData = InputSize;
        DataIn.pbData = (BYTE*)Input;

        if (EGL3_ENSURE(CryptProtectData(&DataIn, NULL, NULL, NULL, NULL, 0, &DataOut), LogLevel::Error, "Could not encrypt data")) {
            OutputSize = DataOut.cbData;
            return std::unique_ptr<char[], std::function<void(char*)>>((char*)DataOut.pbData, LocalFree);
        }
        else {
            OutputSize = 0;
            return nullptr;
        }
    }

    std::unique_ptr<char[], std::function<void(char*)>> Decrypt(const char* Input, size_t InputSize, size_t& OutputSize)
    {
        DATA_BLOB DataIn, DataOut;
        DataIn.cbData = InputSize;
        DataIn.pbData = (BYTE*)Input;

        if (EGL3_ENSURE(CryptUnprotectData(&DataIn, NULL, NULL, NULL, NULL, 0, &DataOut), LogLevel::Error, "Could not decrypt data")) {
            OutputSize = DataOut.cbData;
            return std::unique_ptr<char[], std::function<void(char*)>>((char*)DataOut.pbData, LocalFree);
        }
        else {
            OutputSize = 0;
            return nullptr;
        }
    }
}
