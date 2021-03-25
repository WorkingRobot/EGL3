#include "Options.h"

namespace EGL3::Modules::Friends {
    using namespace Storage::Models;

    OptionsModule::OptionsModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        StorageData(Storage.Get(Storage::Persistent::Key::StoredFriendData)),
        CheckFriendsOffline(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOfflineCheck")),
        CheckFriendsOutgoing(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOutgoingCheck")),
        CheckFriendsIncoming(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsIncomingCheck")),
        CheckFriendsBlocked(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsBlockedCheck")),
        CheckFriendsOverride(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOverrideCheck")),
        CheckDeclineReqs(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsDeclineReqsCheck")),
        CheckProfanity(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsProfanityCheck"))
    {
        CheckFriendsOffline.set_active(StorageData.HasFlag<StoredFriendData::ShowOffline>());
        CheckFriendsOutgoing.set_active(StorageData.HasFlag<StoredFriendData::ShowOutgoing>());
        CheckFriendsIncoming.set_active(StorageData.HasFlag<StoredFriendData::ShowIncoming>());
        CheckFriendsBlocked.set_active(StorageData.HasFlag<StoredFriendData::ShowBlocked>());
        CheckFriendsOverride.set_active(StorageData.HasFlag<StoredFriendData::ShowOverride>());
        CheckDeclineReqs.set_active(StorageData.HasFlag<StoredFriendData::AutoDeclineReqs>());
        CheckProfanity.set_active(StorageData.HasFlag<StoredFriendData::CensorProfanity>());

        CheckFriendsOffline.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsOutgoing.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsIncoming.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsBlocked.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsOverride.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckDeclineReqs.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckProfanity.signal_toggled().connect([this]() { UpdateSelection(); });
    }

    const StoredFriendData& OptionsModule::GetStorageData() const
    {
        return StorageData;
    }

    StoredFriendData& OptionsModule::GetStorageData()
    {
        return StorageData;
    }

    void OptionsModule::UpdateSelection() {
        StorageData.SetFlags(StoredFriendData::OptionFlags(
            (CheckFriendsOffline.get_active() ? (uint8_t)StoredFriendData::ShowOffline : 0) |
            (CheckFriendsOutgoing.get_active() ? (uint8_t)StoredFriendData::ShowOutgoing : 0) |
            (CheckFriendsIncoming.get_active() ? (uint8_t)StoredFriendData::ShowIncoming : 0) |
            (CheckFriendsBlocked.get_active() ? (uint8_t)StoredFriendData::ShowBlocked : 0) |
            (CheckFriendsOverride.get_active() ? (uint8_t)StoredFriendData::ShowOverride : 0) |
            (CheckDeclineReqs.get_active() ? (uint8_t)StoredFriendData::AutoDeclineReqs : 0) |
            (CheckProfanity.get_active() ? (uint8_t)StoredFriendData::CensorProfanity : 0)
        ));

        OnUpdate();
    }
}