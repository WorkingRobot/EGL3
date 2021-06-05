#pragma once

#include "../../utils/streams/FileStream.h"
#include "../../utils/streams/MemoryStream.h"
#include "Key.h"

#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace EGL3::Storage::Persistent {
    class Store {
    public:
        Store(const std::filesystem::path& Path);

        ~Store();

        template<uint32_t Constant, class T>
        T& Get(const KeyType<Constant, T>& Key) {
            return Get<Constant>().Get(Key);
        }

        template<uint32_t Constant>
        Key& Get() {
            std::lock_guard Guard(Mutex);

            auto Itr = Data.find(Constant);
            if (Itr != Data.end()) {
                return Itr->second;
            }

            auto Elem = Data.emplace(Constant, Constant);
            EGL3_ENSURE(Elem.second, LogLevel::Error, "Could not emplace nor find new constant in store.");
            return Elem.first->second;
        }

        void Flush();

    private:
        std::mutex Mutex;
        std::unordered_map<uint32_t, Key> Data;

        std::filesystem::path Path;
    };
}
