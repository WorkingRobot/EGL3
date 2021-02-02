#pragma once

#include "../game/Archive.h"

#include <filesystem>
#include <memory>
#include <variant>

namespace EGL3::Storage::Models {
    class InstalledGame {
        struct MetadataInfo {
            MetadataInfo(const Storage::Game::Archive& Archive) :
                Valid(Archive.IsValid()),
                Header(Valid ? *Archive.GetHeader().Get() : Storage::Game::Header{})
            {

            }

            bool Valid;
            Storage::Game::Header Header;
        };

    public:
        InstalledGame(const std::filesystem::path& Path) :
            Path(Path)
        {

        }

        bool IsValid(bool ForceRefresh = false) {
            EnsureData(ForceRefresh);

            return std::visit([](auto&& Arg) -> bool {
                using T = std::decay_t<decltype(Arg)>;
                if constexpr (std::is_same_v<T, Storage::Game::Archive>)
                    return Arg.IsValid();
                else if constexpr (std::is_same_v<T, MetadataInfo>)
                    return Arg.Valid;
                else
                    return false;
            }, Data);
        }

        const Storage::Game::Header* GetHeader(bool ForceRefresh = false) {
            if (!IsValid(ForceRefresh)) {
                return nullptr;
            }

            return std::visit([](auto&& Arg) -> const Storage::Game::Header* {
                using T = std::decay_t<decltype(Arg)>;
                if constexpr (std::is_same_v<T, Storage::Game::Archive>)
                    return Arg.GetHeader().Get();
                else if constexpr (std::is_same_v<T, MetadataInfo>)
                    return &Arg.Header;
                else
                    return nullptr;
            }, Data);
        }

    private:
        void EnsureData(bool ForceRefresh) {
            if (std::holds_alternative<std::monostate>(Data) || (ForceRefresh && std::holds_alternative<MetadataInfo>(Data))) {
                Storage::Game::Archive Archive(Path, Storage::Game::ArchiveMode::Read);
                Data.emplace<MetadataInfo>(Archive);
            }
        }

        std::filesystem::path Path;
        std::variant<std::monostate, Storage::Game::Archive, MetadataInfo> Data;
    };
}