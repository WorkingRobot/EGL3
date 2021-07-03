#include "Frame.h"

namespace EGL3::Web::Stomp {
    Frame::Frame(CommandType Command, Stomp::Headers&& Headers, std::string&& Body) :
        Command(Command),
        Headers(std::move(Headers)),
        Body(std::move(Body))
    {
        if (!this->Body.empty() && !this->Headers.contains("content-length")) {
            this->Headers.emplace("content-length", std::to_string(this->Body.size()));
        }
    }

    Frame::Frame(std::string_view Data)
    {
        Decode(Data);
    }

    void AppendArray(std::string& Out, std::string_view Data, bool bShouldEscape)
    {
        if (bShouldEscape)
        {
            for (auto& Char : Data) {
                if (std::string_view(":\\\n\r").find_first_of(Char) != std::string_view::npos) {
                    Out += '\\';
                }
                Out += Char;
            }
        }
        else
        {
            Out += Data;
        }
    }

    std::string Frame::Encode() const
    {
        if (Command == CommandType::Heartbeat) {
            if (!Headers.empty()) {
                // Log warning
            }
            if (!Body.empty()) {
                // Log warning
            }

            return "\n";
        }
        else {
            std::string Ret;

            bool bShouldEscapeFrameHeader = Command != CommandType::Connect;

            auto CommandString = CommandTypeToString(Command);
            AppendArray(Ret, CommandString, bShouldEscapeFrameHeader);
            Ret += '\n';

            for (auto& Element : Headers)
            {
                AppendArray(Ret, Element.first, bShouldEscapeFrameHeader);
                Ret += ':';
                AppendArray(Ret, Element.second, bShouldEscapeFrameHeader);
                Ret += '\n';
            }

            Ret += '\n';
            Ret += Body;
            Ret += '\0';

            return Ret;
        }
    }

    CommandType Frame::GetCommand() const noexcept
    {
        return Command;
    }

    const Headers& Frame::GetHeaders() const noexcept
    {
        return Headers;
    }

    const std::string& Frame::GetBody() const noexcept
    {
        return Body;
    }

    void SkipNewlines(std::string_view& Data)
    {
        Data.remove_prefix(std::min(Data.find_first_not_of("\r\n"), Data.size()));
    }

    static uint8_t ReadValue(std::string_view& Data, std::string& Buffer, const char* Delimiters = "\n", bool bAllowEscaping = true)
    {
        bool bEscapeNext = false;
        uint8_t Retval = '\0';
        for (; !Data.empty(); Data.remove_prefix(1))
        {
            if (!bEscapeNext)
            {
                if (bAllowEscaping && Data.front() == '\\')
                {
                    bEscapeNext = true;
                    continue;
                }
                auto Found = std::string_view(Delimiters).find_first_of(Data.front());
                if (Found != std::string_view::npos)
                {
                    Retval = Delimiters[Found];
                    Data.remove_prefix(1);
                    break;
                }
            }
            Buffer += Data.front();
            bEscapeNext = false;
        }

        // The stomp protocol also allows \r\n in addition to \n as line delimiter -- simply trim the \r off the end if present
        // (Unhandled edge case: In case the buffer contains an escaped CR followed by a terminating newline, the CR will be stripped off in any case)
        if (Retval == '\n' && !Buffer.empty() && Buffer.back() == '\r')
        {
            Buffer.pop_back();
        }

        return Retval;
    }

    void Frame::Decode(std::string_view Data)
    {
        if (!Data.empty() && Data.back() == '\0') {
            Data.remove_suffix(1);
        }

        SkipNewlines(Data);

        if (Data.empty()) {
            Command = CommandType::Heartbeat;
            return;
        }

        std::string Buffer;
        ReadValue(Data, Buffer);
        Command = StringToCommandType(Buffer.c_str());

        if (Data.empty()) {
            // log warning
            return;
        }

        while (!Data.empty()) {
            Buffer.clear();
            auto Delimiter = ReadValue(Data, Buffer, "\n:");
            auto HeaderName = Buffer;

            if (Delimiter == ':') {
                Buffer.clear();
                ReadValue(Data, Buffer);
                Headers.emplace(HeaderName, Buffer);
            }
            else if (HeaderName.empty()) {
                // Empty line marks the end of headers
                break;
            }
            else {
                // log warning
                Headers.emplace(HeaderName, "");
            }
        }

        auto ContentLengthItr = Headers.find("content-length");
        if (ContentLengthItr != Headers.end()) {
            size_t ContentLength = std::stoull(ContentLengthItr->second);
            if (ContentLength > Data.size()) {
                ContentLength = Data.size();
                // log warning
            }
            Body = Data.substr(0, ContentLength);
            Data.remove_prefix(ContentLength);
        }
        else {
            ReadValue(Data, Body, "", false);
        }

        Headers.emplace("content-length", std::to_string(Body.size()));

        SkipNewlines(Data);

        if (!Data.empty()) {
            // log warning
        }
    }
}