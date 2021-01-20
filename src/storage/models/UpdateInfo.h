#pragma once

#include "../../utils/TaskPool.h"
#include "../../web/epic/bps/Manifest.h"
#include "../game/Archive.h"

namespace EGL3::Storage::Models {
    class UpdateInfo {
    public:
        UpdateInfo(Game::Archive&& Archive, Web::Epic::BPS::Manifest&& Manifest, size_t TaskCount) :
            Archive(std::move(Archive)),
            Manifest(std::move(Manifest)),
            TaskPool(TaskCount)
        {

        }

    private:
        Game::Archive Archive;
        Web::Epic::BPS::Manifest Manifest;
        Utils::TaskPool TaskPool;
    };
}