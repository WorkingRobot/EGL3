#pragma once

#include "../modules/ImageCache.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class AsyncImage : public Gtk::Image {
    public:
        explicit AsyncImage() {
            ConstructDispatcher();
        }

        explicit AsyncImage(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf) : Gtk::Image(pixbuf) {
            ConstructDispatcher();
        }

        void set_async(const cpr::Url& Url, int Width, int Height, Modules::ImageCacheModule& ImageCache) {
            ImageTask = ImageCache.GetImageAsync(Url, Width, Height, ImageBuf, ImageDispatcher);
        }

        void set_async(const cpr::Url& Url, Modules::ImageCacheModule& ImageCache) {
            set_async(Url, -1, -1, ImageCache);
        }

    private:
        void ConstructDispatcher() {
            ImageDispatcher.connect([this]() {
                this->set(ImageBuf);
            });
        }

        Glib::RefPtr<Gdk::Pixbuf> ImageBuf;
        std::future<void> ImageTask;
        Glib::Dispatcher ImageDispatcher;
    };
}
