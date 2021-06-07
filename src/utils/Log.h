#pragma once

#ifndef SERVICE_NAME
#include "../web/Hosts.h"
#include "AsyncMessageBox.h"
#else
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include "Log.h"
#include "StackTrace.h"
#include <format>

namespace EGL3 {
#ifndef SERVICE_NAME
    static constexpr bool HasUI = true;
#else
    static constexpr bool HasUI = false;
#endif

    enum class LogLevel : uint8_t {
        Critical,
        Error,
        Warning,
        Message,
        Info,
        Debug
    };

    namespace Detail {
        static consteval std::string_view ToString(LogLevel Level) {
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

        static consteval std::string_view FixFilename(const std::string_view Filename) {
            constexpr std::string_view Pattern = "src\\";
            size_t Offset = Filename.rfind(Pattern);
            std::string_view FixedFilename = Offset == std::string_view::npos ? Filename : Filename.substr(Offset + Pattern.size());

            return FixedFilename;
        }

        template<LogLevel LogLevelLiteral>
        struct LogContextBase {
            constexpr static const std::string_view LogLevel = ToString(LogLevelLiteral);
            std::string_view Filename;
            unsigned Line;

            consteval LogContextBase(const char* FilenameLiteral, unsigned Line) :
                Filename(FixFilename(FilenameLiteral)),
                Line(Line)
            {

            }

            std::string operator()(const std::string_view Message) const {
                return std::format("{}: {} ({} @ {})", this->LogLevel, Message, this->Filename, this->Line);
            }
        };

        template<class BaseCtx>
        struct LogContextConditional : public BaseCtx {
            std::string_view Condition;

            template<typename... Args>
            consteval LogContextConditional(const char* ConditionLiteral, Args&& ... args) :
                BaseCtx(std::forward<Args>(args)...),
                Condition(ConditionLiteral)
            {

            }

            std::string operator()(const std::string_view Message) const {
                return std::format("{}: {} ({} @ {} -> {})", this->LogLevel, Message, this->Filename, this->Line, Condition);
            }
        };

        template<class BaseCtx>
        struct LogContextCritical : public BaseCtx {
            using BaseCtx::BaseCtx;

            static std::string GetStackTrace() {
                return Utils::GetStackTrace();
            }
        };

        template<LogLevel LogLevelLiteral, class Base>
        using LogContextCriticalized = std::conditional_t<LogLevelLiteral == LogLevel::Critical, LogContextCritical<Base>, Base>;

        template<LogLevel LogLevelLiteral>
        consteval auto CreateContext(const char* ConditionLiteral, const char* FilenameLiteral, unsigned Line) {
            return LogContextCriticalized<LogLevelLiteral, LogContextConditional<LogContextBase<LogLevelLiteral>>>(ConditionLiteral, FilenameLiteral, Line);
        }

        template<LogLevel LogLevelLiteral>
        consteval auto CreateContext(const char* FilenameLiteral, unsigned Line) {
            return LogContextCriticalized<LogLevelLiteral, LogContextBase<LogLevelLiteral>>(FilenameLiteral, Line);
        }

        template<class Ctx>
        void UseContextPrintf(const Ctx& Context, const std::string_view Message) {
            if constexpr (std::is_base_of_v<LogContextBase<LogLevel::Critical>, Ctx>) {
                printf("%s\n", std::format("{}\n\n{}", Context(Message), Ctx::GetStackTrace()).c_str());
            }
            else {
                printf("%s\n", Context(Message).c_str());
            }
        }

#ifndef SERVICE_NAME
        template<class Ctx>
        void UseContextMessageBox(const Ctx& Context, const std::string_view Message) {
            char Text[2048]{};
            std::format_to_n(Text, 2048, "{}\n\nYou can report this issue at {}/discord", Context(Message), Web::GetHostUrl<Web::Host::EGL3NonApi>());
            Utils::AsyncMessageBox(Text, "EGL3 Critical Error", 0x00000010L | 0x00001000L); // MB_ICONERROR | MB_SYSTEMMODAL
        }
#else
        template<class Ctx>
        void UseContextService(const Ctx& Context, const std::string_view Message) {
            HANDLE hEventSource = RegisterEventSource(NULL, SERVICE_NAME);
            if (hEventSource) {
                LPCSTR lpszStrings[2];
                CHAR Buffer[2048]{};

                std::format_to_n(Buffer, 2048, "A critical error occurred in EGL3:\n{}", Context(Message));

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
        }
#endif
        template<typename T>
        __forceinline auto UseMessageArgument(T MessageValue) {
            if constexpr (std::is_invocable_r_v<std::string_view, T>) {
                return MessageValue();
            }
            else {
                return MessageValue;
            }
        }

        template<class Ctx>
        consteval auto UseContext(const Ctx Context) {
            return [=](auto MessageValue) {
                auto Message = UseMessageArgument(MessageValue);

                UseContextPrintf(Context, Message);
                if constexpr (std::is_base_of_v<LogContextBase<LogLevel::Critical>, Ctx>) {
                    if constexpr (HasUI) {
                        UseContextMessageBox(Context, Message);
                    }
                    else {
                        UseContextService(Context, Message);
                    }
                    std::abort();
                }
            };
        }

        template<class Ctx>
        consteval auto UseContextConditional(const Ctx Context) {
            return [=](bool Result, auto MessageValue) {
                if (Result) {
                    return true;
                }
                std::string_view Message = UseMessageArgument(MessageValue);

                UseContextPrintf(Context, Message);
                if constexpr (std::is_base_of_v<LogContextBase<LogLevel::Critical>, Ctx>) {
                    if constexpr (HasUI) {
                        UseContextMessageBox(Context, Message);
                    }
                    else {
                        UseContextService(Context, Message);
                    }
                    std::abort();
                }
                return false;
            };
        }

        template<LogLevel LogLevelLiteral, typename... Args>
        consteval auto UseContext(Args&& ... args) {
            return UseContext(CreateContext<LogLevelLiteral>(std::forward<Args>(args)...));
        }

        template<LogLevel LogLevelLiteral, typename... Args>
        consteval auto UseContextConditional(Args&& ... args) {
            return UseContextConditional(CreateContext<LogLevelLiteral>(std::forward<Args>(args)...));
        }
    }
}

// Log a formatted message
#define EGL3_LOGF(level, message, ...) (::EGL3::Detail::UseContext<level>(__FILE__, __LINE__)([&]() { return std::format((message), __VA_ARGS__); }))

// Log a message
#define EGL3_LOG(level, message) (::EGL3::Detail::UseContext<level>(__FILE__, __LINE__)((message)))

// A failure will cause a formatted message to be logged, will return true if the condition is true
#define EGL3_ENSUREF(condition, level, message, ...) (::EGL3::Detail::UseContextConditional<level>(#condition, __FILE__, __LINE__)((condition), [&]() { return std::format((message), __VA_ARGS__); }))

// A failure will cause a message to be logged, will return true if the condition is true
#define EGL3_ENSURE(condition, level, message) (::EGL3::Detail::UseContextConditional<level>(#condition, __FILE__, __LINE__)((condition), (message)))

// Log and abort with a critical error with a formatted message
#define EGL3_ABORTF(message, ...) EGL3_LOGF(::EGL3::LogLevel::Critical, (message), __VA_ARGS__)

// Log and abort with a critical error with a message
#define EGL3_ABORT(message) EGL3_LOG(::EGL3::LogLevel::Critical, (message))

// A failure will cause a critical error with a formatted message
#define EGL3_VERIFYF(condition, message, ...) EGL3_ENSUREF(condition, ::EGL3::LogLevel::Critical, (message), __VA_ARGS__)

// A failure will cause a critical error with a message
#define EGL3_VERIFY(condition, message) EGL3_ENSURE(condition, ::EGL3::LogLevel::Critical, (message))