#pragma once

#include <string>

// maybe use something like https://github.com/aklomp/base64 later on, but i don't
// really need speed for something used like twice

namespace EGL3::Utils {
	std::string B64Decode(const std::string& Input);

	std::string B64Encode(const std::string& Input);

	std::string B64Decode(const uint8_t* Input, uint32_t InputSize);

	std::string B64Encode(const uint8_t* Input, uint32_t InputSize);
}