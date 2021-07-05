#include "Store.h"

#include "../../utils/streams/FileStream.h"
#include "../../utils/Log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Storage::Persistent {
    Store::Store(const std::filesystem::path& Path) :
        Path(Path)
    {
        std::error_code Code;
        if (!std::filesystem::is_regular_file(Path, Code)) {
            return;
        }

        Utils::Streams::FileStream Stream;
        if (!Stream.open(Path, "rb")) {
            return;
        }

        size_t Count;
        Stream >> Count;
        for (uint32_t Idx = 0; Idx < Count; ++Idx) {
            uint32_t KeyHash;
            Stream >> KeyHash;
            size_t ValueSize;
            Stream >> ValueSize;
            Elements.emplace(KeyHash, std::make_pair<Detail::MoveableAny, Serializer>(Detail::MakeAny<Detail::TemporaryElement>(Stream, ValueSize), nullptr));
        }
    }

    Store::~Store()
    {
        Flush();
    }

    void Store::Flush()
    {
        std::error_code Code;
        if (std::filesystem::is_regular_file(Path, Code)) {
            std::filesystem::path TempPath = Path;
            TempPath.replace_extension("tmp");

            {
                Utils::Streams::FileStream Stream;
                Stream.open(TempPath, "wb");
                WriteTo(Stream);
            }

            std::filesystem::path BackupPath = Path;
            BackupPath.replace_extension("bak");

            EGL3_ENSUREF(ReplaceFileW(Path.c_str(), TempPath.c_str(), BackupPath.c_str(), REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL), LogLevel::Error, "Could not replace store file with flushed data (GLE: {})", GetLastError());
        }
        else {
            Utils::Streams::FileStream Stream;
            Stream.open(Path, "wb");
            WriteTo(Stream);
        }
    }

    void Store::WriteTo(Utils::Streams::Stream& Stream)
    {
        Stream << Elements.size();
        for (auto& Element : Elements) {
            Stream << Element.first;

            auto TempElement = Element.second.first.TryGet<Detail::TemporaryElement>();
            if (TempElement) {
                Stream << TempElement->size();
                Stream.write(TempElement->get(), TempElement->size());
            }
            else {
                Stream << size_t(0);
                auto StartPos = Stream.tell();
                Element.second.second(Stream, Element.second.first);
                auto EndPos = Stream.tell();
                Stream.seek(StartPos - sizeof(size_t), Utils::Streams::Stream::Beg);
                Stream << size_t(EndPos - StartPos);
                Stream.seek(EndPos, Utils::Streams::Stream::Beg);
            }
        }
    }
}