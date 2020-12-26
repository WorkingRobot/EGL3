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

        void set_async(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Modules::ImageCacheModule& ImageCache) {
            ImageTask = ImageCache.GetImageAsync(Url, FallbackUrl, Width, Height, ImageDispatcher);
        }

        void set_async(const cpr::Url& Url, const cpr::Url& FallbackUrl, Modules::ImageCacheModule& ImageCache) {
            set_async(Url, FallbackUrl, -1, -1, ImageCache);
        }

    private:
        void ConstructDispatcher() {
            ImageDispatcher.connect([this]() {
                if (ImageTask.valid()) {
                    this->set(ImageTask.get());
                }
            });
        }

        std::future<Glib::RefPtr<Gdk::Pixbuf>> ImageTask;
        Glib::Dispatcher ImageDispatcher;
    };
}
