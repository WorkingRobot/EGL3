#pragma once

#include "BaseModule.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class ImageCacheModule : public BaseModule {
        struct CacheKey
        {
            std::string Url;
            int RequestedWidth;
            int RequestedHeight;

            CacheKey(const std::string& Url, int Width, int Height) :
                Url(Url),
                RequestedHeight(Width),
                RequestedWidth(Height)
            {}

            bool operator==(const CacheKey& other) const
            {
                return (Url == other.Url
                    && RequestedWidth == other.RequestedWidth
                    && RequestedHeight == other.RequestedHeight);
            }
        };

        struct CacheKeyHasher {
            std::size_t operator()(const CacheKey& k) const
            {
                using std::size_t;
                using std::hash;
                using std::string;

                return ((hash<string>()(k.Url)
                    ^ (hash<int>()(k.RequestedWidth) << 1)) >> 1)
                    ^ (hash<int>()(k.RequestedHeight) << 1);
            }
        };

        std::mutex CacheMutex;
        std::unordered_map<CacheKey, std::shared_future<std::optional<Glib::RefPtr<Gdk::Pixbuf>>>, CacheKeyHasher> Cache;

    public:
        ImageCacheModule() {}

        std::future<void> GetImageAsync(const std::string& Url, int Width, int Height, Glib::RefPtr<Gdk::Pixbuf>& Output, Glib::Dispatcher& Callback) {
            return std::async(std::launch::async, [&, this, Width, Height](const std::string& Url) {
                auto& Ret = GetImageAsync(Url, Width, Height).get();
                if (Ret.has_value()) {
                    Output = Ret.value();
                    Callback.emit();
                }
            }, Url);
        }

        std::future<void> GetImageAsync(const std::string& Url, Glib::RefPtr<Gdk::Pixbuf>& Output, Glib::Dispatcher& Callback) {
            return GetImageAsync(Url, -1, -1, Output, Callback);
        }

        std::shared_future<std::optional<Glib::RefPtr<Gdk::Pixbuf>>>& GetImageAsync(const std::string& Url, int Width = -1, int Height = -1) {
            std::lock_guard Guard(CacheMutex);

            auto CacheItr = Cache.find(CacheKey(Url, Width, Height));
            if (CacheItr != Cache.end()) {
                return CacheItr->second;
            }

            return Cache.emplace(CacheKey(Url, Width, Height), std::move(std::async(std::launch::async, &ImageCacheModule::GetImage, this, Url, Width, Height))).first->second;
        }

        std::optional<Glib::RefPtr<Gdk::Pixbuf>> GetImage(const std::string& Url, int Width = -1, int Height = -1) {
            auto Response = Web::Http::Get(cpr::Url{ Url });
            if (Response.status_code != 200) {
                return std::nullopt;
            }
            auto Stream = Gio::MemoryInputStream::create();
            Stream->add_bytes(Glib::Bytes::create(Response.text.data(), Response.text.size()));
            if (Width == -1 && Height == -1) {
                return Gdk::Pixbuf::create_from_stream(Stream);
            }
            return Gdk::Pixbuf::create_from_stream_at_scale(Stream, Width, Height, true);
        }
    };
}