#pragma once

#include "../utils/HashCombine.h"
#include "../web/Http.h"
#include "AsyncFF.h"
#include "ModuleList.h"

#include <future>
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
                return Url == other.Url
                    && FallbackUrl == other.FallbackUrl
                    && RequestedWidth == other.RequestedWidth
                    && RequestedHeight == other.RequestedHeight;
            }
        };

        struct CacheKeyHasher {
            std::size_t operator()(const CacheKey& k) const
            {
                return Utils::HashCombine(k.Url, k.FallbackUrl, k.RequestedWidth, k.RequestedHeight);
            }
        };

        struct QueuedCacheKey : public CacheKey
        {
            Glib::Dispatcher& Dispatcher;

            QueuedCacheKey(const std::string& Url, const std::string& FallbackUrl, int Width, int Height, Glib::Dispatcher& Dispatcher) :
                CacheKey(Url, FallbackUrl, Width, Height),
                Dispatcher(Dispatcher)
            {
                
            }

            bool operator==(const QueuedCacheKey& other) const
            {
                return Url == other.Url
                    && FallbackUrl == other.FallbackUrl
                    && RequestedWidth == other.RequestedWidth
                    && RequestedHeight == other.RequestedHeight
                    && std::addressof(Dispatcher) == std::addressof(other.Dispatcher);
            }
        };

        struct QueuedCacheKeyHasher {
            std::size_t operator()(const QueuedCacheKey& k) const
            {
                return Utils::HashCombine(k.Url, k.FallbackUrl, k.RequestedWidth, k.RequestedHeight, std::addressof(k.Dispatcher));
            }
        };

        std::mutex CacheMutex;
        std::unordered_map<CacheKey, std::shared_future<Glib::RefPtr<Gdk::Pixbuf>>, CacheKeyHasher> Cache;
        std::unordered_map<QueuedCacheKey, std::future<void>, QueuedCacheKeyHasher> QueuedCache;

    public:
        ImageCacheModule(ModuleList& Ctx);

        std::future<Glib::RefPtr<Gdk::Pixbuf>> GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Glib::Dispatcher& Callback);

        std::future<Glib::RefPtr<Gdk::Pixbuf>> GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, Glib::Dispatcher& Callback);

        std::shared_future<Glib::RefPtr<Gdk::Pixbuf>>& GetImageAsync(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width = -1, int Height = -1);

        // Run only on gui thread, that's what this is primarily for
        Glib::RefPtr<Gdk::Pixbuf> TryGetOrQueueImage(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width, int Height, Glib::Dispatcher& Callback);

        Glib::RefPtr<Gdk::Pixbuf> GetImage(const cpr::Url& Url, const cpr::Url& FallbackUrl, int Width = -1, int Height = -1);

        bool TryGetImage(const cpr::Url& Url, Glib::RefPtr<Gdk::Pixbuf>& Output, int Width = -1, int Height = -1);
    };
}