#pragma once

#include "../../storage/models/StoredFriendData.h"
#include "../../utils/Callback.h"
#include "../../utils/SlotHolder.h"
#include "../ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules::Friends {
    class OptionsModule : public BaseModule {
    public:
        OptionsModule(ModuleList& Ctx);

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
        Gtk::CheckMenuItem& CheckFriendsOverride;
        Gtk::CheckMenuItem& CheckDeclineReqs;
        Gtk::CheckMenuItem& CheckProfanity;

        Utils::SlotHolder SlotFriendsOffline;
        Utils::SlotHolder SlotFriendsOutgoing;
        Utils::SlotHolder SlotFriendsIncoming;
        Utils::SlotHolder SlotFriendsBlocked;
        Utils::SlotHolder SlotFriendsOverride;
        Utils::SlotHolder SlotDeclineReqs;
        Utils::SlotHolder SlotProfanity;
    };
}