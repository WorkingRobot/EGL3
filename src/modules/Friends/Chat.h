#pragma once

#include "../../storage/models/ChatConversation.h"
#include "../../storage/models/Friend.h"
#include "../../utils/Callback.h"
#include "../../utils/SlotHolder.h"
#include "../../widgets/ChatBubble.h"
#include "../../widgets/FriendList.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../ImageCache.h"

#include <gtkmm.h>

namespace EGL3::Modules::Friends {
    class ChatModule : public BaseModule {
    public:
        ChatModule(ModuleList& Ctx);

        void SetUser(Storage::Models::Friend& Friend);

        void ClearUser();

        void RecieveChatMessage(const std::string& AccountId, const std::string& NewMessage);

        Utils::Callback<void(const std::string& AccountId, const std::string& Content)> SendChatMessage;

    private:
        void OnNewChatUpdate();

        void OnSendMessageClicked();

        void OnRecieveChatMessage(const std::string& AccountId, const std::string& NewMessage, bool Recieved);

        Storage::Models::ChatConversation& GetOrCreateConversation(const std::string& AccountId);

        Modules::ImageCacheModule& ImageCache;
        Modules::AsyncFFModule& AsyncFF;

        Gtk::ScrolledWindow& ChatScrollWindow;
        Gtk::Box& ChatBox;

        Gtk::EventBox& ChatEntryContainer;
        Gtk::Entry& ChatEntry;

        Widgets::FriendList SelectedUserList;
        Storage::Models::Friend* SelectedUserModel;

        Utils::SlotHolder SlotSendKeybind;
        Utils::SlotHolder SlotAutoScroll;

        std::vector<std::unique_ptr<Widgets::ChatBubble>> ChatBubbles;
        std::mutex NewChatMutex;
        std::vector<std::reference_wrapper<Storage::Models::ChatMessage>> NewChatData;
        Glib::Dispatcher NewChatDispatcher;

        std::deque<Storage::Models::ChatConversation> Conversations;
    };
}