#pragma once

#include "../../storage/models/ChatConversation.h"
#include "../../storage/models/Friend.h"
#include "../../utils/Callback.h"
#include "../../utils/GladeBuilder.h"
#include "../../widgets/ChatBubble.h"
#include "../../widgets/FriendItem.h"
#include "../BaseModule.h"
#include "../ModuleList.h"
#include "../AsyncFF.h"
#include "../ImageCache.h"

#include <gtkmm.h>

namespace EGL3::Modules::Friends {
    class ChatModule : public BaseModule {
    public:
        ChatModule(ModuleList& Modules, const Utils::GladeBuilder& Builder);

        void SetUser(const Storage::Models::Friend& Friend);

        void ClearUser();

        void RecieveChatMessage(const std::string& AccountId, std::string&& NewMessage);

        Utils::Callback<void(const std::string& AccountId, const std::string& Content)> SendChatMessage;

    private:
        void OnSelectedFriendUpdate();

        void OnNewChatUpdate();

        void OnSendMessageClicked();

        void OnRecieveChatMessage(const std::string& AccountId, std::string&& NewMessage, bool Recieved);

        Storage::Models::ChatConversation& GetOrCreateConversation(const std::string& AccountId);

        Modules::ImageCacheModule& ImageCache;
        Modules::AsyncFFModule& AsyncFF;

        Gtk::ScrolledWindow& ChatScrollWindow;
        Gtk::Box& ChatBox;

        Gtk::EventBox& ChatEntryContainer;
        Gtk::Entry& ChatEntry;

        const Storage::Models::Friend* SelectedFriend;
        Gtk::Box& SelectedFriendContainer;
        Widgets::FriendItem SelectedFriendWidget;
        sigc::signal<void()>::iterator SelectedFriendUpdateConn;

        std::vector<std::unique_ptr<Widgets::ChatBubble>> ChatBubbles;
        std::mutex NewChatMutex;
        std::vector<std::reference_wrapper<Storage::Models::ChatMessage>> NewChatData;
        Glib::Dispatcher NewChatDispatcher;

        std::deque<Storage::Models::ChatConversation> Conversations;
    };
}