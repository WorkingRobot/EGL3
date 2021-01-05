#pragma once

#include "../storage/models/StoredFriendData.h"
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

        template<typename... Args>
        void SetUpdateCallback(Args&&... Callback) {
            OnUpdate.emplace(std::forward<Args>(Callback)...);
        }

        const Storage::Models::StoredFriendData& GetStorageData() const;

        Storage::Models::StoredFriendData& GetStorageData();

    private:
        void UpdateSelection();

        std::optional<std::function<void()>> OnUpdate;

        Storage::Models::StoredFriendData& StorageData;

        Gtk::CheckMenuItem& CheckFriendsOffline;
        Gtk::CheckMenuItem& CheckFriendsOutgoing;
        Gtk::CheckMenuItem& CheckFriendsIncoming;
        Gtk::CheckMenuItem& CheckFriendsBlocked;
        Gtk::CheckMenuItem& CheckDeclineReqs;
        Gtk::CheckMenuItem& CheckProfanity;
    };
}