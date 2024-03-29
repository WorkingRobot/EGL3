﻿#pragma once

#include <gtkmm.h>

#include "AsyncImage.h"
#include "../modules/ImageCache.h"

namespace EGL3::Widgets {
    template<typename KeyType>
    class AsyncImageKeyed {
    public:
        template<typename UrlCallback>
        AsyncImageKeyed(const KeyType& Key, const KeyType& FallbackKey, int Width, int Height, const UrlCallback& GetUrl, Modules::ImageCacheModule& ImageCache) :
            Key(Key),
            ImageCache(ImageCache)
        {
            Construct(GetUrl(Key), GetUrl(FallbackKey), Width, Height);
        }

        operator Gtk::Widget& () {
            return Image;
        }

        const KeyType& GetKey() const {
            return Key;
        }

    private:
        void Construct(const std::string& Url, const std::string& FallbackUrl, int Width, int Height) {
            Image.set_data("EGL3_ImageBase", this);

            Image.set_async(Url, FallbackUrl, Width, Height, ImageCache);

            Image.show_all();
        }

        Modules::ImageCacheModule& ImageCache;
        KeyType Key;

        AsyncImage Image;
    };
}
