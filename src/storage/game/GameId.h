#pragma once

#include "../../utils/Crc32.h"

namespace EGL3::Storage::Game {
	enum class GameId : uint32_t {
		Unknown = 0,

#define KEY(Name) Name = ~Utils::Crc32(#Name),

		KEY(Fortnite)

#undef KEY
	};
}