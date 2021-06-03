#pragma once

#include "../modules/ImageCache.h"

namespace EGL3::Widgets {
    class KairosAvatar : public Gtk::Widget
    {
    public:
        KairosAvatar(Modules::ImageCacheModule& ImageCache, int Size);
        virtual ~KairosAvatar();

        void set_avatar(const std::string& NewAvatar, const std::string& NewBackground);

    protected:
        void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const override;
        void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const override;
        void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const override;
        void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const override;
        void on_size_allocate(Gtk::Allocation& Allocation) override;
        void on_realize() override;
        void on_unrealize() override;
        bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

    private:
        Modules::ImageCacheModule& ImageCache;
        int Size;
        Glib::RefPtr<Gdk::Window> Window;

        std::string Avatar;
        Glib::RefPtr<Gdk::Pixbuf> AvatarPixbuf;
        std::future<Glib::RefPtr<Gdk::Pixbuf>> AvatarTask;

        std::string Background;
        Glib::RefPtr<Gdk::Pixbuf> BackgroundPixbuf;
        std::future<Glib::RefPtr<Gdk::Pixbuf>> BackgroundTask;

        Glib::Dispatcher Dispatcher;
    };
}
