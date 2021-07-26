#include "Chat.h"

namespace EGL3::Modules::Friends {
    using namespace Storage::Models;

    ChatModule::ChatModule(ModuleList& Ctx) :
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        AsyncFF(Ctx.GetModule<AsyncFFModule>()),
        SysTray(Ctx.GetModule<SysTrayModule>()),
        ChatScrollWindow(Ctx.GetWidget<Gtk::ScrolledWindow>("FriendsChatScrollWindow")),
        ChatBox(Ctx.GetWidget<Gtk::Box>("FriendsChatBox")),
        ChatEntryContainer(Ctx.GetWidget<Gtk::EventBox>("FriendsChatEntryContainer")),
        ChatEntry(Ctx.GetWidget<Gtk::Entry>("FriendsChatEntry")),
        SelectedUserList(Ctx.GetWidget<Gtk::TreeView>("FriendsChatTree"), ImageCache),
        SelectedUserModel(nullptr),
        RequestDisplayName([](const auto&) { return ""; })
    {
        ChatEntryContainer.set_events(Gdk::KEY_PRESS_MASK);
        SlotSendKeybind = ChatEntry.signal_key_press_event().connect([this](GdkEventKey* evt) {
            // https://gitlab.gnome.org/GNOME/gtk/blob/master/gdk/gdkkeysyms.h
            if (evt->keyval != GDK_KEY_Return) {
                return false;
            }
            OnSendMessageClicked();
            return true;
        }, false);

        // scroll to the bottom
        SlotAutoScroll = ChatBox.signal_size_allocate().connect([this](auto& Alloc) {
            auto Adj = ChatScrollWindow.get_vadjustment();
            Adj->set_value(Adj->get_upper());
        });

        NewChatDispatcher.connect([this](Storage::Models::ChatMessage& NewChat) {
            AddMessage(NewChat);
        });
    }

    void ChatModule::SetUser(Friend& Friend)
    {
        if (!SelectedUserModel) {
            ClearUser();
        }

        SelectedUserModel = &Friend;

        SelectedUserList.Add(Friend);

        {
            auto& Conv = GetOrCreateConversation(Friend.Get().GetAccountId());

            ChatBox.foreach([this](Gtk::Widget& Widget) { ChatBox.remove(Widget); });

            ChatBubbles.clear();
            ChatBubbles.reserve(Conv.Messages.size());
            for (auto& Message : Conv.Messages) {
                AddMessage(Message);
            }

            ChatBox.show_all_children();
        }

        ChatEntry.set_placeholder_text(std::format("Message {}", Friend.Get().GetDisplayName()));
    }

    void ChatModule::ClearUser()
    {
        SelectedUserList.Clear();
        SelectedUserModel = nullptr;
    }

    void ChatModule::RecieveChatMessage(const std::string& AccountId, const std::string& NewMessage)
    {
        OnRecieveChatMessage(AccountId, NewMessage, true);
    }

    void ChatModule::AddMessage(Storage::Models::ChatMessage& Message)
    {
        auto& Widget = ChatBubbles.emplace_back(std::make_unique<Widgets::ChatBubble>(Message));
        ChatBox.add(*Widget);
    }

    void ChatModule::OnSendMessageClicked()
    {
        auto Content = ChatEntry.get_text();
        ChatEntry.set_text("");
        if (Content.empty()) {
            return;
        }

        auto& AccountId = SelectedUserModel->Get().GetAccountId();
        OnRecieveChatMessage(AccountId, Content, false);
        SendChatMessage(AccountId, Content);
    }

    void ChatModule::OnRecieveChatMessage(const std::string& AccountId, const std::string& NewMessage, bool Recieved)
    {
        auto& Message = GetOrCreateConversation(AccountId).Messages.emplace_back(ChatMessage{ .Content = NewMessage, .Time = Web::TimePoint::clock::now(), .Recieved = Recieved });

        if (SelectedUserModel && AccountId == SelectedUserModel->Get().GetAccountId()) {
            NewChatDispatcher.emit(std::ref(Message));
        }
        else if (Recieved) {
            SysTray.ShowToast(Utils::ToastTemplate{
                .Type = Utils::ToastType::Text02,
                .TextFields = { std::format("New Chat Message From {}", RequestDisplayName(AccountId)), NewMessage },
                .Actions = { "Reply", "Dismiss" }
            }, {
                .OnClicked = [this, AccountId](int ActionIdx) {
                    if (ActionIdx == 0) {
                        SysTray.SetAppState(SysTrayModule::AppState::Focused, SysTrayModule::StackTab::Account);
                        OpenChatPage(AccountId);
                    }
                }
            });
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

        return Conversations.emplace_back(ChatConversation{ .AccountId = AccountId });
    }
}