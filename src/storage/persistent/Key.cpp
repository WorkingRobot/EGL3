#include "Key.h"

namespace EGL3::Storage::Persistent {
	void Key::Serialize(Utils::Streams::Stream& Stream) const {
		Item->Serialize(Stream);
	}

	void Key::Deserialize(Utils::Streams::Stream& Stream) {
		Item->Deserialize(Stream);
	}

	bool Key::HasValue() const {
		return (bool)Item;
	}
}
