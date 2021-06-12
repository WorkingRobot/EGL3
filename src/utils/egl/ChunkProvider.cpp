#include "ChunkProvider.h"

#include "../../modules/Game/GameInfo.h"
#include "../../web/epic/bps/Manifest.h"
#include "../../web/JsonParsing.h"
#include "../streams/FileStream.h"
#include "../HashCombine.h"
#include "../KnownFolders.h"
#include "../Platform.h"
#include "../SHA.h"

#include <numeric>

namespace EGL3::Utils::EGL {
    using namespace Web::Epic::BPS;

    struct Item {
        int FormatVersion;
        bool IsIncompleteInstall;
        std::string AppVersionString;
        std::string LaunchCommand;
        std::string LaunchExecutable;
        std::string ManifestLocation;
        bool IsApplication;
        bool IsExecutable;
        bool IsManaged;
        bool NeedsValidation;
        bool RequiresAuth;
        bool CanRunOffline;
        bool AllowMultipleInstances;
        std::string AppName;
        std::vector<std::string> BaseURLs;
        std::string BuildLabel;
        std::string CatalogItemId;
        std::string CatalogNamespace;
        std::vector<std::string> AppCategories;
        std::vector<std::string> ChunkDbs;
        std::vector<std::string> CompatibleApps;
        std::string DisplayName;
        std::string FullAppName;
        std::string InstallationGuid;
        std::string InstallLocation;
        std::string InstallSessionId;
        std::vector<std::string> InstallTags;
        std::vector<std::string> InstallComponents;
        std::string HostInstallationGuid;
        std::vector<std::string> PrereqIds;
        std::string StagingLocation;
        std::string TechnicalType;
        std::string VaultThumbnailUrl;
        std::string VaultTitleText;
        int64_t InstallSize;
        std::string MainWindowProcessName;
        std::vector<std::string> ProcessNames;
        std::unordered_map<std::string, bool> ExpectingDLCInstalled;
        std::unordered_map<std::string, bool> ExpectingModInstalled;
        std::string MainGameAppName;
        std::string MainGameCatalogNamespace;
        std::string MainGameCatalogItemId;
        std::string MandatoryAppFolderName;
        std::string OwnershipToken;

        PARSE_DEFINE(Item)
            PARSE_ITEM("FormatVersion", FormatVersion)
            PARSE_ITEM("bIsIncompleteInstall", IsIncompleteInstall)
            PARSE_ITEM("AppVersionString", AppVersionString)
            PARSE_ITEM("LaunchCommand", LaunchCommand)
            PARSE_ITEM("LaunchExecutable", LaunchExecutable)
            PARSE_ITEM("ManifestLocation", ManifestLocation)
            PARSE_ITEM("bIsApplication", IsApplication)
            PARSE_ITEM("bIsExecutable", IsExecutable)
            PARSE_ITEM("bIsManaged", IsManaged)
            PARSE_ITEM("bNeedsValidation", NeedsValidation)
            PARSE_ITEM("bRequiresAuth", RequiresAuth)
            PARSE_ITEM("bCanRunOffline", CanRunOffline)
            PARSE_ITEM_DEF("bAllowMultipleInstances", AllowMultipleInstances, false)
            PARSE_ITEM("AppName", AppName)
            PARSE_ITEM("BaseURLs", BaseURLs)
            PARSE_ITEM("BuildLabel", BuildLabel)
            PARSE_ITEM("CatalogItemId", CatalogItemId)
            PARSE_ITEM("CatalogNamespace", CatalogNamespace)
            PARSE_ITEM("AppCategories", AppCategories)
            PARSE_ITEM("ChunkDbs", ChunkDbs)
            PARSE_ITEM("CompatibleApps", CompatibleApps)
            PARSE_ITEM("DisplayName", DisplayName)
            PARSE_ITEM_OPT("FullAppName", FullAppName)
            PARSE_ITEM("InstallationGuid", InstallationGuid)
            PARSE_ITEM("InstallLocation", InstallLocation)
            PARSE_ITEM("InstallSessionId", InstallSessionId)
            PARSE_ITEM("InstallTags", InstallTags)
            PARSE_ITEM("InstallComponents", InstallComponents)
            PARSE_ITEM("HostInstallationGuid", HostInstallationGuid)
            PARSE_ITEM("PrereqIds", PrereqIds)
            PARSE_ITEM("StagingLocation", StagingLocation)
            PARSE_ITEM("TechnicalType", TechnicalType)
            PARSE_ITEM("VaultThumbnailUrl", VaultThumbnailUrl)
            PARSE_ITEM("VaultTitleText", VaultTitleText)
            PARSE_ITEM("InstallSize", InstallSize)
            PARSE_ITEM("MainWindowProcessName", MainWindowProcessName)
            PARSE_ITEM("ProcessNames", ProcessNames)
            PARSE_ITEM_OPT("ExpectingDLCInstalled", ExpectingDLCInstalled)
            PARSE_ITEM_OPT("ExpectingModInstalled", ExpectingModInstalled)
            PARSE_ITEM_OPT("MainGameAppName", MainGameAppName)
            PARSE_ITEM_OPT("MainGameCatalogNamespace", MainGameCatalogNamespace)
            PARSE_ITEM_OPT("MainGameCatalogItemId", MainGameCatalogItemId)
            PARSE_ITEM("MandatoryAppFolderName", MandatoryAppFolderName)
            PARSE_ITEM("OwnershipToken", OwnershipToken)
        PARSE_END
    };

    ChunkProvider::ChunkProvider(Storage::Game::GameId Id)
    {
        auto ItemFolder = GetInstalledItemFolderPath();
        if (ItemFolder.empty()) {
            return;
        }

        std::string CatalogItemId, AppName;
        EGL3_VERIFY(Modules::Game::GameInfoModule::GetCatalogInfo(Id, CatalogItemId, AppName), "Could not get catalog info");

        std::error_code Code;
        for (auto& Entry : std::filesystem::directory_iterator(ItemFolder, Code)) {
            if (!Entry.is_regular_file(Code) || Entry.path().extension() != ".item") {
                continue;
            }

            Streams::FileStream EntryStream;
            if (!EntryStream.open(Entry, "rb")) {
                continue;
            }

            auto EntryBuf = std::make_unique<char[]>(EntryStream.size());
            EntryStream.read(EntryBuf.get(), EntryStream.size());

            rapidjson::Document Json;
            Json.Parse(EntryBuf.get(), EntryStream.size());
            if (Json.HasParseError()) {
                continue;
            }

            Item InstallItem;
            if (!Item::Parse(Json, InstallItem)) {
                continue;
            }

            if (InstallItem.CatalogItemId != CatalogItemId || InstallItem.AppName != AppName) {
                continue;
            }

            std::filesystem::path ManifestPath = InstallItem.ManifestLocation;
            ManifestPath /= InstallItem.InstallationGuid;
            ManifestPath.replace_extension(".manifest");

            std::error_code Code;
            if (!std::filesystem::is_regular_file(ManifestPath, Code)) {
                continue;
            }

            Streams::FileStream ManifestStream;
            if (!ManifestStream.open(ManifestPath, "rb")) {
                continue;
            }

            Web::Epic::BPS::Manifest Manifest(ManifestStream);
            if (Manifest.HasError()) {
                continue;
            }

            InstallLocation = InstallItem.InstallLocation;

            SetupLUT(std::move(Manifest));
            return;
        }
    }

    bool ChunkProvider::IsValid() const
    {
        return !InstallLocation.empty();
    }

    const std::filesystem::path& ChunkProvider::GetInstallLocation() const
    {
        return InstallLocation;
    }

    bool ChunkProvider::IsChunkProbablyAvailable(const Utils::Guid& Guid) const
    {
        return LUT.find(Guid) != LUT.end();
    }

    std::unique_ptr<char[]> ChunkProvider::GetChunk(const Utils::Guid& Guid) const
    {
        auto ChunkDataItr = LUT.find(Guid);
        if (ChunkDataItr == LUT.end()) {
            return nullptr;
        }

        auto& ChunkData = ChunkDataItr->second;
        auto Data = std::make_unique<char[]>(ChunkData.Info.WindowSize);

        for (auto& Section : ChunkData.Sections) {
            Utils::Streams::FileStream Stream;
            if (!Stream.open(InstallLocation / *Section.File, "rb")) {
                return nullptr;
            }
            Stream.seek(Section.FileOffset, Utils::Streams::Stream::Beg);
            Stream.read(Data.get() + Section.ChunkOffset, Section.Size);
        }

        if (!Utils::SHA1Verify(Data.get(), ChunkData.Info.WindowSize, ChunkData.Info.SHAHash)) {
            return nullptr;
        }

        return Data;
    }

    std::filesystem::path ChunkProvider::GetInstalledItemFolderPath()
    {
        auto Path = Platform::GetKnownFolderPath(FOLDERID_ProgramData);
        if (Path.empty()) {
            return Path;
        }

        Path = Path / "Epic" / "EpicGamesLauncher" / "Data" / "Manifests";

        std::error_code Code;
        Path = std::filesystem::absolute(Path, Code);
        if (Code) {
            return "";
        }

        if (!std::filesystem::is_directory(Path, Code)) {
            return "";
        }
        return Path;
    }

    void ChunkProvider::SetupLUT(Web::Epic::BPS::Manifest&& Manifest)
    {
        LUT.reserve(Manifest.ChunkDataList.ChunkList.size());
        for (auto& Chunk : Manifest.ChunkDataList.ChunkList) {
            LUT.emplace(Chunk.Guid, ChunkData{ .Info = std::move(Chunk) });
        }

        auto& Files = Manifest.FileManifestList.FileList;
        Filenames.reserve(Files.size());
        for (auto& File : Files) {
            auto& Filename = Filenames.emplace_back(std::move(File.Filename));
            size_t FileOffset = 0;
            for (auto& Part : File.ChunkParts) {
                auto& ChunkData = LUT.at(Part.Guid);
                auto& Section = ChunkData.Sections.emplace_back(ChunkSection{ &Filename, FileOffset, Part.Offset, Part.Size });
                FileOffset += Part.Size;
            }
        }
    }
}
