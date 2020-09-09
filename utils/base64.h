#pragma once

#include "base64/decode.h"
#include "base64/encode.h"

#include <sstream>
#include <string>

// maybe use something like https://github.com/aklomp/base64 later on, but i don't
// really need speed for something used like twice

static std::string B64Decode(const std::string& Input) {
	std::ostringstream Output;
	base64::decoder().decode(std::istringstream(Input), Output);
	return Output.str();
}

static std::string B64Encode(const std::string& Input) {
	std::ostringstream Output;
	base64::encoder().encode(std::istringstream(Input), Output);
	auto str = Output.str();
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
	return str;
}

static std::string B64Decode(const uint8_t* Input, uint32_t InputSize) {
	return B64Decode(std::string((char*)Input, InputSize));
}

static std::string B64Encode(const uint8_t* Input, uint32_t InputSize) {
	return B64Encode(std::string((char*)Input, InputSize));
}
