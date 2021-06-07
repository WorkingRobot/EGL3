#pragma once

#include "../modules/ImageCache.h"
#include "../utils/Callback.h"

namespace EGL3::Widgets {
    template<class Fore, class Back, class Icon>
    class CellRendererAvatarStatus : public Gtk::CellRenderer
    {
        struct PropDisabled {
            explicit PropDisabled() = default;
        };

    public:
        using ForeT = std::conditional_t<std::is_void_v<Fore>, PropDisabled, Fore>;
        using BackT = std::conditional_t<std::is_void_v<Back>, PropDisabled, Back>;
        using IconT = std::conditional_t<std::is_void_v<Icon>, PropDisabled, Icon>;

        CellRendererAvatarStatus(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache, int GroundSize, int IconSize, int CellSize) :
            Glib::ObjectBase(typeid(CellRendererAvatarStatus)),
            Gtk::CellRenderer(),
            TreeView(TreeView),
            ImageCache(ImageCache),
            GroundSize(GroundSize),
            IconSize(IconSize),
            CellSize(CellSize),
            GetForegroundUrl([](const ForeT&) { return ""; }),
            GetBackgroundUrl([](const BackT&) { return ""; }),
            GetIconUrl([](const IconT&) { return ""; }),
            prop_foreground(*this, "renderer-foreground"),
            prop_background(*this, "renderer-background"),
            prop_icon(*this, "renderer-icon")
        {
            ImageDispatcher.connect([this]() {
                this->TreeView.queue_draw();
            });
        }

        virtual ~CellRendererAvatarStatus()
        {

        }

        Glib::PropertyProxy<ForeT> property_foreground() { return prop_foreground.get_proxy(); }
        Glib::PropertyProxy<BackT> property_background() { return prop_background.get_proxy(); }
        Glib::PropertyProxy<IconT> property_icon() { return prop_icon.get_proxy(); }

        void SetDefaultForeground(const ForeT& Default)
        {
            static_assert(ForegroundEnabled, "Can't set a default foreground if foregrounds are disabled");
            DefaultForeground = Default;
        }

        void SetDefaultBackground(const BackT& Default)
        {
            static_assert(BackgroundEnabled, "Can't set a default background if backgrounds are disabled");
            DefaultBackground = Default;
        }

        void SetDefaultIcon(const IconT& Default)
        {
            static_assert(IconEnabled, "Can't set a default icon if icons are disabled");
            DefaultIcon = Default;
        }

        Utils::Callback<std::string(const ForeT&)> GetForegroundUrl;
        Utils::Callback<std::string(const BackT&)> GetBackgroundUrl;
        Utils::Callback<std::string(const IconT&)> GetIconUrl;

    protected:
        void get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const override
        {
            minimum_width = natural_width = CellSize;
        }

        void get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width, int& natural_width) const override
        {
            minimum_width = natural_width = CellSize;
        }

        void get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const override
        {
            minimum_height = natural_height = CellSize;
        }

        void get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height, int& natural_height) const override
        {
            minimum_height = natural_height = CellSize;
        }

        void render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) override
        {
            int CellSize = std::min(cell_area.get_width(), cell_area.get_height());
            double GroundOutputSize = std::min(CellSize, GroundSize);
            double IconOutputSize = std::min(CellSize, IconSize);

            if constexpr (BackgroundEnabled) {
                auto Background = prop_background.get_value();
                if (!Background.empty()) {
                    if (auto BackgroundPixbuf = ImageCache.TryGetOrQueueImage(GetBackgroundUrl(Background), GetBackgroundUrl(DefaultBackground), GroundSize, GroundSize, ImageDispatcher)) {
                        cr->save();
                        cr->translate(cell_area.get_x(), cell_area.get_y());
                        cr->scale(GroundOutputSize / BackgroundPixbuf->get_width(), GroundOutputSize / BackgroundPixbuf->get_height());
                        Gdk::Cairo::set_source_pixbuf(cr, BackgroundPixbuf, 0, 0);
                        cr->paint();
                        cr->restore();
                    }
                }
            }

            if constexpr (ForegroundEnabled) {
                auto Foreground = prop_foreground.get_value();
                if (!Foreground.empty()) {
                    if (auto ForegroundPixbuf = ImageCache.TryGetOrQueueImage(GetForegroundUrl(Foreground), GetForegroundUrl(DefaultForeground), GroundSize, GroundSize, ImageDispatcher)) {
                        cr->save();
                        cr->translate(cell_area.get_x(), cell_area.get_y());
                        cr->scale(GroundOutputSize / ForegroundPixbuf->get_width(), GroundOutputSize / ForegroundPixbuf->get_height());
                        Gdk::Cairo::set_source_pixbuf(cr, ForegroundPixbuf, 0, 0);
                        cr->paint();
                        cr->restore();
                    }
                }
            }

            if constexpr (IconEnabled) {
                if (auto IconPixbuf = ImageCache.TryGetOrQueueImage(GetIconUrl(prop_icon.get_value()), GetIconUrl(DefaultIcon), IconSize, IconSize, ImageDispatcher)) {
                    cr->save();
                    cr->translate(cell_area.get_x() + cell_area.get_width() - IconOutputSize, cell_area.get_y() + cell_area.get_height() - IconOutputSize);
                    cr->scale(IconOutputSize / IconPixbuf->get_width(), IconOutputSize / IconPixbuf->get_height());
                    Gdk::Cairo::set_source_pixbuf(cr, IconPixbuf, 0, 0);
                    cr->paint();
                    cr->restore();
                }
            }
        }

    private:
        Gtk::TreeView& TreeView;
        Glib::Dispatcher ImageDispatcher;
        Modules::ImageCacheModule& ImageCache;
        int GroundSize;
        int IconSize;
        int CellSize;

        ForeT DefaultForeground;
        BackT DefaultBackground;
        IconT DefaultIcon;

        static constexpr bool ForegroundEnabled = !std::is_void_v<Fore>;
        static constexpr bool BackgroundEnabled = !std::is_void_v<Back>;
        static constexpr bool IconEnabled = !std::is_void_v<Icon>;

        Glib::Property<ForeT> prop_foreground;
        Glib::Property<BackT> prop_background;
        Glib::Property<IconT> prop_icon;
    };
}
