#pragma once

#include <gtkmm.h>

namespace EGL3::Widgets {
    template<class RendA, class RendB>
    class CellRendererSplit : public Gtk::CellRenderer
    {
    public:
        CellRendererSplit(RendA&& RendererA, RendB&& RendererB) :
            Glib::ObjectBase(typeid(CellRendererSplit)),
            Gtk::CellRenderer(),
            RendererA(std::move(RendererA)),
            RendererB(std::move(RendererB))
        {
            
        }

        virtual ~CellRendererSplit()
        {

        }

        RendA& GetRendererA()
        {
            return RendererA;
        }

        RendB& GetRendererB()
        {
            return RendererB;
        }

    protected:
        void get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const override
        {
            RendererA.get_preferred_width(widget, minimum_width, natural_width);

            int minimum_widthB, natural_widthB;
            RendererB.get_preferred_width(widget, minimum_widthB, natural_widthB);

            minimum_width = std::max(minimum_width, minimum_widthB);
            natural_width = std::max(natural_width, natural_widthB);
        }

        void get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width, int& natural_width) const override
        {
            RendererA.get_preferred_width_for_height(widget, height, minimum_width, natural_width);

            int minimum_widthB, natural_widthB;
            RendererB.get_preferred_width_for_height(widget, height, minimum_widthB, natural_widthB);

            minimum_width = std::max(minimum_width, minimum_widthB);
            natural_width = std::max(natural_width, natural_widthB);
        }

        void get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const override
        {
            RendererA.get_preferred_height(widget, minimum_height, natural_height);

            int minimum_heightB, natural_heightB;
            RendererB.get_preferred_height(widget, minimum_heightB, natural_heightB);

            minimum_height += minimum_heightB;
            natural_height += natural_heightB;
        }

        void get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height, int& natural_height) const override
        {
            RendererA.get_preferred_height_for_width(widget, width, minimum_height, natural_height);

            int minimum_heightB, natural_heightB;
            RendererB.get_preferred_height_for_width(widget, width, minimum_heightB, natural_heightB);

            minimum_height += minimum_heightB;
            natural_height += natural_heightB;
        }

        virtual void connect_props() = 0;

        void render_vfunc(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) override
        {
            connect_props();

            RendererA.render(cr, widget,
                Gdk::Rectangle(background_area.get_x(), background_area.get_y(), background_area.get_width(), background_area.get_height() / 2),
                Gdk::Rectangle(cell_area.get_x(), cell_area.get_y(), cell_area.get_width(), cell_area.get_height() / 2),
                flags);

            RendererB.render(cr, widget,
                Gdk::Rectangle(background_area.get_x(), background_area.get_y() + background_area.get_height() / 2, background_area.get_width(), background_area.get_height() / 2),
                Gdk::Rectangle(cell_area.get_x(), cell_area.get_y() + cell_area.get_height() / 2, cell_area.get_width(), cell_area.get_height() / 2),
                flags);
        }

    private:
        RendA RendererA;
        RendB RendererB;
    };
}
