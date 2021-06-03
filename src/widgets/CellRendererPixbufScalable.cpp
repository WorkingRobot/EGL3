#include "CellRendererPixbufScalable.h"

namespace EGL3::Widgets {
    CellRendererPixbufScalable::CellRendererPixbufScalable() :
        Glib::ObjectBase("CellRendererPixbufScalable"),
        Gtk::CellRendererPixbuf(),
        prop_scale(*this, "renderer-scale", 1.)
    {

    }

    CellRendererPixbufScalable::~CellRendererPixbufScalable()
    {

    }

    void CellRendererPixbufScalable::render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags)
    {
        double Tx = cell_area.get_x() + cell_area.get_width() * property_xalign();
        double Ty = cell_area.get_y() + cell_area.get_height() * property_yalign();
        double Scale = property_scale();

        cr->save();
        cr->translate(Tx, Ty);
        cr->scale(Scale, Scale);
        cr->translate(-Tx, -Ty);
        Gtk::CellRendererPixbuf::render_vfunc(cr, widget, background_area, cell_area, flags);
        cr->restore();
    }
}
