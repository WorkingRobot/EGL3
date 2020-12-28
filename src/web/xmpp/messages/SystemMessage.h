#pragma once

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
			Unblock
		};

	private:
		ActionType Action;
		std::string AccountId;

	public:
		SystemMessage(FriendshipRequest&& Message, const std::string& ClientAccountId);

		SystemMessage(FriendshipRemove&& Message, const std::string& ClientAccountId);

		SystemMessage(UserBlocklistUpdate&& Message);

		ActionType GetAction() const;

		const std::string& GetAccountId() const;
	};
}
