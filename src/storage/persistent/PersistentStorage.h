#pragma once

#include "DataKey.h"

namespace EGL3::Storage::Persistent {
	class PersistentStorage {
	public:
		PersistentStorage(const char* FilePath);

		~PersistentStorage();

		template<class T>
		void StoreData(DataKey Key, T Data);
	};
}
