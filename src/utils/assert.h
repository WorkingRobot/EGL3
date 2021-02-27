#pragma once

#ifndef SERVICE_NAME
#include "../web/Hosts.h"
#include "AsyncMessageBox.h"
#else
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif
#include "StackTrace.h"

#include <stdio.h>
#include <cstdlib>

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

        if (Level == LogLevel::Critical) {
            printf("\n%s\n", Utils::GetStackTrace().c_str());
        }
    }

    template<LogLevel Level>
    static __forceinline bool _EGL3_Log(bool Condition, const char* ConditionString, const char* Message, const char* Filename, unsigned Line) {
        if (ConditionString && Condition) {
            return true;
        }
        _EGL3_LogFunc(Level, ConditionString, Message, Filename, Line);
        if constexpr (Level == LogLevel::Critical) {
#ifndef SERVICE_NAME
            char Text[2048];
            sprintf_s(Text, "A critical error occurred in EGL3:\n\nMessage: %s\nAt: %s @ %u\nReason: %s\n\nYou can report this issue at %s/discord", Message, Filename, Line, ConditionString ? ConditionString : "None", Web::GetHostUrl<Web::Host::EGL3NonApi>());
            Utils::AsyncMessageBox(Text, "EGL3 Critical Error", 0x00000010L | 0x00001000L); // MB_ICONERROR | MB_SYSTEMMODAL
#else
            HANDLE hEventSource = RegisterEventSource(NULL, SERVICE_NAME);
            if (hEventSource) {
                LPCSTR lpszStrings[2];
                CHAR Buffer[2048];

                sprintf_s(Buffer, "Message: %s\nAt: %s @ %u\nReason: %s", Message, Filename, Line, ConditionString ? ConditionString : "None");

                lpszStrings[0] = SERVICE_NAME;
                lpszStrings[1] = Buffer;

                ReportEvent(hEventSource,// event log handle
                    EVENTLOG_ERROR_TYPE, // event type
                    0,                   // event category
                    0xC0020001L,         // event identifier (SVC_ERROR)
                    NULL,                // no security identifier
                    2,                   // size of lpszStrings array
                    0,                   // no binary data
                    lpszStrings,         // array of strings
                    NULL);               // no binary data

                DeregisterEventSource(hEventSource);
            }
#endif
            std::abort();
        }
        return Condition;
    }
}

// If condition is false, return false and log error
#define EGL3_CONDITIONAL_LOG(condition, level, message) (_EGL3_Log<level>((condition), #condition, message, __FILE__, __LINE__))

#define EGL3_LOG(level, message) (_EGL3_Log<level>(true, nullptr, message, __FILE__, __LINE__))
