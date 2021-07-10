#include "Packer.h"

#include "../../utils/streams/FileStream.h"
#include "../../web/JsonParsing.h"
#include "streams/LZ4Stream.h"
#include "Constants.h"

#include <rapidjson/writer.h>

namespace EGL3::Installer::Backend {
    Packer::Packer(const std::filesystem::path& TargetDirectory, VersionInfo& Version, Utils::Streams::Stream& OutEgluStream, Utils::Streams::Stream& OutJsonStream)
    {
        {
            OutEgluStream << FileMagic;

            Streams::LZ4Stream Stream(OutEgluStream);

            uint64_t TotalFileSize = 0;

            auto FileBuffer = std::make_unique<char[]>(BufferSize);
            for (auto& File : std::filesystem::recursive_directory_iterator(TargetDirectory)) {
                if (File.is_directory()) {
                    continue;
                }

                Stream << (uint32_t)File.file_size();
                Stream << File.path().lexically_relative(TargetDirectory).string();

                TotalFileSize += File.file_size();

                Utils::Streams::FileStream FileStream;
                if (!FileStream.open(File.path(), "rb")) {
                    return;
                }

                uint32_t BytesLeft = FileStream.size();
                do {
                    auto BytesToCopy = std::min(BytesLeft, BufferSize);
                    FileStream.read(FileBuffer.get(), BytesToCopy);
                    Stream.write(FileBuffer.get(), BytesToCopy);
                    BytesLeft -= BytesToCopy;
                } while (BytesLeft);
            }

            // Marks end of file data
            Stream << (uint32_t)-1;

            if (Version.EstimatedSize == 0) {
                Version.EstimatedSize = TotalFileSize >> 10;
            }

            Stream << Version;
        }
        {
            rapidjson::StringBuffer JsonBuffer;
            {
                rapidjson::Writer<rapidjson::StringBuffer> Writer(JsonBuffer);

                Writer.StartObject();

                Writer.Key("version");
                Writer.String(Version.VersionFull.c_str(), Version.VersionFull.size());

                Writer.Key("versionHR");
                Writer.String(Version.DisplayVersion.c_str(), Version.DisplayVersion.size());

                Writer.Key("versionNum");
                Writer.Uint64(Version.VersionNum);

                Writer.Key("releaseDate");
                auto Now = Web::GetCurrentTimePoint();
                Writer.String(Now.c_str(), Now.size());

                Writer.Key("patchNotes");
                Writer.String(Version.PatchNotes.c_str(), Version.PatchNotes.size());

                Writer.EndObject();
            }

            OutJsonStream.write(JsonBuffer.GetString(), JsonBuffer.GetSize());
        }
    }
}