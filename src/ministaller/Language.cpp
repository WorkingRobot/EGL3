#include "Language.h"

#include <type_traits>

namespace EGL3::Installer {
    using TypeT = std::underlying_type_t<Language::Type>;
    using EntryT = std::underlying_type_t<Language::Entry>;
    constexpr auto TypeCount = (TypeT)Language::Type::Count;
    constexpr auto EntryCount = (EntryT)Language::Entry::Count;

    constexpr const char* English[] = {
        "EGL3",
        "EGL3 Setup",

        "License Agreement",
        "Please review the license terms before installing EGL3.",
        "Press Page Down to see the rest of the agreement.",
        "If you accept the terms of the agreement, click I Agree to continue. You must accept the agreement to install EGL3.",

        "Choose Install Location",
        "Choose the folder in which to install EGL3.",
        "Setup will install EGL3 in the following folder. To install in a different folder, click Browse and select another folder. Click Install to start the installation.",
        "Destination Folder",
        "Browse...",
        "Select the folder to install EGL3 in:",
        "Space Required: ",
        "Space Available: ",

        "",
        "",
        "Show Details",

        "Installing",
        "Please wait while EGL3 is being installed.",

        "Installation Aborted",
        "Setup was not completed successfully.",

        "Cancel",
        "I Agree",
        "Install",
        "< Back",
        "Next >",

        "An error occurred while connecting to the server. Check your internet connection and try again.",
        "A DNS error occurred while connecting to the server. Check your internet connection and try again.",
        "The server returned an invalid status code. Check your internet connection and try again.",

        "The installation file downloaded is invalid. Try downloading a newer version of the installer and try again.",
        "The installation file downloaded is invalid. Try downloading a newer version of the installer and try again. (Magic = %X)",
        "The downloaded installation file has an unsupported file version. Try downloading a newer version of the installer and try again. (Version = %d)",
        "The installation file requested to execute a shell command, but could not be executed. Try downloading a newer version of the installer and try again. (Cmd = %s)",
        "The installation file requested to execute a shell command, but could not be executed successfully. Try downloading a newer version of the installer and try again. (Cmd = %s, Ret = %u)",
        "The installation file could not successfully load its compression dictionary. Try downloading a newer version of the installer and try again. (DictSize = %u)",
        "The installation file could not obtain valid file metadata. Try downloading a newer version of the installer and try again. (Error = %s)",
        "The installation file could not obtain valid filename data. Try downloading a newer version of the installer and try again. (Error = %s)",
        "The installation file could not open a file for writing. Make sure all instances of EGL3 is closed and try again. (Filename = %s)",
        "The installation file could not successfully decompress some file data. Try downloading a newer version of the installer and try again. (Filename = %s, Error = %s)",
        "The installation file could not write to a file. Make sure the installer has access to the installation folder, and that all instances of EGL3 is closed and try again. (Filename = %s)",

        "Executing command (Cmd = %s)",
        "Executed command (Cmd = %s, Ret = %u)",
        "Installing data (Size = %u)",
        "Installed data (Size = %u)",
        "Installing file from data (Filename = %s, Size = %u)",
    };
    static_assert(sizeof(English) == EntryCount * sizeof(char*));

    constexpr const char* const* LangTable[TypeCount] = { English };

    Language::Language(Type Type) :
        SelectedType(Type)
    {

    }

    Language::Type Language::GetType() const noexcept
    {
        return SelectedType;
    }

    const char* Language::GetEntry(Entry Entry) const noexcept
    {
        return LangTable[(TypeT)SelectedType][(EntryT)Entry];
    }
}