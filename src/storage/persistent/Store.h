#pragma once

#include "../../utils/streams/MemoryStream.h"
#include "../../utils/Crc32.h"
#include "../../utils/Log.h"

#include <any>
#include <filesystem>
#include <unordered_map>

namespace EGL3::Storage::Persistent {
    namespace Detail {
        struct TemporaryElement : public Utils::Streams::MemoryStream {
            using Utils::Streams::MemoryStream::MemoryStream;

            TemporaryElement(const TemporaryElement&) = delete;
            TemporaryElement(TemporaryElement&&) = delete;
        };

        template<class T>
        struct WrappedFakeCopyable {
            template<class... ArgTs>
            WrappedFakeCopyable(ArgTs&&... Args) :
                Data(std::forward<ArgTs>(Args)...)
            {

            }

            WrappedFakeCopyable(const WrappedFakeCopyable&)
            {
                EGL3_ABORT("Attempting to copy fake copyable");
            }

            WrappedFakeCopyable(WrappedFakeCopyable&&) = default;

            T Data;
        };

        struct MoveableAny {
            MoveableAny() = default;
            MoveableAny(const MoveableAny&) = delete;
            MoveableAny(MoveableAny&&) = default;

            template <class T, class... ArgTs>
            explicit MoveableAny(std::in_place_type_t<T>, ArgTs&&... Args) :
                Data(std::in_place_type<WrappedFakeCopyable<T>>, std::forward<ArgTs>(Args)...)
            {

            }

            template<class T, class... ArgTs>
            T& Emplace(ArgTs&&... Args)
            {
                return Data.emplace<WrappedFakeCopyable<T>>(std::forward<ArgTs>(Args)...).Data;
            }

            template<class T, class... ArgTs>
            T& Emplace(ArgTs&&... Args) const
            {
                return Data.emplace<WrappedFakeCopyable<T>>(std::forward<ArgTs>(Args)...).Data;
            }

            void Swap(MoveableAny& Other) noexcept {
                Data.swap(Other.Data);
            }

            template<class T>
            T* TryGet()
            {
                auto Ret = std::any_cast<WrappedFakeCopyable<T>>(&Data);
                return Ret ? &Ret->Data : nullptr;
            }

            template<class T>
            const T* TryGet() const
            {
                auto Ret = std::any_cast<WrappedFakeCopyable<T>>(&Data);
                return Ret ? &Ret->Data : nullptr;
            }

            template<class T>
            T& Get()
            {
                return *TryGet<T>();
            }

            template<class T>
            const T& Get() const
            {
                return *TryGet<T>();
            }

        private:
            std::any Data;
        };

        template <class T, class... ArgTs>
        [[nodiscard]] auto MakeAny(ArgTs&&... Args) {
            return MoveableAny{ std::in_place_type<T>, std::forward<ArgTs>(Args)... };
        }

        template <class _Setting, class = void>
        inline constexpr bool _IsSettingV = false;

        template <class _Setting>
        inline constexpr bool _IsSettingV<_Setting, std::void_t<typename _Setting::Type, decltype(_Setting::KeyHash), decltype(&_Setting::Serializer)>> = true;

        template <class _Setting>
        struct is_setting : std::bool_constant<_IsSettingV<_Setting>> {};
        template <class _Setting>
        inline constexpr bool is_setting_v = _IsSettingV<_Setting>;
    }

    template<uint32_t Hash, class T>
    struct Setting {
        static constexpr uint32_t KeyHash = Hash;
        using Type = T;

        static void Serializer(Utils::Streams::Stream& Stream, const Detail::MoveableAny& Value) {
            Stream << Value.Get<Type>();
        }
    };

    class Store;

    template<class _Setting>
    struct SettingHolder {
        using Setting = _Setting;

        SettingHolder(Store& Store, typename _Setting::Type& Data) :
            Store(Store),
            Data(Data)
        {

        }
        
        SettingHolder& operator=(typename const _Setting::Type& Data)
        {
            this->Data = Data;
            return *this;
        }

        SettingHolder& operator=(typename _Setting::Type&& Data)
        {
            this->Data = std::move(Data);
            return *this;
        }

        typename _Setting::Type& operator*() noexcept {
            return Data;
        }

        typename const _Setting::Type& operator*() const noexcept {
            return Data;
        }

        typename _Setting::Type* operator->() noexcept {
            return &Data;
        }

        typename const _Setting::Type* operator->() const noexcept {
            return &Data;
        }

        void Flush();

    private:
        Store& Store;
        _Setting::Type& Data;
    };

    class Store {
        using Serializer = void (*)(Utils::Streams::Stream&, const Detail::MoveableAny&);

    public:
        Store(const std::filesystem::path& Path);
        Store(const Store&) = delete;
        Store(Store&&) = delete;

        ~Store();

        template<class _Setting, std::enable_if_t<Detail::is_setting_v<_Setting>, bool> = true>
        typename SettingHolder<_Setting> Get()
        {
            auto Itr = Elements.find(_Setting::KeyHash);
            if (Itr == Elements.end()) {
                // Element doesn't exist
                Itr = Elements.emplace(_Setting::KeyHash, std::make_pair<Detail::MoveableAny, Serializer>(Detail::MakeAny<_Setting::Type>(), &_Setting::Serializer)).first;
            }
            
            auto Ret = Itr->second.first.TryGet<_Setting::Type>();
            if (Ret) {
                // Element exists
                return { *this, *Ret };
            }

            // Element exists serialized
            auto ParsedElement = Detail::MakeAny<_Setting::Type>();
            Itr->second.first.Get<Detail::TemporaryElement>() >> ParsedElement.Get<_Setting::Type>();
            Itr->second.first.Swap(ParsedElement);
            Itr->second.second = &_Setting::Serializer;
            return { *this, Itr->second.first.Get<_Setting::Type>() };
        }

        void Flush();

    private:
        void WriteTo(Utils::Streams::Stream& Stream);

        std::filesystem::path Path;
        std::unordered_map<uint32_t, std::pair<Detail::MoveableAny, Serializer>> Elements;
    };

    template<class _Setting>
    void SettingHolder<_Setting>::Flush()
    {
        Store.Flush();
    }
}