#pragma once

#include <gtkmm.h>

#include "AsyncImage.h"
#include "../storage/models/WhatsNew.h"
#include "../utils/Humanize.h"
#include "../web/epic/responses/Responses.h"

namespace EGL3::Widgets {
    class WhatsNewItem {
    public:
        WhatsNewItem(const Glib::ustring& Title, const std::string& ImageUrl, const Glib::ustring& Source, const Glib::ustring& Description, const std::chrono::system_clock::time_point& Date, Modules::ImageCacheModule& ImageCache) {
            this->Title.set_text(Title);
            this->Title.set_tooltip_text(Title);
            this->Source.set_text(Source);
            this->Date.set_text(Utils::Humanize(Date));
            this->Description.set_text(Description);
            this->MainImage.set_async(ImageUrl, "", 768, 432, ImageCache);

            Construct();
        }

        WhatsNewItem(const Web::Epic::Responses::GetBlogPosts::BlogItem& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
            WhatsNewItem(
                Item.Title,
                Item.ShareImage.value_or(Item.TrendingImage),
                Item.Author.empty() ? 
                    Storage::Models::WhatsNew::SourceToString(Source) :
                    Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Item.Author),
                Item.ShareDescription,
                Time,
                ImageCache)
        {}

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericMotd& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
            WhatsNewItem(
                Item.Title,
                Item.Image.value_or(""),
                Storage::Models::WhatsNew::SourceToString(Source),
                Item.Body,
                Time,
                ImageCache)
        {}

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericPlatformMotd& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
            WhatsNewItem(
                Item.Message.Title,
                Item.Message.Image.value_or(""),
                Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::PlatformToString(Item.Platform)),
                Item.Message.Body,
                Time,
                ImageCache)
        {}

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
            WhatsNewItem(
                Item.Title.value_or(""),
                Item.Image.value_or(""),
                (Item.SubGame.has_value() ?
                    Glib::ustring::compose("%2 %1", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::SubGameToString(Item.SubGame.value())) :
                    Storage::Models::WhatsNew::SourceToString(Source)
                ) +
                ((Item.PlaylistId.has_value() && !Item.PlaylistId->empty()) ?
                    Glib::ustring::compose(" (%1)", Item.PlaylistId.value()) :
                    Glib::ustring("")
                ),
                Item.Body.value_or(""),
                Time,
                ImageCache)
        {}

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericPlatformPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
            WhatsNewItem(
                Item.Message.Title.value_or(""),
                Item.Message.Image.value_or(""),
                (Item.Message.SubGame.has_value() && !Item.Message.SubGame->empty()) ?
                    Glib::ustring::compose("%3 %1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::PlatformToString(Item.Platform), Storage::Models::WhatsNew::SubGameToString(Item.Message.SubGame.value())) :
                    Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::PlatformToString(Item.Platform)),
                Item.Message.Body.value_or(""),
                Time,
                ImageCache)
        {}

        WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericRegionPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
            WhatsNewItem(
                Item.Message.Title.value_or(""),
                Item.Message.Image.value_or(""),
                (Item.Message.SubGame.has_value() && !Item.Message.SubGame->empty()) ?
                Glib::ustring::compose("%3 %1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Item.Region, Storage::Models::WhatsNew::SubGameToString(Item.Message.SubGame.value())) :
                Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Item.Region),
                Item.Message.Body.value_or(""),
                Time,
                ImageCache)
        {}

        WhatsNewItem(WhatsNewItem&&) = default;
        WhatsNewItem& operator=(WhatsNewItem&&) = default;

        operator Gtk::Widget&() {
            return BaseContainer;
        }

    private:
        void Construct() {
            BaseContainer.set_border_width(5);
            BaseContainer.get_style_context()->add_class("frame");

            Description.set_xalign(0);
            Description.set_yalign(0);

            Title.set_xalign(0);
            Title.set_yalign(0);

            Description.set_margin_left(10);
            Description.set_margin_right(10);

            Title.set_line_wrap(true);
            Title.set_ellipsize(Pango::ELLIPSIZE_END);
            Title.set_lines(1);

            Description.set_line_wrap(true);
            Description.set_ellipsize(Pango::ELLIPSIZE_END);
            Description.set_lines(4);

            Title.get_style_context()->add_class("font-bold");
            Title.get_style_context()->add_class("font-h2");
            Date.get_style_context()->add_class("font-h5");
            Source.get_style_context()->add_class("font-translucent");
            Source.get_style_context()->add_class("font-h5");

            TitleContainer.pack_start(Title, false, false, 10);
            TitleContainer.pack_end(Date, false, false, 10);
            TitleContainer.pack_end(Source, false, false, 0);

            ButtonContainer.pack_start(MarkReadButton, true, true, 5);
            ButtonContainer.pack_start(ClickButton, true, true, 5);

            BaseContainer.pack_start(MainImage, false, false, 0);
            BaseContainer.pack_start(TitleContainer, false, false, 5);
            BaseContainer.pack_start(Description, false, false, 5);
            BaseContainer.pack_start(ButtonContainer, false, false, 5);

            BaseContainer.show_all();
        }

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
