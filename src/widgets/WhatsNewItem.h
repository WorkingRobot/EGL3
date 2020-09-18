#pragma once

#include <gtkmm.h>

namespace EGL3 {
    namespace Widgets {
        class WhatsNewItem {
        public:
            WhatsNewItem();
            Gtk::Widget& operator()();

        public:
            Gtk::Box    box{ Gtk::ORIENTATION_HORIZONTAL };
            Gtk::Label  label{ "search: " };
            Gtk::Entry  entry;
            Gtk::Button BtnOk{ "find" };
            Gtk::Button BtnNext{ ">" };
            Gtk::Button BtnPrev{ "<" };
        };
    }
}

inline SearchBar::SearchBar() {
    box.pack_start(label);
    box.pack_start(entry, Gtk::PACK_EXPAND_WIDGET);
    box.pack_end(BtnNext);
    box.pack_end(BtnPrev);
    box.pack_end(BtnOk);
}

inline Gtk::Widget& SearchBar::operator()() {
    return box;
}