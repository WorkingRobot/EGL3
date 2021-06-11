#include "IniFile.h"

#include "streams/FileStream.h"
#include "Quote.h"

namespace EGL3::Utils {
    bool LineExtended(const char*& Stream, std::string& Result, int& LinesConsumed, bool bExact)
    {
        bool bGotStream = false;
        bool bIsQuoted = false;
        bool bIgnore = false;
        int BracketDepth = 0;

        Result.clear();
        LinesConsumed = 0;

        while (*Stream != '\0' && ((*Stream != '\n' && *Stream != '\r') || BracketDepth > 0))
        {
            // Start of comments.
            if (!bIsQuoted && !bExact && Stream[0] == '/' && Stream[1] == '/')
            {
                bIgnore = true;
            }

            // Command chaining.
            if (!bIsQuoted && !bExact && *Stream == '|')
            {
                break;
            }

            bGotStream = true;

            // bracketed line break
            if (*Stream == '\n' || *Stream == '\r')
            {
                // checkSlow(BracketDepth > 0);

                Result.push_back(' ');
                LinesConsumed++;
                Stream++;
                if (*Stream == '\n' || *Stream == '\r')
                {
                    Stream++;
                }
            }
            // allow line break if the end of the line is a backslash
            else if (!bIsQuoted && Stream[0] == '\\' && (Stream[1] == '\n' || Stream[1] == '\r'))
            {
                Result.push_back(' ');
                LinesConsumed++;
                Stream += 2;
                if (*Stream == '\n' || *Stream == '\r')
                {
                    Stream++;
                }
            }
            // check for starting or ending brace
            else if (!bIsQuoted && *Stream == '{')
            {
                BracketDepth++;
                Stream++;
            }
            else if (!bIsQuoted && *Stream == '}' && BracketDepth > 0)
            {
                BracketDepth--;
                Stream++;
            }
            // specifically consume escaped backslashes and quotes within quoted strings
            else if (bIsQuoted && !bIgnore && Stream[0] == '\\' && (Stream[1] == '\"' || Stream[1] == '\\'))
            {
                Result.append(Stream, 2);
                Stream += 2;
            }
            else
            {
                bIsQuoted = bIsQuoted ^ (*Stream == '\"');

                // Got stuff.
                if (!bIgnore)
                {
                    Result.push_back(*(Stream++));
                }
                else
                {
                    Stream++;
                }
            }
        }
        if (*Stream == 0)
        {
            if (bGotStream)
            {
                LinesConsumed++;
            }
        }
        else if (bExact)
        {
            // Eat up exactly one CR/LF.
            if (*Stream == '\r' || *Stream == '\n')
            {
                LinesConsumed++;
                if (*Stream == '\r')
                {
                    Stream++;
                }
                if (*Stream == '\n')
                {
                    Stream++;
                }
            }
        }
        else
        {
            // Eat up all CR/LF's.
            while (*Stream == '\n' || *Stream == '\r' || *Stream == '|')
            {
                if (*Stream != '|')
                {
                    LinesConsumed++;
                }
                if ((Stream[0] == '\n' && Stream[1] == '\r') || (Stream[0] == '\r' && Stream[1] == '\n'))
                {
                    Stream++;
                }
                Stream++;
            }
        }

        return *Stream != '\0' || bGotStream;
    }

    IniFile::IniFile(const std::filesystem::path& Path) :
        IniFile(GetString(Path))
    {
        
    }

    // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Core/Private/Misc/ConfigCacheIni.cpp#L900
    IniFile::IniFile(const std::string& Data)
    {
        auto Ptr = Data.c_str();
        IniSection* CurrentSection = nullptr;
        bool Done = false;
        while (!Done && Ptr != nullptr) {
            while (*Ptr == '\r' || *Ptr == '\n') {
                ++Ptr;
            }

            std::string ParsedLine;
            int LinesConsumed = 0;
            LineExtended(Ptr, ParsedLine, LinesConsumed, false);
            if (Ptr == nullptr || *Ptr == 0) {
                Done = true;
            }

            std::string_view Line(ParsedLine);

            // ignore [comment] lines that start with ;
            if (Line.front() == ';') {
                continue;
            }

            // Strip trailing spaces from the current line
            Line.remove_suffix(std::distance(Line.rbegin(), std::find_if_not(Line.rbegin(), Line.rend(), isspace)));

            // If the first character in the line is [ and last char is ], this line indicates a section name
            if (Line.front() == '[' && Line.back() == ']') {
                CurrentSection = &Sections.try_emplace(std::string(Line.substr(1, Line.size() - 2))).first->second;;
            }
            else if (CurrentSection) {
                size_t AssignPos = Line.find('=');

                // Ignore any lines that don't contain a key-value pair
                if (AssignPos != std::string_view::npos)
                {
                    auto Key = Line.substr(0, AssignPos);
                    auto Value = Line.substr(AssignPos + 1);

                    // strip leading whitespace from the property name
                    Key.remove_prefix(std::distance(Key.begin(), std::find_if_not(Key.begin(), Key.end(), isspace)));

                    // Strip trailing spaces from the property name.
                    Key.remove_suffix(std::distance(Key.rbegin(), std::find_if_not(Key.rbegin(), Key.rend(), isspace)));

                    // Strip leading whitespace from the property value
                    Value.remove_prefix(std::distance(Value.begin(), std::find_if_not(Value.begin(), Value.end(), isspace)));

                    // strip trailing whitespace from the property value
                    Value.remove_suffix(std::distance(Value.rbegin(), std::find_if_not(Value.rbegin(), Value.rend(), isspace)));

                    // If this line is delimited by quotes
                    if (Value.front() == '\"')
                    {
                        // Add this pair to the current FConfigSection
                        CurrentSection->emplace(Key, Utils::Unquote(std::string(Value)));
                    }
                    else
                    {
                        // Add this pair to the current FConfigSection
                        CurrentSection->emplace(Key, Value);
                    }
                }
            }
        }
    }

    const IniSection* IniFile::GetSection(const std::string& Name) const
    {
        auto Itr = Sections.find(Name);
        if (Itr != Sections.end()) {
            return &Itr->second;
        }

        return nullptr;
    }

    std::string IniFile::GetString(const std::filesystem::path& Path)
    {
        Streams::FileStream Stream;
        if (!Stream.open(Path, "rb")) {
            return "";
        }
        std::string Data;
        Data.resize(Stream.size());
        Stream.read(Data.data(), Stream.size());
        return Data;
    }
}