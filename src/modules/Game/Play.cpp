#include "Play.h"

namespace EGL3::Modules::Game {
    using namespace Storage::Models;

    PlayModule::PlayModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        Storage(Storage),
        Auth(Modules.GetModule<AuthorizationModule>())
    {
        std::vector<MountedFile> Files
        {
            { "test folder", true, 0, -1, (void*)0 },
            { "testfile.txt", false, 1024 * 1024 * 1024 * 1023llu, 0, (void*)1 },
            { "test.folder", true, 0, 0, (void*)2 },
        };
        printf("emplacing\n");
        Disk.emplace(Files);
        printf("initializing\n");
        Disk->Initialize();
        printf("creating\n");
        Disk->Create();
        printf("mounting\n");
        Disk->Mount(-1);
        printf("waiting\n");
        std::promise<void>().get_future().wait();
    }

    PlayInfo& PlayModule::OnPlayClicked(const InstalledGame& Game)
    {
        CurrentPlay = std::make_unique<PlayInfo>(Game);

        return *CurrentPlay;
    }
}