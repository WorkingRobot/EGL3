#include "ImageCache.h"

#include "../utils/EmitRAII.h"

namespace EGL3::Modules {
    ImageCacheModule::ImageCacheModule(ModuleList& Ctx) {

    }

    std::future<Glib::RefPtr<Gdk::Pixbuf>> ImageCacheModule::GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Glib::Dispatcher& Callback) {
        return std::async(std::launch::async, [&, this, Width, Height](const std::string& Url, const std::string& FallbackUrl) {
            Utils::EmitRAII Emitter(Callback);
            return GetImageAsync(Url, FallbackUrl, Width, Height).get();
        }, Url, FallbackUrl);
    }

    std::future<Glib::RefPtr<Gdk::Pixbuf>> ImageCacheModule::GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, Glib::Dispatcher& Callback) {
        return GetImageAsync(Url, FallbackUrl, -1, -1, Callback);
    }

    std::shared_future<Glib::RefPtr<Gdk::Pixbuf>>& ImageCacheModule::GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height) {
        std::lock_guard Guard(CacheMutex);

        auto CacheItr = Cache.find(CacheKey(Url.str(), FallbackUrl.str(), Width, Height));
        if (CacheItr != Cache.end()) {
            return CacheItr->second;
        }

        return Cache.emplace(CacheKey(Url.str(), FallbackUrl.str(), Width, Height), std::move(std::async(std::launch::async, &ImageCacheModule::GetImage, this, Url, FallbackUrl, Width, Height))).first->second;
    }

    void CreateEmptyImage(Glib::RefPtr<Gdk::Pixbuf>& Output, int Width = -1, int Height = -1) {
        Output = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, std::max(Width, 1), std::max(Height, 1));
        Output->fill(0x00000000); // Fill with transparent black
    }

    Glib::RefPtr<Gdk::Pixbuf> ImageCacheModule::GetImage(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height) {
        Glib::RefPtr<Gdk::Pixbuf> Ret;

        if (TryGetImage(Url, Ret, Width, Height)) {
            return Ret;
        }

        if (!FallbackUrl.str().empty()){
            if (TryGetImage(FallbackUrl, Ret, Width, Height)) {
                return Ret;
            }
        }

        CreateEmptyImage(Ret, Width, Height);
        return Ret;
    }

    bool ImageCacheModule::TryGetImage(const cpr::Url& Url, Glib::RefPtr<Gdk::Pixbuf>& Output, int Width, int Height) {
        auto Response = Web::Http::Get(Url);
        if (Response.status_code != 200) {
            return false;
        }
        auto Stream = Gio::MemoryInputStream::create();
        Stream->add_bytes(Glib::Bytes::create(Response.text.data(), Response.text.size()));
        if (Width == -1 && Height == -1) {
            Output = Gdk::Pixbuf::create_from_stream(Stream);
            return true;
        }
        else {
            Output = Gdk::Pixbuf::create_from_stream_at_scale(Stream, Width, Height, true);
            return true;
        }
    }
}