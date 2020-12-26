#pragma once

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
		SystemMessage(FriendshipRequest&& Message, const std::string& ClientAccountId) {
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

		SystemMessage(FriendshipRemove&& Message, const std::string& ClientAccountId) {
			if (Message.From == ClientAccountId) {
				AccountId = std::move(Message.To);
			}
			else {
				AccountId = std::move(Message.From);
			}
			Action = ActionType::Remove;
		}

		SystemMessage(UserBlocklistUpdate&& Message) {
			if (Message.Status == UserBlocklistUpdate::StatusEnum::BLOCKED) {
				Action = ActionType::Block;
			}
			else if (Message.Status == UserBlocklistUpdate::StatusEnum::UNBLOCKED) {
				Action = ActionType::Unblock;
			}
			AccountId = std::move(Message.Account);
		}

		ActionType GetAction() const {
			return Action;
		}

		const std::string& GetAccountId() const {
			return AccountId;
		}
	};
}
