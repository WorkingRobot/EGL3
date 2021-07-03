#include "StoredFriendData.h"

namespace EGL3::Storage::Models {
    StoredFriendData::StoredFriendData() :
        Options(OptionFlags(OptionFlags::ShowOutgoing | OptionFlags::ShowIncoming | OptionFlags::CensorProfanity)),
        Status(Web::Xmpp::Status::Online),
        StatusText("Using EGL3")
    {

    }

    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, StoredFriendData& Val) {
        Stream >> Val.Options;
        Stream >> Val.Status;
        Stream >> Val.StatusText;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const StoredFriendData& Val) {
        Stream << Val.Options;
        Stream << Val.Status;
        Stream << Val.StatusText;

        return Stream;
    }

    Web::Xmpp::Status StoredFriendData::GetStatus() const {
        return Status;
    }

    const std::string& StoredFriendData::GetStatusText() const {
        return StatusText;
    }

    void StoredFriendData::SetFlags(OptionFlags NewOptions) {
        Options = NewOptions;
    }

    void StoredFriendData::SetStatus(Web::Xmpp::Status NewStatus) {
        Status = NewStatus;
    }

    void StoredFriendData::SetStatusText(const std::string& NewStatusText) {
        StatusText = NewStatusText;
    }
}