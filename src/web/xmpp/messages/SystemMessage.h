#pragma once

#include "FriendshipEntryUpdate.h"
#include "FriendshipRemove.h"
#include "FriendshipRequest.h"
#include "UserBlocklistUpdate.h"

namespace EGL3::Web::Xmpp::Messages {
	class SystemMessage {
	public:
		enum class ActionType : uint8_t {
			Unknown,
			RequestAccept,
			RequestInbound,
			RequestOutbound,
			Remove,
			Block,
			Unblock,
			Update
		};

	private:
		ActionType Action;
		std::string AccountId;

	public:
		SystemMessage(FriendshipRequest&& Message, const std::string& ClientAccountId);

		SystemMessage(FriendshipRemove&& Message, const std::string& ClientAccountId);

		SystemMessage(FriendshipEntryUpdate&& Message);

		SystemMessage(UserBlocklistUpdate&& Message);

		ActionType GetAction() const;

		const std::string& GetAccountId() const;
	};
}
