#pragma once

#include "../storage/models/ChatMessage.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class ChatBubble {
    public:
        ChatBubble(Storage::Models::ChatMessage& Message);

        operator Gtk::Widget& ();

    private:
        void Construct();

        static void DrawBubble(const Cairo::RefPtr<Cairo::Context>& Ctx, Gtk::Allocation Alloc, bool Recieved);

        Gtk::Box BaseContainer{ Gtk::ORIENTATION_VERTICAL };
        Gtk::Box BubbleContainer;
        Gtk::Label Content;

        Storage::Models::ChatMessage& Message;
    };
}
