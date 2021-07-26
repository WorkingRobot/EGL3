#pragma once

#include "../../storage/models/ChatConversation.h"
#include "../../storage/models/Friend.h"
#include "../../utils/Callback.h"
#include "../../utils/DataDispatcher.h"
#include "../../utils/SlotHolder.h"
#include "../../widgets/ChatBubble.h"
#include "../../widgets/FriendList.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../ImageCache.h"
#include "../SysTray.h"

#include <gtkmm.h>

namespace EGL3::Modules::Friends {
    class ChatModule : public BaseModule {
    public:
        ChatModule(ModuleList& Ctx);

        void SetUser(Storage::Models::Friend& Friend);

        void ClearUser();

        void RecieveChatMessage(const std::string& AccountId, const std::string& NewMessage);

        Utils::Callback<void(const std::string& AccountId, const std::string& Content)> SendChatMessage;
        Utils::Callback<std::string(const std::string& AccountId)> RequestDisplayName;
        Utils::Callback<void(const std::string& AccountId)> OpenChatPage;

    private:
        void AddMessage(Storage::Models::ChatMessage& Message);

        void OnSendMessageClicked();

        void OnRecieveChatMessage(const std::string& AccountId, const std::string& NewMessage, bool Recieved);

        Storage::Models::ChatConversation& GetOrCreateConversation(const std::string& AccountId);

        ImageCacheModule& ImageCache;
        AsyncFFModule& AsyncFF;
        SysTrayModule& SysTray;

        Gtk::ScrolledWindow& ChatScrollWindow;
        Gtk::Box& ChatBox;

        Gtk::EventBox& ChatEntryContainer;
        Gtk::Entry& ChatEntry;

        Widgets::FriendList SelectedUserList;
        Storage::Models::Friend* SelectedUserModel;

        Utils::SlotHolder SlotSendKeybind;
        Utils::SlotHolder SlotAutoScroll;

        Utils::DataQueueDispatcher<std::reference_wrapper<Storage::Models::ChatMessage>> NewChatDispatcher;
        std::vector<std::unique_ptr<Widgets::ChatBubble>> ChatBubbles;

        std::deque<Storage::Models::ChatConversation> Conversations;
    };
}