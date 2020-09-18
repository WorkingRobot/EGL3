#pragma once

#include <gtkmm.h>

namespace EGL3::Widgets {
    class WhatsNewItem {
    public:
        inline WhatsNewItem() {
            box.pack_start(label);
            box.pack_start(entry, Gtk::PACK_EXPAND_WIDGET);
            box.pack_end(BtnNext);
            box.pack_end(BtnPrev);
            box.pack_end(BtnOk);
        }

        inline Gtk::Widget& operator()() {
            return box;
        }

    public:
        Gtk::Box Container { Gtk::ORIENTATION_VERTICAL };
        Gtk::Image MainImage;
        Gtk::Label ;
        Gtk::Button BtnOk{ "find" };
        Gtk::Button BtnNext{ ">" };
        Gtk::Button BtnPrev{ "<" };
    };
}