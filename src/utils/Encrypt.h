#pragma once

#include <functional>
#include <memory>

namespace EGL3::Utils {
    std::unique_ptr<char[], std::function<void(char*)>> Encrypt(const char* Input, size_t InputSize, size_t& OutputSize);

    std::unique_ptr<char[], std::function<void(char*)>> Decrypt(const char* Input, size_t InputSize, size_t& OutputSize);
}