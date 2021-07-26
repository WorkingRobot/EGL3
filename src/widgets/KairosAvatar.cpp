#include "KairosAvatar.h"

#include "../modules/Friends/KairosMenu.h"

namespace EGL3::Widgets {
    using namespace Modules;

    KairosAvatar::KairosAvatar(ImageCacheModule& ImageCache, int Size) :
        Glib::ObjectBase("KairosAvatar"),
        Gtk::Widget(),
        ImageCache(ImageCache),
        Size(Size)
    {
        set_has_window(true);
        set_name("kairos-avatar");

        Dispatcher.connect([this]() {
            if (AvatarTask.valid() && AvatarTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                AvatarPixbuf = AvatarTask.get();
                if (get_mapped()) {
                    queue_draw();
                }
            }
            if (BackgroundTask.valid() && BackgroundTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                BackgroundPixbuf = BackgroundTask.get();
                if (get_mapped()) {
                    queue_draw();
                }
            }
        });
    }

    KairosAvatar::~KairosAvatar()
    {

    }

    void KairosAvatar::set_avatar(const std::string& NewAvatar, const std::string& NewBackground)
    {
        if (Avatar != NewAvatar) {
            Avatar = NewAvatar;
            AvatarTask = ImageCache.GetImageAsync(Friends::KairosMenuModule::GetKairosAvatarUrl(Avatar), Friends::KairosMenuModule::GetKairosAvatarUrl(Friends::KairosMenuModule::GetRandomKairosAvatar()), Size, Size, Dispatcher);
        }
        if (Background != NewBackground) {
            Background = NewBackground;
            BackgroundTask = ImageCache.GetImageAsync(Friends::KairosMenuModule::GetKairosBackgroundUrl(Background), Friends::KairosMenuModule::GetKairosBackgroundUrl(Friends::KairosMenuModule::GetRandomKairosBackground()), Size, Size, Dispatcher);
        }
    }

    void KairosAvatar::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
    {
        if (Size != -1) {
            minimum_width = natural_width = Size;
        }
    }

    void KairosAvatar::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
    {
        if (Size == -1) {
            minimum_width = natural_width = height;
        }
        else {
            minimum_width = natural_width = Size;
        }
    }

    void KairosAvatar::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
    {
        if (Size != -1) {
            minimum_height = natural_height = Size;
        }
    }

    void KairosAvatar::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
    {
        if (Size == -1) {
            minimum_height = natural_height = width;
        }
        else {
            minimum_height = natural_height = Size;
        }
    }

    void KairosAvatar::on_size_allocate(Gtk::Allocation& Allocation)
    {
        if (!Allocation.has_zero_area()) {
            if (Allocation.get_height() > Allocation.get_width()) {
                Allocation.set_width(Allocation.get_height());
                set_size_request(Allocation.get_width(), -1);
            }
            else if (Allocation.get_width() > Allocation.get_height()) {
                Allocation.set_height(Allocation.get_width());
                set_size_request(-1, Allocation.get_height());
            }
        }

        set_allocation(Allocation);

        if (Window)
        {
            Window->move_resize(Allocation.get_x(), Allocation.get_y(), Allocation.get_width(), Allocation.get_height());
        }
    }

    void KairosAvatar::on_realize()
    {
        //Do not call base class Gtk::Widget::on_realize().
        //It's intended only for widgets that set_has_window(false).

        set_realized();

        if (!Window)
        {
            //Create the GdkWindow:

            GdkWindowAttr attributes;
            memset(&attributes, 0, sizeof(attributes));

            Gtk::Allocation allocation = get_allocation();

            //Set initial position and size of the Gdk::Window:
            attributes.x = allocation.get_x();
            attributes.y = allocation.get_y();
            attributes.width = allocation.get_width();
            attributes.height = allocation.get_height();

            attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK;
            attributes.window_type = GDK_WINDOW_CHILD;
            attributes.wclass = GDK_INPUT_OUTPUT;

            Window = Gdk::Window::create(get_parent_window(), &attributes,
                GDK_WA_X | GDK_WA_Y);
            set_window(Window);

            //make the widget receive expose events
            Window->set_user_data(gobj());
        }
    }

    void KairosAvatar::on_unrealize()
    {
        Window.reset();

        //Call base class:
        Gtk::Widget::on_unrealize();
    }

    bool KairosAvatar::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
    {
        auto Allocation = get_allocation();
        double OutputSize = std::min(Allocation.get_width(), Allocation.get_height());
        if (Size != -1) {
            OutputSize = std::min(OutputSize, (double)Size);
        }

        auto StyleContext = get_style_context();
        StyleContext->render_background(cr, Allocation.get_x(), Allocation.get_y(), Allocation.get_width(), Allocation.get_height());

        if (BackgroundPixbuf) {
            cr->save();
            cr->scale(OutputSize / BackgroundPixbuf->get_width(), OutputSize / BackgroundPixbuf->get_width());
            Gdk::Cairo::set_source_pixbuf(cr, BackgroundPixbuf, 0, 0);
            cr->paint();
            cr->restore();
        }

        if (AvatarPixbuf) {
            cr->save();
            cr->scale(OutputSize / AvatarPixbuf->get_width(), OutputSize / AvatarPixbuf->get_width());
            Gdk::Cairo::set_source_pixbuf(cr, AvatarPixbuf, 0, 0);
            cr->paint();
            cr->restore();
        }

        return true;
    }
}
