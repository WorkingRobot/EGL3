#pragma once

namespace EGL3::Utils {
	template<int Alignment, class N>
	static constexpr N Align(N Value) {
		return Value + (-Value & (Alignment - 1));
	}
}
