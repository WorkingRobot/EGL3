#pragma once

#include <string>

class StringEx {
public:
	static bool Evaluate(std::string&& input, std::string&& expression);
};