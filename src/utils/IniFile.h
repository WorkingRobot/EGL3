#pragma once

#include <filesystem>
#include <unordered_map>

namespace EGL3::Utils {
    using IniSection = std::unordered_multimap<std::string, std::string>;

    class IniFile {
    public:
        IniFile(const std::filesystem::path& Path);

        IniFile(const std::string& Data);

        const IniSection* GetSection(const std::string& Name) const;

    private:
        static std::string GetString(const std::filesystem::path& Path);

        std::unordered_map<std::string, IniSection> Sections;
    };
}
