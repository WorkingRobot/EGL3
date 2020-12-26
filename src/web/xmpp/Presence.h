#pragma once

#include "ResourceId.h"
#include "ShowStatus.h"
#include "PresenceStatus.h"

namespace EGL3::Web::Xmpp::Json {
	struct Presence {
		ResourceId Resource;

		ShowStatus ShowStatus;

		TimePoint LastUpdated;

		PresenceStatus Status;

		std::weak_ordering operator<=>(const Presence& that) const {
			return Resource <=> that.Resource;
		}

		void Dump() const {
			printf("Resource: %s\nStatus: %d\nTime: %s\n",
				Resource.GetString().c_str(),
				(int)ShowStatus,
				GetTimePoint(LastUpdated).c_str()
			);
			Status.Dump();
			printf("\n");
		}
	};
}
