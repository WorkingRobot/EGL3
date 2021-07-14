#include "Packer.h"

#include "../../utils/streams/FileStream.h"
#include "../../web/JsonParsing.h"
#include "streams/LZ4Stream.h"
#include "Constants.h"

#include <rapidjson/writer.h>

namespace EGL3::Installer::Backend {
    Packer::Packer(const std::string& AppVersion, const std::string& ShortAppVersion, uint32_t VersionMajor, uint32_t VersionMinor, uint32_t VersionPatch, uint64_t VersionNum) :
        AppVersion(AppVersion),
        ShortAppVersion(ShortAppVersion),
        VersionMajor(VersionMajor),
        VersionMinor(VersionMinor),
        VersionPatch(VersionPatch),
        VersionNum(VersionNum)
    {

    }

    void Packer::SetInputPath(const std::filesystem::path& InputPath)
    {
        this->InputPath = InputPath;
    }

    bool Packer::Pack(const std::filesystem::path& OutputPath) const
    {
        Utils::Streams::FileStream OutStream;
        if (!OutStream.open(OutputPath, "wb")) {
            return false;
        }

        OutStream << FileMagic;

        OutStream << AppVersion;
        OutStream << ShortAppVersion;
        OutStream << VersionMajor;
        OutStream << VersionMinor;
        OutStream << VersionPatch;
        OutStream << VersionNum;

        size_t FileCountOffset = OutStream.tell();
        OutStream << (uint32_t)0;

        uint32_t FileCount = 0;
        {
            Streams::LZ4Stream Stream(OutStream);

            auto FileBuffer = std::make_unique<char[]>(BufferSize);
            for (auto& File : std::filesystem::recursive_directory_iterator(InputPath)) {
                if (File.is_directory()) {
                    continue;
                }
                ++FileCount;

                Stream << File.file_size();
                Stream << File.path().lexically_relative(InputPath).string();

                Utils::Streams::FileStream FileStream;
                if (!FileStream.open(File.path(), "rb")) {
                    return false;
                }

                uint64_t BytesLeft = FileStream.size();
                while (BytesLeft) {
                    auto BytesToCopy = std::min(BytesLeft, BufferSize);
                    FileStream.read(FileBuffer.get(), BytesToCopy);
                    Stream.write(FileBuffer.get(), BytesToCopy);
                    BytesLeft -= BytesToCopy;
                }
            }
        }
        OutStream.seek(FileCountOffset, Utils::Streams::Stream::Beg);
        OutStream << FileCount;

        return true;
    }

    bool Packer::ExportJson(const std::filesystem::path& OutputPath, const std::filesystem::path& PatchNotesInput) const
    {
        auto Time = Web::GetCurrentTimePoint();

        std::string PatchNotes;
        {
            Utils::Streams::FileStream NotesStream;
            if (!NotesStream.open(PatchNotesInput, "rb")) {
                return false;
            }
            PatchNotes.resize(NotesStream.size());
            NotesStream.read(PatchNotes.data(), NotesStream.size());
        }

        Utils::Streams::FileStream OutStream;
        if (!OutStream.open(OutputPath, "wb")) {
            return false;
        }

        rapidjson::StringBuffer JsonBuffer;
        {
            rapidjson::Writer<rapidjson::StringBuffer> Writer(JsonBuffer);

            Writer.StartObject();

            Writer.Key("version");
            Writer.String(AppVersion.c_str(), AppVersion.size());

            Writer.Key("versionHR");
            Writer.String(ShortAppVersion.c_str(), ShortAppVersion.size());

            Writer.Key("versionNum");
            Writer.Uint64(VersionNum);

            Writer.Key("releaseDate");
            Writer.String(Time.c_str(), Time.size());

            Writer.Key("patchNotes");
            Writer.String(PatchNotes.c_str(), PatchNotes.size());

            Writer.EndObject();
        }

        OutStream.write(JsonBuffer.GetString(), JsonBuffer.GetSize());

        return true;
    }
}