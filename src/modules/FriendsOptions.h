#pragma once

#include "../storage/models/StoredFriendData.h"
#include "../utils/Callback.h"
#include "../utils/GladeBuilder.h"
#include "BaseModule.h"
#include "ModuleList.h"

#include <functional>
#include <gtkmm.h>
#include <optional>

namespace EGL3::Modules {
    class FriendsOptionsModule : public BaseModule {
    public:
        FriendsOptionsModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder);

        const Storage::Models::StoredFriendData& GetStorageData() const;

        Storage::Models::StoredFriendData& GetStorageData();

        Utils::Callback<void()> OnUpdate;

    private:
        void UpdateSelection();

        Storage::Models::StoredFriendData& StorageData;

        Gtk::CheckMenuItem& CheckFriendsOffline;
        Gtk::CheckMenuItem& CheckFriendsOutgoing;
        Gtk::CheckMenuItem& CheckFriendsIncoming;
        Gtk::CheckMenuItem& CheckFriendsBlocked;
        Gtk::CheckMenuItem& CheckDeclineReqs;
        Gtk::CheckMenuItem& CheckProfanity;
    };
}