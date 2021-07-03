#pragma once

#include "../../utils/streams/Stream.h"
#include "../../web/xmpp/Status.h"

namespace EGL3::Storage::Models {
    class StoredFriendData {
    public:
        enum OptionFlags : uint8_t {
            None            = 0x00,
            ShowOffline     = 0x01,
            ShowOutgoing    = 0x02,
            ShowIncoming    = 0x04,
            ShowBlocked     = 0x08,
            ShowOverride    = 0x10,
            // Invalid2     = 0x20,
            AutoDeclineReqs = 0x40,
            CensorProfanity = 0x80
        };

    private:
        OptionFlags Options;
        Web::Xmpp::Status Status;
        std::string StatusText;

    public:
        StoredFriendData();

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, StoredFriendData& Val);

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const StoredFriendData& Val);

        template<OptionFlags Flag>
        bool HasFlag() const {
            return Options & Flag;
        }

        Web::Xmpp::Status GetStatus() const;

        const std::string& GetStatusText() const;

        void SetFlags(OptionFlags NewOptions);

        template<OptionFlags Flag>
        void SetFlag(bool Val) {
            Options = OptionFlags(Options ^ ((-int8_t(Val) ^ Options) & Flag));
        }

        void SetStatus(Web::Xmpp::Status NewStatus);

        void SetStatusText(const std::string& NewStatusText);
    };
}