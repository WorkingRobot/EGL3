#include "BaseModule.h"

#include <gtkmm.h>

#include "../utils/GladeBuilder.h"

namespace EGL3::Modules {
    class SidebarNotebookModule : public BaseModule {
    public:
        SidebarNotebookModule(Utils::GladeBuilder& Builder) : SelectedBtn(Builder.GetWidget<Gtk::Button>("PlayTabBtn")) {
            auto& Notebook = Builder.GetWidget<Gtk::Notebook>("EGL3Notebook");
            TabSetup<0>("PlayTabBtn", Notebook, Builder);
            TabSetup<1>("NewsTabBtn", Notebook, Builder);
            TabSetup<2>("FriendsTabBtn", Notebook, Builder);
            TabSetup<3>("SettingsTabBtn", Notebook, Builder);
            TabSetup<4>("AboutTabBtn", Notebook, Builder);
        }

    private:
        template<int TabIdx>
        void TabClicked(Gtk::Notebook& Notebook, Gtk::Button& TabBtn) {
            Notebook.set_current_page(TabIdx);
            SelectedBtn.get().set_sensitive(true);
            TabBtn.set_sensitive(false);
            SelectedBtn = TabBtn;
        }

        template<int TabIdx>
        void TabSetup(char* WidgetName, Gtk::Notebook& Notebook, Utils::GladeBuilder& Builder) {
            auto& TabBtn = Builder.GetWidget<Gtk::Button>(WidgetName);
            if (TabIdx == Notebook.get_current_page()) {
                TabBtn.set_sensitive(false);
                SelectedBtn = TabBtn;
            }
            TabBtn.signal_clicked().connect(sigc::bind(&SidebarNotebookModule::TabClicked<TabIdx>, this, std::ref(Notebook), std::ref(TabBtn)));
        }

        std::reference_wrapper<Gtk::Button> SelectedBtn;
    };
}