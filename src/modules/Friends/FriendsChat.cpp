#include "FriendsChat.h"

namespace EGL3::Modules {
    using namespace Storage::Models;

    FriendsChatModule::FriendsChatModule(ModuleList& Modules, const Utils::GladeBuilder& Builder) :
        ImageCache(Modules.GetModule<ImageCacheModule>()),
        AsyncFF(Modules.GetModule<AsyncFFModule>()),
        ChatScrollWindow(Builder.GetWidget<Gtk::ScrolledWindow>("FriendsChatScrollWindow")),
        ChatBox(Builder.GetWidget<Gtk::ListBox>("FriendsChatBox")),
        ChatEntryContainer(Builder.GetWidget<Gtk::EventBox>("FriendsChatEntryContainer")),
        ChatEntry(Builder.GetWidget<Gtk::Entry>("FriendsChatEntry")),
        ChatSendBtn(Builder.GetWidget<Gtk::Button>("FriendsChatSendBtn")),
        SelectedFriendContainer(Builder.GetWidget<Gtk::Box>("FriendsChatSelectedUserContainer")),
        SelectedFriendWidget(ImageCache),
        SelectedFriend(nullptr)
    {
        SelectedFriendContainer.pack_start(SelectedFriendWidget, true, true);

        ChatSendBtn.signal_clicked().connect([this]() { OnSendMessageClicked(); });
        ChatEntryContainer.set_events(Gdk::KEY_PRESS_MASK);
        ChatEntry.signal_key_press_event().connect([this](GdkEventKey* evt) {
            // https://gitlab.gnome.org/GNOME/gtk/blob/master/gdk/gdkkeysyms.h
            // Enter/Return isn't pressed or shift is pressed
            if (evt->keyval != GDK_KEY_Return || evt->state & GdkModifierType::GDK_SHIFT_MASK) {
                return false;
            }
            ChatSendBtn.clicked();
            return true;
        }, false);

        // scroll to the bottom
        ChatBox.signal_size_allocate().connect([this](auto& Alloc) {
            auto Adj = ChatScrollWindow.get_vadjustment();
            Adj->set_value(Adj->get_upper());
        });

        NewChatDispatcher.connect([this]() { OnNewChatUpdate(); });
    }

    void FriendsChatModule::SetUser(const Friend& Friend)
    {
        if (!EGL3_CONDITIONAL_LOG(!SelectedFriend, LogLevel::Warning, "Trying to set selected friend before clearing. Clearing now.")) {
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

    void FriendsChatModule::ClearUser()
    {
        if (!SelectedFriend) {
            return;
        }

        SelectedFriendUpdateConn->disconnect();
        SelectedFriend = nullptr;
    }

    void FriendsChatModule::RecieveChatMessage(const std::string& AccountId, std::string&& NewMessage)
    {
        OnRecieveChatMessage(AccountId, std::forward<std::string>(NewMessage), true);
    }

    void FriendsChatModule::OnSelectedFriendUpdate()
    {
        SelectedFriendWidget.Update();
        ChatEntry.set_placeholder_text(Utils::Format("Message %s", SelectedFriend->Get().GetDisplayName().c_str()));
    }

    void FriendsChatModule::OnNewChatUpdate()
    {
        std::lock_guard Guard(NewChatMutex);

        for (auto& NewChat : NewChatData) {
            auto& Widget = ChatBubbles.emplace_back(std::make_unique<Widgets::ChatBubble>(NewChat.get()));
            ChatBox.add(*Widget);
        }

        NewChatData.clear();
    }

    void FriendsChatModule::OnSendMessageClicked()
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

    void FriendsChatModule::OnRecieveChatMessage(const std::string& AccountId, std::string&& NewMessage, bool Recieved)
    {
        auto& Message = GetOrCreateConversation(AccountId).Messages.emplace_back(ChatMessage{ .Content = NewMessage, .Time = std::chrono::system_clock::now(), .Recieved = Recieved });

        if (SelectedFriend && AccountId == SelectedFriend->Get().GetAccountId()) {
            std::lock_guard Lock(NewChatMutex);
            NewChatData.emplace_back(std::ref(Message));

            NewChatDispatcher.emit();
        }
    }

    ChatConversation& FriendsChatModule::GetOrCreateConversation(const std::string& AccountId)
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