#include "Presence.h"

namespace EGL3::Web::Xmpp::Json {
    std::weak_ordering Presence::operator<=>(const Presence& that) const {
        return Resource <=> that.Resource;
    }

    void Presence::Dump() const {
        printf("Resource: %s\nStatus: %d\nTime: %s\n",
            Resource.GetString().c_str(),
            (int)ShowStatus,
            GetTimePoint(LastUpdated).c_str()
        );
        Status.Dump();
        printf("\n");
    }
}
