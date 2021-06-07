#pragma once

#include "CellRendererSplit.h"

namespace EGL3::Widgets {
    class CellRendererCenterText : public CellRendererSplit<Gtk::CellRendererText, Gtk::CellRendererText>
    {
    public:
        CellRendererCenterText() :
            Glib::ObjectBase("CellRendererCenterText"),
            CellRendererSplit({}, {}),
            prop_name(*this, "renderer-name"),
            prop_status(*this, "renderer-status")
        {
            
        }
        using CellRendererSplit::CellRendererSplit;

        virtual ~CellRendererCenterText()
        {

        }

        Glib::PropertyProxy<Glib::ustring> property_name() { return prop_name.get_proxy(); }
        Glib::PropertyProxy<Glib::ustring> property_status() { return prop_status.get_proxy(); }

    private:
        void connect_props() override
        {
            GetRendererA().property_markup() = prop_name;
            GetRendererB().property_text() = prop_status;
        }

        Glib::Property<Glib::ustring> prop_name;
        Glib::Property<Glib::ustring> prop_status;
    };
}
