#pragma once

#include "../../utils/streams/FileStream.h"
#include "../../utils/streams/MemoryStream.h"
#include "Key.h"

#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace EGL3::Storage::Persistent {
	namespace fs = std::filesystem;

	class Store {
	public:
		Store(const fs::path& Path);

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
			EGL3_CONDITIONAL_LOG(Elem.second, LogLevel::Error, "Could not emplace nor find new constant in store.");
			return Elem.first->second;
		}

		void Flush();

	private:
		std::mutex Mutex;
		std::unordered_map<uint32_t, Key> Data;

		fs::path Path;
	};
}
