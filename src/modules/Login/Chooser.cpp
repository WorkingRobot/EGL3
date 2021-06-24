#include "Chooser.h"


namespace EGL3::Modules::Login {
    using namespace Web::Xmpp;

    ChooserModule::ChooserModule(ModuleList& Ctx) :
        Auth(Ctx.GetModule<AuthModule>()),
        Icon(Ctx.GetWidget<Gtk::Image>("AccountChooserIcon")),
        AccountList(Ctx.GetWidget<Gtk::TreeView>("AccountChooserTree"), Ctx.GetModule<ImageCacheModule>(), Ctx.GetModule<AuthModule>(), 64, 1.50)
    {
        Icon.set(Icon.get_pixbuf()->scale_simple(96, 96, Gdk::INTERP_BILINEAR));
    }
}