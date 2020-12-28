#include "PresenceProperties.h"

#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
    PresenceProperties::PresenceProperties(const PresenceProperties& that) {
        Properties.CopyFrom(that.Properties, Properties.GetAllocator(), true);
    }

    PresenceProperties& PresenceProperties::operator=(const PresenceProperties& that) {
        Properties.CopyFrom(that.Properties, Properties.GetAllocator(), true);
        return *this;
    }

    void PresenceProperties::SetValue(const std::string& Name, int32_t Value) {
        SetValue(Name, rapidjson::Value(Value), "_i");
    }

    void PresenceProperties::SetValue(const std::string& Name, uint32_t Value) {
        SetValue(Name, rapidjson::Value(Value), "_u");
    }

    void PresenceProperties::SetValue(const std::string& Name, float Value) {
        SetValue(Name, rapidjson::Value(Value), "_f");
    }

    void PresenceProperties::SetValue(const std::string& Name, const std::string& Value) {
        SetValue(Name, rapidjson::Value(Value.c_str(), Value.size(), Properties.GetAllocator()), "_s");
    }

    void PresenceProperties::SetValue(const std::string& Name, const rapidjson::StringBuffer& Value) {
        SetValue(Name, rapidjson::Value(Value.GetString(), Value.GetLength(), Properties.GetAllocator()), "_s");
    }

    void PresenceProperties::SetValue(const std::string& Name, bool Value) {
        SetValue(Name, rapidjson::Value(Value), "_b");
    }

    void PresenceProperties::SetValue(const std::string& Name, int64_t Value) {
        SetValue(Name, rapidjson::Value(Value), "_I");
    }

    void PresenceProperties::SetValue(const std::string& Name, uint64_t Value) {
        SetValue(Name, rapidjson::Value(Value), "_U");
    }

    void PresenceProperties::SetValue(const std::string& Name, double Value) {
        SetValue(Name, rapidjson::Value(Value), "_d");
    }

    void PresenceProperties::SetValue(const std::string& Name, const JsonObject& Value) {
        JsonObject ValueCopy;
        ValueCopy.CopyFrom(Value, ValueCopy.GetAllocator());
        SetValue(Name, std::move(ValueCopy), "_j");
    }


    bool PresenceProperties::GetValue(const std::string& Name, int32_t& Value) const {
        return GetValue(Name, Value, "_i");
    }

    bool PresenceProperties::GetValue(const std::string& Name, uint32_t& Value) const {
        return GetValue(Name, Value, "_u");
    }

    bool PresenceProperties::GetValue(const std::string& Name, float& Value) const {
        return GetValue(Name, Value, "_f");
    }

    bool PresenceProperties::GetValue(const std::string& Name, std::string& Value) const {
        return GetValue(Name, Value, "_s");
    }

    bool PresenceProperties::GetValue(const std::string& Name, bool& Value) const {
        return GetValue(Name, Value, "_b");
    }

    bool PresenceProperties::GetValue(const std::string& Name, int64_t& Value) const {
        return GetValue(Name, Value, "_I");
    }

    bool PresenceProperties::GetValue(const std::string& Name, uint64_t& Value) const {
        return GetValue(Name, Value, "_U");
    }

    bool PresenceProperties::GetValue(const std::string& Name, double& Value) const {
        return GetValue(Name, Value, "_d");
    }

    bool PresenceProperties::GetValue(const std::string& Name, JsonObject& Value) const {
        return GetValue(Name, Value, "_j");
    }
}
