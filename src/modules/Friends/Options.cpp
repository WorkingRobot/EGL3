#include "Options.h"

namespace EGL3::Modules::Friends {
    using namespace Storage::Models;

    using StoredFriendSetting = Storage::Persistent::Setting<Utils::Crc32("StoredFriendData"), StoredFriendData>;

    OptionsModule::OptionsModule(ModuleList& Ctx) :
        StorageData(Ctx.Get<StoredFriendSetting>()),
        CheckFriendsOffline(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsOfflineCheck")),
        CheckFriendsOutgoing(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsOutgoingCheck")),
        CheckFriendsIncoming(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsIncomingCheck")),
        CheckFriendsBlocked(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsBlockedCheck")),
        CheckFriendsOverride(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsOverrideCheck")),
        CheckDeclineReqs(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsDeclineReqsCheck")),
        CheckProfanity(Ctx.GetWidget<Gtk::CheckMenuItem>("FriendsProfanityCheck"))
    {
        CheckFriendsOffline.set_active(StorageData.HasFlag<StoredFriendData::ShowOffline>());
        CheckFriendsOutgoing.set_active(StorageData.HasFlag<StoredFriendData::ShowOutgoing>());
        CheckFriendsIncoming.set_active(StorageData.HasFlag<StoredFriendData::ShowIncoming>());
        CheckFriendsBlocked.set_active(StorageData.HasFlag<StoredFriendData::ShowBlocked>());
        CheckFriendsOverride.set_active(StorageData.HasFlag<StoredFriendData::ShowOverride>());
        CheckDeclineReqs.set_active(StorageData.HasFlag<StoredFriendData::AutoDeclineReqs>());
        CheckProfanity.set_active(StorageData.HasFlag<StoredFriendData::CensorProfanity>());

        SlotFriendsOffline = CheckFriendsOffline.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotFriendsOutgoing = CheckFriendsOutgoing.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotFriendsIncoming = CheckFriendsIncoming.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotFriendsBlocked = CheckFriendsBlocked.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotFriendsOverride = CheckFriendsOverride.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotDeclineReqs = CheckDeclineReqs.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotProfanity = CheckProfanity.signal_toggled().connect([this]() { UpdateSelection(); });
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