#pragma once

#include "../../srv/pipe/Client.h"
#include "../../utils/Callback.h"
#include "../../web/epic/EpicClientAuthed.h"
#include "../game/ArchiveList.h"
#include "InstalledGame.h"

#include <future>
#include <optional>
#include <sigc++/sigc++.h>
#include <variant>

namespace EGL3::Storage::Models {
    enum class PlayInfoState : uint8_t {
        Constructed,
        Mounting,
        Playable,
        Playing,
        Closed
    };

    class PlayInfo {
    public:
        PlayInfo(InstalledGame& Game);

        ~PlayInfo();

        PlayInfoState GetState() const;

        void SetState(PlayInfoState NewState);

        void Mount(Service::Pipe::Client& PipeClient);

        void Play(Web::Epic::EpicClientAuthed& Client);

        // TODO: Add PlayInfoStats callback and include playing data and stuff with it

        sigc::signal<void(PlayInfoState)> OnStateUpdate;

    private:
        void OnPlay(Web::Epic::EpicClientAuthed& Client);

        InstalledGame& Game;
        PlayInfoState CurrentState;

        void* ProcessHandle;

        std::future<void> PrimaryTask;
    };
}