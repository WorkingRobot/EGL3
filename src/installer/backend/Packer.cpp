#include "Packer.h"

#include "../../utils/streams/FileStream.h"
#include "streams/LZ4Stream.h"
#include "Constants.h"

namespace EGL3::Installer::Backend {
    Packer::Packer(const std::filesystem::path& TargetDirectory, RegistryInfo& Registry, Utils::Streams::Stream& OutStream)
    {
        OutStream << FileMagic;

        Streams::LZ4Stream Stream(OutStream);

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

        if (Registry.EstimatedSize == 0) {
            Registry.EstimatedSize = TotalFileSize >> 10;
        }

        Stream << Registry;
    }
}