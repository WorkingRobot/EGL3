#pragma once

#include "../../utils/Assert.h"
#include "../../utils/Crc32.h"
#include "../../utils/streams/MemoryStream.h"
#include "../models/Authorization.h"
#include "../models/StoredFriendData.h"

namespace EGL3::Storage::Persistent {
	struct IKeyValue {
		virtual void Serialize(Utils::Streams::Stream& Stream) const = 0;
		virtual void Deserialize(Utils::Streams::Stream& Stream) = 0;
		virtual constexpr uint32_t GetConstant() const = 0;
		virtual void* Get() const = 0;
	};

	template<uint32_t Constant, class T>
	struct KeyType {
		static constexpr uint32_t Constant = Constant;
		using ValueType = T;
	};

	template<uint32_t Constant, class T>
	struct KeyContainer : KeyType<Constant, T>, IKeyValue {
		void Serialize(Utils::Streams::Stream& Stream) const final {
			Stream << Value;
		}

		void Deserialize(Utils::Streams::Stream& Stream) final {
			Stream >> Value;
		}

		constexpr uint32_t GetConstant() const final {
			return Constant;
		}

		void* Get() const final {
			return (void*)&Value;
		}

		ValueType Value;
	};

	struct Key {
		// The VA_ARGS are used because macros notice the , in templates and this is the least complicated way of solving this issue
#define KEY(Name, ...) static constexpr KeyType<Utils::Crc32(#Name), __VA_ARGS__> Name{};

		KEY(WhatsNewTimestamps, std::unordered_map<size_t, std::chrono::system_clock::time_point>);
		KEY(WhatsNewSelection,  uint8_t);
		KEY(Auth,				Models::Authorization);
		KEY(StoredFriendData,	Models::StoredFriendData);

#undef KEY

		Key(uint32_t Constant) {
			switch (Constant)
			{
#define KEY(Name) case Name.Constant: Item = std::make_unique<KeyContainer<Name.Constant, decltype(Name)::ValueType>>(); break;
				
				KEY(WhatsNewTimestamps);
				KEY(WhatsNewSelection);
				KEY(Auth);
				KEY(StoredFriendData);

#undef KEY
			}
		}

		Key(Key&&) = delete;

		void Serialize(Utils::Streams::Stream& Stream) const {
			Item->Serialize(Stream);
		}

		void Deserialize(Utils::Streams::Stream& Stream) {
			Item->Deserialize(Stream);
		}

		template<uint32_t Constant, class T>
		T& Get(const KeyType<Constant, T>& KeyType) const {
			EGL3_ASSERT(Item->GetConstant() == Constant, "Tried to get a mismatched key, constants don't match");
			return std::ref<T>(*(T*)Item->Get());
		}

		std::unique_ptr<IKeyValue> Item;
	};
}
