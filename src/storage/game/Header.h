#pragma once

#include "ArchiveVersion.h"
#include "GameId.h"

#include <string>

namespace EGL3::Storage::Game {
#pragma pack(push, 2)
    // Strings are not required to be 0 terminated!
    class Header {
    public:
        Header() :
            Magic(ExpectedMagic),
            ArchiveVersion(ArchiveVersion::Latest),
            HeaderSize(sizeof(Header)),
            Game{},
            VersionStringLong{},
            VersionStringHR{},
            GameNumeric(GameId::Unknown),
            VersionNumeric(0)
        {

        }

        bool HasValidMagic() const {
            return Magic == ExpectedMagic;
        }

        ArchiveVersion GetVersion() const {
            return ArchiveVersion;
        }

        bool HasValidHeaderSize() const {
            return HeaderSize == sizeof(Header);
        }

        std::string GetGame() const {
            return std::string(Game, strnlen_s(Game, sizeof(Game)));
        }

        std::string GetVersionLong() const {
            return std::string(VersionStringLong, strnlen_s(VersionStringLong, sizeof(VersionStringLong)));
        }

        std::string GetVersionHR() const {
            return std::string(VersionStringHR, strnlen_s(VersionStringHR, sizeof(VersionStringHR)));
        }

        GameId GetGameId() const {
            return GameNumeric;
        }

        uint64_t GetVersionNum() const {
            return VersionNumeric;
        }

        static constexpr uint64_t GetOffsetManifestData() {
            return OffsetManifestData;
        }

        static constexpr uint64_t GetOffsetRunlistFile() {
            return OffsetRunlistFile;
        }

        static constexpr uint64_t GetOffsetRunlistChunkPart() {
            return OffsetRunlistChunkPart;
        }

        static constexpr uint64_t GetOffsetRunlistChunkInfo() {
            return OffsetRunlistChunkInfo;
        }

        static constexpr uint64_t GetOffsetRunlistChunkData() {
            return OffsetRunlistChunkData;
        }

        static constexpr uint64_t GetOffsetRunIndex() {
            return OffsetRunIndex;
        }

        static constexpr uint64_t GetSectorSize() {
            return SectorSize;
        }

        static constexpr uint64_t GetMinimumArchiveSize() {
            return FileDataOffset;
        }

        static constexpr uint64_t GetFirstValidSector() {
            return FileDataSectorOffset;
        }

    private:
        uint32_t Magic;                     // 'EGIA' aka 0x41494745
        uint16_t HeaderSize;                // Size of the header
        ArchiveVersion ArchiveVersion;      // Version of the install archive, currently 0
        char Game[40];                      // Game name - example: Fortnite
        char VersionStringLong[64];         // Example: ++Fortnite+Release-13.30-CL-13884634-Windows
        char VersionStringHR[20];           // Human readable version - example: 13.30
        uint64_t VersionNumeric;            // Game specific, but a new update will always have a higher number (e.g. Fortnite's P4 CL)
        GameId GameNumeric;                 // Should be some sort of enum (GameId)

        static constexpr uint32_t ExpectedMagic = 0x41494745;

        // bytes (not sectors)
        static constexpr uint64_t OffsetManifestData = 256;
        static constexpr uint64_t OffsetRunlistFile = 2048;
        static constexpr uint64_t OffsetRunlistChunkPart = 16384;
        static constexpr uint64_t OffsetRunlistChunkInfo = 32768;
        static constexpr uint64_t OffsetRunlistChunkData = 49152;
        static constexpr uint64_t OffsetRunIndex = 65536;

        static constexpr uint64_t SectorSize = 4096;
        static constexpr uint64_t FileDataOffset = 131072;

        static constexpr uint64_t FileDataSectorOffset = FileDataOffset / SectorSize;
    };
#pragma pack(pop)
    static_assert(sizeof(Header) == 144);
}