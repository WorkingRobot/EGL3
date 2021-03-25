#include "StoredFriendData.h"

namespace EGL3::Storage::Models {
    StoredFriendData::StoredFriendData() :
        Options(OptionFlags(OptionFlags::ShowOutgoing | OptionFlags::ShowIncoming | OptionFlags::CensorProfanity)),
        ShowStatus(Web::Xmpp::Json::ShowStatus::Online),
        Status("Using EGL3")
    {

    }

    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, StoredFriendData& Val) {
        uint8_t Options;
        Stream >> Options;
        Val.Options = StoredFriendData::OptionFlags(Options);
        uint8_t ShowStatus;
        Stream >> ShowStatus;
        Val.ShowStatus = Web::Xmpp::Json::ShowStatus(ShowStatus);
        Stream >> Val.Status;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const StoredFriendData& Val) {
        Stream << uint8_t(Val.Options);
        Stream << uint8_t(Val.ShowStatus);
        Stream << Val.Status;

        return Stream;
    }

    Web::Xmpp::Json::ShowStatus StoredFriendData::GetShowStatus() const {
        return ShowStatus;
    }

    const std::string& StoredFriendData::GetStatus() const {
        return Status;
    }

    void StoredFriendData::SetFlags(OptionFlags NewOptions) {
        Options = NewOptions;
    }

    void StoredFriendData::SetShowStatus(Web::Xmpp::Json::ShowStatus NewShowStatus) {
        ShowStatus = NewShowStatus;
    }

    void StoredFriendData::SetStatus(const std::string& NewStatus) {
        Status = NewStatus;
    }
}