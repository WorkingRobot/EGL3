#include "Chat.h"

namespace EGL3::Modules::Friends {
    using namespace Storage::Models;

    ChatModule::ChatModule(ModuleList& Ctx) :
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        ChatScrollWindow(Ctx.GetWidget<Gtk::ScrolledWindow>("FriendsChatScrollWindow")),
        ChatBox(Ctx.GetWidget<Gtk::Box>("FriendsChatBox")),
        ChatEntryContainer(Ctx.GetWidget<Gtk::EventBox>("FriendsChatEntryContainer")),
        ChatEntry(Ctx.GetWidget<Gtk::Entry>("FriendsChatEntry")),
        SelectedFriendContainer(Ctx.GetWidget<Gtk::Box>("FriendsChatSelectedUserContainer")),
        SelectedFriendWidget(ImageCache),
        SelectedFriend(nullptr)
    {
        SelectedFriendContainer.pack_start(SelectedFriendWidget, true, true);

        ChatEntryContainer.set_events(Gdk::KEY_PRESS_MASK);
        ChatEntry.signal_key_press_event().connect([this](GdkEventKey* evt) {
            // https://gitlab.gnome.org/GNOME/gtk/blob/master/gdk/gdkkeysyms.h
            if (evt->keyval != GDK_KEY_Return) {
                return false;
            }
            OnSendMessageClicked();
            return true;
        }, false);

        // scroll to the bottom
        ChatBox.signal_size_allocate().connect([this](auto& Alloc) {
            auto Adj = ChatScrollWindow.get_vadjustment();
            Adj->set_value(Adj->get_upper());
        });

        NewChatDispatcher.connect([this]() { OnNewChatUpdate(); });
    }

    void ChatModule::SetUser(const Friend& Friend)
    {
        if (!EGL3_ENSURE(!SelectedFriend, LogLevel::Warning, "Trying to set selected friend before clearing. Clearing now.")) {
            ClearUser();
        }

        SelectedFriend = &Friend;

        SelectedFriendUpdateConn = Friend.Get().OnUpdate.connect([this]() { OnSelectedFriendUpdate(); });
        SelectedFriendWidget.SetData(Friend);

        {
            auto& Conv = GetOrCreateConversation(Friend.Get().GetAccountId());

            ChatBox.foreach([this](Gtk::Widget& Widget) { ChatBox.remove(Widget); });

            ChatBubbles.clear();
            ChatBubbles.reserve(Conv.Messages.size());
            for (auto& Message : Conv.Messages) {
                auto& Widget = ChatBubbles.emplace_back(std::make_unique<Widgets::ChatBubble>(Message));
                ChatBox.add(*Widget);
            }

            ChatBox.show_all_children();
        }

        OnSelectedFriendUpdate();
    }

    void ChatModule::ClearUser()
    {
        if (!SelectedFriend) {
            return;
        }

        SelectedFriendUpdateConn->disconnect();
        SelectedFriend = nullptr;
    }

    void ChatModule::RecieveChatMessage(const std::string& AccountId, std::string&& NewMessage)
    {
        OnRecieveChatMessage(AccountId, std::forward<std::string>(NewMessage), true);
    }

    void ChatModule::OnSelectedFriendUpdate()
    {
        SelectedFriendWidget.Update();
        ChatEntry.set_placeholder_text(Utils::Format("Message %s", SelectedFriend->Get().GetDisplayName().c_str()));
    }

    void ChatModule::OnNewChatUpdate()
    {
        std::lock_guard Guard(NewChatMutex);

        for (auto& NewChat : NewChatData) {
            auto& Widget = ChatBubbles.emplace_back(std::make_unique<Widgets::ChatBubble>(NewChat.get()));
            ChatBox.pack_start(*Widget);
        }

        NewChatData.clear();
    }

    void ChatModule::OnSendMessageClicked()
    {
        auto Content = ChatEntry.get_text();
        ChatEntry.set_text("");
        if (Content.empty()) {
            return;
        }

        auto& AccountId = SelectedFriend->Get().GetAccountId();
        OnRecieveChatMessage(AccountId, Content, false);
        SendChatMessage(AccountId, Content);
    }

    void ChatModule::OnRecieveChatMessage(const std::string& AccountId, std::string&& NewMessage, bool Recieved)
    {
        auto& Message = GetOrCreateConversation(AccountId).Messages.emplace_back(ChatMessage{ .Content = NewMessage, .Time = std::chrono::utc_clock::now(), .Recieved = Recieved });

        if (SelectedFriend && AccountId == SelectedFriend->Get().GetAccountId()) {
            std::lock_guard Lock(NewChatMutex);
            NewChatData.emplace_back(std::ref(Message));

            NewChatDispatcher.emit();
        }
    }

    ChatConversation& ChatModule::GetOrCreateConversation(const std::string& AccountId)
    {
        auto Itr = std::find_if(Conversations.begin(), Conversations.end(), [&](const auto& Conv) {
            return Conv.AccountId == AccountId;
        });

        if (Itr != Conversations.end()) {
            return *Itr;
        }

        return Conversations.emplace_back(ChatConversation{.AccountId = AccountId });
    }
}