#include "AsyncImage.h"

namespace EGL3::Widgets {
    AsyncImage::AsyncImage() {
        ConstructDispatcher();
    }

    AsyncImage::AsyncImage(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf) : Gtk::Image(pixbuf) {
        ConstructDispatcher();
    }

    void AsyncImage::set_async(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Modules::ImageCacheModule& ImageCache) {
        if (!IsMapped.test()) {
            ImageData = std::make_unique<ImageInfo>(ImageInfo{
                .Url = Url,
                .FallbackUrl = FallbackUrl,
                .Width = Width,
                .Height = Height,
                .ImageCache = ImageCache
            });
        }
        else {
            ImageTask = ImageCache.GetImageAsync(Url, FallbackUrl, Width, Height, ImageDispatcher);
        }
    }

    void AsyncImage::set_async(const cpr::Url& Url, const cpr::Url& FallbackUrl, Modules::ImageCacheModule& ImageCache) {
        set_async(Url, FallbackUrl, -1, -1, ImageCache);
    }

    void AsyncImage::ConstructDispatcher() {
        ImageDispatcher.connect([this]() {
            if (ImageTask.valid()) {
                this->set(ImageTask.get());
            }
        });

        this->signal_map().connect([this]() {
            if (!IsMapped.test_and_set()) {
                if (ImageData) {
                    ImageTask = ImageData->ImageCache.GetImageAsync(ImageData->Url, ImageData->FallbackUrl, ImageData->Width, ImageData->Height, ImageDispatcher);
                    ImageData.reset();
                }
            }
        });
    }
}
