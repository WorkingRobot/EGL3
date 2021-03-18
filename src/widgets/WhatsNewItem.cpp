#include "WhatsNewItem.h"

#include "../utils/Humanize.h"

#include <array>
#include <regex>

namespace EGL3::Widgets {
    WhatsNewItem::WhatsNewItem(const Glib::ustring& Title, const std::string& ImageUrl, const Glib::ustring& Source, const Glib::ustring& Description, const std::chrono::system_clock::time_point& Date, Modules::ImageCacheModule& ImageCache) :
        UseImage(!ImageUrl.empty())
    {
        this->Title.set_text(Title);
        this->Title.set_tooltip_text(Title);
        this->Source.set_text(Source);
        this->Date.set_text(Utils::Humanize(Date));
        this->Description.set_text(Description);
        if (UseImage) {
            this->MainImage.set_async(ImageUrl, "", 768, 432, ImageCache);
        }

        Construct();
    }

    // Taken from XmppClient.cpp
    // Subtract 1, we aren't comparing \0 at the end of the source string
    template<size_t SourceStringSize>
    bool XmlStringEqual(const char* XmlString, size_t XmlStringSize, const char(&SourceString)[SourceStringSize]) {
        if (XmlStringSize != SourceStringSize - 1) {
            return false;
        }
        return memcmp(XmlString, SourceString, SourceStringSize - 1) == 0;
    }

    std::string GetDescriptionFromMetaTags(const Web::Epic::Responses::GetBlogPosts::BlogItem& Item) {
        // rapidxml has no support for meta tags (that don't say that they're self closing)
        // Instead, we just use regex
        const static std::array<std::regex, 3> DescRegexes {
            std::regex("<meta name=\"description\" content=\"(.+?)\">"),
            std::regex("<meta name=\"twitter:description\" content=\"(.+?)\">"),
            std::regex("<meta property=\"og:description\" content=\"(.+?)\">")
        };
        std::smatch Matches;
        for (auto& Regex : DescRegexes) {
            if (std::regex_search(Item.MetaTags, Matches, Regex)) {
                return Matches[1];
            }
        }
        
        EGL3_LOG(LogLevel::Warning, "Blog post's _metaTags does not have a description");
        return "";
    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetBlogPosts::BlogItem& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
        WhatsNewItem(
            Item.Title,
            Item.ShareImage.value_or(Item.TrendingImage),
            Item.Author.empty() ? 
                Storage::Models::WhatsNew::SourceToString(Source) :
                Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Item.Author),
            Item.ShareDescription.empty() ? GetDescriptionFromMetaTags(Item) : Item.ShareDescription,
            Time,
            ImageCache)
    {

    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericMotd& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
        WhatsNewItem(
            Item.Title,
            Item.Image.value_or(""),
            Storage::Models::WhatsNew::SourceToString(Source),
            Item.Body,
            Time,
            ImageCache)
    {

    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericPlatformMotd& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
        WhatsNewItem(
            Item.Message.Title,
            Item.Message.Image.value_or(""),
            Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::PlatformToString(Item.Platform)),
            Item.Message.Body,
            Time,
            ImageCache)
    {

    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
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
    {
    
    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericPlatformPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
        WhatsNewItem(
            Item.Message.Title.value_or(""),
            Item.Message.Image.value_or(""),
            (Item.Message.SubGame.has_value() && !Item.Message.SubGame->empty()) ?
                Glib::ustring::compose("%3 %1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::PlatformToString(Item.Platform), Storage::Models::WhatsNew::SubGameToString(Item.Message.SubGame.value())) :
                Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Storage::Models::WhatsNew::PlatformToString(Item.Platform)),
            Item.Message.Body.value_or(""),
            Time,
            ImageCache)
    {

    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetPageInfo::GenericRegionPost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
        WhatsNewItem(
            Item.Message.Title.value_or(""),
            Item.Message.Image.value_or(""),
            (Item.Message.SubGame.has_value() && !Item.Message.SubGame->empty()) ?
            Glib::ustring::compose("%3 %1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Item.Region, Storage::Models::WhatsNew::SubGameToString(Item.Message.SubGame.value())) :
            Glib::ustring::compose("%1 (%2)", Storage::Models::WhatsNew::SourceToString(Source), Item.Region),
            Item.Message.Body.value_or(""),
            Time,
            ImageCache)
    {

    }

    template<class TransformerT>
    std::string CombineList(const std::vector<std::string>& Gamemodes, const TransformerT& Transformer) {
        if (Gamemodes.size() == 2) {
            return Utils::Format("%s and %s", Transformer(Gamemodes[0]), Transformer(Gamemodes[1]));
        }
        else {
            std::stringstream Ret;
            for (int i = 0; i < Gamemodes.size(); ++i) {
                Ret << Transformer(Gamemodes[i]);
                if (i + 2 == Gamemodes.size()) {
                    Ret << ", and ";
                }
                else if (i + 1 != Gamemodes.size()) {
                    Ret << ", ";
                }
            }
            return Ret.str();
        }
    }

    WhatsNewItem::WhatsNewItem(const Web::Epic::Responses::GetPageInfo::EmergencyNoticePost& Item, const std::chrono::system_clock::time_point& Time, Storage::Models::WhatsNew::ItemSource Source, Modules::ImageCacheModule& ImageCache) :
        WhatsNewItem(
            Item.Title,
            "",
            Item.Gamemodes.empty() ?
                Storage::Models::WhatsNew::SourceToString(Source) : 
                Glib::ustring::compose("%2 %1%3", Storage::Models::WhatsNew::SourceToString(Source), CombineList(Item.Gamemodes, Storage::Models::WhatsNew::SubGameToString),
                    Item.Platforms.empty() ? "" :
                    " (" + CombineList(Item.Platforms, Storage::Models::WhatsNew::PlatformToString) + ")"
                ),
            Item.Body,
            Time,
            ImageCache
        )
    {
    }

    WhatsNewItem::operator Gtk::Widget&() {
        return BaseContainer;
    }

    void WhatsNewItem::Construct() {
        BaseContainer.set_border_width(5);
        BaseContainer.get_style_context()->add_class("frame");

        Description.set_xalign(0);
        Description.set_yalign(0);

        Title.set_xalign(0);
        Title.set_yalign(0);

        Description.set_margin_start(10);
        Description.set_margin_end(10);

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

        if (UseImage) {
            BaseContainer.pack_start(MainImage, false, false, 0);
        }
        BaseContainer.pack_start(TitleContainer, false, false, 5);
        BaseContainer.pack_start(Description, false, false, 5);
        BaseContainer.pack_start(ButtonContainer, false, false, 5);

        BaseContainer.show_all();
    }
}
