#include "FileManifestList.h"

#include <algorithm>
#include <numeric>

namespace EGL3::Web::Epic::BPS {
    UEStream& operator>>(UEStream& Stream, FileManifestList& Val) {
        auto StartPos = Stream.tell();

        uint32_t DataSize;
        FileManifestListVersion DataVersion;
        int32_t ElementCount;
        {
            Stream >> DataSize;
            Stream >> DataVersion;
            Stream >> ElementCount;
        }

        Val.FileList.resize(ElementCount);

        if (DataVersion >= FileManifestListVersion::Original) {
            for (auto& File : Val.FileList) { Stream >> File.Filename; }
            for (auto& File : Val.FileList) { Stream >> File.SymlinkTarget; }
            for (auto& File : Val.FileList) { Stream >> File.FileHash; }
            for (auto& File : Val.FileList) { Stream >> File.FileMetaFlags; }
            for (auto& File : Val.FileList) { Stream >> File.InstallTags; }
            for (auto& File : Val.FileList) { Stream >> File.ChunkParts; }
        }

        Val.OnPostLoad();

        Stream.seek(StartPos + DataSize, UEStream::Beg);
        return Stream;
    }

    void FileManifestList::OnPostLoad() {
        // Sort file list and calculate file sizes
        std::sort(FileList.begin(), FileList.end());
        for (auto& File : FileList) {
            File.FileSize = std::accumulate(File.ChunkParts.begin(), File.ChunkParts.end(), 0ull, [](uint64_t Val, const auto& ChunkPart) {
                return Val + ChunkPart.Size;
            });
        }
    }
}