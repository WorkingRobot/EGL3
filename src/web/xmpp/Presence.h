#pragma once

#include "PresenceStatus.h"
#include "ResourceId.h"
#include "ShowStatus.h"

namespace EGL3::Web::Xmpp::Json {
    struct Presence {
        ResourceId Resource;

        ShowStatus ShowStatus;

        TimePoint LastUpdated;

        PresenceStatus Status;

        std::weak_ordering operator<=>(const Presence& that) const;

        void Dump() const;
    };
}
