#pragma once

#include "Key.h"

namespace EGL3::Storage::Persistent {
	class Store {
	public:
		Store(const char* FilePath);

		~Store();

		template<class T>
		void Store(Key Key, T Data);
	};
}
