#include "Manifest.h"

#include "../../../utils/Log.h"
#include "../../../utils/Compression.h"
#include "../../../utils/Crc32.h"
#include "../../../utils/Hex.h"
#include "../../../utils/JsonWrapperStream.h"
#include "../../../utils/SHA.h"
#include "../../../utils/streams/BufferStream.h"
#include "../../../utils/StringBlob.h"
#include "UEStream.h"

#include <algorithm>

namespace EGL3::Web::Epic::BPS {
    using namespace Utils::Streams;

    Manifest::Manifest(Stream& Stream) :
        Error(ErrorType::Success)
    {
        Stream >> *this;
    }

    Manifest::Manifest(const char* Data, size_t DataSize) :
        Error(ErrorType::Success)
    {
        // Don't worry, we won't be writing to it
        BufferStream Stream((char*)Data, DataSize);

        Stream >> *this;
    }

    Manifest::Manifest(const rapidjson::Document& Json)
    {
        ReadFromJson(Json);
    }

    bool Manifest::HasError() const
    {
        return GetError() != ErrorType::Success;
    }

    Manifest::ErrorType Manifest::GetError() const
    {
        return Error;
    }

    const FileManifest* Manifest::GetFile(const std::string& Filename) const
    {
        auto Itr = std::find_if(FileManifestList.FileList.begin(), FileManifestList.FileList.end(), [&Filename](const FileManifest& Manifest) {
            return Manifest.Filename == Filename;
        });

        if (Itr != FileManifestList.FileList.end()) {
            return &*Itr;
        }
        return nullptr;
    }

    const ChunkInfo* Manifest::GetChunk(const Utils::Guid& Guid) const
    {
        auto Itr = std::find_if(ChunkDataList.ChunkList.begin(), ChunkDataList.ChunkList.end(), [&Guid](const ChunkInfo& Chunk) {
            return Chunk.Guid == Guid;
        });

        if (Itr != ChunkDataList.ChunkList.end()) {
            return &*Itr;
        }
        return nullptr;
    }

    bool TryGetString(const rapidjson::Value& Val, std::string& Out) {
        if (!Val.IsString()) {
            return false;
        }
        Out = std::string(Val.GetString(), Val.GetStringLength());
        return true;
    }

    bool TryGetString(const rapidjson::Value& Json, const char* Name, std::string& Out) {
        auto Val = Json.FindMember(Name);
        if (Val == Json.MemberEnd()) {
            return false;
        }
        return TryGetString(Val->value, Out);
    }

    template<class T>
    bool TryGetStringBlob(const rapidjson::Value& Val, T& Out) {
        if (!Val.IsString() || Val.GetStringLength() != sizeof(T) * 3) {
            return false;
        }
        Utils::FromBlob(Val.GetString(), (char*)&Out, sizeof(T));
        return true;
    }

    template<class T>
    bool TryGetStringBlob(const rapidjson::Value& Json, const char* Name, T& Out) {
        auto Val = Json.FindMember(Name);
        if (Val == Json.MemberEnd()) {
            return false;
        }
        return TryGetStringBlob(Val->value, Out);
    }
    
    template<uint32_t OutSize>
    bool TryGetHex(const rapidjson::Value& Val, char(&Out)[OutSize]) {
        if (!Val.IsString() || Val.GetStringLength() != OutSize * 2) {
            return false;
        }
        Utils::FromHex(Val.GetString(), Out, OutSize);
        return true;
    }

    template<uint32_t OutSize>
    bool TryGetHex(const rapidjson::Value& Json, const char* Name, char(&Out)[OutSize]) {
        auto Val = Json.FindMember(Name);
        if (Val == Json.MemberEnd()) {
            return false;
        }
        return TryGetHex(Val->value, Out);
    }

    template<class T>
    bool TryGet(const rapidjson::Value& Val, T& Out) {
        if (!Val.Is<T>()) {
            return false;
        }
        Out = Val.Get<T>();
        return true;
    }

    template<class T>
    bool TryGet(const rapidjson::Value& Json, const char* Name, T& Out) {
        auto Val = Json.FindMember(Name);
        if (Val == Json.MemberEnd()) {
            return false;
        }
        return TryGet<T>(Val->value, Out);
    }

    constexpr uint8_t HexDigit(char c)
    {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'f') {
            return c + 10 - 'a';
        }
        else if (c >= 'A' && c <= 'F') {
            return c + 10 - 'A';
        }
        else {
            return 0;
        }
    }

    uint32_t HexNumber(const char* HexString)
    {
        uint32_t Ret = 0;

        for (int i = 0; i < 8; ++i)
        {
            Ret <<= 4;
            Ret += HexDigit(*HexString++);
        }

        return Ret;
    }

    bool TryGetGuid(const rapidjson::Value& Val, Utils::Guid& Out) {
        if (!Val.IsString() || Val.GetStringLength() != sizeof(Utils::Guid) * 2) {
            return false;
        }
        Out.A = HexNumber(Val.GetString() + 0);
        Out.B = HexNumber(Val.GetString() + 8);
        Out.C = HexNumber(Val.GetString() + 16);
        Out.D = HexNumber(Val.GetString() + 24);
        return true;
    }

    bool TryGetGuid(const rapidjson::Value& Json, const char* Name, Utils::Guid& Out) {
        auto Val = Json.FindMember(Name);
        if (Val == Json.MemberEnd()) {
            return false;
        }
        return TryGetGuid(Val->value, Out);
    }

    // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/BuildPatchServices/Private/BuildPatchManifest.cpp#L400
    void Manifest::ReadFromJson(const rapidjson::Document& Json)
    {
        if (Json.HasParseError()) {
            SetError(ErrorType::InvalidJson);
            return;
        }

        if (!TryGetStringBlob(Json, "ManifestFileVersion", ManifestMeta.FeatureLevel)) {
            ManifestMeta.FeatureLevel = BPS::FeatureLevel::CustomFields;
        }
        if (ManifestMeta.FeatureLevel == BPS::FeatureLevel::BrokenJsonVersion) {
            ManifestMeta.FeatureLevel = BPS::FeatureLevel::StoresChunkFileSizes;
        }

        if (!TryGetStringBlob(Json, "AppID", ManifestMeta.AppId)) {
            SetError(ErrorType::BadJson);
            return;
        }

        if (!TryGetString(Json, "AppNameString", ManifestMeta.AppName)) {
            SetError(ErrorType::BadJson);
            return;
        }

        if (!TryGetString(Json, "BuildVersionString", ManifestMeta.BuildVersion)) {
            SetError(ErrorType::BadJson);
            return;
        }

        if (!TryGetString(Json, "LaunchExeString", ManifestMeta.LaunchExe)) {
            SetError(ErrorType::BadJson);
            return;
        }

        if (!TryGetString(Json, "LaunchCommand", ManifestMeta.LaunchCommand)) {
            SetError(ErrorType::BadJson);
            return;
        }

        // Optional
        TryGetString(Json, "PrereqName", ManifestMeta.PrereqName);
        TryGetString(Json, "PrereqPath", ManifestMeta.PrereqPath);
        TryGetString(Json, "PrereqArgs", ManifestMeta.PrereqArgs);

        std::unordered_map<Utils::Guid, ChunkInfo> ChunkMap;

        auto JsonFileManifestList = Json.FindMember("FileManifestList");
        if (JsonFileManifestList == Json.MemberEnd() || !JsonFileManifestList->value.IsArray()) {
            SetError(ErrorType::BadJson);
            return;
        }
        FileManifestList.FileList.reserve(JsonFileManifestList->value.GetArray().Size());
        for (auto& JsonFileManifest : JsonFileManifestList->value.GetArray()) {
            auto& FileManifest = FileManifestList.FileList.emplace_back();

            if (!TryGetString(JsonFileManifest, "Filename", FileManifest.Filename)) {
                SetError(ErrorType::BadJson);
                return;
            }

            if (!TryGetStringBlob(JsonFileManifest, "FileHash", FileManifest.FileHash)) {
                SetError(ErrorType::BadJson);
                return;
            }

            auto JsonChunkParts = JsonFileManifest.FindMember("FileChunkParts");
            if (JsonChunkParts == JsonFileManifest.MemberEnd() || !JsonChunkParts->value.IsArray()) {
                SetError(ErrorType::BadJson);
                return;
            }
            FileManifest.ChunkParts.reserve(JsonChunkParts->value.GetArray().Size());
            for (auto& JsonChunkPart : JsonChunkParts->value.GetArray()) {
                auto& ChunkPart = FileManifest.ChunkParts.emplace_back();

                if (!TryGetGuid(JsonChunkPart, "Guid", ChunkPart.Guid)) {
                    SetError(ErrorType::BadJson);
                    return;
                }

                if (!TryGetStringBlob(JsonChunkPart, "Offset", ChunkPart.Offset)) {
                    SetError(ErrorType::BadJson);
                    return;
                }

                if (!TryGetStringBlob(JsonChunkPart, "Size", ChunkPart.Size)) {
                    SetError(ErrorType::BadJson);
                    return;
                }

                ChunkMap.try_emplace(ChunkPart.Guid, ChunkInfo{ .Guid = ChunkPart.Guid, .WindowSize = 1 << 20 });
            }

            auto JsonInstallTags = JsonFileManifest.FindMember("InstallTags");
            if (JsonInstallTags != JsonFileManifest.MemberEnd() && JsonInstallTags->value.IsArray()) {
                FileManifest.InstallTags.reserve(JsonInstallTags->value.GetArray().Size());
                for (auto& JsonInstallTag : JsonInstallTags->value.GetArray()) {
                    if (!JsonInstallTag.IsString()) {
                        SetError(ErrorType::BadJson);
                        return;
                    }
                    FileManifest.InstallTags.emplace_back(JsonInstallTag.GetString(), JsonInstallTag.GetStringLength());
                }
            }

            auto JsonIsUnixExecutable = JsonFileManifest.FindMember("bIsUnixExecutable");
            if (JsonIsUnixExecutable != JsonFileManifest.MemberEnd() && JsonIsUnixExecutable->value.IsBool() && JsonIsUnixExecutable->value.GetBool()) {
                FileManifest.FileMetaFlags = (FileMetaFlags)((uint8_t)FileManifest.FileMetaFlags | (uint8_t)FileMetaFlags::UnixExecutable);
            }

            auto JsonIsReadOnly = JsonFileManifest.FindMember("bIsReadOnly");
            if (JsonIsReadOnly != JsonFileManifest.MemberEnd() && JsonIsReadOnly->value.IsBool() && JsonIsReadOnly->value.GetBool()) {
                FileManifest.FileMetaFlags = (FileMetaFlags)((uint8_t)FileManifest.FileMetaFlags | (uint8_t)FileMetaFlags::ReadOnly);
            }

            auto JsonIsCompressed = JsonFileManifest.FindMember("bIsCompressed");
            if (JsonIsCompressed != JsonFileManifest.MemberEnd() && JsonIsCompressed->value.IsBool() && JsonIsCompressed->value.GetBool()) {
                FileManifest.FileMetaFlags = (FileMetaFlags)((uint8_t)FileManifest.FileMetaFlags | (uint8_t)FileMetaFlags::Compressed);
            }

            TryGetString(JsonFileManifest, "SymlinkTarget", FileManifest.SymlinkTarget);
        }
        FileManifestList.OnPostLoad();

        bool HasChunkHashList = false;
        auto JsonChunkHashList = Json.FindMember("ChunkHashList");
        if (JsonChunkHashList == Json.MemberEnd() || !JsonChunkHashList->value.IsObject()) {
            SetError(ErrorType::BadJson);
            return;
        }
        for (auto& JsonChunkHash : JsonChunkHashList->value.GetObject()) {
            Utils::Guid TargetGuid;
            if (!TryGetGuid(JsonChunkHash.name, TargetGuid)) {
                SetError(ErrorType::BadJson);
                return;
            }

            auto Itr = ChunkMap.find(TargetGuid);
            if (Itr != ChunkMap.end()) {
                if (!TryGetStringBlob(JsonChunkHash.value, Itr->second.Hash)) {
                    SetError(ErrorType::BadJson);
                    return;
                }
                HasChunkHashList = true;
            }
        }

        auto JsonChunkShaList = Json.FindMember("ChunkShaList");
        if (JsonChunkShaList != Json.MemberEnd()) {
            if (!JsonChunkShaList->value.IsObject()) {
                SetError(ErrorType::BadJson);
                return;
            }
            for (auto& JsonChunkSha : JsonChunkShaList->value.GetObject()) {
                Utils::Guid TargetGuid;
                if (!TryGetGuid(JsonChunkSha.name, TargetGuid)) {
                    SetError(ErrorType::BadJson);
                    return;
                }

                auto Itr = ChunkMap.find(TargetGuid);
                if (Itr != ChunkMap.end()) {
                    if (!TryGetHex(JsonChunkSha.value, Itr->second.SHAHash)) {
                        SetError(ErrorType::BadJson);
                        return;
                    }
                }
            }
        }

        auto JsonPrereqIdsList = Json.FindMember("PrereqIds");
        if (JsonPrereqIdsList != Json.MemberEnd()) {
            if (!JsonPrereqIdsList->value.IsArray()) {
                SetError(ErrorType::BadJson);
                return;
            }
            ManifestMeta.PrereqIds.reserve(JsonPrereqIdsList->value.GetArray().Size());
            for (auto& JsonPreqreqId : JsonPrereqIdsList->value.GetArray()) {
                if (!JsonPreqreqId.IsString()) {
                    SetError(ErrorType::BadJson);
                    return;
                }
                ManifestMeta.PrereqIds.emplace_back(JsonPreqreqId.GetString(), JsonPreqreqId.GetStringLength());
            }
        }
        else {
            std::string PrereqFilename = ManifestMeta.PrereqPath;
            std::replace(PrereqFilename.begin(), PrereqFilename.end(), '\\', '/');
            auto PrereqFile = GetFile(PrereqFilename);
            if (PrereqFile) {
                ManifestMeta.PrereqIds.emplace_back(Utils::ToHex<true>(PrereqFile->FileHash));
            }
        }

        auto JsonDataGroupList = Json.FindMember("DataGroupList");
        if (JsonDataGroupList != Json.MemberEnd()) {
            if (!JsonDataGroupList->value.IsObject()) {
                SetError(ErrorType::BadJson);
                return;
            }
            for (auto& JsonDataGroup : JsonDataGroupList->value.GetObject()) {
                Utils::Guid TargetGuid;
                if (!TryGetGuid(JsonDataGroup.name, TargetGuid)) {
                    SetError(ErrorType::BadJson);
                    return;
                }

                auto Itr = ChunkMap.find(TargetGuid);
                if (Itr != ChunkMap.end()) {
                    if (!TryGetStringBlob(JsonDataGroup.value, Itr->second.GroupNumber)) {
                        SetError(ErrorType::BadJson);
                        return;
                    }
                }
            }
        }
        else {
            for (auto& Chunk : ChunkMap) {
                // Deprecated Crc32!
                Chunk.second.GroupNumber = Utils::Crc32<false, true>((char*)&Chunk.second.Guid, 16) % 100;
            }
        }

        bool HasChunkFilesizeList = false;
        auto JsonChunkFilesizeList = Json.FindMember("ChunkFilesizeList");
        if (JsonChunkFilesizeList != Json.MemberEnd()) {
            if (!JsonChunkFilesizeList->value.IsObject()) {
                SetError(ErrorType::BadJson);
                return;
            }
            for (auto& JsonChunkFilesize : JsonChunkFilesizeList->value.GetObject()) {
                Utils::Guid TargetGuid;
                if (!TryGetGuid(JsonChunkFilesize.name, TargetGuid)) {
                    SetError(ErrorType::BadJson);
                    return;
                }

                auto Itr = ChunkMap.find(TargetGuid);
                if (Itr != ChunkMap.end()) {
                    if (!TryGetStringBlob(JsonChunkFilesize.value, Itr->second.FileSize)) {
                        SetError(ErrorType::BadJson);
                        return;
                    }
                    HasChunkFilesizeList = true;
                }
            }
        }
        if (!HasChunkFilesizeList) {
            for (auto& Chunk : ChunkMap) {
                // We don't save chunks compressed yet, assume all chunks are exactly 1 MB
                Chunk.second.FileSize = 1 << 20;
            }
        }

        if (!TryGet<bool>(Json, "bIsFileData", ManifestMeta.IsFileData)) {
            ManifestMeta.IsFileData = !HasChunkHashList;
        }

        auto JsonCustomFields = Json.FindMember("CustomFields");
        if (JsonCustomFields != Json.MemberEnd()) {
            if (!JsonCustomFields->value.IsObject()) {
                SetError(ErrorType::BadJson);
                return;
            }
            for (auto& JsonCustomField : JsonCustomFields->value.GetObject()) {
                std::string Key, Val;
                if (!TryGetString(JsonCustomField.name, Key) || !TryGetString(JsonCustomField.value, Val)) {
                    SetError(ErrorType::BadJson);
                    return;
                }
                CustomFields.Fields.emplace(Key, Val);
            }
        }

        ChunkDataList.ChunkList.reserve(ChunkMap.size());
        for (auto& Chunk : ChunkMap) {
            ChunkDataList.ChunkList.emplace_back(std::move(Chunk.second));
        }

        ManifestMeta.BuildId = ManifestMeta.GetBackwardsCompatibleBuildId();
    }

    void Manifest::SetError(ErrorType NewError)
    {
        Error = NewError;
    }

    Stream& operator>>(Stream& Stream, Manifest& Val)
    {
        // Check if JSON and parse accordingly
        // Not the best speed if Stream is a FileStream since rapidjson loves caling JsonWrapperStream's Tell which in turn calls fseek/ftell
        {
            char PeekChar;
            Stream.read(&PeekChar, 1);
            Stream.seek(-1, Stream::Cur);
            if (PeekChar == '{') {
                rapidjson::Document Json;
                Utils::JsonWrapperStream JsonStream(Stream);
                Json.ParseStream(JsonStream);

                Val.ReadFromJson(Json);
                return Stream;
            }
        }

        // Read header
        ManifestHeader Header;
        Stream >> Header;

        if (Header.Magic != ManifestHeader::ExpectedMagic) {
            Val.SetError(Manifest::ErrorType::InvalidMagic);
            return Stream;
        }

        if (Header.Version < FeatureLevel::StoredAsBinaryData) {
            Val.SetError(Manifest::ErrorType::TooOld);
            return Stream;
        }

        // Read data
        std::unique_ptr<char[]> ManifestData = std::make_unique<char[]>(Header.DataSizeCompressed);
        Stream.read(ManifestData.get(), Header.DataSizeCompressed);

        // Decompress data if necessary
        if (Header.IsCompressed()) {
            // Move the current data to a compressed buffer, and decompress it back to the old variable
            std::unique_ptr<char[]> CompressedData = std::move(ManifestData);
            ManifestData = std::make_unique<char[]>(Header.DataSizeUncompressed);

            bool DecompSuccess = Utils::Compressor<Utils::CompressionMethod::Zlib>::Decompress(
                ManifestData.get(), Header.DataSizeUncompressed,
                CompressedData.get(), Header.DataSizeCompressed
            );

            if (!DecompSuccess) {
                Val.SetError(Manifest::ErrorType::BadCompression);
                return Stream;
            }
        }

        // Verify data integrity
        if (!Utils::SHA1Verify(ManifestData.get(), Header.DataSizeUncompressed, Header.SHAHash)) {
            Val.SetError(Manifest::ErrorType::BadHash);
            return Stream;
        }

        // Deserialize data
        UEStream DataStream(ManifestData.get(), Header.DataSizeUncompressed);
        DataStream >> Val.ManifestMeta;
        DataStream >> Val.ChunkDataList;
        DataStream >> Val.FileManifestList;
        DataStream >> Val.CustomFields;

        Val.SetError(Manifest::ErrorType::Success);
        return Stream;
    }
}