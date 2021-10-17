#pragma once

#include <stdint.h>

namespace EGL3::Installer {
    class Language {
    public:
        enum class Type : uint8_t {
            English,

            Count
        };

        enum class Entry : uint8_t {
            Branding,
            Caption,

            TitleLicense,
            DescriptionLicense,
            TopTextLicense,
            IntroTextLicense,

            TitleDirectory,
            DescriptionDirectory,
            IntroTextDirectory,
            BoxDirectory,
            BrowseDirectory,
            BrowseTextDirectory,
            SpaceReqDirectory,
            SpaceAvailDirectory,

            TitleInstall,
            DescriptionInstall,
            ShowDetailsInstall,

            TitleInstallRunning,
            DescriptionInstallRunning,

            TitleInstallAborted,
            DescriptionInstallAborted,

            Cancel,
            Agree,
            Install,
            Back,
            Next,

            GenericWebError,
            DnsWebError,
            StatusCodeWebError,

            BadReadInstallation,
            BadMagicInstallation,
            BadVersionInstallation,
            BadCreateProcessInstallation,
            BadReturnCodeInstallation,
            BadDictionaryLoadInstallation,
            BadMetadataReadInstallation,
            BadFilenameReadInstallation,
            BadFileOpenInstallation,
            BadFileDecompressionInstallation,
            BadFileWriteInstallation,

            ExecutingCommandInstallation,
            ExecutedCommandInstallation,
            InstallingDataInstallation,
            InstalledDataInstallation,
            InstallingFileInstallation,

            Count
        };

        Language(Type Type = Type::English);

        Type GetType() const noexcept;
        const char* GetEntry(Entry Entry) const noexcept;

    private:
        Type SelectedType;
    };
}