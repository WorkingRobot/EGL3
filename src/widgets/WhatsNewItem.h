#pragma once

#include "AsyncImage.h"
#include "../storage/models/WhatsNew.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class WhatsNewItem {
    public:
        WhatsNewItem(const Glib::ustring& Title, const std::string& ImageUrl, const Glib::ustring& Source, const Glib::ustring& Description, const std::chrono::system_clock::time_point& Date, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(const Web::Epic::Responses::GetBlogPosts::BlogItem& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericMotd& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericPlatformMotd& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericPlatformPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericRegionPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache);

        WhatsNewItem(WhatsNewItem&&) = default;
        WhatsNewItem& operator=(WhatsNewItem&&) = default;

        operator Gtk::Widget& ();

    private:
        void Construct();

        Gtk::Box BaseContainer{ Gtk::ORIENTATION_VERTICAL };
        AsyncImage MainImage;
        Gtk::Box TitleContainer{ Gtk::ORIENTATION_HORIZONTAL };
        Gtk::Label Title;
        Gtk::Label Source;
        Gtk::Label Date;
        Gtk::Label Description;
        Gtk::Box ButtonContainer{ Gtk::ORIENTATION_HORIZONTAL };
        Gtk::Button MarkReadButton{ "Mark as Read" };
        Gtk::Button ClickButton{ "View Online" };

        sigc::connection MarkReadAction;
        sigc::connection ClickAction;
    };
}
