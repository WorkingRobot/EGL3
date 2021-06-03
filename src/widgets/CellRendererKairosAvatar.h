#pragma once

#include "../modules/AsyncFF.h"
#include "../modules/ImageCache.h"

namespace EGL3::Widgets {
    class CellRendererKairosAvatar : public Gtk::CellRenderer
    {
    public:
        CellRendererKairosAvatar(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache, int Size = -1);
        virtual ~CellRendererKairosAvatar();

        Glib::PropertyProxy<Glib::ustring> property_avatar() { return prop_avatar.get_proxy(); }
        Glib::PropertyProxy<Glib::ustring> property_background() { return prop_background.get_proxy(); }

    protected:
        void get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const override;
        void get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width, int& natural_width) const override;
        void get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const override;
        void get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height, int& natural_height) const override;
        void render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) override;

    private:
        Gtk::TreeView& TreeView;
        Glib::Dispatcher ImageDispatcher;
        Modules::ImageCacheModule& ImageCache;
        int Size;

        Glib::Property<Glib::ustring> prop_avatar;
        Glib::Property<Glib::ustring> prop_background;
    };
}
