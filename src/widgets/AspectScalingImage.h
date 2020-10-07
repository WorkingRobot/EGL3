#pragma once

#include <gtkmm.h>

#include "ScalingImage.h"

// http://gtk.10911.n7.nabble.com/Rescaling-Gtk-Image-td42810.html

namespace EGL3::Widgets {
    class AspectScalingImage : public Gtk::AspectFrame {
    public:
        explicit AspectScalingImage(Glib::RefPtr<Gdk::Pixbuf> const& pixbuf, Gdk::InterpType interp = Gdk::INTERP_BILINEAR) : m_img(pixbuf, interp) {
            set(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, pixbuf->get_width() / float(pixbuf->get_height()));
            // This makes it appear as if there is no frame
            set_shadow_type(Gtk::SHADOW_NONE);
            // This allows a minimum size
            set_size_request(pixbuf->get_width() / 2, pixbuf->get_height() / 2);
            // Finally, add the image to be managed
            add(m_img);
        }

    private:
        ScalingImage m_img;
    };
}
