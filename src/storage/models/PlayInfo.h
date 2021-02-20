#pragma once

#include "../../utils/Callback.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "../game/ArchiveList.h"
#include "InstalledGame.h"
#include "MountedDisk.h"

#include <future>
#include <optional>
#include <sigc++/sigc++.h>
#include <variant>

namespace EGL3::Storage::Models {
    enum class PlayInfoState : uint8_t {
        Reading,
        Initializing,
        Mounting,
        Playable,
        Playing,
        Closed
    };

    class PlayInfo {
    public:
        PlayInfo(const InstalledGame& Game);

        ~PlayInfo();

        void SetState(PlayInfoState NewState);

        void Begin();

        void Play(Web::Epic::EpicClientAuthed& Client);

        bool IsPlaying();

        void Close();

        Utils::Callback<void()> OnFinishPlaying;

        // TODO: Add PlayInfoStats callback and include playing data and stuff with it

        sigc::signal<void(PlayInfoState)> OnStateUpdate;

    private:
        void OnRead();

        void OnInitialize();

        void OnMount();

        void OnPlay(Web::Epic::EpicClientAuthed& Client);

        void OnClose();

        void HandleFileCluster(void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) const;

        const InstalledGame& Game;

        PlayInfoState CurrentState;

        std::optional<MountedDisk> Disk;

        struct Lists {
            const Game::ArchiveList<Game::RunlistId::File> Files;
            const Game::ArchiveList<Game::RunlistId::ChunkPart> ChunkParts;
            const Game::ArchiveList<Game::RunlistId::ChunkInfo> ChunkInfos;
            const Game::ArchiveList<Game::RunlistId::ChunkData> ChunkDatas;

            Lists(Game::Archive& Archive) :
                Files(Archive),
                ChunkParts(Archive),
                ChunkInfos(Archive),
                ChunkDatas(Archive)
            {

            }
        };

        std::optional<Lists> ArchiveLists;

#pragma pack(push, 4)
        struct SectionPart {
            uint64_t Ptr;
            uint32_t Size;
        };
        struct FileSection {
            SectionPart Sections[3];

            FileSection() :
                Sections{}
            {

            }

            uint32_t TotalSize() const {
                return Sections[0].Size + Sections[1].Size + Sections[2].Size;
            }
        };
#pragma pack(pop)
        std::vector<std::vector<FileSection>> SectionLUT;

        std::future<void> PrimaryTask;
    };
}