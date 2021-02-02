#include "ManifestHeader.h"

namespace EGL3::Web::Epic::BPS {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, ManifestHeader& Val) {
        auto StartPos = Stream.tell();

        Stream >> Val.Magic;
        if (Val.Magic != ManifestHeader::ExpectedMagic) {
            return Stream;
        }
        Stream >> Val.HeaderSize;
        Stream >> Val.DataSizeUncompressed;
        Stream >> Val.DataSizeCompressed;
        Stream >> Val.SHAHash;
        uint8_t StoredAs;
        Stream >> StoredAs;
        Val.StoredAs = (ManifestStorageFlags)StoredAs;

        int32_t Version;
        Stream >> Version;
        Val.Version = (FeatureLevel)Version;

        Stream.seek(StartPos + Val.HeaderSize, Utils::Streams::Stream::Beg);
        return Stream;
    }

    bool ManifestHeader::IsCompressed() const {
        return (uint8_t)StoredAs & (uint8_t)ManifestStorageFlags::Compressed;
    }

    bool ManifestHeader::IsEncrypted() const {
        return (uint8_t)StoredAs & (uint8_t)ManifestStorageFlags::Encrypted;
    }
}