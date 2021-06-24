#pragma once

#include "../utils/stringex/ExpressionEvaluator.h"
#include "../utils/SlotHolder.h"
#include "../web/epic/content/SdMeta.h"

#include <gtkmm.h>

namespace EGL3::Widgets {
    class SdTree {
    public:
        SdTree(Gtk::TreeView& TreeView);

        ~SdTree();

        void Initialize(const std::vector<Web::Epic::Content::SdMeta::Data>* Options);

        void SetOptions(const std::vector<std::string>& UniqueIds);

        void SetDefaultOptions();

        void SetAllOptions();

        void GetOptions(std::vector<std::string>& SelectedIds, std::vector<std::string>& InstallTags);

    private:
        Gtk::TreeView& TreeView;
        Gtk::CellRendererText RequiredRenderer;
        Glib::RefPtr<Gtk::TreeModelFilter> TreeFilter;
        Glib::RefPtr<Gtk::TreeStore> TreeStore;

        Utils::SlotHolder SlotTooltip;

        struct ModelColumns : public Gtk::TreeModel::ColumnRecord
        {
            ModelColumns()
            {
                add(Data);
                add(Name);
                add(Required);
                add(Selected);
                add(Selectable);
            }

            Gtk::TreeModelColumn<Web::Epic::Content::SdMeta::Data*> Data;
            Gtk::TreeModelColumn<Glib::ustring> Name;
            Gtk::TreeModelColumn<bool> Required;
            Gtk::TreeModelColumn<bool> Selected;
            Gtk::TreeModelColumn<bool> Selectable;
        };

        ModelColumns Columns;

        Utils::StringEx::ExpressionEvaluator DefaultEvaluator;

        std::vector<std::string> SelectedIds;
        std::vector<Web::Epic::Content::SdMeta::Data> OptionsData;
    };
}
