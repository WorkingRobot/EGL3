#pragma once

#include "../modules/ImageCache.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class AsyncImage : public Gtk::Image {
    public:
        explicit AsyncImage();

        explicit AsyncImage(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf);

        void set_async(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Modules::ImageCacheModule& ImageCache);

        void set_async(const cpr::Url& Url, const cpr::Url& FallbackUrl, Modules::ImageCacheModule& ImageCache);

    private:
        void ConstructDispatcher();

        std::future<Glib::RefPtr<Gdk::Pixbuf>> ImageTask;
        Glib::Dispatcher ImageDispatcher;
    };
}
