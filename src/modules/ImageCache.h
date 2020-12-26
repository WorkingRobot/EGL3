#pragma once

#include "BaseModule.h"
#include "../utils/HashCombine.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class ImageCacheModule : public BaseModule {
        struct CacheKey
        {
            std::string Url;
            std::string FallbackUrl;
            int RequestedWidth;
            int RequestedHeight;

            CacheKey(const std::string& Url, const std::string& FallbackUrl, int Width, int Height) :
                Url(Url),
                RequestedHeight(Width),
                RequestedWidth(Height)
            {

            }

            bool operator==(const CacheKey& other) const
            {
                return (Url == other.Url
                    && FallbackUrl == other.FallbackUrl
                    && RequestedWidth == other.RequestedWidth
                    && RequestedHeight == other.RequestedHeight);
            }
        };

        struct CacheKeyHasher {
            std::size_t operator()(const CacheKey& k) const
            {
                return Utils::HashCombine(k.Url, k.FallbackUrl, k.RequestedWidth, k.RequestedHeight);
            }
        };

        struct EmitRAII {
            EmitRAII(Glib::Dispatcher& Dispatcher) :
                Dispatcher(Dispatcher)
            {

            }

            ~EmitRAII() {
                Dispatcher.emit();
            }

        private:
            Glib::Dispatcher& Dispatcher;
        };

        std::mutex CacheMutex;
        std::unordered_map<CacheKey, std::shared_future<Glib::RefPtr<Gdk::Pixbuf>>, CacheKeyHasher> Cache;

    public:
        ImageCacheModule() {

        }

        std::future<Glib::RefPtr<Gdk::Pixbuf>> GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Glib::Dispatcher& Callback) {
            return std::async(std::launch::async, [&, this, Width, Height](const std::string& Url, const std::string& FallbackUrl) {
                EmitRAII Emitter(Callback);
                return GetImageAsync(Url, FallbackUrl, Width, Height).get();
            }, Url, FallbackUrl);
        }

        std::future<Glib::RefPtr<Gdk::Pixbuf>> GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, Glib::Dispatcher& Callback) {
            return GetImageAsync(Url, FallbackUrl, -1, -1, Callback);
        }

        std::shared_future<Glib::RefPtr<Gdk::Pixbuf>>& GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width = -1, int Height = -1) {
            std::lock_guard Guard(CacheMutex);

            auto CacheItr = Cache.find(CacheKey(Url.str(), FallbackUrl.str(), Width, Height));
            if (CacheItr != Cache.end()) {
                return CacheItr->second;
            }

            return Cache.emplace(CacheKey(Url.str(), FallbackUrl.str(), Width, Height), std::move(std::async(std::launch::async, &ImageCacheModule::GetImage, this, Url, FallbackUrl, Width, Height))).first->second;
        }

        Glib::RefPtr<Gdk::Pixbuf> GetImage(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width = -1, int Height = -1) {
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

        bool TryGetImage(const cpr::Url& Url, Glib::RefPtr<Gdk::Pixbuf>& Output, int Width = -1, int Height = -1) {
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

        void CreateEmptyImage(Glib::RefPtr<Gdk::Pixbuf>& Output, int Width = -1, int Height = -1) {
            Output = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, std::max(Width, 1), std::max(Height, 1));
            Output->fill(0x00000000); // Fill with transparent black
        }
    };
}