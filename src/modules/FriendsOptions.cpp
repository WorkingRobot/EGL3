#include "FriendsOptions.h"

namespace EGL3::Modules {
    using namespace Storage::Models;

    FriendsOptionsModule::FriendsOptionsModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        StorageData(Storage.Get(Storage::Persistent::Key::StoredFriendData)),
        CheckFriendsOffline(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOfflineCheck")),
        CheckFriendsOutgoing(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsOutgoingCheck")),
        CheckFriendsIncoming(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsIncomingCheck")),
        CheckFriendsBlocked(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsBlockedCheck")),
        CheckDeclineReqs(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsDeclineReqsCheck")),
        CheckProfanity(Builder.GetWidget<Gtk::CheckMenuItem>("FriendsProfanityCheck"))
    {
        CheckFriendsOffline.set_active(StorageData.HasFlag<StoredFriendData::ShowOffline>());
        CheckFriendsOutgoing.set_active(StorageData.HasFlag<StoredFriendData::ShowOutgoing>());
        CheckFriendsIncoming.set_active(StorageData.HasFlag<StoredFriendData::ShowIncoming>());
        CheckFriendsBlocked.set_active(StorageData.HasFlag<StoredFriendData::ShowBlocked>());
        CheckDeclineReqs.set_active(StorageData.HasFlag<StoredFriendData::AutoDeclineReqs>());
        CheckProfanity.set_active(StorageData.HasFlag<StoredFriendData::CensorProfanity>());

        CheckFriendsOffline.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsOutgoing.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsIncoming.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckFriendsBlocked.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckDeclineReqs.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckProfanity.signal_toggled().connect([this]() { UpdateSelection(); });
    }

    const StoredFriendData& FriendsOptionsModule::GetStorageData() const
    {
        return StorageData;
    }

    StoredFriendData& FriendsOptionsModule::GetStorageData()
    {
        return StorageData;
    }

    void FriendsOptionsModule::UpdateSelection() {
        StorageData.SetFlags(StoredFriendData::OptionFlags(
            (CheckFriendsOffline.get_active() ? (uint8_t)StoredFriendData::ShowOffline : 0) |
            (CheckFriendsOutgoing.get_active() ? (uint8_t)StoredFriendData::ShowOutgoing : 0) |
            (CheckFriendsIncoming.get_active() ? (uint8_t)StoredFriendData::ShowIncoming : 0) |
            (CheckFriendsBlocked.get_active() ? (uint8_t)StoredFriendData::ShowBlocked : 0) |
            (CheckDeclineReqs.get_active() ? (uint8_t)StoredFriendData::AutoDeclineReqs : 0) |
            (CheckProfanity.get_active() ? (uint8_t)StoredFriendData::CensorProfanity : 0)
        ));

        OnUpdate();
    }
}