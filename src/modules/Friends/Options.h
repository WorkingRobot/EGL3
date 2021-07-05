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

        template<Storage::Models::StoredFriendData::OptionFlags Flag>
        bool HasFlag() const {
            return StorageData->HasFlag<Flag>();
        }

        Web::Xmpp::Status GetStatus() const;

        const std::string& GetStatusText() const;

        void SetStatus(Web::Xmpp::Status NewStatus);

        void SetStatusText(const std::string& NewStatusText);

        Utils::Callback<void()> OnUpdate;

    private:
        void UpdateSelection();

        using StoredFriendSetting = Storage::Persistent::Setting<Utils::Crc32("StoredFriendData"), Storage::Models::StoredFriendData>;
        Storage::Persistent::SettingHolder<StoredFriendSetting> StorageData;

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