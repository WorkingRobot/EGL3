#pragma once

#include "../web/Hosts.h"
#include "AsyncMessageBox.h"

#include <stdio.h>
#include <stdlib.h>

namespace EGL3 {
	enum class LogLevel : uint8_t {
		Critical,
		Error,
		Warning,
		Message,
		Info,
		Debug
	};

	static constexpr const char* LogLevelToString(LogLevel Level) {
		switch (Level)
		{
		case LogLevel::Critical:
			return "Critical";
		case LogLevel::Error:
			return "Error";
		case LogLevel::Warning:
			return "Warning";
		case LogLevel::Message:
			return "Message";
		case LogLevel::Info:
			return "Info";
		case LogLevel::Debug:
			return "Debug";
		default:
			return "Unknown";
		}
	}

	static void _EGL3_LogFunc(LogLevel Level, const char* Condition, const char* Message, const char* Filename, unsigned Line) {
		if (Condition) {
			printf("%s: %s (%s @ %u -> %s)\n", LogLevelToString(Level), Message, Filename, Line, Condition);
		}
		else {
			printf("%s: %s (%s @ %u)\n", LogLevelToString(Level), Message, Filename, Line);
		}
	}

	template<LogLevel Level, size_t ConditionStringSize, size_t MessageSize, size_t FilenameSize>
	static __forceinline bool _EGL3_Log(bool Condition, const char(&ConditionString)[ConditionStringSize], const char(&Message)[MessageSize], const char(&Filename)[FilenameSize], unsigned Line) {
		if (ConditionStringSize && Condition) {
			return true;
		}
		_EGL3_LogFunc(Level, ConditionString, Message, Filename, Line);
		if constexpr (Level == LogLevel::Critical) {
			char Text[2048];
			sprintf_s(Text, "A critical error occurred in EGL3:\n\nMessage: %s\nAt: %s @ %u\nReason: %s\n\nYou can report this issue at %s/discord", Message, Filename, Line, Condition ? Condition : "None", Web::GetHostUrl<Web::Host::EGL3NonApi>());
			Utils::AsyncMessageBox(Text, "EGL3 Critical Error", 0x00000010L | 0x00001000L); // MB_ICONERROR | MB_SYSTEMMODAL
			std::abort();
		}
		return Condition;
	}
}

// If condition is false, return false and log error
#define EGL3_CONDITIONAL_LOG(condition, level, message) (_EGL3_Log<level>((condition), #condition, message, __FILE__, __LINE__))

#define EGL3_LOG(level, message) (_EGL3_Log<level, 0>(true, nullptr, message, __FILE__, __LINE__))

#define EGL3_ASSERT(condition, message) static_assert(false, message " - " #condition) // EGL3_CONDITIONAL_LOG(condition, LogLevel_Critical, message)