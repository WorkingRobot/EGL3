#include "Base64.h"

#include "base64/decode.h"
#include "base64/encode.h"

#include <sstream>

namespace EGL3::Utils {
	std::string B64Decode(const std::string& Input) {
		std::ostringstream Output;
		base64::decoder().decode(std::istringstream(Input), Output);
		return Output.str();
	}

	std::string B64Encode(const std::string& Input) {
		std::ostringstream Output;
		base64::encoder().encode(std::istringstream(Input), Output);
		auto str = Output.str();
		str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
		return str;
	}

	std::string B64Decode(const uint8_t* Input, size_t InputSize) {
		return B64Decode(std::string((char*)Input, InputSize));
	}

	std::string B64Encode(const uint8_t* Input, size_t InputSize) {
		return B64Encode(std::string((char*)Input, InputSize));
	}
}