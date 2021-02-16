#pragma once

#include "../../utils/Callback.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "InstalledGame.h"
#include "MountedDisk.h"

#include <future>
#include <sigc++/sigc++.h>

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

        std::future<void> PrimaryTask;
    };
}