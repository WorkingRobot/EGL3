#include "CellRendererKairosAvatar.h"

#include "../web/xmpp/PresenceKairosProfile.h"

namespace EGL3::Widgets {
    using namespace Web::Xmpp::Json;

    CellRendererKairosAvatar::CellRendererKairosAvatar(Gtk::TreeView& TreeView, Modules::ImageCacheModule& ImageCache, int Size) :
        Glib::ObjectBase("CellRendererKairosAvatar"),
        Gtk::CellRenderer(),
        TreeView(TreeView),
        ImageCache(ImageCache),
        Size(Size),
        prop_avatar(*this, "renderer-avatar"),
        prop_background(*this, "renderer-background")
    {
        ImageDispatcher.connect([this]() {
            this->TreeView.queue_draw();
        });
    }

    CellRendererKairosAvatar::~CellRendererKairosAvatar()
    {

    }

    void CellRendererKairosAvatar::get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const
    {
        if (Size != -1) {
            minimum_width = natural_width = Size;
        }
    }

    void CellRendererKairosAvatar::get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width, int& natural_width) const
    {
        if (Size == -1) {
            minimum_width = natural_width = height;
        }
        else {
            minimum_width = natural_width = Size;
        }
    }

    void CellRendererKairosAvatar::get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const
    {
        if (Size != -1) {
            minimum_height = natural_height = Size;
        }
    }

    void CellRendererKairosAvatar::get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height, int& natural_height) const
    {
        if (Size == -1) {
            minimum_height = natural_height = width;
        }
        else {
            minimum_height = natural_height = Size;
        }
    }

    void CellRendererKairosAvatar::render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags)
    {
        double OutputSize = std::min(cell_area.get_width(), cell_area.get_height());
        if (Size != -1) {
            OutputSize = std::min(OutputSize, (double)Size);
        }
        
        Glib::ustring Background = flags & Gtk::CellRendererState::CELL_RENDERER_SELECTED ?
            prop_background.get_value() :
            "[\"#364D63\",\"#21272C\",\"#000000\"]";
        auto BackgroundPixbuf = ImageCache.TryGetOrQueueImage(PresenceKairosProfile::GetKairosBackgroundUrl(Background), PresenceKairosProfile::GetKairosBackgroundUrl(PresenceKairosProfile::GetDefaultKairosBackground()), Size, Size, ImageDispatcher);
        if (BackgroundPixbuf) {
            cr->save();
            cr->translate(cell_area.get_x(), cell_area.get_y());
            cr->scale(OutputSize / BackgroundPixbuf->get_width(), OutputSize / BackgroundPixbuf->get_width());
            Gdk::Cairo::set_source_pixbuf(cr, BackgroundPixbuf, 0, 0);
            cr->paint();
            cr->restore();
        }

        auto Avatar = prop_avatar.get_value();
        auto AvatarPixbuf = ImageCache.TryGetOrQueueImage(PresenceKairosProfile::GetKairosAvatarUrl(Avatar), PresenceKairosProfile::GetKairosAvatarUrl(PresenceKairosProfile::GetDefaultKairosAvatar()), Size, Size, ImageDispatcher);
        if (AvatarPixbuf) {
            cr->save();
            cr->translate(cell_area.get_x(), cell_area.get_y());
            cr->scale(OutputSize / AvatarPixbuf->get_width(), OutputSize / AvatarPixbuf->get_width());
            Gdk::Cairo::set_source_pixbuf(cr, AvatarPixbuf, 0, 0);
            cr->paint();
            cr->restore();
        }
    }
}
