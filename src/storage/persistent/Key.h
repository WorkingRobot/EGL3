#pragma once

#include "../../utils/Assert.h"
#include "../../utils/Crc32.h"
#include "../models/Authorization.h"
#include "../models/InstalledGame.h"
#include "../models/StoredFriendData.h"

#include <memory>

namespace EGL3::Storage::Persistent {
    struct IKeyValue {
        virtual void Serialize(Utils::Streams::Stream& Stream) const = 0;
        virtual void Deserialize(Utils::Streams::Stream& Stream) = 0;
        virtual constexpr uint32_t GetConstant() const = 0;
        virtual void* Get() const = 0;
    };

    template<uint32_t ConstantTemplate, class T>
    struct KeyType {
        static inline constexpr uint32_t Constant = ConstantTemplate;
        using ValueType = T;
    };

    template<uint32_t Constant, class T>
    struct KeyContainer : KeyType<Constant, T>, IKeyValue {
        void Serialize(Utils::Streams::Stream& Stream) const override final {
            Stream << Value;
        }

        void Deserialize(Utils::Streams::Stream& Stream) override final {
            Stream >> Value;
        }

        constexpr uint32_t GetConstant() const override final {
            return Constant;
        }

        void* Get() const override final {
            return (void*)&Value;
        }

        KeyType<Constant, T>::ValueType Value;
    };

    class Key {
        std::unique_ptr<IKeyValue> Item;

    public:
        // The VA_ARGS are used because macros notice the , in templates and this is the least complicated way of solving this issue
#define KEY(Name, ...) static inline constexpr KeyType<Utils::Crc32(#Name), __VA_ARGS__> Name{};

        KEY(WhatsNewTimestamps, std::unordered_map<size_t, std::chrono::system_clock::time_point>);
        KEY(WhatsNewSelection,  uint8_t);
        KEY(Auth,               Models::Authorization);
        KEY(StoredFriendData,   Models::StoredFriendData);
        KEY(InstalledGames,     std::vector<Models::InstalledGame>);
        KEY(UpdateFrequency,    std::chrono::seconds);

#undef KEY

        Key(uint32_t Constant) {
            switch (Constant)
            {
#define KEY(Name) case Name.Constant: static_assert(Name.Constant); Item = std::make_unique<KeyContainer<Name.Constant, decltype(Name)::ValueType>>(); break;
                
                KEY(WhatsNewTimestamps);
                KEY(WhatsNewSelection);
                KEY(Auth);
                KEY(StoredFriendData);
                KEY(InstalledGames);
                KEY(UpdateFrequency);

#undef KEY
            }
        }

        Key(Key&&) = delete;

        void Serialize(Utils::Streams::Stream& Stream) const;

        void Deserialize(Utils::Streams::Stream& Stream);

        bool HasValue() const;

        template<uint32_t Constant, class T>
        T& Get(const KeyType<Constant, T>& KeyType) const {
            EGL3_CONDITIONAL_LOG(Item->GetConstant() == Constant, LogLevel::Critical, "Tried to get a mismatched key, constants don't match");
            return std::ref<T>(*(T*)Item->Get());
        }
    };
}
