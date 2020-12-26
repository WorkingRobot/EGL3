#pragma once

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EGL3::Web::Xmpp::Json {
	struct PresenceProperties {
		PresenceProperties() = default;

		PresenceProperties(const PresenceProperties& that) {
			Properties.CopyFrom(that.Properties, Properties.GetAllocator(), true);
		}

		PresenceProperties& operator=(const PresenceProperties& that) {
			Properties.CopyFrom(that.Properties, Properties.GetAllocator(), true);
			return *this;
		}

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
		void SetValue(const std::string& Name, int32_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_i");
		}

		void SetValue(const std::string& Name, uint32_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_u");
		}

		void SetValue(const std::string& Name, float Value) {
			SetValue(Name, rapidjson::Value(Value), "_f");
		}

		void SetValue(const std::string& Name, const std::string& Value) {
			SetValue(Name, rapidjson::Value(Value.c_str(), Value.size(), Properties.GetAllocator()), "_s");
		}

		void SetValue(const std::string& Name, const rapidjson::StringBuffer& Value) {
			SetValue(Name, rapidjson::Value(Value.GetString(), Value.GetLength(), Properties.GetAllocator()), "_s");
		}

		void SetValue(const std::string& Name, bool Value) {
			SetValue(Name, rapidjson::Value(Value), "_b");
		}

		void SetValue(const std::string& Name, int64_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_I");
		}

		void SetValue(const std::string& Name, uint64_t Value) {
			SetValue(Name, rapidjson::Value(Value), "_U");
		}

		void SetValue(const std::string& Name, double Value) {
			SetValue(Name, rapidjson::Value(Value), "_d");
		}

		void SetValue(const std::string& Name, const JsonObject& Value) {
			JsonObject ValueCopy;
			ValueCopy.CopyFrom(Value, ValueCopy.GetAllocator());
			SetValue(Name, std::move(ValueCopy), "_j");
		}

		template<typename Serializable>
		void SetValue(const std::string& Name, const Serializable& Value) {
			SetValue(Name, Value.ToProperty());
		}


		bool GetValue(const std::string& Name, int32_t& Value) const {
			return GetValue(Name, Value, "_i");
		}

		bool GetValue(const std::string& Name, uint32_t& Value) const {
			return GetValue(Name, Value, "_u");
		}

		bool GetValue(const std::string& Name, float& Value) const {
			return GetValue(Name, Value, "_f");
		}

		bool GetValue(const std::string& Name, std::string& Value) const {
			return GetValue(Name, Value, "_s");
		}

		bool GetValue(const std::string& Name, bool& Value) const {
			return GetValue(Name, Value, "_b");
		}

		bool GetValue(const std::string& Name, int64_t& Value) const {
			return GetValue(Name, Value, "_I");
		}

		bool GetValue(const std::string& Name, uint64_t& Value) const {
			return GetValue(Name, Value, "_U");
		}

		bool GetValue(const std::string& Name, double& Value) const {
			return GetValue(Name, Value, "_d");
		}

		bool GetValue(const std::string& Name, JsonObject& Value) const {
			return GetValue(Name, Value, "_j");
		}

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
