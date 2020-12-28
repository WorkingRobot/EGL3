#pragma once

#include "../JsonParsing.h"

#include <rapidjson/stringbuffer.h>

namespace EGL3::Web::Xmpp::Json {
    struct PresenceProperties {
        PresenceProperties() = default;

        PresenceProperties(const PresenceProperties& that);

        PresenceProperties& operator=(const PresenceProperties& that);

    private:
        JsonObject Properties;

        template<size_t SuffixSize>
        void SetValue(const std::string& Key, rapidjson::Value&& Value, const char(&Suffix)[SuffixSize]) {

            if (Properties.IsNull()) {
                Properties.SetObject();
            }

            auto KeyPtr = (char*)Properties.GetAllocator().Malloc(Key.size() + SuffixSize - 1);
            memcpy(KeyPtr, Key.c_str(), Key.size());
            memcpy(KeyPtr + Key.size(), Suffix, SuffixSize - 1);

            Properties.EraseMember(std::move(rapidjson::Value(KeyPtr, Key.size() + SuffixSize - 1)));
            Properties.AddMember(std::move(rapidjson::Value(KeyPtr, Key.size() + SuffixSize - 1)), std::move(Value), Properties.GetAllocator());
        }

        template<typename T, size_t SuffixSize>
        bool GetValue(const std::string& Key, T& Value, const char(&Suffix)[SuffixSize]) const {
            if (!Properties.IsObject()) {
                return false;
            }
            auto KeySuffixed = Key + Suffix;
            auto Itr = Properties.FindMember(KeySuffixed.c_str());
            if (Itr != Properties.MemberEnd()) {
                if constexpr (std::is_base_of<rapidjson::Value, T>::value) {
                    Value.CopyFrom(Itr->value, Value.GetAllocator(), true);
                }
                else if constexpr (std::is_base_of<std::string, T>::value) {
                    Value = std::string(Itr->value.GetString(), Itr->value.GetStringLength());
                }
                else {
                    Value = Itr->value.Get<T>();
                }
                return true;
            }
            return false;
        }

    public:
        void SetValue(const std::string& Name, int32_t Value);

        void SetValue(const std::string& Name, uint32_t Value);

        void SetValue(const std::string& Name, float Value);

        void SetValue(const std::string& Name, const std::string& Value);

        void SetValue(const std::string& Name, const rapidjson::StringBuffer& Value);

        void SetValue(const std::string& Name, bool Value);

        void SetValue(const std::string& Name, int64_t Value);

        void SetValue(const std::string& Name, uint64_t Value);

        void SetValue(const std::string& Name, double Value);

        void SetValue(const std::string& Name, const JsonObject& Value);

        template<typename Serializable>
        void SetValue(const std::string& Name, const Serializable& Value) {
            SetValue(Name, Value.ToProperty());
        }


        bool GetValue(const std::string& Name, int32_t& Value) const;

        bool GetValue(const std::string& Name, uint32_t& Value) const;

        bool GetValue(const std::string& Name, float& Value) const;

        bool GetValue(const std::string& Name, std::string& Value) const;

        bool GetValue(const std::string& Name, bool& Value) const;

        bool GetValue(const std::string& Name, int64_t& Value) const;

        bool GetValue(const std::string& Name, uint64_t& Value) const;

        bool GetValue(const std::string& Name, double& Value) const;

        bool GetValue(const std::string& Name, JsonObject& Value) const;

        template<typename RapidJsonWriter>
        void WriteTo(RapidJsonWriter& Writer) const {
            if (Properties.IsObject()) {
                Properties.Accept(Writer);
            }
            else {
                Writer.StartObject();
                Writer.EndObject();
            }
        }

        PARSE_DEFINE(PresenceProperties)
            PARSE_ITEM_ROOT(Properties)
        PARSE_END
    };
}
