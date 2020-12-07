#pragma once

#include "BaseModule.h"

#include <gtkmm.h>

#include "../utils/GladeBuilder.h"

namespace EGL3::Modules {
    class SidebarNotebookModule : public BaseModule {
    public:
        SidebarNotebookModule(const Utils::GladeBuilder& Builder) : SelectedBtn(Builder.GetWidget<Gtk::Button>("NewsTabBtn")) {
            auto& Notebook = Builder.GetWidget<Gtk::Notebook>("EGL3Notebook");
            TabSetup<0>("NewsTabBtn", Notebook, Builder);
            TabSetup<1>("FriendsTabBtn", Notebook, Builder);
            TabSetup<2>("SettingsTabBtn", Notebook, Builder);
            TabSetup<3>("AboutTabBtn", Notebook, Builder);
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
        void TabSetup(char* WidgetName, Gtk::Notebook& Notebook, const Utils::GladeBuilder& Builder) {
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