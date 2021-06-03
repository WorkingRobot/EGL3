#pragma once

#include <gtkmm.h>

namespace EGL3::Widgets {
    class CellRendererPixbufScalable : public Gtk::CellRendererPixbuf
    {
    public:
        CellRendererPixbufScalable();

        virtual ~CellRendererPixbufScalable();

        Glib::PropertyProxy<double> property_scale() { return prop_scale.get_proxy(); }

    protected:
        void render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) override;

    private:
        Glib::Property<double> prop_scale;
    };
}
