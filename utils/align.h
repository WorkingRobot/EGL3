#pragma once

template<int Alignment, class N>
static constexpr N Align(N Value) {
	return Value + (-Value & (Alignment - 1));
}