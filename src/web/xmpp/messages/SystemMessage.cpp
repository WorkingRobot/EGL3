#include "SystemMessage.h"

namespace EGL3::Web::Xmpp::Messages {
	SystemMessage::SystemMessage(FriendshipRequest&& Message, const std::string& ClientAccountId) {
		if (Message.From == ClientAccountId) {
			AccountId = std::move(Message.To);
			if (Message.Status == FriendshipRequest::StatusEnum::ACCEPTED) {
				Action = ActionType::RequestAccept;
			}
			else if (Message.Status == FriendshipRequest::StatusEnum::PENDING) {
				Action = ActionType::RequestOutbound;
			}
		}
		else {
			AccountId = std::move(Message.From);
			if (Message.Status == FriendshipRequest::StatusEnum::ACCEPTED) {
				Action = ActionType::RequestAccept;
			}
			else if (Message.Status == FriendshipRequest::StatusEnum::PENDING) {
				Action = ActionType::RequestInbound;
			}
		}
	}

	SystemMessage::SystemMessage(FriendshipRemove&& Message, const std::string& ClientAccountId) {
		if (Message.From == ClientAccountId) {
			AccountId = std::move(Message.To);
		}
		else {
			AccountId = std::move(Message.From);
		}
		Action = ActionType::Remove;
	}

	SystemMessage::SystemMessage(UserBlocklistUpdate&& Message) {
		if (Message.Status == UserBlocklistUpdate::StatusEnum::BLOCKED) {
			Action = ActionType::Block;
		}
		else if (Message.Status == UserBlocklistUpdate::StatusEnum::UNBLOCKED) {
			Action = ActionType::Unblock;
		}
		AccountId = std::move(Message.Account);
	}

	SystemMessage::ActionType SystemMessage::GetAction() const {
		return Action;
	}

	const std::string& SystemMessage::GetAccountId() const {
		return AccountId;
	}
}
