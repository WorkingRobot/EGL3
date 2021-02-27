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
        Unknown,
        // Same order as Service::Pipe::MessageType
        Opening,
        Reading,
        Initializing,
        Creating,
        Starting,
        Mounting,

        Playable,
        Playing,
        Closed
    };

    class PlayInfo {
    public:
        PlayInfo(const InstalledGame& Game, Service::Pipe::Client& PipeClient);

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
        void OnPlay(Web::Epic::EpicClientAuthed& Client);

        const InstalledGame& Game;
        PlayInfoState CurrentState;

        Service::Pipe::Client& PipeClient;
        void* PipeContext;

        std::future<void> PrimaryTask;
    };
}