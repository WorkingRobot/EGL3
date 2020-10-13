#include "Store.h"

namespace EGL3::Storage::Persistent {
	Store::Store(const fs::path& Path) : Path(Path) {
		if (fs::is_regular_file(Path)) {
			Utils::Streams::FileStream Stream;
			Stream.open(Path, "rb+");
			EGL3_ASSERT(Stream.valid(), "Could not open store file");
			uint32_t ElementCount;
			Stream >> ElementCount;
			for (int i = 0; i < ElementCount; ++i) {
				uint32_t KeyConstant;
				Stream >> KeyConstant;

				auto Elem = Data.emplace(KeyConstant, KeyConstant);
				if (Elem.first->second.Item) {
					EGL3_ASSERT(Elem.second, "Could not emplace new constant to store");
					Elem.first->second.Deserialize(Stream);
				}
				else { // Constant is not defined
					Data.erase(KeyConstant);
				}
			}
		}
	}

	Store::~Store() {
		Flush();
	}

	void Store::Flush() {
		Utils::Streams::FileStream Stream;
		Stream.open(Path, "wb+");
		
		Stream << (uint32_t)Data.size();
		for (auto& Key : Data) {
			Stream << Key.first;
			Key.second.Serialize(Stream);
		}
	}
}