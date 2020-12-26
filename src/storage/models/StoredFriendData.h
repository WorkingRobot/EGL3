#pragma once

#include "../../utils/streams/Stream.h"
#include "../../web/xmpp/ShowStatus.h"

namespace EGL3::Storage::Models {
    class StoredFriendData {
    public:
        enum OptionFlags : uint8_t {
            None            = 0x00,
            ShowOffline     = 0x01,
            ShowOutgoing    = 0x02,
            ShowIncoming    = 0x04,
            ShowBlocked     = 0x08,
            // Invalid1     = 0x10,
            // Invalid2     = 0x20,
            AutoDeclineReqs = 0x40,
            CensorProfanity = 0x80
        };

    private:
        OptionFlags Options;
        Web::Xmpp::Json::ShowStatus ShowStatus;
        std::string Status;

    public:
        StoredFriendData() :
            Options(OptionFlags(OptionFlags::ShowOffline | OptionFlags::ShowOutgoing | OptionFlags::ShowIncoming | OptionFlags::ShowBlocked | OptionFlags::CensorProfanity)),
            ShowStatus(Web::Xmpp::Json::ShowStatus::Online),
            Status("Using EGL3")
        {

        }

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, StoredFriendData& Val) {
            uint8_t Options;
            Stream >> Options;
            Val.Options = OptionFlags(Options);
            uint8_t ShowStatus;
            Stream >> ShowStatus;
            Val.ShowStatus = Web::Xmpp::Json::ShowStatus(ShowStatus);
            Stream >> Val.Status;

            return Stream;
        }

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const StoredFriendData& Val) {
            Stream << uint8_t(Val.Options);
            Stream << uint8_t(Val.ShowStatus);
            Stream << Val.Status;

            return Stream;
        }

        template<OptionFlags Flag>
        bool HasFlag() const {
            return Options & Flag;
        }

        Web::Xmpp::Json::ShowStatus GetShowStatus() const {
            return ShowStatus;
        }

        const std::string& GetStatus() const {
            return Status;
        }

        template<OptionFlags Flag>
        void SetFlag(bool Val) {
            Options = OptionFlags(Options ^ ((-int8_t(Val) ^ Options) & Flag));
        }

        void SetFlags(OptionFlags NewOptions) {
            Options = NewOptions;
        }

        void SetShowStatus(Web::Xmpp::Json::ShowStatus NewShowStatus) {
            ShowStatus = NewShowStatus;
        }

        void SetStatus(const std::string& NewStatus) {
            Status = NewStatus;
        }
    };
}