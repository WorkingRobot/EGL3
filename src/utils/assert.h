#pragma once

#include "AsyncMessageBox.h"

#include <stdio.h>
#include <stdlib.h>

namespace EGL3 {
	enum LogLevel {
		LogLevel_Critical,
		LogLevel_Error,
		LogLevel_Warning,
		LogLevel_Message,
		LogLevel_Info,
		LogLevel_Debug
	};

	static constexpr const char* LogLevelToString(LogLevel Level) {
		switch (Level)
		{
		case LogLevel_Critical:
			return "Critical";
		case LogLevel_Error:
			return "Error";
		case LogLevel_Warning:
			return "Warning";
		case LogLevel_Message:
			return "Message";
		case LogLevel_Info:
			return "Info";
		case LogLevel_Debug:
			return "Debug";
		default:
			return "Unknown";
		}
	}

	static void _EGL3_Default_LogFunc(LogLevel Level, const char* Condition, const char* Message, const char* Filename, unsigned Line) {
		if (Condition) {
			printf("%s: %s (%s @ %u -> %s)\n", LogLevelToString(Level), Message, Filename, Line, Condition);
		}
		else {
			printf("%s: %s (%s @ %u)\n", LogLevelToString(Level), Message, Filename, Line);
		}
	}

	// Condition is nullptr if it wasn't conditional
	static void (*_EGL3_LogFunc) (LogLevel Level, const char* Condition, const char* Message, const char* Filename, unsigned Line) = _EGL3_Default_LogFunc;

	static void _EGL3_SetLogger(decltype(_EGL3_LogFunc) NewLogger) {
		_EGL3_LogFunc = NewLogger;
	}

	template<LogLevel Level>
	static __forceinline void _EGL3_Log(const char* Condition, const char* Message, const char* Filename, unsigned Line) {
		_EGL3_LogFunc(Level, Condition, Message, Filename, Line);
		if constexpr (Level == LogLevel_Critical) {
			char Text[2048];
			sprintf_s(Text, "A critical error occurred in EGL3:\n\nMessage: %s\nAt: %s @ %u", Message, Filename, Line);
			if (Condition) {
				strcat_s(Text, "\nReason: ");
				strcat_s(Text, Condition);
			}
			strcat_s(Text, "\n\nYou can report this issue at https://epic.gl/discord");
			Utils::AsyncMessageBox(Text, "EGL3 Critical Error", MB_ICONERROR | MB_SYSTEMMODAL);
			std::abort();
		}
	}
}

#define EGL3_CONDITIONAL_LOG(condition, level, message) (void)((!!(condition)) || (_EGL3_Log<level>(#condition, message, __FILE__, __LINE__)))

#define EGL3_LOG(level, message) (_EGL3_Log<level>(nullptr, message, __FILE__, __LINE__))

#define EGL3_ASSERT(condition, message) // static_assert(false, message " - " #condition)